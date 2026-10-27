/* Sha512Opt.c -- SHA-512 optimized code for SHA-512 hardware instructions
: Igor Pavlov : Public domain */

#include "Precomp.h"
#include "Compiler.h"
#include "CpuArch.h"

// #define Z7_USE_HW_SHA_STUB // for debug
#ifdef MY_CPU_X86_OR_AMD64
  #if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 2400) && (__INTEL_COMPILER <= 9900) // fix it
      #define USE_HW_SHA
  #elif defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 170001) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 170001) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 140000)
      #define USE_HW_SHA
      #if !defined(__INTEL_COMPILER)
      // icc defines __GNUC__, but icc doesn't support __attribute__(__target__)
      #if !defined(__SHA512__) || !defined(__AVX2__)
        #define ATTRIB_SHA512 __attribute__((__target__("sha512,avx2")))
      #endif
      #endif
  #elif defined(Z7_MSC_VER_ORIGINAL)
    #if (_MSC_VER >= 1940)
      #define USE_HW_SHA
    #else
      // #define Z7_USE_HW_SHA_STUB
    #endif
  #endif
// #endif // MY_CPU_X86_OR_AMD64
#ifndef USE_HW_SHA
  // #define Z7_USE_HW_SHA_STUB // for debug
#endif

#ifdef USE_HW_SHA

// #pragma message("Sha512 HW")

#include <immintrin.h>

#if defined (__clang__) && defined(_MSC_VER)
  #if !defined(__AVX__)
    #include <avxintrin.h>
  #endif
  #if !defined(__AVX2__)
    #include <avx2intrin.h>
  #endif
  #if !defined(__SHA512__)
    #include <sha512intrin.h>
  #endif
#else

#endif

/*
SHA512 uses:
AVX:
  _mm256_loadu_si256  (vmovdqu)
  _mm256_storeu_si256
  _mm256_set_epi32    (unused)
AVX2:
  _mm256_add_epi64     : vpaddq
  _mm256_shuffle_epi8  : vpshufb
  _mm256_shuffle_epi32 : pshufd
  _mm256_blend_epi32   : vpblendd
  _mm256_permute4x64_epi64 : vpermq     : 3c
  _mm256_permute2x128_si256: vperm2i128 : 3c
  _mm256_extracti128_si256 : vextracti128  : 3c
SHA512:
  _mm256_sha512*
*/

// K array must be aligned for 32-bytes at least.
// The compiler can look align attribute and selects
//  vmovdqu - for code without align attribute
//  vmovdqa - for code with    align attribute
extern
MY_ALIGN(64)
const UInt64 SHA512_K_ARRAY[80];
#define K SHA512_K_ARRAY


#define ADD_EPI64(dest, src)      dest = _mm256_add_epi64(dest, src);
#define SHA512_MSG1(dest, src)    dest = _mm256_sha512msg1_epi64(dest, _mm256_extracti128_si256(src, 0));
#define SHA512_MSG2(dest, src)    dest = _mm256_sha512msg2_epi64(dest, src);

#define LOAD_SHUFFLE(m, k) \
    m = _mm256_loadu_si256((const __m256i *)(const void *)(data + (k) * 32)); \
    m = _mm256_shuffle_epi8(m, mask); \

#define NNN(m0, m1, m2, m3)

#define SM1(m1, m2, m3, m0) \
    SHA512_MSG1(m0, m1); \
            
#define SM2(m2, m3, m0, m1) \
    ADD_EPI64(m0, _mm256_permute4x64_epi64(_mm256_blend_epi32(m2, m3, 3), 0x39)); \
    SHA512_MSG2(m0, m3); \

#define RND2(t0, t1, lane) \
    t0 = _mm256_sha512rnds2_epi64(t0, t1, _mm256_extracti128_si256(msg, lane));



#define R4(k, m0, m1, m2, m3, OP0, OP1) \
    msg = _mm256_add_epi64(m0, *(const __m256i *) (const void *) &K[(k) * 4]); \
    RND2(state0, state1, 0);  OP0(m0, m1, m2, m3) \
    RND2(state1, state0, 1);  OP1(m0, m1, m2, m3) \




#define R16(k, OP0, OP1, OP2, OP3, OP4, OP5, OP6, OP7) \
    R4 ( (k)*4+0, m0,m1,m2,m3, OP0, OP1 ) \
    R4 ( (k)*4+1, m1,m2,m3,m0, OP2, OP3 ) \
    R4 ( (k)*4+2, m2,m3,m0,m1, OP4, OP5 ) \
    R4 ( (k)*4+3, m3,m0,m1,m2, OP6, OP7 ) \

#define PREPARE_STATE \
    state0 = _mm256_shuffle_epi32(state0, 0x4e);              /* cdab */ \
    state1 = _mm256_shuffle_epi32(state1, 0x4e);              /* ghef */ \
    tmp = state0; \
    state0 = _mm256_permute2x128_si256(state0, state1, 0x13); /* cdgh */ \
    state1 = _mm256_permute2x128_si256(tmp,    state1, 2);    /* abef */ \


void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA512
ATTRIB_SHA512
#endif
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  const __m256i mask = _mm256_set_epi32(
      0x08090a0b,0x0c0d0e0f, 0x00010203,0x04050607,
      0x08090a0b,0x0c0d0e0f, 0x00010203,0x04050607);
  __m256i tmp, state0, state1;

  if (numBlocks == 0)
    return;

  state0 = _mm256_loadu_si256((const __m256i *) (const void *) &state[0]);
  state1 = _mm256_loadu_si256((const __m256i *) (const void *) &state[4]);
  
  PREPARE_STATE

  do
  {
    __m256i state0_save, state1_save;
    __m256i m0, m1, m2, m3;
    __m256i msg;
    // #define msg tmp

    state0_save = state0;
    state1_save = state1;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)



    R16 ( 0, NNN, NNN, SM1, NNN, SM1, SM2, SM1, SM2 )
    R16 ( 1, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 2, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 3, SM1, SM2, SM1, SM2, SM1, SM2, SM1, SM2 )
    R16 ( 4, SM1, SM2, NNN, SM2, NNN, NNN, NNN, NNN )
    ADD_EPI64(state0, state0_save)
    ADD_EPI64(state1, state1_save)
    
    data += 128;
  }
  while (--numBlocks);

  PREPARE_STATE

  _mm256_storeu_si256((__m256i *) (void *) &state[0], state0);
  _mm256_storeu_si256((__m256i *) (void *) &state[4], state1);
}

#endif // USE_HW_SHA

// gcc 8.5 also supports sha512, but we need also support in assembler that is called by gcc
#elif defined(MY_CPU_ARM64) && defined(MY_CPU_LE)
  
  #if defined(__ARM_FEATURE_SHA512)
    #define USE_HW_SHA
  #else
    #if (defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 130000) \
           || defined(__GNUC__) && (__GNUC__ >= 9) \
          ) \
      || defined(Z7_MSC_VER_ORIGINAL) && (_MSC_VER >= 1940) // fix it
      #define USE_HW_SHA
    #endif
  #endif

#ifdef USE_HW_SHA

// #pragma message("=== Sha512 HW === ")


#if defined(__clang__) || defined(__GNUC__)
#if !defined(__ARM_FEATURE_SHA512)
// #pragma message("=== we define SHA3 ATTRIB_SHA512 === ")
#if defined(__clang__)
    #define ATTRIB_SHA512 __attribute__((__target__("sha3"))) // "armv8.2-a,sha3"
#else
    #define ATTRIB_SHA512 __attribute__((__target__("arch=armv8.2-a+sha3")))
#endif
#endif
#endif


#if defined(Z7_MSC_VER_ORIGINAL)
#include <arm64_neon.h>
#else

#if defined(__clang__) && __clang_major__ < 16
#if !defined(__ARM_FEATURE_SHA512)
// #pragma message("=== we set __ARM_FEATURE_SHA512 1 === ")
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #define Z7_ARM_FEATURE_SHA512_WAS_SET 1
    #define __ARM_FEATURE_SHA512 1
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
#endif
#endif // clang

#include <arm_neon.h>

#if defined(Z7_ARM_FEATURE_SHA512_WAS_SET) && \
    defined(__ARM_FEATURE_SHA512)
    Z7_DIAGNOSTIC_IGNORE_BEGIN_RESERVED_MACRO_IDENTIFIER
    #undef __ARM_FEATURE_SHA512
    #undef Z7_ARM_FEATURE_SHA512_WAS_SET
    Z7_DIAGNOSTIC_IGNORE_END_RESERVED_MACRO_IDENTIFIER
// #pragma message("=== we undefine __ARM_FEATURE_CRYPTO === ")
#endif

#endif // Z7_MSC_VER_ORIGINAL

typedef uint64x2_t v128_64;
// typedef __n128 v128_64; // MSVC

#ifdef MY_CPU_BE
  #define MY_rev64_for_LE(x) x
#else
  #define MY_rev64_for_LE(x) vrev64q_u8(x)
#endif

#define LOAD_128_64(_p)       vld1q_u64(_p)
#define LOAD_128_8(_p)        vld1q_u8 (_p)
#define STORE_128_64(_p, _v)  vst1q_u64(_p, _v)

#define LOAD_SHUFFLE(m, k) \
    m = vreinterpretq_u64_u8( \
        MY_rev64_for_LE( \
        LOAD_128_8(data + (k) * 16))); \

// K array must be aligned for 16-bytes at least.
extern
MY_ALIGN(64)
const UInt64 SHA512_K_ARRAY[80];
#define K SHA512_K_ARRAY

#define NN(m0, m1, m4, m5, m7)
#define SM(m0, m1, m4, m5, m7) \
    m0 = vsha512su1q_u64(vsha512su0q_u64(m0, m1), m7, vextq_u64(m4, m5, 1));

#define R2(k, m0,m1,m2,m3,m4,m5,m6,m7, a0,a1,a2,a3, OP) \
    OP(m0, m1, m4, m5, m7) \
    t = vaddq_u64(m0, vld1q_u64(k)); \
    t = vaddq_u64(vextq_u64(t, t, 1), a3); \
    t = vsha512hq_u64(t, vextq_u64(a2, a3, 1), vextq_u64(a1, a2, 1)); \
    a3 = vsha512h2q_u64(t, a1, a0); \
    a1 = vaddq_u64(a1, t); \

#define R8(k,     m0,m1,m2,m3,m4,m5,m6,m7, OP) \
    R2 ( (k)+0*2, m0,m1,m2,m3,m4,m5,m6,m7, a0,a1,a2,a3, OP ) \
    R2 ( (k)+1*2, m1,m2,m3,m4,m5,m6,m7,m0, a3,a0,a1,a2, OP ) \
    R2 ( (k)+2*2, m2,m3,m4,m5,m6,m7,m0,m1, a2,a3,a0,a1, OP ) \
    R2 ( (k)+3*2, m3,m4,m5,m6,m7,m0,m1,m2, a1,a2,a3,a0, OP ) \

#define R16(k, OP) \
    R8 ( (k)+0*2, m0,m1,m2,m3,m4,m5,m6,m7, OP ) \
    R8 ( (k)+4*2, m4,m5,m6,m7,m0,m1,m2,m3, OP ) \


void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);
#ifdef ATTRIB_SHA512
ATTRIB_SHA512
#endif
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  v128_64 a0, a1, a2, a3;

  if (numBlocks == 0)
    return;
  a0 = LOAD_128_64(&state[0]);
  a1 = LOAD_128_64(&state[2]);
  a2 = LOAD_128_64(&state[4]);
  a3 = LOAD_128_64(&state[6]);
  do
  {
    v128_64 a0_save, a1_save, a2_save, a3_save;
    v128_64 m0, m1, m2, m3, m4, m5, m6, m7;
    v128_64 t;
    unsigned i;
    const UInt64 *k_ptr;
    
    LOAD_SHUFFLE (m0, 0)
    LOAD_SHUFFLE (m1, 1)
    LOAD_SHUFFLE (m2, 2)
    LOAD_SHUFFLE (m3, 3)
    LOAD_SHUFFLE (m4, 4)
    LOAD_SHUFFLE (m5, 5)
    LOAD_SHUFFLE (m6, 6)
    LOAD_SHUFFLE (m7, 7)

    a0_save = a0;
    a1_save = a1;
    a2_save = a2;
    a3_save = a3;
    
    R16 ( K, NN )
    k_ptr = K + 16;
    for (i = 0; i < 4; i++)
    {
      R16 ( k_ptr, SM )
      k_ptr += 16;
    }
    
    a0 = vaddq_u64(a0, a0_save);
    a1 = vaddq_u64(a1, a1_save);
    a2 = vaddq_u64(a2, a2_save);
    a3 = vaddq_u64(a3, a3_save);

    data += 128;
  }
  while (--numBlocks);

  STORE_128_64(&state[0], a0);
  STORE_128_64(&state[2], a1);
  STORE_128_64(&state[4], a2);
  STORE_128_64(&state[6], a3);
}

#endif // USE_HW_SHA

#endif // MY_CPU_ARM_OR_ARM64


#if !defined(USE_HW_SHA) && defined(Z7_USE_HW_SHA_STUB)
// #error Stop_Compiling_UNSUPPORTED_SHA
// #include <stdlib.h>
// We can compile this file with another C compiler,
// or we can compile asm version.
// So we can generate real code instead of this stub function.
// #include "Sha512.h"
// #if defined(_MSC_VER)
#pragma message("Sha512 HW-SW stub was used")
// #endif
void Z7_FASTCALL Sha512_UpdateBlocks   (UInt64 state[8], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);
void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  Sha512_UpdateBlocks(state, data, numBlocks);
  /*
  UNUSED_VAR(state);
  UNUSED_VAR(data);
  UNUSED_VAR(numBlocks);
  exit(1);
  return;
  */
}
#endif


#undef K
#undef RND2
#undef MY_rev64_for_LE
#undef NN
#undef NNN
#undef LOAD_128
#undef STORE_128
#undef LOAD_SHUFFLE
#undef SM1
#undef SM2
#undef SM
#undef R2
#undef R4
#undef R16
#undef PREPARE_STATE
#undef USE_HW_SHA
#undef ATTRIB_SHA512
#undef USE_VER_MIN
#undef Z7_USE_HW_SHA_STUB
