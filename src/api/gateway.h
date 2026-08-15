#pragma once

#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <wininet.h>
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
#include <gdiplus.h>

#include <string>
#include <cstring>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <cmath>
#include <random>
#include <regex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <deque>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <format>
#include <memory>
#include <exception>

#ifdef GATEWAY_NAME
    #define GATEWAY_SPACE " "
    #define GATEWAY_DASH  "-"
    #define GATEWAY_SIM
#else
    #define GATEWAY_NAME  ""
    #define GATEWAY_SPACE ""
    #define GATEWAY_DASH  ""
#endif

#define WM_API_UPDATE       (WM_USER +  2)
#define WM_SYMBOL_RESULTS   (WM_USER +  3)
#define WM_API_LOG          (WM_USER +  4)
#define WM_ACCOUNT_SUMMARY  (WM_USER +  5)
#define WM_PNL_UPDATE       (WM_USER +  6)
#define WM_DIAMONDS_UPDATE  (WM_USER +  7)
#define WM_MARKET_TICK      (WM_USER +  8)
#define WM_MARKET_L1        (WM_USER +  9)
#define WM_MARKET_L2        (WM_USER + 10)   // Level 2 depth change — handler calls getLevel2Snapshot()
#define WM_PNL_SINGLE       (WM_USER + 11)   // Per-position PnL update — posted by pnlSingle() to the subscribed window. wParam = conId (int), lParam = heap-allocated PnlSinglePayload* (caller must delete).
#define WM_API_EXECUTION    (WM_USER + 12)
#define WM_FX_RATE_UPDATE   (WM_USER + 13)   // Posted to the DASHBOARD_EXCHANGE_CLASS_NAME popup whenever the EUR.USD FX rate ticks. No lParam — call getFxRate() to read the latest bid/ask/last.
#define WM_API_UNSENT_ORDER (WM_USER + 14)   // Posted to the Orders window for an untransmitted (transmit=false)
                                              // order. lParam = heap-allocated TradingAPI::OrderInfo* — handler
                                              // owns it and must delete it. Purely cosmetic: never written to
                                              // ordersMap, so it's naturally cleared on the next Orders_Repopulate().
#define WM_OPEN_ORDERS_WINDOW (WM_USER + 15) // Posted from background thread (e.g. HTTP server) to the
                                              // Dashboard window to request opening the Orders window on the UI thread.

static const char* DASHBOARD_CLASS_NAME          = "Dashboard" GATEWAY_NAME;
static const char* DIAMONDS_CLASS_NAME           = "Diamonds" GATEWAY_NAME;
static const char* ORDERS_CLASS_NAME             = "Orders" GATEWAY_NAME;
static const char* MARKET_CLASS_NAME             = "Market" GATEWAY_NAME;
static const char* MARKET_SEARCH_CLASS_NAME      = "Market_SearchSymbol" GATEWAY_NAME;
static const char* SETTINGS_CLASS_NAME           = "Settings" GATEWAY_NAME;
static const char* DEBUGLOG_CLASS_NAME           = "DebugLog" GATEWAY_NAME;
static const char* DASHBOARD_EXCHANGE_CLASS_NAME = "Exchange" GATEWAY_NAME;

class TradingAPI {
public:

    // Payload passed during HWND creation
    struct MarketInitData { std::string symbol; int conId; std::string winKey; };

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
        double      fullStopPrice     = 0.0;
        double      fullProfitPrice     = 0.0;
        double      totalQty  = 0.0;
        double      filledQty = 0.0;
        double      avgFillPx = 0.0;
        double      trailStopPrice = 0.0;
        std::string status;
        std::string time;
        bool includeOvernight = false;
        int      parentId = 0;      // Parent order Id, to associate Auto STP or TRAIL or TAKE-PROFIT orders with the original order.
        std::string ocaGroup;      // one cancels all group name
        int      ocaType = 0;       // 1 = CANCEL_WITH_BLOCK, 2 = REDUCE_WITH_BLOCK, 3 = REDUCE_NON_BLOCK
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
    };

    struct PositionInfo {
        int         conId;
        std::string symbol;
        std::string exchange;
        double      shares            = 0.0;
        double      avgCost           = 0.0;
        PnlSinglePayload pnlSingle;

        // ── 13 / 26 / 52-week % change reference closes ─────────────────────
        // Daily-bar closing prices from ~91 / ~182 / ~364 calendar days ago,
        // used to compute true "% change since N weeks ago" (last vs. this
        // close), as opposed to L1Book::WeekNRangePct() below which reports
        // where price sits *within* the N-week high/low range instead.
        //
        // Populated once per position via a single one-shot reqHistoricalData()
        // daily-bar request (see TradingAPI::Impl::queueWeeklyRangeFetch()),
        // paced several seconds apart so a ~60-symbol portfolio never gets
        // anywhere near IBKR's historical-data pacing limits. 0.0 = not
        // fetched yet — callers should treat that as "no data available",
        // the same convention avgCost/shares use before positions arrive.
        double closeAgo13Week = 0.0;
        double closeAgo26Week = 0.0;
        double closeAgo52Week = 0.0;
    };

    // lParam of WM_MARKET_TICK — handler owns and must delete.
    struct TsTickEntry {
        COLORREF    side;
        double      price    = 0.0;
        double      size     = 0.0;
        std::string time;
        std::string exchange;
    };

    // ── Level 1 quote (per market window) ────────────────────────────────────
    // Populated via reqMktData ticks; retrieved with getMarketData().
    // One row in the watchlist / diamonds watchlist.
    // Posted via WM_MARKET_L1 (lParam = new std::string("conId.symbol")).
    // Handler calls getMarketData(conId, out) then deletes the string.
    struct L1Book {
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
        long long volume     = 0;
        double auctionPrice  = 0.0; // AUCTION_PRICE tick (field 35) — populated during auction sessions only
        double auctionShares = 0.0; // AUCTION_PRICE tick (field 35) — populated during auction sessions only

        // ── Size ticks (tickSize) ────────────────────────────────────────────
        double bidSize   = 0.0;
        double askSize   = 0.0;

        // ── Fundamental ratios (tickString field 47, generic tick "258") ─────
        // Populated once per session; "-99999.99" sentinel is skipped.
        double low13     = 0.0;
        double high13    = 0.0;
        double low26     = 0.0;
        double high26    = 0.0;
        double low52     = 0.0;
        double high52    = 0.0;

        // ── Dividends (tickString field 59, generic tick "456") ──────────────
        double annualDividends  = 0.0;
        double dividendAmount   = 0.0;
        std::string dividendDate;
        double dividendDateSortable;

        // ── Generic ticks (tickGeneric) ──────────────────────────────────────
        bool   halted = false;          // field 49 (HALTED)

        // ── Computed helpers ─────────────────────────────────────────────────
        double change()    const { return (prevClose > 0 && last > 0) ? last - prevClose : 0.0; }
        double changePct() const { return (prevClose > 0 && last > 0) ? (last - prevClose) / prevClose * 100.0 : 0.0; }
        double dividendYield() const { return (last > 0 && annualDividends > 0) ? (annualDividends / last * 100.0) : 0.0; }

        // Position in 52W range: 0% = at 52W low, 100% = at 52W high.
        // Returns a sentinel (0.0) when data is not yet available.
        double Week52RangePct() const {
            double range = high52 - low52;
            if (range <= 0 || last <= 0) return 0.0;
            return (last - low52) / range * 100.0;
        }
        // Position in 52W range: 0% = at 52W low, 100% = at 52W high.
        // Returns a sentinel (0.0) when data is not yet available.
        double Week26RangePct() const {
            double range = high26 - low26;
            if (range <= 0 || last <= 0) return 0.0;
            return (last - low26) / range * 100.0;
        }
        // Position in 52W range: 0% = at 52W low, 100% = at 52W high.
        // Returns a sentinel (0.0) when data is not yet available.
        double Week13RangePct() const {
            double range = high13 - low13;
            if (range <= 0 || last <= 0) return 0.0;
            return (last - low13) / range * 100.0;
        }
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

    // ── Account / PnL ─────────────────────────────────────────────────────────

    std::map<std::string, std::string> getAccountSummary();
    double getDailyPnL()      const;
    double getUnrealizedPnL() const;
    double getRealizedPnL()   const;
    std::string getAccountNumber();

    // ── Orders ────────────────────────────────────────────────────────────────

    void cancelOrders(int coinId);
    std::vector<OrderInfo> getOrdersSorted();
    std::vector<OrderInfo> getExecutions();
    // Transmit a cancel request for the given order.
    void cancelOrder(int orderId);
    // Submit a new limit order
    void submitOrder(int conId, const std::string& symbol, const std::string& action, const bool isOvernight, double qty, double price, double stopPrice, double stopPriceAwayFrom, double profitPrice, bool transmit = true);
    // Resubmit an existing order with a new limit price and quantity while
    // keeping all other order fields (type, action, exchange, …) intact.
    // Pass price=0 to keep the original limit price.
    void modifyOrder(int orderId, double price, double qty);

    // ── Portfolio ─────────────────────────────────────────────────────────────

    std::mutex& getPortfolioMutex();
    std::map<int, PositionInfo>& getPortfolioMap();

    // ── Time and Sales ────────────────────────────────────────────────────────

    void setMarketWindow(HWND hWnd, int conId, const std::string& symbol);
    void unsetMarketWindow(HWND hWnd);

    // ── Level 1 (market window) ────────────────────────────────

    bool getMarketData(int conId, L1Book& out);
    
    // ── Level 2 data (market window) ────────────────────────────────

    // Returns a sorted snapshot of the order book for the given market window.
    //   bids : sorted by price descending  (bids[0] = best bid)
    //   asks : sorted by price ascending   (asks[0] = best ask)
    // Call from the WM_MARKET_L2 handler; thread-safe.
    bool getLevel2Snapshot(int conId, std::vector<Level2Entry>& bids, std::vector<Level2Entry>& asks);

    // ── Symbol search ─────────────────────────────────────────────────────────

    void searchSymbols(HWND hWnd, const std::string& pattern);
    std::vector<std::string> getSymbolResults();

    // ── FX / Currency Conversion (Dashboard "Exchange Currency" popup) ───────────
    // Subscribes streaming market data for the EUR.USD spot FX contract on
    // IDEALPRO and routes ticks to hWnd via WM_FX_RATE_UPDATE (no lParam —
    // call getFxRate() to read the latest bid/ask/last).
    void reqFxRate(HWND hWnd);
    // Cancels the market data subscription started by reqFxRate() for this window.
    void cancelFxRate(HWND hWnd);
    // Returns the latest known EUR.USD bid/ask/last (all 0.0 until the first tick arrives).
    bool getFxRate(L1Book& out);
    // Places a market order converting between USD and EUR on IDEALPRO.
    //   action        = "BUY"  (spend USD, receive EUR) or "SELL" (spend EUR, receive USD)
    //   totalQuantity = order notional, denominated in EUR (matches IB's CASH contract convention)
    void submitCurrencyOrder(const std::string& action, double totalQuantity);

    // ── Historical Data (on-demand) ──────────────────────────────────────────
        // Fetches ~1 year of daily bars for `symbol` (must be a current portfolio
        // position — conId is looked up from getPortfolioMap()). Blocks the
        // calling thread (safe from an HTTP handler thread, NOT the UI thread)
        // until the async TWS response completes or timeoutMs elapses. Returns
        // formatted rows "Date,Open,High,Low,Close,Wap,Volume,TradesCount", one
        // per daily bar; empty on failure/timeout/symbol-not-a-position.
    std::vector<std::string> getHistoricalDataSync(const std::string& symbol, bool isYear, int timeoutMs = 15000);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};