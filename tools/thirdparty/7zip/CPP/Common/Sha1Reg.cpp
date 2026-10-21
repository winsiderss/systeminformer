// Sha1Reg.cpp

#include "StdAfx.h"

#include "../../C/Sha1.h"

#include "../Common/MyBuffer2.h"
#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"

Z7_CLASS_IMP_COM_2(
  CSha1Hasher
  , IHasher
  , ICompressSetCoderProperties
)
  CAlignedBuffer1 _buf;
public:
  Byte _mtDummy[1 << 7];
  
  CSha1 *Sha() { return (CSha1 *)(void *)(Byte *)_buf; }
public:
  CSha1Hasher():
    _buf(sizeof(CSha1))
  {
    Sha1_SetFunction(Sha(), 0);
    Sha1_InitState(Sha());
  }
};

Z7_COM7F_IMF2(void, CSha1Hasher::Init())
{
  Sha1_InitState(Sha());
}

Z7_COM7F_IMF2(void, CSha1Hasher::Update(const void *data, UInt32 size))
{
  Sha1_Update(Sha(), (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CSha1Hasher::Final(Byte *digest))
{
  Sha1_Final(Sha(), digest);
}


Z7_COM7F_IMF(CSha1Hasher::SetCoderProperties(const PROPID *propIDs, const PROPVARIANT *coderProps, UInt32 numProps))
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
  if (!Sha1_SetFunction(Sha(), algo))
    return E_NOTIMPL;
  return S_OK;
}

REGISTER_HASHER(CSha1Hasher, 0x201, "SHA1", SHA1_DIGEST_SIZE)
