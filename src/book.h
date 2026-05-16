#pragma once

HWND hBookWnd = NULL;
static const char* BOOK_CLASS_NAME = "TNTBookWindowClass";

static HWND hAutoComplete = NULL;

// Control IDs
#define ID_BOOK_COMBO        2001
#define ID_BOOK_NEW_LIST     2002
#define ID_BOOK_DEL_LIST     2003
#define ID_BOOK_LISTBOX      2004
#define ID_BOOK_EDIT         2005
#define ID_BOOK_ADD_ITEM     2006
#define ID_BOOK_DEL_ITEM     2007
#define ID_BOOK_AUTOCOMPLETE 2008
#define TIMER_DROPDOWN       99

static HWND hCombo, hListBox, hEdit;
static HWND hAutoCompleteOwner = NULL;

// ─── AutoComplete ─────────────────────────────────────────────────────────────


LRESULT CALLBACK AutoCompleteSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_LBUTTONUP) {
        int sel = SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR) {
            char sym[64] = {};
            SendMessageA(hWnd, LB_GETTEXT, sel, (LPARAM)sym);
            SetWindowTextA(hEdit, sym);
            ShowWindow(hWnd, SW_HIDE);
            // Trigger add via double-click behaviour
            SendMessage(hAutoCompleteOwner, WM_COMMAND,
                MAKEWPARAM(ID_BOOK_AUTOCOMPLETE, LBN_DBLCLK), (LPARAM)hWnd);
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, AutoCompleteSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

void Book_UpdateAutoComplete(HWND hWnd, const std::vector<std::string>& results) {
    if (results.empty()) {
        ShowWindow(hAutoComplete, SW_HIDE);
        return;
    }

    SendMessage(hAutoComplete, LB_RESETCONTENT, 0, 0);
    for (const auto& s : results)
        SendMessageA(hAutoComplete, LB_ADDSTRING, 0, (LPARAM)s.c_str());

    RECT r;
    GetWindowRect(hEdit, &r);  // already screen coords

    int itemHeight = SendMessage(hAutoComplete, LB_GETITEMHEIGHT, 0, 0);
    int visItems = std::min((int)results.size(), 6);

    SetWindowPos(hAutoComplete, HWND_TOPMOST,
        r.left, r.bottom, r.right - r.left, itemHeight * visItems + 4,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

// ─── Registry Helpers ─────────────────────────────────────────────────────────

void Book_SaveList(const char* listName, HWND hLB) {
    int count = SendMessage(hLB, LB_GETCOUNT, 0, 0);
    std::string multiStr;
    for (int i = 0; i < count; i++) {
        int len = SendMessage(hLB, LB_GETTEXTLEN, i, 0);
        std::string item(len + 1, '\0');
        SendMessageA(hLB, LB_GETTEXT, i, (LPARAM)item.data());
        item.resize(len);
        multiStr += item;
        multiStr += '\0';
    }
    multiStr += '\0';

    HKEY hKey;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\IBKR_Tunnel\\Book",
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, listName, 0, REG_MULTI_SZ,
            (const BYTE*)multiStr.data(), (DWORD)multiStr.size());
        RegCloseKey(hKey);
    }
}

void Book_LoadList(const char* listName, HWND hLB) {
    SendMessage(hLB, LB_RESETCONTENT, 0, 0);

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\IBKR_Tunnel\\Book",
        0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    DWORD type, size = 0;
    RegQueryValueExA(hKey, listName, NULL, &type, NULL, &size);
    if (size == 0) { RegCloseKey(hKey); return; }

    std::vector<char> buf(size);
    RegQueryValueExA(hKey, listName, NULL, &type, (LPBYTE)buf.data(), &size);
    RegCloseKey(hKey);

    const char* p = buf.data();
    while (*p) {
        SendMessageA(hLB, LB_ADDSTRING, 0, (LPARAM)p);
        p += strlen(p) + 1;
    }
}

void Book_LoadAllLists(HWND hCB) {
    SendMessage(hCB, CB_RESETCONTENT, 0, 0);

    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\IBKR_Tunnel\\Book",
        0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

    char valueName[256];
    DWORD index = 0, nameSize = sizeof(valueName);
    while (RegEnumValueA(hKey, index++, valueName, &nameSize,
        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        SendMessageA(hCB, CB_ADDSTRING, 0, (LPARAM)valueName);
        nameSize = sizeof(valueName);
    }
    RegCloseKey(hKey);

    if (SendMessage(hCB, CB_GETCOUNT, 0, 0) > 0)
        SendMessage(hCB, CB_SETCURSEL, 0, 0);
}

void Book_DeleteList(const char* listName) {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\IBKR_Tunnel\\Book",
        0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, listName);
        RegCloseKey(hKey);
    }
}

std::string Book_GetSelectedList() {
    int sel = SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return "";
    int len = SendMessage(hCombo, CB_GETLBTEXTLEN, sel, 0);
    std::string name(len + 1, '\0');
    SendMessageA(hCombo, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

// ─── Window Procedure ────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcBook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        int margin = 8;

        hCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            margin, margin, 180, 200,
            hWnd, (HMENU)ID_BOOK_COMBO, hInst, NULL);

        CreateWindowA("BUTTON", "New List",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            margin + 188, margin, 80, 24,
            hWnd, (HMENU)ID_BOOK_NEW_LIST, hInst, NULL);

        CreateWindowA("BUTTON", "Delete List",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            margin + 276, margin, 80, 24,
            hWnd, (HMENU)ID_BOOK_DEL_LIST, hInst, NULL);

        hListBox = CreateWindowA("LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            margin, margin + 32, 356 - margin, 130,
            hWnd, (HMENU)ID_BOOK_LISTBOX, hInst, NULL);

        hEdit = CreateWindowA("EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            margin, margin + 170, 180, 24,
            hWnd, (HMENU)ID_BOOK_EDIT, hInst, NULL);

        hAutoCompleteOwner = hWnd;
        hAutoComplete = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            "LISTBOX", NULL,
            WS_POPUP | WS_BORDER | LBS_NOTIFY,
            0, 0, 180, 100,
            hWnd, NULL, hInst, NULL);
        SetWindowSubclass(hAutoComplete, AutoCompleteSubclassProc, 1, 0);
        ShowWindow(hAutoComplete, SW_HIDE);

        api.setSymbolSearchWindow(hWnd);

        CreateWindowA("BUTTON", "Add",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            margin + 188, margin + 170, 80, 24,
            hWnd, (HMENU)ID_BOOK_ADD_ITEM, hInst, NULL);

        CreateWindowA("BUTTON", "Remove",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            margin + 276, margin + 170, 80, 24,
            hWnd, (HMENU)ID_BOOK_DEL_ITEM, hInst, NULL);

        Book_LoadAllLists(hCombo);
        std::string first = Book_GetSelectedList();
        if (!first.empty()) Book_LoadList(first.c_str(), hListBox);

        break;
    }

    case WM_TIMER:
        if (wParam == TIMER_DROPDOWN) {
            KillTimer(hWnd, TIMER_DROPDOWN);
            ShowWindow(hAutoComplete, SW_HIDE);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case ID_BOOK_COMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                std::string name = Book_GetSelectedList();
                if (!name.empty()) Book_LoadList(name.c_str(), hListBox);
            }
            break;

        case ID_BOOK_NEW_LIST: {
            static bool dialogClassRegistered = false;
            if (!dialogClassRegistered) {
                WNDCLASSA wc = {};
                wc.lpfnWndProc = DefWindowProcA;
                wc.hInstance = GetModuleHandle(NULL);
                wc.lpszClassName = "BookNewListDialog";
                wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
                wc.hCursor = LoadCursor(NULL, IDC_ARROW);
                RegisterClassA(&wc);
                dialogClassRegistered = true;
            }

            int dlgW = 260, dlgH = 75;
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            int dlgX = (screenW - dlgW) / 2;
            int dlgY = (screenH - dlgH) / 2;

            HWND hDlg = CreateWindowExA(
                WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
                "BookNewListDialog", "New List Name",
                WS_POPUP | WS_CAPTION | WS_SYSMENU,
                dlgX, dlgY, dlgW, dlgH,
                hWnd, NULL, GetModuleHandle(NULL), NULL);

            HWND hDlgEdit = CreateWindowA("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 10, dlgW - 28, 24,
                hDlg, (HMENU)1, GetModuleHandle(NULL), NULL);

            ShowWindow(hDlg, SW_SHOW);
            UpdateWindow(hDlg);
            SetFocus(hDlgEdit);

            MSG dlgMsg;
            char newName[128] = {};
            bool dlgDone = false;

            while (!dlgDone && GetMessage(&dlgMsg, NULL, 0, 0)) {
                if (dlgMsg.message == WM_KEYDOWN && dlgMsg.wParam == VK_RETURN) {
                    GetWindowTextA(hDlgEdit, newName, sizeof(newName));
                    dlgDone = true;
                } else if (dlgMsg.message == WM_KEYDOWN && dlgMsg.wParam == VK_ESCAPE) {
                    dlgDone = true;
                } else {
                    TranslateMessage(&dlgMsg);
                    DispatchMessage(&dlgMsg);
                }
                if (!IsWindow(hDlg)) dlgDone = true;
            }
            DestroyWindow(hDlg);

            if (strlen(newName) > 0) {
                HKEY hKey;
                if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\IBKR_Tunnel\\Book",
                    0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                    const char empty[2] = { '\0', '\0' };
                    RegSetValueExA(hKey, newName, 0, REG_MULTI_SZ, (const BYTE*)empty, 2);
                    RegCloseKey(hKey);
                }
                Book_LoadAllLists(hCombo);
                int idx = SendMessageA(hCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)newName);
                if (idx != CB_ERR) {
                    SendMessage(hCombo, CB_SETCURSEL, idx, 0);
                    Book_LoadList(newName, hListBox);
                }
            }
            break;
        }

        case ID_BOOK_DEL_LIST: {
            std::string name = Book_GetSelectedList();
            if (name.empty()) break;
            std::string msg = "Delete list \"" + name + "\"?";
            if (MessageBoxA(hWnd, msg.c_str(), "Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                Book_DeleteList(name.c_str());
                Book_LoadAllLists(hCombo);
                SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
                std::string next = Book_GetSelectedList();
                if (!next.empty()) Book_LoadList(next.c_str(), hListBox);
            }
            break;
        }

        case ID_BOOK_ADD_ITEM: {
            char item[256] = {};
            GetWindowTextA(hEdit, item, sizeof(item));
            if (strlen(item) == 0) break;

            std::string name = Book_GetSelectedList();
            if (name.empty()) {
                MessageBoxA(hWnd, "Select or create a list first.", "No List", MB_OK);
                break;
            }

            auto results = api.getSymbolResults();
            bool valid = std::find(results.begin(), results.end(), std::string(item)) != results.end();
            if (!valid && api.isConnected()) {
                MessageBoxA(hWnd, "Please select a valid symbol from the suggestions.", "Invalid Symbol", MB_OK);
                break;
            }

            int exists = SendMessageA(hListBox, LB_FINDSTRINGEXACT, -1, (LPARAM)item);
            if (exists == LB_ERR) {
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)item);
                Book_SaveList(name.c_str(), hListBox);
                SetWindowTextA(hEdit, "");
                ShowWindow(hAutoComplete, SW_HIDE);
            }
            break;
        }

        case ID_BOOK_DEL_ITEM: {
            int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR) break;
            std::string name = Book_GetSelectedList();
            SendMessage(hListBox, LB_DELETESTRING, sel, 0);
            Book_SaveList(name.c_str(), hListBox);
            break;
        }

        case ID_BOOK_EDIT:
            if (HIWORD(wParam) == EN_CHANGE) {
                char text[256] = {};
                GetWindowTextA(hEdit, text, sizeof(text));
                if (strlen(text) >= 2) {
                    api.searchSymbols(text);
                } else {
                    ShowWindow(hAutoComplete, SW_HIDE);
                }
            } else if (HIWORD(wParam) == EN_KILLFOCUS) {
                SetTimer(hWnd, TIMER_DROPDOWN, 150, NULL);
            }
            break;

        case ID_BOOK_AUTOCOMPLETE:
            if (HIWORD(wParam) == LBN_SELCHANGE || HIWORD(wParam) == LBN_DBLCLK) {
                int sel = SendMessage(hAutoComplete, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    char sym[64] = {};
                    SendMessageA(hAutoComplete, LB_GETTEXT, sel, (LPARAM)sym);
                    SetWindowTextA(hEdit, sym);
                    ShowWindow(hAutoComplete, SW_HIDE);
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_BOOK_ADD_ITEM, 0), 0);
                    }
                }
            }
            break;
        }
        break;

    case WM_LBUTTONDOWN:
        ShowWindow(hAutoComplete, SW_HIDE);
        break;

    case WM_SYMBOL_RESULTS: {
        auto results = api.getSymbolResults();
        Book_UpdateAutoComplete(hWnd, results);
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_MOVE:
        SaveWinPosition(hWnd, BOOK_CLASS_NAME);
        break;

    case WM_DESTROY:
        SaveWinPosition(hWnd, BOOK_CLASS_NAME);
        api.setSymbolSearchWindow(NULL);
        hBookWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void registerBook(HINSTANCE hInst) {
    WNDCLASS swc = { 0 };
    swc.lpfnWndProc = WndProcBook;
    swc.hInstance = hInst;
    swc.lpszClassName = BOOK_CLASS_NAME;
    swc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(2));
    swc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClass(&swc);
}

void startBook() {
    if (hBookWnd && IsWindow(hBookWnd)) {
        ShowWindow(hBookWnd, SW_SHOW);
        SetForegroundWindow(hBookWnd);
    } else {
        int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 380, h = 280;
        LoadWinPosition(BOOK_CLASS_NAME, x, y, w, h);

        hBookWnd = CreateWindowExA(
            WS_EX_APPWINDOW,
            BOOK_CLASS_NAME,
            "Book",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
            x, y, w, h,
            NULL, NULL, GetModuleHandle(NULL), NULL
        );

        api.setSymbolSearchWindow(hBookWnd);
        SetWindowTaskbarId(hBookWnd, L"IBKRTunnel.Book");
    }
}