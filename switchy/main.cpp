#include <Windows.h>

using namespace std;

HHOOK hHook = 0;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

		if (p->vkCode == VK_CAPITAL) {
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
				if (GetKeyState(VK_CAPITAL)) {
					UnhookWindowsHookEx(hHook);
					keybd_event(VK_CAPITAL, 0x3a, 0, 0);
					keybd_event(VK_CAPITAL, 0x3a, KEYEVENTF_KEYUP, 0);
					hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
				}
				
				HWND hWnd = GetForegroundWindow();
				if (hWnd) {
					hWnd = GetAncestor(hWnd, GA_ROOTOWNER);
					PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)HKL_NEXT);
				}
			}

			return 1;
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void ShowError(LPCSTR s) {
	MessageBox(NULL, s, "Error", MB_OK | MB_ICONERROR);
}

int main () {
	HANDLE hMutex = CreateMutex(0, 0, "Switchy");

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		ShowError("Another instance is already running!");
		return 1;
	}
	

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);

	if (hHook == NULL) {
		ShowError("Error with SetWindowsHookEx()");
		return 1;
	}

	MSG messages;

	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	UnhookWindowsHookEx(hHook);
	return 0;
}