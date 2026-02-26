#pragma once
class CEditorPage;
class CMathBoxViewPage;
class CMathBoxLibPage;
class CCfgReader;
class CConfigMRUList;

class CSettingsDlg : public CPropertySheetImpl<CSettingsDlg> {
//DATA
   const CCfgReader&    m_cfgReader;   //external!
   const CConfigMRUList& m_mru;        //external!
   CEditorPage*         m_pEditorPage{ nullptr };
   CMathBoxViewPage*    m_pMBViewPage{ nullptr };
   CMathBoxLibPage*     m_pMBLibPage{ nullptr };
public:
//CTOR/DTOR
   CSettingsDlg(HWND hwndParent, const CCfgReader& cfgReader, const CConfigMRUList& mru);
   ~CSettingsDlg();
//ATTS
// Maps
   BEGIN_MSG_MAP(CSettingsDlg)
      CHAIN_MSG_MAP(CPropertySheetImpl<CSettingsDlg>)
   END_MSG_MAP()
//METHODS
   bool WriteConfig() const;
};