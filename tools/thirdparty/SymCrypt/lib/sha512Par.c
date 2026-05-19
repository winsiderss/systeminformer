//
// Sha512Par.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement SHA-512/SHA-384 from FIPS 180-2 in parallel mode
//

#include "precomp.h"

extern SYMCRYPT_ALIGN_AT( 64 ) const UINT64 SymCryptSha512K[81];


//
// Not all CPU architectures support parallel code.
//
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#define SUPPORT_PARALLEL 1

#define MIN_PARALLEL    2
#define MAX_PARALLEL    4

#elif SYMCRYPT_CPU_ARM

#define SUPPORT_PARALLEL 0
//#define MIN_PARALLEL    3
//#define MAX_PARALLEL    3

#else

#define SUPPORT_PARALLEL 0

#endif


//
// ugly hack, there is no generic way to broadcast a 64-bit value between x86 & amd64
//
#if SYMCRYPT_CPU_X86
#define M2x64broadcast_load(_p) _mm_set_epi32( ((UINT32 *)(_p))[1], ((UINT32 *)(_p))[0], ((UINT32 *)(_p))[1], ((UINT32 *)(_p))[0] )
#elif SYMCRYPT_CPU_AMD64
#define M2x64broadcast_load(_p) _mm_shuffle_epi32( _mm_cvtsi64_si128( *(_p) ), 0x44  )
#endif

VOID
SYMCRYPT_CALL
SymCryptParallelSha512AppendBytes_serial(
    _Inout_updates_( nPar )                 PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork,
    _In_range_(1, MAX_PARALLEL)             SIZE_T                                  nPar,
                                            SIZE_T                                  nBytes );

//
// Currently these are the generic implementations in terms of the single hash code.
//

VOID
SYMCRYPT_CALL
SymCryptParallelSha512Init(
    _Out_writes_( nStates ) PSYMCRYPT_SHA512_STATE pStates,
                            SIZE_T                 nStates )
{
    SIZE_T i;

    for( i=0; i<nStates; i++ )
    {
        SymCryptSha512Init( &pStates[i] );
    }
}

VOID
SYMCRYPT_CALL
SymCryptParallelSha384Init(
    _Out_writes_( nStates ) PSYMCRYPT_SHA384_STATE pStates,
                            SIZE_T                 nStates )
{
    SIZE_T i;

    for( i=0; i<nStates; i++ )
    {
        SymCryptSha384Init( &pStates[i] );
    }
}

#if !SUPPORT_PARALLEL
//
// No parallel support on this CPU
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha512Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA512_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    return SymCryptParallelHashProcess_serial( SymCryptParallelSha512Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha384Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA384_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    return SymCryptParallelHashProcess_serial( SymCryptParallelSha384Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
}

#endif


#if SUPPORT_PARALLEL


//
// This function looks at a state and decides what to do.
// If it returns FALSE, then this state is done and no further processing is required.
// If it returns TRUE, the pbData/cbData have to be processed in parallel.
// This function is called again on the same state after the pbData/cbData have been processed.
//
// Internally, it keeps track of the next step to be taken for this state.
// the processingState keeps track of the next action to take.
//


BOOLEAN
SYMCRYPT_CALL
SymCryptParallelSha512Result1(
    _In_    PCSYMCRYPT_PARALLEL_HASH pParHash,
    _Inout_ PSYMCRYPT_COMMON_HASH_STATE pState,
    _Inout_ PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE pScratch,
    _Out_   BOOLEAN *pRes)
{
    UINT32 bytesInBuffer = pState->bytesInBuffer;

    UNREFERENCED_PARAMETER( pParHash );
    //
    // Function is called when a Result is requested from a parallel hash state.
    // Do the first step of the padding.
    //
    pState->buffer[bytesInBuffer++] = 0x80;
    SymCryptWipe( &pState->buffer[bytesInBuffer], SYMCRYPT_SHA512_INPUT_BLOCK_SIZE - bytesInBuffer );

    pScratch->pbData = &pState->buffer[0];
    pScratch->cbData = SYMCRYPT_SHA512_INPUT_BLOCK_SIZE;

    if( bytesInBuffer > SYMCRYPT_SHA512_INPUT_BLOCK_SIZE - 16 )
    {
        // We need 2 blocks for the padding
        pScratch->processingState = STATE_RESULT2;
    } else {
        SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA512_INPUT_BLOCK_SIZE-16], (pState->dataLengthH << 3) + (pState->dataLengthL >> 61)  );
        SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA512_INPUT_BLOCK_SIZE- 8], (pState->dataLengthL << 3) );
        pScratch->processingState = STATE_RESULT_DONE;
    }

    *pRes = TRUE;        // return value from the SetWork function
    return TRUE;        // Return from the SetWork function
}


BOOLEAN
SYMCRYPT_CALL
SymCryptParallelSha512Result2(
    _In_    PCSYMCRYPT_PARALLEL_HASH                pParHash,
    _Inout_ PSYMCRYPT_COMMON_HASH_STATE             pState,
    _Inout_ PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE   pScratch,
    _Out_   BOOLEAN *pRes)
{
    UNREFERENCED_PARAMETER( pParHash );
    //
    // Called for the 2nd block of a long padding
    //
    SymCryptWipe( &pState->buffer[0], SYMCRYPT_SHA512_INPUT_BLOCK_SIZE );
    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA512_INPUT_BLOCK_SIZE-16], (pState->dataLengthH << 3) + (pState->dataLengthL >> 61)  );
    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA512_INPUT_BLOCK_SIZE- 8], (pState->dataLengthL << 3) );
    pScratch->pbData = &pState->buffer[0];
    pScratch->cbData = SYMCRYPT_SHA512_INPUT_BLOCK_SIZE;
    pScratch->processingState = STATE_RESULT_DONE;
    *pRes = TRUE;
    return TRUE;
}

VOID
SYMCRYPT_CALL
SymCryptParallelSha512ResultDone(
    _In_    PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_ PSYMCRYPT_COMMON_HASH_STATE         pState,
    _In_    PCSYMRYPT_PARALLEL_HASH_OPERATION   pOp)
{
    PSYMCRYPT_SHA512_STATE  pSha512State = (PSYMCRYPT_SHA512_STATE) pState;

    UNREFERENCED_PARAMETER( pParHash );

    SYMCRYPT_ASSERT( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_RESULT );
    SYMCRYPT_ASSERT( pOp->cbBuffer == SYMCRYPT_SHA512_RESULT_SIZE );

    SymCryptUint64ToMsbFirst( &pSha512State->chain.H[0], pOp->pbBuffer, 8 );
    SymCryptWipeKnownSize( pSha512State, sizeof( *pSha512State ));
    SymCryptSha512Init( pSha512State );
}

VOID
SYMCRYPT_CALL
SymCryptParallelSha384ResultDone(
    _In_    PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_ PSYMCRYPT_COMMON_HASH_STATE         pState,
    _In_    PCSYMRYPT_PARALLEL_HASH_OPERATION   pOp)
{
    PSYMCRYPT_SHA384_STATE  pSha384State = (PSYMCRYPT_SHA384_STATE) pState;

    UNREFERENCED_PARAMETER( pParHash );

    SYMCRYPT_ASSERT( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_RESULT );
    SYMCRYPT_ASSERT( pOp->cbBuffer == SYMCRYPT_SHA384_RESULT_SIZE );

    SymCryptUint64ToMsbFirst( &pSha384State->chain.H[0], pOp->pbBuffer, 6 );
    SymCryptWipeKnownSize( pSha384State, sizeof( *pSha384State ));
    SymCryptSha384Init( pSha384State );
}


C_ASSERT( (SYMCRYPT_SIMD_ELEMENT_SIZE & (SYMCRYPT_SIMD_ELEMENT_SIZE - 1 )) == 0 );  // check that it is a power of 2



SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha512Sha384Process(
    _In_                            PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_                         PVOID                               pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 maxParallel;

#if SYMCRYPT_CPU_AMD64 | SYMCRYPT_CPU_X86
    SYMCRYPT_EXTENDED_SAVE_DATA SaveState;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_AVX2 | SYMCRYPT_CPU_FEATURE_SSSE3 ) && SymCryptSaveYmm( &SaveState ) == SYMCRYPT_NO_ERROR )
    {
        maxParallel = 4;
        scError = SymCryptParallelHashProcess(  pParHash,
                                                pStates,
                                                nStates,
                                                pOperations,
                                                nOperations,
                                                pbScratch,
                                                cbScratch,
                                                maxParallel );

        SymCryptRestoreYmm( &SaveState );
    } else if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_SSSE3 ) && SymCryptSaveXmm( &SaveState ) == SYMCRYPT_NO_ERROR )
    {
        maxParallel = 2;
        scError = SymCryptParallelHashProcess(  pParHash,
                                                pStates,
                                                nStates,
                                                pOperations,
                                                nOperations,
                                                pbScratch,
                                                cbScratch,
                                                maxParallel );
        SymCryptRestoreXmm( &SaveState );
    } else {
        scError = SymCryptParallelHashProcess_serial( pParHash, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
    }

#elif SYMCRYPT_CPU_ARM
    maxParallel = MAX_PARALLEL;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON ) )
    {
        scError = SymCryptParallelHashProcess(  pParHash,
                                                pStates,
                                                nStates,
                                                pOperations,
                                                nOperations,
                                                pbScratch,
                                                cbScratch,
                                                maxParallel );
    } else {
        scError = SymCryptParallelHashProcess_serial( pParHash, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
    }
#else
    scError = SymCryptParallelHashProcess_serial( pParHash, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
#endif
    return scError;
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha512Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA512_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    return SymCryptParallelSha512Sha384Process( SymCryptParallelSha512Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
}


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha384Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA384_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    return SymCryptParallelSha512Sha384Process( SymCryptParallelSha384Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
}


#if  SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
//
// Code that uses the XMM registers.
//

#define MAJXMM( x, y, z ) _mm_or_si128( _mm_and_si128( _mm_or_si128( z, y ), x ), _mm_and_si128( z, y ))
#define CHXMM( x, y, z )  _mm_xor_si128( _mm_and_si128( _mm_xor_si128( z, y ), x ), z )

#define CSIGMA0XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi64(x,36)  , _mm_srli_epi64(x, 28) ),\
        _mm_slli_epi64(x,30) ), _mm_srli_epi64(x, 34) ),\
        _mm_slli_epi64(x,25) ), _mm_srli_epi64(x, 39) )
#define CSIGMA1XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi64(x,50)  , _mm_srli_epi64(x, 14) ),\
        _mm_slli_epi64(x,46) ), _mm_srli_epi64(x, 18) ),\
        _mm_slli_epi64(x,23) ), _mm_srli_epi64(x, 41) )
#define LSIGMA0XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi64(x,63)  , _mm_srli_epi64(x,  1) ),\
        _mm_slli_epi64(x,56) ), _mm_srli_epi64(x,  8) ),\
        _mm_srli_epi64(x, 7) )
#define LSIGMA1XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi64(x,45)  , _mm_srli_epi64(x, 19) ),\
        _mm_slli_epi64(x, 3) ), _mm_srli_epi64(x, 61) ),\
        _mm_srli_epi64(x,6) )

#define XMM_TRANSPOSE_64( _R0, _R1, _S0, _S1 ) \
    {\
        _R0 = _mm_unpacklo_epi64( _S0, _S1 );\
        _R1 = _mm_unpackhi_epi64( _S0, _S1 );\
    }

VOID
SYMCRYPT_CALL
SymCryptParallelSha512AppendBlocks_xmm(
    _Inout_updates_( 2 )                                PSYMCRYPT_SHA512_CHAINING_STATE   * pChain,
    _Inout_updates_( 2 )                                PCBYTE                            * ppByte,
                                                        SIZE_T                              nBytes,
    _Out_writes_( PAR_SCRATCH_ELEMENTS_512 )            __m128i                           * pScratch )
{
    __m128i * buf = pScratch;       // chaining state concatenated with the expanded input block
    __m128i * W = &buf[4 + 8];      // W are the 64 words of the expanded input
    __m128i * ha = &buf[4];         // initial state words, in order h, g, ..., b, a
    __m128i A, B, C, D, T;
    __m128i T0, T1;
    const __m128i BYTE_REVERSE_64 = _mm_set_epi8( 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 );
    int r;


    //
    // The chaining state can be unaligned on x86, so we use unaligned loads
    //

    T0 = _mm_loadu_si128( (__m128i *)&pChain[0]->H[0] );
    T1 = _mm_loadu_si128( (__m128i *)&pChain[1]->H[0] );

    XMM_TRANSPOSE_64( ha[7], ha[6], T0, T1 );

    T0 = _mm_loadu_si128( (__m128i *)&pChain[0]->H[2] );
    T1 = _mm_loadu_si128( (__m128i *)&pChain[1]->H[2] );
    XMM_TRANSPOSE_64( ha[5], ha[4], T0, T1 );

    T0 = _mm_loadu_si128( (__m128i *)&pChain[0]->H[4] );
    T1 = _mm_loadu_si128( (__m128i *)&pChain[1]->H[4] );
    XMM_TRANSPOSE_64( ha[3], ha[2], T0, T1 );

    T0 = _mm_loadu_si128( (__m128i *)&pChain[0]->H[6] );
    T1 = _mm_loadu_si128( (__m128i *)&pChain[1]->H[6] );
    XMM_TRANSPOSE_64( ha[1], ha[0], T0, T1 );

    buf[0] = ha[4];
    buf[1] = ha[5];
    buf[2] = ha[6];
    buf[3] = ha[7];

    while( nBytes >= 128 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r += 2 )
        {
            T0 = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *) ppByte[0] ), BYTE_REVERSE_64 ); ppByte[0] += 16;
            T1 = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *) ppByte[1] ), BYTE_REVERSE_64 ); ppByte[1] += 16;

            XMM_TRANSPOSE_64( W[r], W[r+1], T0, T1 );
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
                        b = _mm_add_epi64( _mm_add_epi64( _mm_add_epi64( d, LSIGMA1XMM( b ) ), W[r-7] ), LSIGMA0XMM( c ) ); \
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
                t = _mm_add_epi64( t, CSIGMA1XMM( W[r-5] ) ); \
                t = _mm_add_epi64( t, W[r-8] ); \
                t = _mm_add_epi64( t, CHXMM( W[r-5], W[r-6], W[r-7] ) ); \
                t = _mm_add_epi64( t, M2x64broadcast_load( &SymCryptSha512K[r] )); \
                W[r-4] = _mm_add_epi64( t, d ); \
                d = _mm_add_epi64( t, CSIGMA0XMM( a ) ); \
                d = _mm_add_epi64( d, MAJXMM( c, b, a ) );

            DO_ROUND( A, B, C, D, T, r );
            DO_ROUND( D, A, B, C, T, (r+1) );
            DO_ROUND( C, D, A, B, T, (r+2) );
            DO_ROUND( B, C, D, A, T, (r+3) );
            #undef DO_ROUND
        }

        buf[3] = ha[7] = _mm_add_epi64( buf[3], A );
        buf[2] = ha[6] = _mm_add_epi64( buf[2], B );
        buf[1] = ha[5] = _mm_add_epi64( buf[1], C );
        buf[0] = ha[4] = _mm_add_epi64( buf[0], D );
        ha[3] = _mm_add_epi64( ha[3], W[r-5] );
        ha[2] = _mm_add_epi64( ha[2], W[r-6] );
        ha[1] = _mm_add_epi64( ha[1], W[r-7] );
        ha[0] = _mm_add_epi64( ha[0], W[r-8] );

        nBytes -= 128;
    }


    XMM_TRANSPOSE_64( T0, T1, ha[7], ha[6] );
    _mm_storeu_si128( (__m128i *)&pChain[0]->H[0], T0 );
    _mm_storeu_si128( (__m128i *)&pChain[1]->H[0], T1 );

    XMM_TRANSPOSE_64( T0, T1, ha[5], ha[4] );
    _mm_storeu_si128( (__m128i *)&pChain[0]->H[2], T0 );
    _mm_storeu_si128( (__m128i *)&pChain[1]->H[2], T1 );

    XMM_TRANSPOSE_64( T0, T1, ha[3], ha[2] );
    _mm_storeu_si128( (__m128i *)&pChain[0]->H[4], T0 );
    _mm_storeu_si128( (__m128i *)&pChain[1]->H[4], T1 );

    XMM_TRANSPOSE_64( T0, T1, ha[1], ha[0] );
    _mm_storeu_si128( (__m128i *)&pChain[0]->H[6], T0 );
    _mm_storeu_si128( (__m128i *)&pChain[1]->H[6], T1 );

}

#endif // CPU_X86_X64

#if  SYMCRYPT_CPU_ARM


#endif // CPU_ARM



VOID
SYMCRYPT_CALL
SymCryptParallelSha512AppendBytes_serial(
    _Inout_updates_( nPar )                 PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork,
    _In_range_(1, MAX_PARALLEL)             SIZE_T                                  nPar,
                                            SIZE_T                                  nBytes )
{
    SIZE_T i;
    SIZE_T tmp;

    SYMCRYPT_ASSERT( nBytes % SYMCRYPT_SHA512_INPUT_BLOCK_SIZE == 0 );
    SYMCRYPT_ASSERT( nPar >= 1 && nPar <= MAX_PARALLEL );

    for( i=0; i < nPar; i++ )
    {
        SYMCRYPT_ASSERT( pWork[i]->cbData >= nBytes );
#if SYMCRYPT_CPU_X86
        //
        // On X86 the Sha512 append blocks function saves the XMM registers again, which is not allowed at DISPATCH level.
        // We call the internal function that assumes the XMM registers are already saved.
        // This function is only called when we are doing parallel hashing, which means that at a minimum we have SSSE3 and
        // the XMM registers are saved.
        //
        SymCryptSha512AppendBlocks_xmm( & ((PSYMCRYPT_SHA512_STATE)(pWork[i]->hashState))->chain, pWork[i]->pbData, nBytes, &tmp );
#else
        SymCryptSha512AppendBlocks( & ((PSYMCRYPT_SHA512_STATE)(pWork[i]->hashState))->chain, pWork[i]->pbData, nBytes, &tmp );
#endif
        pWork[i]->pbData += nBytes;
        pWork[i]->cbData -= nBytes;
    }
    return;
}

VOID
SYMCRYPT_CALL
SymCryptParallelSha512Append(
    _Inout_updates_( nPar )                 PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork,
    _In_range_(1, MAX_PARALLEL)             SIZE_T                                  nPar,
                                            SIZE_T                                  nBytes,
    _Inout_updates_( SYMCRYPT_SIMD_ELEMENT_SIZE * PAR_SCRATCH_ELEMENTS_512 )
                                            PBYTE                                   pbSimdScratch,
                                            SIZE_T                                  cbSimdScratch )
{
    PSYMCRYPT_SHA512_CHAINING_STATE apChain[MAX_PARALLEL];
    PCBYTE                          apData[MAX_PARALLEL];
    SIZE_T                          i;
    UINT32                          maxParallel;

    UNREFERENCED_PARAMETER( cbSimdScratch );        // not referenced on FRE builds
    SYMCRYPT_ASSERT( cbSimdScratch >= PAR_SCRATCH_ELEMENTS_512 * SYMCRYPT_SIMD_ELEMENT_SIZE );
    SYMCRYPT_ASSERT( ((SIZE_T)pbSimdScratch & (SYMCRYPT_SIMD_ELEMENT_SIZE - 1)) == 0 );

    //
    // Compute maxParallel; this is 2 if nPar <= 2, and 4 if nPar = 3,4.
    // This is how many parameter sets we have to set up.
    //
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

    maxParallel = (nPar + 1) & ~1;
    SYMCRYPT_ASSERT( maxParallel == 2 || (maxParallel == 4 && SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_AVX2 )) );

#elif SYMCRYPT_CPU_ARM

    maxParallel = 2;

#endif

    SYMCRYPT_ASSERT( nPar >= 1 && nPar <= maxParallel );

    if( nPar < MIN_PARALLEL )
    {
        SymCryptParallelSha512AppendBytes_serial( pWork, nPar, nBytes );

        // Done with this function.
        goto cleanup;
    }

    //
    // Our parallel code expects exactly 2 or 4 parallel computations.
    // We simply duplicate the first one if we get fewer parallel ones.
    // That means we write the result multiple times, but it saves a lot of
    // extra if()s in the main codeline.
    //

    i = 0;
    while( i < nPar )
    {
            SYMCRYPT_ASSERT( pWork[i]->cbData >= nBytes );
            apChain[i] =  & ((PSYMCRYPT_SHA512_STATE)(pWork[i]->hashState))->chain;
            apData[i] = pWork[i]->pbData;
            pWork[i]->pbData += nBytes;
            pWork[i]->cbData -= nBytes;
            i++;
    }

    while( i < maxParallel )
    {
            apChain[i] = apChain[0];
            apData[i] = apData[0];
            i++;
    }

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    if( maxParallel == 4 )
    {
        SymCryptParallelSha512AppendBlocks_ymm(  &apChain[0], &apData[0], nBytes, (PBYTE)((__m256i *)pbSimdScratch) );
    } else {
        SymCryptParallelSha512AppendBlocks_xmm(  &apChain[0], &apData[0], nBytes, (__m128i *)pbSimdScratch );
    }
#elif SYMCRYPT_CPU_ARM
    UNREFERENCED_PARAMETER( pbSimdScratch );
    //SymCryptParallelSha512AppendBlocks_neon( &apChain[0], &apData[0], nBytes, (__n128 *) pbSimdScratch );
#else
#error Unknown CPU
#endif

cleanup:
    ;// no cleanup at this moment.
}


#endif // SUPPORT_PARALLEL

#if SUPPORT_PARALLEL

const SYMCRYPT_PARALLEL_HASH SymCryptParallelSha512Algorithm_default = {
    &SymCryptSha512Algorithm_default,
    PAR_SCRATCH_ELEMENTS_512 * SYMCRYPT_SIMD_ELEMENT_SIZE,
    &SymCryptParallelSha512Result1,
    &SymCryptParallelSha512Result2,
    &SymCryptParallelSha512ResultDone,
    &SymCryptParallelSha512Append,
};

const SYMCRYPT_PARALLEL_HASH SymCryptParallelSha384Algorithm_default = {
    &SymCryptSha384Algorithm_default,
    PAR_SCRATCH_ELEMENTS_512 * SYMCRYPT_SIMD_ELEMENT_SIZE,
    &SymCryptParallelSha512Result1,
    &SymCryptParallelSha512Result2,
    &SymCryptParallelSha384ResultDone,
    &SymCryptParallelSha512Append,
};

#else

//
// For platforms that do not have a parallel hash implementation
// we use this structure to provide the necessary data to the _serial
// implementation of the function.
//
const SYMCRYPT_PARALLEL_HASH SymCryptParallelSha512Algorithm_default = {
    &SymCryptSha512Algorithm_default,
    PAR_SCRATCH_ELEMENTS_512 * SYMCRYPT_SIMD_ELEMENT_SIZE,
    NULL,
    NULL,
    NULL,
    NULL,
};

const SYMCRYPT_PARALLEL_HASH SymCryptParallelSha384Algorithm_default = {
    &SymCryptSha384Algorithm_default,
    PAR_SCRATCH_ELEMENTS_512 * SYMCRYPT_SIMD_ELEMENT_SIZE,
    NULL,
    NULL,
    NULL,
    NULL,
};

#endif

const PCSYMCRYPT_PARALLEL_HASH SymCryptParallelSha384Algorithm = &SymCryptParallelSha384Algorithm_default;
const PCSYMCRYPT_PARALLEL_HASH SymCryptParallelSha512Algorithm = &SymCryptParallelSha512Algorithm_default;


#define N_SELFTEST_STATES   3      // Just enough to trigger YMM usage

VOID
SYMCRYPT_CALL
SymCryptParallelSha384Selftest(void)
{
    SYMCRYPT_ERROR                      scError;
    SYMCRYPT_SHA384_STATE               states[N_SELFTEST_STATES];
    BYTE                                result[N_SELFTEST_STATES][SYMCRYPT_SHA384_RESULT_SIZE];
    SYMCRYPT_PARALLEL_HASH_OPERATION    op[2*N_SELFTEST_STATES];
    BYTE                                scratch[SYMCRYPT_PARALLEL_SHA384_FIXED_SCRATCH + N_SELFTEST_STATES * SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH];
    int                                 i;

    SymCryptParallelSha384Init( &states[0], N_SELFTEST_STATES );

    for( i=0; i<N_SELFTEST_STATES; i++ )
    {
        op[2*i    ].iHash = i;
        op[2*i    ].hashOperation = SYMCRYPT_HASH_OPERATION_APPEND;
        op[2*i    ].pbBuffer = (PBYTE) SymCryptTestMsg3;
        op[2*i    ].cbBuffer = sizeof(SymCryptTestMsg3);
        op[2*i + 1].iHash = i;
        op[2*i + 1].hashOperation = SYMCRYPT_HASH_OPERATION_RESULT;
        op[2*i + 1].pbBuffer = &result[i][0];
        op[2*i + 1].cbBuffer = SYMCRYPT_SHA384_RESULT_SIZE;
    }

    scError = SymCryptParallelSha384Process( &states[0], N_SELFTEST_STATES, op, 2*N_SELFTEST_STATES, scratch, sizeof( scratch ) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'PS38' );
    }

    for( i=0; i<N_SELFTEST_STATES; i++ )
    {
        SymCryptInjectError( &result[i][0], SYMCRYPT_SHA384_RESULT_SIZE );

        if( memcmp( &result[i][0], SymCryptSha384KATAnswer, SYMCRYPT_SHA384_RESULT_SIZE ) != 0 ) {
            SymCryptFatal( 'PS38' );
        }
    }
}

VOID
SYMCRYPT_CALL
SymCryptParallelSha512Selftest(void)
{
    SYMCRYPT_ERROR                      scError;
    SYMCRYPT_SHA512_STATE               states[N_SELFTEST_STATES];
    BYTE                                result[N_SELFTEST_STATES][SYMCRYPT_SHA512_RESULT_SIZE];
    SYMCRYPT_PARALLEL_HASH_OPERATION    op[2*N_SELFTEST_STATES];
    BYTE                                scratch[SYMCRYPT_PARALLEL_SHA512_FIXED_SCRATCH + N_SELFTEST_STATES * SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH];
    int                                 i;

    SymCryptParallelSha512Init( &states[0], N_SELFTEST_STATES );

    for( i=0; i<N_SELFTEST_STATES; i++ )
    {
        op[2*i    ].iHash = i;
        op[2*i    ].hashOperation = SYMCRYPT_HASH_OPERATION_APPEND;
        op[2*i    ].pbBuffer = (PBYTE) SymCryptTestMsg3;
        op[2*i    ].cbBuffer = sizeof(SymCryptTestMsg3);
        op[2*i + 1].iHash = i;
        op[2*i + 1].hashOperation = SYMCRYPT_HASH_OPERATION_RESULT;
        op[2*i + 1].pbBuffer = &result[i][0];
        op[2*i + 1].cbBuffer = SYMCRYPT_SHA512_RESULT_SIZE;
    }

    scError = SymCryptParallelSha512Process( &states[0], N_SELFTEST_STATES, op, 2*N_SELFTEST_STATES, scratch, sizeof( scratch ) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'PS51' );
    }

    for( i=0; i<N_SELFTEST_STATES; i++ )
    {
        SymCryptInjectError( &result[i][0], SYMCRYPT_SHA512_RESULT_SIZE );

        if( memcmp( &result[i][0], SymCryptSha512KATAnswer, SYMCRYPT_SHA512_RESULT_SIZE ) != 0 ) {
            SymCryptFatal( 'PS51' );
        }
    }
}

