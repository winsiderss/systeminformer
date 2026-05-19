//
// aes-zmm.c    code for AES implementation
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// All ZMM code for AES operations
// Requires compiler support for aesni, pclmulqdq, vaes, vpclmulqdq, avx512bw and avx512dq
//

#include "precomp.h"

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#include "xtsaes_definitions.h"

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

#endif // CPU_X86 | CPU_AMD64