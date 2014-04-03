#ifndef __SPLASHWND_H_
#define __SPLASHWND_H_

class CSplashWnd
{
private:
	CSplashWnd(const CSplashWnd&) {};
	CSplashWnd& operator=(const CSplashWnd&) {};
protected:
	HANDLE							m_hThread;
	unsigned int				m_ThreadId;
	HANDLE							m_hEvent;

	Gdiplus::Image*			m_pImage;					
	HWND								m_hSplashWnd;	
	std::wstring				m_WindowName;			
	HWND								m_hProgressWnd;	
	HWND								m_hParentWnd;
	std::wstring				m_ProgressMsg;		
  UINT_PTR            m_TimerId;        

public:
	CSplashWnd( HWND hParent = NULL );
	~CSplashWnd();														

	void SetImage(Gdiplus::Image* pImage);		
  void SetImage(HMODULE hModule, UINT nResourceID);
	void SetWindowName(LPCWSTR windowName);		
	void Show();								
	void Hide();								
  /*!
  @param[in] procent 
  */
  void SetProgress(UINT procent);
	/*!
		@param[in] procent 
		@param[in] msg
	*/
	void SetProgress(UINT procent, const wchar_t* msg );
	/*! 
	@param[in] procent 
	@param[in] nResourceID 
	@param[in] hModule 
	*/
	void SetProgress(UINT procent, UINT nResourceID = 0, HMODULE hModule = NULL );

	/*!
			@param[in] from 
			@param[in] to 
			@param[in] seconds 
			*/
	void SetAutoProgress(UINT from, UINT to, UINT steps);
	void SetProgressBarColor(COLORREF color);	

	HWND GetWindowHwnd() const					
	{
		return m_hSplashWnd;
	};

protected:
	static LRESULT CALLBACK SplashWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static unsigned int __stdcall SplashThreadProc(void* lpParameter);
};

#endif//__SPLASHWND_H_
