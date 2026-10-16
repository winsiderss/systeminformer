/* Md5.h -- MD5 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_MD5_H
#define ZIP7_INC_MD5_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define MD5_NUM_BLOCK_WORDS  16
#define MD5_NUM_DIGEST_WORDS  4

#define MD5_BLOCK_SIZE   (MD5_NUM_BLOCK_WORDS * 4)
#define MD5_DIGEST_SIZE  (MD5_NUM_DIGEST_WORDS * 4)

typedef struct
{
  UInt64 count;
  UInt64 _pad_1;
  // we want 16-bytes alignment here
  UInt32 state[MD5_NUM_DIGEST_WORDS];
  UInt64 _pad_2[4];
  // we want 64-bytes alignment here
  Byte buffer[MD5_BLOCK_SIZE];
} CMd5;

void Md5_Init(CMd5 *p);
void Md5_Update(CMd5 *p, const Byte *data, size_t size);
void Md5_Final(CMd5 *p, Byte *digest);

EXTERN_C_END

#endif
