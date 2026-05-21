#pragma once

#define WM_API_UPDATE      (WM_USER + 2)
#define WM_SYMBOL_RESULTS  (WM_USER + 3)
#define WM_API_ERROR       (WM_USER + 4)
#define WM_ACCOUNT_SUMMARY (WM_USER + 5)
#define WM_PNL_UPDATE      (WM_USER + 6)
#define WM_ORDERS_UPDATE   (WM_USER + 7)
#define WM_DIAMONDS_UPDATE (WM_USER + 8)
#define WM_NEWS_RESULTS    (WM_USER + 9)

#include <algorithm>
#include "Contract.h"
#include "Order.h"
#include "OrderState.h"

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
    HWND hOrdersWnd = nullptr;
    HWND hDiamondsWnd = nullptr;
    std::mutex summaryMutex;
    std::map<std::string, std::string> summaryData; // tag -> value
    double dailyPnL = 0, unrealizedPnL = 0, realizedPnL = 0;

    HWND hSymbolSearchWnd = nullptr; // separate target for symbol results
    HWND hNewsWnd = nullptr; // separate target for news headlines
    std::mutex symbolMutex;
    std::vector<std::string> symbolResults;
    std::mutex newsMutex;
    std::vector<std::string> newsResults;
    std::atomic<bool> newsRequestActive{false};

    std::mutex apiUpdateMutex;
    std::vector<HWND> apiUpdateWindows;
    HWND hErrorWnd = nullptr;

public:
    struct OrderInfo {
        int orderId;
        std::string symbol;
        std::string exchange;
        double price;
        double totalAmount;
        std::string status;
        std::string time;
        long long timestamp; // for sorting
    };

    struct PositionInfo {
        std::string symbol;
        Decimal shares;
        double avgCost;
        double dailyPnL;
        double marketValue;
        double fiftyTwoWeekChange;
        double marketCap;
    };

    struct WatchlistInfo {
        std::string symbol;
        double price;
        double change;
        double percentChange;
        double marketCap;
    };

private:
    std::mutex ordersMutex;
    std::map<int, OrderInfo> ordersMap;
    
    std::mutex portfolioMutex;
    std::map<std::string, PositionInfo> portfolioMap;

    std::mutex watchlistMutex;
    std::map<std::string, WatchlistInfo> watchlistMap;

    bool notifyConnected = false;

    void processMessages() {
        while (isThreadRunning && client->isConnected()) {
            signal.waitForSignal();
            reader->processMsgs();
        }
        isThreadRunning = false;
    }

public:
    TradingAPI() : isThreadRunning(false), accountId("") {
        client = new EClientSocket(this, &signal);
    }

    ~TradingAPI() {
        disconnect();
        delete client;
    }
    
    void addApiUpdateWindow(HWND hWnd) {
        if (!hWnd || !IsWindow(hWnd)) return;
        std::lock_guard<std::mutex> lock(apiUpdateMutex);
        if (std::find(apiUpdateWindows.begin(), apiUpdateWindows.end(), hWnd) == apiUpdateWindows.end())
            apiUpdateWindows.push_back(hWnd);
    }

    void removeApiUpdateWindow(HWND hWnd) {
        std::lock_guard<std::mutex> lock(apiUpdateMutex);
        apiUpdateWindows.erase(std::remove(apiUpdateWindows.begin(), apiUpdateWindows.end(), hWnd), apiUpdateWindows.end());
    }

    void notifyApiUpdate() {
        if (notifyConnected == (isMarketDataConnected() && isTradingConnected())) return;
        notifyConnected = !notifyConnected;
        std::vector<HWND> validWindows;
        {
            std::lock_guard<std::mutex> lock(apiUpdateMutex);
             // avoid flooding on unchanged state
            for (HWND hWnd : apiUpdateWindows) {
                if (hWnd && IsWindow(hWnd)) {
                    validWindows.push_back(hWnd);
                }
            }
            if (validWindows.size() != apiUpdateWindows.size()) {
                apiUpdateWindows.swap(validWindows);
                validWindows.clear();
                for (HWND hWnd : apiUpdateWindows) {
                    if (hWnd && IsWindow(hWnd))
                        validWindows.push_back(hWnd);
                }
            }
        }

        for (HWND hWnd : validWindows) {
            PostMessage(hWnd, WM_API_UPDATE, 0, 0);
        }
    }

    void setApiErrorWindow(HWND hWnd) {
        hErrorWnd = hWnd;
    }

    void clearApiErrorWindow(HWND hWnd) {
        if (hErrorWnd == hWnd) {
            hErrorWnd = nullptr;
        }
    }

    void notifyApiError(const std::string& errorString) {
        if (!hErrorWnd || !IsWindow(hErrorWnd)) {
            hErrorWnd = nullptr;
            return;
        }
        std::string* msg = new std::string(errorString);
        PostMessage(hErrorWnd, WM_API_ERROR, 0, (LPARAM)msg);
    }

    bool connect() {
        if (apiThread.joinable()) {
            isThreadRunning = false;
            signal.issueSignal();
            apiThread.join();
            reader.reset();
        }

        if (client->isConnected()) {
            client->eDisconnect();
        }

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

    void connectAck() override {
        notifyApiUpdate();
    }

    void connectionClosed() override {
        if (tradingConnected.exchange(false)) {
            PlaySound_Async(203);
            LogDebug("Trading connection closed");
        }
        marketDataConnected = false;
        marketDataSoundPlayed = false;
        notifyApiUpdate();
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
                notifyApiUpdate();
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
                notifyApiUpdate();
                break;
            }

            // ── Suppress known noise ──────────────────────────────────────────
            case 2108: case 2119: case 2100:
                break;

            default:
                LogDebug("API Error [" + std::to_string(errorCode) + "]: " + errorString);
                notifyApiError("API Error [" + std::to_string(errorCode) + "]: " + errorString);
                break;
        }
    }

    void setCoinWindow(HWND hWnd) {
        hCoinWnd = hWnd;
        if (!hWnd || !client->isConnected()) return;
        
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
            LogDebug("Requesting account summary and PnL for: " + acc);
            client->reqPnL(9013, acc, "");
        }
    }

    void setOrdersWindow(HWND hWnd) {
        hOrdersWnd = hWnd;
    }

    void setDiamondsWindow(HWND hWnd) {
        hDiamondsWnd = hWnd;
    }

    void requestWatchlistData(const std::string& bookName) {
        if (!client->isConnected()) return;
        // We need to request market data for each symbol in the book
        // This is a simplified version; in a real app we'd load the book from registry first
        LogDebug("Requesting watchlist data for book: " + bookName);
        // The actual implementation of loading the book and requesting data 
        // will be handled in the UI or via a helper.
    }

public:
    std::mutex& getOrdersMutex() { return ordersMutex; }
    std::map<int, OrderInfo>& getOrdersMap() { return ordersMap; }

    std::mutex& getPortfolioMutex() { return portfolioMutex; }
    std::map<std::string, TradingAPI::PositionInfo>& getPortfolioMap() { return portfolioMap; }

    std::mutex& getWatchlistMutex() { return watchlistMutex; }
    std::map<std::string, TradingAPI::WatchlistInfo>& getWatchlistMap() { return watchlistMap; }

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
        notifyApiUpdate();
    }

    void nextValidId(int orderId) override {
        bool wasDisconnected = !tradingConnected.exchange(true);
        if (wasDisconnected) {
            PlaySound_Async(204); // trading_connection_reestablished
            LogDebug("Trading connected, next order ID: " + std::to_string(orderId));
        }
        notifyApiUpdate();
    }

    void position(const std::string& account, const Contract& contract, Decimal shares, double avgCost) {
        std::lock_guard<std::mutex> lock(portfolioMutex);
        
        PositionInfo& info = portfolioMap[contract.symbol];
        info.symbol = contract.symbol;
        info.shares = shares;
        info.avgCost = avgCost;
        
        if (hDiamondsWnd) PostMessage(hDiamondsWnd, WM_DIAMONDS_UPDATE, 0, 0);
    }

    void positionEnd() {
        LogDebug("Position update ended");
    }

    void openOrder(int orderId, const Contract& contract, const Order& order, const OrderState& orderState) override {
        std::lock_guard<std::mutex> lock(ordersMutex);
        
        OrderInfo& info = ordersMap[orderId];
        info.orderId = orderId;
        info.symbol = contract.symbol;
        info.exchange = contract.primaryExchange;
        info.totalAmount = (double)order.totalQuantity;
        info.price = order.lmtPrice;
        info.status = orderState.status;
        
        time_t now = time(0);
        char buf[32];
        strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
        info.time = buf;
        info.timestamp = (long long)now;

        if (hOrdersWnd) PostMessage(hOrdersWnd, WM_ORDERS_UPDATE, 0, 0);
    }

    void orderStatus(int orderId, const std::string& status, Decimal filled, 
                     Decimal remaining, double avgFillPrice, long long permId, int parentId,
                     double lastFillPrice, int clientId, const std::string& whyHeld, double mktCapPrice) override {
        std::lock_guard<std::mutex> lock(ordersMutex);
        
        if (ordersMap.count(orderId)) {
            ordersMap[orderId].status = status;
            
            time_t now = time(0);
            char buf[32];
            strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
            ordersMap[orderId].time = buf;
            ordersMap[orderId].timestamp = (long long)now;
        }
        
        if (hOrdersWnd) PostMessage(hOrdersWnd, WM_ORDERS_UPDATE, 0, 0);
    }

    void setSymbolSearchWindow(HWND hWnd) { hSymbolSearchWnd = hWnd; }
    void setNewsWindow(HWND hWnd) { hNewsWnd = hWnd; }

    void searchSymbols(const std::string& pattern) {
        if (!client->isConnected()) return;
        client->reqMatchingSymbols(9001, pattern);
    }

    std::vector<std::string> getSymbolResults() {
        std::lock_guard<std::mutex> lock(symbolMutex);
        return symbolResults;
    }

    std::vector<std::string> getNewsResults() {
        std::lock_guard<std::mutex> lock(newsMutex);
        return newsResults;
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

    void reqNewsForSymbol(int conId, const std::string& symbol) {
        if (!client->isConnected()) return;

        {
            std::lock_guard<std::mutex> lock(newsMutex);
            newsResults.clear();
        }

        Contract contract;
        contract.conId = conId;
        contract.symbol = symbol;
        contract.secType = "STK";
        contract.exchange = "SMART";
        contract.currency = "USD";

        if (newsRequestActive.exchange(true)) {
            client->cancelMktData(9002);
        }

        // Generic tick 292 is for news.
        // We use "mdoff" to disable standard market data and only get news.
        client->reqMktData(9002, contract, "mdoff,292", false, false, {});
    }

    void tickNews(int reqId, time_t timeStamp, const std::string& providerCode,
                  const std::string& articleId, const std::string& headline,
                  const std::string& extraData) override {
        std::string newsLine = "[" + providerCode + "] " + headline;
        if (!extraData.empty()) {
            newsLine += " - ";
            newsLine += extraData;
        }

        {
            std::lock_guard<std::mutex> lock(newsMutex);
            newsResults.push_back(newsLine);
        }

        if (hNewsWnd)
            PostMessage(hNewsWnd, WM_NEWS_RESULTS, 0, 0);
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
    #define openOrder openOrder_ignored
    #define orderStatus orderStatus_ignored
    #define position position_ignored
    #define positionEnd positionEnd_ignored
    #define tickNews tickNews_ignored

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
    #undef openOrder_ignored
    #undef orderStatus_ignored
    #undef position_ignored
    #undef positionEnd_ignored
    #undef tickNews_ignored
} api;