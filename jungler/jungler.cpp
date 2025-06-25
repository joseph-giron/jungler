#include <windows.h>
#include <shellapi.h>
#include <thread>
#include <atomic>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_ENABLE 1001
#define ID_TRAY_DISABLE 1002
#define ID_TRAY_EXIT 1003

std::atomic<bool> g_running(true);
std::atomic<bool> g_enabled(true);
NOTIFYICONDATA nid = { 0 };
HMENU hTrayMenu = NULL;
HINSTANCE hInst;

DWORD GetIdleTimeMS() {
    LASTINPUTINFO lii = { 0 };
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        return GetTickCount64() - lii.dwTime;
    }
    return 0;
}

void MouseJiggleThread() {
    const DWORD IDLE_THRESHOLD = 60 * 1000; // 60 seconds

    while (g_running) {
        if (g_enabled && GetIdleTimeMS() > IDLE_THRESHOLD) {
            INPUT input = { 0 };
            input.type = INPUT_MOUSE;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            input.mi.dx = 1;
            input.mi.dy = 0;
            SendInput(1, &input, sizeof(INPUT));

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            input.mi.dx = -1;
            SendInput(1, &input, sizeof(INPUT));
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void CreateTrayMenu(HWND hWnd) {
    hTrayMenu = CreatePopupMenu();
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_ENABLE, L"Enable");
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_DISABLE, L"Disable");
    AppendMenu(hTrayMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hTrayMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
    HICON mhIcon = (HICON)LoadImage(NULL, L"jungler.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = mhIcon;
    lstrcpy(nid.szTip, L"Joe's Jaggler");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void CleanupTray() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
    if (hTrayMenu) DestroyMenu(hTrayMenu);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON && lParam == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);
        SetForegroundWindow(hWnd);

        // Update menu checkmarks
        CheckMenuItem(hTrayMenu, ID_TRAY_ENABLE, MF_BYCOMMAND | (g_enabled ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem(hTrayMenu, ID_TRAY_DISABLE, MF_BYCOMMAND | (!g_enabled ? MF_CHECKED : MF_UNCHECKED));

        TrackPopupMenu(hTrayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        return 0;
    }

    switch (msg) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_ENABLE:
            g_enabled = true;
            break;
        case ID_TRAY_DISABLE:
            g_enabled = false;
            break;
        case ID_TRAY_EXIT:
            g_running = false;
            PostQuitMessage(0);
            break;
        }
        break;

    case WM_DESTROY:
        CleanupTray();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    hInst = hInstance;

    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MooseJunglerClass";
    RegisterClass(&wc);

    // Create invisible window
    HWND hWnd = CreateWindowEx(0, wc.lpszClassName, L"Moose Jangler", 0,
        0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);

    CreateTrayMenu(hWnd);

    std::thread jiggleThread(MouseJiggleThread);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    jiggleThread.join();
    return 0;
}
