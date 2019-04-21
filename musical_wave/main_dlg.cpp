#include "main_dlg.hpp"
#include <commdlg.h>
#include "wave_data.h"

CDwm::CDwm()
{
	static HINSTANCE hDwmInstance = ::LoadLibrary(_T("dwmapi.dll"));
	if (hDwmInstance != NULL) {
		fnDwmEnableComposition = (FNDWMENABLECOMPOSITION) ::GetProcAddress(hDwmInstance, "DwmEnableComposition");
		fnDwmIsCompositionEnabled = (FNDWNISCOMPOSITIONENABLED) ::GetProcAddress(hDwmInstance, "DwmIsCompositionEnabled");
		fnDwmEnableBlurBehindWindow = (FNENABLEBLURBEHINDWINDOW) ::GetProcAddress(hDwmInstance, "DwmEnableBlurBehindWindow");
		fnDwmExtendFrameIntoClientArea = (FNDWMEXTENDFRAMEINTOCLIENTAREA) ::GetProcAddress(hDwmInstance, "DwmExtendFrameIntoClientArea");
		fnDwmSetWindowAttribute = (FNDWMSETWINDOWATTRIBUTE) ::GetProcAddress(hDwmInstance, "DwmSetWindowAttribute");
	}
	else {
		fnDwmEnableComposition = NULL;
		fnDwmIsCompositionEnabled = NULL;
		fnDwmEnableBlurBehindWindow = NULL;
		fnDwmExtendFrameIntoClientArea = NULL;
		fnDwmSetWindowAttribute = NULL;
	}
}

BOOL CDwm::IsCompositionEnabled() const
{
	HRESULT Hr = E_NOTIMPL;
	BOOL bRes = FALSE;
	if (fnDwmIsCompositionEnabled != NULL) Hr = fnDwmIsCompositionEnabled(&bRes);
	return SUCCEEDED(Hr) && bRes;
}

BOOL CDwm::EnableComposition(UINT fEnable)
{
	BOOL bRes = FALSE;
	if (fnDwmEnableComposition != NULL) bRes = SUCCEEDED(fnDwmEnableComposition(fEnable));
	return bRes;
}

BOOL CDwm::EnableBlurBehindWindow(HWND hWnd)
{
	BOOL bRes = FALSE;
	if (fnDwmEnableBlurBehindWindow != NULL) {
		DWM_BLURBEHIND bb = { 0 };
		bb.dwFlags = DWM_BB_ENABLE;
		bb.fEnable = TRUE;
		bRes = SUCCEEDED(fnDwmEnableBlurBehindWindow(hWnd, &bb));
	}
	return bRes;
}

BOOL CDwm::EnableBlurBehindWindow(HWND hWnd, const DWM_BLURBEHIND & bb)
{
	BOOL bRes = FALSE;
	if (fnDwmEnableBlurBehindWindow != NULL) {
		bRes = SUCCEEDED(fnDwmEnableBlurBehindWindow(hWnd, &bb));
	}
	return bRes;
}

BOOL CDwm::ExtendFrameIntoClientArea(HWND hWnd, const DWM_MARGINS & Margins)
{
	BOOL bRes = FALSE;
	if (fnDwmEnableComposition != NULL) bRes = SUCCEEDED(fnDwmExtendFrameIntoClientArea(hWnd, &Margins));
	return bRes;
}

BOOL CDwm::SetWindowAttribute(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute)
{
	BOOL bRes = FALSE;
	if (fnDwmSetWindowAttribute != NULL) bRes = SUCCEEDED(fnDwmSetWindowAttribute(hwnd, dwAttribute, pvAttribute, cbAttribute));
	return bRes;
}

CDPI::CDPI() {
	m_nScaleFactor = 0;
	m_nScaleFactorSDA = 0;
	m_Awareness = PROCESS_DPI_UNAWARE;

	static HINSTANCE hUser32Instance = ::LoadLibrary(_T("User32.dll"));
	static HINSTANCE hShcoreInstance = ::LoadLibrary(_T("Shcore.dll"));
	if (hUser32Instance != NULL) {
		fnSetProcessDPIAware = (FNSETPROCESSDPIAWARE) ::GetProcAddress(hUser32Instance, "SetProcessDPIAware");
	}
	else {
		fnSetProcessDPIAware = NULL;
	}

	if (hShcoreInstance != NULL) {
		fnSetProcessDpiAwareness = (FNSETPROCESSDPIAWARENESS) ::GetProcAddress(hShcoreInstance, "SetProcessDpiAwareness");
		fnGetDpiForMonitor = (FNGETDPIFORMONITOR) ::GetProcAddress(hShcoreInstance, "GetDpiForMonitor");
	}
	else {
		fnSetProcessDpiAwareness = NULL;
		fnGetDpiForMonitor = NULL;
	}

	if (fnGetDpiForMonitor != NULL) {
		UINT     dpix = 0, dpiy = 0;
		HRESULT  hr = E_FAIL;
		POINT pt = { 1, 1 };
		HMONITOR hMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
		hr = fnGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy);
		SetScale(dpix);
	}
	else {
		UINT     dpix = 0;
		HDC hDC = ::GetDC(::GetDesktopWindow());
		dpix = GetDeviceCaps(hDC, LOGPIXELSX);
		::ReleaseDC(::GetDesktopWindow(), hDC);
		SetScale(dpix);
	}

	SetAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

PROCESS_DPI_AWARENESS CDPI::GetAwareness()
{
	return m_Awareness;
}

int CDPI::Scale(int x)
{
	if (m_Awareness == PROCESS_DPI_UNAWARE) return x;
	if (m_Awareness == PROCESS_SYSTEM_DPI_AWARE) return MulDiv(x, m_nScaleFactorSDA, 100);
	return MulDiv(x, m_nScaleFactor, 100); // PROCESS_PER_MONITOR_DPI_AWARE
}

UINT CDPI::GetScale()
{
	if (m_Awareness == PROCESS_DPI_UNAWARE) return 100;
	if (m_Awareness == PROCESS_SYSTEM_DPI_AWARE) return m_nScaleFactorSDA;
	return m_nScaleFactor;
}

void CDPI::ScaleRect(RECT * pRect)
{
	pRect->left = Scale(pRect->left);
	pRect->right = Scale(pRect->right);
	pRect->top = Scale(pRect->top);
	pRect->bottom = Scale(pRect->bottom);
}

void CDPI::ScalePoint(POINT * pPoint)
{
	pPoint->x = Scale(pPoint->x);
	pPoint->y = Scale(pPoint->y);
}

void CDPI::OnDPIChanged(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	SetScale(LOWORD(wParam));
	RECT* const prcNewWindow = (RECT*)lParam;
	::SetWindowPos(hWnd,
		NULL,
		prcNewWindow->left,
		prcNewWindow->top,
		prcNewWindow->right - prcNewWindow->left,
		prcNewWindow->bottom - prcNewWindow->top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL CDPI::SetAwareness(PROCESS_DPI_AWARENESS value)
{
	if (fnSetProcessDpiAwareness != NULL) {
		HRESULT Hr = E_NOTIMPL;
		Hr = fnSetProcessDpiAwareness(value);
		if (Hr == S_OK) {
			m_Awareness = value;
			return TRUE;
		}
		else {
			return FALSE;
		}
	}
	else {
		if (fnSetProcessDPIAware) {
			BOOL bRet = fnSetProcessDPIAware();
			if (bRet) m_Awareness = PROCESS_SYSTEM_DPI_AWARE;
			return bRet;
		}
	}
	return FALSE;
}

void CDPI::SetScale(UINT iDPI)
{
	m_nScaleFactor = MulDiv(iDPI, 100, 96);
	if (m_nScaleFactorSDA == 0) m_nScaleFactorSDA = m_nScaleFactor;
}

void CFrameWindowWnd::Init() { }

bool CFrameWindowWnd::OnHChanged(void * param) {
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == _T("valuechanged")) {
		short H, S, L;
		CPaintManagerUI::GetHSL(&H, &S, &L);
		CPaintManagerUI::SetHSL(true, (static_cast<CSliderUI*>(pMsg->pSender))->GetValue(), S, L);
	}
	return true;
}

bool CFrameWindowWnd::OnSChanged(void * param) {
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == _T("valuechanged")) {
		short H, S, L;
		CPaintManagerUI::GetHSL(&H, &S, &L);
		CPaintManagerUI::SetHSL(true, H, (static_cast<CSliderUI*>(pMsg->pSender))->GetValue(), L);
	}
	return true;
}

bool CFrameWindowWnd::OnLChanged(void * param) {
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == _T("valuechanged")) {
		short H, S, L;
		CPaintManagerUI::GetHSL(&H, &S, &L);
		CPaintManagerUI::SetHSL(true, H, S, (static_cast<CSliderUI*>(pMsg->pSender))->GetValue());
	}
	return true;
}

bool CFrameWindowWnd::OnAlphaChanged(void * param) {
	TNotifyUI* pMsg = (TNotifyUI*)param;
	if (pMsg->sType == _T("valuechanged")) {
		int alpha = (static_cast<CSliderUI*>(pMsg->pSender))->GetValue();
		if (100 > alpha)
		{
			alpha = 100;
			(static_cast<CSliderUI*>(pMsg->pSender))->SetValue(alpha);
		}
		m_pm.SetOpacity(alpha);
	}
	return true;
}

void CFrameWindowWnd::OnPrepare()
{
	CSliderUI* pSilder = static_cast<CSliderUI*>(m_pm.FindControl(_T("alpha_controlor")));
	if (pSilder) pSilder->OnNotify += MakeDelegate(this, &CFrameWindowWnd::OnAlphaChanged);
	pSilder = static_cast<CSliderUI*>(m_pm.FindControl(_T("h_controlor")));
	if (pSilder) pSilder->OnNotify += MakeDelegate(this, &CFrameWindowWnd::OnHChanged);
	pSilder = static_cast<CSliderUI*>(m_pm.FindControl(_T("s_controlor")));
	if (pSilder) pSilder->OnNotify += MakeDelegate(this, &CFrameWindowWnd::OnSChanged);
	pSilder = static_cast<CSliderUI*>(m_pm.FindControl(_T("l_controlor")));
	if (pSilder) pSilder->OnNotify += MakeDelegate(this, &CFrameWindowWnd::OnLChanged);
}

void CFrameWindowWnd::OnLoadFileSnapshot()
{
	/* 获取可执行文件目录*/
	CHAR exeFullPath[MAX_PATH];
	GetModuleFileNameA(NULL, exeFullPath, MAX_PATH);
	for (int i = strlen(exeFullPath) - 1; i >= 0; --i)
	{
		if ('\\' == exeFullPath[i] || '/' == exeFullPath[i])
		{
			exeFullPath[i] = 0;
			break;
		}
	}
	/* 打开选择文件对话框 */
	CHAR szBuffer[MAX_PATH] = { 0 };
	OPENFILENAMEA ofn = { 0 };
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hWnd;
	ofn.lpstrFilter = "txt文件(*.txt)\0*.txt\0\0";//要选择的文件后缀
	ofn.lpstrInitialDir = exeFullPath;//默认的文件路径"
	ofn.lpstrFile = szBuffer;//存放文件的缓冲区
	ofn.nMaxFile = MAX_PATH;
	ofn.nFilterIndex = 0;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;//标志如果是多选要加上OFN_ALLOWMULTISELECT
	BOOL bSel = GetOpenFileNameA(&ofn);
	if (bSel)
	{
        wave_data d;
        if (d.load_music_file(szBuffer))
        {
            int szLength = strlen(szBuffer) - 3;
            if (0 < szLength)
            {
                strcpy(szBuffer + szLength, "wav");
            }
            if (d.create_music_wave(szBuffer))
            {
                ::MessageBoxA(m_pWndShadow->GetHWND(), "乐谱文件转wave文件完成", "提示", 0);
            }
            else
                ::MessageBoxA(m_pWndShadow->GetHWND(), "创建wave文件失败", "错误", 0);
        }
        else
            ::MessageBoxA(m_pWndShadow->GetHWND(), "加载乐谱文件失败", "错误", 0);
	}
}

void CFrameWindowWnd::Notify(TNotifyUI & msg)
{
	if (msg.sType == _T("windowinit")) OnPrepare();
	else if (msg.sType == _T("click")) {
		if (msg.pSender->GetName() == _T("load_files_snapshot_bt")) 
		{
			/* 打开文件快照 */
			OnLoadFileSnapshot();
		}
		else if (msg.pSender->GetName() == _T("save_files_snapshot_bt")) 
		{
			::MessageBoxA(m_pWndShadow->GetHWND(), "", "", 0);
		}
	}
}

LRESULT CFrameWindowWnd::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CREATE) {
		m_pm.Init(m_hWnd);
		CDialogBuilder builder;
		CControlUI* pRoot = builder.Create(_T("ui.xml"), (UINT)0, NULL, &m_pm);
		ASSERT(pRoot && "Failed to parse XML");
		m_pm.AttachDialog(pRoot);
		m_pm.AddNotifier(this);

		m_pWndShadow = new CWndShadow;
		m_pWndShadow->Create(m_hWnd);
		RECT rcCorner = { 3,3,4,4 };
		RECT rcHoleOffset = { 0,0,0,0 };
		m_pWndShadow->SetImage(_T("LeftWithFill.png"), rcCorner, rcHoleOffset);

		DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
		SetWindowAttribute(m_hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &ncrp, sizeof(ncrp));

		//DWM_BLURBEHIND bb = {0};
		//bb.dwFlags = DWM_BB_ENABLE;
		//bb.fEnable = true;
		//bb.hRgnBlur = NULL;
		//EnableBlurBehindWindow(m_hWnd, bb);

		//DWM_MARGINS margins = {-1}/*{0,0,0,25}*/;
		//ExtendFrameIntoClientArea(m_hWnd, margins);

		Init();
		return 0;
	}
	else if (uMsg == WM_DESTROY) {
		::PostQuitMessage(0L);
	}
	else if (uMsg == WM_NCACTIVATE) {
		if (!::IsIconic(*this)) return (wParam == 0) ? TRUE : FALSE;
	}
	LRESULT lRes = 0;
	if (m_pm.MessageHandler(uMsg, wParam, lParam, lRes)) return lRes;
	return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}
