#include "stdafx.h"
#include "CfgWriter.h"

//comment (;) if wszKey == NULL!
void CCfgWriter::AddString(PCWSTR wszKey, const wstring& wsVal) {
   if (wszKey) {
      m_sContent += wszKey;
      m_sContent += L" = ";
   }
   else
      m_sContent += L";";
   m_sContent += wsVal + L'\n';
}
void CCfgWriter::AddNumber(PCWSTR wszKey, int32_t nVal) {
   if (wszKey) {
      m_sContent += wszKey;
      m_sContent += L" = ";
   }
   else
      m_sContent += L";";
   m_sContent += std::to_wstring(nVal) + L'\n';
}
void CCfgWriter::AddUNumber(PCWSTR wszKey, uint32_t nVal, bool bHex) {
   if (wszKey) {
      m_sContent += wszKey;
      m_sContent += L" = ";
   }
   else
      m_sContent += L";";

   if (bHex) {
      wchar_t wszHex[16];
      swprintf_s(wszHex, L"0x%X", nVal);
      m_sContent += wszHex;
   }
   else
      m_sContent += std::to_wstring(nVal);
   m_sContent += L'\n';
}
void CCfgWriter::AddFloat(PCWSTR wszKey, float fVal, uint32_t nDigAfterPt) {
   if (wszKey) {
      m_sContent += wszKey;
      m_sContent += L" = ";
   }
   else
      m_sContent += L";";

   wchar_t wszFloat[32];
   swprintf_s(wszFloat, L"%.*f", nDigAfterPt, fVal);
   m_sContent += wszFloat;
   m_sContent += L'\n';
}
void CCfgWriter::AddSize(PCWSTR wszKey, SIZE szeVal) {
   if (wszKey) {
      m_sContent += wszKey;
      m_sContent += L" = ";
   }
   else
      m_sContent += L";";

   m_sContent += std::to_wstring(szeVal.cx);
   m_sContent += L',';
   m_sContent += std::to_wstring(szeVal.cy);
   m_sContent += L'\n';
}
void CCfgWriter::AddBool(PCWSTR wszKey, bool bVal) {
   if (wszKey) {
      m_sContent += wszKey;
      m_sContent += L" = ";
   }
   else
      m_sContent += L";";

   m_sContent += bVal ? L"1\n" : L"0\n";
}
bool CCfgWriter::WriteTo(PCWSTR wszPath) {
   if (!wszPath || m_sContent.empty())
      return false;

   // Convert wstring to UTF-8
   int nSizeNeeded = ::WideCharToMultiByte(CP_UTF8, 0, m_sContent.c_str(), -1, nullptr, 0, nullptr, nullptr);
   if (nSizeNeeded <= 0)
      return false;

   vector<char> vUtf8(nSizeNeeded);
   int nConverted = ::WideCharToMultiByte(CP_UTF8, 0, m_sContent.c_str(), -1, vUtf8.data(), nSizeNeeded, nullptr, nullptr);
   if (nConverted <= 0)
      return false;

   // Create temp file path
   wstring wsTempPath = wszPath;
   wsTempPath += L".tmp";

   // Write to temp file (no BOM)
   ofstream outFile(wsTempPath, ios::out | ios::binary);
   if (!outFile)
      return false;

   // Write without the null terminator
   outFile.write(vUtf8.data(), nConverted - 1);
   outFile.close();

   if (!outFile.good())
      return false;

   // Replace original file with temp file
   if (!::DeleteFileW(wszPath)) {
      // If file doesn't exist, that's okay
      if (::GetLastError() != ERROR_FILE_NOT_FOUND) {
         ::DeleteFileW(wsTempPath.c_str());
         return false;
      }
   }

   if (!::MoveFileW(wsTempPath.c_str(), wszPath)) {
      ::DeleteFileW(wsTempPath.c_str());
      return false;
   }

   return true;
}


