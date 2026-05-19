//
// ParHash.c
// Code shared with all the parallel hash implementations
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelHashProcess_serial(
    _In_                                                            PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_updates_bytes_( nStates * pParHash->pHash->stateSize )   PVOID                               pStates,
                                                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )                                  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )                                       PBYTE                               pbScratch,
                                                                    SIZE_T                              cbScratch )
{
    SYMCRYPT_ERROR scError = SYMCRYPT_NO_ERROR;
    SIZE_T i;
    PSYMCRYPT_PARALLEL_HASH_OPERATION op;
    PCSYMCRYPT_HASH pHash;

    pHash = pParHash->pHash;
    op = pOperations;

    //
    // Wipe the scratch space to detect erroneous callers.
    // We do this so that callers that test on a non-parallel platform will work on a platform that does support
    // parallel operations.
    //
    if( cbScratch < pParHash->parScratchFixed + nStates * SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }
    SymCryptWipeKnownSize( pbScratch, pParHash->parScratchFixed + nStates * SYMCRYPT_PARALLEL_HASH_PER_STATE_SCRATCH );

    for( i=0; i<nOperations; i++ )
    {
        if( op->iHash >= nStates )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
        switch( op->hashOperation )
        {
        case SYMCRYPT_HASH_OPERATION_APPEND:
            (*pHash->appendFunc)( (PBYTE)pStates + pHash->stateSize * op->iHash, op->pbBuffer, op->cbBuffer );
            break;

        case SYMCRYPT_HASH_OPERATION_RESULT:
            if( op->cbBuffer != pHash->resultSize )
            {
                scError = SYMCRYPT_INVALID_ARGUMENT;
                goto cleanup;
            }
            (*pHash->resultFunc)( (PBYTE)pStates + pHash->stateSize * op->iHash, op->pbBuffer );
            break;

        default:
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }
        op++;
    }

cleanup:
    return scError;
}

//
// This function looks at a state and decides what to do.
// If it returns FALSE, then this state is done and no further processing is required.
// If it returns TRUE, the pbData/cbData have to be processed in parallel.
// This function is called again on the same state after the pbData/cbData have been processed.
//
// Internally, it keeps track of the next step to be taken for this state.
// the processingState keeps track of the next action to take.
//

//
// An enum to keep track of the state of a request block
//
BOOLEAN
SYMCRYPT_CALL
SymCryptParallelHashSetNextWork( PCSYMCRYPT_PARALLEL_HASH pParHash, PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE pScratch )
{
    PSYMCRYPT_COMMON_HASH_STATE         pState;
    PCSYMCRYPT_HASH                     pHash;
    PCSYMRYPT_PARALLEL_HASH_OPERATION   pOp;
    SIZE_T                              bytesInBuffer;
    SIZE_T                              todo;
    BOOLEAN                             res;

    // Retrieve the state we will operate on.
    pState = (PSYMCRYPT_COMMON_HASH_STATE) pScratch->hashState;
    pHash = pParHash->pHash;

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
                if( pState->dataLengthL < pOp->cbBuffer ) {
                    pState->dataLengthH ++;                         // This is almost-unreachable code as it requires 2^64 bytes to be hashed.
                }

                if( bytesInBuffer > 0 )
                {
                    SYMCRYPT_ASSERT( pHash->inputBlockSize > bytesInBuffer );

                    todo = SYMCRYPT_MIN( pHash->inputBlockSize - bytesInBuffer, pOp->cbBuffer );
                    memcpy( &pState->buffer[bytesInBuffer], pOp->pbBuffer, todo );
                    pState->bytesInBuffer += (UINT32) todo;
                    if( pState->bytesInBuffer == pHash->inputBlockSize )
                    {
                        //
                        // We filled the buffer; set it for processing.
                        // Remember the # bytes we did and set the next state to process the rest of the request.
                        //
                        pScratch->pbData = &pState->buffer[0];
                        pScratch->cbData = pHash->inputBlockSize;
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

                        pState->bytesInBuffer = 0;          // it will be after we process the block
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
                    if( pScratch->cbData >= pHash->inputBlockSize )
                    {
                        return TRUE;
                    } else {
                        continue;
                    }
                }
            } else {
                SYMCRYPT_ASSERT( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_RESULT );

                if( (*pParHash->parResult1Func)( pParHash, pState, pScratch, &res ) )
                {
                    return res;
                }
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
            if( pScratch->cbData >= pHash->inputBlockSize )
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
                SYMCRYPT_ASSERT( pScratch->cbData < pHash->inputBlockSize );
                memcpy( &pState->buffer[0], pScratch->pbData, pScratch->cbData );
                pState->bytesInBuffer = (UINT32) pScratch->cbData;
            }
            pScratch->next = pOp->next;
            pScratch->processingState = STATE_NEXT;
            continue;

        case STATE_RESULT2:
            if( (*pParHash->parResult2Func)( pParHash, pState, pScratch, &res ) )
            {
                return res;
            }
            continue;

        case STATE_RESULT_DONE:

            (*pParHash->parResultDoneFunc)( pParHash, pState, pOp );

            pScratch->next = pOp->next;
            pScratch->processingState = STATE_NEXT;
            continue;
        }
    }

    return FALSE;
}


//
// Comparison function used to sort the work into largest-first order.
//
int SYMCRYPT_CDECL
compareRequestSize( PCVOID p1, PCVOID p2 )
{
    PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pp1 = (PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE *) p1;
    PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pp2 = (PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE *) p2;

    UINT64 c1 = (*pp1)->bytes;
    UINT64 c2 = (*pp2)->bytes;

    //
    // This is 'reverse' compare function as we want the largest item first.
    //
    if( c1 < c2 )
    {
        return 1;
    } else if( c1 > c2 )
    {
        return -1;
    } else {
        return 0;
    }
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptParallelHashProcess(
    _In_                                                            PCSYMCRYPT_PARALLEL_HASH            pParHash,
    _Inout_updates_bytes_( nStates * pParHash->pHash->stateSize )   PVOID                               pStates,
                                                                    SIZE_T                              nStates,
    _Inout_updates_( nOperations )                                  PSYMCRYPT_PARALLEL_HASH_OPERATION   pOperations,
                                                                    SIZE_T                              nOperations,
    _Out_writes_( cbScratch )                                       PBYTE                               pbScratch,
                                                                    SIZE_T                              cbScratch,
                                                                    UINT32                              maxParallel )
{
    SYMCRYPT_ERROR                          scError = SYMCRYPT_NO_ERROR;
    PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE   pScratchState;
    PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pWork;
    SIZE_T                                  nWork;
    PSYMCRYPT_PARALLEL_HASH_OPERATION       pOp;
    PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE   pSc;
    SIZE_T                                  i;
    UINT64                                  singleSize;
    BOOLEAN                                 sameSize;
    SIZE_T                                  nPar;
    PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE * pNextWork;
    SIZE_T                                  todo;
    SIZE_T                                  nBytes;
    PBYTE                                   pbScratchEnd;
    PBYTE                                   pbFixedScratch;
    SIZE_T                                  cbFixedScratch;
    PCSYMCRYPT_HASH                         pHash;

    if( nOperations == 0 )
    {
        goto cleanup;
    }

    pHash = pParHash->pHash;

    //
    // The caller passes us a scratch buffer. We split that into the following pieces:
    //
    // <alignment space  to SYMCRYPT_ALIGN_VALUE>
    // SYMCRYPT_PARALLEL_HASH_SCRATCH pScratchState[ nStates ]
    // PSYMCRYPT_PARALLEL_HASH_SCRATCH pWork[ nStates ]
    // <alignment space to SYMCRYPT_SIMD_ELEMENT_SIZE>
    // scratch space for parallel function
    //

    pbScratchEnd = pbScratch + cbScratch;
    pScratchState = (PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE) SYMCRYPT_ALIGN_UP( pbScratch );
    pWork = (PSYMCRYPT_PARALLEL_HASH_SCRATCH_STATE *) (pScratchState + nStates);
    pbFixedScratch = (PBYTE)((((SIZE_T)(pWork + nStates)) + SYMCRYPT_SIMD_ELEMENT_SIZE - 1) & ~(SYMCRYPT_SIMD_ELEMENT_SIZE - 1));
    cbFixedScratch = pParHash->parScratchFixed;

    if( pbFixedScratch + cbFixedScratch > pbScratchEnd )
    {
        scError = SYMCRYPT_BUFFER_TOO_SMALL;
        goto cleanup;
    }

    //
    // Wipe the scratch state; this sets the pointers to NULL, and the byte counts to 0.
    //
    memset( pScratchState, 0, nStates * sizeof( *pScratchState ));
    nWork = 0;

    //
    // The general data structure is as follows.
    // For each hash state, we keep our administration in the pScratchState[i]. This contains a pointer to the actual
    // hash state, a pointer to a linked list of operations to be performed on this state, pointer/length of the
    // current data to be processed, and a few more administrative items.
    // We also keep the pWork array of pointers to our scratch states, which contains all the states that still need
    // work to be done.
    //
    // We process over the operations in reverse order to make it easy to build a forward single-linked list
    //
    pOp = &pOperations[ nOperations ];
    while( pOp > pOperations )
    {
        pOp--;

        if( pOp->iHash >= nStates )
        {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        pSc = &pScratchState[ pOp->iHash ];

        if( pSc->hashState == NULL )
        {
            //
            // We found a new state that is being modified by this set of operations.
            // Set the pointer to the hash state, and add it to the work list.
            //
            SYMCRYPT_ASSERT( nWork < nStates );
            pSc->hashState = (PBYTE) pStates + pHash->stateSize * pOp->iHash;
            pWork[nWork] = pSc;
            nWork++;
        }

        //
        // We estimate how much work we have to do on each state, so that we can start on the largest ones
        // and be more efficient.
        //
        if( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_APPEND )
        {
            pSc->bytes += pOp->cbBuffer;
        } else if( pOp->hashOperation == SYMCRYPT_HASH_OPERATION_RESULT )
        {
            //
            // The result could be a 1 or 2-block operation; but it is mostly a 1-block one so that is what we budget.
            //
            pSc->bytes += pHash->inputBlockSize;
        } else {
            scError = SYMCRYPT_INVALID_ARGUMENT;
            goto cleanup;
        }

        //
        // Add the operation to the list of operations for this state
        //
        pOp->next = pSc->next;
        pSc->next = pOp;
    }

    //
    // We have built all the structures.
    // Run the SetNextWork on each of them, and drop the ones that don't have work.
    // Also detect whether they are all the same size so that we can avoid the sorting cost.
    //
    SYMCRYPT_ASSERT( nWork > 0 );
    singleSize = (*pWork)->bytes;
    sameSize = TRUE;
    i = 0;
    while( i < nWork )
    {
        if( !SymCryptParallelHashSetNextWork( pParHash, pWork[i] ) )
        {
            pWork[i] = pWork[nWork-1];
            nWork--;
            continue;
        }

        if( pWork[i]->bytes != singleSize )
        {
            sameSize = FALSE;
        }
        i++;
    }

    if( !sameSize )
    {
        qsort( pWork, nWork, sizeof( *pWork ), &compareRequestSize );
    }

    nPar = SYMCRYPT_MIN( nWork, maxParallel );  // # parallel states we currently work on
    pNextWork = pWork + nPar;        // next work pointer.

    while( nWork > 0 )
    {
        todo = pWork[0]->cbData;
        for( i=1; i<nPar; i++ )
        {
            todo = SYMCRYPT_MIN( todo, pWork[i]->cbData );
        }

        nBytes = todo & ~((SIZE_T)(pHash->inputBlockSize - 1));

        (*pParHash->parAppendFunc)( pWork, nPar, nBytes, pbFixedScratch, cbFixedScratch );

        for( i=0; i<nPar; i++ )
        {
            if( pWork[i]->cbData < pHash->inputBlockSize )
            {
                //
                // Once we start a request we finish it; this is not optimal.
                // It would be better to switch things around a bit, but that is much more complicated.
                // Example: suppose we can do 4-parallel and have requests of size
                //  9 8 7 6 6 6
                // Our code does
                //    Process first 4 of    # blocks      Resulting state
                //      9 8 7 6 / 6 6           6           3 2 1 - / 6 6
                //      6 3 2 1 / 6             1           5 2 1 0 / 6
                //     6 more to finish for a total of 13 blocks.
                //
                // Better would be:
                //    Process first 4 of    # blocks      Resulting state
                //      9 8 7 6 / 6 6           6           3 2 1 - / 6 6
                //      6 6 3 2 / 1 -           2           4 4 1 0 / 1
                //    4 more to finish for total of 12 blocks.
                //
                // Or even better:
                //    Process first 4 of    # blocks      Resulting state
                //      9 8 7 6 / 6 6           5           4 3 2 1 / 6 6
                //      6 6 4 3 / 2 1           3           3 3 1 - / 2 1
                //      3 3 2 1 / 1 -           1           2 2 1 - / 1 -
                //  2 more to finish for a total of 11 blocks.
                // Note that this last one requires the interruption of a started hash computation.
                //

                if( !SymCryptParallelHashSetNextWork( pParHash, pWork[i] ))
                {
                    if( nWork > nPar )
                    {
                        pWork[i] = *pNextWork++;
                        nWork--;
                    } else {
                        //
                        // Ugly: copy the last item here, and wind back the loop counter
                        // by one so that we will process the last item again.
                        //
                        pWork[i] = pWork[ --nPar ];
                        i--;
                        nWork--;
                    }
                }
            }
        }
    }
    SymCryptWipe( pbFixedScratch, cbFixedScratch );

cleanup:
    return scError;
}
