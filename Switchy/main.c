#include <Windows.h>
#if _DEBUG
#include <stdio.h>
#endif // _DEBUG

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

typedef struct {
	BOOL popup;
} Settings;

void ShowError(LPCSTR message);
DWORD GetOSVersion();
void PressKey(int keyCode);
void ReleaseKey(int keyCode);
void ToggleCapsLockState();
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);


HHOOK hHook;
BOOL keystrokeCapsProcessed = FALSE;
BOOL keystrokeShiftProcessed = FALSE;
BOOL winPressed = FALSE;

Settings settings = {
	.popup = FALSE
};


int main(int argc, char** argv)
{
	if (argc > 1 && strcmp(argv[1], "showpopup") != 0)
	{
		settings.popup = GetOSVersion() >= 10;
	}
	else
	{
		settings.popup = FALSE;
	}

#if _DEBUG
	printf("Pop-up is %s\n", settings.popup ? "enabled" : "disabled");
#endif

	HANDLE hMutex = CreateMutex(0, 0, "Switchy");
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		ShowError("Another instance of Switchy is already running!");
		return 1;
	}

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
	if (hHook == NULL)
	{
		ShowError("Error calling \"SetWindowsHookEx(...)\"");
		return 1;
	}

	MSG messages;
	while (GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	UnhookWindowsHookEx(hHook);

	return 0;
}


void ShowError(LPCSTR message)
{
	MessageBox(NULL, message, "Error", MB_OK | MB_ICONERROR);
}


DWORD GetOSVersion()
{
	HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
	RTL_OSVERSIONINFOW osvi = { 0 };

	if (hMod)
	{
		RtlGetVersionPtr p = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");

		if (p)
		{
			osvi.dwOSVersionInfoSize = sizeof(osvi);
			p(&osvi);
		}
	}

	return osvi.dwMajorVersion;
}


void PressKey(int keyCode)
{
	keybd_event(keyCode, 0, 0, 0);
}


void ReleaseKey(int keyCode)
{
	keybd_event(keyCode, 0, KEYEVENTF_KEYUP, 0);
}


void ToggleCapsLockState()
{
	PressKey(VK_CAPITAL);
	ReleaseKey(VK_CAPITAL);
#if _DEBUG
	printf("Caps Lock state has been toggled\n");
#endif // _DEBUG
}


LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)lParam;
	if (nCode == HC_ACTION && !(key->flags & LLKHF_INJECTED))
	{
#if _DEBUG
		const char* keyStatus = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? "pressed" : "released";
		printf("Key %d has been %s\n", key->vkCode, keyStatus);
#endif // _DEBUG
		if (key->vkCode == VK_CAPITAL)
		{

			if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
			{
				keystrokeCapsProcessed = FALSE;

				if (winPressed)
				{
					winPressed = FALSE;
					ReleaseKey(VK_LWIN);
				}

				if (!settings.popup)
				{
					if (!keystrokeShiftProcessed) {
						PressKey(VK_MENU);
						PressKey(VK_LSHIFT);
						ReleaseKey(VK_MENU);
						ReleaseKey(VK_LSHIFT);
					}
					else
					{
						keystrokeShiftProcessed = FALSE;
					}
				}
			}

			if (wParam == WM_KEYDOWN && !keystrokeCapsProcessed)
			{
				keystrokeCapsProcessed = TRUE;

				if (keystrokeShiftProcessed == TRUE)
				{
					ToggleCapsLockState();
					return 1;
				}
				else
				{
					if (settings.popup)
					{
						PressKey(VK_LWIN);
						PressKey(VK_SPACE);
						ReleaseKey(VK_SPACE);
						winPressed = TRUE;
					}
				}
			}
			return 1;
		}

		else if (key->vkCode == VK_LSHIFT)
		{

			if ((wParam == WM_KEYUP || wParam == WM_SYSKEYUP) && !keystrokeCapsProcessed)
			{
				keystrokeShiftProcessed = FALSE;
			}

			if (wParam == WM_KEYDOWN && !keystrokeShiftProcessed)
			{
				keystrokeShiftProcessed = TRUE;

				if (keystrokeCapsProcessed == TRUE)
				{
					ToggleCapsLockState();
					if (settings.popup)
					{
						PressKey(VK_LWIN);
						PressKey(VK_SPACE);
						ReleaseKey(VK_SPACE);
						winPressed = TRUE;
					}

					return 0;
				}
			}
			return 0;
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}