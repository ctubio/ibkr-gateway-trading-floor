#pragma once

void PlaySound_Async(int resourceId) {
    if (!Settings_Load("PlaySounds", 0)) return;

    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return;

    HGLOBAL hMem = LoadResource(NULL, hRes);
    if (!hMem) return;

    DWORD size = SizeofResource(NULL, hRes);
    void* pData = LockResource(hMem);
    if (!pData || size == 0) return;

    LogDebug("Playing sound resource ID: " + std::to_string(resourceId));

    DWORD fdwSound = SND_ASYNC;
    if (resourceId == 201 || resourceId == 202 || resourceId == 203 || resourceId == 204)
        fdwSound = SND_SYNC;
    PlaySoundA(reinterpret_cast<LPCSTR>(pData), NULL, SND_MEMORY | fdwSound | SND_NODEFAULT);
}
