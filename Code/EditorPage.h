#pragma once
#include "resource.h"
#include "CfgReader.h"
#include "CfgWriter.h"
#include "ColorBtn.h"

class CEditorPage : public CPropertyPageImpl<CEditorPage> {
 //DATA
   const CCfgReader& m_cfgReader;   //external!
   wstring           m_sFontName{ L"Tahoma" };
   int32_t           m_nFontSize{ 14 };
   bool              m_bWordWrap{ true };
   bool              m_bSyntaxHighlight{ false };
   bool              m_bOpenLastDoc{ true };
   CColorBtn         m_btnBkgColor;
   CColorBtn         m_btnTextColor;
public:
   enum { IDD = IDD_PAGE_EDITOR };
 //CTOR/DTOR
   CEditorPage(const CCfgReader& cfgReader) :
      CPropertyPageImpl(L"Editor"), m_cfgReader(cfgReader) {
      // Load Editor settings from config
      uint32_t rgb = 0xFFFFFF;   // default white
      m_cfgReader.GetHVal(L"EditColorBkg", rgb);
      m_btnBkgColor.SetColor(rgb);
      rgb = 0;   // default black
      m_cfgReader.GetHVal(L"EditColorText", rgb);
      m_btnTextColor.SetColor(rgb);

      m_cfgReader.GetSVal(L"EditFontName", m_sFontName);
      m_cfgReader.GetNVal(L"EditFontSize", m_nFontSize);
      m_cfgReader.GetBVal(L"EditWordWrap", m_bWordWrap);
      m_cfgReader.GetBVal(L"EditSyntaxHighlight", m_bSyntaxHighlight);
      m_cfgReader.GetBVal(L"EditOpenLastDoc", m_bOpenLastDoc);

   }
//METHODS
   BEGIN_MSG_MAP(CEditorPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_EDIT_BKG_COLOR_BTN, BN_CLICKED, OnBkgColorBtn)
      COMMAND_HANDLER(IDC_EDIT_TEXT_COLOR_BTN, BN_CLICKED, OnTextColorBtn)
      CHAIN_MSG_MAP(CPropertyPageImpl<CEditorPage>)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()
//MSG HANDLERS
   LRESULT OnBkgColorBtn(WORD, WORD, HWND, BOOL&) {
      uint32_t rgb = m_btnBkgColor.GetColor();
      if (PickColor_(IDC_EDIT_BKG_COLOR, rgb))
         m_btnBkgColor.SetColor(rgb);
      return 0;
   }
   LRESULT OnTextColorBtn(WORD, WORD, HWND, BOOL&) {
      uint32_t rgb = m_btnTextColor.GetColor();
      if (PickColor_(IDC_EDIT_TEXT_COLOR, rgb))
         m_btnTextColor.SetColor(rgb);
      return 0;
   }
   LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
      SetDlgItemText(IDC_EDIT_FONT_NAME, m_sFontName.c_str());
      SetDlgItemInt(IDC_EDIT_FONT_SIZE, m_nFontSize, FALSE);
      CheckDlgButton(IDC_EDIT_WORD_WRAP_CHK, m_bWordWrap ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(IDC_EDIT_SYNTAX_HIGHLIGHT_CHK, m_bSyntaxHighlight ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(IDC_EDIT_OPENLASTDOC_CHK, m_bOpenLastDoc ? BST_CHECKED : BST_UNCHECKED);
      SetDlgItemText_(IDC_EDIT_BKG_COLOR, m_btnBkgColor.GetColor());
      SetDlgItemText_(IDC_EDIT_TEXT_COLOR, m_btnTextColor.GetColor());
      m_btnBkgColor.Subclass(GetDlgItem(IDC_EDIT_BKG_COLOR_BTN));
      m_btnTextColor.Subclass(GetDlgItem(IDC_EDIT_TEXT_COLOR_BTN));
      return TRUE;
   }
 //METHODS
   void Write(CCfgWriter& cfgW) const {
      cfgW.AddString(nullptr, L"EditView");
      cfgW.AddUNumber(L"EditColorBkg", m_btnBkgColor.GetColor(), true);
      cfgW.AddUNumber(L"EditColorText", m_btnTextColor.GetColor(), true);
      cfgW.AddString(L"EditFontName", m_sFontName);
      cfgW.AddNumber(L"EditFontSize", m_nFontSize);
      cfgW.AddBool(L"EditWordWrap", m_bWordWrap);
      cfgW.AddBool(L"EditSyntaxHighlight", m_bSyntaxHighlight);
      cfgW.AddBool(L"EditOpenLastDoc", m_bOpenLastDoc);
   }
 //CPropertyPageImpl OVERRIDES
   BOOL OnKillActive() {
      return OnKillActive_();
   }

private:
   void SetDlgItemText_(int nID, uint32_t rgb) {
      wchar_t sz[16];
      swprintf_s(sz, L"%06X", rgb);
      SetDlgItemTextW(nID, sz);
   }
   bool PickColor_(int nEditID, uint32_t& rgb) {
      COLORREF cr = ARGB2COLORREF(rgb);
      HWND hwndParent = ::GetParent(m_hWnd);   // property sheet, not the page
      CColorDialog dlg(cr, CC_FULLOPEN, hwndParent);
      if (dlg.DoModal() != IDOK)
         return false;
      cr = dlg.GetColor();
      rgb = ((uint32_t)GetRValue(cr) << 16) | ((uint32_t)GetGValue(cr) << 8) | GetBValue(cr);
      SetDlgItemText_(nEditID, rgb);
      return true;
   }
   BOOL OnKillActive_() {
      // font size: 4..72
      BOOL bOk = FALSE;
      int nSize = GetDlgItemInt(IDC_EDIT_FONT_SIZE, &bOk, FALSE);
      if (!bOk || nSize < 4 || nSize > 72) {
         MessageBox(L"Font size must be between 4 and 72.", L"Validation", MB_ICONWARNING);
         ::SetFocus(GetDlgItem(IDC_EDIT_FONT_SIZE));
         return TRUE;
      }
      // collect
      m_nFontSize = nSize;
      uint32_t rgb = 0xFFFFFF;   // default white
      ParseHex_(IDC_EDIT_BKG_COLOR, rgb);
      m_btnBkgColor.SetColor(rgb);
      rgb = 0;   // default black
      ParseHex_(IDC_EDIT_TEXT_COLOR, rgb);
      m_btnTextColor.SetColor(rgb);
      wchar_t sz[LF_FACESIZE];
      GetDlgItemTextW(IDC_EDIT_FONT_NAME, sz, LF_FACESIZE);
      m_sFontName = sz;
      m_bWordWrap = IsDlgButtonChecked(IDC_EDIT_WORD_WRAP_CHK) == BST_CHECKED;
      m_bSyntaxHighlight = IsDlgButtonChecked(IDC_EDIT_SYNTAX_HIGHLIGHT_CHK) == BST_CHECKED;
      m_bOpenLastDoc = IsDlgButtonChecked(IDC_EDIT_OPENLASTDOC_CHK) == BST_CHECKED;
      return FALSE; //OK to deactivate
   }
   void ParseHex_(int nID, uint32_t& rgb) {
      wchar_t sz[16];
      GetDlgItemTextW(nID, sz, _countof(sz));
      wchar_t* pEnd = nullptr;
      uint32_t v = (uint32_t)wcstoul(sz, &pEnd, 16);
      if (pEnd != sz)
         rgb = v;
   }
};
