// Crypto/RarAes.h

#ifndef ZIP7_INC_CRYPTO_RAR_AES_H
#define ZIP7_INC_CRYPTO_RAR_AES_H

#include "../../../C/Aes.h"

#include "../../Common/MyBuffer.h"

#include "../IPassword.h"

#include "MyAes.h"

namespace NCrypto {
namespace NRar3 {

const unsigned kAesKeySize = 16;

class CDecoder Z7_final:
  public CAesCbcDecoder
{
  Byte _salt[8];
  bool _thereIsSalt;
  bool _needCalc;
  // bool _rar350Mode;
  
  CByteBuffer _password;
  
  Byte _key[kAesKeySize];
  Byte _iv[AES_BLOCK_SIZE];

  void CalcKey();
public:
  Z7_COM7F_IMP(Init())
  
  void SetPassword(const Byte *data, unsigned size);
  HRESULT SetDecoderProperties2(const Byte *data, UInt32 size);

  CDecoder();

  ~CDecoder() Z7_DESTRUCTOR_override { Wipe(); }
  void Wipe()
  {
    _password.Wipe();
    Z7_memset_0_ARRAY(_salt);
    Z7_memset_0_ARRAY(_key);
    Z7_memset_0_ARRAY(_iv);
  }
  // void SetRar350Mode(bool rar350Mode) { _rar350Mode = rar350Mode; }
};

}}

#endif
