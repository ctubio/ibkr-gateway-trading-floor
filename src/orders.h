#pragma once

void startOrders() { startGenericWindow(ORDERS_CLASS_NAME, "Orders", L"IBKRGatewayClient.Orders", 700, 400); }

// Order Table Columns
struct OrderCol { const char* header; int width; };
static OrderCol orderCols[] = {
    { "Time", 80 },
    { "Symbol", 100 },
    { "Price", 80 },
    { "Amount", 80 },
    { "Status", 100 },
    { "Exchange", 100 },
};
static const int ORDER_COL_COUNT = sizeof(orderCols) / sizeof(orderCols[0]);

LRESULT CALLBACK WndProcOrders(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Create a simple listview or custom table here. 
        // For consistency with the project's style (using STATIC/EDIT), 
        // we'll use a ListView control for the table.
        HWND hList = CreateWindowA("SysListView32", "", 
            WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER,
            0, 0, 700, 400, 
            hWnd, (HMENU)1, NULL, NULL);
        
        LVCOLUMN lvc = { 0 };
        lvc.mask = LVCF_WIDTH | LVCF_TEXT;
        for (int i = 0; i < ORDER_COL_COUNT; i++) {
            lvc.cx = orderCols[i].width;
            lvc.pszText = (LPSTR)orderCols[i].header;
            ListView_InsertColumn(hList, i, &lvc);
        }

        api.setOrdersWindow(hWnd);
        break;
    }

    case WM_ORDERS_UPDATE: {
        HWND hList = GetDlgItem(hWnd, 1);
        if (!hList) break;

        ListView_DeleteAllItems(hList);

        std::vector<TradingAPI::OrderInfo> sortedOrders;
        {
            std::lock_guard<std::mutex> lock(api.getOrdersMutex());
            for (auto const& [id, info] : api.getOrdersMap()) {
                sortedOrders.push_back(info);
            }
        }


        // Sorting logic: 
        // 1. Opened (Submitted/Pending) -> Active (Filled/Partially Filled) -> Completed (Filled/Cancelled)
        // 2. Secondary sort by time (Newer first)
        auto getStatusPriority = [](const std::string& status) {
            if (status == "Submitted" || status == "Pending") return 0;
            if (status == "Partially Filled") return 1;
            if (status == "Filled" || status == "Cancelled") return 2;
            return 3;
        };

        std::sort(sortedOrders.begin(), sortedOrders.end(), [&](const TradingAPI::OrderInfo& a, const TradingAPI::OrderInfo& b) {
            int pA = getStatusPriority(a.status);
            int pB = getStatusPriority(b.status);
            if (pA != pB) return pA < pB;
            return a.timestamp > b.timestamp;
        });

        for (int i = 0; i < (int)sortedOrders.size(); i++) {
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.iSubItem = 0;
            
            char buf[64];
            snprintf(buf, sizeof(buf), "%s", sortedOrders[i].time.c_str());
            lvi.pszText = buf;
            ListView_InsertItem(hList, &lvi);
            
            // Sub-items
            ListView_SetItemText(hList, i, 1, (LPSTR)sortedOrders[i].symbol.c_str());
            
            snprintf(buf, sizeof(buf), "%.2f", sortedOrders[i].price);
            ListView_SetItemText(hList, i, 2, buf);
            
            snprintf(buf, sizeof(buf), "%.2f", sortedOrders[i].totalAmount);
            ListView_SetItemText(hList, i, 3, buf);
            
            ListView_SetItemText(hList, i, 4, (LPSTR)sortedOrders[i].status.c_str());
            ListView_SetItemText(hList, i, 5, (LPSTR)sortedOrders[i].exchange.c_str());
        }
        break;
    }

    case WM_DESTROY:
        break;
    }

    return HandleCommonMessages(hWnd, message, wParam, lParam);
}