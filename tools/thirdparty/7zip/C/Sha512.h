/* Sha512.h -- SHA-512 Hash
: Igor Pavlov : Public domain */

#ifndef ZIP7_INC_SHA512_H
#define ZIP7_INC_SHA512_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define SHA512_NUM_BLOCK_WORDS  16
#define SHA512_NUM_DIGEST_WORDS  8

#define SHA512_BLOCK_SIZE   (SHA512_NUM_BLOCK_WORDS * 8)
#define SHA512_DIGEST_SIZE  (SHA512_NUM_DIGEST_WORDS * 8)
#define SHA512_224_DIGEST_SIZE  (224 / 8)
#define SHA512_256_DIGEST_SIZE  (256 / 8)
#define SHA512_384_DIGEST_SIZE  (384 / 8)

typedef void (Z7_FASTCALL *SHA512_FUNC_UPDATE_BLOCKS)(UInt64 state[8], const Byte *data, size_t numBlocks);

/*
  if (the system supports different SHA512 code implementations)
  {
    (CSha512::func_UpdateBlocks) will be used
    (CSha512::func_UpdateBlocks) can be set by
       Sha512_Init()        - to default (fastest)
       Sha512_SetFunction() - to any algo
  }
  else
  {
    (CSha512::func_UpdateBlocks) is ignored.
  }
*/

typedef struct
{
  union
  {
    struct
    {
      SHA512_FUNC_UPDATE_BLOCKS func_UpdateBlocks;
      UInt64 count;
    } vars;
    UInt64 _pad_64bit[8];
    void *_pad_align_ptr[2];
  } v;
  UInt64 state[SHA512_NUM_DIGEST_WORDS];
  
  Byte buffer[SHA512_BLOCK_SIZE];
} CSha512;


#define SHA512_ALGO_DEFAULT 0
#define SHA512_ALGO_SW      1
#define SHA512_ALGO_HW      2

/*
Sha512_SetFunction()
return:
  0 - (algo) value is not supported, and func_UpdateBlocks was not changed
  1 - func_UpdateBlocks was set according (algo) value.
*/

BoolInt Sha512_SetFunction(CSha512 *p, unsigned algo);
// we support only these (digestSize) values: 224/8, 256/8, 384/8, 512/8
void Sha512_InitState(CSha512 *p, unsigned digestSize);
void Sha512_Init(CSha512 *p, unsigned digestSize);
void Sha512_Update(CSha512 *p, const Byte *data, size_t size);
void Sha512_Final(CSha512 *p, Byte *digest, unsigned digestSize);




// void Z7_FASTCALL Sha512_UpdateBlocks(UInt64 state[8], const Byte *data, size_t numBlocks);

/*
call Sha512Prepare() once at program start.
It prepares all supported implementations, and detects the fastest implementation.
*/

void Sha512Prepare(void);

EXTERN_C_END

#endif
