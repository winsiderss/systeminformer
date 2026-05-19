//
// Sha512Par-ymm.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// All YMM code for SHA-512/SHA-384 Parallel operations
// Requires compiler support for avx2
//

#include "precomp.h"

extern SYMCRYPT_ALIGN_AT( 64 ) const UINT64 SymCryptSha512K[81];


#if  SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
//
// Code that uses the YMM registers.
//

//
// ugly hack, there is no generic way to broadcast a 64-bit value between x86 & amd64
//
#if SYMCRYPT_CPU_X86
#define M4x64broadcast_load(_p) _mm256_set_epi32( ((UINT32 *)(_p))[1], ((UINT32 *)(_p))[0], ((UINT32 *)(_p))[1], ((UINT32 *)(_p))[0],  ((UINT32 *)(_p))[1], ((UINT32 *)(_p))[0], ((UINT32 *)(_p))[1], ((UINT32 *)(_p))[0] )
#elif SYMCRYPT_CPU_AMD64
#define M4x64broadcast_load(_p) _mm256_set1_epi64x( *(_p) )
#endif

#define MAJYMM( x, y, z ) _mm256_or_si256( _mm256_and_si256( _mm256_or_si256( z, y ), x ), _mm256_and_si256( z, y ))
#define CHYMM( x, y, z )  _mm256_xor_si256( _mm256_and_si256( _mm256_xor_si256( z, y ), x ), z )

#define CSIGMA0YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi64(x,36)  , _mm256_srli_epi64(x, 28) ),\
        _mm256_slli_epi64(x,30) ), _mm256_srli_epi64(x, 34) ),\
        _mm256_slli_epi64(x,25) ), _mm256_srli_epi64(x, 39) )
#define CSIGMA1YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi64(x,50)  , _mm256_srli_epi64(x, 14) ),\
        _mm256_slli_epi64(x,46) ), _mm256_srli_epi64(x, 18) ),\
        _mm256_slli_epi64(x,23) ), _mm256_srli_epi64(x, 41) )
#define LSIGMA0YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi64(x,63)  , _mm256_srli_epi64(x,  1) ),\
        _mm256_slli_epi64(x,56) ), _mm256_srli_epi64(x,  8) ),\
        _mm256_srli_epi64(x, 7) )
#define LSIGMA1YMM( x ) \
    _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( _mm256_xor_si256( \
        _mm256_slli_epi64(x,45)  , _mm256_srli_epi64(x, 19) ),\
        _mm256_slli_epi64(x, 3) ), _mm256_srli_epi64(x, 61) ),\
        _mm256_srli_epi64(x,6) )

//
// S0:  00  01  02  03
// S1:  10  11  12  13
// S2:  20  21  22  23
// S3:  30  31  32  33
//
// T0:  00  10  02  12      unpacklo_epi64( S0, S1 )    note: unpacklo in AVX works in parallel on 2 128-bit values
// T1:  01  11  03  13      unpackhi_epi64( S0, S1 )
// T2:  20  30  22  32
// T3:  21  31  23  33
//
// R0:  00  10  20  30
// R1:  01  11  21  31
// R2:  02  12  22  32
// R3:  03  13  23  33


#define YMM_TRANSPOSE_64( _R0, _R1, _R2, _R3, _S0, _S1, _S2, _S3 ) \
    {\
        __m256i _T0, _T1, _T2, _T3;\
        _T0 = _mm256_unpacklo_epi64( _S0, _S1 );  _T1 = _mm256_unpackhi_epi64( _S0, _S1 );\
        _T2 = _mm256_unpacklo_epi64( _S2, _S3 );  _T3 = _mm256_unpackhi_epi64( _S2, _S3 );\
    \
        _R0 = _mm256_permute2x128_si256( _T0, _T2, 0x20 );  _R1 = _mm256_permute2x128_si256( _T1, _T3, 0x20);\
        _R2 = _mm256_permute2x128_si256( _T0, _T2, 0x31 );  _R3 = _mm256_permute2x128_si256( _T1, _T3, 0x31);\
    }

VOID
SYMCRYPT_CALL
SymCryptParallelSha512AppendBlocks_ymm(
    _Inout_updates_( 4 )                                PSYMCRYPT_SHA512_CHAINING_STATE   * pChain,
    _Inout_updates_( 4 )                                PCBYTE                            * ppByte,
                                                        SIZE_T                              nBytes,
    _Out_writes_( PAR_SCRATCH_ELEMENTS_512 * 32 )       PBYTE                               pScratch )
{
    __m256i * buf = (__m256i *)pScratch;    // chaining state concatenated with the expanded input block
    __m256i * W = &buf[4 + 8];              // W are the 64 words of the expanded input
    __m256i * ha = &buf[4];                 // initial state words, in order h, g, ..., b, a
    __m256i A, B, C, D, T;
    __m256i T0, T1, T2, T3;
    int r;
    __m256i BYTE_REVERSE_64;

    _mm256_zeroupper();
    BYTE_REVERSE_64 = _mm256_set_epi8( 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 );

    //
    // The chaining state can be unaligned on x86, so we use unaligned loads
    //

    T0 = _mm256_loadu_si256( (__m256i *)&pChain[0]->H[0] );
    T1 = _mm256_loadu_si256( (__m256i *)&pChain[1]->H[0] );
    T2 = _mm256_loadu_si256( (__m256i *)&pChain[2]->H[0] );
    T3 = _mm256_loadu_si256( (__m256i *)&pChain[3]->H[0] );

    YMM_TRANSPOSE_64( ha[7], ha[6], ha[5], ha[4], T0, T1, T2, T3 );

    T0 = _mm256_loadu_si256( (__m256i *)&pChain[0]->H[4] );
    T1 = _mm256_loadu_si256( (__m256i *)&pChain[1]->H[4] );
    T2 = _mm256_loadu_si256( (__m256i *)&pChain[2]->H[4] );
    T3 = _mm256_loadu_si256( (__m256i *)&pChain[3]->H[4] );

    YMM_TRANSPOSE_64( ha[3], ha[2], ha[1], ha[0], T0, T1, T2, T3 );

    buf[0] = ha[4];
    buf[1] = ha[5];
    buf[2] = ha[6];
    buf[3] = ha[7];

    while( nBytes >= 128 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r += 4 )
        {
            T0 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[0] ), BYTE_REVERSE_64 ); ppByte[0] += 32;
            T1 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[1] ), BYTE_REVERSE_64 ); ppByte[1] += 32;
            T2 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[2] ), BYTE_REVERSE_64 ); ppByte[2] += 32;
            T3 = _mm256_shuffle_epi8( _mm256_loadu_si256( (__m256i *) ppByte[3] ), BYTE_REVERSE_64 ); ppByte[3] += 32;

            YMM_TRANSPOSE_64( W[r], W[r+1], W[r+2], W[r+3], T0, T1, T2, T3 );
        }

        //
        // Expand the message
        //
        A = W[15];
        B = W[14];
        D = W[0];
        for( r=16; r<80; r+= 2 )
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
                        b = _mm256_add_epi64( _mm256_add_epi64( _mm256_add_epi64( d, LSIGMA1YMM( b ) ), W[r-7] ), LSIGMA0YMM( c ) ); \
                        W[r] = b; \

            EXPAND( A, B, C, D, r );
            EXPAND( B, A, D, C, (r+1));

            #undef EXPAND
        }

        A = ha[7];
        B = ha[6];
        C = ha[5];
        D = ha[4];

        for( r=0; r<80; r += 4 )
        {
            //
            // Loop invariant:
            // A, B, C, and D are the a,b,c,d values of the current state.
            // W[r] is the next expanded message word to be processed.
            // W[r-8 .. r-5] contain the current state words h, g, f, e.
            //

            //
            // Macro to compute one round
            // The shuffle is to duplicate the 64-bit value to both lanes.
            // Each half of the immediate is 0100. See the documentation of the
            // PSHUFD instruction.
            //
            #define DO_ROUND( a, b, c, d, t, r ) \
                t = W[r]; \
                t = _mm256_add_epi64( t, CSIGMA1YMM( W[r-5] ) ); \
                t = _mm256_add_epi64( t, W[r-8] ); \
                t = _mm256_add_epi64( t, CHYMM( W[r-5], W[r-6], W[r-7] ) ); \
                t = _mm256_add_epi64( t, M4x64broadcast_load( &SymCryptSha512K[r] )); \
                W[r-4] = _mm256_add_epi64( t, d ); \
                d = _mm256_add_epi64( t, CSIGMA0YMM( a ) ); \
                d = _mm256_add_epi64( d, MAJYMM( c, b, a ) );

            DO_ROUND( A, B, C, D, T, r );
            DO_ROUND( D, A, B, C, T, (r+1) );
            DO_ROUND( C, D, A, B, T, (r+2) );
            DO_ROUND( B, C, D, A, T, (r+3) );
            #undef DO_ROUND
        }

        buf[3] = ha[7] = _mm256_add_epi64( buf[3], A );
        buf[2] = ha[6] = _mm256_add_epi64( buf[2], B );
        buf[1] = ha[5] = _mm256_add_epi64( buf[1], C );
        buf[0] = ha[4] = _mm256_add_epi64( buf[0], D );
        ha[3] = _mm256_add_epi64( ha[3], W[r-5] );
        ha[2] = _mm256_add_epi64( ha[2], W[r-6] );
        ha[1] = _mm256_add_epi64( ha[1], W[r-7] );
        ha[0] = _mm256_add_epi64( ha[0], W[r-8] );

        nBytes -= 128;
    }

    YMM_TRANSPOSE_64( T0, T1, T2, T3, ha[7], ha[6], ha[5], ha[4] );
    _mm256_storeu_si256( (__m256i *)&pChain[0]->H[0], T0 );
    _mm256_storeu_si256( (__m256i *)&pChain[1]->H[0], T1 );
    _mm256_storeu_si256( (__m256i *)&pChain[2]->H[0], T2 );
    _mm256_storeu_si256( (__m256i *)&pChain[3]->H[0], T3 );

    YMM_TRANSPOSE_64( T0, T1, T2, T3, ha[3], ha[2], ha[1], ha[0] );
    _mm256_storeu_si256( (__m256i *)&pChain[0]->H[4], T0 );
    _mm256_storeu_si256( (__m256i *)&pChain[1]->H[4], T1 );
    _mm256_storeu_si256( (__m256i *)&pChain[2]->H[4], T2 );
    _mm256_storeu_si256( (__m256i *)&pChain[3]->H[4], T3 );

    _mm256_zeroupper();
}

#endif // CPU_X86_X64