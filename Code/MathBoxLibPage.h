#pragma once
#include "resource.h"
#include "CfgReader.h"
#include "CfgWriter.h"

class CMathBoxLibPage : public CPropertyPageImpl<CMathBoxLibPage> {
//DATA
   const CCfgReader& m_cfgReader;   //external!
   wstring           m_sMacros{ L"Macros.mth" };
   float             m_fFontSizePts{ 10.0f };
   CColorBtn         m_btnBkgColor;    //page bkg color
   CColorBtn         m_btnTextColor;   //page text color
public:
   enum { IDD = IDD_PAGE_MATHBOXLIB };
//CTOR/DTOR
   CMathBoxLibPage(const CCfgReader& cfgReader) : 
      CPropertyPageImpl(L"MathBox"), m_cfgReader(cfgReader) {
      // Load MathBoxLib settings from config
      m_cfgReader.GetSVal(L"MB_Macros", m_sMacros);
      m_cfgReader.GetFVal(L"MB_FontSizePts", m_fFontSizePts);
      uint32_t rgb = 0xFFFFFF;   // default white
      m_cfgReader.GetHVal(L"MB_ColorBkg", rgb);
      m_btnBkgColor.SetColor(rgb);
      rgb = 0;   // default black
      m_cfgReader.GetHVal(L"MB_ColorText", rgb);
      m_btnTextColor.SetColor(rgb);
   }
//ATTS
//MSG MAP
   BEGIN_MSG_MAP(CMathBoxLibPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_MB_BKG_COLOR_BTN,  BN_CLICKED, OnBkgColorBtn)
      COMMAND_HANDLER(IDC_MB_TEXT_COLOR_BTN, BN_CLICKED, OnTextColorBtn)
      CHAIN_MSG_MAP(CPropertyPageImpl<CMathBoxLibPage>)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()
   LRESULT OnBkgColorBtn(WORD, WORD, HWND, BOOL&) {
      uint32_t rgb = m_btnBkgColor.GetColor();
      if (PickColor_(IDC_MB_BKG_COLOR, rgb))
         m_btnBkgColor.SetColor(rgb);
      return 0;
   }
   LRESULT OnTextColorBtn(WORD, WORD, HWND, BOOL&) {
      uint32_t rgb = m_btnTextColor.GetColor();
      if (PickColor_(IDC_MB_TEXT_COLOR, rgb))
         m_btnTextColor.SetColor(rgb);
      return 0;
   }
   LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
      SetDlgItemTextW(IDC_MB_MACROS, m_sMacros.c_str());
      // font size: display with 1 decimal place
      wchar_t sz[32];
      swprintf_s(sz, L"%.1f", m_fFontSizePts);
      SetDlgItemText(IDC_MB_FONT_SIZE, sz);
      SetColorText_(IDC_MB_BKG_COLOR,  m_btnBkgColor.GetColor());
      SetColorText_(IDC_MB_TEXT_COLOR, m_btnTextColor.GetColor());
      m_btnBkgColor.Subclass(GetDlgItem(IDC_MB_BKG_COLOR_BTN));
      m_btnTextColor.Subclass(GetDlgItem(IDC_MB_TEXT_COLOR_BTN));

      return TRUE;
   }
//METHODS
   void Write(CCfgWriter& cfgW) const {
      cfgW.AddString(nullptr, L"MathBox");
      cfgW.AddString(L"MB_Macros", m_sMacros);
      cfgW.AddFloat(L"MB_FontSizePts", m_fFontSizePts, 1);
      cfgW.AddUNumber(L"MB_ColorBkg", m_btnBkgColor.GetColor(), true);
      cfgW.AddUNumber(L"MB_ColorText", m_btnTextColor.GetColor(), true);
   }
 //CPropertyPageImpl OVERRIDES
   BOOL OnKillActive() {
      return OnKillActive_();
   }

private:
   void SetColorText_(int nID, uint32_t rgb) {
      wchar_t sz[16];
      swprintf_s(sz, L"%06X", rgb);
      SetDlgItemText(nID, sz);
   }
   BOOL OnKillActive_() {
      wchar_t sz[32];
      GetDlgItemTextW(IDC_MB_FONT_SIZE, sz, _countof(sz));
      wchar_t* pEnd = nullptr;
      float fSize = wcstof(sz, &pEnd);
      if (pEnd == sz || fSize < 4.0f || fSize > 72.0f) {
         MessageBox(L"Font size must be between 4 and 72.", L"Validation", MB_ICONWARNING);
         ::SetFocus(GetDlgItem(IDC_MB_FONT_SIZE));
         return TRUE;
      }
      m_fFontSizePts = fSize;
      uint32_t rgb = 0xFFFFFF;   // default white
      ParseHex_(IDC_MB_BKG_COLOR,  rgb);
      m_btnBkgColor.SetColor(rgb);
      rgb = 0;   // default black
      ParseHex_(IDC_MB_TEXT_COLOR, rgb);
      m_btnTextColor.SetColor(rgb);
      return FALSE;
   }
   void ParseHex_(int nID, uint32_t& rgb) {
      wchar_t sz[16];
      GetDlgItemTextW(nID, sz, _countof(sz));
      wchar_t* pEnd = nullptr;
      uint32_t v = (uint32_t)wcstoul(sz, &pEnd, 16);
      if (pEnd != sz)
         rgb = v;
   }
   bool PickColor_(int nEditID, uint32_t& rgb) {
      COLORREF cr = ARGB2COLORREF(rgb);
      CColorDialog dlg(cr, CC_FULLOPEN, ::GetParent(m_hWnd));   // fix parent
      if (dlg.DoModal() != IDOK)
         return false;
      cr  = dlg.GetColor();
      rgb = ((uint32_t)GetRValue(cr) << 16) | ((uint32_t)GetGValue(cr) << 8) | GetBValue(cr);
      SetColorText_(nEditID, rgb);   // update hex edit
      return true;
   }
};

