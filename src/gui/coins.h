#pragma once
#include <sapi.h>
#include <sphelper.h>
#pragma comment(lib, "ole32.lib")

void StartCoins() { StartGenericWindow(COINS_CLASS_NAME, "Coins", L"IBKRGatewayClient.Coins", 250, 410); }

// ─── Colors ───────────────────────────────────────────────────────────────────
#define COINS_CLR_GREEN   RGB(  0, 138,  69)
#define COINS_CLR_RED     RGB(220,  55,  55)
#define COINS_CLR_WHITE   RGB(220, 220, 220)
#define COINS_CLR_BLACK   RGB(30,  30,  30)
#define COINS_CLR_GRAY    RGB(150, 150, 150)
#define COINS_CLR_PURPLE  RGB(185, 105, 225)
// Sentinel: no custom color – let HandleDarkModeMessages / system theme paint this control
#define COINS_CLR_THEME   ((COLORREF)0xFFFFFFFF)

// ─── Fonts ────────────────────────────────────────────────────────────────────
static HFONT hFontCoins_NetLiq  = NULL;
static HFONT hFontCoins_BigPnL  = NULL;
static HFONT hFontCoins_Pct     = NULL;
static HFONT hFontCoins_Label   = NULL;
static HFONT hFontCoins_Value   = NULL;
static HFONT hFontCoins_Speaker = NULL;   // Segoe MDL2 Assets for speaker glyph

static HFONT Coins_MakeFont(int ptSize, bool bold) {
    HDC hdc = GetDC(NULL);
    int h   = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    return CreateFontA(h, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
}
static HFONT Coins_MakeMDL2Font(int ptSize) {
    HDC hdc = GetDC(NULL);
    int h   = -MulDiv(ptSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    ReleaseDC(NULL, hdc);
    return CreateFontW(h, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe MDL2 Assets");
}

// ─── Header controls ──────────────────────────────────────────────────────────
static HWND hCoin_NetLiq  = NULL;
static HWND hCoin_BigPnL  = NULL;
static HWND hCoin_Pct     = NULL;
static HWND hCoin_Speaker = NULL;   // speaker icon button (SS_NOTIFY)

#define ID_COIN_NETLIQ   5100
#define ID_COIN_BIGPNL   5101
#define ID_COIN_PCT      5102
#define ID_COIN_SPEAKER  5103
#define COINS_TTS_TIMER  0xC015   // WM_TIMER id

// ─── TTS state ────────────────────────────────────────────────────────────────
static ISpVoice* g_pCoinsVoice  = nullptr;
static bool      g_coinsTtsOn   = false;
static bool      g_coinsComInit = false;

// Speaker glyph: E767 = volume on (Segoe MDL2 Assets)
static const wchar_t SPEAKER_GLYPH[] = L"\uE767";

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

// ─── Per-control color table ──────────────────────────────────────────────────
static HWND     gClrHwnd[160]  = {};
static COLORREF gClrColor[160] = {};
static int      gClrCount      = 0;

static void SetCtrlColor(HWND hw, COLORREF c) {
    for (int i = 0; i < gClrCount; i++)
        if (gClrHwnd[i] == hw) { gClrColor[i] = c; return; }
    if (gClrCount < 160) { gClrHwnd[gClrCount] = hw; gClrColor[gClrCount++] = c; }
}
static COLORREF GetCtrlColor(HWND hw) {
    for (int i = 0; i < gClrCount; i++)
        if (gClrHwnd[i] == hw) return gClrColor[i];
    return COINS_CLR_THEME;  // not registered = let theme handle it
}

// ─── TTS helpers ──────────────────────────────────────────────────────────────

// Case-insensitive wstring search helper
static bool Coins_ContainsCI(const WCHAR* s, const WCHAR* needle) {
    if (!s || !needle) return false;
    std::wstring ws(s), wn(needle);
    auto toLo = [](wchar_t c) { return (wchar_t)towlower(c); };
    std::transform(ws.begin(), ws.end(), ws.begin(), toLo);
    std::transform(wn.begin(), wn.end(), wn.begin(), toLo);
    return ws.find(wn) != std::wstring::npos;
}

// Search one voice category for Herena/Helena/Catalan; return token (caller releases) or nullptr.
static ISpObjectToken* Coins_FindVoiceInCategory(const WCHAR* categoryId) {
    IEnumSpObjectTokens* pEnum = nullptr;
    if (FAILED(SpEnumTokens(categoryId, NULL, NULL, &pEnum))) return nullptr;

    ISpObjectToken* pToken = nullptr;
    ISpObjectToken* pFound = nullptr;

    while (!pFound && SUCCEEDED(pEnum->Next(1, &pToken, NULL)) && pToken) {
        // 1. Token ID (registry path) – typically contains "HERENA" and "CA-ES"
        WCHAR* pId = nullptr;
        pToken->GetId(&pId);

        // 2. Display description
        WCHAR* pDesc = nullptr;
        SpGetDescription(pToken, &pDesc);

        // 3. Attributes\Name subkey – the most reliable friendly name
        WCHAR* pAttrName = nullptr;
        ISpDataKey* pAttribs = nullptr;
        if (SUCCEEDED(pToken->OpenKey(L"Attributes", &pAttribs))) {
            pAttribs->GetStringValue(L"Name", &pAttrName);
            pAttribs->Release();
        }

        bool match = Coins_ContainsCI(pId,        L"herena")  ||
                     Coins_ContainsCI(pDesc,      L"herena")  ||
                     Coins_ContainsCI(pAttrName,  L"herena")  ||
                     Coins_ContainsCI(pId,        L"helena")  ||
                     Coins_ContainsCI(pDesc,      L"helena")  ||
                     Coins_ContainsCI(pAttrName,  L"helena")  ||
                     Coins_ContainsCI(pId,        L"ca-es")   ||
                     Coins_ContainsCI(pDesc,      L"ca-es")   ||
                     Coins_ContainsCI(pAttrName,  L"ca-es")   ||
                     Coins_ContainsCI(pId,        L"catalan") ||
                     Coins_ContainsCI(pDesc,      L"catalan") ||
                     Coins_ContainsCI(pAttrName,  L"catalan");

        if (pId)       CoTaskMemFree(pId);
        if (pDesc)     CoTaskMemFree(pDesc);
        if (pAttrName) CoTaskMemFree(pAttrName);

        if (match) pFound = pToken;
        else       { pToken->Release(); pToken = nullptr; }
    }
    pEnum->Release();
    return pFound;
}

// Select Herena-Catalan voice; searches both classic SAPI and OneCore (Win10+)
// registries, since modern voices installed for browsers live in OneCore.
// Falls back to system default if not found.
static void Coins_SelectVoice() {
    if (!g_pCoinsVoice) return;

    // 1. Classic SAPI voices (HKLM\SOFTWARE\Microsoft\Speech\Voices)
    ISpObjectToken* pFound = Coins_FindVoiceInCategory(SPCAT_VOICES);

    // 2. OneCore voices (HKLM\SOFTWARE\Microsoft\Speech_OneCore\Voices)
    //    This is where modern/neural voices installed via Windows Settings live.
    if (!pFound)
        pFound = Coins_FindVoiceInCategory(
            L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices");

    if (pFound) {
        g_pCoinsVoice->SetVoice(pFound);
        pFound->Release();
    }
    // else: keep system default voice
}

static bool Coins_InitVoice() {
    if (g_pCoinsVoice) return true;

    if (!g_coinsComInit) {
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        g_coinsComInit = SUCCEEDED(hr) || (hr == RPC_E_CHANGED_MODE);
    }

    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&g_pCoinsVoice);
    if (FAILED(hr)) { g_pCoinsVoice = nullptr; return false; }

    Coins_SelectVoice();
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

    if (g_coinsTtsOn) {
        if (!Coins_InitVoice()) {
            g_coinsTtsOn = false;
            return;
        }
        if (Settings_DarkMode()) {
            SetCtrlColor(hCoin_Speaker, COINS_CLR_WHITE);   // bright = active
        } else {
            SetCtrlColor(hCoin_Speaker, COINS_CLR_BLACK);   // bright = active
        }
        SetTimer(hWnd, COINS_TTS_TIMER, 21000, NULL);
        Coins_SpeakDailyPnL();                          // speak immediately
    } else {
        KillTimer(hWnd, COINS_TTS_TIMER);
        if (g_pCoinsVoice)
            g_pCoinsVoice->Speak(NULL, SVSFPurgeBeforeSpeak, NULL); // stop current
        SetCtrlColor(hCoin_Speaker, COINS_CLR_GRAY);    // dim = inactive
    }

    if (hCoin_Speaker) InvalidateRect(hCoin_Speaker, NULL, TRUE);
}

// ─── Label update ─────────────────────────────────────────────────────────────
void Coins_UpdateLabels(HWND hWnd) {
    auto   summary    = api.getAccountSummary();
    double daily      = api.getDailyPnL();
    double unrealized = api.getUnrealizedPnL();
    double realized   = api.getRealizedPnL();

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

    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "Coins: %s", currency.c_str());
    SetWindowTextA(hWnd, titleBuf);

    if (hCoin_NetLiq) {
        char buf[80];
        std::string formattedNum = FormatWithCommas(netLiq);
        snprintf(buf, sizeof(buf), "%s", formattedNum.c_str());
        SetWindowTextA(hCoin_NetLiq, buf);
    }

    COLORREF pnlClr = daily >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
    if (hCoin_BigPnL) {
        std::string formattedNum = FormatWithCommas(daily);
        if (daily >= 0.0) formattedNum = "+" + formattedNum;
        char buf[64]; snprintf(buf, sizeof(buf), "%s", formattedNum.c_str());
        SetWindowTextA(hCoin_BigPnL, buf);
        SetCtrlColor(hCoin_BigPnL, pnlClr);
        InvalidateRect(hCoin_BigPnL, NULL, TRUE);
    }
    if (hCoin_Pct) {
        double pct = (netLiq != 0.0) ? (daily / netLiq * 100.0) : 0.0;
        std::string formattedNum = FormatWithCommas(pct);
        if (pct >= 0.0) formattedNum = "+" + formattedNum;
        char buf[32]; snprintf(buf, sizeof(buf), "%s%%", formattedNum.c_str());
        SetWindowTextA(hCoin_Pct, buf);
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
        COLORREF clr = COINS_CLR_THEME;  // default: let theme paint it
        try {
            double d = std::stod(raw);
            if (coinRows[i].colorType == 1) {
                std::string formattedNum = FormatWithCommas(d);
                snprintf(buf, sizeof(buf), "%s", formattedNum.c_str());
                clr = d >= 0.0 ? COINS_CLR_GREEN : COINS_CLR_RED;
            } else if (coinRows[i].colorType == 2) {
                std::string formattedNum = FormatWithCommas(d);
                snprintf(buf, sizeof(buf), "%s", formattedNum.c_str());
                clr = COINS_CLR_PURPLE;
            } else if (coinRows[i].colorType == 3) {
                const char* suffix = "";
                std::string dynamicSuffix;
                if (strcmp(coinRows[i].key, "CashBalance") == 0) {
                    dynamicSuffix = " " + currency;
                    suffix = dynamicSuffix.c_str();
                } else if (strncmp(coinRows[i].key, "EUR_", 4) == 0) {
                    suffix = " EUR";
                } else if (strncmp(coinRows[i].key, "USD_", 4) == 0) {
                    suffix = " USD";
                }
                std::string formattedNum = FormatWithCommas(d);
                snprintf(buf, sizeof(buf), "%s%s", formattedNum.c_str(), suffix);
                clr = d > 0.0 ? COINS_CLR_GREEN : (d < 0.0 ? COINS_CLR_RED : COINS_CLR_THEME);
            } else {
                std::string formattedNum = FormatWithCommas(d);
                snprintf(buf, sizeof(buf), "%s", formattedNum.c_str());
            }
        } catch (...) {}

        SetCtrlColor(hCoinVal[i], clr);
        SetWindowTextA(hCoinVal[i], buf);
        InvalidateRect(hCoinVal[i], NULL, TRUE);
    }
}

// ─── Window proc ──────────────────────────────────────────────────────────────
LRESULT CALLBACK WndProcCoins(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        hFontCoins_NetLiq  = Coins_MakeFont(13, true);
        hFontCoins_BigPnL  = Coins_MakeFont(20, true);
        hFontCoins_Pct     = Coins_MakeFont(11, true);
        hFontCoins_Label   = Coins_MakeFont(10, false);
        hFontCoins_Value   = Coins_MakeFont(11, true);
        hFontCoins_Speaker = Coins_MakeMDL2Font(11);

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
        SendMessage(hCoin_Speaker, WM_SETFONT, (WPARAM)hFontCoins_Speaker, TRUE);
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
        lblW  = 110;
        valW  = rW - lblW;
        for (int i = 0; i < COIN_ROW_COUNT; i++) {
            if (coinRows[i].isSeparator) {
                CreateWindowA("STATIC", "",
                    WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                    m, y + 4, rW, 2, hWnd, NULL, hInst, NULL);
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

            y += rowH;
        }

        api.addApiUpdateWindow(hWnd);
        api.setCoinWindow(hWnd);
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC  hdc = (HDC)wParam;
        HWND hw  = (HWND)lParam;
        COLORREF clr = GetCtrlColor(hw);
        if (clr == COINS_CLR_THEME) {
            // Delegate entirely to the shared dark/light mode handler
            return HandleDarkModeMessages(hWnd, message, wParam, lParam);
        }
        SetTextColor(hdc, clr);
        if (Settings_DarkMode()) {
            SetBkColor(hdc, DM_BG);
            return (LRESULT)hDarkBrush;
        } else {
            SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
    }

    case WM_COMMAND: {
        WORD id  = LOWORD(wParam);
        WORD evt = HIWORD(wParam);
        // Both the speaker glyph and the Daily PnL number toggle TTS when clicked
        if (evt == STN_CLICKED && (id == ID_COIN_SPEAKER || id == ID_COIN_BIGPNL))
            Coins_ToggleTTS(hWnd);
        break;
    }

    case WM_TIMER:
        if (wParam == COINS_TTS_TIMER)
            Coins_SpeakDailyPnL();
        break;

    case WM_API_UPDATE:
        if (api.isMarketDataConnected() && api.isTradingConnected()) {
            api.setCoinWindow(hWnd);
        } else {
            if (hCoin_NetLiq) SetWindowTextA(hCoin_NetLiq, "--");
            if (hCoin_BigPnL) SetWindowTextA(hCoin_BigPnL, "--");
            if (hCoin_Pct)    SetWindowTextA(hCoin_Pct,    "--");
            for (int i = 0; i < COIN_ROW_COUNT; i++)
                if (hCoinVal[i]) SetWindowTextA(hCoinVal[i], "--");
        }
        break;

    case WM_ACCOUNT_SUMMARY:
    case WM_PNL_UPDATE:
        Coins_UpdateLabels(hWnd);
        break;

    case WM_DESTROY:
        // Stop TTS
        if (g_coinsTtsOn) KillTimer(hWnd, COINS_TTS_TIMER);
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
        if (hFontCoins_Speaker) { DeleteObject(hFontCoins_Speaker); hFontCoins_Speaker = NULL; }

        hCoin_NetLiq = hCoin_BigPnL = hCoin_Pct = hCoin_Speaker = NULL;
        memset(hCoinLbl, 0, sizeof(hCoinLbl));
        memset(hCoinVal, 0, sizeof(hCoinVal));
        gClrCount = 0;

        api.unsetCoinWindow();
        api.removeApiUpdateWindow(hWnd);
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}