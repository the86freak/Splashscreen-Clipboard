#include <Windows.h>
#include <CommCtrl.h>
#include <GdiPlus.h>
#include <string>
#include <string.h>
#include <process.h>
#include <tchar.h>
#include "SplashWnd.h"
#include <tlhelp32.h>

#define MSG_UPDATECLIPBOARD (WM_APP + 1)

BOOL GetProcessList( );
BOOL ListProcessModules( DWORD dwPID );
BOOL ListProcessThreads( DWORD dwOwnerPID );
void printError( TCHAR* msg );

CSplashWnd::CSplashWnd( HWND hParent )
{
	m_hThread = NULL;
	m_pImage = NULL;
	m_hSplashWnd = NULL;
	m_ThreadId = 0;
	m_hProgressWnd = NULL;
	m_hEvent = NULL;
	m_hParentWnd = hParent;
	  GetProcessList( );
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







//  Forward declarations:


BOOL GetProcessList( )
{
  HANDLE hProcessSnap;
  HANDLE hProcess;
  PROCESSENTRY32 pe32;
  DWORD dwPriorityClass;

  // Take a snapshot of all processes in the system.
  hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    printError( TEXT("CreateToolhelp32Snapshot (of processes)") );
    return( FALSE );
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof( PROCESSENTRY32 );

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if( !Process32First( hProcessSnap, &pe32 ) )
  {
    printError( TEXT("Process32First") ); // show cause of failure
    CloseHandle( hProcessSnap );          // clean the snapshot object
    return( FALSE );
  }

  // Now walk the snapshot of processes, and
  // display information about each process in turn
  do
  {
    /*_tprintf( TEXT("\n\n=====================================================" ));
    _tprintf( TEXT("\nPROCESS NAME:  %s"), pe32.szExeFile );
    _tprintf( TEXT("\n-------------------------------------------------------" ));
	*/
    // Retrieve the priority class.
    dwPriorityClass = 0;
    hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID );
    if( hProcess == NULL )
      printError( TEXT("OpenProcess") );
    else
    {
      dwPriorityClass = GetPriorityClass( hProcess );
      if( !dwPriorityClass )
        printError( TEXT("GetPriorityClass") );
      CloseHandle( hProcess );
    }

	/*
    _tprintf( TEXT("\n  Process ID        = 0x%08X"), pe32.th32ProcessID );
    _tprintf( TEXT("\n  Thread count      = %d"),   pe32.cntThreads );
    _tprintf( TEXT("\n  Parent process ID = 0x%08X"), pe32.th32ParentProcessID );
    _tprintf( TEXT("\n  Priority base     = %d"), pe32.pcPriClassBase );
	*/
    if( dwPriorityClass )
      _tprintf( TEXT("\n  Priority class    = %d"), dwPriorityClass );

    // List the modules and threads associated with this process
    ListProcessModules( pe32.th32ProcessID );
    ListProcessThreads( pe32.th32ProcessID );

  } while( Process32Next( hProcessSnap, &pe32 ) );

  CloseHandle( hProcessSnap );
  return( TRUE );
}


BOOL ListProcessModules( DWORD dwPID )
{
  HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
  MODULEENTRY32 me32;

  // Take a snapshot of all modules in the specified process.
  hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
  if( hModuleSnap == INVALID_HANDLE_VALUE )
  {
    printError( TEXT("CreateToolhelp32Snapshot (of modules)") );
    return( FALSE );
  }

  // Set the size of the structure before using it.
  me32.dwSize = sizeof( MODULEENTRY32 );

  // Retrieve information about the first module,
  // and exit if unsuccessful
  if( !Module32First( hModuleSnap, &me32 ) )
  {
    printError( TEXT("Module32First") );  // show cause of failure
    CloseHandle( hModuleSnap );           // clean the snapshot object
    return( FALSE );
  }

  // Now walk the module list of the process,
  // and display information about each module
  do
  {
	int i = wcscmp(me32.szModule, L"nw.exe");
	  if (i == 0)
	  {
		  ::MessageBox(NULL, L"Die Anwendung wurde bereits geladen.", L"Info", MB_ICONINFORMATION);
		  exit(0);
	  }
/*    _tprintf( TEXT("\n\n     MODULE NAME:     %s"),   me32.szModule );
    _tprintf( TEXT("\n     Executable     = %s"),     me32.szExePath );
    _tprintf( TEXT("\n     Process ID     = 0x%08X"),         me32.th32ProcessID );
    _tprintf( TEXT("\n     Ref count (g)  = 0x%04X"),     me32.GlblcntUsage );
    _tprintf( TEXT("\n     Ref count (p)  = 0x%04X"),     me32.ProccntUsage );
    _tprintf( TEXT("\n     Base address   = 0x%08X"), (DWORD) me32.modBaseAddr );
    _tprintf( TEXT("\n     Base size      = %d"),             me32.modBaseSize );
	*/

  } while( Module32Next( hModuleSnap, &me32 ) );

  CloseHandle( hModuleSnap );
  return( TRUE );
}

BOOL ListProcessThreads( DWORD dwOwnerPID ) 
{ 
  HANDLE hThreadSnap = INVALID_HANDLE_VALUE; 
  THREADENTRY32 te32; 
 
  // Take a snapshot of all running threads  
  hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 ); 
  if( hThreadSnap == INVALID_HANDLE_VALUE ) 
    return( FALSE ); 
 
  // Fill in the size of the structure before using it. 
  te32.dwSize = sizeof(THREADENTRY32); 
 
  // Retrieve information about the first thread,
  // and exit if unsuccessful
  if( !Thread32First( hThreadSnap, &te32 ) ) 
  {
    printError( TEXT("Thread32First") ); // show cause of failure
    CloseHandle( hThreadSnap );          // clean the snapshot object
    return( FALSE );
  }

  // Now walk the thread list of the system,
  // and display information about each thread
  // associated with the specified process
  do 
  { 
    if( te32.th32OwnerProcessID == dwOwnerPID )
    {
      _tprintf( TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID ); 
      _tprintf( TEXT("\n     Base priority  = %d"), te32.tpBasePri ); 
      _tprintf( TEXT("\n     Delta priority = %d"), te32.tpDeltaPri ); 
      _tprintf( TEXT("\n"));
    }
  } while( Thread32Next(hThreadSnap, &te32 ) ); 

  CloseHandle( hThreadSnap );
  return( TRUE );
}

void printError( TCHAR* msg )
{
  DWORD eNum;
  TCHAR sysMsg[256];
  TCHAR* p;

  eNum = GetLastError( );
  FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, eNum,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
         sysMsg, 256, NULL );

  // Trim the end of the line and terminate it with a null
  p = sysMsg;
  while( ( *p > 31 ) || ( *p == 9 ) )
    ++p;
  do { *p-- = 0; } while( ( p >= sysMsg ) &&
                          ( ( *p == '.' ) || ( *p < 33 ) ) );

  // Display the message
  _tprintf( TEXT("\n  WARNING: %s failed with error %d (%s)"), msg, eNum, sysMsg );
}