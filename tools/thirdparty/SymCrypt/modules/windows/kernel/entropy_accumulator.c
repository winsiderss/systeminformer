//
// entropy_accumulator.c
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include <ntddk.h>
#include "symcrypt_winkernel.h"
#include "sc_lib.h"


#define SYMCRYPT_ENTROPY_ACCUMULATOR_TAG                'aeCS'
#define SYMCRYPT_ENTROPY_ACCUMULATOR_KEY_NAME           (L"\\Registry\\Machine\\SYSTEM\\RNG\\EntropyAccumulator")
#define SYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG_VALUE_NAME  (L"Config")
#define SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_FILENAME    (L"\\SystemRoot\\EntropyAccumulator\\samples.dat")
#define SYMCRYPT_ENTROPY_ACCUMULATOR_MAX_SAMPLES        (1ULL << 27)

// Calculates the size of the buffer for storing raw samples plus conditioned samples
// 
// The first portion of the buffer consists of RawSamples many raw samples,
// and the second portion is the associated conditioned samples as blocks of segments.
// RawSamples must be a multiple of SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT,
// so that we store a complete block of raw samples with their conditioned forms.
#define SYMCRYPT_ENTROPY_COMBINED_BUFFER_SIZE(RawSamples) ( \
            (RawSamples * sizeof(SYMCRYPT_ENTROPY_RAW_SAMPLE)) + \
            (RawSamples / SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT) * SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE )

// Calculates the offset in the combined sample buffer where the conditioned samples will be written to
#define SYMCRYPT_ENTROPY_COMBINED_BUFFER_SEGMENT_OFFSET(RawSamples, SamplesProcessed) ( \
            (RawSamples * sizeof(SYMCRYPT_ENTROPY_RAW_SAMPLE)) + \
            (SamplesProcessed / SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT) * SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE )

// The next accumulator ID to be used when an entropy accumulator is initialized. Atomically incremented in each initialization.
UINT32 g_SymCryptEntropyAccumulatorNextId = 0;

// Callback function entropy accumulators call in DPC to provide entropy to other components
PSYMCRYPT_CALLBACK_ENTROPY_ACCUMULATOR_PROVIDE_ENTROPY_FUNC g_SymCryptCallbackEntropyAccumulatorProvideEntropy = NULL;

typedef struct _SYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE
{
    PUCHAR          pCombinedSampleBuffer;
    UINT64          nRawSamples;
    UINT64          accumulatorId;
    WORK_QUEUE_ITEM WorkQueueItem;
} SYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE, *PSYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE;

SYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE g_SymCryptEntropyAccumulatorLoggingState;


typedef enum _SYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG
{
    SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_UNINITIALIZED    = 0,
    SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_OFF              = 1,
    SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_ON               = 2,
} SYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG, * PSYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG;

// Sample collection configuration value, will be updated by SymCryptEntropyAccumulatorReadConfig
SYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG g_SymCryptEntropyAccumulatorConfig = SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_UNINITIALIZED;


VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorDpcRoutine(
    _In_        PKDPC   Dpc,
    _In_opt_    PVOID   Context,
    _In_        PVOID   SystemArgument1,
    _In_opt_    PVOID   SystemArgument2 );
    
VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorInit0(
    _Out_   PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE pState )
{
    SymCryptWipeKnownSize( pState, sizeof(*pState) );

    KeInitializeDpc(&pState->Dpc,
                    &SymCryptEntropyAccumulatorDpcRoutine,
                    pState);
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptEntropyAccumulatorInit1(
    _Out_   PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE pState,
            UINT64                              config )
{
    UINT32 accumulatorId = SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(&g_SymCryptEntropyAccumulatorNextId, 1);
    pState->accumulatorId = accumulatorId;

    // The intent is that we collect samples only for logical processor 0, when test signing is enabled, a regkey
    // with restricted control has a value with specific data, and we successfully delete the value.
    //
    // Unfortunately there is no public API we can call to query test signing which will be available at the time in
    // boot when this function will be called (NtQuerySystemInformation with SystemCodeIntegrityInformation cannot be
    // used at this point in boot, and also might not be stable across Windows versions).
    // Instead we rely on the kernel to pass us a config UINT64 value read from the registry.
    //
    // As we cannot query the registry yet when this function is called, we defer interacting with the registry to later,
    // when SymCryptEntropyAccumulatorGlobalInitFromRegistry is called.
    // At that point based on our interaction with the registry we may either mark all allocated sample buffers to
    // be wiped, or log them for certification or testing purposes.

    // Ensure that the number of samples is a multiple of SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT
    config &= ~(SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT - 1);

    // For now we just take the config value provided to us to indicate the number of samples to collect for logical processor 0.
    if( (accumulatorId == 0) && (config != 0) )
    {
        pState->nRawSamples = SYMCRYPT_MIN(config, SYMCRYPT_ENTROPY_ACCUMULATOR_MAX_SAMPLES);
        pState->pCombinedSampleBuffer = ExAllocatePool2(
                                            POOL_FLAG_NON_PAGED, 
                                            SYMCRYPT_ENTROPY_COMBINED_BUFFER_SIZE(pState->nRawSamples),
                                            SYMCRYPT_ENTROPY_ACCUMULATOR_TAG
                                            );
    }

    // Discard any interrupt data up to this point; we cannot use it because we cannot get raw samples for it
    // We reset here in case there are any interrupts in ExAllocatePool2.
    // In practice we discard low 10s of samples only for logical processor 0.
    SymCryptWipeKnownSize(pState->buffer, SYMCRYPT_ENTROPY_ACCUMULATOR_BUFFER_SIZE);

    // The first segment will be processed when nSamplesAccumulated == SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT
    //
    // The initial value of nSamplesAccumulated is biased by the accumulatorId in order
    // to avoid having a large number of processors deliver entropy at a single point in
    // time. This can cause issues on a large system where many processors do not receive
    // independent device interrupts and therefore remain in sync with respect to clock
    // interrupts or profiling interrupts.
    // The bias is a small value which effectively means the first segment processed
    // may not be full of entropy.
    pState->nSamplesAccumulated = (accumulatorId * 3) % SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT;
    pState->nHealthTestFailures = 0;
    pState->nDPCScheduleFailures = 0;

    return SYMCRYPT_NO_ERROR;
}

FORCEINLINE
UINT64
SYMCRYPT_CALL
SymCryptReadTimeStampCounter(void)
{
#if SYMCRYPT_CPU_AMD64
    return __rdtsc();
#elif SYMCRYPT_CPU_ARM64
    return (UINT64)_ReadStatusReg(ARM64_PMCCNTR_EL0);
#else
    #error Unexpected CPU (Only AMD64 / Arm64 supported in SymCrypt Kernel Module)
#endif
}

VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorAccumulateSample(
    _Inout_ PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE pState )
{
    UINT64 sample = SymCryptReadTimeStampCounter();
    UINT64 nSamplesAccumulated = pState->nSamplesAccumulated;

    //
    // Compute the UINT64-index at which to store the current sample.
    // The entropy accumulator code mixes 64 consecutive samples into a single UINT64.
    //
    SIZE_T bufferIndex = (nSamplesAccumulated & (SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_BUFFER - 1)) / 64;
    PUINT64 bufferTarget = ((PUINT64)(&pState->buffer[0])) + bufferIndex;

    *bufferTarget = ROR64(*bufferTarget, 19) ^ sample;

    //
    // Save raw sample to raw sample buffer if raw sample buffer exists.
    // This is only used in certification or testing.
    //
    if( pState->pCombinedSampleBuffer != NULL )
    {
        if( nSamplesAccumulated < pState->nRawSamples )
        {
            PSYMCRYPT_ENTROPY_RAW_SAMPLE pRawSample = ((PSYMCRYPT_ENTROPY_RAW_SAMPLE)(&pState->pCombinedSampleBuffer[0])) + nSamplesAccumulated;
            pRawSample->sampleIndex = nSamplesAccumulated;
            pRawSample->sampleValue = sample;
        }
    }

    //
    // Trigger entropy processing if sufficient samples have been collected to fill a segment.
    //
    nSamplesAccumulated++;

    if( (nSamplesAccumulated & (SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT - 1)) == 0 )
    {
        BOOLEAN successfullyScheduledDPC = FALSE;

        //
        // Schedule DPC to process the accumulated samples if there is no DPC in progress (expected case)
        //
        if( pState->dpcInProgress == FALSE )
        {
            // KeInsertQueueDpc returns TRUE on success; it should always succeed here, as we guarantee to
            // only try queuing the DPC if it's completed the previous call, but if we fail, we discard the
            // segment.
            successfullyScheduledDPC = KeInsertQueueDpc(&pState->Dpc, (PVOID) nSamplesAccumulated, NULL);
            pState->dpcInProgress = successfullyScheduledDPC;
        }

        if( !successfullyScheduledDPC )
        {
            //
            // If successfullyScheduledDPC is FALSE, this indicates that the DPC is already either queued or
            // is still running (i.e. the processing of a previous segment has not completed), or there was some
            // other failure in queueing the DPC.
            //
            // This is an unexpected case, as it is highly unlikely that 1024 interrupts occur before the
            // previous DPC completes. However, to make it easy to keep a 1:1 correspondence between
            // collected raw samples and the accumulated samples in the segment, we discard
            // the segment we just filled, and reuse it, starting from a zero-ed state.
            // We do this by resetting nSamplesAccumulated and wiping the segment.
            // If we are collecting raw samples, we do not need to wipe the raw sample buffer as
            // it is written to destructively, rather than accumulated into.
            //
            nSamplesAccumulated -= SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT;

            // Compute the byte-index we will next accumulate into; this is the start of segment we wish to wipe
            // As we know nSamplesAccumulated is a multiple of 128, we can just align to the nearest byte
            bufferIndex = (nSamplesAccumulated & (SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_BUFFER - 1)) / 8;

            SymCryptWipeKnownSize( &pState->buffer[bufferIndex], SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE );

            pState->nDPCScheduleFailures++;
        }
    }

    //
    // Store the updated sample count.
    //
    pState->nSamplesAccumulated = nSamplesAccumulated;
}

VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorWriteSamplesWorker(PVOID Context)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    UNICODE_STRING FilePath = { 0 };
    OBJECT_ATTRIBUTES oac = { 0 };
    IO_STATUS_BLOCK iostatus;
    LARGE_INTEGER byteOffset = { 0 };
    HANDLE hFile = NULL;

    PSYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE pLoggingState = (PSYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE)Context;

    RtlInitUnicodeString(&FilePath, SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_FILENAME);

    InitializeObjectAttributes(
        &oac,
        &FilePath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    ntStatus = ZwCreateFile(
        &hFile,
        FILE_GENERIC_WRITE,
        &oac,
        &iostatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_CREATE,
        FILE_SEQUENTIAL_ONLY | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (!NT_SUCCESS(ntStatus))
    {
        goto cleanup;
    }

    ntStatus = ZwWriteFile(
        hFile,
        NULL,
        NULL,
        NULL,
        &iostatus,
        (PVOID)pLoggingState->pCombinedSampleBuffer,
        (LONG)SYMCRYPT_ENTROPY_COMBINED_BUFFER_SIZE(pLoggingState->nRawSamples),
        &byteOffset,
        NULL);

cleanup:

    if (hFile)
    {
        ZwClose(hFile);
    }

    // We can release the sample buffer memory here as we saved the samples to disk.
    // The actual buffer pointer inside the SYMCRYPT_ENTROPY_ACCUMULATOR_STATE was set to NULL in the DPC.
    ExFreePoolWithTag(pLoggingState->pCombinedSampleBuffer, SYMCRYPT_ENTROPY_ACCUMULATOR_TAG);
}

VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorLogSamples(
    _In_   PSYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE pState)
{
// ExInitializeWorkItem and ExQueueWorkItem are marked as deprecated
#pragma warning(push)
#pragma warning(disable : 4996)
    ExInitializeWorkItem(
        &pState->WorkQueueItem,
        SymCryptEntropyAccumulatorWriteSamplesWorker,
        (PVOID)pState
    );

    ExQueueWorkItem(&pState->WorkQueueItem, DelayedWorkQueue);
#pragma warning(pop)
}



// For entropy estimation purposes, we estimate 6 bits of entropy per byte of the buffer.
// This corresponds to each raw sample having a little more than 0.75 bits of entropy.
// This value can be tweaked later based on observed data and what is allowed by health tests.
#define SYMCRYPT_ENTROPY_ACCUMULATOR_ENTROPY_ESTIMATE_PER_BYTE (6000)

VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorDpcRoutine(
    _In_        PKDPC   Dpc,
    _In_opt_    PVOID   Context,
    _In_        PVOID   SystemArgument1,
    _In_opt_    PVOID   SystemArgument2 )
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument2);

    PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE pState = (PSYMCRYPT_ENTROPY_ACCUMULATOR_STATE)Context;
    const UINT64 nSamplesAtDpcQueueTime = (UINT64)SystemArgument1;
    const UINT64 nSamplesProcessed = nSamplesAtDpcQueueTime - SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT;
    SIZE_T segmentId;
    SIZE_T i;
    PUINT64 pu64SampleSegment;
    UINT32 entropyEstimateInMilliBits = SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE * SYMCRYPT_ENTROPY_ACCUMULATOR_ENTROPY_ESTIMATE_PER_BYTE;

    // 
    // Conceptually we have a series of up to 2^(54) accumulators, each being filled by accumulating 2^(10) consecutive samples.
    // In reality we keep 1 actual buffer consisting of 2 segments at any given time.
    //
    // As this DPC runs at a lower IRQL (and could even run on a different logical processor) than the sample accumulation
    // logic, it is possible that in a scenario with a large number of interrupts, we do not complete this DPC before the other
    // segment is filled. If this ever happens, the following segment is simply discarded and refilled.
    //

    SYMCRYPT_ASSERT( nSamplesAtDpcQueueTime >= SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT );
    SYMCRYPT_ASSERT( (nSamplesAtDpcQueueTime & (SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT - 1)) == 0 );

    //
    // Compute the starting offset of the segment we will process
    //
    segmentId = (nSamplesProcessed / SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT) %
                        SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_COUNT;
    pu64SampleSegment = (PUINT64) &pState->buffer[segmentId * SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE];

    //
    // FIPS health tests
    //
    // The raw samples are 64-bit counter values which monotonically increase.
    // Segments are initialized to 0, and each 64-bit value within the segment is constructed
    // by rotation and exclusive-or of 64 consecutive counter values.
    //
    // We can guarantee to detect if any counter value is repeated 127 times consecutively, by checking if
    // each 64-bit value is 0 or -1. If a repeated counter value has an even number of set bits, then one
    // 64-bit value in the segment will take a value of 0. If the repeated counter has an odd number of set
    // bits then one 64-bit value in the segment will take a value of -1.
    //
    // This corresponds to a Developer-Defined Alternative continuous health test with:
    // a) The probability of detecting a single value appearing consecutively more than ceil(100/0.75) = 133 > 127 being
    // 100% (with a ~2^-63 false positive rate)
    // b) Given the counter is monotonic, the only way the probability of a specific sample being observed would increase
    // is if the counter was stuck. If a stuck counter has as 2^(-0.375) probability of being observed across 50000
    // consecutive samples then we are talking about 1000s of consecutive samples having this specific value - again much
    // greater than 127 that we are _guaranteed_ to detect. So we also have 100% chance of detecting this condition.
    //

    // 
    // If there are any 64-bit 0 or -1 values in the buffer, then mark the buffer as having no entropy to indicate failure.
    //
    for(i = 0; i < (SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE / sizeof(UINT64)); i++)
    {
        if( pu64SampleSegment[i] + 1 <= 1 )
        {
            entropyEstimateInMilliBits = 0;
            pState->nHealthTestFailures++;
            break;
        }
    }

    //
    // If sample logging is enabled, check if we've collected sufficient number of
    // samples and start the logging process if so.
    //
    if (pState->pCombinedSampleBuffer != NULL)
    {
        SYMCRYPT_ASSERT( (pState->nRawSamples & (SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLES_PER_SEGMENT - 1)) == 0 );

        if (g_SymCryptEntropyAccumulatorConfig == SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_ON)
        {
            //
            // Copy conditioned samples to sample buffer
            //
            RtlCopyMemory(
                &pState->pCombinedSampleBuffer[SYMCRYPT_ENTROPY_COMBINED_BUFFER_SEGMENT_OFFSET(pState->nRawSamples, nSamplesProcessed)],
                pu64SampleSegment,
                SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE );

            if (nSamplesAtDpcQueueTime >= pState->nRawSamples)
            {
                PSYMCRYPT_ENTROPY_ACCUMULATOR_LOGGING_STATE pLogState = &g_SymCryptEntropyAccumulatorLoggingState;
                pLogState->accumulatorId = pState->accumulatorId;
                pLogState->nRawSamples = pState->nRawSamples;
                pLogState->pCombinedSampleBuffer = pState->pCombinedSampleBuffer;

                pState->pCombinedSampleBuffer = NULL; // Only log samples once!

                SymCryptEntropyAccumulatorLogSamples(pLogState);
            }
        }
        else if (g_SymCryptEntropyAccumulatorConfig == SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_OFF)
        {
            // In this case the config we were passed at SymCryptEntropyAccumulatorInit time indicated we should 
            // allocate a sample buffer, however we weren't able to successfully delete the registry value in 
            // SymCryptEntropyAccumulatorGlobalInitFromRegistry. We should not collect samples when this is the
            // case.
            //
            // We simply discard the pointer to our sample buffer, though we technically leak the sample buffer in this
            // case, freeing it would introduce more complexity than is worth it given that we should not allocate at all
            // in the real world.
            //
            // We also wipe the buffer here. This does not guarantee that the unused raw sample buffer is completely
            // zero-ed for the rest of time, as we may race with concurrent interrupts, but the window of time in which
            // raw samples can be preserved in the unused buffer is shortened significantly.
            PUCHAR pSampleBufferToWipe = pState->pCombinedSampleBuffer;
            pState->pCombinedSampleBuffer = NULL;
            SymCryptWipe(pSampleBufferToWipe, SYMCRYPT_ENTROPY_COMBINED_BUFFER_SIZE(pState->nRawSamples));
        }
    }

    // If entropy pool ready (callback is set), feed the segment into it
    if( g_SymCryptCallbackEntropyAccumulatorProvideEntropy )
    {
        g_SymCryptCallbackEntropyAccumulatorProvideEntropy(
            (PCBYTE)pu64SampleSegment,
            SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE,
            entropyEstimateInMilliBits );
    }

    // Zero the segment
    SymCryptWipeKnownSize(pu64SampleSegment, SYMCRYPT_ENTROPY_ACCUMULATOR_SEGMENT_SIZE);

    // Indicate that the DPC processing is complete
    // Use volatile write to ensure compiler doesn't reorder this write w.r.t. the wipe above
    SYMCRYPT_INTERNAL_VOLATILE_WRITE8( &pState->dpcInProgress, FALSE );
}

//
// Read and delete the SymCrypt Entropy Accumulator configuration value
// 
// Currently, this value is used to determine whether sample collection is enabled.
// It is read by the OS Loader and passed to SymCryptEntropyAccumulatorInit1 function
// by the kernel, here we only try to delete it.
// 
// pConfig parameter is a pointer to the variable that will be set to
// SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_ON if the configuration
// registry value exists and it can be deleted. Otherwise, it will be set to
// SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_OFF.
//
VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorReadConfig(
    _Out_ PSYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG pConfig
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG AttributeFlags = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE | OBJ_FORCE_ACCESS_CHECK;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKey = NULL;
    UNICODE_STRING SubKeyName;
    UNICODE_STRING ValueName;
    SYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG ConfigValue = SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_OFF;

    RtlInitUnicodeString(&SubKeyName, SYMCRYPT_ENTROPY_ACCUMULATOR_KEY_NAME);
    RtlInitUnicodeString(&ValueName, SYMCRYPT_ENTROPY_ACCUMULATOR_CONFIG_VALUE_NAME);

    InitializeObjectAttributes(
        &ObjectAttributes, &SubKeyName,
        AttributeFlags,
        NULL, NULL);

    // Fail if we can't open the Entropy Analysis subkey
    status = ZwOpenKey(&hKey, KEY_READ | KEY_WRITE | READ_CONTROL | DELETE, &ObjectAttributes);
    if(!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    // Fail if we can't delete the enable value
    status = ZwDeleteValueKey(hKey, &ValueName);
    if(!NT_SUCCESS(status))
    {
        goto cleanup;
    }

    ConfigValue = SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_ON;

cleanup:
    if(hKey)
    {
        ZwClose(hKey);
    }

    *pConfig = ConfigValue;
}


VOID
SYMCRYPT_CALL
SymCryptEntropyAccumulatorGlobalInitFromRegistry( VOID )
{
    if( g_SymCryptEntropyAccumulatorConfig == SYMCRYPT_ENTROPY_ACCUMULATOR_SAMPLE_COLLECTION_UNINITIALIZED )
    {
        //
        // Read config to determine whether samples should be collected
        //
        SymCryptEntropyAccumulatorReadConfig(&g_SymCryptEntropyAccumulatorConfig);
    }
}

BOOLEAN
SYMCRYPT_CALL
SymCryptEntropyAccumulatorGlobalSetCallbackProvideEntropyFn(
    _In_ PSYMCRYPT_CALLBACK_ENTROPY_ACCUMULATOR_PROVIDE_ENTROPY_FUNC provideEntropyCallbackFn )
{
    // The provide entropy callback function can only be set once
    if( g_SymCryptCallbackEntropyAccumulatorProvideEntropy != NULL )
    {
        return FALSE;
    }

    //
    // Set the global pointer to the callback function
    //
    g_SymCryptCallbackEntropyAccumulatorProvideEntropy = provideEntropyCallbackFn;

    return TRUE;
}
