//
// SymCrypt_winkernel.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

//
// This file contains definitions that are specific to the Windows kernel module.
// Note that this header requires inclusion of symcrypt.h to be compiled, whilst
// symcrypt_winkernel_types.h does not.
//

#pragma once

#include <symcrypt.h>
#include "symcrypt_winkernel_types.h"

//////////////////////////////////////////////////////////
//
// SymCryptEntropyAccumulator
//

//
// The struct and associated functions are exposed from the Windows Kernel SymCrypt module
// to enable certification of this entropy source within the module.
//

// Check at compile time that constants are powers of 2
C_ASSERT((SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE        & (SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE-1))        == 0);
C_ASSERT((SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT & (SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT-1)) == 0);
C_ASSERT((SYMCRYPT_ENTROPY_ACCUMULATOR_BUFFER_SIZE         & (SYMCRYPT_ENTROPY_ACCUMULATOR_BUFFER_SIZE-1))         == 0);
C_ASSERT((SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_BUFFER  & (SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_BUFFER-1))  == 0);

#define SYMCRYPT_FLAG_ENTROPY_ACCUMULATOR_ALLOW_RAW_SAMPLE_COLLECTION (0x01)

VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorInit0(
    _Out_   PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE pState );
//
// Step 0 initialization of a SYMCRYPT_ENTROPY_ACCUMULATOR_STATE.
// 
// - pState points to a preallocated SYMCRYPT_ENTROPY_ACCUMULATOR_STATE
//
// Note: This function calls KeInitializeDpc, so must be called only when this
// API is available.
//
// This call makes it safe to call SymCryptEntropyAccumulatorAccumulateSample
// with this state, but these calls will not affect the entropy output until
// the state is further initialized with SymCryptEntropyAccumulatorInit1.
//


SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEntropyAccumulatorInit1(
    _Inout_ PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE pState,
            UINT64                              config );
//
// Step 1 initialization of a SYMCRYPT_ENTROPY_ACCUMULATOR_STATE.
//
// - pState points to a preallocated SYMCRYPT_ENTROPY_ACCUMULATOR_STATE
//   which has already had SymCryptEntropyAccumulatorInit0 called on it
// - config is an opaque value read from the registry by the kernel to
//   control internal details about how the accumulator state is initialized
//
// Note: This function may call ExAllocatePool2, so must be called only when
// this allocator is available
//
// After a successful call, subsequent calls to SymCryptEntropyAccumulatorAccumulateSample
// may affect entropy output.
//
// Returns SYMCRYPT_NO_ERROR on successful initialization.
//

VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorAccumulateSample(
    _Inout_ PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE pState );
//
// Accumulate a timestamp counter into the given accumulator state. This may potentially trigger
// further processing in a DPC.
//

VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorGlobalInitFromRegistry( VOID );
//
// This function must be called once by the kernel when the registry is ready to be read and written
// to enable the collection if configured in the registry.
//

//
// Callback routine
//
typedef
VOID
(SYMCRYPT_CALL * PSYMCRYPT_CALLBACK_ENTROPY_ACCUMULATOR_PROVIDE_ENTROPY_FUNC) (
    _In_reads_( cbData )        PCBYTE                          pbData,
                                SIZE_T                          cbData,
                                UINT32                          entropyEstimateInMilliBits );
// The form of the callback function to be defined by our caller to process entropy produced
// by any initialized entropy accumulators
//
// - pbData is a pointer to a buffer of bytes containing entropy
// - cbData is the number of bytes in the buffer
// - entropyEstimateInMilliBits is the number of millibits of entropy that the entropy accumulator
//   asserts is in the entropy buffer

BOOLEAN
SYMCRYPT_CALL
SymCryptEntropyAccumulatorGlobalSetCallbackProvideEntropyFn(
    _In_ PSYMCRYPT_CALLBACK_ENTROPY_ACCUMULATOR_PROVIDE_ENTROPY_FUNC provideEntropyCallbackFn );
//
// Sets the callback function that all entropy accumulators initialized by the module will call into periodically
// to provide entropy buffers to the caller; see documentation of
// PSYMCRYPT_CALLBACK_ENTROPY_ACCUMULATOR_PROVIDE_ENTROPY_FUNC.
//
// This can only be set once, and returns TRUE when it is set.
//
