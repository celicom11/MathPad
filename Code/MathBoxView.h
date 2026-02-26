#pragma once
#include "MathBoxHost/MathBoxHost.h"
#include "CfgReader.h"

class CMathBoxView : public CWindowImpl<CMathBoxView>,
                     public CZoomScrollImpl<CMathBoxView> {
//TYPES
//DATA
   SIZE                 m_szePageDIPs{ 794,1123};  // A4, in DIPs, 96 DIPs/inch
   SIZE                 m_szeBoxMarins{ 10,10 };   // in DIPs
   int32_t              m_nPageVertGap{ 20 };      // in DIPs
   uint32_t             m_rgbBkg{ 0x003F00 };      // view background color
   uint32_t             m_rgbPage{ 0xFFFFFF };     // page background color
   CPoint               m_ptSel;                   // Selection anchor
   CRect                m_rcSel;                   //
   const CCfgReader&    m_cfgr;                    // external
   CD2DRenderer&        m_d2dr;                    // external
   CMathBoxHost         m_mathBoxHost;
   vector<CRectF>       m_vLines;                  // cached line rects
   vector<int32_t>      m_vPageLines;              // first line index of each page

public:
   DECLARE_WND_CLASS(L"MathBoxView")
   CMathBoxView(const CCfgReader& cfgr, CD2DFontManager& d2dFontManager, CD2DRenderer& d2dr):
      m_cfgr(cfgr), m_d2dr(d2dr), m_mathBoxHost(d2dFontManager, m_d2dr){}

   BOOL PreTranslateMessage(MSG* pMsg)
   {
      pMsg;
      return FALSE;
   }
//WMs
   BEGIN_MSG_MAP(CMathBoxView)
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkg)
      MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
      MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
      MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
      MESSAGE_HANDLER(WM_PAINT, OnPaint)
      MESSAGE_HANDLER(WM_PRINTCLIENT, OnPaint)

      CHAIN_MSG_MAP(CZoomScrollImpl<CMathBoxView>)
   END_MSG_MAP()
//WM HANDLERS
   LRESULT OnGetDlgCode(UINT uMsg, WPARAM , LPARAM , BOOL& ) {
      uMsg;
      return DLGC_WANTARROWS;
   }
   LRESULT OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnMouseWheel(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
   LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
   LRESULT OnEraseBkg(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
      wParam; lParam; bHandled;
      return TRUE;//do nothing!
   }
   LRESULT OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//ATTS
   uint32_t PageCount() const { return (uint32_t)m_vPageLines.size(); }
   void SetPageSizeDIPs(const SIZE szePage) { m_szePageDIPs = szePage; }
   int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE) {
      return CWindowImpl<CMathBoxView>::SetScrollPos(nBar, nPos, bRedraw);
   }
   int GetScrollPos(int nBar) const { return CWindowImpl<CMathBoxView>::GetScrollPos(nBar); }
   bool SetMathContent(const string& sTeX);
   float FontSizePts() const { return m_mathBoxHost.FontSizePts(); }
   const SMathBoxError& LastError() { return m_mathBoxHost.LastError(); }
   uint32_t MathBoxAbiVersion() const { return m_mathBoxHost.AbiVersion(); }
//METHODS
   bool InitMathBox(OUT wstring& sErrorMsg);
   //CZoomScrollImpl overrides  
   void DoPaint(HDC) {} //do nothing!
   
   //  Clipboard copy 
   LRESULT OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      return 0;
   }
   LRESULT OnEraseBackground(UINT , WPARAM , LPARAM , BOOL& ) {
      return TRUE;//do nothing
   }
   // 1 inch = 96 DIPs
   // 1 inch = 1000 units (for margins)
   inline int DIPsToMarginUnits(long dips) { return long((dips / 96.0) * 1000.0 + 0.5); }
   inline int MarginUnitsToDIPs(long mu)   { return long((mu / 1000.0) * 96.0 + 0.5); }
   bool BeginPrintJob(const wstring& sPrinterName);
   bool PrintPage(UINT nPage);
   void EndPrintJob();
   CRect GetPrintPageMargins() {
      // Convert m_szeBoxMarins (DIPs) to 1/1000 inch units for the dialog
      return CRect(
         DIPsToMarginUnits(m_szeBoxMarins.cx), // left
         DIPsToMarginUnits(m_szeBoxMarins.cy), // top
         DIPsToMarginUnits(m_szeBoxMarins.cx), // right
         DIPsToMarginUnits(m_szeBoxMarins.cy)  // bottom
      );
   }
   void SetPrintPageDimensions(const RECT& rcMargins, POINT ptPaperSize) {
      // Convert from 1/1000 inch units to DIPs
      m_szeBoxMarins.cx = MarginUnitsToDIPs(rcMargins.left);
      m_szeBoxMarins.cy = MarginUnitsToDIPs(rcMargins.top);
      m_szePageDIPs.cx = MarginUnitsToDIPs(ptPaperSize.x);
      m_szePageDIPs.cy = MarginUnitsToDIPs(ptPaperSize.y);
   }
private:
   void DrawPage_(int nPageNum, const CRectF& rcfPage);
   SIZE CalcViewSize_() const;
   void ReadCfgSettings_();
   void OverrideSettings_(const string& sTeX);
};

