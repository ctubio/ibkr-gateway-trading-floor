#pragma once

HWND hDBWnd = NULL;
static const char* DB_CLASS_NAME = "TNTDashboardClass";

#define WM_TRAYICON (WM_USER + 1)
NOTIFYICONDATA nid = { 0 };

HICON hIconConnected;
HICON hIconOffline;

void UpdateTrayIcon(HWND hWnd) {
    std::string tooltipText;
    std::string windowTitle = "IBKR Tunnel Dashboard: Offline";
    HICON hIcon;

    if (api.isConnected()) {
        std::string accNum = api.getAccountNumber();
        if (accNum.empty()) {
            tooltipText = "Connecting...";
            hIcon = hIconOffline;
        } else {
            tooltipText = "Account: " + accNum;
            windowTitle = tooltipText;
            hIcon = hIconConnected;
        }
    } else {
        tooltipText = "Offline";
        hIcon = hIconOffline;
    }

    strncpy(nid.szTip, tooltipText.c_str(), sizeof(nid.szTip) - 1);
    nid.szTip[sizeof(nid.szTip) - 1] = '\0';

    nid.uFlags = NIF_TIP | NIF_ICON;
    nid.hIcon  = hIcon;
    Shell_NotifyIcon(NIM_MODIFY, &nid);

    // Update window icon
    SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessage(hWnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
    SetWindowTextA(hWnd, windowTitle.c_str());
}

LRESULT CALLBACK WndProcDashboard(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:	{
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        HINSTANCE hInst = pcs->hInstance;
		
		// Create qb button
        SendMessage(
            CreateWindow(
                "BUTTON", "Click Me",
                WS_VISIBLE | WS_CHILD | BS_ICON,
                7, 7, 26, 26,
                hWnd, (HMENU)ID_M_SYMBOLS, hInst, NULL
            ),
            BM_SETIMAGE, (WPARAM)IMAGE_ICON,
            (LPARAM)LoadImage(hInst, MAKEINTRESOURCE(2), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)
        );

        api.setWindowHandle(hWnd);
        
        SetTimer(hWnd, TIMER_WATCHDOG, 10000, NULL);
        SendMessage(hWnd, WM_TIMER, TIMER_WATCHDOG, 0);
		break;
	}

    case WM_API_UPDATE:
        UpdateTrayIcon(hWnd);
        break;

    case WM_TIMER:
        if (wParam == TIMER_DROPDOWN) {
            KillTimer(hWnd, TIMER_DROPDOWN);
            ShowWindow(hAutoComplete, SW_HIDE);
        }
        if (wParam == TIMER_WATCHDOG) {
            if (shouldBeConnected && !api.isConnected()) {
                EnsureGatewayRunning();
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
                Shell_NotifyIcon(NIM_DELETE, &nid); // Remove icon from tray
                PostQuitMessage(0);
                break;
                
            case ID_M_SYMBOLS:
                startBook();
                break;
        }
        break;
		
    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE); // "Close" just hides it to tray
        return 0;

	case WM_MOVE:
		SaveWinPosition(hWnd, DB_CLASS_NAME);
        break;
		
	case WM_DESTROY:
		SaveWinPosition(hWnd, DB_CLASS_NAME);
		Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;
		
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void startDashboard(HINSTANCE hInst) {
    // 2. Create the Dashboard Window
	int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 400, h = 70;
	LoadWinPosition(DB_CLASS_NAME, x, y, w, h);
	
    hDBWnd = CreateWindow(DB_CLASS_NAME, "IBKR Tunnel Dashboard: Offline", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x, y, w, h,
        NULL, NULL, hInst, NULL);
	ShowWindow(hDBWnd, SW_SHOW);
	UpdateWindow(hDBWnd);

    SetWindowTaskbarId(hDBWnd, L"IBKRTunnel.Dashboard");
}

void registerDashboard(HINSTANCE hInst) {
    hIconConnected = (HICON)LoadImage(hInst, MAKEINTRESOURCE(1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    hIconOffline   = CreateGrayIcon(hIconConnected);

    // 1. Register Window Class
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProcDashboard;
    wc.hInstance = hInst;
    wc.lpszClassName = DB_CLASS_NAME;
    wc.hIcon = hIconOffline;
    RegisterClass(&wc);

	startDashboard(hInst);
}

void registerSystemIcon(HINSTANCE hInst) {
    // 3. Initialize Tray Icon
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hDBWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = hIconOffline;
    lstrcpy(nid.szTip, "IBKR Tunnel");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

HANDLE mutex_on() {
	// 1. Create a unique name for your Mutex (use a GUID or a unique string)
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\IBKRTunnelMutex_17072025");

    // 2. Check if the Mutex already exists
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // If it exists, another instance is running. 
        // We find the existing window and bring it to the front before exiting.
        HWND existingWnd = FindWindow(DB_CLASS_NAME, NULL);
        if (existingWnd) {
            ShowWindow(existingWnd, SW_SHOW);
            SetForegroundWindow(existingWnd);
        }
        
        if (hMutex) CloseHandle(hMutex);
        return 0; // Exit this instance
    }

    return hMutex;
}

void mutex_off(HANDLE hMutex) {
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
}
