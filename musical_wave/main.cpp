// App.cpp : Defines the entry point for the application.
//

#include "main_dlg.hpp"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int nCmdShow)
{
	CPaintManagerUI::SetInstance(hInstance);
	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

	HRESULT Hr = ::CoInitialize(NULL);
	if (FAILED(Hr)) return 0;

	CWndShadow::Initialize(hInstance);

	CFrameWindowWnd* pFrame = new CFrameWindowWnd();
	if (pFrame == NULL) return 0;
	pFrame->Create(NULL, _T("wave生成器"), UI_WNDSTYLE_FRAME | WS_CLIPCHILDREN, WS_EX_WINDOWEDGE);
	pFrame->CenterWindow();
	pFrame->ShowWindow(true);
	CPaintManagerUI::MessageLoop();

	::CoUninitialize();
	return 0;
}
