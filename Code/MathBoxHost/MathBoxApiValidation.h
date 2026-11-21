#pragma once
#include "MathBox_CAPI.h"

inline bool IsMathBoxApiCompatible(const MBI_API* pAPI) {
   // Inspect the common prefix before accessing any ABI-2 function pointer.
   return pAPI && pAPI->size_bytes >= sizeof(MBI_API) &&
      pAPI->abi_version == MB_VERSION_MAJOR &&
      pAPI->createEngine && pAPI->destroyEngine && pAPI->destroyMathItem &&
      pAPI->getLastError && pAPI->addMacros && pAPI->parseLatex &&
      pAPI->mathItemLineCount && pAPI->mathItemDraw && pAPI->mathItemDrawLines &&
      pAPI->mathItemSelect && pAPI->mathItemGetBox && pAPI->mathItemGetLineBox;
}
