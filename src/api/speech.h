#pragma once

// ── TTS voice persistence ─────────────────────────────────────────────────────
// Saves/loads the SAPI token ID (registry path string) for the selected TTS voice.
// Default is empty string → caller should fall back to Herena-Catalan search.

void Settings_SaveTtsVoice(const std::string& tokenId) {
    RegSetString(SETTINGS_CLASS_NAME, "VoiceTokenId", tokenId);
}

std::string Settings_LoadTtsVoice() {
    return RegGetString(SETTINGS_CLASS_NAME, "VoiceTokenId", "");
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

static bool TTS_IsHerenaCatalan(const WCHAR* id, const WCHAR* description, const WCHAR* attributeName) {
    const WCHAR* fields[] = { id, description, attributeName };
    const WCHAR* needles[] = { L"herena", L"helena", L"ca-es", L"catalan" };
    for (const WCHAR* needle : needles)
        for (const WCHAR* field : fields)
            if (TTS_ContainsCI(field, needle)) return true;
    return false;
}

struct TtsTokenInfo {
    std::wstring id;
    std::wstring description;
    std::wstring attributeName;
    bool hasId = false;
    bool hasDescription = false;
    bool hasAttributeName = false;
};

static TtsTokenInfo TTS_ReadTokenInfo(ISpObjectToken* pToken) {
    WCHAR* pId = nullptr;
    WCHAR* pDesc = nullptr;
    WCHAR* pAttrName = nullptr;

    if (pToken) {
        pToken->GetId(&pId);
        SpGetDescription(pToken, &pDesc);
        ISpDataKey* pAttribs = nullptr;
        if (SUCCEEDED(pToken->OpenKey(L"Attributes", &pAttribs))) {
            pAttribs->GetStringValue(L"Name", &pAttrName);
            pAttribs->Release();
        }
    }

    TtsTokenInfo info;
    if (pId) info.id = pId;
    if (pDesc) info.description = pDesc;
    if (pAttrName) info.attributeName = pAttrName;
    info.hasId = pId != nullptr;
    info.hasDescription = pDesc != nullptr;
    info.hasAttributeName = pAttrName != nullptr;

    if (pId) CoTaskMemFree(pId);
    if (pDesc) CoTaskMemFree(pDesc);
    if (pAttrName) CoTaskMemFree(pAttrName);
    return info;
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
        TtsTokenInfo info = TTS_ReadTokenInfo(pToken);

        bool match = false;
        if (!tokenId.empty()) {
            // Exact match on token ID (registry path)
            match = info.hasId && tokenId == info.id;
        } else {
            // Default fallback: search for Herena/Catalan
            match = TTS_IsHerenaCatalan(
                info.id.c_str(), info.description.c_str(), info.attributeName.c_str());
        }

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
    std::wstring tokenId = StringToWide(savedA);

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
        ULONG fetched = 0;
        while (pEnum->Next(1, &pToken, &fetched) == S_OK && fetched == 1 && pToken) {
            TtsTokenInfo info = TTS_ReadTokenInfo(pToken);

            if (info.hasId) {
                // Deduplicate by tokenId
                bool dup = false;
                for (const auto& v : voices) if (v.tokenId == info.id) { dup = true; break; }
                if (!dup) {
                    const std::wstring& display = info.hasAttributeName ? info.attributeName
                                                 : info.hasDescription   ? info.description
                                                 :                         info.id;
                    voices.push_back({ info.id, display });
                }
            }

            pToken->Release();
            pToken = nullptr;
        }
        pEnum->Release();
    };

    addFromCategory(SPCAT_VOICES);
    addFromCategory(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech_OneCore\\Voices");
    return voices;
}