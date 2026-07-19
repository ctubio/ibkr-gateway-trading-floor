#pragma once

int windowDashboardWidth  = 250;
int windowDashboardHeight = 450;

void StartDashboard(HINSTANCE hInst) { StartGenericWindow(DASHBOARD_CLASS_NAME, "Trading Floor: Offline", L"TWSAPIClientTradingFloor.Dashboard", windowDashboardWidth, windowDashboardHeight, hInst); }

#define WM_TRAYICON (WM_APP + 101)

#define TIMER_WATCHDOG 1

#define ID_M_DASHBOARD   1001
#define ID_MB_DIAMONDS   1003
#define ID_MB_SETTINGS   1004
#define ID_MB_SCANNER    1005
#define ID_MB_WATCHLIST  1006
#define ID_MB_MARKET     1007
#define ID_MB_ORDERS     1008
#define ID_M_ORDERS      1009
#define ID_M_DIAMONDS    1011
#define ID_M_SETTINGS    1012
#define ID_M_SCANNER     1013
#define ID_M_WATCHLIST   1014
#define ID_M_MARKET      1015
#define ID_M_DEBUGLOG    1016


#define ID_M_CONNECT    1100
#define ID_M_DISCONNECT 1101
#define ID_M_EXIT       1102

#define ID_M_MARKET_BASE 1500
#define ID_M_MARKET_MAX   100

bool shouldBeConnected = true;

// ─── Fonts ────────────────────────────────────────────────────────────────────
static HFONT hFontCoins_NetLiq  = NULL;
static HFONT hFontCoins_BigPnL  = NULL;
static HFONT hFontCoins_Pct     = NULL;
static HFONT hFontCoins_Label   = NULL;
static HFONT hFontCoins_Value   = NULL;
static HFONT hFontCoins_Icons = NULL;   // Segoe MDL2 Assets for speaker glyph

void MutexGatewayInstance() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\TWSAPIClientTradingFloorMutex_17072025");

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
    }
}

static HFONT Coins_MakeMDL2Font(int ptSize) {
    HDC hdc = GetDC(NULL);
    int h   = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    return CreateFontW(h, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe MDL2 Assets");
}

// ─── Header controls ──────────────────────────────────────────────────────────
static HWND hCoin_NetLiq  = NULL;
static HWND hCoin_BigPnL  = NULL;
static HWND hCoin_Pct     = NULL;
static HWND hCoin_Speaker = NULL;   // speaker icon button (SS_NOTIFY)
static HWND hCoin_FxIcon  = NULL;   // exchange-currency icon button (SS_NOTIFY) — color never changes

#define ID_COIN_NETLIQ   5100
#define ID_COIN_BIGPNL   5101
#define ID_COIN_PCT      5102
#define ID_COIN_SPEAKER  5103
#define ID_COIN_FXICON   5104
#define TIMER_COINS_SPEAKER  0xC015   // WM_TIMER id

// ─── Exchange Currency popup (DASHBOARD_EXCHANGE_CLASS_NAME) ───────────────────
#define ID_DASHFX_ACTION_COMBO  5201
#define ID_DASHFX_AMOUNT_EDIT   5202

// ─── TTS state ────────────────────────────────────────────────────────────────
static ISpVoice* g_pCoinsVoice  = nullptr;
static bool      g_coinsTtsOn   = false;
static bool      g_coinsComInit = false;

// ─── Row data ─────────────────────────────────────────────────────────────────
struct CoinRow { const char* label; const char* key; int colorType; bool isSeparator; };

static CoinRow coinRows[] = {
    { "Unrealized PnL:",   "UnrealizedPnL",       1, false },
    { "Realized PnL:",     "RealizedPnL",         1, false },
    { "Dividends:",        "AccruedDividend",     2, false },
    { nullptr, nullptr, 0, true },
    { "Gross Position:",   "GrossPositionValue",  0, false },
    { "Buying Power:",     "BuyingPower",         0, false },
 // { "Available Funds:",  "AvailableFunds",      0, false },
 // { "Excess Liquidity:", "ExcessLiquidity",     0, false },
 // { "Init Margin:",      "InitMarginReq",       0, false },
    { "Maint Margin:",     "MaintMarginReq",      0, false },
    { "Accrued Cash:",     "AccruedCash",         3, false },
    { nullptr, nullptr, 0, true },
    { "EUR Cash:",         "EUR_CashBalance",     3, false },
    { "USD Cash:",         "USD_CashBalance",     3, false },
    { "Total Cash:",        "CashBalance",        3, false },
};
static const int COIN_ROW_COUNT = (int)(sizeof(coinRows) / sizeof(coinRows[0]));

#define ID_COINS_LABEL_BASE 5000

static HWND hCoinLbl[COIN_ROW_COUNT] = {};
static HWND hCoinVal[COIN_ROW_COUNT] = {};

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
    std::string tooltip = "Trading Floor: ";
    std::string title;
    bool connected = false;

    if (!api().isConnected()) {
        title    = "Offline";
        tooltip += title;
    } else {
        std::string acc = api().getAccountNumber();
        if (acc.empty()) {
            title    = "Connecting..";
            tooltip += title;
        } else {
            connected = api().isMarketDataConnected() && api().isTradingConnected();
            tooltip = acc + " | " + currency + " | " + (connected ? "Connected" : "Disconnected");
            title   = acc;
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
            
    SetWindowTextA(hWnd, title.c_str());
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
    
    if (hCoin_NetLiq) {
        std::string formattedNum = FormatWithCommas(netLiq);
        SetWindowTextA(hCoin_NetLiq, formattedNum.c_str());
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

    for (int i = 0; i < COIN_ROW_COUNT; i++) {
        if (coinRows[i].isSeparator || !hCoinVal[i]) continue;

        std::string raw;
        if      (strcmp(coinRows[i].key, "UnrealizedPnL") == 0) raw = std::to_string(unrealized);
        else if (strcmp(coinRows[i].key, "RealizedPnL")   == 0) raw = std::to_string(realized);
        else {
            auto it = summary.find(coinRows[i].key);
            raw = (it != summary.end()) ? it->second : "--";
        }

        char buf[80] = "--";
        COLORREF clr = COLOR_THEME;  // default: let theme paint it
        try {
            double d = std::stod(raw);
            if (coinRows[i].colorType == 1) {
                std::string formattedNum = FormatWithCommas(d);
                strncpy(buf, formattedNum.c_str(), sizeof(buf) - 1);
                clr = d >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
            } else if (coinRows[i].colorType == 2) {
                std::string formattedNum = FormatWithCommas(d);
                strncpy(buf, formattedNum.c_str(), sizeof(buf) - 1);
                clr = COINS_CLR_PURPLE;
            } else if (coinRows[i].colorType == 3) {
                std::string suffixStr;
                if (strcmp(coinRows[i].key, "CashBalance") == 0) {
                    suffixStr = " " + currency;
                } else if (strncmp(coinRows[i].key, "EUR_", 4) == 0) {
                    suffixStr = " EUR";
                } else if (strncmp(coinRows[i].key, "USD_", 4) == 0) {
                    suffixStr = " USD";
                }
                std::string formattedNum = FormatWithCommas(d) + suffixStr;
                strncpy(buf, formattedNum.c_str(), sizeof(buf) - 1);
                clr = d > 0.0 ? COINS_CLR_GREEN : (d < 0.0 ? COINS_CLR_RED : COLOR_THEME);
            } else {
                std::string formattedNum = FormatWithCommas(d);
                strncpy(buf, formattedNum.c_str(), sizeof(buf) - 1);
            }
        } catch (...) {}
        
        SetCtrlColor(hCoinVal[i], clr);
        SetWindowTextA(hCoinVal[i], buf);
        InvalidateRect(hCoinVal[i], NULL, TRUE);
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

    if (amount > 0.0) {
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

            hFontCoins_NetLiq  = MakeFont(14, true);
            hFontCoins_BigPnL  = MakeFont(21, true);
            hFontCoins_Pct     = MakeFont(12, true);
            hFontCoins_Value   = MakeFont(12, true);
            hFontCoins_Label   = MakeFont(11, false);
            hFontCoins_Icons = Coins_MakeMDL2Font(11);

            const int m  = 12;
            const int rW = 220;
            int lblW = 70;
            int valW = rW - lblW;

            // Net Liquidation Label
            HWND hLblNetLiq = CreateWindowA("STATIC", "Net Liq:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m, 12, lblW, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblNetLiq, WM_SETFONT, (WPARAM)hFontCoins_Label, TRUE);

            // Net Liquidation Value
            hCoin_NetLiq = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + lblW, 10, valW, 22, hWnd, (HMENU)ID_COIN_NETLIQ, hInst, NULL);
            SendMessage(hCoin_NetLiq, WM_SETFONT, (WPARAM)hFontCoins_NetLiq, TRUE);

            // Daily PnL Label
            HWND hLblBigPnL = CreateWindowA("STATIC", "Daily PnL:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m, 46, lblW, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblBigPnL, WM_SETFONT, (WPARAM)hFontCoins_Label, TRUE);

            // Speaker icon - placed immediately to the left of the BigPnL block
            hCoin_Speaker = CreateWindowW(L"STATIC", SPEAKER_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + lblW - 10, 48, 22, 22, hWnd, (HMENU)ID_COIN_SPEAKER, hInst, NULL);
            SendMessage(hCoin_Speaker, WM_SETFONT, (WPARAM)hFontCoins_Icons, TRUE);
            SetCtrlColor(hCoin_Speaker, COINS_CLR_GRAY);

            // Daily PnL Value - bounding box reduced slightly so the text pulls tight to the speaker icon
            hCoin_BigPnL = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOTIFY,
                m + lblW + 10, 36, valW - 10, 40, hWnd, (HMENU)ID_COIN_BIGPNL, hInst, NULL);
            SendMessage(hCoin_BigPnL, WM_SETFONT, (WPARAM)hFontCoins_BigPnL, TRUE);
            SetCtrlColor(hCoin_BigPnL, COINS_CLR_GREEN);

            // Daily PnL % Label
            HWND hLblPct = CreateWindowA("STATIC", "Daily PnL %:",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                m, 76, lblW, 18, hWnd, NULL, hInst, NULL);
            SendMessage(hLblPct, WM_SETFONT, (WPARAM)hFontCoins_Label, TRUE);

            // Daily PnL % Value
            hCoin_Pct = CreateWindowA("STATIC", "--",
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                m + lblW, 76, valW, 18, hWnd, (HMENU)ID_COIN_PCT, hInst, NULL);
            SendMessage(hCoin_Pct, WM_SETFONT, (WPARAM)hFontCoins_Pct, TRUE);
            SetCtrlColor(hCoin_Pct, COINS_CLR_GREEN);

            // Separator after header
            CreateWindowA("STATIC", "",
                WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                m, 103, rW, 2, hWnd, NULL, hInst, NULL);

            // Body rows
            const int rowH = 23;
            int y = 111;
            int lastRowY = y;
            lblW  = 90;
            valW  = rW - lblW;
            for (int i = 0; i < COIN_ROW_COUNT; i++) {
                if (coinRows[i].isSeparator) {
                    CreateWindowA("STATIC", "",
                        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                        m, y + 5, rW, 2, hWnd, NULL, hInst, NULL);
                    y += 16;
                    continue;
                }

                hCoinLbl[i] = CreateWindowA("STATIC", coinRows[i].label,
                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                    m, y + 2, lblW, 18, hWnd, NULL, hInst, NULL);
                SendMessage(hCoinLbl[i], WM_SETFONT, (WPARAM)hFontCoins_Label, TRUE);
                // No SetCtrlColor – let the theme paint it

                hCoinVal[i] = CreateWindowA("STATIC", "--",
                    WS_CHILD | WS_VISIBLE | SS_RIGHT,
                    m + lblW, y + 2, valW, 18, hWnd,
                    (HMENU)(UINT_PTR)(ID_COINS_LABEL_BASE + i), hInst, NULL);
                SendMessage(hCoinVal[i], WM_SETFONT, (WPARAM)hFontCoins_Value, TRUE);
                // No SetCtrlColor – let the theme paint it (accent rows get colored on first update)

                lastRowY = y;
                y += rowH;
            }

            // Exchange-currency icon — right side of the last account-summary row.
            // Color is set once here and intentionally never touched again on click.
            hCoin_FxIcon = CreateWindowW(L"STATIC", FX_GLYPH,
                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
                m + lblW - 18, lastRowY + 3, 20, 20, hWnd, (HMENU)ID_COIN_FXICON, hInst, NULL);
            SendMessage(hCoin_FxIcon, WM_SETFONT, (WPARAM)hFontCoins_Icons, TRUE);
            SetCtrlColor(hCoin_FxIcon, COINS_CLR_GRAY);

            y += 12;
            int steps = 1;
            int stepz = 0;
            addButtons(hWnd, hInst, "Orders",         (7 * steps++) + (26 * stepz++) + m, y, (HMENU)ID_MB_ORDERS,    103);
            addButtons(hWnd, hInst, "Diamonds",       (7 * steps++) + (26 * stepz++) + m, y, (HMENU)ID_MB_DIAMONDS,  104);
            
            addButtons(hWnd, hInst, "Watchlist",  8 + (7 * steps++) + (26 * stepz++) + m, y, (HMENU)ID_MB_WATCHLIST, 105);
            addButtons(hWnd, hInst, "Market",     8 + (7 * steps++) + (26 * stepz++) + m, y, (HMENU)ID_MB_MARKET,    106);
            addButtons(hWnd, hInst, "Scanner",   8 + (7 * steps++) + (26 * stepz++) + m, y, (HMENU)ID_MB_SCANNER,    107);

            addButtons(hWnd, hInst, "Settings",  14 + (7 * steps++) + (26 * stepz++) + m, y, (HMENU)ID_MB_SETTINGS,  108);

            api().addApiUpdateWindow(hWnd);

            BindTrayIcon(hWnd);
                    
            SetTimer(hWnd, TIMER_WATCHDOG, 10000, NULL);
            SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);

            if (RegGetDword(DASHBOARD_CLASS_NAME, "Speaker", 0)) {
                Coins_ToggleTTS(hWnd);
            }
            break;
        }
        
        case WM_ACTIVATE: {
            RECT windowRect; 
            GetWindowRect(hWnd, &windowRect);
            if (LOWORD(wParam) != WA_INACTIVE) {
                MoveWindow(hWnd, windowRect.left, windowRect.top, windowDashboardWidth, windowDashboardHeight     , TRUE);
            } else {
                MoveWindow(hWnd, windowRect.left, windowRect.top, windowDashboardWidth, windowDashboardHeight - 40, TRUE);
            }
            break;
        }

        case WM_API_UPDATE:
            UpdateTrayIcon(hWnd);
            if (!api().isMarketDataConnected() || !api().isTradingConnected()) {
                if (hCoin_NetLiq) SetWindowTextA(hCoin_NetLiq, "--");
                if (hCoin_BigPnL) SetWindowTextA(hCoin_BigPnL, "--");
                if (hCoin_Pct)    SetWindowTextA(hCoin_Pct,    "--");
                for (int i = 0; i < COIN_ROW_COUNT; i++)
                    if (hCoinVal[i]) SetWindowTextA(hCoinVal[i], "--");
            }
            break;
        case WM_API_SOUND: {
            PlaySound_Async((int)lParam);
            break;
        }
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
            if  (id == ID_COIN_SPEAKER || id == ID_COIN_BIGPNL || id == ID_COIN_FXICON) {
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            break; 
        }
        case WM_TIMER:
            if (wParam == TIMER_COINS_SPEAKER) // 21000
                Coins_SpeakDailyPnL();
            if (wParam == TIMER_WATCHDOG) { // 10000
                if (shouldBeConnected && !api().isConnected()) {
                    EnsureGatewayRunning(hWnd);
                    api().connect(IsProcessRunning("ibgateway.exe") ? 4001 : 7496);
                    UpdateTrayIcon(hWnd);
                } else if (!shouldBeConnected && api().isConnected()) {
                    api().disconnect();
                    UpdateTrayIcon(hWnd);
                }
            }
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
                    if (FindWindowA(WATCHLIST_CLASS_NAME, NULL)) AppendMenuW(hMenu, MF_STRING, ID_M_WATCHLIST, IsWindowAlwaysOnTop(WATCHLIST_CLASS_NAME) ? L"[ ★ ] Watchlist" : L"[  ] Watchlist");


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
                    
                    if (FindWindowA(SCANNER_CLASS_NAME, NULL))   AppendMenuW(hMenu, MF_STRING, ID_M_SCANNER,   IsWindowAlwaysOnTop(SCANNER_CLASS_NAME)   ? L"[ ★ ] Scanner"   : L"[  ] Scanner");
                    if (FindWindowA(SETTINGS_CLASS_NAME, NULL))  AppendMenuW(hMenu, MF_STRING, ID_M_SETTINGS,  IsWindowAlwaysOnTop(SETTINGS_CLASS_NAME)  ? L"[ ★ ] Settings"  : L"[  ] Settings");
                    if (FindWindowA(DEBUGLOG_CLASS_NAME, NULL))  AppendMenuW(hMenu, MF_STRING, ID_M_DEBUGLOG,  IsWindowAlwaysOnTop(DEBUGLOG_CLASS_NAME)  ? L"[ ★ ] Debug Log" : L"[  ] Debug Log");

                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                    AppendMenuW(hMenu, MF_STRING, ID_M_EXIT, L"Exit");

                    // 3. TrackPopupMenu with TPM_RETURNCMD
                    int selectedCmd = TrackPopupMenu(hMenu, 
                        TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, 
                        pt.x, pt.y, 0, hWnd, NULL);

                    // 4. Route the command
                    switch (selectedCmd) {
                        case 0:
                            // User clicked completely outside the menu
                            bKeepMenuAlive = false;
                            break;

                        // GROUP A: The Toggle Items (Menu stays open)
                        case ID_M_DASHBOARD:
                        case ID_M_DIAMONDS:
                        case ID_M_ORDERS:
                        case ID_M_WATCHLIST:
                        case ID_M_SCANNER:
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

                    // 5. Destroy the menu at the end of every loop so it can be rebuilt clean
                    DestroyMenu(hMenu);
                }
            }
            break;
        }

        case WM_COMMAND: {
            WORD id  = LOWORD(wParam);
            WORD evt = HIWORD(wParam);
            switch (id) {
                case ID_COIN_SPEAKER:
                case ID_COIN_BIGPNL:
                    // Both the speaker glyph and the Daily PnL number toggle TTS when clicked
                    if (evt == STN_CLICKED)
                        Coins_ToggleTTS(hWnd);
                    break;
                case ID_COIN_FXICON:
                    // Opens the currency-exchange popup. Deliberately does NOT
                    // call SetCtrlColor here — this icon's color never changes.
                    if (evt == STN_CLICKED)
                        StartGenericWindow(DASHBOARD_EXCHANGE_CLASS_NAME, "Exchange", L"TWSAPIClientTradingFloor.ExchangeCurrency", 320, 70);
                    break;
                case ID_M_CONNECT:
                    shouldBeConnected = true; // Turn the auto-watchdog back on
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_DISCONNECT:
                    shouldBeConnected = false; // Stop the watchdog from auto-reconnecting
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_EXIT:
                    api().disconnect();
                    if (Settings_KillGatewayOnExit())
                        KillGateway();
                    Shell_NotifyIconW(NIM_DELETE, &nid); // Remove icon from tray
                    PostQuitMessage(0);
                    break;
                    
                case ID_MB_DIAMONDS:
                    StartDiamonds();
                    break;
                case ID_MB_SCANNER:
                    StartScanner();
                    break;
                case ID_MB_MARKET:
                    StartMarket();
                    break;
                case ID_MB_WATCHLIST:
                    StartWatchlist();
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
                case ID_M_SCANNER:
                    ToggleWindowAlwaysOnTop(SCANNER_CLASS_NAME);
                    break;
                case ID_M_WATCHLIST:
                    ToggleWindowAlwaysOnTop(WATCHLIST_CLASS_NAME);
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
                
                // Handle dynamically generated Market menu items
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

            // Free fonts
            if (hFontCoins_NetLiq)  { DeleteObject(hFontCoins_NetLiq);  hFontCoins_NetLiq  = NULL; }
            if (hFontCoins_BigPnL)  { DeleteObject(hFontCoins_BigPnL);  hFontCoins_BigPnL  = NULL; }
            if (hFontCoins_Pct)     { DeleteObject(hFontCoins_Pct);     hFontCoins_Pct     = NULL; }
            if (hFontCoins_Label)   { DeleteObject(hFontCoins_Label);   hFontCoins_Label   = NULL; }
            if (hFontCoins_Value)   { DeleteObject(hFontCoins_Value);   hFontCoins_Value   = NULL; }
            if (hFontCoins_Icons)   { DeleteObject(hFontCoins_Icons); hFontCoins_Icons = NULL; }

            hCoin_NetLiq = hCoin_BigPnL = hCoin_Pct = hCoin_Speaker = hCoin_FxIcon = NULL;
            memset(hCoinLbl, 0, sizeof(hCoinLbl));
            memset(hCoinVal, 0, sizeof(hCoinVal));
            gClrCount = 0;

            PostQuitMessage(0);
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}