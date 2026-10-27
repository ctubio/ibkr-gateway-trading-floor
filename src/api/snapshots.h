#pragma once

struct OrderInfo {
    int orderId = 0;
    int conId = 0;
    std::string symbol;
    std::string exchange;
    std::string action;
    std::string orderType;
    std::string tif;
    double price = 0.0;
    double fullStopPrice = 0.0;
    double fullProfitPrice = 0.0;
    double totalQty = 0.0;
    double filledQty = 0.0;
    double avgFillPx = 0.0;
    double trailStopPrice = 0.0;
    std::string status;
    std::string time;
    bool includeOvernight = false;
    bool outsideRth = false;
    int parentId = 0;
    std::string ocaGroup;
    int ocaType = 0;
};

struct PnlSinglePayload {
    int conId = 0;
    double dailyPnL = 0.0;
    double unrealizedPnL = 0.0;
    double realizedPnL = 0.0;
};

struct PositionInfo {
    int conId = 0;
    std::string symbol;
    std::string exchange;
    double shares = 0.0;
    double avgCost = 0.0;
    PnlSinglePayload pnlSingle;
    bool isWatchOnly = false;
    double closeAgo13Week = 0.0;
    double closeAgo26Week = 0.0;
    double closeAgo52Week = 0.0;
};

struct L1Book {
    std::string symbol;
    double last = 0.0;
    double prevClose = 0.0;
    double open = 0.0;
    double bid = 0.0;
    double ask = 0.0;
    double high = 0.0;
    double low = 0.0;
    double vwap = 0.0;
    double bidSize = 0.0;
    double askSize = 0.0;
    double low13 = 0.0;
    double high13 = 0.0;
    double low26 = 0.0;
    double high26 = 0.0;
    double low52 = 0.0;
    double high52 = 0.0;
    double annualDividends = 0.0;
    double dividendAmount = 0.0;
    std::string dividendDate;
    double dividendDateSortable = 0.0;
    bool halted = false;

    double change() const { return (prevClose > 0 && last > 0) ? last - prevClose : 0.0; }
    double changePct() const { return (prevClose > 0 && last > 0) ? (last - prevClose) / prevClose * 100.0 : 0.0; }
    double dividendYield() const {
        return (last > 0 && annualDividends > 0)
            ? (annualDividends / last * 100.0)
            : ((prevClose > 0 && annualDividends > 0) ? (annualDividends / prevClose * 100.0) : 0.0);
    }

    double RangePct(double lo, double hi) const {
        double range = hi - lo;
        if (range <= 0 || last <= 0) return 0.0;
        return (last - lo) / range * 100.0;
    }
    double Week52RangePct() const { return RangePct(low52, high52); }
    double Week26RangePct() const { return RangePct(low26, high26); }
    double Week13RangePct() const { return RangePct(low13, high13); }

    double rtVolSum5min = 0.0;
    double rtVolRatio5min = 0.0;
};

struct L2Book {
    struct Row {
        double price = 0.0;
        double size = 0.0;
        bool populated = false;
    };

    std::array<Row, 12> rows{};
};

template<class T> using Snap = std::shared_ptr<const T>;

struct AccountSnap {
    std::map<std::string, std::string> summary;
    double daily = 0.0;
    double unrealized = 0.0;
    double realized = 0.0;
    std::string accountId;
};

using PositionMap = std::map<int, PositionInfo>;
using OrderMap = std::map<int, OrderInfo>;
using ExecList = std::vector<OrderInfo>;
using SymbolList = std::vector<std::string>;

inline std::vector<OrderInfo> orderSort(const OrderMap& orders) {
    std::vector<OrderInfo> sorted;
    sorted.reserve(orders.size());
    for (const auto& [id, order] : orders) sorted.push_back(order);

    auto priority = [](const std::string& status) {
        if (status == "Submitted" || status == "PreSubmitted" ||
            status == "PendingSubmit" || status == "Pending") return 0;
        if (status == "Partially Filled") return 1;
        if (status == "Filled" || status == "Cancelled" ||
            status == "Inactive" || status == "PendingCancel") return 2;
        return 3;
    };
    std::sort(sorted.begin(), sorted.end(), [&](const OrderInfo& a, const OrderInfo& b) {
        const int aPriority = priority(a.status);
        const int bPriority = priority(b.status);
        if (aPriority != bPriority) return aPriority < bPriority;
        return a.orderId > b.orderId;
    });
    return sorted;
}

class SnapshotStore {
public:
    Snap<AccountSnap> account() const { return account_.load(std::memory_order_acquire); }
    Snap<PositionMap> positions() const { return positions_.load(std::memory_order_acquire); }
    Snap<OrderMap> orders() const { return orders_.load(std::memory_order_acquire); }
    Snap<ExecList> executions() const { return executions_.load(std::memory_order_acquire); }
    Snap<L1Book> fx() const { return fx_.load(std::memory_order_acquire); }
    Snap<SymbolList> symbols() const { return symbols_.load(std::memory_order_acquire); }

    Snap<L1Book> quote(int conId) const {
        std::shared_lock<std::shared_mutex> lock(quotesMutex_);
        auto it = quotes_.find(conId);
        return it == quotes_.end() ? Snap<L1Book>{} : it->second;
    }

    Snap<L2Book> book(int conId) const {
        std::shared_lock<std::shared_mutex> lock(booksMutex_);
        auto it = books_.find(conId);
        return it == books_.end() ? Snap<L2Book>{} : it->second;
    }

    void publishAccount(Snap<AccountSnap> value) {
        account_.store(value ? std::move(value) : std::make_shared<const AccountSnap>(), std::memory_order_release);
    }
    void publishPositions(Snap<PositionMap> value) {
        positions_.store(value ? std::move(value) : std::make_shared<const PositionMap>(), std::memory_order_release);
    }
    void publishOrders(Snap<OrderMap> value) {
        orders_.store(value ? std::move(value) : std::make_shared<const OrderMap>(), std::memory_order_release);
    }
    void publishExecutions(Snap<ExecList> value) {
        executions_.store(value ? std::move(value) : std::make_shared<const ExecList>(), std::memory_order_release);
    }
    void publishFx(Snap<L1Book> value) {
        fx_.store(value ? std::move(value) : std::make_shared<const L1Book>(), std::memory_order_release);
    }
    void publishSymbols(Snap<SymbolList> value) {
        symbols_.store(value ? std::move(value) : std::make_shared<const SymbolList>(), std::memory_order_release);
    }

    void setQuote(int conId, Snap<L1Book> value) {
        std::unique_lock<std::shared_mutex> lock(quotesMutex_);
        if (value) quotes_.insert_or_assign(conId, std::move(value));
        else quotes_.erase(conId);
    }
    void dropQuote(int conId) {
        std::unique_lock<std::shared_mutex> lock(quotesMutex_);
        quotes_.erase(conId);
    }
    void clearQuotes() {
        std::unique_lock<std::shared_mutex> lock(quotesMutex_);
        quotes_.clear();
    }
    void setBook(int conId, Snap<L2Book> value) {
        std::unique_lock<std::shared_mutex> lock(booksMutex_);
        if (value) books_.insert_or_assign(conId, std::move(value));
        else books_.erase(conId);
    }
    void dropBook(int conId) {
        std::unique_lock<std::shared_mutex> lock(booksMutex_);
        books_.erase(conId);
    }
    void clearBooks() {
        std::unique_lock<std::shared_mutex> lock(booksMutex_);
        books_.clear();
    }

private:
    std::atomic<Snap<AccountSnap>> account_{std::make_shared<const AccountSnap>()};
    std::atomic<Snap<PositionMap>> positions_{std::make_shared<const PositionMap>()};
    std::atomic<Snap<OrderMap>> orders_{std::make_shared<const OrderMap>()};
    std::atomic<Snap<ExecList>> executions_{std::make_shared<const ExecList>()};
    std::atomic<Snap<L1Book>> fx_{std::make_shared<const L1Book>()};
    std::atomic<Snap<SymbolList>> symbols_{std::make_shared<const SymbolList>()};
    mutable std::shared_mutex quotesMutex_;
    std::map<int, Snap<L1Book>> quotes_;
    mutable std::shared_mutex booksMutex_;
    std::map<int, Snap<L2Book>> books_;
};
