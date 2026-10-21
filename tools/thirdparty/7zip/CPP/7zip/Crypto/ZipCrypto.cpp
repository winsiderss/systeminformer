// Crypto/ZipCrypto.cpp

#include "StdAfx.h"

#include "../../../C/7zCrc.h"

#include "../Common/StreamUtils.h"

#include "RandGen.h"
#include "ZipCrypto.h"

namespace NCrypto {
namespace NZip {

#define UPDATE_KEYS(b) { \
  key0 = CRC_UPDATE_BYTE(key0, b); \
  key1 = (key1 + (key0 & 0xFF)) * 0x8088405 + 1; \
  key2 = CRC_UPDATE_BYTE(key2, (Byte)(key1 >> 24)); } \

#define DECRYPT_BYTE_1 UInt32 temp = key2 | 2;
#define DECRYPT_BYTE_2 ((Byte)((temp * (temp ^ 1)) >> 8))

Z7_COM7F_IMF(CCipher::CryptoSetPassword(const Byte *data, UInt32 size))
{
  UInt32 key0 = 0x12345678;
  UInt32 key1 = 0x23456789;
  UInt32 key2 = 0x34567890;
  
  for (UInt32 i = 0; i < size; i++)
    UPDATE_KEYS(data[i])

  KeyMem0 = key0;
  KeyMem1 = key1;
  KeyMem2 = key2;
  
  return S_OK;
}

Z7_COM7F_IMF(CCipher::Init())
{
  return S_OK;
}

HRESULT CEncoder::WriteHeader_Check16(ISequentialOutStream *outStream, UInt16 crc)
{
  Byte h[kHeaderSize];
  
  /* PKZIP before 2.0 used 2 byte CRC check.
     PKZIP 2.0+ used 1 byte CRC check. It's more secure.
     We also use 1 byte CRC. */

  MY_RAND_GEN(h, kHeaderSize - 1);
  // h[kHeaderSize - 2] = (Byte)(crc);
  h[kHeaderSize - 1] = (Byte)(crc >> 8);
  
  RestoreKeys();
  Filter(h, kHeaderSize);
  return WriteStream(outStream, h, kHeaderSize);
}

Z7_COM7F_IMF2(UInt32, CEncoder::Filter(Byte *data, UInt32 size))
{
  UInt32 key0 = this->Key0;
  UInt32 key1 = this->Key1;
  UInt32 key2 = this->Key2;

  for (UInt32 i = 0; i < size; i++)
  {
    Byte b = data[i];
    DECRYPT_BYTE_1
    data[i] = (Byte)(b ^ DECRYPT_BYTE_2);
    UPDATE_KEYS(b)
  }

  this->Key0 = key0;
  this->Key1 = key1;
  this->Key2 = key2;

  return size;
}

HRESULT CDecoder::ReadHeader(ISequentialInStream *inStream)
{
  return ReadStream_FAIL(inStream, _header, kHeaderSize);
}

void CDecoder::Init_BeforeDecode()
{
  RestoreKeys();
  Filter(_header, kHeaderSize);
}

Z7_COM7F_IMF2(UInt32, CDecoder::Filter(Byte *data, UInt32 size))
{
  UInt32 key0 = this->Key0;
  UInt32 key1 = this->Key1;
  UInt32 key2 = this->Key2;
  
  for (UInt32 i = 0; i < size; i++)
  {
    DECRYPT_BYTE_1
    Byte b = (Byte)(data[i] ^ DECRYPT_BYTE_2);
    UPDATE_KEYS(b)
    data[i] = b;
  }
  
  this->Key0 = key0;
  this->Key1 = key1;
  this->Key2 = key2;
  
  return size;
}

}}
