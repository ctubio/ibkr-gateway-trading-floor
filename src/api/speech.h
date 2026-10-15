#pragma once

// ── TTS voice persistence ─────────────────────────────────────────────────────
// Saves/loads the SAPI token ID (registry path string) for the selected TTS voice.
// Default is empty string → caller should fall back to Herena-Catalan search.

void Settings_SaveTtsVoice(const std::string& tokenId) {
    Settings_SaveString("VoiceTokenId", tokenId);
}

std::string Settings_LoadTtsVoice() {
    return Settings_LoadString("VoiceTokenId", "");
}

// ── Shared TTS voice-selection helper ────────────────────────────────────────
// Case-insensitive wstring contains check (shared utility, safe to define here).
static bool TTS_ContainsCI(const WCHAR* s, const WCHAR* needle) {
    if (!s || !needle) return false;
    std::wstring ws(s), wn(needle);
    auto toLo = [](wchar_t c) { return (wchar_t)towlower(c); };
    std::transform(ws.begin(), ws.end(), ws.begin(), toLo);
    std::transform(wn.begin(), wn.end(), wn.begin(), toLo);
    return ws.find(wn) != std::wstring::npos;
}

// Search one voice category for a token matching the given ID string.
// If tokenId is empty, falls back to searching for Herena/Catalan.
// Returns a token the caller must Release(), or nullptr.
static ISpObjectToken* TTS_FindVoiceInCategory(const WCHAR* categoryId, const std::wstring& tokenId) {
    IEnumSpObjectTokens* pEnum = nullptr;
    if (FAILED(SpEnumTokens(categoryId, NULL, NULL, &pEnum))) return nullptr;

    ISpObjectToken* pToken  = nullptr;
    ISpObjectToken* pFound  = nullptr;

    while (!pFound && SUCCEEDED(pEnum->Next(1, &pToken, NULL)) && pToken) {
        WCHAR* pId       = nullptr;
        WCHAR* pDesc     = nullptr;
        WCHAR* pAttrName = nullptr;

        pToken->GetId(&pId);
        SpGetDescription(pToken, &pDesc);
        ISpDataKey* pAttribs = nullptr;
        if (SUCCEEDED(pToken->OpenKey(L"Attributes", &pAttribs))) {
            pAttribs->GetStringValue(L"Name", &pAttrName);
            pAttribs->Release();
        }

        bool match = false;
        if (!tokenId.empty()) {
            // Exact match on token ID (registry path)
            match = (pId && tokenId == std::wstring(pId));
        } else {
            // Default fallback: search for Herena/Catalan
            match = TTS_ContainsCI(pId,       L"herena")  ||
                    TTS_ContainsCI(pDesc,     L"herena")  ||
                    TTS_ContainsCI(pAttrName, L"herena")  ||
                    TTS_ContainsCI(pId,       L"helena")  ||
                    TTS_ContainsCI(pDesc,     L"helena")  ||
                    TTS_ContainsCI(pAttrName, L"helena")  ||
                    TTS_ContainsCI(pId,       L"ca-es")   ||
                    TTS_ContainsCI(pDesc,     L"ca-es")   ||
                    TTS_ContainsCI(pAttrName, L"ca-es")   ||
                    TTS_ContainsCI(pId,       L"catalan") ||
                    TTS_ContainsCI(pDesc,     L"catalan") ||
                    TTS_ContainsCI(pAttrName, L"catalan");
        }

        if (pId)       CoTaskMemFree(pId);
        if (pDesc)     CoTaskMemFree(pDesc);
        if (pAttrName) CoTaskMemFree(pAttrName);

        if (match) pFound = pToken;
        else       { pToken->Release(); pToken = nullptr; }
    }
    pEnum->Release();
    return pFound;
}

// Apply the saved (or default Herena-Catalan) voice to an ISpVoice instance.
// Searches both classic SAPI and OneCore registries.
static void TTS_ApplySavedVoice(ISpVoice* pVoice) {
    if (!pVoice) return;

    std::string savedA = Settings_LoadTtsVoice();
    std::wstring tokenId(savedA.begin(), savedA.end());

    // 1. Classic SAPI voices
    ISpObjectToken* pFound = TTS_FindVoiceInCategory(SPCAT_VOICES, tokenId);

    // 2. OneCore voices (Win10+ neural / browser voices)
    if (!pFound)
        pFound = TTS_FindVoiceInCategory(
            L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices", tokenId);

    if (pFound) {
        pVoice->SetVoice(pFound);
        pFound->Release();
    }
    // else: keep system default
}

// ── Voice enumeration for the Settings UI ────────────────────────────────────
struct TtsVoiceEntry {
    std::wstring tokenId;   // SAPI token registry path (used as key)
    std::wstring display;   // Friendly name shown in the combo
};

// Enumerate ALL voices from both classic SAPI and OneCore registries.
// Duplicates (same tokenId) are suppressed so voices that appear in both
// registries are not listed twice.
static std::vector<TtsVoiceEntry> TTS_EnumerateVoices() {
    std::vector<TtsVoiceEntry> voices;
    auto addFromCategory = [&](const WCHAR* categoryId) {
        IEnumSpObjectTokens* pEnum = nullptr;
        if (FAILED(SpEnumTokens(categoryId, NULL, NULL, &pEnum))) return;

        ISpObjectToken* pToken = nullptr;
        while (SUCCEEDED(pEnum->Next(1, &pToken, NULL)) && pToken) {
            WCHAR* pId       = nullptr;
            WCHAR* pDesc     = nullptr;
            WCHAR* pAttrName = nullptr;

            pToken->GetId(&pId);
            SpGetDescription(pToken, &pDesc);
            ISpDataKey* pAttribs = nullptr;
            if (SUCCEEDED(pToken->OpenKey(L"Attributes", &pAttribs))) {
                pAttribs->GetStringValue(L"Name", &pAttrName);
                pAttribs->Release();
            }

            if (pId) {
                std::wstring tid(pId);
                // Deduplicate by tokenId
                bool dup = false;
                for (const auto& v : voices) if (v.tokenId == tid) { dup = true; break; }
                if (!dup) {
                    std::wstring disp = pAttrName ? std::wstring(pAttrName)
                                      : pDesc    ? std::wstring(pDesc)
                                      :            tid;
                    voices.push_back({ tid, disp });
                }
            }

            if (pId)       CoTaskMemFree(pId);
            if (pDesc)     CoTaskMemFree(pDesc);
            if (pAttrName) CoTaskMemFree(pAttrName);
            pToken->Release();
        }
        pEnum->Release();
    };

    addFromCategory(SPCAT_VOICES);
    addFromCategory(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices");
    return voices;
}