#pragma once
#define WINVER				_WIN32_WINNT_WIN10
#define _WIN32_WINNT    _WIN32_WINNT_WIN10
#define _WIN32_IE			_WIN32_IE_IE110

//SKIA
#pragma warning(push)
#pragma warning( disable: 4244 )
#pragma warning( disable: 4267 )
#include "include/core/SkRefCnt.h"
#include "include/core/SkData.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkMatrix.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/ports/SkFontMgr_empty.h"
#include "include/core/SkPath.h"

#pragma warning(pop)

#define _WTL_USE_CSTRING
//WTL
#include <atlbase.h>
#include <atlapp.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlscrl.h>
#include <atldlgs.h>
#include <atlmisc.h>
#include <atlctrlw.h>
#include <atlprint.h>
#include <atlfind.h>
#include <atlcrack.h>
#define ARGB2COLORREF(x) ( ((0xFF&(x))<<16)|((0xFF0000&(x))>>16)|(0x00FF00&(x)) )

extern CAppModule _Module;

#include <atltypes.h>
#include <atlwin.h>
//STL
#include <vector>
#include <cmath>
#include <string>
#include <fstream>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <filesystem>
#include <utility>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <type_traits>
using namespace std;



#define _ASSERT_RET(expr,retval) {if(!(expr)){_ASSERT(0);return retval;}}
#define CHECK_HR(hr) {if(FAILED(hr)){_ASSERT(0);return hr;}}
template <class T> void SafeRelease(T** ppT) {
   if (*ppT) {
      (*ppT)->Release();
      *ppT = nullptr;
   }
}

//MSCC 6.0 manifest
#if defined _M_IX86
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else 
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
