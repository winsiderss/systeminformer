// HmacSha256.h
// Implements HMAC-SHA-256 (RFC2104, FIPS-198)

#ifndef ZIP7_INC_CRYPTO_HMAC_SHA256_H
#define ZIP7_INC_CRYPTO_HMAC_SHA256_H

#include "../../../C/Sha256.h"

namespace NCrypto {
namespace NSha256 {

const unsigned kBlockSize = SHA256_BLOCK_SIZE;
const unsigned kDigestSize = SHA256_DIGEST_SIZE;

class CHmac
{
  CSha256 _sha;
  CSha256 _sha2;
public:
  void SetKey(const Byte *key, size_t keySize);
  void Update(const Byte *data, size_t dataSize) { Sha256_Update(&_sha, data, dataSize); }
  void Final(Byte *mac);
};

}}

#endif
