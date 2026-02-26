#pragma once
class CCfgReader;
class CCfgWriter;

class CConfigMRUList : public CRecentDocumentList {
public:
   CConfigMRUList(int nMaxEntries = 10);
   bool ReadFromConfig(const CCfgReader& cfgr);
   bool WriteToConfig(const wstring& sPath);
   void WriteToWriter(CCfgWriter& w) const;
};