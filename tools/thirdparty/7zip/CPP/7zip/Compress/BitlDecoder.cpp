// BitlDecoder.cpp

#include "StdAfx.h"

#include "BitlDecoder.h"

namespace NBitl {

#if defined(Z7_BITL_USE_REVERSE_BITS_TABLE)

MY_ALIGN(64)
Byte kReverseTable[256];

static
struct CReverseerTableInitializer
{
  CReverseerTableInitializer()
  {
    for (unsigned i = 0; i < 256; i++)
    {
      unsigned
      x = ((i & 0x55) << 1) | ((i >> 1) & 0x55);
      x = ((x & 0x33) << 2) | ((x >> 2) & 0x33);
      kReverseTable[i] = (Byte)((x << 4) | (x >> 4));
    }
  }
} g_ReverseerTableInitializer;

#elif 0
unsigned ReverseBits8test(unsigned i) { return ReverseBits8(i); }
#endif
}
