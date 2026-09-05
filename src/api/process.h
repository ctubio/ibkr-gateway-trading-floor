#pragma once

#include <tlhelp32.h>
DWORD PIDProcessRunning(const char* processName) {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return pid;
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, processName) == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return pid;
}

// Returns the full image path of a running process, or empty on failure
// (e.g. no permission to query it — fine here, since anything we can't
// query isn't something IBKR spawned under the install folder anyway).
static std::string GetProcessFullPath(DWORD pid) {
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return "";
    char path[MAX_PATH] = {};
    DWORD size = sizeof(path);
    std::string result;
    if (QueryFullProcessImageNameA(hProc, 0, path, &size))
        result.assign(path, size);
    CloseHandle(hProc);
    return result;
}

// Returns true if any currently running process's image lives under rootDir
// (case-insensitive prefix match). Catches TWS/IB Gateway's self-update
// helper process(es) regardless of what IBKR names them for a given build —
// PIDProcessRunning() alone only matches the fixed names we already know.
static bool IsAnyProcessRunningUnder(const std::string& rootDir) {
    if (rootDir.empty()) return false;

    std::string rootLower = rootDir;
    std::transform(rootLower.begin(), rootLower.end(), rootLower.begin(), ::tolower);
    if (rootLower.back() != '\\') rootLower += '\\';

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    bool found = false;
    if (Process32First(hSnap, &pe)) {
        do {
            std::string imgPath = GetProcessFullPath(pe.th32ProcessID);
            if (imgPath.empty()) continue;
            std::transform(imgPath.begin(), imgPath.end(), imgPath.begin(), ::tolower);
            if (imgPath.rfind(rootLower, 0) == 0) { found = true; break; }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return found;
}

// Structure to pass data to our EnumWindows callback
struct WindowFinderData {
    DWORD targetPID;
    int swState;
    std::vector<HWND> foundWindows; // Now using a vector to store multiple handles
};

// Callback function to evaluate each window
// Callback function to evaluate each window
BOOL CALLBACK EnumAnyWindowsCallback(HWND hwnd, LPARAM lParam) {
    WindowFinderData* data = reinterpret_cast<WindowFinderData*>(lParam);
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    if (data->targetPID == processId) {
        // Get the length of the window's title
        int textLength = GetWindowTextLengthW(hwnd);
        
        if (textLength > 0) {
            // Allocate a buffer to hold the title text
            std::vector<wchar_t> titleBuffer(textLength + 1);
            GetWindowTextW(hwnd, titleBuffer.data(), textLength + 1);
            
            // Convert to a C++ wstring for easy searching
            std::wstring windowTitle(titleBuffer.data());
            
            // Check if the title contains '@' OR 'U423'
            //bool containsAtSymbol = (windowTitle.find(L"@") != std::wstring::npos);
            //bool containsU423 = (windowTitle.find(L"Interactive Brokers") != std::wstring::npos);
            //bool containsGateway = (windowTitle.find(L"IBKR Gateway") != std::wstring::npos);
            bool containsToolkit       = (windowTitle.find(L"ToolkitWindow") != std::wstring::npos);
            bool containsPortfolio     = (windowTitle.find(L"Portfolio") != std::wstring::npos);
            bool containsLoading       = (windowTitle.find(L"Loading") != std::wstring::npos);
            bool containsLoginMessages = (windowTitle.find(L"Login Messages") != std::wstring::npos);

            // Only add the window if it matches your specific criteria
            //if (containsAtSymbol || containsU423 || containsGateway) {
            if (data->swState == SW_HIDE || (!containsToolkit && !containsLoading && !containsPortfolio && !containsLoginMessages)) {
                data->foundWindows.push_back(hwnd);
            }
        }
    }
    
    return TRUE; // Continue enumerating
}

std::string GetGatewayPath() {
    HKEY hKey;
    std::string fullPath = std::format("{}\\{}", APP_REG_ROOT, SETTINGS_CLASS_NAME);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char path[MAX_PATH] = {};
        DWORD size = sizeof(path);
        if (RegQueryValueExA(hKey, "Gateway_Path", NULL, NULL, (LPBYTE)path, &size) == ERROR_SUCCESS && strlen(path) > 0) {
            RegCloseKey(hKey);
            return std::string(path);
        }
        RegCloseKey(hKey);
    }
    return "";
}

void SaveGatewayPath(const std::string& path) {
    HKEY hKey;
    std::string fullPath = std::format("{}\\{}", APP_REG_ROOT, SETTINGS_CLASS_NAME);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "Gateway_Path", 0, REG_SZ, (const BYTE*)path.c_str(), (DWORD)path.size() + 1);
        RegCloseKey(hKey);
    }
}

std::string AskGatewayPath(HWND hWnd) {
    OPENFILENAMEA ofn = {};
    char path[MAX_PATH] = "";
    char folder[MAX_PATH] = "C:\\";
    
    std::string gatewayPath = GetGatewayPath();
    if (!gatewayPath.empty()) {
        auto systemPath = std::filesystem::path(gatewayPath);
        std::string filename = systemPath.filename().string();
        std::string pathname = systemPath.remove_filename().string();
        if (!filename.empty()) {
            strncpy(path, filename.c_str(), sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        }
        if (!pathname.empty()) {
            strncpy(folder, pathname.c_str(), sizeof(folder) - 1);
            folder[sizeof(folder) - 1] = '\0';
        }
    }
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = hWnd;
    ofn.lpstrFilter     = "Executable\0*.exe\0All Files\0*.*\0";
    ofn.lpstrFile       = path;
    ofn.nMaxFile        = sizeof(path);
    ofn.lpstrTitle      = "Locate ibgateway.exe or tws.exe";
    ofn.lpstrInitialDir = folder;
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return std::string(path);
    return "";
}

struct EnsureOnceFlag {
    bool& flag;
    EnsureOnceFlag(bool& f) : flag(f) { flag = true; }
    ~EnsureOnceFlag() { flag = false; }
};

bool alreadyEnsureGatewayRunning = false;

bool ensureGatewayLoggedInOnce = false;

bool EnsureGatewayRunning(HWND hWnd) {
    if (alreadyEnsureGatewayRunning || !Settings_AutoGateway()) return false;

    std::string path   = GetGatewayPath();
    std::string installRoot = path.empty() ? "" : std::filesystem::path(path).parent_path().string();

    if (PIDProcessRunning("ibgateway.exe") > 0 || PIDProcessRunning("tws.exe") > 0 || IsAnyProcessRunningUnder(installRoot))
        return true;

    EnsureOnceFlag guard(alreadyEnsureGatewayRunning);
    if (path.empty() || GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(hWnd, "TWS or IB Gateway not found.\nPlease locate tws.exe or ibgateway.exe.", "TWS or IB Gateway Not Found", MB_OK | MB_ICONINFORMATION);
        path = AskGatewayPath(hWnd);
        if (path.empty()) return false;
        SaveGatewayPath(path);
    }
    LogDebug("Running " + std::filesystem::path(path).filename().string() + ", please login..");
    ensureGatewayLoggedInOnce = false;
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
    return true;
}

// Helper function to send a string as keyboard input
void SendString(const std::string& text) {
    std::vector<INPUT> inputs;
    for (char c : text) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = c;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.push_back(input);
        
        input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.push_back(input);
    }
    SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
}

// Helper function to simulate a specific virtual key (like VK_TAB or VK_RETURN)
void SendKey(WORD vKey) {
    INPUT inputs[2] = {0};
    
    // Key down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vKey;
    
    // Key up
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vKey;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    
    SendInput(2, inputs, sizeof(INPUT));
}

void EnsureGatewayLoggedIn(HWND hWnd) {
    if (ensureGatewayLoggedInOnce || !Settings_AutoGateway()) return;

    std::string path = GetGatewayPath();
    if (path.empty()) return;

    std::string filename = std::filesystem::path(path).filename().string();
    if (filename != "ibgateway.exe" && filename != "tws.exe") return;

    std::string username, password;
    if (!Credentials_Load(username, password) || username.empty() || password.empty()) {
        LogDebug("EnsureGatewayLoggedIn: no saved credentials, skipping auto-login.");
        return;
    }

    DWORD pid = PIDProcessRunning(filename.c_str());
    if (pid == 0) return;

    WindowFinderData data;
    data.targetPID = pid;
    data.swState = SW_SHOW;
    
    EnumWindows(EnumAnyWindowsCallback, reinterpret_cast<LPARAM>(&data));

    for (HWND hwnd : data.foundWindows) {
        char title[256] = {};
        GetWindowTextA(hwnd, title, sizeof(title));
        std::string titleStr(title);
        if (titleStr.find("Login") != std::string::npos) {
            ensureGatewayLoggedInOnce = true;
            LogDebug("Found login window: " + titleStr);
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
            Sleep(100);
            SendString(username);
            Sleep(50);
            SendKey(VK_TAB);
            Sleep(50);
            SendString(password);
            Sleep(50);
            SendKey(VK_RETURN);
            break;
        }
    }
}

struct EnumData {
    DWORD dwProcessId;
    HWND hMainWindow;
};

BOOL CALLBACK EnumTWSMainWindowProc(HWND hWnd, LPARAM lParam) {
    EnumData& data = *reinterpret_cast<EnumData*>(lParam);
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    if (dwProcessId == data.dwProcessId) {
        if (!IsWindowVisible(hWnd)) {
            return TRUE;
        }

        if (GetWindow(hWnd, GW_OWNER) != NULL) {
            return TRUE; 
        }

        char windowTitle[256] = {0};
        GetWindowTextA(hWnd, windowTitle, sizeof(windowTitle));
        std::string title(windowTitle);

        if (title.empty()) {
            return TRUE;
        }

        // Check if the title matches the core application frame
        // TWS main windows typically include "Interactive Brokers"
        if (title.find("Interactive Brokers") != std::string::npos) {
            data.hMainWindow = hWnd;
            return FALSE; // Stop enumeration: main window found
        }
    }
    return TRUE;
}

void KillGateway() {
    std::string path = GetGatewayPath();
    if (path.empty()) return;

    std::string targetExe = std::filesystem::path(path).filename().string();

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, targetExe.c_str()) == 0) {
                EnumData data = { pe.th32ProcessID, NULL };
                
                EnumWindows(EnumTWSMainWindowProc, reinterpret_cast<LPARAM>(&data));

                if (data.hMainWindow) {
                    // How to verify: Check if the window handle is valid
                    // Send WM_CLOSE specifically to the main container frame
                    PostMessage(data.hMainWindow, WM_CLOSE, 0, 0);
                }
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
}

static void ToggleTWS(int swState) {
    DWORD pid = PIDProcessRunning(std::filesystem::path(GetGatewayPath()).filename().string().c_str());
    if (pid == 0) return;

    WindowFinderData data;
    data.targetPID = pid;
    data.swState = swState;
    
    EnumWindows(EnumAnyWindowsCallback, reinterpret_cast<LPARAM>(&data));

    if (!data.foundWindows.empty()) {
        for (HWND hwnd : data.foundWindows) {
            ShowWindow(hwnd, data.swState);
            if (data.swState == SW_SHOW) {
                ShowWindow(hwnd, SW_MINIMIZE);
                ShowWindow(hwnd, SW_RESTORE);
            }
        }
    }
}

LONG WINAPI WindowsCrashHandler(EXCEPTION_POINTERS* exceptionInfo) {
    DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;
    std::string errorType = "UNKNOWN CRITICAL EXCEPTION";
    
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            errorType = "EXCEPTION_ACCESS_VIOLATION (Null pointer dereference or invalid memory access)";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            errorType = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED (Out of bounds array access)";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            errorType = "EXCEPTION_INT_DIVIDE_BY_ZERO (Division by zero)";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            errorType = "EXCEPTION_STACK_OVERFLOW (Infinite recursion or exhausted stack space)";
            break;
        case 0xE06D7363: // Magical Microsoft C++ Exception Code
            errorType = "Unhandled C++ Exception (thrown via throw keyword, missed by try/catch)";
            break;
    }

    std::stringstream ss;
    ss << "!!! CRITICAL APPLICATION CRASH !!!\n\n"
       << "Exception Code: 0x" << std::hex << code << "\n"
       << "Description: " << errorType << "\n"
       << "Fault Address: 0x" << exceptionInfo->ExceptionRecord->ExceptionAddress << "\n\n";

    // Alert the developer instantly via Windows Pop-up
    MessageBoxA(NULL, ss.str().c_str(), "Gateway Gateway Fatal Error", MB_ICONERROR | MB_OK);

    // Tell Windows to close the application cleanly now that we handled it
    return EXCEPTION_EXECUTE_HANDLER; 
}