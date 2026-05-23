#pragma once

#include <windows.h>
#include <shobjidl.h>
#include <dwmapi.h>
#include <initguid.h>
#include <propkey.h>
#include <propvarutil.h>
#include <richedit.h>

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

#include "messages.h"

// ─────────────────────────────────────────────────────────────────────────────
// TradingAPI — public interface
// Implementation lives in Trading-Floor-Gateway.a (private.cpp / Impl).
// ─────────────────────────────────────────────────────────────────────────────

class TradingAPI {
public:

    // ── Data types ────────────────────────────────────────────────────────────

    struct OrderInfo {
        int         orderId   = 0;
        std::string symbol;
        std::string exchange;
        std::string action;
        std::string orderType;
        double      price     = 0.0;
        double      auxPrice  = 0.0;
        double      totalQty  = 0.0;
        double      filledQty = 0.0;
        double      avgFillPx = 0.0;
        std::string status;
        std::string time;
        long long   timestamp = 0;
    };

    struct PositionInfo {
        std::string symbol;
        double      shares            = 0.0;
        double      avgCost           = 0.0;
        double      dailyPnL          = 0.0;
        double      marketValue       = 0.0;
        double      fiftyTwoWeekChange = 0.0;
        double      marketCap         = 0.0;
    };

    // lParam of WM_TIMESALES_TICK — handler owns and must delete.
    struct TsTickEntry {
        double      price    = 0.0;
        double      size     = 0.0;
        std::string time;
        std::string exchange;
    };

    // lParam of WM_NEWS_RESULTS — handler owns and must delete.
    struct NewsTickEntry {
        std::string providerCode;
        std::string articleId;
        std::string headline;
        std::string timeStamp;
        std::string extraData;
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    TradingAPI();
    ~TradingAPI();

    // Non-copyable, non-movable.
    TradingAPI(const TradingAPI&)            = delete;
    TradingAPI& operator=(const TradingAPI&) = delete;

    // ── Connection ────────────────────────────────────────────────────────────

    bool connect();
    void disconnect();
    bool isConnected()           const;
    bool isMarketDataConnected() const;
    bool isTradingConnected()    const;

    // ── API update broadcast ──────────────────────────────────────────────────

    void addApiUpdateWindow(HWND hWnd);
    void removeApiUpdateWindow(HWND hWnd);
    void setApiErrorWindow(HWND hWnd);
    void clearApiErrorWindow(HWND hWnd);

    // ── Account / PnL ─────────────────────────────────────────────────────────

    void setCoinWindow(HWND hWnd);
    void unsetCoinWindow();
    std::map<std::string, std::string> getAccountSummary();
    double getDailyPnL()      const;
    double getUnrealizedPnL() const;
    double getRealizedPnL()   const;
    std::string getAccountNumber();

    // ── Orders ────────────────────────────────────────────────────────────────

    void setOrdersWindow(HWND hWnd);
    void unsetOrdersWindow();
    std::vector<OrderInfo> getOrdersSorted();

    // ── Portfolio ─────────────────────────────────────────────────────────────

    void setDiamondsWindow(HWND hWnd);
    void unsetDiamondsWindow();
    std::mutex& getPortfolioMutex();
    std::map<std::string, PositionInfo>& getPortfolioMap();

    // ── Time and Sales ────────────────────────────────────────────────────────

    void setTimesalesWindow(HWND hWnd, int conId, const std::string& symbol);
    void unsetTimesalesWindow();

    // ── Symbol search ─────────────────────────────────────────────────────────

    void setSymbolSearchWindow(HWND hWnd);
    void searchSymbols(const std::string& pattern);
    std::vector<std::string> getSymbolResults();

    // ── News ──────────────────────────────────────────────────────────────────

    void setNewsWindow(HWND hWnd);
    void reqNewsForSymbol(int conId, const std::string& symbol);
    void reqNewsArticle(const std::string& providerCode, const std::string& articleId);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

// ── Global instance — defined in Trading-Floor-Gateway.a ─────────────────────
extern TradingAPI api;