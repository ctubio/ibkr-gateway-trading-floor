#pragma once

void startBook() { startGenericWindow(BOOK_CLASS_NAME, "Book", L"IBKRGatewayClient.Book", 373, 240); }

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
#define ID_BOOK_ITEM_UP      2009
#define ID_BOOK_ITEM_DOWN    2010
#define TIMER_DROPDOWN       99

static HWND hBookCombo, hBookListBox, hBookEdit, hBookBtnDelList, hBookLabelNewSymbol, hBookBtnUp, hBookBtnDown;
static bool suppressSearch = false;
static bool showingOffline = false;
static std::vector<std::string> currentResults;  // full conId.symbol.exchange
static std::string pendingFullEntry;             // set when user selects from dropdown

// ─── Helpers ─────────────────────────────────────────────────────────────────

std::string Book_DisplayLabel(const std::string& entry) {
    // Format: conId.symbol.exchange — skip the conId part
    auto first = entry.find('.');
    if (first == std::string::npos) return entry;
    return entry.substr(first + 1); // returns "symbol.exchange"
}

std::string Book_GetSelectedBook() {
    int sel = SendMessage(hBookCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return "";
    int len = SendMessage(hBookCombo, CB_GETLBTEXTLEN, sel, 0);
    std::string name(len + 1, '\0');
    SendMessageA(hBookCombo, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

void Book_UpdateControlStates();  // forward declaration

// ─── Registry Helpers ─────────────────────────────────────────────────────────

void Book_SaveFullList(const char* listName, const std::vector<std::string>& items) {
    std::string multiStr;
    for (const auto& item : items) {
        multiStr += item;
        multiStr += '\0';
    }
    multiStr += '\0';

    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Book", APP_REG_ROOT);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, listName, 0, REG_MULTI_SZ,
            (const BYTE*)multiStr.data(), (DWORD)multiStr.size());
        RegCloseKey(hKey);
    }
}

static std::vector<std::string> bookItems; // full conId.symbol.exchange mirroring listbox

std::vector<std::string> Book_ReadListEntries(const char* listName) {
    std::vector<std::string> entries;
    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Book", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return entries;
    DWORD type, size = 0;
    RegQueryValueExA(hKey, listName, NULL, &type, NULL, &size);
    if (size > 0) {
        std::vector<char> buf(size);
        RegQueryValueExA(hKey, listName, NULL, &type, (LPBYTE)buf.data(), &size);
        const char* p = buf.data();
        while (*p) { entries.push_back(p); p += strlen(p) + 1; }
    }
    RegCloseKey(hKey);
    return entries;
}

void Book_LoadList(const char* listName, HWND hLB) {
    SendMessage(hLB, LB_RESETCONTENT, 0, 0);
    bookItems.clear();

    auto entries = Book_ReadListEntries(listName);
    for (const auto& full : entries) {
        bookItems.push_back(full);
        SendMessageA(hLB, LB_ADDSTRING, 0, (LPARAM)Book_DisplayLabel(full).c_str());
    }
}

void Book_LoadAllLists(HWND hCB) {
    SendMessage(hCB, CB_RESETCONTENT, 0, 0);

    HKEY hKey;
    char fullPath[256];
    wsprintf(fullPath, "%s\\Book", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return;

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
    char fullPath[256];
    wsprintf(fullPath, "%s\\Book", APP_REG_ROOT);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, listName);
        RegCloseKey(hKey);
    }
}

// ─── AutoComplete ─────────────────────────────────────────────────────────────

void Book_UpdateAutoComplete(HWND hWnd, const std::vector<std::string>& results) {
    EnableWindow(hAutoComplete, TRUE);
    currentResults = results;

    if (results.empty()) {
        ShowWindow(hAutoComplete, SW_HIDE);
        return;
    }

    SendMessage(hAutoComplete, LB_RESETCONTENT, 0, 0);
    for (const auto& s : results)
        SendMessageA(hAutoComplete, LB_ADDSTRING, 0, (LPARAM)Book_DisplayLabel(s).c_str());

    RECT r;
    GetWindowRect(hBookEdit, &r);

    int itemHeight = SendMessage(hAutoComplete, LB_GETITEMHEIGHT, 0, 0);
    int visItems = std::min((int)results.size(), 6);

    SetWindowPos(hAutoComplete, HWND_TOPMOST,
        r.left, r.bottom, r.right - r.left, itemHeight * visItems + 4,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

void Book_UpdateControlStates() {
    std::string book = Book_GetSelectedBook();
    bool hasBook = !book.empty();
    int itemSel = SendMessage(hBookListBox, LB_GETCURSEL, 0, 0);
    int itemCount = SendMessage(hBookListBox, LB_GETCOUNT, 0, 0);
    bool hasItem = itemSel != LB_ERR;

    std::string title = hasBook ? "Book: " + book : "Book";
    SetWindowTextA(GetParent(hBookBtnDelList), title.c_str());

    EnableWindow(hBookBtnDelList,     hasBook);
    EnableWindow(hBookEdit,           hasBook);
    EnableWindow(hBookLabelNewSymbol, hasBook);
    EnableWindow(hBookBtnUp,          hasBook && hasItem && itemSel > 0);
    EnableWindow(hBookBtnDown,        hasBook && hasItem && itemSel < itemCount - 1);
}

// ─── Subclass Procs ───────────────────────────────────────────────────────────

LRESULT CALLBACK ListBoxSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                      UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_KEYDOWN && wParam == VK_DELETE) {
        int sel = SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)bookItems.size()) {
            std::string name = Book_GetSelectedBook();
            SendMessage(hWnd, LB_DELETESTRING, sel, 0);
            bookItems.erase(bookItems.begin() + sel);
            Book_SaveFullList(name.c_str(), bookItems);
            Book_UpdateControlStates();
        }
        return 0;
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, ListBoxSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                   UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_KEYDOWN) {
        bool acVisible = IsWindowVisible(hAutoComplete);

        if (wParam == VK_DOWN && acVisible) {
            int count = SendMessage(hAutoComplete, LB_GETCOUNT, 0, 0);
            int sel = SendMessage(hAutoComplete, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR) sel = -1;
            if (sel + 1 < count)
                SendMessage(hAutoComplete, LB_SETCURSEL, sel + 1, 0);
            return 0;
        }
        if (wParam == VK_UP && acVisible) {
            int sel = SendMessage(hAutoComplete, LB_GETCURSEL, 0, 0);
            if (sel > 0)
                SendMessage(hAutoComplete, LB_SETCURSEL, sel - 1, 0);
            return 0;
        }
        if (wParam == VK_RETURN && acVisible) {
            int sel = SendMessage(hAutoComplete, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR && sel < (int)currentResults.size()) {
                pendingFullEntry = currentResults[sel];
                std::string label = Book_DisplayLabel(pendingFullEntry);
                suppressSearch = true;
                SetWindowTextA(hBookEdit, label.c_str());
                suppressSearch = false;
                ShowWindow(hAutoComplete, SW_HIDE);
                SendMessage(hBookEdit, EM_SETSEL, label.size(), label.size());
                SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(ID_BOOK_ADD_ITEM, 0), 0);
            }
            return 0;
        }
        if (wParam == VK_RETURN && !acVisible) {
            SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(ID_BOOK_ADD_ITEM, 0), 0);
            return 0;
        }
        if (wParam == VK_ESCAPE && acVisible) {
            ShowWindow(hAutoComplete, SW_HIDE);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, EditSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK AutoCompleteSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_LBUTTONUP) {
        int sel = SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)currentResults.size()) {
            pendingFullEntry = currentResults[sel];
            std::string label = Book_DisplayLabel(pendingFullEntry);
            suppressSearch = true;
            SetWindowTextA(hBookEdit, label.c_str());
            suppressSearch = false;
            ShowWindow(hWnd, SW_HIDE);
            KillTimer(GetParent(hWnd), TIMER_DROPDOWN);
            SetFocus(hBookEdit);
            SendMessage(hBookEdit, EM_SETSEL, label.size(), label.size());
            SendMessage(GetParent(hWnd), WM_COMMAND,
                MAKEWPARAM(ID_BOOK_AUTOCOMPLETE, LBN_DBLCLK), (LPARAM)hWnd);
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, AutoCompleteSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// ─── Window Procedure ────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcBook(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
            int margin = 8;

            hBookCombo = CreateWindowA("COMBOBOX", NULL,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                margin, margin, 166, 193,
                hWnd, (HMENU)ID_BOOK_COMBO, hInst, NULL);

            CreateWindowA("BUTTON", "New Book",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                margin + 171, margin, 80, 24,
                hWnd, (HMENU)ID_BOOK_NEW_LIST, hInst, NULL);

            hBookBtnDelList = CreateWindowA("BUTTON", "Delete Book",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                margin + 259, margin, 90, 24,
                hWnd, (HMENU)ID_BOOK_DEL_LIST, hInst, NULL);

            hBookListBox = CreateWindowA("LISTBOX", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
                margin, margin + 32, 313 - margin, 130,
                hWnd, (HMENU)ID_BOOK_LISTBOX, hInst, NULL);

            SetWindowSubclass(hBookListBox, ListBoxSubclassProc, 3, 0);

            hBookBtnUp = CreateWindowW(L"BUTTON", L"▲",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                320, margin + 32, 36, 62,
                hWnd, (HMENU)ID_BOOK_ITEM_UP, hInst, NULL);

            hBookBtnDown = CreateWindowW(L"BUTTON", L"▼",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                320, margin + 32 + 68, 36, 62,
                hWnd, (HMENU)ID_BOOK_ITEM_DOWN, hInst, NULL);

            hBookLabelNewSymbol = CreateWindowA("STATIC", "New Symbol:",
                WS_CHILD | WS_VISIBLE,
                margin, margin + 174, 90, 16,
                hWnd, NULL, hInst, NULL);

            hBookEdit = CreateWindowA("EDIT", NULL,
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
                margin + 95, margin + 170, 254, 24,
                hWnd, (HMENU)ID_BOOK_EDIT, hInst, NULL);

            SetWindowSubclass(hBookEdit, EditSubclassProc, 2, 0);

            hAutoComplete = CreateWindowExA(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                "LISTBOX", NULL,
                WS_POPUP | WS_BORDER | LBS_NOTIFY,
                0, 0, 180, 100,
                hWnd, NULL, hInst, NULL);
            SetWindowSubclass(hAutoComplete, AutoCompleteSubclassProc, 1, 0);
            ShowWindow(hAutoComplete, SW_HIDE);

            Book_LoadAllLists(hBookCombo);

            if (SendMessage(hBookCombo, CB_GETCOUNT, 0, 0) > 0)
                SendMessage(hBookCombo, CB_SETCURSEL, 0, 0);

            std::string first = Book_GetSelectedBook();
            if (!first.empty()) Book_LoadList(first.c_str(), hBookListBox);

            Book_UpdateControlStates();

            
            api.setSymbolSearchWindow(hWnd);
            break;
        }

        case WM_DESTROY:
            api.setSymbolSearchWindow(NULL);
            break;

        case WM_TIMER:
            if (wParam == TIMER_DROPDOWN) {
                KillTimer(hWnd, TIMER_DROPDOWN);
                ShowWindow(hAutoComplete, SW_HIDE);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {

            case ID_BOOK_ITEM_UP: {
                int sel = SendMessage(hBookListBox, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR || sel == 0) break;
                char item[256] = {};
                SendMessageA(hBookListBox, LB_GETTEXT, sel, (LPARAM)item);
                SendMessage(hBookListBox, LB_DELETESTRING, sel, 0);
                SendMessageA(hBookListBox, LB_INSERTSTRING, sel - 1, (LPARAM)item);
                SendMessage(hBookListBox, LB_SETCURSEL, sel - 1, 0);
                std::swap(bookItems[sel], bookItems[sel - 1]);
                Book_SaveFullList(Book_GetSelectedBook().c_str(), bookItems);
                Book_UpdateControlStates();
                break;
            }

            case ID_BOOK_ITEM_DOWN: {
                int sel = SendMessage(hBookListBox, LB_GETCURSEL, 0, 0);
                int count = SendMessage(hBookListBox, LB_GETCOUNT, 0, 0);
                if (sel == LB_ERR || sel == count - 1) break;
                char item[256] = {};
                SendMessageA(hBookListBox, LB_GETTEXT, sel, (LPARAM)item);
                SendMessage(hBookListBox, LB_DELETESTRING, sel, 0);
                SendMessageA(hBookListBox, LB_INSERTSTRING, sel + 1, (LPARAM)item);
                SendMessage(hBookListBox, LB_SETCURSEL, sel + 1, 0);
                std::swap(bookItems[sel], bookItems[sel + 1]);
                Book_SaveFullList(Book_GetSelectedBook().c_str(), bookItems);
                Book_UpdateControlStates();
                break;
            }

            case ID_BOOK_COMBO:
                if (HIWORD(wParam) == CBN_SELCHANGE) {
                    std::string name = Book_GetSelectedBook();
                    if (!name.empty()) Book_LoadList(name.c_str(), hBookListBox);
                    Book_UpdateControlStates();
                }
                break;

            case ID_BOOK_NEW_LIST: {
                int dlgW = 260;
                startGenericWindow(BOOK_NEW_LIST_CLASS_NAME, "New Book Name", L"IBKRGatewayClient.BookNewList", dlgW, 75);

                HWND hDlgEdit = CreateWindowA("EDIT", "",
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    10, 10, dlgW - 28, 24,
                    g_AppWindows[BOOK_NEW_LIST_CLASS_NAME], (HMENU)1, GetModuleHandle(NULL), NULL);

                ShowWindow(g_AppWindows[BOOK_NEW_LIST_CLASS_NAME], SW_SHOW);
                UpdateWindow(g_AppWindows[BOOK_NEW_LIST_CLASS_NAME]);
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
                    if (!IsWindow(g_AppWindows[BOOK_NEW_LIST_CLASS_NAME])) dlgDone = true;
                }
                DestroyWindow(g_AppWindows[BOOK_NEW_LIST_CLASS_NAME]);

                if (strlen(newName) > 0) {
                    HKEY hKey;
                    char fullPath[256];
                    wsprintf(fullPath, "%s\\Book", APP_REG_ROOT);
                    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                        const char empty[2] = { '\0', '\0' };
                        RegSetValueExA(hKey, newName, 0, REG_MULTI_SZ, (const BYTE*)empty, 2);
                        RegCloseKey(hKey);
                    }
                    Book_LoadAllLists(hBookCombo);
                    int idx = SendMessageA(hBookCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)newName);
                    if (idx != CB_ERR) {
                        SendMessage(hBookCombo, CB_SETCURSEL, idx, 0);
                        Book_LoadList(newName, hBookListBox);
                        Book_UpdateControlStates();
                    }
                }
                break;
            }

            case ID_BOOK_DEL_LIST: {
                std::string name = Book_GetSelectedBook();
                if (name.empty()) break;
                std::string msg = "Delete book \"" + name + "\"?";
                if (MessageBoxA(hWnd, msg.c_str(), "Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    Book_DeleteList(name.c_str());
                    Book_LoadAllLists(hBookCombo);
                    SendMessage(hBookListBox, LB_RESETCONTENT, 0, 0);
                    bookItems.clear();
                    std::string next = Book_GetSelectedBook();
                    if (!next.empty()) Book_LoadList(next.c_str(), hBookListBox);
                    Book_UpdateControlStates();
                }
                break;
            }

            case ID_BOOK_ADD_ITEM: {
                char item[256] = {};
                GetWindowTextA(hBookEdit, item, sizeof(item));
                if (strlen(item) == 0) break;

                std::string name = Book_GetSelectedBook();
                if (name.empty()) {
                    MessageBoxA(hWnd, "Select or create a book first.", "No Book", MB_OK);
                    break;
                }

                // Use pendingFullEntry if set, otherwise reject
                std::string toStore = pendingFullEntry;
                pendingFullEntry = "";

                if (toStore.empty() && api.isConnected()) {
                    MessageBoxA(hWnd, "Please select a valid symbol from the suggestions.", "Invalid Symbol", MB_OK);
                    break;
                }

                if (toStore.empty()) toStore = item; // offline fallback

                // Check for duplicates by display label
                std::string displayLabel = Book_DisplayLabel(toStore);
                int exists = SendMessageA(hBookListBox, LB_FINDSTRINGEXACT, -1, (LPARAM)displayLabel.c_str());
                if (exists == LB_ERR) {
                    SendMessageA(hBookListBox, LB_ADDSTRING, 0, (LPARAM)displayLabel.c_str());
                    bookItems.push_back(toStore);
                    Book_SaveFullList(name.c_str(), bookItems);
                    SetWindowTextA(hBookEdit, "");
                    ShowWindow(hAutoComplete, SW_HIDE);
                    Book_UpdateControlStates();
                }
                break;
            }

            case ID_BOOK_EDIT:
                if (HIWORD(wParam) == EN_CHANGE) {
                    if (suppressSearch) break;
                    pendingFullEntry = ""; // clear pending when user types manually
                    char text[256] = {};
                    GetWindowTextA(hBookEdit, text, sizeof(text));

                    std::string query = text;
                    auto dot = query.find('.');
                    if (dot != std::string::npos)
                        query = query.substr(0, dot);

                    if (query.size() >= 1) {
                        if (!api.isConnected()) {
                            showingOffline = true;
                            SendMessage(hAutoComplete, LB_RESETCONTENT, 0, 0);
                            SendMessageA(hAutoComplete, LB_ADDSTRING, 0, (LPARAM)"Gateway is disconnected!");
                            RECT r;
                            GetWindowRect(hBookEdit, &r);
                            int itemHeight = SendMessage(hAutoComplete, LB_GETITEMHEIGHT, 0, 0);
                            SetWindowPos(hAutoComplete, HWND_TOP,
                                r.left, r.bottom, r.right - r.left, itemHeight + 4,
                                SWP_SHOWWINDOW | SWP_NOACTIVATE);
                            EnableWindow(hAutoComplete, FALSE);
                        } else {
                            showingOffline = false;
                            api.searchSymbols(query);
                        }
                    } else {
                        ShowWindow(hAutoComplete, SW_HIDE);
                    }
                } else if (HIWORD(wParam) == EN_KILLFOCUS) {
                    SetTimer(hWnd, TIMER_DROPDOWN, 150, NULL);
                }
                break;

            case ID_BOOK_LISTBOX:
                if (HIWORD(wParam) == LBN_SELCHANGE)
                    Book_UpdateControlStates();
                break;

            case ID_BOOK_AUTOCOMPLETE:
                if (HIWORD(wParam) == LBN_DBLCLK) {
                    SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_BOOK_ADD_ITEM, 0), 0);
                }
                break;
            }
            break;

        case WM_LBUTTONDOWN:
            ShowWindow(hAutoComplete, SW_HIDE);
            break;

        case WM_SYMBOL_RESULTS: {
            if (showingOffline) break;
            char text[256] = {};
            GetWindowTextA(hBookEdit, text, sizeof(text));
            std::string query = text;

            auto results = api.getSymbolResults();

            if (query.find('.') != std::string::npos) {
                std::string upper = query;
                for (auto& c : upper) c = toupper(c);
                std::vector<std::string> filtered;
                for (const auto& r : results) {
                    std::string label = Book_DisplayLabel(r);
                    std::string lu = label;
                    for (auto& c : lu) c = toupper(c);
                    if (lu.find(upper) == 0)
                        filtered.push_back(r);
                }
                Book_UpdateAutoComplete(hWnd, filtered);
            } else {
                Book_UpdateAutoComplete(hWnd, results);
            }
            break;
        }
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
