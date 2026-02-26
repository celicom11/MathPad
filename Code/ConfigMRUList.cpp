#include "stdafx.h"
#include "ConfigMRUList.h"
#include "CfgReader.h"
#include "CfgWriter.h"



CConfigMRUList::CConfigMRUList(int nMaxEntries) {
   SetMaxEntries(nMaxEntries);
}
// Read MRU from config file (replaces ReadFromRegistry)
bool CConfigMRUList::ReadFromConfig(const CCfgReader& cfgr) {
   m_arrDocs.RemoveAll();  // Clear existing

   // Read in REVERSE order so MRU0 (most recent) ends up at the top
   for (int nIdx = m_nMaxEntries - 1; nIdx >= 0; --nIdx) {
      wstring sEntryKey = L"MRU" + to_wstring(nIdx);  // MRU0, MRU1, etc.
      wstring sPath;
      if (cfgr.GetSVal(sEntryKey.c_str(), sPath) && !sPath.empty())
         AddToList(sPath.c_str());  // Uses base class logic
   }

   return UpdateMenu();  // Uses base class menu builder
}

// Write MRU to config file (replaces WriteToRegistry)
bool CConfigMRUList::WriteToConfig(const wstring& sPath) {
   // Read config line by line, ignoring MRU# lines
   wifstream inFile(sPath);
   if (!inFile)
      return false;

   vector<wstring> vConfigLines;
   wstring wsLine;
   while (std::getline(inFile, wsLine)) {
      // Skip MRU# lines
      if (wsLine.find(L"MRU") == 0)
         continue;
      vConfigLines.push_back(wsLine);
   }
   inFile.close();

   // Write back existing lines
   wofstream ofs(sPath);
   if (!ofs)
      return false;

   for (const wstring& sLine : vConfigLines) {
      ofs << sLine << L"\n";
   }

   // Append MRU entries (most recent = MRU0)
   int nSize = m_arrDocs.GetSize();
   for (int nIdx = 0; nIdx < nSize; ++nIdx) {
      wstring sEntryKey = L"MRU" + to_wstring(nIdx);
      // Most recent is at the END of m_arrDocs, so write in reverse
      ofs << sEntryKey << L" = " << m_arrDocs[nSize - 1 - nIdx].szDocName << L"\n";
   }

   // Pad with empty entries up to m_nMaxEntries
   for (int nIdx = nSize; nIdx < m_nMaxEntries; ++nIdx) {
      wstring sEntryKey = L"MRU" + to_wstring(nIdx);
      ofs << sEntryKey << L" =\n";
   }

   return true;
}

void CConfigMRUList::WriteToWriter(CCfgWriter& w) const {
   w.AddString(nullptr, L"MRU List. Add more MRU#= lines if needed");
   int nSize = m_arrDocs.GetSize();
   for (int nIdx = 0; nIdx < nSize; ++nIdx) {
      wstring sKey = L"MRU" + to_wstring(nIdx);
      // Most recent is at the END of m_arrDocs
      w.AddString(sKey.c_str(), wstring(m_arrDocs[nSize - 1 - nIdx].szDocName));
   }
   // Pad with empty entries up to m_nMaxEntries
   for (int nIdx = nSize; nIdx < m_nMaxEntries; ++nIdx) {
      wstring sKey = L"MRU" + to_wstring(nIdx);
      w.AddString(sKey.c_str(), wstring());
   }
}
