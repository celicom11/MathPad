#include "stdafx.h"
#include "MathPad.h"
#include "mainfrm.h"


// Globals

PCWSTR _szFileFilters =
L"All Files (*.*)\0*.*\0"
L"Text Files (*.txt)\0*.txt\0"
L"MathPad Files (*.mth)\0*.mth\0"
L"";

CAppModule _Module;
int CMTPadApp::Run(LPTSTR lpstrCmdLine, int nCmdShow)
{
   lpstrCmdLine;
   m_pWndFrame = new CMainFrame();
   if (!m_pWndFrame || m_pWndFrame->CreateEx() == NULL)
   {
      ATLTRACE(_T("CMainFrame creation failed!\n"));
      return 0;
   }
   m_pWndFrame->ShowWindow(nCmdShow);

   int nRet = CMessageLoop::Run();
   delete m_pWndFrame; m_pWndFrame = nullptr;
   return nRet;
}

BOOL CMTPadApp::PreTranslateMessage(MSG* pMsg){
   return m_pWndFrame ? m_pWndFrame->PreTranslateMessage(pMsg) : FALSE;
}

BOOL CMTPadApp::OnIdle(int) {
   if (m_pWndFrame)
      m_pWndFrame->UpdateUIAll();
   return FALSE;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int nCmdShow)
{
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);// | _CRTDBG_CHECK_ALWAYS_DF |_CRTDBG_DELAY_FREE_MEM_DF);
   HRESULT hRes = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
   ATLASSERT(SUCCEEDED(hRes));
   // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
   ::DefWindowProc(HWND(0), 0, 0, 0L);
   HINSTANCE hInstRich = ::LoadLibrary(CRichEditCtrl::GetLibraryName());
   hRes = _Module.Init(NULL, hInstance);
   ATLASSERT(SUCCEEDED(hRes));
   CMTPadApp theApp;
   _Module.AddMessageLoop(&theApp);
   int nRet = theApp.Run(lpCmdLine, nCmdShow);
   //CLEANUP/DELETE MainFrame
   _Module.RemoveMessageLoop();

   _Module.Term();
   ::FreeLibrary(hInstRich);
   ::CoUninitialize();


   return nRet;
}
