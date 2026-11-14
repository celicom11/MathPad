#include "stdafx.h"
#include "SkiaFontManager.h"
#include "MathBoxHostDefs.h"

namespace {
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
   m_vTypefaces.swap(vTypefaces);
   m_sFontDir.swap(sDirectory);
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
   if (!pTypeface || !nCount || !pIndices || !pGlyphMetrics ||
       pTypeface->getUnitsPerEm() != otfUnitsPerEm) {
      return false;
   }
   const int nGlyphCount = pTypeface->countGlyphs();
   for (uint32_t nIdx = 0; nIdx < nCount; ++nIdx) {
      if (pIndices[nIdx] >= nGlyphCount) {
         return false;
      }
   }
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
