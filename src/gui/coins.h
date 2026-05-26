#pragma once

void StartCoins() { StartGenericWindow(COINS_CLASS_NAME, "Coins", L"IBKRGatewayClient.Coins", 350, 450); }

// Label IDs
#define ID_COINS_LABEL_BASE 5000

static HWND hCoinsLabels[20] = {};
static int  hCoinsLabelCount = 0;

struct CoinRow { const char* label; const char* key; bool isPnL; };

static CoinRow coinRows[] = {
    { "Net Liquidation:",    "NetLiquidation",      false },
    { "Total Cash:",         "TotalCashValue",      false },
    { "Gross Position:",     "GrossPositionValue",  false },
    { "Buying Power:",       "BuyingPower",         false },
    { "Available Funds:",    "AvailableFunds",      false },
    { "Excess Liquidity:",   "ExcessLiquidity",     false },
    { "Init Margin:",        "InitMarginReq",       false },
    { "Maint Margin:",       "MaintMarginReq",      false },
    { "Base Cash:",          "CashBalance",         false },
    { "EUR Cash:",           "EUR_CashBalance",     false },
    { "USD Cash:",           "USD_CashBalance",     false },
    { "Daily PnL:",          "DailyPnL",            true  },
    { "Unrealized PnL:",     "UnrealizedPnL",       true  },
    { "Realized PnL:",       "RealizedPnL",         true  },
    { "Accrued Cash:",       "AccruedCash",         false },
    { "Dividends:",          "AccruedDividend",     true  }
};
static const int COIN_ROW_COUNT = sizeof(coinRows) / sizeof(coinRows[0]);

// Value label handles
static HWND hCoinValues[COIN_ROW_COUNT] = {};

void Coins_UpdateLabels(HWND hWnd) {
    auto summary = api.getAccountSummary();
    double daily      = api.getDailyPnL();
    double unrealized = api.getUnrealizedPnL();
    double realized   = api.getRealizedPnL();

    // Update Window Title with currency of NetLiquidation
    std::string currency = "???";
    if (summary.count("EUR_NetLiquidation")) currency = "EUR";
    else if (summary.count("USD_NetLiquidation")) currency = "USD";
    else if (summary.count("NetLiquidation")) {
        // If we only have the base one, we can't be 100% sure but usually it's the base currency
        // We'll just leave it as is or try to find any currency prefix in the map
        for (auto const& [key, val] : summary) {
            if (key.find("_NetLiquidation") != std::string::npos) {
                currency = key.substr(0, key.find("_"));
                break;
            }
        }
    }
    
    char titleBuf[64];
    snprintf(titleBuf, sizeof(titleBuf), "Coins: %s", currency.c_str());
    SetWindowTextA(hWnd, titleBuf);

    for (int i = 0; i < COIN_ROW_COUNT; i++) {
        std::string val;
        if (strcmp(coinRows[i].key, "DailyPnL") == 0)
            val = std::to_string(daily);
        else if (strcmp(coinRows[i].key, "UnrealizedPnL") == 0)
            val = std::to_string(unrealized);
        else if (strcmp(coinRows[i].key, "RealizedPnL") == 0)
            val = std::to_string(realized);
        else {
            auto it = summary.find(coinRows[i].key);
            val = (it != summary.end()) ? it->second : "--";
        }

        // Format to 2 decimal places if numeric
        try {
            double d = std::stod(val);
            char buf[64];
            snprintf(buf, sizeof(buf), "%.2f", d);
            val = buf;
        } catch (...) {}

        if (hCoinValues[i]) {
            std::string displayVal = val;
            // Append currency if it's a cash balance
            if (strstr(coinRows[i].key, "CashBalance")) {
                std::string curr = "";
                if (strcmp(coinRows[i].key, "CashBalance") == 0) curr = " (BASE)";
                else if (strncmp(coinRows[i].key, "EUR_", 4) == 0) curr = " EUR";
                else if (strncmp(coinRows[i].key, "USD_", 4) == 0) curr = " USD";
                displayVal += curr;
            }
            SetWindowTextA(hCoinValues[i], displayVal.c_str());
        }
    }
}

LRESULT CALLBACK WndProcCoins(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        int margin = 10;
        int rowH   = 22;
        int labelW = 130;
        int valueW = 140;

        for (int i = 0; i < COIN_ROW_COUNT; i++) {
            int y = margin + i * rowH;

            // Label
            CreateWindowA("STATIC", coinRows[i].label,
                WS_CHILD | WS_VISIBLE | SS_RIGHT,
                margin, y + 2, labelW, 18,
                hWnd, NULL, hInst, NULL);

            // Value
            hCoinValues[i] = CreateWindowA("EDIT", "--",
                WS_CHILD | WS_VISIBLE | SS_LEFT | ES_READONLY,
                margin + labelW + 8, y + 2, valueW, 18,
                hWnd, (HMENU)(UINT_PTR)(ID_COINS_LABEL_BASE + i), hInst, NULL);
        }

        api.addApiUpdateWindow(hWnd);
        api.setCoinWindow(hWnd);
        break;
    }

    case WM_API_UPDATE:
        if (api.isMarketDataConnected() && api.isTradingConnected()) {
            api.setCoinWindow(hWnd);
        } else {
            for (int i = 0; i < COIN_ROW_COUNT; i++) {
                if (hCoinValues[i]) {
                    SetWindowTextA(hCoinValues[i], "--");
                }
            }
        }
        break;

    case WM_ACCOUNT_SUMMARY:
    case WM_PNL_UPDATE:
        Coins_UpdateLabels(hWnd);
        break;

    case WM_DESTROY:
        api.unsetCoinWindow();
        api.removeApiUpdateWindow(hWnd);
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
