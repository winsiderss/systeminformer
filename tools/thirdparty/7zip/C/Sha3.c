/* Sha3.c -- SHA-3 Hash
: Igor Pavlov : Public domain
This code is based on public domain code from Wei Dai's Crypto++ library. */

#include "Precomp.h"

#include <string.h>

#include "Sha3.h"
#include "RotateDefs.h"
#include "CpuArch.h"

#define U64C(x) UINT64_CONST(x)

static
MY_ALIGN(64)
const UInt64 SHA3_K_ARRAY[24] =
{
  U64C(0x0000000000000001), U64C(0x0000000000008082),
  U64C(0x800000000000808a), U64C(0x8000000080008000),
  U64C(0x000000000000808b), U64C(0x0000000080000001),
  U64C(0x8000000080008081), U64C(0x8000000000008009),
  U64C(0x000000000000008a), U64C(0x0000000000000088),
  U64C(0x0000000080008009), U64C(0x000000008000000a),
  U64C(0x000000008000808b), U64C(0x800000000000008b),
  U64C(0x8000000000008089), U64C(0x8000000000008003),
  U64C(0x8000000000008002), U64C(0x8000000000000080),
  U64C(0x000000000000800a), U64C(0x800000008000000a),
  U64C(0x8000000080008081), U64C(0x8000000000008080),
  U64C(0x0000000080000001), U64C(0x8000000080008008)
};

void Sha3_Init(CSha3 *p)
{
  p->count = 0;
  memset(p->state, 0, sizeof(p->state));
}

#define GET_state(i, a)   UInt64 a = state[i];
#define SET_state(i, a)   state[i] = a;

#define LS_5(M, i, a0,a1,a2,a3,a4) \
        M ((i) * 5    , a0) \
        M ((i) * 5 + 1, a1) \
        M ((i) * 5 + 2, a2) \
        M ((i) * 5 + 3, a3) \
        M ((i) * 5 + 4, a4) \

#define LS_25(M) \
        LS_5 (M, 0, a50, a51, a52, a53, a54) \
        LS_5 (M, 1, a60, a61, a62, a63, a64) \
        LS_5 (M, 2, a70, a71, a72, a73, a74) \
        LS_5 (M, 3, a80, a81, a82, a83, a84) \
        LS_5 (M, 4, a90, a91, a92, a93, a94) \


#define XOR_1(i, a0) \
        a0 ^= GetUi64(data + (i) * 8); \

#define XOR_4(i, a0,a1,a2,a3) \
        XOR_1 ((i)    , a0); \
        XOR_1 ((i) + 1, a1); \
        XOR_1 ((i) + 2, a2); \
        XOR_1 ((i) + 3, a3); \

#define D(d,b1,b2) \
        d = b1 ^ Z7_ROTL64(b2, 1);

#define D5 \
        D (d0, c4, c1) \
        D (d1, c0, c2) \
        D (d2, c1, c3) \
        D (d3, c2, c4) \
        D (d4, c3, c0) \

#define C0(c,a,d) \
        c = a ^ d; \

#define C(c,a,d,k) \
        c = a ^ d; \
        c = Z7_ROTL64(c, k); \

#define E4(e1,e2,e3,e4) \
        e1 = c1 ^ (~c2 & c3); \
        e2 = c2 ^ (~c3 & c4); \
        e3 = c3 ^ (~c4 & c0); \
        e4 = c4 ^ (~c0 & c1); \

#define CK(   v0,w0,    \
              v1,w1,k1, \
              v2,w2,k2, \
              v3,w3,k3, \
              v4,w4,k4, e0,e1,e2,e3,e4, keccak_c) \
        C0(c0,v0,w0)    \
        C (c1,v1,w1,k1) \
        C (c2,v2,w2,k2) \
        C (c3,v3,w3,k3) \
        C (c4,v4,w4,k4) \
        e0 = c0 ^ (~c1 & c2) ^ keccak_c; \
        E4(e1,e2,e3,e4) \

#define CE(   v0,w0,k0, \
              v1,w1,k1, \
              v2,w2,k2, \
              v3,w3,k3, \
              v4,w4,k4, e0,e1,e2,e3,e4) \
        C (c0,v0,w0,k0) \
        C (c1,v1,w1,k1) \
        C (c2,v2,w2,k2) \
        C (c3,v3,w3,k3) \
        C (c4,v4,w4,k4) \
        e0 = c0 ^ (~c1 & c2); \
        E4(e1,e2,e3,e4) \

// numBlocks != 0
static
Z7_NO_INLINE
void Z7_FASTCALL Sha3_UpdateBlocks(UInt64 state[SHA3_NUM_STATE_WORDS],
    const Byte *data, size_t numBlocks, size_t blockSize)
{
  LS_25 (GET_state)

  do
  {
    unsigned round;
                              XOR_4 ( 0, a50, a51, a52, a53)
                              XOR_4 ( 4, a54, a60, a61, a62)
                              XOR_1 ( 8, a63)
    if (blockSize > 8 *  9) { XOR_4 ( 9, a64, a70, a71, a72)  // sha3-384
    if (blockSize > 8 * 13) { XOR_4 (13, a73, a74, a80, a81)  // sha3-256
    if (blockSize > 8 * 17) { XOR_1 (17, a82)                 // sha3-224
    if (blockSize > 8 * 18) { XOR_1 (18, a83)                 // shake128
                              XOR_1 (19, a84)
                              XOR_1 (20, a90) }}}}
    data += blockSize;

    for (round = 0; round < 24; round += 2)
    {
      UInt64 c0, c1, c2, c3, c4;
      UInt64 d0, d1, d2, d3, d4;
      UInt64 e50, e51, e52, e53, e54;
      UInt64 e60, e61, e62, e63, e64;
      UInt64 e70, e71, e72, e73, e74;
      UInt64 e80, e81, e82, e83, e84;
      UInt64 e90, e91, e92, e93, e94;

      c0 = a50^a60^a70^a80^a90;
      c1 = a51^a61^a71^a81^a91;
      c2 = a52^a62^a72^a82^a92;
      c3 = a53^a63^a73^a83^a93;
      c4 = a54^a64^a74^a84^a94;
      D5
      CK( a50, d0,
          a61, d1, 44,
          a72, d2, 43,
          a83, d3, 21,
          a94, d4, 14, e50, e51, e52, e53, e54, SHA3_K_ARRAY[round])
      CE( a53, d3, 28,
          a64, d4, 20,
          a70, d0,  3,
          a81, d1, 45,
          a92, d2, 61, e60, e61, e62, e63, e64)
      CE( a51, d1,  1,
          a62, d2,  6,
          a73, d3, 25,
          a84, d4,  8,
          a90, d0, 18, e70, e71, e72, e73, e74)
      CE( a54, d4, 27,
          a60, d0, 36,
          a71, d1, 10,
          a82, d2, 15,
          a93, d3, 56, e80, e81, e82, e83, e84)
      CE( a52, d2, 62,
          a63, d3, 55,
          a74, d4, 39,
          a80, d0, 41,
          a91, d1,  2, e90, e91, e92, e93, e94)
      
      // ---------- ROUND + 1 ----------

      c0 = e50^e60^e70^e80^e90;
      c1 = e51^e61^e71^e81^e91;
      c2 = e52^e62^e72^e82^e92;
      c3 = e53^e63^e73^e83^e93;
      c4 = e54^e64^e74^e84^e94;
      D5
      CK( e50, d0,
          e61, d1, 44,
          e72, d2, 43,
          e83, d3, 21,
          e94, d4, 14, a50, a51, a52, a53, a54, SHA3_K_ARRAY[(size_t)round + 1])
      CE( e53, d3, 28,
          e64, d4, 20,
          e70, d0,  3,
          e81, d1, 45,
          e92, d2, 61, a60, a61, a62, a63, a64)
      CE( e51, d1,  1,
          e62, d2,  6,
          e73, d3, 25,
          e84, d4,  8,
          e90, d0, 18, a70, a71, a72, a73, a74)
      CE (e54, d4, 27,
          e60, d0, 36,
          e71, d1, 10,
          e82, d2, 15,
          e93, d3, 56, a80, a81, a82, a83, a84)
      CE (e52, d2, 62,
          e63, d3, 55,
          e74, d4, 39,
          e80, d0, 41,
          e91, d1,  2, a90, a91, a92, a93, a94)
    }
  }
  while (--numBlocks);

  LS_25 (SET_state)
}


#define Sha3_UpdateBlock(p) \
        Sha3_UpdateBlocks(p->state, p->buffer, 1, p->blockSize)

void Sha3_Update(CSha3 *p, const Byte *data, size_t size)
{
/*
  for (;;)
  {
    if (size == 0)
      return;
    unsigned cur = p->blockSize - p->count;
    if (cur > size)
      cur = (unsigned)size;
    size -= cur;
    unsigned pos = p->count;
    p->count = pos + cur;
    while (pos & 7)
    {
      if (cur == 0)
        return;
      Byte *pb = &(((Byte *)p->state)[pos]);
      *pb = (Byte)(*pb ^ *data++);
      cur--;
      pos++;
    }
    if (cur >= 8)
    {
      do
      {
        *(UInt64 *)(void *)&(((Byte *)p->state)[pos]) ^= GetUi64(data);
        data += 8;
        pos += 8;
        cur -= 8;
      }
      while (cur >= 8);
    }
    if (pos != p->blockSize)
    {
      if (cur)
      {
        Byte *pb = &(((Byte *)p->state)[pos]);
        do
        {
          *pb = (Byte)(*pb ^ *data++);
          pb++;
        }
        while (--cur);
      }
      return;
    }
    Sha3_UpdateBlock(p->state);
    p->count = 0;
  }
*/
  if (size == 0)
    return;
  {
    const unsigned pos = p->count;
    const unsigned num = p->blockSize - pos;
    if (num > size)
    {
      p->count = pos + (unsigned)size;
      memcpy(p->buffer + pos, data, size);
      return;
    }
    if (pos != 0)
    {
      size -= num;
      memcpy(p->buffer + pos, data, num);
      data += num;
      Sha3_UpdateBlock(p);
    }
  }
  if (size >= p->blockSize)
  {
    const size_t numBlocks = size / p->blockSize;
    const Byte *dataOld = data;
    data += numBlocks * p->blockSize;
    size = (size_t)(dataOld + size - data);
    Sha3_UpdateBlocks(p->state, dataOld, numBlocks, p->blockSize);
  }
  p->count = (unsigned)size;
  if (size)
    memcpy(p->buffer, data, size);
}


// we support only (digestSize % 4 == 0) cases
void Sha3_Final(CSha3 *p, Byte *digest, unsigned digestSize, unsigned shake)
{
  memset(p->buffer + p->count, 0, p->blockSize - p->count);
  // we write bits markers from low to higher in current byte:
  //   - if sha-3 : 2 bits : 0,1
  //   - if shake : 4 bits : 1111
  // then we write bit 1 to same byte.
  // And we write bit 1 to highest bit of last byte of block.
  p->buffer[p->count] = (Byte)(shake ? 0x1f : 0x06);
  // we need xor operation (^= 0x80) here because we must write 0x80 bit
  // to same byte as (0x1f : 0x06), if (p->count == p->blockSize - 1) !!!
  p->buffer[p->blockSize - 1] ^= 0x80;
/*
  ((Byte *)p->state)[p->count] ^= (Byte)(shake ? 0x1f : 0x06);
  ((Byte *)p->state)[p->blockSize - 1] ^= 0x80;
*/
  Sha3_UpdateBlock(p);
#if 1 && defined(MY_CPU_LE)
  memcpy(digest, p->state, digestSize);
#else
  {
    const unsigned numWords = digestSize >> 3;
    unsigned i;
    for (i = 0; i < numWords; i++)
    {
      const UInt64 v = p->state[i];
      SetUi64(digest, v)
      digest += 8;
    }
    if (digestSize & 4) // for SHA3-224
    {
      const UInt32 v = (UInt32)p->state[numWords];
      SetUi32(digest, v)
    }
  }
#endif
  Sha3_Init(p);
}

#undef GET_state
#undef SET_state
#undef LS_5
#undef LS_25
#undef XOR_1
#undef XOR_4
#undef D
#undef D5
#undef C0
#undef C
#undef E4
#undef CK
#undef CE
