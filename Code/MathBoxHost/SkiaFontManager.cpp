#include "stdafx.h"
#include "SkiaFontManager.h"
#include "MathBoxHostDefs.h"
#include "include/core/SkString.h"
#pragma comment(lib, "advapi32.lib")

namespace {

   void GetInstalledFontFiles_(HKEY hRoot, const filesystem::path& sDirectory,
                               set<filesystem::path>& paths, const wchar_t* szFamily = nullptr) {
      HKEY hFonts = nullptr;
      if (RegOpenKeyExW(hRoot, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts)",
                        0, KEY_READ, &hFonts) != ERROR_SUCCESS) {
         return;
      }
      vector<wchar_t> vName(16384), vValue(32768), vExpanded(32768);
      for (DWORD nIndex = 0;; ++nIndex) {
         DWORD nNameChars = static_cast<DWORD>(vName.size());
         DWORD nValueBytes = static_cast<DWORD>((vValue.size() - 1) * sizeof(wchar_t));
         DWORD nType = 0;
         LSTATUS nStatus = RegEnumValueW(hFonts, nIndex, vName.data(), &nNameChars,
                                         nullptr, &nType, reinterpret_cast<BYTE*>(vValue.data()), &nValueBytes);
         if (nStatus == ERROR_NO_MORE_ITEMS) { break; }
         if (nStatus != ERROR_SUCCESS || (nType != REG_SZ && nType != REG_EXPAND_SZ)) { continue; }
         // Registry display names normally start with the family. Verify the
         // actual family through FreeType below; unrelated faces need no I/O.
         if (szFamily && _wcsnicmp(vName.data(), szFamily, wcslen(szFamily))) { continue; }
         vValue[nValueBytes / sizeof(wchar_t)] = 0;
         const wchar_t* szPath = vValue.data();
         if (nType == REG_EXPAND_SZ) {
            DWORD nChars = ExpandEnvironmentStringsW(szPath, vExpanded.data(), static_cast<DWORD>(vExpanded.size()));
            if (!nChars || nChars > vExpanded.size()) { continue; }
            szPath = vExpanded.data();
         }
         if (!*szPath) { continue; }
         filesystem::path sPath(szPath);
         paths.insert(sPath.is_absolute() ? sPath : sDirectory / sPath);
      }
      RegCloseKey(hFonts);
   }
   sk_sp<SkData> ReadFontData_(const filesystem::path& sPath) {
      ifstream stream(sPath, ios::binary | ios::ate);
      if (!stream) { return nullptr; }
      streamsize nBytes = stream.tellg();
      if (nBytes <= 0) { return nullptr; }
      auto data = SkData::MakeUninitialized(static_cast<size_t>(nBytes));
      stream.seekg(0);
      if (!stream.read(static_cast<char*>(data->writable_data()), nBytes)) { return nullptr; }
      return data;
   }
   SkRect GetCFFControlBounds_(const SkPath& path) {
      SkRect bounds = SkRect::MakeEmpty();
      bool bHasPoint = false;
      auto addPoint = [&bounds, &bHasPoint](const SkPoint& pt) {
         if (!bHasPoint) {
            bounds.setLTRB(pt.x(), pt.y(), pt.x(), pt.y());
            bHasPoint = true;
         } else {
            bounds.setLTRB(min(bounds.left(), pt.x()), min(bounds.top(), pt.y()),
                           max(bounds.right(), pt.x()), max(bounds.bottom(), pt.y()));
         }
      };
      SkPath::RawIter iter(path);
      SkPoint aPoints[4];
      for (SkPath::Verb verb = iter.next(aPoints); verb != SkPath::kDone_Verb; verb = iter.next(aPoints)) {
         switch (verb) {
         case SkPath::kMove_Verb:
            addPoint(aPoints[0]);
            break;
         case SkPath::kLine_Verb:
            addPoint(aPoints[0]);
            addPoint(aPoints[1]);
            break;
         case SkPath::kQuad_Verb:
            // Skia can simplify CFF cubics to equivalent quadratics. Recover
            // their original cubic controls for the font's metric box.
            addPoint(aPoints[0]);
            addPoint({ (aPoints[0].x() + 2 * aPoints[1].x()) / 3,
                       (aPoints[0].y() + 2 * aPoints[1].y()) / 3 });
            addPoint({ (aPoints[2].x() + 2 * aPoints[1].x()) / 3,
                       (aPoints[2].y() + 2 * aPoints[1].y()) / 3 });
            addPoint(aPoints[2]);
            break;
         case SkPath::kCubic_Verb:
            for (const SkPoint& pt : aPoints) {
               addPoint(pt);
            }
            break;
         case SkPath::kConic_Verb:
            // Not produced by the bundled CFF fonts.
            return path.getBounds();
         default:
            break;
         }
      }
      return bounds;
   }
   uint16_t ReadUInt16_(const uint8_t* pData) {
      return static_cast<uint16_t>((pData[0] << 8) | pData[1]);
   }
   int16_t ReadInt16_(const uint8_t* pData) {
      return static_cast<int16_t>(ReadUInt16_(pData));
   }
   bool ReadMetric_(const SkData& data, uint16_t nLongMetrics, uint16_t nGlyph,
                    uint32_t& nAdvance, int32_t& nBearing) {
      if (!nLongMetrics) {
         return false;
      }
      const size_t nAdvanceOffset = 4u * (std::min<uint32_t>(nGlyph, nLongMetrics - 1));
      const size_t nBearingOffset = nGlyph < nLongMetrics ? 4u * nGlyph + 2 :
         4u * nLongMetrics + 2u * (nGlyph - nLongMetrics);
      if (nAdvanceOffset + 2 > data.size() || nBearingOffset + 2 > data.size()) {
         return false;
      }
      nAdvance = ReadUInt16_(data.bytes() + nAdvanceOffset);
      nBearing = ReadInt16_(data.bytes() + nBearingOffset);
      return true;
   }
}

// FreeType loads the bundled fonts without system installation.
CSkiaFontManager::CSkiaFontManager() :
   m_pFontManager(SkFontMgr_New_Custom_Empty().release()) {}

CSkiaFontManager::~CSkiaFontManager() {
   m_vTypefaces.clear();
   if (m_pFontManager) {
      m_pFontManager->unref();
   }
}

bool CSkiaFontManager::Init(const std::wstring& sFontDir) {
   if (!m_pFontManager || sFontDir.empty()) {
      return false;
   }
   wstring sDirectory = sFontDir;
   if (sDirectory.back() != L'\\' && sDirectory.back() != L'/') {
      sDirectory += L'\\';
   }
   vector<sk_sp<SkTypeface>> vTypefaces;
   vTypefaces.reserve(_countof(_aLMFonts));
   for (PCWSTR szFontFile : _aLMFonts) {
      const filesystem::path sFontPath(sDirectory + szFontFile);
      ifstream fontStream(sFontPath, ios::binary | ios::ate);
      if (!fontStream) {
         return false;
      }
      const streamsize nSize = static_cast<streamsize>(fontStream.tellg());
      if (nSize <= 0) {
         return false;
      }
      sk_sp<SkData> pData = SkData::MakeUninitialized(static_cast<size_t>(nSize));
      fontStream.seekg(0);
      if (!fontStream.read(static_cast<char*>(pData->writable_data()), nSize)) {
         return false;
      }
      sk_sp<SkTypeface> pTypeface = m_pFontManager->makeFromData(move(pData), 0);
      if (!pTypeface) {
         return false;
      }
      vTypefaces.push_back(move(pTypeface));
   }
   vector<float> vScales(vTypefaces.size(), 1.0f);
   m_vFontScales.swap(vScales);
   m_vTypefaces.swap(vTypefaces);
   m_sFontDir.swap(sDirectory);
   return true;
}

bool CSkiaFontManager::AddUtf8Font(const wchar_t* szFontName, float fScale) {
   return AddSystemFonts_(szFontName, FONT_UTF8_REGULAR, 4, fScale);
}
bool CSkiaFontManager::AddSymbolFont(const wchar_t* szFontName, float fScale) {
   return AddSystemFonts_(szFontName, FONT_SYMBOL, 1, fScale);
}
bool CSkiaFontManager::AddSystemFonts_(const wchar_t* szFontName, int32_t nFirstSlot,
                                      uint32_t nCount, float fScale) {
   if (!m_pFontManager || m_vTypefaces.size() < _countof(_aLMFonts) ||
       !szFontName || !*szFontName || !isfinite(fScale) || fScale <= 0) {
      return false;
   }
   wchar_t wszWindows[MAX_PATH], wszLocal[32768];
   UINT nWindowsChars = GetWindowsDirectoryW(wszWindows, _countof(wszWindows));
   if (!nWindowsChars || nWindowsChars >= _countof(wszWindows)) { return false; }
   set<filesystem::path> paths;
   GetInstalledFontFiles_(HKEY_LOCAL_MACHINE, filesystem::path(wszWindows) / L"Fonts", paths, szFontName);
   DWORD nLocalChars = GetEnvironmentVariableW(L"LOCALAPPDATA", wszLocal, _countof(wszLocal));
   if (nLocalChars && nLocalChars < _countof(wszLocal)) {
      GetInstalledFontFiles_(HKEY_CURRENT_USER, filesystem::path(wszLocal) / L"Microsoft/Windows/Fonts", paths, szFontName);
   }
   vector<sk_sp<SkTypeface>> vFaces(nCount);
   vector<int> vScores(nCount, INT_MAX);
   for (const auto& sPath : paths) {
      auto data = ReadFontData_(sPath);
      if (!data) { continue; }
      uint32_t nFaces = 1;
      if (data->size() >= 12 && memcmp(data->data(), "ttcf", 4) == 0) {
         const uint8_t* pBytes = data->bytes() + 8;
         nFaces = (uint32_t(pBytes[0]) << 24) | (uint32_t(pBytes[1]) << 16) |
                  (uint32_t(pBytes[2]) << 8) | pBytes[3];
         if (!nFaces || nFaces > (data->size() - 12) / 4) { continue; }
      }
      for (uint32_t nFace = 0; nFace < nFaces; ++nFace) {
         auto face = m_pFontManager->makeFromData(data, static_cast<int>(nFace));
         if (!face) { continue; }
         SkString sFamily;
         face->getFamilyName(&sFamily);
         int nChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sFamily.c_str(), -1, nullptr, 0);
         if (!nChars) { continue; }
         vector<wchar_t> vFamily(nChars);
         MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, sFamily.c_str(), -1, vFamily.data(), nChars);
         if (_wcsicmp(vFamily.data(), szFontName)) { continue; }
         const SkFontStyle style = face->fontStyle();
         for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
            int nWeight = (nIdx == 1 || nIdx == 3) ? 700 : 400;
            bool bItalic = nIdx >= 2;
            // Prefer the requested slant, then width, then the closest weight.
            int nScore = (bItalic != (style.slant() != SkFontStyle::kUpright_Slant) ? 100000 : 0) +
                         abs(style.width() - SkFontStyle::kNormal_Width) * 10000 + abs(style.weight() - nWeight);
            if (nScore < vScores[nIdx]) {
               vScores[nIdx] = nScore;
               vFaces[nIdx] = face;
            }
         }
      }
   }
   if (any_of(vFaces.begin(), vFaces.end(), [](const auto& face) { return !face; })) { return false; }
   const size_t nRequired = static_cast<size_t>(nFirstSlot) + nCount;
   m_vTypefaces.reserve(nRequired);
   m_vFontScales.reserve(nRequired);
   if (m_vTypefaces.size() < nRequired) {
      m_vTypefaces.resize(nRequired);
      m_vFontScales.resize(nRequired, 1.0f);
   }
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      m_vTypefaces[nFirstSlot + nIdx] = move(vFaces[nIdx]);
      m_vFontScales[nFirstSlot + nIdx] = fScale;
   }
   return true;
}

SkTypeface* CSkiaFontManager::GetTypeface(int32_t nFontIdx) const {
   if (nFontIdx < 0 || static_cast<size_t>(nFontIdx) >= m_vTypefaces.size())
      return nullptr;
   return m_vTypefaces[nFontIdx].get();
}

bool CSkiaFontManager::GetFontIndices(int32_t nFontIdx, uint32_t nCount,
                                     const uint32_t* pUnicodes, uint16_t* pIndices) {
   SkTypeface* pTypeface = GetTypeface(nFontIdx);
   if (!pTypeface || !nCount || !pUnicodes || !pIndices) {
      return false;
   }
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      const uint32_t nUnicode = pUnicodes[nIdx];
      pIndices[nIdx] = nUnicode > 0x10FFFF || (nUnicode >= 0xD800 && nUnicode <= 0xDFFF) ?
         0 : pTypeface->unicharToGlyph(static_cast<SkUnichar>(nUnicode));
   }
   return true;
}

bool CSkiaFontManager::GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount,
                                        const uint16_t* pIndices, MB_GlyphMetrics* pGlyphMetrics,
                                        MB_Bounds& bounds) {
   SkTypeface* pTypeface = GetTypeface(nFontIdx);
   if (!pTypeface || !nCount || !pIndices || !pGlyphMetrics) {
      return false;
   }
   const int nGlyphCount = pTypeface->countGlyphs();
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      if (pIndices[nIdx] >= nGlyphCount) {
         return false;
      }
   }
   if (nFontIdx >= FONT_UTF8_REGULAR) {
      return GetSystemGlyphRunMetrics_(nFontIdx, nCount, pIndices, pGlyphMetrics, bounds);
   }
   if (pTypeface->getUnitsPerEm() != otfUnitsPerEm) { return false; }
   uint8_t aHhea[36];
   uint8_t aOS2[72];
   if (pTypeface->getTableData(SkSetFourByteTag('h','h','e','a'), 0, sizeof(aHhea), aHhea) != sizeof(aHhea) ||
       pTypeface->getTableData(SkSetFourByteTag('O','S','/','2'), 0, sizeof(aOS2), aOS2) != sizeof(aOS2)) {
      return false;
   }
   auto pHmtx = pTypeface->copyTableData(SkSetFourByteTag('h','m','t','x'));
   if (!pHmtx) {
      return false;
   }
   const uint16_t nHorizontalMetrics = ReadUInt16_(aHhea + 34);
   const int32_t nTypoAscent = ReadInt16_(aOS2 + 68);
   const int32_t nTypoDescent = ReadInt16_(aOS2 + 70);
   // The bundled Latin Modern fonts have no vertical metrics. Preserve the
   // baseline's OS/2 typo ascent/descent cell, rather than SkFontMetrics.
   auto pVmtx = pTypeface->copyTableData(SkSetFourByteTag('v','m','t','x'));
   uint8_t aVhea[36];
   uint16_t nVerticalMetrics = 0;
   if (pVmtx) {
      if (pTypeface->getTableData(SkSetFourByteTag('v','h','e','a'), 0, sizeof(aVhea), aVhea) != sizeof(aVhea)) {
         return false;
      }
      nVerticalMetrics = ReadUInt16_(aVhea + 34);
   } else if (nTypoAscent < nTypoDescent) {
      return false;
   }
   SkFont font(sk_ref_sp(pTypeface), static_cast<float>(otfUnitsPerEm));
   font.setHinting(SkFontHinting::kNone);
   font.setLinearMetrics(true);
   font.setSubpixel(true);
   // Skia canonicalizes path requests to 64 pixels. Increase horizontal
   // sampling precision before undoing this scale in design space, so integer
   // side bearings do not depend on FreeType's 26.6-pixel rounding at that size.
   constexpr float fPathScaleX = 256.0f;
   font.setScaleX(fPathScaleX);
   const bool bCFF = pTypeface->getTableSize(SkSetFourByteTag('C','F','F',' ')) != 0;
   vector<MB_GlyphMetrics> vMetrics(nCount);
   SkRect rcRun = SkRect::MakeEmpty();
   bool bHasOutline = false;
   float fPositionX = 0;
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      auto path = font.getPath(pIndices[nIdx]);
      if (!path) {
         return false; // MathBox needs vector outlines, not bitmap raster bounds.
      }
      path = path->tryMakeTransform(SkMatrix::Scale(1.0f / fPathScaleX, 1.0f));
      if (!path) {
         return false;
      }
      MB_GlyphMetrics& metrics = vMetrics[nIdx];
      metrics.size_bytes = sizeof(MB_GlyphMetrics);
      if (!ReadMetric_(*pHmtx, nHorizontalMetrics, pIndices[nIdx], metrics.nAdvanceWidth, metrics.nLSB)) {
         return false;
      }
      // Glyph cell metrics use the control box; run ink bounds use curve extrema.
      const SkRect rcGlyph = bCFF ? GetCFFControlBounds_(*path) : path->getBounds();
      // CFF side bearings in hmtx can differ from the outline's control box.
      // The existing adapter reports the latter for these OTF fonts.
      metrics.nLSB = F2NEAREST(rcGlyph.left());
      // Snap floating-point noise at integer edges, then truncate the CFF right edge.
      const int32_t nRight = static_cast<int32_t>(std::floor(rcGlyph.right() + 0.001f));
      const int32_t nGlyphWidth = nRight - F2NEAREST(rcGlyph.left());
      const int32_t nGlyphHeight = F2NEAREST(rcGlyph.bottom()) - F2NEAREST(rcGlyph.top());
      metrics.nRSB = static_cast<int32_t>(metrics.nAdvanceWidth) - metrics.nLSB - nGlyphWidth;
      if (pVmtx) {
         if (!ReadMetric_(*pVmtx, nVerticalMetrics, pIndices[nIdx], metrics.nAdvanceHeight, metrics.nTSB)) {
            return false;
         }
      } else {
         metrics.nAdvanceHeight = static_cast<uint32_t>(nTypoAscent - nTypoDescent);
         metrics.nTSB = nTypoAscent + F2NEAREST(rcGlyph.top());
      }
      metrics.nBSB = static_cast<int32_t>(metrics.nAdvanceHeight) - metrics.nTSB - nGlyphHeight;
      if (!path->isEmpty()) {
         SkRect rcInk = path->computeTightBounds();
         rcInk.offset(fPositionX, 0);
         if (!bHasOutline) {
            rcRun = rcInk;
            bHasOutline = true;
         } else {
            rcRun.setLTRB(min(rcRun.left(), rcInk.left()), min(rcRun.top(), rcInk.top()),
                          max(rcRun.right(), rcInk.right()), max(rcRun.bottom(), rcInk.bottom()));
         }
      }
      fPositionX += metrics.nAdvanceWidth;
   }
   std::copy(vMetrics.begin(), vMetrics.end(), pGlyphMetrics);
   bounds = { F2NEAREST(rcRun.left()), F2NEAREST(rcRun.top()),
              F2NEAREST(rcRun.right()), F2NEAREST(rcRun.bottom()) };
   return true;
}

bool CSkiaFontManager::GetSystemGlyphRunMetrics_(int32_t nFontIdx, uint32_t nCount,
                                               const uint16_t* pIndices, MB_GlyphMetrics* pGlyphMetrics,
                                               MB_Bounds& bounds) {
   SkTypeface* pTypeface = GetTypeface(nFontIdx);
   int nUnits = pTypeface->getUnitsPerEm();
   if (nUnits <= 0) { return false; }
   const float fScale = GetFontScale(nFontIdx);
   SkFont font(sk_ref_sp(pTypeface), static_cast<float>(nUnits));
   font.setHinting(SkFontHinting::kNone);
   font.setLinearMetrics(true);
   font.setSubpixel(true);
   vector<SkScalar> vAdvances(nCount);
   font.getWidths({ pIndices, nCount }, { vAdvances.data(), nCount });
   const float fNormalize = otfUnitsPerEm * fScale / nUnits;
   uint8_t aMetrics[72];
   if (pTypeface->getTableData(SkSetFourByteTag('O','S','/','2'), 0, sizeof(aMetrics), aMetrics) != sizeof(aMetrics)) {
      return false;
   }
   const int32_t nAscent = ReadInt16_(aMetrics + 68);
   const int32_t nDescent = ReadInt16_(aMetrics + 70);
   vector<MB_GlyphMetrics> vMetrics(nCount);
   SkRect rcRun = SkRect::MakeEmpty();
   bool bHasOutline = false;
   float fPosition = 0;
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      auto path = font.getPath(pIndices[nIdx]);
      if (!path) { return false; }
      SkRect rcInk = path->computeTightBounds();
      auto& metrics = vMetrics[nIdx];
      metrics.size_bytes = sizeof(MB_GlyphMetrics);
      metrics.nLSB = F2NEAREST(rcInk.left() * fNormalize);
      metrics.nAdvanceWidth = static_cast<uint32_t>(F2NEAREST(vAdvances[nIdx] * fNormalize));
      metrics.nRSB = F2NEAREST((vAdvances[nIdx] - rcInk.right()) * fNormalize);
      metrics.nAdvanceHeight = static_cast<uint32_t>(F2NEAREST((nAscent - nDescent) * fNormalize));
      metrics.nTSB = F2NEAREST((rcInk.top() + nAscent) * fNormalize);
      metrics.nBSB = F2NEAREST((-nDescent - rcInk.bottom()) * fNormalize);
      if (!path->isEmpty()) {
         rcInk = SkRect::MakeLTRB((rcInk.left() + fPosition) * fNormalize, rcInk.top() * fNormalize,
                                  (rcInk.right() + fPosition) * fNormalize, rcInk.bottom() * fNormalize);
         if (!bHasOutline) { rcRun = rcInk; bHasOutline = true; }
         else { rcRun.join(rcInk); }
      }
      fPosition += vAdvances[nIdx];
   }
   copy(vMetrics.begin(), vMetrics.end(), pGlyphMetrics);
   bounds = { F2NEAREST(rcRun.left()), F2NEAREST(rcRun.top()), F2NEAREST(rcRun.right()), F2NEAREST(rcRun.bottom()) };
   return true;
}
