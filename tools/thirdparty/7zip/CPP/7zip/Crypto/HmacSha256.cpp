// HmacSha256.cpp

#include "StdAfx.h"

#include <string.h>

#include "../../../C/CpuArch.h"

#include "HmacSha256.h"

namespace NCrypto {
namespace NSha256 {

void CHmac::SetKey(const Byte *key, size_t keySize)
{
  MY_ALIGN (16)
  UInt32 temp[SHA256_NUM_BLOCK_WORDS];
  size_t i;
  
  for (i = 0; i < SHA256_NUM_BLOCK_WORDS; i++)
    temp[i] = 0;
  
  if (keySize > kBlockSize)
  {
    Sha256_Init(&_sha);
    Sha256_Update(&_sha, key, keySize);
    Sha256_Final(&_sha, (Byte *)temp);
  }
  else
    memcpy(temp, key, keySize);
  
  for (i = 0; i < SHA256_NUM_BLOCK_WORDS; i++)
    temp[i] ^= 0x36363636;
  
  Sha256_Init(&_sha);
  Sha256_Update(&_sha, (const Byte *)temp, kBlockSize);
  
  for (i = 0; i < SHA256_NUM_BLOCK_WORDS; i++)
    temp[i] ^= 0x36363636 ^ 0x5C5C5C5C;

  Sha256_Init(&_sha2);
  Sha256_Update(&_sha2, (const Byte *)temp, kBlockSize);
}


void CHmac::Final(Byte *mac)
{
  Sha256_Final(&_sha, mac);
  Sha256_Update(&_sha2, mac, SHA256_DIGEST_SIZE);
  Sha256_Final(&_sha2, mac);
}

}}
