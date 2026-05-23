#pragma once

void startDashboard(HINSTANCE hInst) { startGenericWindow(DASHBOARD_CLASS_NAME, "IBKR Gateway: Offline", L"IBKRGatewayClient.Dashboard", 324, 70, hInst); }

#define WM_TRAYICON (WM_USER + 1)

#define TIMER_WATCHDOG 1

#define ID_M_SYMBOLS    1001
#define ID_M_COINS      1002
#define ID_M_DIAMONDS   1003
#define ID_M_SETTINGS   1004
#define ID_M_NEWS       1005
#define ID_M_TICKER     1006
#define ID_M_TIMESALES  1007
#define ID_M_LEVELS     1008
#define ID_M_ORDERS     1009

#define ID_M_CONNECT    3001
#define ID_M_DISCONNECT 3002
#define ID_M_EXIT       3003

bool shouldBeConnected = true;

void UpdateTrayIcon(HWND hWnd) {
    std::string tooltip;
    std::string title = "IBKR Gateway: ";
    bool connected = false;

    if (!api.isConnected()) {
        tooltip = "Offline";
        title  += tooltip;
    } else {
        std::string acc = api.getAccountNumber();
        if (acc.empty()) {
            tooltip = "Connecting...";
            title  += tooltip;
        } else {
            connected = api.isMarketDataConnected() && api.isTradingConnected();
            std::string md = api.isMarketDataConnected() ? "MD✓" : "MD✗";
            std::string tr = api.isTradingConnected()    ? "TR✓" : "TR✗";
            tooltip = acc + " | " + md + " " + tr;
            title   = "Account: " + acc;
        }
    }

    strncpy(nid.szTip, tooltip.c_str(), sizeof(nid.szTip) - 1);
    nid.szTip[sizeof(nid.szTip) - 1] = '\0';
    nid.uFlags = NIF_TIP | NIF_ICON;
    nid.hIcon  = connected ? onlineIcons[DASHBOARD_CLASS_NAME] : offlineIcons[DASHBOARD_CLASS_NAME];
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    
    for (const auto& pair : g_AppWindows) {
        const std::string& key = pair.first;
        const HWND& hwnd = pair.second;
        
        // Validate window is still alive
        if (!hwnd || !IsWindow(hwnd)) continue;
        
        // Check icons exist before using them
        if (onlineIcons.find(key) == onlineIcons.end() || 
            offlineIcons.find(key) == offlineIcons.end()) continue;
        
        HICON hIcon = connected ? onlineIcons[key] : offlineIcons[key];
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
    }
            
    SetWindowTextA(hWnd, title.c_str());
}

void addButtons(HWND hWnd, HINSTANCE hInst, LPCSTR buttonText, int x, int y, HMENU menuId, int iconId) {
		// Create the button
        HWND hBtn = CreateWindow(
            "BUTTON", buttonText,
            WS_VISIBLE | WS_CHILD | BS_ICON,
            x, y, 26, 26,
            hWnd, menuId, hInst, NULL
        );
        SendMessage(hBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON,
            (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(iconId), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));

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

LRESULT CALLBACK WndProcDashboard(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:	{
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
            HINSTANCE hInst = pcs->hInstance;

            int steps = 1;
            int stepz = 0;
            //charts from tradingview
            addButtons(hWnd, hInst, "Coins",          (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_COINS,     3); // total daily eur usd
            addButtons(hWnd, hInst, "Orders",         (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_ORDERS,   10); // open, messages, history
            addButtons(hWnd, hInst, "Diamonds",       (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_DIAMONDS,  4); // portfolio + watchlist
            
            addButtons(hWnd, hInst, "Ticker",     6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_TICKER,    9);        
            addButtons(hWnd, hInst, "Levels",     6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_LEVELS,    8);
            addButtons(hWnd, hInst, "Timesales",  6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_TIMESALES, 7); // 3: 0, 100, 1000
            addButtons(hWnd, hInst, "News",       6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_NEWS,      6);

            addButtons(hWnd, hInst, "Symbols",   12 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_SYMBOLS,   2);
            addButtons(hWnd, hInst, "Settings",  12 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_M_SETTINGS,  5);

            api.addApiUpdateWindow(hWnd);
            api.setApiErrorWindow(hWnd);
                    
            SetTimer(hWnd, TIMER_WATCHDOG, 10000, NULL);
            SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
            break;
        }
        
        case WM_API_UPDATE:
            UpdateTrayIcon(hWnd);
            break;

        case WM_API_ERROR: {
            std::string* msg = (std::string*)lParam;
            LogDebug(msg->c_str());
            delete msg;
            break;
        }

        case WM_TIMER:
            if (wParam == TIMER_DROPDOWN) {
                KillTimer(hWnd, TIMER_DROPDOWN);
                ShowWindow(hAutoComplete, SW_HIDE);
            }
            if (wParam == TIMER_WATCHDOG) {
                if (shouldBeConnected && !api.isConnected()) {
                    EnsureGatewayRunning(hWnd);
                    api.connect();
                    UpdateTrayIcon(hWnd);
                } else if (!shouldBeConnected && api.isConnected()) {
                    api.disconnect();
                    UpdateTrayIcon(hWnd);
                }
            }
            break;

        case WM_TRAYICON:
            if (lParam == WM_LBUTTONUP) {
                ShowWindow(hWnd, IsWindowVisible(hWnd) ? SW_HIDE : SW_SHOW);
            }
            else if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                // Determine flags based on current API state

                if (api.isConnected()) {
                    std::string accText = std::string("Account: ") + api.getAccountNumber();
                    AppendMenu(hMenu, (MF_STRING | MF_GRAYED), 0, accText.c_str());
                    AppendMenu(hMenu, MF_STRING, ID_M_DISCONNECT, "Disconnect");
                } else {
                    AppendMenu(hMenu, MF_STRING, ID_M_CONNECT, "Connect");
                }
                
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, ID_M_COINS, "Coins");
                AppendMenu(hMenu, MF_STRING, ID_M_DIAMONDS, "Diamonds");
                AppendMenu(hMenu, MF_STRING, ID_M_ORDERS, "Orders");
                AppendMenu(hMenu, MF_STRING, ID_M_TICKER, "Ticker");
                AppendMenu(hMenu, MF_STRING, ID_M_LEVELS, "Levels");
                AppendMenu(hMenu, MF_STRING, ID_M_TIMESALES, "Timesales");
                AppendMenu(hMenu, MF_STRING, ID_M_NEWS, "News");
                AppendMenu(hMenu, MF_STRING, ID_M_SYMBOLS, "Symbols");
                AppendMenu(hMenu, MF_STRING, ID_M_SETTINGS, "Settings");
                AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenu(hMenu, MF_STRING, ID_M_EXIT, "Exit");

                // Track where the mouse is to pop up the menu right above the tray
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd); // Fixes a notorious Win32 menu-focus bug
                
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_M_CONNECT:
                    shouldBeConnected = true; // Turn the auto-watchdog back on
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_DISCONNECT:
                    shouldBeConnected = false; // Stop the watchdog from auto-reconnecting
                    SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
                    break;

                case ID_M_EXIT:
                    api.disconnect();
                    if (Settings_KillGatewayOnExit())
                        KillGateway();
                    Shell_NotifyIcon(NIM_DELETE, &nid); // Remove icon from tray
                    PostQuitMessage(0);
                    break;
                    
                case ID_M_SYMBOLS:
                    startBook();
                    break;
                case ID_M_COINS:
                    startCoins();
                    break;
                case ID_M_DIAMONDS:
                    startDiamonds();
                    break;
                case ID_M_NEWS:
                    startNews();
                    break;
                case ID_M_TIMESALES:
                    startTimesales();
                    break;
                case ID_M_LEVELS:
                    startLevels();
                    break;
                case ID_M_TICKER:
                    startTicker();
                    break;
                case ID_M_SETTINGS:
                    startSettings();
                    break;
                case ID_M_ORDERS:
                    startOrders();
                    break;
            }
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

void MutexInstance() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\IBKRGatewayClientMutex_17072025");

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindow(DASHBOARD_CLASS_NAME, NULL);
        if (existingWnd) {
            DWORD processId;
            GetWindowThreadProcessId(existingWnd, &processId);
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
            if (hProcess) {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
        
        if (hMutex) CloseHandle(hMutex);
        // Re-create to own the mutex for this instance
        CreateMutex(NULL, TRUE, "Global\\IBKRGatewayClientMutex_17072025");
    }
}
