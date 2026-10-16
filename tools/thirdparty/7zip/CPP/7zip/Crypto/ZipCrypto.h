// Crypto/ZipCrypto.h

#ifndef ZIP7_INC_CRYPTO_ZIP_CRYPTO_H
#define ZIP7_INC_CRYPTO_ZIP_CRYPTO_H

#include "../../Common/MyCom.h"

#include "../ICoder.h"
#include "../IPassword.h"

namespace NCrypto {
namespace NZip {

const unsigned kHeaderSize = 12;

/* ICompressFilter::Init() does nothing for this filter.
  Call to init:
    Encoder:
      CryptoSetPassword();
      WriteHeader();
    Decoder:
      [CryptoSetPassword();]
      ReadHeader();
      [CryptoSetPassword();] Init_and_GetCrcByte();
      [CryptoSetPassword();] Init_and_GetCrcByte();
*/

class CCipher:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_1(ICryptoSetPassword)
  Z7_COM7F_IMP(Init())
public:
  Z7_IFACE_COM7_IMP(ICryptoSetPassword)
protected:
  UInt32 Key0;
  UInt32 Key1;
  UInt32 Key2;
  
  UInt32 KeyMem0;
  UInt32 KeyMem1;
  UInt32 KeyMem2;

  void RestoreKeys()
  {
    Key0 = KeyMem0;
    Key1 = KeyMem1;
    Key2 = KeyMem2;
  }

public:
  virtual ~CCipher()
  {
    Key0 = KeyMem0 =
    Key1 = KeyMem1 =
    Key2 = KeyMem2 = 0;
  }
};

class CEncoder Z7_final: public CCipher
{
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  HRESULT WriteHeader_Check16(ISequentialOutStream *outStream, UInt16 crc);
};

class CDecoder Z7_final: public CCipher
{
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  Byte _header[kHeaderSize];
  HRESULT ReadHeader(ISequentialInStream *inStream);
  void Init_BeforeDecode();
};

}}

#endif
