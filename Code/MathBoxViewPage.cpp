#include "stdafx.h"
#include "MathBoxViewPage.h"

namespace {
   struct SPagePreset { PCWSTR wszName; int32_t nW; int32_t nH; };
   static const SPagePreset _aPagePresets[] = {
      { L"A4 (595 x 842)",        595,  842 },
      { L"A3 (842 x 1191)",       842, 1191 },
      { L"Letter (612 x 792)",    612,  792 },
      { L"Legal (612 x 1008)",    612, 1008 },
      { L"Custom",                  0,    0 },
   };
}
CMathBoxViewPage::CMathBoxViewPage(const CCfgReader& cfgReader) : 
   CPropertyPageImpl(L"MathView"), m_cfgReader(cfgReader) {
   // Load MathBoxView settings from config
   uint32_t rgb{ 0x003E00 };
   m_cfgReader.GetHVal(L"ColorBkg", rgb);
   m_btnBkgColor.SetColor(rgb);

   m_cfgReader.GetSize(L"PageSizePts", m_szePage.cx, m_szePage.cy);
   m_cfgReader.GetSize(L"PageMarginsPts", m_szeMargins.cx, m_szeMargins.cy);
   m_cfgReader.GetNVal(L"PageVertGapPts", m_nVertGap);
}
LRESULT CMathBoxViewPage::OnBkgColorBtn(WORD, WORD, HWND, BOOL&) {
   uint32_t rgb = m_btnBkgColor.GetColor();
   COLORREF cr = ARGB2COLORREF(rgb);
   CColorDialog dlg(cr, CC_FULLOPEN, ::GetParent(m_hWnd));
   if (dlg.DoModal() == IDOK) {
      cr  = dlg.GetColor();
      rgb = ((uint32_t)GetRValue(cr) << 16) | ((uint32_t)GetGValue(cr) << 8) | GetBValue(cr);
      m_btnBkgColor.SetColor(rgb);
      SetDlgItemText_(IDC_VIEW_BKG_COLOR, rgb);   // keep hex edit in sync
   }
   return 0;
}
LRESULT CMathBoxViewPage::OnPageSizeCombo(WORD, WORD, HWND, BOOL&) {
   CComboBox cb(GetDlgItem(IDC_PAGE_SIZE_COMBO));
   UpdateSizeEdits_(cb.GetCurSel());
   return 0;
}
LRESULT CMathBoxViewPage::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {

   // fill combo
   CComboBox cb(GetDlgItem(IDC_PAGE_SIZE_COMBO));
   for (const SPagePreset& preset : _aPagePresets)
      cb.AddString(preset.wszName);
   // select matching preset or "Custom"
   int nSel = _countof(_aPagePresets) - 1;
   for (int nIdx = 0; nIdx < nSel; ++nIdx) {
      if (_aPagePresets[nIdx].nW == m_szePage.cx && _aPagePresets[nIdx].nH == m_szePage.cy) {
         nSel = nIdx;
         break;
      }
   }
   cb.SetCurSel(nSel);
   // populate edit controls
   SetDlgItemInt(IDC_PAGE_WIDTH,    m_szePage.cx,    FALSE);
   SetDlgItemInt(IDC_PAGE_HEIGHT,   m_szePage.cy,    FALSE);
   SetDlgItemInt(IDC_PAGE_MARGIN_X, m_szeMargins.cx, FALSE);
   SetDlgItemInt(IDC_PAGE_MARGIN_Y, m_szeMargins.cy, FALSE);
   SetDlgItemInt(IDC_PAGE_VERT_GAP, m_nVertGap,      FALSE);
   UpdateSizeEdits_(nSel);
   SetDlgItemText_(IDC_VIEW_BKG_COLOR, m_btnBkgColor.GetColor());
   // subclass color button last
   m_btnBkgColor.Subclass(GetDlgItem(IDC_VIEW_BKG_COLOR_BTN));
   return TRUE;
}
void CMathBoxViewPage::Write(CCfgWriter& cfgW) const {
   cfgW.AddString(nullptr, L"MathBoxView");
   cfgW.AddUNumber(L"ColorBkg",       m_btnBkgColor.GetColor(), true);
   cfgW.AddSize(L"PageSizePts",       m_szePage);
   cfgW.AddSize(L"PageMarginsPts",    m_szeMargins);
   cfgW.AddNumber(L"PageVertGapPts",  m_nVertGap);
}
void CMathBoxViewPage::UpdateSizeEdits_(int nComboSel) {
   BOOL bCustom = (nComboSel == _countof(_aPagePresets) - 1) ? TRUE : FALSE;
   ::EnableWindow(GetDlgItem(IDC_PAGE_WIDTH),  bCustom);
   ::EnableWindow(GetDlgItem(IDC_PAGE_HEIGHT), bCustom);
   if (!bCustom) {
      SetDlgItemInt(IDC_PAGE_WIDTH,  _aPagePresets[nComboSel].nW, FALSE);
      SetDlgItemInt(IDC_PAGE_HEIGHT, _aPagePresets[nComboSel].nH, FALSE);
   }
}
BOOL CMathBoxViewPage::OnKillActive_() {
   BOOL bOk = FALSE;
   if (!ParseHex_(IDC_VIEW_BKG_COLOR, m_btnBkgColor)) {
      ShowErr_(L"Invalid background color.", IDC_VIEW_BKG_COLOR);
      return TRUE;
   }

   m_szePage.cx    = (int32_t)GetDlgItemInt(IDC_PAGE_WIDTH, &bOk, FALSE);
   if (!bOk || m_szePage.cx < 1)    { 
      ShowErr_(L"Invalid page width.", IDC_PAGE_WIDTH);
      return TRUE; 
   }
   m_szePage.cy = (int32_t)GetDlgItemInt(IDC_PAGE_HEIGHT, &bOk, FALSE);
   if (!bOk || m_szePage.cy < 1)    { 
      ShowErr_(L"Invalid page height.",         IDC_PAGE_HEIGHT);   
      return TRUE; 
   }
   m_szeMargins.cx = (int32_t)GetDlgItemInt(IDC_PAGE_MARGIN_X, &bOk, FALSE);
   if (!bOk || m_szeMargins.cx < 0) { 
      ShowErr_(L"Invalid horizontal margin.", IDC_PAGE_MARGIN_X);
      return TRUE; 
   }
   m_szeMargins.cy = (int32_t)GetDlgItemInt(IDC_PAGE_MARGIN_Y, &bOk, FALSE);
   if (!bOk || m_szeMargins.cy < 0) { 
      ShowErr_(L"Invalid vertical margin.", IDC_PAGE_MARGIN_Y); 
      return TRUE; 
   }
   m_nVertGap = (int32_t)GetDlgItemInt(IDC_PAGE_VERT_GAP, &bOk, FALSE);
   if (!bOk || m_nVertGap < 0) { 
      ShowErr_(L"Invalid vertical gap.", IDC_PAGE_VERT_GAP); 
      return TRUE; 
   }
   return FALSE;
}
// read hex edit back into button
bool CMathBoxViewPage::ParseHex_(int nID, CColorBtn& btn) {
   wchar_t sz[16];
   GetDlgItemTextW(nID, sz, _countof(sz));
   wchar_t* pEnd = nullptr;
   uint32_t nRGB = (uint32_t)wcstoul(sz, &pEnd, 16);
   if (pEnd == sz)
      return false;
   btn.SetColor(nRGB);
   return true;
}
void CMathBoxViewPage::ShowErr_(PCWSTR wsz, int nFocusID) {
   MessageBox(wsz, L"Validation", MB_ICONWARNING);
   ::SetFocus(GetDlgItem(nFocusID));
}
