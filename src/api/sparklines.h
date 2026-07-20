#pragma once

struct SparkPoint {
    ULONGLONG date; 
    double price;
};

class Sparkline {
private:
    std::vector<SparkPoint> data;

    // NEW: long-lived history (~65 min) used only for the 10/20/30/40/50-min reference dots.
    // Kept completely separate from `data` so the existing sparkline logic is untouched.
    std::vector<SparkPoint> priceHistory;

    // Equivalent to your d3_scale_linear
    float MapScale(double value, double minDomain, double maxDomain, float minRange, float maxRange) {
        if (maxDomain == minDomain) return minRange + (maxRange - minRange) / 2.0f;
        return minRange + (float)((value - minDomain) / (maxDomain - minDomain)) * (maxRange - minRange);
    }
    // NEW: maps a % price change into a color (gray -> saturated green/red)
    // and a radius (small -> large), both scaled by magnitude.
    void GetDotStyle(double pctChange, float minR, float maxR,
                      Gdiplus::Color& outColor, float& outRadius) const {
        const double maxPct = 0.5; // % change at which color/size reach full intensity — tune as needed
        double mag = fabs(pctChange);
        double t = mag / maxPct;
        if (t > 1.0) t = 1.0;

        outRadius = minR + (float)t * (maxR - minR);

        // Base neutral gray, blended toward vivid green or red as magnitude grows
        int grayC = 150;
        if (pctChange > 0.0) {
            int r = (int)(grayC + t * (1   - grayC));
            int g = (int)(grayC + t * (166 - grayC));
            int b = (int)(grayC + t * (1   - grayC));
            outColor = Gdiplus::Color(255, r, g, b);
        } else if (pctChange < 0.0) {
            int r = (int)(grayC + t * (220 - grayC));
            int g = (int)(grayC + t * (0   - grayC));
            int b = (int)(grayC + t * (0   - grayC));
            outColor = Gdiplus::Color(255, r, g, b);
        } else {
            outColor = Gdiplus::Color(255, grayC, grayC, grayC);
        }
    }
    // NEW: finds the price closest to (now - minutesAgo) in priceHistory.
    // Returns false if we don't yet have history reaching that far back
    // (this is what makes the dots appear one by one as time passes).
    bool GetPriceAgo(ULONGLONG now, ULONGLONG minutesAgo, double& outPrice) const {
        ULONGLONG minMs = minutesAgo * 60000ULL;
        if (now < minMs) return false;
        ULONGLONG target = now - minMs;
        if (priceHistory.empty() || priceHistory.front().date > target) return false;

        size_t bestIdx = 0;
        ULONGLONG bestDiff = (priceHistory[0].date > target)
            ? (priceHistory[0].date - target) : (target - priceHistory[0].date);
        for (size_t i = 1; i < priceHistory.size(); ++i) {
            ULONGLONG diff = (priceHistory[i].date > target)
                ? (priceHistory[i].date - target) : (target - priceHistory[i].date);
            if (diff < bestDiff) { bestDiff = diff; bestIdx = i; }
        }
        outPrice = priceHistory[bestIdx].price;
        return true;
    }

public:
    void AddPrice(double price) {
        ULONGLONG now = GetTickCount64();

        // 1. If price hasn't changed, ignore (matching your JS)
        if (!data.empty() && data.back().price == price) return;

        // 2. JS Logic: if 2nd to last point is newer than 30s ago, pop the last point
        if (data.size() > 1 && data[data.size() - 2].date > now - 30000) {
            data.pop_back(); 
        }

        // 3. Add new data
        data.push_back({now, price});

        // 4. Max array size of 21
        if (data.size() > 21) {
            data.erase(data.begin()); 
        }

        // NEW: maintain the separate long-term history used for the reference dots
        if (priceHistory.empty() || priceHistory.back().price != price) {
            priceHistory.push_back({ now, price });
        }
        const ULONGLONG maxAge = 65ULL * 60ULL * 1000ULL; // keep ~65 minutes
        while (!priceHistory.empty() && now > maxAge && priceHistory.front().date < now - maxAge) {
            priceHistory.erase(priceHistory.begin());
        }
    }

    void Draw(HDC hdc, RECT clientRect, float W, float H) {
        if (data.size() < 2) return;

        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        // NEW: reserve a small strip on the right for the 5 reference dots.
        // This is the only change to the existing line drawing: it uses `lineW`
        // instead of `W` when mapping x, so the line stops a bit short of the edge.
        const float dotAreaWidth = 15.0f;
        float lineW = (W - dotAreaWidth > 4.0f) ? (W - dotAreaWidth) : W;

        // Find Min/Max Domains
        ULONGLONG minTime = data[0].date;
        ULONGLONG maxTime = data.back().date;
        
        double minPrice = data[0].price;
        double maxPrice = data[0].price;
        for (const auto& p : data) {
            if (p.price < minPrice) minPrice = p.price;
            if (p.price > maxPrice) maxPrice = p.price;
        }

        // Prevent division by zero if all prices/times are identical
        if (minTime == maxTime) maxTime++;
        if (minPrice == maxPrice) { minPrice -= 1.0; maxPrice += 1.0; }

        // Map data to Gdiplus screen coordinates
        std::vector<Gdiplus::PointF> points(data.size());
        for (size_t i = 0; i < data.size(); ++i) {
            // X spans from 0 to lineW (was W)
            float x = MapScale(data[i].date, minTime, maxTime, 0, lineW);
            // Y is inverted (0 is top, H is bottom in Windows)
            float y = MapScale(data[i].price, minPrice, maxPrice, H, 1);
            
            points[i] = Gdiplus::PointF(clientRect.left + x, clientRect.top + y);
        }

        // Create the Canvas Gradient (Alpha 0.7 * 255 = ~178)
        Gdiplus::PointF pntTop(0.0f, clientRect.top);
        Gdiplus::PointF pntBottom(0.0f, clientRect.top+H+2); 
        Gdiplus::LinearGradientBrush brush(pntTop, pntBottom, Gdiplus::Color(255,0,0,0), Gdiplus::Color(255,0,0,0)); 
        
        // Match JS color stops: 0 = Green, 0.20 = Orange, 0.25 = Red
        Gdiplus::Color colors[] = {
            Gdiplus::Color(178, 1, 166, 1),   // Green
            Gdiplus::Color(178, 255, 165, 0), // Orange
            Gdiplus::Color(178, 255, 0, 0)    // Red
        };
        float positions[] = { 0.0f, 0.50f, 1.0f }; // Clamped red to the bottom
        brush.SetInterpolationColors(colors, positions, 3);

        // Match JS lineWidth = 3
        Gdiplus::Pen pen(&brush, 3.0f);
        pen.SetLineJoin(Gdiplus::LineJoinRound);

        graphics.DrawLines(&pen, points.data(), (INT)points.size());

        // NEW: draw the 5 reference dots (10/20/30/40/50 min ago), top to bottom.
        // Both color saturation and dot size scale with the magnitude of % change.
        ULONGLONG now = GetTickCount64();
        double lastPrice = data.back().price;
        static const int minutesAgo[5] = { 10, 20, 30, 40, 50 };
        const float minRadius = 1.5f;
        const float maxRadius = 4.5f;
        float dotX = clientRect.left + W - maxRadius - 1.0f;

        for (int i = 0; i < 5; ++i) {
            double histPrice;
            if (!GetPriceAgo(now, minutesAgo[i], histPrice)) continue; // not enough history yet
            float dotY = clientRect.top + H * ((i + 0.5f) / 5.0f);

            double pctChange = (histPrice != 0.0) ? ((lastPrice - histPrice) / histPrice * 100.0) : 0.0;

            Gdiplus::Color dotColor;
            float dotRadius;
            GetDotStyle(pctChange, minRadius, maxRadius, dotColor, dotRadius);

            Gdiplus::SolidBrush dotBrush(dotColor);
            graphics.FillEllipse(&dotBrush, dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
        }
    }
};

class MiniSparkline {
private:
    struct MiniSparkPoint { ULONGLONG date; double price; };
    std::vector<MiniSparkPoint> data;

    // NEW: same idea as in Sparkline, a separate long-lived history for the dots
    std::vector<MiniSparkPoint> priceHistory;

    float MapScale(double value, double minD, double maxD, float minR, float maxR) const {
        if (maxD == minD) return minR + (maxR - minR) / 2.0f;
        return minR + (float)((value - minD) / (maxD - minD)) * (maxR - minR);
    }
    // NEW: maps a % price change into a color (gray -> saturated green/red)
    // and a radius (small -> large), both scaled by magnitude.
    void GetDotStyle(double pctChange, float minR, float maxR,
                      Gdiplus::Color& outColor, float& outRadius) const {
        const double maxPct = 0.5; // % change at which color/size reach full intensity — tune as needed
        double mag = fabs(pctChange);
        double t = mag / maxPct;
        if (t > 1.0) t = 1.0;

        outRadius = minR + (float)t * (maxR - minR);

        int grayC = 150;
        if (pctChange > 0.0) {
            int r = (int)(grayC + t * (1   - grayC));
            int g = (int)(grayC + t * (166 - grayC));
            int b = (int)(grayC + t * (1   - grayC));
            outColor = Gdiplus::Color(255, r, g, b);
        } else if (pctChange < 0.0) {
            int r = (int)(grayC + t * (220 - grayC));
            int g = (int)(grayC + t * (0   - grayC));
            int b = (int)(grayC + t * (0   - grayC));
            outColor = Gdiplus::Color(255, r, g, b);
        } else {
            outColor = Gdiplus::Color(255, grayC, grayC, grayC);
        }
    }
    // NEW
    bool GetPriceAgo(ULONGLONG now, ULONGLONG minutesAgo, double& outPrice) const {
        ULONGLONG minMs = minutesAgo * 60000ULL;
        if (now < minMs) return false;
        ULONGLONG target = now - minMs;
        if (priceHistory.empty() || priceHistory.front().date > target) return false;

        size_t bestIdx = 0;
        ULONGLONG bestDiff = (priceHistory[0].date > target)
            ? (priceHistory[0].date - target) : (target - priceHistory[0].date);
        for (size_t i = 1; i < priceHistory.size(); ++i) {
            ULONGLONG diff = (priceHistory[i].date > target)
                ? (priceHistory[i].date - target) : (target - priceHistory[i].date);
            if (diff < bestDiff) { bestDiff = diff; bestIdx = i; }
        }
        outPrice = priceHistory[bestIdx].price;
        return true;
    }

public:
    void AddPrice(double price) {
        ULONGLONG now = GetTickCount64();
        if (!data.empty() && data.back().price == price) return;
        // If 2nd-to-last point is newer than 30 s ago, replace the last point.
        if (data.size() > 1 && data[data.size() - 2].date > now - 30000)
            data.pop_back();
        data.push_back({ now, price });
        if (data.size() > 21)
            data.erase(data.begin());

        // NEW: maintain long-term history for the reference dots
        if (priceHistory.empty() || priceHistory.back().price != price) {
            priceHistory.push_back({ now, price });
        }
        const ULONGLONG maxAge = 65ULL * 60ULL * 1000ULL;
        while (!priceHistory.empty() && now > maxAge && priceHistory.front().date < now - maxAge) {
            priceHistory.erase(priceHistory.begin());
        }
    }

    // Draw the sparkline into the sub-item bounding rect.
    // Leaves a small left margin so the text (position number) is still visible.
    void Draw(HDC hdc, const RECT& cellRect) const {
        if (data.size() < 2) return;

        // Reserve the right portion for the numeric text; sparkline fills the rest.
        const int rightMargin = -20;  // px gap from cell right edge
        const int topPad     = 3;
        const int botPad     = 3;

        float W = (float)(cellRect.right - cellRect.left - rightMargin);
        float H = (float)(cellRect.bottom - cellRect.top  - topPad     - botPad);
        if (W < 4 || H < 4) return;

        float ox = (float)(cellRect.left + rightMargin);
        float oy = (float)(cellRect.top  + topPad);

        // NEW: reserve a small strip on the right for the 5 reference dots
        const float dotAreaWidth = 18.0f;
        float lineW = (W - dotAreaWidth > 4.0f) ? (W - dotAreaWidth) : W;

        Gdiplus::Graphics g(hdc);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

        ULONGLONG minT = data.front().date, maxT = data.back().date;
        double minP = data[0].price, maxP = data[0].price;
        for (const auto& p : data) {
            if (p.price < minP) minP = p.price;
            if (p.price > maxP) maxP = p.price;
        }
        if (minT == maxT) maxT++;
        if (minP == maxP) { minP -= 0.5; maxP += 0.5; }

        std::vector<Gdiplus::PointF> pts(data.size());
            for (size_t i = 0; i < data.size(); ++i) {
                float x = MapScale((double)data[i].date,  (double)minT, (double)maxT, 0, lineW); // was W
                float y = MapScale(data[i].price,          minP,         maxP,         H, 1);
                pts[i]  = Gdiplus::PointF(ox + x, oy + y);
            }

        // Gradient: green (top/recent-high) → orange → red (bottom/loss)
        Gdiplus::LinearGradientBrush brush(
            Gdiplus::PointF(0.f, oy),
            Gdiplus::PointF(0.f, oy + H + 1),
            Gdiplus::Color(200, 1, 166, 1),
            Gdiplus::Color(200, 1, 166, 1));
        Gdiplus::Color  cols[] = {
            Gdiplus::Color(200,   1, 166,   1),  // green
            Gdiplus::Color(200, 255, 165,   0),  // orange
            Gdiplus::Color(200, 255,   0,   0)   // red
        };
        float stops[] = { 0.0f, 0.50f, 1.0f };
        brush.SetInterpolationColors(cols, stops, 3);

        Gdiplus::Pen pen(&brush, 3.0f);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawLines(&pen, pts.data(), (INT)pts.size());

        // NEW: draw the 5 reference dots (10/20/30/40/50 min ago), top to bottom
        ULONGLONG now = GetTickCount64();
        double lastPrice = data.back().price;
        static const int minutesAgo[5] = { 10, 20, 30, 40, 50 };
        const float minRadius = 1.0f;
        const float maxRadius = 2.8f;
        float dotX = ox + W - maxRadius - 1.0f;

        for (int i = 0; i < 5; ++i) {
            double histPrice;
            if (!GetPriceAgo(now, minutesAgo[i], histPrice)) continue;
            float dotY = oy + H * ((i + 0.5f) / 5.0f);

            double pctChange = (histPrice != 0.0) ? ((lastPrice - histPrice) / histPrice * 100.0) : 0.0;

            Gdiplus::Color dotColor;
            float dotRadius;
            GetDotStyle(pctChange, minRadius, maxRadius, dotColor, dotRadius);

            Gdiplus::SolidBrush dotBrush(dotColor);
            g.FillEllipse(&dotBrush, dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
        }
    }

    bool HasData() const { return data.size() >= 2; }
};
