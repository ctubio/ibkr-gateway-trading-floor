#pragma once

int windowDashboardWidth  = 250;
int windowDashboardHeight = 382;

void StartDashboard(HINSTANCE hInst) { StartGenericWindow(DASHBOARD_CLASS_NAME, "Trading Floor" GATEWAY_SPACE GATEWAY_NAME, L"TWSAPIClientTradingFloor.Dashboard", windowDashboardWidth, windowDashboardHeight, hInst); }

#define WM_TRAYICON (WM_APP + 101)

#define TIMER_WATCHDOG 1
#define TIMER_MARKET_CLOCK 2 // Live US market-session clock (1s tick)

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

bool shouldBeConnected = true;

struct QuickLink { const char* label; const char* url; };
static const QuickLink g_QuickLinks[] = {
    { "Today", "https://www.investing.com/dividends-calendar" },
    { "WSB",   "https://www.reddit.com/r/wallstreetbets"      },
    { "Scan",  "https://stockscan.io/all-stocks"              },
    { "List",  "https://stockanalysis.com/list/"              },
    { "Map",   "https://finviz.com/map.ashx?t=sec"            },
    { "Data",  "https://www.benzinga.com/quote"               },
    { "Paper", "http://192.168.1.105:2025/paper"              },
    { "Chat",  "https://192.168.1.105/openclaw/chat"          },
    { "GitHub", "https://github.com/ctubio/ibkr-gateway-trading-floor" },
};
static const int LINKS_COUNT = (int)(sizeof(g_QuickLinks) / sizeof(g_QuickLinks[0]));

static bool fullDetails = true;

static const char* day_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

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

static HWND hCoinBox1 = NULL;
static HWND hCoinBox2 = NULL;
static HWND hCoinBox3 = NULL;

static HWND hLblDividends = NULL;
static HWND hLblAccruals  = NULL;
static HWND hLblBP        = NULL;
static HWND hLblMM        = NULL;
static HWND hLblEUR = NULL;
static HWND hLblUSD = NULL;

static HWND hCoin_NetLiq      = NULL;
static HWND hCoin_BigPnL      = NULL;
static HWND hCoin_Pct         = NULL;
static HWND hCoin_Realized    = NULL;
static HWND hCoin_Speaker     = NULL;   // speaker icon button (SS_NOTIFY)
static HWND hCoin_Lock        = NULL;   // lock icon button (WM_KEYDOWN VK_SCROLL)

static HWND hCoin_Positions   = NULL;
static HWND hCoin_Unrealized  = NULL;
static HWND hCoin_Dividends   = NULL;
static HWND hCoin_Accruals    = NULL;
static HWND hCoin_BuyingPower = NULL;
static HWND hCoin_MaintMargin = NULL;

static HWND hCoin_Cash        = NULL;
static HWND hCoin_Clock       = NULL;
static HWND hCoin_EUR         = NULL;
static HWND hCoin_USD         = NULL;

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
    if (!hCoin_Clock) return;

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
    const int T_OPEN_BELL = 9 * 3600 + 20 * 60;           // 09:20 ET (10m before 09:30)
    const int T_OPEN = 9 * 3600 + 30 * 60;                // 09:30 ET
    const int T_CLOSE_IMBAL = 15 * 3600 + 30 * 60;        // 15:30 ET (20m before 15:50)
    const int T_CLOSE_BELL = 15 * 3600 + 50 * 60;         // 15:50 ET (10m before 16:00)
    const int T_CLOSE = 16 * 3600;                        // 16:00 ET
    const int T_AFTERHOURS_END = 20 * 3600;               // 20:00 ET
    const int T_SUNDAY_OPEN = 20 * 3600;                  // 20:00 ET (Sunday 8:00 PM)
    
    std::string phase;
    int target_secs = 0;

    // 1. Handle Weekend / Market Closed State (Targeting Sunday 20:00 ET)
    if (wd == std::chrono::Friday && total_secs >= T_AFTERHOURS_END) {
        phase = "Market Closed"; 
        target_secs = (2 * 24 * 3600) + T_SUNDAY_OPEN; // Fri to Sun 20:00
        SetCtrlColor(hCoin_Clock, COINS_CLR_GRAY);
    } else if (wd == std::chrono::Saturday) {
        phase = "Market Closed"; 
        target_secs = (1 * 24 * 3600) + T_SUNDAY_OPEN; // Sat to Sun 20:00
        SetCtrlColor(hCoin_Clock, COINS_CLR_GRAY);
    } else if (wd == std::chrono::Sunday && total_secs < T_SUNDAY_OPEN) {
        phase = "Market Closed"; 
        target_secs = T_SUNDAY_OPEN;                   // Sunday morning/afternoon to Sun 20:00
        SetCtrlColor(hCoin_Clock, COINS_CLR_GRAY);
    } else if (wd == std::chrono::Sunday && total_secs >= T_SUNDAY_OPEN) {
        phase = "OVERNIGHT"; 
        target_secs = (1 * 24 * 3600) + T_PREMARKET;   // Market is OPEN! Counting to Mon 04:00 AM
        SetCtrlColor(hCoin_Clock, COINS_CLR_GRAY);
    } 
    // 2. Standard Weekday State Machine (Monday - Friday)
    else {
        if (total_secs < T_PREMARKET) {
            phase = "OVERNIGHT"; target_secs = T_PREMARKET;
            SetCtrlColor(hCoin_Clock, COINS_CLR_GRAY);
        } else if (total_secs < T_OPEN_IMBAL) {
            phase = "Pre-Market"; target_secs = T_OPEN;
            SetCtrlColor(hCoin_Clock, COINS_CLR_ORANGE);
        } else if (total_secs < T_OPEN_BELL) {
            phase = "IMBALANCE"; target_secs = T_OPEN;
            SetCtrlColor(hCoin_Clock, COINS_CLR_RED);
        } else if (total_secs < T_OPEN) {
            phase = "Opening Bell"; target_secs = T_OPEN;
            SetCtrlColor(hCoin_Clock, COINS_CLR_BLUE);
        } else if (total_secs < T_CLOSE_IMBAL) {
            phase = "Market Open"; target_secs = T_CLOSE;
            SetCtrlColor(hCoin_Clock, COINS_CLR_YELLOW);
        } else if (total_secs < T_CLOSE_BELL) {
            phase = "IMBALANCE"; target_secs = T_CLOSE;
            SetCtrlColor(hCoin_Clock, COINS_CLR_RED);
        } else if (total_secs < T_CLOSE) {
            phase = "Closing Bell"; target_secs = T_CLOSE;
            SetCtrlColor(hCoin_Clock, COINS_CLR_BLUE);
        } else if (total_secs < T_AFTERHOURS_END) {
            phase = "After-Hours"; target_secs = T_AFTERHOURS_END;
            SetCtrlColor(hCoin_Clock, COINS_CLR_ORANGE);
        } else {
            phase = "OVERNIGHT"; target_secs = T_PREMARKET + (24 * 3600); // Tomorrow's 04:00 AM
            SetCtrlColor(hCoin_Clock, COINS_CLR_GRAY);
        }
    }

    if (fullDetails) { 
        std::string day_str = day_names[wd.c_encoding()];
        std::string time_str = day_str + " " + std::format("{:02}:{:02}", time_of_day.hours().count(), time_of_day.minutes().count());
        SetWindowTextA(hWnd, time_str.c_str());
    } else SetWindowTextA(hWnd, phase.c_str());

    // Calculate time left
    int secs_left = target_secs - total_secs;
    int h_left = secs_left / 3600;
    int m_left = (secs_left % 3600) / 60;

    // Format output string (HH:MM)
    std::string time_left_str = std::format("{:02}:{:02}", h_left, m_left);

    SetWindowTextA(hCoin_Clock, time_left_str.c_str());
    InvalidateRect(hCoin_Clock, NULL, TRUE);
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

// ─── TTS state ────────────────────────────────────────────────────────────────
static ISpVoice* g_pCoinsVoice  = nullptr;
static bool      g_coinsTtsOn   = false;
static bool      g_coinsComInit = false;

// ─── TTS helpers ──────────────────────────────────────────────────────────────

static bool Coins_InitVoice() {
    if (g_pCoinsVoice) return true;

    if (!g_coinsComInit) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        g_coinsComInit = SUCCEEDED(hr) || (hr == RPC_E_CHANGED_MODE);
    }

    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&g_pCoinsVoice);
    if (FAILED(hr)) { g_pCoinsVoice = nullptr; return false; }

    TTS_ApplySavedVoice(g_pCoinsVoice);
    return true;
}

static void Coins_SpeakDailyPnL() {
    if (!g_pCoinsVoice || !hCoin_BigPnL) return;
    char buf[128] = {};
    GetWindowTextA(hCoin_BigPnL, buf, sizeof(buf));
    std::wstring wtext(buf, buf + strlen(buf));
    size_t dotPos = wtext.find(L'.');
    if (dotPos != std::wstring::npos) {
        wtext.erase(dotPos);
    }
    if (!wtext.empty() && wtext.front() == L'+') {
        wtext.erase(0, 1);
    }
    wtext.erase(std::remove(wtext.begin(), wtext.end(), L','), wtext.end());
    g_pCoinsVoice->Speak(wtext.c_str(), SVSFlagsAsync | SVSFPurgeBeforeSpeak, NULL);
}

static void Coins_ToggleTTS(HWND hWnd) {
    g_coinsTtsOn = !g_coinsTtsOn;

    // Persist state to registry
    RegSetDword(DASHBOARD_CLASS_NAME, "Speaker", g_coinsTtsOn ? 1 : 0);

    if (g_coinsTtsOn) {
        if (!Coins_InitVoice()) {
            g_coinsTtsOn = false;
            return;
        }
        SetCtrlColor(hCoin_Speaker, Settings_DarkMode() ? COINS_CLR_WHITE : COINS_CLR_BLACK);   // bright = active
        SetTimer(hWnd, TIMER_COINS_SPEAKER, 21000, NULL);
        Coins_SpeakDailyPnL();                          // speak immediately
    } else {
        KillTimer(hWnd, TIMER_COINS_SPEAKER);
        if (g_pCoinsVoice)
            g_pCoinsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL); // stop current
        SetCtrlColor(hCoin_Speaker, COINS_CLR_GRAY);    // dim = inactive
    }

    if (hCoin_Speaker) InvalidateRect(hCoin_Speaker, NULL, TRUE);
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

void UpdateTrayIcon(HWND hWnd, std::string currency = "???") {
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
            tooltip = acc + " | " + currency + " | " + (connected ? "Connected" : "Disconnected");
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

void Coins_UpdateLabels(HWND hWnd) {
    auto   summary    = api().getAccountSummary();
    double daily      = api().getDailyPnL();
    double unrealized = api().getUnrealizedPnL();
    double realized   = api().getRealizedPnL();

    std::string currency = "???";
    double netLiq = 0.0;
    auto tryParse = [&](const std::string& k) -> double {
        auto it = summary.find(k);
        if (it == summary.end()) return 0.0;
        try { return std::stod(it->second); } catch (...) { return 0.0; }
    };
    if (summary.count("EUR_NetLiquidation")) {
        currency = "EUR"; netLiq = tryParse("EUR_NetLiquidation");
    } else if (summary.count("USD_NetLiquidation")) {
        currency = "USD"; netLiq = tryParse("USD_NetLiquidation");
    } else if (summary.count("NetLiquidation")) {
        netLiq = tryParse("NetLiquidation");
        for (auto const& [k, v] : summary)
            if (k.find("_NetLiquidation") != std::string::npos)
                { currency = k.substr(0, k.find('_')); break; }
    }
    
    UpdateTrayIcon(hWnd, currency);
    
    const int m = 10;
    int y1 = 8;
    int box1H = 94;
    int y2 = y1 + box1H + 9;
    int box2H = 124;
    int y3 = y2 + box2H + 9;

    if (hCoin_NetLiq) {
        std::string formattedNum = FormatWithCommas(netLiq, fullDetails);
        SetWindowTextA(hCoin_NetLiq, formattedNum.c_str());
        int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
        SetWindowPos(hCoin_NetLiq, NULL, m + 10 + 48, y1 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hCoin_NetLiq, NULL, TRUE);
    }

    COLORREF pnlClr = daily >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
    if (hCoin_BigPnL) {
        std::string formattedNum = FormatWithCommas(daily);
        if (daily >= 0.0) formattedNum = "+" + formattedNum;
        SetWindowTextA(hCoin_BigPnL, formattedNum.c_str());
        SetCtrlColor(hCoin_BigPnL, pnlClr);
        InvalidateRect(hCoin_BigPnL, NULL, TRUE);
    }
    if (hCoin_Pct) {
        double pct = (netLiq != 0.0) ? (daily / netLiq * 100.0) : 0.0;
        std::string formattedNum = FormatWithCommas(pct);
        if (pct >= 0.0) formattedNum = "+" + formattedNum;
        SetWindowTextA(hCoin_Pct, std::format("{}%", formattedNum).c_str());
        SetCtrlColor(hCoin_Pct, pnlClr);
        InvalidateRect(hCoin_Pct, NULL, TRUE);
    }
    if (hCoin_Realized) {
        std::string formattedNum = FormatWithCommas(realized);
        COLORREF clr = realized >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
        SetWindowTextA(hCoin_Realized, formattedNum.c_str());
        SetCtrlColor(hCoin_Realized, clr);
        InvalidateRect(hCoin_Realized, NULL, TRUE);
    }

    if (hCoin_Positions) {
        double grossPos = tryParse("GrossPositionValue");
        std::string formattedNum = FormatWithCommas(grossPos, fullDetails);
        SetWindowTextA(hCoin_Positions, formattedNum.c_str());
        int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
        SetWindowPos(hCoin_Positions, NULL, m + 10 + 70, y2 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hCoin_Positions, NULL, TRUE);
    }
    if (hCoin_Unrealized) {
        std::string formattedNum = FormatWithCommas(unrealized, fullDetails);
        COLORREF clr = unrealized >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
        SetWindowTextA(hCoin_Unrealized, formattedNum.c_str());
        SetCtrlColor(hCoin_Unrealized, clr);
        InvalidateRect(hCoin_Unrealized, NULL, TRUE);
    }
    if (hCoin_Dividends) {
        double div = tryParse("AccruedDividend");
        std::string formattedNum = FormatWithCommas(div);
        SetWindowTextA(hCoin_Dividends, formattedNum.c_str());
        SetCtrlColor(hCoin_Dividends, COINS_CLR_PURPLE);
        InvalidateRect(hCoin_Dividends, NULL, TRUE);
    }
    if (hCoin_Accruals) {
        double acc = tryParse("AccruedCash");
        std::string formattedNum = FormatWithCommas(acc);
        COLORREF clr = acc > 0.0 ? COINS_CLR_GREEN : (acc < 0.0 ? COINS_CLR_RED : COLOR_THEME);
        SetWindowTextA(hCoin_Accruals, formattedNum.c_str());
        SetCtrlColor(hCoin_Accruals, clr);
        InvalidateRect(hCoin_Accruals, NULL, TRUE);
    }
    if (hCoin_BuyingPower) {
        double bp = tryParse("BuyingPower");
        std::string formattedNum = FormatWithCommas(bp, fullDetails);
        SetWindowTextA(hCoin_BuyingPower, formattedNum.c_str());
    }
    if (hCoin_MaintMargin) {
        double mm = tryParse("MaintMarginReq");
        std::string formattedNum = FormatWithCommas(mm, fullDetails);
        SetWindowTextA(hCoin_MaintMargin, formattedNum.c_str());
    }

    if (hCoin_Cash) {
        double cash = tryParse("CashBalance");
        std::string formattedNum = FormatWithCommas(cash, fullDetails) + " " + currency;
        COLORREF clr = cash > 0.0 ? COINS_CLR_GREEN : (cash < 0.0 ? COINS_CLR_RED : COLOR_THEME);
        SetWindowTextA(hCoin_Cash, formattedNum.c_str());
        SetCtrlColor(hCoin_Cash, clr);
        int w2 = Coins_GetTextWidth(hWnd, hFont14ptbold.get(), formattedNum.c_str());
        SetWindowPos(hCoin_Cash, NULL, m + 10 + 45, y3 - 3, w2 + 4, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        InvalidateRect(hCoin_Cash, NULL, TRUE);
    }
    if (hCoin_EUR) {
        double eur = tryParse("EUR_CashBalance");
        std::string formattedNum = FormatWithCommas(eur, fullDetails) + (fullDetails ? "" : " €");
        COLORREF clr = eur > 0.0 ? COINS_CLR_GREEN : (eur < 0.0 ? COINS_CLR_RED : COLOR_THEME);
        SetWindowTextW(hCoin_EUR, StringToWide(formattedNum).c_str());
        SetCtrlColor(hCoin_EUR, clr);
        InvalidateRect(hCoin_EUR, NULL, TRUE);
    }
    if (hCoin_USD) {
        double usd = tryParse("USD_CashBalance");
        std::string formattedNum = FormatWithCommas(usd, fullDetails) + (fullDetails ? "" : " $");
        COLORREF clr = usd > 0.0 ? COINS_CLR_GREEN : (usd < 0.0 ? COINS_CLR_RED : COLOR_THEME);
        SetWindowTextW(hCoin_USD, StringToWide(formattedNum).c_str());
        SetCtrlColor(hCoin_USD, clr);
        InvalidateRect(hCoin_USD, NULL, TRUE);
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
            hCoinBox1 = CreateWindowA("BUTTON", "Today:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y1, boxW, box1H, hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hCoinBox1, DarkGroupBoxSubclassProc, 1, 0);
            SendMessage(hCoinBox1, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_NetLiq = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 62, y1 - 4, 30, 18, hWnd, (HMENU)ID_COIN_NETLIQ, hInst, NULL);
            SendMessage(hCoin_NetLiq, WM_SETFONT, (WPARAM)hFont14ptbold.get(), TRUE);

            // Lock icon (hidden by default)
            hCoin_Lock = CreateWindowW(L"STATIC", LOCK_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + boxW - 25, y1, 20, 18, hWnd, (HMENU)ID_COIN_LOCK, hInst, NULL);
            SendMessage(hCoin_Lock, WM_SETFONT, (WPARAM)hFont_Icons, TRUE);
            SetCtrlColor(hCoin_Lock, COINS_CLR_GRAY);
            ShowWindow(hCoin_Lock, SW_HIDE);

            hCoin_Clock = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOTIFY,
                m + boxW - 60, y1, 40, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_Clock, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            // Row 1: PnL: 🔊 +0.00
            HWND hLblBigPnL = CreateWindowA("STATIC", "PnL:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 24, 30, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblBigPnL, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Speaker = CreateWindowW(L"STATIC", SPEAKER_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + 42, y1 + 24, 20, 20, hWnd, (HMENU)ID_COIN_SPEAKER, hInst, NULL);
            SendMessage(hCoin_Speaker, WM_SETFONT, (WPARAM)hFont_Icons, TRUE);
            SetCtrlColor(hCoin_Speaker, COINS_CLR_GRAY);

            hCoin_BigPnL = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOTIFY,
                m + 70, y1 + 16, boxW - 82, 32, hWnd, (HMENU)ID_COIN_BIGPNL, hInst, NULL);
            SendMessage(hCoin_BigPnL, WM_SETFONT, (WPARAM)hFont21ptbold.get(), TRUE);
            SetCtrlColor(hCoin_BigPnL, COINS_CLR_GREEN);

            // Row 2: PnL %: +0.00%
            HWND hLblPct = CreateWindowA("STATIC", "PnL %:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 48, 50, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblPct, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Pct = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 70, y1 + 48, boxW - 82, 18, hWnd, (HMENU)ID_COIN_PCT, hInst, NULL);
            SendMessage(hCoin_Pct, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);
            SetCtrlColor(hCoin_Pct, COINS_CLR_GREEN);

            // Row 3: Realized: 0.00
            HWND hLblRealized = CreateWindowA("STATIC", "Realized:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y1 + 68, 65, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblRealized, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Realized = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 77, y1 + 68, boxW - 89, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_Realized, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);


            // ─── Box 2: Positions & Margin ─────────────────────────────────────
            int y2 = y1 + box1H + 9; // y2 = 114
            int box2H = 124;
            hCoinBox2 = CreateWindowA("BUTTON", "Positions:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y2, boxW, box2H, hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hCoinBox2, DarkGroupBoxSubclassProc, 2, 0);
            SendMessage(hCoinBox2, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Positions = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 77, y2 - 4, 30, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_Positions, WM_SETFONT, (WPARAM)hFont14ptbold.get(), TRUE);

            // Row 1: Unrealized: 0.00
            HWND hLblUnrealized = CreateWindowA("STATIC", "Unrealized:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 20, 75, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblUnrealized, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Unrealized = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 20, boxW - 102, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_Unrealized, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 2: Dividends: 33.75
            hLblDividends = CreateWindowA("STATIC", "Dividends:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 40, 75, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblDividends, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Dividends = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 40, boxW - 102, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_Dividends, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 3: Accruals: -1.64
            hLblAccruals = CreateWindowA("STATIC", "Accruals:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 60, 75, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblAccruals, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Accruals = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 90, y2 + 60, boxW - 102, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_Accruals, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 4: Buying Power: 86,483.04
            hLblBP = CreateWindowA("STATIC", "Buying Power:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 80, 100, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblBP, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_BuyingPower = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 105, y2 + 80, boxW - 117, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_BuyingPower, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 5: Maint Margin: 4,403.05
            hLblMM = CreateWindowA("STATIC", "Maintenance:", //  Margin:
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y2 + 100, 85, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblMM, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_MaintMargin = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 97, y2 + 100, boxW - 109, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_MaintMargin, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);


            // ─── Box 3: Cash ───────────────────────────────────────────────────
            int y3 = y2 + box2H + 9; // y3 = 250
            int box3H = 64;
            hCoinBox3 = CreateWindowA("BUTTON", "Cash:", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                m, y3, boxW, box3H, hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hCoinBox3, DarkGroupBoxSubclassProc, 3, 0);
            SendMessage(hCoinBox3, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_Cash = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 53, y3 - 4, 30, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_Cash, WM_SETFONT, (WPARAM)hFont14ptbold.get(), TRUE);

            // Row 1: EUR: 285.31
            hLblEUR = CreateWindowA("STATIC", "EUR:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y3 + 20, 35, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblEUR, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_EUR = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 50, y3 + 20, boxW - 62, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_EUR, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);

            // Row 2: USD: -11,559.66
            hLblUSD = CreateWindowA("STATIC", "USD:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m + 12, y3 + 40, 35, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblUSD, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);

            hCoin_USD = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + 50, y3 + 40, boxW - 62, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hCoin_USD, WM_SETFONT, (WPARAM)hFont12ptbold.get(), TRUE);


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
            std::thread([hWnd]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(721));
                SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
            }).detach();

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
            LONG_PTR style = GetWindowLongPtr(hCoin_EUR, GWL_STYLE);
            if (LOWORD(wParam) != WA_INACTIVE) {
                if (lockHotkeys) break;
                ShowWindow(hLblDividends, SW_SHOW);
                ShowWindow(hLblAccruals, SW_SHOW);
                ShowWindow(hCoin_Accruals, SW_SHOW);
                ShowWindow(hCoin_Dividends, SW_SHOW);
                ShowWindow(hCoin_Cash, SW_SHOW);
                ShowWindow(hLblEUR, SW_SHOW);
                ShowWindow(hLblUSD, SW_SHOW);
                SetWindowPos(hCoinBox2, NULL, m, y2, boxW, box2H, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hLblBP, NULL,  m + 12, y2 + 80, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoin_BuyingPower, NULL, m + 105, y2 + 80, boxW - 117, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hLblMM, NULL, m + 12, y2 + 100, 85, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoin_MaintMargin, NULL, m + 97, y2 + 100, boxW - 109, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoinBox3, NULL, m, y3, boxW, box3H, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoin_EUR, NULL, m + 50, y3 + 20, boxW - 62, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                style &= ~SS_TYPEMASK;
                SetWindowLongPtr(hCoin_EUR, GWL_STYLE, style | SS_RIGHT);
                SetWindowPos(hCoin_USD, NULL, m + 50, y3 + 40, boxW - 62, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                MoveWindow(hWnd, windowRect.left, windowRect.top, windowDashboardWidth, windowDashboardHeight     , TRUE);
                fullDetails = true;
            } else {
                ShowWindow(hLblDividends, SW_HIDE);
                ShowWindow(hLblAccruals, SW_HIDE);
                ShowWindow(hCoin_Accruals, SW_HIDE);
                ShowWindow(hCoin_Dividends, SW_HIDE);
                ShowWindow(hCoin_Cash, SW_HIDE);
                ShowWindow(hLblEUR, SW_HIDE);
                ShowWindow(hLblUSD, SW_HIDE);
                SetWindowPos(hCoinBox2, NULL, m, y2, boxW, box2H - 40, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hLblBP, NULL,  m + 12, y2 + 40, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoin_BuyingPower, NULL, m + 105, y2 + 40, boxW - 117, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hLblMM, NULL, m + 12, y2 + 60, 85, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoin_MaintMargin, NULL, m + 97, y2 + 60, boxW - 109, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoinBox3, NULL, m, y3 - 40, boxW, box3H - 20, SWP_NOZORDER | SWP_NOACTIVATE);
                SetWindowPos(hCoin_EUR, NULL, m + 12, y3 + 20 - 40, 100, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                style &= ~SS_TYPEMASK;
                SetWindowLongPtr(hCoin_EUR, GWL_STYLE, style | SS_LEFT);
                SetWindowPos(hCoin_USD, NULL, m + 112, y3 + 20 - 40, boxW - 124, 18, SWP_NOZORDER | SWP_NOACTIVATE);
                MoveWindow(hWnd, windowRect.left, windowRect.top, windowDashboardWidth, windowDashboardHeight - 60 - 38, TRUE);
                fullDetails = false;
            }
            if (api().isMarketDataConnected() && api().isTradingConnected()) {
                Coins_UpdateLabels(hWnd);
            }
            UpdateMarketClock(hWnd);
            break;
        }

        case WM_API_UPDATE:
            UpdateTrayIcon(hWnd);
            if (!api().isMarketDataConnected() || !api().isTradingConnected()) {
                const int m    = 10;
                const int boxW = 226;
                int box1H = 94;
                int y1 = 8;
                int y2 = y1 + box1H + 9; // y2 = 114
                int box2H = 124;
                int y3 = y2 + box2H + 9; // y3 = 250
                int box3H = 64;
                if (hCoin_NetLiq)      { SetWindowTextA(hCoin_NetLiq,      "--"); SetWindowPos(hCoin_NetLiq, NULL, m + 67, y1 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE); InvalidateRect(hCoin_NetLiq, NULL, TRUE); }
                if (hCoin_BigPnL)      SetWindowTextA(hCoin_BigPnL,      "--");
                if (hCoin_Pct)         SetWindowTextA(hCoin_Pct,         "--");
                if (hCoin_Realized)    SetWindowTextA(hCoin_Realized,    "--");
                if (hCoin_Positions)   { SetWindowTextA(hCoin_Positions,   "--"); SetWindowPos(hCoin_Positions, NULL, m + 77, y2 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE); InvalidateRect(hCoin_Positions, NULL, TRUE); }
                if (hCoin_Unrealized)  SetWindowTextA(hCoin_Unrealized,  "--");
                if (hCoin_Dividends)   SetWindowTextA(hCoin_Dividends,   "--");
                if (hCoin_Accruals)    SetWindowTextA(hCoin_Accruals,    "--");
                if (hCoin_BuyingPower) SetWindowTextA(hCoin_BuyingPower, "--");
                if (hCoin_MaintMargin) SetWindowTextA(hCoin_MaintMargin, "--");
                if (hCoin_Cash)        { SetWindowTextA(hCoin_Cash,        "--"); SetWindowPos(hCoin_Cash, NULL, m + 53, y3 - 4, 30, 18, SWP_NOZORDER | SWP_NOACTIVATE); InvalidateRect(hCoin_Cash, NULL, TRUE); }
                if (hCoin_EUR)         SetWindowTextA(hCoin_EUR,         "--");
                if (hCoin_USD)         SetWindowTextA(hCoin_USD,         "--");
            }
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
            if (wParam == TIMER_WATCHDOG) { // 10000
                if (shouldBeConnected && !api().isConnected()) {
#ifndef GATEWAY_SIM
                    EnsureGatewayRunning(hWnd);
#endif
                    api().connect(
#ifndef GATEWAY_SIM
                        std::filesystem::path(GetGatewayPath()).filename() == "ibgateway.exe" ? 4001 : 7496
#else
                        0
#endif
                    , (int)Settings_Load("ClientId", 0)
                    , (int)Settings_Load("GroupId", 4)
                    );
                    UpdateTrayIcon(hWnd);
                } else if (!shouldBeConnected && api().isConnected()) {
                    api().disconnect();
                    UpdateTrayIcon(hWnd);
                }
            }
            break;

        case WM_OPEN_ORDERS_WINDOW:
            StartOrders();
            break;

        case WM_TTS_VOICE_CHANGED: {
            // Hot-swap the TTS voice without closing the window.
            // Release the current voice object so the next Coins_InitVoice call
            // picks up the newly saved token from the registry.
            if (g_pCoinsVoice) {
                g_pCoinsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
                g_pCoinsVoice->Release();
                g_pCoinsVoice = nullptr;
            }
            // If TTS is currently active, re-initialise with the new voice and
            // restart the timer so it fires on the normal 21-second cadence.
            if (g_coinsTtsOn) {
                KillTimer(hWnd, TIMER_COINS_SPEAKER);
                if (Coins_InitVoice()) {
                    SetTimer(hWnd, TIMER_COINS_SPEAKER, 21000, NULL);
                    Coins_SpeakDailyPnL(); // speak immediately with the new voice
                } else {
                    g_coinsTtsOn = false;
                    SetCtrlColor(hCoin_Speaker, COINS_CLR_GRAY);
                    if (hCoin_Speaker) InvalidateRect(hCoin_Speaker, NULL, TRUE);
                }
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
                    if (FindWindowA(DASHBOARD_CLASS_NAME, NULL)) AppendMenuW(hMenu, MF_STRING, ID_M_DASHBOARD, IsWindowAlwaysOnTop(DASHBOARD_CLASS_NAME) ? L"[ ★ ] Dashboard" : L"[  ] Dashboard");
                    if (FindWindowA(DIAMONDS_CLASS_NAME, NULL))  AppendMenuW(hMenu, MF_STRING, ID_M_DIAMONDS,  IsWindowAlwaysOnTop(DIAMONDS_CLASS_NAME)  ? L"[ ★ ] Diamonds"  : L"[  ] Diamonds");
                    if (FindWindowA(ORDERS_CLASS_NAME, NULL))    AppendMenuW(hMenu, MF_STRING, ID_M_ORDERS,    IsWindowAlwaysOnTop(ORDERS_CLASS_NAME)    ? L"[ ★ ] Orders"    : L"[  ] Orders");

                    auto tsWindows = EnumerateMarketWindows();
                    std::sort(tsWindows.begin(), tsWindows.end(), [](const auto& a, const auto& b) {
                        return a.symbol < b.symbol;
                    });
                    for (size_t i = 0; i < tsWindows.size() && i < ID_M_MARKET_MAX; ++i) {
                        std::wstring label = IsMarketAlwaysOnTop(tsWindows[i].symbol) ? 
                            L"[ ★ ] Market: " + StringToWide(tsWindows[i].symbol) : 
                            L"[  ] Market: " + StringToWide(tsWindows[i].symbol);
                            
                        AppendMenuW(hMenu, MF_STRING, ID_M_MARKET_BASE + (int)i, label.c_str());
                    }
                    
                    if (FindWindowA(SETTINGS_CLASS_NAME, NULL))  AppendMenuW(hMenu, MF_STRING, ID_M_SETTINGS,  IsWindowAlwaysOnTop(SETTINGS_CLASS_NAME)  ? L"[ ★ ] Settings"  : L"[  ] Settings");
                    if (FindWindowA(DEBUGLOG_CLASS_NAME, NULL))  AppendMenuW(hMenu, MF_STRING, ID_M_DEBUGLOG,  IsWindowAlwaysOnTop(DEBUGLOG_CLASS_NAME)  ? L"[ ★ ] Debug Log" : L"[  ] Debug Log");

                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
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
                lockHotkeys = !lockHotkeys;
                ShowWindow(hCoin_Lock, lockHotkeys ? SW_SHOW : SW_HIDE);
                PostMessage(hWnd, WM_ACTIVATE, lockHotkeys ? WA_INACTIVE : WA_ACTIVE, 0);
                return 0;
            }
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
                    shouldBeConnected = true;
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_DISCONNECT:
                    shouldBeConnected = false;
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
                        if (g_QuickLinks[i].label == "Paper" || g_QuickLinks[i].label == "GitHub")
                            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                        AppendMenuA(hMenu, MF_STRING, ID_M_LINKS_BASE + i, g_QuickLinks[i].label);   
                    }

                    SetForegroundWindow(hWnd);
                    int cmd = TrackPopupMenu(hMenu,
                        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
                        rc.left, rc.bottom, 0, hWnd, NULL);
                    DestroyMenu(hMenu);

                    if (cmd >= ID_M_LINKS_BASE && cmd < ID_M_LINKS_BASE + LINKS_COUNT) {
                        ShellExecuteA(NULL, "open", g_QuickLinks[cmd - ID_M_LINKS_BASE].url, NULL, NULL, SW_SHOWNORMAL);
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
            if (g_coinsTtsOn) KillTimer(hWnd, TIMER_COINS_SPEAKER);
            if (g_pCoinsVoice) {
                g_pCoinsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
                g_pCoinsVoice->Release();
                g_pCoinsVoice = nullptr;
            }
            g_coinsTtsOn = false;

            hLblEUR = hLblUSD = hCoin_NetLiq = hCoin_BigPnL = hCoin_Pct = hCoin_Realized = hCoin_Speaker = hCoin_Lock = NULL;
            hCoin_Positions = hCoin_Unrealized = hCoin_Dividends = hCoin_Accruals = hCoin_BuyingPower = hCoin_MaintMargin = NULL;
            hCoin_Clock = hLblBP = hLblMM = hLblDividends = hLblAccruals = hCoinBox1 = hCoinBox2 = hCoinBox3 = hCoin_Cash = hCoin_EUR = hCoin_USD = NULL;
            gClrCount = 0;

            PostQuitMessage(0);
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}