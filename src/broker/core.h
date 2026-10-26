#pragma once

#include <cassert>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <windows.h>

#include "../api/volume.h"
#include "../domain/snapshots.h"

struct CoreState {
    struct PacedJobs {
        struct Job {
            std::function<void()> run;
            std::chrono::milliseconds delayAfter{0};
        };

        std::deque<Job> jobs;
        bool timerArmed = false;
    };

    struct SyncContract {
        std::string symbol;
        int conId = 0;
        std::promise<int> done;
        bool fulfilled = false;
    };

    struct SyncHistorical {
        std::vector<std::string> rows;
        std::promise<std::vector<std::string>> done;
        bool fulfilled = false;
    };

    struct MarketInstrument {
        int reqIdMarketDataL1 = 0;
        int reqIdMarketDataL2 = 0;
        int l2Retries = 0;
        int reqIdPnLSingle = 0;
        int reqIdTicksOvernight = 0;
        int reqIdTicksSmart = 0;
        int l1Holders = 0;
        bool l2Holder = false;
        L1Book L1BookData;
        std::vector<L2Book::Row> askLevels;
        std::vector<L2Book::Row> bidLevels;

        struct VolRateState {
            double lastTotalVolume = -1.0;
            RollingRateTracker tracker;
            bool spikeFired1_5 = false;
            bool spikeFired3 = false;
        } volRate;
    };

    struct ContractMeta {
        std::string secType;
        std::string currency;
        std::string primaryExch;
    };

    struct PriceAlertState {
        std::string symbol;
        double up = 0.0;
        double down = 0.0;
        bool firedUp = false;
        bool firedDown = false;
    };

    std::map<int, MarketInstrument> instruments;
    std::set<int> marketConIds;
    std::map<HWND, int> marketWindowConIds;
    std::unordered_map<int, int> reqIdToConId;
    std::map<int, PositionInfo> portfolioMap;
    OrderMap ordersMap;
    bool openOrdersSnapshotInProgress = false;
    bool completedOrdersSnapshotInProgress = false;
    int nextExecId = 0;
    std::map<int, OrderInfo> executionMap;
    L1Book fxRateBook;
    SymbolList symbolResults;
    std::map<std::string, std::string> summaryData;
    double dailyPnL = 0.0;
    double unrealizedPnL = 0.0;
    double realizedPnL = 0.0;
    std::string accountId;
    bool pnlRequested = false;

    std::unordered_map<int, ContractMeta> contractCache;
    std::unordered_map<int, double> priceEvents;
    std::unordered_map<int, double> priceEventsAlreadyRecorded;
    std::unordered_map<int, PriceAlertState> priceAlerts;
    std::unordered_map<int, int> weeklyRangeReqIdToConId;
    std::unordered_map<int, std::vector<std::pair<std::string, double>>> weeklyRangeBarsCache;
    std::unordered_set<int> weeklyRangeInFlight;
    std::unordered_map<int, int> dividendReqIdToConId;
    std::unordered_set<int> dividendInFlight;
    std::unordered_set<int> dividendFetchedThisSession;
    std::unordered_map<int, SyncContract> contractRequests;
    std::unordered_map<std::string, int> resolvedSymbolCache;
    std::unordered_map<int, SyncHistorical> historicalRequests;
    PacedJobs pacedJobs;
    PacedJobs lowPriorityJobs;
};

class Core {
public:
    using Task = std::function<void(CoreState&)>;

    Core() = default;
    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;

    ~Core() { stop(); }

    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (started_ || stopping_) return;
        started_ = true;
        try {
            worker_ = std::thread([this] { run(); });
        } catch (...) {
            started_ = false;
            throw;
        }
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!stopping_) {
                stopping_ = true;
                while (!delayed_.empty()) delayed_.pop();
            }
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    void post(Task task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!started_ || stopping_) return;
            tasks_.push_back(std::move(task));
        }
        cv_.notify_one();
    }

    void postDelayed(std::chrono::milliseconds delay, Task task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!started_ || stopping_) return;
            delayed_.push(DelayedTask{
                std::chrono::steady_clock::now() + delay,
                nextSequence_++,
                std::move(task)
            });
        }
        cv_.notify_one();
    }

    bool onCoreThread() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return workerId_ == std::this_thread::get_id();
    }

    void postAndWait(Task task) {
        assert(!onCoreThread());
        auto completion = std::make_shared<std::promise<void>>();
        auto result = completion->get_future();
        const bool accepted = enqueue([task = std::move(task), completion](CoreState& state) mutable {
            try {
                task(state);
                completion->set_value();
            } catch (...) {
                completion->set_exception(std::current_exception());
                throw;
            }
        });
        if (!accepted) throw std::runtime_error("Core is not running");
        result.get();
    }

private:
    struct DelayedTask {
        std::chrono::steady_clock::time_point due;
        std::uint64_t sequence;
        Task task;
    };

    struct EarlierTask {
        bool operator()(const DelayedTask& lhs, const DelayedTask& rhs) const {
            if (lhs.due != rhs.due) return lhs.due > rhs.due;
            return lhs.sequence > rhs.sequence;
        }
    };

    bool enqueue(Task task) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!started_ || stopping_) return false;
            tasks_.push_back(std::move(task));
        }
        cv_.notify_one();
        return true;
    }

    void run() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            workerId_ = std::this_thread::get_id();
        }

        CoreState state;
        for (;;) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                for (;;) {
                    if (stopping_) {
                        if (tasks_.empty()) return;
                        break;
                    }
                    if (!tasks_.empty()) break;
                    if (delayed_.empty()) {
                        cv_.wait(lock);
                    } else if (cv_.wait_until(lock, delayed_.top().due) == std::cv_status::timeout) {
                        if (std::chrono::steady_clock::now() >= delayed_.top().due) break;
                    }
                }

                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop_front();
                } else {
                    task = delayed_.top().task;
                    delayed_.pop();
                }
            }

            try {
                task(state);
            } catch (...) {
            }
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<Task> tasks_;
    std::priority_queue<DelayedTask, std::vector<DelayedTask>, EarlierTask> delayed_;
    std::thread worker_;
    std::thread::id workerId_;
    std::uint64_t nextSequence_ = 0;
    bool started_ = false;
    bool stopping_ = false;
};
