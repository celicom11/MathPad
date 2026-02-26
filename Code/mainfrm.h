#pragma once
#include "MathBoxHost\D2DRenderer.h"
#include "MathBoxHost\D2DFontManager.h"
#include "MathBoxHost\MathBoxHost.h"
#include "resource.h"
#include "finddlg.h"
#include "aboutdlg.h"
#include "EditView.h"
#include "MathBoxView.h"
#include "CfgReader.h"
#include "ConfigMRUList.h"
#include "SettingsDlg.h"

#define FILE_MENU_POSITION	0
#define RECENT_MENU_POSITION	10
#define CFGFILENAME   L"MathPad.cfg"
namespace {
   uint32_t _CountCRs(PCSTR szText, uint32_t nLen){
      uint32_t nRet = 0;
      for (uint32_t nIdx = 0; nIdx + 1 < nLen; ++nIdx) {
         if (szText[nIdx] == '\r')
            ++nRet;
      }
      return nRet;
   };
}
class CMainFrame :
   public CFrameWindowImpl<CMainFrame>,
   public CUpdateUI<CMainFrame>,
   public IFontProvider_ {
public:
   DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME);

   bool                    m_bModified{ false };
   bool                    m_bMathBoxView{ false }; //current view

   HWND                    m_hWndOldClient{ NULL };
   CCfgReader              m_CfgReader;

   //MathBox resources
   CD2DRenderer            m_d2dr;
   CD2DFontManager         m_D2DFontManager;

   CCommandBarCtrl         m_CmdBar;
   CConfigMRUList          m_mru;
   CMultiPaneStatusBarCtrl m_sbar;
   CEditView               m_EditView;
   CMathBoxView            m_MathBoxView;

   wstring                 m_sPrinterName;
   wstring                 m_sAppDir;
   CPrinterT<true>         m_printer;
   CDevModeT<true>         m_devmode;

   CMainFrame() : m_d2dr(*this), m_MathBoxView(m_CfgReader, m_D2DFontManager, m_d2dr)
   {
      m_printer.OpenDefaultPrinter();
      m_devmode.CopyFromPrinter(m_printer);
   }
//ATTS
   float GetFontSizePts() const { return m_MathBoxView.FontSizePts(); }

   BOOL PreTranslateMessage(MSG* pMsg)
   {
      if (m_bMathBoxView) {
         if (m_MathBoxView.PreTranslateMessage(pMsg))
            return TRUE;
      }
      else 
         if (m_EditView.PreTranslateMessage(pMsg))
         return TRUE;

      return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
   }

   BEGIN_MSG_MAP(CMainFrame)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_CLOSE, OnClose)
      MESSAGE_HANDLER(WM_UPDATEROWCOL, OnUpdateRowCol)
      COMMAND_ID_HANDLER(ID_FILE_PRINT, OnFilePrint)
      COMMAND_ID_HANDLER(ID_FILE_PAGE_SETUP, OnFilePageSetup)
      COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
      COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
      COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
      COMMAND_ID_HANDLER(ID_FILE_SAVE_AS, OnFileSaveAs)
      COMMAND_RANGE_HANDLER(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnFileRecent)
      COMMAND_ID_HANDLER(ID_FILE_NEW_WINDOW, OnFileNewWindow)
      COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
      //COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
      //COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
      COMMAND_ID_HANDLER(ID_VIEW_MATH, OnViewMath)
      COMMAND_ID_HANDLER(ID_VIEW_SETTINGS, OnViewSettings)
      COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
      MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
      CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
      CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
      if (m_bMathBoxView) {
         CHAIN_COMMANDS_MEMBER((m_MathBoxView))
      }
      else {
         CHAIN_COMMANDS_MEMBER((m_EditView))
      }
   END_MSG_MAP()

   BEGIN_UPDATE_UI_MAP(CMainFrame)
      UPDATE_ELEMENT(ID_FILE_PAGE_SETUP, UPDUI_MENUPOPUP )
      UPDATE_ELEMENT(ID_FILE_PRINT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_UNDO, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_CUT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_COPY, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_PASTE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_CLEAR, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_SELECT_ALL, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_FIND, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_REPEAT, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_REPLACE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      UPDATE_ELEMENT(ID_EDIT_WORD_WRAP, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_MATH, UPDUI_MENUPOPUP)
      //UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
      //UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
   END_UPDATE_UI_MAP()

   LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
      // create command bar window
      HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
      // atach menu
      m_CmdBar.AttachMenu(GetMenu());
      // load command bar images
      m_CmdBar.LoadImages(IDR_MAINFRAME);
      // remove old menu
      SetMenu(NULL);

      HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);

      CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
      AddSimpleReBarBand(hWndCmdBar);
      AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

      CreateSimpleStatusBar();
      m_sbar.SubclassWindow(m_hWndStatusBar);
      int arrParts[] =
      {
         ID_DEFAULT_PANE,
         ID_ROW_PANE,
         ID_COL_PANE
      };
      m_sbar.SetPanes(arrParts, sizeof(arrParts) / sizeof(int), false);
      // MATHBOX VIEW
      m_MathBoxView.Create(m_hWnd, rcDefault, NULL,
         WS_CHILD | WS_VSCROLL | WS_HSCROLL,
         WS_EX_CLIENTEDGE); //hidden initially
      //initialize D2D renderer and font manager
      HRESULT hRes = m_d2dr.Initialize((HWND)m_MathBoxView);
      _ASSERT_RET(hRes >= S_OK, -1); //snbh
      WCHAR wszDir[MAX_PATH] = { 0 };
      ::GetCurrentDirectoryW(_countof(wszDir), wszDir);
      m_sAppDir = wszDir;
      if (m_sAppDir.back() != L'\\')
         m_sAppDir.append(1, L'\\');
      wstring sLMFontsDir = m_sAppDir + L"LatinModernFonts\\";
      hRes = m_D2DFontManager.Init(sLMFontsDir, m_d2dr.DWFactory());
      if (FAILED(hRes)) {
         CWindow::MessageBox(L"Could not load Latin Modern fonts", L"Error", MB_OK | MB_ICONERROR);
         return -1;
      }
      wstring sErrorMsg;
      if (!m_CfgReader.Init((m_sAppDir + CFGFILENAME).c_str())) {
         sErrorMsg = L"Could not read MathBox.cfg";
         CWindow::MessageBox(sErrorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
         return -1;
      }

      if(!m_MathBoxView.InitMathBox(sErrorMsg)) {
         CWindow::MessageBox(sErrorMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
         return -1;
      }
      // EDIT VIEW
      m_hWndClient = m_EditView.Create(m_hWnd, rcDefault, NULL,
         WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
         ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL
         | ES_SAVESEL | ES_SELECTIONBAR,
         WS_EX_CLIENTEDGE);

      m_EditView.InitView(m_CfgReader);
      UIAddToolBar(hWndToolBar);

      HMENU hMenu = m_CmdBar.GetMenu();
      HMENU hFileMenu = ::GetSubMenu(hMenu, FILE_MENU_POSITION);
      HMENU hMruMenu = ::GetSubMenu(hFileMenu, RECENT_MENU_POSITION);
      m_mru.SetMenuHandle(hMruMenu);
      m_mru.ReadFromConfig(m_CfgReader);
      //update row and col indicators
      BOOL bDummy;
      OnUpdateRowCol(WM_UPDATEROWCOL, 0, 0, bDummy);
      //AutoLoad last opened file
      bool bOpenLastDoc = false;
      m_CfgReader.GetBVal(L"EditOpenLastDoc", bOpenLastDoc);
      if (bOpenLastDoc) {
         TCHAR szFile[MAX_PATH];
         if (m_mru.GetFromList(ID_FILE_MRU_FIRST, szFile, MAX_PATH))
         {
            // find file name without the path
            LPTSTR lpstrFileName = szFile;
            for (int i = lstrlen(szFile) - 1; i >= 0; i--)
            {
               if (szFile[i] == '\\' || szFile[i] == '/')
               {
                  lpstrFileName = &szFile[i + 1];
                  break;
               }
            }
            // open file
            if (!_FileExists(szFile) || !DoFileOpen(szFile, lpstrFileName))
               DoFileNew();
         }
      }
      return 0;
   }

   LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
   {
      // Save MRU to config before closing
      m_mru.WriteToConfig(m_sAppDir + CFGFILENAME);
      
      bHandled = !m_EditView.QueryClose();
      return 0;
   }

   LRESULT OnUpdateRowCol(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      TCHAR szBuff[100];
      wsprintf(szBuff, _T("Row: %i"), m_EditView.m_nRow + 1);
      m_sbar.SetPaneText(ID_ROW_PANE, szBuff);
      wsprintf(szBuff, _T("Col: %i"), m_EditView.m_nCol + 1);
      m_sbar.SetPaneText(ID_COL_PANE, szBuff);
      return 0;
   }

   LRESULT UpdateTitle()
   {
      int nAppendLen = lstrlen(m_EditView.m_strFileName);

      TCHAR strTitleBase[256];
      ::LoadString(_Module.GetResourceInstance(), IDR_MAINFRAME, strTitleBase, 255);
      int nBaseLen = lstrlen(strTitleBase);

      // 1 == "*" (optional), 3 == " - ", 1 == null termination
      LPTSTR lpstrFullTitle = (LPTSTR)_alloca((nAppendLen + nBaseLen + 1 + 3 + 1) * sizeof(TCHAR));

      lstrcpy(lpstrFullTitle, m_EditView.m_strFileName);
      if (m_bModified)
         lstrcat(lpstrFullTitle, _T("*"));
      lstrcat(lpstrFullTitle, _T(" - "));
      lstrcat(lpstrFullTitle, strTitleBase);

      SetWindowText(lpstrFullTitle);

      return 0;
   }

   LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      DoFileNew();
      return 0;
   }

   void DoFileNew()
   {
      if (m_EditView.QueryClose())
      {
         m_EditView.SetWindowText(NULL);
         m_EditView.OnFileOpened(_T(""), _T("Untitled"));
         UpdateTitle();
      }
   }

   LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CFileDialog dlg(TRUE, NULL, _T(""), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _szFileFilters);
      int nRet = (int)dlg.DoModal();

      if (nRet == IDOK)
      {
         ATLTRACE(_T("File path: %s\n"), dlg.m_ofn.lpstrFile);
         BOOL bRet = m_EditView.QueryClose();
         if (!bRet)
         {
            if (!DoFileSaveAs())
               return 0;
         }

         if (DoFileOpen(dlg.m_ofn.lpstrFile, dlg.m_ofn.lpstrFileTitle))
         {
            m_mru.AddToList(dlg.m_ofn.lpstrFile);
         }
      }

      return 0;
   }

   BOOL DoFileOpen(LPCTSTR lpstrFileName, LPCTSTR lpstrFileTitle)
   {
      BOOL bRet = m_EditView.LoadFile(lpstrFileName);
      if (bRet)
      {
         m_EditView.OnFileOpened(lpstrFileName, lpstrFileTitle);
         UpdateTitle();
         m_mru.AddToList(lpstrFileName);
      }
      else
         MessageBox(_T("Error reading file!\n"));

      return bRet;
   }

   LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if (m_EditView.m_strFilePath[0] != 0) {
         if (m_EditView.SaveFile(m_EditView.m_strFilePath))
            m_mru.AddToList(m_EditView.m_strFilePath);
         else
            MessageBox(_T("Error writing file!\n"));
      }
      else if (m_EditView.DoFileSaveAs()) {
         UpdateTitle();
         m_mru.AddToList(m_EditView.m_strFilePath);
      }

      return 0;
   }

   LRESULT OnFileSaveAs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if (m_EditView.DoFileSaveAs()) {
         UpdateTitle();
         m_mru.AddToList(m_EditView.m_strFilePath);
      }
      return 0;
   }

   BOOL DoFileSaveAs()
   {
      BOOL bRet = FALSE;
      CFileDialog dlg(FALSE);
      int nRet = (int)dlg.DoModal();

      if (nRet == IDOK)
      {
         ATLTRACE(_T("File path: %s\n"), dlg.m_ofn.lpstrFile);
         bRet = m_EditView.SaveFile(dlg.m_ofn.lpstrFile);
         if (bRet)
         {
            m_EditView.OnFileOpened(dlg.m_ofn.lpstrFile, dlg.m_ofn.lpstrFileTitle);
            UpdateTitle();
         }
         else
            MessageBox(_T("Error writing file!\n"));
      }

      return bRet;
   }

   LRESULT OnFileRecent(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      // check if we have to save the current one
      if (!m_EditView.QueryClose())
      {
         if (!m_EditView.DoFileSaveAs())
            return 0;
      }
      // get file name from the MRU list
      TCHAR szFile[MAX_PATH];
      if (m_mru.GetFromList(wID, szFile, MAX_PATH))
      {
         // find file name without the path
         LPTSTR lpstrFileName = szFile;
         for (int i = lstrlen(szFile) - 1; i >= 0; i--)
         {
            if (szFile[i] == '\\' || szFile[i] == '/')
            {
               lpstrFileName = &szFile[i + 1];
               break;
            }
         }
         
         // Check if file exists
         if (_FileExists(szFile))
         {
            // File exists - open it
            if (DoFileOpen(szFile, lpstrFileName))
            {
               m_mru.MoveToTop(wID);
            }
         }
         else
         {
            // File does not exist - ask user what to do
            TCHAR szMsg[MAX_PATH + 100];
            wsprintf(szMsg, 
                     _T("The file does not exist:\n\n%s\n\nDo you want to remove it from the recent files list?"), 
                     szFile);
            
            int nResult = MessageBox(szMsg, _T("File Not Found"), MB_YESNO | MB_ICONQUESTION);
            
            if (nResult == IDYES)
            {
               // User wants to remove it
               m_mru.RemoveFromList(wID);
            }
            // If IDNO, keep it in the list (do nothing)
         }
      }
      else
      {
         ::MessageBeep(MB_ICONERROR);
      }

      return 0;
   }

   LRESULT OnFileNewWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      ::PostThreadMessage(_Module.m_dwMainThreadID, WM_USER, 0, 0L);
      return 0;
   }

   LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      PostMessage(WM_CLOSE);
      return 0;
   }

   LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      static BOOL bNew = TRUE;	// initially visible
      bNew = !bNew;
      ::SendMessage(m_hWndToolBar, RB_SHOWBAND, 1, bNew); // toolbar is band #1
      UISetCheck(ID_VIEW_TOOLBAR, bNew);
      UpdateLayout();
      return 0;
   }
   LRESULT OnViewSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CSettingsDlg dlg(NULL, m_CfgReader, m_mru);
      if (dlg.DoModal() == IDOK) {
         if (!dlg.WriteConfig()) {
            MessageBox(L"Error writing configuration!", L"Error", MB_OK | MB_ICONERROR);
            return 0;
         }
         //reset CfrgReader
         m_CfgReader.Init(CFGFILENAME);
         m_EditView.InitView(m_CfgReader);
         
         //re-parse LaTeX from edit control to reflect settings change
         int nLength = m_EditView.GetWindowTextLength();
         vector<char> vLaTex(nLength + 1, '\0');
         ::GetWindowTextA(m_EditView, vLaTex.data(), nLength + 1);
         if ('\0' != vLaTex[0])
            m_MathBoxView.SetMathContent(vLaTex.data());
      }
      return 0;
   }
   LRESULT OnViewMath(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      if (!m_bMathBoxView) {
         //re-parse LaTeX from edit control
         int nLength = m_EditView.GetWindowTextLength();
         vector<char> vLaTex(nLength + 1, '\0');
         ::GetWindowTextA(m_EditView, vLaTex.data(), nLength + 1);
         if ('\0' == vLaTex[0]) {
            //empty content/nothing to show
            MessageBeep(MB_ICONERROR);
            return 0; 
         }
         //clear error if any
         m_EditView.ClearErrorHighlight();
         SetStatusTextA("");
         if (!m_MathBoxView.SetMathContent(vLaTex.data())) {
            const SMathBoxError& err = m_MathBoxView.LastError();
            // 1. Show first line in status bar
            string sFirstLine = err.sError.substr(0, err.sError.find('\n'));
            SetStatusTextA(sFirstLine.c_str());
            // 2. Highlight error region in edit view
            // Convert parser byte offsets (\r\n=2) to RichEdit char offsets (\r=1)
            // by subtracting the count of \r\n pairs before each position.
            uint32_t nErrStart = err.nErrorStartPos - _CountCRs(vLaTex.data(), err.nErrorStartPos);
            uint32_t nErrEnd = err.nErrorEndPos - _CountCRs(vLaTex.data(), err.nErrorEndPos);
            m_EditView.HighlightError(nErrStart, nErrEnd, err.sError);
            MessageBeep(MB_ICONERROR);
            return 0;
         }
         //else
         m_bMathBoxView = !m_bMathBoxView;
         m_MathBoxView.ShowWindow(SW_SHOW);
         m_EditView.ShowWindow(SW_HIDE);
         m_hWndClient = (HWND)m_MathBoxView;
      }
      else {
         m_bMathBoxView = !m_bMathBoxView;
         m_EditView.ShowWindow(SW_SHOW);
         m_MathBoxView.ShowWindow(SW_HIDE);
         m_hWndClient = (HWND)m_EditView;
      }
      ::SetFocus(m_hWndClient);
      UpdateLayout();

      return 0;
   }
   LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      BOOL bNew = !::IsWindowVisible(m_hWndStatusBar);
      ::ShowWindow(m_hWndStatusBar, bNew ? SW_SHOWNOACTIVATE : SW_HIDE);
      UISetCheck(ID_VIEW_STATUS_BAR, bNew);
      UpdateLayout();
      return 0;
   }

   LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CAboutDlg dlg(m_MathBoxView.MathBoxAbiVersion());
      dlg.DoModal();
      return 0;
   }

   LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
   {
      if ((HWND)wParam == m_EditView.m_hWnd)
      {
         CMenu menuContext;
         menuContext.LoadMenu(IDR_CONTEXTMENU);
         CMenuHandle menuPopup(menuContext.GetSubMenu(0));
         m_CmdBar.TrackPopupMenu(menuPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON, LOWORD(lParam), HIWORD(lParam));
      }
      else
      {
         bHandled = FALSE;
      }

      return 0;
   }
   LRESULT OnFilePageSetup(WORD, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
      CPageSetupDialog dlg(PSD_MARGINS | PSD_INTHOUSANDTHSOFINCHES);
      //test: set paper size to A4
      m_devmode.m_pDevMode->dmFields |= DM_PAPERSIZE;
      m_devmode.m_pDevMode->dmPaperSize = DMPAPER_A4;
      dlg.m_psd.hDevMode = m_devmode.CopyToHDEVMODE();
      dlg.m_psd.hDevNames = m_printer.CopyToHDEVNAMES();
      dlg.m_psd.rtMargin = m_MathBoxView.GetPrintPageMargins();

      if (dlg.DoModal() == IDOK)
      {
         m_devmode.CopyFromHDEVMODE(dlg.m_psd.hDevMode);
         m_printer.ClosePrinter();
         m_printer.OpenPrinter(dlg.m_psd.hDevNames, m_devmode.m_pDevMode);
         m_MathBoxView.SetPrintPageDimensions(dlg.m_psd.rtMargin, dlg.m_psd.ptPaperSize);
      }

      GlobalFree(dlg.m_psd.hDevMode);
      GlobalFree(dlg.m_psd.hDevNames);

      return 0;
   }
   LRESULT OnFilePrint(WORD, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
      UINT nPageCount = m_MathBoxView.PageCount();

      CPrintDialog dlg(FALSE);
      dlg.m_pd.hDevMode = m_devmode.CopyToHDEVMODE();
      dlg.m_pd.hDevNames = m_printer.CopyToHDEVNAMES();
      dlg.m_pd.nMinPage = 1;
      dlg.m_pd.nMaxPage = (WORD)nPageCount;
      dlg.m_pd.nFromPage = 1;
      dlg.m_pd.nToPage = (WORD)nPageCount;
      dlg.m_pd.Flags &= ~PD_NOPAGENUMS;

      if (dlg.DoModal() == IDOK)
      {
         m_devmode.CopyFromHDEVMODE(dlg.m_pd.hDevMode);
         m_printer.ClosePrinter();
         m_printer.OpenPrinter(dlg.m_pd.hDevNames, m_devmode.m_pDevMode);
         m_sPrinterName = dlg.GetDeviceName();
         UINT nMin = 0;
         UINT nMax = nPageCount - 1;
         if (dlg.PrintRange() != FALSE)
         {
            nMin = dlg.m_pd.nFromPage - 1;
            nMax = dlg.m_pd.nToPage - 1;
         }
         // Begin print job in D2D
         if(!m_MathBoxView.BeginPrintJob(m_sPrinterName)) {
            MessageBox(L"Failed to initialize D2D printing", L"Error", MB_OK | MB_ICONERROR);
            // ALWAYS End print job
            m_MathBoxView.EndPrintJob();
            GlobalFree(dlg.m_pd.hDevMode);
            GlobalFree(dlg.m_pd.hDevNames);
            return 0;
         }
         // Print each page
         bool bSuccess = true;
         for (UINT nPage = nMin; nPage <= nMax && bSuccess; ++nPage) {
            bSuccess = m_MathBoxView.PrintPage(nPage);  // HDC not needed anymore!
         }
         // ALWAYS End print job
         m_MathBoxView.EndPrintJob();

         // Show result
         if (!bSuccess)
            MessageBox(L"Printing failed", L"Error", MB_OK | MB_ICONERROR);

      }

      GlobalFree(dlg.m_pd.hDevMode);
      GlobalFree(dlg.m_pd.hDevNames);

      return 0;
   }

//IFontProvider_ Impl
   uint32_t FontCount() const override { return m_D2DFontManager.FontCount(); }
   IDWriteFontFace* GetDWFont(int32_t nIdx) const override {
      return m_D2DFontManager.GetDWFont(nIdx);
   }
//UI UPDATE
   void UpdateUIAll()
   {
      bool bModified = !!m_EditView.GetModify();
      if (bModified != m_bModified)
      {
         m_bModified = bModified;
         UpdateTitle();
      }

      UIEnable(ID_EDIT_UNDO, m_EditView.CanUndo());
      UIEnable(ID_EDIT_PASTE, m_EditView.CanPaste(CF_TEXT));

      BOOL bSel = (m_EditView.GetSelectionType() != SEL_EMPTY);

      UIEnable(ID_EDIT_CUT, bSel);
      UIEnable(ID_EDIT_COPY, bSel);
      UIEnable(ID_EDIT_CLEAR, bSel);

      BOOL bNotEmpty = (m_EditView.GetWindowTextLength() > 0);
      UIEnable(ID_EDIT_SELECT_ALL, bNotEmpty);
      UIEnable(ID_EDIT_FIND, bNotEmpty);
      UIEnable(ID_EDIT_REPEAT, bNotEmpty);
      UIEnable(ID_EDIT_REPLACE, bNotEmpty);
      UIEnable(ID_FILE_PAGE_SETUP, !m_bMathBoxView);
      UIEnable(ID_FILE_PRINT, m_bMathBoxView);

      UISetCheck(ID_EDIT_WORD_WRAP, m_EditView.m_bWordWrap);
      UISetCheck(ID_VIEW_MATH, m_bMathBoxView);

      UIUpdateToolBar();
   }
   void SetStatusText(const wstring& sText) {
      if(m_hWndStatusBar)
         ::SetWindowTextW(m_hWndStatusBar, sText.c_str());
   }
   void SetStatusTextA(const string& sText) {
      if (m_hWndStatusBar)
         ::SetWindowTextA(m_hWndStatusBar, sText.c_str());
   }

private:
   static bool _FileExists(PCWSTR szPath) {
      return ::GetFileAttributesW(szPath) != INVALID_FILE_ATTRIBUTES;
   }
};
