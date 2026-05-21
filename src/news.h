#pragma once

void startNews() { startGenericWindow(NEWS_CLASS_NAME, "News", L"IBKRGatewayClient.News", 420, 500); }

#define ID_NEWS_LIST_COMBO   6001
#define ID_NEWS_SYM_COMBO    6002
#define ID_NEWS_RESULTS_LIST 6003

static HWND hNewsListCombo = NULL;   // selects which book list
static HWND hNewsSymCombo  = NULL;   // selects symbol within that list
static HWND hNewsResults   = NULL;   // displays news items

// Full entries (conId.symbol.exchange) parallel to hNewsSymCombo indices
static std::vector<std::string> newsSymEntries;

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Returns the currently selected list name from hNewsListCombo, or "".
static std::string News_GetSelectedList() {
    int sel = SendMessage(hNewsListCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return "";
    int len = SendMessage(hNewsListCombo, CB_GETLBTEXTLEN, sel, 0);
    if (len <= 0) return "";
    std::string name(len + 1, '\0');
    SendMessageA(hNewsListCombo, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

// Populates hNewsListCombo from the registry and restores savedList selection.
// Returns true if the list combo contains at least one entry.
static bool News_LoadListCombo(const std::string& savedList = "") {
    SendMessage(hNewsListCombo, CB_RESETCONTENT, 0, 0);

    Book_LoadAllLists(hNewsListCombo);

    int count = (int)SendMessage(hNewsListCombo, CB_GETCOUNT, 0, 0);
    if (count == 0) return false;

    // Try to restore saved list, fall back to first entry
    int idx = CB_ERR;
    if (!savedList.empty())
        idx = (int)SendMessageA(hNewsListCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)savedList.c_str());
    if (idx == CB_ERR) idx = 0;

    SendMessage(hNewsListCombo, CB_SETCURSEL, idx, 0);
    return true;
}

// Populates hNewsSymCombo from the selected list. Restores savedEntry if provided.
// Returns true if at least one symbol was loaded.
static bool News_LoadSymbolCombo(const std::string& savedEntry = "") {
    SendMessage(hNewsSymCombo, CB_RESETCONTENT, 0, 0);
    newsSymEntries.clear();

    std::string listName = News_GetSelectedList();
    if (listName.empty()) return false;

    auto entries = Book_ReadListEntries(listName.c_str());
    for (const auto& full : entries) {
        newsSymEntries.push_back(full);
        SendMessageA(hNewsSymCombo, CB_ADDSTRING, 0,
                     (LPARAM)Book_DisplayLabel(full).c_str());
    }

    if (newsSymEntries.empty()) return false;

    // Restore saved selection or default to first
    int idx = CB_ERR;
    if (!savedEntry.empty()) {
        for (int i = 0; i < (int)newsSymEntries.size(); ++i) {
            if (newsSymEntries[i] == savedEntry) { idx = i; break; }
        }
    }
    if (idx == CB_ERR) idx = 0;
    SendMessage(hNewsSymCombo, CB_SETCURSEL, idx, 0);
    return true;
}

// ─── News request helper ──────────────────────────────────────────────────────

void News_RequestForSymbol(const std::string& fullEntry) {
    auto firstDot = fullEntry.find('.');
    if (firstDot == std::string::npos) return;
    std::string conIdStr = fullEntry.substr(0, firstDot);
    std::string rest     = fullEntry.substr(firstDot + 1);
    auto secondDot       = rest.find('.');
    std::string symbol   = (secondDot != std::string::npos) ? rest.substr(0, secondDot) : rest;

    Settings_SaveString("LastNewsEntry", fullEntry);

    LogDebug("News request for: " + symbol + " conId: " + conIdStr);

    HWND newsWnd = g_AppWindows[NEWS_CLASS_NAME];
    if (newsWnd) SetWindowTextA(newsWnd, ("News: " + symbol).c_str());

    if (hNewsResults) SendMessage(hNewsResults, LB_RESETCONTENT, 0, 0);
    api.reqNewsForSymbol(std::stoi(conIdStr), symbol);
}

// ─── Window Procedure ─────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcNews(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    const int comboGap = 8;
    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        const int margin = 8;
        const int rowH   = 24;
        const int labelW = 55;

        // Create two comboboxes side-by-side (List | Symbol)
        int comboY = margin;
        int comboH = 200;

        hNewsListCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            margin, comboY, 180, comboH,
            hWnd, (HMENU)ID_NEWS_LIST_COMBO, hInst, NULL);

        hNewsSymCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            margin + 180 + comboGap, comboY, 180, comboH,
            hWnd, (HMENU)ID_NEWS_SYM_COMBO, hInst, NULL);

        // Start hidden until mouse hover
        ShowWindow(hNewsListCombo, SW_HIDE);
        ShowWindow(hNewsSymCombo,  SW_HIDE);

        // News results list — fills remaining space
        int listY = comboY + rowH + 7;
        hNewsResults = CreateWindowA("LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            margin, listY, 400 - margin * 2, 500 - listY - margin,
            hWnd, (HMENU)ID_NEWS_RESULTS_LIST, hInst, NULL);

        api.setNewsWindow(hWnd);
        SetWindowTextA(hWnd, "News");

        // Restore last session
        std::string lastList  = Settings_LoadString("LastNewsList");
        std::string lastEntry = Settings_LoadString("LastNewsEntry");

        News_LoadListCombo(lastList);
        if (News_LoadSymbolCombo(lastEntry)) {
            // Re-request news for the restored symbol
            int sel = (int)SendMessage(hNewsSymCombo, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR && sel < (int)newsSymEntries.size())
                News_RequestForSymbol(newsSymEntries[sel]);
        }
        break;
    }

    case WM_SIZE: {
        if (!hNewsResults) return 0;
        RECT rc;
        GetClientRect(hWnd, &rc);
        const int margin = 8;
        const int rowH   = 24;
        const int comboGap = 8;
        int listY = margin + rowH + 7;   // below combo row
        MoveWindow(hNewsResults,
                   margin, listY,
                   rc.right  - margin * 2,
                   rc.bottom - listY - margin,
                   TRUE);

        // Position combos side-by-side and stretch with window
        int availW = rc.right - margin * 2 - comboGap;
        int eachW = availW / 2;
        SetWindowPos(hNewsListCombo, NULL, margin, margin, eachW, 200,
                 SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hNewsSymCombo,  NULL, margin + eachW + comboGap, margin, eachW, 200,
                 SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    case WM_ACTIVATE: {
        // Show combos when the window becomes active/focused; hide when inactive
        if (LOWORD(wParam) != WA_INACTIVE) {
            if (hNewsListCombo) ShowWindow(hNewsListCombo, SW_SHOW);
            if (hNewsSymCombo)  ShowWindow(hNewsSymCombo,  SW_SHOW);
        } else {
            if (hNewsListCombo) ShowWindow(hNewsListCombo, SW_HIDE);
            if (hNewsSymCombo)  ShowWindow(hNewsSymCombo,  SW_HIDE);
        }

        // Recalculate layout immediately
        RECT rc;
        GetClientRect(hWnd, &rc);
        const int margin = 8;
        const int rowH   = 24;
        const int comboGap = 8;

        bool combosVisible = (hNewsListCombo && IsWindowVisible(hNewsListCombo));
        int listY = combosVisible ? (margin + rowH + 7) : margin;

        // Move/resize results list
        if (hNewsResults) {
            MoveWindow(hNewsResults,
                       margin, listY,
                       rc.right  - margin * 2,
                       rc.bottom - listY - margin,
                       TRUE);
        }

        // Position combos if visible
        if (combosVisible && hNewsListCombo && hNewsSymCombo) {
            int availW = rc.right - margin * 2 - comboGap;
            int eachW = availW / 2;
            SetWindowPos(hNewsListCombo, NULL, margin, margin, eachW, 200,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(hNewsSymCombo,  NULL, margin + eachW + comboGap, margin, eachW, 200,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_NEWS_LIST_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            std::string listName = News_GetSelectedList();
            Settings_SaveString("LastNewsList", listName);
            News_LoadSymbolCombo();          // reset symbol combo for new list
            // Auto-load news for first symbol in the new list
            if (!newsSymEntries.empty())
                News_RequestForSymbol(newsSymEntries[0]);
        }
        if (LOWORD(wParam) == ID_NEWS_SYM_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = (int)SendMessage(hNewsSymCombo, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR && sel < (int)newsSymEntries.size())
                News_RequestForSymbol(newsSymEntries[sel]);
        }
        break;

    case WM_NEWS_RESULTS: {
        auto news = api.getNewsResults();
        if (hNewsResults) {
            SendMessage(hNewsResults, LB_RESETCONTENT, 0, 0);
            for (const auto& item : news)
                SendMessageA(hNewsResults, LB_ADDSTRING, 0, (LPARAM)item.c_str());
        }
        break;
    }

    case WM_DESTROY:
        api.setNewsWindow(NULL);
        hNewsListCombo = NULL;
        hNewsSymCombo  = NULL;
        hNewsResults   = NULL;
        newsSymEntries.clear();
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}