#pragma once
static const int windowEventsWidth = 260;
void StartEvents() { StartGenericWindow(EVENTS_CLASS_NAME, "Events", L"TWSAPIClientTradingFloor.Events", windowEventsWidth, 420); }

#define ID_EVENTS_LIST          8001

// Column indices, matching eventCols[] event below.
enum EventColIdx { ECOL_SIDE = 0, ECOL_SYMBOL, ECOL_QUOTE, ECOL_STATUS };

// ── Column definitions ────────────────────────────────────────────────────────

struct EventCol { const char* header; int width; int fmt; };
static const EventCol eventCols[] = {
    { "Time",          70,  LVCFMT_CENTER},
    { "Event",        170,  LVCFMT_LEFT  },
};
static const int EVENT_COL_COUNT = (int)(sizeof(eventCols) / sizeof(eventCols[0]));

// ── Helpers ───────────────────────────────────────────────────────────────────

// Resize ListView and show/hide the edit panel controls to fit the window.
static void Events_LayoutPanel(HWND hWnd) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right;
    int h = rc.bottom;

    HWND hList          = GetDlgItem(hWnd, ID_EVENTS_LIST);

    // Count controls for DeferWindowPos
    int ctrlCount = 0;
    if (hList) ctrlCount++;

    HDWP hdwp = BeginDeferWindowPos(ctrlCount);
    if (!hdwp) return;

    // ListView
    if (hList) {
        hdwp = DeferWindowPos(hdwp, hList, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    EndDeferWindowPos(hdwp);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcEvents(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            DWORD lvStyle = WS_CHILD | WS_VISIBLE | WS_BORDER
                        | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
            HWND hList = CreateWindowExA(
                WS_EX_CLIENTEDGE, "SysListView32", "",
                lvStyle,
                0, 0, 760, 420,
                hWnd, (HMENU)ID_EVENTS_LIST, hInst, NULL);

            SendMessage(hList, WM_SETFONT, (WPARAM)hFont14pt.get(), TRUE);
            SendMessage(ListView_GetHeader(hList), WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);
            SetWindowSubclass(hList, ListViewNoFlickerProc, 0, 0);
            SetWindowSubclass(hList, OrdersList_SubclassProc, 1, 0);

            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNA lvc = {};
            lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
            for (int i = 0; i < EVENT_COL_COUNT; ++i) {
                lvc.cx      = eventCols[i].width;
                lvc.pszText = (LPSTR)eventCols[i].header;
                lvc.fmt     = eventCols[i].fmt;
                ListView_InsertColumn(hList, i, &lvc);
                if (i == 0) {
                    LVCOLUMN lvcUpdate = { 0 };
                    lvcUpdate.mask = LVCF_FMT;
                    lvcUpdate.fmt = eventCols[i].fmt;
                    ListView_SetColumn(hList, i, &lvcUpdate);
                }
            }

            api().addApiUpdateWindow(hWnd);  
            // Events_Repopulate(hWnd);
            break;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = windowEventsWidth;
            mmi->ptMaxTrackSize.x = windowEventsWidth;
            
            RECT workArea;
            SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

            // Force the maximized size and position to respect the work area
            mmi->ptMaxSize.x = workArea.right - workArea.left;
            mmi->ptMaxSize.y = workArea.bottom - workArea.top;
            mmi->ptMaxPosition.x = workArea.left - workArea.left + ((mmi->ptMaxSize.x - mmi->ptMaxTrackSize.x)/2); // relative to monitor, or workArea.left
            mmi->ptMaxPosition.y = workArea.top - workArea.top;
            return DefWindowProc(hWnd, WM_GETMINMAXINFO, wParam, lParam);
        }
        
        case WM_SIZE: {
            Events_LayoutPanel(hWnd);
            break;
        }

        case WM_API_UPDATE: {
            if (api().isMarketDataConnected() && api().isTradingConnected()) {
                // Events_Repopulate(hWnd);
            } else {
                HWND hList = GetDlgItem(hWnd, ID_EVENTS_LIST);
                if (hList) {
                    ListView_DeleteAllItems(hList);
                    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
                    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
                }
            }
            break;
        }
        

        case WM_DESTROY:
            api().removeApiUpdateWindow(hWnd);
            break;
    }
    
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}