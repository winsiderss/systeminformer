/* Xxh64.h -- XXH64 hash calculation interfaces
2023-08-18 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_XXH64_H
#define ZIP7_INC_XXH64_H

#include "7zTypes.h"

EXTERN_C_BEGIN

#define Z7_XXH64_BLOCK_SIZE  (4 * 8)

typedef struct
{
  UInt64 v[4];
} CXxh64State;

void Xxh64State_Init(CXxh64State *p);

// end != data && end == data + Z7_XXH64_BLOCK_SIZE * numBlocks
void Z7_FASTCALL Xxh64State_UpdateBlocks(CXxh64State *p, const void *data, const void *end);

/*
Xxh64State_Digest():
data:
  the function processes only
    (totalCount & (Z7_XXH64_BLOCK_SIZE - 1)) bytes in (data): (smaller than 32 bytes).
totalCount: total size of hashed stream:
  it includes total size of data processed by previous Xxh64State_UpdateBlocks() calls,
  and it also includes current processed size in (data).
*/
UInt64 Xxh64State_Digest(const CXxh64State *p, const void *data, UInt64 totalCount);


typedef struct
{
  CXxh64State state;
  UInt64 count;
  UInt64 buf64[4];
} CXxh64;

void Xxh64_Init(CXxh64 *p);
void Xxh64_Update(CXxh64 *p, const void *data, size_t size);

#define Xxh64_Digest(p) \
  Xxh64State_Digest(&(p)->state, (p)->buf64, (p)->count)

EXTERN_C_END

#endif
