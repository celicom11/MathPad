#pragma once
#include "resource.h"
#include "CfgReader.h"
#include "CfgWriter.h"
#include "ColorBtn.h"

class CMathBoxViewPage : public CPropertyPageImpl<CMathBoxViewPage> {
//DATA
   const CCfgReader& m_cfgReader;   //external!
   CColorBtn         m_btnBkgColor;
   SIZE              m_szePage{ 595, 842 };    // A4
   SIZE              m_szeMargins{ 72, 72 };
   int32_t           m_nVertGap{ 10 };
public:
   enum { IDD = IDD_PAGE_MATHBOXVIEW };
//CTOR/DTOR
   CMathBoxViewPage(const CCfgReader& cfgReader);
//MSG MAP
   BEGIN_MSG_MAP(CMathBoxViewPage)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_HANDLER(IDC_VIEW_BKG_COLOR_BTN, BN_CLICKED, OnBkgColorBtn)
      COMMAND_HANDLER(IDC_PAGE_SIZE_COMBO, CBN_SELCHANGE, OnPageSizeCombo)
      CHAIN_MSG_MAP(CPropertyPageImpl<CMathBoxViewPage>)
      REFLECT_NOTIFICATIONS()
   END_MSG_MAP()
//MSG HANDLERS
   LRESULT OnBkgColorBtn(WORD, WORD, HWND, BOOL&);
   LRESULT OnPageSizeCombo(WORD, WORD, HWND, BOOL&);
   LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
//METHODS
   void Write(CCfgWriter& cfgW) const;
//CPropertyPageImpl OVERRIDES
   BOOL OnKillActive() {
      return OnKillActive_();
   }

private:
   void UpdateSizeEdits_(int nComboSel);
   BOOL OnKillActive_();
   bool ParseHex_(int nID, CColorBtn& btn);
   void ShowErr_(PCWSTR wsz, int nFocusID);
   void SetDlgItemText_(int nID, uint32_t rgb) {
      wchar_t sz[16];
      swprintf_s(sz, L"%06X", rgb);
      SetDlgItemText(nID, sz);
   }
};

