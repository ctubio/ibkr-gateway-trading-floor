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
public:
    // ── Public data types (declared first so private members can reference them) ──

    struct OrderInfo {
        int         orderId     = 0;
        std::string symbol;
        std::string exchange;
        std::string action;      // BUY / SELL
        std::string orderType;   // LMT / MKT / STP …
        double      price       = 0.0;   // limit price (0 for MKT)
        double      auxPrice    = 0.0;   // stop price when relevant
        double      totalQty    = 0.0;   // original quantity
        double      filledQty   = 0.0;   // cumulative filled
        double      avgFillPx   = 0.0;   // average fill price
        std::string status;
        std::string time;        // display string
        long long   timestamp   = 0;     // unix seconds, for sorting
    };

    struct PositionInfo {
        std::string symbol;
        Decimal     shares;
        double      avgCost            = 0.0;
        double      dailyPnL           = 0.0;
        double      marketValue        = 0.0;
        double      fiftyTwoWeekChange = 0.0;
        double      marketCap          = 0.0;
        
        //double price         = 0.0;
        //double change        = 0.0;
        //double percentChange = 0.0;
    };

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
    std::atomic<bool> marketDataSoundPlayed{false};

    HWND hCoinWnd     = nullptr;
    HWND hOrdersWnd   = nullptr;
    HWND hDiamondsWnd = nullptr;

    std::mutex summaryMutex;
    std::map<std::string, std::string> summaryData;
    double dailyPnL = 0, unrealizedPnL = 0, realizedPnL = 0;

    HWND hSymbolSearchWnd = nullptr;
    HWND hNewsWnd         = nullptr;
    std::mutex symbolMutex;
    std::vector<std::string> symbolResults;
    std::mutex newsMutex;
    std::vector<std::string> newsResults;
    std::atomic<bool> newsRequestActive{false};

    std::mutex apiUpdateMutex;
    std::vector<HWND> apiUpdateWindows;
    HWND hErrorWnd = nullptr;

    bool notifyConnected = false;

    // ── Orders ────────────────────────────────────────────────────────────────
    std::mutex ordersMutex;
    std::map<int, OrderInfo> ordersMap;

    // ── Portfolio  ─────────────────────────────────────────────────
    std::mutex portfolioMutex;
    std::map<std::string, PositionInfo> portfolioMap;


    // ─────────────────────────────────────────────────────────────────────────

    void processMessages() {
        while (isThreadRunning && client->isConnected()) {
            signal.waitForSignal();
            reader->processMsgs();
        }
        isThreadRunning = false;
    }

    // Build a human-readable time string from a time_t.
    static std::string FormatTime(time_t t) {
        char buf[32] = {};
        strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&t));
        return buf;
    }

    void postOrdersUpdate() {
        if (hOrdersWnd && IsWindow(hOrdersWnd))
            PostMessage(hOrdersWnd, WM_ORDERS_UPDATE, 0, 0);
    }

public:
    TradingAPI() : isThreadRunning(false), accountId("") {
        client = new EClientSocket(this, &signal);
    }

    ~TradingAPI() {
        disconnect();
        delete client;
    }

    // ── API update broadcast ──────────────────────────────────────────────────

    void addApiUpdateWindow(HWND hWnd) {
        if (!hWnd || !IsWindow(hWnd)) return;
        std::lock_guard<std::mutex> lock(apiUpdateMutex);
        if (std::find(apiUpdateWindows.begin(), apiUpdateWindows.end(), hWnd) == apiUpdateWindows.end())
            apiUpdateWindows.push_back(hWnd);
    }

    void removeApiUpdateWindow(HWND hWnd) {
        std::lock_guard<std::mutex> lock(apiUpdateMutex);
        apiUpdateWindows.erase(
            std::remove(apiUpdateWindows.begin(), apiUpdateWindows.end(), hWnd),
            apiUpdateWindows.end());
    }

    void notifyApiUpdate() {
        if (notifyConnected == (isMarketDataConnected() && isTradingConnected())) return;
        notifyConnected = !notifyConnected;
        std::vector<HWND> windows;
        {
            std::lock_guard<std::mutex> lock(apiUpdateMutex);
            for (HWND h : apiUpdateWindows)
                if (h && IsWindow(h)) windows.push_back(h);
            apiUpdateWindows = windows;
        }
        for (HWND h : windows)
            PostMessage(h, WM_API_UPDATE, 0, 0);
    }

    void setApiErrorWindow(HWND hWnd)   { hErrorWnd = hWnd; }
    void clearApiErrorWindow(HWND hWnd) { if (hErrorWnd == hWnd) hErrorWnd = nullptr; }

    void notifyApiError(const std::string& errorString) {
        if (!hErrorWnd || !IsWindow(hErrorWnd)) { hErrorWnd = nullptr; return; }
        PostMessage(hErrorWnd, WM_API_ERROR, 0, (LPARAM)(new std::string(errorString)));
    }

    // ── Connection ────────────────────────────────────────────────────────────

    bool connect() {
        if (apiThread.joinable()) {
            isThreadRunning = false;
            signal.issueSignal();
            apiThread.join();
            reader.reset();
        }
        if (client->isConnected())
            client->eDisconnect();

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
            PlaySound_Async(203);
            LogDebug("Trading disconnected by user");
        }
        if (marketDataConnected.exchange(false)) {
            PlaySound_Async(201);
            LogDebug("Market data disconnected by user");
        }
        marketDataSoundPlayed = false;
    }

    bool isConnected()           const { return client && client->isConnected(); }
    bool isMarketDataConnected() const { return marketDataConnected; }
    bool isTradingConnected()    const { return tradingConnected; }

    // ── EWrapper: connection events ───────────────────────────────────────────

    void connectAck() override {
        notifyApiUpdate();
    }

    void connectionClosed() override {
        if (tradingConnected.exchange(false)) {
            PlaySound_Async(203);
            LogDebug("Trading connection closed");
        }
        marketDataConnected  = false;
        marketDataSoundPlayed = false;
        notifyApiUpdate();
    }

    void managedAccounts(const std::string& accountsList) override {
        std::lock_guard<std::mutex> lock(dataMutex);
        auto comma = accountsList.find(',');
        accountId = (comma != std::string::npos) ? accountsList.substr(0, comma) : accountsList;
        notifyApiUpdate();
    }

    void nextValidId(int orderId) override {
        bool wasDisconnected = !tradingConnected.exchange(true);
        if (wasDisconnected) {
            PlaySound_Async(204);
            LogDebug("Trading connected, next order ID: " + std::to_string(orderId));

            // Clear stale orders and re-fetch on every fresh connection.
            {
                std::lock_guard<std::mutex> lock(ordersMutex);
                ordersMap.clear();
            }
            client->reqOpenOrders();
            client->reqCompletedOrders(false);
        }
        notifyApiUpdate();
    }

    // ── EWrapper: errors ──────────────────────────────────────────────────────

    void error(int id, time_t errorTime, int errorCode,
               const std::string& errorString,
               const std::string& advancedOrderRejectJson) override {
        switch (errorCode) {
            case 502: break; // Cannot connect — expected noise

            case 2104: case 2106: case 2107: case 2158: {
                bool wasDisconnected = !marketDataConnected.exchange(true);
                if (wasDisconnected && !marketDataSoundPlayed.exchange(true)) {
                    PlaySound_Async(202);
                    LogDebug("Market data connected: " + errorString);
                }
                notifyApiUpdate();
                break;
            }
            case 2103: case 2105: case 2157: {
                bool wasConnected = marketDataConnected.exchange(false);
                marketDataSoundPlayed = false;
                if (wasConnected) {
                    PlaySound_Async(201);
                    LogDebug("Market data lost: " + errorString);
                }
                notifyApiUpdate();
                break;
            }
            case 2108: case 2119: case 2100: break; // known noise

            default:
                LogDebug("API Error [" + std::to_string(errorCode) + "]: " + errorString);
                notifyApiError("API Error [" + std::to_string(errorCode) + "]: " + errorString);
                break;
        }
    }

    // ── Orders ────────────────────────────────────────────────────────────────

    // Register the orders window. Triggers a fresh fetch if already connected.
    void setOrdersWindow(HWND hWnd) {
        hOrdersWnd = hWnd;
        if (!hWnd || !client->isConnected()) return;
        {
            std::lock_guard<std::mutex> lock(ordersMutex);
            ordersMap.clear();
        }
        // Snapshot of all currently open orders (any client session).
        client->reqOpenOrders();
        client->reqCompletedOrders(false);
    }

    void unsetOrdersWindow() {
        hOrdersWnd = nullptr;
    }

    // Returns a sorted snapshot safe to read from the UI thread.
    // Sort: Submitted/Pending → Partially Filled → Filled/Cancelled; newest first within group.
    std::vector<OrderInfo> getOrdersSorted() {
        std::vector<OrderInfo> out;
        {
            std::lock_guard<std::mutex> lock(ordersMutex);
            out.reserve(ordersMap.size());
            for (auto const& [id, info] : ordersMap)
                out.push_back(info);
        }

        auto priority = [](const std::string& s) -> int {
            if (s == "Submitted" || s == "PreSubmitted" || s == "Pending")  return 0;
            if (s == "Partially Filled")                                     return 1;
            if (s == "Filled" || s == "Cancelled" || s == "Inactive")       return 2;
            return 3;
        };

        std::sort(out.begin(), out.end(), [&](const OrderInfo& a, const OrderInfo& b) {
            int pa = priority(a.status), pb = priority(b.status);
            if (pa != pb) return pa < pb;
            return a.timestamp > b.timestamp; // newest first within group
        });
        return out;
    }

    // ── EWrapper: order callbacks ─────────────────────────────────────────────

    void openOrder(int orderId, const Contract& contract,
                   const Order& order, const OrderState& orderState) override {
        time_t now = time(nullptr);
        std::lock_guard<std::mutex> lock(ordersMutex);

        OrderInfo& info   = ordersMap[orderId];
        info.orderId      = orderId;
        info.symbol       = contract.symbol;
        info.exchange     = contract.primaryExchange.empty()
                                ? contract.exchange
                                : contract.primaryExchange;
        info.action       = order.action;
        info.orderType    = order.orderType;
        info.price        = order.lmtPrice;
        info.auxPrice     = order.auxPrice;
        info.totalQty     = DecimalFunctions::decimalToDouble(order.totalQuantity);
        info.status       = orderState.status;

        // Use completedTime when available (filled/cancelled orders coming from history),
        // otherwise keep the existing timestamp if we already have one, else use now.
        if (!orderState.completedTime.empty()) {
            // completedTime is "YYYYMMDD HH:MM:SS"
            struct tm t = {};
            sscanf(orderState.completedTime.c_str(),
                   "%4d%2d%2d %2d:%2d:%2d",
                   &t.tm_year, &t.tm_mon, &t.tm_mday,
                   &t.tm_hour, &t.tm_min, &t.tm_sec);
            t.tm_year -= 1900;
            t.tm_mon  -= 1;
            time_t ct = mktime(&t);
            if (ct > 0) { info.timestamp = (long long)ct; info.time = FormatTime(ct); }
        }
        if (info.timestamp == 0) {
            info.timestamp = (long long)now;
            info.time      = FormatTime(now);
        }
        // Note: do NOT post here — wait for openOrderEnd to do a single bulk update.
    }

    void openOrderEnd() override {
        // All open orders have been delivered — do one update.
        postOrdersUpdate();
    }

    void completedOrder(const Contract& contract, const Order& order,
                        const OrderState& orderState) override {
        time_t now = time(nullptr);
        std::lock_guard<std::mutex> lock(ordersMutex);

        // Skip if we already have a live entry for this order (open orders take priority).
        auto it = ordersMap.find(order.orderId);
        if (it != ordersMap.end() && !it->second.status.empty())
            return;

        OrderInfo& info = ordersMap[order.orderId];
        info.orderId   = order.orderId;
        info.symbol    = contract.symbol;
        info.exchange  = contract.primaryExchange.empty()
                             ? contract.exchange
                             : contract.primaryExchange;
        info.action    = order.action;
        info.orderType = order.orderType;
        info.price     = order.lmtPrice;
        info.auxPrice  = order.auxPrice;
        info.totalQty  = DecimalFunctions::decimalToDouble(order.totalQuantity);
        info.status    = orderState.status;

        // completedTime is reliably populated for completed orders: "YYYYMMDD HH:MM:SS"
        if (!orderState.completedTime.empty()) {
            struct tm t = {};
            sscanf(orderState.completedTime.c_str(),
                   "%4d%2d%2d %2d:%2d:%2d",
                   &t.tm_year, &t.tm_mon, &t.tm_mday,
                   &t.tm_hour, &t.tm_min, &t.tm_sec);
            t.tm_year -= 1900;
            t.tm_mon  -= 1;
            time_t ct = mktime(&t);
            if (ct > 0) { info.timestamp = (long long)ct; info.time = FormatTime(ct); }
        }
        if (info.timestamp == 0) {
            info.timestamp = (long long)now;
            info.time      = FormatTime(now);
        }
        // Wait for completedOrdersEnd for the bulk UI update.
    }

    void completedOrdersEnd() override {
        postOrdersUpdate();
    }

    void orderStatus(int orderId, const std::string& status,
                     Decimal filled, Decimal remaining,
                     double avgFillPrice, long long permId, int parentId,
                     double lastFillPrice, int clientId,
                     const std::string& whyHeld, double mktCapPrice) override {
        time_t now = time(nullptr);
        {
            std::lock_guard<std::mutex> lock(ordersMutex);

            // Create a stub if openOrder hasn't arrived yet (can happen on reconnect).
            OrderInfo& info = ordersMap[orderId];
            if (info.orderId == 0) {
                info.orderId   = orderId;
                info.timestamp = (long long)now;
                info.time      = FormatTime(now);
            }
            info.status     = status;
            info.filledQty  = DecimalFunctions::decimalToDouble(filled);
            info.avgFillPx  = avgFillPrice;

            // Refresh timestamp on meaningful state changes.
            if (status == "Filled" || status == "Cancelled" ||
                status == "Submitted" || status == "PreSubmitted") {
                info.timestamp = (long long)now;
                info.time      = FormatTime(now);
            }
        }
        postOrdersUpdate();
    }

    // ── Account / PnL ─────────────────────────────────────────────────────────

    void setCoinWindow(HWND hWnd) {
        hCoinWnd = hWnd;
        if (!hWnd || !client->isConnected()) return;
        client->reqAccountSummary(9010, "All",
            "NetLiquidation,TotalCashValue,BuyingPower,"
            "AvailableFunds,ExcessLiquidity,GrossPositionValue,"
            "MaintMarginReq,InitMarginReq");
        client->reqAccountSummary(9011, "All", "$LEDGER:ALL");
        std::string acc;
        { std::lock_guard<std::mutex> lock(dataMutex); acc = accountId; }
        if (!acc.empty()) {
            LogDebug("Requesting account summary and PnL for: " + acc);
            client->reqPnL(9013, acc, "");
        }
    }

    void unsetCoinWindow() {
        hCoinWnd = nullptr;
        if (!client->isConnected()) return;
        client->cancelAccountSummary(9010);
        client->cancelAccountSummary(9011);
        client->cancelAccountSummary(9012);
        client->cancelPnL(9013);
    }

    void accountSummary(int reqId, const std::string& account,
                        const std::string& tag, const std::string& value,
                        const std::string& currency) override {
        std::lock_guard<std::mutex> lock(summaryMutex);
        if (!currency.empty()) summaryData[currency + "_" + tag] = value;
        summaryData[tag] = value;
        if (hCoinWnd) PostMessage(hCoinWnd, WM_ACCOUNT_SUMMARY, 0, 0);
    }

    void accountSummaryEnd(int reqId) override {}

    void pnl(int reqId, double daily, double unrealized, double realized) override {
        dailyPnL      = daily;
        unrealizedPnL = unrealized;
        realizedPnL   = realized;
        if (hCoinWnd) PostMessage(hCoinWnd, WM_PNL_UPDATE, 0, 0);
    }

    std::map<std::string, std::string> getAccountSummary() {
        std::lock_guard<std::mutex> lock(summaryMutex);
        return summaryData;
    }

    double getDailyPnL()      const { return dailyPnL; }
    double getUnrealizedPnL() const { return unrealizedPnL; }
    double getRealizedPnL()   const { return realizedPnL; }

    std::string getAccountNumber() {
        std::lock_guard<std::mutex> lock(dataMutex);
        return accountId;
    }

    // ── Portfolio ─────────────────────────────────────────────────────────────

    void setDiamondsWindow(HWND hWnd) { hDiamondsWnd = hWnd; }

    void position(const std::string& account, const Contract& contract,
                  Decimal shares, double avgCost) {
        std::lock_guard<std::mutex> lock(portfolioMutex);
        PositionInfo& info = portfolioMap[contract.symbol];
        info.symbol  = contract.symbol;
        info.shares  = shares;
        info.avgCost = avgCost;
        if (hDiamondsWnd) PostMessage(hDiamondsWnd, WM_DIAMONDS_UPDATE, 0, 0);
    }

    void positionEnd() { LogDebug("Position update ended"); }

    std::mutex& getPortfolioMutex() { return portfolioMutex; }
    std::map<std::string, TradingAPI::PositionInfo>& getPortfolioMap() { return portfolioMap; }

    // ── Symbol search ─────────────────────────────────────────────────────────

    void setSymbolSearchWindow(HWND hWnd) { hSymbolSearchWnd = hWnd; }

    void searchSymbols(const std::string& pattern) {
        if (!client->isConnected()) return;
        client->reqMatchingSymbols(9001, pattern);
    }

    std::vector<std::string> getSymbolResults() {
        std::lock_guard<std::mutex> lock(symbolMutex);
        return symbolResults;
    }

    void symbolSamples(int reqId,
                       const std::vector<ContractDescription>& contractDescriptions) override {
        std::lock_guard<std::mutex> lock(symbolMutex);
        symbolResults.clear();
        for (const auto& cd : contractDescriptions) {
            if (cd.contract.secType == "STK" || cd.contract.secType == "ETF") {
                std::string entry = std::to_string(cd.contract.conId)
                                  + "." + cd.contract.symbol;
                if (!cd.contract.primaryExchange.empty())
                    entry += "." + cd.contract.primaryExchange;
                symbolResults.push_back(entry);
            }
        }
        if (hSymbolSearchWnd)
            PostMessage(hSymbolSearchWnd, WM_SYMBOL_RESULTS, 0, 0);
    }

    // ── News ──────────────────────────────────────────────────────────────────

    void setNewsWindow(HWND hWnd) { hNewsWnd = hWnd; }

    void reqNewsForSymbol(int conId, const std::string& symbol) {
        if (!client->isConnected()) return;
        { std::lock_guard<std::mutex> lock(newsMutex); newsResults.clear(); }

        Contract contract;
        contract.conId    = conId;
        contract.symbol   = symbol;
        contract.secType  = "STK";
        contract.exchange = "SMART";
        contract.currency = "USD";

        if (newsRequestActive.exchange(true))
            client->cancelMktData(9002);

        client->reqMktData(9002, contract, "mdoff,292", false, false, {});
    }

    std::vector<std::string> getNewsResults() {
        std::lock_guard<std::mutex> lock(newsMutex);
        return newsResults;
    }

    void tickNews(int reqId, time_t timeStamp, const std::string& providerCode,
                  const std::string& articleId, const std::string& headline,
                  const std::string& extraData) override {
        std::string line = "[" + providerCode + "] " + headline;
        if (!extraData.empty()) line += " - " + extraData;
        { std::lock_guard<std::mutex> lock(newsMutex); newsResults.push_back(line); }
        if (hNewsWnd) PostMessage(hNewsWnd, WM_NEWS_RESULTS, 0, 0);
    }

    // ── EWrapper stubs (all unimplemented callbacks) ──────────────────────────

    #define managedAccounts   managedAccounts_ignored
    #define error             error_ignored
    #define symbolSamples     symbolSamples_ignored
    #define connectAck        connectAck_ignored
    #define connectionClosed  connectionClosed_ignored
    #define nextValidId       nextValidId_ignored
    #define accountSummary    accountSummary_ignored
    #define accountSummaryEnd accountSummaryEnd_ignored
    #define pnl               pnl_ignored
    #define openOrder         openOrder_ignored
    #define openOrderEnd      openOrderEnd_ignored
    #define completedOrder    completedOrder_ignored
    #define completedOrdersEnd completedOrdersEnd_ignored
    #define orderStatus       orderStatus_ignored
    #define position          position_ignored
    #define positionEnd       positionEnd_ignored
    #define tickNews          tickNews_ignored

    #define EWRAPPER_VIRTUAL_IMPL {}
    #include "EWrapper_prototypes.h"
    #undef EWRAPPER_VIRTUAL_IMPL

    #undef managedAccounts
    #undef error
    #undef symbolSamples
    #undef connectAck
    #undef connectionClosed
    #undef nextValidId
    #undef accountSummary
    #undef accountSummaryEnd
    #undef pnl
    #undef openOrder
    #undef openOrderEnd
    #undef completedOrder
    #undef completedOrdersEnd
    #undef orderStatus
    #undef position
    #undef positionEnd
    #undef tickNews

} api;