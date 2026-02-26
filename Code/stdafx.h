#define WINVER				_WIN32_WINNT_WIN10
#define _WIN32_WINNT    _WIN32_WINNT_WIN10
#define _WIN32_IE			_WIN32_IE_IE110

#define _WTL_USE_CSTRING

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

#include <dwrite.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <d2d1_1helper.h>
#include <dxgi1_2.h>
#include <wincodec.h>

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
using namespace std;

#define _ASSERT_RET(expr,retval) {if(!(expr)){_ASSERT(0);return retval;}}
#define CHECK_HR(hr) {if(FAILED(hr)){_ASSERT(0);return hr;}}
template <class T> void SafeRelease(T** ppT) {
   if (*ppT) {
      (*ppT)->Release();
      *ppT = nullptr;
   }
}

//linker
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")
#pragma comment(lib, "windowscodecs")

//MSCC 6.0 manifest
#if defined _M_IX86
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else 
# pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
