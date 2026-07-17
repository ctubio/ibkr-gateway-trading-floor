#pragma once

void StartOrders() { StartGenericWindow(ORDERS_CLASS_NAME, "Orders", L"TWSAPIClientTradingFloor.Orders", 596, 240); }

#define ID_ORDERS_LIST          9003
#define ID_ORDERS_PRICE_EDIT    9010
#define ID_ORDERS_QTY_EDIT      9011
#define ID_ORDERS_PRICE_LABEL   9012
#define ID_ORDERS_QTY_LABEL     9013
#define ID_ORDERS_HINT_LABEL    9014
#define ID_ORDERS_FOCUS_TIMER   9020   // one-shot: defers SetFocus past click/activation processing

#define EDIT_PANEL_H  44   // height reserved at the bottom when the panel is visible

struct OrdersEditState {
    int    orderId      = -1;
    bool   partialFill  = false;
    double originalQty  = 0.0;
    bool   panelVisible = false;
};
static OrdersEditState s_editState;
static ListViewZoomData OrdersZoomData = { NULL, NULL, 14, "Zoom_Orders" };
static HFONT fontInputs   = NULL;

// ── Column definitions ────────────────────────────────────────────────────────

struct OrderCol { const char* header; int width; int fmt; };
static const OrderCol orderCols[] = {
    { "Side",           0,  LVCFMT_CENTER},
    { "Symbol",        80,  LVCFMT_CENTER},
    { "Quote",        135,  LVCFMT_RIGHT },
    { "Avg Filled",   135,  LVCFMT_RIGHT },
    { "Status",       225,  LVCFMT_CENTER},
    // { "Time",          80,  LVCFMT_CENTER},
};
static const int ORDER_COL_COUNT = (int)(sizeof(orderCols) / sizeof(orderCols[0]));
// ── Helpers ───────────────────────────────────────────────────────────────────

// Returns a color for the status text (used in NM_CUSTOMDRAW).
static COLORREF Orders_StatusColor(const std::string& orderType, const std::string& status, bool dark) {
    if (status == "Filled")  { // return RGB(196, 110, 43);
        if (orderType == "BUY") return RGB(54, 133, 80);
        else if (orderType == "SELL") return RGB(130, 53, 53);
    } 
    if (status == "Partially Filled")                 return RGB(255, 200, 60);
    if (status == "Cancelled" || status == "Inactive" || status == "PendingCancel")
        return dark ? RGB(130, 130, 130) : RGB(160, 160, 160);
    if (status == "Submitted" || status == "PreSubmitted" || status == "PendingSubmit" || status == "Pending") {
        if (orderType == "BUY") return RGB(80, 200, 120);
        else if (orderType == "SELL") return RGB(220, 80, 80);
    }
        
    return dark ? DM_TEXT : LM_TEXT;
}


// ── Inline edit panel ─────────────────────────────────────────────────────────
// Price and Qty edit fields shown at the bottom of the Orders window when an
// editable order is selected.  Hidden (and the ListView expands to fill) when
// no order is selected or the order is in a terminal / non-editable state.

// Resize ListView and show/hide the edit panel controls to fit the window.
static void Orders_LayoutPanel(HWND hWnd, bool showPanel) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right;
    int h = rc.bottom;

    HWND hList      = GetDlgItem(hWnd, ID_ORDERS_LIST);
    HWND hPriceLbl  = GetDlgItem(hWnd, ID_ORDERS_PRICE_LABEL);
    HWND hPriceEdit = GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT);
    HWND hQtyLbl    = GetDlgItem(hWnd, ID_ORDERS_QTY_LABEL);
    HWND hQtyEdit   = GetDlgItem(hWnd, ID_ORDERS_QTY_EDIT);
    HWND hHint      = GetDlgItem(hWnd, ID_ORDERS_HINT_LABEL);

    int listH = showPanel ? h - EDIT_PANEL_H : h;
    if (hList) MoveWindow(hList, 0, 0, w, listH, TRUE);

    int show = showPanel ? SW_SHOW : SW_HIDE;
    // Panel geometry — two groups side by side.
    // [Price: |__edit__]   [Qty: |__edit__]   [↑↓ Tab  Enter]
    int py     = listH + 6;
    int editH  = 37;
    int lblW   = 50;
    int editW  = 120;
    int gapX   = 14;
    int startX = 18;

    int x = startX;
    if (hHint) {
        MoveWindow(hHint, x, py + 5, 160, 24, TRUE);
        ShowWindow(hHint, show);
        x += 160 + 5;
    }
    if (hPriceLbl)  { MoveWindow(hPriceLbl,  x,         py + 5, lblW,  24,    TRUE); ShowWindow(hPriceLbl,  show); } x += lblW + 5;
    if (hPriceEdit) { MoveWindow(hPriceEdit, x,         py,     editW, editH, TRUE); ShowWindow(hPriceEdit, show); } x += editW + gapX;

    // Qty is hidden when partialFill.
    bool showQty = showPanel && !s_editState.partialFill;
    int  qshow   = showQty ? SW_SHOW : SW_HIDE;
    if (hQtyLbl)  { MoveWindow(hQtyLbl,  x,         py + 5, lblW,  24,    TRUE); ShowWindow(hQtyLbl,  qshow); } x += lblW + 5;
    if (hQtyEdit) { MoveWindow(hQtyEdit, x,         py,     editW - 40, editH, TRUE); ShowWindow(hQtyEdit, qshow); } x += editW + gapX;
    
}

// Returns true if the given status string allows modification.
static bool Orders_IsEditable(const std::string& status) {
    return !(status == "Filled" || status == "Cancelled" ||
             status == "Inactive" || status == "PendingCancel");
}

// Hide the panel and let the ListView fill the window.
static void Orders_HideInlinePanel(HWND hWnd) {
    s_editState.orderId      = 0;
    s_editState.panelVisible = false;
    Orders_LayoutPanel(hWnd, false);
    InvalidateRect(GetDlgItem(hWnd, ID_ORDERS_LIST), NULL, TRUE);
}

// Rebuilds the entire ListView from the current snapshot.
static void Orders_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
    if (!hList) return;
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    auto orders = api().getOrdersSorted();
    int submitted = 0;
    int filled = 0;
    for (int i = 0; i < (int)orders.size(); ++i) {
        const auto& o = orders[i];

        int col = 0;
        LVITEMA lvi  = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = i;
        lvi.iSubItem = col++;
        lvi.lParam   = (LPARAM)o.orderId;  // orderId — used by cancel/edit handlers
        lvi.pszText  = (LPSTR)o.action.c_str();
        ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, i, col++, (LPSTR)o.symbol.c_str());

        std::string quoteStr;
        if (o.price > 0)
            quoteStr = std::format("{:.0f} @ {:.2f}", o.totalQty, o.price);
        else if (o.auxPrice > 0)
            quoteStr = std::format("{:.0f} @ {:.2f}", o.totalQty, o.auxPrice);
        else
            quoteStr = std::format("{:.0f} @ MKT", o.totalQty);
        ListView_SetItemText(hList, i, col++, (LPSTR)quoteStr.c_str());

        std::string fillStr;
        if (o.filledQty > 0)
            fillStr = std::format("{:.0f} @ {:.2f}", o.filledQty, o.avgFillPx);
        else
            fillStr = "-- @ --";
        ListView_SetItemText(hList, i, col++, (LPSTR)fillStr.c_str());
        
        std::string fullTypeStr = o.tif + " " + o.orderType + " " + o.status;
        ListView_SetItemText(hList, i, col++, (LPSTR)fullTypeStr.c_str());

        // ListView_SetItemText(hList, i, col++, (LPSTR)o.time.c_str());

        if (o.status == "Submitted" || o.status == "PreSubmitted" || o.status == "PendingSubmit" || o.status == "Pending") submitted++;
        if (o.status == "Filled") filled++;
    }   

    SetWindowTextA(hWnd, ("Orders: " + std::to_string(submitted) + " Submitted | " + std::to_string(filled) + " Filled").c_str());

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    // If the order currently shown in the inline panel is no longer editable
    // (e.g. it just got filled/cancelled), hide the panel. Otherwise leave it as is.
    if (s_editState.panelVisible && s_editState.orderId != 0) {
        bool stillEditable = false;
        for (const auto& o : orders) {
            if (o.orderId == s_editState.orderId) {
                stillEditable = Orders_IsEditable(o.status);
                break;
            }
        }
        if (!stillEditable)
            Orders_HideInlinePanel(hWnd);
    }
}


// Moves the ListView selection up or down by one row (clamped to the ends).
// If nothing is currently selected, selects the first row regardless of dir.
// Does NOT move keyboard focus — safe to call while an edit field has focus.
static void Orders_MoveSelection(HWND hWnd, int dir) {
    HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
    if (!hList) return;
    int count = ListView_GetItemCount(hList);
    if (count <= 0) return;

    int sel  = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    int next = (sel < 0) ? 0 : sel + dir;
    if (next < 0) next = 0;
    if (next >= count) next = count - 1;
    if (next == sel) return;

    LVITEMA lvi = {};
    lvi.mask  = LVIF_PARAM;
    lvi.iItem = next;
    if (!ListView_GetItem(hList, &lvi)) {
        return;
    }
    int orderId = (int)lvi.lParam;
    auto orders = api().getOrdersSorted();
    for (const auto& o : orders) {
        if (o.orderId == orderId) {
            if (Orders_IsEditable(o.status)) {
                ListView_SetItemState(hList, sel, 0, LVIS_SELECTED | LVIS_FOCUSED);
                
                UINT state = LVIS_SELECTED | LVIS_FOCUSED;
                ListView_SetItemState(hList, next, state, state);
                ListView_EnsureVisible(hList, next, FALSE);
            }
            break;
        }
    }

}

// Subclass for the orders ListView: intercepts Ctrl+Up/Ctrl+Down so they move
// the selection by one row instead of the default multi-select behavior
// (which — since this list isn't LVS_SINGLESEL — would otherwise just move
// the dotted focus rectangle without changing the selection). Everything
// else (plain arrows, Home/End, mouse, etc.) is passed straight through.
static LRESULT CALLBACK OrdersList_SubclassProc(HWND hWnd, UINT message, WPARAM wParam,
                                                 LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (message == WM_KEYDOWN && (wParam == VK_UP || wParam == VK_DOWN) &&
        (GetKeyState(VK_CONTROL) & 0x8000)) {
        Orders_MoveSelection(GetParent(hWnd), (wParam == VK_UP) ? -1 : 1);
        return 0;
    }
    return DefSubclassProc(hWnd, message, wParam, lParam);
}

// Forward ENTER from an edit control up to the Orders window; handle TAB and Arrows.
static LRESULT CALLBACK EditField_SubclassProc(HWND hWnd, UINT message, WPARAM wParam,
                                               LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (message == WM_GETDLGCODE) {
        LRESULT res = DefSubclassProc(hWnd, message, wParam, lParam);
        return res | DLGC_WANTTAB | DLGC_WANTARROWS | DLGC_WANTALLKEYS;
    }

    if (message == WM_CHAR) {
        if (wParam == VK_TAB || wParam == VK_RETURN)
            return 0;
    }

    if (message == WM_KEYDOWN) {
        if (wParam == VK_ESCAPE) {
            if (s_editState.panelVisible && s_editState.orderId != 0) {
                api().cancelOrder(s_editState.orderId);
                HWND hParent = GetParent(hWnd);
                Orders_HideInlinePanel(hParent);
            }
            return 0;
        }
        if (wParam == VK_RETURN) {
            // ENTER from a subclassed edit field → submit the pending edit.
            if (s_editState.panelVisible && s_editState.orderId != 0) {
                HWND hParent = GetParent(hWnd);
                HWND hPriceEdit = GetDlgItem(hParent, ID_ORDERS_PRICE_EDIT);
                HWND hQtyEdit   = GetDlgItem(hParent, ID_ORDERS_QTY_EDIT);
                char pBuf[32] = {}, qBuf[32] = {};
                if (hPriceEdit) GetWindowTextA(hPriceEdit, pBuf, sizeof(pBuf));
                double price = atof(pBuf);
                double qty;
                if (!s_editState.partialFill && hQtyEdit) {
                    GetWindowTextA(hQtyEdit, qBuf, sizeof(qBuf));
                    qty = atof(qBuf);
                } else {
                    qty = s_editState.originalQty;
                }
                if (qty > 0)
                    api().modifyOrder(s_editState.orderId, price, qty);
            }
            return 0;
        }

        if (wParam == VK_TAB) {
            HWND hParent = GetParent(hWnd);
            HWND hPrice  = GetDlgItem(hParent, ID_ORDERS_PRICE_EDIT);
            HWND hQty    = GetDlgItem(hParent, ID_ORDERS_QTY_EDIT);
            // Only cycle between visible fields.
            bool qtyVisible = hQty && IsWindowVisible(hQty);
            if (qtyVisible) {
                HWND hNext = (hWnd == hPrice) ? hQty : hPrice;
                SetFocus(hNext);
                int len = GetWindowTextLengthA(hNext);
                SendMessageA(hNext, EM_SETSEL, len, len);
            }
            return 0;
        }

        if ((wParam == VK_UP || wParam == VK_DOWN) && (GetKeyState(VK_CONTROL) & 0x8000)) {
            // Ctrl+Arrow: move the ListView's selected order instead of
            // stepping the price/qty value. Focus deliberately stays in
            // this edit field.
            HWND hParent = GetParent(hWnd);
            Orders_MoveSelection(hParent, (wParam == VK_UP) ? -1 : 1);
            return 0;
        }

        if (wParam == VK_UP || wParam == VK_DOWN) {
            char buf[32] = {};
            GetWindowTextA(hWnd, buf, sizeof(buf));
            double val  = atof(buf);
            double step = (uIdSubclass == 1) ? 0.01 : 1.0;  // price vs qty
            val += (wParam == VK_UP) ? step : -step;
            if (val < 0.0) val = 0.0;
            std::string s = (uIdSubclass == 1) ? std::format("{:.2f}", val) : std::format("{:.0f}", val);
            SetWindowTextA(hWnd, s.c_str());
            int len = GetWindowTextLengthA(hWnd);
            SendMessageA(hWnd, EM_SETSEL, len, len);
            return 0;
        }
    }
    return DefSubclassProc(hWnd, message, wParam, lParam);
}

// Populate the inline edit fields from the given order and make the panel visible.
static void Orders_ShowInlinePanel(HWND hWnd, const TradingAPI::OrderInfo& order) {
    s_editState.orderId     = order.orderId;
    s_editState.partialFill = (order.status == "Partially Filled");
    s_editState.originalQty = order.totalQty;
    s_editState.panelVisible = true;

    HWND hPriceEdit = GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT);
    HWND hQtyEdit   = GetDlgItem(hWnd, ID_ORDERS_QTY_EDIT);

    std::string priceStr;
    if (order.price > 0)         priceStr = std::format("{:.2f}", order.price);
    else if (order.auxPrice > 0) priceStr = std::format("{:.2f}", order.auxPrice);
    else                         priceStr = "0.00";
    if (hPriceEdit) SetWindowTextA(hPriceEdit, priceStr.c_str());

    std::string qtyStr = std::format("{:.0f}", order.totalQty);
    if (hQtyEdit) SetWindowTextA(hQtyEdit, qtyStr.c_str());

    HWND hHint = GetDlgItem(hWnd, ID_ORDERS_HINT_LABEL);
    if (hHint) {
        std::string hint = order.symbol + " " + order.action + " " + order.orderType;
        SetWindowTextA(hHint, hint.c_str());
        SetCtrlColor(hHint, order.action == "BUY" ? COINS_CLR_GREEN : COINS_CLR_RED);
        InvalidateRect(hHint, NULL, TRUE);
    }
    Orders_LayoutPanel(hWnd, true);
    InvalidateRect(GetDlgItem(hWnd, ID_ORDERS_LIST), NULL, TRUE);

    // Focus the price field and select all. Deferred via a one-shot timer
    // rather than called directly: on the FIRST click into an inactive Orders
    // window, the OS's own activation sequence (WM_MOUSEACTIVATE → SetForegroundWindow
    // → the list view claiming focus for itself) runs interleaved with this
    // notification handler and steals focus back after we set it. A timer message
    // is only dispatched once the queue is otherwise empty, so by the time it
    // fires, activation + the list's own focus grab have already finished, and
    // we reliably win the focus. (PostMessage is not late enough for this case,
    // since some of that activation handling can itself be queued behind it.)
    if (hPriceEdit) {
        SetTimer(hWnd, ID_ORDERS_FOCUS_TIMER, 333, NULL);
    }
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
            ApplyListViewFont(hList, OrdersZoomData.hFont, OrdersZoomData.hBoldFont, OrdersZoomData.fontSize);
            SetWindowSubclass(hList, ListViewZoomProc, 0, (DWORD_PTR)&OrdersZoomData);
            SetWindowSubclass(hList, OrdersList_SubclassProc, 1, 0);

            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNA lvc = {};
            lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
            for (int i = 0; i < ORDER_COL_COUNT; ++i) {
                lvc.cx      = orderCols[i].width;
                lvc.pszText = (LPSTR)orderCols[i].header;
                lvc.fmt     = orderCols[i].fmt;
                ListView_InsertColumn(hList, i, &lvc);
                if (i == 0) {
                    LVCOLUMN lvcUpdate = { 0 };
                    lvcUpdate.mask = LVCF_FMT;
                    lvcUpdate.fmt = orderCols[i].fmt;
                    ListView_SetColumn(hList, i, &lvcUpdate);
                }
            }

            // ── Inline edit panel controls (initially hidden) ──────────────────
            CreateWindowA("STATIC", "Price:",
                WS_CHILD | SS_RIGHT,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_PRICE_LABEL, hInst, NULL);
            HWND hEditPrice = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                // Added ES_MULTILINE here
                WS_CHILD | ES_AUTOHSCROLL | ES_CENTER | ES_MULTILINE, 
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_PRICE_EDIT, hInst, NULL);

            CreateWindowA("STATIC", "Qty:",
                WS_CHILD | SS_RIGHT,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_QTY_LABEL, hInst, NULL);
            HWND hEditQty = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                // Added ES_MULTILINE here
                WS_CHILD | ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | ES_MULTILINE, 
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_QTY_EDIT, hInst, NULL);

            CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_QTY_EDIT, hInst, NULL);

            // Subclass edit fields to intercept keyboard navigation.
            SetWindowSubclass(GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT), EditField_SubclassProc, 1, 0);
            SetWindowSubclass(GetDlgItem(hWnd, ID_ORDERS_QTY_EDIT),   EditField_SubclassProc, 2, 0);

            CreateWindowA("STATIC", "",
                WS_CHILD | SS_LEFT,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_HINT_LABEL, hInst, NULL);

            // Apply font to all children.
            EnumChildWindows(hWnd, SetFontCallback, (LPARAM)OrdersZoomData.hFont);
            
            fontInputs   = MakeFont(18, true);
            SendMessage(hEditPrice, WM_SETFONT, (WPARAM)fontInputs, TRUE);
            SendMessage(hEditQty, WM_SETFONT, (WPARAM)fontInputs, TRUE);

            api().addApiUpdateWindow(hWnd);  
            Orders_Repopulate(hWnd);
            break;
        }

        case WM_SIZE: {
            Orders_LayoutPanel(hWnd, s_editState.panelVisible);
            break;
        }

        case WM_TIMER: {
            if (wParam == ID_ORDERS_FOCUS_TIMER) {
                KillTimer(hWnd, ID_ORDERS_FOCUS_TIMER);
                HWND hPriceEdit = GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT);
                if (hPriceEdit) {
                    SetFocus(hPriceEdit);
                    int len = GetWindowTextLengthA(hPriceEdit);
                    SendMessageA(hPriceEdit, EM_SETSEL, len, len);
                }
            }
            break;
        }
        case WM_NOTIFY: {
            NMHDR* hdr = (NMHDR*)lParam;
            if (hdr->idFrom != ID_ORDERS_LIST) break;

            if (hdr->code == LVN_KEYDOWN) {
                NMLVKEYDOWN* kd = (NMLVKEYDOWN*)lParam;
                if (kd->wVKey == VK_ESCAPE) {
                    HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (sel >= 0) {
                        LVITEMA lvi = {};
                        lvi.mask  = LVIF_PARAM;
                        lvi.iItem = sel;
                        if (ListView_GetItem(hList, &lvi)) {
                            api().cancelOrder((int)lvi.lParam);
                            Orders_HideInlinePanel(hWnd);
                        }
                    }
                }
                return 0;
            }

            // ── Selection change: update inline edit panel ────────────────────
            if (hdr->code == LVN_ITEMCHANGED) {
                NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
                // Only care about a newly-selected item.
                if (!(nmlv->uChanged & LVIF_STATE)) break;
                if (!(nmlv->uNewState & LVIS_SELECTED)) break;

                HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                if (sel < 0) {
                    Orders_HideInlinePanel(hWnd);
                    break;
                }
                LVITEMA lvi = {};
                lvi.mask  = LVIF_PARAM;
                lvi.iItem = sel;
                if (!ListView_GetItem(hList, &lvi)) {
                    Orders_HideInlinePanel(hWnd);
                    break;
                }
                int orderId = (int)lvi.lParam;
                auto orders = api().getOrdersSorted();
                for (const auto& o : orders) {
                    if (o.orderId == orderId) {
                        if (Orders_IsEditable(o.status))
                            Orders_ShowInlinePanel(hWnd, o);
                        else
                            Orders_HideInlinePanel(hWnd);
                        break;
                    }
                }
                return 0;
            }
            if (hdr->code == NM_CUSTOMDRAW) {
                NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
                bool dark = Settings_DarkMode();

                switch (cd->nmcd.dwDrawStage) {
                    case CDDS_PREPAINT:
                        return CDRF_NOTIFYITEMDRAW;

                    case CDDS_ITEMPREPAINT:
                        cd->nmcd.uItemState &= ~CDIS_SELECTED;
                        if (dark) {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            cd->clrText   = DM_TEXT;
                        } else {
                            cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? GetSysColor(COLOR_WINDOW) : RGB(245, 245, 245);
                            cd->clrText   = LM_TEXT;
                        }
                        // Highlight the row of the order currently open in the inline edit panel.
                        // lItemlParam is the LVITEM.lParam set in Orders_Repopulate (the orderId).
                        if (s_editState.panelVisible && s_editState.orderId != 0 &&
                            (int)cd->nmcd.lItemlParam == s_editState.orderId) {
                            cd->clrTextBk = dark ? RGB(40, 50, 75) : RGB(255, 244, 190);
                            return CDRF_NOTIFYSUBITEMDRAW;
                        }
                        return CDRF_NOTIFYSUBITEMDRAW;

                    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                        if (cd->iSubItem >= 0) {
                            // Get status text
                            char statusBuf[64] = {};
                            HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                            ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, 4, statusBuf, sizeof(statusBuf));
                            std::string statusStr(statusBuf);
                            size_t pos = statusStr.rfind(' ');
                            statusStr = (pos == std::string::npos) ? statusStr : statusStr.substr(pos + 1);
                            char buf[16] = {};
                            ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, 0, buf, sizeof(buf));
                            size_t len = strlen(buf);
                            std::string orderType;
                            if (len >= 3 && strcmp(buf + len - 3, "BUY") == 0)       orderType = "BUY";
                            else if (len >= 4 && strcmp(buf + len - 4, "SELL") == 0) orderType = "SELL";
                            cd->clrText = Orders_StatusColor(orderType, statusStr, dark);
                            bool isEditRow = s_editState.panelVisible && s_editState.orderId != 0 &&
                                             (int)cd->nmcd.lItemlParam == s_editState.orderId;
                            if (isEditRow)
                                cd->clrTextBk = dark ? RGB(40, 50, 75) : RGB(255, 244, 190);
                            else if (dark)
                                cd->clrTextBk = (cd->nmcd.dwItemSpec % 2 == 0) ? DM_BG : DM_BG2;
                            return CDRF_NEWFONT;
                        }
                        return CDRF_DODEFAULT;
                    }
                }
            }
            break;
        }


        case WM_API_UPDATE: {
            if (api().isMarketDataConnected() && api().isTradingConnected()) {
                Orders_Repopulate(hWnd);
            } else {
                HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                if (hList) ListView_DeleteAllItems(hList);
            }
            break;
        }
        
        case WM_DESTROY:
            api().removeApiUpdateWindow(hWnd);
            
            if (fontInputs)   {
                DeleteObject(fontInputs);
                fontInputs = NULL;
            }
            if (OrdersZoomData.hFont) {
                DeleteObject(OrdersZoomData.hFont);
            }   
            if (OrdersZoomData.hBoldFont) {
                DeleteObject(OrdersZoomData.hBoldFont);
            }
            break;
    }
    
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}