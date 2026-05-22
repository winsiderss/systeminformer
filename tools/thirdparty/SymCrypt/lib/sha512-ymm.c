#include "precomp.h"

#if  SYMCRYPT_CPU_AMD64


extern SYMCRYPT_ALIGN_AT(64) const  UINT64 SymCryptSha512K[81];


// Endianness transformation for 4 64-bit values in a YMM register
const SYMCRYPT_ALIGN_AT(32) UINT64 BYTE_REVERSE_64X2[4] = {
    0x0001020304050607, 0x08090a0b0c0d0e0f,
    0x0001020304050607, 0x08090a0b0c0d0e0f
};

// Rotate right each 64-bit value in a YMM register by 1 byte
const SYMCRYPT_ALIGN_AT(32) UINT64 BYTE_ROTATE_64[4] = {
    0x0007060504030201, 0x080f0e0d0c0b0a09,
    0x0007060504030201, 0x080f0e0d0c0b0a09,
};


#if SYMCRYPT_MS_VC && !defined(__clang__)
#define RORX_U32  _rorx_u32
#define RORX_U64  _rorx_u64
#else
#define RORX_U32  ROR32
#define RORX_U64  ROR64
#endif // SYMCRYPT_MS_VC && !defined(__clang__)


//
// For documentation on these function see FIPS 180-2
//
// MAJ and CH are the functions Maj and Ch from the standard.
// CSIGMA0 and CSIGMA1 are the capital sigma functions.
// LSIGMA0 and LSIGMA1 are the lowercase sigma functions.
//
// The canonical definitions of the MAJ and CH functions are:
//#define MAJ( x, y, z )    (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
//#define CH( x, y, z )  (((x) & (y)) ^ ((~(x)) & (z)))
// We use optimized versions defined below
//

#define MAJ( x, y, z )  ((((z) | (y)) & (x) ) | ((z) & (y)))
#define CH( x, y, z )  ((((z) ^ (y)) & (x)) ^ (z))

#define CSIGMA0(x) (RORX_U64(x, 28) ^ RORX_U64(x, 34) ^ RORX_U64(x, 39))
#define CSIGMA1(x) (RORX_U64(x, 14) ^ RORX_U64(x, 18) ^ RORX_U64(x, 41))

#define LSIGMA0( x )    (ROR64( (x) ^ ROR64((x),   7),  1) ^ ((x)>> 7))
#define LSIGMA1( x )    (ROR64( (x) ^ ROR64((x),  42), 19) ^ ((x)>> 6))

#define YMMADD( _a, _b ) _mm256_add_epi64((_a), (_b))
#define YMMROR( _a, _n ) _mm256_xor_si256( _mm256_slli_epi64( (_a), 64-(_n)), _mm256_srli_epi64( (_a), (_n)) )
#define YMMSHR( _a, _n ) _mm256_srli_epi64((_a), (_n))
#define YMMXOR( _a, _b ) _mm256_xor_si256((_a), (_b))

// Rotation by 8 bits is faster with byte shuffling 
#if 1
#define YMMROR8( _a ) _mm256_shuffle_epi8((_a), _mm256_load_si256((__m256i*)BYTE_ROTATE_64))
#define YMMLSIGMA0( x )    YMMXOR( YMMXOR( YMMROR((x),  1), YMMROR8((x))), YMMSHR((x), 7))
#else
#define YMMLSIGMA0( x )    YMMXOR( YMMXOR( YMMROR((x),  1), YMMROR((x),  8)), YMMSHR((x), 7))
#endif

#define YMMLSIGMA1( x )    YMMXOR( YMMXOR( YMMROR((x), 19), YMMROR((x), 61)), YMMSHR((x), 6))


//
// YMM implementation that processes 1 message block at a time
//

// Core round function
// Message words are loaded from Wx.ul[80].
#define CROUND_1BLOCK(a, b, c, d, e, f, g, h, r) {;\
    UINT64 T1 = CSIGMA1(e) + CH(e, f, g) + Wx.ul[r] + SymCryptSha512K[r]; \
    UINT64 T2 = CSIGMA0(a) + MAJ(a, b, c); \
    h += T1; \
    d += h;\
    h += T2;\
}

//
// Message expansion for 4 rounds
//
// Each element of Wx.ymm[] array contains 4 message words, with the
// first 4 elements containing the original 16 message words. These are
// then expanded 16 times to generate the next 16 * 4 message words,
// comprising the 80 expanded message words in the union arrays Wx.ymm[20] or Wx.ul[80].
//
// rnd: starts from 16 (updating the 4th element of the Wx.ymm[] array) and
//      goes up to 76 in multiples of 4.
//
#define SHA512_MSG_EXPAND_1BLOCK_4ROUNDS(rnd) { \
    Wx.ymm[(rnd) / 4] = _mm256_add_epi64(_mm256_add_epi64( \
    YMMLSIGMA0(_mm256_loadu_si256((__m256i*)& Wx.ul[(rnd) - 15])), \
    _mm256_load_si256((__m256i*)& Wx.ul[(rnd) - 16])), \
    _mm256_loadu_si256((__m256i*)& Wx.ul[(rnd) - 7])); \
    Wx.ul[(rnd) + 0] += LSIGMA1(Wx.ul[(rnd) - 2]); \
    Wx.ul[(rnd) + 1] += LSIGMA1(Wx.ul[(rnd) - 1]); \
    Wx.ul[(rnd) + 2] += LSIGMA1(Wx.ul[(rnd) + 0]); \
    Wx.ul[(rnd) + 3] += LSIGMA1(Wx.ul[(rnd) + 1]); \
}


VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_1block(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE* pChain,
    _In_reads_(cbData)      PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T*                         pcbRemaining)
{
    SYMCRYPT_ALIGN_AT(32) union { UINT64 ul[80]; __m256i ymm[20]; } Wx;
    UINT64 A, B, C, D, E, F, G, H;
  
    _mm256_zeroupper();

    while (cbData >= SYMCRYPT_SHA512_INPUT_BLOCK_SIZE)
    {
        A = pChain->H[0];
        B = pChain->H[1];
        C = pChain->H[2];
        D = pChain->H[3];
        E = pChain->H[4];
        F = pChain->H[5];
        G = pChain->H[6];
        H = pChain->H[7];

#if 0
        Wx.ul[ 0] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  0]);
        Wx.ul[ 1] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  1]);
        Wx.ul[ 2] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  2]);
        Wx.ul[ 3] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  3]);
        Wx.ul[ 4] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  4]);
        Wx.ul[ 5] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  5]);
        Wx.ul[ 6] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  6]);
        Wx.ul[ 7] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  7]);
        Wx.ul[ 8] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  8]);
        Wx.ul[ 9] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 *  9]);
        Wx.ul[10] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 * 10]);
        Wx.ul[11] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 * 11]);
        Wx.ul[12] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 * 12]);
        Wx.ul[13] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 * 13]);
        Wx.ul[14] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 * 14]);
        Wx.ul[15] = SYMCRYPT_LOAD_MSBFIRST64(&pbData[8 * 15]);
#else
        Wx.ymm[0] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (0) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
        Wx.ymm[1] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (1) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
        Wx.ymm[2] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (2) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
        Wx.ymm[3] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (3) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
#endif

        for (int iterCount=0; iterCount<(64/16); iterCount++)
        {
            const int roundBase = iterCount*16;

            CROUND_1BLOCK(A, B, C, D, E, F, G, H, roundBase +  0);
            CROUND_1BLOCK(H, A, B, C, D, E, F, G, roundBase +  1);
            CROUND_1BLOCK(G, H, A, B, C, D, E, F, roundBase +  2);
            CROUND_1BLOCK(F, G, H, A, B, C, D, E, roundBase +  3);
            SHA512_MSG_EXPAND_1BLOCK_4ROUNDS(roundBase + 16);

            CROUND_1BLOCK(E, F, G, H, A, B, C, D, roundBase +  4);
            CROUND_1BLOCK(D, E, F, G, H, A, B, C, roundBase +  5);
            CROUND_1BLOCK(C, D, E, F, G, H, A, B, roundBase +  6);
            CROUND_1BLOCK(B, C, D, E, F, G, H, A, roundBase +  7);
            SHA512_MSG_EXPAND_1BLOCK_4ROUNDS(roundBase + 20);

            CROUND_1BLOCK(A, B, C, D, E, F, G, H, roundBase +  8);
            CROUND_1BLOCK(H, A, B, C, D, E, F, G, roundBase +  9);
            CROUND_1BLOCK(G, H, A, B, C, D, E, F, roundBase + 10);
            CROUND_1BLOCK(F, G, H, A, B, C, D, E, roundBase + 11);
            SHA512_MSG_EXPAND_1BLOCK_4ROUNDS(roundBase + 24);

            CROUND_1BLOCK(E, F, G, H, A, B, C, D, roundBase + 12);
            CROUND_1BLOCK(D, E, F, G, H, A, B, C, roundBase + 13);
            CROUND_1BLOCK(C, D, E, F, G, H, A, B, roundBase + 14);
            CROUND_1BLOCK(B, C, D, E, F, G, H, A, roundBase + 15);
            SHA512_MSG_EXPAND_1BLOCK_4ROUNDS(roundBase + 28);
        }

        CROUND_1BLOCK(A, B, C, D, E, F, G, H, 64 +  0);
        CROUND_1BLOCK(H, A, B, C, D, E, F, G, 64 +  1);
        CROUND_1BLOCK(G, H, A, B, C, D, E, F, 64 +  2);
        CROUND_1BLOCK(F, G, H, A, B, C, D, E, 64 +  3);
        CROUND_1BLOCK(E, F, G, H, A, B, C, D, 64 +  4);
        CROUND_1BLOCK(D, E, F, G, H, A, B, C, 64 +  5);
        CROUND_1BLOCK(C, D, E, F, G, H, A, B, 64 +  6);
        CROUND_1BLOCK(B, C, D, E, F, G, H, A, 64 +  7);
        CROUND_1BLOCK(A, B, C, D, E, F, G, H, 64 +  8);
        CROUND_1BLOCK(H, A, B, C, D, E, F, G, 64 +  9);
        CROUND_1BLOCK(G, H, A, B, C, D, E, F, 64 + 10);
        CROUND_1BLOCK(F, G, H, A, B, C, D, E, 64 + 11);
        CROUND_1BLOCK(E, F, G, H, A, B, C, D, 64 + 12);
        CROUND_1BLOCK(D, E, F, G, H, A, B, C, 64 + 13);
        CROUND_1BLOCK(C, D, E, F, G, H, A, B, 64 + 14);
        CROUND_1BLOCK(B, C, D, E, F, G, H, A, 64 + 15);

        pChain->H[0] = A + pChain->H[0];
        pChain->H[1] = B + pChain->H[1];
        pChain->H[2] = C + pChain->H[2];
        pChain->H[3] = D + pChain->H[3];
        pChain->H[4] = E + pChain->H[4];
        pChain->H[5] = F + pChain->H[5];
        pChain->H[6] = G + pChain->H[6];
        pChain->H[7] = H + pChain->H[7];

        pbData += SYMCRYPT_SHA512_INPUT_BLOCK_SIZE;
        cbData -= SYMCRYPT_SHA512_INPUT_BLOCK_SIZE;
    }

    *pcbRemaining = cbData;

    _mm256_zeroupper();
    
    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize(Wx.ymm, sizeof(Wx.ymm));
 }




 //
 // 2-way parallel message block processing
 //

 // Core round function
 // 
 // r : round number ( 0 <= r < 80)
 // bl: message block index ( bl = 0, 1)
 // 
 // The message words are generated by YMM code into the array Wx.ul[40][4].
 // Let W0, W1, ..., W15 be the message words from the first message block and 
 // Y0, Y1, ..., Y15 be the message words from the second message block. After message
 // expansion, Wx.ul[][] array will take the following form:
 //
 // Wx.ul[40][4] = { 
 //          W0,  W1,  Y0,  Y1,
 //          W2,  W3,  Y2,  Y3,
 //         ...
 //         W78, W79, Y78, Y79
 // };
 //
#define CROUND_2BLOCKS(a, b, c, d, e, f, g, h, r, bl ) { \
    UINT64 T1 = CSIGMA1(e) + CH(e, f, g) + Wx.ul[(r) / 2][2 * (bl) + ((r) & 1)] + SymCryptSha512K[r]; \
    UINT64 T2 = CSIGMA0(a) + MAJ(a, b, c); \
    h += T1; \
    d += h;\
    h += T2;\
}

// Message expansion of 2 message blocks for 2 rounds
#define SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(ind) { \
    __m256i t1 = _mm256_permute4x64_epi64(_mm256_blend_epi32(Wx.ymm[ind + 0], Wx.ymm[ind + 1], 0x33), 0xb1); \
    __m256i t2 = _mm256_permute4x64_epi64(_mm256_blend_epi32(Wx.ymm[ind + 4], Wx.ymm[ind + 5], 0x33), 0xb1); \
    __m256i  s = _mm256_add_epi64(Wx.ymm[ind], _mm256_add_epi64(YMMLSIGMA0(t1), _mm256_add_epi64(YMMLSIGMA1(Wx.ymm[ind + 7]), t2))); \
    _mm256_store_si256(&Wx.ymm[ind + 8], s); \
}

//
// 16 rounds of 2-block message expansion with 16 rounds of message processing of the first message block
// 
// This macro is called four times to generate 64 expanded message words and do 64 rounds of processing of 
// first message block. The indices substituted for SYMCRYPT_SHA512_MS_2B_ROUND_YMM() (resp. CROUND_512_VAR_MS_2B() ) 
// range from 0 to 31 (resp. 0 to 63).
#define SHA512_2BLOCKS_ROUND_STITCHED_16X(rb, ind) { \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 0); \
    CROUND_2BLOCKS(A, B, C, D, E, F, G, H, 2 * (rb + ind) + 0, 0); \
    CROUND_2BLOCKS(H, A, B, C, D, E, F, G, 2 * (rb + ind) + 1, 0); \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 1); \
    CROUND_2BLOCKS(G, H, A, B, C, D, E, F, 2 * (rb + ind) + 2, 0); \
    CROUND_2BLOCKS(F, G, H, A, B, C, D, E, 2 * (rb + ind) + 3, 0); \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 2); \
    CROUND_2BLOCKS(E, F, G, H, A, B, C, D, 2 * (rb + ind) + 4, 0); \
    CROUND_2BLOCKS(D, E, F, G, H, A, B, C, 2 * (rb + ind) + 5, 0); \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 3); \
    CROUND_2BLOCKS(C, D, E, F, G, H, A, B, 2 * (rb + ind) + 6, 0); \
    CROUND_2BLOCKS(B, C, D, E, F, G, H, A, 2 * (rb + ind) + 7, 0); \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 4); \
    CROUND_2BLOCKS(A, B, C, D, E, F, G, H, 2 * (rb + ind) + 8, 0); \
    CROUND_2BLOCKS(H, A, B, C, D, E, F, G, 2 * (rb + ind) + 9, 0); \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 5); \
    CROUND_2BLOCKS(G, H, A, B, C, D, E, F, 2 * (rb + ind) + 10, 0); \
    CROUND_2BLOCKS(F, G, H, A, B, C, D, E, 2 * (rb + ind) + 11, 0); \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 6); \
    CROUND_2BLOCKS(E, F, G, H, A, B, C, D, 2 * (rb + ind) + 12, 0); \
    CROUND_2BLOCKS(D, E, F, G, H, A, B, C, 2 * (rb + ind) + 13, 0); \
    SHA512_MSG_EXPAND_2BLOCKS_2ROUNDS(rb + ind + 7); \
    CROUND_2BLOCKS(C, D, E, F, G, H, A, B, 2 * (rb + ind) + 14, 0); \
    CROUND_2BLOCKS(B, C, D, E, F, G, H, A, 2 * (rb + ind) + 15, 0); \
}


VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_2blocks(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE* pChain,
    _In_reads_(cbData)      PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T*                         pcbRemaining)
{
    SYMCRYPT_ALIGN_AT(32) union { UINT64 ul[40][4]; __m256i ymm[40]; } Wx;
    __m256i w1[4], w2[4];
    UINT64 A, B, C, D, E, F, G, H;
    SIZE_T numBlocks;

    _mm256_zeroupper();

    while (cbData >= SYMCRYPT_SHA512_INPUT_BLOCK_SIZE)
    {
        // Load message words from first block
        // 
        // w1[0] =  W3  W2  W1  W0
        // w1[1] =  W7  W6  W5  W4
        // w1[2] = W11 W10  W9  W8
        // w1[3] = W15 W14 W13 W12
        //
        numBlocks = 1;
        w1[0] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (0) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
        w1[1] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (1) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
        w1[2] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (2) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
        w1[3] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[0 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (3) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));

        if (cbData >= (2 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE))
        {
            // Load message words from second block
            // 
            // w2[0] =  Y3  Y2  Y1  Y0
            // w2[1] =  Y7  Y6  Y5  Y4
            // w2[2] = Y11 Y10  Y9  Y8
            // w2[3] = Y15 Y14 Y13 Y12
            //
            numBlocks = 2;
            w2[0] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[1 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (0) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
            w2[1] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[1 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (1) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
            w2[2] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[1 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (2) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
            w2[3] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) & pbData[1 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + (3) * 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2));
        }

        // process first block and do the message expansion for two blocks at the same time
        {
            A = pChain->H[0];
            B = pChain->H[1];
            C = pChain->H[2];
            D = pChain->H[3];
            E = pChain->H[4];
            F = pChain->H[5];
            G = pChain->H[6];
            H = pChain->H[7];

            //
            // Combine message words from two blocks
            // 
            // Wx.ymm[0] =  Y1  Y0  W1  W0
            //   ...             ...
            // Wx.ymm[7] = Y15 Y14 W15 W14
            //
            Wx.ymm[0] = _mm256_permute2x128_si256(w1[0], w2[0], 0x20);
            Wx.ymm[1] = _mm256_permute2x128_si256(w1[0], w2[0], 0x31);
            Wx.ymm[2] = _mm256_permute2x128_si256(w1[1], w2[1], 0x20);
            Wx.ymm[3] = _mm256_permute2x128_si256(w1[1], w2[1], 0x31);
            Wx.ymm[4] = _mm256_permute2x128_si256(w1[2], w2[2], 0x20);
            Wx.ymm[5] = _mm256_permute2x128_si256(w1[2], w2[2], 0x31);
            Wx.ymm[6] = _mm256_permute2x128_si256(w1[3], w2[3], 0x20);
            Wx.ymm[7] = _mm256_permute2x128_si256(w1[3], w2[3], 0x31);

            // Do the message expansion of two message blocks together with the
            // processing of first 64 rounds of first message block
            SHA512_2BLOCKS_ROUND_STITCHED_16X(0,  0);
            SHA512_2BLOCKS_ROUND_STITCHED_16X(0,  8);
            SHA512_2BLOCKS_ROUND_STITCHED_16X(16, 0);
            SHA512_2BLOCKS_ROUND_STITCHED_16X(16, 8);

            //
            // Last 16 rounds of round processing
            //
            CROUND_2BLOCKS(A, B, C, D, E, F, G, H, 64 + 0, 0);
            CROUND_2BLOCKS(H, A, B, C, D, E, F, G, 64 + 1, 0);
            CROUND_2BLOCKS(G, H, A, B, C, D, E, F, 64 + 2, 0);
            CROUND_2BLOCKS(F, G, H, A, B, C, D, E, 64 + 3, 0);
            CROUND_2BLOCKS(E, F, G, H, A, B, C, D, 64 + 4, 0);
            CROUND_2BLOCKS(D, E, F, G, H, A, B, C, 64 + 5, 0);
            CROUND_2BLOCKS(C, D, E, F, G, H, A, B, 64 + 6, 0);
            CROUND_2BLOCKS(B, C, D, E, F, G, H, A, 64 + 7, 0);

            CROUND_2BLOCKS(A, B, C, D, E, F, G, H, 72 + 0, 0);
            CROUND_2BLOCKS(H, A, B, C, D, E, F, G, 72 + 1, 0);
            CROUND_2BLOCKS(G, H, A, B, C, D, E, F, 72 + 2, 0);
            CROUND_2BLOCKS(F, G, H, A, B, C, D, E, 72 + 3, 0);
            CROUND_2BLOCKS(E, F, G, H, A, B, C, D, 72 + 4, 0);
            CROUND_2BLOCKS(D, E, F, G, H, A, B, C, 72 + 5, 0);
            CROUND_2BLOCKS(C, D, E, F, G, H, A, B, 72 + 6, 0);
            CROUND_2BLOCKS(B, C, D, E, F, G, H, A, 72 + 7, 0);

            pChain->H[0] = A + pChain->H[0];
            pChain->H[1] = B + pChain->H[1];
            pChain->H[2] = C + pChain->H[2];
            pChain->H[3] = D + pChain->H[3];
            pChain->H[4] = E + pChain->H[4];
            pChain->H[5] = F + pChain->H[5];
            pChain->H[6] = G + pChain->H[6];
            pChain->H[7] = H + pChain->H[7];
        }

        // second block
        if(numBlocks > 1)
        {
            A = pChain->H[0];
            B = pChain->H[1];
            C = pChain->H[2];
            D = pChain->H[3];
            E = pChain->H[4];
            F = pChain->H[5];
            G = pChain->H[6];
            H = pChain->H[7];

            for (int iterCount=0; iterCount<(80/8); iterCount++)
            {
                const int roundBase = iterCount*8;
                CROUND_2BLOCKS(A, B, C, D, E, F, G, H, roundBase +  0, 1);
                CROUND_2BLOCKS(H, A, B, C, D, E, F, G, roundBase +  1, 1);
                CROUND_2BLOCKS(G, H, A, B, C, D, E, F, roundBase +  2, 1);
                CROUND_2BLOCKS(F, G, H, A, B, C, D, E, roundBase +  3, 1);
                CROUND_2BLOCKS(E, F, G, H, A, B, C, D, roundBase +  4, 1);
                CROUND_2BLOCKS(D, E, F, G, H, A, B, C, roundBase +  5, 1);
                CROUND_2BLOCKS(C, D, E, F, G, H, A, B, roundBase +  6, 1);
                CROUND_2BLOCKS(B, C, D, E, F, G, H, A, roundBase +  7, 1);
            }

            pChain->H[0] = A + pChain->H[0];
            pChain->H[1] = B + pChain->H[1];
            pChain->H[2] = C + pChain->H[2];
            pChain->H[3] = D + pChain->H[3];
            pChain->H[4] = E + pChain->H[4];
            pChain->H[5] = F + pChain->H[5];
            pChain->H[6] = G + pChain->H[6];
            pChain->H[7] = H + pChain->H[7];
        }

        pbData += (numBlocks * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE);
        cbData -= (numBlocks * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE);
    }

    *pcbRemaining = cbData;

    _mm256_zeroupper();

    //
    // Wipe the variables;
    //
    SymCryptWipeKnownSize(Wx.ymm, sizeof(Wx.ymm));
    SymCryptWipeKnownSize(w1, sizeof(w1));
    SymCryptWipeKnownSize(w2, sizeof(w2));
}



//
// 4-way parallel message block processing
//


// Initial loading of message words and endianness transformation.
// 
// _bl : Number of message blocks to load,  1 <= bl <= 4.
//
// When bl < 4, the high order lanes of the YMM registers corresponding to the missing blocks are unused.
//
#define SHA512_MSG_LOAD_4BLOCKS(bl)  { \
        for(int i = 0; i < bl; i++) \
        { \
            Wx.ymm[i +  0] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) &pbData[i * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE +  0]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2)); \
            Wx.ymm[i +  4] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) &pbData[i * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + 32]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2)); \
            Wx.ymm[i +  8] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) &pbData[i * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + 64]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2)); \
            Wx.ymm[i + 12] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) &pbData[i * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE + 96]), _mm256_load_si256((__m256i*)BYTE_REVERSE_64X2)); \
        } \
}

// Shuffles the initially loaded message words from multiple blocks
// so that each YMM register contains message words with the same index
// within a block (e.g. Wx.ymm[0] contains the first words of each block).
// 
// We have to use this macro four times to transform message blocks of 128-bytes.
// ind=0 processes the first quarter (32-bytes), ind=1 does the second quarter and so on.
//
#define SHA512_MSG_TRANSPOSE_QUARTER_4BLOCKS(ind)  { \
        __m256i t1, t2, t3, t4; \
        t1 = _mm256_unpacklo_epi64(Wx.ymm[4 * (ind) + 0], Wx.ymm[4 * (ind) + 1]); \
        t2 = _mm256_unpacklo_epi64(Wx.ymm[4 * (ind) + 2], Wx.ymm[4 * (ind) + 3]); \
        t3 = _mm256_unpackhi_epi64(Wx.ymm[4 * (ind) + 0], Wx.ymm[4 * (ind) + 1]); \
        t4 = _mm256_unpackhi_epi64(Wx.ymm[4 * (ind) + 2], Wx.ymm[4 * (ind) + 3]); \
        Wx.ymm[4 * (ind) + 0] = _mm256_permute2x128_si256(t1, t2, 0x20); \
        Wx.ymm[4 * (ind) + 1] = _mm256_permute2x128_si256(t3, t4, 0x20); \
        Wx.ymm[4 * (ind) + 2] = _mm256_permute2x128_si256(t1, t2, 0x31); \
        Wx.ymm[4 * (ind) + 3] = _mm256_permute2x128_si256(t3, t4, 0x31); \
}

// Transpose all message words. Each SYMCRYPT_SHA512_MSG_TRANSPOSE_QUARTER_YMM() does the
// transposition for four message words (i.e. 0 1 2 3, 4 5 6 7, 8 9 10 11, 12 13 14 15) 
#define SHA512_MSG_TRANSPOSE_4BLOCKS() { \
    SHA512_MSG_TRANSPOSE_QUARTER_4BLOCKS(0); \
    SHA512_MSG_TRANSPOSE_QUARTER_4BLOCKS(1); \
    SHA512_MSG_TRANSPOSE_QUARTER_4BLOCKS(2); \
    SHA512_MSG_TRANSPOSE_QUARTER_4BLOCKS(3); \
}

// One round message schedule, updates the rth message word, and adds the constants to message words for (r-16).
#define SHA512_MSG_EXPAND_4BLOCKS_1ROUND(r) { \
        Wx.ymm[r] = _mm256_add_epi64(_mm256_add_epi64(_mm256_add_epi64(Wx.ymm[r - 16], Wx.ymm[r - 7]), \
                                    YMMLSIGMA0(Wx.ymm[r - 15])), YMMLSIGMA1(Wx.ymm[r - 2])); \
        Wx.ymm[r - 16] = _mm256_add_epi64(Wx.ymm[r - 16], _mm256_set1_epi64x(SymCryptSha512K[r - 16])); \
}

// Four rounds of message schedule. Generates message words for rounds r, r+1, r+2, r+3.
#define SHA512_MSG_EXPAND_4BLOCKS_4ROUNDS(r) { \
        SHA512_MSG_EXPAND_4BLOCKS_1ROUND((r) + 0); SHA512_MSG_EXPAND_4BLOCKS_1ROUND((r) + 1); \
        SHA512_MSG_EXPAND_4BLOCKS_1ROUND((r) + 2); SHA512_MSG_EXPAND_4BLOCKS_1ROUND((r) + 3); \
}

// Sixteen rounds of message schedule. Generates message words for rounds r, ..., r+15.
#define SHA512_MSG_EXPAND_4BLOCKS_16ROUNDS(r) { \
        SHA512_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 0); SHA512_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 4); \
        SHA512_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 8); SHA512_MSG_EXPAND_4BLOCKS_4ROUNDS((r) + 12); \
}

//
// Core round for 4-way message expansion without constant addition
//
// r: round number (0 <= r < 80)
// 
// bl: message block index (0 <= bl < 4)
// 
// Message words for four blocks are store in Wx.ul[80][4] in interleaved form:
//   W0  X0  Y0  Z0
//   W1  X1  Y1  Z1
//      ...
//  W79 X79 Y79 Z79
//
#define CROUND_4BLOCKS(a, b, c, d, e, f, g, h, r, bl ) { \
    UINT64 T1 = CSIGMA1(e) + CH(e, f, g) + Wx.ul4[r][bl]; \
    UINT64 T2 = CSIGMA0(a) + MAJ(a, b, c); \
    h += T1; \
    d += h; \
    h += T2; \
}

// Core round for single block
#define CROUND(a, b, c, d, e, f, g, h, r, r16) { \
    Wx.ul[r16] = Wt; \
    UINT64 T1 = CSIGMA1(e) + CH(e, f, g) + Wt + SymCryptSha512K[r]; \
    UINT64 T2 = CSIGMA0(a) + MAJ(a, b, c); \
    h += T1; \
    d += h;\
    h += T2;\
}

// Initial round for single block
#define IROUND( a, b, c, d, e, f, g, h, r ) { \
    Wt = SYMCRYPT_LOAD_MSBFIRST64( &pbData[ 8*r ] );\
    CROUND( a, b, c, d, e, f, g, h, r, r);\
}

// Full round for single block
#define FROUND( a, b, c, d, e, f, g, h, r, r16 ) { \
    Wt = LSIGMA1( Wx.ul[(r16-2) & 15] ) +   Wx.ul[(r16-7) & 15] +  \
         LSIGMA0( Wx.ul[(r16-15) & 15]) +   Wx.ul[r16 & 15];       \
    CROUND( a, b, c, d, e, f, g, h, r, r16 ); \
}

// Constant addition and round processing for 8 rounds. Constants up to r=64 are added in message expansion. 
// This macro is called to twice to add the constants do the round processing for the last 16 rounds.
#define SHA512_4BLOCKS_FINAL_ROUNDS_8X(rnd) { \
    Wx.ymm[rnd + 0] = _mm256_add_epi64(Wx.ymm[rnd + 0], _mm256_set1_epi64x(SymCryptSha512K[rnd + 0])); \
    Wx.ymm[rnd + 1] = _mm256_add_epi64(Wx.ymm[rnd + 1], _mm256_set1_epi64x(SymCryptSha512K[rnd + 1])); \
    Wx.ymm[rnd + 2] = _mm256_add_epi64(Wx.ymm[rnd + 2], _mm256_set1_epi64x(SymCryptSha512K[rnd + 2])); \
    Wx.ymm[rnd + 3] = _mm256_add_epi64(Wx.ymm[rnd + 3], _mm256_set1_epi64x(SymCryptSha512K[rnd + 3])); \
    CROUND_4BLOCKS(A, B, C, D, E, F, G, H, rnd + 0, 0); \
    CROUND_4BLOCKS(H, A, B, C, D, E, F, G, rnd + 1, 0); \
    CROUND_4BLOCKS(G, H, A, B, C, D, E, F, rnd + 2, 0); \
    CROUND_4BLOCKS(F, G, H, A, B, C, D, E, rnd + 3, 0); \
    Wx.ymm[rnd + 4] = _mm256_add_epi64(Wx.ymm[rnd + 4], _mm256_set1_epi64x(SymCryptSha512K[rnd + 4])); \
    Wx.ymm[rnd + 5] = _mm256_add_epi64(Wx.ymm[rnd + 5], _mm256_set1_epi64x(SymCryptSha512K[rnd + 5])); \
    Wx.ymm[rnd + 6] = _mm256_add_epi64(Wx.ymm[rnd + 6], _mm256_set1_epi64x(SymCryptSha512K[rnd + 6])); \
    Wx.ymm[rnd + 7] = _mm256_add_epi64(Wx.ymm[rnd + 7], _mm256_set1_epi64x(SymCryptSha512K[rnd + 7])); \
    CROUND_4BLOCKS(E, F, G, H, A, B, C, D, rnd + 4, 0); \
    CROUND_4BLOCKS(D, E, F, G, H, A, B, C, rnd + 5, 0); \
    CROUND_4BLOCKS(C, D, E, F, G, H, A, B, rnd + 6, 0); \
    CROUND_4BLOCKS(B, C, D, E, F, G, H, A, rnd + 7, 0); \
}


VOID
SYMCRYPT_CALL
SymCryptSha512AppendBlocks_ymm_4blocks(
    _Inout_                 SYMCRYPT_SHA512_CHAINING_STATE* pChain,
    _In_reads_(cbData)      PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T*                         pcbRemaining)
{
    SYMCRYPT_ALIGN_AT(32) union { UINT64 ul[16]; UINT64 ul4[80][4]; __m256i ymm[80]; } Wx;
    UINT64 Wt;
    UINT64 A, B, C, D, E, F, G, H;
    UINT32 uWipeSize = (cbData >= (3 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE)) ? (80 * 4 * sizeof(UINT64)) : (16 * sizeof(UINT64));


    _mm256_zeroupper();

    while (cbData >= (3 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE))
    {         
        SIZE_T numBlocks = (cbData >= 4 * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE) ? 4 : (cbData / SYMCRYPT_SHA512_INPUT_BLOCK_SIZE);

        SHA512_MSG_LOAD_4BLOCKS(numBlocks);
        SHA512_MSG_TRANSPOSE_4BLOCKS();

        //
        // Process the first block together with message expansion
        //
        A = pChain->H[0];
        B = pChain->H[1];
        C = pChain->H[2];
        D = pChain->H[3];
        E = pChain->H[4];
        F = pChain->H[5];
        G = pChain->H[6];
        H = pChain->H[7];

        for (int iterCount=0; iterCount<(64/8); iterCount++)
        {
            const int roundBase = iterCount*8;

            SHA512_MSG_EXPAND_4BLOCKS_4ROUNDS(roundBase + 16);
            CROUND_4BLOCKS(A, B, C, D, E, F, G, H, roundBase + 0, 0);
            CROUND_4BLOCKS(H, A, B, C, D, E, F, G, roundBase + 1, 0);
            CROUND_4BLOCKS(G, H, A, B, C, D, E, F, roundBase + 2, 0);
            CROUND_4BLOCKS(F, G, H, A, B, C, D, E, roundBase + 3, 0);

            SHA512_MSG_EXPAND_4BLOCKS_4ROUNDS(roundBase + 20);
            CROUND_4BLOCKS(E, F, G, H, A, B, C, D, roundBase + 4, 0);
            CROUND_4BLOCKS(D, E, F, G, H, A, B, C, roundBase + 5, 0);
            CROUND_4BLOCKS(C, D, E, F, G, H, A, B, roundBase + 6, 0);
            CROUND_4BLOCKS(B, C, D, E, F, G, H, A, roundBase + 7, 0);
        }

        // Last 16 rounds; add round constants and process. Message expansion is completed above.
        SHA512_4BLOCKS_FINAL_ROUNDS_8X(64);
        SHA512_4BLOCKS_FINAL_ROUNDS_8X(72);

        pChain->H[0] = A + pChain->H[0];
        pChain->H[1] = B + pChain->H[1];
        pChain->H[2] = C + pChain->H[2];
        pChain->H[3] = D + pChain->H[3];
        pChain->H[4] = E + pChain->H[4];
        pChain->H[5] = F + pChain->H[5];
        pChain->H[6] = G + pChain->H[6];
        pChain->H[7] = H + pChain->H[7];
            
        // Process the remaining message blocks
        for (int bl = 1; bl < numBlocks; bl++)
        {
            A = pChain->H[0];
            B = pChain->H[1];
            C = pChain->H[2];
            D = pChain->H[3];
            E = pChain->H[4];
            F = pChain->H[5];
            G = pChain->H[6];
            H = pChain->H[7];

            for (int iterCount=0; iterCount<(80/8); iterCount++)
            {
                const int roundBase = iterCount*8;

                CROUND_4BLOCKS(A, B, C, D, E, F, G, H, roundBase +  0, bl);
                CROUND_4BLOCKS(H, A, B, C, D, E, F, G, roundBase +  1, bl);
                CROUND_4BLOCKS(G, H, A, B, C, D, E, F, roundBase +  2, bl);
                CROUND_4BLOCKS(F, G, H, A, B, C, D, E, roundBase +  3, bl);
                CROUND_4BLOCKS(E, F, G, H, A, B, C, D, roundBase +  4, bl);
                CROUND_4BLOCKS(D, E, F, G, H, A, B, C, roundBase +  5, bl);
                CROUND_4BLOCKS(C, D, E, F, G, H, A, B, roundBase +  6, bl);
                CROUND_4BLOCKS(B, C, D, E, F, G, H, A, roundBase +  7, bl);
                //CROUND_4BLOCKS(A, B, C, D, E, F, G, H, roundBase +  8, bl);
                //CROUND_4BLOCKS(H, A, B, C, D, E, F, G, roundBase +  9, bl);
                //CROUND_4BLOCKS(G, H, A, B, C, D, E, F, roundBase + 10, bl);
                //CROUND_4BLOCKS(F, G, H, A, B, C, D, E, roundBase + 11, bl);
                //CROUND_4BLOCKS(E, F, G, H, A, B, C, D, roundBase + 12, bl);
                //CROUND_4BLOCKS(D, E, F, G, H, A, B, C, roundBase + 13, bl);
                //CROUND_4BLOCKS(C, D, E, F, G, H, A, B, roundBase + 14, bl);
                //CROUND_4BLOCKS(B, C, D, E, F, G, H, A, roundBase + 15, bl);
            }

            pChain->H[0] = A + pChain->H[0];
            pChain->H[1] = B + pChain->H[1];
            pChain->H[2] = C + pChain->H[2];
            pChain->H[3] = D + pChain->H[3];
            pChain->H[4] = E + pChain->H[4];
            pChain->H[5] = F + pChain->H[5];
            pChain->H[6] = G + pChain->H[6];
            pChain->H[7] = H + pChain->H[7];
        }

        pbData += (numBlocks * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE);
        cbData -= (numBlocks * SYMCRYPT_SHA512_INPUT_BLOCK_SIZE);
    }

    _mm256_zeroupper();


    // The vectorized version above consumes multiple blocks at a time.
    // The remaining blocks if any are processed here.
    while (cbData >= SYMCRYPT_SHA512_INPUT_BLOCK_SIZE)
    {
        A = pChain->H[0];
        B = pChain->H[1];
        C = pChain->H[2];
        D = pChain->H[3];
        E = pChain->H[4];
        F = pChain->H[5];
        G = pChain->H[6];
        H = pChain->H[7];

        //
        // initial rounds 1 to 16
        //

        IROUND(A, B, C, D, E, F, G, H, 0);
        IROUND(H, A, B, C, D, E, F, G, 1);
        IROUND(G, H, A, B, C, D, E, F, 2);
        IROUND(F, G, H, A, B, C, D, E, 3);
        IROUND(E, F, G, H, A, B, C, D, 4);
        IROUND(D, E, F, G, H, A, B, C, 5);
        IROUND(C, D, E, F, G, H, A, B, 6);
        IROUND(B, C, D, E, F, G, H, A, 7);
        IROUND(A, B, C, D, E, F, G, H, 8);
        IROUND(H, A, B, C, D, E, F, G, 9);
        IROUND(G, H, A, B, C, D, E, F, 10);
        IROUND(F, G, H, A, B, C, D, E, 11);
        IROUND(E, F, G, H, A, B, C, D, 12);
        IROUND(D, E, F, G, H, A, B, C, 13);
        IROUND(C, D, E, F, G, H, A, B, 14);
        IROUND(B, C, D, E, F, G, H, A, 15);

        for (int iterCount=1; iterCount<(80/16); iterCount++)
        {
            const int roundBase = iterCount*16;

            FROUND(A, B, C, D, E, F, G, H, roundBase + 0, 0);
            FROUND(H, A, B, C, D, E, F, G, roundBase + 1, 1);
            FROUND(G, H, A, B, C, D, E, F, roundBase + 2, 2);
            FROUND(F, G, H, A, B, C, D, E, roundBase + 3, 3);
            FROUND(E, F, G, H, A, B, C, D, roundBase + 4, 4);
            FROUND(D, E, F, G, H, A, B, C, roundBase + 5, 5);
            FROUND(C, D, E, F, G, H, A, B, roundBase + 6, 6);
            FROUND(B, C, D, E, F, G, H, A, roundBase + 7, 7);
            FROUND(A, B, C, D, E, F, G, H, roundBase + 8, 8);
            FROUND(H, A, B, C, D, E, F, G, roundBase + 9, 9);
            FROUND(G, H, A, B, C, D, E, F, roundBase + 10, 10);
            FROUND(F, G, H, A, B, C, D, E, roundBase + 11, 11);
            FROUND(E, F, G, H, A, B, C, D, roundBase + 12, 12);
            FROUND(D, E, F, G, H, A, B, C, roundBase + 13, 13);
            FROUND(C, D, E, F, G, H, A, B, roundBase + 14, 14);
            FROUND(B, C, D, E, F, G, H, A, roundBase + 15, 15);
        }

        pChain->H[0] = A + pChain->H[0];
        pChain->H[1] = B + pChain->H[1];
        pChain->H[2] = C + pChain->H[2];
        pChain->H[3] = D + pChain->H[3];
        pChain->H[4] = E + pChain->H[4];
        pChain->H[5] = F + pChain->H[5];
        pChain->H[6] = G + pChain->H[6];
        pChain->H[7] = H + pChain->H[7];

        pbData += SYMCRYPT_SHA512_INPUT_BLOCK_SIZE;
        cbData -= SYMCRYPT_SHA512_INPUT_BLOCK_SIZE;
    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipe(&Wx, uWipeSize);
}

#endif // SYMCRYPT_CPU_AMD64
