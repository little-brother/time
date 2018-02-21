#include <windows.h>
#include <aygshell.h>
#include <wingdi.h>

#pragma comment(lib, "aygshell.lib")

#define IOCTL_HAL_REBOOT  0x101003C
extern "C" BOOL KernelIoControl(DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD);

const int WIDTH = GetSystemMetrics(SM_CXSCREEN);
const int HEIGHT = GetSystemMetrics(SM_CYSCREEN);
const int CW = WIDTH / 15;
const int CH = HEIGHT / 5;

HBRUSH hBrush = (HBRUSH) CreateSolidBrush(RGB(255, 0, 0));
HBRUSH hEraseBrush = (HBRUSH) GetStockObject(BLACK_BRUSH);

int colorNo = 0;
const int COLORS[20][3] = { 
	{255, 0, 0}, {200, 0, 0}, {150, 0, 0}, {100, 0, 0}, 
	{255, 255, 0}, {200, 200, 0}, {150, 150, 0}, {100, 100, 0},
	{0, 255, 0}, {0, 200, 0}, {0, 150, 0}, {0, 100, 0},
	{0, 0, 255}, {0, 0, 200}, {0, 0, 150}, {0, 0, 100},
	{255, 255, 255}, {200, 200, 200}, {150, 150, 150}, {100, 100, 100}
}; 
const int COLOR_COUNT = 20;

const boolean DIGITS[10][15] = {
	{1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1},
	{0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
	{1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1},
	{1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1},
	{1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1},
	{1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1},
	{1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1},
	{1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1},
	{1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
	{1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1}
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_LBUTTONDOWN: {
				int x = LOWORD(lParam); 
				int y = HIWORD(lParam); 
				if (x < 10 && y < 10) {
					DestroyWindow(hWnd);
					return 0;
				}

				if (x > WIDTH - 10 && y < 10) {
					KernelIoControl(IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL); // Power Off
					return 0;
				}

				if (y < CH || y > 4 * CH) {
					SYSTEMTIME st;
					GetLocalTime(&st);
					int digit = x / CW / 4;
					int sign = y < CH ? 1 : -1;
					
					int hour = st.wHour;
					int minute = st.wMinute;

					if (digit == 0 || digit == 1) {
						hour = hour + sign;
						if (hour == -1)
							hour = 23;
						if (hour == 24)
							hour = 0;
					}

					if (digit == 2 || digit == 3) {
						minute = minute + sign;
						if (minute == -1)
							minute = 59;
						if (minute == 60)
							minute = 0;
					}

					st.wHour = hour;
					st.wMinute = minute;
					SetLocalTime(&st);	
				} else {
					colorNo = (colorNo + ((x < WIDTH/2) ? - 1 : 1) + COLOR_COUNT) % COLOR_COUNT;
					DeleteObject(hBrush);
					hBrush = (HBRUSH) CreateSolidBrush(RGB(COLORS[colorNo][0], COLORS[colorNo][1], COLORS[colorNo][2]));
				}

				SendMessage(hWnd, WM_PAINT, 0, 0);
			}
			break;

		case WM_DESTROY: 
			STARTUPINFOW siStartupInfo; 
			PROCESS_INFORMATION piProcessInfo; 
			memset(&siStartupInfo, 0, sizeof(siStartupInfo)); 
			memset(&piProcessInfo, 0, sizeof(piProcessInfo)); 
			siStartupInfo.cb = sizeof(siStartupInfo);     
			CreateProcess(TEXT("explorer.exe"), 0, 0, 0, false, 0, 0, 0, &siStartupInfo, &piProcessInfo);

			PostQuitMessage(0);
			break;

		case WM_PAINT: {
				HDC hDC = GetWindowDC(hWnd);

				SYSTEMTIME st;
				GetLocalTime(&st);
				int time[] = {st.wHour / 10, st.wHour % 10, st.wMinute / 10, st.wMinute % 10};
				for (int digit = 0; digit < 4; digit++) {
					for (int i = 0; i < 16; i++) {						
						int colno = digit * 4 + i % 3;
						int rowno = i / 3;
						RECT rect = {colno * CW, rowno * CH, (colno + 1) * CW, (rowno + 1) * CH};
						FillRect(hDC, &rect, (DIGITS[time[digit]][i]) ? hBrush : hEraseBrush);					
					}
				}
				ReleaseDC(hWnd, hDC);

				return DefWindowProc(hWnd, msg, wParam, lParam);
			} 
			break;	

		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);		
	}

	return 0;
}

void CALLBACK onTimer(HWND hWnd, UINT msg, UINT idEvent, DWORD dwTime) {
	SendMessage(hWnd, WM_PAINT, 0, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = 0;
	wc.hCursor       = 0;
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = TEXT("MyTimeClass");

	RegisterClass(&wc);

	HWND hWnd = CreateWindowEx(
		WS_EX_NODRAG | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
		TEXT("MyTimeClass"), 
		NULL, 
		WS_VISIBLE | WS_BORDER , 
		0, 0, WIDTH, HEIGHT, 
		NULL, NULL, 
		hInstance, 
		NULL);

	if (!hWnd)
		return EXIT_FAILURE;

	SHFullScreen(hWnd, SHFS_HIDETASKBAR | SHFS_HIDESIPBUTTON | SHFS_HIDESTARTICON);

	SetTimer(hWnd, 1, 2000, (TIMERPROC)onTimer);

	SendMessage(hWnd, WM_PAINT, 0, 0);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}