#pragma once

#include <atomic>
#include <string>
#include <queue>
#include <condition_variable>

void PlaySound_Async(int resourceId) {
    if (!Settings_Load("PlaySounds", 0)) return;

    // We use a static worker thread and a queue to ensure sounds play sequentially
    struct SoundQueue {
        std::queue<int> queue;
        std::mutex mutex;
        std::condition_variable cv;
        std::atomic<bool> running{true};
        std::thread worker;

        SoundQueue() {
            worker = std::thread([this]() {
                while (running) {
                    int resId = 0;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(lock, [this] { return !queue.empty() || !running; });
                        if (!running) break;
                        resId = queue.front();
                        queue.pop();
                    }

                    // Actual playback logic
                    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resId), RT_RCDATA);
                    if (hRes) {
                        HGLOBAL hMem = LoadResource(NULL, hRes);
                        if (hMem) {
                            void* pData = LockResource(hMem);
                            DWORD size = SizeofResource(NULL, hRes);
                            if (pData && size) {
                                static std::atomic<int> soundSequence(0);
                                int currentSeq = ++soundSequence;

                                char tempPath[MAX_PATH];
                                GetTempPathA(MAX_PATH, tempPath);
                                std::string mp3File = std::string(tempPath) + "ib_snd_" + std::to_string(currentSeq) + ".mp3";
                                std::string alias = "mci_snd_" + std::to_string(currentSeq);

                                FILE* f = fopen(mp3File.c_str(), "wb");
                                if (f) {
                                    fwrite(pData, 1, size, f);
                                    fclose(f);

                                    std::string openCmd = "open \"" + mp3File + "\" type mpegvideo alias " + alias;
                                    std::string playCmd = "play " + alias + " from 0";
                                    mciSendStringA(openCmd.c_str(), NULL, 0, NULL);
                                    mciSendStringA(playCmd.c_str(), NULL, 0, NULL);

                                    // Wait for sound to finish (approximate) or use a fixed delay
                                    // Since we are in a dedicated worker thread, we can afford to sleep
                                    std::this_thread::sleep_for(std::chrono::seconds(3));
                                    
                                    std::string closeCmd = "close " + alias;
                                    mciSendStringA(closeCmd.c_str(), NULL, 0, NULL);
                                    DeleteFileA(mp3File.c_str());
                                }
                            }
                        }
                    }
                }
            });
        }
        ~SoundQueue() {
            running = false;
            cv.notify_all();
            if (worker.joinable()) worker.join();
        }
    };

    static SoundQueue sq;
    {
        std::lock_guard<std::mutex> lock(sq.mutex);
        sq.queue.push(resourceId);
    }
    sq.cv.notify_one();
}