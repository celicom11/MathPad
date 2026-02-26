#pragma once
class CAboutDlg : public CDialogImpl<CAboutDlg>
{
   uint32_t m_MathBoxAbiVersion;
public:
   enum { IDD = IDD_ABOUTDLG };
//CTOR
   CAboutDlg(uint32_t nMathBoxAbiVersion) :m_MathBoxAbiVersion(nMathBoxAbiVersion) {}
   BEGIN_MSG_MAP(CAboutDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
      COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
   END_MSG_MAP()

   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
   {
      CenterWindow(GetParent());
      // Set MathBoxLib version in a static text control, e.g. IDC_MATHBOXLIB_VERSION
      wchar_t wszVer[32];
      wsprintf(wszVer, _T("MathBox ABI: %u"), m_MathBoxAbiVersion);
      SetDlgItemTextW(IDC_MATHBOXABI_VERSION, wszVer);

      return TRUE;
   }

   LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      EndDialog(wID);
      return 0;
   }
};
