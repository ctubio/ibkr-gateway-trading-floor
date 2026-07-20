#pragma once

// ─── Lightweight HTTP API Server ─────────────────────────────────────────────
//
//  Serves endpoints over a background thread using raw Winsock (no extra libs):
//
//    GET /account            → JSON object with account number, PnL, and summary map
//    GET /portfolio          → JSON array of all positions from api().getPortfolioMap()
//    GET /portfolio/{SYMBOL} → JSON object for the single position matching SYMBOL
//
//  The server listens on 127.0.0.1:PORT (default 7779) so it is only reachable
//  from the local machine.  Call HttpServer_Start() once after WinMain initialises
//  and HttpServer_Stop() before the process exits.
//
//  JSON is hand-built (no external library required).
// ─────────────────────────────────────────────────────────────────────────────

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
static std::string PriceToJson(const TradingAPI::PositionInfo& p) {
    TradingAPI::L1Book l1;
    api().getWatchlistData(p.conId, l1);

    std::string j;
    j.reserve(64);
    j += "{";
    j += "\"" + JsonEscapeString(p.symbol) + "\":" + JsonDouble(l1.last);
    j += "}";
    return j;
}

// Serialise one PositionInfo to a JSON object string.
static std::string PositionToJson(const TradingAPI::PositionInfo& p) {
    TradingAPI::L1Book l1;
    api().getWatchlistData(p.conId, l1);

    double unrealizedPct = 0;
    if (p.avgCost > 0.0 && l1.last > 0.0 && p.shares != 0.0) {
        double sign = (p.shares < 0.0) ? -1.0 : 1.0;
        unrealizedPct = (l1.last - p.avgCost) / p.avgCost * 100.0 * sign;
    }

    std::string j;
    j.reserve(512);
    j += "{";
    j += "\"conId\":"              + std::to_string(p.conId)                 + ",";
    j += "\"symbol\":\""           + JsonEscapeString(p.symbol)              + "\",";
    j += "\"exchange\":\""         + JsonEscapeString(p.exchange)            + "\",";
    j += "\"shares\":"             + std::format("{:.0f}", p.shares)         + ",";
    j += "\"avgCost\":"            + JsonDouble(p.avgCost)                   + ",";
    j += "\"last\":"               + JsonDouble(l1.last)                     + ",";
    j += "\"marketValue\":"        + JsonDouble(p.shares * l1.last)          + ",";
    j += "\"change_pct\":\""       + std::format("{:+.2f}%", l1.changePct()) + "\",";
    j += "\"dailyPnL\":"           + JsonDouble(p.pnlSingle.dailyPnL)        + ",";
    j += "\"unrealizedPnL\":"      + JsonDouble(p.pnlSingle.unrealizedPnL)   + ",";
    j += "\"unrealizedPnL_pct\":\"" + std::format("{:+.2f}%", unrealizedPct) + "\"";
    j += "}";
    return j;
}

// ── Endpoint handlers ─────────────────────────────────────────────────────────

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

// GET /price/{SYMBOL}  →  JSON object for a single price (or empty on miss)
static std::string HandleGetPriceBySymbol(const std::string& symbol) {
    std::string upper = symbol;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    std::lock_guard<std::mutex> lock(api().getPortfolioMutex());
    const auto& map = api().getPortfolioMap();

    for (const auto& [conId, pos] : map) {
        std::string posSymbol = pos.symbol;
        std::transform(posSymbol.begin(), posSymbol.end(), posSymbol.begin(), ::toupper);
        if (posSymbol == upper) {
            return PriceToJson(pos);
        }
    }
    return "";
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

static std::string MakeOk(const std::string& body) {
    return MakeHttpResponse(200, "OK", body);
}

static std::string MakeNotFound(const std::string& msg = "{\"error\":\"not found\"}") {
    return MakeHttpResponse(404, "Not Found", msg);
}

static std::string MakeMethodNotAllowed() {
    return MakeHttpResponse(405, "Method Not Allowed", "{\"error\":\"method not allowed\"}");
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

    if (method != "GET") return MakeMethodNotAllowed();
    
    LogDebug(std::string("HTTP request: ") + method + " " + path);
    
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

    // Route: GET /price/{SYMBOL}
    const std::string pricePrefix = "/price/";
    if (path.size() > pricePrefix.size() && path.substr(0, pricePrefix.size()) == pricePrefix) {
        std::string symbol = path.substr(pricePrefix.size());
        while (!symbol.empty() && symbol.back() == '/') symbol.pop_back();

        if (!symbol.empty()) {
            std::string body = HandleGetPriceBySymbol(symbol);
            if (!body.empty()) return MakeOk(body);
            return MakeNotFound("{\"error\":\"symbol not found\"}");
        }
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
        if (buf.find("\r\n\r\n") != std::string::npos) break;
        if (buf.size() > 8192) break;
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