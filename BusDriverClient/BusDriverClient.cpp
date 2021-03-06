// BusDriverClient.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BusDriverClient.h"
#include "ControllerMgr.h"
#include "SocketMgr.h"
#include <fstream>
#include <opencv2/opencv.hpp>

using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

ControllerMgr * g_pControllerMgr = NULL;
SocketMgr * g_pSocketMgr = NULL;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
void				InputUpdateCallback(int index, BOOL value);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	// Initialize OpenCV window.
	string windowName = "Video Stream from Edison";
	cv::namedWindow(windowName);

 	// Initialize socket manager.
	g_pSocketMgr = new SocketMgr();
	if (!g_pSocketMgr->Initialize())
	{
		MessageBox(NULL, _T("Cannot initialize socket manager."), _T("Bus Driver Client"), MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return false;
	}

	// Read IP address from file and attempt to connect.
	string ipAddress;
	ifstream infile("ip.txt");
	if (!infile.is_open())
	{
		MessageBox(NULL, _T("Cannot find ip.txt file."), _T("Bus Driver Client"), MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return false;
	}
	getline(infile, ipAddress);
	infile.close();

	if (!g_pSocketMgr->Connect(ipAddress.c_str()))
	{
		MessageBox(NULL, _T("Cannot connect to Edison."), _T("Bus Driver Client"), MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		return false;
	}

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BUSDRIVERCLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BUSDRIVERCLIENT));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BUSDRIVERCLIENT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BUSDRIVERCLIENT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 320, 240, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		{
		// Initialize controller manager.
		g_pControllerMgr = new ControllerMgr();
		if (!g_pControllerMgr->Initialize(hWnd, &InputUpdateCallback))
			MessageBox(NULL, _T("Cannot initialize controller manager."), _T("Bus Driver Client"), MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		break;
		}
	case WM_INPUT:
		if (g_pControllerMgr)
		{
			// Handle raw input data from joystick.
			// First determine size of input data and allocate buffer from heap.
			UINT bufferSize;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
			HANDLE hHeap = GetProcessHeap();
			PRAWINPUT pRawInput = (PRAWINPUT)HeapAlloc(hHeap, 0, bufferSize);
			if (pRawInput)
			{
				// Retrieve actual raw device data and hand it off to the controller manager.
				GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pRawInput, &bufferSize, sizeof(RAWINPUTHEADER));
				g_pControllerMgr->ProcessRawInput(pRawInput);
				HeapFree(hHeap, 0, pRawInput);
			}
			else
				MessageBox(NULL, _T("Cannot allocate buffer for raw device input."), _T("Stream Video"), MB_OK|MB_ICONERROR|MB_SYSTEMMODAL);
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		// Clean up.
		delete g_pControllerMgr;
		g_pControllerMgr = NULL;
		delete g_pSocketMgr;
		g_pSocketMgr = NULL;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void InputUpdateCallback(int index, BOOL value)
{
	// For debug only.
}
