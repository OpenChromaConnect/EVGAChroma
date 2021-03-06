#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include <string>
#include <tchar.h>
#include <shellapi.h>
#include <RzErrors.h>
#include <RzChromaBroadcastAPITypes.h>
#include <RzChromaBroadcastAPIDefines.h>
#include "OlsApi.h"
#include "Acx30.h"
#include "resource.h"

using namespace RzChromaBroadcastAPI;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _WIN64
#define CHROMABROADCASTAPIDLL        _T("RzChromaBroadcastAPI64.dll")
#else
#define CHROMABROADCASTAPIDLL        _T("RzChromaBroadcastAPI.dll")
#endif

typedef RZRESULT(*INIT)(RZAPPID app);
typedef RZRESULT(*INITEX)(int index, const char *title);
typedef RZRESULT(*UNINIT)();
typedef RZRESULT(*REGISTEREVENTNOTIFICATION)(RZEVENTNOTIFICATIONCALLBACK callback);
typedef RZRESULT(*UNREGISTEREVENTNOTIFICATION)();

INIT Init = nullptr;
INITEX InitEx = nullptr;
UNINIT UnInit = nullptr;
REGISTEREVENTNOTIFICATION RegisterEventNotification = nullptr;
UNREGISTEREVENTNOTIFICATION UnRegisterEventNotification = nullptr;

NOTIFYICONDATA niData;
#define SWM_TRAYMSG (WM_USER + 1)
#define SWM_EXIT (WM_USER + 2)
#define SWM_COLOR1 (WM_USER + 3)
#define SWM_COLOR2 (WM_USER + 4)
#define SWM_COLOR3 (WM_USER + 5)
#define SWM_COLOR4 (WM_USER + 6)
#define SWM_COLOR5 (WM_USER + 7)

bool EnabledColors[5] = { true, false, false, false, false };

RZRESULT BroadcastHandler(CHROMA_BROADCAST_TYPE type, PRZPARAM pData)
{
	switch (type)
	{
	case BROADCAST_EFFECT:
	{
		CHROMA_BROADCAST_EFFECT* effect = (CHROMA_BROADCAST_EFFECT*)pData;
#ifdef _DEBUG
		printf("BROADCAST_EFFECT: %06x %06x %06x %06x %06x %d\n", effect->CL1, effect->CL2, effect->CL3, effect->CL4, effect->CL5, effect->IsAppSpecific);
#endif
		uint32_t r = 0, g = 0, b = 0, c = 0;
		RZCOLOR colors[] = { effect->CL1, effect->CL2, effect->CL3, effect->CL4, effect->CL5 };
		for (int i = 0; i < 5; i++)
		{
			auto color = colors[i];
			if (EnabledColors[i] && color)
			{
				r += color & 0xFF;
				g += (color >> 8) & 0xFF;
				b += (color >> 16) & 0xFF;
				++c;
			}
		}
		if (c)
		{
			r /= c;
			g /= c;
			b /= c;
		}
		SetLedColor(r, g, b);
		break;
	}
	case BROADCAST_STATUS:
		CHROMA_BROADCAST_STATUS status = (CHROMA_BROADCAST_STATUS)(LONG)pData;
#ifdef _DEBUG
		printf("BROADCAST_STATUS: %x\n", status);
#endif
		if (status == LIVE)
		{
			SetLedMode(LED_MODE_STATIC);

		}
		else if (status == NOT_LIVE)
		{
			SetLedMode(LED_MODE_OFF);

		}
		break;
	}

	return 0;
}

void ShowContextMenu(HWND hWnd)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if (hMenu)
	{
		InsertMenu(hMenu, -1, MF_BYPOSITION | (EnabledColors[0] ? MF_CHECKED : MF_UNCHECKED), SWM_COLOR1, _T("Color 1"));
		InsertMenu(hMenu, -1, MF_BYPOSITION | (EnabledColors[1] ? MF_CHECKED : MF_UNCHECKED), SWM_COLOR2, _T("Color 2"));
		InsertMenu(hMenu, -1, MF_BYPOSITION | (EnabledColors[2] ? MF_CHECKED : MF_UNCHECKED), SWM_COLOR3, _T("Color 3"));
		InsertMenu(hMenu, -1, MF_BYPOSITION | (EnabledColors[3] ? MF_CHECKED : MF_UNCHECKED), SWM_COLOR4, _T("Color 4"));
		InsertMenu(hMenu, -1, MF_BYPOSITION | (EnabledColors[4] ? MF_CHECKED : MF_UNCHECKED), SWM_COLOR5, _T("Color 5"));
		InsertMenu(hMenu, -1, MF_SEPARATOR, 0, 0);
		InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));

		SetForegroundWindow(hWnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN,
			pt.x, pt.y, 0, hWnd, NULL);
		DestroyMenu(hMenu);
	}
}

INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case SWM_TRAYMSG:
		switch (lParam)
		{
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case SWM_COLOR1:
			EnabledColors[0] = !EnabledColors[0];
			break;
		case SWM_COLOR2:
			EnabledColors[1] = !EnabledColors[1];
			break;
		case SWM_COLOR3:
			EnabledColors[2] = !EnabledColors[2];
			break;
		case SWM_COLOR4:
			EnabledColors[3] = !EnabledColors[3];
			break;
		case SWM_COLOR5:
			EnabledColors[4] = !EnabledColors[4];
			break;
		case SWM_EXIT:
			DestroyWindow(hWnd);
			break;
		}
		return 1;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &niData);
		PostQuitMessage(0);
		break;
	}

	return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef _DEBUG
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
#endif

	if (!InitializeOls())
	{
		MessageBoxA(0, "Failed to init Ring0", "[UNOFFICIAL] EVGA Chroma X299", 0);
		return -1;
	}

	if (!InitSMBus())
	{
		MessageBoxA(0, "Failed to init SMBus", "[UNOFFICIAL] EVGA Chroma X299", 0);
		return -1;
	}

	if (!ledInit())
	{
		MessageBoxA(0, "Failed to init LED", "[UNOFFICIAL] EVGA Chroma X299", 0);
		return -1;
	}

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
	niData.cbSize = sizeof(NOTIFYICONDATA);
	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	niData.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	niData.hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	niData.uCallbackMessage = SWM_TRAYMSG;
	Shell_NotifyIcon(NIM_ADD, &niData);

	SetLedMode(LED_MODE_OFF);

	HMODULE hModule = ::LoadLibrary(CHROMABROADCASTAPIDLL);
	if (hModule == NULL)
	{
		MessageBoxA(0, "Failed to load Chroma Broadcast API", "[UNOFFICIAL] EVGA Chroma X299", 0);
		return -1;

	}

	Init = (INIT)::GetProcAddress(hModule, "Init");
	InitEx = (INITEX)::GetProcAddress(hModule, "InitEx");
	UnInit = (UNINIT)::GetProcAddress(hModule, "UnInit");
	RegisterEventNotification = (REGISTEREVENTNOTIFICATION)::GetProcAddress(hModule, "RegisterEventNotification");
	UnRegisterEventNotification = (UNREGISTEREVENTNOTIFICATION)::GetProcAddress(hModule, "UnRegisterEventNotification");

	if (!InitEx)
	{
		MessageBoxA(0, "Missing InitEx in Chroma Broadcast API", "[UNOFFICIAL] EVGA Chroma X299", 0);
		return -1;
	}

	if (!InitEx(1002, "[UNOFFICIAL] EVGA Chroma X299") == RZRESULT_SUCCESS)
	{
		MessageBoxA(0, "Failed to init Chroma Broadcast API", "[UNOFFICIAL] EVGA Chroma X299", 0);
		return -1;
	}

	if (RegisterEventNotification)
		RegisterEventNotification(BroadcastHandler);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessage(msg.hwnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	Shell_NotifyIcon(NIM_DELETE, &niData);
	SetLedMode(LED_MODE_OFF);
	DeinitializeOls();

	return 0;
}

