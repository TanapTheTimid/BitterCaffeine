#include <stdio.h>
#include <Windows.h>
#include <synchapi.h>
#include "resource.h"

UINT MSG_ID = 5555;
char* TIP_ENABLED = "Caffeine Enabled! Your screen will stay awake.";
char* TIP_DISABLED = "Caffeine Disabled!";
char* INPUT_HINT = "Console (Type \"quit\" to exit the application): ";

WNDPROC default_wndproc;
NOTIFYICONDATA nidd;
HWND windowHandle;

HICON hIcon1;
HICON hIcon2;

volatile int state_wakelocked = 0;

LRESULT CALLBACK CustomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == MSG_ID && lParam == WM_LBUTTONDBLCLK) {
		if (state_wakelocked == 0) {
			state_wakelocked = 1;

			//activate wakelock
			nidd.hIcon = hIcon2;
			strcpy_s(nidd.szTip, 64, TIP_ENABLED);
			Shell_NotifyIconA(NIM_MODIFY, (PNOTIFYICONDATAA)&nidd);
			SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
		}
		else {
			state_wakelocked = 0;

			//disable wakelock
			nidd.hIcon = hIcon1;
			strcpy_s(nidd.szTip, 64, TIP_DISABLED);
			Shell_NotifyIconA(NIM_MODIFY, (PNOTIFYICONDATAA)&nidd);
			SetThreadExecutionState(ES_CONTINUOUS);
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI MyThreadFunction(LPVOID lpParam)
{
	char input[100];
	fprintf(stdout, "%s", INPUT_HINT);
	while (scanf_s("%50s", input, sizeof(input))) {
		if (!strncmp(input, "quit", 4)) {
			break;
		}
		else {
			fprintf(stdout, "Unknown Command: %s\n", input);
		}

		fprintf(stdout, "%s", INPUT_HINT);
	}

	int ret = Shell_NotifyIconA(NIM_DELETE, (PNOTIFYICONDATAA)&nidd);
	fprintf(stdout, "Quitting: %d\n", ret);

	SetThreadExecutionState(ES_CONTINUOUS);

	DeleteObject(windowHandle); //doing it just in case

	ExitProcess(0);

	return 0;
}

int main() {

	/*	Window construction
	*
	*	Construct a hidden window to handle notification icon...
	*
	*/

	WNDCLASS windowClass = { 0 };
	windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hInstance = NULL;
	windowClass.lpfnWndProc = CustomWndProc;
	windowClass.lpszClassName = L"Window in Console";
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClass(&windowClass))
		MessageBox(NULL, L"Could not register class", L"Error", MB_OK);
	windowHandle = CreateWindow(L"Window in Console",
		NULL,
		WS_POPUP,
		0,
		0,
		0,
		0,
		NULL,
		NULL,
		NULL,
		NULL);
	ShowWindow(windowHandle, SW_HIDE);


	/*	Icon and Notifications
	*
	*	Setup notification in the taskbar and preload icons
	*
	*/

	HMODULE hmodule = GetModuleHandleA(NULL);

	hIcon1;
	hIcon1 = LoadImageA(hmodule, MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
	hIcon2;
	hIcon2 = LoadImageA(hmodule, MAKEINTRESOURCE(IDI_ICON3), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);

	nidd;
	nidd.hWnd = windowHandle;
	nidd.cbSize = sizeof(nidd);
	nidd.hIcon = hIcon1;
	nidd.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nidd.uCallbackMessage = MSG_ID;
	strcpy_s(nidd.szTip, 64, TIP_DISABLED);

	int ret = Shell_NotifyIconA(NIM_ADD, (PNOTIFYICONDATAA)&nidd);
	fprintf(stdout, "Setting up notifications: %d\n", ret);


	/*	Console Input Thread
	*
	*	Setup second thread to handle console input
	*
	*/

	DWORD tid;
	HANDLE thand = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		MyThreadFunction,       // thread function name
		NULL,					// argument to thread function 
		0,                      // use default creation flags 
		&tid);   // returns the thread identifier
	
	if (thand == NULL)
	{
		fprintf(stdout, "Thread creation error...\n");
	}


	/*	WIN32API messaging
	*
	*	Handle win32 api messaging in the main thread
	*
	*/

	MSG messages;
	while (GetMessage(&messages, NULL, 0, 0) > 0)
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}


	/* Clean up */
	WaitForSingleObject(thand, INFINITE);
}
