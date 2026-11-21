#pragma once
#include <windows.h>
#include <string>
#include <algorithm>
#include <cstdint>

inline bool WideTextToUtf8(const wchar_t* pText, int nLength, std::string& sText) {
   sText.clear();
   if (!nLength) { return true; }
   int nBytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pText, nLength,
                                   nullptr, 0, nullptr, nullptr);
   if (!nBytes) { return false; }
   sText.resize(nBytes);
   return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, pText, nLength,
                              &sText[0], nBytes, nullptr, nullptr) == nBytes;
}

inline uint32_t Utf8ByteOffsetToRichEdit(const std::string& sText, uint32_t nOffset) {
   size_t nLimit = (std::min)(static_cast<size_t>(nOffset), sText.size());
   uint32_t nPosition = 0;
   for (size_t nByte = 0; nByte < nLimit;) {
      unsigned char nLead = static_cast<unsigned char>(sText[nByte]);
      size_t nBytes = nLead < 0x80 ? 1 : nLead < 0xE0 ? 2 : nLead < 0xF0 ? 3 : 4;
      if (nByte + nBytes > nLimit) { break; }
      // RichEdit represents CRLF as a single UTF-16 paragraph character.
      if (nLead != '\n' || !nByte || sText[nByte - 1] != '\r') {
         nPosition += nBytes == 4 ? 2 : 1;
      }
      nByte += nBytes;
   }
   return nPosition;
}
