#pragma once

static HWND hDebugEdit = NULL;
static std::vector<std::string> debugBuffer; // stores messages when window is closed

void LogDebug(const std::string& msg) {
    time_t now = time(0);
    char tstr[26] = {};
    ctime_s(tstr, sizeof(tstr), &now);
    std::string timestamp = tstr;
    if (!timestamp.empty()) timestamp.pop_back();

    std::string fullMsg = "[" + timestamp + "] " + msg + "\r\n";
    debugBuffer.push_back(fullMsg);

    if (hDebugEdit && IsWindow(hDebugEdit)) {
        // Append to edit control
        int len = GetWindowTextLength(hDebugEdit);
        SendMessage(hDebugEdit, EM_SETSEL, len, len);
        SendMessageA(hDebugEdit, EM_REPLACESEL, FALSE, (LPARAM)fullMsg.c_str());
        // Auto-scroll to bottom
        SendMessage(hDebugEdit, EM_SCROLLCARET, 0, 0);
    }
}