#pragma once

#include <windows.h>
#include <windowsx.h>
#include <shobjidl.h>
#include <dwmapi.h>
#include <initguid.h>
#include <propkey.h>
#include <propvarutil.h>
#include <richedit.h>
#include <sapi.h>
#include <sphelper.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <commctrl.h>

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <cstring>
#include <algorithm>
#include <filesystem>

#define WM_API_UPDATE       (WM_USER +  2)
#define WM_SYMBOL_RESULTS   (WM_USER +  3)
#define WM_API_LOG          (WM_USER +  4)
#define WM_ACCOUNT_SUMMARY  (WM_USER +  5)
#define WM_PNL_UPDATE       (WM_USER +  6)
#define WM_ORDERS_UPDATE    (WM_USER +  7)
#define WM_DIAMONDS_UPDATE  (WM_USER +  8)
#define WM_NEWS_RESULTS     (WM_USER +  9)
#define WM_MARKET_TICK      (WM_USER + 10)
#define WM_NEWS_ARTICLE     (WM_USER + 11)
#define WM_WATCHLIST_UPDATE (WM_USER + 12)
#define WM_MARKET_L1        (WM_USER + 13)   // Level 1 quote tick — handler calls getLevel1Data()
#define WM_MARKET_L2        (WM_USER + 14)   // Level 2 depth change — handler calls getLevel2Snapshot()
#define WM_PNL_SINGLE       (WM_USER + 15)   // Per-position PnL update — posted by pnlSingle() to the subscribed window. wParam = conId (int), lParam = heap-allocated PnlSinglePayload* (caller must delete).

class TradingAPI {
public:

    // ── Data types ────────────────────────────────────────────────────────────

    struct OrderInfo {
        int         orderId   = 0;
        int         conId     = 0;      // TWS contract ID — populated from openOrder
        std::string symbol;
        std::string exchange;
        std::string action;
        std::string orderType;
        std::string tif;
        double      price     = 0.0;
        double      auxPrice  = 0.0;
        double      totalQty  = 0.0;
        double      filledQty = 0.0;
        double      avgFillPx = 0.0;
        std::string status;
        std::string time;
        long long   timestamp = 0;
        bool includeOvernight = false;
    };

    struct PositionInfo {
        std::string symbol;
        std::string exchange;
        int         conId;
        double      shares            = 0.0;
        double      avgCost           = 0.0;
        double      dailyPnL          = 0.0;
        double      marketValue       = 0.0;
        double      fiftyTwoWeekChange = 0.0;  // kept for compat; populated via WatchlistInfo now
        double      marketCap         = 0.0;   // kept for compat; populated via WatchlistInfo now
    };

    // lParam of WM_MARKET_TICK — handler owns and must delete.
    struct TsTickEntry {
        COLORREF    side;
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

    // One row in the watchlist / diamonds watchlist.
    // Posted via WM_WATCHLIST_UPDATE (lParam = new std::string("conId.symbol")).
    // Handler calls getWatchlistData(conId, symbol, out) then deletes the string.
    struct WatchlistInfo {
        std::string symbol;

        // ── Price ticks (tickPrice) ──────────────────────────────────────────
        double last      = 0.0;
        double prevClose = 0.0;  // CLOSE tick — used to compute change
        double open      = 0.0;  // OPEN tick (field 14)
        double bid       = 0.0;
        double ask       = 0.0;
        double high      = 0.0;
        double low       = 0.0;
        double vwap      = 0.0;  // VWAP tick (field 236, generic tick "258") — populated during regular trading hours only

        // ── Size ticks (tickSize) ────────────────────────────────────────────
        double bidSize   = 0.0;
        double askSize   = 0.0;

        // ── Fundamental ratios (tickString field 47, generic tick "258") ─────
        // Populated once per session; "-99999.99" sentinel is skipped.
        double fiftyTwoWeekHigh = 0.0;  // NHIGH52
        double fiftyTwoWeekLow  = 0.0;  // NLOW52
        double marketCap        = 0.0;  // MKTCAP  (billions)
        double beta             = 0.0;  // BETA

        // ── Dividends (tickString field 59, generic tick "456") ──────────────
        double annualDividends  = 0.0;
        double dividendAmount   = 0.0;
        std::string dividendDate;

        // ── Generic ticks (tickGeneric) ──────────────────────────────────────
        bool   halted = false;          // field 49 (HALTED)

        // ── Computed helpers ─────────────────────────────────────────────────
        double change()    const { return prevClose > 0 ? last - prevClose : 0.0; }
        double changePct() const { return prevClose > 0 ? (last - prevClose) / prevClose * 100.0 : 0.0; }
        double dividendYield() const { return (last > 0 && annualDividends > 0) ? (annualDividends / last * 100.0) : 0.0; }

        // Position in 52W range: 0% = at 52W low, 100% = at 52W high.
        // Returns a sentinel (-999) when data is not yet available.
        double fiftyTwoWeekRangePct() const {
            double range = fiftyTwoWeekHigh - fiftyTwoWeekLow;
            if (range <= 0 || last <= 0) return -999.0;
            return (last - fiftyTwoWeekLow) / range * 100.0;
        }
    };
 
    // ── Level 1 quote (per market window) ────────────────────────────────────
    // Populated via reqMktData ticks; retrieved with getLevel1Data().
    // Posted via WM_MARKET_L1 (no lParam — call getLevel1Data to read).
    struct Level1Info {
        double open      = 0.0;   // OPEN tick (field 14, generic tick "221")
        double prevClose = 0.0;   // CLOSE tick (field 9) — yesterday's close
        double last      = 0.0;   // LAST tick (field 4)
        double high      = 0.0;   // HIGH tick (field 6)
        double low       = 0.0;   // LOW tick  (field 7)
        double bid       = 0.0;   // BID  tick (field 1)
        double bidSize   = 0.0;
        double ask       = 0.0;   // ASK  tick (field 2)
        double askSize   = 0.0;
        double vwap      = 0.0;   // VWAP tick (field 236, generic tick "258") — populated during regular trading hours only
        double change()    const { return prevClose > 0 ? last - prevClose : 0.0; }
        double changePct() const { return prevClose > 0 ? (last - prevClose) / prevClose * 100.0 : 0.0; }
    };

    // ── Level 2 depth entry (one row per side) ────────────────────────────────
    // Retrieved with getLevel2Snapshot(); bids sorted best-first (price desc),
    // asks sorted best-first (price asc).
    // Posted via WM_MARKET_L2 (no lParam — call getLevel2Snapshot to read).
    struct Level2Entry {
        std::string mmid;      // market-maker ID or venue name (may be empty)
        double      price = 0.0;
        double      size  = 0.0;
    };

    // ── Per-position live PnL payload ─────────────────────────────────────────
    // Heap-allocated by pnlSingle(); posted via WM_PNL_SINGLE.
    //   wParam = conId (int cast)
    //   lParam = PnlSinglePayload*  — the UI handler owns it and must delete it.
    // Only fields whose value != UNSET_DOUBLE are valid; check the has_* guards.
    struct PnlSinglePayload {
        int    conId          = 0;
        double dailyPnL       = 0.0;
        double unrealizedPnL  = 0.0;
        double realizedPnL    = 0.0;
        bool   has_daily      = false;
        bool   has_unrealized = false;
        bool   has_realized   = false;
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    TradingAPI();
    ~TradingAPI();

    // Non-copyable, non-movable.
    TradingAPI(const TradingAPI&)            = delete;
    TradingAPI& operator=(const TradingAPI&) = delete;

    // ── Connection ────────────────────────────────────────────────────────────

    bool connect(int port);
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
    void cancelOrders(int coinId);
    std::vector<OrderInfo> getOrdersSorted();

    // Transmit a cancel request for the given order.
    void cancelOrder(int orderId);

    // Submit a new limit order
    void submitOrder(int conId, const std::string& symbol, const std::string& action, const bool isOvernight, double qty, double price, double stopPrice, double profitPrice);

    // Resubmit an existing order with a new limit price and quantity while
    // keeping all other order fields (type, action, exchange, …) intact.
    // Pass price=0 to keep the original limit price.
    void modifyOrder(int orderId, double price, double qty);

    // ── Portfolio ─────────────────────────────────────────────────────────────

    void setDiamondsWindow(HWND hWnd);
    void unsetDiamondsWindow();
    void reqDiamondsWatchlist();   // call after reconnect to re-subscribe market data
    std::mutex& getPortfolioMutex();
    std::map<std::string, PositionInfo>& getPortfolioMap();

    // ── Per-position live PnL streaming ──────────────────────────────────────
    // Subscribe one position: posts WM_PNL_SINGLE to hWnd whenever TWS
    // sends a pnlSingle update for conId.  Safe to call from the UI thread.
    void subscribePositionPnL(HWND hWnd, int conId);

    // Cancel every pnlSingle subscription registered for hWnd and remove them
    // from the internal map.  Call from WM_DESTROY before the HWND is gone.
    void unsubscribePositionPnL(HWND hWnd);

    // ── Time and Sales ────────────────────────────────────────────────────────

    void setMarketWindow(HWND hWnd, int conId, const std::string& symbol);
    void unsetMarketWindow(HWND hWnd);

    // ── Level 1 / Level 2 data (market window) ────────────────────────────────

    // Returns the latest L1 quote snapshot for the given market window.
    // Call from the WM_MARKET_L1 handler; thread-safe.
    bool getLevel1Data(HWND hMarketWnd, Level1Info& out);

    // Returns a sorted snapshot of the order book for the given market window.
    //   bids : sorted by price descending  (bids[0] = best bid)
    //   asks : sorted by price ascending   (asks[0] = best ask)
    // Call from the WM_MARKET_L2 handler; thread-safe.
    bool getLevel2Snapshot(HWND hMarketWnd,
                           std::vector<Level2Entry>& bids,
                           std::vector<Level2Entry>& asks);

    // ── Watchlist (watchlist) ────────────────────────────────────────────────────

    void setWatchlistWindow(HWND hWnd, const std::vector<std::string>& entries);
    void unsetWatchlistWindow();
    void reqWatchlist();
    bool getWatchlistData(int conId, const std::string& symbol, WatchlistInfo& out);

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