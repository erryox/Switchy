#include <Windows.h>
#if _DEBUG
#include <stdio.h>
#endif // _DEBUG

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

typedef struct {
	BOOL popup;
	BYTE modifier;
} Settings;

void ShowError(LPCSTR message);
DWORD GetOSVersion();
void ToggleCapsLockState();
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);


HHOOK hHook;
BOOL modifier;
BOOL enabled = TRUE;
BOOL capsLockPressed = FALSE;
BOOL winPressed = FALSE;

Settings settings = {
	.popup = FALSE,
	.modifier = VK_LSHIFT
};


int main(int argc, char** argv) {
	if (argc > 1 && strcmp(argv[1], "nopopup") == 0) {
		settings.popup = FALSE;
	} else {
		settings.popup = GetOSVersion() >= 10;
	}
#if _DEBUG
	printf("Pop-up %s\n", settings.popup ? "enabled" : "disabled");
#endif

	HANDLE hMutex = CreateMutex(0, 0, "Switchy");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		ShowError("Another instance of Switchy is already running!");
		return 1;
	}

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
	if (hHook == NULL) {
		ShowError("Error calling \"SetWindowsHookEx(...)\"");
		return 1;
	}

	MSG messages;
	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 0;
}


void ShowError(LPCSTR message) {
	MessageBox(NULL, message, "Error", MB_OK | MB_ICONERROR);
}


DWORD GetOSVersion() {
	HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
	RTL_OSVERSIONINFOW osvi = { 0 };

	if (hMod) {
		RtlGetVersionPtr p = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");

		if (p) {
			osvi.dwOSVersionInfoSize = sizeof(osvi);
			p(&osvi);
		}
	}

	return osvi.dwMajorVersion;
}


void ToggleCapsLockState() {
	UnhookWindowsHookEx(hHook);
	keybd_event(VK_CAPITAL, 0x3A, 0, 0);
	keybd_event(VK_CAPITAL, 0x3A, KEYEVENTF_KEYUP, 0);
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
#if _DEBUG
	printf("Caps Lock state has been toggled\n");
#endif // _DEBUG
}


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)lParam;
	if (nCode == HC_ACTION && !(key->flags & LLKHF_INJECTED)) {
#if _DEBUG
		const char* keyStatus = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? "pressed" : "released";
		printf("Key %d has been %s\n", key->vkCode, keyStatus);
#endif // _DEBUG
		if (key->vkCode == VK_CAPITAL) {
			if (wParam == WM_SYSKEYDOWN) {
				enabled = !enabled;
#if _DEBUG
				printf("Switchy has been %s\n", enabled ? "enabled" : "disabled");
#endif // _DEBUG
				return 1;
			}

			if (!enabled) {
				return CallNextHookEx(NULL, nCode, wParam, lParam);
			}

			if (wParam == WM_KEYDOWN && !capsLockPressed) {
				capsLockPressed = TRUE;

				if (GetKeyState(settings.modifier) & 0x8000) {
					ToggleCapsLockState();
					return 1;
				} else {
					if (settings.popup) {
						keybd_event(VK_LWIN, 0x3A, 0, 0);
						keybd_event(VK_SPACE, 0x3A, 0, 0);
						keybd_event(VK_SPACE, 0x3A, KEYEVENTF_KEYUP, 0);
						winPressed = TRUE;
					}
					else {
						HWND hWnd = GetForegroundWindow();
						if (hWnd) {
							hWnd = GetAncestor(hWnd, GA_ROOTOWNER);
							PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)HKL_NEXT);
						}
					}
				}
			}
			else if (wParam == WM_KEYUP) {
				capsLockPressed = FALSE;

				if (winPressed) {
					keybd_event(VK_LWIN, 0x3A, KEYEVENTF_KEYUP, 0);
				}
			}

			return 1;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}