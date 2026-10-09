#pragma once

// Rolling "recent vs. baseline" rate tracker.
//
// Records (time, amount) samples and maintains, incrementally (O(1) amortized
// per add or read):
//   - total():   sum of all samples in the trailing baseline window
//   - ratio(now): recent share-rate / baseline share-rate, where "recent" is
//                 the trailing recentMs and "baseline" is everything older
//                 (up to baselineMs). Returns 0 until baselineMs of history
//                 has been observed since the last clear() (warm-up gate),
//                 so a thin baseline can't produce a misleadingly huge ratio.
// Reads prune against their passed-in `now`, so values decay even when no new
// samples arrive.
//
// `now` is always passed in by the caller (GetTickCount64-style ms), so the
// class has no Win32 dependency and is trivially testable.
class RollingRateTracker {
public:
    using Tick = unsigned long long;
    static constexpr Tick DEFAULT_RECENT_MS   = 15000ULL;   // 15 s recent window
    static constexpr Tick DEFAULT_BASELINE_MS = 300000ULL;  // 5 min baseline window

    explicit RollingRateTracker(Tick recentMs = DEFAULT_RECENT_MS,
                                Tick baselineMs = DEFAULT_BASELINE_MS)
        : recentMs_(recentMs), baselineMs_(baselineMs) {}

    // Feed one sample. Samples must be added in non-decreasing time order.
    void add(Tick now, double amount) {
        // trackingStart is set when the first sample arrives after construction
        // or clear(); pruning never resets it, so ready() can't flip-flop at
        // the baseline boundary.
        if (!tracking_) {
            trackingStart_ = now;
            tracking_ = true;
        }

        history_.push_back({ now, amount });
        sumTotal_  += amount;
        sumRecent_ += amount;

        prune(now);
    }

    void clear() {
        history_.clear();
        sumTotal_ = sumRecent_ = sumBaseline_ = 0.0;
        boundaryIdx_ = 0;
        trackingStart_ = 0;
        tracking_ = false;
    }

    bool   empty() const { return history_.empty(); }
    double total(Tick now) const {
        prune(now);
        return sumTotal_;
    }   // trailing-baseline-window sum

    // True once at least baselineMs has elapsed since tracking began.
    bool ready(Tick now) const {
        return tracking_ && now >= trackingStart_ && now - trackingStart_ >= baselineMs_;
    }

    // recent-rate / baseline-rate; 0.0 until ready().
    double ratio(Tick now) const {
        prune(now);
        if (!ready(now)) return 0.0;
        const double recentSec   = recentMs_ / 1000.0;
        const double baselineSec = (baselineMs_ - recentMs_) / 1000.0;
        const double recentRate   = sumRecent_   / recentSec;
        const double baselineRate = sumBaseline_ / baselineSec;
        return (baselineRate > 0.0001) ? (recentRate / baselineRate)
                                       : (recentRate > 0.0 ? 9.9 : 0.0);
    }

private:
    void prune(Tick now) const {
        const Tick recentCutoff   = now > recentMs_   ? now - recentMs_   : 0;
        const Tick baselineCutoff = now > baselineMs_ ? now - baselineMs_ : 0;

        // Entries only ever move recent -> baseline, never back.
        while (boundaryIdx_ < history_.size() && history_[boundaryIdx_].time < recentCutoff) {
            const double a = history_[boundaryIdx_].amount;
            sumRecent_   -= a;
            sumBaseline_ += a;
            ++boundaryIdx_;
        }
        // Expire old entries off the front (always already in baseline).
        while (!history_.empty() && history_.front().time < baselineCutoff) {
            const double a = history_.front().amount;
            sumTotal_    -= a;
            sumBaseline_ -= a;
            history_.pop_front();
            if (boundaryIdx_ > 0) --boundaryIdx_;
        }
    }

    struct Sample { Tick time; double amount; };
    Tick recentMs_, baselineMs_;
    mutable std::deque<Sample> history_;
    mutable double sumTotal_ = 0.0, sumRecent_ = 0.0, sumBaseline_ = 0.0;
    mutable size_t boundaryIdx_ = 0;   // index of first entry still "recent"
    Tick   trackingStart_ = 0;
    bool   tracking_ = false;
};