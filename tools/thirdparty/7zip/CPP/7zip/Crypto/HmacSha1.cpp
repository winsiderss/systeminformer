// HmacSha1.cpp

#include "StdAfx.h"

#include <string.h>

#include "../../../C/CpuArch.h"

#include "HmacSha1.h"

namespace NCrypto {
namespace NSha1 {

void CHmac::SetKey(const Byte *key, size_t keySize)
{
  MY_ALIGN (16)
  UInt32 temp[SHA1_NUM_BLOCK_WORDS];
  size_t i;
  
  for (i = 0; i < SHA1_NUM_BLOCK_WORDS; i++)
    temp[i] = 0;
  
  if (keySize > kBlockSize)
  {
    _sha.Init();
    _sha.Update(key, keySize);
    _sha.Final((Byte *)temp);
  }
  else
    memcpy(temp, key, keySize);
  
  for (i = 0; i < SHA1_NUM_BLOCK_WORDS; i++)
    temp[i] ^= 0x36363636;
  
  _sha.Init();
  _sha.Update((const Byte *)temp, kBlockSize);
  
  for (i = 0; i < SHA1_NUM_BLOCK_WORDS; i++)
    temp[i] ^= 0x36363636 ^ 0x5C5C5C5C;

  _sha2.Init();
  _sha2.Update((const Byte *)temp, kBlockSize);
}


void CHmac::Final(Byte *mac)
{
  _sha.Final(mac);
  _sha2.Update(mac, kDigestSize);
  _sha2.Final(mac);
}


void CHmac::GetLoopXorDigest1(void *mac, UInt32 numIteration)
{
  MY_ALIGN (16)  UInt32 block [SHA1_NUM_BLOCK_WORDS];
  MY_ALIGN (16)  UInt32 block2[SHA1_NUM_BLOCK_WORDS];
  MY_ALIGN (16)  UInt32 mac2  [SHA1_NUM_BLOCK_WORDS];
  
  _sha. PrepareBlock((Byte *)block,  SHA1_DIGEST_SIZE);
  _sha2.PrepareBlock((Byte *)block2, SHA1_DIGEST_SIZE);

  block[0] = ((const UInt32 *)mac)[0];
  block[1] = ((const UInt32 *)mac)[1];
  block[2] = ((const UInt32 *)mac)[2];
  block[3] = ((const UInt32 *)mac)[3];
  block[4] = ((const UInt32 *)mac)[4];

  mac2[0] = block[0];
  mac2[1] = block[1];
  mac2[2] = block[2];
  mac2[3] = block[3];
  mac2[4] = block[4];

  for (UInt32 i = 0; i < numIteration; i++)
  {
    _sha. GetBlockDigest((const Byte *)block,  (Byte *)block2);
    _sha2.GetBlockDigest((const Byte *)block2, (Byte *)block);

    mac2[0] ^= block[0];
    mac2[1] ^= block[1];
    mac2[2] ^= block[2];
    mac2[3] ^= block[3];
    mac2[4] ^= block[4];
  }

  ((UInt32 *)mac)[0] = mac2[0];
  ((UInt32 *)mac)[1] = mac2[1];
  ((UInt32 *)mac)[2] = mac2[2];
  ((UInt32 *)mac)[3] = mac2[3];
  ((UInt32 *)mac)[4] = mac2[4];
}

}}
