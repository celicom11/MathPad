#pragma once

class CCfgReader {
//DATA
   wstring                 m_sPath;
   map<wstring, wstring>   m_mapDict;
public:
//CTOR/DTOR/Init
   CCfgReader() = default;
   ~CCfgReader() = default;
   bool Init(PCWSTR wszPath);
//ATTS
   const wstring& GetPath() const { return m_sPath; }
//METHODS
   bool GetSVal(PCWSTR wszKey, IN OUT wstring& wsVal) const;
   bool GetNVal(PCWSTR wszKey, IN OUT int32_t& nVal) const;
   bool GetSize(PCWSTR wszKey, IN OUT int32_t& nSX, IN OUT int32_t& nSY) const;
   bool GetSize(PCWSTR wszKey, IN OUT long& lSX, IN OUT long& lSY) const;
   bool GetNVal(PCWSTR wszKey, IN OUT uint32_t& nVal) const;
   bool GetHVal(PCWSTR wszKey, IN OUT uint32_t& nVal) const;
   bool GetBVal(PCWSTR wszKey, IN OUT bool& bVal) const;
   bool GetFVal(PCWSTR wszKey, IN OUT float& fVal) const;
};