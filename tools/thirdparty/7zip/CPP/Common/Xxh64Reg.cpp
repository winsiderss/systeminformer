// Xxh64Reg.cpp

#include "StdAfx.h"

#include "../../C/Xxh64.h"
#include "../../C/CpuArch.h"

// #define Z7_USE_HHX64_ORIGINAL
#ifdef Z7_USE_HHX64_ORIGINAL
#ifdef __clang__
#include "../../C/zstd7z/7z_zstd_cmpl.h"
#pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#endif
#define XXH_STATIC_LINKING_ONLY
#include "../../C/zstd7z/common/xxhash.h"
#endif

#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"

Z7_CLASS_IMP_COM_1(
  CXxh64Hasher
  , IHasher
)
  CXxh64 _state;
public:
  Byte _mtDummy[1 << 7];  // it's public to eliminate clang warning: unused private field
  CXxh64Hasher() { Init(); }
};

Z7_COM7F_IMF2(void, CXxh64Hasher::Init())
{
  Xxh64_Init(&_state);
}

Z7_COM7F_IMF2(void, CXxh64Hasher::Update(const void *data, UInt32 size))
{
#if 1
  Xxh64_Update(&_state, data, size);
#else // for debug:
  for (;;)
  {
    if (size == 0)
      return;
    UInt32 size2 = (size * 0x85EBCA87) % size / 8;
    if (size2 == 0)
      size2 = 1;
    Xxh64_Update(&_state, data, size2);
    data = (const void *)((const Byte *)data + size2);
    size -= size2;
  }
#endif
}

Z7_COM7F_IMF2(void, CXxh64Hasher::Final(Byte *digest))
{
  const UInt64 val = Xxh64_Digest(&_state);
  SetUi64(digest, val)
}

REGISTER_HASHER(CXxh64Hasher, 0x211, "XXH64", 8)



#ifdef Z7_USE_HHX64_ORIGINAL
namespace NOriginal
{
Z7_CLASS_IMP_COM_1(
  CXxh64Hasher
  , IHasher
)
  XXH64_state_t _state;
public:
  Byte _mtDummy[1 << 7];  // it's public to eliminate clang warning: unused private field
  CXxh64Hasher() { Init(); }
};

Z7_COM7F_IMF2(void, CXxh64Hasher::Init())
{
  XXH64_reset(&_state, 0);
}

Z7_COM7F_IMF2(void, CXxh64Hasher::Update(const void *data, UInt32 size))
{
  XXH64_update(&_state, data, size);
}

Z7_COM7F_IMF2(void, CXxh64Hasher::Final(Byte *digest))
{
  const UInt64 val = XXH64_digest(&_state);
  SetUi64(digest, val)
}

REGISTER_HASHER(CXxh64Hasher, 0x212, "XXH64a", 8)
}
#endif
