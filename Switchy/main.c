#include <Windows.h>

#if _DEBUG
#include <stdio.h>

#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

HHOOK hHook;
BOOL enabled = TRUE;
BOOL appSwitchRequired = FALSE;
BOOL capsLockSwitchRequired = FALSE;
BOOL capsLockProcessed = FALSE;
BOOL shiftProcessed = FALSE;

void ShowError(LPCSTR message) {
  MessageBox(NULL, message, "Error", MB_OK | MB_ICONERROR);
}

void PressKey(WORD keyCode) {
  INPUT input = {
    .type = INPUT_KEYBOARD,
    .ki.wVk = keyCode,
    .ki.dwFlags = 0
  };
  SendInput(1, &input, sizeof(INPUT));
}

void ReleaseKey(WORD keyCode) {
  INPUT input = {
    .type = INPUT_KEYBOARD,
    .ki.wVk = keyCode,
    .ki.dwFlags = KEYEVENTF_KEYUP  
  };
  SendInput(1, &input, sizeof(INPUT));
}

void SwitchCapsLockState() {
  PressKey(VK_CAPITAL);
  ReleaseKey(VK_CAPITAL);
  LOG("Caps Lock state has been switched\n");
}

void SetCapsLockProcessed(BOOL value) {
  capsLockProcessed = value;
}

void SetShiftProcessed(BOOL value) {
  shiftProcessed = value;
}

void SetAppSwitchRequired(BOOL value) {
  appSwitchRequired = value;
}

void SetCapsLockSwitchRequired(BOOL value) {
  capsLockSwitchRequired = value;
}

LRESULT CALLBACK HandleKeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam) {
  KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)lParam;
  if (nCode == HC_ACTION && !(key->flags & LLKHF_INJECTED)) {
    if (key->vkCode == VK_CAPITAL) {
      if (wParam == WM_SYSKEYDOWN) {
        SetAppSwitchRequired(TRUE);
        SetCapsLockProcessed(TRUE);
        return 1;
      }

      if (wParam == WM_SYSKEYUP || (wParam == WM_KEYUP && appSwitchRequired)) {
        enabled = !enabled;
        SetAppSwitchRequired(FALSE);
        SetCapsLockProcessed(FALSE);
        LOG("Switchy has been %s\n", enabled ? "enabled" : "disabled");
        return 1;
      }

      if (wParam == WM_KEYDOWN) {
        SetCapsLockProcessed(TRUE);
        if (enabled) {
          if (shiftProcessed) {
            SetCapsLockSwitchRequired(TRUE);
          }
          return 1;
        }
      } else if (wParam == WM_KEYUP) {
        SetCapsLockProcessed(FALSE);
        if (enabled) {
          if (shiftProcessed || capsLockSwitchRequired) {
            SwitchCapsLockState();
            SetCapsLockSwitchRequired(FALSE);
          } else {
            PressKey(VK_MENU);
            PressKey(VK_LSHIFT);
            ReleaseKey(VK_MENU);
            ReleaseKey(VK_LSHIFT);
          }
          return 1;
        }
      }
    } else if (key->vkCode == 164) {
      if (wParam == WM_SYSKEYDOWN && capsLockProcessed) {
        SetAppSwitchRequired(TRUE);
      }
    } else if (key->vkCode == VK_LSHIFT) {
      if (wParam == WM_KEYDOWN) {
        SetShiftProcessed(TRUE);
        if (capsLockProcessed) {
          SetCapsLockSwitchRequired(TRUE);
        }
      } else if (wParam == WM_KEYUP) {
        SetShiftProcessed(FALSE);
      }
    }
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
  HANDLE hMutex = CreateMutex(0, 0, "Switchy");
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    ShowError("Another instance of Switchy is already running!");
    return 1;
  }

  hHook = SetWindowsHookEx(WH_KEYBOARD_LL, HandleKeyboardEvent, 0, 0);
  if (hHook == NULL) {
    ShowError("Error calling 'SetWindowsHookEx'");
    return 1;
  }

  MSG messages;
  while (GetMessage(&messages, NULL, 0, 0)) {
    TranslateMessage(&messages);
    DispatchMessage(&messages);
  }

  UnhookWindowsHookEx(hHook);
  if (hMutex) {
    ReleaseMutex(hMutex);
  }

  return 0;
}

