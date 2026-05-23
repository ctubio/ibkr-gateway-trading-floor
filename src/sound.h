#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <condition_variable>

DWORD Settings_Load(const char* key, DWORD defaultValue);

struct SoundQueue {
    std::mutex mutex;
    std::atomic<bool> running{true};
    std::thread worker;
    std::queue<int> queue;
    std::condition_variable cv;

    SoundQueue() {
        worker = std::thread([this]() { run(); });
    }

    ~SoundQueue() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            running = false;
        }
        cv.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }

    void enqueue(int resourceId) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            queue.push(resourceId);
        }
        cv.notify_one();
    }

private:
    void run() {
        std::unique_lock<std::mutex> lock(mutex);
        while (running) {
            cv.wait(lock, [this] { return !queue.empty() || !running; });
            if (!running && queue.empty())
                break;

            int resourceId = queue.front();
            queue.pop();
            lock.unlock();

            playSoundSync(resourceId);

            lock.lock();
        }
    }

    static void playSoundSync(int resourceId) {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hRes) return;

        HGLOBAL hMem = LoadResource(NULL, hRes);
        if (!hMem) return;

        DWORD size = SizeofResource(NULL, hRes);
        void* pData = LockResource(hMem);
        if (!pData || size == 0) return;

        PlaySoundA(reinterpret_cast<LPCSTR>(pData), NULL,
                   SND_MEMORY | SND_SYNC | SND_NODEFAULT);
    }
};

void PlaySound_Async(int resourceId) {
    if (!Settings_Load("PlaySounds", 0)) return;

    static SoundQueue soundQueue;
    soundQueue.enqueue(resourceId);
}
