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
   std::vector<float>              m_vFontScales;
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
   // Installed families are discovered through the Windows font registry;
   // all face loading, glyph mapping, metrics and drawing remain FreeType/Skia.
   bool AddUtf8Font(const wchar_t* szFontName, float fScale = 1.0f);
   bool AddSymbolFont(const wchar_t* szFontName, float fScale = 0.8f);
//ATTS
   float GetFontScale(int32_t nIdx) const {
      return nIdx >= 0 && static_cast<size_t>(nIdx) < m_vFontScales.size() ? m_vFontScales[nIdx] : 1.0f;
   }
   const std::wstring& GetFontDir() const { return m_sFontDir; }
   uint32_t FontCount() const { return (uint32_t)m_vTypefaces.size(); }
   bool GetFontIndices(int32_t nFontIdx, uint32_t nCount, const uint32_t* pUnicodes, uint16_t* pIndices);
   // Bundled 1000-units/em fonts; outline bounds use the existing Y-down baseline.
   // Empty outlines have zero ink bounds. Invalid input leaves metric outputs unchanged.
   bool GetGlyphRunMetrics(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                           MB_GlyphMetrics* pGlyphMetrics, MB_Bounds& bounds);
   // Borrowed; valid until the next successful Init or manager destruction.
   SkTypeface* GetTypeface(int32_t nFontIdx) const;
private:
   bool GetSystemGlyphRunMetrics_(int32_t nFontIdx, uint32_t nCount, const uint16_t* pIndices,
                                  MB_GlyphMetrics* pGlyphMetrics, MB_Bounds& bounds);
   bool AddSystemFonts_(const wchar_t* szFontName, int32_t nFirstSlot, uint32_t nCount, float fScale);
};
