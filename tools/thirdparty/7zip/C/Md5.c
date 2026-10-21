/* Md5.c -- MD5 Hash
: Igor Pavlov : Public domain
This code is based on Colin Plumb's public domain md5.c code */

#include "Precomp.h"

#include <string.h>

#include "Md5.h"
#include "RotateDefs.h"
#include "CpuArch.h"

#define MD5_UPDATE_BLOCKS(p) Md5_UpdateBlocks

Z7_NO_INLINE
void Md5_Init(CMd5 *p)
{
  p->count = 0;
  p->state[0] = 0x67452301;
  p->state[1] = 0xefcdab89;
  p->state[2] = 0x98badcfe;
  p->state[3] = 0x10325476;
}

#if 0 && !defined(MY_CPU_LE_UNALIGN)
// optional optimization for Big-endian processors or processors without unaligned access:
// it is intended to reduce the number of complex LE32 memory reading from 64 to 16.
// But some compilers (sparc, armt) are better without this optimization.
#define Z7_MD5_USE_DATA32_ARRAY
#endif

#define LOAD_DATA(i)  GetUi32((const UInt32 *)(const void *)data + (i))

#ifdef Z7_MD5_USE_DATA32_ARRAY
#define D(i)  data32[i]
#else
#define D(i)  LOAD_DATA(i)
#endif

#define F1(x, y, z)   (z ^ (x & (y ^ z)))
#define F2(x, y, z)   F1(z, x, y)
#define F3(x, y, z)   (x ^ y ^ z)
#define F4(x, y, z)   (y ^ (x | ~z))

#define R1(i, f, start, step, w, x, y, z, s, k) \
    w += D((start + step * (i)) % 16) + k; \
    w += f(x, y, z); \
    w = rotlFixed(w, s) + x; \

#define R4(i4,  f, start, step, s0,s1,s2,s3, k0,k1,k2,k3) \
    R1 (i4*4+0, f, start, step, a,b,c,d, s0, k0) \
    R1 (i4*4+1, f, start, step, d,a,b,c, s1, k1) \
    R1 (i4*4+2, f, start, step, c,d,a,b, s2, k2) \
    R1 (i4*4+3, f, start, step, b,c,d,a, s3, k3) \

#define R16(f, start, step, s0,s1,s2,s3, k00,k01,k02,k03, k10,k11,k12,k13, k20,k21,k22,k23, k30,k31,k32,k33)  \
    R4 (0,  f, start, step, s0,s1,s2,s3, k00,k01,k02,k03) \
    R4 (1,  f, start, step, s0,s1,s2,s3, k10,k11,k12,k13) \
    R4 (2,  f, start, step, s0,s1,s2,s3, k20,k21,k22,k23) \
    R4 (3,  f, start, step, s0,s1,s2,s3, k30,k31,k32,k33) \

static
Z7_NO_INLINE
void Z7_FASTCALL Md5_UpdateBlocks(UInt32 state[4], const Byte *data, size_t numBlocks)
{
  UInt32 a, b, c, d;
  // if (numBlocks == 0) return;
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  do
  {
#ifdef Z7_MD5_USE_DATA32_ARRAY
    UInt32 data32[MD5_NUM_BLOCK_WORDS];
    {
#define LOAD_data32_x4(i) { \
      data32[i    ] = LOAD_DATA(i    ); \
      data32[i + 1] = LOAD_DATA(i + 1); \
      data32[i + 2] = LOAD_DATA(i + 2); \
      data32[i + 3] = LOAD_DATA(i + 3); }
#if 1
      LOAD_data32_x4 (0 * 4)
      LOAD_data32_x4 (1 * 4)
      LOAD_data32_x4 (2 * 4)
      LOAD_data32_x4 (3 * 4)
#else
      unsigned i;
      for (i = 0; i < MD5_NUM_BLOCK_WORDS; i += 4)
      {
        LOAD_data32_x4(i)
      }
#endif
    }
#endif

    R16 (F1, 0, 1,  7,12,17,22, 0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
                                0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
                                0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
                                0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821)
    R16 (F2, 1, 5,  5, 9,14,20, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                                0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
                                0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
                                0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a)
    R16 (F3, 5, 3,  4,11,16,23, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
                                0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                                0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
                                0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665)
    R16 (F4, 0, 7,  6,10,15,21, 0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
                                0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
                                0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                                0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391)

    a += state[0];
    b += state[1];
    c += state[2];
    d += state[3];
    
    state[0] = a;
    state[1] = b;
    state[2] = c;
    state[3] = d;
    
    data += MD5_BLOCK_SIZE;
  }
  while (--numBlocks);
}


#define Md5_UpdateBlock(p) MD5_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Md5_Update(CMd5 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->count & (MD5_BLOCK_SIZE - 1);
    const unsigned num = MD5_BLOCK_SIZE - pos;
    p->count += size;
    if (num > size)
    {
      memcpy(p->buffer + pos, data, size);
      return;
    }
    if (pos != 0)
    {
      size -= num;
      memcpy(p->buffer + pos, data, num);
      data += num;
      Md5_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 6;
    if (numBlocks)
    MD5_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= MD5_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 6);
    memcpy(p->buffer, data, size);
  }
}


void Md5_Final(CMd5 *p, Byte *digest)
{
  unsigned pos = (unsigned)p->count & (MD5_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (MD5_BLOCK_SIZE - 4 * 2))
  {
    while (pos != MD5_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, MD5_BLOCK_SIZE - pos);
    Md5_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (MD5_BLOCK_SIZE - 4 * 2) - pos);
  {
    const UInt64 numBits = p->count << 3;
#if defined(MY_CPU_LE_UNALIGN)
    SetUi64 (p->buffer + MD5_BLOCK_SIZE - 4 * 2, numBits)
#else
    SetUi32a(p->buffer + MD5_BLOCK_SIZE - 4 * 2, (UInt32)(numBits))
    SetUi32a(p->buffer + MD5_BLOCK_SIZE - 4 * 1, (UInt32)(numBits >> 32))
#endif
  }
  Md5_UpdateBlock(p);

  SetUi32(digest,      p->state[0])
  SetUi32(digest + 4,  p->state[1])
  SetUi32(digest + 8,  p->state[2])
  SetUi32(digest + 12, p->state[3])
  
  Md5_Init(p);
}

#undef R1
#undef R4
#undef R16
#undef D
#undef LOAD_DATA
#undef LOAD_data32_x4
#undef F1
#undef F2
#undef F3
#undef F4
