#pragma once

#include <tlhelp32.h>
bool IsProcessRunning(const char* processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    bool found = false;
    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, processName) == 0) {
                found = true; break;
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
    return found;
}

std::string GetGatewayPath() {
    HKEY hKey;
    std::string fullPath = std::format("{}\\Settings", APP_REG_ROOT);
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
    std::string fullPath = std::format("{}\\Settings", APP_REG_ROOT);
    if (RegCreateKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "Gateway_Path", 0, REG_SZ, (const BYTE*)path.c_str(), (DWORD)path.size() + 1);
        RegCloseKey(hKey);
    }
}

std::string AskGatewayPath(HWND hParent) {
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
    ofn.hwndOwner       = hParent;
    ofn.lpstrFilter     = "Executable\0*.exe\0All Files\0*.*\0";
    ofn.lpstrFile       = path;
    ofn.nMaxFile        = sizeof(path);
    ofn.lpstrTitle      = "Locate ibgateway.exe or tws.exe";
    ofn.lpstrInitialDir = folder;
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return std::string(path);
    return "";
}

bool alreadyEnsureGatewayRunning = false;
void EnsureGatewayRunning(HWND hParent) {
    if (alreadyEnsureGatewayRunning || !Settings_AutoGateway() || IsProcessRunning("ibgateway.exe") || IsProcessRunning("tws.exe")) return;
    alreadyEnsureGatewayRunning = true;
    std::string path = GetGatewayPath();
    if (path.empty() || GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(hParent, "TWS or IB Gateway not found.\nPlease locate tws.exe or ibgateway.exe.", "TWS or IB Gateway Not Found", MB_OK | MB_ICONINFORMATION);
        path = AskGatewayPath(hParent);
        if (path.empty()) return; 
        SaveGatewayPath(path);
    }
    alreadyEnsureGatewayRunning = false;
    LogDebug("Running " + std::filesystem::path(path).filename().string() + ", please login..");
    ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOW);
}

void KillGateway() {
    std::string path = GetGatewayPath();
    if (path.empty()) return;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32 pe = { sizeof(PROCESSENTRY32) };
    if (Process32First(hSnap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, std::filesystem::path(path).filename().string().c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hSnap, &pe));
    }
    CloseHandle(hSnap);
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

void MutexGatewayInstance() {
    HANDLE hMutex = CreateMutex(NULL, TRUE, "Global\\TWSAPIClientTradingFloorMutex_17072025");

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existingWnd = FindWindow(DASHBOARD_CLASS_NAME, NULL);
        if (existingWnd) {
            DWORD processId;
            GetWindowThreadProcessId(existingWnd, &processId);
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
            if (hProcess) {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
        
        if (hMutex) CloseHandle(hMutex);
        // Re-create to own the mutex for this instance
        CreateMutex(NULL, TRUE, "Global\\TWSAPIClientTradingFloorMutex_17072025");
    }
}
