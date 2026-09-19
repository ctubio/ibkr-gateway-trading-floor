#pragma once
static const int windowEventsWidth = 260;
void StartEvents() { StartGenericWindow(EVENTS_CLASS_NAME, "Events", L"TWSAPIClientTradingFloor.Events", windowEventsWidth, 420); }

#define ID_EVENTS_LIST          8001

// Column indices, matching eventCols[] below.
enum EventColIdx { ECOL_TIME = 0, ECOL_TEXT };

// ── Column definitions ────────────────────────────────────────────────────────
struct EventCol { const char* header; int width; int fmt; };
static const EventCol eventCols[] = {
    { "Time",   65, LVCFMT_CENTER },
    { "Event", 175, LVCFMT_LEFT   },
};
static const int EVENT_COL_COUNT = (int)(sizeof(eventCols) / sizeof(eventCols[0]));

// ── Event log storage ─────────────────────────────────────────────────────────
// In-memory only (not persisted). Newest event at index 0 — same "insert at
// top" convention as the Time & Sales lists in market.h. Capped at
// EVENTS_MAX entries; once full, the oldest entry is dropped to make room.
// Backs the Events window's virtual (LVS_OWNERDATA) list.
struct EventEntry {
    std::string time;
    std::string text;
    COLORREF    color = 0;   // 0 = default theme text color
    int         conId = 0;   // 0 = no associated symbol (double-click no-ops)
    std::string symbol;
};

static const size_t EVENTS_MAX = 21;
static std::deque<EventEntry> eventsList;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void Events_LayoutPanel(HWND hWnd) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    HWND hList = GetDlgItem(hWnd, ID_EVENTS_LIST);
    if (hList)
        SetWindowPos(hList, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER | SWP_NOACTIVATE);
}

// Refreshes the (virtual) ListView's item count from eventsList and repaints.
static void Events_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_EVENTS_LIST);
    if (!hList) return;
    ListView_SetItemCountEx(hList, (int)eventsList.size(), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    InvalidateRect(hList, NULL, FALSE);
}

static void Events_AddEvent(const std::string& text, COLORREF color = 0, int conId = 0, const std::string& symbol = "") {
    time_t now = time(0);
    struct tm ltm = {};
    localtime_s(&ltm, &now);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", ltm.tm_hour, ltm.tm_min, ltm.tm_sec);

    eventsList.push_front(EventEntry{ std::string(buf), text, color, conId, symbol });
    while (eventsList.size() > EVENTS_MAX) eventsList.pop_back();

    HWND hWnd = FindWindowA(EVENTS_CLASS_NAME, NULL);
    if (hWnd && IsWindow(hWnd)) Events_Repopulate(hWnd);
}

static void Events_AddAlertEvent(int conId, const std::string& symbol, double price, bool isUp) {
    std::string text = (isUp ? "▲ " : "▼ ") + symbol + " at " + FormatFixed(price, 2);
    Events_AddEvent(text, isUp ? COINS_CLR_GREEN : COINS_CLR_RED, conId, symbol);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcEvents(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            DWORD lvStyle = WS_CHILD | WS_VISIBLE | WS_BORDER
                        | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER | LVS_OWNERDATA;
            HWND hList = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"SysListView32", L"",
                lvStyle,
                0, 0, 760, 420,
                hWnd, (HMENU)ID_EVENTS_LIST, hInst, NULL);

            SendMessage(hList, WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);
            SendMessage(ListView_GetHeader(hList), WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);
            SetWindowSubclass(hList, ListViewNoFlickerProc, 0, 0);

            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNW lvc = {};
            lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
            for (int i = 0; i < EVENT_COL_COUNT; ++i) {
                lvc.cx      = eventCols[i].width;
                //MultiByteToWideChar(CP_UTF8, 0, eventCols[i].header, -1, lvc.pszText, lvc.cchTextMax);
                wchar_t wHeader[64];
                MultiByteToWideChar(CP_UTF8, 0, eventCols[i].header, -1, wHeader, 64);
                lvc.pszText = wHeader;

                lvc.fmt     = eventCols[i].fmt;
                SendMessageW(hList, LVM_INSERTCOLUMNW, i, (LPARAM)&lvc);
            }

            api().addApiUpdateWindow(hWnd);
            Events_Repopulate(hWnd);
            break;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = windowEventsWidth;
            mmi->ptMaxTrackSize.x = windowEventsWidth;

            RECT workArea;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

            mmi->ptMaxSize.x = workArea.right - workArea.left;
            mmi->ptMaxSize.y = workArea.bottom - workArea.top;
            mmi->ptMaxPosition.x = workArea.left - workArea.left + ((mmi->ptMaxSize.x - mmi->ptMaxTrackSize.x)/2);
            mmi->ptMaxPosition.y = workArea.top - workArea.top;
            return DefWindowProc(hWnd, WM_GETMINMAXINFO, wParam, lParam);
        }

        case WM_SIZE: {
            Events_LayoutPanel(hWnd);
            break;
        }

        case WM_API_UPDATE: {
            Events_Repopulate(hWnd);
            break;
        }
        
        case WM_NOTIFYFORMAT:
            return NFR_UNICODE;

        case WM_NOTIFY: {
            NMHDR* hdr = (NMHDR*)lParam;
            if (hdr->idFrom != ID_EVENTS_LIST) break;

            if (hdr->code == LVN_ITEMCHANGED) {
                NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
                if ((nmlv->uChanged & LVIF_STATE) && (nmlv->uNewState & LVIS_SELECTED) &&
                    nmlv->iItem >= 0 && nmlv->iItem < (int)eventsList.size()) {
                    const EventEntry& ev = eventsList[nmlv->iItem];
                    api().updateDisplayGroup(ev.conId);
                    ListView_SetItemState(hdr->hwndFrom, nmlv->iItem, 0, LVIS_SELECTED);
                }
            }

            if (hdr->code == LVN_GETDISPINFOW) {
                NMLVDISPINFOW* pdi = (NMLVDISPINFOW*)lParam;
                if (pdi->item.iItem < 0 || pdi->item.iItem >= (int)eventsList.size()) return 0;
                const EventEntry& ev = eventsList[pdi->item.iItem];
                if (pdi->item.mask & LVIF_TEXT) {
                    const std::string& s = (pdi->item.iSubItem == ECOL_TIME) ? ev.time : ev.text;
                    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, pdi->item.pszText, pdi->item.cchTextMax);
                }
                return 0;
            }

            if (hdr->code == NM_DBLCLK) {
                if (!lockHotkeys) {
                    LPNMITEMACTIVATE act = (LPNMITEMACTIVATE)lParam;
                    int row = act->iItem;
                    if (row >= 0 && row < (int)eventsList.size()) {
                        const EventEntry& ev = eventsList[row];
                        if (ev.conId > 0 && !ev.symbol.empty())
                            StartMarket(ev.symbol, ev.conId);
                    }
                }
            }

            if (hdr->code == NM_CUSTOMDRAW) {
                NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
                switch (cd->nmcd.dwDrawStage) {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;

                    case CDDS_ITEMPREPAINT: {
                        cd->nmcd.uItemState &= ~CDIS_SELECTED;
                        if (darkMode) {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            cd->clrText   = DM_TEXT;
                        } else {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                            cd->clrText   = LM_TEXT;
                        }
                        size_t idx = (size_t)cd->nmcd.dwItemSpec;
                        if (idx < eventsList.size() && eventsList[idx].color != 0)
                            cd->clrText = eventsList[idx].color;
                        return CDRF_DODEFAULT;
                    }
                }
                break;
            }
            break;
        }

        case WM_DESTROY:
            api().removeApiUpdateWindow(hWnd);
            break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}