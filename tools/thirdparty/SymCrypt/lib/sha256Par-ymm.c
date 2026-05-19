//
// Sha256Par-ymm.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// All YMM code for SHA256 Parallel operations
// Requires compiler support for avx2
//

#include "precomp.h"

extern SYMCRYPT_ALIGN_AT( 256 ) const UINT32 SymCryptSha256K[64];

#if  SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
//
// Code that uses the YMM registers.
//

#define MAJYMM( x, y, z ) _mm256_or_si256( _mm256_and_si256( _mm256_or_si256( z, y ), x ), _mm256_and_si256( z, y ))
#define CHYMM( x, y, z )  _mm256_xor_si256( _mm256_and_si256( _mm256_xor_si256( z, y ), x ), z )

#define CSIGMA0YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi32(x,30)  , _mm256_srli_epi32(x,  2) ),\
        _mm256_slli_epi32(x,19) ), _mm256_srli_epi32(x, 13) ),\
        _mm256_slli_epi32(x,10) ), _mm256_srli_epi32(x, 22) )
#define CSIGMA1YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi32(x,26)  , _mm256_srli_epi32(x,  6) ),\
        _mm256_slli_epi32(x,21) ), _mm256_srli_epi32(x, 11) ),\
        _mm256_slli_epi32(x,7) ), _mm256_srli_epi32(x, 25) )
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

//
// Transpose macro, convert S0..S7 into R0..R7; R0 is the lane 0, R3 is lane 7.
//
//
// S0 = S00, S01, S02, S03,  S04, S05, S06, S07
// S1 = S10, S11, S12, S13,  S14, S15, S16, S17
// S2 = S20, S21, S22, S23,  S24, S25, S26, S27
// S3 = S30, S31, S32, S33,  S34, S35, S36, S37
// S4 = S40, S41, S42, S43,  S44, S45, S46, S47
// S5 = S50, S51, S52, S53,  S54, S55, S56, S57
// S6 = S60, S61, S62, S63,  S64, S65, S66, S67
// S7 = S70, S71, S72, S73,  S74, S75, S76, S77
//
// T0 = S00, S10, S01, S11,  S04, S14, S05, S15
// T1 = S02, S12, S03, S13,  S06, S16, S07, S17
// T2 = S20, S30, S21, S31,  S24, S34, S25, S35
// T3 = S22, S32, S23, S33,  S26, S36, S27, S37
// T4 = S40, S50, S41, S51,  S44, S54, S45, S55
// T5 = S42, S52, S43, S53,  S46, S56, S47, S57
// T6 = S60, S70, S61, S71,  S64, S74, S65, S75
// T7 = S62, S72, S63, S73,  S66, S76, S67, S77
//
// U0 = S00, S10, S20, S30,  S04, S14, S24, S34
// U1 = S01, S11, S21, S31,  S05, S15, S25, S35
// U2 = S02, S12, S22, S32,  S06, S16, S26, S36
// U3 = S03, S13, S23, S33,  S07, S17, S27, S37
// U4 = S40, S50, S60, S70,  S44, S54, S64, S74
// U5 = S41, S51, S61, S71,  S45, S55, S65, S75
// U6 = S42, S52, S62, S72,  S46, S56, S66, S76
// U7 = S43, S53, S63, S73,  S47, S47, S67, S77
//
// R0 = s00, s10, s20, s30,  s40, s50, s60, s70
// R1 = s01, s11, s21, s31,  s41, s51, s61, s71
// R2 = s02, s12, s22, s32,  s42, s52, s62, s72
// R3 = s03, s13, s23, s33,  s43, s53, s63, s73
// R4 = s04, s14, s24, s34,  s44, s54, s64, s74
// R5 = s05, s15, s25, s35,  s45, s55, s65, s75
// R6 = s06, s16, s26, s36,  s46, s56, s66, s76
// R7 = s07, s17, s27, s37,  s47, s57, s67, s77
//
#define YMM_TRANSPOSE_32( _R0, _R1, _R2, _R3, _R4, _R5, _R6, _R7, _S0, _S1, _S2, _S3, _S4, _S5, _S6, _S7 ) \
    {\
        __m256i _T0, _T1, _T2, _T3, _T4, _T5, _T6, _T7;\
        __m256i _U0, _U1, _U2, _U3, _U4, _U5, _U6, _U7;\
        _T0 = _mm256_unpacklo_epi32( _S0, _S1 );  _T1 = _mm256_unpackhi_epi32( _S0, _S1 );\
        _T2 = _mm256_unpacklo_epi32( _S2, _S3 );  _T3 = _mm256_unpackhi_epi32( _S2, _S3 );\
        _T4 = _mm256_unpacklo_epi32( _S4, _S5 );  _T5 = _mm256_unpackhi_epi32( _S4, _S5 );\
        _T6 = _mm256_unpacklo_epi32( _S6, _S7 );  _T7 = _mm256_unpackhi_epi32( _S6, _S7 );\
        \
        _U0 = _mm256_unpacklo_epi64( _T0, _T2 );  _U1 = _mm256_unpackhi_epi64( _T0, _T2 );\
        _U2 = _mm256_unpacklo_epi64( _T1, _T3 );  _U3 = _mm256_unpackhi_epi64( _T1, _T3 );\
        _U4 = _mm256_unpacklo_epi64( _T4, _T6 );  _U5 = _mm256_unpackhi_epi64( _T4, _T6 );\
        _U6 = _mm256_unpacklo_epi64( _T5, _T7 );  _U7 = _mm256_unpackhi_epi64( _T5, _T7 );\
        \
        _R0 = _mm256_permute2x128_si256( _U0, _U4, 0x20 );  _R1 = _mm256_permute2x128_si256( _U1, _U5, 0x20);\
        _R2 = _mm256_permute2x128_si256( _U2, _U6, 0x20 );  _R3 = _mm256_permute2x128_si256( _U3, _U7, 0x20);\
        _R4 = _mm256_permute2x128_si256( _U0, _U4, 0x31 );  _R5 = _mm256_permute2x128_si256( _U1, _U5, 0x31);\
        _R6 = _mm256_permute2x128_si256( _U2, _U6, 0x31 );  _R7 = _mm256_permute2x128_si256( _U3, _U7, 0x31);\
    }

VOID
SYMCRYPT_CALL
SymCryptParallelSha256AppendBlocks_ymm(
    _Inout_updates_( 8 )                                PSYMCRYPT_SHA256_CHAINING_STATE   * pChain,
    _Inout_updates_( 8 )                                PCBYTE                            * ppByte,
                                                        SIZE_T                              nBytes,
    _Out_writes_( PAR_SCRATCH_ELEMENTS_256 * 32 )       PBYTE                               pScratch )
{
    //
    // Implementation that uses 8 lanes in the YMM registers
    //
    __m256i * buf = (__m256i *)pScratch;
    __m256i * W = &buf[4 + 8];
    __m256i * ha = &buf[4]; // initial state words, in order h, g, ..., b, a
    __m256i A, B, C, D, T;
    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i BYTE_REVERSE_32;
    int r;

    _mm256_zeroupper();
    BYTE_REVERSE_32 = _mm256_set_epi8( 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 );

    //
    // The chaining state can be unaligned on x86, so we use unaligned loads
    //

    T0 = _mm256_loadu_si256( (__m256i *)&pChain[0]->H[0] );
    T1 = _mm256_loadu_si256( (__m256i *)&pChain[1]->H[0] );
    T2 = _mm256_loadu_si256( (__m256i *)&pChain[2]->H[0] );
    T3 = _mm256_loadu_si256( (__m256i *)&pChain[3]->H[0] );
    T4 = _mm256_loadu_si256( (__m256i *)&pChain[4]->H[0] );
    T5 = _mm256_loadu_si256( (__m256i *)&pChain[5]->H[0] );
    T6 = _mm256_loadu_si256( (__m256i *)&pChain[6]->H[0] );
    T7 = _mm256_loadu_si256( (__m256i *)&pChain[7]->H[0] );

    YMM_TRANSPOSE_32( ha[7], ha[6], ha[5], ha[4], ha[3], ha[2], ha[1], ha[0], T0, T1, T2, T3, T4, T5, T6, T7 );

    buf[0] = ha[4];
    buf[1] = ha[5];
    buf[2] = ha[6];
    buf[3] = ha[7];

    while( nBytes >= 64 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r += 8 )
        {
            T0 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[0] ), BYTE_REVERSE_32 ); ppByte[0] += 32;
            T1 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[1] ), BYTE_REVERSE_32 ); ppByte[1] += 32;
            T2 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[2] ), BYTE_REVERSE_32 ); ppByte[2] += 32;
            T3 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[3] ), BYTE_REVERSE_32 ); ppByte[3] += 32;
            T4 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[4] ), BYTE_REVERSE_32 ); ppByte[4] += 32;
            T5 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[5] ), BYTE_REVERSE_32 ); ppByte[5] += 32;
            T6 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[6] ), BYTE_REVERSE_32 ); ppByte[6] += 32;
            T7 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[7] ), BYTE_REVERSE_32 ); ppByte[7] += 32;

            YMM_TRANSPOSE_32( W[r], W[r+1], W[r+2], W[r+3], W[r+4], W[r+5], W[r+6], W[r+7], T0, T1, T2, T3, T4, T5, T6, T7 );
        }

        //
        // Expand the message
        //
        A = W[15];
        B = W[14];
        D = W[0];
        for( r=16; r<64; r+= 2 )
        {
            // Loop invariant: A=W[r-1], B = W[r-2], D = W[r-16]

            //
            // Macro for one word of message expansion.
            // Invariant:
            // on entry: a = W[r-1], b = W[r-2], d = W[r-16]
            // on exit:  W[r] computed, a = W[r-1], b = W[r], c = W[r-15]
            //
            #define EXPAND( a, b, c, d, r ) \
                        c = W[r-15]; \
                        b = _mm256_add_epi32( _mm256_add_epi32( _mm256_add_epi32( d, LSIGMA1YMM( b ) ), W[r-7] ), LSIGMA0YMM( c ) ); \
                        W[r] = b; \

            EXPAND( A, B, C, D, r );
            EXPAND( B, A, D, C, (r+1));

            #undef EXPAND
        }

        A = ha[7];
        B = ha[6];
        C = ha[5];
        D = ha[4];

        for( r=0; r<64; r += 4 )
        {
            //
            // Loop invariant:
            // A, B, C, and D are the a,b,c,d values of the current state.
            // W[r] is the next expanded message word to be processed.
            // W[r-8 .. r-5] contain the current state words h, g, f, e.
            //

            //
            // Macro to compute one round
            //
            #define DO_ROUND( a, b, c, d, t, r ) \
                t = W[r]; \
                t = _mm256_add_epi32( t, CSIGMA1YMM( W[r-5] ) ); \
                t = _mm256_add_epi32( t, W[r-8] ); \
                t = _mm256_add_epi32( t, CHYMM( W[r-5], W[r-6], W[r-7] ) ); \
                t = _mm256_add_epi32( t, _mm256_set1_epi32( SymCryptSha256K[r] )); \
                W[r-4] = _mm256_add_epi32( t, d ); \
                d = _mm256_add_epi32( t, CSIGMA0YMM( a ) ); \
                d = _mm256_add_epi32( d, MAJYMM( c, b, a ) );

            DO_ROUND( A, B, C, D, T, r );
            DO_ROUND( D, A, B, C, T, (r+1) );
            DO_ROUND( C, D, A, B, T, (r+2) );
            DO_ROUND( B, C, D, A, T, (r+3) );
            #undef DO_ROUND
        }

        buf[3] = ha[7] = _mm256_add_epi32( buf[3], A );
        buf[2] = ha[6] = _mm256_add_epi32( buf[2], B );
        buf[1] = ha[5] = _mm256_add_epi32( buf[1], C );
        buf[0] = ha[4] = _mm256_add_epi32( buf[0], D );
        ha[3] = _mm256_add_epi32( ha[3], W[r-5] );
        ha[2] = _mm256_add_epi32( ha[2], W[r-6] );
        ha[1] = _mm256_add_epi32( ha[1], W[r-7] );
        ha[0] = _mm256_add_epi32( ha[0], W[r-8] );

        nBytes -= 64;
    }

    //
    // Copy the chaining state back into the hash structure
    //
    YMM_TRANSPOSE_32( T0, T1, T2, T3, T4, T5, T6, T7, ha[7], ha[6], ha[5], ha[4], ha[3], ha[2], ha[1], ha[0] );
    _mm256_storeu_si256( (__m256i *)&pChain[0]->H[0], T0 );
    _mm256_storeu_si256( (__m256i *)&pChain[1]->H[0], T1 );
    _mm256_storeu_si256( (__m256i *)&pChain[2]->H[0], T2 );
    _mm256_storeu_si256( (__m256i *)&pChain[3]->H[0], T3 );
    _mm256_storeu_si256( (__m256i *)&pChain[4]->H[0], T4 );
    _mm256_storeu_si256( (__m256i *)&pChain[5]->H[0], T5 );
    _mm256_storeu_si256( (__m256i *)&pChain[6]->H[0], T6 );
    _mm256_storeu_si256( (__m256i *)&pChain[7]->H[0], T7 );

    _mm256_zeroupper();

}

#endif // CPU_X86_X64
