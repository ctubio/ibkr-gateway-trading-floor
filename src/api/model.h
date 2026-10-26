#pragma once

#include <windows.h>

#include <cassert>
#include <functional>
#include <map>
#include <mutex>
#include <span>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "dispatcher.h"

void LogDebug(const std::string& msg);

struct Tick {
    double price;
    double size;
    std::string time;
    std::string exchange;
};

enum class Topic { Connection, Account, Positions, Orders, Executions, FxRate, SymbolResults, Alerts, TtsVoice };

class Model {
    enum class SubscriptionKind { Topic, Quote, AnyQuote, Pnl, AnyPnl, OrdersFor, Ticks, Book };

public:
    class Token {
    public:
        Token() = default;
        Token(const Token&) = delete;
        Token& operator=(const Token&) = delete;

        Token(Token&& other) noexcept
            : model_(std::exchange(other.model_, nullptr)),
              kind_(other.kind_),
              topic_(other.topic_),
              conId_(other.conId_),
              id_(std::exchange(other.id_, 0)) {}

        Token& operator=(Token&& other) noexcept {
            if (this == &other) return *this;
            reset();
            model_ = std::exchange(other.model_, nullptr);
            kind_ = other.kind_;
            topic_ = other.topic_;
            conId_ = other.conId_;
            id_ = std::exchange(other.id_, 0);
            return *this;
        }

        ~Token() { reset(); }

        void reset() {
            if (!model_) return;
            model_->unsubscribe(kind_, topic_, conId_, id_);
            model_ = nullptr;
            id_ = 0;
        }

    private:
        friend class Model;
        Token(Model* model, SubscriptionKind kind, Topic topic, int conId, std::size_t id)
            : model_(model), kind_(kind), topic_(topic), conId_(conId), id_(id) {}

        Model* model_ = nullptr;
        SubscriptionKind kind_ = SubscriptionKind::Topic;
        Topic topic_ = Topic::Connection;
        int conId_ = 0;
        std::size_t id_ = 0;
    };

    Model() = default;

    void initializeUiThread() {
        const DWORD currentThreadId = GetCurrentThreadId();
        if (uiThreadId_ != 0 && uiThreadId_ != currentThreadId)
            throw std::logic_error("Model UI thread is already initialized");
        uiThreadId_ = currentThreadId;
    }

    Token onTopic(Topic topic, std::function<void()> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        subscribers_[topic].emplace(id, std::move(callback));
        return Token(this, SubscriptionKind::Topic, topic, 0, id);
    }

    Token onQuote(int conId, std::function<void()> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        quoteSubscribers_[conId].emplace(id, std::move(callback));
        return Token(this, SubscriptionKind::Quote, Topic::Connection, conId, id);
    }

    Token onAnyQuote(std::function<void(int conId)> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        anyQuoteSubscribers_.emplace(id, std::move(callback));
        return Token(this, SubscriptionKind::AnyQuote, Topic::Connection, 0, id);
    }

    Token onPnl(int conId, std::function<void()> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        pnlSubscribers_[conId].emplace(id, std::move(callback));
        return Token(this, SubscriptionKind::Pnl, Topic::Connection, conId, id);
    }

    Token onOrdersFor(int conId, std::function<void()> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        ordersForSubscribers_[conId].emplace(id, std::move(callback));
        return Token(this, SubscriptionKind::OrdersFor, Topic::Connection, conId, id);
    }

    Token onTicks(int conId, std::function<void(std::span<const Tick>)> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        tickSubscribers_[conId].emplace(id, std::move(callback));
        {
            std::lock_guard<std::mutex> lock(tickMutex_);
            ++tickSubscriberCounts_[conId];
        }
        return Token(this, SubscriptionKind::Ticks, Topic::Connection, conId, id);
    }

    Token onBook(int conId, std::function<void()> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        bookSubscribers_[conId].emplace(id, std::move(callback));
        return Token(this, SubscriptionKind::Book, Topic::Connection, conId, id);
    }

    Token onAnyPnl(std::function<void(int conId)> callback) {
        assertUiThread();
        const std::size_t id = nextSubscriberId_++;
        anyPnlSubscribers_.emplace(id, std::move(callback));
        return Token(this, SubscriptionKind::AnyPnl, Topic::Connection, 0, id);
    }

    void markDirty(Topic topic) {
        bool needWake = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dirty_.insert(topic);
            needWake = !flushPending_;
            flushPending_ = true;
        }
        if (needWake)
            postFlush();
    }

    void markQuoteDirty(int conId) {
        bool needWake = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dirtyQuotes_.insert(conId);
            needWake = !flushPending_;
            flushPending_ = true;
        }
        if (needWake)
            postFlush();
    }

    void markBookDirty(int conId) {
        bool needWake = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dirtyBooks_.insert(conId);
            needWake = !flushPending_;
            flushPending_ = true;
        }
        if (needWake)
            postFlush();
    }

    void markPnlDirty(int conId) {
        bool needWake = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dirtyPnl_.insert(conId);
            needWake = !flushPending_;
            flushPending_ = true;
        }
        if (needWake)
            postFlush();
    }

    void markOrdersDirty(int conId) {
        bool needWake = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dirtyOrdersFor_.insert(conId);
            needWake = !flushPending_;
            flushPending_ = true;
        }
        if (needWake)
            postFlush();
    }

    void pushTick(int conId, Tick tick) {
        bool needWake = false;
        bool logOverflow = false;
        {
            std::lock_guard<std::mutex> lock(tickMutex_);
            if (!tickSubscriberCounts_.contains(conId)) return;

            auto& buffer = tickBuffers_[conId];
            constexpr std::size_t maxTicks = 10000;
            if (buffer.ticks.size() == maxTicks) {
                buffer.ticks.erase(buffer.ticks.begin());
                if (!buffer.overflowLogged) {
                    buffer.overflowLogged = true;
                    logOverflow = true;
                }
            }
            buffer.ticks.push_back(std::move(tick));
            needWake = !buffer.flushPending;
            buffer.flushPending = true;
        }

        if (logOverflow)
            UiDispatcher::instance().post([] {
                LogDebug("Model tick buffer overflow; oldest ticks are being dropped");
            });
        if (needWake)
            UiDispatcher::instance().post(
                [this, conId] { flushTicks(conId); },
                [this, conId] {
                    std::lock_guard<std::mutex> lock(tickMutex_);
                    auto buffer = tickBuffers_.find(conId);
                    if (buffer == tickBuffers_.end()) return;
                    buffer->second.flushPending = false;
                    if (!tickSubscriberCounts_.contains(conId))
                        tickBuffers_.erase(buffer);
                });
    }

private:
    using Subscribers = std::map<std::size_t, std::function<void()>>;
    using SymbolSubscribers = std::map<int, Subscribers>;
    using TickSubscribers = std::map<std::size_t, std::function<void(std::span<const Tick>)>>;

    struct TickBuffer {
        std::vector<Tick> ticks;
        bool flushPending = false;
        bool overflowLogged = false;
    };

    DWORD uiThreadId_ = 0;
    std::mutex mutex_;
    std::set<Topic> dirty_;
    std::set<int> dirtyQuotes_;
    std::set<int> dirtyBooks_;
    std::set<int> dirtyPnl_;
    std::set<int> dirtyOrdersFor_;
    bool flushPending_ = false;
    std::map<Topic, Subscribers> subscribers_;
    SymbolSubscribers quoteSubscribers_;
    std::map<std::size_t, std::function<void(int)>> anyQuoteSubscribers_;
    SymbolSubscribers pnlSubscribers_;
    std::map<std::size_t, std::function<void(int)>> anyPnlSubscribers_;
    SymbolSubscribers ordersForSubscribers_;
    std::map<int, TickSubscribers> tickSubscribers_;
    std::map<int, Subscribers> bookSubscribers_;
    std::size_t nextSubscriberId_ = 1;
    std::mutex tickMutex_;
    std::map<int, std::size_t> tickSubscriberCounts_;
    std::map<int, TickBuffer> tickBuffers_;

    void assertUiThread() const {
        assert(uiThreadId_ != 0 && GetCurrentThreadId() == uiThreadId_);
    }

    void postFlush() {
        UiDispatcher::instance().post(
            [this] { flush(); },
            [this] {
                std::lock_guard<std::mutex> lock(mutex_);
                flushPending_ = false;
            });
    }

    void unsubscribe(SubscriptionKind kind, Topic topic, int conId, std::size_t id) {
        assertUiThread();
        if (kind == SubscriptionKind::Quote) {
            auto quoteIt = quoteSubscribers_.find(conId);
            if (quoteIt == quoteSubscribers_.end()) return;
            quoteIt->second.erase(id);
            if (quoteIt->second.empty()) quoteSubscribers_.erase(quoteIt);
            return;
        }
        if (kind == SubscriptionKind::AnyQuote) {
            anyQuoteSubscribers_.erase(id);
            return;
        }
        if (kind == SubscriptionKind::Pnl) {
            auto pnlIt = pnlSubscribers_.find(conId);
            if (pnlIt == pnlSubscribers_.end()) return;
            pnlIt->second.erase(id);
            if (pnlIt->second.empty()) pnlSubscribers_.erase(pnlIt);
            return;
        }
        if (kind == SubscriptionKind::AnyPnl) {
            anyPnlSubscribers_.erase(id);
            return;
        }
        if (kind == SubscriptionKind::OrdersFor) {
            auto ordersIt = ordersForSubscribers_.find(conId);
            if (ordersIt == ordersForSubscribers_.end()) return;
            ordersIt->second.erase(id);
            if (ordersIt->second.empty()) ordersForSubscribers_.erase(ordersIt);
            return;
        }
        if (kind == SubscriptionKind::Ticks) {
            auto ticksIt = tickSubscribers_.find(conId);
            if (ticksIt != tickSubscribers_.end()) {
                ticksIt->second.erase(id);
                if (ticksIt->second.empty()) tickSubscribers_.erase(ticksIt);
            }
            std::lock_guard<std::mutex> lock(tickMutex_);
            auto countIt = tickSubscriberCounts_.find(conId);
            if (countIt == tickSubscriberCounts_.end()) return;
            if (--countIt->second == 0) {
                tickSubscriberCounts_.erase(countIt);
                auto bufferIt = tickBuffers_.find(conId);
                if (bufferIt != tickBuffers_.end()) {
                    bufferIt->second.ticks.clear();
                    bufferIt->second.overflowLogged = false;
                    if (!bufferIt->second.flushPending)
                        tickBuffers_.erase(bufferIt);
                }
            }
            return;
        }
        if (kind == SubscriptionKind::Book) {
            auto booksIt = bookSubscribers_.find(conId);
            if (booksIt == bookSubscribers_.end()) return;
            booksIt->second.erase(id);
            if (booksIt->second.empty()) bookSubscribers_.erase(booksIt);
            return;
        }

        auto topicIt = subscribers_.find(topic);
        if (topicIt == subscribers_.end()) return;
        topicIt->second.erase(id);
        if (topicIt->second.empty()) subscribers_.erase(topicIt);
    }

    void flush() {
        assertUiThread();

        std::set<Topic> dirty;
        std::set<int> dirtyQuotes;
        std::set<int> dirtyBooks;
        std::set<int> dirtyPnl;
        std::set<int> dirtyOrdersFor;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            dirty.swap(dirty_);
            dirtyQuotes.swap(dirtyQuotes_);
            dirtyBooks.swap(dirtyBooks_);
            dirtyPnl.swap(dirtyPnl_);
            dirtyOrdersFor.swap(dirtyOrdersFor_);
            flushPending_ = false;
        }

        std::map<Topic, std::vector<std::function<void()>>> callbacksByTopic;
        for (Topic topic : dirty) {
            auto topicIt = subscribers_.find(topic);
            if (topicIt == subscribers_.end()) continue;
            auto& callbacks = callbacksByTopic[topic];
            callbacks.reserve(topicIt->second.size());
            for (const auto& [id, callback] : topicIt->second)
                callbacks.push_back(callback);
        }

        for (Topic topic : dirty) {
            auto callbacksIt = callbacksByTopic.find(topic);
            if (callbacksIt == callbacksByTopic.end()) continue;
            const auto& callbacks = callbacksIt->second;
            for (const auto& callback : callbacks)
                callback();
        }

        std::map<int, std::vector<std::function<void()>>> quoteCallbacks;
        for (int conId : dirtyQuotes) {
            auto quoteIt = quoteSubscribers_.find(conId);
            if (quoteIt == quoteSubscribers_.end()) continue;
            auto& callbacks = quoteCallbacks[conId];
            callbacks.reserve(quoteIt->second.size());
            for (const auto& [id, callback] : quoteIt->second)
                callbacks.push_back(callback);
        }
        std::vector<std::function<void(int)>> anyQuoteCallbacks;
        anyQuoteCallbacks.reserve(anyQuoteSubscribers_.size());
        for (const auto& [id, callback] : anyQuoteSubscribers_)
            anyQuoteCallbacks.push_back(callback);

        for (int conId : dirtyQuotes) {
            auto callbacksIt = quoteCallbacks.find(conId);
            if (callbacksIt != quoteCallbacks.end()) {
                for (const auto& callback : callbacksIt->second)
                    callback();
            }
            for (const auto& callback : anyQuoteCallbacks)
                callback(conId);
        }

        std::map<int, std::vector<std::function<void()>>> bookCallbacks;
        for (int conId : dirtyBooks) {
            auto booksIt = bookSubscribers_.find(conId);
            if (booksIt == bookSubscribers_.end()) continue;
            auto& callbacks = bookCallbacks[conId];
            callbacks.reserve(booksIt->second.size());
            for (const auto& [id, callback] : booksIt->second)
                callbacks.push_back(callback);
        }
        for (int conId : dirtyBooks) {
            auto callbacksIt = bookCallbacks.find(conId);
            if (callbacksIt == bookCallbacks.end()) continue;
            for (const auto& callback : callbacksIt->second)
                callback();
        }

        std::map<int, std::vector<std::function<void()>>> pnlCallbacks;
        for (int conId : dirtyPnl) {
            auto pnlIt = pnlSubscribers_.find(conId);
            if (pnlIt == pnlSubscribers_.end()) continue;
            auto& callbacks = pnlCallbacks[conId];
            callbacks.reserve(pnlIt->second.size());
            for (const auto& [id, callback] : pnlIt->second)
                callbacks.push_back(callback);
        }
        std::vector<std::function<void(int)>> anyPnlCallbacks;
        anyPnlCallbacks.reserve(anyPnlSubscribers_.size());
        for (const auto& [id, callback] : anyPnlSubscribers_)
            anyPnlCallbacks.push_back(callback);

        for (int conId : dirtyPnl) {
            auto callbacksIt = pnlCallbacks.find(conId);
            if (callbacksIt != pnlCallbacks.end()) {
                for (const auto& callback : callbacksIt->second)
                    callback();
            }
            for (const auto& callback : anyPnlCallbacks)
                callback(conId);
        }

        std::map<int, std::vector<std::function<void()>>> ordersForCallbacks;
        for (int conId : dirtyOrdersFor) {
            auto ordersIt = ordersForSubscribers_.find(conId);
            if (ordersIt == ordersForSubscribers_.end()) continue;
            auto& callbacks = ordersForCallbacks[conId];
            callbacks.reserve(ordersIt->second.size());
            for (const auto& [id, callback] : ordersIt->second)
                callbacks.push_back(callback);
        }
        for (int conId : dirtyOrdersFor) {
            auto callbacksIt = ordersForCallbacks.find(conId);
            if (callbacksIt == ordersForCallbacks.end()) continue;
            for (const auto& callback : callbacksIt->second)
                callback();
        }
    }

    void flushTicks(int conId) {
        assertUiThread();

        std::vector<Tick> ticks;
        {
            std::lock_guard<std::mutex> lock(tickMutex_);
            auto bufferIt = tickBuffers_.find(conId);
            if (bufferIt == tickBuffers_.end()) return;
            ticks = std::move(bufferIt->second.ticks);
            bufferIt->second.ticks.clear();
            bufferIt->second.flushPending = false;
            bufferIt->second.overflowLogged = false;
            if (!tickSubscriberCounts_.contains(conId))
                tickBuffers_.erase(bufferIt);
        }
        if (ticks.empty()) return;

        auto subscribersIt = tickSubscribers_.find(conId);
        if (subscribersIt == tickSubscribers_.end()) return;
        std::vector<std::function<void(std::span<const Tick>)>> callbacks;
        callbacks.reserve(subscribersIt->second.size());
        for (const auto& [id, callback] : subscribersIt->second)
            callbacks.push_back(callback);
        const std::span<const Tick> batch(ticks);
        for (const auto& callback : callbacks)
            callback(batch);
    }

};
