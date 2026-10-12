// Sha3Reg.cpp

#include "StdAfx.h"

#include "../../C/Sha3.h"

#include "../Common/MyBuffer2.h"
#include "../Common/MyCom.h"

#include "../7zip/Common/RegisterCodec.h"

Z7_CLASS_IMP_COM_1(
  CSha3Hasher
  , IHasher
)
  unsigned _digestSize;
  bool _isShake;
  CAlignedBuffer1 _buf;
public:
  Byte _mtDummy[1 << 7];

  CSha3 *Sha() { return (CSha3 *)(void *)(Byte *)_buf; }
public:
  CSha3Hasher(unsigned digestSize, bool isShake, unsigned blockSize):
     _digestSize(digestSize),
     _isShake(isShake),
    _buf(sizeof(CSha3))
  {
    CSha3 *p = Sha();
    Sha3_SET_blockSize(p, blockSize)
    Sha3_Init(Sha());
  }
};

Z7_COM7F_IMF2(void, CSha3Hasher::Init())
{
  Sha3_Init(Sha());
}

Z7_COM7F_IMF2(void, CSha3Hasher::Update(const void *data, UInt32 size))
{
  Sha3_Update(Sha(), (const Byte *)data, size);
}

Z7_COM7F_IMF2(void, CSha3Hasher::Final(Byte *digest))
{
  Sha3_Final(Sha(), digest, _digestSize, _isShake);
}

Z7_COM7F_IMF2(UInt32, CSha3Hasher::GetDigestSize())
{
  return (UInt32)_digestSize;
}


#define REGISTER_SHA3_HASHER_2(cls, id, name, digestSize, isShake, digestSize_for_blockSize) \
  namespace N ## cls { \
  static IHasher *CreateHasherSpec() \
    { return new CSha3Hasher(digestSize / 8, isShake, \
        SHA3_BLOCK_SIZE_FROM_DIGEST_SIZE(digestSize_for_blockSize / 8)); } \
  static const CHasherInfo g_HasherInfo = { CreateHasherSpec, id, name, digestSize / 8 }; \
  struct REGISTER_HASHER_NAME(cls) { REGISTER_HASHER_NAME(cls)() { RegisterHasher(&g_HasherInfo); }}; \
  static REGISTER_HASHER_NAME(cls) g_RegisterHasher; }

#define REGISTER_SHA3_HASHER(  cls, id, name, size, isShake) \
        REGISTER_SHA3_HASHER_2(cls, id, name, size, isShake, size)

// REGISTER_SHA3_HASHER (Sha3_224_Hasher, 0x230, "SHA3-224", 224, false)
REGISTER_SHA3_HASHER (Sha3_256_Hasher, 0x231, "SHA3-256", 256, false)
// REGISTER_SHA3_HASHER (Sha3_386_Hasher, 0x232, "SHA3-384", 384, false)
// REGISTER_SHA3_HASHER (Sha3_512_Hasher, 0x233, "SHA3-512", 512, false)
// REGISTER_SHA3_HASHER (Shake128_Hasher, 0x240, "SHAKE128", 128, true)
// REGISTER_SHA3_HASHER (Shake256_Hasher, 0x241, "SHAKE256", 256, true)
// REGISTER_SHA3_HASHER_2 (Shake128_512_Hasher, 0x248, "SHAKE128-256", 256, true, 128) // -1344 (max)
// REGISTER_SHA3_HASHER_2 (Shake256_512_Hasher, 0x249, "SHAKE256-512", 512, true, 256) // -1088 (max)
// Shake supports different digestSize values for same blockSize
