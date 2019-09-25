#include <iostream>
#include <Windows.h>

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

HHOOK hHook = 0;
bool showPopup = false;
bool wasPressed = false;

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

				if (!wasPressed) {
					wasPressed = true;

					if (showPopup) {
						keybd_event(VK_LWIN, 0x3a, 0, 0);
						keybd_event(VK_SPACE, 0x3a, 0, 0);
					} else {
						HWND hWnd = GetForegroundWindow();
						if (hWnd) {
							hWnd = GetAncestor(hWnd, GA_ROOTOWNER);
							PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)HKL_NEXT);
						}
					}
				}
			}

			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
				wasPressed = false;

				if (showPopup) {
					keybd_event(VK_LWIN, 0x3a, KEYEVENTF_KEYUP, 0);
					keybd_event(VK_SPACE, 0x3a, KEYEVENTF_KEYUP, 0);
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

DWORD GetOSVersion() {
	HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
	RTL_OSVERSIONINFOW osvi = { 0 };

	if (hMod) {
		RtlGetVersionPtr p = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");

		if (p) {
			osvi.dwOSVersionInfoSize = sizeof(osvi);
			p(&osvi);
		}
	}

	return osvi.dwMajorVersion;
}

int main(int argc, char* argv[]) {
	showPopup = GetOSVersion() >= 10;
	if (argc > 1) {
		if (_stricmp("nopopup", argv[1]) == 0)
			showPopup = false;
	}

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