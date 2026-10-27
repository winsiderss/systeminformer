// Md5Reg.cpp

#include "StdAfx.h"

#include "../../C/Md5.h"

#include "../Common/MyBuffer2.h"
#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"

Z7_CLASS_IMP_COM_1(
  CMd5Hasher
  , IHasher
)
  CAlignedBuffer1 _buf;
public:
  Byte _mtDummy[1 << 7];

  CMd5 *Md5() { return (CMd5 *)(void *)(Byte *)_buf; }
public:
  CMd5Hasher():
    _buf(sizeof(CMd5))
  {
    Md5_Init(Md5());
  }
};

Z7_COM7F_IMF2(void, CMd5Hasher::Init())
{
  Md5_Init(Md5());
}

Z7_COM7F_IMF2(void, CMd5Hasher::Update(const void *data, UInt32 size))
{
  Md5_Update(Md5(), (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CMd5Hasher::Final(Byte *digest))
{
  Md5_Final(Md5(), digest);
}

REGISTER_HASHER(CMd5Hasher, 0x208, "MD5", MD5_DIGEST_SIZE)
