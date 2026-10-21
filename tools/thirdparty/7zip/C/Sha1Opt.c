/* Sha1Opt.c -- SHA-1 optimized code for SHA-1 hardware instructions
: Igor Pavlov : Public domain */

#include "Precomp.h"
#include "Compiler.h"
#include "CpuArch.h"

// #define Z7_USE_HW_SHA_STUB // for debug
#ifdef MY_CPU_X86_OR_AMD64
  #if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1600) // fix that check
      #define USE_HW_SHA
  #elif defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 30800) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 50100) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 40900)
      #define USE_HW_SHA
      #if !defined(__INTEL_COMPILER)
      // icc defines __GNUC__, but icc doesn't support __attribute__(__target__)
      #if !defined(__SHA__) || !defined(__SSSE3__)
        #define ATTRIB_SHA __attribute__((__target__("sha,ssse3")))
      #endif
      #endif
  #elif defined(_MSC_VER)
    #if (_MSC_VER >= 1900)
      #define USE_HW_SHA
    #else
      #define Z7_USE_HW_SHA_STUB
    #endif
  #endif
// #endif // MY_CPU_X86_OR_AMD64
#ifndef USE_HW_SHA
  // #define Z7_USE_HW_SHA_STUB // for debug
#endif

#ifdef USE_HW_SHA

// #pragma message("Sha1 HW")




// sse/sse2/ssse3:
#include <tmmintrin.h>
// sha*:
#include <immintrin.h>

#if defined (__clang__) && defined(_MSC_VER)
  #if !defined(__SHA__)
    #include <shaintrin.h>
  #endif
#else

#endif

/*
SHA1 uses:
SSE2:
  _mm_loadu_si128
  _mm_storeu_si128
  _mm_set_epi32
  _mm_add_epi32
  _mm_shuffle_epi32 / pshufd
  _mm_xor_si128
  _mm_cvtsi128_si32
  _mm_cvtsi32_si128
SSSE3:
  _mm_shuffle_epi8 / pshufb

SHA:
  _mm_sha1*
*/

#define XOR_SI128(dest, src)      dest = _mm_xor_si128(dest, src);
#define SHUFFLE_EPI8(dest, mask)  dest = _mm_shuffle_epi8(dest, mask);
#define SHUFFLE_EPI32(dest, mask) dest = _mm_shuffle_epi32(dest, mask);
#ifdef __clang__
#define SHA1_RNDS4_RET_TYPE_CAST (__m128i)
#else
#define SHA1_RNDS4_RET_TYPE_CAST
#endif
#define SHA1_RND4(abcd, e0, f)    abcd = SHA1_RNDS4_RET_TYPE_CAST _mm_sha1rnds4_epu32(abcd, e0, f);
#define SHA1_NEXTE(e, m)          e = _mm_sha1nexte_epu32(e, m);
#define ADD_EPI32(dest, src)      dest = _mm_add_epi32(dest, src);
#define SHA1_MSG1(dest, src)      dest = _mm_sha1msg1_epu32(dest, src);
#define SHA1_MSG2(dest, src)      dest = _mm_sha1msg2_epu32(dest, src);

#define LOAD_SHUFFLE(m, k) \
    m = _mm_loadu_si128((const __m128i *)(const void *)(data + (k) * 16)); \
    SHUFFLE_EPI8(m, mask) \

#define NNN(m0, m1, m2, m3)

#define SM1(m0, m1, m2, m3) \
    SHA1_MSG1(m0, m1) \

#define SM2(m0, m1, m2, m3) \
    XOR_SI128(m3, m1) \
    SHA1_MSG2(m3, m2) \

#define SM3(m0, m1, m2, m3) \
    XOR_SI128(m3, m1) \
    SM1(m0, m1, m2, m3) \
    SHA1_MSG2(m3, m2) \

#define R4(k, m0, m1, m2, m3, e0, e1, OP) \
    e1 = abcd; \
    SHA1_RND4(abcd, e0, (k) / 5) \
    SHA1_NEXTE(e1, m1) \
    OP(m0, m1, m2, m3) \



#define R16(k, mx, OP0, OP1, OP2, OP3) \
    R4 ( (k)*4+0, m0,m1,m2,m3, e0,e1, OP0 ) \
    R4 ( (k)*4+1, m1,m2,m3,m0, e1,e0, OP1 ) \
    R4 ( (k)*4+2, m2,m3,m0,m1, e0,e1, OP2 ) \
    R4 ( (k)*4+3, m3,mx,m1,m2, e1,e0, OP3 ) \

#define PREPARE_STATE \
    SHUFFLE_EPI32 (abcd, 0x1B) \
    SHUFFLE_EPI32 (e0,   0x1B) \





void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  const __m128i mask = _mm_set_epi32(0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f);

  
  __m128i abcd, e0;

  if (numBlocks == 0)
    return;
  
  abcd = _mm_loadu_si128((const __m128i *) (const void *) &state[0]); // dbca
  e0 = _mm_cvtsi32_si128((int)state[4]); // 000e
  
  PREPARE_STATE
  
  do
  {
    __m128i abcd_save, e2;
    __m128i m0, m1, m2, m3;
    __m128i e1;
    

    abcd_save = abcd;
    e2 = e0;

    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)

    ADD_EPI32(e0, m0)
    
    R16 ( 0, m0, SM1, SM3, SM3, SM3 )
    R16 ( 1, m0, SM3, SM3, SM3, SM3 )
    R16 ( 2, m0, SM3, SM3, SM3, SM3 )
    R16 ( 3, m0, SM3, SM3, SM3, SM3 )
    R16 ( 4, e2, SM2, NNN, NNN, NNN )
    
    ADD_EPI32(abcd, abcd_save)
    
    data += 64;
  }
  while (--numBlocks);

  PREPARE_STATE

  _mm_storeu_si128((__m128i *) (void *) state, abcd);
  *(state + 4) = (UInt32)_mm_cvtsi128_si32(e0);
}

#endif // USE_HW_SHA

#elif defined(MY_CPU_ARM_OR_ARM64) && defined(MY_CPU_LE) \
   && (!defined(Z7_MSC_VER_ORIGINAL) || (_MSC_VER >= 1929) && (_MSC_FULL_VER >= 192930037))
  #if   defined(__ARM_FEATURE_SHA2) \
     || defined(__ARM_FEATURE_CRYPTO)
    #define USE_HW_SHA
  #else
    #if  defined(MY_CPU_ARM64) \
      || defined(__ARM_ARCH) && (__ARM_ARCH >= 4) \
      || defined(Z7_MSC_VER_ORIGINAL)
    #if  defined(__ARM_FP) && \
          (   defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 30800) \
           || defined(__GNUC__) && (__GNUC__ >= 6) \
          ) \
      || defined(Z7_MSC_VER_ORIGINAL) && (_MSC_VER >= 1910)
    #if  defined(MY_CPU_ARM64) \
      || !defined(Z7_CLANG_VERSION) \
      || defined(__ARM_NEON) && \
          (Z7_CLANG_VERSION < 170000 || \
           Z7_CLANG_VERSION > 170001)
      #define USE_HW_SHA
    #endif
    #endif
    #endif
  #endif

#ifdef USE_HW_SHA

// #pragma message("=== Sha1 HW === ")
// __ARM_FEATURE_CRYPTO macro is deprecated in favor of the finer grained feature macro __ARM_FEATURE_SHA2

#if defined(__clang__) || defined(__GNUC__)
#if !defined(__ARM_FEATURE_SHA2) && \
    !defined(__ARM_FEATURE_CRYPTO)
  #ifdef MY_CPU_ARM64
#if defined(__clang__)
    #define ATTRIB_SHA __attribute__((__target__("crypto")))
#else
    #define ATTRIB_SHA __attribute__((__target__("+crypto")))
#endif
  #else
#if defined(__clang__) && (__clang_major__ >= 1)
    #define ATTRIB_SHA __attribute__((__target__("armv8-a,sha2")))
#else
    #define ATTRIB_SHA __attribute__((__target__("fpu=crypto-neon-fp-armv8")))
#endif
  #endif
#endif
#else
  // _MSC_VER
  // for arm32
  #define _ARM_USE_NEW_NEON_INTRINSICS
#endif

#if defined(Z7_MSC_VER_ORIGINAL) && defined(MY_CPU_ARM64)
#include <arm64_neon.h>
#else

#if defined(__clang__) && __clang_major__ < 16
#if !defined(__ARM_FEATURE_SHA2) && \
    !defined(__ARM_FEATURE_CRYPTO)
//     #pragma message("=== we set __ARM_FEATURE_CRYPTO 1 === ")
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #define Z7_ARM_FEATURE_CRYPTO_WAS_SET 1
// #if defined(__clang__) && __clang_major__ < 13
    #define __ARM_FEATURE_CRYPTO 1
// #else
    #define __ARM_FEATURE_SHA2 1
// #endif
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif
#endif // clang

#if defined(__clang__)

#if defined(__ARM_ARCH) && __ARM_ARCH < 8
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
//    #pragma message("#define __ARM_ARCH 8")
    #undef  __ARM_ARCH
    #define __ARM_ARCH 8
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif

#endif // clang

#include <arm_neon.h>

#if defined(Z7_ARM_FEATURE_CRYPTO_WAS_SET) && \
    defined(__ARM_FEATURE_CRYPTO) && \
    defined(__ARM_FEATURE_SHA2)
Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #undef __ARM_FEATURE_CRYPTO
    #undef __ARM_FEATURE_SHA2
    #undef Z7_ARM_FEATURE_CRYPTO_WAS_SET
Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
//    #pragma message("=== we undefine __ARM_FEATURE_CRYPTO === ")
#endif

#endif // Z7_MSC_VER_ORIGINAL

typedef uint32x4_t v128;
// typedef __n128 v128; // MSVC
// the bug in clang 3.8.1:
// __builtin_neon_vgetq_lane_i32((int8x16_t)__s0, __p1);
#if defined(__clang__) && (__clang_major__ <= 9)
#pragma GCC diagnostic ignored "-Wvector-conversion"
#endif

#ifdef MY_CPU_BE
  #define MY_rev32_for_LE(x) x
#else
  #define MY_rev32_for_LE(x) vrev32q_u8(x)
#endif

#define LOAD_128_32(_p)       vld1q_u32(_p)
#define LOAD_128_8(_p)        vld1q_u8 (_p)
#define STORE_128_32(_p, _v)  vst1q_u32(_p, _v)

#define LOAD_SHUFFLE(m, k) \
    m = vreinterpretq_u32_u8( \
        MY_rev32_for_LE( \
        LOAD_128_8(data + (k) * 16))); \

#define N0(dest, src2, src3)
#define N1(dest, src)
#define U0(dest, src2, src3)  dest = vsha1su0q_u32(dest, src2, src3);
#define U1(dest, src)         dest = vsha1su1q_u32(dest, src);
#define C(e)                  abcd = vsha1cq_u32(abcd, e, t)
#define P(e)                  abcd = vsha1pq_u32(abcd, e, t)
#define M(e)                  abcd = vsha1mq_u32(abcd, e, t)
#define H(e)                  e = vsha1h_u32(vgetq_lane_u32(abcd, 0))
#define T(m, c)               t = vaddq_u32(m, c)

#define R16(d0,d1,d2,d3, f0,z0, f1,z1, f2,z2, f3,z3, w0,w1,w2,w3) \
    T(m0, d0);  f0(m3, m0, m1)  z0(m2, m1)  H(e1);  w0(e0); \
    T(m1, d1);  f1(m0, m1, m2)  z1(m3, m2)  H(e0);  w1(e1); \
    T(m2, d2);  f2(m1, m2, m3)  z2(m0, m3)  H(e1);  w2(e0); \
    T(m3, d3);  f3(m2, m3, m0)  z3(m1, m0)  H(e0);  w3(e1); \


void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA
ATTRIB_SHA
#endif
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[8], const Byte *data, size_t numBlocks)
{
  v128 abcd;
  v128 c0, c1, c2, c3;
  uint32_t e0;

  if (numBlocks == 0)
    return;

  c0 = vdupq_n_u32(0x5a827999);
  c1 = vdupq_n_u32(0x6ed9eba1);
  c2 = vdupq_n_u32(0x8f1bbcdc);
  c3 = vdupq_n_u32(0xca62c1d6);

  abcd = LOAD_128_32(&state[0]);
  e0 = state[4];
  
  do
  {
    v128 abcd_save;
    v128 m0, m1, m2, m3;
    v128 t;
    uint32_t e0_save, e1;

    abcd_save = abcd;
    e0_save = e0;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)
                     
    R16 ( c0,c0,c0,c0, N0,N1, U0,N1, U0,U1, U0,U1, C,C,C,C )
    R16 ( c0,c1,c1,c1, U0,U1, U0,U1, U0,U1, U0,U1, C,P,P,P )
    R16 ( c1,c1,c2,c2, U0,U1, U0,U1, U0,U1, U0,U1, P,P,M,M )
    R16 ( c2,c2,c2,c3, U0,U1, U0,U1, U0,U1, U0,U1, M,M,M,P )
    R16 ( c3,c3,c3,c3, U0,U1, N0,U1, N0,N1, N0,N1, P,P,P,P )
                                                                                                                     
    abcd = vaddq_u32(abcd, abcd_save);
    e0 += e0_save;
    
    data += 64;
  }
  while (--numBlocks);

  STORE_128_32(&state[0], abcd);
  state[4] = e0;
}

#endif // USE_HW_SHA

#endif // MY_CPU_ARM_OR_ARM64

#if !defined(USE_HW_SHA) && defined(Z7_USE_HW_SHA_STUB)
// #error Stop_Compiling_UNSUPPORTED_SHA
// #include <stdlib.h>
// #include "Sha1.h"
// #if defined(_MSC_VER)
#pragma message("Sha1   HW-SW stub was used")
// #endif
void Z7_FASTCALL Sha1_UpdateBlocks   (UInt32 state[5], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha1_UpdateBlocks_HW(UInt32 state[5], const Byte *data, size_t numBlocks)
{
  Sha1_UpdateBlocks(state, data, numBlocks);
  /*
  UNUSED_VAR(state);
  UNUSED_VAR(data);
  UNUSED_VAR(numBlocks);
  exit(1);
  return;
  */
}
#endif

#undef U0
#undef U1
#undef N0
#undef N1
#undef C
#undef P
#undef M
#undef H
#undef T
#undef MY_rev32_for_LE
#undef NNN
#undef LOAD_128
#undef STORE_128
#undef LOAD_SHUFFLE
#undef SM1
#undef SM2
#undef SM3
#undef NNN
#undef R4
#undef R16
#undef PREPARE_STATE
#undef USE_HW_SHA
#undef ATTRIB_SHA
#undef USE_VER_MIN
#undef Z7_USE_HW_SHA_STUB
