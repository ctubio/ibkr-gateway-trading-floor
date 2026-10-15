#pragma once

struct CaptionCache {
    HDC dc{};
    HBITMAP bmp{}, old{};
    void* bits{};
    int w{}, h{};
    HTHEME theme{};

    void Free(bool alsoTheme) {
        if (dc) {
            if (old) SelectObject(dc, old);
            if (bmp) DeleteObject(bmp);
            DeleteDC(dc);
        } else if (bmp) {
            DeleteObject(bmp);
        }
        dc = nullptr;
        bmp = old = nullptr;
        bits = nullptr;
        w = h = 0;
        if (alsoTheme && theme) {
            CloseThemeData(theme);
            theme = nullptr;
        }
    }

    bool Ensure(HDC ref, int width, int height) {
        if (width <= 0 || height <= 0) return false;
        if (dc && bmp && bits && w == width && h == height) return true;

        Free(false);
        dc = CreateCompatibleDC(ref);
        if (!dc) return false;

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = width;
        bi.bmiHeader.biHeight = -height;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        bmp = CreateDIBSection(ref, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!bmp || !bits) {
            Free(false);
            return false;
        }
        HGDIOBJ previous = SelectObject(dc, bmp);
        if (!previous || previous == HGDI_ERROR) {
            Free(false);
            return false;
        }
        old = (HBITMAP)previous;
        w = width;
        h = height;
        return true;
    }
};

static int Caption_FrameY(HWND hWnd) {
    UINT dpi = GetDpiForWindow(hWnd);
    return GetSystemMetricsForDpi(SM_CYFRAME, dpi) +
           GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

static int Caption_H(HWND hWnd) {
    UINT dpi = GetDpiForWindow(hWnd);
    int height = GetSystemMetricsForDpi(SM_CYCAPTION, dpi) + Caption_FrameY(hWnd);
    return std::max(1, height - MulDiv(4, dpi, 96));
}

static void Caption_UpdateMargins(HWND hWnd) {
    MARGINS margins = { 0, 0, Caption_H(hWnd), 0 };
    DwmExtendFrameIntoClientArea(hWnd, &margins);
}

static void Caption_DrawIcon(HWND hWnd, HDC dc, void* bits, int width, int height,
                             int verticalOffset = 0) {
    HICON icon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
    if (!icon) icon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
    if (!icon) return;

    const int iconW = GetSystemMetrics(SM_CXSMICON);
    const int iconH = GetSystemMetrics(SM_CYSMICON);
    const int iconX = 8;
    const int iconY = verticalOffset + (height - verticalOffset - iconH) / 2;
    DrawIconEx(dc, iconX, iconY, icon, iconW, iconH, 0, NULL, DI_NORMAL);

    DWORD* pixels = (DWORD*)bits;
    for (int y = std::max(0, iconY); y < std::min(height, iconY + iconH); ++y) {
        for (int x = iconX; x < std::min(width, iconX + iconW); ++x) {
            DWORD& pixel = pixels[static_cast<size_t>(y) * width + x];
            if ((pixel >> 24) == 0 && (pixel & 0x00FFFFFF) != 0)
                pixel |= 0xFF000000;
        }
    }
}

static bool Caption_HandleNcMessages(HWND hWnd, UINT message, WPARAM wParam,
                                     LPARAM lParam, LRESULT& result) {
    switch (message) {
    case WM_NCCALCSIZE:
        if (wParam) {
            auto* params = (NCCALCSIZE_PARAMS*)lParam;
            LONG origTop = params->rgrc[0].top;
            DefWindowProc(hWnd, message, wParam, lParam);
            params->rgrc[0].top = origTop;
            result = 0;
            return true;
        }
        return false;

    case WM_NCHITTEST: {
        if (DwmDefWindowProc(hWnd, message, wParam, lParam, &result)) return true;
        POINT clientPt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hWnd, &clientPt);
        if (clientPt.y >= 0 && clientPt.y < Caption_H(hWnd)) {
            result = HTCAPTION;
            return true;
        }
        return false;
    }

    case WM_NCMOUSEMOVE:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONUP:
    case WM_NCMOUSELEAVE:
        return DwmDefWindowProc(hWnd, message, wParam, lParam, &result) != FALSE;

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        RECT caption = rc;
        caption.bottom = Caption_H(hWnd);
        FillRect(hdc, &caption, (HBRUSH)GetStockObject(BLACK_BRUSH));
        RECT rest = rc;
        rest.top = caption.bottom;
        FillRect(hdc, &rest, darkMode ? hDarkBrush : (HBRUSH)(COLOR_BTNFACE + 1));
        result = 1;
        return true;
    }
    }
    return false;
}