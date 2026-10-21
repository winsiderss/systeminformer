// Crypto/WzAes.cpp
/*
This code implements Brian Gladman's scheme
specified in "A Password Based File Encryption Utility".

Note: you must include MyAes.cpp to project to initialize AES tables
*/

#include "StdAfx.h"

#include "../../../C/CpuArch.h"

#include "../Common/StreamUtils.h"

#include "Pbkdf2HmacSha1.h"
#include "RandGen.h"
#include "WzAes.h"

namespace NCrypto {
namespace NWzAes {

const unsigned kAesKeySizeMax = 32;

static const UInt32 kNumKeyGenIterations = 1000;

Z7_COM7F_IMF(CBaseCoder::CryptoSetPassword(const Byte *data, UInt32 size))
{
  if (size > kPasswordSizeMax)
    return E_INVALIDARG;
  _key.Password.Wipe();
  _key.Password.CopyFrom(data, (size_t)size);
  return S_OK;
}

void CBaseCoder::Init2()
{
  _hmacOverCalc = 0;
  const unsigned dkSizeMax32 = (2 * kAesKeySizeMax + kPwdVerifSize + 3) / 4;
  Byte dk[dkSizeMax32 * 4];

  const unsigned keySize = _key.GetKeySize();
  const unsigned dkSize = 2 * keySize + ((kPwdVerifSize + 3) & ~(unsigned)3);
  
  // for (unsigned ii = 0; ii < 1000; ii++)
  {
    NSha1::Pbkdf2Hmac(
        _key.Password, _key.Password.Size(),
        _key.Salt, _key.GetSaltSize(),
        kNumKeyGenIterations,
        dk, dkSize);
  }

  Hmac()->SetKey(dk + keySize, keySize);
  memcpy(_key.PwdVerifComputed, dk + 2 * keySize, kPwdVerifSize);

  // Aes_SetKey_Enc(_aes.Aes() + 8, dk, keySize);
  // AesCtr2_Init(&_aes);
  _aesCoderSpec->SetKeySize(keySize);
  if (_aesCoderSpec->SetKey(dk, keySize) != S_OK) throw 2;
  if (_aesCoderSpec->Init() != S_OK) throw 3;
}

Z7_COM7F_IMF(CBaseCoder::Init())
{
  return S_OK;
}

HRESULT CEncoder::WriteHeader(ISequentialOutStream *outStream)
{
  unsigned saltSize = _key.GetSaltSize();
  MY_RAND_GEN(_key.Salt, saltSize);
  Init2();
  RINOK(WriteStream(outStream, _key.Salt, saltSize))
  return WriteStream(outStream, _key.PwdVerifComputed, kPwdVerifSize);
}

HRESULT CEncoder::WriteFooter(ISequentialOutStream *outStream)
{
  MY_ALIGN (16)
  UInt32 mac[NSha1::kNumDigestWords];
  Hmac()->Final((Byte *)mac);
  return WriteStream(outStream, mac, kMacSize);
}

/*
Z7_COM7F_IMF(CDecoder::SetDecoderProperties2(const Byte *data, UInt32 size))
{
  if (size != 1)
    return E_INVALIDARG;
  _key.Init();
  return SetKeyMode(data[0]) ? S_OK : E_INVALIDARG;
}
*/

HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream)
{
  const unsigned saltSize = _key.GetSaltSize();
  const unsigned extraSize = saltSize + kPwdVerifSize;
  Byte temp[kSaltSizeMax + kPwdVerifSize];
  RINOK(ReadStream_FAIL(inStream, temp, extraSize))
  unsigned i;
  for (i = 0; i < saltSize; i++)
    _key.Salt[i] = temp[i];
  for (i = 0; i < kPwdVerifSize; i++)
    _pwdVerifFromArchive[i] = temp[saltSize + i];
  return S_OK;
}

static inline bool CompareArrays(const Byte *p1, const Byte *p2, unsigned size)
{
  for (unsigned i = 0; i < size; i++)
    if (p1[i] != p2[i])
      return false;
  return true;
}

bool CDecoder::Init_and_CheckPassword()
{
  Init2();
  return CompareArrays(_key.PwdVerifComputed, _pwdVerifFromArchive, kPwdVerifSize);
}

HRESULT CDecoder::CheckMac(ISequentialInStream *inStream, bool &isOK)
{
  isOK = false;
  MY_ALIGN (16)
  Byte mac1[kMacSize];
  RINOK(ReadStream_FAIL(inStream, mac1, kMacSize))
  MY_ALIGN (16)
  UInt32 mac2[NSha1::kNumDigestWords];
  Hmac()->Final((Byte *)mac2);
  isOK = CompareArrays(mac1, (const Byte *)mac2, kMacSize);
  if (_hmacOverCalc)
    isOK = false;
  return S_OK;
}

/*

CAesCtr2::CAesCtr2():
    aes((4 + AES_NUM_IVMRK_WORDS) * 4)
{
  // offset = ((0 - (unsigned)(ptrdiff_t)aes) & 0xF) / sizeof(UInt32);
  // first 16 bytes are buffer for last block data.
  // so the ivAES is aligned for (Align + 16).
}

void AesCtr2_Init(CAesCtr2 *p)
{
  UInt32 *ctr = p->Aes() + 4;
  unsigned i;
  for (i = 0; i < 4; i++)
    ctr[i] = 0;
  p->pos = AES_BLOCK_SIZE;
}

// (size != 16 * N) is allowed only for last call

void AesCtr2_Code(CAesCtr2 *p, Byte *data, SizeT size)
{
  unsigned pos = p->pos;
  UInt32 *buf32 = p->Aes();
  if (size == 0)
    return;
  
  if (pos != AES_BLOCK_SIZE)
  {
    const Byte *buf = (const Byte *)buf32;
    do
      *data++ ^= buf[pos++];
    while (--size != 0 && pos != AES_BLOCK_SIZE);
  }

  // (size == 0 || pos == AES_BLOCK_SIZE)
  
  if (size >= 16)
  {
    SizeT size2 = size >> 4;
    g_AesCtr_Code(buf32 + 4, data, size2);
    size2 <<= 4;
    data += size2;
    size -= size2;
    // (pos == AES_BLOCK_SIZE)
  }

  // (size < 16)
  
  if (size != 0)
  {
    unsigned j;
    const Byte *buf;
    for (j = 0; j < 4; j++)
      buf32[j] = 0;
    g_AesCtr_Code(buf32 + 4, (Byte *)buf32, 1);
    buf = (const Byte *)buf32;
    pos = 0;
    do
      *data++ ^= buf[pos++];
    while (--size != 0);
  }
  
  p->pos = pos;
}
*/

/* (size != 16 * N) is allowed only for last Filter() call */

Z7_COM7F_IMF2(UInt32, CEncoder::Filter(Byte *data, UInt32 size))
{
  // AesCtr2_Code(&_aes, data, size);
  size = _aesCoder->Filter(data, size);
  Hmac()->Update(data, size);
  return size;
}

Z7_COM7F_IMF2(UInt32, CDecoder::Filter(Byte *data, UInt32 size))
{
  if (size >= 16)
    size &= ~(UInt32)15;
  if (_hmacOverCalc < size)
  {
    Hmac()->Update(data + _hmacOverCalc, size - _hmacOverCalc);
    _hmacOverCalc = size;
  }
  // AesCtr2_Code(&_aes, data, size);
  size = _aesCoder->Filter(data, size);
  _hmacOverCalc -= size;
  return size;
}

}}
