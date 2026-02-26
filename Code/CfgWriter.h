#pragma once

class CCfgWriter {
//DATA
  wstring m_sContent;
public:
//CTOR/DTOR
  CCfgWriter() = default;
  ~CCfgWriter() = default;
//ATTS
  const wstring& GetContent() const { return m_sContent; }
//METHODS
  void Reset() { m_sContent.clear(); }
  void AddString(PCWSTR wszKey, const wstring& wsVal); //comment , if wszKey == NULL!
  void AddNumber(PCWSTR wszKey, int32_t nVal);
  void AddUNumber(PCWSTR wszKey, uint32_t nVal, bool bHex=false);
  void AddFloat(PCWSTR wszKey, float fVal, uint32_t nDigAfterPt = 2);
  void AddSize(PCWSTR wszKey, SIZE szeVal);
  void AddBool(PCWSTR wszKey, bool bVal);
  bool WriteTo(PCWSTR wszPath);
};