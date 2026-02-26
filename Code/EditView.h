#pragma once
#include <textserv.h> // Required for ITextDocument
#include <tom.h>
#include "CfgReader.h"

#define RGB2BGR(nRGB)      ((nRGB&0x00FF00)|((nRGB&0x0000FF)<<16)|((nRGB&0xFF0000)>>16))
#define WM_UPDATEROWCOL    (WM_USER + 1000)
extern PCWSTR _szFileFilters;

class CEditView :
   public CWindowImpl<CEditView, CRichEditCtrl>,
   public CRichEditCommands<CEditView>,
   public CRichEditFindReplaceImpl<CEditView, CFindReplaceDialogWithMessageFilter>
{
protected:
   typedef CEditView thisClass;
   typedef CRichEditCommands<CEditView> editCommandsClass;
   typedef CRichEditFindReplaceImpl<CEditView, CFindReplaceDialogWithMessageFilter> findReplaceClass;

public:
   DECLARE_WND_SUPERCLASS(NULL, CRichEditCtrl::GetWndClassName())

   enum
   {
      cchTAB = 8,
      nMaxBufferLen = 4000000
   };

   bool           m_bWordWrap{ false };
   int            m_nRow{ 0 };
   int            m_nCol{ 0 };
   CRect          m_rcMargin{ 1000, 1000, 1000, 1000 };
   CFont          m_font;
   TCHAR          m_strFilePath[MAX_PATH]{ 0 };
   TCHAR          m_strFileName[MAX_PATH]{ 0 };
   vector<long>   m_vPages;

   // Error highlighting state
   long           m_nErrorStart{ -1 };
   long           m_nErrorEnd{ -1 };
   string         m_sErrorTooltip;
   CToolTipCtrl   m_tooltip;

   CEditView() = default;

   BOOL PreTranslateMessage(MSG* pMsg)
   {
      // Relay mouse messages to tooltip
      if (m_tooltip.IsWindow())
         m_tooltip.RelayEvent(pMsg);

      // In non Multi-thread SDI cases, CFindReplaceDialogWithMessageFilter will add itself to the
      // global message filters list.  In our case, we'll call it directly.
      if (m_pFindReplaceDialog != NULL)
      {
         if (m_pFindReplaceDialog->PreTranslateMessage(pMsg))
            return TRUE;
      }
      return FALSE;
   }

   BEGIN_MSG_MAP(CEditView)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKey)
      MESSAGE_HANDLER(WM_KEYUP, OnKey)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnKey)
      MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)

      COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditPaste)
      CHAIN_MSG_MAP_ALT(editCommandsClass, 1)

      CHAIN_MSG_MAP_ALT(findReplaceClass, 1)
      COMMAND_ID_HANDLER(ID_EDIT_WORD_WRAP, OnEditWordWrap)
      COMMAND_ID_HANDLER(ID_FORMAT_FONT, OnViewFormatFont)
   END_MSG_MAP()

   //HANDLERS
   LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

      LimitText(nMaxBufferLen);
      // Create tooltip control
      m_tooltip.Create(m_hWnd, NULL, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX);
      m_tooltip.SetMaxTipWidth(400); // Multi-line tooltip support
      // Register the entire edit control as a tool (required!)
      CRect rcClient;
      GetClientRect(&rcClient);
      CToolInfo ti(TTF_SUBCLASS, m_hWnd, 0, &rcClient, _T(""));
      m_tooltip.AddTool(&ti);

      return lRet;
   }

   LRESULT OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
   {
      LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

      // calc caret position
      long nStartPos, nEndPos;
      GetSel(nStartPos, nEndPos);
      m_nRow = LineFromChar(nEndPos);
      m_nCol = 0;
      int nChar = nEndPos - LineIndex();
      if (nChar > 0)
      {
         LPTSTR lpstrLine = (LPTSTR)_alloca(max(2, (nChar + 1) * sizeof(TCHAR)));	// min = WORD for length
         nChar = GetLine(m_nRow, lpstrLine, nChar);
         for (int i = 0; i < nChar; i++)
         {
            if (lpstrLine[i] == _T('\t'))
               m_nCol = ((m_nCol / cchTAB) + 1) * cchTAB;
            else
               m_nCol++;
         }
      }

      ::SendMessage(GetParent(), WM_UPDATEROWCOL, 0, 0L);

      return lRet;
   }

   LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
   {
      bHandled = FALSE;

      // Update tooltip text if hovering over error region
      if (m_nErrorStart >= 0 && m_nErrorEnd >= 0 && !m_sErrorTooltip.empty()) {
         POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
         long nCharIndex = CharFromPos(pt);

         if (nCharIndex >= m_nErrorStart && nCharIndex <= m_nErrorEnd) {
            // Convert ANSI error message to wide string for tooltip
            int nLen = MultiByteToWideChar(CP_ACP, 0, m_sErrorTooltip.c_str(), -1, NULL, 0);
            vector<wchar_t> wsBuf(nLen);
            MultiByteToWideChar(CP_ACP, 0, m_sErrorTooltip.c_str(), -1, wsBuf.data(), nLen);

            CToolInfo ti(TTF_SUBCLASS, m_hWnd, 0, NULL, wsBuf.data());
            m_tooltip.UpdateTipText(&ti);
            m_tooltip.Activate(TRUE);
         }
         else
            m_tooltip.Activate(FALSE);
      }

      return 0;
   }

   LRESULT OnEditPaste(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      PasteSpecial(CF_TEXT);
      return 0;
   }

   LRESULT OnEditWordWrap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      m_bWordWrap = !m_bWordWrap;
      int nLine = m_bWordWrap ? 0 : 1;
      SetTargetDevice(NULL, nLine);

      return 0;
   }

   LRESULT OnViewFormatFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CFontDialog dlg;
      m_font.GetLogFont(&dlg.m_lf);
      dlg.m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
      if (dlg.DoModal() == IDOK)
      {
         m_font.DeleteObject();
         m_font.CreateFontIndirect(&dlg.m_lf);
         SetFont(m_font);
      }
      return 0;
   }

   //Overrides from CEditFindReplaceImpl
   CFindReplaceDialogWithMessageFilter* CreateFindReplaceDialog(BOOL bFindOnly, // TRUE for Find, FALSE for FindReplace
      LPCTSTR lpszFindWhat, LPCTSTR lpszReplaceWith = NULL, DWORD dwFlags = FR_DOWN, HWND hWndParent = NULL)
   {
      CFindReplaceDialogWithMessageFilter* findReplaceDialog =
         new CFindReplaceDialogWithMessageFilter(NULL);

      if (findReplaceDialog == NULL)
      {
         ::MessageBeep(MB_ICONHAND);
      }
      else
      {
         HWND hWndFindReplace = findReplaceDialog->Create(bFindOnly,
            lpszFindWhat, lpszReplaceWith, dwFlags, hWndParent);
         if (hWndFindReplace == NULL)
         {
            delete findReplaceDialog;
            findReplaceDialog = NULL;
         }
         else
         {
            findReplaceDialog->SetActiveWindow();
            findReplaceDialog->ShowWindow(SW_SHOW);
         }
      }

      return findReplaceDialog;
   }

   DWORD GetFindReplaceDialogFlags(void) const
   {
      DWORD dwFlags = FR_HIDEWHOLEWORD;

      if (m_bFindDown)
         dwFlags |= FR_DOWN;
      if (m_bMatchCase)
         dwFlags |= FR_MATCHCASE;

      return dwFlags;
   }

   //METHODS
   void HighlightError(uint32_t nStart, uint32_t nEnd, const string& sErrorTooltip) {
      BOOL bModified = CRichEditCtrl::GetModify();
      // Clear previous error highlighting
      ClearErrorHighlight();

      // Validate range
      uint32_t nTextLength = (uint32_t)GetTextLengthEx(GTL_NUMCHARS);
      if (nStart == 0 || nStart > nTextLength || nEnd < nStart) {
         // Invalid range - highlight first word
         nStart = 0;
         // Find end of first word
         nEnd = 0;
         while (nEnd < nTextLength) {
            wchar_t szOneChar[2] = { 0 };
            GetTextRange(nEnd, nEnd + 1, szOneChar);
            if (!isalnum(szOneChar[0]) && szOneChar[0] != '_' && szOneChar[0] != '\\')
               break;
            nEnd++;
         }
         if (nEnd == 0)
            nEnd = min(10L, nTextLength); // Fallback to first 10 chars
      }
      //else {
      //   --nStart; //1-based to 0-based index
      //   --nEnd;
      //}

      // Clamp to valid range
      nEnd = min(nEnd, nTextLength);

      m_nErrorStart = (long)nStart;
      m_nErrorEnd = (long)nEnd;
      m_sErrorTooltip = sErrorTooltip;
      
      // Suspend undo
      IRichEditOle* pOle = GetOleInterface();
      ITextDocument* pDoc = nullptr;
      if (pOle)
         pOle->QueryInterface(__uuidof(ITextDocument), (void**)&pDoc);
      if(pDoc)
         pDoc->Undo(tomSuspend, nullptr);

      // Apply red wavy underline formatting
      SetSel((long)nStart, (long)nEnd);
      CHARFORMAT2W cf;
      cf.cbSize = sizeof(CHARFORMAT2W);
      cf.dwMask = CFM_UNDERLINETYPE | CFM_COLOR;
      cf.bUnderlineType = CFU_UNDERLINEWAVE;
      cf.dwEffects = 0; // Clear CFE_AUTOCOLOR
      cf.crTextColor = RGB(255, 0, 0); // Red

      SetSelectionCharFormat(cf);

      // Scroll to make error visible
      LineScroll(LineFromChar(nStart) - LineFromChar(GetFirstVisibleLine()));
      SetSel(nStart, nStart); // Move caret to error start
      // Resume undo
      if (pDoc) {
         pDoc->Undo(tomResume, nullptr);
         pDoc->Release();
      }
      if (pOle)
         pOle->Release();
      if(!bModified)
         CRichEditCtrl::SetModify(FALSE);
   }

   void ClearErrorHighlight() {
      if (m_nErrorStart >= 0 && m_nErrorEnd > m_nErrorStart) {
         BOOL bModify = CRichEditCtrl::GetModify();

         long nCurSelStart, nCurSelEnd;
         GetSel(nCurSelStart, nCurSelEnd);
         // Suspend undo
         IRichEditOle* pOle = GetOleInterface();
         ITextDocument* pDoc = nullptr;
         if (pOle)
            pOle->QueryInterface(__uuidof(ITextDocument), (void**)&pDoc);
         if (pDoc)
            pDoc->Undo(tomSuspend, nullptr);

         // Remove formatting
         //SetSel(m_nErrorStart, m_nErrorEnd);
         SetSelAll();
         CHARFORMAT2W cf;
         cf.cbSize = sizeof(CHARFORMAT2W);
         cf.dwMask = CFM_UNDERLINETYPE | CFM_COLOR;
         cf.bUnderlineType = CFU_UNDERLINENONE;
         cf.dwEffects = CFE_AUTOCOLOR;

         SetSelectionCharFormat(cf);
         SetSel(nCurSelStart, nCurSelEnd); // Restore original selection
         // Resume undo
         if (pDoc) {
            pDoc->Undo(tomResume, nullptr);
            pDoc->Release();
         }
         if (pOle)
            pOle->Release();

         m_nErrorStart = -1;
         m_nErrorEnd = -1;
         m_sErrorTooltip.clear();
         m_tooltip.Activate(FALSE);
         if(!bModify)
            CRichEditCtrl::SetModify(FALSE);
      }
   }

   void InitView(const CCfgReader& cfgr) {
      // View settings
      uint32_t nRGB = 0xFFFFFF;
      cfgr.GetHVal(L"EditColorBkg", nRGB);
      CRichEditCtrl::SetBackgroundColor(RGB2BGR(nRGB));

      nRGB = 0;
      cfgr.GetHVal(L"EditColorText", nRGB);
      CHARFORMAT cf;
      CRichEditCtrl::GetDefaultCharFormat(cf); // Get current defaults
      cf.dwMask |= CFM_COLOR;
      cf.dwEffects &= ~CFE_AUTOCOLOR; // Disable automatic system color
      cf.crTextColor = RGB2BGR(nRGB);
      CRichEditCtrl::SetDefaultCharFormat(cf);

      wstring sFontName{ L"Tahoma" };
      cfgr.GetSVal(L"EditFontName", sFontName);
      int nFontSize = 14;
      cfgr.GetNVal(L"EditFontSize", nFontSize);
      m_font = NULL;
      m_font.CreatePointFont(nFontSize * 10, sFontName.c_str());
      SetFont(m_font);
      //Word wrap
      m_bWordWrap = false;
      cfgr.GetBVal(L"EditWordWrap", m_bWordWrap);
      m_bWordWrap = !m_bWordWrap; //will be toggled!
      BOOL bDummy = FALSE;
      OnEditWordWrap(0, 0, NULL, bDummy);
      /*legacy MTPad code for tab stops
      CClientDC dc(m_hWnd);
      HFONT hOldFont = dc.SelectFont(m_font);
      TEXTMETRIC tm;
      dc.GetTextMetrics(&tm);
      int nLogPix = dc.GetDeviceCaps(LOGPIXELSX);
      dc.SelectFont(hOldFont);
      int cxTab = ::MulDiv(tm.tmAveCharWidth * cchTAB, 1440, nLogPix);	// 1440 twips = 1 inch
      if (cxTab != -1)
      {
         PARAFORMAT pf;
         pf.cbSize = sizeof(PARAFORMAT);
         pf.dwMask = PFM_TABSTOPS;
         pf.cTabCount = MAX_TAB_STOPS;
         for (int i = 0; i < MAX_TAB_STOPS; i++)
            pf.rgxTabs[i] = (i + 1) * cxTab;
         CRichEditCtrl::SetParaFormat(pf);
      }*/
      CRichEditCtrl::SetModify(FALSE);
   }

   BOOL LoadFile(LPCTSTR lpstrFilePath) {
      _ASSERTE(lpstrFilePath != NULL);

      HANDLE hFile = ::CreateFile(lpstrFilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
      if (hFile == INVALID_HANDLE_VALUE)
         return FALSE;
      ClearErrorHighlight();
      EDITSTREAM es;
      es.dwCookie = (DWORD_PTR)hFile;
      es.dwError = 0;
      es.pfnCallback = _StreamReadCallback;
      StreamIn(SF_TEXT, es);

      ::CloseHandle(hFile);
      CRichEditCtrl::SetModify(FALSE);
      return !(BOOL)es.dwError;
   }

   static DWORD CALLBACK _StreamReadCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR* pcb)
   {
      _ASSERTE(dwCookie != 0);
      _ASSERTE(pcb != NULL);

      return !::ReadFile((HANDLE)dwCookie, pbBuff, cb, (LPDWORD)pcb, NULL);
   }

   BOOL SaveFile(LPTSTR lpstrFilePath)
   {
      _ASSERT_RET(lpstrFilePath != NULL, FALSE);

      HANDLE hFile = ::CreateFile(lpstrFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
      if (hFile == INVALID_HANDLE_VALUE)
         return FALSE;

      EDITSTREAM es;
      es.dwCookie = (DWORD_PTR)hFile;
      es.dwError = 0;
      es.pfnCallback = _StreamWriteCallback;
      StreamOut(SF_TEXT, es);

      ::CloseHandle(hFile);
      if (es.dwError)
         return FALSE;
      CRichEditCtrl::SetModify(FALSE);
      return TRUE;
   }

   static DWORD CALLBACK _StreamWriteCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR* pcb)
   {
      _ASSERTE(dwCookie != 0);
      _ASSERTE(pcb != NULL);

      return !::WriteFile((HANDLE)dwCookie, pbBuff, cb, (LPDWORD)pcb, NULL);
   }

   void OnFileOpened(LPCTSTR lpstrFilePath, LPCTSTR lpstrFileName)
   {
      lstrcpy(m_strFilePath, lpstrFilePath);
      lstrcpy(m_strFileName, lpstrFileName);
      CRichEditCtrl::SetModify(FALSE);
   }

   BOOL QueryClose()
   {
      if (!CRichEditCtrl::GetModify())
         return TRUE;

      CWindow wndMain(GetParent());
      TCHAR szBuff[MAX_PATH + 30];
      wsprintf(szBuff, _T("Save changes to %s ?"), m_strFileName);
      int nRet = wndMain.MessageBox(szBuff, _T("MathPad"), MB_YESNOCANCEL | MB_ICONEXCLAMATION);

      if (nRet == IDCANCEL)
         return FALSE;
      else if (nRet == IDYES)
      {
         if (!DoFileSaveAs())
            return FALSE;
      }

      return TRUE;
   }

   BOOL DoFileSaveAs()
   {
      BOOL bRet = FALSE;

      CFileDialog dlg(FALSE, NULL, m_strFilePath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _szFileFilters);
      int nRet = (int)dlg.DoModal();

      if (nRet == IDOK)
      {
         ATLTRACE(_T("File path: %s\n"), dlg.m_ofn.lpstrFile);
         bRet = SaveFile(dlg.m_ofn.lpstrFile);
         if (bRet)
            OnFileOpened(dlg.m_ofn.lpstrFile, dlg.m_ofn.lpstrFileTitle);
         else
            MessageBox(_T("Error writing file!\n"));
      }

      return bRet;
   }
};