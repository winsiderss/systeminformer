//
// Sha256Par.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This module contains the routines to implement SHA2-256 from FIPS 180-2 in parallel mode
//

#include "precomp.h"

extern SYMCRYPT_ALIGN_AT( 256 ) const UINT32 SymCryptSha256K[64];


//
// Not all CPU architectures support parallel code.
//
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

#define SUPPORT_PARALLEL 1
#define MIN_PARALLEL    2
#define MAX_PARALLEL    8

#elif SYMCRYPT_CPU_ARM

#define SUPPORT_PARALLEL 1
#define MIN_PARALLEL    3
#define MAX_PARALLEL    4

#else

#define SUPPORT_PARALLEL 0

#endif


VOID
SYMCRYPT_CALL
SymCryptParallelSha256AppendBytes_serial(
    _Inout_updates_( nPar )                 PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork,
    _In_range_(1, MAX_PARALLEL)             SIZE_T                                  nPar,
                                            SIZE_T                                  nBytes );

//
// Currently these are the generic implementations in terms of the single hash code.
//

VOID
SYMCRYPT_CALL
SymCryptParallelSha256Init(
    _Out_writes_( nStates ) PSYMCRYPT_SHA256_STATE pStates,
                            SIZE_T                 nStates )
{
    SIZE_T i;

    for( i=0; i<nStates; i++ )
    {
        SymCryptSha256Init( &pStates[i] );
    }
}

#if !SUPPORT_PARALLEL
//
// No parallel support on this CPU
//

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha256Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA256_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    return SymCryptParallelHashProcess_serial( SymCryptParallelSha256Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
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
SymCryptParallelSha256Result1(
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
    SymCryptWipe( &pState->buffer[bytesInBuffer], SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - bytesInBuffer );

    pScratch->pbData = &pState->buffer[0];
    pScratch->cbData = SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;

    if( bytesInBuffer > SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8 )
    {
        // We need 2 blocks for the padding
        pScratch->processingState = STATE_RESULT2;
    } else {
        SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8], pState->dataLengthL * 8 );
        pScratch->processingState = STATE_RESULT_DONE;
    }

    *pRes = TRUE;        // return value from the SetWork function
    return TRUE;        // Return from the SetWork function
}


BOOLEAN
SYMCRYPT_CALL
SymCryptParallelSha256Result2(
    _In_    PCSYMCRYPT_PARALLEL_HASH                pParHash,
    _Inout_ PSYMCRYPT_COMMON_HASH_STATE             pState,
    _Inout_ PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE   pScratch,
    _Out_   BOOLEAN *pRes)
{
    UNREFERENCED_PARAMETER( pParHash );
    //
    // Called for the 2nd block of a long padding
    //
    SymCryptWipe( &pState->buffer[0], SYMCRYPT_SHA256_INPUT_BLOCK_SIZE );
    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8], pState->dataLengthL * 8 );
    pScratch->pbData = &pState->buffer[0];
    pScratch->cbData = SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;
    pScratch->processingState = STATE_RESULT_DONE;
    *pRes = TRUE;
    return TRUE;
}

VOID
SYMCRYPT_CALL
SymCryptParallelSha256ResultDone(
    _In_    PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_ PSYMCRYPT_COMMON_HASH_STATE         pState,
    _In_    PCSYMRYPT_PARALLEL_HASH_OPERATION   pOp)
{
    PSYMCRYPT_SHA256_STATE  pSha256State = (PSYMCRYPT_SHA256_STATE) pState;

    UNREFERENCED_PARAMETER( pParHash );

    SYMCRYPT_ASSERT( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_RESULT );
    SYMCRYPT_ASSERT( pOp->cbBuffer == SYMCRYPT_SHA256_RESULT_SIZE );

    SymCryptUint32ToMsbFirst( &pSha256State->chain.H[0], pOp->pbBuffer, 8 );
    SymCryptWipeKnownSize( pSha256State, sizeof( *pSha256State ));
    SymCryptSha256Init( pSha256State );
}


#if 0

BOOL
SYMCRYPT_CALL
SymCryptParallelSha256SetNextWork( PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE pScratch )
{
    PSYMCRYPT_SHA256_STATE              pState;
    PCSYMRYPT_PARALLEL_HASH_OPERATION   pOp;
    UINT32                              bytesInBuffer;
    UINT32                              todo;

    // Retrieve the state we will operate on.
    pState = (PSYMCRYPT_SHA256_STATE) pScratch->hashState;

    //
    // This is a state machine where some states have to iterate
    // The loop allows them to use 'continue' for that.
    //
#pragma warning( suppress: 4127 )       // conditional expression is constant
    while( TRUE )
    {
        //
        // At this point, the processing state, pbData/cbData, and next pointer define what needs to be done.
        // STATE_NEXT: cbData == 0 and we have to process the remaining operations.
        // STATE_DATA_START: We are working on the next operation; the first BytesAlreadyProcessed have been hashed,
        //                      and the hash state has an empty buffer.
        // STATE_DATA_END: We are working on the next operation (an append), and pbData/cbData have whatever partial block remains
        //                  after all the whole blocks have been processed.
        // STATE_PAD2:      We are working on the next operation (a result), and have processed the first half of a 2-block padding.
        // STATE_RESULT:    We are working on the next operation (a result), and have processed all the padding.
        //
        // The pState->dataLength is updated whenever we copy bytes from the append into the state's buffer, or when
        //      we return TRUE and process bulk data.
        //
        pOp = pScratch->next;
        switch( pScratch->processingState )
        {
        case STATE_NEXT:

            if( pOp == NULL )
            {
                return FALSE;
            }

            bytesInBuffer = pState->bytesInBuffer;

            // SYMCRYPT_ASSERT( pOp->cbBuffer < ((SIZE_T)-1)/2 );   // used during testing

            if( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_APPEND )
            {
                pState->dataLengthL += pOp->cbBuffer;
                if( bytesInBuffer > 0 )
                {
                    todo = (UINT32) SYMCRYPT_MIN( SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - bytesInBuffer, pOp->cbBuffer );
                    memcpy( &pState->buffer[bytesInBuffer], pOp->pbBuffer, todo );
                    pState->bytesInBuffer += todo;
                    if( pState->bytesInBuffer == SYMCRYPT_SHA256_INPUT_BLOCK_SIZE )
                    {
                        //
                        // We filled the buffer; set it for processing.
                        // Remember the # bytes we did and set the next state to process the rest of the request.
                        //
                        pScratch->pbData = &pState->buffer[0];
                        pScratch->cbData = sizeof( pState->buffer );
                        pState->bytesInBuffer = 0;
                        if( todo == pOp->cbBuffer )
                        {
                            //
                            // We finished the request after the pbData processing
                            //
                            pScratch->next = pOp->next;
                            // pScratch->processingState = STATE_NEXT       // already has that value
                        } else {
                            pScratch->processingState = STATE_DATA_START;
                            SYMCRYPT_ASSERT( todo <= 0xff );
                            pScratch->bytesAlreadyProcessed = (BYTE) todo;
                        }
                        //
                        // We process the buffer here, no need to update the dataLength
                        //
                        return TRUE;
                    } else {
                        //
                        // We finished the operation; skip to the next one.
                        //
                        pScratch->next = pOp->next;
                        // pScratch->processingState = STATE_NEXT       // already has that value
                        continue;
                    }
                } else {
                    //
                    // Buffer is empty; process the bulk data
                    //
                    pScratch->pbData = pOp->pbBuffer;
                    pScratch->cbData = pOp->cbBuffer;
                    pScratch->processingState = STATE_DATA_END;

                    //
                    // Return TRUE if there is real data to process, and just re-run the state
                    // machine if we should copy the partial block to the buffer.
                    //
                    if( pScratch->cbData >= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE )
                    {
                        return TRUE;
                    } else {
                        continue;
                    }
                }
            } else {
                SYMCRYPT_ASSERT( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_RESULT );

                pState->buffer[bytesInBuffer++] = 0x80;
                SymCryptWipe( &pState->buffer[bytesInBuffer], SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - bytesInBuffer );

                pScratch->pbData = &pState->buffer[0];
                pScratch->cbData = sizeof( pState->buffer );

                if( bytesInBuffer > SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8 )
                {
                    // We need 2 blocks for the padding
                    pScratch->processingState = STATE_PAD2;
                } else {
                    SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8], pState->dataLengthL * 8 );
                    pScratch->processingState = STATE_RESULT;
                }
                return TRUE;
            }
            break;

        case STATE_DATA_START:
            //
            // The next operation is an append, and the first few bytes of that operation have already been copied to
            // the buffer and processed. We need to process the rest.
            // Note that the # bytes remaining is never zero.
            //
            SYMCRYPT_ASSERT( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_APPEND && pOp->cbBuffer >= pScratch->bytesAlreadyProcessed );

            pScratch->pbData = pOp->pbBuffer + pScratch->bytesAlreadyProcessed;
            pScratch->cbData = pOp->cbBuffer - pScratch->bytesAlreadyProcessed;
            if( pScratch->cbData >= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE )
            {
                pScratch->processingState = STATE_DATA_END;
                return TRUE;
            }

            //
            // We have less than one block left; this is exactly the same state as we have at the end of
            // a normal append. Fall through to that code.
            //
            // FALLTHROUGH!

        case STATE_DATA_END:
            //
            // We finished processing the whole blocks of the pScratch->pbData, and have to process the rest.
            // The current append is already popped off the work list.
            //
            if( pScratch->cbData > 0 )
            {
                SYMCRYPT_ASSERT( pScratch->cbData < SYMCRYPT_SHA256_INPUT_BLOCK_SIZE );
                memcpy( &pState->buffer[0], pScratch->pbData, pScratch->cbData );
                pState->bytesInBuffer = (UINT32) pScratch->cbData;
            }
            pScratch->next = pOp->next;
            pScratch->processingState = STATE_NEXT;
            continue;

        case STATE_PAD2:
            SymCryptWipe( &pState->buffer[0], sizeof( pState->buffer ));
            SYMCRYPT_STORE_MSBFIRST64( &pState->buffer[SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8], pState->dataLengthL * 8 );
            pScratch->pbData = &pState->buffer[0];
            pScratch->cbData = sizeof( pState->buffer );
            pScratch->processingState = STATE_RESULT;
            return TRUE;

        case STATE_RESULT:
            SYMCRYPT_ASSERT( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_RESULT );

            SymCryptUint32ToMsbFirst( &pState->chain.H[0], pOp->pbBuffer, 8 );
            SymCryptWipeKnownSize( pState, sizeof( *pState ));
            SymCryptSha256Init( pState );

            pScratch->next = pOp->next;
            pScratch->processingState = STATE_NEXT;
            continue;
        }
    }

#if 0       // old code, retain until we have the new one working.
    ============ old code


    SIZE_T bytesInBuffer;
    SIZE_T todo;

    switch( pScratch->processingState )
    {
    case START:

        if( pState->pbData != NULL )
        {
            bytesInBuffer = pState->internalState.hashState.dataLength & SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 1;

            //
            // There are bytes in the buffer; consume enough input to get rid of them.
            //
            if( bytesInBuffer > 0 )
            {
                todo = SYMCRYPT_MIN( SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - bytesInBuffer, pState->cbData );
                memcpy( &pState->internalState.hashState.buffer[bytesInBuffer], pState->pbData, todo );
                pState->pbData += todo;
                pState->cbData -= todo;
                pState->internalState.hashState.dataLength += todo;

                //
                // We don't parallelize the processing of the first block to get to the whole-block state.
                // It would mean we get a 1-size block up front, and that interferes with the sorted scheduling
                // we do. This is not a common case, and we document that this is inefficient.
                //
                if( (pState->internalState.hashState.dataLength & (SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 1)) == 0 )
                {
                    SymCryptSha256AppendBlocks( &pState->internalState.hashState.chain,
                                                &pState->internalState.hashState.buffer[0],
                                                SYMCRYPT_SHA256_INPUT_BLOCK_SIZE );
                }
            }

            if( pState->cbData >= SYMCRYPT_SHA256_INPUT_BLOCK_SIZE )
            {
                //
                // We have more bytes to do; this means that the internal buffer is empty.
                // Set the data blocks up for processing. We increment the dataLength here
                // as that is part of this function, not of the processing code.
                //
                pState->internalState.processingState = DATA;
                pState->internalState.hashState.dataLength += pState->cbData & ~(SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 1);
                return TRUE;
            }

        }

        //
        // FALL THROUGH TO THE DATA PROCESSING
        //
        // There are two cases here:
        // - the internal buffer is empty and we have between 1 and 63 bytes left to hash.
        // - We have no bytes left to hash, but the internal buffer might contain data.
        // The first case is exactly what we get after DATA processing.
        // The second case is trivially handled by the same code paths as the first one.
        // Instead of duplicating the code,
        // we fall through to the DATA section.
        //

        pState->internalState.processingState = DATA;

    case DATA:
        //
        // We just finished the data work, or the START code fell through here to handle the
        // padding and/or pbResult
        // If we just did data processing, the internal buffer is empty.
        // If the internal buffer contains data, then cbData == 0.
        //

        if( pState->pbData != NULL && pState->cbData > 0 )
        {
            SYMCRYPT_ASSERT( pState->cbData < SYMCRYPT_SHA256_INPUT_BLOCK_SIZE );
            memcpy( &pState->internalState.hashState.buffer[0], pState->pbData, pState->cbData );
            pState->internalState.hashState.dataLength += pState->cbData;
        }

        //
        // This completes the consumption of the pbData. Set it to NULL as per the API spec.
        //

        pState->pbData = NULL;
        pState->cbData = 0;

        //
        // This concludes the data processing. Now let's see if we have to compute the results
        //

        if( pState->pbResult == NULL )
        {
            return FALSE;
        }

        bytesInBuffer = pState->internalState.hashState.dataLength & SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 1;

        // Add the first byte of padding. (Always fits as the buffer is never left full.)
        pState->internalState.hashState.buffer[bytesInBuffer++] = 0x80;
        SymCryptWipe( &pState->internalState.hashState.buffer[bytesInBuffer], SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - bytesInBuffer );

        if( bytesInBuffer > SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8 )
        {
            //
            // We need 2 blocks for the padding.
            //
            pState->internalState.processingState = PAD_INTERMEDIATE;
            pState->pbData = &pState->internalState.hashState.buffer[0];
            pState->cbData = SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;

            return TRUE;
        }

        //
        // Single padding block
        //
        SYMCRYPT_STORE_MSBFIRST64( &pState->internalState.hashState.buffer[SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8], pState->internalState.hashState.dataLength * 8 );
        pState->internalState.processingState = PAD_FINAL;
        pState->pbData = &pState->internalState.hashState.buffer[0];
        pState->cbData = SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;
        return TRUE;

    case PAD_INTERMEDIATE:
        //
        // Done with the intermediate padding, do the final padding.
        // We wipe to the end of the buffer, as it is 16-aligned and therefore often faster
        //
        SymCryptWipe( &pState->internalState.hashState.buffer[0], SYMCRYPT_SHA256_INPUT_BLOCK_SIZE );
        SYMCRYPT_STORE_MSBFIRST64( &pState->internalState.hashState.buffer[SYMCRYPT_SHA256_INPUT_BLOCK_SIZE - 8], pState->internalState.hashState.dataLength * 8 );

        pState->internalState.processingState = PAD_FINAL;
        pState->pbData = &pState->internalState.hashState.buffer[0];
        pState->cbData = SYMCRYPT_SHA256_INPUT_BLOCK_SIZE;
        return TRUE;

    case PAD_FINAL:
        SymCryptUint32ToMsbFirst( &pState->internalState.hashState.chain.H[0], pState->pbResult, 8 );

        SymCryptWipeKnownSize( pState, sizeof( *pState ) );
        SymCryptSha256Init( &pState->internalState.hashState );
        SYMCRYPT_SET_MAGIC( &pState->internalState );
        return FALSE;
    }
#endif

    SymCryptFatal( 'psha' );
    return FALSE;
}
#endif

C_ASSERT( (SYMCRYPT_SIMD_ELEMENT_SIZE & (SYMCRYPT_SIMD_ELEMENT_SIZE - 1 )) == 0 );  // check that it is a power of 2


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelSha256Process(
    _Inout_updates_( nStates )      PSYMCRYPT_SHA256_STATE              pStates,
                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )       PBYTE                               pbScratch,
                                    SIZE_T                              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    UINT32 maxParallel;

#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
    SYMCRYPT_EXTENDED_SAVE_DATA SaveState;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_AVX2 ) && SymCryptSaveYmm( &SaveState ) == SYMCRYPT_NO_ERROR )
    {
        maxParallel = 8;
        scError = SymCryptParallelHashProcess(  SymCryptParallelSha256Algorithm,
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
        maxParallel = 4;
        scError = SymCryptParallelHashProcess(  SymCryptParallelSha256Algorithm,
                                                pStates,
                                                nStates,
                                                pOperations,
                                                nOperations,
                                                pbScratch,
                                                cbScratch,
                                                maxParallel );
        SymCryptRestoreXmm( &SaveState );
    } else {
        scError = SymCryptParallelHashProcess_serial( SymCryptParallelSha256Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
    }

#elif SYMCRYPT_CPU_ARM
    maxParallel = MAX_PARALLEL;

    if( SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_NEON ) )
    {
        scError = SymCryptParallelHashProcess(  SymCryptParallelSha256Algorithm,
                                                pStates,
                                                nStates,
                                                pOperations,
                                                nOperations,
                                                pbScratch,
                                                cbScratch,
                                                maxParallel );
    } else {
        scError = SymCryptParallelHashProcess_serial( SymCryptParallelSha256Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
    }
#else
    scError = SymCryptParallelHashProcess_serial( SymCryptParallelSha256Algorithm, pStates, nStates, pOperations, nOperations, pbScratch, cbScratch );
#endif
    return scError;
}


#if  SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64
//
// Code that uses the XMM registers.
//

#define MAJXMM( x, y, z ) _mm_or_si128( _mm_and_si128( _mm_or_si128( z, y ), x ), _mm_and_si128( z, y ))
#define CHXMM( x, y, z )  _mm_xor_si128( _mm_and_si128( _mm_xor_si128( z, y ), x ), z )

#define CSIGMA0XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,30)  , _mm_srli_epi32(x,  2) ),\
        _mm_slli_epi32(x,19) ), _mm_srli_epi32(x, 13) ),\
        _mm_slli_epi32(x,10) ), _mm_srli_epi32(x, 22) )
#define CSIGMA1XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,26)  , _mm_srli_epi32(x,  6) ),\
        _mm_slli_epi32(x,21) ), _mm_srli_epi32(x, 11) ),\
        _mm_slli_epi32(x,7) ), _mm_srli_epi32(x, 25) )
#define LSIGMA0XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,25)  , _mm_srli_epi32(x,  7) ),\
        _mm_slli_epi32(x,14) ), _mm_srli_epi32(x, 18) ),\
        _mm_srli_epi32(x, 3) )
#define LSIGMA1XMM( x ) \
    _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( _mm_xor_si128( \
        _mm_slli_epi32(x,15)  , _mm_srli_epi32(x, 17) ),\
        _mm_slli_epi32(x,13) ), _mm_srli_epi32(x, 19) ),\
        _mm_srli_epi32(x,10) )

//
// Transpose macro, convert S0..S3 into R0..R3; R0 is the lane 0, R3 is lane 3.
// S0 = S00, S01, S02, S03; S1 = S10, S11, S12, S13; S2 = S20, S21, S22, S23; S3 = S30, S31, S32, S33
// T0 = S00, S10, S01, S11; T1 = S02, S12, S03, S13; T2 = S20, S30, S21, S31; T3 = S22, S32, S23, S33
// R0 = S00, S10, S20, S30; R1 = S01, S11, S21, S31; R2 = S02, S12, S22, S32; R3 = S03, S13, S23, S33
//
#define XMM_TRANSPOSE_32( _R0, _R1, _R2, _R3, _S0, _S1, _S2, _S3 ) \
    {\
        __m128i _T0, _T1, _T2, _T3;\
        _T0 = _mm_unpacklo_epi32( _S0, _S1 ); _T1 = _mm_unpackhi_epi32( _S0, _S1 );\
        _T2 = _mm_unpacklo_epi32( _S2, _S3 ); _T3 = _mm_unpackhi_epi32( _S2, _S3 );\
        _R0 = _mm_unpacklo_epi64( _T0, _T2 ); _R1 = _mm_unpackhi_epi64( _T0, _T2 );\
        _R2 = _mm_unpacklo_epi64( _T1, _T3 ); _R3 = _mm_unpackhi_epi64( _T1, _T3 );\
    }

VOID
SYMCRYPT_CALL
SymCryptParallelSha256AppendBlocks_xmm(
    _Inout_updates_( 4 )                                PSYMCRYPT_SHA256_CHAINING_STATE   * pChain,
    _Inout_updates_( 4 )                                PCBYTE                            * ppByte,
                                                        SIZE_T                              nBytes,
    _Out_writes_( PAR_SCRATCH_ELEMENTS_256 )            __m128i                           * pScratch )
{
    //
    // Implementation that uses 4 lanes in the XMM registers
    //
    __m128i * buf = pScratch;       // chaining state concatenated with the expanded input block
    __m128i * W = &buf[4 + 8];      // W are the 64 words of the expanded input
    __m128i * ha = &buf[4]; // initial state words, in order h, g, ..., b, a
    __m128i A, B, C, D, T;
    __m128i T0, T1, T2, T3;
    const __m128i BYTE_REVERSE_32 = _mm_set_epi8( 12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3 );
    int r;

    //
    // The chaining state can be unaligned on x86, so we use unaligned loads
    //

    T0 = _mm_loadu_si128( (__m128i *)&pChain[0]->H[0] );
    T1 = _mm_loadu_si128( (__m128i *)&pChain[1]->H[0] );
    T2 = _mm_loadu_si128( (__m128i *)&pChain[2]->H[0] );
    T3 = _mm_loadu_si128( (__m128i *)&pChain[3]->H[0] );

    XMM_TRANSPOSE_32( ha[7], ha[6], ha[5], ha[4], T0, T1, T2, T3 );

    T0 = _mm_loadu_si128( (__m128i *)&pChain[0]->H[4] );
    T1 = _mm_loadu_si128( (__m128i *)&pChain[1]->H[4] );
    T2 = _mm_loadu_si128( (__m128i *)&pChain[2]->H[4] );
    T3 = _mm_loadu_si128( (__m128i *)&pChain[3]->H[4] );

    XMM_TRANSPOSE_32( ha[3], ha[2], ha[1], ha[0], T0, T1, T2, T3 );

    buf[0] = ha[4];
    buf[1] = ha[5];
    buf[2] = ha[6];
    buf[3] = ha[7];

    while( nBytes >= 64 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r += 4 )
        {
            T0 = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *) ppByte[0] ), BYTE_REVERSE_32 ); ppByte[0] += 16;
            T1 = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *) ppByte[1] ), BYTE_REVERSE_32 ); ppByte[1] += 16;
            T2 = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *) ppByte[2] ), BYTE_REVERSE_32 ); ppByte[2] += 16;
            T3 = _mm_shuffle_epi8( _mm_loadu_si128( (__m128i *) ppByte[3] ), BYTE_REVERSE_32 ); ppByte[3] += 16;

            XMM_TRANSPOSE_32( W[r], W[r+1], W[r+2], W[r+3], T0, T1, T2, T3 );
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
                        b = _mm_add_epi32( _mm_add_epi32( _mm_add_epi32( d, LSIGMA1XMM( b ) ), W[r-7] ), LSIGMA0XMM( c ) ); \
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
                t = _mm_add_epi32( t, CSIGMA1XMM( W[r-5] ) ); \
                t = _mm_add_epi32( t, W[r-8] ); \
                t = _mm_add_epi32( t, CHXMM( W[r-5], W[r-6], W[r-7] ) ); \
                t = _mm_add_epi32( t, _mm_set1_epi32( SymCryptSha256K[r] )); \
                W[r-4] = _mm_add_epi32( t, d ); \
                d = _mm_add_epi32( t, CSIGMA0XMM( a ) ); \
                d = _mm_add_epi32( d, MAJXMM( c, b, a ) );

            DO_ROUND( A, B, C, D, T, r );
            DO_ROUND( D, A, B, C, T, (r+1) );
            DO_ROUND( C, D, A, B, T, (r+2) );
            DO_ROUND( B, C, D, A, T, (r+3) );
            #undef DO_ROUND
        }

        buf[3] = ha[7] = _mm_add_epi32( buf[3], A );
        buf[2] = ha[6] = _mm_add_epi32( buf[2], B );
        buf[1] = ha[5] = _mm_add_epi32( buf[1], C );
        buf[0] = ha[4] = _mm_add_epi32( buf[0], D );
        ha[3] = _mm_add_epi32( ha[3], W[r-5] );
        ha[2] = _mm_add_epi32( ha[2], W[r-6] );
        ha[1] = _mm_add_epi32( ha[1], W[r-7] );
        ha[0] = _mm_add_epi32( ha[0], W[r-8] );

        nBytes -= 64;
    }

    //
    // Copy the chaining state back into the hash structure
    //
    XMM_TRANSPOSE_32( T0, T1, T2, T3, ha[7], ha[6], ha[5], ha[4] );
    _mm_storeu_si128( (__m128i *)&pChain[0]->H[0], T0 );
    _mm_storeu_si128( (__m128i *)&pChain[1]->H[0], T1 );
    _mm_storeu_si128( (__m128i *)&pChain[2]->H[0], T2 );
    _mm_storeu_si128( (__m128i *)&pChain[3]->H[0], T3 );

    XMM_TRANSPOSE_32( T0, T1, T2, T3, ha[3], ha[2], ha[1], ha[0] );
    _mm_storeu_si128( (__m128i *)&pChain[0]->H[4], T0 );
    _mm_storeu_si128( (__m128i *)&pChain[1]->H[4], T1 );
    _mm_storeu_si128( (__m128i *)&pChain[2]->H[4], T2 );
    _mm_storeu_si128( (__m128i *)&pChain[3]->H[4], T3 );

}

#endif // CPU_X86_X64

#if  SYMCRYPT_CPU_ARM
//
// Code that uses the Neon registers.
//

#define MAJ( x, y, z ) vorrq_u32( vandq_u32( vorrq_u32( z, y ), x ), vandq_u32( z, y ))
#define CH( x, y, z )  veorq_u32( vandq_u32( veorq_u32( z, y ), x ), z )

#define CSIGMA0( x ) \
    veorq_u32( veorq_u32( veorq_u32( veorq_u32( veorq_u32( \
        vshlq_n_u32(x,30)  , vshrq_n_u32(x,  2) ),\
        vshlq_n_u32(x,19) ), vshrq_n_u32(x, 13) ),\
        vshlq_n_u32(x,10) ), vshrq_n_u32(x, 22) )
#define CSIGMA1( x ) \
    veorq_u32( veorq_u32( veorq_u32( veorq_u32( veorq_u32( \
        vshlq_n_u32(x,26)  , vshrq_n_u32(x,  6) ),\
        vshlq_n_u32(x,21) ), vshrq_n_u32(x, 11) ),\
        vshlq_n_u32(x,7) ), vshrq_n_u32(x, 25) )
#define LSIGMA0( x ) \
    veorq_u32( veorq_u32( veorq_u32( veorq_u32( \
        vshlq_n_u32(x,25)  , vshrq_n_u32(x,  7) ),\
        vshlq_n_u32(x,14) ), vshrq_n_u32(x, 18) ),\
        vshrq_n_u32(x, 3) )
#define LSIGMA1( x ) \
    veorq_u32( veorq_u32( veorq_u32( veorq_u32( \
        vshlq_n_u32(x,15)  , vshrq_n_u32(x, 17) ),\
        vshlq_n_u32(x,13) ), vshrq_n_u32(x, 19) ),\
        vshrq_n_u32(x,10) )

VOID
SYMCRYPT_CALL
SymCryptParallelSha256AppendBlocks_neon(
    _Inout_updates_( 4 )                                PSYMCRYPT_SHA256_CHAINING_STATE   * pChain,
    _Inout_updates_( 4 )                                PCBYTE                            * ppByte,
                                                        SIZE_T                              nBytes,
    _Out_writes_( PAR_SCRATCH_ELEMENTS_256 )            __n128                            * pScratch )
{
    //
    // Implementation that uses 4 lanes in the Neon registers
    //
    __n128 * buf = pScratch;
    __n128 * W = &buf[4 + 8];
    __n128 * ha = &buf[4]; // initial state words, in order h, g, ..., b, a
    __n128 A, B, C, D, T;
    __n128 T0;
    int r;

    //
    // This can probably be done faster, but we are missing the VTRN.64 instruction
    // which makes it hard to do this efficient in intrinsics.
    //
    ha[7] = vsetq_lane_u32( pChain[0]->H[0], ha[7], 0 );
    ha[7] = vsetq_lane_u32( pChain[1]->H[0], ha[7], 1 );
    ha[7] = vsetq_lane_u32( pChain[2]->H[0], ha[7], 2 );
    ha[7] = vsetq_lane_u32( pChain[3]->H[0], ha[7], 3 );

    ha[6] = vsetq_lane_u32( pChain[0]->H[1], ha[6], 0 );
    ha[6] = vsetq_lane_u32( pChain[1]->H[1], ha[6], 1 );
    ha[6] = vsetq_lane_u32( pChain[2]->H[1], ha[6], 2 );
    ha[6] = vsetq_lane_u32( pChain[3]->H[1], ha[6], 3 );

    ha[5] = vsetq_lane_u32( pChain[0]->H[2], ha[5], 0 );
    ha[5] = vsetq_lane_u32( pChain[1]->H[2], ha[5], 1 );
    ha[5] = vsetq_lane_u32( pChain[2]->H[2], ha[5], 2 );
    ha[5] = vsetq_lane_u32( pChain[3]->H[2], ha[5], 3 );

    ha[4] = vsetq_lane_u32( pChain[0]->H[3], ha[4], 0 );
    ha[4] = vsetq_lane_u32( pChain[1]->H[3], ha[4], 1 );
    ha[4] = vsetq_lane_u32( pChain[2]->H[3], ha[4], 2 );
    ha[4] = vsetq_lane_u32( pChain[3]->H[3], ha[4], 3 );

    ha[3] = vsetq_lane_u32( pChain[0]->H[4], ha[3], 0 );
    ha[3] = vsetq_lane_u32( pChain[1]->H[4], ha[3], 1 );
    ha[3] = vsetq_lane_u32( pChain[2]->H[4], ha[3], 2 );
    ha[3] = vsetq_lane_u32( pChain[3]->H[4], ha[3], 3 );

    ha[2] = vsetq_lane_u32( pChain[0]->H[5], ha[2], 0 );
    ha[2] = vsetq_lane_u32( pChain[1]->H[5], ha[2], 1 );
    ha[2] = vsetq_lane_u32( pChain[2]->H[5], ha[2], 2 );
    ha[2] = vsetq_lane_u32( pChain[3]->H[5], ha[2], 3 );

    ha[1] = vsetq_lane_u32( pChain[0]->H[6], ha[1], 0 );
    ha[1] = vsetq_lane_u32( pChain[1]->H[6], ha[1], 1 );
    ha[1] = vsetq_lane_u32( pChain[2]->H[6], ha[1], 2 );
    ha[1] = vsetq_lane_u32( pChain[3]->H[6], ha[1], 3 );

    ha[0] = vsetq_lane_u32( pChain[0]->H[7], ha[0], 0 );
    ha[0] = vsetq_lane_u32( pChain[1]->H[7], ha[0], 1 );
    ha[0] = vsetq_lane_u32( pChain[2]->H[7], ha[0], 2 );
    ha[0] = vsetq_lane_u32( pChain[3]->H[7], ha[0], 3 );

    buf[0] = ha[4];
    buf[1] = ha[5];
    buf[2] = ha[6];
    buf[3] = ha[7];

    while( nBytes >= 64 )
    {

        //
        // Capture the input into W[0..15]
        //
        for( r=0; r<16; r ++ )
        {
            T0 = vsetq_lane_u32( SYMCRYPT_LOAD_MSBFIRST32( ppByte[0] ), T0, 0 ); ppByte[0] += 4;
            T0 = vsetq_lane_u32( SYMCRYPT_LOAD_MSBFIRST32( ppByte[1] ), T0, 1 ); ppByte[1] += 4;
            T0 = vsetq_lane_u32( SYMCRYPT_LOAD_MSBFIRST32( ppByte[2] ), T0, 2 ); ppByte[2] += 4;
            T0 = vsetq_lane_u32( SYMCRYPT_LOAD_MSBFIRST32( ppByte[3] ), T0, 3 ); ppByte[3] += 4;
            W[r] = T0;
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
                        b = vaddq_u32( vaddq_u32( vaddq_u32( d, LSIGMA1( b ) ), W[r-7] ), LSIGMA0( c ) ); \
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
                t = vaddq_u32( t, CSIGMA1( W[r-5] ) ); \
                t = vaddq_u32( t, W[r-8] ); \
                t = vaddq_u32( t, CH( W[r-5], W[r-6], W[r-7] ) ); \
                t = vaddq_u32( t, vdupq_n_u32( SymCryptSha256K[r] )); \
                W[r-4] = vaddq_u32( t, d ); \
                d = vaddq_u32( t, CSIGMA0( a ) ); \
                d = vaddq_u32( d, MAJ( c, b, a ) );

            DO_ROUND( A, B, C, D, T, r );
            DO_ROUND( D, A, B, C, T, (r+1) );
            DO_ROUND( C, D, A, B, T, (r+2) );
            DO_ROUND( B, C, D, A, T, (r+3) );
            #undef DO_ROUND
        }

        buf[3] = ha[7] = vaddq_u32( buf[3], A );
        buf[2] = ha[6] = vaddq_u32( buf[2], B );
        buf[1] = ha[5] = vaddq_u32( buf[1], C );
        buf[0] = ha[4] = vaddq_u32( buf[0], D );
        ha[3] = vaddq_u32( ha[3], W[r-5] );
        ha[2] = vaddq_u32( ha[2], W[r-6] );
        ha[1] = vaddq_u32( ha[1], W[r-7] );
        ha[0] = vaddq_u32( ha[0], W[r-8] );

        nBytes -= 64;
    }

    //
    // Copy the chaining state back into the hash structure
    //
    pChain[0]->H[0] = vgetq_lane_u32( ha[7], 0 );
    pChain[1]->H[0] = vgetq_lane_u32( ha[7], 1 );
    pChain[2]->H[0] = vgetq_lane_u32( ha[7], 2 );
    pChain[3]->H[0] = vgetq_lane_u32( ha[7], 3 );

    pChain[0]->H[1] = vgetq_lane_u32( ha[6], 0 );
    pChain[1]->H[1] = vgetq_lane_u32( ha[6], 1 );
    pChain[2]->H[1] = vgetq_lane_u32( ha[6], 2 );
    pChain[3]->H[1] = vgetq_lane_u32( ha[6], 3 );

    pChain[0]->H[2] = vgetq_lane_u32( ha[5], 0 );
    pChain[1]->H[2] = vgetq_lane_u32( ha[5], 1 );
    pChain[2]->H[2] = vgetq_lane_u32( ha[5], 2 );
    pChain[3]->H[2] = vgetq_lane_u32( ha[5], 3 );

    pChain[0]->H[3] = vgetq_lane_u32( ha[4], 0 );
    pChain[1]->H[3] = vgetq_lane_u32( ha[4], 1 );
    pChain[2]->H[3] = vgetq_lane_u32( ha[4], 2 );
    pChain[3]->H[3] = vgetq_lane_u32( ha[4], 3 );

    pChain[0]->H[4] = vgetq_lane_u32( ha[3], 0 );
    pChain[1]->H[4] = vgetq_lane_u32( ha[3], 1 );
    pChain[2]->H[4] = vgetq_lane_u32( ha[3], 2 );
    pChain[3]->H[4] = vgetq_lane_u32( ha[3], 3 );

    pChain[0]->H[5] = vgetq_lane_u32( ha[2], 0 );
    pChain[1]->H[5] = vgetq_lane_u32( ha[2], 1 );
    pChain[2]->H[5] = vgetq_lane_u32( ha[2], 2 );
    pChain[3]->H[5] = vgetq_lane_u32( ha[2], 3 );

    pChain[0]->H[6] = vgetq_lane_u32( ha[1], 0 );
    pChain[1]->H[6] = vgetq_lane_u32( ha[1], 1 );
    pChain[2]->H[6] = vgetq_lane_u32( ha[1], 2 );
    pChain[3]->H[6] = vgetq_lane_u32( ha[1], 3 );

    pChain[0]->H[7] = vgetq_lane_u32( ha[0], 0 );
    pChain[1]->H[7] = vgetq_lane_u32( ha[0], 1 );
    pChain[2]->H[7] = vgetq_lane_u32( ha[0], 2 );
    pChain[3]->H[7] = vgetq_lane_u32( ha[0], 3 );

    SymCryptWipeKnownSize( buf, sizeof( buf ) );
}

#undef CH
#undef MAJ
#undef CSIGMA0
#undef CSIGMA1
#undef LSIGMA0
#undef LSIGMA1

#endif // CPU_X86_X64



#if SYMCRYPT_CPU_X86 || SYMCRYPT_CPU_AMD64 || SYMCRYPT_CPU_ARM

VOID
SYMCRYPT_CALL
SymCryptParallelSha256AppendBytes_serial(
    _Inout_updates_( nPar )                 PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork,
    _In_range_(1, MAX_PARALLEL)             SIZE_T                                  nPar,
                                            SIZE_T                                  nBytes )
{
    SIZE_T i;
    SIZE_T tmp;

    SYMCRYPT_ASSERT( nBytes % SYMCRYPT_SHA256_INPUT_BLOCK_SIZE == 0 );
    SYMCRYPT_ASSERT( nPar >= 1 && nPar <= MAX_PARALLEL );

    for( i=0; i < nPar; i++ )
    {
        SYMCRYPT_ASSERT( pWork[i]->cbData >= nBytes );
        SymCryptSha256AppendBlocks( & ((PSYMCRYPT_SHA256_STATE)(pWork[i]->hashState))->chain, pWork[i]->pbData, nBytes, &tmp );
        pWork[i]->pbData += nBytes;
        pWork[i]->cbData -= nBytes;
    }
    return;
}

VOID
SYMCRYPT_CALL
SymCryptParallelSha256Append(
    _Inout_updates_( nPar )                 PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork,
    _In_range_(1, MAX_PARALLEL)             SIZE_T                                  nPar,
                                            SIZE_T                                  nBytes,
    _Out_writes_to_( SYMCRYPT_SIMD_ELEMENT_SIZE * PAR_SCRATCH_ELEMENTS_256, 0 )
                                            PBYTE                                   pbSimdScratch,
                                            SIZE_T                                  cbSimdScratch )
{
    PSYMCRYPT_SHA256_CHAINING_STATE apChain[MAX_PARALLEL];
    PCBYTE                          apData[MAX_PARALLEL];
    SIZE_T                          i;
    UINT32                          maxParallel;

    UNREFERENCED_PARAMETER( cbSimdScratch );        // not referenced on FRE builds
    SYMCRYPT_ASSERT( cbSimdScratch >= PAR_SCRATCH_ELEMENTS_256 * SYMCRYPT_SIMD_ELEMENT_SIZE );
    SYMCRYPT_ASSERT( ((SIZE_T)pbSimdScratch & (SYMCRYPT_SIMD_ELEMENT_SIZE - 1)) == 0 );

    //
    // Compute maxParallel; this is 4 if nPar <= 4, and 8 if nPar = 5, ..., 8.
    // This is how many parameter sets we have to set up.
    //
#if SYMCRYPT_CPU_X86 | SYMCRYPT_CPU_AMD64

    maxParallel = (nPar + 3) & ~3;
    SYMCRYPT_ASSERT( maxParallel == 4 || (maxParallel == 8 && SYMCRYPT_CPU_FEATURES_PRESENT( SYMCRYPT_CPU_FEATURE_AVX2 )) );

#elif SYMCRYPT_CPU_ARM

    maxParallel = 4;

#endif

    SYMCRYPT_ASSERT( nPar >= 1 && nPar <= maxParallel );

    if( nPar < MIN_PARALLEL )
    {
        SymCryptParallelSha256AppendBytes_serial( pWork, nPar, nBytes );

        // Done with this function.
        goto cleanup;
    }

    //
    // Our parallel code expects exactly four or eight parallel computations.
    // We simply duplicate the first one if we get fewer parallel ones.
    // That means we write the result multiple times, but it saves a lot of
    // extra if()s in the main codeline.
    //

    i = 0;
    while( i < nPar )
    {
            SYMCRYPT_ASSERT( pWork[i]->cbData >= nBytes );
            apChain[i] =  & ((PSYMCRYPT_SHA256_STATE)(pWork[i]->hashState))->chain;
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
    if( maxParallel == 8 )
    {
        SymCryptParallelSha256AppendBlocks_ymm(  &apChain[0], &apData[0], nBytes, (PBYTE)((__m256i *)pbSimdScratch) );
    } else {
        SymCryptParallelSha256AppendBlocks_xmm(  &apChain[0], &apData[0], nBytes, (__m128i *)pbSimdScratch );
    }
#elif SYMCRYPT_CPU_ARM
    SymCryptParallelSha256AppendBlocks_neon( &apChain[0], &apData[0], nBytes, (__n128 *) pbSimdScratch );
#else
#error Unknown CPU
#endif

cleanup:
    ;// no cleanup at this moment.
}

#endif

/*
VOID
SYMCRYPT_CALL
SymCryptParallelSha256AppendBlocks(
    _Inout_updates_( nWork )    PSYMCRYPT_PARALLEL_SHA256_STATE *   pWork,
                                SIZE_T                              nWork,
                                SIZE_T                              nBytes )
{
    SIZE_T i;

    SYMCRYPT_ASSERT( nWork >= 1 && nWork <= 4 );

    for( i=0; i < nWork; i++ )
    {
        SYMCRYPT_ASSERT( pWork[i]->cbData >= nBytes );
        SymCryptSha256AppendBlocks( &pWork[i]->internalState.hashState.chain, pWork[i]->pbData, nBytes );
        pWork[i]->pbData += nBytes;
        pWork[i]->cbData -= nBytes;
    }
}

*/

#endif // SUPPORT_PARALLEL

#if SUPPORT_PARALLEL

const SYMCRYPT_PARALLEL_HASH SymCryptParallelSha256Algorithm_default = {
    &SymCryptSha256Algorithm_default,
    PAR_SCRATCH_ELEMENTS_256 * SYMCRYPT_SIMD_ELEMENT_SIZE,
    &SymCryptParallelSha256Result1,
    &SymCryptParallelSha256Result2,
    &SymCryptParallelSha256ResultDone,
    &SymCryptParallelSha256Append,
};

#else

//
// For platforms that do not have a parallel hash implementation
// we use this structure to provide the necessary data to the _serial
// implementation of the function.
//
const SYMCRYPT_PARALLEL_HASH SymCryptParallelSha256Algorithm_default = {
    &SymCryptSha256Algorithm_default,
    PAR_SCRATCH_ELEMENTS_256 * SYMCRYPT_SIMD_ELEMENT_SIZE,
    NULL,
    NULL,
    NULL,
    NULL,
};

#endif

const PCSYMCRYPT_PARALLEL_HASH SymCryptParallelSha256Algorithm = &SymCryptParallelSha256Algorithm_default;


#define N_SELFTEST_STATES   5      // Just enough to trigger YMM usage

VOID
SYMCRYPT_CALL
SymCryptParallelSha256Selftest(void)
{
    SYMCRYPT_ERROR                      scError;
    SYMCRYPT_SHA256_STATE               states[N_SELFTEST_STATES];
    BYTE                                result[N_SELFTEST_STATES][SYMCRYPT_SHA256_RESULT_SIZE];
    SYMCRYPT_PARALLEL_HASH_OPERATION    op[2*N_SELFTEST_STATES];
    BYTE                                scratch[SYMCRYPT_PARALLEL_SHA256_FIXED_SCRATCH + N_SELFTEST_STATES * SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH];
    int                                 i;

    SymCryptParallelSha256Init( &states[0], N_SELFTEST_STATES );

    for( i=0; i<N_SELFTEST_STATES; i++ )
    {
        op[2*i    ].iHash = i;
        op[2*i    ].hashOperation = SYMCRYPT_HASH_OPERATION_APPEND;
        op[2*i    ].pbBuffer = (PBYTE) SymCryptTestMsg3;
        op[2*i    ].cbBuffer = sizeof(SymCryptTestMsg3);
        op[2*i + 1].iHash = i;
        op[2*i + 1].hashOperation = SYMCRYPT_HASH_OPERATION_RESULT;
        op[2*i + 1].pbBuffer = &result[i][0];
        op[2*i + 1].cbBuffer = SYMCRYPT_SHA256_RESULT_SIZE;
    }

    scError = SymCryptParallelSha256Process( &states[0], N_SELFTEST_STATES, op, 2*N_SELFTEST_STATES, scratch, sizeof( scratch ) );
    if( scError != SYMCRYPT_NO_ERROR )
    {
        SymCryptFatal( 'PS25' );
    }

    for( i=0; i<N_SELFTEST_STATES; i++ )
    {
        SymCryptInjectError( &result[i][0], SYMCRYPT_SHA256_RESULT_SIZE );

        if( memcmp( &result[i][0], SymCryptSha256KATAnswer, SYMCRYPT_SHA256_RESULT_SIZE ) != 0 ) {
            SymCryptFatal( 'PS25' );
        }
    }
}

