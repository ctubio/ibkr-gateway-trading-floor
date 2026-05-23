#pragma once

void startNews() { startGenericWindow(NEWS_CLASS_NAME, "News", L"IBKRGatewayClient.News", 420, 500); }

void startNewsArticle() { startGenericWindow(NEWS_ARTICLE_CLASS_NAME, "News Article", L"IBKRGatewayClient.NewsArticle", 600, 500); }

#define ID_NEWS_LIST_COMBO   6001
#define ID_NEWS_SYM_COMBO    6002
#define ID_NEWS_RESULTS_LIST 6003

static HWND hNewsArticleEdit = NULL;
static HWND hNewsListCombo = NULL;
static HWND hNewsSymCombo  = NULL;
static HWND hNewsResults   = NULL;
static std::vector<std::string> newsSymEntries;

static const int NEWS_COMBO_H   = 24;
static const int NEWS_COMBO_GAP = 8;
static const int NEWS_SELECTOR_H = 8 + NEWS_COMBO_H + 8;

std::string articleBuffer;

// Convert basic HTML to RTF for RichEdit display
static std::string ConvertHtmlToRtf(const std::string& html) {
    std::string text = html;
    std::string::size_type pos = 0;

    // 1. ESCAPE RTF CHARACTERS FIRST (Crucial Fix)
    // We must escape literal \, {, and } in the raw text BEFORE adding RTF tags.
    pos = 0;
    while ((pos = text.find("\\", pos)) != std::string::npos)
        text.replace(pos, 1, "\\\\"), pos += 2;
    pos = 0;
    while ((pos = text.find("{", pos)) != std::string::npos)
        text.replace(pos, 1, "\\{"), pos += 2;
    pos = 0;
    while ((pos = text.find("}", pos)) != std::string::npos)
        text.replace(pos, 1, "\\}"), pos += 2;

    // 2. Normalize manual text newlines so they don't break RTF layout
    pos = 0;
    while ((pos = text.find("\n", pos)) != std::string::npos)
        text.replace(pos, 1, " "), pos += 1;

    // 3. Decode HTML entities
    pos = 0;
    while ((pos = text.find("&apos;", pos)) != std::string::npos)
        text.replace(pos, 6, "'"), pos += 1;
    pos = 0;
    while ((pos = text.find("&quot;", pos)) != std::string::npos)
        text.replace(pos, 6, "\""), pos += 1;
    pos = 0;
    while ((pos = text.find("&amp;", pos)) != std::string::npos)
        text.replace(pos, 5, "&"), pos += 1;
    pos = 0;
    while ((pos = text.find("&lt;", pos)) != std::string::npos)
        text.replace(pos, 4, "<"), pos += 1;
    pos = 0;
    while ((pos = text.find("&gt;", pos)) != std::string::npos)
        text.replace(pos, 4, ">"), pos += 1;

    // 4. Convert structural HTML breaks to genuine RTF paragraphs/lines
    // Handle line breaks
    pos = 0;
    while ((pos = text.find("<br>", pos)) != std::string::npos)
        text.replace(pos, 4, "\\line "), pos += 6;
    pos = 0;
    while ((pos = text.find("<br/>", pos)) != std::string::npos)
        text.replace(pos, 5, "\\line "), pos += 6;

    // Handle paragraphs
    pos = 0;
    while ((pos = text.find("<p>", pos)) != std::string::npos)
        text.replace(pos, 3, ""), pos += 0;
    pos = 0;
    while ((pos = text.find("</p>", pos)) != std::string::npos)
        text.replace(pos, 4, "\\par\\par "), pos += 9; // Two breaks look cleaner for paragraphs

    // 5. Clean up any remaining/unsupported HTML tags
    pos = 0;
    while ((pos = text.find("<", pos)) != std::string::npos) {
        size_t end = text.find(">", pos);
        if (end != std::string::npos)
            text.erase(pos, end - pos + 1);
        else
            break;
    }

    // 6. Wrap everything cleanly inside the RTF payload header
    std::string result = "{\\rtf1\\ansi\\ansicpg1252\\deff0{\\fonttbl{\\f0\\fnil Segoe UI;}}{\\colortbl;\\red0\\green0\\blue0;}\n\\viewkind4\\uc1\\pard\\f0\\fs20 ";
    result += text;
    result += "}";
    
    return result;
}

void FlushArticleBuffer() {
    LogDebug("Flushing article buffer. Length: " + std::to_string(articleBuffer.size()));
    LogDebug(!hNewsArticleEdit ? "hNewsArticleEdit is null" : (!IsWindow(hNewsArticleEdit) ? "hNewsArticleEdit is not a valid window" : "hNewsArticleEdit is valid"));
    if (!hNewsArticleEdit || !IsWindow(hNewsArticleEdit)) return;
    
    // Convert HTML to RTF
    std::string rtf = ConvertHtmlToRtf(articleBuffer);
    LogDebug("Flushing article buffer to edit control. Original length: " + std::to_string(articleBuffer.size()) +
             ", RTF length: " + std::to_string(rtf.size()));
    // Use SETTEXTEX to load RTF
    SETTEXTEX st = { ST_DEFAULT, CP_UTF8 };
    SendMessageA(hNewsArticleEdit, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)rtf.c_str());
}

LRESULT CALLBACK WndProcNewsArticle(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            // RICHEDIT20A lives in riched20.dll — must be loaded before CreateWindowExA.
            LoadLibraryA("riched20.dll");

            hNewsArticleEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "RICHEDIT20A", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                0, 0, 0, 0, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hNewsArticleEdit, EM_SETTARGETDEVICE, 0, 0);
            RECT rc;
            GetClientRect(hWnd, &rc);
            SetWindowPos(hNewsArticleEdit, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
            FlushArticleBuffer(); // ← show all buffered messages
            break;
        }
        case WM_SIZE: {
            if (hNewsArticleEdit && IsWindow(hNewsArticleEdit)) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                SetWindowPos(hNewsArticleEdit, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
            }
            break;
        }
    }
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}
// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string News_GetSelectedList() {
    int sel = (int)SendMessage(hNewsListCombo, CB_GETCURSEL, 0, 0);
    if (sel == CB_ERR) return "";
    int len = (int)SendMessage(hNewsListCombo, CB_GETLBTEXTLEN, sel, 0);
    if (len <= 0) return "";
    std::string name(len + 1, '\0');
    SendMessageA(hNewsListCombo, CB_GETLBTEXT, sel, (LPARAM)name.data());
    name.resize(len);
    return name;
}

static bool News_LoadListCombo(const std::string& savedList = "") {
    SendMessage(hNewsListCombo, CB_RESETCONTENT, 0, 0);
    Book_LoadAllLists(hNewsListCombo);
    if ((int)SendMessage(hNewsListCombo, CB_GETCOUNT, 0, 0) == 0) return false;
    int idx = CB_ERR;
    if (!savedList.empty())
        idx = (int)SendMessageA(hNewsListCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)savedList.c_str());
    SendMessage(hNewsListCombo, CB_SETCURSEL, idx == CB_ERR ? 0 : idx, 0);
    return true;
}

static bool News_LoadSymbolCombo(const std::string& savedEntry = "") {
    SendMessage(hNewsSymCombo, CB_RESETCONTENT, 0, 0);
    newsSymEntries.clear();
    std::string listName = News_GetSelectedList();
    if (listName.empty()) return false;
    for (const auto& full : Book_ReadListEntries(listName.c_str())) {
        newsSymEntries.push_back(full);
        SendMessageA(hNewsSymCombo, CB_ADDSTRING, 0, (LPARAM)Book_DisplayLabel(full).c_str());
    }
    if (newsSymEntries.empty()) return false;
    int idx = CB_ERR;
    for (int i = 0; i < (int)newsSymEntries.size(); ++i)
        if (newsSymEntries[i] == savedEntry) { idx = i; break; }
    SendMessage(hNewsSymCombo, CB_SETCURSEL, idx == CB_ERR ? 0 : idx, 0);
    return true;
}

// ── Layout ────────────────────────────────────────────────────────────────────

static void News_Layout(HWND hWnd) {
    if (!hNewsResults) return;
    RECT rc; GetClientRect(hWnd, &rc);
    const int m = 8;

    bool combosVisible = hNewsListCombo && IsWindowVisible(hNewsListCombo);

    if (combosVisible) {
        int availW = rc.right - m * 2 - NEWS_COMBO_GAP;
        int eachW  = availW / 2;
        int comboY = rc.bottom - NEWS_COMBO_H - m;
        
        // Position ListView at top, taking up most of the space
        MoveWindow(hNewsResults, 0, 0, rc.right, comboY - m, TRUE);
        
        // Position combos at bottom
        SetWindowPos(hNewsListCombo, NULL, m, comboY, eachW, NEWS_COMBO_H,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(hNewsSymCombo,  NULL, m + eachW + NEWS_COMBO_GAP, comboY, eachW, NEWS_COMBO_H,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        MoveWindow(hNewsResults, 0, 0, rc.right, rc.bottom, TRUE);
    }
}

// ── News request ──────────────────────────────────────────────────────────────

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

    if (hNewsResults) {
        int count = ListView_GetItemCount(hNewsResults);
        for (int i = 0; i < count; ++i) {
            LVITEMA lvi = {};
            lvi.mask  = LVIF_PARAM;
            lvi.iItem = i;
            SendMessageA(hNewsResults, LVM_GETITEMA, 0, (LPARAM)&lvi);
            delete reinterpret_cast<std::string*>(lvi.lParam);
        }
        ListView_DeleteAllItems(hNewsResults);
    }
    api.reqNewsForSymbol(std::stoi(conIdStr), symbol);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcNews(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
        const int m = 8;

        // Results ListView — takes up most of the space initially
        hNewsResults = CreateWindowExA(
            WS_EX_CLIENTEDGE, "SysListView32", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER |
            LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
            0, 0, 400, 450,
            hWnd, (HMENU)ID_NEWS_RESULTS_LIST, hInst, NULL);

        // Combos at the bottom — initially hidden until focused
        hNewsListCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            m, 460, 180, 200,
            hWnd, (HMENU)ID_NEWS_LIST_COMBO, hInst, NULL);

        hNewsSymCombo = CreateWindowA("COMBOBOX", NULL,
            WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
            m + 180 + NEWS_COMBO_GAP, 460, 180, 200,
            hWnd, (HMENU)ID_NEWS_SYM_COMBO, hInst, NULL);

        DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
        if (Settings_DarkMode()) exStyle |= LVS_EX_GRIDLINES;
        ListView_SetExtendedListViewStyle(hNewsResults, exStyle);

        // Add columns for each NewsTickEntry property
        LVCOLUMNA lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;

        lvc.cx = 90;
        lvc.pszText = (LPSTR)"Time";
        ListView_InsertColumn(hNewsResults, 0, &lvc);

        lvc.cx = 100;
        lvc.pszText = (LPSTR)"Provider";
        ListView_InsertColumn(hNewsResults, 1, &lvc);

        lvc.cx = 350;
        lvc.pszText = (LPSTR)"Headline";
        ListView_InsertColumn(hNewsResults, 2, &lvc);

        api.setNewsWindow(hWnd);
        SetWindowTextA(hWnd, "News");

        std::string lastList  = Settings_LoadString("LastNewsList");
        std::string lastEntry = Settings_LoadString("LastNewsEntry");
        News_LoadListCombo(lastList);
        if (News_LoadSymbolCombo(lastEntry)) {
            int sel = (int)SendMessage(hNewsSymCombo, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR && sel < (int)newsSymEntries.size())
                News_RequestForSymbol(newsSymEntries[sel]);
        }
        break;
    }

    case WM_SIZE:
        News_Layout(hWnd);
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
            ShowWindow(hNewsListCombo, SW_SHOW);
            ShowWindow(hNewsSymCombo,  SW_SHOW);
        } else {
            ShowWindow(hNewsListCombo, SW_HIDE);
            ShowWindow(hNewsSymCombo,  SW_HIDE);
        }
        News_Layout(hWnd);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_NEWS_LIST_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            Settings_SaveString("LastNewsList", News_GetSelectedList());
            News_LoadSymbolCombo();
            if (!newsSymEntries.empty()) News_RequestForSymbol(newsSymEntries[0]);
        }
        if (LOWORD(wParam) == ID_NEWS_SYM_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = (int)SendMessage(hNewsSymCombo, CB_GETCURSEL, 0, 0);
            if (sel != CB_ERR && sel < (int)newsSymEntries.size())
                News_RequestForSymbol(newsSymEntries[sel]);
        }
        break;

    case WM_NEWS_RESULTS: {
        auto* news = reinterpret_cast<TradingAPI::NewsTickEntry*>(lParam);
        if (!news) break;
        if (!hNewsResults) { delete news; break; }

        // Store "providerCode|articleId" as the item's lParam for double-click retrieval.
        // We heap-allocate it; it lives as long as the row does.
        auto* rowId = new std::string(news->providerCode + "|" + news->articleId);

        LVITEMA lvi  = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = 0;
        lvi.lParam   = (LPARAM)rowId;
        lvi.pszText  = (LPSTR)news->timeStamp.c_str();
        int idx = (int)SendMessageA(hNewsResults, LVM_INSERTITEMA, 0, (LPARAM)&lvi);

        ListView_SetItemText(hNewsResults, idx, 1, (LPSTR)news->providerCode.c_str());
        ListView_SetItemText(hNewsResults, idx, 2, (LPSTR)news->headline.c_str());

        delete news;
        break;
    }

    case WM_NEWS_ARTICLE: {
        auto* body = reinterpret_cast<std::string*>(lParam);
        if (!body) break;
        int articleType = (int)wParam; // 0 = text, 1 = binary/PDF

        if (articleType == 1) {
            MessageBoxA(hWnd, "This article is in binary/PDF format and cannot be displayed inline.",
                        "Article", MB_OK | MB_ICONINFORMATION);
        } else if (body->empty()) {
            MessageBoxA(hWnd, "No article body returned (provider may require a separate subscription).",
                        "Article", MB_OK | MB_ICONINFORMATION);
        } else {
            // Simple popup edit box — read-only, scrollable, word-wrapped.
            // Use WS_OVERLAPPEDWINDOW for full window capabilities (draggable, resizable, closable)
            articleBuffer = std::string(*body);
            startNewsArticle();
        }
        delete body;
        break;
    }

    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom == ID_NEWS_RESULTS_LIST) {
            if (hdr->code == NM_DBLCLK) {
                NMITEMACTIVATE* nm = (NMITEMACTIVATE*)lParam;
                if (nm->iItem >= 0) {
                    LVITEMA lvi = {};
                    lvi.mask   = LVIF_PARAM;
                    lvi.iItem  = nm->iItem;
                    SendMessageA(hNewsResults, LVM_GETITEMA, 0, (LPARAM)&lvi);
                    auto* rowId = reinterpret_cast<std::string*>(lvi.lParam);
                    if (rowId) {
                        auto sep = rowId->find('|');
                        if (sep != std::string::npos) {
                            std::string provider  = rowId->substr(0, sep);
                            std::string articleId = rowId->substr(sep + 1);
                            api.reqNewsArticle(provider, articleId);
                        }
                    }
                }
                return 0;
            }
            if (hdr->code == NM_CUSTOMDRAW) {
                NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
                if (!Settings_DarkMode()) break;
                switch (cd->nmcd.dwDrawStage) {
                    case CDDS_PREPAINT:     return CDRF_NOTIFYITEMDRAW;
                    case CDDS_ITEMPREPAINT:
                        cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        cd->clrText   = DM_TEXT;
                        return CDRF_DODEFAULT;
                }
            }
        }
        break;
    }

    case WM_DESTROY:
        // Free the heap-allocated rowId strings stored as item lParams.
        if (hNewsResults) {
            int count = ListView_GetItemCount(hNewsResults);
            for (int i = 0; i < count; ++i) {
                LVITEMA lvi = {};
                lvi.mask  = LVIF_PARAM;
                lvi.iItem = i;
                SendMessageA(hNewsResults, LVM_GETITEMA, 0, (LPARAM)&lvi);
                delete reinterpret_cast<std::string*>(lvi.lParam);
            }
        }
        api.setNewsWindow(NULL);
        hNewsListCombo = NULL;
        hNewsSymCombo  = NULL;
        hNewsResults   = NULL;
        newsSymEntries.clear();
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}