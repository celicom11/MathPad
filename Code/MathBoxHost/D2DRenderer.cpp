#include "stdafx.h"
#include "D2DRenderer.h"
#include <DocumentTarget.h>

namespace //static helpers
{
   // D3D11 / DXGI / D2D 1.1 (NEW - for print support)
   ID3D11Device*        _pD3D11Device{ nullptr };
   ID2D1Device*         _pD2DDevice{ nullptr };
   ID2D1DeviceContext*  _pDeviceContext{ nullptr };
   IWICImagingFactory*  _pWicFactory = nullptr;
   IPrintDocumentPackageTarget* _pDocumentTarget = nullptr;
   ID2D1CommandList*    _pCmdList = nullptr;
   ID2D1PrintControl*   _pPrintControl = nullptr;
   IDXGIDevice*         _pDXGIDevice{ nullptr };
}
HRESULT CD2DRenderer::Initialize(HWND hwnd) {
   HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);
   CHECK_HR(hr);
   hr = ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), 
                              reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
   CHECK_HR(hr);
   //WCHAR wszDir[MAX_PATH] = { 0 };
   //GetCurrentDirectoryW(_countof(wszDir), wszDir);
   //hr = g_LMFManager.Init(wszDir, pDWriteFactory);
   //CHECK_HR(hr);
   RECT rc;
   GetClientRect(hwnd, &rc);
   D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);
   hr = m_pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                              D2D1::HwndRenderTargetProperties(hwnd, size), 
                                              &m_pHWNDRenderTarget);
   CHECK_HR(hr);
   m_pRenderTarget = m_pHWNDRenderTarget;
   m_pHWNDRenderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
   hr = m_pHWNDRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pSolidBrush);
   CHECK_HR(hr);
   //add stroke styles
   //elsDash,elsDot,elsDashDot
   m_vStrokeStyles.resize(3, nullptr);
   static const float _aDashes[] = { 4.0f, 2.0f };
   D2D1_STROKE_STYLE_PROPERTIES dssp;
   dssp = D2D1::StrokeStyleProperties(
      D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT,
      D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER,
      10.0f, D2D1_DASH_STYLE_CUSTOM, 0.0f);
   hr = m_pD2DFactory->CreateStrokeStyle(dssp, _aDashes, _countof(_aDashes), &m_vStrokeStyles[0]);
   CHECK_HR(hr);
   dssp.dashStyle = D2D1_DASH_STYLE_DOT;
   hr = m_pD2DFactory->CreateStrokeStyle(dssp, nullptr, 0, &m_vStrokeStyles[1]);
   CHECK_HR(hr);
   dssp.dashStyle = D2D1_DASH_STYLE_DASH_DOT;
   hr = m_pD2DFactory->CreateStrokeStyle(dssp, nullptr, 0, &m_vStrokeStyles[2]);
   CHECK_HR(hr);

   return S_OK;
}
//PRINTING/BITMAP TARGETS
bool CD2DRenderer::OnBeginPrintJob(const wstring& sPrinterName) {
   SafeRelease(&_pPrintControl);
   SafeRelease(&_pDocumentTarget);
   SafeRelease(&_pWicFactory);
   SafeRelease(&_pCmdList);
   SafeRelease(&_pDeviceContext);

   SafeRelease(&_pD2DDevice);
   SafeRelease(&_pDXGIDevice);
   SafeRelease(&_pD3D11Device);
   // Create Direct3D 11 device
   HRESULT hRes = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
      D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &_pD3D11Device, nullptr, nullptr);
   _ASSERT_RET(SUCCEEDED(hRes), false);
   // Get DXGI device
   hRes = _pD3D11Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&_pDXGIDevice);
   _ASSERT_RET(SUCCEEDED(hRes), false);

   // Create Direct2D device using ID2D1Factory1
   ID2D1Factory1* pFactory1 = nullptr;
   hRes = m_pD2DFactory->QueryInterface(__uuidof(ID2D1Factory1), (void**)&pFactory1);
   _ASSERT_RET(SUCCEEDED(hRes), false);
   hRes = pFactory1->CreateDevice(_pDXGIDevice, &_pD2DDevice);
   SafeRelease(&pFactory1);
   _ASSERT_RET(SUCCEEDED(hRes), false);

   // Create WIC factory
   hRes = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, 
                           IID_PPV_ARGS(&_pWicFactory));
   _ASSERT_RET(SUCCEEDED(hRes), false);
   // Create the document package target for print job
   IPrintDocumentPackageTargetFactory* pPrintDocTargetFactory = nullptr;
   hRes = ::CoCreateInstance(__uuidof(PrintDocumentPackageTargetFactory),
      nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pPrintDocTargetFactory)
   );
   _ASSERT_RET(SUCCEEDED(hRes), false);
   hRes = pPrintDocTargetFactory->CreateDocumentPackageTargetForPrintJob(
      sPrinterName.c_str(),                       // printer name
      L"MathPad Printing Job",                    // job name
      nullptr,                                    // job output stream; when nullptr, send to printer
      nullptr,                                    // job print ticket
      &_pDocumentTarget                           // result IPrintDocumentPackageTarget object
   );
   SafeRelease(&pPrintDocTargetFactory);
   _ASSERT_RET(SUCCEEDED(hRes), false);
   // Create device context from global Direct2D device
   hRes = _pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_pDeviceContext);
   _ASSERT_RET(SUCCEEDED(hRes), false);
   // Create PrintControl ONCE for the entire job
   D2D1_PRINT_CONTROL_PROPERTIES printControlProperties = {};
   printControlProperties.rasterDPI = 300.0f;
   printControlProperties.fontSubset = D2D1_PRINT_FONT_SUBSET_MODE_DEFAULT;
   printControlProperties.colorSpace = D2D1_COLOR_SPACE_SRGB;

   hRes = _pD2DDevice->CreatePrintControl(
      _pWicFactory,
      _pDocumentTarget,
      printControlProperties,
      &_pPrintControl
   );
   _ASSERT_RET(SUCCEEDED(hRes), false);
   //replace bkg brush!
   SafeRelease(&m_pSolidBrush);
   hRes = _pDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pSolidBrush);
   _ASSERT_RET(SUCCEEDED(hRes), false);

   m_pRenderTarget = _pDeviceContext;
   return true;
}
void CD2DRenderer::OnEndPrintJob() {
   //replace bkg brush!
   SafeRelease(&m_pSolidBrush);
   HRESULT hRes = m_pHWNDRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &m_pSolidBrush);
   _ASSERT(SUCCEEDED(hRes));
   if (_pPrintControl) {
      hRes = _pPrintControl->Close();
      _ASSERT(SUCCEEDED(hRes));
   }
   SafeRelease(&_pPrintControl);
   SafeRelease(&_pDocumentTarget);
   SafeRelease(&_pWicFactory);
   SafeRelease(&_pCmdList);
   SafeRelease(&_pDeviceContext);

   SafeRelease(&_pD2DDevice);
   SafeRelease(&_pDXGIDevice);
   SafeRelease(&_pD3D11Device);
   m_pRenderTarget = m_pHWNDRenderTarget;
}

bool CD2DRenderer::OnPrintPageBegin(uint32_t argbBkg) {
   _ASSERT_RET(m_pRenderTarget, false);
   // Create command list
   SafeRelease(&_pCmdList);
   HRESULT hRes = _pDeviceContext->CreateCommandList(&_pCmdList);
   _ASSERT_RET(SUCCEEDED(hRes), false);
   // Record drawing to command list
   _pDeviceContext->SetTarget(_pCmdList);

   BeginDraw();
   EraseBkgnd(argbBkg);
   return true;
}
bool CD2DRenderer::OnPrintPageEnd(SIZE szePageDIPs) {
   _ASSERT_RET(m_pRenderTarget, false);
   _ASSERT_RET(_pCmdList, false);
   _ASSERT_RET(_pPrintControl, false);

   EndDraw();
   HRESULT hRes = _pCmdList->Close();
   _ASSERT_RET(SUCCEEDED(hRes), false);
   // Add page with command list
   hRes = _pPrintControl->AddPage(_pCmdList,
      D2D1::SizeF((float)szePageDIPs.cx, (float)szePageDIPs.cy), nullptr);
   _ASSERT_RET(SUCCEEDED(hRes), false);

   SafeRelease(&_pCmdList);
   return true;
}

//DRAWING
void CD2DRenderer::DrawLine(float x1, float y1, float x2, float y2, EnumLineStyles style, float width, 
                            uint32_t argb) {
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));
   ID2D1StrokeStyle* pSStyle =
      style > elsSolid && style <= elsDashDot ? m_vStrokeStyles[style - 1] : nullptr;
   m_pRenderTarget->DrawLine({x1,y1}, { x2, y2 }, m_pSolidBrush, width, pSStyle);
}
void CD2DRenderer::DrawRect(float left, float top, float right, float bottom, EnumLineStyles style,
                            float width, uint32_t argb) {
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));
   ID2D1StrokeStyle* pSStyle =
      style > elsSolid && style <= elsDashDot ? m_vStrokeStyles[style - 1] : nullptr;
   D2D1_RECT_F rcd2d{ left, top, right,bottom};
   m_pRenderTarget->DrawRectangle(rcd2d, m_pSolidBrush, width, pSStyle);
}
void CD2DRenderer::FillRect(float left, float top, float right, float bottom, uint32_t argb) {
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));
   D2D1_RECT_F rcd2d{ left, top, right,bottom };
   m_pRenderTarget->FillRectangle(rcd2d, m_pSolidBrush);
}
void CD2DRenderer::DrawGlyphRun(int32_t font_idx, uint32_t count, const uint16_t* indices,
                                 float base_x, float base_y, float scale, uint32_t argb) {
   _ASSERT_RET(count && indices, );
   IDWriteFontFace* pFontFace = m_FontProvider.GetDWFont(font_idx);
   _ASSERT_RET(pFontFace, );
   m_pSolidBrush->SetColor(D2D1::ColorF(argb ? argb : m_argbText));

   DWRITE_GLYPH_RUN glyphRun = {};
   glyphRun.fontFace = pFontFace;
   glyphRun.fontEmSize = PTS2DIPS(m_FontProvider.GetFontSizePts() * scale);
   glyphRun.glyphCount = count;
   glyphRun.glyphIndices = indices;
   glyphRun.glyphAdvances = nullptr; //use font advances
   glyphRun.glyphOffsets = nullptr; //offsets, not used
   glyphRun.isSideways = FALSE;
   glyphRun.bidiLevel = 0;
   m_pRenderTarget->DrawGlyphRun({ base_x,base_y }, &glyphRun, m_pSolidBrush);
}
