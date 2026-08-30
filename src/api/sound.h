#pragma once

// ── WAV Player ─────────────────────────────────────────────────────────────────

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

        PlaySoundA(reinterpret_cast<LPCSTR>(pData), NULL, SND_MEMORY | SND_SYNC | SND_NODEFAULT);
    }
};

void PlaySound_Async(int resourceId) {
    if (!Settings_Load("PlaySounds", 0)) return;

    static SoundQueue soundQueue;
    soundQueue.enqueue(resourceId);
}

// ── Shared TTS Engine ─────────────────────────────────────────────────────

class SharedTtsEngine {
    ISpVoice*  voice_    = nullptr;
    bool       comInit_  = false;
    int        refCount_ = 0;
    std::mutex mutex_;

public:
    // Acquires a reference, lazily creating the engine on the first caller.
    // Returns false if SAPI couldn't be initialized — caller should treat
    // its own "TTS on" toggle as failed and not hold a reference.
    bool Acquire() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (voice_) { ++refCount_; return true; }

        if (!comInit_) {
            HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            comInit_ = SUCCEEDED(hr) || (hr == RPC_E_CHANGED_MODE);
        }
        HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&voice_);
        if (FAILED(hr)) { voice_ = nullptr; return false; }

        TTS_ApplySavedVoice(voice_);
        refCount_ = 1;
        return true;
    }

    // Releases a reference. Once the last window still speaking releases
    // its reference, the engine stops any speech in progress and tears
    // itself down, so an idle app isn't holding a live SAPI engine.
    void Release() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (!voice_ || refCount_ <= 0) return;
        if (--refCount_ > 0) return;
        voice_->Speak(NULL, SVSFPurgeBeforeSpeak, NULL);
        voice_->Release();
        voice_ = nullptr;
    }

    // Speaks `text` async, purging whatever the shared voice was doing
    // before (see class comment for the multi-window implication). No-op
    // if nobody currently holds a reference.
    void Speak(const std::wstring& text) {
        std::lock_guard<std::mutex> lk(mutex_);
        if (voice_) voice_->Speak(text.c_str(), SVSFlagsAsync | SVSFPurgeBeforeSpeak, NULL);
    }

    // Re-applies the (just-changed) saved voice token in place. No-op if
    // nobody currently holds a reference — the next Acquire() will pick up
    // the new token anyway via TTS_ApplySavedVoice.
    void ReapplySavedVoice() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (voice_) TTS_ApplySavedVoice(voice_);
    }
};

SharedTtsEngine& SharedTts() {
    static SharedTtsEngine engine;
    return engine;
}