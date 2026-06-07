#pragma once

void StartOrders() { StartGenericWindow(ORDERS_CLASS_NAME, "Orders", L"IBKRGatewayClient.Orders", 646, 240); }

#define ID_ORDERS_LIST          9003

static ListViewZoomData OrdersZoomData = { NULL, NULL, 14, "Zoom_Orders" };

// ── Column definitions ────────────────────────────────────────────────────────

struct OrderCol { const char* header; int width; int fmt; };
static const OrderCol orderCols[] = {
    { "Side",          50,  LVCFMT_CENTER},
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
    if (status == "Filled")                           return RGB(196, 110, 43);
    if (status == "Partially Filled")                 return RGB(255, 200, 60);
    if (status == "Cancelled" || status == "Inactive" || status == "PendingCancel")
        return dark ? RGB(130, 130, 130) : RGB(160, 160, 160);
    if (status == "Submitted" || status == "PreSubmitted") {
        if (orderType == "BUY") return RGB(80, 200, 120);
        else if (orderType == "SELL") return RGB(220, 80, 80);
    }
        
    return dark ? DM_TEXT : LM_TEXT;
}

// Rebuilds the entire ListView from the current snapshot.
static void Orders_Repopulate(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
    if (!hList) return;
    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    auto orders = api.getOrdersSorted();
    int submitted = 0;
    int filled = 0;
    for (int i = 0; i < (int)orders.size(); ++i) {
        const auto& o = orders[i];
        char buf[64];

        int col = 0;
        LVITEMA lvi  = {};
        lvi.mask     = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem    = i;
        lvi.iSubItem = col++;
        lvi.lParam   = (LPARAM)o.orderId;  // orderId — used by cancel/edit handlers
        lvi.pszText  = (LPSTR)o.action.c_str();
        ListView_InsertItem(hList, &lvi);

        ListView_SetItemText(hList, i, col++, (LPSTR)o.symbol.c_str());

        if (o.price > 0)
            snprintf(buf, sizeof(buf), "%.0f @ %.2f", o.totalQty, o.price);
        else
            snprintf(buf, sizeof(buf), "%.0f @ MKT", o.totalQty);
        ListView_SetItemText(hList, i, col++, buf);

        if (o.avgFillPx > 0)
            snprintf(buf, sizeof(buf), "%.0f @ %.2f", o.filledQty, o.avgFillPx);
        else
            snprintf(buf, sizeof(buf), "-- @ --");
        ListView_SetItemText(hList, i, col++, buf);
        
        std::string fullTypeStr = o.tif + " " + o.orderType + " " + o.status;
        ListView_SetItemText(hList, i, col++, (LPSTR)fullTypeStr.c_str());

        // ListView_SetItemText(hList, i, col++, (LPSTR)o.time.c_str());

        if (o.status == "Submitted" || o.status == "PreSubmitted") submitted++;
        if (o.status == "Filled") filled++;
    }   

    SetWindowTextA(hWnd, ("Orders: " + std::to_string(submitted) + " Submitted | " + std::to_string(filled) + " Filled").c_str());

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

// ── Edit-Order popup ──────────────────────────────────────────────────────────
// A lightweight non-modal popup with two edit boxes (Price / Qty).
// ENTER confirms and calls api.modifyOrder(); ESCAPE closes without action.
// Only one popup may be open at a time.

struct EditOrderCtx {
    int    orderId;
    HWND   hPriceEdit;
    HWND   hQtyEdit;       // NULL when partialFill — qty row is omitted
    bool   partialFill;    // true → show price only; use originalQty for modifyOrder
    double originalQty;    // captured from order when partialFill is true
};

// Forward ENTER / ESC from an edit control up to the popup parent, handle TAB and Arrows.
static LRESULT CALLBACK EditField_SubclassProc(HWND hWnd, UINT message, WPARAM wParam,
                                               LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (message == WM_GETDLGCODE) {
        LRESULT res = DefSubclassProc(hWnd, message, wParam, lParam);
        // Explicitly tell Windows this control wants to process TAB and ARROW keys
        return res | DLGC_WANTTAB | DLGC_WANTARROWS;
    }

    if (message == WM_KEYDOWN) {
        if (wParam == VK_RETURN || wParam == VK_ESCAPE) {
            SendMessage(GetParent(hWnd), WM_KEYDOWN, wParam, lParam);
            return 0;
        }

        if (wParam == VK_TAB) {
            HWND hParent = GetParent(hWnd);
            EditOrderCtx* ctx = (EditOrderCtx*)GetWindowLongPtr(hParent, GWLP_USERDATA);
            if (ctx && ctx->hQtyEdit) {
                HWND hNext = (hWnd == ctx->hPriceEdit) ? ctx->hQtyEdit : ctx->hPriceEdit;
                SetFocus(hNext);
                // Place caret at end, no selection
                int len = GetWindowTextLengthA(hNext);
                SendMessageA(hNext, EM_SETSEL, len, len);
            }
            return 0;
        }

        if (wParam == VK_UP || wParam == VK_DOWN) {
            char buf[32] = {};
            GetWindowTextA(hWnd, buf, sizeof(buf));

            double val  = atof(buf);
            double step = (uIdSubclass == 1) ? 0.01 : 1.0;  // price vs qty
            if (wParam == VK_UP) {
                val += step;
            } else {
                val -= step;
            }

            // Prevent negative pricing/quantities
            if (val < 0.0) val = 0.0;

            snprintf(buf, sizeof(buf), (uIdSubclass == 1) ? "%.2f" : "%.0f", val);
            SetWindowTextA(hWnd, buf);

            // Place caret at end, no selection
            int len = GetWindowTextLengthA(hWnd);
            SendMessageA(hWnd, EM_SETSEL, len, len);
            return 0;
        }
    }
    return DefSubclassProc(hWnd, message, wParam, lParam);
}

static HWND s_hEditOrderPopup = NULL;   // at most one popup at a time

static LRESULT CALLBACK WndProcEditOrder(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    EditOrderCtx* ctx = (EditOrderCtx*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (message) {

        case WM_CREATE: {
            ctx = (EditOrderCtx*)((LPCREATESTRUCT)lParam)->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);

            HINSTANCE hInst = GetModuleHandle(NULL);
            int lx = 17, ex = 90, ew = 112, fh = 30;

            // Row 1 — Price
            int y = 14;
            CreateWindowA("STATIC", "Price:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                lx, y + 3, 45, 25, hWnd, NULL, hInst, NULL);
            ctx->hPriceEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER,
                ex, y, ew, fh, hWnd, (HMENU)1, hInst, NULL);

            // Row 2 — Qty (omitted for Partially Filled orders)
            if (!ctx->partialFill) {
                y += 38;
                CreateWindowA("STATIC", "Qty:", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                    lx, y + 3, 45, 25, hWnd, NULL, hInst, NULL);
                ctx->hQtyEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER,
                    ex, y, ew, fh, hWnd, (HMENU)2, hInst, NULL);
                SetWindowSubclass(ctx->hQtyEdit, EditField_SubclassProc, 2, 0);
            }

            // Subclass price field; qty field only when it exists.
            SetWindowSubclass(ctx->hPriceEdit, EditField_SubclassProc, 1, 0);

            EnumChildWindows(hWnd, SetFontCallback, (LPARAM)OrdersZoomData.hFont);
            SetFocus(ctx->hPriceEdit);
            break;
        }

        case WM_KEYDOWN: {
            if (!ctx) break;
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }
            if (wParam == VK_RETURN) {
                char pBuf[32] = {}, qBuf[32] = {};
                GetWindowTextA(ctx->hPriceEdit, pBuf, sizeof(pBuf));
                double price = atof(pBuf);
                double qty;
                if (ctx->hQtyEdit) {
                    GetWindowTextA(ctx->hQtyEdit, qBuf, sizeof(qBuf));
                    qty = atof(qBuf);
                } else {
                    qty = ctx->originalQty;   // Partially Filled: keep existing qty
                }
                if (qty > 0)
                    api.modifyOrder(ctx->orderId, price, qty);
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_NCDESTROY:
            s_hEditOrderPopup = NULL;
            delete ctx;
            SetFocus(GetDlgItem(GetParent(hWnd), ID_ORDERS_LIST));
            return 0;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}

static void Orders_ShowEditPopup(HWND hParent, const TradingAPI::OrderInfo& order) {
    // Guard: only one popup at a time.
    if (s_hEditOrderPopup && IsWindow(s_hEditOrderPopup)) {
        SetForegroundWindow(s_hEditOrderPopup);
        return;
    }

    // Don't allow editing orders that are already terminal.
    const std::string& st = order.status;
    if (st == "Filled" || st == "Cancelled" || st == "Inactive" || st == "PendingCancel") return;

    auto* ctx    = new EditOrderCtx{};
    ctx->orderId     = order.orderId;
    ctx->partialFill = (st == "Partially Filled");
    ctx->originalQty = order.totalQty;

    char title[80];
    snprintf(title, sizeof(title), "Edit %s: %s %s %s", order.symbol.c_str(), order.tif.c_str(), order.orderType.c_str(), order.action.c_str());

    // Center over the parent window.
    RECT pr;
    GetWindowRect(hParent, &pr);
    // Partially Filled orders only show the Price row → shorter popup.
    int w = 222, h = ctx->partialFill ? 80 : 118;
    int x = pr.left + (pr.right  - pr.left - w) / 2;
    int y = pr.top  + (pr.bottom - pr.top  - h) / 2;

    HWND hPop = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        ORDERS_EDIT_CLASS_NAME, title,
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, w, h,
        hParent, NULL, GetModuleHandle(NULL), ctx);

    if (!hPop) { delete ctx; return; }
    s_hEditOrderPopup = hPop;

    // Pre-fill fields.
    char buf[32];
    if (order.price > 0) snprintf(buf, sizeof(buf), "%.2f", order.price);
    else                  snprintf(buf, sizeof(buf), "0.00");
    SetWindowTextA(ctx->hPriceEdit, buf);

    snprintf(buf, sizeof(buf), "%.0f", order.totalQty);
    if (ctx->hQtyEdit)
        SetWindowTextA(ctx->hQtyEdit, buf);
    
    ShowWindow(hPop, SW_SHOW);
    UpdateWindow(hPop);
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
            Orders_Repopulate(hWnd);
            break;
        }

        // ── Dark mode: paint the ListView background and items ────────────────────
        case WM_NOTIFY: {
            NMHDR* hdr = (NMHDR*)lParam;
            if (hdr->idFrom != ID_ORDERS_LIST) break;
            if (hdr->code == LVN_KEYDOWN) {
                NMLVKEYDOWN* kd = (NMLVKEYDOWN*)lParam;
                if (kd->wVKey == VK_DELETE) {
                    HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    if (sel >= 0) {
                        LVITEMA lvi = {};
                        lvi.mask  = LVIF_PARAM;
                        lvi.iItem = sel;
                        if (ListView_GetItem(hList, &lvi)) {
                            api.cancelOrder((int)lvi.lParam);
                        }
                    }
                }
                return 0;  // ← was: break (fell through, but also swallowed return value)
            }

            // NM_DBLCLK sends NMITEMACTIVATE; NM_RETURN sends only NMHDR — handle separately
            if (hdr->code == NM_DBLCLK) {
                NMITEMACTIVATE* ia = (NMITEMACTIVATE*)lParam;
                if (ia->iItem < 0) break;
                HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                LVITEMA lvi = {};
                lvi.mask  = LVIF_PARAM;
                lvi.iItem = ia->iItem;
                if (ListView_GetItem(hList, &lvi)) {
                    int orderId = (int)lvi.lParam;
                    auto orders = api.getOrdersSorted();
                    for (const auto& o : orders)
                        if (o.orderId == orderId) { Orders_ShowEditPopup(hWnd, o); break; }
                }
                return 0;
            }

            if (hdr->code == NM_RETURN) {
                // NM_RETURN gives us only NMHDR — get selection manually
                HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                if (sel >= 0) {
                    LVITEMA lvi = {};
                    lvi.mask  = LVIF_PARAM;
                    lvi.iItem = sel;
                    if (ListView_GetItem(hList, &lvi)) {
                        int orderId = (int)lvi.lParam;
                        auto orders = api.getOrdersSorted();
                        for (const auto& o : orders)
                            if (o.orderId == orderId) { Orders_ShowEditPopup(hWnd, o); break; }
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
            if (api.isMarketDataConnected() && api.isTradingConnected()) {
                Orders_Repopulate(hWnd);
            } else {
                HWND hList = GetDlgItem(hWnd, ID_ORDERS_LIST);
                if (hList) ListView_DeleteAllItems(hList);
            }
            break;
        }
        
        case WM_DESTROY:
            if (s_hEditOrderPopup && IsWindow(s_hEditOrderPopup))
                DestroyWindow(s_hEditOrderPopup);
            api.unsetOrdersWindow();
            api.removeApiUpdateWindow(hWnd);
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