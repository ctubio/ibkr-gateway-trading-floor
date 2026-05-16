#include "api.h"
#include "shared.h"
#include "book.h"
#include "dashboard.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    HANDLE hMutex = mutex_on();
	if (!hMutex) return 0;
    
    registerBook(hInst);
	registerDashboard(hInst);
    registerSystemIcon(hInst);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	
    mutex_off(hMutex);
    
    return (int)msg.wParam;
}
