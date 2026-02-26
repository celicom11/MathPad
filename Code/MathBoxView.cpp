#include "stdafx.h"
#include "MathBoxView.h"
#include "CfgReader.h"
#include "MathBoxHost/D2DRenderer.h"

//STATIC HELPERS
namespace {
   void _TrimString(IN OUT string& sStr) {
      while (!sStr.empty() && isspace(sStr.front()))
         sStr.erase(sStr.begin());
      while (!sStr.empty() && isspace(sStr.back()))
         sStr.pop_back();
   }
   bool _ParseSize(const string& sVal, OUT int32_t& nSX, OUT int32_t& nSY) {
      nSX = nSY = 0;
      size_t nSep = sVal.find(',');
      if (nSep == string::npos) {
         //check for std paper sizes in Pts
         if(sVal == "A4" || sVal == "a4") {
            nSX = 595; nSY = 842;
            return true;
         }
         if (sVal == "A5" || sVal == "a5") {
            nSX = 420; nSY = 595;
            return true;
         }
         if (sVal == "Letter" || sVal == "letter") {
            nSX = 612; nSY = 792;
            return true;
         }
         if (sVal == "Legal" || sVal == "legal") {
            nSX = 612; nSY = 1008;
            return true;
         }
         return false;
      }
      string sSX = sVal.substr(0, nSep);
      string sSY = sVal.substr(nSep + 1);
      _TrimString(sSX);
      _TrimString(sSY);
      if (sSX.empty() || sSY.empty())
         return false;
      nSX = std::stoi(sSX);
      nSY = std::stoi(sSY);
      return true;
   }
   //<number>[unit], unit can be "pt","in" or "mm" default is pt
   bool _ParseLengthPts(const string& sVal, OUT float& fPts) {
      fPts = 0;
      size_t nSep = sVal.find_first_not_of("0123456789.");
      string sNum = sVal.substr(0, nSep);
      string sUnit = sVal.substr(nSep);
      _TrimString(sNum);
      _TrimString(sUnit);
      if (sNum.empty())
         return false;
      try {
         fPts = std::stof(sNum);
      }
      catch (const std::exception&) {
         return false;
      }
      if (sUnit.empty() || sUnit == "pt")
         return true;
      if (sUnit == "in") {
         fPts *= 72.0f;
         return true;
      }
      if (sUnit == "mm") {
         fPts *= 72.0f / 25.4f;
         return true;
      }
      return false;// unknown unit
   }
   // #RRGGBB or known color name
   bool _ParseColor(const string& sVal, OUT uint32_t& rgb) {
      rgb = 0;
      if (sVal.empty())
         return false;
      if (sVal[0] == '#') {
         try {
            rgb = std::stoul(sVal.substr(1), nullptr, 16);
            return true;
         }
         catch (const std::exception&) {
            return false;
         }
      }
      //check for known 19 color names
      // black, white, red, green, blue, cyan, magenta, yellow, gray, darkgray, lightgray, 
      // brown, lime, olive, orange, pink, purple, teal, violet
      static const map<string, uint32_t> mColors = {
         {"black", 0x000000},
         {"white", 0xFFFFFF},
         {"red", 0xFF0000},
         {"green", 0x00FF00},
         {"blue", 0x0000FF},
         {"cyan", 0x00FFFF},
         {"magenta", 0xFF00FF},
         {"yellow", 0xFFFF00},
         {"gray", 0x808080},
         {"darkgray", 0x404040},
         {"lightgray", 0xC0C0C0},
         {"brown", 0xA52A2A},
         {"lime", 0x00FF00},
         {"olive", 0x808000},
         {"orange", 0xFFA500},
         {"pink", 0xFFC0CB},
         {"purple", 0x800080},
         {"teal", 0x008080},
         {"violet", 0xEE82EE}
      };
      auto it = mColors.find(sVal);
      if (it != mColors.end()) {
         rgb = it->second;
         return true;
      }
      return false;// unknown color name
   }
}
//WM HANDLERS
LRESULT CMathBoxView::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
   wParam;
   if (lParam)
      m_d2dr.OnSize(lParam);
   bHandled = FALSE;
   return 0;
}
LRESULT CMathBoxView::OnMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
   lParam; // unused - centering on viewport, not mouse
   short nDelta = short(wParam >> 16) / 40;
   USHORT nFlags = LOWORD(wParam);
   const bool bCtrl = nFlags & MK_CONTROL;
   if (bCtrl) {
      const float fZoom = CZoomScrollImpl<CMathBoxView>::GetZoomScale();
      float fZoomNew = fZoom;
      if (nDelta > 0) {
         fZoomNew = fZoom + 0.05f;
         if (fZoomNew > CZoomScrollImpl<CMathBoxView>::GetZoomScaleMax())
            fZoomNew = CZoomScrollImpl<CMathBoxView>::GetZoomScaleMax();
      }
      else if (nDelta < 0) {
         fZoomNew = fZoom - 0.05f;
         if (fZoomNew < CZoomScrollImpl<CMathBoxView>::GetZoomScaleMin())
            fZoomNew = CZoomScrollImpl<CMathBoxView>::GetZoomScaleMin();
      }
      if (fZoomNew != fZoom && fZoomNew > 0 && fZoomNew < 11.0f) {
         CRect rcClient;
         GetClientRect(&rcClient);

         // Calculate viewport center in document coordinates (before zoom)
         int nScrollX = GetScrollPos(SB_HORZ);
         int nScrollY = GetScrollPos(SB_VERT);
         
         float fDocX = (nScrollX + rcClient.Width() / 2.0f) / fZoom;
         float fDocY = (nScrollY + rcClient.Height() / 2.0f) / fZoom;

         // Update zoom scale
         CZoomScrollImpl<CMathBoxView>::SetZoomScale(fZoomNew);
         SIZE szeView = CalcViewSize_();
         CZoomScrollImpl<CMathBoxView>::SetScrollSize(szeView, FALSE, FALSE);

         // Calculate new scroll position to keep center point at viewport center
         int nNewScrollX = (int)(fDocX * fZoomNew - rcClient.Width() / 2.0f);
         int nNewScrollY = (int)(fDocY * fZoomNew - rcClient.Height() / 2.0f);

         // Clamp scroll positions to valid range
         SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE };
         GetScrollInfo(SB_HORZ, &si);
         nNewScrollX = max(0, min(nNewScrollX, si.nMax - (int)si.nPage + 1));
         
         GetScrollInfo(SB_VERT, &si);
         nNewScrollY = max(0, min(nNewScrollY, si.nMax - (int)si.nPage + 1));

         // Set new scroll positions
         SetScrollPos(SB_HORZ, nNewScrollX, TRUE);
         SetScrollPos(SB_VERT, nNewScrollY, TRUE);

         // Redraw
         Invalidate(FALSE);
      }
   }
   else
      bHandled = FALSE;
   return 0;
}
LRESULT CMathBoxView::OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
   if (wParam >= VK_PRIOR && wParam <= VK_DOWN) {// keybd scrolling
      switch (wParam) {
      case VK_PRIOR: CZoomScrollImpl::ScrollPageUp(); break;
      case VK_NEXT: CZoomScrollImpl::ScrollPageDown(); break;
      case VK_END: CZoomScrollImpl::ScrollBottom(); break;
      case VK_HOME: CZoomScrollImpl::ScrollTop(); break;
      case VK_LEFT: CZoomScrollImpl::ScrollLineLeft(); break;
      case VK_UP: CZoomScrollImpl::ScrollLineUp(); break;
      case VK_RIGHT: CZoomScrollImpl::ScrollLineRight(); break;
      case VK_DOWN: CZoomScrollImpl::ScrollLineDown(); break;
      }
   }
   else
      bHandled = FALSE;
   return 0;
}
LRESULT CMathBoxView::OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
   wParam; lParam; bHandled;

   // Get scroll position for correct offset
   int nScrollX = GetScrollPos(SB_HORZ);
   int nScrollY = GetScrollPos(SB_VERT);

   m_d2dr.BeginDraw();
   // Fill the view background
   m_d2dr.EraseBkgnd(m_rgbBkg);

   CRect rcClient;
   GetClientRect(&rcClient);
   const float fZoom = CZoomScrollImpl<CMathBoxView>::GetZoomScale();
   const float fPageSX = m_szePageDIPs.cx * fZoom;
   const float fPageSY = m_szePageDIPs.cy * fZoom;
   const float fPageHeightWithGap = fPageSY + m_nPageVertGap;
   const int nVisible0 = (int)max(0, (nScrollY - m_nPageVertGap) / fPageHeightWithGap);
   const int nVisible1 = (int)min(m_vPageLines.size() - 1, 
                              (nScrollY - m_nPageVertGap + rcClient.Height()) / fPageHeightWithGap);
   const float fLeft = rcClient.Width() > fPageSX ? (rcClient.Width() - fPageSX) / 2 : 0;
   // Draw each page, centered horizontally
   for (int nPageIdx = nVisible0; nPageIdx <= nVisible1; ++nPageIdx) {
      CRectF rcfPage;
      rcfPage.left = fLeft - nScrollX;
      rcfPage.top = m_nPageVertGap + fPageHeightWithGap * nPageIdx - nScrollY;
      rcfPage.right = rcfPage.left + fPageSX;
      rcfPage.bottom = rcfPage.top + fPageSY;
      DrawPage_(nPageIdx, rcfPage);
   }

   HRESULT hr = m_d2dr.EndDraw();
   if (FAILED(hr)) {
      wchar_t errorMsg[256];
      swprintf_s(errorMsg, L"EndDraw failed with HRESULT: 0x%08X", hr);
      ::MessageBoxW(this->GetParent(), errorMsg, L"Render Error", MB_OK);
   }
   ::ValidateRect(*this, nullptr);
   return 0;
}
//METHODS
bool CMathBoxView::InitMathBox(OUT wstring& sErrorMsg) {
   // MathBox settings
   wstring wsMacrosFile = L"Macros.mth"; //default
   m_cfgr.GetSVal(L"MB_Macros", wsMacrosFile);
   // Load MathBoxLib
   if (!m_mathBoxHost.LoadMathBoxLib()) {
      string sMBError = m_mathBoxHost.LastError().sError;
      if (!sMBError.empty()) {
         vector<wchar_t> vError(sMBError.size() + 1);
         MultiByteToWideChar(CP_ACP, 0, sMBError.c_str(), -1, vError.data(), (int)vError.size());
         sErrorMsg.assign(vError.data(), vError.size() - 1);
      }
      else
         sErrorMsg = L"Could not load MathBoxLib";
      _ASSERT_RET(0, false);
   }
   // Load macros from file
   if (!wsMacrosFile.empty()) {
      //read file
      string sMacros, sMacrosFile;
      //wsMacrosFile -> sMacrosFile
      {
         vector<char> vBuf((wsMacrosFile.size() + 1) * sizeof(wchar_t));
         int nChars = WideCharToMultiByte(CP_ACP, 0, wsMacrosFile.c_str(), -1, vBuf.data(), (int)vBuf.size(), NULL, NULL);
         sMacrosFile.assign(vBuf.data(), nChars - 1);
      }
      ifstream ifsFile(sMacrosFile, std::ios::in | std::ios::binary);
      if (!ifsFile) {
         sErrorMsg = L"Failed to read macros file: " + wsMacrosFile;
         return false;
      }
      sMacros = string(istreambuf_iterator<char>(ifsFile), istreambuf_iterator<char>());
      if (!m_mathBoxHost.AddMacros(sMacros, sMacrosFile)) {
         sErrorMsg = L"Failed to add macros file: " + wsMacrosFile;
         return false;
      }
   }

   return true;
}
void CMathBoxView::ReadCfgSettings_() {
   // View settings
   m_cfgr.GetHVal(L"ColorBkg", m_rgbBkg);

   m_cfgr.GetSize(L"PageSizePts", m_szePageDIPs.cx, m_szePageDIPs.cy);
   m_szePageDIPs.cx = F2NEAREST(PTS2DIPS(m_szePageDIPs.cx));
   m_szePageDIPs.cy = F2NEAREST(PTS2DIPS(m_szePageDIPs.cy));

   m_cfgr.GetSize(L"PageMarginsPts", m_szeBoxMarins.cx, m_szeBoxMarins.cy);
   m_szeBoxMarins.cx = F2NEAREST(PTS2DIPS(m_szeBoxMarins.cx));
   m_szeBoxMarins.cy = F2NEAREST(PTS2DIPS(m_szeBoxMarins.cy));

   m_cfgr.GetNVal(L"PageVertGapPts", m_nPageVertGap);
   m_nPageVertGap = F2NEAREST(PTS2DIPS(m_nPageVertGap));

   // MathBox colors and font
   uint32_t rgbText{ 0x004F00 };
   m_cfgr.GetHVal(L"MB_ColorText", rgbText);
   m_mathBoxHost.SetClrText(rgbText);
   m_d2dr.SetTextColor(rgbText);

   m_cfgr.GetHVal(L"MB_ColorBkg", m_rgbPage);
   m_mathBoxHost.SetClrBkg(m_rgbPage);

   uint32_t rgbSel{ 0xFF4FFF };
   m_cfgr.GetHVal(L"MB_ColorSelect", rgbSel);
   m_mathBoxHost.SetClrSel(rgbSel); 

   float fFontSizePts{ 24.0f };
   m_cfgr.GetFVal(L"MB_FontSizePts", fFontSizePts);
   m_mathBoxHost.SetFontSizePts(fFontSizePts);

   // Recompute max box width from updated page/margin sizes
   m_mathBoxHost.SetMaxWidth(DIPS2EM(fFontSizePts, m_szePageDIPs.cx - 2 * m_szeBoxMarins.cx));
}
bool CMathBoxView::SetMathContent(const string& sTeX) {
   m_vPageLines.resize(1, 0); //first line of first page
   m_vLines.clear();
   //parse first %%key=value lines to override settings to document specific
   ReadCfgSettings_();
   OverrideSettings_(sTeX);
   if( !m_mathBoxHost.Parse(sTeX, m_vLines) )
      return false;
   //set pages layout
   float fLineY = m_vLines.front().top;
   const int32_t nInnerPageHeight = m_szePageDIPs.cy - 2 * m_szeBoxMarins.cy;
   for (int32_t nLine = 0; nLine < (int32_t)m_vLines.size(); ++ nLine) {
      const CRectF& rcfLine = m_vLines[nLine];
      if (rcfLine.Height() > nInnerPageHeight) {
         //line too tall/does not fit the page, todo: report error
         return false;
      }
      //todo: check line's width against page inner width
      if (rcfLine.bottom - fLineY > nInnerPageHeight) {
         //start new page
         fLineY = rcfLine.top;
         m_vPageLines.push_back(nLine);
      }
   }
   SIZE szeView = CalcViewSize_();
   CZoomScrollImpl<CMathBoxView>::SetScrollSize(szeView);
   // SetScrollSize resets line/page to defaults; set finer line step after
   const int nLineStep = max(1, m_szePageDIPs.cy / 20); // 1/20th of a page height
   CZoomScrollImpl<CMathBoxView>::SetScrollLine(nLineStep, nLineStep);
   return true;
}
//PRINTING
bool CMathBoxView::BeginPrintJob(const wstring& sPrinterName) {
   return m_d2dr.OnBeginPrintJob(sPrinterName);
}
void CMathBoxView::EndPrintJob() {
   m_d2dr.OnEndPrintJob();
}

bool CMathBoxView::PrintPage(UINT nPage) {
   if (nPage >= (UINT)m_vPageLines.size())
      return false;
   bool bOk = m_d2dr.OnPrintPageBegin(m_rgbPage);
   if(!bOk)
      return false;
   // Draw all lines for this page
   int nLineStart = m_vPageLines[nPage];
   int nLineEnd = (nPage + 1 < (UINT)m_vPageLines.size()) ?
                  m_vPageLines[nPage + 1] - 1 : (UINT)m_vLines.size() - 1;
   float left = (float)m_szeBoxMarins.cx;
   float top = (float)m_szeBoxMarins.cy;
   m_mathBoxHost.DrawLines(left, top, nLineStart, nLineEnd, 1.0f);
   return m_d2dr.OnPrintPageEnd(m_szePageDIPs);
}
//PRIVATE METHODS
void CMathBoxView::DrawPage_(int nPageNum, const CRectF& rcfPage) {
   // Draw page background
   m_d2dr.FillRect(rcfPage.left, rcfPage.top, rcfPage.right, rcfPage.bottom, m_rgbPage);
   // Draw border: test
   m_d2dr.DrawRect(rcfPage.left, rcfPage.top, rcfPage.right, rcfPage.bottom,
                  elsSolid, 1.0f, 0xFF808080);
   // Draw page shadow (future)
   // Draw content
   const float fZoom = CZoomScrollImpl<CMathBoxView>::GetZoomScale();
   const float fPageLeftInner = rcfPage.left + m_szeBoxMarins.cx * fZoom;
   const float fPageTopInner = rcfPage.top + m_szeBoxMarins.cy * fZoom;
   const int32_t nLineStart = m_vPageLines[nPageNum];
   const int32_t nLineEnd = ((nPageNum + 1 < (int32_t)m_vPageLines.size()) ?
                            m_vPageLines[nPageNum + 1] : 
                            (int32_t)m_vLines.size()) - 1; //inclusive
   m_mathBoxHost.DrawLines(fPageLeftInner, fPageTopInner, nLineStart, nLineEnd, fZoom);
}
SIZE CMathBoxView::CalcViewSize_() const {
   SIZE szeView;
   const float fZoom = CZoomScrollImpl<CMathBoxView>::GetZoomScale();
   szeView.cx = m_szePageDIPs.cx;
   const long lPages = (long)m_vPageLines.size();
   szeView.cy = lPages * m_szePageDIPs.cy + (lPages + 1) * int(m_nPageVertGap / fZoom);
   return szeView;
}
void CMathBoxView::OverrideSettings_(const string& sTeX) {
   //parse first %%key=value lines to override settings to document specific
   istringstream iss(sTeX);
   map<string, string> mapDict;
   string sLine;
   while(std::getline(iss, sLine)) {
      if (sLine.size() < 2 || sLine[0] != '%' || sLine[1] != '%')
         break;
      size_t nEq = sLine.find('=');
      if (nEq != string::npos) {
         string sKey = sLine.substr(2, nEq - 2);
         _TrimString(sKey);

         string sVal = sLine.substr(nEq + 1);
         size_t nCmnt = sVal.find('%');
         if (nCmnt != string::npos) {
            sVal = sVal.substr(0, nCmnt);
         }
         _TrimString(sVal);
         mapDict[sKey] = sVal;
      }
   }
   if(mapDict.empty())
      return; //ntd
   auto itPair = mapDict.find("paper");
   if (itPair != mapDict.end()) {
      int32_t nSX, nSY;
      if(_ParseSize(itPair->second, nSX, nSY) && nSX > 10 && nSY > 10) {
         m_szePageDIPs.cx = F2NEAREST(PTS2DIPS(nSX));
         m_szePageDIPs.cy = F2NEAREST(PTS2DIPS(nSY));
      }
   }
   itPair = mapDict.find("margins");
   if (itPair != mapDict.end()) {
      int32_t nSX, nSY;
      if (_ParseSize(itPair->second, nSX, nSY)) {
         int32_t nXDIPs = F2NEAREST(PTS2DIPS(nSX));
         int32_t nYDIPs = F2NEAREST(PTS2DIPS(nSY));
         if (2 * nXDIPs < m_szePageDIPs.cx && 2 * nYDIPs < m_szePageDIPs.cy) {
            m_szeBoxMarins.cx = nXDIPs;
            m_szeBoxMarins.cy = nYDIPs;
         }
      }
   }
   //font
   float fFontSizePts = m_mathBoxHost.FontSizePts(), fNew=0.0f;
   itPair = mapDict.find("fontsize");
   if (itPair != mapDict.end() && _ParseLengthPts(itPair->second, fNew) && fNew >= 8.0f && fNew <= 72.0f) {
      fFontSizePts = fNew;
      m_mathBoxHost.SetFontSizePts(fFontSizePts);
   }
   m_mathBoxHost.SetMaxWidth(DIPS2EM(fFontSizePts, m_szePageDIPs.cx - 2 * m_szeBoxMarins.cx));

   //colors
   itPair = mapDict.find("pagecolor");
   uint32_t rgbColor = 0;
   if (itPair != mapDict.end() && _ParseColor(itPair->second, rgbColor)) {
      m_rgbPage = rgbColor;
      m_mathBoxHost.SetClrBkg(m_rgbPage);
   }

   itPair = mapDict.find("textcolor");
   if (itPair != mapDict.end() && _ParseColor(itPair->second, rgbColor)) {
      m_mathBoxHost.SetClrText(rgbColor);
      m_d2dr.SetTextColor(rgbColor);
   }
}