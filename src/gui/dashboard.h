#pragma once

int windowDashboardWidth  = 250;
int windowDashboardHeight = 382;

void StartDashboard(HINSTANCE hInst) { StartGenericWindow(DASHBOARD_CLASS_NAME, "Trading Floor" GATEWAY_SPACE GATEWAY_NAME, L"TWSAPIClientTradingFloor.Dashboard", windowDashboardWidth, windowDashboardHeight, hInst); }

#define WM_TRAYICON (WM_APP + 101)

#define TIMER_WATCHDOG               1
#define TIMER_WATCHDOG_DELAYED_START 2
#define TIMER_MARKET_CLOCK           3 // Live US market-session clock (1s tick)

#define ID_M_DASHBOARD   1001
#define ID_MB_DIAMONDS   1003
#define ID_MB_SETTINGS   1004
#define ID_MB_MARKET     1007
#define ID_MB_EXCHANGE   1008
#define ID_MB_ORDERS     1009
#define ID_M_ORDERS      1010
#define ID_M_DIAMONDS    1011
#define ID_M_SETTINGS    1012
#define ID_M_MARKET      1015
#define ID_M_DEBUGLOG    1016
#define ID_MB_LINKS      1017

#define ID_M_CONNECT    1100
#define ID_M_DISCONNECT 1101
#define ID_M_EXIT       1102

#define ID_M_MARKET_BASE 1500
#define ID_M_MARKET_MAX   100

#define ID_M_LINKS_BASE  1700   // one command ID per quick link below

struct QuickLink { const char* label; const char* url; };
static const QuickLink quickLinks[] = {
    { "Today", "https://www.investing.com/dividends-calendar" },
    { "WSB",   "https://www.reddit.com/r/wallstreetbets"      },
    { "Scan",  "https://stockscan.io/all-stocks"              },
    { "List",  "https://stockanalysis.com/list/"              },
    { "Map",   "https://finviz.com/map.ashx?t=sec"            },
    { "Data",  "https://www.benzinga.com/quote"               },
    { "Paper", "http://192.168.1.105:2025/paper"              },
    { "Chat",  "https://192.168.1.105/chat/"          },
    { "GitHub", "https://github.com/ctubio/ibkr-gateway-trading-floor" },
};
static const int LINKS_COUNT = (int)(sizeof(quickLinks) / sizeof(quickLinks[0]));

static const char* day_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// ─── Dashboard State ──────────────────────────────────────────────────────────
// Encapsulates all dashboard-specific HWNDs and state variables

struct DashboardState {
    // Window state
    bool fullDetails = true;
    bool shouldBeConnected = true;
    std::string currencyDashboard = "--";
    
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
    HWND hCoin_Unrealized = NULL;
    HWND hCoin_Dividends = NULL;
    HWND hCoin_Accruals = NULL;
    HWND hCoin_BuyingPower = NULL;
    HWND hCoin_MaintMargin = NULL;
    
    // Box 3: Cash
    HWND hCoin_Cash = NULL;
    HWND hCoin_EUR = NULL;
    HWND hCoin_USD = NULL;
    
    // TTS state — speech goes through the shared SharedTtsEngine (shared.h)
    // now; this just tracks whether the dashboard holds a reference to it.
    bool coinsTtsOn = false;
};

// Global dashboard state instance
static DashboardState dashboardState;

void MutexGatewayInstance() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\TWSAPIClientTradingFloorMutex_17072025" GATEWAY_NAME);

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindow(DASHBOARD_CLASS_NAME, NULL);
        if (existingWnd) {
            DWORD processId;
            GetWindowThreadProcessId(existingWnd, &processId);

            PostMessage(existingWnd, WM_COMMAND, ID_M_EXIT, 0);

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
        
        CreateMutex(NULL, TRUE, "Global\\TWSAPIClientTradingFloorMutex_17072025");

        std::this_thread::sleep_for(std::chrono::milliseconds(1021));
    }
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
    if (!dashboardState.hCoin_Clock) return;

    // Get current time in New York (Eastern Time)
    auto now = std::chrono::system_clock::now();
    std::chrono::zoned_time zt{"America/New_York", now};
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
    //const int T_OPEN_BELL = 9 * 3600 + 20 * 60;           // 09:20 ET (10m before 09:30)
    const int T_OPEN_BELL = 8 * 3600 + 30 * 60;          // 08:30 ET (60m before 09:30)
    const int T_OPEN = 9 * 3600 + 30 * 60;                // 09:30 ET
    const int T_CLOSE_IMBAL = 15 * 3600 + 30 * 60;        // 15:30 ET (20m before 15:50)
    //const int T_CLOSE_BELL = 15 * 3600 + 50 * 60;         // 15:50 ET (10m before 16:00)
    const int T_CLOSE_BELL = 15 * 3600 + 00 * 60;        // 15:00 ET (60m before 16:00)
    const int T_CLOSE = 16 * 3600;                        // 16:00 ET
    const int T_AFTERHOURS_END = 20 * 3600;               // 20:00 ET
    const int T_SUNDAY_OPEN = 20 * 3600;                  // 20:00 ET (Sunday 8:00 PM)
    
    std::string phase;
    int target_secs = 0;

    // 1. Handle Weekend / Market Closed State (Targeting Sunday 20:00 ET)
    if (wd == std::chrono::Friday && total_secs >= T_AFTERHOURS_END) {
        phase = "Market Closed"; 
        target_secs = (2 * 24 * 3600) + T_SUNDAY_OPEN; // Fri to Sun 20:00
        SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_GRAY);
    } else if (wd == std::chrono::Saturday) {
        phase = "Market Closed"; 
        target_secs = (1 * 24 * 3600) + T_SUNDAY_OPEN; // Sat to Sun 20:00
        SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_GRAY);
    } else if (wd == std::chrono::Sunday && total_secs < T_SUNDAY_OPEN) {
        phase = "Market Closed"; 
        target_secs = T_SUNDAY_OPEN;                   // Sunday morning/afternoon to Sun 20:00
        SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_GRAY);
    } else if (wd == std::chrono::Sunday && total_secs >= T_SUNDAY_OPEN) {
        phase = "OVERNIGHT"; 
        target_secs = (1 * 24 * 3600) + T_PREMARKET;   // Market is OPEN! Counting to Mon 04:00 AM
        SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_GRAY);
    } 
    // 2. Standard Weekday State Machine (Monday - Friday)
    else {
        if (total_secs < T_PREMARKET) {
            phase = "OVERNIGHT"; target_secs = T_PREMARKET;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_GRAY);
        } else if (total_secs < T_OPEN_IMBAL) {
            phase = "Pre-Market"; target_secs = T_OPEN;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_ORANGE);
        } else if (total_secs < T_OPEN_BELL) {
            phase = "IMBALANCE"; target_secs = T_OPEN;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_RED);
        } else if (total_secs < T_OPEN) {
            phase = "Opening Bell"; target_secs = T_OPEN;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_BLUE);
        } else if (total_secs < T_CLOSE_IMBAL) {
            phase = "Market Open"; target_secs = T_CLOSE;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_YELLOW);
        } else if (total_secs < T_CLOSE_BELL) {
            phase = "IMBALANCE"; target_secs = T_CLOSE;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_RED);
        } else if (total_secs < T_CLOSE) {
            phase = "Closing Bell"; target_secs = T_CLOSE;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_BLUE);
        } else if (total_secs < T_AFTERHOURS_END) {
            phase = "After-Hours"; target_secs = T_AFTERHOURS_END;
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_ORANGE);
        } else {
            phase = "OVERNIGHT"; target_secs = T_PREMARKET + (24 * 3600); // Tomorrow's 04:00 AM
            SetCtrlColor(dashboardState.hCoin_Clock, COINS_CLR_GRAY);
        }
    }

    if (dashboardState.fullDetails) { 
        std::string day_str = day_names[wd.c_encoding()];
        std::string time_str = std::format("{:02}:{:02}", time_of_day.hours().count(), time_of_day.minutes().count()) + " " + day_str;
        SetWindowTextA(hWnd, time_str.c_str());
    } else SetWindowTextA(hWnd, phase.c_str());

    // Calculate time left
    int secs_left = target_secs - total_secs;
    int h_left = secs_left / 3600;
    int m_left = (secs_left % 3600) / 60;

    // Format output string (HH:MM)
    std::string time_left_str = std::format("{:02}:{:02}", h_left, m_left);

    SetWindowTextA(dashboardState.hCoin_Clock, time_left_str.c_str());
    InvalidateRect(dashboardState.hCoin_Clock, NULL, TRUE);
}

#define ID_COIN_NETLIQ   5100
#define ID_COIN_BIGPNL   5101
#define ID_COIN_PCT      5102
#define ID_COIN_SPEAKER  5103
#define ID_COIN_LOCK     5104
#define TIMER_COINS_SPEAKER  0xC015   // WM_TIMER id

// ─── Exchange Currency popup (DASHBOARD_EXCHANGE_CLASS_NAME) ───────────────────
#define ID_DASHFX_ACTION_COMBO  5201
#define ID_DASHFX_AMOUNT_EDIT   5202

// ─── TTS helpers ──────────────────────────────────────────────────────────────
// Speech goes through the shared SharedTtsEngine (shared.h). The dashboard just
// tracks whether it currently holds a reference to it.

static void Coins_SpeakDailyPnL() {
    if (!dashboardState.coinsTtsOn || !dashboardState.hCoin_BigPnL) return;
    char buf[128] = {};
    GetWindowTextA(dashboardState.hCoin_BigPnL, buf, sizeof(buf));
    std::wstring wtext(buf, buf + strlen(buf));
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
    dashboardState.coinsTtsOn = !dashboardState.coinsTtsOn;

    // Persist state to registry
    RegSetDword(DASHBOARD_CLASS_NAME, "Speaker", dashboardState.coinsTtsOn ? 1 : 0);

    if (dashboardState.coinsTtsOn) {
        if (!SharedTts().Acquire()) {
            dashboardState.coinsTtsOn = false;
            return;
        }
        SetCtrlColor(dashboardState.hCoin_Speaker, darkMode ? COINS_CLR_WHITE : COINS_CLR_BLACK);   // bright = active
        SetTimer(hWnd, TIMER_COINS_SPEAKER, 21000, NULL);
        Coins_SpeakDailyPnL();                          // speak immediately
    } else {
        KillTimer(hWnd, TIMER_COINS_SPEAKER);
        SharedTts().Release();                          // stop + tear down if we were the last one
        SetCtrlColor(dashboardState.hCoin_Speaker, COINS_CLR_GRAY);    // dim = inactive
    }

    if (dashboardState.hCoin_Speaker) InvalidateRect(dashboardState.hCoin_Speaker, NULL, TRUE);
}

// ─── Icons update ─────────────────────────────────────────────────────────────

struct IconUpdateContext {
    bool connected;
    const std::unordered_map<std::string, HICON>& onlineIcons;
    const std::unordered_map<std::string, HICON>& offlineIcons;
};

BOOL CALLBACK IconsEnumWindowsProc(HWND hwnd, LPARAM lParam) {
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

    Shell_NotifyIconW(NIM_ADD, &nid);
    
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

void UpdateTrayIcon(HWND hWnd) {
    std::string tooltip = std::string("Trading Floor" GATEWAY_SPACE GATEWAY_NAME) + ": ";
    bool connected = false;

    if (!api().isConnected()) {
        tooltip += "Disconnected";
    } else {
        std::string acc = api().getAccountNumber();
        if (acc.empty()) {
            tooltip += "Connecting..";
        } else {
            connected = api().isMarketDataConnected() && api().isTradingConnected();
            tooltip = acc + " | " + dashboardState.currencyDashboard + " | " + (connected ? "Connected" : "Disconnected");
        }
    }
    std::wstring wTooltip(tooltip.begin(), tooltip.end());
    wcsncpy(nid.szTip, wTooltip.c_str(), _countof(nid.szTip) - 1);
    nid.szTip[_countof(nid.szTip) - 1] = L'\0'; // Note the L prefix for wide null terminator

    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.hIcon  = connected ? onlineIcons[std::string(DASHBOARD_CLASS_NAME)] : offlineIcons[std::string(DASHBOARD_CLASS_NAME)];

    Shell_NotifyIconW(NIM_MODIFY, &nid);

    IconUpdateContext ctx = { connected, onlineIcons, offlineIcons };
    EnumWindows(IconsEnumWindowsProc, (LPARAM)&ctx);
}

// ─── Labels update ─────────────────────────────────────────────────────────────

static bool SetWindowTextIfChanged(HWND hWnd, const std::string& newText) {
    if (!hWnd) return false;
    char buf[256];
    GetWindowTextA(hWnd, buf, sizeof(buf));
    if (std::string(buf) != newText) {
        SetWindowTextA(hWnd, newText.c_str());
        InvalidateRect(hWnd, NULL, TRUE);
        return true;
    }
    return false;
}

static bool SetWindowTextWIfChanged(HWND hWnd, const std::wstring& newText) {
    if (!hWnd) return false;
    wchar_t buf[256];
    GetWindowTextW(hWnd, buf, sizeof(buf));
    if (std::wstring(buf) != newText) {
        SetWindowTextW(hWnd, newText.c_str());
        InvalidateRect(hWnd, NULL, TRUE);
        return true;
    }
    return false;
}

void Coins_UpdateLabels(HWND hWnd) {
    auto   summary    = api().getAccountSummary();
    double daily      = api().getDailyPnL();
    double unrealized = api().getUnrealizedPnL();
    double realized   = api().getRealizedPnL();
    
    std::string prevCurrency = dashboardState.currencyDashboard;
    dashboardState.currencyDashboard = "--";
    auto tryParse = [&](const std::string& k) -> double {
        auto it = summary.find(k);
        if (it == summary.end()) return 0.0;
        try { return std::stod(it->second); } catch (...) { return 0.0; }
    };
    if (summary.count("EUR_NetLiquidation")) {
        dashboardState.currencyDashboard = "EUR"; NetLiquidation = tryParse("EUR_NetLiquidation");
    } else if (summary.count("USD_NetLiquidation")) {
        dashboardState.currencyDashboard = "USD"; NetLiquidation = tryParse("USD_NetLiquidation");
    } else if (summary.count("NetLiquidation")) {
        NetLiquidation = tryParse("NetLiquidation");
        for (auto const& [k, v] : summary)
            if (k.find("_NetLiquidation") != std::string::npos)
                { dashboardState.currencyDashboard = k.substr(0, k.find('_')); break; }
    }

    if (dashboardState.currencyDashboard != prevCurrency)
        UpdateTrayIcon(hWnd);

    const int m = 10;
    int y1 = 8;
    int box1H = 94;
    int y2 = y1 + box1H + 9;
    int box2H = 124;
    int y3 = y2 + box2H + 9;

    if (dashboardState.hCoin_NetLiq) {
        std::string formattedNum = FormatWithCommas(NetLiquidation, dashboardState.fullDetails);
        if (SetWindowTextIfChanged(dashboardState.hCoin_NetLiq, formattedNum)) {
            int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
            SetWindowPos(dashboardState.hCoin_NetLiq, NULL, m + 10 + 48, y1 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    COLORREF pnlClr = daily >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
    if (dashboardState.hCoin_BigPnL) {
        std::string formattedNum = FormatWithCommas(daily);
        if (daily >= 0.0) formattedNum = "+" + formattedNum;
        if (SetWindowTextIfChanged(dashboardState.hCoin_BigPnL, formattedNum)) {
            SetCtrlColor(dashboardState.hCoin_BigPnL, pnlClr);
        }
    }
    if (dashboardState.hCoin_Pct) {
        double pct = (NetLiquidation != 0.0) ? (daily / NetLiquidation * 100.0) : 0.0;
        std::string formattedNum = FormatWithCommas(pct);
        if (pct >= 0.0) formattedNum = "+" + formattedNum;
        if (SetWindowTextIfChanged(dashboardState.hCoin_Pct, std::format("{}%", formattedNum))) {
            SetCtrlColor(dashboardState.hCoin_Pct, pnlClr);
        }
    }
    if (dashboardState.hCoin_Realized) {
        std::string formattedNum = FormatWithCommas(realized);
        if (SetWindowTextIfChanged(dashboardState.hCoin_Realized, formattedNum)) {
            COLORREF clr = realized >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
            SetCtrlColor(dashboardState.hCoin_Realized, clr);
        }
    }

    if (dashboardState.hCoin_Positions) {
        double grossPos = tryParse("GrossPositionValue");
        std::string formattedNum = FormatWithCommas(grossPos, dashboardState.fullDetails);
        if (SetWindowTextIfChanged(dashboardState.hCoin_Positions, formattedNum)) {
            int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
            SetWindowPos(dashboardState.hCoin_Positions, NULL, m + 10 + 70, y2 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    if (dashboardState.hCoin_Unrealized) {
        std::string formattedNum = FormatWithCommas(unrealized, dashboardState.fullDetails);
        if (SetWindowTextIfChanged(dashboardState.hCoin_Unrealized, formattedNum)) {
            COLORREF clr = unrealized >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
            SetCtrlColor(dashboardState.hCoin_Unrealized, clr);
        }
    }
    if (dashboardState.hCoin_Dividends) {
        double div = tryParse("AccruedDividend");
        std::string formattedNum = FormatWithCommas(div);
        if (SetWindowTextIfChanged(dashboardState.hCoin_Dividends, formattedNum)) {
            SetCtrlColor(dashboardState.hCoin_Dividends, COINS_CLR_PURPLE);
        }
    }
    if (dashboardState.hCoin_Accruals) {
        double acc = tryParse("AccruedCash");
        std::string formattedNum = FormatWithCommas(acc);
        if (SetWindowTextIfChanged(dashboardState.hCoin_Accruals, formattedNum)) {
            COLORREF clr = acc > 0.0 ? COINS_CLR_GREEN : (acc < 0.0 ? COINS_CLR_RED : COLOR_THEME);
            SetCtrlColor(dashboardState.hCoin_Accruals, clr);
        }
    }
    if (dashboardState.hCoin_BuyingPower) {
        double bp = tryParse("BuyingPower");
        std::string formattedNum = FormatWithCommas(bp, dashboardState.fullDetails);
        SetWindowTextIfChanged(dashboardState.hCoin_BuyingPower, formattedNum);
    }
    if (dashboardState.hCoin_MaintMargin) {
        double mm = tryParse("MaintMarginReq");
        std::string formattedNum = FormatWithCommas(mm, dashboardState.fullDetails);
        SetWindowTextIfChanged(dashboardState.hCoin_MaintMargin, formattedNum);
    }

    if (dashboardState.hCoin_Cash) {
        double cash = tryParse("CashBalance");
        std::string formattedNum = FormatWithCommas(cash, dashboardState.fullDetails) + " " + dashboardState.currencyDashboard;
        if (SetWindowTextIfChanged(dashboardState.hCoin_Cash, formattedNum)) {
            COLORREF clr = cash > 0.0 ? COINS_CLR_GREEN : (cash < 0.0 ? COINS_CLR_RED : COLOR_THEME);
            SetCtrlColor(dashboardState.hCoin_Cash, clr);
            int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
            SetWindowPos(dashboardState.hCoin_Cash, NULL, m + 10 + 45, y3 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    if (dashboardState.hCoin_EUR) {
        double eur = tryParse("EUR_CashBalance");
        std::string formattedNum = FormatWithCommas(eur, dashboardState.fullDetails) + (dashboardState.fullDetails ? "" : " €");
        if (SetWindowTextWIfChanged(dashboardState.hCoin_EUR, StringToWide(formattedNum))) {
            COLORREF clr = eur > 0.0 ? COINS_CLR_GREEN : (eur < 0.0 ? COINS_CLR_RED : COLOR_THEME);
            SetCtrlColor(dashboardState.hCoin_EUR, clr);
        }
    }
    if (dashboardState.hCoin_USD) {
        double usd = tryParse("USD_CashBalance");
        std::string formattedNum = FormatWithCommas(usd, dashboardState.fullDetails) + (dashboardState.fullDetails ? "" : " $");
        if (SetWindowTextWIfChanged(dashboardState.hCoin_USD, StringToWide(formattedNum))) {
            COLORREF clr = usd > 0.0 ? COINS_CLR_GREEN : (usd < 0.0 ? COINS_CLR_RED : COLOR_THEME);
            SetCtrlColor(dashboardState.hCoin_USD, clr);
        }
    }
}

void addButtons(HWND hWnd, HINSTANCE hInst, LPCSTR buttonText, int x, int y, HMENU menuId, int iconId) {
		// Create the button
        HWND hBtn = CreateWindow(
            "BUTTON", buttonText,
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
            x, y, 26, 26,
            hWnd, menuId, hInst, NULL
        );
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
// A small single-instance popup (see DASHBOARD_EXCHANGE_CLASS_NAME) that shows a
// live EUR.USD rate in its title bar and lets the user submit a market order
// converting between USD and EUR on IDEALPRO. Opened from the 🗘 icon at the
// right of the last account-summary row.

static void DashboardFx_UpdateTitle(HWND hWnd) {
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

    char buf[32] = {};
    if (hEdit) GetWindowTextA(hEdit, buf, sizeof(buf));
    double amount = atof(buf);

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
static void DashboardFx_SubmitAndClose(HWND hWnd) {
    HWND hCombo = GetDlgItem(hWnd, ID_DASHFX_ACTION_COMBO);
    HWND hEdit  = GetDlgItem(hWnd, ID_DASHFX_AMOUNT_EDIT);
    int sel = hCombo ? (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0) : 0;
    bool isBuy = (sel != 1);

    char buf[32] = {};
    if (hEdit) GetWindowTextA(hEdit, buf, sizeof(buf));
    double amount = atof(buf);

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
static LRESULT CALLBACK DashboardFx_KeySubclassProc(HWND hCtrl, UINT msg, WPARAM wParam, LPARAM lParam,
                                                     UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_CHAR) {
        if (wParam == VK_RETURN || wParam == VK_ESCAPE)
            return 0;
    }
    if (msg == WM_KEYDOWN) {
        HWND hParent = GetParent(hCtrl);
        if (wParam == VK_ESCAPE) {
            DestroyWindow(hParent);
            return 0;
        }
        if (wParam == VK_RETURN) {
            DashboardFx_SubmitAndClose(hParent);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hCtrl, DashboardFx_KeySubclassProc, uIdSubclass);
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

            SetWindowSubclass(hCombo, DashboardFx_KeySubclassProc, 1, 0);
            SetWindowSubclass(hEdit,  DashboardFx_KeySubclassProc, 2, 0);

            api().reqFxRate(hWnd);
            DashboardFx_UpdateTitle(hWnd);

            SetFocus(hEdit);
            int len = GetWindowTextLengthA(hEdit);
            SendMessageA(hEdit, EM_SETSEL, 0, len);
            break;
        }

        case WM_FX_RATE_UPDATE:
            DashboardFx_UpdateTitle(hWnd);
            break;

        case WM_COMMAND:
            if ((LOWORD(wParam) == ID_DASHFX_ACTION_COMBO && HIWORD(wParam) == CBN_SELCHANGE) ||
                (LOWORD(wParam) == ID_DASHFX_AMOUNT_EDIT  && HIWORD(wParam) == EN_CHANGE)) {
                DashboardFx_UpdateTitle(hWnd);
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

LRESULT CALLBACK WndProcDashboard(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:	{
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            const int m    = 10;
            const int boxW = 226;

            // ─── Box 1: Net Liq & PnL ──────────────────────────────────────────
            int y1 = 8;
            int box1H = 94;
            dashboardState.hCoinBox1 = CreateWindowA("BUTTON", "Today:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y1, boxW, box1H, hWnd, NULL, hInst, NULL);
            SetWindowSubclass(dashboardState.hCoinBox1, DarkGroupBoxSubclassProc, 1, 0);
            SendMessage(dashboardState.hCoinBox1, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_NetLiq = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 62, y1 - 4, 30, 18, hWnd, (HMENU)ID_COIN_NETLIQ, hInst, NULL);
            SendMessage(dashboardState.hCoin_NetLiq, WM_SETFONT, (WPARAM)hFont14ptbold.get(), TRUE);

            // Lock icon (hidden by default)
            dashboardState.hCoin_Lock = CreateWindowW(L"STATIC", LOCK_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + boxW - 90, y1, 20, 18, hWnd, (HMENU)ID_COIN_LOCK, hInst, NULL);
            SendMessage(dashboardState.hCoin_Lock, WM_SETFONT, (WPARAM)hFont_Icons, TRUE);
            SetCtrlColor(dashboardState.hCoin_Lock, COINS_CLR_GRAY);
            ShowWindow(dashboardState.hCoin_Lock, SW_HIDE);

            dashboardState.hCoin_Clock = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY,
                m + boxW - 60, y1, 40, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_Clock, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            // Row 1: PnL: 🔊 +0.00
            HWND hLblBigPnL = CreateWindowA("STATIC", "PnL:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 24, 30, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblBigPnL, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Speaker = CreateWindowW(L"STATIC", SPEAKER_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + 42, y1 + 24, 20, 20, hWnd, (HMENU)ID_COIN_SPEAKER, hInst, NULL);
            SendMessage(dashboardState.hCoin_Speaker, WM_SETFONT, (WPARAM)hFont_Icons, TRUE);
            SetCtrlColor(dashboardState.hCoin_Speaker, COINS_CLR_GRAY);

            dashboardState.hCoin_BigPnL = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOTIFY,
                m + 70, y1 + 16, boxW - 82, 32, hWnd, (HMENU)ID_COIN_BIGPNL, hInst, NULL);
            SendMessage(dashboardState.hCoin_BigPnL, WM_SETFONT, (WPARAM)hFont21ptbold.get(), TRUE);
            SetCtrlColor(dashboardState.hCoin_BigPnL, COINS_CLR_GREEN);

            // Row 2: PnL %: +0.00%
            HWND hLblPct = CreateWindowA("STATIC", "PnL %:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 48, 50, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblPct, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Pct = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 70, y1 + 48, boxW - 82, 18, hWnd, (HMENU)ID_COIN_PCT, hInst, NULL);
            SendMessage(dashboardState.hCoin_Pct, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);
            SetCtrlColor(dashboardState.hCoin_Pct, COINS_CLR_GREEN);

            // Row 3: Realized: 0.00
            HWND hLblRealized = CreateWindowA("STATIC", "Realized:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 68, 65, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblRealized, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Realized = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 77, y1 + 68, boxW - 89, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_Realized, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);


            // ─── Box 2: Positions & Margin ─────────────────────────────────────
            int y2 = y1 + box1H + 9; // y2 = 114
            int box2H = 124;
            dashboardState.hCoinBox2 = CreateWindowA("BUTTON", "Positions:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y2, boxW, box2H, hWnd, NULL, hInst, NULL);
            SetWindowSubclass(dashboardState.hCoinBox2, DarkGroupBoxSubclassProc, 2, 0);
            SendMessage(dashboardState.hCoinBox2, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Positions = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 77, y2 - 4, 30, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_Positions, WM_SETFONT, (WPARAM)hFont14ptbold.get(), TRUE);

            // Row 1: Unrealized: 0.00
            HWND hLblUnrealized = CreateWindowA("STATIC", "Unrealized:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 20, 75, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblUnrealized, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Unrealized = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 20, boxW - 102, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_Unrealized, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 2: Dividends: 33.75
            dashboardState.hLblDividends = CreateWindowA("STATIC", "Dividends:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 40, 75, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hLblDividends, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Dividends = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 40, boxW - 102, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_Dividends, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 3: Accruals: -1.64
            dashboardState.hLblAccruals = CreateWindowA("STATIC", "Accruals:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 60, 75, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hLblAccruals, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Accruals = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 60, boxW - 102, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_Accruals, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 4: Buying Power: 86,483.04
            dashboardState.hLblBP = CreateWindowA("STATIC", "Buying Power:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 80, 100, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hLblBP, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_BuyingPower = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 105, y2 + 80, boxW - 117, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_BuyingPower, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 5: Maint Margin: 4,403.05
            dashboardState.hLblMM = CreateWindowA("STATIC", "Maintenance:", //  Margin:
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 100, 85, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hLblMM, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_MaintMargin = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 97, y2 + 100, boxW - 109, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_MaintMargin, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);


            // ─── Box 3: Cash ───────────────────────────────────────────────────
            int y3 = y2 + box2H + 9; // y3 = 250
            int box3H = 64;
            dashboardState.hCoinBox3 = CreateWindowA("BUTTON", "Cash:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y3, boxW, box3H, hWnd, NULL, hInst, NULL);
            SetWindowSubclass(dashboardState.hCoinBox3, DarkGroupBoxSubclassProc, 3, 0);
            SendMessage(dashboardState.hCoinBox3, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_Cash = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 53, y3 - 4, 30, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_Cash, WM_SETFONT, (WPARAM)hFont14ptbold.get(), TRUE);

            // Row 1: EUR: 285.31
            dashboardState.hLblEUR = CreateWindowA("STATIC", "EUR:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y3 + 20, 35, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hLblEUR, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_EUR = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 50, y3 + 20, boxW - 62, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_EUR, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 2: USD: -11,559.66
            dashboardState.hLblUSD = CreateWindowA("STATIC", "USD:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y3 + 40, 35, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hLblUSD, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            dashboardState.hCoin_USD = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 50, y3 + 40, boxW - 62, 18, hWnd, NULL, hInst, NULL);
            SendMessage(dashboardState.hCoin_USD, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);


            // ─── Bottom action buttons ──────────────────────────────────────────
            int yBtn = y3 + box3H + 10; // yBtn = 326
            int steps = 1;
            int stepz = 0;
            addButtons(hWnd, hInst, "Orders",    (7 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_ORDERS,    103);
            addButtons(hWnd, hInst, "Diamonds",  (7 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_DIAMONDS,  104);
            addButtons(hWnd, hInst, "Exchange",  (7 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_EXCHANGE,  106);
            addButtons(hWnd, hInst, "Market",    9 + (7 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_MARKET,    105);
            addButtons(hWnd, hInst, "Links",   9 + (7 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_LINKS,    109);

            addButtons(hWnd, hInst, "Settings", 18 + (7 * steps++) + (26 * stepz++) + m, yBtn, (HMENU)ID_MB_SETTINGS,  107);

            api().addApiUpdateWindow(hWnd);

            BindTrayIcon(hWnd);

            if (RegGetDword(DASHBOARD_CLASS_NAME, "Speaker", 0)) {
                Coins_ToggleTTS(hWnd);
            }
            
            SetTimer(hWnd, TIMER_MARKET_CLOCK, 21000, NULL); // live market clock
            SendMessage(hWnd, WM_TIMER, TIMER_MARKET_CLOCK, 0);

            SetTimer(hWnd, TIMER_WATCHDOG, 10000, NULL);
            SetTimer(hWnd, TIMER_WATCHDOG_DELAYED_START, 721, NULL);

            break;
        }
        
        case WM_ACTIVATE: {
            RECT windowRect; 
            GetWindowRect(hWnd, &windowRect);
            const int m    = 10;
            const int boxW = 226;
            int y1 = 8;
            int box1H = 94;
            int y2 = y1 + box1H + 9; // y2 = 114
            int box2H = 124;
            int y3 = y2 + box2H + 9; // y3 = 250
            int box3H = 64;
            LONG_PTR style = GetWindowLongPtr(dashboardState.hCoin_EUR, GWL_STYLE);
            if (LOWORD(wParam) != WA_INACTIVE) {
                if (lockHotkeys) break;
                ShowWindow(dashboardState.hLblDividends, SW_SHOW);
                ShowWindow(dashboardState.hLblAccruals, SW_SHOW);
                ShowWindow(dashboardState.hCoin_Accruals, SW_SHOW);
                ShowWindow(dashboardState.hCoin_Dividends, SW_SHOW);
                ShowWindow(dashboardState.hCoin_Cash, SW_SHOW);
                ShowWindow(dashboardState.hLblEUR, SW_SHOW);
                ShowWindow(dashboardState.hLblUSD, SW_SHOW);
                SetWindowPos(dashboardState.hCoinBox2, NULL, m, y2, boxW, box2H, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hLblBP, NULL,  m + 12, y2 + 80, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoin_BuyingPower, NULL, m + 105, y2 + 80, boxW - 117, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hLblMM, NULL, m + 12, y2 + 100, 85, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoin_MaintMargin, NULL, m + 97, y2 + 100, boxW - 109, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoinBox3, NULL, m, y3, boxW, box3H, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoin_EUR, NULL, m + 50, y3 + 20, boxW - 62, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                style &= ~SS_TYPEMASK;
                SetWindowLongPtr(dashboardState.hCoin_EUR, GWL_STYLE, style | SS_RIGHT);
                SetWindowPos(dashboardState.hCoin_USD, NULL, m + 50, y3 + 40, boxW - 62, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                MoveWindow(hWnd, windowRect.left, windowRect.top, windowDashboardWidth, windowDashboardHeight     , TRUE);
                dashboardState.fullDetails = true;
            } else {
                ShowWindow(dashboardState.hLblDividends, SW_HIDE);
                ShowWindow(dashboardState.hLblAccruals, SW_HIDE);
                ShowWindow(dashboardState.hCoin_Accruals, SW_HIDE);
                ShowWindow(dashboardState.hCoin_Dividends, SW_HIDE);
                ShowWindow(dashboardState.hCoin_Cash, SW_HIDE);
                ShowWindow(dashboardState.hLblEUR, SW_HIDE);
                ShowWindow(dashboardState.hLblUSD, SW_HIDE);
                SetWindowPos(dashboardState.hCoinBox2, NULL, m, y2, boxW, box2H - 40, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hLblBP, NULL,  m + 12, y2 + 40, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoin_BuyingPower, NULL, m + 105, y2 + 40, boxW - 117, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hLblMM, NULL, m + 12, y2 + 60, 85, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoin_MaintMargin, NULL, m + 97, y2 + 60, boxW - 109, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoinBox3, NULL, m, y3 - 40, boxW, box3H - 20, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(dashboardState.hCoin_EUR, NULL, m + 12, y3 + 20 - 40, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                style &= ~SS_TYPEMASK;
                SetWindowLongPtr(dashboardState.hCoin_EUR, GWL_STYLE, style | SS_LEFT);
                SetWindowPos(dashboardState.hCoin_USD, NULL, m + 112, y3 + 20 - 40, boxW - 124, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                MoveWindow(hWnd, windowRect.left, windowRect.top, windowDashboardWidth, windowDashboardHeight - 60 - 38, TRUE);
                dashboardState.fullDetails = false;
            }
            if (api().isMarketDataConnected() && api().isTradingConnected()) {
                Coins_UpdateLabels(hWnd);
            }
            UpdateMarketClock(hWnd);
            break;
        }

        case WM_API_UPDATE:
            if (!api().isMarketDataConnected() || !api().isTradingConnected()) {
                const int m    = 10;
                const int boxW = 226;
                int box1H = 94;
                int y1 = 8;
                int y2 = y1 + box1H + 9; // y2 = 114
                int box2H = 124;
                int y3 = y2 + box2H + 9; // y3 = 250
                int box3H = 64;
                if (dashboardState.hCoin_NetLiq)      { SetWindowTextA(dashboardState.hCoin_NetLiq,      "--"); SetWindowPos(dashboardState.hCoin_NetLiq, NULL, m + 67, y1 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE); InvalidateRect(dashboardState.hCoin_NetLiq, NULL, TRUE); }
                if (dashboardState.hCoin_BigPnL)      SetWindowTextA(dashboardState.hCoin_BigPnL,      "--");
                if (dashboardState.hCoin_Pct)         SetWindowTextA(dashboardState.hCoin_Pct,         "--");
                if (dashboardState.hCoin_Realized)    SetWindowTextA(dashboardState.hCoin_Realized,    "--");
                if (dashboardState.hCoin_Positions)   { SetWindowTextA(dashboardState.hCoin_Positions,   "--"); SetWindowPos(dashboardState.hCoin_Positions, NULL, m + 77, y2 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE); InvalidateRect(dashboardState.hCoin_Positions, NULL, TRUE); }
                if (dashboardState.hCoin_Unrealized)  SetWindowTextA(dashboardState.hCoin_Unrealized,  "--");
                if (dashboardState.hCoin_Dividends)   SetWindowTextA(dashboardState.hCoin_Dividends,   "--");
                if (dashboardState.hCoin_Accruals)    SetWindowTextA(dashboardState.hCoin_Accruals,    "--");
                if (dashboardState.hCoin_BuyingPower) SetWindowTextA(dashboardState.hCoin_BuyingPower, "--");
                if (dashboardState.hCoin_MaintMargin) SetWindowTextA(dashboardState.hCoin_MaintMargin, "--");
                if (dashboardState.hCoin_Cash)        { SetWindowTextA(dashboardState.hCoin_Cash,        "--"); SetWindowPos(dashboardState.hCoin_Cash, NULL, m + 53, y3 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE); InvalidateRect(dashboardState.hCoin_Cash, NULL, TRUE); }
                if (dashboardState.hCoin_EUR)         SetWindowTextA(dashboardState.hCoin_EUR,         "--");
                if (dashboardState.hCoin_USD)         SetWindowTextA(dashboardState.hCoin_USD,         "--");
                dashboardState.currencyDashboard = "--";
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
        case WM_PNL_UPDATE:
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
                if (dashboardState.shouldBeConnected && !api().isConnected()) {
                    if (EnsureGatewayRunning(hWnd)) {
                        EnsureGatewayLoggedIn(hWnd);
                    }
                    int port = std::filesystem::path(GetGatewayPath()).filename() == "ibgateway.exe" ? 4001 : 7496;
                    int clientId = (int)Settings_Load("ClientId", 0);
                    int groupId = (int)Settings_Load("GroupId", 4);
                    api().connect(port , clientId, groupId);
                } else if (!dashboardState.shouldBeConnected && api().isConnected()) {
                    api().disconnect();
                }
            }
            break;

        case WM_OPEN_ORDERS_WINDOW:
            StartOrders();
            break;

        case WM_TTS_VOICE_CHANGED: {
            // The shared engine holds one voice for every window now — just
            // re-apply the (now-changed) saved voice token to it in place.
            SharedTts().ReapplySavedVoice();
            if (dashboardState.coinsTtsOn) Coins_SpeakDailyPnL(); // speak immediately with the new voice
            break;
        }

        case WM_SHOW_ALERT: {
            AlertPopupData* data = (AlertPopupData*)lParam;
            if (data) {
                FlashScreen(data->isUp, 1000);
                StartGenericWindow(ALERT_NOTIFY_CLASS_NAME, data->title.c_str(), L"Alert Notification", 300, 127, NULL, "", data);
                PlaySound_Async(209);
            }
            return 0;
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
                    if (hWnd && IsWindowVisible(hWnd)) { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_DASHBOARD, IsWindowAlwaysOnTop(DASHBOARD_CLASS_NAME) ? L"[ ★ ] Dashboard" : L"[  ] Dashboard"); }
                    if (FindWindowA(DIAMONDS_CLASS_NAME, NULL))  { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_DIAMONDS,  IsWindowAlwaysOnTop(DIAMONDS_CLASS_NAME)  ? L"[ ★ ] Diamonds"  : L"[  ] Diamonds"); }
                    if (FindWindowA(ORDERS_CLASS_NAME, NULL))    { hasSubmenus++; AppendMenuW(hMenu, MF_STRING, ID_M_ORDERS,    IsWindowAlwaysOnTop(ORDERS_CLASS_NAME)    ? L"[ ★ ] Orders"    : L"[  ] Orders"); }

                    auto tsWindows = EnumerateMarketWindows();
                    std::sort(tsWindows.begin(), tsWindows.end(), [](const auto& a, const auto& b) {
                        return a.symbol < b.symbol;
                    });
                    for (size_t i = 0; i < tsWindows.size() && i < ID_M_MARKET_MAX; ++i) {
                        std::wstring label = IsMarketAlwaysOnTop(tsWindows[i].symbol) ? 
                            L"[ ★ ] Market: " + StringToWide(tsWindows[i].symbol) : 
                            L"[  ] Market: " + StringToWide(tsWindows[i].symbol);
                            
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
                        case ID_M_DASHBOARD:
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
                /*
                if (lockHotKeys) {
                    std::string lockPass = Settings_LoadString("Lock", "");
                    if (!lockPass.empty()) {
                        std::string inputLock = PromptForLock();
                        if (lockPass != inputLock) {
                            MessageBoxA(hWnd, "Incorrect Lock Keyword", "Lock Keyword Required", MB_ICONERROR);}
                            return 0;
                        }
                    }
                }
                lockHotkeys = !lockHotkeys;
                ShowWindow(dashboardState.hCoin_Lock, lockHotkeys ? SW_SHOW : SW_HIDE);
                PostMessage(hWnd, WM_ACTIVATE, lockHotkeys ? WA_INACTIVE : WA_ACTIVE, 0);
                ToggleTWS(lockHotkeys ? SW_HIDE : SW_SHOW);
                */
                return 0;
            }
        }

        case WM_SHOWWINDOW:
            if (!lockHotkeys && wParam == TRUE) {
                ToggleTWS(SW_SHOW); 
            }
            return 0;

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
                    dashboardState.shouldBeConnected = true;
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_DISCONNECT:
                    dashboardState.shouldBeConnected = false;
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_EXIT:
                    DestroyWindow(hWnd);
                    break;
                    
                case ID_MB_DIAMONDS:
                    StartDiamonds();
                    break;
                case ID_MB_EXCHANGE:
                    StartGenericWindow(DASHBOARD_EXCHANGE_CLASS_NAME, "Exchange", L"TWSAPIClientTradingFloor.ExchangeCurrency", 320, 70);
                    break;
                case ID_MB_LINKS: {
                    HWND hBtn = GetDlgItem(hWnd, ID_MB_LINKS);
                    RECT rc; GetWindowRect(hBtn, &rc);

                    HMENU hMenu = CreatePopupMenu();
                    for (int i = 0; i < LINKS_COUNT; ++i) {
                        if (quickLinks[i].label == "Paper" || quickLinks[i].label == "GitHub")
                            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                        AppendMenuA(hMenu, MF_STRING, ID_M_LINKS_BASE + i, quickLinks[i].label);   
                    }

                    SetForegroundWindow(hWnd);
                    int cmd = TrackPopupMenu(hMenu,
                        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
                        rc.left, rc.bottom, 0, hWnd, NULL);
                    DestroyMenu(hMenu);

                    if (cmd >= ID_M_LINKS_BASE && cmd < ID_M_LINKS_BASE + LINKS_COUNT) {
                        ShellExecuteA(NULL, "open", quickLinks[cmd - ID_M_LINKS_BASE].url, NULL, NULL, SW_SHOWNORMAL);
                    }
                    break;
                }
                case ID_MB_MARKET:
                    StartMarket();
                    break;
                case ID_MB_SETTINGS:
                    StartSettings();
                    break;
                case ID_MB_ORDERS:
                    StartOrders();
                    break;        
                case ID_M_DASHBOARD:
                    ToggleWindowAlwaysOnTop(DASHBOARD_CLASS_NAME);
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
                        auto tsWindows = EnumerateMarketWindows();
                         std::sort(tsWindows.begin(), tsWindows.end(), [](const auto& a, const auto& b) {
                            return a.symbol < b.symbol;
                        });
                         int index = LOWORD(wParam) - ID_M_MARKET_BASE;
                        if (index >= 0 && index < (int)tsWindows.size()) {
                            ToggleMarketAlwaysOnTop(tsWindows[index].hWnd, tsWindows[index].symbol);
                        }
                    }
                    break;
            }
            break;
        }
        case WM_DESTROY:
            api().disconnect();
#ifndef GATEWAY_SIM
            if (Settings_KillGatewayOnExit())
                KillGateway();
#endif
            api().removeApiUpdateWindow(hWnd);
            Shell_NotifyIconW(NIM_DELETE, &nid);
            // Stop TTS
            if (dashboardState.coinsTtsOn) {
                KillTimer(hWnd, TIMER_COINS_SPEAKER);
                SharedTts().Release();
            }
            dashboardState.coinsTtsOn = false;

            dashboardState.hLblEUR = dashboardState.hLblUSD = dashboardState.hCoin_NetLiq = dashboardState.hCoin_BigPnL = dashboardState.hCoin_Pct = dashboardState.hCoin_Realized = dashboardState.hCoin_Speaker = dashboardState.hCoin_Lock = NULL;
            dashboardState.hCoin_Positions = dashboardState.hCoin_Unrealized = dashboardState.hCoin_Dividends = dashboardState.hCoin_Accruals = dashboardState.hCoin_BuyingPower = dashboardState.hCoin_MaintMargin = NULL;
            dashboardState.hCoin_Clock = dashboardState.hLblBP = dashboardState.hLblMM = dashboardState.hLblDividends = dashboardState.hLblAccruals = dashboardState.hCoinBox1 = dashboardState.hCoinBox2 = dashboardState.hCoinBox3 = dashboardState.hCoin_Cash = dashboardState.hCoin_EUR = dashboardState.hCoin_USD = NULL;

            PostQuitMessage(0);
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}