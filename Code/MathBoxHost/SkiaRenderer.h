#pragma once
#include "MathBoxHostDefs.h"
#include <cstdint>
#include <string>
#include <vector>
#include "include/core/SkRefCnt.h"

class CSkiaFontManager;
class SkSurface;
class SkMatrix;
class SkPaint;
class SkCanvas;

class CSkiaRenderer {
//DATA
   uint32_t                m_argbText{ 0 }; //and lines == foreground color
   float                   m_fFontSizePts{ 12.0f };
   const CSkiaFontManager& m_FontManager; //borrowed; must outlive this renderer
   HWND                    m_hwnd{ nullptr }; //borrowed
   SkSurface*              m_pSurface{ nullptr }; //owned; released with unref()
   SkCanvas*               m_pCanvas{ nullptr }; //borrowed during screen/PDF drawing
   bool                    m_bPDFDrawing{ false };
   int                     m_nCanvasSaveCount{ 0 };
   HRESULT                 m_hDrawResult{ S_OK };
   bool                    m_bDrawing{ false };
public:
//CTOR/DTOR/INIT
   explicit CSkiaRenderer(const CSkiaFontManager& fontManager) : m_FontManager(fontManager) {}
   ~CSkiaRenderer();
   CSkiaRenderer(const CSkiaRenderer&) = delete;
   CSkiaRenderer& operator=(const CSkiaRenderer&) = delete;
   CSkiaRenderer(CSkiaRenderer&&) = delete;
   CSkiaRenderer& operator=(CSkiaRenderer&&) = delete;

   bool Initialize(HWND hwnd);
//ATTS
   void SetTextColor(uint32_t nARGB) { m_argbText = nARGB; }
   void SetFontSizePts(float fFontSizePts) { m_fFontSizePts = fFontSizePts; }
//METHODS
   // The document owns pCanvas and must keep its page alive until EndPDFPage().
   bool BeginPDFPage(SkCanvas* pCanvas);
   HRESULT EndPDFPage();
   void EraseBkgnd(uint32_t nRGB);
   void OnSize(LPARAM lParam);
   void BeginDraw();
   // Optional presentation DC is borrowed for this call only.
   HRESULT EndDraw(HDC hDC = nullptr);
   void SetTransform(const SkMatrix& mat);
   void DrawLine(float x1, float y1, float x2, float y2, EnumLineStyles style, float width, uint32_t argb);
   void DrawRect(float left, float top, float right, float bottom, EnumLineStyles style,
                  float width, uint32_t argb);
   void FillRect(float left, float top, float right, float bottom, uint32_t argb);
   void DrawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                     float base_x, float base_y, float scale, uint32_t argb);
private:
//INTERNALS
   HRESULT ResizeSurface_();
   void SetStroke_(SkPaint& paint, EnumLineStyles style, float fWidth, uint32_t nARGB);
};
