#pragma once

struct SparkPoint {
    ULONGLONG date; 
    double price;
};

static Gdiplus::Color sparkColors[3];
static Gdiplus::Color sparkColorsMini[3];
static const float sparkStops[] = { 0.0f, 0.50f, 1.0f };

// ── Shared sparkline engine ─────────────────────────────────────────────────
// Sparkline (large, Market window header chart) and MiniSparkline (small,
// per-row Diamonds chart) are the same chart at two sizes: a gradient price
// line plus five "N minutes ago" reference dots whose color/size scale with
// % change. This base holds everything that used to be duplicated between
// them, point storage, history pruning, the price/time scaling math, dot
// styling, and the line+dots paint routine. Each derived class supplies only
// its own palette/sizing constants and its own Draw() geometry (a Market
// window computes area from a fixed W/H against a client rect; a Diamonds
// cell computes it from a shrinking ListView subitem rect with margins).
// Nothing here is virtual, neither class is ever used through a base
// pointer, and this is a hot path (~30 FPS per open Market window, plus
// once per visible Diamonds row), so every difference between the two is
// just a parameter passed into DrawLineAndDots()/PrepareGradient(), not a
// runtime dispatch.
class SparklineEngine {
protected:
    std::deque<SparkPoint> data;

    // Long-lived history (~65 min) used only for the reference dots. Kept
    // completely separate from `data` so the short-window line-drawing logic
    // is untouched by how far back the dots need to look.
    std::deque<SparkPoint> priceHistory;

    // Graphics is tied to the paint HDC, and the gradient coordinates are in
    // that HDC's screen space, so refresh the resources when the row/area
    // moves. `endpointColor`/`endpointPad` reproduce each derived class's
    // original two-color constructor arguments verbatim, functionally inert
    // once SetInterpolationColors() takes over the blend, but kept exactly
    // as each class originally had them rather than assumed irrelevant.
    mutable std::unique_ptr<Gdiplus::LinearGradientBrush> gradientBrush;
    mutable std::unique_ptr<Gdiplus::Pen> gradientPen;
    mutable std::unique_ptr<Gdiplus::SolidBrush> dotBrush;
    mutable float gradientHeight = -1.0f;
    mutable float gradientOriginY = -1.0f;

    void PrepareGradient(float height, float originY,
                          const Gdiplus::Color* colors, const float* stops, int colorCount,
                          Gdiplus::Color endpointColor, float endpointPad) const {
        if (!gradientBrush || gradientHeight != height || gradientOriginY != originY) {
            gradientBrush = std::make_unique<Gdiplus::LinearGradientBrush>(
                Gdiplus::PointF(0.0f, originY),
                Gdiplus::PointF(0.0f, originY + height + endpointPad),
                endpointColor, endpointColor);
            gradientBrush->SetInterpolationColors(colors, stops, colorCount);
            gradientPen = std::make_unique<Gdiplus::Pen>(gradientBrush.get(), 3.0f);
            gradientPen->SetLineJoin(Gdiplus::LineJoinRound);
            gradientHeight = height;
            gradientOriginY = originY;
        }
    }

    // Equivalent to a d3 linear scale. Pure function, static, no shared state.
    static float MapScale(double value, double minDomain, double maxDomain, float minRange, float maxRange) {
        if (maxDomain == minDomain) return minRange + (maxRange - minRange) / 2.0f;
        return minRange + (float)((value - minDomain) / (maxDomain - minDomain)) * (maxRange - minRange);
    }

    // Maps a % price change into a color (gray -> saturated green/red) and a
    // radius (small -> large), both scaled by magnitude. Identical in both
    // original classes, no per-class parameters needed here at all.
    static void GetDotStyle(double pctChange, float minR, float maxR,
                             Gdiplus::Color& outColor, float& outRadius) {
        const double maxPct = 0.5; // % change at which color/size reach full intensity
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

    // Finds the price closest to (now - minutesAgo) in priceHistory.
    // `strict` (only ever passed false, by MiniSparkline::GetPriceMinutesAgo,
    // for the 5-minute column) skips the "do we have enough history yet" gate
    // so that one caller can get a best-effort answer immediately instead of
    // waiting for the full window to fill, Sparkline never needed that path
    // and so never passes strict=false, exactly reproducing its old
    // strict-only behavior.
    //
    // PERF: priceHistory is appended in strictly non-decreasing time order
    // (every AddPrice() call timestamps with GetTickCount64()), so instead of
    // scanning every entry to find the closest one (O(n), and this runs once
    // per reference dot per Draw() call, plus, for MiniSparkline specifically,
    // once per raw L1 tick via GetPriceMinutesAgo() on the unthrottled
    // tick-ingest path), binary-search for the insertion point and only
    // compare the two neighbors around it. O(log n) instead of O(n).
    bool GetPriceAgo(ULONGLONG now, ULONGLONG minutesAgo, double& outPrice, bool strict = true) const {
        if (priceHistory.empty()) return false;

        ULONGLONG minMs = minutesAgo * 60000ULL;
        ULONGLONG target = (now > minMs) ? (now - minMs) : 0;

        if (strict && (now < minMs || priceHistory.front().date > target)) return false;

        auto it = std::lower_bound(priceHistory.begin(), priceHistory.end(), target,
            [](const SparkPoint& p, ULONGLONG t) { return p.date < t; });

        size_t bestIdx;
        if (it == priceHistory.end()) {
            // target is at/after the newest sample, nothing after it to compare
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

    // Shared line + reference-dot rendering. `originX/originY` is the drawing
    // origin each derived Draw() computes for itself (a client-rect corner for
    // Sparkline, a margin-adjusted cell corner for MiniSparkline); everything
    // else that differed between the two originals, dot-reserved strip
    // width, dot radius range, the "flat price" epsilon nudged into min/max
    // price when every point in `data` shares one price, and the gradient
    // palette/endpoint, is passed in explicitly rather than assumed.
    void DrawLineAndDots(HDC hdc, float originX, float originY, float W, float H,
                          float dotAreaWidth, float minRadius, float maxRadius,
                          double flatPriceEpsilon,
                          const Gdiplus::Color* paletteColors, const float* paletteStops, int paletteCount,
                          Gdiplus::Color endpointColor, float endpointPad) const {
        if (data.size() < 2) return;

        Gdiplus::Graphics graphics(hdc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        // Everything below is in cell-local coordinates; the transform places it.
        // The gradient brush lives in this same space, so its cache key no longer
        // depends on where the row happens to be on screen.
        graphics.TranslateTransform(originX, originY);

        float lineW = (W - dotAreaWidth > 4.0f) ? (W - dotAreaWidth) : W;

        ULONGLONG minTime = data.front().date;
        ULONGLONG maxTime = data.back().date;
        double minPrice = data[0].price;
        double maxPrice = data[0].price;
        for (const auto& p : data) {
            if (p.price < minPrice) minPrice = p.price;
            if (p.price > maxPrice) maxPrice = p.price;
        }
        if (minTime == maxTime) maxTime++;
        if (minPrice == maxPrice) { minPrice -= flatPriceEpsilon; maxPrice += flatPriceEpsilon; }

        // AddPrice() caps `data` at 21 entries, so no heap allocation is needed.
        constexpr size_t kMaxPoints = 21;
        Gdiplus::PointF points[kMaxPoints];
        const size_t n = std::min(data.size(), kMaxPoints);
        for (size_t i = 0; i < n; ++i) {
            float x = MapScale((double)data[i].date, (double)minTime, (double)maxTime, 0, lineW);
            float y = MapScale(data[i].price, minPrice, maxPrice, H, 1);
            points[i] = Gdiplus::PointF(x, y);
        }

        // originY is now always 0: rebuilt only if H changes.
        PrepareGradient(H, 0.0f, paletteColors, paletteStops, paletteCount, endpointColor, endpointPad);
        graphics.DrawLines(gradientPen.get(), points, (INT)n);

        ULONGLONG now = GetTickCount64();
        double lastPrice = data.back().price;
        static const int minutesAgo[5] = { 10, 20, 30, 40, 50 };
        float dotX = W - maxRadius - 1.0f;

        for (int i = 0; i < 5; ++i) {
            double histPrice;
            if (!GetPriceAgo(now, minutesAgo[i], histPrice)) continue;
            float dotY = H * ((i + 0.5f) / 5.0f);

            double pctChange = (histPrice != 0.0) ? ((lastPrice - histPrice) / histPrice * 100.0) : 0.0;

            Gdiplus::Color dotColor;
            float dotRadius;
            GetDotStyle(pctChange, minRadius, maxRadius, dotColor, dotRadius);

            if (!dotBrush) dotBrush = std::make_unique<Gdiplus::SolidBrush>(dotColor);
            else           dotBrush->SetColor(dotColor);
            graphics.FillEllipse(dotBrush.get(), dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
        }
    }

public:
    // Identical in both originals: dedup unchanged prices, coalesce a point
    // added <30s after the previous one, cap `data` at 21 points, and
    // separately maintain the long-lived `priceHistory` (deduped by price,
    // pruned past ~65 minutes) used only for the reference dots.
    void AddPrice(double price) {
        ULONGLONG now = GetTickCount64();

        // 1. If price hasn't changed, ignore.
        if (!data.empty() && data.back().price == price) return;

        // 2. If 2nd-to-last point is newer than 30s ago, pop the last point.
        if (data.size() > 1 && data[data.size() - 2].date > now - 30000) {
            data.pop_back();
        }

        // 3. Add new data.
        data.push_back({now, price});

        // 4. Max array size of 21.
        if (data.size() > 21) {
            data.erase(data.begin());
        }

        // Maintain the separate long-term history used for the reference dots.
        if (priceHistory.empty() || priceHistory.back().price != price) {
            priceHistory.push_back({ now, price });
        }
        const ULONGLONG maxAge = 65ULL * 60ULL * 1000ULL; // keep ~65 minutes
        // PERF: pop_front() on a deque is O(1); erase(begin()) on a vector
        // would be O(n) per call (shifts every remaining element down), and
        // this loop can run it repeatedly in a single AddPrice().
        while (!priceHistory.empty() && now > maxAge && priceHistory.front().date < now - maxAge) {
            priceHistory.pop_front();
        }
    }

    bool HasData() const { return data.size() >= 2; }
};

// ── Large sparkline: Market window header ───────────────────────────────────
class Sparkline : public SparklineEngine {
public:
    void Draw(HDC hdc, RECT clientRect, float W, float H) {
        DrawLineAndDots(hdc, (float)clientRect.left, (float)clientRect.top, W, H,
                         /*dotAreaWidth=*/15.0f, /*minRadius=*/1.5f, /*maxRadius=*/4.5f,
                         /*flatPriceEpsilon=*/1.0,
                         sparkColors, sparkStops, 3,
                         Gdiplus::Color(255, 0, 0, 0), /*endpointPad=*/2.0f);
    }
};

// ── Mini sparkline: Diamonds per-row Position cell ──────────────────────────
class MiniSparkline : public SparklineEngine {
public:
    // Draws into the sub-item bounding rect. Leaves a small left margin so
    // the text (position number) is still visible.
    void Draw(HDC hdc, const RECT& cellRect) const {
        if (data.size() < 2) return;

        // Reserve the right portion for the numeric text; the sparkline
        // fills the rest, including a small part of the previous cell to the left.
        const int rightMargin = 20;
        const int topPad = 3;
        const int botPad = 3;

        float W = (float)(cellRect.right - cellRect.left + rightMargin);
        float H = (float)(cellRect.bottom - cellRect.top - topPad - botPad);
        if (W < 4 || H < 4) return;

        float ox = (float)(cellRect.left - rightMargin);
        float oy = (float)(cellRect.top  + topPad);

        DrawLineAndDots(hdc, ox, oy, W, H,
                         /*dotAreaWidth=*/18.0f, /*minRadius=*/1.0f, /*maxRadius=*/2.8f,
                         /*flatPriceEpsilon=*/0.5,
                         sparkColorsMini, sparkStops, 3,
                         Gdiplus::Color(200, 1, 166, 1), /*endpointPad=*/1.0f);
    }

    // Public accessor: price from `minutesAgo` minutes ago, sampled from the
    // same long-lived history the reference dots in Draw() use. Returns false
    // if there isn't yet enough history reaching that far back (so callers
    // can show "--" until it's ready, same pattern as the dots). Passes
    // strict=false so the 5-minute column returns data immediately.
    bool GetPriceMinutesAgo(int minutesAgo, double& outPrice) const {
        return GetPriceAgo(GetTickCount64(), (ULONGLONG)minutesAgo, outPrice, false);
    }
};