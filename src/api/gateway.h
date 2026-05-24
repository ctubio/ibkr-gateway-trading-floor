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

#define WM_API_UPDATE      (WM_USER + 2)
#define WM_SYMBOL_RESULTS  (WM_USER + 3)
#define WM_API_ERROR       (WM_USER + 4)
#define WM_ACCOUNT_SUMMARY (WM_USER + 5)
#define WM_PNL_UPDATE      (WM_USER + 6)
#define WM_ORDERS_UPDATE   (WM_USER + 7)
#define WM_DIAMONDS_UPDATE (WM_USER + 8)
#define WM_NEWS_RESULTS    (WM_USER + 9)
#define WM_TIMESALES_TICK  (WM_USER + 10)
#define WM_NEWS_ARTICLE    (WM_USER + 11)
#define WM_TICKER_UPDATE   (WM_USER + 12)

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
        int         conId;
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

    // One row in the watchlist ticker. Posted via WM_TICKER_UPDATE (lParam = new std::string*(symbol)).
    // Handler calls getTickerData(symbol) to read updated values, then deletes the string.
    struct TickerInfo {
        std::string symbol;
        double    last      = 0.0;
        double    prevClose = 0.0;  // previous close, used to compute change
        double    bid       = 0.0;
        double    ask       = 0.0;
        double    high      = 0.0;
        double    low       = 0.0;
        long long volume    = 0;
 
        double change()    const { return prevClose > 0 ? last - prevClose : 0.0; }
        double changePct() const { return prevClose > 0 ? (last - prevClose) / prevClose * 100.0 : 0.0; }
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

    // ── Ticker ────────────────────────────────────────────────────────

    void setTickerWindow(HWND hWnd, const std::vector<std::string>& entries);
    void unsetTickerWindow();
    void reqWatchlist();
    bool getTickerData(int conId, const std::string& symbol, TickerInfo& out);

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