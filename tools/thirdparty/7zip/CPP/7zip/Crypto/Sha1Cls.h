// Crypto/Sha1Cls.h

#ifndef ZIP7_INC_CRYPTO_SHA1_CLS_H
#define ZIP7_INC_CRYPTO_SHA1_CLS_H

#include "../../../C/Sha1.h"

namespace NCrypto {
namespace NSha1 {

const unsigned kNumBlockWords = SHA1_NUM_BLOCK_WORDS;
const unsigned kNumDigestWords = SHA1_NUM_DIGEST_WORDS;

const unsigned kBlockSize = SHA1_BLOCK_SIZE;
const unsigned kDigestSize = SHA1_DIGEST_SIZE;

class CContext
{
  CSha1 _s;
 
public:
  void Init() throw() { Sha1_Init(&_s); }
  void Update(const Byte *data, size_t size) throw() { Sha1_Update(&_s, data, size); }
  void Final(Byte *digest) throw() { Sha1_Final(&_s, digest); }
  void PrepareBlock(Byte *block, unsigned size) const throw()
  {
    Sha1_PrepareBlock(&_s, block, size);
  }
  void GetBlockDigest(const Byte *blockData, Byte *destDigest) const throw()
  {
    Sha1_GetBlockDigest(&_s, blockData, destDigest);
  }
};

}}

#endif
