#pragma once

#include <windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>

class UiDispatcher {
public:
    static UiDispatcher& instance() {
        static UiDispatcher dispatcher;
        return dispatcher;
    }

    bool init(HINSTANCE hInstance) {
        {
            std::lock_guard<std::mutex> lock(m_);
            if (hwnd_) return true;
        }

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = hInstance;
        windowClass.lpszClassName = windowClassName_;

        if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        HWND hwnd = CreateWindowExW(
            0, windowClassName_, L"", 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, this);
        if (!hwnd) {
            UnregisterClassW(windowClassName_, hInstance);
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(m_);
            hInstance_ = hInstance;
            hwnd_ = hwnd;
            acceptingPosts_ = true;
        }
        return true;
    }

    void shutdown() {
        HWND hwnd;
        HINSTANCE hInstance;
        {
            std::lock_guard<std::mutex> lock(m_);
            acceptingPosts_ = false;
            hwnd = hwnd_;
            hwnd_ = nullptr;
            hInstance = hInstance_;
            hInstance_ = nullptr;
            q_.clear();
        }

        {
            std::unique_lock<std::mutex> lock(m_);
            inFlightPosts_.wait(lock, [this] {
                return postsInFlight_.load(std::memory_order_acquire) == 0;
            });
        }

        wakePending_.store(false, std::memory_order_release);
        if (hwnd) DestroyWindow(hwnd);
        if (hInstance) UnregisterClassW(windowClassName_, hInstance);
    }

    void post(std::function<void()> fn) {
        HWND hwnd;
        {
            std::lock_guard<std::mutex> lock(m_);
            if (!acceptingPosts_ || !hwnd_) return;
            q_.push_back(std::move(fn));
            postsInFlight_.fetch_add(1, std::memory_order_acq_rel);
            hwnd = hwnd_;
        }

        if (!wakePending_.exchange(true, std::memory_order_acq_rel)) {
            if (!PostMessageW(hwnd, WM_UI_DISPATCH, 0, 0))
                wakePending_.store(false, std::memory_order_release);
        }

        if (postsInFlight_.fetch_sub(1, std::memory_order_acq_rel) == 1)
            inFlightPosts_.notify_all();
    }

private:
    std::mutex m_;
    std::deque<std::function<void()>> q_;
    std::atomic<bool> wakePending_{false};
    HWND hwnd_ = nullptr;
    HINSTANCE hInstance_ = nullptr;
    bool acceptingPosts_ = false;
    std::atomic<std::size_t> postsInFlight_{0};
    std::condition_variable inFlightPosts_;
    static constexpr UINT WM_UI_DISPATCH = WM_APP + 1;
    static constexpr wchar_t windowClassName_[] = L"TradingFloor.UiDispatcher";

    UiDispatcher() = default;

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NCCREATE) {
            auto* createInfo = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(createInfo->lpCreateParams));
            return TRUE;
        }

        auto* dispatcher = reinterpret_cast<UiDispatcher*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (dispatcher && message == WM_UI_DISPATCH) {
            dispatcher->drain();
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    void drain() {
        wakePending_.store(false, std::memory_order_release);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(4);
        constexpr std::size_t maxItems = 64;
        std::size_t itemsRun = 0;

        while (itemsRun < maxItems && std::chrono::steady_clock::now() < deadline) {
            std::function<void()> fn;
            {
                std::lock_guard<std::mutex> lock(m_);
                if (q_.empty()) return;
                fn = std::move(q_.front());
                q_.pop_front();
            }

            try {
                fn();
            } catch (...) {
            }
            ++itemsRun;
        }

        bool hasItems;
        {
            std::lock_guard<std::mutex> lock(m_);
            hasItems = !q_.empty();
        }
        if (hasItems && !wakePending_.exchange(true, std::memory_order_acq_rel)) {
            if (!PostMessageW(hwnd_, WM_UI_DISPATCH, 0, 0))
                wakePending_.store(false, std::memory_order_release);
        }
    }
};
