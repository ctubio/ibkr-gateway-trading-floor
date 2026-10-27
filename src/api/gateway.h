#pragma once


#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <wincred.h>
#include <wininet.h>
#include <shobjidl.h>
#include <dwmapi.h>
#include <initguid.h>
#include <propkey.h>
#include <propvarutil.h>
#include <richedit.h>
#include <sapi.h>
#include <sphelper.h>
#include <uxtheme.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <tlhelp32.h>

#include <string>
#include <limits>
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <cstddef>
#include <cctype>
#include <cmath>
#include <utility>
#include <random>
#include <regex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <deque>
#include <array>
#include <map>
#include <vector>
#include <span>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <iterator>
#include <format>
#include <memory>
#include <cassert>
#include <exception>
#include <condition_variable>
#include <charconv>
#include <optional>
#include <string_view>
#include <variant>

#include "images.h"
#include "volume.h"
#include "model.h"
#include "snapshots.h"
#include "core.h"

inline std::wstring StringToWide(const std::string& str) {
    if (str.empty()) return {};
    if (str.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("UTF-8 string is too large to convert");

    const int length = static_cast<int>(str.size());
    const int wideLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                                str.data(), length, nullptr, 0);
    if (wideLength == 0)
        throw std::runtime_error("Invalid UTF-8 string");

    std::wstring wide(static_cast<size_t>(wideLength), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(), length,
                            wide.data(), wideLength) == 0)
        throw std::runtime_error("UTF-8 to UTF-16 conversion failed");
    return wide;
}

inline std::string WideToString(const std::wstring& wide) {
    if (wide.empty()) return {};
    if (wide.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("UTF-16 string is too large to convert");

    const int length = static_cast<int>(wide.size());
    const int utf8Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                                wide.data(), length, nullptr, 0,
                                                nullptr, nullptr);
    if (utf8Length == 0)
        throw std::runtime_error("Invalid UTF-16 string");

    std::string utf8(static_cast<size_t>(utf8Length), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(), length,
                            utf8.data(), utf8Length, nullptr, nullptr) == 0)
        throw std::runtime_error("UTF-16 to UTF-8 conversion failed");
    return utf8;
}

#define WM_TRAYICON             (WM_USER + 20)
#define WM_ACTIVATE_ORDER_ROW   (WM_USER + 22)

struct AlertPopupData {
    std::string title;
    std::string msg;
    std::string symbol;
    double shares;
    double price;
    int conId;
    bool isUp;
};

// ─── Colors ───────────────────────────────────────────────────────────────────
#define COINS_CLR_GREEN       RGB( 18, 220,  18)
#define COINS_CLR_GREEN_DARK  RGB(92, 214, 92)
#define COINS_CLR_GREEN_DARK2 RGB(32, 150, 32)
#define COINS_CLR_RED         RGB(220,  55,  55)
#define COINS_CLR_RED_DARK    RGB(217, 95, 95)
#define COINS_CLR_RED_DARK2   RGB(148, 33, 33)
#define COINS_CLR_WHITE       RGB(220, 220, 220)
#define COINS_CLR_BLACK       RGB(30,  30,  30)
#define COINS_CLR_GRAY        RGB(150, 150, 150)
#define COINS_CLR_BLUE        RGB(80, 160, 255)
#define COINS_CLR_PURPLE      RGB(185, 105, 225)
#define COINS_CLR_CYAN        RGB(0, 255, 255)
#define COINS_CLR_PINK        RGB(225, 105, 211)
#define COINS_CLR_ORANGE      RGB(255, 165, 0)
#define COINS_CLR_YELLOW      RGB(201, 183, 41)
// Dark background fills for order-side colored input boxes
#define COINS_BG_DARK_GREEN RGB( 34, 82, 50)
#define COINS_BG_DARK_RED   RGB(102, 43, 43)

static const char* COINS_CLASS_NAME            = "Coins";
static const char* DIAMONDS_CLASS_NAME             = "Diamonds";
static const char* ORDERS_CLASS_NAME               = "Orders";
static const char* MARKET_CLASS_NAME               = "Market";
static const char* MARKET_SEARCH_CLASS_NAME        = "Market_SearchSymbol";
static const char* SETTINGS_CLASS_NAME             = "Settings";
static const char* DEBUGLOG_CLASS_NAME             = "DebugLog";
static const char* COINS_EXCHANGE_CLASS_NAME   = "Exchange";
static const char* ALERTS_EDITOR_CLASS_NAME        = "AlertEditor";
static const char* ALERT_NOTIFY_CLASS_NAME         = "AlertNotification";
static const char* LOCK_CLASS_NAME                 = "Lock";
static const char* SCREEN_FLASH_OVERLAY_CLASS_NAME = "ScreenFlashOverlay";
static const char* NEWS_CACHE_SUBKEY               = "News";

class TradingAPI {
public:

    // Payload passed during HWND creation
    struct MarketInitData { std::string symbol; int conId; std::string winKey; };

    // ── Data types ────────────────────────────────────────────────────────────

    using OrderInfo = ::OrderInfo;
    using PnlSinglePayload = ::PnlSinglePayload;
    using PositionInfo = ::PositionInfo;

    using L1Book = ::L1Book;

    struct L1VolumeRate {
        double sum5min = 0.0;
        double ratio5min = 0.0;
    };

    struct PriceAlert {
        std::string symbol;
        int conId = 0;
        double up = 0.0;
        double down = 0.0;
    };
    // ── Lifecycle ─────────────────────────────────────────────────────────────

    TradingAPI();
    ~TradingAPI();

    // Non-copyable, non-movable.
    TradingAPI(const TradingAPI&)            = delete;
    TradingAPI& operator=(const TradingAPI&) = delete;

    // ── Connection ────────────────────────────────────────────────────────────

    bool connect(std::string address, int port, int clientId = 0, int groupId = 4);
    void disconnect();
    void shutdown();
    bool isConnected()           const;
    bool isMarketDataConnected() const;
    bool isTradingConnected()    const;

    Model& model();

    Snap<L1Book> quote(int conId) const;
    Snap<L2Book> book(int conId) const;
    Snap<PositionMap> positions() const;
    Snap<OrderMap> orders() const;
    Snap<ExecList> executions() const;
    Snap<AccountSnap> account() const;
    Snap<L1Book> fxSnap() const;
    Snap<SymbolList> symbolsSnap() const;

    // ── Orders ────────────────────────────────────────────────────────────────

    bool orderIsEditable(const std::string& status) const;
    void cancelOrders(int coinId);
    // Transmit a cancel request for the given order.
    void cancelOrder(int orderId, bool refresh);
    // Submit a new limit order
    void submitOrder(int conId, std::string symbol, std::string action, bool isOvernight, double qty, double price, double stopPrice, double stopPriceAwayFrom, double profitPrice, bool transmit = true);
    // Resubmit an existing order with a new limit price and quantity while
    // keeping all other order fields (type, action, exchange, …) intact.
    // Pass price=0 to keep the original limit price.
    void modifyOrder(int orderId, double price, double qty);

    // ── Portfolio ─────────────────────────────────────────────────────────────

    // ── Time and Sales ────────────────────────────────────────────────────────

    void setMarketWindow(HWND windowId, int conId, const std::string& symbol);
    void unsetMarketWindow(HWND windowId);

    // ── Watch-only symbols (Diamonds "Quarantine" alert-only rows) ───────────
    // Ensures L1 market data + weekly-range (13/26/52-week) + dividend data
    // flow for `conId`/`symbol` even when it isn't (yet) a held portfolio
    // position, e.g. a symbol with only an Alert Up/Down price set. Safe to
    // call repeatedly (idempotent), and safe to call for a conId that already
    // IS a held position (no-ops on top of the existing subscription).
    // Deliberately skips PnLSingle/L2/tick-by-tick, those stay Market-window-only.
    void watchSymbol(int conId, std::string symbol);
    // Replaces active price alerts and starts L1 watches for their symbols.
    // resetConId re-arms that symbol's one-shot alerts after an edit.
    void setPriceAlerts(std::vector<PriceAlert> alerts, int resetConId = 0);

    // ── Level 1 (market window) ────────────────────────────────

    std::optional<L1VolumeRate> getVolumeRate(int conId);

    // ── Symbol search ─────────────────────────────────────────────────────────

    void searchSymbols(HWND hWnd, std::string pattern);

    // ── FX / Currency Conversion (Coins "Exchange Currency" popup) ───────────
    // Subscribes streaming market data for the EUR.USD spot FX contract on
    // IDEALPRO; call fxSnap() to read the latest bid/ask/last.
    void reqFxRate(HWND hWnd);
    // Cancels the market data subscription started by reqFxRate() for this window.
    void cancelFxRate(HWND hWnd);
    // Places a market order converting between USD and EUR on IDEALPRO.
    //   action        = "BUY"  (spend USD, receive EUR) or "SELL" (spend EUR, receive USD)
    //   totalQuantity = order notional, denominated in EUR (matches IB's CASH contract convention)
    void submitCurrencyOrder(std::string action, double totalQuantity);

    // ── Display Group Linking (TWS "linked" window groups) ───────────────────
    // Pushes contract `conId` (on `exchange`) to every other TWS window/app
    // currently linked to the same group, so they switch their symbol to match.
    // The client subscribes to display-group events during connection setup.
    void updateDisplayGroup(int conId, std::string exchange = "SMART");

    // ── Historical Data (on-demand) ──────────────────────────────────────────
        // Fetches historical bars for `symbol` (resolved from the current
        // portfolio first, then via contract details). Blocks the
        // calling thread (safe from an HTTP handler thread, NOT the UI thread)
        // until the async TWS response completes or timeoutMs elapses. Returns
        // formatted rows "Date,Open,High,Low,Close,Wap,Volume,TradesCount";
        // empty on failure, timeout, or unknown symbol.
    std::vector<std::string> getHistoricalDataSync(const std::string& symbol, const std::string& timeframe, int timeoutMs = 15000);

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

void Orders_AddUnsentFromGateway(HWND hWnd, const TradingAPI::OrderInfo& info);