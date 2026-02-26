#include "stdafx.h"
#include "SettingsDlg.h"
#include "EditorPage.h"
#include "MathBoxViewPage.h"
#include "MathBoxLibPage.h"
#include "CfgReader.h"
#include "CfgWriter.h"
#include "ConfigMRUList.h"

CSettingsDlg::CSettingsDlg(HWND hwndParent, const CCfgReader& cfgReader, const CConfigMRUList& mru)
   : CPropertySheetImpl(L"Settings", 0, hwndParent), m_cfgReader(cfgReader), m_mru(mru)
{
   m_psh.dwFlags |= PSH_NOAPPLYNOW| PSH_NOCONTEXTHELP;   // remove Apply button

   m_pEditorPage = new CEditorPage(cfgReader);
   m_pMBViewPage = new CMathBoxViewPage(cfgReader);
   m_pMBLibPage  = new CMathBoxLibPage(cfgReader);

   AddPage(*m_pEditorPage);
   AddPage(*m_pMBViewPage);
   AddPage(*m_pMBLibPage);
}

CSettingsDlg::~CSettingsDlg() {
   delete m_pMBLibPage;
   delete m_pMBViewPage;
   delete m_pEditorPage;
}

bool CSettingsDlg::WriteConfig() const {
   CCfgWriter cfgW;
   m_pEditorPage->Write(cfgW);
   m_pMBViewPage->Write(cfgW);
   m_pMBLibPage->Write(cfgW);

   // Append MRU entries (OpenLastDoc + MRU0..MRU9)
   m_mru.WriteToWriter(cfgW);
   if (!cfgW.WriteTo(m_cfgReader.GetPath().c_str())) {
      _ASSERT_RET(0, false);
   }
   return true;
}

