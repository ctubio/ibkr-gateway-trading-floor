#pragma once

int windowOrdersWidth = 470;

void StartOrders() { StartGenericWindow(ORDERS_CLASS_NAME, "Orders", L"TWSAPIClientTradingFloor.Orders", windowOrdersWidth, 240); }

#define ID_ORDERS_LIST          9003
#define ID_ORDERS_PRICE_EDIT    9010
#define ID_ORDERS_QTY_EDIT      9011
#define ID_ORDERS_PRICE_LABEL   9012
#define ID_ORDERS_HINT_LABEL    9013
#define ID_ORDERS_QTY_TIF_LABEL 9014   // small hint, top-left inside the Qty edit
#define ID_ORDERS_QTY_TYPE_LABEL 9015  // small hint, bottom-left inside the Qty edit
#define ID_ORDERS_FOCUS_TIMER   9020   // one-shot: defers SetFocus past click/activation processing

#define EDIT_PANEL_H  44   // height reserved at the bottom when the panel is visible

struct OrdersEditState {
    int    orderId      = -1;
    bool   partialFill  = false;
    double originalQty  = 0.0;
    bool   panelVisible = false;
    // ── Unsent (transmit=false) placeholder rows ────────────────────────────
    // These never made it into api()'s ordersMap, so Enter must re-submit a
    // fresh order (transmit=true) instead of modifyOrder()'ing a nonexistent one.
    bool        isUnsent    = false;
    int         conId       = 0;
    std::string symbol;
    std::string action;
    bool        isOvernight = false;
    double fullStopPrice;
    double fullProfitPrice;
};
static OrdersEditState s_editState;

// Cosmetic-only cache of Unsent placeholder rows, keyed by orderId, so the
// inline panel can be repopulated when one is clicked (they're absent from
// api()'s ordersMap). Cleared on every Orders_Repopulate() — the same event
// that wipes the phantom row from the ListView.
static std::map<int, TradingAPI::OrderInfo> s_unsentOrders;

// Column indices, matching orderCols[] order below.
enum OrderColIdx { OCOL_SIDE = 0, OCOL_SYMBOL, OCOL_QUOTE, OCOL_STATUS };

// ── Column definitions ────────────────────────────────────────────────────────

struct OrderCol { const char* header; int width; int fmt; };
static const OrderCol orderCols[] = {
    { "Side",           0,  LVCFMT_CENTER},
    { "Symbol",        80,  LVCFMT_CENTER},
    { "Quote",        180,  LVCFMT_LEFT },
    { "Status",       170,  LVCFMT_CENTER},
};
static const int ORDER_COL_COUNT = (int)(sizeof(orderCols) / sizeof(orderCols[0]));
// ── Helpers ───────────────────────────────────────────────────────────────────

// Returns a color for the status text (used in NM_CUSTOMDRAW).
static COLORREF Orders_StatusColor(const std::string& orderType, const std::string& status, bool dark) {
    if (status == "Filled")  { // return RGB(196, 110, 43);
        if (orderType == "BUY") return RGB(34, 82, 50);
        else if (orderType == "SELL") return RGB(102, 43, 43);
    } 
    if (status == "Partially Filled")                 return RGB(255, 200, 60);
    if (status == "Submitted" || status == "PreSubmitted" || status == "PreSub" || status == "PendingSubmit" || status == "Pending" || status == "Unsent") {
        if (orderType == "BUY") return RGB(80, 200, 120);
        else if (orderType == "SELL") return RGB(220, 80, 80);
    }
        
    return dark ? DM_TEXT : LM_TEXT;
}


// ── Inline edit panel ─────────────────────────────────────────────────────────


static void UpdatePriceLabel(HWND hWnd) {
    if (!s_editState.panelVisible) return;
    HWND hTotalLabel = GetDlgItem(hWnd, ID_ORDERS_PRICE_LABEL);
    HWND hPriceEdit  = GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT);
    HWND hOrderQty   = GetDlgItem(hWnd, ID_ORDERS_QTY_EDIT);

    if (!hTotalLabel || !hPriceEdit || !hOrderQty) return;

    char priceBuf[32] = {}, qtyBuf[32] = {};
    GetWindowTextA(hPriceEdit, priceBuf, sizeof(priceBuf));
    GetWindowTextA(hOrderQty, qtyBuf, sizeof(qtyBuf));

    double price = 0;
    double qty   = 0;
    try {
        price = std::stod(priceBuf);
        qty = std::abs(std::stod(qtyBuf));
    } catch (...) { price = 0; qty = 0; }

    SetWindowTextA(hTotalLabel, FormatWithCommas(price * qty).c_str());
    InvalidateRect(hOrderQty, NULL, TRUE);
    InvalidateRect(hTotalLabel, NULL, TRUE);
}

// Price and Qty edit fields shown at the bottom of the Orders window when an
// editable order is selected.  Hidden (and the ListView expands to fill) when
// no order is selected or the order is in a terminal / non-editable state.

// Resize ListView and show/hide the edit panel controls to fit the window.
static void Orders_LayoutPanel(HWND hWnd, bool showPanel) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right;
    int h = rc.bottom;

    HWND hList          = GetDlgItem(hWnd, ID_ORDERS_LIST);
    HWND hTotalLabel    = GetDlgItem(hWnd, ID_ORDERS_PRICE_LABEL);
    HWND hPriceEdit     = GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT);
    HWND hOrderQty      = GetDlgItem(hWnd, ID_ORDERS_QTY_EDIT);
    HWND hOrderTypeHint = GetDlgItem(hWnd, ID_ORDERS_HINT_LABEL);
    HWND hQtyTypeLabel  = GetDlgItem(hWnd, ID_ORDERS_QTY_TYPE_LABEL);
    HWND hQtyTifLabel   = GetDlgItem(hWnd, ID_ORDERS_QTY_TIF_LABEL);

    int listH = showPanel ? h - EDIT_PANEL_H : h;
    int py    = listH + 6;
    int editH = 37;
    int editW = 180;

    int show = showPanel ? SW_SHOW : SW_HIDE;

    // Count controls for DeferWindowPos
    int ctrlCount = 0;
    if (hList) ctrlCount++;
    if (hOrderTypeHint) ctrlCount++;
    if (hPriceEdit) ctrlCount++;
    if (hOrderQty) ctrlCount++;
    if (hTotalLabel) ctrlCount++;
    if (hQtyTifLabel) ctrlCount++;
    if (hQtyTypeLabel) ctrlCount++;

    HDWP hdwp = BeginDeferWindowPos(ctrlCount);
    if (!hdwp) return;

    // ListView
    if (hList) {
        hdwp = DeferWindowPos(hdwp, hList, NULL, 0, 0, w, listH, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Order type hint (left side)
    if (hOrderTypeHint) {
        int hintW = 130;
        hdwp = DeferWindowPos(hdwp, hOrderTypeHint, NULL, 8, py + 5, hintW, 24, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(hOrderTypeHint, show);
    }

    // Price edit (right side)
    int x = rc.right - (editW * 2) + 60 - 10;
    if (hPriceEdit) {
        hdwp = DeferWindowPos(hdwp, hPriceEdit, NULL, x, py, editW - 60, editH, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(hPriceEdit, show);
    }
    x += editW - 60 + 10;

    // Qty edit (hidden when partialFill)
    bool showQty = showPanel && !s_editState.partialFill;
    int qshow = showQty ? SW_SHOW : SW_HIDE;
    if (hOrderQty) {
        hdwp = DeferWindowPos(hdwp, hOrderQty, NULL, x, py, editW, editH, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(hOrderQty, qshow);
    }

    // Notional value hint (bottom-right of Qty edit)
    const int hintH = 16;
    const int hintMargin = 4;
    const int hintW = std::max(40, editW / 2 - 6);
    if (hTotalLabel) {
        hdwp = DeferWindowPos(hdwp, hTotalLabel, NULL,
            x + editW - hintW - hintMargin, py + editH - hintH - 2,
            hintW, hintH, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(hTotalLabel, show);
    }

    // TIF hint (top-left of Qty edit)
    const int qtyHintW = 48;
    const int qtyHintH = 16;
    if (hQtyTifLabel) {
        hdwp = DeferWindowPos(hdwp, hQtyTifLabel, NULL, x + 4, py + 2, qtyHintW, qtyHintH, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(hQtyTifLabel, show);
    }

    // Order type hint (bottom-left of Qty edit)
    if (hQtyTypeLabel) {
        hdwp = DeferWindowPos(hdwp, hQtyTypeLabel, NULL, x + 4, py + editH - qtyHintH - 2, qtyHintW, qtyHintH, SWP_NOZORDER | SWP_NOACTIVATE);
        ShowWindow(hQtyTypeLabel, show);
    }

    EndDeferWindowPos(hdwp);

    CenterEditText(hPriceEdit);
    CenterEditText(hOrderQty);

    if (show) UpdatePriceLabel(hWnd);
}

// Returns true if the given status string allows modification.
static bool Orders_IsEditable(const std::string& status) {
    return !(status == "Filled" || status == "Cancelled" ||
             status == "Inactive" || status == "PendingCancel");
}

static void SetOrdersTitle(HWND hWnd) {
    int submitted = (int)s_unsentOrders.size();
    int filled = 0;
    for (const auto& o : api().getOrdersSorted()) {
        if (o.status == "Submitted" || o.status == "PreSubmitted" || o.status == "PendingSubmit" || o.status == "Pending") submitted++;
        if (o.status == "Filled") filled++;
    }

    SetWindowTextA(hWnd, ("Orders: " + std::to_string(submitted) + " Submitted | " + std::to_string(filled) + " Filled").c_str());
}

// Hide the panel and let the ListView fill the window.
static void Orders_HideInlinePanel(HWND hWnd) {
    s_editState.orderId      = 0;
    s_editState.panelVisible = false;
    Orders_LayoutPanel(hWnd, false);
    InvalidateRect(GetDlgItem(hWnd, ID_ORDERS_LIST), NULL, TRUE);
}

// Rebuilds the ListView from the current snapshot, preserving any Unsent
// placeholder rows in place. Only rows whose orderId is NOT in s_unsentOrders
// get deleted/rebuilt; Unsent rows are left untouched unless a real order
// with the same orderId has since appeared (meaning it was transmitted for
// real — the placeholder is stale and gets dropped).
static void Orders_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
    if (!hList) return;
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);

    auto orders = api().getOrdersSorted();

    // A placeholder whose orderId now has a real ordersMap entry has been
    // superseded (the user resubmitted it via the inline panel) — drop it so
    // its row gets removed in the sweep below instead of lingering as a dupe.
    for (const auto& o : orders) s_unsentOrders.erase(o.orderId);

    // Remove only the rows that aren't (still) an Unsent placeholder.
    for (int i = ListView_GetItemCount(hList) - 1; i >= 0; --i) {
        LVITEMA lvi = {};
        lvi.mask  = LVIF_PARAM;
        lvi.iItem = i;
        ListView_GetItem(hList, &lvi);
        int orderId = (int)lvi.lParam;
        if (!s_unsentOrders.count(orderId))
            ListView_DeleteItem(hList, i);
    }

    for (const auto& o : orders) {
        int row = ListView_GetItemCount(hList);   // append after any surviving Unsent rows

        int col = 0;
        LVITEMA lvi  = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = row;
        lvi.iSubItem = col++;
        lvi.lParam   = (LPARAM)o.orderId;
        lvi.pszText  = (LPSTR)o.action.c_str();
        ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, row, col++, (LPSTR)o.symbol.c_str());

        std::string quoteStr;
        if (o.trailStopPrice > 0.0)
            quoteStr += std::format("{:.0f} @ {:.2f} | {:.2f}", o.totalQty, o.trailStopPrice, o.price);
        else if (o.price > 0)
            quoteStr = std::format("{:.0f} @ {:.2f}", o.totalQty, o.price);
        else
            quoteStr = std::format("{:.0f} @ MKT", o.totalQty);
        ListView_SetItemText(hList, row, col++, (LPSTR)quoteStr.c_str());

        std::string fullTypeStr;
        if (o.filledQty > 0)
            fullTypeStr = std::format("{:.0f} @ {:.2f} ", o.filledQty, o.avgFillPx);
        if (o.status == "Filled")
            fullTypeStr += o.status;
        else fullTypeStr += o.tif + " " + o.orderType + " " + (o.status == "PreSubmitted" ? "PreSub" : o.status);
        ListView_SetItemText(hList, row, col++, (LPSTR)fullTypeStr.c_str());
    }

    SetOrdersTitle(hWnd);

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

    // If the order currently shown in the inline panel is no longer editable
    // (e.g. it just got filled/cancelled), hide the panel. Otherwise leave it as is.
    if (s_editState.panelVisible && s_editState.orderId != 0 && !s_editState.isUnsent) {
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

    auto makeSelection = [&]() -> void {
        ListView_SetItemState(hList, sel, 0, LVIS_SELECTED | LVIS_FOCUSED);
        
        UINT state = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(hList, next, state, state);
        ListView_EnsureVisible(hList, next, FALSE);
    };
    
    char statusBuf[64] = {};
    ListView_GetItemText(hList, next, OCOL_STATUS, statusBuf, sizeof(statusBuf));
    std::string statusStr(statusBuf);
    size_t pos = statusStr.rfind(' ');
    statusStr = (pos == std::string::npos) ? statusStr : statusStr.substr(pos + 1);
    if (statusStr == "Unsent") {
        makeSelection();
        return;

    }

    int orderId = (int)lvi.lParam;
    auto orders = api().getOrdersSorted();
    for (const auto& o : orders) {
        if (o.orderId == orderId) {
            if (Orders_IsEditable(o.status))
                makeSelection();
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
static LRESULT CALLBACK EditField_SubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
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
                HWND hParent = GetParent(hWnd);
                // Unsent placeholder never reached TWS — nothing to cancel there.
                HWND hList = GetDlgItem(hParent, ID_ORDERS_LIST);
                if (s_editState.isUnsent) {
                    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (sel >= 0) ListView_DeleteItem(hList, sel);
                    
                    auto uit = s_unsentOrders.find(s_editState.orderId);
                    if (uit != s_unsentOrders.end())
                        s_unsentOrders.erase(s_editState.orderId);
                    SetOrdersTitle(hParent);
                } else {
                    api().cancelOrder(s_editState.orderId);
                }
                Orders_HideInlinePanel(hParent);
                SetFocus(hList);
            }
            return 0;
        }
        if (wParam == VK_RETURN) {
            if (s_editState.panelVisible && s_editState.orderId != 0) {
                HWND hParent = GetParent(hWnd);
                HWND hPriceEdit = GetDlgItem(hParent, ID_ORDERS_PRICE_EDIT);
                HWND hOrderQty  = GetDlgItem(hParent, ID_ORDERS_QTY_EDIT);
                char pBuf[32] = {}, qBuf[32] = {};
                if (hPriceEdit) GetWindowTextA(hPriceEdit, pBuf, sizeof(pBuf));
                double price = atof(pBuf);
                double qty = s_editState.originalQty;
                if (!s_editState.partialFill && hOrderQty) {
                    GetWindowTextA(hOrderQty, qBuf, sizeof(qBuf));
                    qty = std::abs(atof(qBuf));
                }
                if (qty > 0) {
                    if (s_editState.isUnsent) {
                        // Placeholder was never transmitted — place it for real now.
                        bool stopValid = s_editState.fullStopPrice <= 0.0 || price <= 0.0 ||
                            (s_editState.action == "BUY"  ? s_editState.fullStopPrice < price
                                                        : s_editState.fullStopPrice > price);

                        bool profitValid = s_editState.fullProfitPrice <= 0.0 || price <= 0.0 ||
                            (s_editState.action == "BUY"  ? s_editState.fullProfitPrice > price
                                                        : s_editState.fullProfitPrice < price);

                        if (!stopValid || !profitValid || qty > 10) {
                            // bail out / show error instead of submitting
                            return 0;
                        }

                        // defensive: distances should never be negative once validated above
                        double stopPrice   = std::max(0.0, s_editState.fullStopPrice);
                        double profitPrice = std::max(0.0, s_editState.fullProfitPrice);
                        if ((price > 0 || stopPrice > 0) && qty > 0 && qty <= 10) {
                            if (stopPrice < 0.1) stopPrice = 0.0;
                            if (profitPrice < 0.1) profitPrice = 0.0;
                            api().submitOrder(s_editState.conId, s_editState.symbol, s_editState.action, s_editState.isOvernight, qty, price, stopPrice, price, profitPrice);
                        }
                        HWND hList = GetDlgItem(hParent, ID_ORDERS_LIST);
                        int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                        if (sel >= 0) ListView_DeleteItem(hList, sel);
                        Orders_HideInlinePanel(hParent);
                    } else {
                        api().modifyOrder(s_editState.orderId, price, qty);
                    }
                }
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
            if (uIdSubclass == 2) val = std::abs(val);
            double step = 0.0;
            if ((GetKeyState(VK_SHIFT) & 0x8000) != 0) {
                step = uIdSubclass == 1 ? 1.0 : 10.0;
            } else {
                step = uIdSubclass == 1 ? 0.01 : 1.0;
            }
            val += (wParam == VK_UP) ? step : -step;
            if (val < 0.0) val = 0.0;
            std::string s = (uIdSubclass == 1) ? std::format("{:.2f}", val) : std::format("{:+}", val * (s_editState.action == "BUY" ? 1 : -1));
            SetWindowTextA(hWnd, s.c_str());
            UpdatePriceLabel(GetParent(hWnd));
            int len = GetWindowTextLengthA(hWnd);
            SendMessageA(hWnd, EM_SETSEL, len, len);
            return 0;
        }
    }
        // Plain hover (no button held) can't change the selection — skip it so
    // we don't force a repaint on every hover pixel (mirrors market.h).
    if (message == WM_MOUSEMOVE && !(wParam & MK_LBUTTON))
        return DefSubclassProc(hWnd, message, wParam, lParam);

    LRESULT res = DefSubclassProc(hWnd, message, wParam, lParam);

    // The Qty edit's blue selection highlight is drawn directly by EDIT via
    // GetDC (not necessarily through WM_PAINT), so mouse drag-select can
    // paint it over the transparent hTotalLabel hint sitting on top. Re-assert
    // the hint after every message that could've changed the selection or
    // focus — same fix as Market_RedrawHintsFor() in market.h.
    if (uIdSubclass == 2) { // Qty edit
        auto redraw = [](HWND h) {
            if (!h || !IsWindowVisible(h)) return;
            InvalidateRect(h, NULL, TRUE);
            UpdateWindow(h);
        };
        HWND hParent = GetParent(hWnd);
        redraw(GetDlgItem(hParent, ID_ORDERS_PRICE_LABEL));
        redraw(GetDlgItem(hParent, ID_ORDERS_QTY_TYPE_LABEL));
        redraw(GetDlgItem(hParent, ID_ORDERS_QTY_TIF_LABEL));
    }

    return res;
}

// Populate the inline edit fields from the given order and make the panel visible.
static void Orders_ShowInlinePanel(HWND hWnd, const TradingAPI::OrderInfo& order) {
    s_editState.orderId     = order.orderId;
    s_editState.partialFill = (order.status == "Partially Filled");
    s_editState.originalQty = order.totalQty;
    s_editState.panelVisible = true;
    s_editState.isUnsent    = (order.status == "Unsent");
    s_editState.conId       = order.conId;
    s_editState.symbol      = order.symbol;
    s_editState.action      = order.action;
    s_editState.isOvernight = order.includeOvernight;
    s_editState.fullStopPrice = order.fullStopPrice;
    s_editState.fullProfitPrice = order.fullProfitPrice;

    HWND hPriceEdit = GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT);
    HWND hOrderQty  = GetDlgItem(hWnd, ID_ORDERS_QTY_EDIT);

    std::string priceStr;
    if (order.price > 0) priceStr = std::format("{:.2f}", order.price);
    else                 priceStr = "0.00";
    if (hPriceEdit) SetWindowTextA(hPriceEdit, priceStr.c_str());

    std::string qtyStr = std::format("{:+}", order.totalQty * (order.action == "BUY" ? 1 : -1));
    if (hOrderQty) SetWindowTextA(hOrderQty, qtyStr.c_str());

    HWND hOrderTypeHint = GetDlgItem(hWnd, ID_ORDERS_HINT_LABEL);
    if (hOrderTypeHint) {
        std::string hint = order.symbol + " " + order.action;// + " " + order.orderType + " " + order.tif;
        SetWindowTextA(hOrderTypeHint, hint.c_str());
        SetCtrlColor(hOrderTypeHint, order.action == "BUY" ? COINS_CLR_GREEN : COINS_CLR_RED);
        InvalidateRect(hOrderTypeHint, NULL, TRUE);
    }

    // Side-color the Qty hints green (BUY) / red (SELL).
    COLORREF qtyClr = order.action == "BUY" ? COINS_CLR_GREEN : COINS_CLR_RED;
    HWND hQtyTifLabel = GetDlgItem(hWnd, ID_ORDERS_QTY_TIF_LABEL);
    if (hQtyTifLabel) {
        SetCtrlColor(hQtyTifLabel, qtyClr);
        SetWindowTextA(hQtyTifLabel, order.tif.c_str());
        InvalidateRect(hQtyTifLabel, NULL, TRUE);
    }
    HWND hQtyTypeLabel = GetDlgItem(hWnd, ID_ORDERS_QTY_TYPE_LABEL);
    if (hQtyTypeLabel) {
        SetCtrlColor(hQtyTypeLabel, qtyClr);
        SetWindowTextA(hQtyTypeLabel, order.orderType.c_str());
        InvalidateRect(hQtyTypeLabel, NULL, TRUE);
    }

    Orders_LayoutPanel(hWnd, true);
    InvalidateRect(GetDlgItem(hWnd, ID_ORDERS_LIST), NULL, TRUE);

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

            SendMessage(hList, WM_SETFONT, (WPARAM)hFont14pt.get(), TRUE);
            SendMessage(ListView_GetHeader(hList), WM_SETFONT, (WPARAM)hFont11pt.get(), TRUE);
            SetWindowSubclass(hList, ListViewNoFlickerProc, 0, 0);
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
            HWND hEditPrice = CreateWindowA("EDIT", "",
                WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER | ES_MULTILINE, 
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_PRICE_EDIT, hInst, NULL);

            HWND hEditQty = CreateWindowA("EDIT", "",
                WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER | ES_NUMBER | ES_MULTILINE,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_QTY_EDIT, hInst, NULL);

            HWND hTotalLabel = CreateWindowExA(WS_EX_TRANSPARENT, "STATIC", "0",
                WS_CHILD | SS_RIGHT | SS_NOPREFIX,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_PRICE_LABEL, hInst, NULL);
            SetCtrlColor(hTotalLabel, COINS_CLR_ORANGE);

            // Two small hints overlaid on the top-left / bottom-left of the Qty edit:
            // the order's time-in-force (tif) and its order type. Side-colored like
            // the order-bar hint (green for BUY, red for SELL).
            HWND hQtyTifLabel = CreateWindowExA(WS_EX_TRANSPARENT, "STATIC", "",
                WS_CHILD | SS_LEFT | SS_NOPREFIX,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_QTY_TIF_LABEL, hInst, NULL);
            SetCtrlColor(hQtyTifLabel, COINS_CLR_GREEN);

            HWND hQtyTypeLabel = CreateWindowExA(WS_EX_TRANSPARENT, "STATIC", "",
                WS_CHILD | SS_LEFT | SS_NOPREFIX,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_QTY_TYPE_LABEL, hInst, NULL);
            SetCtrlColor(hQtyTypeLabel, COINS_CLR_GREEN);

            // Subclass edit fields to intercept keyboard navigation.
            SetWindowSubclass(GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT), EditField_SubclassProc, 1, 0);
            SetWindowSubclass(GetDlgItem(hWnd, ID_ORDERS_QTY_EDIT),   EditField_SubclassProc, 2, 0);

            HWND hOrderTypeHint = CreateWindowA("STATIC", "",
                WS_CHILD | SS_CENTER,
                0, 0, 1, 1, hWnd, (HMENU)ID_ORDERS_HINT_LABEL, hInst, NULL);

            SendMessage(hOrderTypeHint, WM_SETFONT, (WPARAM)hFont16ptbold.get(), TRUE);
            SendMessage(hEditPrice, WM_SETFONT, (WPARAM)hFont16ptbold.get(), TRUE);
            SendMessage(hEditQty, WM_SETFONT, (WPARAM)hFont16ptbold.get(), TRUE);
            SendMessage(hTotalLabel, WM_SETFONT, (WPARAM)hFont11ptbold.get(), TRUE);
            SendMessage(hQtyTifLabel, WM_SETFONT, (WPARAM)hFont11ptbold.get(), TRUE);
            SendMessage(hQtyTypeLabel, WM_SETFONT, (WPARAM)hFont11ptbold.get(), TRUE);

            api().addApiUpdateWindow(hWnd);  
            Orders_Repopulate(hWnd);
            break;
        }

        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = windowOrdersWidth;
            mmi->ptMaxTrackSize.x = windowOrdersWidth;
            return 0;
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
        
        case WM_KEYDOWN: {
            if (wParam == VK_TAB) {
                HWND hPrice  = GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT);
                bool qtyVisible = hPrice && IsWindowVisible(hPrice);
                if (qtyVisible) {
                    SetFocus(hPrice);
                    int len = GetWindowTextLengthA(hPrice);
                    SendMessageA(hPrice, EM_SETSEL, len, len);
                }
                return 0;
            }
            break;
        }

        case WM_NOTIFY: {
            NMHDR* hdr = (NMHDR*)lParam;
            if (hdr->idFrom != ID_ORDERS_LIST) break;

            if (hdr->code == LVN_KEYDOWN) {
                NMLVKEYDOWN* kd = (NMLVKEYDOWN*)lParam;
                if (kd->wVKey == VK_ESCAPE) {
                    if (s_editState.panelVisible && s_editState.orderId != 0) {
                        // Unsent placeholder never reached TWS — nothing to cancel there.
                        HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                        if (s_editState.isUnsent) {
                            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (sel >= 0) ListView_DeleteItem(hList, sel);
                    
                            auto uit = s_unsentOrders.find(s_editState.orderId);
                            if (uit != s_unsentOrders.end())
                                s_unsentOrders.erase(s_editState.orderId);
                            SetOrdersTitle(hWnd);
                        } else {
                            api().cancelOrder(s_editState.orderId);
                        }
                        Orders_HideInlinePanel(hWnd);
                        SetFocus(hList);
                    }
                }
                return 0;
            }

            // ── Selection change: update inline edit panel ────────────────────
            if (hdr->code == LVN_ITEMCHANGED) {
                if (lockHotkeys) break;
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
                bool found = false;
                for (const auto& o : orders) {
                    if (o.orderId == orderId) {
                        found = true;
                        if (Orders_IsEditable(o.status))
                            Orders_ShowInlinePanel(hWnd, o);
                        else
                            Orders_HideInlinePanel(hWnd);
                        break;
                    }
                }
                if (!found) {
                    auto uit = s_unsentOrders.find(orderId);
                    if (uit != s_unsentOrders.end())
                        Orders_ShowInlinePanel(hWnd, uit->second);
                    else
                        Orders_HideInlinePanel(hWnd);
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
                            ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, OCOL_STATUS, statusBuf, sizeof(statusBuf));
                            std::string statusStr(statusBuf);
                            size_t pos = statusStr.rfind(' ');
                            statusStr = (pos == std::string::npos) ? statusStr : statusStr.substr(pos + 1);
                            char buf[16] = {};
                            ListView_GetItemText(hList, (int)cd->nmcd.dwItemSpec, OCOL_SIDE, buf, sizeof(buf));
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
                            if (cd->iSubItem == OCOL_STATUS) {
                                SelectObject(cd->nmcd.hdc, hFont11pt.get());
                            }
                            return CDRF_NEWFONT;
                        }
                        return CDRF_DODEFAULT;
                    }
                }
            }
            break;
        }
        
        case WM_API_UNSENT_ORDER: {
            auto* info = reinterpret_cast<TradingAPI::OrderInfo*>(lParam);
            if (info) {
                s_unsentOrders[info->orderId] = *info;
                HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                if (hList) {
                    int idx = ListView_GetItemCount(hList);
                    LVITEMA lvi = {};
                    lvi.mask     = LVIF_TEXT | LVIF_PARAM;
                    lvi.iItem    = idx;
                    lvi.iSubItem = 0;
                    lvi.lParam   = (LPARAM)info->orderId;
                    lvi.pszText  = (LPSTR)info->action.c_str();
                    ListView_InsertItem(hList, &lvi);
                    ListView_SetItemText(hList, idx, OCOL_SYMBOL, (LPSTR)info->symbol.c_str());
                    std::string quoteStr = std::format("{:.0f} @ {:.2f}", info->totalQty, info->price);
                    ListView_SetItemText(hList, idx, OCOL_QUOTE, (LPSTR)quoteStr.c_str());
                    std::string fullTypeStr;
                    if (info->fullStopPrice > 0 || info->fullProfitPrice > 0) 
                        fullTypeStr = std::format("{:.2f} | {:.2f} ", info->fullStopPrice, info->fullProfitPrice);
                    fullTypeStr += /*info->tif + " " + info->orderType + " " + */info->status;
                    ListView_SetItemText(hList, idx, OCOL_STATUS, (LPSTR)fullTypeStr.c_str());
                    Orders_Repopulate(hWnd);
                }
                delete info;
            }
            break;
        }

        case WM_API_UPDATE: {
            if (api().isMarketDataConnected() && api().isTradingConnected()) {
                Orders_Repopulate(hWnd);
            } else {
                HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                if (hList) {
                    ListView_DeleteAllItems(hList);
                    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
                    RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
                }
            }
            break;
        }
        
        case WM_COMMAND: {
            if (s_editState.panelVisible && (LOWORD(wParam) == ID_ORDERS_PRICE_EDIT || LOWORD(wParam) == ID_ORDERS_QTY_EDIT) && HIWORD(wParam) == EN_CHANGE) {
                UpdatePriceLabel(hWnd);
            }
            break;
        }

        // Paint the Price edit's background: dark green for BUY, dark red for SELL.
        // Anything else (Qty, or the price edit while the panel is hidden) falls
        // through to the default dark/light handling in HandleCommonMessages.
        case WM_CTLCOLOREDIT: {
            HWND hCtrl = (HWND)lParam;
            if (s_editState.panelVisible && hCtrl == GetDlgItem(hWnd, ID_ORDERS_PRICE_EDIT)) {
                HDC hdc = (HDC)wParam;
                bool isBuy = (s_editState.action == "BUY");
                SetTextColor(hdc, DM_TEXT);
                SetBkColor(hdc, isBuy ? COINS_BG_DARK_GREEN : COINS_BG_DARK_RED);
                return (LRESULT)(isBuy ? hBrushDarkGreen : hBrushDarkRed);
            }
            break;
        }

        // hTotalLabel (notional value) and the two Qty-side hints (tif / order
        // type) are transparent hints overlaid on the Qty input, same treatment
        // as the market.h order-bar hint labels.
        case WM_CTLCOLORSTATIC: {
            HWND hCtrl = (HWND)lParam;
            if (hCtrl == GetDlgItem(hWnd, ID_ORDERS_PRICE_LABEL) ||
                hCtrl == GetDlgItem(hWnd, ID_ORDERS_QTY_TIF_LABEL) ||
                hCtrl == GetDlgItem(hWnd, ID_ORDERS_QTY_TYPE_LABEL)) {
                HDC hdc = (HDC)wParam;
                SetBkMode(hdc, TRANSPARENT);
                COLORREF clr = GetCtrlColor(hCtrl);
                SetTextColor(hdc, clr != COLOR_THEME ? clr : COINS_CLR_ORANGE);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            break;
        }

        case WM_DESTROY:
            api().removeApiUpdateWindow(hWnd);
            if (s_editState.panelVisible)
                Orders_HideInlinePanel(hWnd);
            break;
    }
    
    return HandleCommonMessages(hWnd, message, wParam, lParam);
}