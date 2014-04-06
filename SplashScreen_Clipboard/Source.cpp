#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#pragma comment (lib, "gdiplus.lib")
#pragma comment (lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#define ISOLATION_AWARE_ENABLED 1


#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>
#include <iostream>

#include <sstream>

#include <CommCtrl.h>
#include <GdiPlus.h>
#include <string>

#include "SplashWnd.h"

#include <stdlib.h>

#include <vector>

#include "comutil.h"
#include <io.h>

#include "CGdiPlusBitmap.h"

#include "resource.h"

using namespace std;

//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by test.rc
//
#define IDC_MYICON                      2
#define IDD_TEST_DIALOG                 102
#define IDD_ABOUTBOX                    103
#define IDM_ABOUT                       104
#define IDM_EXIT                        105
#define IDC_TEST                        109
#define IDR_MAINFRAME                   128
#define IDS_PROGRESS_FROM_RESOURCES     129
#define IDC_STATIC                      -1

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        130
#define _APS_NEXT_COMMAND_VALUE         32771
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           110
#endif
#endif

#define BUFSIZE 1024 

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

void CreateChildProcess(void); 
void WriteToPipe(void); 
void ReadFromPipe(void); 
void ErrorExit(PTSTR); 

#define MAX_LOADSTRING 100


using namespace std;



wchar_t *convertCharArrayToLPCWSTR(const char* charArray)
{ 
	size_t newsize = strlen(charArray) + 1;
	wchar_t* wString=new wchar_t[newsize];

	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wString, newsize, charArray, _TRUNCATE);

	//MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, newsize);

	return wString;
}



std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

MSG msg; // Window Messages that are thrown back and forth

#define MSG_UPDATECLIPBOARD     (WM_APP + 1)

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance,
					LPSTR lpszArgument, int nFunsterStil)
					//int _tmain(int argc, TCHAR *argv[]) 
{ 
	try{
	MyRegisterClass(hThisInstance);

	Gdiplus::GdiplusStartupInput gdiSI;
	Gdiplus::GdiplusStartupOutput gdiSO;
	ULONG_PTR gdiToken;
	ULONG_PTR gdiHookToken;
	gdiSI.SuppressBackgroundThread = TRUE;
	Gdiplus::GdiplusStartup(&gdiToken,&gdiSI,&gdiSO);
	gdiSO.NotificationHook(&gdiHookToken);

	CreateChildProcess();
	CSplashWnd splash;

	Gdiplus::Bitmap* pImage;

	if (_access("splash.jpg", 0)==-1)
	{			
		CGdiPlusBitmapResource* pBitmap = new CGdiPlusBitmapResource;
		if (pBitmap->Load(SPLASH_GIF))
		{
			pImage = pBitmap->m_pBitmap;
		}
	}else{
		pImage = Gdiplus::Bitmap::FromFile(L"splash.jpg");
	}

	splash.SetImage(pImage);
	splash.SetWindowName(L"Marktfuehrer wird geladen...");

	delete pImage; // you are free to delete now
	splash.Show();
	BOOL loaded = FALSE;
	//HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

		UINT progressTotal = 0;
		UINT oldProgress = -1;

		// Main message loop:
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	
	gdiSO.NotificationUnhook(gdiHookToken);
	Gdiplus::GdiplusShutdown(gdiToken);

	exit(0);
	return (int) msg.wParam;
	}
	catch(exception& e){
		::MessageBox(NULL, L"Die Anwendung konnte die Ladeanzeige nicht finden. Der Marktführer wurde dennoch gestartet. Das kann einen Momemt dauern. Dabei wir Ihnen kein Ladebildschirm gezeigt.", L"Failed", MB_ICONSTOP);
		exit(0);
		return (int) msg.wParam;
	}
} 




void CreateChildProcess()
	// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
	SECURITY_ATTRIBUTES saAttr; 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	TCHAR szCmdline[]=TEXT("nw.exe");
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE; 

	// Set up members of the PROCESS_INFORMATION structure. 
	
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.

	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = g_hChildStd_OUT_Wr;
	siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
	
	// Create the child process. 

	bSuccess = CreateProcess(NULL, 
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	// If an error occurs, exit the application. 
	if ( ! bSuccess ){
		ErrorExit(TEXT("CreateProcess"));
	}
	else 
	{
		// Close handles to the child process and its primary thread.
		// Some applications might keep these handles to monitor the status
		// of the child process, for example. 

		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}
}


void ErrorExit(PTSTR lpszFunction) 

	// Format a readable error message, display a message box, 
	// and exit from the application.
{ 
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(1);
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
	wcex.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_TEST);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(NULL, IDI_WINLOGO);

	return RegisterClassEx(&wcex);
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
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
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
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
