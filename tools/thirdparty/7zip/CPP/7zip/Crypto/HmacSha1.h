// HmacSha1.h
// Implements HMAC-SHA-1 (RFC2104, FIPS-198)

#ifndef ZIP7_INC_CRYPTO_HMAC_SHA1_H
#define ZIP7_INC_CRYPTO_HMAC_SHA1_H

#include "Sha1Cls.h"

namespace NCrypto {
namespace NSha1 {

// Use:  SetKey(key, keySize); for () Update(data, size); FinalFull(mac);

class CHmac
{
  CContext _sha;
  CContext _sha2;
public:
  void SetKey(const Byte *key, size_t keySize);
  void Update(const Byte *data, size_t dataSize) { _sha.Update(data, dataSize); }
  
  // Final() : mac is recommended to be aligned for 4 bytes
  // GetLoopXorDigest1() : mac is required    to be aligned for 4 bytes
  // The caller can use: UInt32 mac[NSha1::kNumDigestWords] and typecast to (Byte *) and (void *);
  void Final(Byte *mac);
  void GetLoopXorDigest1(void *mac, UInt32 numIteration);
};

}}

#endif
