#pragma once

class CColorBtn : public CWindowImpl<CColorBtn, CButton>,
                  public CCustomDraw<CColorBtn> {
//DATA
   COLORREF m_rgb{ 0xFFFFFF };
public:
//CTOR/DTOR
   CColorBtn() = default;
 //ATTS
   void SetColor(uint32_t rgb) {
      m_rgb = rgb;
      if(m_hWnd)
         Invalidate();
   }
   uint32_t GetColor() const { return m_rgb; }
//METHODS
   bool Subclass(HWND hWnd) {
      return CWindowImpl<CColorBtn, CButton>::SubclassWindow(hWnd);
   }
//Message map
   BEGIN_MSG_MAP_EX(CColorBtn)
      // Handle custom draw notifications
      CHAIN_MSG_MAP_ALT(CCustomDraw<CColorBtn>, 1)
      DEFAULT_REFLECTION_HANDLER()
   END_MSG_MAP()
   // Custom Draw Handler
   DWORD OnPrePaint(int /*idCtrl*/, LPNMCUSTOMDRAW ) {
      return CDRF_NOTIFYPOSTPAINT;
   }

   DWORD OnPostPaint(int /*idCtrl*/, LPNMCUSTOMDRAW lpNMCD) {
      CDCHandle dc(lpNMCD->hdc);
      COLORREF clrBtn = ARGB2COLORREF(m_rgb);
      CBrush br; br.CreateSolidBrush(clrBtn);
      CRect rcColor;
      GetClientRect(&rcColor);
      rcColor.DeflateRect(5, 5);//todo
      dc.FillRect(&rcColor, br);
      //focus rect
      dc.DrawFocusRect(&rcColor);
      return CDRF_SKIPDEFAULT; // We did all the drawing
   }
};