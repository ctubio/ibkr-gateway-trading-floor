#pragma once

void StartDashboard(HINSTANCE hInst) { StartGenericWindow(DASHBOARD_CLASS_NAME, "IBKR Gateway: Offline", L"IBKRGatewayClient.Dashboard", 260, 70, hInst); }

#define WM_TRAYICON (WM_APP + 100)

#define TIMER_WATCHDOG 1

#define ID_M_DASHBOARD   1001
#define ID_MB_COINS      1002
#define ID_MB_DIAMONDS   1003
#define ID_MB_SETTINGS   1004
#define ID_MB_NEWS       1005
#define ID_MB_WATCHLIST  1006
#define ID_MB_MARKET     1007
#define ID_MB_ORDERS     1008
#define ID_M_ORDERS      1009
#define ID_M_COINS       1010
#define ID_M_DIAMONDS    1011
#define ID_M_SETTINGS    1012
#define ID_M_NEWS        1013
#define ID_M_WATCHLIST   1014
#define ID_M_MARKET      1015
#define ID_M_DEBUGLOG    1016

#define ID_M_CONNECT    1100
#define ID_M_DISCONNECT 1101
#define ID_M_EXIT       1102

#define ID_M_MARKET_BASE 1500
#define ID_M_MARKET_MAX   100

bool shouldBeConnected = true;

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
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    
    nid.hIcon = offlineIcons[std::string(classNameA)]; 

    wcscpy_s(nid.szTip, sizeof(nid.szTip) / sizeof(wchar_t), L"Offline");

    Shell_NotifyIconW(NIM_ADD, &nid);
    
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

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
    std::wstring wTooltip(tooltip.begin(), tooltip.end());
    wcsncpy(nid.szTip, wTooltip.c_str(), _countof(nid.szTip) - 1);
    nid.szTip[_countof(nid.szTip) - 1] = L'\0'; // Note the L prefix for wide null terminator

    nid.uFlags = NIF_TIP | NIF_ICON;
    nid.hIcon  = connected ? onlineIcons[std::string(DASHBOARD_CLASS_NAME)] : offlineIcons[std::string(DASHBOARD_CLASS_NAME)];

    Shell_NotifyIconW(NIM_MODIFY, &nid);

    IconUpdateContext ctx = { connected, onlineIcons, offlineIcons };
    EnumWindows(IconsEnumWindowsProc, (LPARAM)&ctx);
            
    SetWindowTextA(hWnd, title.c_str());
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

LRESULT CALLBACK WndProcDashboard(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:	{
            LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
            HINSTANCE hInst = pcs->hInstance;

            int steps = 1;
            int stepz = 0;
            addButtons(hWnd, hInst, "Coins",          (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_COINS,     101);
            addButtons(hWnd, hInst, "Orders",         (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_ORDERS,    103);
            addButtons(hWnd, hInst, "Diamonds",       (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_DIAMONDS,  104);
            
            addButtons(hWnd, hInst, "Watchlist",  6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_WATCHLIST, 105);
            addButtons(hWnd, hInst, "Market",     6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_MARKET,    106);
            addButtons(hWnd, hInst, "News",       6 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_NEWS,      107);

            addButtons(hWnd, hInst, "Settings",  12 + (7 * steps++) + (26 * stepz++) + 1, 7, (HMENU)ID_MB_SETTINGS,  108);

            api.addApiUpdateWindow(hWnd);
            api.setApiErrorWindow(hWnd);

            BindTrayIcon(hWnd);
                    
            SetTimer(hWnd, TIMER_WATCHDOG, 10000, NULL);
            SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
            break;
        }
        
        case WM_API_UPDATE:
            UpdateTrayIcon(hWnd);
            break;

        case WM_API_LOG: {
            std::string* msg = (std::string*)lParam;
            LogDebug(msg->c_str());
            delete msg;
            break;
        }

        case WM_TIMER:
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
                    if (api.isConnected()) {
                        std::wstring accText = std::wstring(L"Account: ") + StringToWide(api.getAccountNumber()); 
                        AppendMenuW(hMenu, (MF_STRING | MF_GRAYED), 0, accText.c_str());
                        AppendMenuW(hMenu, MF_STRING, ID_M_DISCONNECT, L"Disconnect");
                    } else {
                        AppendMenuW(hMenu, MF_STRING, ID_M_CONNECT, L"Connect");
                    }
                    
                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

                    // Re-evaluate window states on every loop iteration
                    if (FindWindowA(DASHBOARD_CLASS_NAME, NULL)) AppendMenuW(hMenu, MF_STRING, ID_M_DASHBOARD, IsWindowAlwaysOnTop(DASHBOARD_CLASS_NAME) ? L"[ ★ ] Dashboard" : L"[  ] Dashboard");
                    if (FindWindowA(COINS_CLASS_NAME, NULL))     AppendMenuW(hMenu, MF_STRING, ID_M_COINS,     IsWindowAlwaysOnTop(COINS_CLASS_NAME)     ? L"[ ★ ] Coins"     : L"[  ] Coins");
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
                    
                    if (FindWindowA(NEWS_CLASS_NAME, NULL))      AppendMenuW(hMenu, MF_STRING, ID_M_NEWS,      IsWindowAlwaysOnTop(NEWS_CLASS_NAME)      ? L"[ ★ ] News"      : L"[  ] News");
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
                        case ID_M_COINS:
                        case ID_M_DIAMONDS:
                        case ID_M_ORDERS:
                        case ID_M_WATCHLIST:
                        case ID_M_NEWS:
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
                    Shell_NotifyIconW(NIM_DELETE, &nid); // Remove icon from tray
                    PostQuitMessage(0);
                    break;
                    
                case ID_MB_COINS:
                    StartCoins();
                    break;
                case ID_MB_DIAMONDS:
                    StartDiamonds();
                    break;
                case ID_MB_NEWS:
                    StartNews();
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
                case ID_M_COINS:
                    ToggleWindowAlwaysOnTop(COINS_CLASS_NAME);
                    break;
                case ID_M_DIAMONDS:
                    ToggleWindowAlwaysOnTop(DIAMONDS_CLASS_NAME);
                    break;
                case ID_M_NEWS:
                    ToggleWindowAlwaysOnTop(NEWS_CLASS_NAME);
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
                            ToggleMarketAlwaysOnTop(tsWindows[index].symbol);
                        }
                    }
                    break;
            }
            break;
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

void MutexGatewayInstance() {
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