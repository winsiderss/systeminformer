//
// aes-zmm.c    code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// All ZMM code for AES operations
// Requires compiler support for aesni, pclmulqdq, vaes, vpclmulqdq, avx512bw and avx512dq
//

#include "precomp.h"

#if SYMCRYPT_CPU_AMD64

#include "xtsaes_definitions.h"
#include "ghash_definitions.h"

#define AES_ENCRYPT_ZMM_2048( pExpandedKey, c0, c1, c2, c3 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m512i roundkeys; \
\
    keyPtr = pExpandedKey->RoundKey; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
    keyPtr ++; \
\
    c0 = _mm512_xor_si512( c0, roundkeys ); \
    c1 = _mm512_xor_si512( c1, roundkeys ); \
    c2 = _mm512_xor_si512( c2, roundkeys ); \
    c3 = _mm512_xor_si512( c3, roundkeys ); \
\
    do \
    { \
        roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
        keyPtr ++; \
        c0 = _mm512_aesenc_epi128( c0, roundkeys ); \
        c1 = _mm512_aesenc_epi128( c1, roundkeys ); \
        c2 = _mm512_aesenc_epi128( c2, roundkeys ); \
        c3 = _mm512_aesenc_epi128( c3, roundkeys ); \
    } while( keyPtr < keyLimit ); \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
\
    c0 = _mm512_aesenclast_epi128( c0, roundkeys ); \
    c1 = _mm512_aesenclast_epi128( c1, roundkeys ); \
    c2 = _mm512_aesenclast_epi128( c2, roundkeys ); \
    c3 = _mm512_aesenclast_epi128( c3, roundkeys ); \
};

#define AES_ENCRYPT_ZMM_512( pExpandedKey, c0 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m512i roundkeys; \
\
    keyPtr = pExpandedKey->RoundKey; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
    keyPtr ++; \
\
    c0 = _mm512_xor_si512( c0, roundkeys ); \
\
    do \
    { \
        roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
        keyPtr ++; \
        c0 = _mm512_aesenc_epi128( c0, roundkeys ); \
    } while( keyPtr < keyLimit ); \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
\
    c0 = _mm512_aesenclast_epi128( c0, roundkeys ); \
};

#define AES_DECRYPT_ZMM_2048( pExpandedKey, c0, c1, c2, c3 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m512i roundkeys; \
\
    keyPtr = pExpandedKey->lastEncRoundKey; \
    keyLimit = pExpandedKey->lastDecRoundKey; \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
    keyPtr ++; \
\
    /* _mm512_xor_si512 requires AVX512F */ \
    c0 = _mm512_xor_si512( c0, roundkeys ); \
    c1 = _mm512_xor_si512( c1, roundkeys ); \
    c2 = _mm512_xor_si512( c2, roundkeys ); \
    c3 = _mm512_xor_si512( c3, roundkeys ); \
\
    do \
    { \
        roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
        keyPtr ++; \
        c0 = _mm512_aesdec_epi128( c0, roundkeys ); \
        c1 = _mm512_aesdec_epi128( c1, roundkeys ); \
        c2 = _mm512_aesdec_epi128( c2, roundkeys ); \
        c3 = _mm512_aesdec_epi128( c3, roundkeys ); \
    } while( keyPtr < keyLimit ); \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (__m128i *) keyPtr ) ); \
\
    c0 = _mm512_aesdeclast_epi128( c0, roundkeys ); \
    c1 = _mm512_aesdeclast_epi128( c1, roundkeys ); \
    c2 = _mm512_aesdeclast_epi128( c2, roundkeys ); \
    c3 = _mm512_aesdeclast_epi128( c3, roundkeys ); \
};

VOID
SYMCRYPT_CALL
SymCryptXtsAesEncryptDataUnitZmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i t0, t1, t2, t3, t4, t5, t6, t7;
    __m512i c0, c1, c2, c3;
    __m128i XTS_ALPHA_MASK;
    __m512i XTS_ALPHA_MULTIPLIER_Zmm;

    // Load tweaks into big T
    __m512i T0, T1, T2, T3;

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop

    // To simplify logic and unusual size processing, we handle all
    // data not a multiple of 16 blocks in the tail loop
    cbDataTail = cbData & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1);
    // Additionally, so that ciphertext stealing logic does not rely on
    // reading back from the destination buffer, when we have a non-zero
    // tail, we ensure that we handle at least 1 whole block in the tail
    cbDataTail += ((cbDataTail > 0) && (cbDataTail < SYMCRYPT_AES_BLOCK_SIZE)) ? (16*SYMCRYPT_AES_BLOCK_SIZE) : 0;
    cbDataMain = cbData - cbDataTail;

    SYMCRYPT_ASSERT(cbDataMain <= cbData);
    SYMCRYPT_ASSERT(cbDataTail <= cbData);
    SYMCRYPT_ASSERT((cbDataMain & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1)) == 0);

    if( cbDataMain == 0 )
    {
        SymCryptXtsAesEncryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
        return;
    }

    t0 = _mm_loadu_si128( (__m128i *) pbTweakBlock );
    XTS_ALPHA_MASK = _mm_set_epi32( 1, 1, 1, 0x87 );
    XTS_ALPHA_MULTIPLIER_Zmm = _mm512_set_epi64( 0, 0x87, 0, 0x87, 0, 0x87, 0, 0x87 );

    // Do not stall.
    XTS_MUL_ALPHA4( t0, t4 );
    XTS_MUL_ALPHA ( t0, t1 );
    XTS_MUL_ALPHA ( t4, t5 );
    XTS_MUL_ALPHA ( t1, t2 );
    XTS_MUL_ALPHA ( t5, t6 );
    XTS_MUL_ALPHA ( t2, t3 );
    XTS_MUL_ALPHA ( t6, t7 );

    T0 = _mm512_castsi128_si512( t0 );
    T0 = _mm512_inserti64x2( T0, t1, 1 );
    T0 = _mm512_inserti64x2( T0, t2, 2 );
    T0 = _mm512_inserti64x2( T0, t3, 3 );

    T1 = _mm512_castsi128_si512( t4 );
    T1 = _mm512_inserti64x2( T1, t5, 1 );
    T1 = _mm512_inserti64x2( T1, t6, 2 );
    T1 = _mm512_inserti64x2( T1, t7, 3 );

    XTS_MUL_ALPHA8_ZMM(T0, T2);
    XTS_MUL_ALPHA8_ZMM(T1, T3);

    for(;;)
    {
        c0 = _mm512_xor_si512( T0, _mm512_loadu_si512( ( pbSrc +                           0 ) ) );
        c1 = _mm512_xor_si512( T1, _mm512_loadu_si512( ( pbSrc +   4*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c2 = _mm512_xor_si512( T2, _mm512_loadu_si512( ( pbSrc +   8*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c3 = _mm512_xor_si512( T3, _mm512_loadu_si512( ( pbSrc +  12*SYMCRYPT_AES_BLOCK_SIZE ) ) );

        pbSrc += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        AES_ENCRYPT_ZMM_2048( pExpandedKey, c0, c1, c2, c3 );
        _mm512_storeu_si512( ( pbDst +                          0 ), _mm512_xor_si512( c0, T0 ) );
        _mm512_storeu_si512( ( pbDst +  4*SYMCRYPT_AES_BLOCK_SIZE ), _mm512_xor_si512( c1, T1 ) );
        _mm512_storeu_si512( ( pbDst +  8*SYMCRYPT_AES_BLOCK_SIZE ), _mm512_xor_si512( c2, T2 ) );
        _mm512_storeu_si512( ( pbDst + 12*SYMCRYPT_AES_BLOCK_SIZE ), _mm512_xor_si512( c3, T3 ) );

        pbDst += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        cbDataMain -= 16 * SYMCRYPT_AES_BLOCK_SIZE;
        if( cbDataMain < 16 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            break;
        }

        XTS_MUL_ALPHA16_ZMM(T0, T0);
        XTS_MUL_ALPHA16_ZMM(T1, T1);
        XTS_MUL_ALPHA16_ZMM(T2, T2);
        XTS_MUL_ALPHA16_ZMM(T3, T3);
    }

    // We won't do another 16-block set so we don't update the tweak blocks

    if( cbDataTail > 0 )
    {
        //
        // This is a rare case: the data unit length is not a multiple of 256 bytes.
        // We do this in the Xmm implementation.
        // Fix up the tweak block first
        //
        t7 = _mm512_extracti64x2_epi64( T3, 3 /* Highest 128 bits */ );
        _mm256_zeroupper();
        XTS_MUL_ALPHA( t7, t0 );
        _mm_storeu_si128( (__m128i *) pbTweakBlock, t0 );

        SymCryptXtsAesEncryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
    }
    else {
        _mm256_zeroupper();
    }
}

VOID
SYMCRYPT_CALL
SymCryptXtsAesDecryptDataUnitZmm_2048(
    _In_                                        PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _Inout_updates_( SYMCRYPT_AES_BLOCK_SIZE )  PBYTE                       pbTweakBlock,
    _Out_writes_( SYMCRYPT_AES_BLOCK_SIZE*16 )  PBYTE                       pbScratch,
    _In_reads_( cbData )                        PCBYTE                      pbSrc,
    _Out_writes_( cbData )                      PBYTE                       pbDst,
                                                SIZE_T                      cbData )
{
    __m128i t0, t1, t2, t3, t4, t5, t6, t7;
    __m512i c0, c1, c2, c3;
    __m128i XTS_ALPHA_MASK;
    __m512i XTS_ALPHA_MULTIPLIER_Zmm;

    // Load tweaks into big T
    __m512i T0, T1, T2, T3;

    SIZE_T cbDataMain;  // number of bytes to handle in the main loop
    SIZE_T cbDataTail;  // number of bytes to handle in the tail loop

    // To simplify logic and unusual size processing, we handle all
    // data not a multiple of 16 blocks in the tail loop
    cbDataTail = cbData & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1);
    // Additionally, so that ciphertext stealing logic does not rely on
    // reading back from the destination buffer, when we have a non-zero
    // tail, we ensure that we handle at least 1 whole block in the tail
    cbDataTail += ((cbDataTail > 0) && (cbDataTail < SYMCRYPT_AES_BLOCK_SIZE)) ? (16*SYMCRYPT_AES_BLOCK_SIZE) : 0;
    cbDataMain = cbData - cbDataTail;

    SYMCRYPT_ASSERT(cbDataMain <= cbData);
    SYMCRYPT_ASSERT(cbDataTail <= cbData);
    SYMCRYPT_ASSERT((cbDataMain & ((16*SYMCRYPT_AES_BLOCK_SIZE)-1)) == 0);

    if( cbDataMain == 0 )
    {
        SymCryptXtsAesDecryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
        return;
    }

    t0 = _mm_loadu_si128( (__m128i *) pbTweakBlock );
    XTS_ALPHA_MASK = _mm_set_epi32( 1, 1, 1, 0x87 );
    XTS_ALPHA_MULTIPLIER_Zmm = _mm512_set_epi64( 0, 0x87, 0, 0x87, 0, 0x87, 0, 0x87 );

    // Do not stall.
    XTS_MUL_ALPHA4( t0, t4 );
    XTS_MUL_ALPHA ( t0, t1 );
    XTS_MUL_ALPHA ( t4, t5 );
    XTS_MUL_ALPHA ( t1, t2 );
    XTS_MUL_ALPHA ( t5, t6 );
    XTS_MUL_ALPHA ( t2, t3 );
    XTS_MUL_ALPHA ( t6, t7 );

    T0 = _mm512_castsi128_si512( t0 );
    T0 = _mm512_inserti64x2( T0, t1, 1 );
    T0 = _mm512_inserti64x2( T0, t2, 2 );
    T0 = _mm512_inserti64x2( T0, t3, 3 );

    T1 = _mm512_castsi128_si512( t4 );
    T1 = _mm512_inserti64x2( T1, t5, 1 );
    T1 = _mm512_inserti64x2( T1, t6, 2 );
    T1 = _mm512_inserti64x2( T1, t7, 3 );

    XTS_MUL_ALPHA8_ZMM(T0, T2);
    XTS_MUL_ALPHA8_ZMM(T1, T3);

    for(;;)
    {
        c0 = _mm512_xor_si512( T0, _mm512_loadu_si512( ( pbSrc +                           0 ) ) );
        c1 = _mm512_xor_si512( T1, _mm512_loadu_si512( ( pbSrc +   4*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c2 = _mm512_xor_si512( T2, _mm512_loadu_si512( ( pbSrc +   8*SYMCRYPT_AES_BLOCK_SIZE ) ) );
        c3 = _mm512_xor_si512( T3, _mm512_loadu_si512( ( pbSrc +  12*SYMCRYPT_AES_BLOCK_SIZE ) ) );

        pbSrc += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        AES_DECRYPT_ZMM_2048( pExpandedKey, c0, c1, c2, c3 );
        _mm512_storeu_si512( ( pbDst +                          0 ), _mm512_xor_si512( c0, T0 ) );
        _mm512_storeu_si512( ( pbDst +  4*SYMCRYPT_AES_BLOCK_SIZE ), _mm512_xor_si512( c1, T1 ) );
        _mm512_storeu_si512( ( pbDst +  8*SYMCRYPT_AES_BLOCK_SIZE ), _mm512_xor_si512( c2, T2 ) );
        _mm512_storeu_si512( ( pbDst + 12*SYMCRYPT_AES_BLOCK_SIZE ), _mm512_xor_si512( c3, T3 ) );

        pbDst += 16 * SYMCRYPT_AES_BLOCK_SIZE;

        cbDataMain -= 16 * SYMCRYPT_AES_BLOCK_SIZE;
        if( cbDataMain < 16 * SYMCRYPT_AES_BLOCK_SIZE )
        {
            break;
        }

        XTS_MUL_ALPHA16_ZMM(T0, T0);
        XTS_MUL_ALPHA16_ZMM(T1, T1);
        XTS_MUL_ALPHA16_ZMM(T2, T2);
        XTS_MUL_ALPHA16_ZMM(T3, T3);
    }

    // We won't do another 16-block set so we don't update the tweak blocks

    if( cbDataTail > 0 )
    {
        //
        // This is a rare case: the data unit length is not a multiple of 256 bytes.
        // We do this in the Xmm implementation.
        // Fix up the tweak block first
        //
        t7 = _mm512_extracti64x2_epi64( T3, 3 /* Highest 128 bits */ );
        _mm256_zeroupper();
        XTS_MUL_ALPHA( t7, t0 );
        _mm_storeu_si128( (__m128i *) pbTweakBlock, t0 );

        SymCryptXtsAesDecryptDataUnitXmm( pExpandedKey, pbTweakBlock, pbScratch, pbSrc, pbDst, cbDataTail );
    }
    else {
        _mm256_zeroupper();
    }
}

//
// AES-GCM ZMM (AVX-512 VAES) implementation
// Processes 32 blocks (512 bytes) per iteration using 8 ZMM registers
//

// Reduce 3 ZMM accumulators to their bottom 128 bits by XOR-folding 512->256->128.
// After this macro, the bottom 128 bits of each ZMM hold the reduced value, and the
// corresponding XMM aliases are set via _mm512_castsi512_si128.. Ideally this will be
// optimized by the compiler such that the XMM variables are just aliases and no 
// additional registers or instructions are needed to store them.
#define REDUCE_ZMM_TO_XMM_3( a0, a1, a2, a0_xmm, a1_xmm, a2_xmm ) \
{ \
    (a0) = _mm512_zextsi256_si512( _mm256_xor_si256( _mm512_extracti32x8_epi32((a0), 1), _mm512_castsi512_si256((a0)) ) ); \
    (a0) = _mm512_zextsi128_si512( _mm_xor_si128( _mm512_extracti64x2_epi64((a0), 1), _mm512_castsi512_si128((a0)) ) ); \
    (a1) = _mm512_zextsi256_si512( _mm256_xor_si256( _mm512_extracti32x8_epi32((a1), 1), _mm512_castsi512_si256((a1)) ) ); \
    (a1) = _mm512_zextsi128_si512( _mm_xor_si128( _mm512_extracti64x2_epi64((a1), 1), _mm512_castsi512_si128((a1)) ) ); \
    (a2) = _mm512_zextsi256_si512( _mm256_xor_si256( _mm512_extracti32x8_epi32((a2), 1), _mm512_castsi512_si256((a2)) ) ); \
    (a2) = _mm512_zextsi128_si512( _mm_xor_si128( _mm512_extracti64x2_epi64((a2), 1), _mm512_castsi512_si128((a2)) ) ); \
    (a0_xmm) = _mm512_castsi512_si128( (a0) ); \
    (a1_xmm) = _mm512_castsi512_si128( (a1) ); \
    (a2_xmm) = _mm512_castsi512_si128( (a2) ); \
}

// Compute a __mmask16 for a ZMM register holding up to 4 AES blocks (16 dwords).
// nPart: total remaining blocks (0 to 31)
// regIdx: which ZMM register (0 to 7), each holding 4 blocks at offset regIdx*4
// Returns: 0xFFFF for full register, partial mask for 1-3 blocks, or 0 for inactive register.
#define GCM_ZMM_MASK16( nPart, regIdx ) \
    ( ((SIZE_T)(nPart) >= (SIZE_T)((regIdx) + 1) * 4) \
      ? (__mmask16)0xFFFF \
      : ((SIZE_T)(nPart) > (SIZE_T)(regIdx) * 4) \
        ? (__mmask16)((1u << (((nPart) - (SIZE_T)(regIdx) * 4) * 4)) - 1) \
        : (__mmask16)0 )

// Number of blocks processed per full ZMM iteration (8 ZMM regs x 4 blocks each)
#define GCM_ZMM_FULLROUND_BLOCKS 32

#define AES_ENCRYPT_ZMM_4096( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m512i roundkeys; \
\
    keyPtr = pExpandedKey->RoundKey; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr )  ); \
    keyPtr ++; \
\
    c0 = _mm512_xor_si512( c0, roundkeys ); \
    c1 = _mm512_xor_si512( c1, roundkeys ); \
    c2 = _mm512_xor_si512( c2, roundkeys ); \
    c3 = _mm512_xor_si512( c3, roundkeys ); \
    c4 = _mm512_xor_si512( c4, roundkeys ); \
    c5 = _mm512_xor_si512( c5, roundkeys ); \
    c6 = _mm512_xor_si512( c6, roundkeys ); \
    c7 = _mm512_xor_si512( c7, roundkeys ); \
\
    do \
    { \
        roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr )  ); \
        keyPtr ++; \
        c0 = _mm512_aesenc_epi128( c0, roundkeys ); \
        c1 = _mm512_aesenc_epi128( c1, roundkeys ); \
        c2 = _mm512_aesenc_epi128( c2, roundkeys ); \
        c3 = _mm512_aesenc_epi128( c3, roundkeys ); \
        c4 = _mm512_aesenc_epi128( c4, roundkeys ); \
        c5 = _mm512_aesenc_epi128( c5, roundkeys ); \
        c6 = _mm512_aesenc_epi128( c6, roundkeys ); \
        c7 = _mm512_aesenc_epi128( c7, roundkeys ); \
    } while( keyPtr < keyLimit ); \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr )  ); \
\
    c0 = _mm512_aesenclast_epi128( c0, roundkeys ); \
    c1 = _mm512_aesenclast_epi128( c1, roundkeys ); \
    c2 = _mm512_aesenclast_epi128( c2, roundkeys ); \
    c3 = _mm512_aesenclast_epi128( c3, roundkeys ); \
    c4 = _mm512_aesenclast_epi128( c4, roundkeys ); \
    c5 = _mm512_aesenclast_epi128( c5, roundkeys ); \
    c6 = _mm512_aesenclast_epi128( c6, roundkeys ); \
    c7 = _mm512_aesenclast_epi128( c7, roundkeys ); \
};

// Paired stitched rounds: 2 AES rounds on c0-c7 + 8 GHASH blocks with deferred vpternlogq
// accumulation. First round computes GHASH partial products into tlow/tmid/thigh temporaries.
// Second round computes new partial products and uses TERNARY_XOR_512 to accumulate both
// rounds' results into resl/resm/resh in a single instruction (3-way XOR). roundkeys must be
// pre-loaded before the macro is used. Loads the NEXT round key during each GHASH gap.
#define AES_2FULLROUNDS_32_GHASH_8_Zmm( roundkeys, keyPtr, c0, c1, c2, c3, c4, c5, c6, c7, r0, t0, t1, tlow, tmid, thigh, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    c0 = _mm512_aesenc_epi128( c0, roundkeys ); \
    c1 = _mm512_aesenc_epi128( c1, roundkeys ); \
    c2 = _mm512_aesenc_epi128( c2, roundkeys ); \
    c3 = _mm512_aesenc_epi128( c3, roundkeys ); \
    c4 = _mm512_aesenc_epi128( c4, roundkeys ); \
    c5 = _mm512_aesenc_epi128( c5, roundkeys ); \
    c6 = _mm512_aesenc_epi128( c6, roundkeys ); \
    c7 = _mm512_aesenc_epi128( c7, roundkeys ); \
\
    r0 = _mm512_loadu_si512( (__m512i *) gHashPointer ); \
    r0 = _mm512_shuffle_epi8( r0, byteReverseOrder ); \
    gHashPointer += 64; \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr) ); \
    keyPtr ++; \
\
    t1 = _mm512_loadu_si512( (__m512i *) &GHASH_H_POWER(gHashExpandedKeyTable, todo) ); \
    tlow = _mm512_clmulepi64_epi128( r0, t1, 0x00 ); \
    thigh = _mm512_clmulepi64_epi128( r0, t1, 0x11 ); \
\
    t0 = _mm512_bsrli_epi128( r0, 8 ); \
    r0 = _mm512_xor_si512( r0, t0 ); \
    t1 = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(gHashExpandedKeyTable, todo) ); \
    tmid = _mm512_clmulepi64_epi128( r0, t1, 0x00 ); \
    todo -= 4; \
\
    c0 = _mm512_aesenc_epi128( c0, roundkeys ); \
    c1 = _mm512_aesenc_epi128( c1, roundkeys ); \
    c2 = _mm512_aesenc_epi128( c2, roundkeys ); \
    c3 = _mm512_aesenc_epi128( c3, roundkeys ); \
    c4 = _mm512_aesenc_epi128( c4, roundkeys ); \
    c5 = _mm512_aesenc_epi128( c5, roundkeys ); \
    c6 = _mm512_aesenc_epi128( c6, roundkeys ); \
    c7 = _mm512_aesenc_epi128( c7, roundkeys ); \
\
    r0 = _mm512_loadu_si512( (__m512i *) gHashPointer ); \
    r0 = _mm512_shuffle_epi8( r0, byteReverseOrder ); \
    gHashPointer += 64; \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr ) ); \
    keyPtr ++; \
\
    t1 = _mm512_loadu_si512( (__m512i *) &GHASH_H_POWER(gHashExpandedKeyTable, todo) ); \
    t0 = _mm512_clmulepi64_epi128( r0, t1, 0x00 ); \
    t1 = _mm512_clmulepi64_epi128( r0, t1, 0x11 ); \
    resl = TERNARY_XOR_512( resl, t0, tlow ); \
    resh = TERNARY_XOR_512( resh, t1, thigh ); \
\
    t0 = _mm512_bsrli_epi128( r0, 8 ); \
    r0 = _mm512_xor_si512( r0, t0 ); \
    t1 = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(gHashExpandedKeyTable, todo) ); \
    t1 = _mm512_clmulepi64_epi128( r0, t1, 0x00 ); \
    resm = TERNARY_XOR_512( resm, t1, tmid ); \
    todo -= 4; \
};

// Full stitched iteration: 8 stitched AES+GHASH rounds (consuming 32 GHASH blocks), remaining AES rounds.
// Round key loads are software-pipelined: each stitched round uses a pre-loaded key and loads
// the NEXT key during the GHASH gap, hiding broadcast latency behind CLMUL work.
#define AES_GCM_ENCRYPT_32_Zmm( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ) \
{ \
    const BYTE (*keyPtr)[4][4]; \
    const BYTE (*keyLimit)[4][4]; \
    __m512i roundkeys; \
    __m512i t0, t1; \
    __m512i tlow, tmid, thigh; \
    __m512i r0; \
    int aesEncryptGhashLoop; \
\
    keyPtr = pExpandedKey->RoundKey; \
    keyLimit = pExpandedKey->lastEncRoundKey; \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr ) ); \
    keyPtr ++; \
\
    c0 = _mm512_xor_si512( c0, roundkeys ); \
    c1 = _mm512_xor_si512( c1, roundkeys ); \
    c2 = _mm512_xor_si512( c2, roundkeys ); \
    c3 = _mm512_xor_si512( c3, roundkeys ); \
    c4 = _mm512_xor_si512( c4, roundkeys ); \
    c5 = _mm512_xor_si512( c5, roundkeys ); \
    c6 = _mm512_xor_si512( c6, roundkeys ); \
    c7 = _mm512_xor_si512( c7, roundkeys ); \
\
    /* Pre-load first stitched round key */ \
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr ) ); \
    keyPtr ++; \
\
    /* Do 4 paired rounds with stitched GHASH (8 GHASH blocks per pair = 32 total, 2 AES rounds per pair = 8 total) */ \
    for( aesEncryptGhashLoop = 0; aesEncryptGhashLoop < 4; aesEncryptGhashLoop++ ) \
    { \
        AES_2FULLROUNDS_32_GHASH_8_Zmm( roundkeys, keyPtr, c0, c1, c2, c3, c4, c5, c6, c7, r0, t0, t1, tlow, tmid, thigh, gHashPointer, byteReverseOrder, gHashExpandedKeyTable, todo, resl, resm, resh ); \
    } \
\
    /* roundkeys now holds one key ahead from the last GHASH pre-load. */ \
    /* Apply remaining AES rounds (for AES-192/256 there are extra rounds beyond the 8 stitched). */ \
    while( keyPtr < keyLimit ) \
    { \
        c0 = _mm512_aesenc_epi128( c0, roundkeys ); \
        c1 = _mm512_aesenc_epi128( c1, roundkeys ); \
        c2 = _mm512_aesenc_epi128( c2, roundkeys ); \
        c3 = _mm512_aesenc_epi128( c3, roundkeys ); \
        c4 = _mm512_aesenc_epi128( c4, roundkeys ); \
        c5 = _mm512_aesenc_epi128( c5, roundkeys ); \
        c6 = _mm512_aesenc_epi128( c6, roundkeys ); \
        c7 = _mm512_aesenc_epi128( c7, roundkeys ); \
        roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr ) ); \
        keyPtr ++; \
    } \
\
    /* Apply final vaesenc round with the pre-loaded key */ \
    c0 = _mm512_aesenc_epi128( c0, roundkeys ); \
    c1 = _mm512_aesenc_epi128( c1, roundkeys ); \
    c2 = _mm512_aesenc_epi128( c2, roundkeys ); \
    c3 = _mm512_aesenc_epi128( c3, roundkeys ); \
    c4 = _mm512_aesenc_epi128( c4, roundkeys ); \
    c5 = _mm512_aesenc_epi128( c5, roundkeys ); \
    c6 = _mm512_aesenc_epi128( c6, roundkeys ); \
    c7 = _mm512_aesenc_epi128( c7, roundkeys ); \
\
    roundkeys = _mm512_broadcast_i32x4( _mm_loadu_si128( (const __m128i*) keyPtr ) ); \
\
    c0 = _mm512_aesenclast_epi128( c0, roundkeys ); \
    c1 = _mm512_aesenclast_epi128( c1, roundkeys ); \
    c2 = _mm512_aesenclast_epi128( c2, roundkeys ); \
    c3 = _mm512_aesenclast_epi128( c3, roundkeys ); \
    c4 = _mm512_aesenclast_epi128( c4, roundkeys ); \
    c5 = _mm512_aesenclast_epi128( c5, roundkeys ); \
    c6 = _mm512_aesenclast_epi128( c6, roundkeys ); \
    c7 = _mm512_aesenclast_epi128( c7, roundkeys ); \
};

VOID
SYMCRYPT_CALL
SymCryptAesGcmEncryptStitchedZmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __m128i chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    __m128i BYTE_REVERSE_ORDER_xmm = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m512i BYTE_REVERSE_ORDER = _mm512_set_epi64(
            0x0001020304050607, 0x08090a0b0c0d0e0f,
            0x0001020304050607, 0x08090a0b0c0d0e0f,
            0x0001020304050607, 0x08090a0b0c0d0e0f,
            0x0001020304050607, 0x08090a0b0c0d0e0f );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    __m512i chainIncrement4  = _mm512_set_epi32( 0,0,0,4, 0,0,0,4, 0,0,0,4, 0,0,0,4 );
    __m512i chainIncrement32 = _mm512_set_epi32( 0,0,0,32, 0,0,0,32, 0,0,0,32, 0,0,0,32 );

    __m512i ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    __m512i c0, c1, c2, c3, c4, c5, c6, c7;
    __m512i r0, r1, r2, r3, r4, r5, r6, r7;
    __m512i Hi, Hix, Hi2, Hix2;

    __m128i state;
    __m128i a0_xmm, a1_xmm, a2_xmm;
    __m512i a0, a1, a2;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    PCBYTE pbGhashSrc = pbDst;

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );

    state = _mm_loadu_si128( (__m128i *) pState );

    // Set up 8 ZMM counters, each holding 4 sequential counter values
    // ctr0 = [ctr+0, ctr+1, ctr+2, ctr+3], ctr1 = [ctr+4, ..., ctr+7], etc.
    // Counter field is at the lowest 32-bit lane of each 128-bit block after byte-reversal
    ctr0 = _mm512_broadcast_i32x4( chain );
    ctr0 = _mm512_add_epi32( ctr0, _mm512_set_epi32( 0,0,0,3, 0,0,0,2, 0,0,0,1, 0,0,0,0 ) );
    ctr1 = _mm512_add_epi32( ctr0, chainIncrement4 );
    ctr2 = _mm512_add_epi32( ctr1, chainIncrement4 );
    ctr3 = _mm512_add_epi32( ctr2, chainIncrement4 );
    ctr4 = _mm512_add_epi32( ctr3, chainIncrement4 );
    ctr5 = _mm512_add_epi32( ctr4, chainIncrement4 );
    ctr6 = _mm512_add_epi32( ctr5, chainIncrement4 );
    ctr7 = _mm512_add_epi32( ctr6, chainIncrement4 );

    if( nBlocks >= GCM_ZMM_FULLROUND_BLOCKS )
    {
        SIZE_T todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_ZMM_FULLROUND_BLOCKS-1);

        CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
        a0 = _mm512_zextsi128_si512( a0_xmm );
        a1 = _mm512_zextsi128_si512( a1_xmm );
        a2 = _mm512_zextsi128_si512( a2_xmm );

        // First iteration: pure AES encryption (no GHASH data yet for encrypt)
        c0 = _mm512_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
        c1 = _mm512_shuffle_epi8( ctr1, BYTE_REVERSE_ORDER );
        c2 = _mm512_shuffle_epi8( ctr2, BYTE_REVERSE_ORDER );
        c3 = _mm512_shuffle_epi8( ctr3, BYTE_REVERSE_ORDER );
        c4 = _mm512_shuffle_epi8( ctr4, BYTE_REVERSE_ORDER );
        c5 = _mm512_shuffle_epi8( ctr5, BYTE_REVERSE_ORDER );
        c6 = _mm512_shuffle_epi8( ctr6, BYTE_REVERSE_ORDER );
        c7 = _mm512_shuffle_epi8( ctr7, BYTE_REVERSE_ORDER );

        ctr0 = _mm512_add_epi32( ctr0, chainIncrement32 );
        ctr1 = _mm512_add_epi32( ctr1, chainIncrement32 );
        ctr2 = _mm512_add_epi32( ctr2, chainIncrement32 );
        ctr3 = _mm512_add_epi32( ctr3, chainIncrement32 );
        ctr4 = _mm512_add_epi32( ctr4, chainIncrement32 );
        ctr5 = _mm512_add_epi32( ctr5, chainIncrement32 );
        ctr6 = _mm512_add_epi32( ctr6, chainIncrement32 );
        ctr7 = _mm512_add_epi32( ctr7, chainIncrement32 );

        AES_ENCRYPT_ZMM_4096( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7 );

        _mm512_storeu_si512( (pbDst +   0), _mm512_xor_si512( c0, _mm512_loadu_si512( pbSrc +   0 ) ) );
        _mm512_storeu_si512( (pbDst +  64), _mm512_xor_si512( c1, _mm512_loadu_si512( pbSrc +  64 ) ) );
        _mm512_storeu_si512( (pbDst + 128), _mm512_xor_si512( c2, _mm512_loadu_si512( pbSrc + 128 ) ) );
        _mm512_storeu_si512( (pbDst + 192), _mm512_xor_si512( c3, _mm512_loadu_si512( pbSrc + 192 ) ) );
        _mm512_storeu_si512( (pbDst + 256), _mm512_xor_si512( c4, _mm512_loadu_si512( pbSrc + 256 ) ) );
        _mm512_storeu_si512( (pbDst + 320), _mm512_xor_si512( c5, _mm512_loadu_si512( pbSrc + 320 ) ) );
        _mm512_storeu_si512( (pbDst + 384), _mm512_xor_si512( c6, _mm512_loadu_si512( pbSrc + 384 ) ) );
        _mm512_storeu_si512( (pbDst + 448), _mm512_xor_si512( c7, _mm512_loadu_si512( pbSrc + 448 ) ) );

        pbDst  += 32 * SYMCRYPT_AES_BLOCK_SIZE;
        pbSrc  += 32 * SYMCRYPT_AES_BLOCK_SIZE;

        while( nBlocks >= 2*GCM_ZMM_FULLROUND_BLOCKS )
        {
            c0 = _mm512_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
            c1 = _mm512_shuffle_epi8( ctr1, BYTE_REVERSE_ORDER );
            c2 = _mm512_shuffle_epi8( ctr2, BYTE_REVERSE_ORDER );
            c3 = _mm512_shuffle_epi8( ctr3, BYTE_REVERSE_ORDER );
            c4 = _mm512_shuffle_epi8( ctr4, BYTE_REVERSE_ORDER );
            c5 = _mm512_shuffle_epi8( ctr5, BYTE_REVERSE_ORDER );
            c6 = _mm512_shuffle_epi8( ctr6, BYTE_REVERSE_ORDER );
            c7 = _mm512_shuffle_epi8( ctr7, BYTE_REVERSE_ORDER );

            ctr0 = _mm512_add_epi32( ctr0, chainIncrement32 );
            ctr1 = _mm512_add_epi32( ctr1, chainIncrement32 );
            ctr2 = _mm512_add_epi32( ctr2, chainIncrement32 );
            ctr3 = _mm512_add_epi32( ctr3, chainIncrement32 );
            ctr4 = _mm512_add_epi32( ctr4, chainIncrement32 );
            ctr5 = _mm512_add_epi32( ctr5, chainIncrement32 );
            ctr6 = _mm512_add_epi32( ctr6, chainIncrement32 );
            ctr7 = _mm512_add_epi32( ctr7, chainIncrement32 );

            AES_GCM_ENCRYPT_32_Zmm( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );

            _mm512_storeu_si512( (pbDst +   0), _mm512_xor_si512( c0, _mm512_loadu_si512( pbSrc +   0 ) ) );
            _mm512_storeu_si512( (pbDst +  64), _mm512_xor_si512( c1, _mm512_loadu_si512( pbSrc +  64 ) ) );
            _mm512_storeu_si512( (pbDst + 128), _mm512_xor_si512( c2, _mm512_loadu_si512( pbSrc + 128 ) ) );
            _mm512_storeu_si512( (pbDst + 192), _mm512_xor_si512( c3, _mm512_loadu_si512( pbSrc + 192 ) ) );
            _mm512_storeu_si512( (pbDst + 256), _mm512_xor_si512( c4, _mm512_loadu_si512( pbSrc + 256 ) ) );
            _mm512_storeu_si512( (pbDst + 320), _mm512_xor_si512( c5, _mm512_loadu_si512( pbSrc + 320 ) ) );
            _mm512_storeu_si512( (pbDst + 384), _mm512_xor_si512( c6, _mm512_loadu_si512( pbSrc + 384 ) ) );
            _mm512_storeu_si512( (pbDst + 448), _mm512_xor_si512( c7, _mm512_loadu_si512( pbSrc + 448 ) ) );

            pbDst  += 32 * SYMCRYPT_AES_BLOCK_SIZE;
            pbSrc  += 32 * SYMCRYPT_AES_BLOCK_SIZE;
            nBlocks -= 32;

            if ( todo == 0 )
            {
                REDUCE_ZMM_TO_XMM_3( a0, a1, a2, a0_xmm, a1_xmm, a2_xmm );

                a1_xmm = TERNARY_XOR_128( a0_xmm, a1_xmm, a2_xmm ); // CLMUL_3_POST
                MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );

                todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_ZMM_FULLROUND_BLOCKS-1);
                CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
                a0 = _mm512_zextsi128_si512( a0_xmm );
                a1 = _mm512_zextsi128_si512( a1_xmm );
                a2 = _mm512_zextsi128_si512( a2_xmm );
            }
        }

        // Final GHASH of last ciphertext batch (32 blocks)
        r0 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc +   0 ), BYTE_REVERSE_ORDER );
        r1 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc +  64 ), BYTE_REVERSE_ORDER );
        r2 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc + 128 ), BYTE_REVERSE_ORDER );
        r3 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc + 192 ), BYTE_REVERSE_ORDER );
        r4 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc + 256 ), BYTE_REVERSE_ORDER );
        r5 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc + 320 ), BYTE_REVERSE_ORDER );
        r6 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc + 384 ), BYTE_REVERSE_ORDER );
        r7 = _mm512_shuffle_epi8( _mm512_loadu_si512( pbGhashSrc + 448 ), BYTE_REVERSE_ORDER );

        // Use paired CLMUL_2ACC_3_Zmm with vpternlogq for 3-way XOR accumulation
        Hi   = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo -  0) );
        Hix  = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo -  0) );
        Hi2  = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo -  4) );
        Hix2 = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo -  4) );
        CLMUL_2ACC_3_Zmm( r0, Hi, Hix, r1, Hi2, Hix2, a0, a1, a2 );

        Hi   = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo -  8) );
        Hix  = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo -  8) );
        Hi2  = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo - 12) );
        Hix2 = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 12) );
        CLMUL_2ACC_3_Zmm( r2, Hi, Hix, r3, Hi2, Hix2, a0, a1, a2 );

        Hi   = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo - 16) );
        Hix  = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 16) );
        Hi2  = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo - 20) );
        Hix2 = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 20) );
        CLMUL_2ACC_3_Zmm( r4, Hi, Hix, r5, Hi2, Hix2, a0, a1, a2 );

        Hi   = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo - 24) );
        Hix  = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 24) );
        Hi2  = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, todo - 28) );
        Hix2 = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, todo - 28) );
        CLMUL_2ACC_3_Zmm( r6, Hi, Hix, r7, Hi2, Hix2, a0, a1, a2 );

        REDUCE_ZMM_TO_XMM_3( a0, a1, a2, a0_xmm, a1_xmm, a2_xmm );

        a1_xmm = TERNARY_XOR_128( a0_xmm, a1_xmm, a2_xmm ); // CLMUL_3_POST
        MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );

        nBlocks -= GCM_ZMM_FULLROUND_BLOCKS; // Account for first iteration
    }

    // Partial tail: encrypt and GHASH remaining 1-31 blocks. Loop over 4-block chunks using 1 ZMM
    // register; handle the 1-3 block residue via a single masked ZMM register + XMM GHASH.
    if ( nBlocks > 0 )
    {
        // The main full-round path (if taken) leaves pbGhashSrc lagging pbDst by
        // GCM_ZMM_FULLROUND_BLOCKS due to its 1-iteration pipeline lag. The tail has no such lag,
        // so resync pbGhashSrc to the current pbDst.
        pbGhashSrc = pbDst;

        // Initialize tail GHASH accumulator: state * H^nBlocks
        CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, nBlocks), GHASH_Hx_POWER(expandedKeyTable, nBlocks), a0_xmm, a1_xmm, a2_xmm );
        a0 = _mm512_zextsi128_si512( a0_xmm );
        a1 = _mm512_zextsi128_si512( a1_xmm );
        a2 = _mm512_zextsi128_si512( a2_xmm );

        // 4-block-at-a-time ZMM loop
        while ( nBlocks >= 4 )
        {
            c0 = _mm512_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
            ctr0 = _mm512_add_epi32( ctr0, chainIncrement4 );

            AES_ENCRYPT_ZMM_512( pExpandedKey, c0 );

            _mm512_storeu_si512( pbDst, _mm512_xor_si512( c0, _mm512_loadu_si512( pbSrc ) ) );

            r0  = _mm512_shuffle_epi8( _mm512_loadu_si512( (__m512i *)pbGhashSrc ), BYTE_REVERSE_ORDER );
            Hi  = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, nBlocks) );
            Hix = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, nBlocks) );
            CLMUL_ACC_3_Zmm( r0, Hi, Hix, a0, a1, a2 );

            pbGhashSrc += 4 * SYMCRYPT_AES_BLOCK_SIZE;
            pbDst      += 4 * SYMCRYPT_AES_BLOCK_SIZE;
            pbSrc      += 4 * SYMCRYPT_AES_BLOCK_SIZE;
            nBlocks    -= 4;
        }

        // Reduce ZMM accumulator down to XMM. Residue (1-3 blocks) GHASHes in XMM (cheaper than
        // masked ZMM clmul for so few blocks).
        REDUCE_ZMM_TO_XMM_3( a0, a1, a2, a0_xmm, a1_xmm, a2_xmm );

        // 1-3 block residue using 1 masked ZMM register for AES, and XMM CLMUL for GHASH (one
        // block at a time). nBlocks is preserved here so it can advance the chain counter below.
        if ( nBlocks > 0 )
        {
            __mmask16 m0 = (__mmask16)( (1u << ((unsigned)nBlocks * 4)) - 1 );

            c0 = _mm512_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );

            AES_ENCRYPT_ZMM_512( pExpandedKey, c0 );

            _mm512_mask_storeu_epi32( pbDst, m0,
                _mm512_xor_si512( c0, _mm512_maskz_loadu_epi32( m0, (__m512i *)pbSrc ) ) );

            // XMM GHASH for the 1-3 residue blocks; ciphertext is in pbDst (output).
            for ( SIZE_T i = nBlocks; i > 0; i-- )
            {
                __m128i r_xmm = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *)pbGhashSrc ), BYTE_REVERSE_ORDER_xmm );
                CLMUL_ACC_3( r_xmm, GHASH_H_POWER(expandedKeyTable, i), GHASH_Hx_POWER(expandedKeyTable, i), a0_xmm, a1_xmm, a2_xmm );
                pbGhashSrc += SYMCRYPT_AES_BLOCK_SIZE;
            }
        }

        a1_xmm = TERNARY_XOR_128( a0_xmm, a1_xmm, a2_xmm ); // CLMUL_3_POST
        MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );
    }

    // Extract the chain counter (low 128 bits of ctr0). The 4-block tail loop advanced ctr0 by 4
    // per iteration; nBlocks now holds the residue count (0-3) which the masked-residue path did
    // not advance ctr0 by.
    chain = _mm512_extracti64x2_epi64( ctr0, 0 );
    chain = _mm_add_epi32( chain, _mm_set_epi32( 0, 0, 0, (UINT32) nBlocks ) );
    _mm256_zeroupper();

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );
    _mm_storeu_si128((__m128i *) pbChainingValue, chain );
    _mm_storeu_si128((__m128i *) pState, state );
}

VOID
SYMCRYPT_CALL
SymCryptAesGcmDecryptStitchedZmm(
    _In_                                    PCSYMCRYPT_AES_EXPANDED_KEY pExpandedKey,
    _In_reads_( SYMCRYPT_AES_BLOCK_SIZE )   PBYTE                       pbChainingValue,
    _In_reads_( SYMCRYPT_GF128_FIELD_SIZE ) PCSYMCRYPT_GF128_ELEMENT    expandedKeyTable,
    _Inout_                                 PSYMCRYPT_GF128_ELEMENT     pState,
    _In_reads_( cbData )                    PCBYTE                      pbSrc,
    _Out_writes_( cbData )                  PBYTE                       pbDst,
                                            SIZE_T                      cbData )
{
    __m128i chain = _mm_loadu_si128( (__m128i *) pbChainingValue );

    __m128i BYTE_REVERSE_ORDER_xmm = _mm_set_epi8(
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 );
    __m512i BYTE_REVERSE_ORDER = _mm512_set_epi64(
            0x0001020304050607, 0x08090a0b0c0d0e0f,
            0x0001020304050607, 0x08090a0b0c0d0e0f,
            0x0001020304050607, 0x08090a0b0c0d0e0f,
            0x0001020304050607, 0x08090a0b0c0d0e0f );
    __m128i vMultiplicationConstant = _mm_set_epi32( 0, 0, 0xc2000000, 0 );

    __m512i chainIncrement4  = _mm512_set_epi32( 0,0,0,4, 0,0,0,4, 0,0,0,4, 0,0,0,4 );
    __m512i chainIncrement32 = _mm512_set_epi32( 0,0,0,32, 0,0,0,32, 0,0,0,32, 0,0,0,32 );

    __m512i ctr0, ctr1, ctr2, ctr3, ctr4, ctr5, ctr6, ctr7;
    __m512i c0, c1, c2, c3, c4, c5, c6, c7;
    __m512i r0;
    __m512i Hi, Hix;

    __m128i state;
    __m128i a0_xmm, a1_xmm, a2_xmm;
    __m512i a0, a1, a2;
    SIZE_T nBlocks = cbData / SYMCRYPT_GF128_BLOCK_SIZE;
    PCBYTE pbGhashSrc = pbSrc; // Decrypt: GHASH runs on source (ciphertext)

    SYMCRYPT_ASSERT( (cbData & SYMCRYPT_GCM_BLOCK_MOD_MASK) == 0 ); // cbData is multiple of block size

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );

    state = _mm_loadu_si128( (__m128i *) pState );

    // Set up 8 ZMM counters
    // Counter field is at the lowest 32-bit lane of each 128-bit block after byte-reversal
    ctr0 = _mm512_broadcast_i32x4( chain );
    ctr0 = _mm512_add_epi32( ctr0, _mm512_set_epi32( 0,0,0,3, 0,0,0,2, 0,0,0,1, 0,0,0,0 ) );
    ctr1 = _mm512_add_epi32( ctr0, chainIncrement4 );
    ctr2 = _mm512_add_epi32( ctr1, chainIncrement4 );
    ctr3 = _mm512_add_epi32( ctr2, chainIncrement4 );
    ctr4 = _mm512_add_epi32( ctr3, chainIncrement4 );
    ctr5 = _mm512_add_epi32( ctr4, chainIncrement4 );
    ctr6 = _mm512_add_epi32( ctr5, chainIncrement4 );
    ctr7 = _mm512_add_epi32( ctr6, chainIncrement4 );

    if( nBlocks >= GCM_ZMM_FULLROUND_BLOCKS )
    {
        SIZE_T todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_ZMM_FULLROUND_BLOCKS-1);

        CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
        a0 = _mm512_zextsi128_si512( a0_xmm );
        a1 = _mm512_zextsi128_si512( a1_xmm );
        a2 = _mm512_zextsi128_si512( a2_xmm );

        // Decrypt: no 1-iteration lag, GHASH can start immediately on source ciphertext
        while( nBlocks >= GCM_ZMM_FULLROUND_BLOCKS )
        {
            c0 = _mm512_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
            c1 = _mm512_shuffle_epi8( ctr1, BYTE_REVERSE_ORDER );
            c2 = _mm512_shuffle_epi8( ctr2, BYTE_REVERSE_ORDER );
            c3 = _mm512_shuffle_epi8( ctr3, BYTE_REVERSE_ORDER );
            c4 = _mm512_shuffle_epi8( ctr4, BYTE_REVERSE_ORDER );
            c5 = _mm512_shuffle_epi8( ctr5, BYTE_REVERSE_ORDER );
            c6 = _mm512_shuffle_epi8( ctr6, BYTE_REVERSE_ORDER );
            c7 = _mm512_shuffle_epi8( ctr7, BYTE_REVERSE_ORDER );

            ctr0 = _mm512_add_epi32( ctr0, chainIncrement32 );
            ctr1 = _mm512_add_epi32( ctr1, chainIncrement32 );
            ctr2 = _mm512_add_epi32( ctr2, chainIncrement32 );
            ctr3 = _mm512_add_epi32( ctr3, chainIncrement32 );
            ctr4 = _mm512_add_epi32( ctr4, chainIncrement32 );
            ctr5 = _mm512_add_epi32( ctr5, chainIncrement32 );
            ctr6 = _mm512_add_epi32( ctr6, chainIncrement32 );
            ctr7 = _mm512_add_epi32( ctr7, chainIncrement32 );

            AES_GCM_ENCRYPT_32_Zmm( pExpandedKey, c0, c1, c2, c3, c4, c5, c6, c7, pbGhashSrc, BYTE_REVERSE_ORDER, expandedKeyTable, todo, a0, a1, a2 );

            _mm512_storeu_si512( (pbDst +   0), _mm512_xor_si512( c0, _mm512_loadu_si512( pbSrc +   0 ) ) );
            _mm512_storeu_si512( (pbDst +  64), _mm512_xor_si512( c1, _mm512_loadu_si512( pbSrc +  64 ) ) );
            _mm512_storeu_si512( (pbDst + 128), _mm512_xor_si512( c2, _mm512_loadu_si512( pbSrc + 128 ) ) );
            _mm512_storeu_si512( (pbDst + 192), _mm512_xor_si512( c3, _mm512_loadu_si512( pbSrc + 192 ) ) );
            _mm512_storeu_si512( (pbDst + 256), _mm512_xor_si512( c4, _mm512_loadu_si512( pbSrc + 256 ) ) );
            _mm512_storeu_si512( (pbDst + 320), _mm512_xor_si512( c5, _mm512_loadu_si512( pbSrc + 320 ) ) );
            _mm512_storeu_si512( (pbDst + 384), _mm512_xor_si512( c6, _mm512_loadu_si512( pbSrc + 384 ) ) );
            _mm512_storeu_si512( (pbDst + 448), _mm512_xor_si512( c7, _mm512_loadu_si512( pbSrc + 448 ) ) );

            pbDst  += 32 * SYMCRYPT_AES_BLOCK_SIZE;
            pbSrc  += 32 * SYMCRYPT_AES_BLOCK_SIZE;
            nBlocks -= 32;

            if ( todo == 0 )
            {
                REDUCE_ZMM_TO_XMM_3( a0, a1, a2, a0_xmm, a1_xmm, a2_xmm );

                a1_xmm = TERNARY_XOR_128( a0_xmm, a1_xmm, a2_xmm ); // CLMUL_3_POST
                MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );

                if ( nBlocks >= GCM_ZMM_FULLROUND_BLOCKS )
                {
                    todo = SYMCRYPT_MIN( nBlocks, SYMCRYPT_GHASH_PCLMULQDQ_HPOWERS ) & ~(GCM_ZMM_FULLROUND_BLOCKS-1);
                    CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, todo), GHASH_Hx_POWER(expandedKeyTable, todo), a0_xmm, a1_xmm, a2_xmm );
                    a0 = _mm512_zextsi128_si512( a0_xmm );
                    a1 = _mm512_zextsi128_si512( a1_xmm );
                    a2 = _mm512_zextsi128_si512( a2_xmm );
                }
            }
        }
    }

    // Partial tail: GHASH and decrypt remaining 1-31 blocks. Loop over 4-block chunks using 1 ZMM
    // register; handle the 1-3 block residue via a single masked ZMM register + XMM GHASH.
    if ( nBlocks > 0 )
    {
        // Decrypt's main loop keeps pbGhashSrc == pbSrc (no lag), but resync here for symmetry
        // with the encrypt path.
        pbGhashSrc = pbSrc;

        // Initialize tail GHASH accumulator: state * H^nBlocks
        CLMUL_3( state, GHASH_H_POWER(expandedKeyTable, nBlocks), GHASH_Hx_POWER(expandedKeyTable, nBlocks), a0_xmm, a1_xmm, a2_xmm );
        a0 = _mm512_zextsi128_si512( a0_xmm );
        a1 = _mm512_zextsi128_si512( a1_xmm );
        a2 = _mm512_zextsi128_si512( a2_xmm );

        // 4-block-at-a-time ZMM loop
        while ( nBlocks >= 4 )
        {
            c0 = _mm512_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );
            ctr0 = _mm512_add_epi32( ctr0, chainIncrement4 );

            AES_ENCRYPT_ZMM_512( pExpandedKey, c0 );

            r0  = _mm512_shuffle_epi8( _mm512_loadu_si512( (__m512i *)pbGhashSrc ), BYTE_REVERSE_ORDER );
            Hi  = _mm512_loadu_si512( (__m512i *)  &GHASH_H_POWER(expandedKeyTable, nBlocks) );
            Hix = _mm512_loadu_si512( (__m512i *) &GHASH_Hx_POWER(expandedKeyTable, nBlocks) );
            CLMUL_ACC_3_Zmm( r0, Hi, Hix, a0, a1, a2 );

            _mm512_storeu_si512( pbDst, _mm512_xor_si512( c0, _mm512_loadu_si512( pbSrc ) ) );

            pbGhashSrc += 4 * SYMCRYPT_AES_BLOCK_SIZE;
            pbDst      += 4 * SYMCRYPT_AES_BLOCK_SIZE;
            pbSrc      += 4 * SYMCRYPT_AES_BLOCK_SIZE;
            nBlocks    -= 4;
        }

        REDUCE_ZMM_TO_XMM_3( a0, a1, a2, a0_xmm, a1_xmm, a2_xmm );

        // 1-3 block residue using 1 masked ZMM register for AES, and XMM CLMUL for GHASH (one
        // block at a time). nBlocks is preserved here so it can advance the chain counter below.
        if ( nBlocks > 0 )
        {
            __mmask16 m0 = (__mmask16)( (1u << ((unsigned)nBlocks * 4)) - 1 );

            c0 = _mm512_shuffle_epi8( ctr0, BYTE_REVERSE_ORDER );

            AES_ENCRYPT_ZMM_512( pExpandedKey, c0 );

            // XMM GHASH for the 1-3 residue blocks; ciphertext is in pbSrc (input).
            for ( SIZE_T i = nBlocks; i > 0; i-- )
            {
                __m128i r_xmm = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *)pbGhashSrc ), BYTE_REVERSE_ORDER_xmm );
                CLMUL_ACC_3( r_xmm, GHASH_H_POWER(expandedKeyTable, i), GHASH_Hx_POWER(expandedKeyTable, i), a0_xmm, a1_xmm, a2_xmm );
                pbGhashSrc += SYMCRYPT_AES_BLOCK_SIZE;
            }

            _mm512_mask_storeu_epi32( pbDst, m0,
                _mm512_xor_si512( c0, _mm512_maskz_loadu_epi32( m0, (__m512i *)pbSrc ) ) );
        }

        a1_xmm = TERNARY_XOR_128( a0_xmm, a1_xmm, a2_xmm ); // CLMUL_3_POST
        MODREDUCE( vMultiplicationConstant, a0_xmm, a1_xmm, a2_xmm, state );
    }

    // Extract the chain counter (low 128 bits of ctr0). The 4-block tail loop advanced ctr0 by 4
    // per iteration; nBlocks now holds the residue count (0-3) which the masked-residue path did
    // not advance ctr0 by.
    chain = _mm512_extracti64x2_epi64( ctr0, 0 );
    chain = _mm_add_epi32( chain, _mm_set_epi32( 0, 0, 0, (UINT32) nBlocks ) );
    _mm256_zeroupper();

    chain = _mm_shuffle_epi8( chain, BYTE_REVERSE_ORDER_xmm );
    _mm_storeu_si128((__m128i *) pbChainingValue, chain );
    _mm_storeu_si128((__m128i *) pState, state );
}

#endif // CPU_AMD64
