#include <Windows.h>

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

HHOOK hHook;
bool popup;
bool modifier;
bool enabled = true, enabledSwitched = false;
bool wasPressed = false, winPressed = false;
BYTE winKeyCode = 0x00;
bool modifierPressed = false;

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

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

		if (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN) {
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) { winPressed = true; winKeyCode = static_cast<BYTE>(p->vkCode); return 1; }
			else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
				if (enabledSwitched) { winPressed = enabledSwitched = false; return 1; }
				else if (winPressed) {
					if (winKeyCode) {
						UnhookWindowsHookEx(hHook);
						keybd_event(winKeyCode, 0x3a, 0, 0);
						keybd_event(winKeyCode, 0x3a, KEYEVENTF_KEYUP, 0);
						hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
						winKeyCode = 0x00;
					}
					winPressed = false;
				}
			}
		}

		if (p->vkCode == VK_CAPITAL) {
			if (winPressed) {
				if (!wasPressed) { if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) enabled = !enabled, enabledSwitched = wasPressed = true; }
				else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) wasPressed = false;

				return 1;
			}

			if (enabled) {
				if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
					if (GetKeyState(VK_LSHIFT) & 0x8000) {
						UnhookWindowsHookEx(hHook);
						keybd_event(VK_CAPITAL, 0x3a, 0, 0);
						keybd_event(VK_CAPITAL, 0x3a, KEYEVENTF_KEYUP, 0);
						hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
						return 1;
					}

					if (!wasPressed) {
						wasPressed = true;

						if (popup) {
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

					if (popup) {
						keybd_event(VK_LWIN, 0x3a, KEYEVENTF_KEYUP, 0);
						keybd_event(VK_SPACE, 0x3a, KEYEVENTF_KEYUP, 0);
					}
				}
				return 1;
			}
		}
		else if (winPressed) {
			if (winKeyCode) {
				UnhookWindowsHookEx(hHook);
				keybd_event(winKeyCode, 0x3a, 0, 0);
				hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
				winKeyCode = 0x00;
			}
			winPressed = false;
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

int main(int argc, char* argv[]) {
	popup = GetOSVersion() >= 10;
	if (argc > 1) {
		if (argv[1] == "nopopup") popup = false;
	}

	HANDLE hMutex = CreateMutex(0, 0, "Switchy");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		ShowError("Another instance of Switchy is already running!");
		return 1;
	}

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
	if (hHook == nullptr) {
		ShowError("Error with \"SetWindowsHookEx\"");
		return 1;
	}

	MSG messages;
	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 0;
}
