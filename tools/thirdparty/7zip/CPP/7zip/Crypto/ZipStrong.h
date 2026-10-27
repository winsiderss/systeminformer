// Crypto/ZipStrong.h

#ifndef ZIP7_INC_CRYPTO_ZIP_STRONG_H
#define ZIP7_INC_CRYPTO_ZIP_STRONG_H

#include "../../Common/MyBuffer2.h"

#include "../IPassword.h"

#include "MyAes.h"

namespace NCrypto {
namespace NZipStrong {

/* ICompressFilter::Init() does nothing for this filter.
  Call to init:
    Decoder:
      [CryptoSetPassword();]
      ReadHeader();
      [CryptoSetPassword();] Init_and_CheckPassword();
      [CryptoSetPassword();] Init_and_CheckPassword();
*/

struct CKeyInfo
{
  Byte MasterKey[32];
  UInt32 KeySize;
  
  void SetPassword(const Byte *data, UInt32 size);

  void Wipe()
  {
    Z7_memset_0_ARRAY(MasterKey);
  }
};


const unsigned kAesPadAllign = AES_BLOCK_SIZE;

Z7_CLASS_IMP_COM_2(
  CDecoder
  , ICompressFilter
  , ICryptoSetPassword
)
  CAesCbcDecoder *_cbcDecoder;
  CMyComPtr<ICompressFilter> _aesFilter;
  CKeyInfo _key;
  CAlignedBuffer _bufAligned;

  UInt32 _ivSize;
  Byte _iv[16];
  UInt32 _remSize;
public:
  HRESULT ReadHeader(ISequentialInStream *inStream, UInt32 crc, UInt64 unpackSize);
  HRESULT Init_and_CheckPassword(bool &passwOK);
  UInt32 GetPadSize(UInt32 packSize32) const
  {
    // Padding is to align to blockSize of cipher.
    // Change it, if is not AES
    return kAesPadAllign - (packSize32 & (kAesPadAllign - 1));
  }
  CDecoder();
  ~CDecoder() { Wipe(); }
  void Wipe()
  {
    Z7_memset_0_ARRAY(_iv);
    _key.Wipe();
  }
};

}}

#endif
