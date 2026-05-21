#pragma once

void startOrders() { startGenericWindow(ORDERS_CLASS_NAME, "Orders", L"IBKRGatewayClient.Orders", 800, 240); }

// ── Column definitions ────────────────────────────────────────────────────────

struct OrderCol { const char* header; int width; };
static const OrderCol orderCols[] = {
    { "Time",       80  },
    { "Symbol",     90  },
    { "Action",     55  },
    { "Type",       55  },
    { "Qty",        65  },
    { "Filled",     65  },
    { "Price",      75  },
    { "Avg Fill",   75  },
    { "Status",     110 },
    { "Exchange",   90  },
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

        // Col 0 — Time (insert the row here)
        LVITEMA lvi  = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = i;
        lvi.iSubItem = 0;
        lvi.lParam   = (LPARAM)i;         // store row index for custom draw
        lvi.pszText  = (LPSTR)o.time.c_str();
        ListView_InsertItem(hList, &lvi);

        // Col 1 — Symbol
        ListView_SetItemText(hList, i, 1, (LPSTR)o.symbol.c_str());

        // Col 2 — Action (BUY / SELL)
        ListView_SetItemText(hList, i, 2, (LPSTR)o.action.c_str());

        // Col 3 — Order type
        ListView_SetItemText(hList, i, 3, (LPSTR)o.orderType.c_str());

        // Col 4 — Total qty
        snprintf(buf, sizeof(buf), "%.0f", o.totalQty);
        ListView_SetItemText(hList, i, 4, buf);

        // Col 5 — Filled qty
        if (o.filledQty > 0)
            snprintf(buf, sizeof(buf), "%.0f", o.filledQty);
        else
            buf[0] = '\0';
        ListView_SetItemText(hList, i, 5, buf);

        // Col 6 — Limit price (blank for MKT orders)
        if (o.price > 0)
            snprintf(buf, sizeof(buf), "%.2f", o.price);
        else
            snprintf(buf, sizeof(buf), "MKT");
        ListView_SetItemText(hList, i, 6, buf);

        // Col 7 — Avg fill price
        if (o.avgFillPx > 0)
            snprintf(buf, sizeof(buf), "%.2f", o.avgFillPx);
        else
            buf[0] = '\0';
        ListView_SetItemText(hList, i, 7, buf);

        // Col 8 — Status
        ListView_SetItemText(hList, i, 8, (LPSTR)o.status.c_str());

        // Col 9 — Exchange
        ListView_SetItemText(hList, i, 9, (LPSTR)o.exchange.c_str());
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
            hWnd, (HMENU)1, hInst, NULL);

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
            lvc.fmt     = (i >= 4 && i <= 7) ? LVCFMT_RIGHT : LVCFMT_LEFT;
            ListView_InsertColumn(hList, i, &lvc);
        }

        api.setOrdersWindow(hWnd);
        break;
    }

    case WM_SIZE: {
        HWND hList = GetDlgItem(hWnd, 1);
        if (!hList) return 0;
        RECT rc;
        GetClientRect(hWnd, &rc);
        MoveWindow(hList, 0, 0, rc.right, rc.bottom, TRUE);
        break;
    }

    case WM_ORDERS_UPDATE: {
        HWND hList = GetDlgItem(hWnd, 1);
        if (hList) Orders_Repopulate(hList);
        break;
    }

    // ── Dark mode: paint the ListView background and items ────────────────────
    case WM_NOTIFY: {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->idFrom != 1) break;

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
                    }
                    return CDRF_NOTIFYSUBITEMDRAW;

                case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                    if (cd->iSubItem == 8) { // Status column — color by state
                        // Get status text
                        char statusBuf[64] = {};
                        HWND hList = GetDlgItem(hWnd, 1);
                        ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, 8,
                                            statusBuf, sizeof(statusBuf));
                        cd->clrText = Orders_StatusColor(statusBuf, dark);
                        if (dark) cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                        return CDRF_NEWFONT;
                    }
                    if (cd->iSubItem == 2) { // Action — BUY green, SELL red
                        char buf[16] = {};
                        HWND hList = GetDlgItem(hWnd, 1);
                        ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, 2, buf, sizeof(buf));
                        if (strcmp(buf, "BUY") == 0)
                            cd->clrText = RGB(80, 200, 120);
                        else if (strcmp(buf, "SELL") == 0)
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

    case WM_DESTROY:
        api.unsetOrdersWindow();
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}