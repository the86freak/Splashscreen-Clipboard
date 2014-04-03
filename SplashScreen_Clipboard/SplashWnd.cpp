#include <Windows.h>
#include <CommCtrl.h>
#include <GdiPlus.h>
#include <string>
#include <process.h>
#include <tchar.h>
#include "SplashWnd.h"


#define MSG_UPDATECLIPBOARD (WM_APP + 1)

CSplashWnd::CSplashWnd( HWND hParent )
{
	m_hThread = NULL;
	m_pImage = NULL;
	m_hSplashWnd = NULL;
	m_ThreadId = 0;
	m_hProgressWnd = NULL;
	m_hEvent = NULL;
	m_hParentWnd = hParent;
}

CSplashWnd::~CSplashWnd()
{
	Hide();
	//if (m_pImage) delete m_pImage;
}

void CSplashWnd::SetImage( Gdiplus::Image* pImage )
{
	if (m_pImage == NULL && pImage != NULL)
		m_pImage = pImage->Clone();
}

void CSplashWnd::Show()
{
	if (m_hThread == NULL)
	{
		m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, SplashThreadProc, static_cast<LPVOID>(this), 0, &m_ThreadId);
		if (WaitForSingleObject(m_hEvent, 5000) == WAIT_TIMEOUT)
		{
			OutputDebugString(L"Error starting SplashThread\n");
		}
	}
	else
	{
		PostThreadMessage( m_ThreadId, WM_ACTIVATE, WA_CLICKACTIVE, 0 );
	}
}

void CSplashWnd::Hide()
{
	if (m_hThread)
	{
		PostThreadMessage(m_ThreadId, WM_QUIT, 0, 0);
		if ( WaitForSingleObject(m_hThread, 9000) == WAIT_TIMEOUT )
		{
			::TerminateThread( m_hThread, 2222 );
		}
		CloseHandle(m_hThread);
		CloseHandle(m_hEvent);
	}
	m_hThread = NULL;
}

UINT progressTotal = 0;
UINT oldProgress = -1;


unsigned int __stdcall CSplashWnd::SplashThreadProc( void* lpParameter )
{
	CSplashWnd* pThis = static_cast<CSplashWnd*>(lpParameter);
	if (pThis->m_pImage == NULL) return 0;

	// Register your unique class name
	WNDCLASS wndcls = {0};

	wndcls.style = CS_HREDRAW | CS_VREDRAW;
	wndcls.lpfnWndProc = SplashWndProc; 
	wndcls.hInstance = GetModuleHandle(NULL);
	wndcls.hCursor = LoadCursor(NULL, IDC_APPSTARTING);
	wndcls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndcls.lpszClassName = L"SplashWnd";
	wndcls.hIcon = LoadIcon(wndcls.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClass(&wndcls))
	{
		if (GetLastError() != 0x00000582) // already registered)
		{
			OutputDebugString(L"Unable to register class SplashWnd\n");
			return 0;
		}
	}

  // try to find monitor where mouse was last time
  POINT point = { 0 };
  MONITORINFO mi = { sizeof(MONITORINFO), 0 };
  HMONITOR hMonitor = 0;
  RECT rcArea = { 0 };

  ::GetCursorPos( &point );
  hMonitor = ::MonitorFromPoint( point, MONITOR_DEFAULTTONEAREST );
  if ( ::GetMonitorInfo( hMonitor, &mi ) )
  {
    rcArea.left = ( mi.rcMonitor.right + mi.rcMonitor.left - static_cast<long>(pThis->m_pImage->GetWidth()) ) /2;
    rcArea.top = ( mi.rcMonitor.top + mi.rcMonitor.bottom - static_cast<long>(pThis->m_pImage->GetHeight()) ) /2;
  }
  else
  {
    SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcArea, NULL);
    rcArea.left = (rcArea.right + rcArea.left - pThis->m_pImage->GetWidth())/2;
    rcArea.top = (rcArea.top + rcArea.bottom - pThis->m_pImage->GetHeight())/2;
  }  

	//
	pThis->m_hSplashWnd = CreateWindowEx(pThis->m_WindowName.length()?0:WS_EX_TOOLWINDOW, L"SplashWnd", pThis->m_WindowName.c_str(), 
		WS_CLIPCHILDREN|WS_POPUP, rcArea.left, rcArea.top, pThis->m_pImage->GetWidth(), pThis->m_pImage->GetHeight(), 
		pThis->m_hParentWnd, NULL, wndcls.hInstance, NULL);
	if (!pThis->m_hSplashWnd)
	{
		OutputDebugString(L"Unable to create SplashWnd\n");
		return 0;
	}
  

	SetClipboardViewer(pThis->m_hSplashWnd);

	SetWindowLongPtr(pThis->m_hSplashWnd, GWL_USERDATA, reinterpret_cast<LONG_PTR>(pThis) );
	ShowWindow(pThis->m_hSplashWnd, SW_SHOWNOACTIVATE);

	MSG msg;
	BOOL bRet;
	LONG timerCount = 0;

	PeekMessage(&msg, NULL, 0, 0, 0); // invoke creating message queue
	SetEvent(pThis->m_hEvent);
	SendMessage(pThis->m_hSplashWnd, PBM_SETPOS, 0, 0); // initiate progress bar creation
	while (((bRet = GetMessage( &msg, NULL, 0, 0 )) != 0) && progressTotal<100)
	{ 
		if (msg.message == WM_QUIT) break;
		if (msg.message == PBM_SETPOS)
		{
			KillTimer(NULL, pThis->m_TimerId);
			SendMessage(pThis->m_hSplashWnd, PBM_SETPOS, msg.wParam, msg.lParam);
			continue;
		}
		if (msg.message == PBM_SETSTEP)
		{
			SendMessage(pThis->m_hSplashWnd, PBM_SETPOS, LOWORD(msg.wParam), 0); // initiate progress bar creation
			SendMessage(pThis->m_hProgressWnd, PBM_SETSTEP, (HIWORD(msg.wParam)-LOWORD(msg.wParam))/msg.lParam, 0);
			timerCount = static_cast<LONG>(msg.lParam);
			pThis->m_TimerId = SetTimer(NULL, 0, 1000, NULL);
			continue;
		}
		if (msg.message == WM_TIMER && msg.wParam == pThis->m_TimerId)
		{
			SendMessage(pThis->m_hProgressWnd, PBM_STEPIT, 0, 0);
			timerCount--;
			if (timerCount <= 0) {
				timerCount =0;
				KillTimer(NULL, pThis->m_TimerId);
        Sleep(0);
			}
			continue;
		}
		if (msg.message == PBM_SETBARCOLOR)
		{
			if (!IsWindow(pThis->m_hProgressWnd)) {
				SendMessage(pThis->m_hSplashWnd, PBM_SETPOS, 0, 0); // initiate progress bar creation
			}
			SendMessage(pThis->m_hProgressWnd, PBM_SETBARCOLOR, msg.wParam, msg.lParam);
			continue;
		}

		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
		}
	}
	DestroyWindow(pThis->m_hSplashWnd);
	exit(0);
	return 0;
}


void updateProgressBar(std::wstring unparsedMessage, CSplashWnd* splash){
			//LPWSTR ptr = convertCharArrayToLPCWSTR(chBuf);
	//LPWSTR ptr = wString; //convertCharArrayToLPCWSTR(clipBoardBuffer);
	//OutputDebugString(ptr);
	std::wstring convertingString = unparsedMessage;		    

	int foundSplash;
	foundSplash = convertingString.find(L"SPLASH: {progress: '") + 20;
	std::wstring progress = convertingString.substr(foundSplash);
	int progressEnd = progress.find(L"'");
	progress = progress.substr(0,progressEnd);

	UINT progressNumber = _ttoi(progress.c_str());

	progressTotal = progressNumber;

	int foundMessageStart = convertingString.find(L"message:'")+9;
	int foundMessageEnd = convertingString.find(L"'}");

	std::wstring message;
	if (foundMessageEnd != -1 && progressTotal != oldProgress)
	{
		message = convertingString.substr(foundMessageStart, foundMessageEnd-foundMessageStart);
		/*
		TCHAR s[256];
		swprintf(s, _T("Progress: %d"), progressTotal);	
		OutputDebugString(s);
		OutputDebugString(L"\n");
		OutputDebugString(L" Message: ");
		OutputDebugString((LPCWSTR)(message.c_str()));
		OutputDebugString(L"\n");
		*/
	}else
	{
		message = L"";
	}			

	splash->SetProgress(progressTotal, (message.c_str()) );

	oldProgress = progressTotal;
	if (oldProgress >= 100)
	{
		exit(0);
	}
}


static bool g_fIgnoreClipboardChange = false;
LRESULT CALLBACK CSplashWnd::SplashWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	CSplashWnd* pInstance = reinterpret_cast<CSplashWnd*>(GetWindowLongPtr(hwnd, GWL_USERDATA));
	if (pInstance == NULL)
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

  switch (uMsg)
	{
		case WM_PAINT:
		{
			if (pInstance->m_pImage)
			{
				Gdiplus::Graphics gdip(hwnd);
				gdip.DrawImage(pInstance->m_pImage, 0, 0, pInstance->m_pImage->GetWidth(), pInstance->m_pImage->GetHeight());

				if ( pInstance->m_ProgressMsg.size() > 0 )
				{
					Gdiplus::Font msgFont( L"Tahoma", 8, Gdiplus::UnitPixel );
					Gdiplus::SolidBrush msgBrush( static_cast<DWORD>(Gdiplus::Color::Black) );
					gdip.DrawString( pInstance->m_ProgressMsg.c_str(), -1, &msgFont, Gdiplus::PointF(2.0f, pInstance->m_pImage->GetHeight()-34.0f), &msgBrush );
				}
			}
			ValidateRect(hwnd, NULL);
			return 0;
		} break;
		// enhancement: http://stackoverflow.com/questions/18311274/monitoring-clipboard check if clipboard hasn't changed yet or is in progress currently.
		case WM_DRAWCLIPBOARD:
			{
				OutputDebugString(L" SW : WM_DRAW ::: ");
				

					char * clipBoardBuffer = NULL;
			//open the clipboard	
				
			std::wstring wString;
				
			if ( OpenClipboard(hwnd) ) 
			{
				HANDLE hData = GetClipboardData( CF_TEXT );
				if (hData)
				{
					clipBoardBuffer = (char*)GlobalLock( hData );			
					if (strlen(clipBoardBuffer) > 1)
					{
						wString=new wchar_t[strlen(clipBoardBuffer)+1];

						OutputDebugString(L"OPENED!!!!");

						MultiByteToWideChar(CP_ACP, 0, clipBoardBuffer, -1, ((LPWSTR)wString.c_str()), strlen(clipBoardBuffer)+1);
						GlobalUnlock( hData );
						CloseClipboard();
						if (wString.find(L"SPLASH") != -1)
						{
							updateProgressBar(wString, pInstance);
						}
					}

				}


				CloseClipboard();
			}	
			
			
				//OutputDebugString((LPCWSTR)(wString.c_str()));
				//OutputDebugString(L"\n");
				
				//if (wString.find(L"message") != -1)
				//{
				//	pInstance->SetProgress( 10, (wString.c_str()));
				//}
				
				return 0;
			}break;
		case WM_CHANGECBCHAIN:
			{
				OutputDebugString(L" SW : WM_CHANGEC : ");
				char * clipBoardBuffer = NULL;
			//open the clipboard	
	
				/*
			if ( OpenClipboard(hwnd) ) 
			{
				HANDLE hData = GetClipboardData( CF_TEXT );
				clipBoardBuffer = (char*)GlobalLock( hData );
				//fromClipboard = buffer.;
				
			
				std::wstring wString=new wchar_t[1024];
				MultiByteToWideChar(CP_ACP, 0, clipBoardBuffer, -1, ((LPWSTR)wString.c_str()), 1024);

				//LPWSTR ptr = convertCharArrayToLPCWSTR(chBuf);
				//LPWSTR ptr = wString; //convertCharArrayToLPCWSTR(clipBoardBuffer);
				OutputDebugString((LPCWSTR)(wString.c_str()));
				OutputDebugString(L"\n");
				
				GlobalUnlock( hData );
				CloseClipboard();
			}
			OutputDebugString(L"\n");
			*/

				return 0;
			}
			break;
		case MSG_UPDATECLIPBOARD:
			{
			
			OutputDebugString(L" SW : MSG_UPDATEC ");


				char * clipBoardBuffer = NULL;
			//open the clipboard			
			if ( OpenClipboard(NULL) ) 
			{
				HANDLE hData = GetClipboardData( CF_TEXT );
				clipBoardBuffer = (char*)GlobalLock( hData );
				//fromClipboard = buffer.;
				GlobalUnlock( hData );
				CloseClipboard();
			}


			std::wstring wString=new wchar_t[1024];
			MultiByteToWideChar(CP_ACP, 0, clipBoardBuffer, -1, ((LPWSTR)wString.c_str()), 1024);


			//LPWSTR ptr = convertCharArrayToLPCWSTR(chBuf);
			//LPWSTR ptr = wString; //convertCharArrayToLPCWSTR(clipBoardBuffer);
			OutputDebugString((LPCWSTR)(wString.c_str()));


			/*
				g_fIgnoreClipboardChange = true;
				if(OpenClipboard(hwnd)) {
					HGLOBAL hClipboardData;
					hClipboardData = GlobalAlloc(GMEM_MOVEABLE, test.size() + 1);
					char * pchData;
					pchData = (char*)GlobalLock(hClipboardData);
					memcpy(pchData, test.c_str(), test.size() + 1);
					GlobalUnlock(hClipboardData);
					SetClipboardData(CF_TEXT, hClipboardData);
					CloseClipboard();
				}

			g_fIgnoreClipboardChange = false;*/
			}
			break;
		case PBM_SETPOS:
			{
				//progress bar
				if (!IsWindow(pInstance->m_hProgressWnd))
				{
					RECT client;
					GetClientRect(hwnd, &client);
					pInstance->m_hProgressWnd = CreateWindow(PROGRESS_CLASS, NULL, WS_CHILD|WS_VISIBLE,
						4, client.bottom-20, client.right-8, 16, hwnd, NULL, GetModuleHandle(NULL), NULL);
					SendMessage(pInstance->m_hProgressWnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			}
			SendMessage(pInstance->m_hProgressWnd, PBM_SETPOS, wParam, 0);

			
			const std::wstring* msg = reinterpret_cast<std::wstring*>( lParam );
			if ( msg && pInstance->m_ProgressMsg != *msg )
			{
				pInstance->m_ProgressMsg = *msg;
				delete msg;
        SendMessage( pInstance->m_hSplashWnd, WM_PAINT, 0, 0 );
			}
			return 0;
		} break; 
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CSplashWnd::SetWindowName( const wchar_t* windowName )
{
	m_WindowName = windowName;
}

void CSplashWnd::SetProgress( UINT procent )
{
  SetProgress( procent, static_cast<wchar_t*>(NULL) );
}

void CSplashWnd::SetProgress( UINT procent, const wchar_t* msg )
{
	std::wstring* tempmsg = new std::wstring( msg );
	PostThreadMessage( m_ThreadId, PBM_SETPOS, procent, reinterpret_cast<LPARAM>(tempmsg) );
}

void CSplashWnd::SetProgress( UINT procent, UINT nResourceID, HMODULE hModule )
{
	wchar_t* msg;
	int len = ::LoadString( hModule, nResourceID, reinterpret_cast<wchar_t*>(&msg), 0 );

	std::wstring* tempmsg = new std::wstring( msg, len );
	PostThreadMessage( m_ThreadId, PBM_SETPOS, procent, reinterpret_cast<LPARAM>(tempmsg) );
}

void CSplashWnd::SetAutoProgress( UINT from, UINT to, UINT seconds )
{
	PostThreadMessage(m_ThreadId, PBM_SETSTEP, MAKEWPARAM(from, to), seconds);
}

void CSplashWnd::SetProgressBarColor( COLORREF color )
{
	PostThreadMessage(m_ThreadId, PBM_SETBARCOLOR, 0, color);
}
