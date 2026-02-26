#include "StdAfx.h"
#include "CfgReader.h"

namespace {
   void _TrimString(IN OUT wstring& wsStr) {
      while (!wsStr.empty() && iswspace(wsStr.front()))
         wsStr.erase(wsStr.begin());
      while (!wsStr.empty() && iswspace(wsStr.back()))
         wsStr.pop_back();
   }
};
bool CCfgReader::Init(PCWSTR wszPath) {
   m_mapDict.clear();
   m_sPath.clear();
   wifstream inFile;
   inFile.open(wszPath, wifstream::in);
   if (!inFile)
      return false;
   m_sPath = wszPath;
   wstring wsLine, wsKey, wsVal;
   while (std::getline(inFile, wsLine)) {
      if (wsLine.front() == L';' || wsLine.find('=') == wstring::npos) //skip comments/garbage lines
         continue;
      uint32_t nEqPos = (uint32_t)wsLine.find(L'=');
      wsKey = wsLine.substr(0, nEqPos);
      _TrimString(wsKey);
      wsVal = wsLine.substr(nEqPos + 1);
      nEqPos = (uint32_t)wsVal.find(L';');
      if (nEqPos != wstring::npos) 
         wsVal = wsVal.substr(0, nEqPos);
      _TrimString(wsVal);
      if (wsKey.empty() || m_mapDict.find(wsKey) != m_mapDict.end()) {  //duplicates are not allowed
         _ASSERT(0);
         return false;
      }
      m_mapDict[wsKey] = wsVal;
   }
   return true;
}
//METHODS
bool CCfgReader::GetSVal(PCWSTR wszKey, IN OUT wstring& wsVal) const {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   wsVal = m_mapDict.find(wszKey)->second;
   return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT int32_t& nVal) const {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   nVal = std::stoi(m_mapDict.find(wszKey)->second);
   return true;
}
bool CCfgReader::GetNVal(PCWSTR wszKey, IN OUT uint32_t& nVal) const {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   nVal = std::stoul(m_mapDict.find(wszKey)->second);
   return true;
}
bool CCfgReader::GetHVal(PCWSTR wszKey, IN OUT uint32_t& nVal) const {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   nVal = std::stoul(m_mapDict.find(wszKey)->second, nullptr, 16);
   return true;
}
bool CCfgReader::GetBVal(PCWSTR wszKey, IN OUT bool& bVal) const {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   wstring wsVal = m_mapDict.find(wszKey)->second;
   bVal = wsVal.front() == L'1' || wsVal.front() == L'y' || wsVal.front() == L'Y'; //todo: overflow check
   return true;
}
bool CCfgReader::GetFVal(PCWSTR wszKey, IN OUT float& fVal) const {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   fVal = std::stof(m_mapDict.find(wszKey)->second);
   return true;
}
bool CCfgReader::GetSize(PCWSTR wszKey, IN OUT int32_t& nSX, IN OUT int32_t& nSY) const {
   if (!wszKey || m_mapDict.find(wszKey) == m_mapDict.end())
      return false;
   wstring wsVal = m_mapDict.find(wszKey)->second;
   uint32_t nCommaPos = (uint32_t)wsVal.find(L',');
   if (nCommaPos == wstring::npos)
      return false;
   wstring wsSX = wsVal.substr(0, nCommaPos);
   wstring wsSY = wsVal.substr(nCommaPos + 1);
   _TrimString(wsSX);
   _TrimString(wsSY);
   if (wsSX.empty() || wsSY.empty())
      return false;
   nSX = std::stoi(wsSX);
   nSY = std::stoi(wsSY);
   return true;
}
bool CCfgReader::GetSize(PCWSTR wszKey, IN OUT long& lSX, IN OUT long& lSY) const {
   int32_t nSX, nSY;
   if (!GetSize(wszKey, nSX, nSY))
      return false;
   lSX = (long)nSX;
   lSY = (long)nSY;
   return true;
}


