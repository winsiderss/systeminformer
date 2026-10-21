// Crypto/Rar5Aes.h

#ifndef ZIP7_INC_CRYPTO_RAR5_AES_H
#define ZIP7_INC_CRYPTO_RAR5_AES_H

#include "../../../C/Sha256.h"

#include "../../Common/MyBuffer.h"

#include "MyAes.h"

namespace NCrypto {
namespace NRar5 {

const unsigned kSaltSize = 16;
const unsigned kPswCheckSize32 = 2;
const unsigned kAesKeySize = 32;

namespace NCryptoFlags
{
  const unsigned kPswCheck = 1 << 0;
  const unsigned kUseMAC   = 1 << 1;
}

struct CKeyBase
{
protected:
  UInt32 _key32[kAesKeySize / 4];
  UInt32 _hashKey32[SHA256_NUM_DIGEST_WORDS];
  UInt32 _check_Calced32[kPswCheckSize32];

  void Wipe()
  {
    memset(this, 0, sizeof(*this));
  }
  
  void CopyCalcedKeysFrom(const CKeyBase &k)
  {
    *this = k;
  }
};

struct CKey: public CKeyBase
{
  CByteBuffer _password;
  bool _needCalc;
  unsigned _numIterationsLog;
  Byte _salt[kSaltSize];
  
  bool IsKeyEqualTo(const CKey &key)
  {
    return _numIterationsLog == key._numIterationsLog
        && memcmp(_salt, key._salt, sizeof(_salt)) == 0
        && _password == key._password;
  }

  CKey();
  ~CKey();
  
  void Wipe();

#ifdef Z7_CPP_IS_SUPPORTED_default
  // CKey(const CKey &) = default;
  CKey& operator =(const CKey &) = default;
#endif
};


class CDecoder Z7_final:
  public CAesCbcDecoder,
  public CKey
{
  UInt32 _check32[kPswCheckSize32];
  bool _canCheck;
  UInt64 Flags;

  bool IsThereCheck() const { return (Flags & NCryptoFlags::kPswCheck) != 0; }
public:
  Byte _iv[AES_BLOCK_SIZE];
  
  CDecoder();

  Z7_COM7F_IMP(Init())

  void SetPassword(const Byte *data, size_t size);
  HRESULT SetDecoderProps(const Byte *data, unsigned size, bool includeIV, bool isService);

  bool CalcKey_and_CheckPassword();

  bool UseMAC() const { return (Flags & NCryptoFlags::kUseMAC) != 0; }
  UInt32 Hmac_Convert_Crc32(UInt32 crc) const;
  void Hmac_Convert_32Bytes(Byte *data) const;
};

}}

#endif
