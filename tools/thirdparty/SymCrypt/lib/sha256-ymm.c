#include "precomp.h"


#if  SYMCRYPT_CPU_AMD64


extern SYMCRYPT_ALIGN_AT(256) const  UINT32 SymCryptSha256K[64];

// Endianness transformation for 8 32-bit values in a YMM register
const SYMCRYPT_ALIGN_AT(32) UINT32 BYTE_REVERSE_32X2[8] = {
    0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f,
    0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f,
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

#define CSIGMA0(x) (RORX_U32(x, 2) ^ RORX_U32(x, 13) ^ RORX_U32(x, 22))
#define CSIGMA1(x) (RORX_U32(x, 6) ^ RORX_U32(x, 11) ^ RORX_U32(x, 25))

#define LSIGMA0( x )    (ROR32((x),  7) ^ ROR32((x), 18) ^ ((x)>> 3))
#define LSIGMA1( x )    (ROR32((x), 17) ^ ROR32((x), 19) ^ ((x)>>10))

#define LSIGMA0YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi32(x,25)  , _mm256_srli_epi32(x,  7) ),\
        _mm256_slli_epi32(x,14) ), _mm256_srli_epi32(x, 18) ),\
        _mm256_srli_epi32(x, 3) )

#define LSIGMA1YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi32(x,15)  , _mm256_srli_epi32(x, 17) ),\
        _mm256_slli_epi32(x,13) ), _mm256_srli_epi32(x, 19) ),\
        _mm256_srli_epi32(x,10) )



// Initial loading of message words and endianness transformation.
// bl : The number of blocks to load,  1 <= bl <= 8.
//
// When bl < 8, the high order lanes of the YMM registers corresponding to the missing blocks are unused.
//
#define SHA256_MSG_LOAD_8BLOCKS(_bl)  { \
    for (int i = 0; i < (_bl); i++) \
    { \
        Wx.ymm[i + 0] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) &pbData[i * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE +  0]), _mm256_load_si256((const __m256i*)BYTE_REVERSE_32X2)); \
        Wx.ymm[i + 8] = _mm256_shuffle_epi8(_mm256_loadu_si256((__m256i*) &pbData[i * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE + 32]), _mm256_load_si256((const __m256i*)BYTE_REVERSE_32X2)); \
    }\
}

// Shuffles the initially loaded message words from multiple blocks
// so that each YMM register contains message words with the same index
// within a block (e.g. Wx.ymm[0] contains the first words of each block).
// 
// We have to use this macro twice to transform the message blocks of 64-bytes.
// ind=0 processes the first halves (32-bytes) of message blocks and ind=1 does the second halves.
//
#define SHA256_MSG_TRANSPOSE_HALF_8BLOCKS(ind)  { \
        __m256i s1, s2, s3, s4, s5, s6, s7, s8; \
        __m256i u1, u2, u3, u4, u5, u6, u7, u8; \
        s1 = _mm256_unpacklo_epi32(Wx.ymm[8 * (ind) + 0], Wx.ymm[8 * (ind) + 1]); \
        s2 = _mm256_unpacklo_epi32(Wx.ymm[8 * (ind) + 2], Wx.ymm[8 * (ind) + 3]); \
        s3 = _mm256_unpacklo_epi32(Wx.ymm[8 * (ind) + 4], Wx.ymm[8 * (ind) + 5]); \
        s4 = _mm256_unpacklo_epi32(Wx.ymm[8 * (ind) + 6], Wx.ymm[8 * (ind) + 7]); \
        s5 = _mm256_unpackhi_epi32(Wx.ymm[8 * (ind) + 0], Wx.ymm[8 * (ind) + 1]); \
        s6 = _mm256_unpackhi_epi32(Wx.ymm[8 * (ind) + 2], Wx.ymm[8 * (ind) + 3]); \
        s7 = _mm256_unpackhi_epi32(Wx.ymm[8 * (ind) + 4], Wx.ymm[8 * (ind) + 5]); \
        s8 = _mm256_unpackhi_epi32(Wx.ymm[8 * (ind) + 6], Wx.ymm[8 * (ind) + 7]); \
        u1 = _mm256_unpacklo_epi64(s1, s2); \
        u2 = _mm256_unpacklo_epi64(s3, s4); \
        u3 = _mm256_unpacklo_epi64(s5, s6); \
        u4 = _mm256_unpacklo_epi64(s7, s8); \
        u5 = _mm256_unpackhi_epi64(s1, s2); \
        u6 = _mm256_unpackhi_epi64(s3, s4); \
        u7 = _mm256_unpackhi_epi64(s5, s6); \
        u8 = _mm256_unpackhi_epi64(s7, s8); \
        Wx.ymm[8 * (ind) + 0] = _mm256_permute2x128_si256(u1, u2, 0x20); \
        Wx.ymm[8 * (ind) + 1] = _mm256_permute2x128_si256(u5, u6, 0x20); \
        Wx.ymm[8 * (ind) + 2] = _mm256_permute2x128_si256(u3, u4, 0x20); \
        Wx.ymm[8 * (ind) + 3] = _mm256_permute2x128_si256(u7, u8, 0x20); \
        Wx.ymm[8 * (ind) + 4] = _mm256_permute2x128_si256(u1, u2, 0x31); \
        Wx.ymm[8 * (ind) + 5] = _mm256_permute2x128_si256(u5, u6, 0x31); \
        Wx.ymm[8 * (ind) + 6] = _mm256_permute2x128_si256(u3, u4, 0x31); \
        Wx.ymm[8 * (ind) + 7] = _mm256_permute2x128_si256(u7, u8, 0x31); \
}

#define SHA256_MSG_TRANSPOSE_8BLOCKS() { \
    SHA256_MSG_TRANSPOSE_HALF_8BLOCKS(0); \
    SHA256_MSG_TRANSPOSE_HALF_8BLOCKS(1); \
}

//
// One round of message expansion, generates message word at index r ( 16 <= r < 64 ).
//
// Additionally adds the constant to the (r-16)^th message word. We cannot add the constants to
// the message words with indices greater than (r-16) since they will be used in the message expansion.
// Constants for the last 16 words are added after message expansion is completed.
//
#define SHA256_MSG_EXPAND_8BLOCKS_1ROUND(r) { \
        Wx.ymm[r] = _mm256_add_epi32(_mm256_add_epi32(_mm256_add_epi32(Wx.ymm[r - 16], Wx.ymm[r - 7]), LSIGMA0YMM(Wx.ymm[r - 15])), LSIGMA1YMM(Wx.ymm[r - 2])); \
        Wx.ymm[r - 16] = _mm256_add_epi32(Wx.ymm[r - 16], _mm256_set1_epi32(SymCryptSha256K[r - 16])); \
}

// Four rounds of message schedule. Generates message words for rounds r, r+1, r+2, r+3.
#define SHA256_MSG_EXPAND_8BLOCKS_4ROUNDS(r) { \
        SHA256_MSG_EXPAND_8BLOCKS_1ROUND((r) + 0); SHA256_MSG_EXPAND_8BLOCKS_1ROUND((r) + 1); SHA256_MSG_EXPAND_8BLOCKS_1ROUND((r) + 2); SHA256_MSG_EXPAND_8BLOCKS_1ROUND((r) + 3); \
}

// Sixteen rounds of message schedule. Generates message words for rounds r, ..., r+15.
#define SHA256_MSG_EXPAND_8BLOCKS_16ROUNDS(r) { \
        SHA256_MSG_EXPAND_8BLOCKS_4ROUNDS((r) + 0); SHA256_MSG_EXPAND_8BLOCKS_4ROUNDS((r) + 4); SHA256_MSG_EXPAND_8BLOCKS_4ROUNDS((r) + 8); SHA256_MSG_EXPAND_8BLOCKS_4ROUNDS((r) + 12); \
}


// Core round function without the constant addition. Uses rorx versions of CSIGMA functions.
//
// r16 : round number mod 16.
// rb  : base round number so that (rb + r16) gives the actual round number. rb = 0, 16, 32, 48.
// bl  : message block index, bl = 0..7.
#define CROUND_8BLOCKS(r16, rb, bl) { \
    UINT32 T2 = CSIGMA0(ah[(r16+7)&7]) + MAJ(ah[(r16+7)&7], ah[(r16+6)&7], ah[(r16+5)&7]); \
    UINT32 T1 = CSIGMA1(ah[(r16+3)&7]) + CH (ah[(r16+3)&7], ah[(r16+2)&7], ah[(r16+1)&7]) + Wx.ul8[(rb) + (r16)][bl];\
    ah[(r16+4)&7] += T1 + ah[ r16   &7]; \
    ah[ r16   &7] += T1 + T2; \
}

//
// Core round function for single message block processing
// r16 : round number mod 16
// r   : round number, r = 0..79
//
#define CROUND( r16, r ) { \
    ah[ r16   &7] += CSIGMA1(ah[(r16+3)&7]) + CH(ah[(r16+3)&7], ah[(r16+2)&7], ah[(r16+1)&7]) + SymCryptSha256K[r] + Wt;\
    ah[(r16+4)&7] += ah[r16 &7];\
    ah[ r16   &7] += CSIGMA0(ah[(r16+7)&7]) + MAJ(ah[(r16+7)&7], ah[(r16+6)&7], ah[(r16+5)&7]);\
}



//
// Initial round that reads the message.
// r is the round number 0..15
//
#define IROUND( r ) { \
    Wt = SYMCRYPT_LOAD_MSBFIRST32( &pbData[ 4*r ] );\
    Wx.ul[r] = Wt; \
    CROUND(r,r);\
}

//
// Subsequent rounds.
// r16 is the round number mod 16. rb is the round number minus r16.
//
#define FROUND(r16, rb) { \
    Wt = LSIGMA1( Wx.ul[(r16-2) & 15] ) +   Wx.ul[(r16-7) & 15] +  \
         LSIGMA0( Wx.ul[(r16-15) & 15]) +   Wx.ul[r16 & 15];       \
    Wx.ul[r16] = Wt; \
    CROUND( r16, r16+rb ); \
}

// Constant addition and round processing for rounds = 48..63, must be called twice.
// This macro is not used at the moment but kept here for completeness. The implementation using
// this macro turns out to be slower compared to the existing one.
#define SHA256_8BLOCKS_FINAL_ROUNDS_8X(rnd) { \
    Wx.ymm[rnd + 0] = _mm256_add_epi32(Wx.ymm[rnd + 0], _mm256_set1_epi32(SymCryptSha256K[rnd + 0])); \
    Wx.ymm[rnd + 1] = _mm256_add_epi32(Wx.ymm[rnd + 1], _mm256_set1_epi32(SymCryptSha256K[rnd + 1])); \
    Wx.ymm[rnd + 2] = _mm256_add_epi32(Wx.ymm[rnd + 2], _mm256_set1_epi32(SymCryptSha256K[rnd + 2])); \
    Wx.ymm[rnd + 3] = _mm256_add_epi32(Wx.ymm[rnd + 3], _mm256_set1_epi32(SymCryptSha256K[rnd + 3])); \
    CROUND_8BLOCKS(0, rnd, 0); \
    CROUND_8BLOCKS(1, rnd, 0); \
    CROUND_8BLOCKS(2, rnd, 0); \
    CROUND_8BLOCKS(3, rnd, 0); \
    Wx.ymm[rnd + 4] = _mm256_add_epi32(Wx.ymm[rnd + 4], _mm256_set1_epi32(SymCryptSha256K[rnd + 4])); \
    Wx.ymm[rnd + 5] = _mm256_add_epi32(Wx.ymm[rnd + 5], _mm256_set1_epi32(SymCryptSha256K[rnd + 5])); \
    Wx.ymm[rnd + 6] = _mm256_add_epi32(Wx.ymm[rnd + 6], _mm256_set1_epi32(SymCryptSha256K[rnd + 6])); \
    Wx.ymm[rnd + 7] = _mm256_add_epi32(Wx.ymm[rnd + 7], _mm256_set1_epi32(SymCryptSha256K[rnd + 7])); \
    CROUND_8BLOCKS(4, rnd, 0); \
    CROUND_8BLOCKS(5, rnd, 0); \
    CROUND_8BLOCKS(6, rnd, 0); \
    CROUND_8BLOCKS(7, rnd, 0); \
}

VOID
SYMCRYPT_CALL
SymCryptSha256AppendBlocks_ymm_8blocks(
    _Inout_                 SYMCRYPT_SHA256_CHAINING_STATE* pChain,
    _In_reads_(cbData)      PCBYTE                          pbData,
                            SIZE_T                          cbData,
    _Out_                   SIZE_T*                         pcbRemaining)
{

    SYMCRYPT_ALIGN_AT(32) union { UINT32 ul[16]; UINT32 ul8[64][8]; __m256i ymm[64]; } Wx;
    SYMCRYPT_ALIGN UINT32 ah[8];
    UINT32 Wt;
    UINT32 uWipeSize = (cbData >= (5 * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE)) ? (64 * 8 * sizeof(UINT32)) : (16 * sizeof(UINT32));


    _mm256_zeroupper();

    while (cbData >= (5 * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE))
    {
        // If we have 8 or more blocks then process 8, else process whatever is left.
        SIZE_T numBlocks = (cbData >= 8 * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE) ? 8 : (cbData / SYMCRYPT_SHA256_INPUT_BLOCK_SIZE);

        SHA256_MSG_LOAD_8BLOCKS(numBlocks);
        SHA256_MSG_TRANSPOSE_8BLOCKS();

        // Process the first block together with message expansion.
        // For the last 16 rounds we don't expand the message, instead just
        // add the round constants.
        {
            ah[7] = pChain->H[0];
            ah[6] = pChain->H[1];
            ah[5] = pChain->H[2];
            ah[4] = pChain->H[3];
            ah[3] = pChain->H[4];
            ah[2] = pChain->H[5];
            ah[1] = pChain->H[6];
            ah[0] = pChain->H[7];

            for (int r = 0; r < 64; r += 8)
            {
                if (r < 48)
                {
                    SHA256_MSG_EXPAND_8BLOCKS_4ROUNDS(r + 16);
                }
                else
                {
                    Wx.ymm[r + 0] = _mm256_add_epi32(Wx.ymm[r + 0], _mm256_set1_epi32(SymCryptSha256K[r + 0]));
                    Wx.ymm[r + 1] = _mm256_add_epi32(Wx.ymm[r + 1], _mm256_set1_epi32(SymCryptSha256K[r + 1]));
                    Wx.ymm[r + 2] = _mm256_add_epi32(Wx.ymm[r + 2], _mm256_set1_epi32(SymCryptSha256K[r + 2]));
                    Wx.ymm[r + 3] = _mm256_add_epi32(Wx.ymm[r + 3], _mm256_set1_epi32(SymCryptSha256K[r + 3]));
                }

                CROUND_8BLOCKS(0, r, 0);
                CROUND_8BLOCKS(1, r, 0);
                CROUND_8BLOCKS(2, r, 0);
                CROUND_8BLOCKS(3, r, 0);

                if (r < 48)
                {
                    SHA256_MSG_EXPAND_8BLOCKS_4ROUNDS(r + 20);
                }
                else
                {
                    Wx.ymm[r + 4] = _mm256_add_epi32(Wx.ymm[r + 4], _mm256_set1_epi32(SymCryptSha256K[r + 4]));
                    Wx.ymm[r + 5] = _mm256_add_epi32(Wx.ymm[r + 5], _mm256_set1_epi32(SymCryptSha256K[r + 5]));
                    Wx.ymm[r + 6] = _mm256_add_epi32(Wx.ymm[r + 6], _mm256_set1_epi32(SymCryptSha256K[r + 6]));
                    Wx.ymm[r + 7] = _mm256_add_epi32(Wx.ymm[r + 7], _mm256_set1_epi32(SymCryptSha256K[r + 7]));
                }

                CROUND_8BLOCKS(4, r, 0);
                CROUND_8BLOCKS(5, r, 0);
                CROUND_8BLOCKS(6, r, 0);
                CROUND_8BLOCKS(7, r, 0);
            }

            // Alternative version where the loop above goes up to round=48 and
            // the remaining 16 rounds are processed here. Despite the conditional logic,
            // the above version is faster compared to the commented out one.
            //SHA256_MS_8BLOCKS_FINAL_ROUNDS_8X(48);
            //SHA256_MS_8BLOCKS_FINAL_ROUNDS_8X(56);

            pChain->H[0] = ah[7] + pChain->H[0];
            pChain->H[1] = ah[6] + pChain->H[1];
            pChain->H[2] = ah[5] + pChain->H[2];
            pChain->H[3] = ah[4] + pChain->H[3];
            pChain->H[4] = ah[3] + pChain->H[4];
            pChain->H[5] = ah[2] + pChain->H[5];
            pChain->H[6] = ah[1] + pChain->H[6];
            pChain->H[7] = ah[0] + pChain->H[7];
        }


        for (int bl = 1; bl < numBlocks; bl++)
        {
            ah[7] = pChain->H[0];
            ah[6] = pChain->H[1];
            ah[5] = pChain->H[2];
            ah[4] = pChain->H[3];
            ah[3] = pChain->H[4];
            ah[2] = pChain->H[5];
            ah[1] = pChain->H[6];
            ah[0] = pChain->H[7];

            for (int iterCount=0; iterCount<(64/16); iterCount++)
            {
                const int roundBase = iterCount*16;
                CROUND_8BLOCKS( 0, roundBase, bl);
                CROUND_8BLOCKS( 1, roundBase, bl);
                CROUND_8BLOCKS( 2, roundBase, bl);
                CROUND_8BLOCKS( 3, roundBase, bl);
                CROUND_8BLOCKS( 4, roundBase, bl);
                CROUND_8BLOCKS( 5, roundBase, bl);
                CROUND_8BLOCKS( 6, roundBase, bl);
                CROUND_8BLOCKS( 7, roundBase, bl);
                CROUND_8BLOCKS( 8, roundBase, bl);
                CROUND_8BLOCKS( 9, roundBase, bl);
                CROUND_8BLOCKS(10, roundBase, bl);
                CROUND_8BLOCKS(11, roundBase, bl);
                CROUND_8BLOCKS(12, roundBase, bl);
                CROUND_8BLOCKS(13, roundBase, bl);
                CROUND_8BLOCKS(14, roundBase, bl);
                CROUND_8BLOCKS(15, roundBase, bl);
            }

            pChain->H[0] = ah[7] + pChain->H[0];
            pChain->H[1] = ah[6] + pChain->H[1];
            pChain->H[2] = ah[5] + pChain->H[2];
            pChain->H[3] = ah[4] + pChain->H[3];
            pChain->H[4] = ah[3] + pChain->H[4];
            pChain->H[5] = ah[2] + pChain->H[5];
            pChain->H[6] = ah[1] + pChain->H[6];
            pChain->H[7] = ah[0] + pChain->H[7];
        }

        pbData += (numBlocks * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE);
        cbData -= (numBlocks * SYMCRYPT_SHA256_INPUT_BLOCK_SIZE);
        
    } 

    _mm256_zeroupper();


    while (cbData >= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE)
    {
        ah[7] = pChain->H[0];
        ah[6] = pChain->H[1];
        ah[5] = pChain->H[2];
        ah[4] = pChain->H[3];
        ah[3] = pChain->H[4];
        ah[2] = pChain->H[5];
        ah[1] = pChain->H[6];
        ah[0] = pChain->H[7];

        //
        // initial rounds 1 to 16
        //

        IROUND(0);
        IROUND(1);
        IROUND(2);
        IROUND(3);
        IROUND(4);
        IROUND(5);
        IROUND(6);
        IROUND(7);
        IROUND(8);
        IROUND(9);
        IROUND(10);
        IROUND(11);
        IROUND(12);
        IROUND(13);
        IROUND(14);
        IROUND(15);


        //
        // rounds 16 to 64.
        //
        for (int iterCount=1; iterCount<(64/16); iterCount++)
        {
            const int roundBase = iterCount*16; 
            FROUND( 0, roundBase);
            FROUND( 1, roundBase);
            FROUND( 2, roundBase);
            FROUND( 3, roundBase);
            FROUND( 4, roundBase);
            FROUND( 5, roundBase);
            FROUND( 6, roundBase);
            FROUND( 7, roundBase);
            FROUND( 8, roundBase);
            FROUND( 9, roundBase);
            FROUND(10, roundBase);
            FROUND(11, roundBase);
            FROUND(12, roundBase);
            FROUND(13, roundBase);
            FROUND(14, roundBase);
            FROUND(15, roundBase);
        }

        pChain->H[0] = ah[7] + pChain->H[0];
        pChain->H[1] = ah[6] + pChain->H[1];
        pChain->H[2] = ah[5] + pChain->H[2];
        pChain->H[3] = ah[4] + pChain->H[3];
        pChain->H[4] = ah[3] + pChain->H[4];
        pChain->H[5] = ah[2] + pChain->H[5];
        pChain->H[6] = ah[1] + pChain->H[6];
        pChain->H[7] = ah[0] + pChain->H[7];

        pbData += SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;
        cbData -= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;
    }

    *pcbRemaining = cbData;

    //
    // Wipe the variables;
    //
    SymCryptWipe(&Wx, uWipeSize);
    SymCryptWipeKnownSize(ah, sizeof(ah));
    SYMCRYPT_FORCE_WRITE32(&Wt, 0);
}


#endif // SYMCRYPT_CPU_AMD64

