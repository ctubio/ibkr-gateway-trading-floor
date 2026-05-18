#pragma once

#define WM_API_UPDATE     (WM_USER + 2)
#define WM_SYMBOL_RESULTS (WM_USER + 3)
#define WM_API_ERROR      (WM_USER + 4)
#define WM_ACCOUNT_SUMMARY (WM_USER + 5)
#define WM_PNL_UPDATE      (WM_USER + 6)

#define TIMER_WATCHDOG 1

#define ID_M_SYMBOLS    1001
#define ID_M_COINS      1002
#define ID_M_DIAMONDS   1003
#define ID_M_SETTINGS   1004
#define ID_M_NEWS       1005
#define ID_M_TICKER     1006
#define ID_M_TIMESALES  1007
#define ID_M_LEVELS     1008
#define ID_M_ORDERS     1009

#define ID_M_CONNECT    3001
#define ID_M_DISCONNECT 3002
#define ID_M_EXIT       3003

bool shouldBeConnected = true;

class TradingAPI : public EWrapper {
private:
    HWND hTargetWnd = nullptr;
    
    EClientSocket* client;
    EReaderOSSignal signal;
    std::unique_ptr<EReader> reader;
    std::thread apiThread;
    std::atomic<bool> isThreadRunning;

    std::mutex dataMutex;

    std::string accountId;
    
    std::atomic<bool> marketDataConnected{false};
    std::atomic<bool> tradingConnected{false};
    std::atomic<bool> marketDataSoundPlayed{false}; // prevent duplicate sounds

    HWND hCoinWnd = nullptr;
    std::mutex summaryMutex;
    std::map<std::string, std::string> summaryData; // tag -> value
    double dailyPnL = 0, unrealizedPnL = 0, realizedPnL = 0;

    HWND hSymbolSearchWnd = nullptr; // separate target for symbol results
    std::mutex symbolMutex;
    std::vector<std::string> symbolResults;

    void processMessages() {
        while (isThreadRunning && client->isConnected()) {
            signal.waitForSignal();
            reader->processMsgs();
        }
    }

public:
    TradingAPI() : isThreadRunning(false), accountId("") {
        client = new EClientSocket(this, &signal);
    }

    ~TradingAPI() {
        disconnect();
        delete client;
    }
    
    void setWindowHandle(HWND hWnd) { hTargetWnd = hWnd; }

    bool connect() {
        if (client->eConnect("127.0.0.1", 4001, 0)) {
            reader = std::make_unique<EReader>(client, &signal);
            reader->start();
            isThreadRunning = true;
            apiThread = std::thread(&TradingAPI::processMessages, this);
            return true;
        }
        return false;
    }

    void disconnect() {
        isThreadRunning = false;
        if (client->isConnected())
            client->eDisconnect();
        if (apiThread.joinable()) {
            signal.issueSignal();
            apiThread.join();
        }
        reader.reset();
        
        if (tradingConnected.exchange(false)) {
            PlaySound_Async(203); // trading_connection_lost
            LogDebug("Trading disconnected by user");
        }
        if (marketDataConnected.exchange(false)) {
            PlaySound_Async(201); // md_connection_lost
            LogDebug("Market data disconnected by user");
        }
        marketDataSoundPlayed = false;
    }

    bool isConnected() const { return client && client->isConnected(); }

    // ── Callbacks you actually care about ────────────────────────────────────
    void connectAck() override {
        // Safe empty implementation override, but triggers a UI refresh notice
        if (hTargetWnd) {
            PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
        }
    }

    void connectionClosed() override {
        if (tradingConnected.exchange(false)) {
            PlaySound_Async(203); // trading_connection_lost
            LogDebug("Trading connection closed");
        }
        marketDataConnected = false;
        marketDataSoundPlayed = false;
        if (hTargetWnd) PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
    }
    bool isMarketDataConnected() const { return marketDataConnected; }
    bool isTradingConnected()    const { return tradingConnected; }

    void error(int id, time_t errorTime, int errorCode,
            const std::string& errorString,
            const std::string& advancedOrderRejectJson) override {
        switch (errorCode) {
            case 502: // Cannot connect
                break;

            // ── Market data OK ────────────────────────────────────────────────
            case 2104: case 2106: case 2107: case 2158: {
                bool wasDisconnected = !marketDataConnected.exchange(true);
                if (wasDisconnected && !marketDataSoundPlayed.exchange(true)) {
                    PlaySound_Async(202); // md_connection_reestablished
                    LogDebug("Market data connected: " + errorString);
                }
                if (hTargetWnd) PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
                break;
            }

            // ── Market data broken ────────────────────────────────────────────
            case 2103: case 2105: case 2157: {
                bool wasConnected = marketDataConnected.exchange(false);
                marketDataSoundPlayed = false; // reset so next reconnect plays sound
                if (wasConnected) {
                    PlaySound_Async(201); // md_connection_lost
                    LogDebug("Market data lost: " + errorString);
                }
                if (hTargetWnd) PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
                break;
            }

            // ── Suppress known noise ──────────────────────────────────────────
            case 2108: case 2119: case 2100:
                break;

            default:
                LogDebug("API Error [" + std::to_string(errorCode) + "]: " + errorString);
                if (hTargetWnd) {
                    std::string* msg = new std::string(
                        "API Error [" + std::to_string(errorCode) + "]: " + errorString
                    );
                    PostMessage(hTargetWnd, WM_API_ERROR, 0, (LPARAM)msg);
                }
                break;
        }
    }

    void setCoinWindow(HWND hWnd) {
        hCoinWnd = hWnd;
        if (!hWnd || !client->isConnected()) return;
        
        LogDebug("Requesting account summary...");
        client->reqAccountSummary(9010, "All",
            "NetLiquidation,TotalCashValue,BuyingPower,"
            "AvailableFunds,ExcessLiquidity,GrossPositionValue,"
            "MaintMarginReq,InitMarginReq");
        client->reqAccountSummary(9011, "All", "$LEDGER:ALL");
        
        std::string acc;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            acc = accountId;
        }
        if (!acc.empty()) {
            LogDebug("Requesting PnL for: " + acc);
            client->reqPnL(9013, acc, "");
        } else {
            LogDebug("setCoinWindow: accountId empty, PnL not requested");
        }
    }

    void unsetCoinWindow() {
        hCoinWnd = nullptr;
        if (!client->isConnected()) return; // ← guard
        client->cancelAccountSummary(9010);
        client->cancelAccountSummary(9011);
        client->cancelAccountSummary(9012);
        client->cancelPnL(9013);
    }

    std::map<std::string, std::string> getAccountSummary() {
        std::lock_guard<std::mutex> lock(summaryMutex);
        return summaryData;
    }

    double getDailyPnL()      const { return dailyPnL; }
    double getUnrealizedPnL() const { return unrealizedPnL; }
    double getRealizedPnL()   const { return realizedPnL; }

    void accountSummary(int reqId, const std::string& account,
                    const std::string& tag, const std::string& value,
                    const std::string& currency) override {
        std::lock_guard<std::mutex> lock(summaryMutex);
        std::string key = tag;
        if (!currency.empty()) {
            key = currency + "_" + tag;
        }
        summaryData[key] = value;
        
        // Also store the base currency version if it's not already there
        // This helps if the UI expects just "NetLiquidation" but the API returns "EUR_NetLiquidation"
        if (!currency.empty()) {
            summaryData[tag] = value; 
        }

        if (hCoinWnd) PostMessage(hCoinWnd, WM_ACCOUNT_SUMMARY, 0, 0);
    }

    void accountSummaryEnd(int reqId) override {}

    void pnl(int reqId, double daily, double unrealized, double realized) override {
        dailyPnL      = daily;
        unrealizedPnL = unrealized;
        realizedPnL   = realized;
        if (hCoinWnd) PostMessage(hCoinWnd, WM_PNL_UPDATE, 0, 0);
    }

    std::string getAccountNumber() {
        std::lock_guard<std::mutex> lock(dataMutex);
        return accountId;
    }

    void managedAccounts(const std::string& accountsList) override {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto comma = accountsList.find(',');
        accountId = (comma != std::string::npos) ? accountsList.substr(0, comma) : accountsList;
        // don't play sound here — wait for nextValidId
        if (hTargetWnd) PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
    }

    void nextValidId(int orderId) override {
        bool wasDisconnected = !tradingConnected.exchange(true);
        if (wasDisconnected) {
            PlaySound_Async(204); // trading_connection_reestablished
            LogDebug("Trading connected, next order ID: " + std::to_string(orderId));
        }
        if (hTargetWnd) PostMessage(hTargetWnd, WM_API_UPDATE, 0, 0);
    }


    void setSymbolSearchWindow(HWND hWnd) { hSymbolSearchWnd = hWnd; }

    void searchSymbols(const std::string& pattern) {
        if (!client->isConnected()) return;
        client->reqMatchingSymbols(9001, pattern);
    }

    std::vector<std::string> getSymbolResults() {
        std::lock_guard<std::mutex> lock(symbolMutex);
        return symbolResults;
    }

    void symbolSamples(int reqId, const std::vector<ContractDescription>& contractDescriptions) override {
        std::lock_guard<std::mutex> lock(symbolMutex);
        symbolResults.clear();
        for (const auto& cd : contractDescriptions) {
            if (cd.contract.secType == "STK" || cd.contract.secType == "ETF") {
                std::string entry = std::to_string(cd.contract.conId) + "." + cd.contract.symbol;
                if (!cd.contract.primaryExchange.empty())
                    entry += "." + cd.contract.primaryExchange;
                symbolResults.push_back(entry);
            }
        }
        if (hSymbolSearchWnd)
            PostMessage(hSymbolSearchWnd, WM_SYMBOL_RESULTS, 0, 0);
    }
    
    #define managedAccounts managedAccounts_ignored
    #define error error_ignored
    #define symbolSamples symbolSamples_ignored
    #define connectAck connectAck_ignored
    #define connectionClosed connectionClosed_ignored
    #define nextValidId nextValidId_ignored
    #define accountSummary accountSummary_ignored
    #define accountSummaryEnd accountSummaryEnd_ignored
    #define pnl pnl_ignored

    #define EWRAPPER_VIRTUAL_IMPL {}
    #include "EWrapper_prototypes.h"
    #undef EWRAPPER_VIRTUAL_IMPL

    #undef managedAccounts_ignored
    #undef error_ignored
    #undef symbolSamples_ignored
    #undef connectAck_ignored
    #undef connectionClosed_ignored
    #undef nextValidId_ignored 
    #undef accountSummary_ignored
    #undef accountSummaryEnd_ignored
    #undef pnl_ignored
} api;