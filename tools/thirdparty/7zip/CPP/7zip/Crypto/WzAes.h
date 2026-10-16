// Crypto/WzAes.h
/*
This code implements Brian Gladman's scheme
specified in "A Password Based File Encryption Utility":
  - AES encryption (128,192,256-bit) in Counter (CTR) mode.
  - HMAC-SHA1 authentication for encrypted data (10 bytes)
  - Keys are derived by PPKDF2(RFC2898)-HMAC-SHA1 from ASCII password and
    Salt (saltSize = aesKeySize / 2).
  - 2 bytes contain Password Verifier's Code
*/

#ifndef ZIP7_INC_CRYPTO_WZ_AES_H
#define ZIP7_INC_CRYPTO_WZ_AES_H

#include "../../Common/MyBuffer.h"

#include "../IPassword.h"

#include "HmacSha1.h"
#include "MyAes.h"

namespace NCrypto {
namespace NWzAes {

/* ICompressFilter::Init() does nothing for this filter.

  Call to init:
    Encoder:
      CryptoSetPassword();
      WriteHeader();
    Decoder:
      [CryptoSetPassword();]
      ReadHeader();
      [CryptoSetPassword();] Init_and_CheckPassword();
      [CryptoSetPassword();] Init_and_CheckPassword();
*/

const UInt32 kPasswordSizeMax = 99; // 128;

const unsigned kSaltSizeMax = 16;
const unsigned kPwdVerifSize = 2;
const unsigned kMacSize = 10;

enum EKeySizeMode
{
  kKeySizeMode_AES128 = 1,
  kKeySizeMode_AES192 = 2,
  kKeySizeMode_AES256 = 3
};

struct CKeyInfo
{
  EKeySizeMode KeySizeMode;
  Byte Salt[kSaltSizeMax];
  Byte PwdVerifComputed[kPwdVerifSize];

  CByteBuffer Password;

  unsigned GetKeySize()  const { return (8 * KeySizeMode + 8); }
  unsigned GetSaltSize() const { return (4 * KeySizeMode + 4); }
  unsigned GetNumSaltWords() const { return (KeySizeMode + 1); }

  CKeyInfo(): KeySizeMode(kKeySizeMode_AES256) {}

  void Wipe()
  {
    Password.Wipe();
    Z7_memset_0_ARRAY(Salt);
    Z7_memset_0_ARRAY(PwdVerifComputed);
  }

  ~CKeyInfo() { Wipe(); }
};

/*
struct CAesCtr2
{
  unsigned pos;
  CAlignedBuffer aes;
  UInt32 *Aes() { return (UInt32 *)(Byte *)aes; }

  // unsigned offset;
  // UInt32 aes[4 + AES_NUM_IVMRK_WORDS + 3];
  // UInt32 *Aes() { return aes + offset; }
  CAesCtr2();
};

void AesCtr2_Init(CAesCtr2 *p);
void AesCtr2_Code(CAesCtr2 *p, Byte *data, SizeT size);
*/

class CBaseCoder:
  public ICompressFilter,
  public ICryptoSetPassword,
  public CMyUnknownImp
{
  Z7_COM_UNKNOWN_IMP_1(ICryptoSetPassword)
  Z7_COM7F_IMP(Init())
public:
  Z7_IFACE_COM7_IMP(ICryptoSetPassword)
protected:
  CKeyInfo _key;

  // NSha1::CHmac _hmac;
  // NSha1::CHmac *Hmac() { return &_hmac; }
  CAlignedBuffer1 _hmacBuf;
  UInt32 _hmacOverCalc;

  NSha1::CHmac *Hmac() { return (NSha1::CHmac *)(void *)(Byte *)_hmacBuf; }

  // CAesCtr2 _aes;
  CAesCoder *_aesCoderSpec;
  CMyComPtr<ICompressFilter> _aesCoder;
  CBaseCoder():
    _hmacBuf(sizeof(NSha1::CHmac))
  {
    _aesCoderSpec = new CAesCtrCoder(32);
    _aesCoder = _aesCoderSpec;
  }

  void Init2();
public:
  unsigned GetHeaderSize() const { return _key.GetSaltSize() + kPwdVerifSize; }
  unsigned GetAddPackSize() const { return GetHeaderSize() + kMacSize; }

  bool SetKeyMode(unsigned mode)
  {
    if (mode < kKeySizeMode_AES128 || mode > kKeySizeMode_AES256)
      return false;
    _key.KeySizeMode = (EKeySizeMode)mode;
    return true;
  }

  virtual ~CBaseCoder() {}
};

class CEncoder Z7_final:
  public CBaseCoder
{
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  HRESULT WriteHeader(ISequentialOutStream *outStream);
  HRESULT WriteFooter(ISequentialOutStream *outStream);
};

class CDecoder Z7_final:
  public CBaseCoder
  // public ICompressSetDecoderProperties2
{
  Byte _pwdVerifFromArchive[kPwdVerifSize];
  Z7_COM7F_IMP2(UInt32, Filter(Byte *data, UInt32 size))
public:
  // Z7_IFACE_COM7_IMP(ICompressSetDecoderProperties2)
  HRESULT ReadHeader(ISequentialInStream *inStream);
  bool Init_and_CheckPassword();
  HRESULT CheckMac(ISequentialInStream *inStream, bool &isOK);
};

}}

#endif
