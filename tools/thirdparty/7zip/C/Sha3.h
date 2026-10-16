/* Sha3.h -- SHA-3 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_MD5_H
#define ZIP7_INC_MD5_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define SHA3_NUM_STATE_WORDS  25

#define SHA3_BLOCK_SIZE_FROM_DIGEST_SIZE(digestSize) \
    (SHA3_NUM_STATE_WORDS * 8 - (digestSize) * 2)

typedef struct
{
  UInt32 count;     // < blockSize
  UInt32 blockSize; // <= SHA3_NUM_STATE_WORDS * 8
  UInt64 _pad1[3];
  // we want 32-bytes alignment here
  UInt64 state[SHA3_NUM_STATE_WORDS];
  UInt64 _pad2[3];
  // we want 64-bytes alignment here
  Byte buffer[SHA3_NUM_STATE_WORDS * 8]; // last bytes will be unused with predefined blockSize values
} CSha3;

#define Sha3_SET_blockSize(p, blockSize) { (p)->blockSize = (blockSize); }

void Sha3_Init(CSha3 *p);
void Sha3_Update(CSha3 *p, const Byte *data, size_t size);
void Sha3_Final(CSha3 *p, Byte *digest, unsigned digestSize, unsigned shake);

EXTERN_C_END

#endif
