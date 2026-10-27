/* Xxh64.c -- XXH64 hash calculation
original code: Copyright (c) Yann Collet.
modified by Igor Pavlov.
This source code is licensed under BSD 2-Clause License.
*/

#include "Precomp.h"

#include "CpuArch.h"
#include "RotateDefs.h"
#include "Xxh64.h"

#define Z7_XXH_PRIME64_1  UINT64_CONST(0x9E3779B185EBCA87)
#define Z7_XXH_PRIME64_2  UINT64_CONST(0xC2B2AE3D27D4EB4F)
#define Z7_XXH_PRIME64_3  UINT64_CONST(0x165667B19E3779F9)
#define Z7_XXH_PRIME64_4  UINT64_CONST(0x85EBCA77C2B2AE63)
#define Z7_XXH_PRIME64_5  UINT64_CONST(0x27D4EB2F165667C5)

void Xxh64State_Init(CXxh64State *p)
{
  const UInt64 seed = 0;
  p->v[0] = seed + Z7_XXH_PRIME64_1 + Z7_XXH_PRIME64_2;
  p->v[1] = seed + Z7_XXH_PRIME64_2;
  p->v[2] = seed;
  p->v[3] = seed - Z7_XXH_PRIME64_1;
}

#if !defined(MY_CPU_64BIT) && defined(MY_CPU_X86) && defined(_MSC_VER)
  #define Z7_XXH64_USE_ASM
#elif !defined(MY_CPU_LE_UNALIGN_64) // && defined (MY_CPU_LE)
  #define Z7_XXH64_USE_ALIGNED
#endif

#ifdef Z7_XXH64_USE_ALIGNED
  #define Xxh64State_UpdateBlocks_Unaligned_Select  Xxh64State_UpdateBlocks_Unaligned
#else
  #define Xxh64State_UpdateBlocks_Unaligned_Select  Xxh64State_UpdateBlocks
#endif

#if !defined(MY_CPU_64BIT) && defined(MY_CPU_X86) \
    && defined(Z7_MSC_VER_ORIGINAL) && Z7_MSC_VER_ORIGINAL > 1200
/* we try to avoid __allmul calls in MSVC for 64-bit multiply.
   But MSVC6 still uses __allmul for our code.
   So for MSVC6 we use default 64-bit multiply without our optimization.
*/
#define LOW32(b)        ((UInt32)(b & 0xffffffff))
/* MSVC compiler (MSVC > 1200) can use "mul" instruction
   without __allmul for our MY_emulu MACRO.
   MY_emulu is similar to __emulu(a, b) MACRO */
#define MY_emulu(a, b)      ((UInt64)(a) * (b))
#define MY_SET_HIGH32(a)    ((UInt64)(a) << 32)
#define MY_MUL32_SET_HIGH32(a, b) MY_SET_HIGH32((UInt32)(a) * (UInt32)(b))
// /*
#define MY_MUL64(a, b) \
    ( MY_emulu((UInt32)(a), LOW32(b)) + \
      MY_SET_HIGH32( \
        (UInt32)((a) >> 32) * LOW32(b) + \
        (UInt32)(a) * (UInt32)((b) >> 32) \
      ))
// */
/*
#define MY_MUL64(a, b) \
    ( MY_emulu((UInt32)(a), LOW32(b)) \
      + MY_MUL32_SET_HIGH32((a) >> 32, LOW32(b)) + \
      + MY_MUL32_SET_HIGH32(a, (b) >> 32) \
    )
*/

#define MY_MUL_32_64(a32, b) \
    ( MY_emulu((UInt32)(a32), LOW32(b)) \
      + MY_MUL32_SET_HIGH32(a32, (b) >> 32) \
    )

#else
#define MY_MUL64(a, b)        ((a) * (b))
#define MY_MUL_32_64(a32, b)  ((a32) * (UInt64)(b))
#endif


static
Z7_FORCE_INLINE
UInt64 Xxh64_Round(UInt64 acc, UInt64 input)
{
  acc += MY_MUL64(input, Z7_XXH_PRIME64_2);
  acc = Z7_ROTL64(acc, 31);
  return MY_MUL64(acc, Z7_XXH_PRIME64_1);
}

static UInt64 Xxh64_Merge(UInt64 acc, UInt64 val)
{
  acc ^= Xxh64_Round(0, val);
  return MY_MUL64(acc, Z7_XXH_PRIME64_1) + Z7_XXH_PRIME64_4;
}


#ifdef Z7_XXH64_USE_ASM

#define Z7_XXH_PRIME64_1_HIGH  0x9E3779B1
#define Z7_XXH_PRIME64_1_LOW   0x85EBCA87
#define Z7_XXH_PRIME64_2_HIGH  0xC2B2AE3D
#define Z7_XXH_PRIME64_2_LOW   0x27D4EB4F

void
Z7_NO_INLINE
__declspec(naked)
Z7_FASTCALL
Xxh64State_UpdateBlocks(CXxh64State *p, const void *data, const void *end)
{
 #if !defined(__clang__)
  UNUSED_VAR(p)
  UNUSED_VAR(data)
  UNUSED_VAR(end)
 #endif
  __asm   push    ebx
  __asm   push    ebp
  __asm   push    esi
  __asm   push    edi

  #define STACK_OFFSET  4 * 8
  __asm   sub     esp, STACK_OFFSET

#define COPY_1(n) \
  __asm   mov     eax, [ecx + n * 4] \
  __asm   mov     [esp + n * 4], eax \

#define COPY_2(n) \
  __asm   mov     eax, [esp + n * 4] \
  __asm   mov     [ecx + n * 4], eax \

  COPY_1(0)
  __asm   mov     edi, [ecx + 1 * 4] \
  COPY_1(2)
  COPY_1(3)
  COPY_1(4)
  COPY_1(5)
  COPY_1(6)
  COPY_1(7)

  __asm   mov     esi, edx \
  __asm   mov     [esp + 0 * 8 + 4], ecx
  __asm   mov     ecx, Z7_XXH_PRIME64_2_LOW \
  __asm   mov     ebp, Z7_XXH_PRIME64_1_LOW \

#define R(n, state1, state1_reg) \
  __asm   mov     eax, [esi + n * 8] \
  __asm   imul    ebx, eax, Z7_XXH_PRIME64_2_HIGH \
  __asm   add     ebx, state1 \
  __asm   mul     ecx \
  __asm   add     edx, ebx \
  __asm   mov     ebx, [esi + n * 8 + 4] \
  __asm   imul    ebx, ecx \
  __asm   add     eax, [esp + n * 8] \
  __asm   adc     edx, ebx \
  __asm   mov     ebx, eax \
  __asm   shld    eax, edx, 31 \
  __asm   shld    edx, ebx, 31 \
  __asm   imul    state1_reg, eax, Z7_XXH_PRIME64_1_HIGH \
  __asm   imul    edx, ebp \
  __asm   add     state1_reg, edx \
  __asm   mul     ebp \
  __asm   add     state1_reg, edx \
  __asm   mov     [esp + n * 8], eax \

#define R2(n) \
  R(n, [esp + n * 8 + 4], ebx) \
  __asm   mov     [esp + n * 8 + 4], ebx \

  __asm   align 16
  __asm   main_loop:
  R(0, edi, edi)
  R2(1)
  R2(2)
  R2(3)
  __asm   add     esi, 32
  __asm   cmp     esi, [esp + STACK_OFFSET + 4 * 4 + 4]
  __asm   jne     main_loop

  __asm   mov     ecx, [esp + 0 * 8 + 4]

  COPY_2(0)
  __asm   mov     [ecx + 1 * 4], edi
  COPY_2(2)
  COPY_2(3)
  COPY_2(4)
  COPY_2(5)
  COPY_2(6)
  COPY_2(7)

  __asm   add     esp, STACK_OFFSET
  __asm   pop     edi
  __asm   pop     esi
  __asm   pop     ebp
  __asm   pop     ebx
  __asm   ret     4
}

#else

#ifdef Z7_XXH64_USE_ALIGNED
static
#endif
void
Z7_NO_INLINE
Z7_FASTCALL
Xxh64State_UpdateBlocks_Unaligned_Select(CXxh64State *p, const void *_data, const void *end)
{
  const Byte *data = (const Byte *)_data;
  UInt64 v0, v1, v2, v3;
  v0 = p->v[0];
  v1 = p->v[1];
  v2 = p->v[2];
  v3 = p->v[3];
  do
  {
    v0 = Xxh64_Round(v0, GetUi64(data));  data += 8;
    v1 = Xxh64_Round(v1, GetUi64(data));  data += 8;
    v2 = Xxh64_Round(v2, GetUi64(data));  data += 8;
    v3 = Xxh64_Round(v3, GetUi64(data));  data += 8;
  }
  while (data != end);
  p->v[0] = v0;
  p->v[1] = v1;
  p->v[2] = v2;
  p->v[3] = v3;
}


#ifdef Z7_XXH64_USE_ALIGNED

static
void
Z7_NO_INLINE
Z7_FASTCALL
Xxh64State_UpdateBlocks_Aligned(CXxh64State *p, const void *_data, const void *end)
{
  const Byte *data = (const Byte *)_data;
  UInt64 v0, v1, v2, v3;
  v0 = p->v[0];
  v1 = p->v[1];
  v2 = p->v[2];
  v3 = p->v[3];
  do
  {
    v0 = Xxh64_Round(v0, GetUi64a(data));  data += 8;
    v1 = Xxh64_Round(v1, GetUi64a(data));  data += 8;
    v2 = Xxh64_Round(v2, GetUi64a(data));  data += 8;
    v3 = Xxh64_Round(v3, GetUi64a(data));  data += 8;
  }
  while (data != end);
  p->v[0] = v0;
  p->v[1] = v1;
  p->v[2] = v2;
  p->v[3] = v3;
}

void
Z7_NO_INLINE
Z7_FASTCALL
Xxh64State_UpdateBlocks(CXxh64State *p, const void *data, const void *end)
{
  if (((unsigned)(ptrdiff_t)data & 7) == 0)
    Xxh64State_UpdateBlocks_Aligned(p, data, end);
  else
    Xxh64State_UpdateBlocks_Unaligned(p, data, end);
}

#endif // Z7_XXH64_USE_ALIGNED
#endif // Z7_XXH64_USE_ASM

UInt64 Xxh64State_Digest(const CXxh64State *p, const void *_data, UInt64 count)
{
  UInt64 h = p->v[2];
 
  if (count >= 32)
  {
    h = Z7_ROTL64(p->v[0], 1) +
        Z7_ROTL64(p->v[1], 7) +
        Z7_ROTL64(h, 12) +
        Z7_ROTL64(p->v[3], 18);
    h = Xxh64_Merge(h, p->v[0]);
    h = Xxh64_Merge(h, p->v[1]);
    h = Xxh64_Merge(h, p->v[2]);
    h = Xxh64_Merge(h, p->v[3]);
  }
  else
    h += Z7_XXH_PRIME64_5;
  
  h += count;
  
  // XXH64_finalize():
  {
    unsigned cnt = (unsigned)count & 31;
    const Byte *data = (const Byte *)_data;
    while (cnt >= 8)
    {
      h ^= Xxh64_Round(0, GetUi64(data));
      data += 8;
      h = Z7_ROTL64(h, 27);
      h = MY_MUL64(h, Z7_XXH_PRIME64_1) + Z7_XXH_PRIME64_4;
      cnt -= 8;
    }
    if (cnt >= 4)
    {
      const UInt32 v = GetUi32(data);
      data += 4;
      h ^= MY_MUL_32_64(v, Z7_XXH_PRIME64_1);
      h = Z7_ROTL64(h, 23);
      h = MY_MUL64(h, Z7_XXH_PRIME64_2) + Z7_XXH_PRIME64_3;
      cnt -= 4;
    }
    while (cnt)
    {
      const UInt32 v = *data++;
      h ^= MY_MUL_32_64(v, Z7_XXH_PRIME64_5);
      h = Z7_ROTL64(h, 11);
      h = MY_MUL64(h, Z7_XXH_PRIME64_1);
      cnt--;
    }
    // XXH64_avalanche(h):
    h ^= h >> 33;  h = MY_MUL64(h, Z7_XXH_PRIME64_2);
    h ^= h >> 29;  h = MY_MUL64(h, Z7_XXH_PRIME64_3);
    h ^= h >> 32;
    return h;
  }
}


void Xxh64_Init(CXxh64 *p)
{
  Xxh64State_Init(&p->state);
  p->count = 0;
  p->buf64[0] = 0;
  p->buf64[1] = 0;
  p->buf64[2] = 0;
  p->buf64[3] = 0;
}

void Xxh64_Update(CXxh64 *p, const void *_data, size_t size)
{
  const Byte *data = (const Byte *)_data;
  unsigned cnt;
  if (size == 0)
    return;
  cnt = (unsigned)p->count;
  p->count += size;
  
  if (cnt &= 31)
  {
    unsigned rem = 32 - cnt;
    Byte *dest = (Byte *)p->buf64 + cnt;
    if (rem > size)
      rem = (unsigned)size;
    size -= rem;
    cnt += rem;
    // memcpy((Byte *)p->buf64 + cnt, data, rem);
    do
      *dest++ = *data++;
    while (--rem);
    if (cnt != 32)
      return;
#ifdef Z7_XXH64_USE_ALIGNED
    Xxh64State_UpdateBlocks_Aligned
#else
    Xxh64State_UpdateBlocks_Unaligned_Select
#endif
      (&p->state, p->buf64, &p->buf64[4]);
  }

  if (size &= ~(size_t)31)
  {
#ifdef Z7_XXH64_USE_ALIGNED
    if (((unsigned)(ptrdiff_t)data & 7) == 0)
      Xxh64State_UpdateBlocks_Aligned(&p->state, data, data + size);
    else
#endif
      Xxh64State_UpdateBlocks_Unaligned_Select(&p->state, data, data + size);
    data += size;
  }
  
  cnt = (unsigned)p->count & 31;
  if (cnt)
  {
    // memcpy(p->buf64, data, cnt);
    Byte *dest = (Byte *)p->buf64;
    do
      *dest++ = *data++;
    while (--cnt);
  }
}
