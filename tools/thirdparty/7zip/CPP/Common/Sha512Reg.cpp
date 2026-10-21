// Sha512Reg.cpp

#include "StdAfx.h"

#include "../../C/Sha512.h"

#include "../Common/MyBuffer2.h"
#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"

Z7_CLASS_IMP_COM_2(
  CSha512Hasher
  , IHasher
  , ICompressSetCoderProperties
)
  unsigned _digestSize;
  CAlignedBuffer1 _buf;
public:
  Byte _mtDummy[1 << 7];

  CSha512 *Sha() { return (CSha512 *)(void *)(Byte *)_buf; }
public:
  CSha512Hasher(unsigned digestSize):
     _digestSize(digestSize),
    _buf(sizeof(CSha512))
  {
    Sha512_SetFunction(Sha(), 0);
    Sha512_InitState(Sha(), _digestSize);
  }
};

Z7_COM7F_IMF2(void, CSha512Hasher::Init())
{
  Sha512_InitState(Sha(), _digestSize);
}

Z7_COM7F_IMF2(void, CSha512Hasher::Update(const void *data, UInt32 size))
{
  Sha512_Update(Sha(), (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CSha512Hasher::Final(Byte *digest))
{
  Sha512_Final(Sha(), digest, _digestSize);
}

Z7_COM7F_IMF2(UInt32, CSha512Hasher::GetDigestSize())
{
  return (UInt32)_digestSize;
}

Z7_COM7F_IMF(CSha512Hasher::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
{
  unsigned algo = 0;
  for (UInt32 i = 0; i < numProps; i++)
  {
    if (propIDs[i] == NCoderPropID::kDefaultProp)
    {
      const PROPVARIANT &prop = coderProps[i];
      if (prop.vt != VT_UI4)
        return E_INVALIDARG;
      if (prop.ulVal > 2)
        return E_NOTIMPL;
      algo = (unsigned)prop.ulVal;
    }
  }
  if (!Sha512_SetFunction(Sha(), algo))
    return E_NOTIMPL;
  return S_OK;
}

#define REGISTER_SHA512_HASHER(cls, id, name, size) \
  namespace N ## cls { \
  static IHasher *CreateHasherSpec() { return new CSha512Hasher(size); } \
  static const CHasherInfo g_HasherInfo = { CreateHasherSpec, id, name, size }; \
  struct REGISTER_HASHER_NAME(cls) { REGISTER_HASHER_NAME(cls)() { RegisterHasher(&g_HasherInfo); }}; \
  static REGISTER_HASHER_NAME(cls) g_RegisterHasher; }

// REGISTER_SHA512_HASHER (Sha512_224_Hasher, 0x220, "SHA512-224", SHA512_224_DIGEST_SIZE)
// REGISTER_SHA512_HASHER (Sha512_256_Hasher, 0x221, "SHA512-256", SHA512_256_DIGEST_SIZE)
REGISTER_SHA512_HASHER (Sha384Hasher,      0x222, "SHA384",     SHA512_384_DIGEST_SIZE)
REGISTER_SHA512_HASHER (Sha512Hasher,      0x223, "SHA512",     SHA512_DIGEST_SIZE)
