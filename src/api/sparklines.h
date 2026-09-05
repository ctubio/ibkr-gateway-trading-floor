#pragma once

struct SparkPoint {
    ULONGLONG date; 
    double price;
};

class Sparkline {
private:
    std::vector<SparkPoint> data;

    // Graphics is tied to the paint HDC, but the gradient resources are not.
    // The sparkline geometry is fixed for the lifetime of its owner, so these
    // are initialized once and reused for every paint.
    mutable std::unique_ptr<Gdiplus::LinearGradientBrush> gradientBrush;
    mutable std::unique_ptr<Gdiplus::Pen> gradientPen;

    void PrepareGradient(float height, float originY) const {
        if (!gradientBrush) {
            gradientBrush = std::make_unique<Gdiplus::LinearGradientBrush>(
                Gdiplus::PointF(0.0f, originY),
                Gdiplus::PointF(0.0f, originY + height + 2.0f),
                Gdiplus::Color(255, 0, 0, 0),
                Gdiplus::Color(255, 0, 0, 0));
            gradientBrush->SetInterpolationColors(sparkColors, sparkStops, 3);
            gradientPen = std::make_unique<Gdiplus::Pen>(gradientBrush.get(), 3.0f);
            gradientPen->SetLineJoin(Gdiplus::LineJoinRound);
        }
    }

    // NEW: long-lived history (~65 min) used only for the 10/20/30/40/50-min reference dots.
    // Kept completely separate from `data` so the existing sparkline logic is untouched.
    // PERF: std::deque instead of std::vector — AddPrice() prunes stale entries off
    // the *front* every call. On a vector that's an O(n) shift per erase (and this
    // loop can erase repeatedly in one call); on a deque, pop_front() is O(1).
    std::deque<SparkPoint> priceHistory;

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
    //
    // PERF: priceHistory is appended in strictly non-decreasing time order
    // (every AddPrice() call timestamps with GetTickCount64()), so instead of
    // scanning every entry to find the closest one (O(n), and this runs 5x
    // per Draw() call — once per reference dot — at ~30 FPS per open Market
    // window), binary-search for the insertion point and only compare the
    // two neighbors around it. O(log n) instead of O(n).
    bool GetPriceAgo(ULONGLONG now, ULONGLONG minutesAgo, double& outPrice) const {
        ULONGLONG minMs = minutesAgo * 60000ULL;
        if (now < minMs) return false;
        ULONGLONG target = now - minMs;
        if (priceHistory.empty() || priceHistory.front().date > target) return false;

        auto it = std::lower_bound(priceHistory.begin(), priceHistory.end(), target,
            [](const SparkPoint& p, ULONGLONG t) { return p.date < t; });

        size_t bestIdx;
        if (it == priceHistory.end()) {
            // target is at/after the newest sample — nothing after it to compare
            bestIdx = priceHistory.size() - 1;
        } else if (it == priceHistory.begin()) {
            // target is at/before the oldest sample
            bestIdx = 0;
        } else {
            size_t idxAfter  = (size_t)(it - priceHistory.begin());
            size_t idxBefore = idxAfter - 1;
            ULONGLONG diffAfter  = it->date - target;
            ULONGLONG diffBefore = target - priceHistory[idxBefore].date;
            bestIdx = (diffAfter < diffBefore) ? idxAfter : idxBefore;
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
        // PERF: pop_front() on a deque is O(1); this used to be
        // priceHistory.erase(priceHistory.begin()) on a vector, which is O(n)
        // per call (shifts every remaining element down) and this loop can
        // run it repeatedly in a single AddPrice().
        while (!priceHistory.empty() && now > maxAge && priceHistory.front().date < now - maxAge) {
            priceHistory.pop_front();
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

        PrepareGradient(H, (float)clientRect.top);
        graphics.DrawLines(gradientPen.get(), points.data(), (INT)points.size());

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

    // Graphics is tied to the paint HDC, but the sparkline geometry is fixed
    // for the lifetime of its owner, so these are initialized once per instance.
    mutable std::unique_ptr<Gdiplus::LinearGradientBrush> gradientBrush;
    mutable std::unique_ptr<Gdiplus::Pen> gradientPen;

    void PrepareGradient(float height, float originY) const {
        if (!gradientBrush) {
            gradientBrush = std::make_unique<Gdiplus::LinearGradientBrush>(
                Gdiplus::PointF(0.0f, originY),
                Gdiplus::PointF(0.0f, originY + height + 1.0f),
                Gdiplus::Color(200, 1, 166, 1),
                Gdiplus::Color(200, 1, 166, 1));
            gradientBrush->SetInterpolationColors(sparkColors, sparkStops, 3);
            gradientPen = std::make_unique<Gdiplus::Pen>(gradientBrush.get(), 3.0f);
            gradientPen->SetLineJoin(Gdiplus::LineJoinRound);
        }
    }

    // NEW: same idea as in Sparkline, a separate long-lived history for the dots
    // PERF: deque, not vector — see the comment on Sparkline::priceHistory above.
    std::deque<MiniSparkPoint> priceHistory;

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

    // PERF: same binary-search treatment as Sparkline::GetPriceAgo. This one
    // matters even more — it's called once per L1 tick per symbol from
    // Diamonds_UpdateMarketCols() -> GetPriceMinutesAgo(), i.e. on the raw
    // unthrottled tick-ingest path, in addition to 5x per Draw() per visible row.
    bool GetPriceAgo(ULONGLONG now, ULONGLONG minutesAgo, double& outPrice, bool strict = true) const {
        if (priceHistory.empty()) return false;

        ULONGLONG minMs = minutesAgo * 60000ULL;
        
        ULONGLONG target = (now > minMs) ? (now - minMs) : 0;

        if (strict && (now < minMs || priceHistory.front().date > target)) return false;

        auto it = std::lower_bound(priceHistory.begin(), priceHistory.end(), target,
            [](const MiniSparkPoint& p, ULONGLONG t) { return p.date < t; });

        size_t bestIdx;
        if (it == priceHistory.end()) {
            bestIdx = priceHistory.size() - 1;
        } else if (it == priceHistory.begin()) {
            bestIdx = 0;
        } else {
            size_t idxAfter  = (size_t)(it - priceHistory.begin());
            size_t idxBefore = idxAfter - 1;
            ULONGLONG diffAfter  = it->date - target;
            ULONGLONG diffBefore = target - priceHistory[idxBefore].date;
            bestIdx = (diffAfter < diffBefore) ? idxAfter : idxBefore;
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
        // PERF: pop_front() is O(1) on a deque — see Sparkline::AddPrice comment.
        while (!priceHistory.empty() && now > maxAge && priceHistory.front().date < now - maxAge) {
            priceHistory.pop_front();
        }
    }

    // Draw the sparkline into the sub-item bounding rect.
    // Leaves a small left margin so the text (position number) is still visible.
    void Draw(HDC hdc, const RECT& cellRect) const {
        if (data.size() < 2) return;

        // Reserve the right portion for the numeric text; sparkline fills the rest, including a small part of the previous cell to the left.
        const int rightMargin = 20;  // px gap from cell right edge
        const int topPad     = 3;
        const int botPad     = 3;

        float W = (float)(cellRect.right - cellRect.left + rightMargin);
        float H = (float)(cellRect.bottom - cellRect.top  - topPad     - botPad);
        if (W < 4 || H < 4) return;

        float ox = (float)(cellRect.left - rightMargin);
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

        PrepareGradient(H, oy);
        g.DrawLines(gradientPen.get(), pts.data(), (INT)pts.size());

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

    // Public accessor: price from `minutesAgo` minutes ago, sampled from the
    // same long-lived history used for the reference dots in Draw(). Returns
    // false if there isn't yet enough history reaching that far back (so
    // callers can show "--" until it's ready, same pattern as the dots).
    bool GetPriceMinutesAgo(int minutesAgo, double& outPrice) const {
        // Pass strict = false so the 5-minute column returns data immediately
        return GetPriceAgo(GetTickCount64(), (ULONGLONG)minutesAgo, outPrice, false);
    }
};