#include "stdafx.h"
#include "SkiaRenderer.h"
#include "SkiaFontManager.h"

CSkiaRenderer::~CSkiaRenderer() {
   if (m_pSurface) {
      m_pSurface->unref();
   }
}

bool CSkiaRenderer::Initialize(HWND hwnd) {
   if (!::IsWindow(hwnd) || m_bDrawing) {
      return false;
   }
   m_hwnd = hwnd;
   m_hDrawResult = ResizeSurface_();
   return SUCCEEDED(m_hDrawResult);
}

// PRINTING (not supported by this screen-only adapter yet)
bool CSkiaRenderer::OnBeginPrintJob(const wstring& sPrinterName) {
   (void)sPrinterName;
   return false;
}
void CSkiaRenderer::OnEndPrintJob() {}
bool CSkiaRenderer::OnPrintPageBegin(uint32_t argbBkg) {
   (void)argbBkg;
   return false;
}
bool CSkiaRenderer::OnPrintPageEnd(SIZE szePageDIPs) {
   (void)szePageDIPs;
   return false;
}

// DRAWING
void CSkiaRenderer::EraseBkgnd(uint32_t nRGB) {
   if (m_bDrawing && SUCCEEDED(m_hDrawResult) && m_pSurface) {
      m_pSurface->getCanvas()->clear(0xFF000000u | nRGB);
   }
}
void CSkiaRenderer::OnSize(LPARAM lParam) {
   (void)lParam; // Query the window rather than truncate dimensions to 16 bits.
   if (!m_bDrawing) {
      m_hDrawResult = ResizeSurface_();
   }
}
void CSkiaRenderer::BeginDraw() {
   if (m_bDrawing) {
      m_hDrawResult = E_UNEXPECTED;
      return;
   }
   m_bDrawing = true;
   m_hDrawResult = ResizeSurface_();
   if (SUCCEEDED(m_hDrawResult) && m_pSurface) {
      SkCanvas* pCanvas = m_pSurface->getCanvas();
      pCanvas->restoreToCount(1);
      pCanvas->resetMatrix();
      // Match the view's client-coordinate page placement and scroll offsets.
      // Applying another DPI scale here would also scale those offsets.
   }
}
HRESULT CSkiaRenderer::EndDraw(HDC hDC) {
   if (!m_bDrawing) {
      return E_UNEXPECTED;
   }
   m_bDrawing = false;
   if (FAILED(m_hDrawResult) || !m_pSurface) {
      return m_hDrawResult;
   }
   SkPixmap pixels;
   if (!m_pSurface->peekPixels(&pixels)) {
      return E_FAIL;
   }
   const bool bReleaseDC = hDC == nullptr;
   if (bReleaseDC) {
      hDC = ::GetDC(m_hwnd);
   }
   if (!hDC) {
      return E_FAIL;
   }
   BITMAPINFO bitmapInfo = {};
   bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bitmapInfo.bmiHeader.biWidth = pixels.width();
   bitmapInfo.bmiHeader.biHeight = -pixels.height(); //top-down BGRA
   bitmapInfo.bmiHeader.biPlanes = 1;
   bitmapInfo.bmiHeader.biBitCount = 32;
   bitmapInfo.bmiHeader.biCompression = BI_RGB;
   const int nResult = ::StretchDIBits(hDC, 0, 0, pixels.width(), pixels.height(),
      0, 0, pixels.width(), pixels.height(), pixels.addr(), &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
   if (bReleaseDC) {
      ::ReleaseDC(m_hwnd, hDC);
   }
   return nResult == 0 || nResult == GDI_ERROR ? E_FAIL : S_OK;
}
void CSkiaRenderer::SetTransform(const SkMatrix& mat) {
   if (m_bDrawing && SUCCEEDED(m_hDrawResult) && m_pSurface) {
      SkCanvas* pCanvas = m_pSurface->getCanvas();
      if (!mat.isFinite()) {
         m_hDrawResult = E_INVALIDARG;
         return;
      }
      pCanvas->setMatrix(mat);
   }
}
void CSkiaRenderer::DrawLine(float x1, float y1, float x2, float y2,
                            EnumLineStyles style, float width, uint32_t argb) {
   if (!m_bDrawing || FAILED(m_hDrawResult) || !m_pSurface) {
      return;
   }
   SkPaint paint;
   SetStroke_(paint, style, width, argb);
   m_pSurface->getCanvas()->drawLine(x1, y1, x2, y2, paint);
}
void CSkiaRenderer::DrawRect(float left, float top, float right, float bottom,
                            EnumLineStyles style, float width, uint32_t argb) {
   if (!m_bDrawing || FAILED(m_hDrawResult) || !m_pSurface) {
      return;
   }
   SkPaint paint;
   SetStroke_(paint, style, width, argb);
   m_pSurface->getCanvas()->drawRect(SkRect::MakeLTRB(left, top, right, bottom), paint);
}
void CSkiaRenderer::FillRect(float left, float top, float right, float bottom, uint32_t argb) {
   if (!m_bDrawing || FAILED(m_hDrawResult) || !m_pSurface) {
      return;
   }
   SkPaint paint;
   paint.setColor(0xFF000000u | (argb ? argb : m_argbText));
   m_pSurface->getCanvas()->drawRect(SkRect::MakeLTRB(left, top, right, bottom), paint);
}
void CSkiaRenderer::DrawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                                 float base_x, float base_y, float scale, uint32_t argb) {
   if (!m_bDrawing || FAILED(m_hDrawResult) || !m_pSurface || !count || !indices) {
      return;
   }
   SkTypeface* pTypeface = m_FontManager.GetTypeface(font_idx);
   if (!pTypeface || !isfinite(scale) || scale <= 0 ||
       !isfinite(m_fFontSizePts) || m_fFontSizePts <= 0) {
      m_hDrawResult = E_INVALIDARG;
      return;
   }
   const float fFontSizeDIPs = PTS2DIPS(m_fFontSizePts * scale);
   if (!isfinite(fFontSizeDIPs)) {
      m_hDrawResult = E_INVALIDARG;
      return;
   }
   SkFont font(sk_ref_sp(pTypeface), fFontSizeDIPs);
   font.setSubpixel(true);
   font.setLinearMetrics(true);
   font.setHinting(SkFontHinting::kNone);
   font.setEdging(SkFont::Edging::kAntiAlias);
   vector<SkScalar> vAdvances(count);
   font.getWidths({ indices, count }, { vAdvances.data(), count });
   vector<SkPoint> vPositions(count);
   float fPositionX = 0;
   for (uint32_t nIdx = 0; nIdx < count; ++nIdx) {
      vPositions[nIdx] = { fPositionX, 0 };
      fPositionX += vAdvances[nIdx];
   }
   SkPaint paint;
   paint.setAntiAlias(true);
   paint.setColor(0xFF000000u | (argb ? argb : m_argbText));
   m_pSurface->getCanvas()->drawGlyphs({ indices, count }, { vPositions.data(), count },
                                     { base_x, base_y }, font, paint);
}

// INTERNALS
HRESULT CSkiaRenderer::ResizeSurface_() {
   RECT rcClient;
   if (!::GetClientRect(m_hwnd, &rcClient)) {
      return E_FAIL;
   }
   const int nWidth = rcClient.right - rcClient.left;
   const int nHeight = rcClient.bottom - rcClient.top;
   if (nWidth <= 0 || nHeight <= 0) {
      if (m_pSurface) {
         m_pSurface->unref();
         m_pSurface = nullptr;
      }
      return S_OK;
   }
   if (m_pSurface && m_pSurface->width() == nWidth && m_pSurface->height() == nHeight) {
      return S_OK;
   }
   SkSurface* pSurface = SkSurfaces::Raster(
      SkImageInfo::Make(nWidth, nHeight, kBGRA_8888_SkColorType, kOpaque_SkAlphaType),
      static_cast<size_t>(nWidth) * 4, nullptr).release();
   if (!pSurface) {
      return E_OUTOFMEMORY;
   }
   if (m_pSurface) {
      m_pSurface->unref();
   }
   m_pSurface = pSurface;
   return S_OK;
}
void CSkiaRenderer::SetStroke_(SkPaint& paint, EnumLineStyles style, float fWidth, uint32_t nARGB) {
   paint.setAntiAlias(true);
   paint.setColor(0xFF000000u | (nARGB ? nARGB : m_argbText));
   paint.setStyle(SkPaint::kStroke_Style);
   paint.setStrokeWidth(fWidth);
   paint.setStrokeMiter(10.0f);
   const float fDashUnit = fWidth > 0 ? fWidth : 1.0f;
   switch (style) {
   case elsDash: {
      const SkScalar aIntervals[] = { 4 * fDashUnit, 2 * fDashUnit };
      paint.setPathEffect(SkDashPathEffect::Make(aIntervals, 0));
      break;
   }
   case elsDot: {
      // Preserve the baseline dot pattern and flat dash caps.
      const SkScalar aIntervals[] = { 0, 2 * fDashUnit };
      paint.setPathEffect(SkDashPathEffect::Make(aIntervals, 0));
      break;
   }
   case elsDashDot: {
      const SkScalar aIntervals[] = { 2 * fDashUnit, 2 * fDashUnit, 0, 2 * fDashUnit };
      paint.setPathEffect(SkDashPathEffect::Make(aIntervals, 0));
      break;
   }
   default:
      break;
   }
}
