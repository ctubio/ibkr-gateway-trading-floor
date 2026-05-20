#pragma once

void startNews() { startGenericWindow(NEWS_CLASS_NAME, "News", L"IBKRGatewayClient.News", 420, 500); }

#define ID_NEWS_EDIT         6001
#define ID_NEWS_LIST         6002
#define ID_NEWS_AUTOCOMPLETE 6003
#define TIMER_NEWS_DROPDOWN  98

static HWND hNewsEdit        = NULL;
static HWND hNewsList        = NULL;
static HWND hNewsAutoComplete = NULL;
static bool suppressNewsSearch = false;
static std::vector<std::string> newsCurrentResults;
static std::string newsPendingFullEntry;

// ─── AutoComplete (same pattern as book.h) ────────────────────────────────────

void News_UpdateAutoComplete(HWND hWnd, const std::vector<std::string>& results) {
    EnableWindow(hNewsAutoComplete, TRUE);
    newsCurrentResults = results;

    if (results.empty()) {
        ShowWindow(hNewsAutoComplete, SW_HIDE);
        return;
    }

    SendMessage(hNewsAutoComplete, LB_RESETCONTENT, 0, 0);
    for (const auto& s : results)
        SendMessageA(hNewsAutoComplete, LB_ADDSTRING, 0, (LPARAM)Book_DisplayLabel(s).c_str());

    RECT r;
    GetWindowRect(hNewsEdit, &r);
    int itemHeight = SendMessage(hNewsAutoComplete, LB_GETITEMHEIGHT, 0, 0);
    int visItems = std::min((int)results.size(), 6);

    SetWindowPos(hNewsAutoComplete, HWND_TOPMOST,
        r.left, r.bottom, r.right - r.left, itemHeight * visItems + 4,
        SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

LRESULT CALLBACK NewsAutoCompleteSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                               UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_LBUTTONUP) {
        int sel = SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)newsCurrentResults.size()) {
            newsPendingFullEntry = newsCurrentResults[sel];
            std::string label = Book_DisplayLabel(newsPendingFullEntry);
            suppressNewsSearch = true;
            SetWindowTextA(hNewsEdit, label.c_str());
            suppressNewsSearch = false;
            ShowWindow(hWnd, SW_HIDE);
            KillTimer(GetParent(hWnd), TIMER_NEWS_DROPDOWN);
            SetFocus(hNewsEdit);
            SendMessage(hNewsEdit, EM_SETSEL, label.size(), label.size());
            // Trigger news request for selected symbol
            SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(ID_NEWS_EDIT, 0), 0);
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, NewsAutoCompleteSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK NewsEditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                       UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_KEYDOWN) {
        bool acVisible = IsWindowVisible(hNewsAutoComplete);

        if (wParam == VK_DOWN && acVisible) {
            int count = SendMessage(hNewsAutoComplete, LB_GETCOUNT, 0, 0);
            int sel   = SendMessage(hNewsAutoComplete, LB_GETCURSEL, 0, 0);
            if (sel == LB_ERR) sel = -1;
            if (sel + 1 < count)
                SendMessage(hNewsAutoComplete, LB_SETCURSEL, sel + 1, 0);
            return 0;
        }
        if (wParam == VK_UP && acVisible) {
            int sel = SendMessage(hNewsAutoComplete, LB_GETCURSEL, 0, 0);
            if (sel > 0)
                SendMessage(hNewsAutoComplete, LB_SETCURSEL, sel - 1, 0);
            return 0;
        }
        if (wParam == VK_RETURN && acVisible) {
            int sel = SendMessage(hNewsAutoComplete, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR && sel < (int)newsCurrentResults.size()) {
                newsPendingFullEntry = newsCurrentResults[sel];
                std::string label = Book_DisplayLabel(newsPendingFullEntry);
                suppressNewsSearch = true;
                SetWindowTextA(hNewsEdit, label.c_str());
                suppressNewsSearch = false;
                ShowWindow(hNewsAutoComplete, SW_HIDE);
                SendMessage(hNewsEdit, EM_SETSEL, label.size(), label.size());
                // Request news for selected symbol
                SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(ID_NEWS_EDIT, 0), 0);
            }
            return 0;
        }
        if (wParam == VK_ESCAPE && acVisible) {
            ShowWindow(hNewsAutoComplete, SW_HIDE);
            return 0;
        }
    }
    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hWnd, NewsEditSubclassProc, uIdSubclass);
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

// ─── News request helper ──────────────────────────────────────────────────────

void News_RequestForSymbol(const std::string& fullEntry) {
    auto firstDot = fullEntry.find('.');
    if (firstDot == std::string::npos) return;
    std::string conIdStr = fullEntry.substr(0, firstDot);
    std::string rest     = fullEntry.substr(firstDot + 1);
    auto secondDot       = rest.find('.');
    std::string symbol   = (secondDot != std::string::npos) ? rest.substr(0, secondDot) : rest;

    Settings_SaveString("LastNewsSymbol", fullEntry); // ← save full entry

    LogDebug("News request for: " + symbol + " conId: " + conIdStr);

    HWND newsWnd = g_AppWindows[NEWS_CLASS_NAME];
    if (newsWnd) SetWindowTextA(newsWnd, ("News: " + symbol).c_str());

    if (hNewsList) SendMessage(hNewsList, LB_RESETCONTENT, 0, 0);
    api.reqNewsForSymbol(std::stoi(conIdStr), symbol);
}

// ─── Window Procedure ─────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcNews(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        int margin = 8;

        // Symbol search edit
        CreateWindowA("STATIC", "Symbol:",
            WS_CHILD | WS_VISIBLE,
            margin, margin + 4, 55, 16,
            hWnd, NULL, hInst, NULL);

        hNewsEdit = CreateWindowA("EDIT", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_UPPERCASE,
            margin + 60, margin, 330, 24,
            hWnd, (HMENU)ID_NEWS_EDIT, hInst, NULL);

        // News list
        hNewsList = CreateWindowA("LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            margin, margin + 32, 400 - margin * 2, 430,
            hWnd, (HMENU)ID_NEWS_LIST, hInst, NULL);

        // Autocomplete popup (same as book.h)
        hNewsAutoComplete = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            "LISTBOX", NULL,
            WS_POPUP | WS_BORDER | LBS_NOTIFY,
            0, 0, 180, 100,
            hWnd, NULL, hInst, NULL);
        SetWindowSubclass(hNewsAutoComplete, NewsAutoCompleteSubclassProc, 1, 0);
        ShowWindow(hNewsAutoComplete, SW_HIDE);

        SetWindowSubclass(hNewsEdit, NewsEditSubclassProc, 2, 0);
        api.setNewsWindow(hWnd);
        SetWindowTextA(hWnd, "News: empty");

        std::string lastEntry = Settings_LoadString("LastNewsSymbol");
        if (!lastEntry.empty()) {
            std::string label = Book_DisplayLabel(lastEntry);
            suppressNewsSearch = true;
            SetWindowTextA(hNewsEdit, label.c_str());
            suppressNewsSearch = false;
            newsPendingFullEntry = lastEntry;
            News_RequestForSymbol(lastEntry);
            newsPendingFullEntry = "";
        }
        break;
    }

    case WM_SIZE: {
        if (hNewsList) {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int margin = 8;
            MoveWindow(hNewsList, margin, margin + 32, rc.right - margin * 2, rc.bottom - (margin + 32), TRUE);
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_NEWS_DROPDOWN) {
            KillTimer(hWnd, TIMER_NEWS_DROPDOWN);
            ShowWindow(hNewsAutoComplete, SW_HIDE);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_NEWS_EDIT) {
            // Triggered after symbol selected — request news
            if (!newsPendingFullEntry.empty()) {
                News_RequestForSymbol(newsPendingFullEntry);
                newsPendingFullEntry = "";
            }
        }
        if (LOWORD(wParam) == ID_NEWS_EDIT && HIWORD(wParam) == EN_CHANGE) {
            if (suppressNewsSearch) break;
            newsPendingFullEntry = "";
            char text[256] = {};
            GetWindowTextA(hNewsEdit, text, sizeof(text));

            std::string query = text;
            auto dot = query.find('.');
            if (dot != std::string::npos) query = query.substr(0, dot);

            if (query.size() >= 1) {
                if (!api.isConnected()) {
                    SendMessage(hNewsAutoComplete, LB_RESETCONTENT, 0, 0);
                    SendMessageA(hNewsAutoComplete, LB_ADDSTRING, 0, (LPARAM)"Gateway is disconnected!");
                    RECT r;
                    GetWindowRect(hNewsEdit, &r);
                    int itemHeight = SendMessage(hNewsAutoComplete, LB_GETITEMHEIGHT, 0, 0);
                    SetWindowPos(hNewsAutoComplete, HWND_TOP,
                        r.left, r.bottom, r.right - r.left, itemHeight + 4,
                        SWP_SHOWWINDOW | SWP_NOACTIVATE);
                    EnableWindow(hNewsAutoComplete, FALSE);
                } else {
                    api.searchSymbols(query);
                }
            } else {
                ShowWindow(hNewsAutoComplete, SW_HIDE);
            }
        }
        if (LOWORD(wParam) == ID_NEWS_EDIT && HIWORD(wParam) == EN_KILLFOCUS) {
            SetTimer(hWnd, TIMER_NEWS_DROPDOWN, 150, NULL);
        }
        break;

    case WM_SYMBOL_RESULTS: {
        // Filter results same as book.h
        char text[256] = {};
        GetWindowTextA(hNewsEdit, text, sizeof(text));
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
            News_UpdateAutoComplete(hWnd, filtered);
        } else {
            News_UpdateAutoComplete(hWnd, results);
        }
        break;
    }

    case WM_NEWS_RESULTS: {
        auto news = api.getNewsResults();
        if (hNewsList) {
            SendMessage(hNewsList, LB_RESETCONTENT, 0, 0);
            for (const auto& item : news) {
                SendMessageA(hNewsList, LB_ADDSTRING, 0, (LPARAM)item.c_str());
            }
        }
        break;
    }

    case WM_DESTROY:
        api.setNewsWindow(NULL);
        hNewsEdit         = NULL;
        hNewsList         = NULL;
        hNewsAutoComplete = NULL;
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}