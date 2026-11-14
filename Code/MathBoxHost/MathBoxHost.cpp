#include "stdafx.h"
#include "MathBoxHost.h"
#include "SkiaFontManager.h"
#include "SkiaRenderer.h"

namespace {
   CSkiaFontManager*  _pFontManager{ nullptr };
   CSkiaRenderer*     _pSKR{ nullptr };
}
CMathBoxHost::CMathBoxHost(CSkiaFontManager& skFontManager, CSkiaRenderer& skr) {
   _pFontManager = &skFontManager;
   _pSKR = &skr;
   m_DocParams.size_bytes = sizeof(MB_DocParams);
   m_DocParams.max_width_fdu = 0;   //no limit
   m_DocParams.font_size_pts = 24.0f;
   m_DocParams.color_text_argb = 0x004F00;
   m_DocParams.color_bkg_argb = 0xFFFFFF;
   m_DocParams.color_selection_argb = 0xFF4FFF;
   //SkiaFontManager shim
   m_DocParams.font_mgr.size_bytes = sizeof(MBI_FontManager);
   m_DocParams.font_mgr.fontCount = 0; //unknown yet
   m_DocParams.font_mgr.getFontsDir = _getFontsDir;
   m_DocParams.font_mgr.getFontIndices = _getFontIndices;
   m_DocParams.font_mgr.getGlyphRunMetrics = _getGlyphRunMetrics;
   //DocRenderer shim
   m_DocRenderer.size_bytes = sizeof(MBI_DocRenderer);
   m_DocRenderer.drawLine = _drawLine;
   m_DocRenderer.drawRect = _drawRect;
   m_DocRenderer.fillRect = _fillRect;
   m_DocRenderer.drawGlyphRun = _drawGlyphRun;
}
bool CMathBoxHost::LoadMathBoxLib() {
   //update font counts
   m_DocParams.font_mgr.fontCount = _pFontManager->FontCount();
   _ASSERT_RET(!m_pMBAPI && !m_hMathBoxLib && !m_engine, false);//snbh!
   m_hMathBoxLib = ::LoadLibraryW(L"MathBoxLib.dll");
   _ASSERT_RET(m_hMathBoxLib, false);
   auto pfnMB_GetApi = (const MBI_API * (*)())GetProcAddress(m_hMathBoxLib, "MB_GetApi");
   _ASSERT_RET(pfnMB_GetApi, false);
   m_pMBAPI = pfnMB_GetApi();
   _ASSERT_RET(m_pMBAPI && m_pMBAPI->abi_version >= 1, false);
   MB_RET ret = m_pMBAPI->createEngine(&m_DocParams, &m_engine);
   if (ret != MBOK) {
      PCSTR szError = m_pMBAPI->getLastError(m_engine, &m_Error.nErrorStartPos, &m_Error.nErrorEndPos);
      m_Error.sError = szError ? szError : "Unknown error";
      m_Error.nErrorCode = ret;
      return false;
   }
   return true;
}
void CMathBoxHost::CleanUp() {
   if (m_pMBAPI) {
      if (m_pMBItem) {
         m_pMBAPI->destroyMathItem(m_pMBItem);
         m_pMBItem = nullptr;
      }
      if (m_engine) {
         m_pMBAPI->destroyEngine(m_engine);
         m_engine = nullptr;
      }
      m_pMBAPI = nullptr;
   }
   if (m_hMathBoxLib) {
      ::FreeModule(m_hMathBoxLib);
      m_hMathBoxLib = NULL;
   }
}
bool CMathBoxHost::AddMacros(const string& sMacros, const string& sFile) {
   _ASSERT_RET(!!m_engine && !!m_pMBAPI, false);
   MB_RET ret = m_pMBAPI->addMacros(m_engine, sMacros.c_str(), sFile.c_str());
   if (ret != MBOK) {
      PCSTR szError = m_pMBAPI->getLastError(m_engine, &m_Error.nErrorStartPos, &m_Error.nErrorEndPos);
      m_Error.sError = szError ? szError : "Unknown error adding macros";
      m_Error.nErrorCode = ret;
      return false;
   }
   return true;
}

bool CMathBoxHost::Parse(const string& sTeX, OUT vector<CRectF>& vLines) {
   _ASSERT_RET(!!m_engine, false);
   if (m_pMBItem) {
      m_pMBAPI->destroyMathItem(m_pMBItem);
      m_pMBItem = nullptr;
   }
   m_Error.nErrorCode = MBOK;
   m_Error.sError.clear();

   MB_RET ret = m_pMBAPI->parseLatex(m_engine, sTeX.c_str(), &m_pMBItem);
   if (ret != MBOK) {
      PCSTR szError = m_pMBAPI->getLastError(m_engine, &m_Error.nErrorStartPos, &m_Error.nErrorEndPos);
      m_Error.sError = szError ? szError : "Unknown parsing error";
      m_Error.nErrorCode = ret;
      return false;
   }
   uint32_t nLines = m_pMBAPI->mathItemLineCount(m_pMBItem);
   _ASSERT_RET(nLines, false); //snbh!
   vLines.resize(nLines);
   for( int32_t nLine = 0; nLine < (int32_t)nLines; ++nLine) {
      CRectF& rcf = vLines[nLine];
      ret = m_pMBAPI->mathItemGetLineBox(m_pMBItem, nLine, &rcf.left, &rcf.top, &rcf.right, &rcf.bottom);
      _ASSERT_RET(ret == MBOK, false); //snbh!
   }
   return true;
}
void CMathBoxHost::Draw(float fX, float fY, float fZoom) {
   if (!!m_pMBAPI && !!m_pMBItem) {
      if (!isfinite(fZoom) || fZoom <= 0) {
         return;
      }
      _pSKR->SetFontSizePts(m_DocParams.font_size_pts);
      if(fZoom != 1.0f) {
         SkMatrix matScale = SkMatrix::ScaleTranslate(fZoom, fZoom, 0, 0);
         _pSKR->SetTransform(matScale);
      }
      m_pMBAPI->mathItemDraw(m_pMBItem, fX, fY, &m_DocRenderer);
      if (fZoom != 1.0f) { //restore
         _pSKR->SetTransform(SkMatrix::I());
      }
   }
}
void CMathBoxHost::DrawLines(float fX, float fY, int32_t nLineStart, int32_t nLineEnd, float fZoom) {
   if (!m_pMBAPI || !m_pMBItem)
      return; //ntd
   if (!isfinite(fZoom) || fZoom <= 0) {
      return;
   }
   _pSKR->SetFontSizePts(m_DocParams.font_size_pts);
   if (fZoom != 1.0f) {
      SkMatrix matScale = SkMatrix::ScaleTranslate(fZoom, fZoom, 0, 0);
      _pSKR->SetTransform(matScale);
   }
   m_pMBAPI->mathItemDrawLines(m_pMBItem, fX/fZoom, fY/fZoom, nLineStart, nLineEnd, &m_DocRenderer);
   if (fZoom != 1.0f) { //restore
      _pSKR->SetTransform(SkMatrix::I());
   }
}
//C-API/callbacks
bool CMathBoxHost::_getFontsDir(IN OUT uint32_t* buf_len, OUT wchar_t* out_dir) {
   _ASSERT_RET(buf_len, false);
   const wstring wsFontDir = _pFontManager->GetFontDir();
   const uint32_t nLenChars = (uint32_t)wsFontDir.size() + 1;
   if(out_dir) {
      if(*buf_len < nLenChars)
         _ASSERT_RET(0, false); //snbh!
      *buf_len = nLenChars;
      memcpy(out_dir, wsFontDir.c_str(), sizeof(wchar_t)*nLenChars);
   }
   else
      *buf_len = nLenChars;
   return true;
}
bool CMathBoxHost::_getFontIndices(int32_t font_idx, uint32_t count, const uint32_t* unicodes, 
                                    OUT uint16_t* out_indices) {
   return _pFontManager->GetFontIndices(font_idx, count, unicodes, out_indices);
}
bool CMathBoxHost::_getGlyphRunMetrics(int32_t font_idx, uint32_t count,
                                          const uint16_t* indices, OUT MB_GlyphMetrics* out_glyph_metrics, 
                                          OUT MB_Bounds* out_bounds) {
   if (!out_bounds) {
      return false;
   }
   return _pFontManager->GetGlyphRunMetrics(font_idx, count, indices, out_glyph_metrics, *out_bounds);
}
//
void CMathBoxHost::_drawLine(float x1, float y1, float x2, float y2, uint32_t style, float width, uint32_t argb) {
   _pSKR->DrawLine(x1, y1, x2, y2, (EnumLineStyles)style, width, argb);
}
void CMathBoxHost::_drawRect(float left, float top, float right, float bottom, 
                             uint32_t style, float width, uint32_t argb) {
   _pSKR->DrawRect(left, top, right, bottom, (EnumLineStyles)style, width, argb);
}
void CMathBoxHost::_fillRect(float left, float top, float right, float bottom, uint32_t argb) {
   _pSKR->FillRect(left, top, right, bottom, argb);
}
void CMathBoxHost::_drawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                                 float base_x, float base_y, float scale, uint32_t argb) {
   _pSKR->DrawGlyphRun(font_idx, count, indices, base_x, base_y, scale, argb);
}
