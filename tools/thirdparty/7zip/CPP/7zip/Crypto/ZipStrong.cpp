// Crypto/ZipStrong.cpp

#include "StdAfx.h"

#include "../../../C/7zCrc.h"
#include "../../../C/CpuArch.h"

#include "../Common/StreamUtils.h"

#include "Sha1Cls.h"
#include "ZipStrong.h"

namespace NCrypto {
namespace NZipStrong {

static const UInt16 kAES128 = 0x660E;

/*
  DeriveKey() function is similar to CryptDeriveKey() from Windows.
  New version of MSDN contains the following condition in CryptDeriveKey() description:
    "If the hash is not a member of the SHA-2 family and the required key is for either 3DES or AES".
  Now we support ZipStrong for AES only. And it uses SHA1.
  Our DeriveKey() code is equal to CryptDeriveKey() in Windows for such conditions: (SHA1 + AES).
  if (method != AES && method != 3DES), probably we need another code.
*/

static void DeriveKey2(const UInt32 *digest32, Byte c, UInt32 *dest32)
{
  const unsigned kBufSize = 64;
  MY_ALIGN (16)
  UInt32 buf32[kBufSize / 4];
  memset(buf32, c, kBufSize);
  for (unsigned i = 0; i < NSha1::kNumDigestWords; i++)
    buf32[i] ^= digest32[i];
  MY_ALIGN (16)
  NSha1::CContext sha;
  sha.Init();
  sha.Update((const Byte *)buf32, kBufSize);
  sha.Final((Byte *)dest32);
}
 
static void DeriveKey(NSha1::CContext &sha, Byte *key)
{
  MY_ALIGN (16)
  UInt32 digest32[NSha1::kNumDigestWords];
  sha.Final((Byte *)digest32);
  MY_ALIGN (16)
  UInt32 temp32[NSha1::kNumDigestWords * 2];
  DeriveKey2(digest32, 0x36, temp32);
  DeriveKey2(digest32, 0x5C, temp32 + NSha1::kNumDigestWords);
  memcpy(key, temp32, 32);
}

void CKeyInfo::SetPassword(const Byte *data, UInt32 size)
{
  MY_ALIGN (16)
  NSha1::CContext sha;
  sha.Init();
  sha.Update(data, size);
  DeriveKey(sha, MasterKey);
}



CDecoder::CDecoder()
{
  CAesCbcDecoder *d = new CAesCbcDecoder();
  _cbcDecoder = d;
  _aesFilter = d;
}

Z7_COM7F_IMF(CDecoder::CryptoSetPassword(const Byte *data, UInt32 size))
{
  _key.SetPassword(data, size);
  return S_OK;
}

Z7_COM7F_IMF(CDecoder::Init())
{
  return S_OK;
}

Z7_COM7F_IMF2(UInt32, CDecoder::Filter(Byte *data, UInt32 size))
{
  return _aesFilter->Filter(data, size);
}


HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream, UInt32 crc, UInt64 unpackSize)
{
  Byte temp[4];
  RINOK(ReadStream_FALSE(inStream, temp, 2))
  _ivSize = GetUi16(temp);
  if (_ivSize == 0)
  {
    memset(_iv, 0, 16);
    SetUi32(_iv + 0, crc)
    SetUi64(_iv + 4, unpackSize)
    _ivSize = 12;
  }
  else if (_ivSize == 16)
  {
    RINOK(ReadStream_FALSE(inStream, _iv, _ivSize))
  }
  else
    return E_NOTIMPL;
  RINOK(ReadStream_FALSE(inStream, temp, 4))
  _remSize = GetUi32(temp);
  // const UInt32 kAlign = 16;
  if (_remSize < 16 || _remSize > (1 << 18))
    return E_NOTIMPL;
  if (_remSize > _bufAligned.Size())
  {
    _bufAligned.AllocAtLeast(_remSize);
    if (!(Byte *)_bufAligned)
      return E_OUTOFMEMORY;
  }
  return ReadStream_FALSE(inStream, _bufAligned, _remSize);
}

HRESULT CDecoder::Init_and_CheckPassword(bool &passwOK)
{
  passwOK = false;
  if (_remSize < 16)
    return E_NOTIMPL;
  Byte * const p = _bufAligned;
  const unsigned format = GetUi16a(p);
  if (format != 3)
    return E_NOTIMPL;
  unsigned algId = GetUi16a(p + 2);
  if (algId < kAES128)
    return E_NOTIMPL;
  algId -= kAES128;
  if (algId > 2)
    return E_NOTIMPL;
  const unsigned bitLen = GetUi16a(p + 4);
  const unsigned flags = GetUi16a(p + 6);
  if (algId * 64 + 128 != bitLen)
    return E_NOTIMPL;
  _key.KeySize = 16 + algId * 8;
  const bool cert = ((flags & 2) != 0);

  if (flags & 0x4000)
  {
    // Use 3DES for rd data
    return E_NOTIMPL;
  }

  if (cert)
  {
    return E_NOTIMPL;
  }
  else
  {
    if ((flags & 1) == 0)
      return E_NOTIMPL;
  }

  UInt32 rdSize = GetUi16a(p + 8);

  if (rdSize + 16 > _remSize)
    return E_NOTIMPL;

  const unsigned kPadSize = kAesPadAllign; // is equal to blockSize of cipher for rd

  /*
  if (cert)
  {
    if ((rdSize & 0x7) != 0)
      return E_NOTIMPL;
  }
  else
  */
  {
    // PKCS7 padding
    if (rdSize < kPadSize)
      return E_NOTIMPL;
    if (rdSize & (kPadSize - 1))
      return E_NOTIMPL;
  }

  memmove(p, p + 10, rdSize);
  const Byte *p2 = p + rdSize + 10;
  UInt32 reserved = GetUi32(p2);
  p2 += 4;
  
  /*
  if (cert)
  {
    UInt32 numRecipients = reserved;

    if (numRecipients == 0)
      return E_NOTIMPL;

    {
      UInt32 hashAlg = GetUi16(p2);
      hashAlg = hashAlg;
      UInt32 hashSize = GetUi16(p2 + 2);
      hashSize = hashSize;
      p2 += 4;

      reserved = reserved;
      // return E_NOTIMPL;

      for (unsigned r = 0; r < numRecipients; r++)
      {
        UInt32 specSize = GetUi16(p2);
        p2 += 2;
        p2 += specSize;
      }
    }
  }
  else
  */
  {
    if (reserved != 0)
      return E_NOTIMPL;
  }

  UInt32 validSize = GetUi16(p2);
  p2 += 2;
  const size_t validOffset = (size_t)(p2 - p);
  if ((validSize & 0xF) != 0 || validOffset + validSize != _remSize)
    return E_NOTIMPL;

  {
    RINOK(_cbcDecoder->SetKey(_key.MasterKey, _key.KeySize))
    RINOK(_cbcDecoder->SetInitVector(_iv, 16))
    // SetInitVector() calls also Init()
    RINOK(_cbcDecoder->Init()) // it's optional
    Filter(p, rdSize);

    rdSize -= kPadSize;
    for (unsigned i = 0; i < kPadSize; i++)
      if (p[(size_t)rdSize + i] != kPadSize)
        return S_OK; // passwOK = false;
  }

  MY_ALIGN (16)
  Byte fileKey[32];
  MY_ALIGN (16)
  NSha1::CContext sha;
  sha.Init();
  sha.Update(_iv, _ivSize);
  sha.Update(p, rdSize);
  DeriveKey(sha, fileKey);
  
  RINOK(_cbcDecoder->SetKey(fileKey, _key.KeySize))
  RINOK(_cbcDecoder->SetInitVector(_iv, 16))
  // SetInitVector() calls also Init()
  RINOK(_cbcDecoder->Init()) // it's optional

  memmove(p, p + validOffset, validSize);
  Filter(p, validSize);

  if (validSize < 4)
    return E_NOTIMPL;
  validSize -= 4;
  if (GetUi32(p + validSize) != CrcCalc(p, validSize))
    return S_OK;
  passwOK = true;
  return S_OK;
}

}}
