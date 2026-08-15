#pragma once

// ─── Lightweight HTTP API Server ─────────────────────────────────────────────
//
//  Serves endpoints over a background thread using raw Winsock (no extra libs):
//
//    GET  /balance            → JSON object with account number, PnL, and summary map
//    GET  /positions          → JSON array of all positions from api().getPortfolioMap()
//    GET  /position/{SYMBOL}  → JSON object for the single position matching SYMBOL
//    GET  /today              → Plain-text market news for today (TraderTV watchlist)
//    GET  /week               → Plain-text market news for today + 4 previous trading days
//    POST /trade              → Place an untransmitted limit order via IBKR
//                               Body: {"symbol":"AAPL","side":"BUY","quantity":10,"price":175.50}
//                               Also forwards the order to the external dashboard at
//                               http://192.168.1.105:2025/paper?action=place_trade
//
//  The server listens on 0.0.0.0:PORT (default 4011) so it is reachable from
//  the LAN.  Call HttpServer_Start() once after WinMain initialises and
//  HttpServer_Stop() before the process exits.
//
//  News content is cached in the Windows Registry under:
//    HKCU\Software\ibkr-gateway-trading-floor\NewsCache
//  One REG_SZ value per day (key = YYYY-MM-DD). Values older than 7 days are
//  purged automatically at the end of every /today or /week request.
//
//  JSON is hand-built (no external library required).
// ─────────────────────────────────────────────────────────────────────────────

#ifndef GATEWAY_SIM

// ── Config ────────────────────────────────────────────────────────────────────

static constexpr unsigned short HTTP_SERVER_PORT = 4011;

// ── Internal state ────────────────────────────────────────────────────────────

static SOCKET            g_httpListenSocket = INVALID_SOCKET;
static std::thread       g_httpThread;
static std::atomic<bool> g_httpRunning{ false };

// ── JSON helpers ──────────────────────────────────────────────────────────────

static std::string JsonEscapeString(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    return out;
}

static std::string JsonDouble(double v) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}

// Serialise one PositionInfo to a JSON object string.
static std::string PositionToJson(const TradingAPI::PositionInfo& p) {
    TradingAPI::L1Book l1;
    api().getMarketData(p.conId, l1);

    double unrealizedPct = 0;
    double costBasis = p.avgCost * std::fabs(p.shares);
    if (costBasis > 0.0) {
        unrealizedPct = p.pnlSingle.unrealizedPnL / costBasis * 100.0;
    }

    auto weekChangePct = [&](double closeAgo) -> double {
        return (closeAgo > 0.0 && l1.last > 0.0) ? (l1.last - closeAgo) / closeAgo * 100.0 : 0.0;
    };
    double week13Pct = weekChangePct(p.closeAgo13Week);
    double week26Pct = weekChangePct(p.closeAgo26Week);
    double week52Pct = weekChangePct(p.closeAgo52Week);

    std::string j;
    j.reserve(512);
    j += "{";
    j += "\"conId\":"              + std::to_string(p.conId)                 + ",";
    j += "\"symbol\":\""           + JsonEscapeString(p.symbol)              + "\",";
    j += "\"exchange\":\""         + JsonEscapeString(p.exchange)            + "\",";
    j += "\"shares\":"             + std::format("{:.0f}", p.shares)         + ",";
    j += "\"avgCost\":"            + JsonDouble(p.avgCost)                   + ",";
    j += "\"last\":"               + JsonDouble(l1.last)                     + ",";
    j += "\"vwap\":"               + JsonDouble(l1.vwap)                     + ",";
    j += "\"marketValue\":"        + JsonDouble(p.shares * l1.last)          + ",";
    j += "\"week13_low\":"         + JsonDouble(l1.low13)                    + ",";
    j += "\"week26_low\":"         + JsonDouble(l1.low26)                    + ",";
    j += "\"week52_low\":"         + JsonDouble(l1.low52)                    + ",";
    j += "\"week13_high\":"        + JsonDouble(l1.high13)                   + ",";
    j += "\"week26_high\":"        + JsonDouble(l1.high26)                   + ",";
    j += "\"week52_high\":"        + JsonDouble(l1.high52)                   + ",";
    j += "\"week13Change_pct\":\""       + std::format("{:+.2f}%", week13Pct) + "\",";
    j += "\"week26Change_pct\":\""       + std::format("{:+.2f}%", week26Pct) + "\",";
    j += "\"week52Change_pct\":\""       + std::format("{:+.2f}%", week52Pct) + "\",";
    j += "\"dailyChange_pct\":\""  + std::format("{:+.2f}%", l1.changePct()) + "\",";
    j += "\"dailyPnL\":"           + JsonDouble(p.pnlSingle.dailyPnL)        + ",";
    j += "\"unrealizedPnL\":"      + JsonDouble(p.pnlSingle.unrealizedPnL)   + ",";
    j += "\"unrealizedPnL_pct\":\"" + std::format("{:+.2f}%", unrealizedPct) + "\"";
    j += "}";
    return j;
}

// ── HTTP framing helpers ──────────────────────────────────────────────────────

static std::string MakeHttpResponse(int status, const std::string& statusText,
                                    const std::string& body) {
    std::string resp;
    resp += "HTTP/1.1 " + std::to_string(status) + " " + statusText + "\r\n";
    resp += "Content-Type: application/json\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;
    return resp;
}

static std::string MakePlainText(const std::string& body) {
    std::string resp;
    resp += "HTTP/1.1 200 OK\r\n";
    resp += "Content-Type: text/plain; charset=utf-8\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    resp += "Access-Control-Allow-Origin: *\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;
    return resp;
}

static std::string MakeOk(const std::string& body) {
    return MakeHttpResponse(200, "OK", body);
}

static std::string MakeNotFound(const std::string& msg = "{\"error\":\"not found\"}") {
    return MakeHttpResponse(404, "Not Found", msg);
}

static std::string MakeMethodNotAllowed() {
    return MakeHttpResponse(405, "Method Not Allowed", "{\"error\":\"method not allowed\"}");
}

// ── Endpoint handlers ─────────────────────────────────────────────────────────

// GET /chart/year/{SYMBOL}  →  plain-text CSV of ~1 year of daily bars for a
// current portfolio position. Blocks this connection's server thread (never
// the UI thread) until the async TWS response arrives or times out.
static std::string HandleGetHistory(const std::string& symbol, const std::string& timeframe) {
    auto rows = api().getHistoricalDataSync(symbol, timeframe);
    if (rows.empty()) {
        return MakeNotFound(
            "{\"error\":\"no historical data available (symbol must be a current "
            "portfolio position, or the request may have timed out)\"}");
    }
    std::string body = "Date,Open,High,Low,Close,Wap,Volume,TradesCount\n";
    for (const auto& row : rows) { body += row; body += "\n"; }
    return MakePlainText(body);
}

// GET /account  →  JSON object with account number, PnL, and full summary map
static std::string HandleGetBalance() {
    std::string acc   = api().getAccountNumber();
    double daily      = api().getDailyPnL();
    double unrealized = api().getUnrealizedPnL();
    double realized   = api().getRealizedPnL();
    std::map<std::string, std::string> summary = api().getAccountSummary();

    if (summary["NetLiquidation"].empty()) summary["NetLiquidation"] = "0.00";
    if (summary["AccruedDividend"].empty()) summary["AccruedDividend"] = "0.00";
    if (summary["GrossPositionValue"].empty()) summary["GrossPositionValue"] = "0.00";
    if (summary["BuyingPower"].empty()) summary["BuyingPower"] = "0.00";
    if (summary["MaintMarginReq"].empty()) summary["MaintMarginReq"] = "0.00";
    if (summary["AccruedCash"].empty()) summary["AccruedCash"] = "0.00";
    if (summary["EUR_CashBalance"].empty()) summary["EUR_CashBalance"] = "0.00";
    if (summary["USD_CashBalance"].empty()) summary["USD_CashBalance"] = "0.00";
    if (summary["BASE_CashBalance"].empty()) summary["BASE_CashBalance"] = "0.00";
    
    std::string j;
    j.reserve(512);
    j += "{";
    j += "\"accountNumber\":\"" + JsonEscapeString(acc) + "\",";
    j += "\"NetLiquidation\":"   + summary["NetLiquidation"]   + ",";
    j += "\"pnl\":{";
    j += "\"dailyPnL\":"        + JsonDouble(daily)      + ",";
    j += "\"unrealizedPnL\":"   + JsonDouble(unrealized) + ",";
    j += "\"realizedPnL\":"     + JsonDouble(realized)   + ",";
    j += "\"AccruedDividend\":"  + summary["AccruedDividend"];
    j += "},";
    j += "\"margin\":{";
    j += "\"GrossPositionValue\":" + summary["GrossPositionValue"] + ",";
    j += "\"BuyingPower\":"       + summary["BuyingPower"]   + ",";
    j += "\"MaintMarginReq\":"    + summary["MaintMarginReq"]   + ",";
    j += "\"AccruedCash\":"      + summary["AccruedCash"];
    j += "},";
    j += "\"cash\":{";
    j += "\"EUR_CashBalance\":"  + summary["EUR_CashBalance"]  + ",";
    j += "\"USD_CashBalance\":"  + summary["USD_CashBalance"]  + ",";
    j += "\"TOTAL_CashBalance\":" + summary["BASE_CashBalance"];
    j += "}";
    j += "}";
    return j;
}

// GET /portfolio  →  JSON array of all positions
static std::string HandleGetPositions() {
    std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
    const auto& map = api().getPortfolioMap();

    std::string body = "[";
    bool first = true;
    for (const auto& [conId, pos] : map) {
        if (!first) body += ",";
        body += PositionToJson(pos);
        first = false;
    }
    body += "]";
    return body;
}

// GET /portfolio/{SYMBOL}  →  JSON object for a single position (or empty on miss)
static std::string HandleGetPositionBySymbol(const std::string& symbol) {
    std::string upper = symbol;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
    const auto& map = api().getPortfolioMap();

    for (const auto& [conId, pos] : map) {
        std::string posSymbol = pos.symbol;
        std::transform(posSymbol.begin(), posSymbol.end(), posSymbol.begin(), ::toupper);
        if (posSymbol == upper) {
            return PositionToJson(pos);
        }
    }
    return "";
}

// ── News fetch helpers (mirrors fetch_news.py logic) ─────────────────────────

// Registry sub-key used to store news cache entries
static constexpr const char* NEWS_CACHE_SUBKEY = "NewsCache";

// Month names table (matches Python MONTHS dict)
static const char* s_monthNames[] = {
    "", "january", "february", "march", "april",
    "may", "june", "july", "august",
    "september", "october", "november", "december"
};

// Build the TraderTV watchlist URL for a given date (YYYY, MM, DD)
static std::string News_BuildUrl(int year, int month, int day) {
    return std::string("https://tradertv-live.beehiiv.com/p/tradertv-watchlist-")
        + s_monthNames[month] + "-" + std::to_string(day)
        + "-" + std::to_string(year);
}

// Format a date as "Month D, YYYY" (no leading zero on day, matching Python)
static std::string News_FormatHeader(SYSTEMTIME st) {
    return std::string(s_monthNames[st.wMonth][0] ? (std::string() + (char)toupper(s_monthNames[st.wMonth][0]) + (s_monthNames[st.wMonth] + 1)) : "")
        + " " + std::to_string(st.wDay)
        + ", " + std::to_string(st.wYear);
}

// Load a cached news entry from the registry; returns empty string on miss
static std::string News_LoadCache(const std::string& dateKey) {
    HKEY hKey;
    std::string fullPath = std::string(APP_REG_ROOT) + "\\" + NEWS_CACHE_SUBKEY;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return "";

    DWORD size = 0;
    if (RegQueryValueExA(hKey, dateKey.c_str(), NULL, NULL, NULL, &size) != ERROR_SUCCESS || size == 0) {
        RegCloseKey(hKey);
        return "";
    }
    std::vector<char> buf(size);
    RegQueryValueExA(hKey, dateKey.c_str(), NULL, NULL, (LPBYTE)buf.data(), &size);
    RegCloseKey(hKey);
    return std::string(buf.data());
}

// Save a news entry to the registry under the NewsCache subkey
static void News_SaveCache(const std::string& dateKey, const std::string& content) {
    HKEY hKey;
    std::string fullPath = std::string(APP_REG_ROOT) + "\\" + NEWS_CACHE_SUBKEY;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, dateKey.c_str(), 0, REG_SZ,
                       (const BYTE*)content.c_str(), (DWORD)content.size() + 1);
        RegCloseKey(hKey);
    }
}

// Delete registry values whose key name (YYYY-MM-DD) is older than max_age_days.
// Returns the number of entries deleted.
static int News_CleanupCache(int maxAgeDays = 7) {
    HKEY hKey;
    std::string fullPath = std::string(APP_REG_ROOT) + "\\" + NEWS_CACHE_SUBKEY;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), 0,
                      KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey) != ERROR_SUCCESS)
        return 0;

    // Compute cutoff as a YYYYMMDD integer for easy comparison
    SYSTEMTIME now;
    GetLocalTime(&now);
    // Roll back maxAgeDays days using FileTime arithmetic
    FILETIME nowFT, cutoffFT;
    SystemTimeToFileTime(&now, &nowFT);
    ULONGLONG ns100 = ((ULONGLONG)nowFT.dwHighDateTime << 32) | nowFT.dwLowDateTime;
    ns100 -= (ULONGLONG)maxAgeDays * 24ULL * 3600ULL * 10000000ULL;
    cutoffFT.dwHighDateTime = (DWORD)(ns100 >> 32);
    cutoffFT.dwLowDateTime  = (DWORD)(ns100 & 0xFFFFFFFF);
    SYSTEMTIME cutoffST;
    FileTimeToSystemTime(&cutoffFT, &cutoffST);
    char cutoffBuf[12];
    snprintf(cutoffBuf, sizeof(cutoffBuf), "%04d-%02d-%02d",
             cutoffST.wYear, cutoffST.wMonth, cutoffST.wDay);
    std::string cutoff(cutoffBuf);

    // Enumerate all values and collect names to delete
    std::vector<std::string> toDelete;
    DWORD index = 0;
    char valueName[32];
    DWORD nameSize = sizeof(valueName);
    while (RegEnumValueA(hKey, index++, valueName, &nameSize,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        std::string name(valueName);
        // Keys are YYYY-MM-DD; lexicographic comparison works for ISO dates
        if (name.size() == 10 && name < cutoff)
            toDelete.push_back(name);
        nameSize = sizeof(valueName);
    }

    for (const auto& name : toDelete)
        RegDeleteValueA(hKey, name.c_str());

    RegCloseKey(hKey);
    return (int)toDelete.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// The helpers below re-implement, in C++, the HTML → text pipeline from
// fetch_tradertv_watchlist.py (extract_main_content / format_market_cells /
// add_section_linebreaks / remove_click_to_watch / remove_inline_sources /
// remove_attribution_footers / clean_whitespace). News_FetchContent() below
// calls them in the same order the Python script's process_watchlist() does.
// ─────────────────────────────────────────────────────────────────────────────

// ── Minimal HTML entity decoding (mirrors BeautifulSoup's automatic decoding) ──
static void Utf8AppendCodepoint(std::string& out, unsigned int cp) {
    if (cp <= 0x7F) {
        out += (char)cp;
    } else if (cp <= 0x7FF) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
    } else if (cp <= 0xFFFF) {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    } else {
        out += (char)(0xF0 | (cp >> 18));
        out += (char)(0x80 | ((cp >> 12) & 0x3F));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
    }
}

static std::string HtmlDecodeEntities(const std::string& in) {
    static const std::unordered_map<std::string, unsigned int> namedEntities = {
        {"amp", '&'}, {"lt", '<'}, {"gt", '>'}, {"quot", '"'}, {"apos", '\''},
        {"nbsp", 0x00A0}, {"mdash", 0x2014}, {"ndash", 0x2013}, {"hellip", 0x2026},
        {"rsquo", 0x2019}, {"lsquo", 0x2018}, {"rdquo", 0x201D}, {"ldquo", 0x201C},
        {"copy", 0x00A9}, {"reg", 0x00AE}, {"trade", 0x2122}, {"deg", 0x00B0},
        {"bull", 0x2022}, {"middot", 0x00B7}, {"euro", 0x20AC}, {"pound", 0x00A3},
        {"cent", 0x00A2}, {"times", 0x00D7}, {"divide", 0x00F7},
    };

    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ) {
        if (in[i] == '&') {
            size_t semi = in.find(';', i + 1);
            if (semi != std::string::npos && semi - i <= 12) {
                std::string body = in.substr(i + 1, semi - i - 1);
                if (!body.empty() && body[0] == '#') {
                    bool hex = (body.size() > 1 && (body[1] == 'x' || body[1] == 'X'));
                    std::string numPart = body.substr(hex ? 2 : 1);
                    if (!numPart.empty()) {
                        try {
                            unsigned int cp = (unsigned int)std::stoul(numPart, nullptr, hex ? 16 : 10);
                            Utf8AppendCodepoint(out, cp);
                            i = semi + 1;
                            continue;
                        } catch (...) {}
                    }
                } else {
                    auto it = namedEntities.find(body);
                    if (it != namedEntities.end()) {
                        Utf8AppendCodepoint(out, it->second);
                        i = semi + 1;
                        continue;
                    }
                }
            }
        }
        out += in[i++];
    }
    return out;
}

// ── Tiny attribute / class helpers ────────────────────────────────────────────
static std::string HtmlExtractAttr(const std::string& tag, const std::string& attrName) {
    std::string lowerTag = tag;
    std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);
    std::string needle = attrName + "=";
    size_t pos = lowerTag.find(needle);
    while (pos != std::string::npos) {
        if (pos == 0 || isspace((unsigned char)tag[pos - 1])) {
            size_t valStart = pos + needle.size();
            if (valStart < tag.size() && (tag[valStart] == '"' || tag[valStart] == '\'')) {
                char quote = tag[valStart];
                size_t valEnd = tag.find(quote, valStart + 1);
                if (valEnd != std::string::npos)
                    return tag.substr(valStart + 1, valEnd - valStart - 1);
            }
        }
        pos = lowerTag.find(needle, pos + 1);
    }
    return "";
}

static bool HtmlClassHasToken(const std::string& classAttr, const std::string& token) {
    std::istringstream iss(classAttr);
    std::string cls;
    while (iss >> cls) {
        if (cls == token) return true;
    }
    return false;
}

// Strips tags from an HTML fragment, decodes entities, and joins the
// remaining non-empty trimmed text runs with `sep` — mirrors
// element.get_text(sep, strip=True) closely enough for cell-text parsing.
static std::string HtmlStripTagsToPlainText(const std::string& fragment, const std::string& sep) {
    std::string out;
    bool first = true;
    size_t i = 0;
    while (i < fragment.size()) {
        size_t lt = fragment.find('<', i);
        std::string chunk = (lt == std::string::npos) ? fragment.substr(i) : fragment.substr(i, lt - i);
        std::string decoded = HtmlDecodeEntities(chunk);
        size_t s = decoded.find_first_not_of(" \t\r\n");
        size_t e = decoded.find_last_not_of(" \t\r\n");
        if (s != std::string::npos) {
            if (!first) out += sep;
            out += decoded.substr(s, e - s + 1);
            first = false;
        }
        if (lt == std::string::npos) break;
        size_t gt = fragment.find('>', lt);
        if (gt == std::string::npos) break;
        i = gt + 1;
    }
    return out;
}

// ── Generic "find element body" with proper nested-tag depth tracking ────────
// Finds the first <tagName ...> whose opening tag satisfies `matches`, then
// returns the substring of HTML between it and its correctly-nested matching
// close tag (mirrors soup.find(tagName, class_=...) + reading that element's contents).
static std::string HtmlFindElementBodyIf(const std::string& html, const std::string& tagName,
                                          const std::function<bool(const std::string&)>& matches) {
    std::regex openRe("<" + tagName + R"(\b[^>]*>)", std::regex::icase);
    auto begin = std::sregex_iterator(html.begin(), html.end(), openRe);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string tag = it->str();
        if (!matches(tag)) continue;
        if (tag.size() >= 2 && tag[tag.size() - 2] == '/') return ""; // self-closed, no body

        size_t contentStart = (size_t)it->position() + tag.size();
        std::regex tagRe("<\\s*(/?)\\s*" + tagName + R"(\b[^>]*>)", std::regex::icase);
        int depth = 1;
        auto tb = std::sregex_iterator(html.begin() + contentStart, html.end(), tagRe);
        auto te = std::sregex_iterator();
        for (auto tit = tb; tit != te; ++tit) {
            std::string t = tit->str();
            bool isClose    = (*tit)[1].length() > 0;
            bool selfClosed = (t.size() >= 2 && t[t.size() - 2] == '/');
            if (isClose) {
                if (--depth == 0) {
                    size_t closeStart = contentStart + (size_t)tit->position();
                    return html.substr(contentStart, closeStart - contentStart);
                }
            } else if (!selfClosed) {
                depth++;
            }
        }
        return html.substr(contentStart); // unterminated — take the rest of the document
    }
    return "";
}

// Primary: soup.find("div", class_="content")
static std::string HtmlFindContentDiv(const std::string& html) {
    return HtmlFindElementBodyIf(html, "div", [](const std::string& tag) {
        return HtmlClassHasToken(HtmlExtractAttr(tag, "class"), "content");
    });
}

// Fallback: any div whose class merely *contains* one of these words
// (approximates the div[class*='content'] / [class*='post-body'] / [class*='article-body'] selectors).
static std::string HtmlFindDivByClassSubstring(const std::string& html, const std::vector<std::string>& needles) {
    return HtmlFindElementBodyIf(html, "div", [&](const std::string& tag) {
        std::string classAttr = HtmlExtractAttr(tag, "class");
        std::transform(classAttr.begin(), classAttr.end(), classAttr.begin(), ::tolower);
        for (const auto& n : needles)
            if (classAttr.find(n) != std::string::npos) return true;
        return false;
    });
}

static std::string HtmlFindElementBody(const std::string& html, const std::string& tagName) {
    return HtmlFindElementBodyIf(html, tagName, [](const std::string&) { return true; });
}

// Mirrors extract_main_content()'s selector fallback chain: div.content →
// div[class*='content'/'post-body'/'article-body'] → <article> → <body> → whole document.
static std::string HtmlLocateArticleBody(const std::string& html) {
    std::string body = HtmlFindContentDiv(html);
    if (!body.empty()) return body;

    body = HtmlFindDivByClassSubstring(html, {"content", "post-body", "article-body"});
    if (!body.empty()) return body;

    body = HtmlFindElementBody(html, "article");
    if (!body.empty()) return body;

    body = HtmlFindElementBody(html, "body");
    return body.empty() ? html : body;
}

// ── format_market_cells(): reformat <td class="market-cell"> content as "name: value" ──
static std::string HtmlFormatMarketCells(const std::string& html) {
    static const std::regex tdOpenRe(R"(<td\b[^>]*>)", std::regex::icase);
    static const std::regex valueStartRe(R"(^[+\-\$0-9])");

    std::string out;
    out.reserve(html.size());
    size_t lastCopied = 0;

    auto begin = std::sregex_iterator(html.begin(), html.end(), tdOpenRe);
    auto end   = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string tag = it->str();
        if (!HtmlClassHasToken(HtmlExtractAttr(tag, "class"), "market-cell")) continue;

        size_t tagStart = (size_t)it->position();
        if (tagStart < lastCopied) continue;
        size_t cellStart = tagStart + tag.size();
        size_t closeTag = html.find("</td", cellStart);
        if (closeTag == std::string::npos) continue;
        size_t closeTagEnd = html.find('>', closeTag);
        if (closeTagEnd == std::string::npos) continue;
        closeTagEnd += 1;

        std::string cellText = HtmlStripTagsToPlainText(html.substr(cellStart, closeTag - cellStart), " ");

        std::istringstream iss(cellText);
        std::vector<std::string> words;
        std::string w;
        while (iss >> w) words.push_back(w);
        if (words.size() < 2) continue; // matches Python's "continue" — cell left untouched

        int splitIdx = -1;
        for (int i = 0; i < (int)words.size(); ++i) {
            if (std::regex_search(words[i], valueStartRe) ||
                words[i] == "Near" || words[i] == "Slightly" || words[i] == "Small-cap") {
                splitIdx = i;
                break;
            }
        }

        std::string name, value;
        if (splitIdx > 0 && splitIdx < (int)words.size()) {
            for (int i = 0; i < splitIdx; ++i) { if (i) name += " "; name += words[i]; }
            for (int i = splitIdx; i < (int)words.size(); ++i) { if (i > splitIdx) value += " "; value += words[i]; }
        } else {
            for (int i = 0; i + 1 < (int)words.size(); ++i) { if (i) name += " "; name += words[i]; }
            value = words.back();
        }

        out.append(html, lastCopied, cellStart - lastCopied);
        out += name + ": " + value;
        out.append(html, closeTag, closeTagEnd - closeTag);
        lastCopied = closeTagEnd;
    }
    out.append(html, lastCopied, html.size() - lastCopied);
    return out;
}

// ── add_section_linebreaks() + content_div.get_text("\n", strip=True) ────────
static const char* const kNewsSectionHeaders[] = {
    "Sector & Theme Watch",
    "Premarket Trading",
    "Stocks in Focus",
    "More Stocks to Watch",
    "Economic Events - ET",
    "Earnings Today",
};

// Walks the (already cell-formatted) article HTML, skipping tags/comments and
// the contents of <script>/<style>, joining each non-empty stripped text run
// with "\n" — mirroring get_text("\n", strip=True). A blank line is inserted
// immediately before any run containing one of kNewsSectionHeaders, and before
// the start of any <div class="sector-theme"> / <div class="story-card">,
// mirroring the MARKER insertion + text.replace(MARKER, "\n\n") in the Python.
static std::string HtmlExtractTextWithSectionBreaks(const std::string& html) {
    std::string out;
    bool pendingBreak = false;
    bool firstPiece = true;

    auto emit = [&](const std::string& raw) {
        size_t b = raw.find_first_not_of(" \t\r\n");
        size_t e = raw.find_last_not_of(" \t\r\n");
        if (b == std::string::npos) return;
        std::string s = raw.substr(b, e - b + 1);

        for (const char* h : kNewsSectionHeaders) {
            if (s.find(h) != std::string::npos) { pendingBreak = true; break; }
        }

        if (!firstPiece) out += "\n";
        if (pendingBreak) { out += "\n\n"; pendingBreak = false; }
        out += s;
        firstPiece = false;
    };

    size_t i = 0;
    while (i < html.size()) {
        size_t lt = html.find('<', i);
        std::string chunk = (lt == std::string::npos) ? html.substr(i) : html.substr(i, lt - i);
        if (!chunk.empty()) emit(HtmlDecodeEntities(chunk));
        if (lt == std::string::npos) break;

        size_t gt = html.find('>', lt);
        if (gt == std::string::npos) break;
        std::string tag = html.substr(lt, gt - lt + 1);
        std::string lowerTag = tag;
        std::transform(lowerTag.begin(), lowerTag.end(), lowerTag.begin(), ::tolower);

        if (lowerTag.rfind("<!--", 0) == 0) {
            size_t closePos = html.find("-->", lt);
            i = (closePos == std::string::npos) ? html.size() : closePos + 3;
            continue;
        }
        if (lowerTag.rfind("<script", 0) == 0 || lowerTag.rfind("<style", 0) == 0) {
            bool isScript = lowerTag.rfind("<script", 0) == 0;
            size_t closePos = html.find(isScript ? "</script" : "</style", gt + 1);
            if (closePos == std::string::npos) { i = html.size(); }
            else {
                size_t closeGt = html.find('>', closePos);
                i = (closeGt == std::string::npos) ? html.size() : closeGt + 1;
            }
            continue;
        }
        if (lowerTag.size() > 4 && lowerTag.compare(0, 4, "<div") == 0 &&
            (lowerTag[4] == ' ' || lowerTag[4] == '\t' || lowerTag[4] == '>' || lowerTag[4] == '/')) {
            std::string classAttr = HtmlExtractAttr(tag, "class");
            if (HtmlClassHasToken(classAttr, "sector-theme") || HtmlClassHasToken(classAttr, "story-card"))
                pendingBreak = true;
        }

        i = gt + 1;
    }
    return out;
}

// ── remove_click_to_watch() ───────────────────────────────────────────────────
static std::string News_RemoveClickToWatch(const std::string& text) {
    static const std::vector<std::regex> patterns = {
        std::regex(R"(\[CLICK TO WATCH NOW\]\([^)]+\))", std::regex::icase),
        std::regex(R"(CLICK TO WATCH NOW)", std::regex::icase),
        std::regex(R"(Click to watch now)", std::regex::icase),
        std::regex(R"(https://youtube\.com/live/[^\s]+)", std::regex::icase),
    };
    std::string result = text;
    for (const auto& re : patterns)
        result = std::regex_replace(result, re, "");
    return result;
}

// ── remove_inline_sources() ───────────────────────────────────────────────────
static std::string News_RemoveInlineSources(const std::string& text) {
    static const std::regex sourcesLineRe(R"(^Sources?:$)", std::regex::icase);
    static const std::regex sourceNameRe(
        R"(^(WSJ|Barron|MarketWatch|FT|IBD|Caterpillar IR|onsemi IR|BP 6-K|HSBC IR|Kiplinger earnings|StockMarketWatch|Investors\.|AP Hormuz))");

    std::vector<std::string> lines;
    {
        std::istringstream ss(text);
        std::string line;
        while (std::getline(ss, line)) lines.push_back(line);
    }

    std::vector<std::string> output;
    bool skipNext = false;
    for (const auto& line : lines) {
        size_t b = line.find_first_not_of(" \t\r\n");
        size_t e = line.find_last_not_of(" \t\r\n");
        std::string stripped = (b == std::string::npos) ? "" : line.substr(b, e - b + 1);

        if (std::regex_search(stripped, sourcesLineRe)) { skipNext = true; continue; }

        if (skipNext) {
            bool endsWithSemi = !stripped.empty() && stripped.back() == ';';
            if (endsWithSemi || std::regex_search(stripped, sourceNameRe)) continue;
            skipNext = false;
        }
        if (stripped == "Source") continue;

        output.push_back(line);
    }

    std::string result;
    for (size_t i = 0; i < output.size(); ++i) { if (i) result += "\n"; result += output[i]; }
    return result;
}

// ── remove_attribution_footers() ──────────────────────────────────────────────
static std::string News_RemoveAttributionFooters(const std::string& text) {
    static const std::vector<std::regex> footerMarkers = {
        std::regex(R"(^Principal sources:)", std::regex::icase),
        std::regex(R"(^Sources:)", std::regex::icase),
        std::regex(R"(^Source:)", std::regex::icase),
        std::regex(R"(^\[Source\])", std::regex::icase),
        std::regex(R"(^<<<END_EXTERNAL)", std::regex::icase),
        std::regex(R"(^SECURITY NOTICE:)", std::regex::icase),
        std::regex(R"(^<<<EXTERNAL_UNTRUSTED_CONTENT)", std::regex::icase),
        std::regex(R"(^---)", std::regex::icase),
        std::regex(R"(^\[NYSE holiday)", std::regex::icase),
        std::regex(R"(^\[Trading Economics)", std::regex::icase),
        std::regex(R"(^\[MarketWatch)", std::regex::icase),
        std::regex(R"(^\[WSJ)", std::regex::icase),
        std::regex(R"(^\[Barron)", std::regex::icase),
        std::regex(R"(^\[MarketWatch movers)", std::regex::icase),
        std::regex(R"(^\[WSJ stocks)", std::regex::icase),
    };

    std::vector<std::string> lines;
    {
        std::istringstream ss(text);
        std::string line;
        while (std::getline(ss, line)) lines.push_back(line);
    }

    std::vector<std::string> output;
    bool footerStarted = false;
    for (const auto& line : lines) {
        size_t b = line.find_first_not_of(" \t\r\n");
        size_t e = line.find_last_not_of(" \t\r\n");
        std::string stripped = (b == std::string::npos) ? "" : line.substr(b, e - b + 1);

        for (const auto& marker : footerMarkers) {
            if (std::regex_search(stripped, marker)) { footerStarted = true; break; }
        }
        if (!footerStarted) output.push_back(line);
    }

    std::string result;
    for (size_t i = 0; i < output.size(); ++i) { if (i) result += "\n"; result += output[i]; }
    return result;
}

// ── Site-chrome trimming (nav / title / byline / footer boilerplate) ────────
// beehiiv's post template can (and, as of ~Aug 13 2026, did) change so the
// "content" div located by HtmlLocateArticleBody() ends up wrapping more than
// just the article — the top nav, Login/Subscribe bar, breadcrumbs, post
// title + author byline + share icons, and any sponsor block preceding it,
// plus the "TraderTV Live Morning Research Note" sign-off, logo, copyright,
// and policy/beehiiv links trailing it. Chasing the exact CSS class after
// every template tweak is fragile, so instead trim on the two literal
// markers that are stable across every daily post regardless of surrounding
// markup: real content always opens with the "Morning Market Setup" heading
// and always ends right before the "TraderTV Live Morning Research Note"
// sign-off line. If a marker isn't found (e.g. a future rename), that half
// is left untouched rather than guessed at.
static std::string News_TrimSiteChrome(const std::string& text) {
    std::string result = text;

    static const char* kBodyStartMarker = "Morning Market Setup";
    size_t startPos = result.find(kBodyStartMarker);
    if (startPos != std::string::npos) {
        result.erase(0, startPos);
    }

    static const char* kFooterStartMarker = "TraderTV Live Morning Research Note";
    size_t footerPos = result.find(kFooterStartMarker);
    if (footerPos != std::string::npos) {
        result.erase(footerPos);
    }

    return result;
}

// ── clean_whitespace() ────────────────────────────────────────────────────────
static std::string News_CleanWhitespace(const std::string& text) {
    static const std::regex manyNewlines(R"(\n{3,})");
    std::string collapsed = std::regex_replace(text, manyNewlines, "\n\n");

    std::vector<std::string> lines;
    {
        std::istringstream ss(collapsed);
        std::string line;
        while (std::getline(ss, line)) {
            size_t e = line.find_last_not_of(" \t\r");
            lines.push_back(e == std::string::npos ? "" : line.substr(0, e + 1));
        }
    }

    std::string joined;
    for (size_t i = 0; i < lines.size(); ++i) { if (i) joined += "\n"; joined += lines[i]; }

    size_t b = joined.find_first_not_of(" \t\r\n");
    size_t e = joined.find_last_not_of(" \t\r\n");
    return (b == std::string::npos) ? "" : joined.substr(b, e - b + 1);
}

// Fetch the TraderTV page via WinInet, then run the exact same pipeline as
// fetch_tradertv_watchlist.py's process_watchlist(): locate the article body,
// reformat market-data table cells, extract text (with section-header blank
// lines), strip the "click to watch" call-out, strip inline source lists,
// strip the attribution footer, then normalise whitespace.
// Returns empty string if the page is not found (404 / "doesn't exist" page).
static std::string News_FetchContent(const std::string& url, int day, int month, int year) {
    HINTERNET hInet = InternetOpenA("Mozilla/5.0 (compatible; OpenClawBot/1.0)", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInet) return "";

    HINTERNET hConn = InternetOpenUrlA(hInet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
    if (!hConn) { InternetCloseHandle(hInet); return ""; }

    std::string html;
    char chunk[8192];
    DWORD bytesRead = 0;
    while (InternetReadFile(hConn, chunk, sizeof(chunk), &bytesRead) && bytesRead > 0)
        html.append(chunk, bytesRead);

    InternetCloseHandle(hConn);
    InternetCloseHandle(hInet);

    // Check for 404 / not-found page (beehiiv returns a full page even for missing posts)
    if (html.find("doesn&#x27;t exist") != std::string::npos ||
        html.find("The page you requested doesn") != std::string::npos)
        return ""; // signal: not found

    std::string articleHtml = HtmlLocateArticleBody(html);
    articleHtml = HtmlFormatMarketCells(articleHtml);
    std::string text = HtmlExtractTextWithSectionBreaks(articleHtml);

    text = News_TrimSiteChrome(text); // strip nav/title/byline + footer chrome
    text = News_RemoveClickToWatch(text);
    text = News_RemoveInlineSources(text);
    text = News_RemoveAttributionFooters(text);
    text = News_CleanWhitespace(text);

    return text;
}

// Return a list of up to `num` trading dates counting backward from today
// (inclusive), oldest first. Mirrors Python get_trading_days().
static std::vector<SYSTEMTIME> News_GetTradingDays(int num) {
    SYSTEMTIME today;
    GetLocalTime(&today);

    std::vector<SYSTEMTIME> days;
    FILETIME ft;
    SystemTimeToFileTime(&today, &ft);
    ULONGLONG ns = ((ULONGLONG)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

    while ((int)days.size() < num) {
        FILETIME cur;
        cur.dwHighDateTime = (DWORD)(ns >> 32);
        cur.dwLowDateTime  = (DWORD)(ns & 0xFFFFFFFF);
        SYSTEMTIME st;
        FileTimeToSystemTime(&cur, &st);
        // wDayOfWeek: 0=Sun, 6=Sat
        if (st.wDayOfWeek != 0 && st.wDayOfWeek != 6)
            days.push_back(st);
        // Step back one day (100ns units)
        ns -= (ULONGLONG)24 * 3600 * 10000000;
    }

    // Reverse to get chronological order (oldest first)
    std::reverse(days.begin(), days.end());
    return days;
}

// Build the formatted day header string (mirrors Python print_day_header)
static std::string News_DayHeader(SYSTEMTIME st) {
    // Capitalise month name
    std::string month = s_monthNames[st.wMonth];
    if (!month.empty()) month[0] = (char)toupper(month[0]);

    std::string header;
    header += "\n";
    header += std::string(60, '=') + "\n";
    header += "  " + month + " " + std::to_string(st.wDay) + ", " + std::to_string(st.wYear) + "\n";
    header += std::string(60, '=') + "\n\n";
    return header;
}

// Shared implementation for /today and /week.
// `numDays` = 1 for /today, 5 for /week.
static std::string HandleGetNews(int numDays) {
    std::vector<SYSTEMTIME> days = News_GetTradingDays(numDays);

    std::string body;
    for (const auto& st : days) {
        std::string dateKey = std::format("{:04d}-{:02d}-{:02d}", st.wYear, st.wMonth, st.wDay);

        // Try registry cache first
        std::string content = News_LoadCache(dateKey);
        if (content.empty()) {
            // Fetch from the web
            std::string url = News_BuildUrl(st.wYear, st.wMonth, st.wDay);
            LogDebug("Fetching news: " + url);
            content = News_FetchContent(url, st.wDay, st.wMonth, st.wYear);
            if (content.empty()) {
                // Page not found for this date — skip it
                if (numDays != 1) body += "\n" + std::string(60, '=') + "\n  ";
                body += "News not yet published for " + News_FormatHeader(st) + ", please try again after 14:00";
                if (numDays != 1) body += ".\n";
                else body += ", or call GET /week.\n";
                if (numDays != 1) body += std::string(60, '=') + "\n\n";
                continue;
            }
            // Guard against permanently caching a fetch that still carries
    // site chrome the trim markers didn't catch (e.g. yet another
    // template change) — better to re-fetch next call than freeze
    // bad content in the registry for up to 7 days.
    bool looksClean = content.find("Powered by beehiiv") == std::string::npos &&
                      content.find("TraderTV Live Morning Research Note") == std::string::npos;
    if (looksClean) News_SaveCache(dateKey, content);
        } else {
            // body += "\n[Using cached data for " + News_FormatHeader(st) + "]\n";
        }

        body += News_DayHeader(st);
        body += content;
    }

    // Cleanup old registry entries (mirrors Python cleanup_old_cache)
    /*int deleted = */News_CleanupCache(7);
    //if (deleted > 0) body += "\n\n[Cleaned up " + std::to_string(deleted) + " expired cache entry(ies)]\n";

    return body;
}

// ── Simple JSON field extractor ──────────────────────────────────────────────
// Extracts the value of a JSON string field: "key":"value" → value
// Returns empty string if not found.
static std::string JsonExtractString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    // skip whitespace and colon
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return "";
    if (json[pos] == '"') {
        ++pos;
        std::string val;
        while (pos < json.size() && json[pos] != '"') val += json[pos++];
        return val;
    }
    return "";
}

// Extracts the value of a JSON number field: "key":123.45 → "123.45" (as string)
// Returns empty string if not found.
static std::string JsonExtractNumber(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return "";
    std::string val;
    while (pos < json.size() && (std::isdigit((unsigned char)json[pos]) || json[pos] == '.' || json[pos] == '-' || json[pos] == '+' || json[pos] == 'e' || json[pos] == 'E'))
        val += json[pos++];
    return val;
}

// ── POST /trade handler ───────────────────────────────────────────────────────

// Forward the trade to the external paper-trading dashboard.
// Uses WinInet (already linked for news fetching) so no new dependencies.
static void ForwardTradeToDashboard(const std::string& symbol, const std::string& side, double quantity, double price, double stopPrice, double profitPrice) {
    std::string profitPriceStr;
    if (profitPrice > 0.0) {
        profitPriceStr = ",\"profitPrice\":" + std::format("{:.2f}", profitPrice);
    }
    // Build JSON payload
    std::string payload = std::format(R"({{"symbol":"{}", "side":"{}", "quantity":{:.0f}, "price":{:.2f}, "stopPrice":{:.2f}{}}})", symbol, side, quantity, price, stopPrice, profitPriceStr);

    // Build the raw HTTP POST request
    // Host: 192.168.1.105:2025  Path: /paper?action=place_trade
    const std::string host    = "192.168.1.105";
    const int         port    = 2025;
    const std::string urlPath = "/paper?action=place_trade";

    std::string request;
    request += "POST " + urlPath + " HTTP/1.1\r\n";
    request += "Host: " + host + ":" + std::to_string(port) + "\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + std::to_string(payload.length()) + "\r\n";
    request += "Connection: close\r\n";
    request += "\r\n";
    request += payload;

    // Open a raw TCP socket to the dashboard
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        LogDebug("ForwardTrade: failed to create socket");
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((u_short)port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    // Set a short connect timeout (2 s) so we don't block the server thread
    DWORD timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        LogDebug("ForwardTrade: connect failed to " + host);
        closesocket(sock);
        return;
    }

    send(sock, request.c_str(), (int)request.size(), 0);

    // Read (and discard) the response so the server knows we consumed it
    char rbuf[512];
    while (recv(sock, rbuf, sizeof(rbuf) - 1, 0) > 0) {}

    closesocket(sock);
    LogDebug("ForwardTrade: forwarded " + side + " " + symbol + " to dashboard: " + payload);
}

// POST /trade
// Body: {"symbol":"AAPL","side":"BUY","quantity":10,"price":175.50}
static std::string HandlePostTrade(const std::string& body) {
    // ── Parse JSON fields ─────────────────────────────────────────────────────
    std::string symbol = JsonExtractString(body, "symbol");
    std::string side   = JsonExtractString(body, "side");
    std::string qtyStr = JsonExtractNumber(body, "quantity");
    std::string prxStr = JsonExtractNumber(body, "price");
    std::string stpStr = JsonExtractNumber(body, "stopPrice");
    std::string proStr = JsonExtractNumber(body, "profitPrice");

    if (symbol.empty() || side.empty() || qtyStr.empty() || prxStr.empty()) {
        return MakeHttpResponse(400, "Bad Request",
            "{\"error\":\"missing required fields: symbol, side, quantity, price\"}");
    }

    // Normalise
    std::transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);
    std::transform(side.begin(),   side.end(),   side.begin(),   ::toupper);
    if (side != "BUY" && side != "SELL") {
        return MakeHttpResponse(400, "Bad Request",
            "{\"error\":\"side must be BUY or SELL\"}");
    }

    double quantity = 0.0;
    double price    = 0.0;
    double stopPrice = 0.0;
    
    try {
        quantity = std::stod(qtyStr);
        price    = std::stod(prxStr);
    } catch (...) {
        return MakeHttpResponse(400, "Bad Request",
            "{\"error\":\"quantity and price and stopPrice must be numeric\"}");
    }

    try {
        stopPrice = std::max(0.0, std::stod(stpStr));
    } catch (...) {
        stopPrice = 0.0;
    }
    
    if (quantity <= 0.0 || price <= 0.0) {
        return MakeHttpResponse(400, "Bad Request",
            "{\"error\":\"quantity and price must be positive\"}");
    }
    if (quantity > 10.0) {
        return MakeHttpResponse(400, "Bad Request",
            "{\"error\":\"quantity must be less than 10\"}");
    }
    if (stopPrice <= 0.0 && side == "BUY") {
        return MakeHttpResponse(400, "Bad Request",
            "{\"error\":\"stopPrice must be positive for BUY orders\"}");
    }

    double profitPrice = 0.0;
    try {
        profitPrice = std::max(0.0, std::stod(proStr));
    } catch (...) {
        profitPrice = 0.0;
    }

    // BUY: stop-loss must sit below entry price
    if (stopPrice > 0.0) {
        if (side == "BUY" && stopPrice >= price) {
            return MakeHttpResponse(400, "Bad Request",
                "{\"error\":\"stopPrice must be lower than price for BUY orders\"}");
        }
        // SELL: stop-loss must sit above entry price
        if (side == "SELL" && stopPrice <= price) {
            return MakeHttpResponse(400, "Bad Request",
                "{\"error\":\"stopPrice must be higher than price for SELL orders\"}");
        }
    }
    // BUY: take-profit must sit above entry price
    if (profitPrice > 0.0) {
        if (side == "BUY" && profitPrice <= price) {
            return MakeHttpResponse(400, "Bad Request",
                "{\"error\":\"profitPrice must be higher than price for BUY orders\"}");
        }
        // SELL: take-profit must sit below entry price
        if (side == "SELL" && profitPrice >= price) {
            return MakeHttpResponse(400, "Bad Request",
                "{\"error\":\"profitPrice must be lower than price for SELL orders\"}");
        }
    }

    // ── Look up conId from live portfolio ─────────────────────────────────────
    int conId = 0;
    {
        std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
        const auto& map = api().getPortfolioMap();
        for (const auto& [id, pos] : map) {
            std::string sym = pos.symbol;
            std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
            if (sym == symbol) { conId = id; break; }
        }
    }

    // ── Forward to external dashboard first ───────────────────────────────────
    ForwardTradeToDashboard(symbol, side, quantity, price, stopPrice, profitPrice);

    // ── Submit the order to IBKR (transmit = false) ───────────────────────────
    std::thread([conId, symbol, side, quantity, price, stopPrice, profitPrice]() {
        HWND hDashboard = FindWindowA(DASHBOARD_CLASS_NAME, NULL);
        if (hDashboard && IsWindow(hDashboard)) {
            PostMessageA(hDashboard, WM_OPEN_ORDERS_WINDOW, 0, 0);
        }
        HWND hOrders = FindWindowA(ORDERS_CLASS_NAME, NULL);
        int max = 10;
        while (!hOrders || !IsWindow(hOrders)) {
            if (!max--) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(121));
            hOrders = FindWindowA(ORDERS_CLASS_NAME, NULL);
        }
        if (hOrders && IsWindow(hOrders)) {
            api().submitOrder(conId, symbol, side, false, quantity, price, stopPrice, price, profitPrice, false);
        }
    }).detach();

    /*LogDebug("POST /trade: " + side + " " + std::to_string((int)quantity) +
             " " + symbol + " @ " + std::to_string(price) +
             " stop=" + std::format("{:.2f}", stopPrice) +
             " profit=" + std::format("{:.2f}", profitPrice) +
             " conId=" + std::to_string(conId));*/

    // ── Build success response ────────────────────────────────────────────────
    std::string resp;
    resp += "{";
    resp += "\"status\":\"submitted\",";
    resp += "\"symbol\":\""   + JsonEscapeString(symbol)       + "\",";
    resp += "\"side\":\""     + JsonEscapeString(side)         + "\",";
    resp += "\"quantity\":"   + JsonDouble(quantity)           + ",";
    resp += "\"price\":"      + JsonDouble(price)              + ",";
    resp += "\"stopPrice\":"      + JsonDouble(stopPrice)      + ",";
    resp += "\"profitPrice\":"      + JsonDouble(profitPrice)  + ",";
    resp += "\"conId\":"      + std::to_string(conId);
    resp += "}";
    return MakeOk(resp);
}

// ── Request routing ───────────────────────────────────────────────────────────

static std::string RouteRequest(const std::string& rawRequest) {
    // Extract first line: "GET /path HTTP/1.1"
    size_t lineEnd = rawRequest.find("\r\n");
    if (lineEnd == std::string::npos) lineEnd = rawRequest.find("\n");
    if (lineEnd == std::string::npos) return MakeNotFound();

    std::string requestLine = rawRequest.substr(0, lineEnd);

    size_t sp1 = requestLine.find(' ');
    if (sp1 == std::string::npos) return MakeNotFound();
    std::string method = requestLine.substr(0, sp1);

    size_t sp2 = requestLine.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return MakeNotFound();
    std::string fullPath = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
    size_t qmark = fullPath.find('?');
    std::string path = (qmark != std::string::npos) ? fullPath.substr(0, qmark) : fullPath;

    LogDebug(std::string("HTTP request: ") + method + " " + path);

    // ── POST /trade ───────────────────────────────────────────────────────────
    if (path == "/trade" || path == "/trade/") {
        if (method != "POST") return MakeMethodNotAllowed();
        // Extract body: everything after the blank line separating headers from body
        std::string body;
        size_t bodyStart = rawRequest.find("\r\n\r\n");
        if (bodyStart != std::string::npos) body = rawRequest.substr(bodyStart + 4);
        else {
            size_t alt = rawRequest.find("\n\n");
            if (alt != std::string::npos) body = rawRequest.substr(alt + 2);
        }
        return HandlePostTrade(body);
    }

    if (method != "GET") return MakeMethodNotAllowed();
    
        // Route: GET /balance
    if (path == "/balance" || path == "/balance/") {
        return MakeOk(HandleGetBalance());
    }

    // Route: GET /positions
    if (path == "/positions" || path == "/positions/") {
        return MakeOk(HandleGetPositions());
    }

    // Route: GET /position/{SYMBOL}
    const std::string positionPrefix = "/position/";
    if (path.size() > positionPrefix.size() && path.substr(0, positionPrefix.size()) == positionPrefix) {
        std::string symbol = path.substr(positionPrefix.size());
        while (!symbol.empty() && symbol.back() == '/') symbol.pop_back();

        if (!symbol.empty()) {
            std::string body = HandleGetPositionBySymbol(symbol);
            if (!body.empty()) return MakeOk(body);
            return MakeNotFound("{\"error\":\"symbol not found\"}");
        }
    }
    
    // Route: GET /chart/year//{SYMBOL}
    const std::string chartYearPrefix = "/chart/year/";
    if (path.size() > chartYearPrefix.size() && path.substr(0, chartYearPrefix.size()) == chartYearPrefix) {
        std::string symbol = path.substr(chartYearPrefix.size());
        while (!symbol.empty() && symbol.back() == '/') symbol.pop_back();
        if (!symbol.empty()) return HandleGetHistory(symbol, "1 Y");
    }

    const std::string chartMonthPrefix = "/chart/month/";
    if (path.size() > chartMonthPrefix.size() && path.substr(0, chartMonthPrefix.size()) == chartMonthPrefix) {
        std::string symbol = path.substr(chartMonthPrefix.size());
        while (!symbol.empty() && symbol.back() == '/') symbol.pop_back();
        if (!symbol.empty()) return HandleGetHistory(symbol, "1 M");
    }

    const std::string chartWeekPrefix = "/chart/week/";
    if (path.size() > chartWeekPrefix.size() && path.substr(0, chartWeekPrefix.size()) == chartWeekPrefix) {
        std::string symbol = path.substr(chartWeekPrefix.size());
        while (!symbol.empty() && symbol.back() == '/') symbol.pop_back();
        if (!symbol.empty()) return HandleGetHistory(symbol, "1 W");
    }

    const std::string chartDayPrefix = "/chart/day/";
    if (path.size() > chartDayPrefix.size() && path.substr(0, chartDayPrefix.size()) == chartDayPrefix) {
        std::string symbol = path.substr(chartDayPrefix.size());
        while (!symbol.empty() && symbol.back() == '/') symbol.pop_back();
        if (!symbol.empty()) return HandleGetHistory(symbol, "1 D");
    }

    // Route: GET /news/today
    if (path == "/news/today" || path == "/news/today/") {
        return MakePlainText(HandleGetNews(1));
    }

    // Route: GET /news/week
    if (path == "/news/week" || path == "/news/week/") {
        return MakePlainText(HandleGetNews(5));
    }

    return MakeNotFound("{\"error\":\"unknown endpoint\"}");
}

// ── Per-connection handler ────────────────────────────────────────────────────

static void HandleHttpClient(SOCKET client) {
    std::string buf;
    buf.reserve(2048);
    char tmp[1024];
    while (true) {
        int n = recv(client, tmp, sizeof(tmp) - 1, 0);
        if (n <= 0) break;
        tmp[n] = '\0';
        buf.append(tmp, n);
        // For GET requests stop after headers; for POST keep reading the body too
        if (buf.find("\r\n\r\n") != std::string::npos) {
            // If this is a POST, also wait for the Content-Length bytes
            if (buf.substr(0, 4) == "POST") {
                // Parse Content-Length header
                size_t clPos = buf.find("Content-Length:");
                if (clPos == std::string::npos) clPos = buf.find("content-length:");
                if (clPos != std::string::npos) {
                    size_t valStart = buf.find_first_not_of(" \t", clPos + 15);
                    size_t valEnd   = buf.find('\r', valStart);
                    int contentLen  = std::stoi(buf.substr(valStart, valEnd - valStart));
                    size_t hdEnd    = buf.find("\r\n\r\n") + 4;
                    // Keep reading until we have the full body
                    while ((int)(buf.size() - hdEnd) < contentLen) {
                        int m = recv(client, tmp, sizeof(tmp) - 1, 0);
                        if (m <= 0) break;
                        tmp[m] = '\0';
                        buf.append(tmp, m);
                    }
                }
            }
            break;
        }
        if (buf.size() > 32768) break;
    }

    if (!buf.empty()) {
        std::string response = RouteRequest(buf);
        send(client, response.c_str(), (int)response.size(), 0);
    }

    closesocket(client);
}

// ── Server loop ───────────────────────────────────────────────────────────────

static void HttpServerLoop() {
    while (g_httpRunning.load()) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(g_httpListenSocket, &readSet);

        timeval tv{ 0, 500000 }; // 500 ms poll interval
        int sel = select(0, &readSet, nullptr, nullptr, &tv);
        if (sel <= 0) continue;

        SOCKET client = accept(g_httpListenSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) continue;

        HandleHttpClient(client);
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

static bool HttpServer_Start() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;

    g_httpListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_httpListenSocket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    BOOL yes = TRUE;
    setsockopt(g_httpListenSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(HTTP_SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 — all interfaces (LAN accessible)

    if (bind(g_httpListenSocket,
             reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(g_httpListenSocket);
        g_httpListenSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    if (listen(g_httpListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(g_httpListenSocket);
        g_httpListenSocket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    g_httpRunning.store(true);
    g_httpThread = std::thread(HttpServerLoop);

    LogDebug(std::string("HTTP API listening on 127.0.0.1:") +
             std::to_string(HTTP_SERVER_PORT));
    return true;
}

static void HttpServer_Stop() {
    g_httpRunning.store(false);

    if (g_httpListenSocket != INVALID_SOCKET) {
        closesocket(g_httpListenSocket);
        g_httpListenSocket = INVALID_SOCKET;
    }

    if (g_httpThread.joinable()) {
        g_httpThread.join();
    }

    WSACleanup();
}

class HttpServerRAII {
public:
    HttpServerRAII() {
        HttpServer_Start();
    }
    ~HttpServerRAII() {
        HttpServer_Stop();
    }
};

#endif