
#if defined(_MSC_VER) && defined(_WIN32_WCE) && (_WIN32_WCE < 0x800) && !defined(__INTRIN_H_) && !defined(_INTRIN)
#define _STDINT

#ifdef _M_ARM
#include <armintr.h>
#if (_WIN32_WCE >= 0x700) && defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__)
#include <arm_neon.h>
#endif
#endif // _M_ARM

#endif
