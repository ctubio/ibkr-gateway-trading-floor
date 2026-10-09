#pragma once

// ═════════════════════════════════════════════════════════════════════════════
// Decision model (Ollama /v1/systemone, clef-flash)
// Press X in a Market window -> snapshot of the symbol is sent to the model as
// `state`, a fixed set of typed trading questions is scored, and the result is
// shown in a MessageBox. The HTTP call runs on a worker thread so the UI and
// market-data feed never block.
// Relies on JsonEscapeString() from server.h (already included before the gui/*.h
// headers in main.cpp) and on WinInet (already used by server.h).
// ═════════════════════════════════════════════════════════════════════════════
static const char*         DECISION_HOST  = "192.168.1.21";
static const INTERNET_PORT DECISION_PORT  = 11434;
static const char*         DECISION_PATH  = "/v1/systemone";
static const char*         DECISION_MODEL = "nimble:9b";
static std::atomic<bool>   s_decisionBusy{ false };

// ── Volume rate / print-frequency rate ────────────────────────────────────────
// Compares a short "recent" window of trade prints against a longer "baseline"
// window (the trailing history minus the recent slice) to catch a sudden
// increase in either total share volume or trade frequency, the day-trading
// "something is happening right now" signal. Both ratios are derived from the
// same tick-by-tick history (state->volRate), never from RTVolume, since
// the conflated L1 feed can fold several prints into one update and hides the
// print-frequency signal entirely.
struct VolRateResult {
    double volRatio  = 0.0;   // recent shares/sec  ÷ baseline shares/sec
    double vol5min   = 0.0;   // total shares traded in the trailing baseline window (5 min)
                               // from volRate. Computed unconditionally (unlike volRatio
                               // below): a partial sum during the first 5 min after a clear is
                               // still a meaningful number, not a misleading ratio off a thin
                               // denominator.
};

static VolRateResult Market_ComputeVolRates(const TsState* state, ULONGLONG now) {
    VolRateResult r;
    r.vol5min  = state->volRate.total(now);
    r.volRatio = state->volRate.ratio(now);
    return r;
}

// ── Formatting helpers ────────────────────────────────────────────────────────
static std::string Market_Fmt(double v, int dec = 2) {
    if (v == 0.0) return "--";
    return FormatFixed(v, dec);
}
static std::string Market_FmtQty(double v) {
    if (v == 0.0) return "--";
    if (v == (long long)v) return FormatFixed(v, 0);
    return FormatFixed(v, 2);
}

// ── Snapshot helpers (UI thread) ─────────────────────────────────────────────
static std::string Decision_ListText(HWND h, int row, int col) {
    char buf[64] = {};
    ListView_GetItemText(h, row, col, buf, sizeof(buf));
    return buf;
}

// Time & Sales colours are assigned in WM_MARKET_TICK: red family = traded at/below
// bid (seller aggressive), green family = at/above ask (buyer aggressive).
static const char* Decision_SideFromColor(COLORREF c) {
    if (c == COINS_CLR_RED   || c == COINS_CLR_RED_DARK   || c == COINS_CLR_RED_DARK2)   return "S";
    if (c == COINS_CLR_GREEN || c == COINS_CLR_GREEN_DARK || c == COINS_CLR_GREEN_DARK2) return "B";
    return "-";
}

static std::string Decision_RecentPrints(HWND hList, int maxRows, double& buyVol, double& sellVol) {
    std::string out;
    if (!hList) return out;
    int n = std::min(ListView_GetItemCount(hList), maxRows);
    for (int i = 0; i < n; ++i) {
        LVITEMA lvi = {};
        lvi.mask = LVIF_PARAM; lvi.iItem = i;
        if (!ListView_GetItem(hList, &lvi)) continue;
        COLORREF c = (COLORREF)(lvi.lParam & 0xFFFFFFFF);
        std::string price = Decision_ListText(hList, i, 0);
        std::string size  = Decision_ListText(hList, i, 1);
        std::string time  = Decision_ListText(hList, i, 2);
        const char* side  = Decision_SideFromColor(c);
        double sz = std::atof(size.c_str());
        if (side[0] == 'B') buyVol += sz; else if (side[0] == 'S') sellVol += sz;
        out += std::format("  {} {} x {} {}\n", time, price, size, side);
    }
    return out;
}

static std::string Decision_BookText(HWND hL2, double& askTotal, double& bidTotal) {
    std::string asks, bids;
    if (!hL2) return "";
    auto add = [&](int r, std::string& dst, double& total) {
        std::string p = Decision_ListText(hL2, r, 0), s = Decision_ListText(hL2, r, 1);
        if (p.empty() || p == "--") return;
        total += std::atof(s.c_str());
        dst += p + " x " + s + ", ";
    };
    for (int r = 5; r >= 0; --r) add(r, asks, askTotal);   // rows 0-5 = asks, row 5 = best ask
    for (int r = 6; r < 12; ++r) add(r, bids, bidTotal);   // rows 6-11 = bids, row 6 = best bid
    auto trim = [](std::string& s) { if (s.size() >= 2) s.resize(s.size() - 2); };
    trim(asks); trim(bids);
    return "Asks (best first): " + (asks.empty() ? "--" : asks) + "\n" +
           "Bids (best first): " + (bids.empty() ? "--" : bids) + "\n";
}

static std::string Market_BuildDecisionState(TsState* st) {
    const TradingAPI::L1Book& L = st->l1Info;
    TradingAPI::L1Book full;
    bool haveFull = api().getMarketData(st->conId, full);
    ULONGLONG now = GetTickCount64();
    VolRateResult vr = Market_ComputeVolRates(st, now);

    std::string s;
    s += std::format("Trading snapshot for {} (US-listed, short-term trading).\n", st->symbol);
    try {
        static const std::chrono::time_zone* ny = std::chrono::locate_zone("America/New_York");
        std::chrono::zoned_time zt{ ny, std::chrono::system_clock::now() };
        auto lt  = zt.get_local_time();
        auto dp  = std::chrono::floor<std::chrono::days>(lt);
        auto tod = std::chrono::hh_mm_ss{ lt - dp };
        s += std::format("US Eastern time: {:02}:{:02}\n", tod.hours().count(), tod.minutes().count());
    } catch (...) {}
    if (st->isOvernight) s += "Session: overnight trading mode.\n";
    if (haveFull && full.halted) s += "WARNING: trading is HALTED.\n";

    // ── Quote ────────────────────────────────────────────────────────────────
    s += "\n[Quote]\n";
    s += std::format("Last {} | Prev close {} | Open {} | Day high {} | Day low {} | VWAP {}\n",
        Market_Fmt(L.last), Market_Fmt(L.prevClose), Market_Fmt(L.open),
        Market_Fmt(L.high), Market_Fmt(L.low), Market_Fmt(L.vwap));
    if (L.prevClose > 0.0)
        s += std::format("Change vs prev close: {:+.2f} ({:+.2f}%)\n", L.change(), L.changePct());
    if (L.open > 0.0)
        s += std::format("Last vs open: {:+.2f} ({:+.2f}%)\n", L.last - L.open, (L.last - L.open) / L.open * 100.0);
    if (L.high > L.low && L.low > 0.0)
        s += std::format("Position in day range: {:.0f}% (0% = at day low, 100% = at day high)\n",
                         (L.last - L.low) / (L.high - L.low) * 100.0);
    if (L.vwap > 0.0)
        s += std::format("Last vs VWAP: {:+.2f} ({:+.2f}%)\n", L.last - L.vwap, (L.last - L.vwap) / L.vwap * 100.0);
    s += std::format("Bid {} x {} | Ask {} x {}\n", Market_Fmt(L.bid), Market_FmtQty(L.bidSize),
                     Market_Fmt(L.ask), Market_FmtQty(L.askSize));
    if (L.bid > 0.0 && L.ask > 0.0)
        s += std::format("Spread: {:.2f} ({:.3f}% of price)\n", L.ask - L.bid, (L.ask - L.bid) / L.last * 100.0);

    // ── Volume ───────────────────────────────────────────────────────────────
    s += "\n[Volume]\n";
    s += std::format("Shares traded in the last 5 min: {}\n", formatVolume((long long)vr.vol5min));
    if (st->volRate.ready(now))
        s += std::format("Recent 15s share rate vs prior 5-min baseline: {:.1f}x (1x = normal pace, 3x+ = hot)\n", vr.volRatio);
    else
        s += "Recent-vs-baseline volume ratio: still warming up (needs 5 min of ticks).\n";

    // ── Longer-term ranges ───────────────────────────────────────────────────
    if (haveFull && L.last > 0.0) {
        auto rangeLine = [&](const char* name, double lo, double hi) {
            if (hi > lo && lo > 0.0)
                s += std::format("{}: low {} / high {} | position in range {:.0f}% | {:.1f}% below high, {:.1f}% above low\n",
                    name, Market_Fmt(lo), Market_Fmt(hi), (L.last - lo) / (hi - lo) * 100.0,
                    (hi - L.last) / hi * 100.0, (L.last - lo) / lo * 100.0);
        };
        std::string ranges;
        size_t before = s.size();
        s += "\n[Longer-term ranges]\n";
        size_t hdrEnd = s.size();
        rangeLine("13-week", full.low13, full.high13);
        rangeLine("26-week", full.low26, full.high26);
        rangeLine("52-week", full.low52, full.high52);
        if (s.size() == hdrEnd) s.resize(before);   // nothing available, drop the header
    }

    // ── Order flow ───────────────────────────────────────────────────────────
    double buyVol = 0, sellVol = 0, dummyB = 0, dummyS = 0;
    std::string prints = Decision_RecentPrints(st->hTsList, 24, buyVol, sellVol);
    if (!prints.empty()) {
        s += "\n[Recent prints, newest first (B = at/above ask, S = at/below bid, - = between)]\n" + prints;
        double tot = buyVol + sellVol;
        if (tot > 0.0)
            s += std::format("Aggressor volume in those prints: buy {:.0f} vs sell {:.0f} ({:.0f}% buy)\n",
                             buyVol, sellVol, buyVol / tot * 100.0);
    }
    std::string big = Decision_RecentPrints(st->hTsListF100, 8, dummyB, dummyS);
    if (!big.empty()) s += "\n[Recent large prints (>= 100 shares), newest first]\n" + big;

    double askTot = 0, bidTot = 0;
    std::string book = Decision_BookText(st->hL2List, askTot, bidTot);
    if (askTot > 0.0 || bidTot > 0.0) {
        s += "\n[Level 2 book]\n" + book;
        s += std::format("Total displayed size: asks {:.0f} vs bids {:.0f}\n", askTot, bidTot);
    }

    // ── Position & orders ────────────────────────────────────────────────────
    s += "\n[Account / position]\n";
    if (st->position != 0.0) {
        double sign = st->position > 0 ? 1.0 : -1.0;
        s += std::format("Current position: {} shares ({}) @ avg {}\n", Market_FmtQty(st->position),
                         st->position > 0 ? "LONG" : "SHORT", Market_Fmt(st->avgPrice));
        if (st->avgPrice > 0.0 && L.last > 0.0)
            s += std::format("Position return: {:+.2f}% | unrealized P/L {:+.2f} | daily P/L {:+.2f}\n",
                (L.last - st->avgPrice) / st->avgPrice * 100.0 * sign, st->unrealizedPnL, st->dailyPnL);
        if (NetLiquidation > 0.0)
            s += std::format("Position market value: {:.2f} ({:.1f}% of net liquidation)\n",
                             st->position * L.last, std::abs(st->position * L.last) / NetLiquidation * 100.0);
    } else {
        s += "No current position in this symbol.\n";
    }
    if (NetLiquidation > 0.0)
        s += std::format("Net liquidation: {:.2f} | max risk per trade: {:.2f}% ({:.2f})\n",
                         NetLiquidation, riskGateway, NetLiquidation * riskGateway / 100.0);
    s += std::format("Trader defaults: order qty {} | stop distance {:.2f} | profit distance {:.2f}\n",
                     qtyGateway, stopGateway, profitGateway);
    for (const auto& o : st->openOrdersSummary)
        s += std::format("Open order: {} {} {} @ {} ({})\n", o.action, Market_FmtQty(o.totalQty),
                         o.orderType, o.price > 0 ? Market_Fmt(o.price) : "MKT", o.status);
    return s;
}

// ── Questions ────────────────────────────────────────────────────────────────
static const char* DECISION_QUESTIONS_BASE = R"JSON(
"q_bias":{"type":"choice","instructions":"Given the snapshot, which trade bias has the best edge right now for a short-term trader?","criteria":{
  "neutral":"No clear edge: wait for a better setup or stay out",
  "long":"Buying has a favorable edge: strength, buyers in control, room to run",
  "short":"Selling or shorting has a favorable edge: weakness, sellers in control, room to fall"}},
"q_momentum":{"type":"score","instructions":"How strong is the current short-term price momentum and order flow?","criteria":["Strongly bearish","Mildly bearish","Neutral / choppy","Mildly bullish","Strongly bullish"]},
"q_setup":{"type":"score","instructions":"How good is the risk/reward of opening a new trade right now, in the better direction?","criteria":["Very poor: avoid","Poor","Acceptable","Good","Excellent"]},
"q_extended":{"type":"noul","instructions":"Is price already extended, so that entering now would be chasing the move?","criteria":{"true":"Price has run far from VWAP/open/range and entering now means chasing.","false":"Price is not extended; entry is reasonable relative to VWAP and the day range."}},
"q_activity":{"type":"noul","instructions":"Is there unusually high volume or trading activity right now?","criteria":{"true":"Volume or print activity is well above normal.","false":"Volume and print activity are normal or quiet."}},
"q_liquidity":{"type":"noul","instructions":"Are the spread and book depth good enough to trade this symbol safely right now?","criteria":{"true":"Tight spread, decent size on both sides, not halted.","false":"Wide spread, thin book, or halted."}},
"q_risk":{"type":"score","instructions":"What is the overall risk of trading this symbol right now (volatility, spread, thin book, halt)?","criteria":["Low","Moderate","High","Extreme"]}
)JSON";

static const char* DECISION_QUESTION_POSITION = R"JSON(
"q_position":{"type":"choice","instructions":"Given the existing position and the snapshot, what should be done with the position?","criteria":{
  "hold":"Keep the position unchanged",
  "reduce":"Reduce the position: take partial profit or cut part of the loss",
  "exit":"Close the entire position"}}
)JSON";

// ── HTTP (worker thread) ─────────────────────────────────────────────────────
static bool Decision_HttpPost(const std::string& body, std::string& response, DWORD& status) {
    HINTERNET hInet = InternetOpenA("TradingFloor/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInet) return false;
    DWORD timeout = 120000;   // allow for a cold model load
    InternetSetOptionA(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInet, INTERNET_OPTION_SEND_TIMEOUT,    &timeout, sizeof(timeout));
    InternetSetOptionA(hInet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hConn = InternetConnectA(hInet, DECISION_HOST, DECISION_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConn) { InternetCloseHandle(hInet); return false; }
    HINTERNET hReq = HttpOpenRequestA(hConn, "POST", DECISION_PATH, NULL, NULL, NULL,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_UI, 0);
    bool ok = false;
    if (hReq) {
        static const char hdr[] = "Content-Type: application/json\r\n";
        if (HttpSendRequestA(hReq, hdr, (DWORD)strlen(hdr), (LPVOID)body.data(), (DWORD)body.size())) {
            DWORD len = sizeof(status);
            status = 0;
            HttpQueryInfoA(hReq, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status, &len, NULL);
            char chunk[4096]; DWORD n = 0;
            while (InternetReadFile(hReq, chunk, sizeof(chunk), &n) && n > 0) response.append(chunk, n);
            ok = true;
        }
        InternetCloseHandle(hReq);
    }
    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);
    return ok;
}

// ── Response parsing / formatting ────────────────────────────────────────────
// Finds `"key"` followed by ':' and returns the index of the value, or npos.
static size_t Decision_KeyValuePos(const std::string& s, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    size_t pos = 0;
    while ((pos = s.find(needle, pos)) != std::string::npos) {
        size_t p = pos + needle.size();
        while (p < s.size() && isspace((unsigned char)s[p])) ++p;
        if (p < s.size() && s[p] == ':') {
            ++p;
            while (p < s.size() && isspace((unsigned char)s[p])) ++p;
            return p;
        }
        pos += needle.size();
    }
    return std::string::npos;
}

static std::string Decision_Block(const std::string& json, const std::string& id) {
    size_t p = Decision_KeyValuePos(json, id);
    if (p == std::string::npos || p >= json.size() || json[p] != '{') return "";
    int depth = 0;
    for (size_t i = p; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}' && --depth == 0) return json.substr(p, i - p + 1);
    }
    return "";
}

static bool Decision_GetString(const std::string& b, const std::string& key, std::string& out) {
    size_t p = Decision_KeyValuePos(b, key);
    if (p == std::string::npos || p >= b.size() || b[p] != '"') return false;
    size_t e = b.find('"', p + 1);
    if (e == std::string::npos) return false;
    out = b.substr(p + 1, e - p - 1);
    return true;
}

static bool Decision_GetNumber(const std::string& b, const std::string& key, double& out) {
    size_t p = Decision_KeyValuePos(b, key);
    if (p == std::string::npos) return false;
    char* end = nullptr;
    out = std::strtod(b.c_str() + p, &end);
    return end != b.c_str() + p;
}

static std::vector<std::pair<std::string, double>> Decision_GetProbs(const std::string& b) {
    std::vector<std::pair<std::string, double>> v;
    size_t p = Decision_KeyValuePos(b, "probabilities");
    if (p == std::string::npos || p >= b.size() || b[p] != '{') return v;
    ++p;
    while (p < b.size()) {
        while (p < b.size() && (isspace((unsigned char)b[p]) || b[p] == ',')) ++p;
        if (p >= b.size() || b[p] != '"') break;
        size_t e = b.find('"', p + 1);
        if (e == std::string::npos) break;
        std::string k = b.substr(p + 1, e - p - 1);
        p = e + 1;
        while (p < b.size() && (isspace((unsigned char)b[p]) || b[p] == ':')) ++p;
        char* end = nullptr;
        double val = std::strtod(b.c_str() + p, &end);
        if (end == b.c_str() + p) break;
        p = (size_t)(end - b.c_str());
        v.emplace_back(k, val);
    }
    std::sort(v.begin(), v.end(), [](const auto& a, const auto& c) { return a.second > c.second; });
    return v;
}

static std::string Decision_Format(const std::string& resp) {
    std::string out;
    bool any = false;

    auto choiceLine = [&](const char* id, const char* title) {
        std::string b = Decision_Block(resp, id), c;
        if (b.empty() || !Decision_GetString(b, "choice", c)) return;
        double conf = 0.0; Decision_GetNumber(b, "confidence", conf);
        std::string up = c;
        std::transform(up.begin(), up.end(), up.begin(), [](unsigned char ch) { return (char)toupper(ch); });
        out += std::format("{}: {}  (confidence {:.2f})\n    ", title, up, conf);
        for (const auto& [k, v] : Decision_GetProbs(b)) out += std::format("{} {:.0f}%   ", k, v * 100.0);
        out += "\n";
        any = true;
        out += "\n";
    };
    auto scoreLine = [&](const char* id, const char* title, std::vector<const char*> labels) {
        std::string b = Decision_Block(resp, id);
        double sc = 0.0;
        if (b.empty() || !Decision_GetNumber(b, "score", sc)) return;
        int idx = std::max(0, std::min((int)labels.size() - 1, (int)std::lround(sc)));
        out += std::format("{}: {:.1f} / {}  -  {}\n", title, sc, labels.size() - 1, labels[idx]);
        any = true;
        out += "\n";
    };
    auto noulLine = [&](const char* id, const char* title) {
        std::string b = Decision_Block(resp, id);
        double v = 0.0;
        if (b.empty() || !Decision_GetNumber(b, "noul", v)) return;
        out += std::format("{}: {}  ({:.0f}%)\n", title, v >= 0.5 ? "YES" : "NO", v * 100.0);
        any = true;
        out += "\n";
    };

    choiceLine("q_bias",     "Bias");
    choiceLine("q_position", "Position");
    scoreLine("q_momentum", "Momentum", { "Strongly bearish", "Mildly bearish", "Neutral / choppy", "Mildly bullish", "Strongly bullish" });
    scoreLine("q_setup",    "Setup quality", { "Very poor", "Poor", "Acceptable", "Good", "Excellent" });
    scoreLine("q_risk",     "Risk", { "Low", "Moderate", "High", "Extreme" });
    noulLine("q_extended",  "Extended / chasing?");
    noulLine("q_activity",  "Unusual activity?");
    noulLine("q_liquidity", "Liquidity OK?");

    if (!any) return "Unexpected response from the decision model:\n\n" + resp.substr(0, 600);
    return out;
}

// ── Entry point: called from WM_KEYDOWN 'X' ──────────────────────────────────
static void Market_RequestDecision(HWND hWnd, TsState* st) {
    if (!st || st->symbol.empty()) return;
    if (st->l1Info.last <= 0.0) {
        MessageBoxA(hWnd, "No market data yet for this symbol.", "Decision", MB_ICONINFORMATION);
        return;
    }
    bool expected = false;
    if (!s_decisionBusy.compare_exchange_strong(expected, true)) return;   // request/dialog already active

    std::string stateText = Market_BuildDecisionState(st);
    std::string questions = std::string("{") + DECISION_QUESTIONS_BASE;
    if (st->position != 0.0) questions += std::string(",") + DECISION_QUESTION_POSITION;
    questions += "}";

    std::string body = std::string("{\"model\":\"") + DECISION_MODEL + "\",\"state\":\"" +
                       JsonEscapeString(stateText) + "\",\"questions\":" + questions + "}";

    std::string symbol = st->symbol;
    double last = st->l1Info.last;

    std::thread([symbol, last, body]() {
        std::string text;
        try {
            std::string resp; DWORD status = 0;
            if (!Decision_HttpPost(body, resp, status))
                text = std::format("Could not reach the decision model at {}:{}.\nIs Ollama running and listening on the LAN?",
                                   DECISION_HOST, DECISION_PORT);
            else if (status != 200)
                text = std::format("Decision model returned HTTP {}:\n\n{}", status, resp.substr(0, 500));
            else
                text = Decision_Format(resp);
        } catch (const std::exception& e) {
            text = std::string("Decision request failed: ") + e.what();
        } catch (...) {
            text = "Decision request failed (unknown error).";
        }
        // Own thread => own modal loop, so the rest of the app keeps running live.
        MessageBoxA(NULL, text.c_str(), std::format("Decision: {} @ {}", symbol, FormatFixed(last, 2)).c_str(), MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
        s_decisionBusy.store(false);
    }).detach();
}