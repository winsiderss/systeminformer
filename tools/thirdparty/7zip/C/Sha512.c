/* Sha512.c -- SHA-512 Hash
: Igor Pavlov : Public domain
This code is based on public domain code from Wei Dai's Crypto++ library. */

#include "Precomp.h"

#include <string.h>

#include "Sha512.h"
#include "RotateDefs.h"
#include "CpuArch.h"

#ifdef MY_CPU_X86_OR_AMD64
  #if   defined(Z7_LLVM_CLANG_VERSION)  && (Z7_LLVM_CLANG_VERSION  >= 170001) \
     || defined(Z7_APPLE_CLANG_VERSION) && (Z7_APPLE_CLANG_VERSION >= 170001) \
     || defined(Z7_GCC_VERSION)         && (Z7_GCC_VERSION         >= 140000) \
     || defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 2400) && (__INTEL_COMPILER <= 9900) \
     || defined(_MSC_VER) && (_MSC_VER >= 1940)
      #define Z7_COMPILER_SHA512_SUPPORTED
  #endif
#elif defined(MY_CPU_ARM64) && defined(MY_CPU_LE)
  #if defined(__ARM_FEATURE_SHA512)
    #define Z7_COMPILER_SHA512_SUPPORTED
  #else
    #if (defined(Z7_CLANG_VERSION) && (Z7_CLANG_VERSION >= 130000) \
           || defined(__GNUC__) && (__GNUC__ >= 9) \
        ) \
      || defined(Z7_MSC_VER_ORIGINAL) && (_MSC_VER >= 1940) // fix it
      #define Z7_COMPILER_SHA512_SUPPORTED
    #endif
  #endif
#endif














void Z7_FASTCALL Sha512_UpdateBlocks(UInt64 state[8], const Byte *data, size_t numBlocks);

#ifdef Z7_COMPILER_SHA512_SUPPORTED
  void Z7_FASTCALL Sha512_UpdateBlocks_HW(UInt64 state[8], const Byte *data, size_t numBlocks);

  static SHA512_FUNC_UPDATE_BLOCKS g_SHA512_FUNC_UPDATE_BLOCKS = Sha512_UpdateBlocks;
  static SHA512_FUNC_UPDATE_BLOCKS g_SHA512_FUNC_UPDATE_BLOCKS_HW;

  #define SHA512_UPDATE_BLOCKS(p) p->v.vars.func_UpdateBlocks
#else
  #define SHA512_UPDATE_BLOCKS(p) Sha512_UpdateBlocks
#endif


BoolInt Sha512_SetFunction(CSha512 *p, unsigned algo)
{
  SHA512_FUNC_UPDATE_BLOCKS func = Sha512_UpdateBlocks;
  
  #ifdef Z7_COMPILER_SHA512_SUPPORTED
    if (algo != SHA512_ALGO_SW)
    {
      if (algo == SHA512_ALGO_DEFAULT)
        func = g_SHA512_FUNC_UPDATE_BLOCKS;
      else
      {
        if (algo != SHA512_ALGO_HW)
          return False;
        func = g_SHA512_FUNC_UPDATE_BLOCKS_HW;
        if (!func)
          return False;
      }
    }
  #else
    if (algo > 1)
      return False;
  #endif

  p->v.vars.func_UpdateBlocks = func;
  return True;
}


/* define it for speed optimization */

#if 0 // 1 for size optimization
  #define STEP_PRE 1
  #define STEP_MAIN 1
#else
  #define STEP_PRE 2
  #define STEP_MAIN 4
  // #define Z7_SHA512_UNROLL
#endif

#undef Z7_SHA512_BIG_W
#if STEP_MAIN != 16
  #define Z7_SHA512_BIG_W
#endif


#define U64C(x) UINT64_CONST(x)

static MY_ALIGN(64) const UInt64 SHA512_INIT_ARRAYS[4][8] = {
{ U64C(0x8c3d37c819544da2), U64C(0x73e1996689dcd4d6), U64C(0x1dfab7ae32ff9c82), U64C(0x679dd514582f9fcf),
  U64C(0x0f6d2b697bd44da8), U64C(0x77e36f7304c48942), U64C(0x3f9d85a86a1d36c8), U64C(0x1112e6ad91d692a1)
},
{ U64C(0x22312194fc2bf72c), U64C(0x9f555fa3c84c64c2), U64C(0x2393b86b6f53b151), U64C(0x963877195940eabd),
  U64C(0x96283ee2a88effe3), U64C(0xbe5e1e2553863992), U64C(0x2b0199fc2c85b8aa), U64C(0x0eb72ddc81c52ca2)
},
{ U64C(0xcbbb9d5dc1059ed8), U64C(0x629a292a367cd507), U64C(0x9159015a3070dd17), U64C(0x152fecd8f70e5939),
  U64C(0x67332667ffc00b31), U64C(0x8eb44a8768581511), U64C(0xdb0c2e0d64f98fa7), U64C(0x47b5481dbefa4fa4)
},
{ U64C(0x6a09e667f3bcc908), U64C(0xbb67ae8584caa73b), U64C(0x3c6ef372fe94f82b), U64C(0xa54ff53a5f1d36f1),
  U64C(0x510e527fade682d1), U64C(0x9b05688c2b3e6c1f), U64C(0x1f83d9abfb41bd6b), U64C(0x5be0cd19137e2179)
}};

void Sha512_InitState(CSha512 *p, unsigned digestSize)
{
  p->v.vars.count = 0;
  memcpy(p->state, SHA512_INIT_ARRAYS[(size_t)(digestSize >> 4) - 1], sizeof(p->state));
}

void Sha512_Init(CSha512 *p, unsigned digestSize)
{
  p->v.vars.func_UpdateBlocks =
  #ifdef Z7_COMPILER_SHA512_SUPPORTED
      g_SHA512_FUNC_UPDATE_BLOCKS;
  #else
      NULL;
  #endif
  Sha512_InitState(p, digestSize);
}

#define S0(x) (Z7_ROTR64(x,28) ^ Z7_ROTR64(x,34) ^ Z7_ROTR64(x,39))
#define S1(x) (Z7_ROTR64(x,14) ^ Z7_ROTR64(x,18) ^ Z7_ROTR64(x,41))
#define s0(x) (Z7_ROTR64(x, 1) ^ Z7_ROTR64(x, 8) ^ (x >> 7))
#define s1(x) (Z7_ROTR64(x,19) ^ Z7_ROTR64(x,61) ^ (x >> 6))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) ((x&y)|(z&(x|y)))


#define W_PRE(i) (W[(i) + (size_t)(j)] = GetBe64(data + ((size_t)(j) + i) * 8))

#define blk2_main(j, i)  s1(w(j, (i)-2)) + w(j, (i)-7) + s0(w(j, (i)-15))

#ifdef Z7_SHA512_BIG_W
    // we use +i instead of +(i) to change the order to solve CLANG compiler warning for signed/unsigned.
    #define w(j, i)     W[(size_t)(j) + i]
    #define blk2(j, i)  (w(j, i) = w(j, (i)-16) + blk2_main(j, i))
#else
    #if STEP_MAIN == 16
        #define w(j, i)  W[(i) & 15]
    #else
        #define w(j, i)  W[((size_t)(j) + (i)) & 15]
    #endif
    #define blk2(j, i)  (w(j, i) += blk2_main(j, i))
#endif

#define W_MAIN(i)  blk2(j, i)


#define T1(wx, i) \
    tmp = h + S1(e) + Ch(e,f,g) + K[(i)+(size_t)(j)] + wx(i); \
    h = g; \
    g = f; \
    f = e; \
    e = d + tmp; \
    tmp += S0(a) + Maj(a, b, c); \
    d = c; \
    c = b; \
    b = a; \
    a = tmp; \

#define R1_PRE(i)  T1( W_PRE, i)
#define R1_MAIN(i) T1( W_MAIN, i)

#if (!defined(Z7_SHA512_UNROLL) || STEP_MAIN < 8) && (STEP_MAIN >= 4)
#define R2_MAIN(i) \
    R1_MAIN(i) \
    R1_MAIN(i + 1) \

#endif



#if defined(Z7_SHA512_UNROLL) && STEP_MAIN >= 8

#define T4( a,b,c,d,e,f,g,h, wx, i) \
    h += S1(e) + Ch(e,f,g) + K[(i)+(size_t)(j)] + wx(i); \
    tmp = h; \
    h += d; \
    d = tmp + S0(a) + Maj(a, b, c); \

#define R4( wx, i) \
    T4 ( a,b,c,d,e,f,g,h, wx, (i  )); \
    T4 ( d,a,b,c,h,e,f,g, wx, (i+1)); \
    T4 ( c,d,a,b,g,h,e,f, wx, (i+2)); \
    T4 ( b,c,d,a,f,g,h,e, wx, (i+3)); \

#define R4_PRE(i)  R4( W_PRE, i)
#define R4_MAIN(i) R4( W_MAIN, i)


#define T8( a,b,c,d,e,f,g,h, wx, i) \
    h += S1(e) + Ch(e,f,g) + K[(i)+(size_t)(j)] + wx(i); \
    d += h; \
    h += S0(a) + Maj(a, b, c); \

#define R8( wx, i) \
    T8 ( a,b,c,d,e,f,g,h, wx, i  ); \
    T8 ( h,a,b,c,d,e,f,g, wx, i+1); \
    T8 ( g,h,a,b,c,d,e,f, wx, i+2); \
    T8 ( f,g,h,a,b,c,d,e, wx, i+3); \
    T8 ( e,f,g,h,a,b,c,d, wx, i+4); \
    T8 ( d,e,f,g,h,a,b,c, wx, i+5); \
    T8 ( c,d,e,f,g,h,a,b, wx, i+6); \
    T8 ( b,c,d,e,f,g,h,a, wx, i+7); \

#define R8_PRE(i)  R8( W_PRE, i)
#define R8_MAIN(i) R8( W_MAIN, i)

#endif


extern
MY_ALIGN(64) const UInt64 SHA512_K_ARRAY[80];
MY_ALIGN(64) const UInt64 SHA512_K_ARRAY[80] = {
  U64C(0x428a2f98d728ae22), U64C(0x7137449123ef65cd), U64C(0xb5c0fbcfec4d3b2f), U64C(0xe9b5dba58189dbbc),
  U64C(0x3956c25bf348b538), U64C(0x59f111f1b605d019), U64C(0x923f82a4af194f9b), U64C(0xab1c5ed5da6d8118),
  U64C(0xd807aa98a3030242), U64C(0x12835b0145706fbe), U64C(0x243185be4ee4b28c), U64C(0x550c7dc3d5ffb4e2),
  U64C(0x72be5d74f27b896f), U64C(0x80deb1fe3b1696b1), U64C(0x9bdc06a725c71235), U64C(0xc19bf174cf692694),
  U64C(0xe49b69c19ef14ad2), U64C(0xefbe4786384f25e3), U64C(0x0fc19dc68b8cd5b5), U64C(0x240ca1cc77ac9c65),
  U64C(0x2de92c6f592b0275), U64C(0x4a7484aa6ea6e483), U64C(0x5cb0a9dcbd41fbd4), U64C(0x76f988da831153b5),
  U64C(0x983e5152ee66dfab), U64C(0xa831c66d2db43210), U64C(0xb00327c898fb213f), U64C(0xbf597fc7beef0ee4),
  U64C(0xc6e00bf33da88fc2), U64C(0xd5a79147930aa725), U64C(0x06ca6351e003826f), U64C(0x142929670a0e6e70),
  U64C(0x27b70a8546d22ffc), U64C(0x2e1b21385c26c926), U64C(0x4d2c6dfc5ac42aed), U64C(0x53380d139d95b3df),
  U64C(0x650a73548baf63de), U64C(0x766a0abb3c77b2a8), U64C(0x81c2c92e47edaee6), U64C(0x92722c851482353b),
  U64C(0xa2bfe8a14cf10364), U64C(0xa81a664bbc423001), U64C(0xc24b8b70d0f89791), U64C(0xc76c51a30654be30),
  U64C(0xd192e819d6ef5218), U64C(0xd69906245565a910), U64C(0xf40e35855771202a), U64C(0x106aa07032bbd1b8),
  U64C(0x19a4c116b8d2d0c8), U64C(0x1e376c085141ab53), U64C(0x2748774cdf8eeb99), U64C(0x34b0bcb5e19b48a8),
  U64C(0x391c0cb3c5c95a63), U64C(0x4ed8aa4ae3418acb), U64C(0x5b9cca4f7763e373), U64C(0x682e6ff3d6b2b8a3),
  U64C(0x748f82ee5defb2fc), U64C(0x78a5636f43172f60), U64C(0x84c87814a1f0ab72), U64C(0x8cc702081a6439ec),
  U64C(0x90befffa23631e28), U64C(0xa4506cebde82bde9), U64C(0xbef9a3f7b2c67915), U64C(0xc67178f2e372532b),
  U64C(0xca273eceea26619c), U64C(0xd186b8c721c0c207), U64C(0xeada7dd6cde0eb1e), U64C(0xf57d4f7fee6ed178),
  U64C(0x06f067aa72176fba), U64C(0x0a637dc5a2c898a6), U64C(0x113f9804bef90dae), U64C(0x1b710b35131c471b),
  U64C(0x28db77f523047d84), U64C(0x32caab7b40c72493), U64C(0x3c9ebe0a15c9bebc), U64C(0x431d67c49c100d4c),
  U64C(0x4cc5d4becb3e42b6), U64C(0x597f299cfc657e2a), U64C(0x5fcb6fab3ad6faec), U64C(0x6c44198c4a475817)
};

#define K SHA512_K_ARRAY

Z7_NO_INLINE
void Z7_FASTCALL Sha512_UpdateBlocks(UInt64 state[8], const Byte *data, size_t numBlocks)
{
  UInt64 W
#ifdef Z7_SHA512_BIG_W
      [80];
#else
      [16];
#endif
  unsigned j;
  UInt64 a,b,c,d,e,f,g,h;
#if !defined(Z7_SHA512_UNROLL) || (STEP_MAIN <= 4) || (STEP_PRE <= 4)
  UInt64 tmp;
#endif

  if (numBlocks == 0) return;
  
  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];
  f = state[5];
  g = state[6];
  h = state[7];

  do
  {

  for (j = 0; j < 16; j += STEP_PRE)
  {
    #if STEP_PRE > 4

      #if STEP_PRE < 8
      R4_PRE(0);
      #else
      R8_PRE(0);
      #if STEP_PRE == 16
      R8_PRE(8);
      #endif
      #endif

    #else

      R1_PRE(0)
      #if STEP_PRE >= 2
      R1_PRE(1)
      #if STEP_PRE >= 4
      R1_PRE(2)
      R1_PRE(3)
      #endif
      #endif
    
    #endif
  }

  for (j = 16; j < 80; j += STEP_MAIN)
  {
    #if defined(Z7_SHA512_UNROLL) && STEP_MAIN >= 8

      #if STEP_MAIN < 8
      R4_MAIN(0)
      #else
      R8_MAIN(0)
      #if STEP_MAIN == 16
      R8_MAIN(8)
      #endif
      #endif

    #else
      
      R1_MAIN(0)
      #if STEP_MAIN >= 2
      R1_MAIN(1)
      #if STEP_MAIN >= 4
      R2_MAIN(2)
      #if STEP_MAIN >= 8
      R2_MAIN(4)
      R2_MAIN(6)
      #if STEP_MAIN >= 16
      R2_MAIN(8)
      R2_MAIN(10)
      R2_MAIN(12)
      R2_MAIN(14)
      #endif
      #endif
      #endif
      #endif
    #endif
  }

  a += state[0]; state[0] = a;
  b += state[1]; state[1] = b;
  c += state[2]; state[2] = c;
  d += state[3]; state[3] = d;
  e += state[4]; state[4] = e;
  f += state[5]; state[5] = f;
  g += state[6]; state[6] = g;
  h += state[7]; state[7] = h;

  data += SHA512_BLOCK_SIZE;
  }
  while (--numBlocks);
}


#define Sha512_UpdateBlock(p) SHA512_UPDATE_BLOCKS(p)(p->state, p->buffer, 1)

void Sha512_Update(CSha512 *p, const Byte *data, size_t size)
{
  if (size == 0)
    return;
  {
    const unsigned pos = (unsigned)p->v.vars.count & (SHA512_BLOCK_SIZE - 1);
    const unsigned num = SHA512_BLOCK_SIZE - pos;
    p->v.vars.count += size;
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
      Sha512_UpdateBlock(p);
    }
  }
  {
    const size_t numBlocks = size >> 7;
    // if (numBlocks)
    SHA512_UPDATE_BLOCKS(p)(p->state, data, numBlocks);
    size &= SHA512_BLOCK_SIZE - 1;
    if (size == 0)
      return;
    data += (numBlocks << 7);
    memcpy(p->buffer, data, size);
  }
}


void Sha512_Final(CSha512 *p, Byte *digest, unsigned digestSize)
{
  unsigned pos = (unsigned)p->v.vars.count & (SHA512_BLOCK_SIZE - 1);
  p->buffer[pos++] = 0x80;
  if (pos > (SHA512_BLOCK_SIZE - 8 * 2))
  {
    while (pos != SHA512_BLOCK_SIZE) { p->buffer[pos++] = 0; }
    // memset(&p->buf.buffer[pos], 0, SHA512_BLOCK_SIZE - pos);
    Sha512_UpdateBlock(p);
    pos = 0;
  }
  memset(&p->buffer[pos], 0, (SHA512_BLOCK_SIZE - 8 * 2) - pos);
  {
    const UInt64 numBits = p->v.vars.count << 3;
    SetBe64(p->buffer + SHA512_BLOCK_SIZE - 8 * 2, 0) // = (p->v.vars.count >> (64 - 3)); (high 64-bits)
    SetBe64(p->buffer + SHA512_BLOCK_SIZE - 8 * 1, numBits)
  }
  Sha512_UpdateBlock(p);
#if 1 && defined(MY_CPU_BE)
  memcpy(digest, p->state, digestSize);
#else
  {
    const unsigned numWords = digestSize >> 3;
    unsigned i;
    for (i = 0; i < numWords; i++)
    {
      const UInt64 v = p->state[i];
      SetBe64(digest, v)
      digest += 8;
    }
    if (digestSize & 4) // digestSize == SHA512_224_DIGEST_SIZE
    {
      const UInt32 v = (UInt32)((p->state[numWords]) >> 32);
      SetBe32(digest, v)
    }
  }
#endif
  Sha512_InitState(p, digestSize);
}



// #define Z7_SHA512_PROBE_DEBUG // for debug

#if defined(Z7_SHA512_PROBE_DEBUG) || defined(Z7_COMPILER_SHA512_SUPPORTED)

#if defined(Z7_SHA512_PROBE_DEBUG) \
    || defined(_WIN32) && defined(MY_CPU_ARM64)
#ifndef Z7_SHA512_USE_PROBE
#define Z7_SHA512_USE_PROBE
#endif
#endif

#ifdef Z7_SHA512_USE_PROBE

#ifdef Z7_SHA512_PROBE_DEBUG
#include <stdio.h>
#define PRF(x) x
#else
#define PRF(x)
#endif

#if 0 || !defined(_MSC_VER) // 1 || : for debug LONGJMP mode
// MINGW doesn't support __try. So we use signal() / longjmp().
// Note: signal() / longjmp() probably is not thread-safe.
// So we must call Sha512Prepare() from main thread at program start.
#ifndef Z7_SHA512_USE_LONGJMP
#define Z7_SHA512_USE_LONGJMP
#endif
#endif

#ifdef Z7_SHA512_USE_LONGJMP
#include <signal.h>
#include <setjmp.h>
static jmp_buf g_Sha512_jmp_buf;
// static int g_Sha512_Unsupported;

#if defined(__GNUC__) && (__GNUC__ >= 8) \
    || defined(__clang__) && (__clang_major__ >= 3)
  __attribute__((noreturn))
#endif
static void Z7_CDECL Sha512_signal_Handler(int v)
{
  PRF(printf("======== Sha512_signal_Handler = %x\n", (unsigned)v);)
  // g_Sha512_Unsupported = 1;
  longjmp(g_Sha512_jmp_buf, 1);
}
#endif // Z7_SHA512_USE_LONGJMP


#if defined(_WIN32)
#include "7zWindows.h"
#endif

#if defined(MY_CPU_ARM64)
// #define Z7_SHA512_USE_SIMPLIFIED_PROBE // for debug
#endif

#ifdef Z7_SHA512_USE_SIMPLIFIED_PROBE
#include <arm_neon.h>
#if defined(__clang__)
  __attribute__((__target__("sha3")))
#elif !defined(_MSC_VER)
  __attribute__((__target__("arch=armv8.2-a+sha3")))
#endif
#endif
static BoolInt CPU_IsSupported_SHA512_Probe(void)
{
  PRF(printf("\n== CPU_IsSupported_SHA512_Probe\n");)
#if defined(_WIN32) && defined(MY_CPU_ARM64)
  // we have no SHA512 flag for IsProcessorFeaturePresent() still.
  if (!CPU_IsSupported_CRYPTO())
    return False;
  PRF(printf("==== Registry check\n");)
  {
    // we can't read ID_AA64ISAR0_EL1 register from application.
    // but ID_AA64ISAR0_EL1 register is mapped to "CP 4030" registry value.
    HKEY key = NULL;
    LONG res = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
        0, KEY_READ, &key);
    if (res != ERROR_SUCCESS)
      return False;
    {
      DWORD type = 0;
      DWORD count = sizeof(UInt64);
      UInt64 val = 0;
      res = RegQueryValueEx(key, TEXT("CP 4030"), NULL,
          &type, (LPBYTE)&val, &count);
      RegCloseKey(key);
      if (res != ERROR_SUCCESS
          || type != REG_QWORD
          || count != sizeof(UInt64)
          || ((unsigned)(val >> 12) & 0xf) != 2)
        return False;
      // we parse SHA2 field of ID_AA64ISAR0_EL1 register:
      //   0 : No SHA2 instructions implemented
      //   1 : SHA256 implemented
      //   2 : SHA256 and SHA512 implemented
    }
  }
#endif // defined(_WIN32) && defined(MY_CPU_ARM64)


#if 1  // 0 for debug to disable SHA512 PROBE code

/*
----- SHA512 PROBE -----

We suppose that "CP 4030" registry reading is enough.
But we use additional SHA512 PROBE code, because
we can catch exception here, and we don't catch exceptions,
if we call Sha512 functions from main code.

NOTE: arm64 PROBE code doesn't work, if we call it via Wine in linux-arm64.
The program just stops.
Also x64 version of PROBE code doesn't work, if we run it via Intel SDE emulator
without SHA512 support (-skl switch),
The program stops, and we have message from SDE:
  TID 0 SDE-ERROR: Executed instruction not valid for specified chip (SKYLAKE): vsha512msg1
But we still want to catch that exception instead of process stopping.
Does this PROBE code work in native Windows-arm64 (with/without sha512 hw instructions)?
Are there any ways to fix the problems with arm64-wine and x64-SDE cases?
*/

  PRF(printf("==== CPU_IsSupported_SHA512 PROBE\n");)
  {
    BoolInt isSupported = False;
#ifdef Z7_SHA512_USE_LONGJMP
    void (Z7_CDECL *signal_prev)(int);
    /*
    if (g_Sha512_Unsupported)
    {
      PRF(printf("==== g_Sha512_Unsupported\n");)
      return False;
    }
    */
    printf("====== signal(SIGILL)\n");
    signal_prev = signal(SIGILL, Sha512_signal_Handler);
    if (signal_prev == SIG_ERR)
    {
      PRF(printf("====== signal fail\n");)
      return False;
    }
    // PRF(printf("==== signal_prev = %p\n", (void *)signal_prev);)
    // docs: Before the specified function is executed,
    // the value of func is set to SIG_DFL.
    // So we can exit if (setjmp(g_Sha512_jmp_buf) != 0).
    PRF(printf("====== setjmp\n");)
    if (!setjmp(g_Sha512_jmp_buf))
#else //  Z7_SHA512_USE_LONGJMP

#ifdef _MSC_VER
#ifdef __clang_major__
  #pragma GCC diagnostic ignored "-Wlanguage-extension-token"
#endif
    __try
#endif
#endif //  Z7_SHA512_USE_LONGJMP

    {
#if defined(Z7_COMPILER_SHA512_SUPPORTED)
#ifdef Z7_SHA512_USE_SIMPLIFIED_PROBE
      // simplified sha512 check for arm64:
      const uint64x2_t a = vdupq_n_u64(1);
      const uint64x2_t b = vsha512hq_u64(a, a, a);
      PRF(printf("======== vsha512hq_u64 probe\n");)
      if ((UInt32)vgetq_lane_u64(b, 0) == 0x11800002)
#else
      MY_ALIGN(16)
      UInt64 temp[SHA512_NUM_DIGEST_WORDS + SHA512_NUM_BLOCK_WORDS];
      memset(temp, 0x5a, sizeof(temp));
      PRF(printf("======== Sha512_UpdateBlocks_HW\n");)
      Sha512_UpdateBlocks_HW(temp,
          (const Byte *)(const void *)(temp + SHA512_NUM_DIGEST_WORDS), 1);
      // PRF(printf("======== t = %x\n", (UInt32)temp[0]);)
      if ((UInt32)temp[0] == 0xa33cfdf7)
#endif
      {
        PRF(printf("======== PROBE SHA512: SHA512 is supported\n");)
        isSupported = True;
      }
#else // Z7_COMPILER_SHA512_SUPPORTED
      // for debug : we generate bad instrction or raise exception.
      // __except() doesn't catch raise() calls.
#ifdef Z7_SHA512_USE_LONGJMP
      PRF(printf("====== raise(SIGILL)\n");)
      raise(SIGILL);
#else
#if defined(_MSC_VER) && defined(MY_CPU_X86)
      __asm  ud2
#endif
#endif // Z7_SHA512_USE_LONGJMP
#endif // Z7_COMPILER_SHA512_SUPPORTED
    }

#ifdef Z7_SHA512_USE_LONGJMP
    PRF(printf("====== restore signal SIGILL\n");)
    signal(SIGILL, signal_prev);
#elif _MSC_VER
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
      PRF(printf("==== CPU_IsSupported_SHA512 __except(EXCEPTION_EXECUTE_HANDLER)\n");)
    }
#endif
    PRF(printf("== return (sha512 supported) = %d\n", isSupported);)
    return isSupported;
  }
#else
  // without SHA512 PROBE code
  return True;
#endif
}

#endif // Z7_SHA512_USE_PROBE
#endif // defined(Z7_SHA512_PROBE_DEBUG) || defined(Z7_COMPILER_SHA512_SUPPORTED)


void Sha512Prepare(void)
{
#ifdef Z7_COMPILER_SHA512_SUPPORTED
  SHA512_FUNC_UPDATE_BLOCKS f, f_hw;
  f = Sha512_UpdateBlocks;
  f_hw = NULL;
#ifdef Z7_SHA512_USE_PROBE
  if (CPU_IsSupported_SHA512_Probe())
#elif defined(MY_CPU_X86_OR_AMD64)
  if (CPU_IsSupported_SHA512() && CPU_IsSupported_AVX2())
#else
  if (CPU_IsSupported_SHA512())
#endif
  {
    // printf("\n========== HW SHA512 ======== \n");
    f = f_hw = Sha512_UpdateBlocks_HW;
  }
  g_SHA512_FUNC_UPDATE_BLOCKS    = f;
  g_SHA512_FUNC_UPDATE_BLOCKS_HW = f_hw;
#elif defined(Z7_SHA512_PROBE_DEBUG)
  CPU_IsSupported_SHA512_Probe(); // for debug
#endif
}


#undef K
#undef S0
#undef S1
#undef s0
#undef s1
#undef Ch
#undef Maj
#undef W_MAIN
#undef W_PRE
#undef w
#undef blk2_main
#undef blk2
#undef T1
#undef T4
#undef T8
#undef R1_PRE
#undef R1_MAIN
#undef R2_MAIN
#undef R4
#undef R4_PRE
#undef R4_MAIN
#undef R8
#undef R8_PRE
#undef R8_MAIN
#undef STEP_PRE
#undef STEP_MAIN
#undef Z7_SHA512_BIG_W
#undef Z7_SHA512_UNROLL
#undef Z7_COMPILER_SHA512_SUPPORTED
