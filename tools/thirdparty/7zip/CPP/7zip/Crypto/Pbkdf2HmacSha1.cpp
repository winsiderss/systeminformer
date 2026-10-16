// Pbkdf2HmacSha1.cpp

#include "StdAfx.h"

#include <string.h>

#include "../../../C/CpuArch.h"

#include "HmacSha1.h"
#include "Pbkdf2HmacSha1.h"

namespace NCrypto {
namespace NSha1 {

void Pbkdf2Hmac(const Byte *pwd, size_t pwdSize,
    const Byte *salt, size_t saltSize,
    UInt32 numIterations,
    Byte *key, size_t keySize)
{
  MY_ALIGN (16)
  CHmac baseCtx;
  baseCtx.SetKey(pwd, pwdSize);
  
  for (UInt32 i = 1; keySize != 0; i++)
  {
    MY_ALIGN (16)
    CHmac ctx;
    ctx = baseCtx;
    ctx.Update(salt, saltSize);
  
    MY_ALIGN (16)
    UInt32 u[kNumDigestWords];
    SetBe32(u, i)
    
    ctx.Update((const Byte *)u, 4);
    ctx.Final((Byte *)u);

    ctx = baseCtx;
    ctx.GetLoopXorDigest1((void *)u, numIterations - 1);

    const unsigned curSize = (keySize < kDigestSize) ? (unsigned)keySize : kDigestSize;
    memcpy(key, (const Byte *)u, curSize);
    key += curSize;
    keySize -= curSize;
  }
}

}}
