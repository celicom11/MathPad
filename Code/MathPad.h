#pragma once
class CMainFrame; //forward

class CMTPadApp : public CMessageLoop {
// DATA
   CMainFrame* m_pWndFrame{ nullptr };
public:
// METHODS
   int Run(LPTSTR lpstrCmdLine, int nCmdShow);
// CMessageLoop
   BOOL PreTranslateMessage(MSG* pMsg) override;
   BOOL OnIdle(int) override;
};
