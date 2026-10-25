#pragma once

static const int windowCoinsWidth  = 250;
static const int windowCoinsHeight = 382;

constexpr int y1 = 8;
constexpr int box1H = 94;
constexpr int y2 = y1 + box1H + 9;
constexpr int box2H = 124;
constexpr int y3 = y2 + box2H + 9;
constexpr int box3H = 64;
constexpr int boxW = 226;
constexpr int m = 10;

void StartCoins(HINSTANCE hInst) { StartGenericWindow(COINS_CLASS_NAME, "Trading Floor", L"TWSAPIClientTradingFloor.Coins", windowCoinsWidth, windowCoinsHeight, hInst); }

#define ID_M_COINS   1001
#define ID_MB_DIAMONDS   1003
#define ID_MB_SETTINGS   1004
#define ID_MB_MARKET     1007
#define ID_MB_EXCHANGE   1008
#define ID_MB_ORDERS     1009
#define ID_M_ORDERS      1010
#define ID_M_DIAMONDS    1012
#define ID_M_SETTINGS    1013
#define ID_M_MARKET      1014
#define ID_M_DEBUGLOG    1015

#define ID_M_CONNECT    1100
#define ID_M_DISCONNECT 1101
#define ID_M_EXIT       1102
#define ID_M_EXIT_SOFT  1103

#define ID_M_MARKET_BASE 1500
#define ID_M_MARKET_MAX   100

#define ID_M_LINKS_BASE  1700   // one command ID per quick link below

#define TIMER_WATCHDOG               1800
#define TIMER_WATCHDOG_DELAYED_START 1801
#define TIMER_MARKET_CLOCK           1802

struct QuickLink { const char* label; const char* url; };
static const QuickLink quickLinks[] = {
    { "Today",  "https://www.investing.com/dividends-calendar" },
    { "WSB",    "https://www.reddit.com/r/wallstreetbets"      },
    { "Scan",   "https://stockscan.io/all-stocks"              },
    { "List",   "https://stockanalysis.com/list/"              },
    { "Map",    "https://finviz.com/map.ashx?t=sec"            },
    { "Data",   "https://www.benzinga.com/quote"               },
    { "Notes",  "https://x.com/tradertvshawn"                  },
    { "Month",  "http://192.168.1.105:2025/ibkr"               },
    { "Chat",   "http://192.168.1.105:2025/chat"               },
    { "Portal", "https://www.interactivebrokers.ie/portal/"    },
    { "GitHub", "https://github.com/ctubio/ibkr-gateway-trading-floor" },
};
static const int LINKS_COUNT = (int)(sizeof(quickLinks) / sizeof(quickLinks[0]));

static const char* day_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// ─── Coins State ──────────────────────────────────────────────────────────
// Encapsulates all coins-specific HWNDs and state variables

struct CoinsState {
    // Window state
    bool fullDetails = true;
    bool shouldBeConnected = true;
    std::string currencyCoins = "--";
    ITaskbarList3* taskbar = nullptr;
    bool taskbarComInitialized = false;
    TBPFLAG taskbarLastState = TBPF_NOPROGRESS;
    
    // Group boxes
    HWND hCoinBox1 = NULL;
    HWND hCoinBox2 = NULL;
    HWND hCoinBox3 = NULL;
    
    // Labels (static text labels for rows)
    HWND hLblDividends = NULL;
    HWND hLblAccruals = NULL;
    HWND hLblBP = NULL;
    HWND hLblMM = NULL;
    HWND hLblEUR = NULL;
    HWND hLblUSD = NULL;
    
    // Box 1: Net Liq & PnL
    HWND hCoin_NetLiq = NULL;
    HWND hCoin_BigPnL = NULL;
    HWND hCoin_Pct = NULL;
    HWND hCoin_Realized = NULL;
    HWND hCoin_Speaker = NULL;   // speaker icon button (SS_NOTIFY)
    HWND hCoin_Lock = NULL;      // lock icon button (WM_KEYDOWN VK_SCROLL)
    HWND hCoin_Clock = NULL;     // market clock
    
    // Box 2: Positions & Margin
    HWND hCoin_Positions = NULL;
    HWND hCoin_Orders = NULL;
    HWND hCoin_Unrealized = NULL;
    HWND hCoin_Dividends = NULL;
    HWND hCoin_Accruals = NULL;
    HWND hCoin_BuyingPower = NULL;
    HWND hCoin_MaintMargin = NULL;
    
    // Box 3: Cash
    HWND hCoin_Cash = NULL;
    HWND hCoin_EUR = NULL;
    HWND hCoin_USD = NULL;
    
    // TTS state, speech goes through the shared SharedTtsEngine (shared.h)
    // now; this just tracks whether the coins holds a reference to it.
    bool coinsTtsOn = false;

    COLORREF clockColor = COINS_CLR_GRAY;
    COLORREF speakerColor = COINS_CLR_GRAY;
    COLORREF bigPnlColor = COINS_CLR_GREEN;
    COLORREF pctColor = COINS_CLR_GREEN;
    COLORREF orderColor = COINS_CLR_GRAY;
    COLORREF realizedColor = COINS_CLR_GRAY;
    COLORREF unrealizedColor = COINS_CLR_GRAY;
    COLORREF dividendsColor = COINS_CLR_PURPLE;
    COLORREF accrualsColor = COINS_CLR_GRAY;
    COLORREF cashColor = COINS_CLR_GRAY;
    COLORREF eurColor = COINS_CLR_GRAY;
    COLORREF usdColor = COINS_CLR_GRAY;
    COLORREF lockColor = COINS_CLR_GRAY;
};

// Global coins state instance
static CoinsState coinsState;

static void Coins_UpdateTaskbarOrders(HWND hWnd, int openOrdersCount) {
    if (!coinsState.taskbar) return;

    if (openOrdersCount > 0) {
        if (coinsState.taskbarLastState != TBPF_ERROR) {
            coinsState.taskbarLastState = TBPF_ERROR;
            coinsState.taskbar->SetProgressState(hWnd, TBPF_ERROR);
            coinsState.taskbar->SetProgressValue(hWnd, 100, 100);
        }
    } else if (coinsState.taskbarLastState != TBPF_NOPROGRESS) {
        coinsState.taskbarLastState = TBPF_NOPROGRESS;
        coinsState.taskbar->SetProgressState(hWnd, TBPF_NOPROGRESS);
    }
}

// Returns false if another instance exists but can't be accessed (e.g. it runs
// elevated and we don't), in which case the caller must exit immediately.
bool MutexGatewayInstance() {
    static const char* MUTEX_NAME = "Local\\TWSAPIClientTradingFloorMutex_17072025";

    HANDLE hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME);
    DWORD err = GetLastError();

    if (!hMutex && err == ERROR_ACCESS_DENIED) {
        MessageBoxA(NULL,
            "Another instance of Trading Floor is already running, but access to it was denied.\n\n"
            "It is probably running with higher privileges (as Administrator).\n"
            "Please close it from there, or run both instances with the same privileges.",
            "Trading Floor: Instance Access Denied",
            MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
        return false;
    }

    if (err == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindowA(COINS_CLASS_NAME, NULL);
        if (existingWnd) {
            DWORD processId;
            GetWindowThreadProcessId(existingWnd, &processId);

            PostMessage(existingWnd, WM_COMMAND, ID_M_EXIT_SOFT, 0);

            HANDLE hProcess = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, processId);
            if (hProcess) {
                DWORD waitResult = WaitForSingleObject(hProcess, 7000);

                if (waitResult == WAIT_TIMEOUT) {
                    TerminateProcess(hProcess, 0);
                }
                CloseHandle(hProcess);
            }
        }

        if (hMutex) CloseHandle(hMutex);

        hMutex = CreateMutexA(NULL, TRUE, MUTEX_NAME);
        if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
            // The old instance didn't go away (or someone else grabbed it).
            if (hMutex) CloseHandle(hMutex);
            MessageBoxA(NULL,
                "Another instance of Trading Floor is still running and could not be closed.",
                "Trading Floor: Instance Still Running",
                MB_OK | MB_ICONWARNING | MB_TOPMOST | MB_SETFOREGROUND);
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1021));
    }

    // Intentionally keep the handle open for the lifetime of the process;
    // the mutex disappears automatically when we exit.
    (void)hMutex;
    return true;
}

static int Coins_GetTextWidth(HWND hWnd, HFONT hFont, const char* text) {
    if (!hWnd || !text) return 0;
    HDC hdc = GetDC(hWnd);
    HFONT hOld = (HFONT)SelectObject(hdc, hFont);
    SIZE sz = { 0, 0 };
    GetTextExtentPoint32A(hdc, text, (int)strlen(text), &sz);
    SelectObject(hdc, hOld);
    ReleaseDC(hWnd, hdc);
    return sz.cx;
}

// ─── US market-session clock ─────────────────────────────────────────────
static void UpdateMarketClock(HWND hWnd) {
    if (!coinsState.hCoin_Clock) return;

    // Get current time in New York (Eastern Time)
    auto now = std::chrono::system_clock::now();
    static const std::chrono::time_zone* nyZone = nullptr;
    try {
        if (!nyZone) nyZone = std::chrono::locate_zone("America/New_York");
    } catch (...) {
        return;
    }
    std::chrono::zoned_time zt{nyZone, now};
    auto ny_time = zt.get_local_time();
    
    // Extract exact time of day and weekday
    auto dp = std::chrono::floor<std::chrono::days>(ny_time);
    auto time_of_day = std::chrono::hh_mm_ss{ny_time - dp};
    std::chrono::weekday wd{dp}; // Get current day of the week
    
    int total_secs = time_of_day.hours().count() * 3600 + 
                     time_of_day.minutes().count() * 60 + 
                     time_of_day.seconds().count();

    // Define NY Market Boundaries in seconds from Midnight
    const int T_PREMARKET = 4 * 3600;                     // 04:00 ET
    const int T_OPEN_IMBAL = 9 * 3600;                    // 09:00 ET (20m before 09:20)
    const int T_OPEN_BELL = 9 * 3600 + 20 * 60;           // 09:20 ET (10m before 09:30)
    //meh const int T_OPEN_BELL = 8 * 3600 + 30 * 60;          // 08:30 ET (60m before 09:30)
    const int T_OPEN = 9 * 3600 + 30 * 60;                // 09:30 ET
    const int T_CLOSE_IMBAL = 15 * 3600 + 30 * 60;        // 15:30 ET (20m before 15:50)
    const int T_CLOSE_BELL = 15 * 3600 + 50 * 60;         // 15:50 ET (10m before 16:00)
    //mehconst int T_CLOSE_BELL = 15 * 3600 + 00 * 60;        // 15:00 ET (60m before 16:00)
    const int T_CLOSE = 16 * 3600;                        // 16:00 ET
    const int T_AFTERHOURS_END = 20 * 3600;               // 20:00 ET
    const int T_SUNDAY_OPEN = 20 * 3600;                  // 20:00 ET (Sunday 8:00 PM)
    
    std::string phase;
    int target_secs = 0;

    // 1. Handle Weekend / Market Closed State (Targeting Sunday 20:00 ET)
    if (wd == std::chrono::Friday && total_secs >= T_AFTERHOURS_END) {
        phase = "Market Closed"; 
        target_secs = (2 * 24 * 3600) + T_SUNDAY_OPEN; // Fri to Sun 20:00
        coinsState.clockColor = COINS_CLR_GRAY;
    } else if (wd == std::chrono::Saturday) {
        phase = "Market Closed"; 
        target_secs = (1 * 24 * 3600) + T_SUNDAY_OPEN; // Sat to Sun 20:00
        coinsState.clockColor = COINS_CLR_GRAY;
    } else if (wd == std::chrono::Sunday && total_secs < T_SUNDAY_OPEN) {
        phase = "Market Closed"; 
        target_secs = T_SUNDAY_OPEN;                   // Sunday morning/afternoon to Sun 20:00
        coinsState.clockColor = COINS_CLR_GRAY;
    } else if (wd == std::chrono::Sunday && total_secs >= T_SUNDAY_OPEN) {
        phase = "OVERNIGHT"; 
        target_secs = (1 * 24 * 3600) + T_PREMARKET;   // Market is OPEN! Counting to Mon 04:00 AM
        coinsState.clockColor = COINS_CLR_GRAY;
    } 
    // 2. Standard Weekday State Machine (Monday - Friday)
    else {
        if (total_secs < T_PREMARKET) {
            phase = "OVERNIGHT"; target_secs = T_PREMARKET;
            coinsState.clockColor = COINS_CLR_GRAY;
        } else if (total_secs < T_OPEN_IMBAL) {
            phase = "Pre-Market"; target_secs = T_OPEN;
            coinsState.clockColor = COINS_CLR_ORANGE;
        } else if (total_secs < T_OPEN_BELL) {
            phase = "IMBALANCE"; target_secs = T_OPEN;
            coinsState.clockColor = COINS_CLR_RED;
        } else if (total_secs < T_OPEN) {
            phase = "Opening Bell"; target_secs = T_OPEN;
            coinsState.clockColor = COINS_CLR_BLUE;
        } else if (total_secs < T_CLOSE_IMBAL) {
            phase = "Market Open"; target_secs = T_CLOSE;
            coinsState.clockColor = COINS_CLR_YELLOW;
        } else if (total_secs < T_CLOSE_BELL) {
            phase = "IMBALANCE"; target_secs = T_CLOSE;
            coinsState.clockColor = COINS_CLR_RED;
        } else if (total_secs < T_CLOSE) {
            phase = "Closing Bell"; target_secs = T_CLOSE;
            coinsState.clockColor = COINS_CLR_BLUE;
        } else if (total_secs < T_AFTERHOURS_END) {
            phase = "After-Hours"; target_secs = T_AFTERHOURS_END;
            coinsState.clockColor = COINS_CLR_ORANGE;
        } else {
            phase = "OVERNIGHT"; target_secs = T_PREMARKET + (24 * 3600); // Tomorrow's 04:00 AM
            coinsState.clockColor = COINS_CLR_GRAY;
        }
    }

    SetWindowTextA(hWnd, phase.c_str());
    
    std::string day_str = day_names[wd.c_encoding()];
    std::string time_str = std::format("{:02}:{:02}", time_of_day.hours().count(), time_of_day.minutes().count()) + " " + day_str;
    Diamonds_SetTime(time_str.c_str());    

    // Calculate time left
    int secs_left = target_secs - total_secs;
    int h_left = secs_left / 3600;
    int m_left = (secs_left % 3600) / 60;

    // Format output string (HH:MM)
    std::string time_left_str = std::format("{:02}:{:02}", h_left, m_left);

    SetWindowTextA(coinsState.hCoin_Clock, time_left_str.c_str());
    InvalidateRect(coinsState.hCoin_Clock, NULL, TRUE);
}

static void Coins_ApplyFocusLayout(HWND hWnd, bool active) {
    const int show = active ? SW_SHOW : SW_HIDE;
    ShowWindow(coinsState.hLblDividends, show);
    ShowWindow(coinsState.hLblAccruals, show);
    ShowWindow(coinsState.hCoin_Accruals, show);
    ShowWindow(coinsState.hCoin_Dividends, show);
    ShowWindow(coinsState.hCoin_Cash, show);
    ShowWindow(coinsState.hLblEUR, show);
    ShowWindow(coinsState.hLblUSD, show);

    HWND controls[] = {
        coinsState.hCoinBox2, coinsState.hLblBP,
        coinsState.hCoin_BuyingPower, coinsState.hLblMM,
        coinsState.hCoin_MaintMargin, coinsState.hCoinBox3,
        coinsState.hCoin_EUR, coinsState.hCoin_USD
    };
    HDWP hdwp = BeginDeferWindowPos((int)std::size(controls));
    if (!hdwp) return;

    if (active) {
        hdwp = DeferWindowPos(hdwp, coinsState.hCoinBox2, NULL, m, y2, boxW, box2H, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hLblBP, NULL, m + 12, y2 + 80, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_BuyingPower, NULL, m + 105, y2 + 80, boxW - 117, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hLblMM, NULL, m + 12, y2 + 100, 85, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_MaintMargin, NULL, m + 97, y2 + 100, boxW - 109, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoinBox3, NULL, m, y3, boxW, box3H, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_EUR, NULL, m + 50, y3 + 20, boxW - 62, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_USD, NULL, m + 50, y3 + 40, boxW - 62, 18, SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        hdwp = DeferWindowPos(hdwp, coinsState.hCoinBox2, NULL, m, y2, boxW, box2H - 40, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hLblBP, NULL, m + 12, y2 + 40, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_BuyingPower, NULL, m + 105, y2 + 40, boxW - 117, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hLblMM, NULL, m + 12, y2 + 60, 85, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_MaintMargin, NULL, m + 97, y2 + 60, boxW - 109, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoinBox3, NULL, m, y3 - 40, boxW, box3H - 20, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_EUR, NULL, m + 12, y3 - 20, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
        hdwp = DeferWindowPos(hdwp, coinsState.hCoin_USD, NULL, m + 112, y3 - 20, boxW - 124, 18, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    EndDeferWindowPos(hdwp);

    LONG_PTR style = GetWindowLongPtr(coinsState.hCoin_EUR, GWL_STYLE);
    style &= ~SS_TYPEMASK;
    SetWindowLongPtr(coinsState.hCoin_EUR, GWL_STYLE, style | (active ? SS_RIGHT : SS_LEFT));

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);
    MoveWindow(hWnd, windowRect.left, windowRect.top, windowCoinsWidth,
               active ? windowCoinsHeight : windowCoinsHeight - 60 - 38, TRUE);
    coinsState.fullDetails = active;
}

#define ID_COIN_NETLIQ          1900
#define ID_COIN_BIGPNL          1901
#define ID_COIN_PCT             1902
#define ID_COIN_SPEAKER         1903
#define ID_COIN_LOCK            1904
#define ID_DASHFX_ACTION_COMBO  1905
#define ID_DASHFX_AMOUNT_EDIT   1906
#define TIMER_COINS_SPEAKER     1920   // WM_TIMER id

// ─── Exchange Currency popup (COINS_EXCHANGE_CLASS_NAME) ───────────────────

// ─── TTS helpers ──────────────────────────────────────────────────────────────
// Speech goes through the shared SharedTtsEngine (shared.h). The coins just
// tracks whether it currently holds a reference to it.

static void Coins_SpeakDailyPnL() {
    if (!coinsState.coinsTtsOn || !coinsState.hCoin_BigPnL) return;
    std::string text = EditText(coinsState.hCoin_BigPnL);
    std::wstring wtext(text.begin(), text.end());
    size_t dotPos = wtext.find(L'.');
    if (dotPos != std::wstring::npos) {
        wtext.erase(dotPos);
    }
    if (!wtext.empty() && wtext.front() == L'+') {
        wtext.erase(0, 1);
    }
    wtext.erase(std::remove(wtext.begin(), wtext.end(), L','), wtext.end());
    SharedTts().Speak(wtext);
}

static void Coins_ToggleTTS(HWND hWnd) {
    coinsState.coinsTtsOn = !coinsState.coinsTtsOn;

    // Persist state to registry
    RegSetDword(COINS_CLASS_NAME, "Speaker", coinsState.coinsTtsOn ? 1 : 0);

    if (coinsState.coinsTtsOn) {
        if (!SharedTts().Acquire()) {
            coinsState.coinsTtsOn = false;
            return;
        }
        coinsState.speakerColor = darkMode ? COINS_CLR_WHITE : COINS_CLR_BLACK;   // bright = active
        SetTimer(hWnd, TIMER_COINS_SPEAKER, 21000, NULL);
        Coins_SpeakDailyPnL();                          // speak immediately
    } else {
        KillTimer(hWnd, TIMER_COINS_SPEAKER);
        SharedTts().Release();                          // stop + tear down if we were the last one
        coinsState.speakerColor = COINS_CLR_GRAY;    // dim = inactive
    }

    if (coinsState.hCoin_Speaker) InvalidateRect(coinsState.hCoin_Speaker, NULL, TRUE);
}

// ─── Icons update ─────────────────────────────────────────────────────────────

struct IconUpdateContext {
    bool connected;
    const std::unordered_map<std::string, HICON>& onlineIcons;
    const std::unordered_map<std::string, HICON>& offlineIcons;
};

BOOL CALLBACK IconsEnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != GetCurrentProcessId()) return TRUE;

    IconUpdateContext* ctx = (IconUpdateContext*)lParam;

    char className[256];
    if (GetClassNameA(hwnd, className, sizeof(className)) > 0) {
        std::string key = className;

        // Safely check if this window's class exists in your icon maps
        auto itOnline = ctx->onlineIcons.find(key);
        auto itOffline = ctx->offlineIcons.find(key);

        if (itOnline != ctx->onlineIcons.end() && itOffline != ctx->offlineIcons.end()) {
            HICON hIcon = ctx->connected ? itOnline->second : itOffline->second;
            
            // Apply icons
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
        }
    }

    return TRUE; // Continue reading
}

void BindTrayIcon(HWND hWnd) {
    char classNameA[256];
    GetClassNameA(hWnd, classNameA, sizeof(classNameA));

    ZeroMemory(&nid, sizeof(NOTIFYICONDATAW));

    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TRAYICON;
    
    nid.hIcon = offlineIcons[std::string(classNameA)]; 

    wcscpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(wchar_t), L"Offline");

    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_ADD, &nid);
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

void UpdateTrayIcon(HWND hWnd) {
    std::string tooltip = std::string("Trading Floor") + ": ";
    bool connected = false;

    if (!api().isConnected()) {
        tooltip += "Disconnected";
    } else {
        std::string acc = api().getAccountNumber();
        if (acc.empty()) {
            tooltip += "Connecting..";
        } else {
            connected = api().isMarketDataConnected() && api().isTradingConnected();
            tooltip = acc + " | " + coinsState.currencyCoins + " | " + (connected ? "Connected" : "Disconnected");
        }
    }
    std::wstring wTooltip(tooltip.begin(), tooltip.end());
    wcsncpy(nid.szTip, wTooltip.c_str(), _countof(nid.szTip) - 1);
    nid.szTip[_countof(nid.szTip) - 1] = L'\0'; // Note the L prefix for wide null terminator

    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.hIcon  = connected ? onlineIcons[std::string(COINS_CLASS_NAME)] : offlineIcons[std::string(COINS_CLASS_NAME)];

    Shell_NotifyIconW(NIM_MODIFY, &nid);

    IconUpdateContext ctx = { connected, onlineIcons, offlineIcons };
    EnumWindows(IconsEnumWindowsProc, (LPARAM)&ctx);
}

// ─── Labels update ─────────────────────────────────────────────────────────────

void Coins_UpdateLabels(HWND hWnd) {
    auto   summary      = api().getAccountSummary();
    double daily        = api().getDailyPnL();
    double unrealized   = api().getUnrealizedPnL();
    double realized     = api().getRealizedPnL();
    int openOrdersCount = api().getOpenOrdersCount();

    std::string prevCurrency = coinsState.currencyCoins;
    coinsState.currencyCoins = "--";
    auto tryParse = [&](const std::string& k) -> double {
        auto it = summary.find(k);
        if (it == summary.end()) return 0.0;
        try { return std::stod(it->second); } catch (...) { return 0.0; }
    };
    if (summary.count("EUR_NetLiquidation")) {
        coinsState.currencyCoins = "EUR"; NetLiquidation = tryParse("EUR_NetLiquidation");
    } else if (summary.count("USD_NetLiquidation")) {
        coinsState.currencyCoins = "USD"; NetLiquidation = tryParse("USD_NetLiquidation");
    } else if (summary.count("NetLiquidation")) {
        NetLiquidation = tryParse("NetLiquidation");
        for (auto const& [k, v] : summary)
            if (k.find("_NetLiquidation") != std::string::npos)
                { coinsState.currencyCoins = k.substr(0, k.find('_')); break; }
    }

    if (coinsState.currencyCoins != prevCurrency)
        UpdateTrayIcon(hWnd);

    HDWP valueLabelMoves = BeginDeferWindowPos(4);

    if (coinsState.hCoin_NetLiq) {
        std::string formattedNum = FormatWithCommas(NetLiquidation, coinsState.fullDetails);
        if (SetWindowTextAIfChanged(coinsState.hCoin_NetLiq, formattedNum)) {
            int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
            if (valueLabelMoves)
                valueLabelMoves = DeferWindowPos(valueLabelMoves, coinsState.hCoin_NetLiq, NULL, m + 10 + 48, y1 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    if (coinsState.hCoin_Orders) {
        std::string formattedNum = std::to_string(openOrdersCount);
        if (SetWindowTextAIfChanged(coinsState.hCoin_Orders, formattedNum)) {
            int w2 = Coins_GetTextWidth(hWnd, hFont11ptbold.get(), formattedNum.c_str());
            if (valueLabelMoves)
                valueLabelMoves = DeferWindowPos(valueLabelMoves, coinsState.hCoin_Orders, NULL, m + boxW - 30, y2, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
            coinsState.orderColor = openOrdersCount ? COINS_CLR_YELLOW : COINS_CLR_GRAY;
        }
    }
    Coins_UpdateTaskbarOrders(hWnd, openOrdersCount);
    
    if (coinsState.hCoin_BigPnL) {
        std::string formattedNum = FormatWithCommas(daily);
        if (daily >= 0.0) formattedNum = "+" + formattedNum;
        SetSigned(coinsState.hCoin_BigPnL, daily, coinsState.bigPnlColor, formattedNum);
    }
    if (coinsState.hCoin_Pct) {
        double pct = (NetLiquidation != 0.0) ? (daily / NetLiquidation * 100.0) : 0.0;
        std::string formattedNum = FormatWithCommas(pct);
        if (pct >= 0.0) formattedNum = "+" + formattedNum;
        SetSigned(coinsState.hCoin_Pct, pct, coinsState.pctColor, std::format("{}%", formattedNum));
    }
    if (coinsState.hCoin_Realized) {
        SetSigned(coinsState.hCoin_Realized, realized, coinsState.realizedColor);
    }

    if (coinsState.hCoin_Positions) {
        double grossPos = tryParse("GrossPositionValue");
        std::string formattedNum = FormatWithCommas(grossPos, coinsState.fullDetails);
        if (SetWindowTextAIfChanged(coinsState.hCoin_Positions, formattedNum)) {
            int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
            if (valueLabelMoves)
                valueLabelMoves = DeferWindowPos(valueLabelMoves, coinsState.hCoin_Positions, NULL, m + 10 + 70, y2 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    if (coinsState.hCoin_Unrealized) {
        std::string formattedNum = FormatWithCommas(unrealized, coinsState.fullDetails);
        SetSigned(coinsState.hCoin_Unrealized, unrealized, coinsState.unrealizedColor, formattedNum);
    }
    if (coinsState.hCoin_Dividends) {
        double div = tryParse("AccruedDividend");
        std::string formattedNum = FormatWithCommas(div);
        if (SetWindowTextAIfChanged(coinsState.hCoin_Dividends, formattedNum)) {
            coinsState.dividendsColor = COINS_CLR_PURPLE;
        }
    }
    if (coinsState.hCoin_Accruals) {
        double acc = tryParse("AccruedCash");
        SetSigned(coinsState.hCoin_Accruals, acc, coinsState.accrualsColor);
    }
    if (coinsState.hCoin_BuyingPower) {
        double bp = tryParse("BuyingPower");
        std::string formattedNum = FormatWithCommas(bp, coinsState.fullDetails);
        SetWindowTextAIfChanged(coinsState.hCoin_BuyingPower, formattedNum);
    }
    if (coinsState.hCoin_MaintMargin) {
        double mm = tryParse("MaintMarginReq");
        std::string formattedNum = FormatWithCommas(mm, coinsState.fullDetails);
        SetWindowTextAIfChanged(coinsState.hCoin_MaintMargin, formattedNum);
    }

    if (coinsState.hCoin_Cash) {
        double cash = tryParse("BASE_CashBalance");
        std::string formattedNum = FormatWithCommas(cash, coinsState.fullDetails) + " " + coinsState.currencyCoins;
        if (SetSigned(coinsState.hCoin_Cash, cash, coinsState.cashColor, formattedNum)) {
            int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
            if (valueLabelMoves)
                valueLabelMoves = DeferWindowPos(valueLabelMoves, coinsState.hCoin_Cash, NULL, m + 10 + 45, y3 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    if (coinsState.hCoin_EUR) {
        double eur = tryParse("EUR_CashBalance");
        std::string formattedNum = FormatWithCommas(eur, coinsState.fullDetails) + (coinsState.fullDetails ? "" : " €");
        SetSigned(coinsState.hCoin_EUR, eur, coinsState.eurColor, StringToWide(formattedNum));
    }
    if (coinsState.hCoin_USD) {
        double usd = tryParse("USD_CashBalance");
        std::string formattedNum = FormatWithCommas(usd, coinsState.fullDetails) + (coinsState.fullDetails ? "" : " $");
        SetSigned(coinsState.hCoin_USD, usd, coinsState.usdColor, StringToWide(formattedNum));
    }
    if (valueLabelMoves) EndDeferWindowPos(valueLabelMoves);
}

static LRESULT CALLBACK CoinsButton_CursorSubclassProc(HWND hButton, UINT msg, WPARAM wParam, LPARAM lParam,
                                                            UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_SETCURSOR && LOWORD(lParam) == HTCLIENT) {
        SetCursor(LoadCursor(NULL, IDC_HAND));
        return TRUE;
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hButton, CoinsButton_CursorSubclassProc, uIdSubclass);
    return DefSubclassProc(hButton, msg, wParam, lParam);
}

void addButtons(HWND hWnd, HINSTANCE hInst, LPCSTR buttonText, int x, int y, HMENU menuId, int iconId) {
		// Create the button
        HWND hBtn = CreateWindow(
            "BUTTON", buttonText,
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            x, y, 26, 26,
            hWnd, menuId, hInst, NULL
        );
        SetWindowSubclass(hBtn, CoinsButton_CursorSubclassProc, 1, 0);
        // Store the icon via SetProp so WM_DRAWITEM can retrieve it with GetProp.
        // (BM_GETIMAGE is unreliable without BS_ICON in the style.)
        HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(iconId), IMAGE_ICON, 24, 24, LR_DEFAULTCOLOR);
        SetProp(hBtn, "hIcon", (HANDLE)hIcon);

        // Add tooltip
        HWND hTip = CreateWindowA(TOOLTIPS_CLASS, NULL,
            WS_POPUP | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hWnd, NULL, hInst, NULL);

        TOOLINFOA ti = {};
        ti.cbSize   = sizeof(ti);
        ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd     = hWnd;
        ti.uId      = (UINT_PTR)hBtn;
        ti.lpszText = (LPSTR)buttonText;
        SendMessage(hTip, TTM_ADDTOOLA, 0, (LPARAM)&ti);
}

// ─── Exchange Currency (FX conversion) popup ─────────────────────────────────
// A small single-instance popup (see COINS_EXCHANGE_CLASS_NAME) that shows a
// live EUR.USD rate in its title bar and lets the user submit a market order
// converting between USD and EUR on IDEALPRO. Opened from the 🗘 icon at the
// right of the last account-summary row.

static void CoinsFx_UpdateTitle(HWND hWnd) {
    TradingAPI::L1Book fx;
    api().getFxRate(fx);

    double rate = 0.0;
    if      (fx.bid > 0.0 && fx.ask > 0.0) rate = (fx.bid + fx.ask) / 2.0;
    else if (fx.last > 0.0)                rate = fx.last;
    else if (fx.bid > 0.0)                 rate = fx.bid;
    else if (fx.ask > 0.0)                 rate = fx.ask;

    HWND hCombo = GetDlgItem(hWnd, ID_DASHFX_ACTION_COMBO);
    HWND hEdit  = GetDlgItem(hWnd, ID_DASHFX_AMOUNT_EDIT);
    int sel = hCombo ? (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0) : 0;
    bool isBuy = (sel != 1);   // 0 = BUY (USD->EUR), 1 = SELL (EUR->USD)

    double amount = EditNum(hEdit);

    std::string title = "Exchange: ";
    if (rate > 0.0) {
        if (amount > 0.0) {
            title += std::format("{:.2f} ", amount);
            title += isBuy ? "USD" : "EUR";
            if (isBuy) {
                double receivedEUR = amount / rate;   // spending `amount` USD
                title += " = " + FormatWithCommas(receivedEUR) + " EUR";
            } else {
                double receivedUSD = amount * rate;   // spending `amount` EUR
                title += " = " + FormatWithCommas(receivedUSD) + " USD";
            }
        } else {
            title += std::format("1 EUR = {:.5f} USD", rate);
        }
    } else {
        title += "waiting for rate..";
    }
    SetWindowTextA(hWnd, title.c_str());
}

// Reads the selector + amount, submits the conversion order (if valid), then
// closes the popup. order.totalQuantity for a CASH/IDEALPRO contract is always
// denominated in the base currency (EUR), so a BUY (USD->EUR) amount typed in
// USD is converted to its EUR notional before being sent.
static void CoinsFx_SubmitAndClose(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, ID_DASHFX_ACTION_COMBO);
    HWND hEdit  = GetDlgItem(hWnd, ID_DASHFX_AMOUNT_EDIT);
    int sel = hCombo ? (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0) : 0;
    bool isBuy = (sel != 1);

    double amount = EditNum(hEdit);

    if (amount > 2.0) {
        TradingAPI::L1Book fx;
        api().getFxRate(fx);
        double rate = 0.0;
        if      (fx.bid > 0.0 && fx.ask > 0.0) rate = (fx.bid + fx.ask) / 2.0;
        else if (fx.last > 0.0)                rate = fx.last;
        else if (fx.bid > 0.0)                 rate = fx.bid;
        else if (fx.ask > 0.0)                 rate = fx.ask;

        if (rate > 0.0) {
            double quantityEUR = isBuy ? (amount / rate) : amount;
            api().submitCurrencyOrder(isBuy ? "BUY" : "SELL", quantityEUR);
        }
    }
    DestroyWindow(hWnd);
}

// Subclass shared by the action combo and the amount edit: ESC closes without
// submitting, ENTER submits and closes.
static LRESULT CALLBACK CoinsFx_KeySubclassProc(HWND hCtrl, UINT msg, WPARAM wParam, LPARAM lParam,
                                                     UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_CHAR) {
        if (!IsAllowedNumericEditCharacter(wParam))
            return 0;
    }
    if (msg == WM_KEYDOWN) {
        HWND hParent = GetParent(hCtrl);
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hParent);
            return 0;
        }
        if (wParam == VK_RETURN) {
            CoinsFx_SubmitAndClose(hParent);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hCtrl, CoinsFx_KeySubclassProc, uIdSubclass);
    return DefSubclassProc(hCtrl, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcExchange(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            HWND hCombo = CreateWindowA("COMBOBOX", "",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                10, 10, 140, 200, hWnd, (HMENU)ID_DASHFX_ACTION_COMBO, hInst, NULL);
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"USD to EUR"); // BUY
            SendMessageA(hCombo, CB_ADDSTRING, 0, (LPARAM)"EUR to USD"); // SELL
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);

            CreateWindowA("STATIC", "Amount:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                160, 14, 55, 18, hWnd, NULL, hInst, NULL);

            HWND hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "0.00",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER,
                218, 10, 90, 24, hWnd, (HMENU)ID_DASHFX_AMOUNT_EDIT, hInst, NULL);

            SetWindowSubclass(hCombo, CoinsFx_KeySubclassProc, 1, 0);
            SetWindowSubclass(hEdit,  CoinsFx_KeySubclassProc, 2, 0);

            api().reqFxRate(hWnd);
            CoinsFx_UpdateTitle(hWnd);

            SetFocus(hEdit);
            int len = GetWindowTextLengthA(hEdit);
            SendMessageA(hEdit, EM_SETSEL, 0, len);
            break;
        }

        case WM_FX_RATE_UPDATE:
            CoinsFx_UpdateTitle(hWnd);
            break;

        case WM_COMMAND:
            if ((LOWORD(wParam) == ID_DASHFX_ACTION_COMBO && HIWORD(wParam) == CBN_SELCHANGE) ||
                (LOWORD(wParam) == ID_DASHFX_AMOUNT_EDIT  && HIWORD(wParam) == EN_CHANGE)) {
                CoinsFx_UpdateTitle(hWnd);
            }
            break;

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }
        case WM_DESTROY:
            api().cancelFxRate(hWnd);
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
// ─── Lock keyword popup ───────────────────────────────────────────────────
//
// Single-instance popup (same pattern as ALERTS_EDITOR_CLASS_NAME) used to gate
// unlocking the coins hotkeys. Locking never needs the keyword, only
// unlocking does, so this window is only ever opened on the "unlock" path,
// either from WndProcCoins's Scroll Lock handler or, at startup, from
// PromptLockAtStartup() before the rest of the app is even created.

#define ID_LOCK_EDIT 5501

struct LockState {
    bool startupMode = false; // true while gating app startup (see PromptLockAtStartup)
    bool success      = false; // set true the moment the correct keyword is submitted
};
static LockState lockState;

// Compares the entered keyword against the saved one.
//   - Correct: marks success, performs the coins unlock (unless we're in
//     startup mode, where nothing has been hidden yet), then closes.
//   - Incorrect: shows the same error box as before and leaves the popup open
//     so the user can retry.
static void Lock_TrySubmit(HWND hWnd) {
    HWND hEdit = GetDlgItem(hWnd, ID_LOCK_EDIT);
    std::string text = EditText(hEdit);

    if (lockScreen != text) {
        MessageBoxA(hWnd, "Incorrect Lock Keyword", "Lock Keyword Required", MB_ICONERROR);
        if (hEdit) {
            SetWindowTextA(hEdit, "");
            SetFocus(hEdit);
        }
        return;
    }

    lockState.success = true;

    if (!lockState.startupMode) {
        // Runtime unlock: reverse exactly what the "lock" branch in
        // WndProcCoins's WM_KEYDOWN did.
        lockHotkeys = false;
        ShowWindow(coinsState.hCoin_Lock, SW_HIDE);
        HWND hCoins = FindWindowA(COINS_CLASS_NAME, NULL);
        if (hCoins && IsWindow(hCoins))
            PostMessage(hCoins, WM_ACTIVATE, WA_ACTIVE, 0);
        ToggleTWS(SW_SHOW);
    }
    // Startup mode: nothing to reverse yet, just report success to
    // PromptLockAtStartup() via lockState.success and close.

    DestroyWindow(hWnd);
}

// ESC cancels without unlocking. At startup this means the app never opens
// (see PromptLockAtStartup); at runtime it just leaves the coins locked.
static LRESULT CALLBACK LockEdit_KeySubclassProc(HWND hCtrl, UINT msg, WPARAM wParam, LPARAM lParam,
                                                  UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_CHAR) {
        if (wParam == VK_RETURN || wParam == VK_ESCAPE)
            return 0;
    }
    if (msg == WM_KEYDOWN) {
        HWND hParent = GetParent(hCtrl);
        if (wParam == VK_RETURN) {
            Lock_TrySubmit(hParent);
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hParent);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hCtrl, LockEdit_KeySubclassProc, uIdSubclass);
    return DefSubclassProc(hCtrl, msg, wParam, lParam);
}

LRESULT CALLBACK WndProcLock(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            CreateWindowA("STATIC", "Enter Lock Keyword:",
                WS_CHILD | WS_VISIBLE | SS_CENTER,
                10, 14, 220, 20, hWnd, NULL, hInst, NULL);

            HWND hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER | ES_PASSWORD,
                20, 42, 200, 26, hWnd, (HMENU)ID_LOCK_EDIT, hInst, NULL);
            SetWindowSubclass(hEdit, LockEdit_KeySubclassProc, 1, 0);

            SetFocus(hEdit);
            break;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;

        case WM_DESTROY:
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

void StartLock() {
    lockState.success = false;

    HWND hWnd = StartGenericWindow(LOCK_CLASS_NAME, "Enter Lock Keyword", L"TWSAPIClientTradingFloor.Lock", 245, 110);
    if (!hWnd) return;

    HWND hEdit = GetDlgItem(hWnd, ID_LOCK_EDIT);
    if (hEdit) {
        SetWindowTextA(hEdit, "");
        SetFocus(hEdit);
    }
}

// Blocks by running its own message loop, WinMain hasn't started the real
// one yet, and nothing else exists at this point, so this is safe, until
// the Lock popup is dismissed. Returns true iff the correct keyword was
// entered; false on ESC/close, meaning the caller should abort startup.
static bool PromptLockAtStartup() {
    lockState.startupMode = true;
    lockState.success     = false;

    StartLock();

    HWND hLockWnd = FindWindowA(LOCK_CLASS_NAME, NULL);
    MSG msg;
    while (hLockWnd && IsWindow(hLockWnd) && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    lockState.startupMode = false;
    return lockState.success;
}

LRESULT CALLBACK WndProcCoins(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == taskbarCreatedMessage) {
        Shell_NotifyIconW(NIM_ADD, &nid);
        nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &nid);
        return 0;
    }

    switch (message) {
        case WM_CREATE:	{
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            // ─── Box 1: Net Liq & PnL ──────────────────────────────────────────
            coinsState.hCoinBox1 = GroupBox(hWnd, "Coins:", m, y1, boxW, box1H, 1, WS_CLIPSIBLINGS);

            coinsState.hCoin_NetLiq = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 62, y1 - 4, 30, 18, ID_COIN_NETLIQ, hFont14ptbold.get());

            // Lock icon (hidden by default)
            coinsState.hCoin_Lock = CreateWindowW(L"STATIC", LOCK_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + boxW - 90, y1, 20, 18, hWnd, (HMENU)ID_COIN_LOCK, hInst, NULL);
            SendMessage(coinsState.hCoin_Lock, WM_SETFONT, (WPARAM)hFont_Icons.get(), TRUE);
            ShowWindow(coinsState.hCoin_Lock, SW_HIDE);

            coinsState.hCoin_Clock = Ctl(hWnd, "STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY,
                m + boxW - 60, y1, 40, 18, 0, hFont11pt.get());

            // Row 1: PnL: 🔊 +0.00
            Ctl(hWnd, "STATIC", "PnL:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 24, 30, 18, 0, hFont11pt.get());

            coinsState.hCoin_Speaker = CreateWindowW(L"STATIC", SPEAKER_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + 42, y1 + 24, 20, 20, hWnd, (HMENU)ID_COIN_SPEAKER, hInst, NULL);
            SendMessage(coinsState.hCoin_Speaker, WM_SETFONT, (WPARAM)hFont_Icons.get(), TRUE);

            coinsState.hCoin_BigPnL = Ctl(hWnd, "STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOTIFY,
                m + 70, y1 + 16, boxW - 82, 32, ID_COIN_BIGPNL, hFont21ptbold.get());

            // Row 2: PnL %: +0.00%
            Ctl(hWnd, "STATIC", "PnL %:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 48, 50, 18, 0, hFont11pt.get());

            coinsState.hCoin_Pct = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 70, y1 + 48, boxW - 82, 18, ID_COIN_PCT, hFont12ptbold.get());

            // Row 3: Realized: 0.00
            Ctl(hWnd, "STATIC", "Realized:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 68, 65, 18, 0, hFont11pt.get());

            coinsState.hCoin_Realized = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 77, y1 + 68, boxW - 89, 18, 0, hFont12ptbold.get());


            // ─── Box 2: Positions & Margin ─────────────────────────────────────
            coinsState.hCoinBox2 = GroupBox(hWnd, "Positions:", m, y2, boxW, box2H, 2, WS_CLIPSIBLINGS);

            coinsState.hCoin_Positions = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 77, y2 - 4, 30, 18, 0, hFont14ptbold.get());

            coinsState.hCoin_Orders = Ctl(hWnd, "STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY,
                m + boxW - 30, y2, 20, 18, 0, hFont11pt.get());

            // Row 1: Unrealized: 0.00
            Ctl(hWnd, "STATIC", "Unrealized:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 20, 75, 18, 0, hFont11pt.get());

            coinsState.hCoin_Unrealized = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 20, boxW - 102, 18, 0, hFont12ptbold.get());

            // Row 2: Dividends: 33.75
            coinsState.hLblDividends = Ctl(hWnd, "STATIC", "Dividends:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 40, 75, 18, 0, hFont11pt.get());

            coinsState.hCoin_Dividends = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 40, boxW - 102, 18, 0, hFont12ptbold.get());

            // Row 3: Accruals: -1.64
            coinsState.hLblAccruals = Ctl(hWnd, "STATIC", "Accruals:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 60, 75, 18, 0, hFont11pt.get());

            coinsState.hCoin_Accruals = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 60, boxW - 102, 18, 0, hFont12ptbold.get());

            // Row 4: Buying Power: 86,483.04
            coinsState.hLblBP = Ctl(hWnd, "STATIC", "Buying Power:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 80, 100, 18, 0, hFont11pt.get());

            coinsState.hCoin_BuyingPower = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 105, y2 + 80, boxW - 117, 18, 0, hFont12ptbold.get());

            // Row 5: Maint Margin: 4,403.05
            coinsState.hLblMM = Ctl(hWnd, "STATIC", "Maintenance:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 100, 85, 18, 0, hFont11pt.get());

            coinsState.hCoin_MaintMargin = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 97, y2 + 100, boxW - 109, 18, 0, hFont12ptbold.get());


            // ─── Box 3: Cash ───────────────────────────────────────────────────
            coinsState.hCoinBox3 = GroupBox(hWnd, "Cash:", m, y3, boxW, box3H, 3, WS_CLIPSIBLINGS);

            coinsState.hCoin_Cash = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 53, y3 - 4, 30, 18, 0, hFont14ptbold.get());

            // Row 1: EUR: 285.31
            coinsState.hLblEUR = Ctl(hWnd, "STATIC", "EUR:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y3 + 20, 35, 18, 0, hFont11pt.get());

            coinsState.hCoin_EUR = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 50, y3 + 20, boxW - 62, 18, 0, hFont12ptbold.get());

            // Row 2: USD: -11,559.66
            coinsState.hLblUSD = Ctl(hWnd, "STATIC", "USD:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y3 + 40, 35, 18, 0, hFont11pt.get());

            coinsState.hCoin_USD = Ctl(hWnd, "STATIC", "--", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 50, y3 + 40, boxW - 62, 18, 0, hFont12ptbold.get());


            // ─── Bottom action buttons ──────────────────────────────────────────
            int yBtn = y3 + box3H + 10; // yBtn = 326
            int steps = 1;
            int stepz = 0;
            addButtons(hWnd, hInst, "Diamonds",  (9 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_DIAMONDS,  104);
            addButtons(hWnd, hInst, "Orders",    (9 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_ORDERS,    109);
            
            addButtons(hWnd, hInst, "Market",    10 + 9 + (9 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_MARKET,    105);
            addButtons(hWnd, hInst, "Exchange",  10 + 9 + (9 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_EXCHANGE,  106);

            addButtons(hWnd, hInst, "Settings", 24 + 18 + (9 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_SETTINGS,  107);

            api().addApiUpdateWindow(hWnd);

            if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
                coinsState.taskbarComInitialized = true;
            if (SUCCEEDED(CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
                                           IID_PPV_ARGS(&coinsState.taskbar)))) {
                coinsState.taskbar->HrInit();
                coinsState.taskbar->SetProgressState(hWnd, TBPF_NOPROGRESS);
            }

            BindTrayIcon(hWnd);

            if (RegGetDword(COINS_CLASS_NAME, "Speaker", 0)) {
                Coins_ToggleTTS(hWnd);
            }
            
            SetTimer(hWnd, TIMER_MARKET_CLOCK, 21000, NULL); // live market clock, intentionaly delayed 21 seconds
            SendMessage(hWnd, WM_TIMER, TIMER_MARKET_CLOCK, 0);

            SetTimer(hWnd, TIMER_WATCHDOG, 10000, NULL);
            SetTimer(hWnd, TIMER_WATCHDOG_DELAYED_START, 721, NULL);

            break;
        }
        
        case WM_ACTIVATE: {
            if (LOWORD(wParam) != WA_INACTIVE) {
                if (lockHotkeys) break;
                Coins_ApplyFocusLayout(hWnd, true);
            } else {
                Coins_ApplyFocusLayout(hWnd, false);
            }
            if (api().isMarketDataConnected() && api().isTradingConnected()) {
                Coins_UpdateLabels(hWnd);
            }
            break;
        }

        case WM_API_UPDATE: // only called when the api connects or disconnects
            if (!api().isMarketDataConnected() || !api().isTradingConnected()) {
                const HWND labels[] = {
                    coinsState.hCoin_NetLiq, coinsState.hCoin_BigPnL, coinsState.hCoin_Pct,
                    coinsState.hCoin_Realized, coinsState.hCoin_Positions, coinsState.hCoin_Unrealized,
                    coinsState.hCoin_Dividends, coinsState.hCoin_Accruals, coinsState.hCoin_BuyingPower,
                    coinsState.hCoin_MaintMargin, coinsState.hCoin_Cash, coinsState.hCoin_EUR,
                    coinsState.hCoin_USD,
                };
                for (HWND label : labels) {
                    if (label) SetWindowTextA(label, "--");
                }
                if (coinsState.hCoin_NetLiq) {
                    SetWindowPos(coinsState.hCoin_NetLiq, NULL, m + 67, y1 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                    InvalidateRect(coinsState.hCoin_NetLiq, NULL, TRUE);
                }
                if (coinsState.hCoin_Positions) {
                    SetWindowPos(coinsState.hCoin_Positions, NULL, m + 77, y2 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                    InvalidateRect(coinsState.hCoin_Positions, NULL, TRUE);
                }
                if (coinsState.hCoin_Cash) {
                    SetWindowPos(coinsState.hCoin_Cash, NULL, m + 53, y3 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                    InvalidateRect(coinsState.hCoin_Cash, NULL, TRUE);
                }
                coinsState.currencyCoins = "--";
                Coins_UpdateTaskbarOrders(hWnd, 0);
            }
            UpdateTrayIcon(hWnd);
            break;
            
        case WM_API_LOG: {
            std::string* msg = (std::string*)lParam;
            LogDebug(msg->c_str());
            delete msg;
            break;
        }

        case WM_ACCOUNT_SUMMARY:
            api().ackAccountSummary();
            Coins_UpdateLabels(hWnd);
            break;

        case WM_SETCURSOR: {
            int id = GetDlgCtrlID((HWND)wParam);
            if  (id == ID_COIN_SPEAKER || id == ID_COIN_BIGPNL) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            break; 
        }
        case WM_TIMER:
            if (wParam == TIMER_MARKET_CLOCK) {
                UpdateMarketClock(hWnd);
            }
            if (wParam == TIMER_COINS_SPEAKER) // 21000
                Coins_SpeakDailyPnL();
            if (wParam == TIMER_WATCHDOG || wParam == TIMER_WATCHDOG_DELAYED_START) { // 10000
                if (wParam == TIMER_WATCHDOG_DELAYED_START)
                    KillTimer(hWnd, TIMER_WATCHDOG_DELAYED_START);
                if (coinsState.shouldBeConnected && !api().isConnected()) {
                    if (EnsureGatewayRunning(hWnd)) {
                        EnsureGatewayLoggedIn(hWnd);
                    }
                    int port = std::filesystem::path(pathGateway).filename() == "ibgateway.exe" ? 4001 : 7496;
                    api().connect(addressGateway, port , clientIdGateway, groupIdGateway);
                } else if (!coinsState.shouldBeConnected && api().isConnected()) {
                    api().disconnect();
                }
            }
            break;

        case WM_OPEN_ORDERS_WINDOW:
            StartOrders();
            break;

        case WM_TTS_VOICE_CHANGED: {
            // The shared engine holds one voice for every window now, just
            // re-apply the (now-changed) saved voice token to it in place.
            SharedTts().ReapplySavedVoice();
            if (coinsState.coinsTtsOn) Coins_SpeakDailyPnL(); // speak immediately with the new voice
            break;
        }

        case WM_SHOW_ALERT: {
            AlertPopupData* data = (AlertPopupData*)lParam;
            if (data) {
                HWND hwnd = StartGenericWindow(ALERT_NOTIFY_CLASS_NAME, data->title.c_str(), L"Alert Notification", 300, 127, NULL, "", data);
                if (!hwnd || (AlertPopupData*)GetWindowLongPtr(hwnd, GWLP_USERDATA) != data) {
                    delete data;
                } else {
                    FlashScreen(data->isUp, 1000);
                    std::string price = FormatFixed(data->price, 2);
                    COLORREF color = data->isUp ? COINS_CLR_GREEN_DARK : COINS_CLR_RED_DARK;
                    Diamonds_AddTitleEvent(data->symbol, data->conId, color, price);
                    PlaySound_Async(209);
                }
            }
            return 0;
        }

        case WM_ADD_EVENT: {
            TradingAPI::EventData* data = (TradingAPI::EventData*)lParam;
            if (data) {
                Diamonds_AddTitleEvent(data->symbol, data->conId, data->color, data->text);
                if (data->color != COINS_CLR_BLUE) PlaySound_Async(210, false);
                delete data;
            }
            return 0;
        }

        case WM_RBUTTONUP: {
            if (lockHotkeys) break;

            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            for (int i = 0; i < LINKS_COUNT; ++i) {
                if (strcmp(quickLinks[i].label, "Month") == 0 || strcmp(quickLinks[i].label, "Portal") == 0)
                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuA(hMenu, MF_STRING, ID_M_LINKS_BASE + i, quickLinks[i].label);   
            }

            SetForegroundWindow(hWnd);
            int cmd = TrackPopupMenu(hMenu,
                TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
                pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);

            if (cmd >= ID_M_LINKS_BASE && cmd < ID_M_LINKS_BASE + LINKS_COUNT) {
                ShellExecuteA(NULL, "open", quickLinks[cmd - ID_M_LINKS_BASE].url, NULL, NULL, SW_SHOWNORMAL);
            }

            break;
        }

        case WM_TRAYICON: {
            WORD trayEvent = LOWORD(lParam);
            if (trayEvent == WM_LBUTTONUP) {
                if (IsIconic(hWnd)) { 
                    ShowWindow(hWnd, SW_RESTORE);
                } else {
                    ShowWindow(hWnd, SW_SHOW);
                }
                
                SetForegroundWindow(hWnd);
                SetActiveWindow(hWnd);
                SetFocus(hWnd);
            }
            else if (trayEvent == WM_RBUTTONUP || trayEvent == WM_CONTEXTMENU) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                
                bool bKeepMenuAlive = true;
                
                while (bKeepMenuAlive) {
                    HMENU hMenu = CreatePopupMenu();
                    
                    // Determine flags based on current API state
                    if (api().isConnected()) {
                        std::wstring accText = std::wstring(L"Account: ") + StringToWide(api().getAccountNumber()); 
                        AppendMenuW(hMenu, (MF_STRING | MF_GRAYED), 0, accText.c_str());
                        AppendMenuW(hMenu, MF_STRING, ID_M_DISCONNECT, L"Disconnect");
                    } else {
                        AppendMenuW(hMenu, MF_STRING, ID_M_CONNECT, L"Connect");
                    }
                    
                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

                    // Re-evaluate window states on every loop iteration
                    int hasSubmenus = 0;
                    if (hWnd && IsWindowVisible(hWnd)) { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_COINS, IsWindowAlwaysOnTop(COINS_CLASS_NAME) ? L"[ ★ ] Coins" : L"[  ] Coins"); }
                    if (FindWindowA(DIAMONDS_CLASS_NAME, NULL))  { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_DIAMONDS,  IsWindowAlwaysOnTop(DIAMONDS_CLASS_NAME)  ? L"[ ★ ] Diamonds"  : L"[  ] Diamonds"); }
                    if (FindWindowA(ORDERS_CLASS_NAME, NULL))    { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_ORDERS,    IsWindowAlwaysOnTop(ORDERS_CLASS_NAME)    ? L"[ ★ ] Orders"    : L"[  ] Orders"); }

                    struct OpenMarketItem { HWND hWnd; std::string symbol; };
                    std::vector<OpenMarketItem> openMarkets;
                    openMarkets.reserve(marketStates.size());
                    for (const auto& [h, st] : marketStates) {
                        if (st && !st->symbol.empty()) openMarkets.push_back({h, st->symbol});
                    }
                    std::sort(openMarkets.begin(), openMarkets.end(), [](const auto& a, const auto& b) {
                        return a.symbol < b.symbol;
                    });
                    for (size_t i = 0; i < openMarkets.size() && i < ID_M_MARKET_MAX; ++i) {
                        std::wstring label = IsMarketAlwaysOnTop(openMarkets[i].symbol) ? 
                            L"[ ★ ] Market: " + StringToWide(openMarkets[i].symbol) : 
                            L"[  ] Market: " + StringToWide(openMarkets[i].symbol);
                            
                        hasSubmenus++;
                        AppendMenuW(hMenu, MF_STRING, ID_M_MARKET_BASE + (int)i, label.c_str());
                    }
                    
                    if (FindWindowA(SETTINGS_CLASS_NAME, NULL))  { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_SETTINGS,  IsWindowAlwaysOnTop(SETTINGS_CLASS_NAME)  ? L"[ ★ ] Settings"  : L"[  ] Settings"); }
                    if (FindWindowA(DEBUGLOG_CLASS_NAME, NULL))  { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_DEBUGLOG,  IsWindowAlwaysOnTop(DEBUGLOG_CLASS_NAME)  ? L"[ ★ ] Debug Log" : L"[  ] Debug Log"); }

                    if (hasSubmenus) AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

                    AppendMenuW(hMenu, MF_STRING, ID_M_EXIT, L"Exit");

                    int selectedCmd = TrackPopupMenu(hMenu, 
                        TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, 
                        pt.x, pt.y, 0, hWnd, NULL);

                    switch (selectedCmd) {
                        case 0:
                            // User clicked completely outside the menu
                            bKeepMenuAlive = false;
                            break;

                        // GROUP A: The Toggle Items (Menu stays open)
                        case ID_M_COINS:
                        case ID_M_DIAMONDS:
                        case ID_M_ORDERS:
                        case ID_M_SETTINGS:
                        case ID_M_DEBUGLOG:
                            SendMessage(hWnd, WM_COMMAND, selectedCmd, 0);
                            break; // Do NOT set bKeepMenuAlive to false

                        // GROUP B: Action Items (Menu closes)
                        case ID_M_CONNECT:
                        case ID_M_DISCONNECT:
                        case ID_M_EXIT:
                            if (selectedCmd != 0) {
                                PostMessage(hWnd, WM_COMMAND, selectedCmd, 0);
                            }
                            bKeepMenuAlive = false;
                            break;
                        
                        // Handle dynamically generated Market menu items
                        default:
                            if (selectedCmd >= ID_M_MARKET_BASE && selectedCmd < ID_M_MARKET_BASE + ID_M_MARKET_MAX) {
                                SendMessage(hWnd, WM_COMMAND, selectedCmd, 0);
                                // Menu stays open
                            } else if (selectedCmd != 0) {
                                PostMessage(hWnd, WM_COMMAND, selectedCmd, 0);
                                bKeepMenuAlive = false;
                            }
                            break;
                    }

                    DestroyMenu(hMenu);
                }
            }
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_SCROLL) {
                if (lockHotkeys) {
                    // Currently locked -> unlocking requires the saved keyword.
                    // Nothing changes here until Lock_TrySubmit() confirms it;
                    // if no keyword is configured, unlock immediately instead.
                    if (lockScreen.empty()) {
                        lockHotkeys = false;
                        ShowWindow(coinsState.hCoin_Lock, SW_HIDE);
                        PostMessage(hWnd, WM_ACTIVATE, WA_ACTIVE, 0);
                        ToggleTWS(SW_SHOW);
                    } else {
                        StartLock();
                    }
                } else {
                    // Locking never requires the keyword.
                    lockHotkeys = true;
                    ShowWindow(coinsState.hCoin_Lock, SW_SHOW);
                    PostMessage(hWnd, WM_ACTIVATE, WA_INACTIVE, 0);
                    ToggleTWS(SW_HIDE);
                }
                return 0;
            }
            break;
        }

        case WM_SHOWWINDOW:
            if (!lockHotkeys && wParam == TRUE) {
                ToggleTWS(SW_SHOW); 
            }
            break;

        case WM_CTLCOLORSTATIC: {
            HWND hCtrl = (HWND)lParam;
            COLORREF clr = ThemeText();
            if (hCtrl == coinsState.hCoin_Clock) clr = coinsState.clockColor;
            else if (hCtrl == coinsState.hCoin_Speaker) clr = coinsState.speakerColor;
            else if (hCtrl == coinsState.hCoin_BigPnL) clr = coinsState.bigPnlColor;
            else if (hCtrl == coinsState.hCoin_Pct) clr = coinsState.pctColor;
            else if (hCtrl == coinsState.hCoin_Realized) clr = coinsState.realizedColor;
            else if (hCtrl == coinsState.hCoin_Unrealized) clr = coinsState.unrealizedColor;
            else if (hCtrl == coinsState.hCoin_Dividends) clr = coinsState.dividendsColor;
            else if (hCtrl == coinsState.hCoin_Accruals) clr = coinsState.accrualsColor;
            else if (hCtrl == coinsState.hCoin_Cash) clr = coinsState.cashColor;
            else if (hCtrl == coinsState.hCoin_EUR) clr = coinsState.eurColor;
            else if (hCtrl == coinsState.hCoin_USD) clr = coinsState.usdColor;
            else if (hCtrl == coinsState.hCoin_Lock) clr = coinsState.lockColor;
            else if (hCtrl == coinsState.hCoin_Orders) clr = coinsState.orderColor;

            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, clr);
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)ThemeBrush();
        }

        case WM_COMMAND: {
            WORD id  = LOWORD(wParam);
            WORD evt = HIWORD(wParam);
            switch (id) {
                case ID_COIN_SPEAKER:
                case ID_COIN_BIGPNL:
                    if (evt == STN_CLICKED)
                        Coins_ToggleTTS(hWnd);
                    break;
                case ID_M_CONNECT:
                    coinsState.shouldBeConnected = true;
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_DISCONNECT:
                    coinsState.shouldBeConnected = false;
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_EXIT_SOFT:
                    killGatewayOnExit = false;
                case ID_M_EXIT:
                    DestroyWindow(hWnd);
                    break;
                    
                case ID_MB_DIAMONDS:
                    StartDiamonds();
                    break;
                case ID_MB_EXCHANGE:
                    StartGenericWindow(COINS_EXCHANGE_CLASS_NAME, "Exchange", L"TWSAPIClientTradingFloor.ExchangeCurrency", 320, 70);
                    break;
                case ID_MB_MARKET:
                    StartMarket();
                    break;
                case ID_MB_SETTINGS:
                    StartSettings();
                    break;
                case ID_MB_ORDERS:
                    StartOrders();
                    break;        
                case ID_M_COINS:
                    ToggleWindowAlwaysOnTop(COINS_CLASS_NAME);
                    break;
                case ID_M_DIAMONDS:
                    ToggleWindowAlwaysOnTop(DIAMONDS_CLASS_NAME);
                    break;
                case ID_M_SETTINGS:
                    ToggleWindowAlwaysOnTop(SETTINGS_CLASS_NAME);
                    break;
                case ID_M_DEBUGLOG:
                    ToggleWindowAlwaysOnTop(DEBUGLOG_CLASS_NAME);
                    break;
                case ID_M_ORDERS:
                    ToggleWindowAlwaysOnTop(ORDERS_CLASS_NAME);
                    break;
                
                default:
                    if (LOWORD(wParam) >= ID_M_MARKET_BASE && LOWORD(wParam) < ID_M_MARKET_BASE + ID_M_MARKET_MAX) {
                        struct OpenMarketItem { HWND hWnd; std::string symbol; };
                        std::vector<OpenMarketItem> openMarkets;
                        openMarkets.reserve(marketStates.size());
                        for (const auto& [h, st] : marketStates) {
                            if (st && !st->symbol.empty()) openMarkets.push_back({h, st->symbol});
                        }
                        std::sort(openMarkets.begin(), openMarkets.end(), [](const auto& a, const auto& b) {
                            return a.symbol < b.symbol;
                        });
                        int index = LOWORD(wParam) - ID_M_MARKET_BASE;
                        if (index >= 0 && index < (int)openMarkets.size()) {
                            ToggleMarketAlwaysOnTop(openMarkets[index].hWnd, openMarkets[index].symbol);
                        }
                    }
                    break;
            }
            break;
        }
        case WM_DESTROY:
            api().disconnect();
            KillGateway();
            api().removeApiUpdateWindow(hWnd);
            Shell_NotifyIconW(NIM_DELETE, &nid);
            // Stop TTS
            if (coinsState.coinsTtsOn) {
                KillTimer(hWnd, TIMER_COINS_SPEAKER);
                SharedTts().Release();
            }
            if (coinsState.taskbar) {
                coinsState.taskbar->SetProgressState(hWnd, TBPF_NOPROGRESS);
                coinsState.taskbar->Release();
            }
            if (coinsState.taskbarComInitialized) {
                CoUninitialize();
            }

            coinsState = {};

            Settings_Market_CleanupOldWindows();
            
            PostQuitMessage(0);
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}