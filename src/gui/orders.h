#pragma once

void StartOrders() { StartGenericWindow(ORDERS_CLASS_NAME, "Orders", L"IBKRGatewayClient.Orders", 786, 240); }

#define ID_ORDERS_LIST          9003

static ListViewZoomData OrdersZoomData = { NULL, 14, "OrdersListZoom" };

// ── Column definitions ────────────────────────────────────────────────────────

struct OrderCol { const char* header; int width; int fmt; };
static const OrderCol orderCols[] = {
    { "Time",          80,  LVCFMT_LEFT  },
    { "Symbol",        80,  LVCFMT_LEFT  },
    { "Type",         100,  LVCFMT_LEFT  },
    { "Price",         85,  LVCFMT_RIGHT },
    { "Avg Fill",      85,  LVCFMT_RIGHT },
    { "Filled / Qty", 100,  LVCFMT_RIGHT },
    { "Status",       135, LVCFMT_RIGHT  },
    { "Exchange",     100,  LVCFMT_LEFT  },
};
static const int ORDER_COL_COUNT = (int)(sizeof(orderCols) / sizeof(orderCols[0]));

// ── Helpers ───────────────────────────────────────────────────────────────────

// Returns a color for the status text (used in NM_CUSTOMDRAW).
static COLORREF Orders_StatusColor(const std::string& status, bool dark) {
    if (status == "Filled")                           return RGB(80, 200, 120);
    if (status == "Partially Filled")                 return RGB(255, 200, 60);
    if (status == "Cancelled" || status == "Inactive")
        return dark ? RGB(130, 130, 130) : RGB(160, 160, 160);
    if (status == "Submitted" || status == "PreSubmitted")
        return dark ? RGB(120, 180, 255) : RGB(30, 100, 220);
    return dark ? DM_TEXT : LM_TEXT;
}

// Rebuilds the entire ListView from the current snapshot.
static void Orders_Repopulate(HWND hList) {
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    auto orders = api.getOrdersSorted();

    for (int i = 0; i < (int)orders.size(); ++i) {
        const auto& o = orders[i];
        char buf[64];

        int col = 0;
        LVITEMA lvi  = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = i;
        lvi.iSubItem = col++;
        lvi.lParam   = (LPARAM)i;         // store row index for custom draw
        lvi.pszText  = (LPSTR)o.time.c_str();
        ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, i, col++, (LPSTR)o.symbol.c_str());

        ListView_SetItemText(hList, i, col++, (LPSTR)(o.orderType + " " + o.action).c_str());

        if (o.price > 0)
            snprintf(buf, sizeof(buf), "%.2f", o.price);
        else
            snprintf(buf, sizeof(buf), "MKT");
        ListView_SetItemText(hList, i, col++, buf);

        if (o.avgFillPx > 0)
            snprintf(buf, sizeof(buf), "%.2f", o.avgFillPx);
        else
            snprintf(buf, sizeof(buf), "--");
        ListView_SetItemText(hList, i, col++, buf);

        snprintf(buf, sizeof(buf), "%.0f / %.0f", o.filledQty, o.totalQty);
        ListView_SetItemText(hList, i, col++, buf);

        ListView_SetItemText(hList, i, col++, (LPSTR)o.status.c_str());

        ListView_SetItemText(hList, i, col++, (LPSTR)o.exchange.c_str());
    }

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

// ── Window procedure ──────────────────────────────────────────────────────────

LRESULT CALLBACK WndProcOrders(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            DWORD lvStyle = WS_CHILD | WS_VISIBLE | WS_BORDER
                        | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER;
            HWND hList = CreateWindowExA(
                WS_EX_CLIENTEDGE, "SysListView32", "",
                lvStyle,
                0, 0, 760, 420,
                hWnd, (HMENU)ID_ORDERS_LIST, hInst, NULL);

            OrdersZoomData.fontSize = (int)Settings_Load(OrdersZoomData.settingKey, OrdersZoomData.fontSize);
            ApplyListViewFont(hList, OrdersZoomData.hFont, OrdersZoomData.fontSize);
            SetWindowSubclass(hList, ListViewZoomProc, 0, (DWORD_PTR)&OrdersZoomData);

            // Full-row selection + double-buffer to reduce flicker
            DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
            if (Settings_DarkMode())
                exStyle |= LVS_EX_GRIDLINES; // grid lines are more readable in dark mode
            ListView_SetExtendedListViewStyle(hList, exStyle);

            LVCOLUMNA lvc = {};
            lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
            for (int i = 0; i < ORDER_COL_COUNT; ++i) {
                lvc.cx      = orderCols[i].width;
                lvc.pszText = (LPSTR)orderCols[i].header;
                lvc.fmt     = orderCols[i].fmt;
                ListView_InsertColumn(hList, i, &lvc);
            }

            api.setOrdersWindow(hWnd);
            api.addApiUpdateWindow(hWnd);
            break;
        }

        case WM_SIZE: {
            HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
            if (!hList) return 0;
            RECT rc;
            GetClientRect(hWnd, &rc);
            MoveWindow(hList, 0, 0, rc.right, rc.bottom, TRUE);
            break;
        }

        case WM_ORDERS_UPDATE: {
            HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
            if (hList) Orders_Repopulate(hList);
            break;
        }

        // ── Dark mode: paint the ListView background and items ────────────────────
        case WM_NOTIFY: {
            NMHDR* hdr = (NMHDR*)lParam;
            if (hdr->idFrom != ID_ORDERS_LIST) break;

            if (hdr->code == NM_CUSTOMDRAW) {
                NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
                bool dark = Settings_DarkMode();

                switch (cd->nmcd.dwDrawStage) {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;

                    case CDDS_ITEMPREPAINT:
                        if (dark) {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            cd->clrText   = DM_TEXT;
                        } else {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                            cd->clrText   = LM_TEXT;
                        }
                        return CDRF_NOTIFYSUBITEMDRAW;

                    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                        if (cd->iSubItem == 0 || cd->iSubItem == 6) { // Status column — color by state
                            // Get status text
                            char statusBuf[64] = {};
                            HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                            ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, 6, statusBuf, sizeof(statusBuf));
                            cd->clrText = Orders_StatusColor(statusBuf, dark);
                            if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            return CDRF_NEWFONT;
                        }
                        if (cd->iSubItem == 2) { // Action — BUY green, SELL red
                            char buf[16] = {};
                            HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                            ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, 2, buf, sizeof(buf));
                            size_t len = strlen(buf);
                            if (len >= 3 && strcmp(buf + len - 3, "BUY") == 0)
                                cd->clrText = RGB(80, 200, 120);
                            else if (len >= 4 && strcmp(buf + len - 4, "SELL") == 0)
                                cd->clrText = RGB(220, 80, 80);
                            if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            return CDRF_NEWFONT;
                        }
                        return CDRF_DODEFAULT;
                    }
                }
            }
            break;
        }


        case WM_API_UPDATE: {
            HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
            if (hList) {
                if (api.isMarketDataConnected() && api.isTradingConnected()) {
                    Orders_Repopulate(hList);
                } else {
                    ListView_DeleteAllItems(hList);
                }
            }
            break;
        }
        
        case WM_DESTROY:
            api.unsetOrdersWindow();
            api.removeApiUpdateWindow(hWnd);
            if (OrdersZoomData.hFont) {
                DeleteObject(OrdersZoomData.hFont);
            }
            break;
    }
    
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}