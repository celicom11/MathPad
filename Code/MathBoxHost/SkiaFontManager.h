#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "include/core/SkRefCnt.h"
#include "MathBox_CAPI.h"

class SkFontMgr;
class SkTypeface;

class CSkiaFontManager {
//DATA
   SkFontMgr*                       m_pFontManager{ nullptr }; //owned; released with unref()
   std::wstring                     m_sFontDir;
   std::vector<sk_sp<SkTypeface>>   m_vTypefaces;
public:
//CTOR/DTOR/INIT
   CSkiaFontManager();
   ~CSkiaFontManager();

   CSkiaFontManager(const CSkiaFontManager&) = delete;
   CSkiaFontManager& operator=(const CSkiaFontManager&) = delete;
   CSkiaFontManager(CSkiaFontManager&&) = delete;
   CSkiaFontManager& operator=(CSkiaFontManager&&) = delete;

   // Loads all Latin Modern faces in MathBox font-index order, at face index 0.
   // On failure, previously loaded fonts and their directory remain unchanged.
   bool Init(const std::wstring& sFontDir);
//ATTS
   const std::wstring& GetFontDir() const { return m_sFontDir; }
   uint32_t FontCount() const { return (uint32_t)m_vTypefaces.size(); }
   bool GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes, uint16_t* pIndices);
   // Bundled 1000-units/em fonts; outline bounds use the existing Y-down baseline.
   // Empty outlines have zero ink bounds. Invalid input leaves metric outputs unchanged.
   bool GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                           MB_GlyphMetrics* pGlyphMetrics, MB_Bounds& bounds);
   // Borrowed; valid until the next successful Init or manager destruction.
   SkTypeface* GetTypeface(int32_t nFontIdx) const;
};
