#include <Windows.h>

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

typedef struct {
	BOOL popup;
	BYTE modifier;
} Settings;

void ShowError(LPCSTR message);
DWORD GetOSVersion();
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);


HHOOK hHook;
BOOL modifier;
BOOL processed = FALSE;
BOOL winPressed = FALSE;

Settings settings = {
	.popup = FALSE,
	.modifier = VK_LSHIFT
};


int main(int argc, char** argv) {
	if (argc > 1 && argv[1] == "nopopup") {
		settings.popup = FALSE;
	} else {
		settings.popup = GetOSVersion() >= 10;
	}

	HANDLE hMutex = CreateMutex(0, 0, "Switchy");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		ShowError("Another instance of Switchy is already running!");
		return 1;
	}

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
	if (hHook == NULL) {
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


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)lParam;

		if (key->vkCode == VK_CAPITAL) {
			if (wParam == WM_KEYDOWN) {
				if (GetKeyState(settings.modifier)) {
					UnhookWindowsHookEx(hHook);
					keybd_event(VK_CAPITAL, 0x3a, 0, 0);
					keybd_event(VK_CAPITAL, 0x3a, KEYEVENTF_KEYUP, 0);
					hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
					return 1;
				}
				else if (!processed) {
					processed = TRUE;

					if (settings.popup) {
						keybd_event(VK_LWIN, 0x3a, 0, 0);
						keybd_event(VK_SPACE, 0x3a, 0, 0);
						keybd_event(VK_SPACE, 0x3a, KEYEVENTF_KEYUP, 0);
						winPressed = TRUE;
					}
					else {
						HWND hWnd = GetForegroundWindow();
						if (hWnd) {
							hWnd = GetAncestor(hWnd, GA_ROOTOWNER);
							PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)HKL_NEXT);
						}
					}

					return 1;
				}
			}
			else if (wParam == WM_KEYUP) {
				processed = FALSE;

				if (winPressed) {
					keybd_event(VK_LWIN, 0x3a, KEYEVENTF_KEYUP, 0);
				}

				return 1;
			}
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}