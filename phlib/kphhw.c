/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <ph.h>
#include <kphhw.h>
#include <sistatus.h>

typedef struct _KPH_HW_SCRIPT
{
    PH_QUEUED_LOCK Lock;
    HANDLE Handle;
    ULONG Flags;
    PCPH_STRINGREF FileName;
} KPH_HW_SCRIPT, *PKPH_HW_SCRIPT;

static CONST PH_STRINGREF KphpHwCpuFileName = PH_STRINGREF_INIT(L"scripts\\cpu.bin");
static CONST PH_STRINGREF KphpHwSuperIoFileName = PH_STRINGREF_INIT(L"scripts\\superio.bin");
static KPH_HW_SCRIPT KphpHwCpuScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_VOLATILE, &KphpHwCpuFileName };
static KPH_HW_SCRIPT KphpHwSuperIoScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_VOLATILE, &KphpHwSuperIoFileName };

/**
 * Retrieves the handle of a script, loading the script on first use.
 *
 * \param Script The script.
 * \param ScriptHandle Receives the handle. The script is held shared while the
 * handle is in use; release it with PhReleaseQueuedLockShared.
 * \return Successful or errant status.
 */
static NTSTATUS KphpHwAcquireScript(
    _Inout_ PKPH_HW_SCRIPT Script,
    _Out_ PHANDLE ScriptHandle
    )
{
    NTSTATUS status;
    PPH_STRING fileName;

    for (;;)
    {
        PhAcquireQueuedLockShared(&Script->Lock);

        if (Script->Handle)
        {
            *ScriptHandle = Script->Handle;
            return STATUS_SUCCESS;
        }

        PhReleaseQueuedLockShared(&Script->Lock);

        PhAcquireQueuedLockExclusive(&Script->Lock);

        if (Script->Handle)
        {
            status = STATUS_SUCCESS;
        }
        else if (!(fileName = PhGetApplicationDirectoryFileName(Script->FileName, TRUE)))
        {
            status = STATUS_NO_SUCH_FILE;
        }
        else
        {
            status = KphLoadScript(&Script->Handle, &fileName->sr, Script->Flags);
            PhDereferenceObject(fileName);
        }

        PhReleaseQueuedLockExclusive(&Script->Lock);

        if (!NT_SUCCESS(status))
            return status;
    }
}

/**
 * Closes the handle of a script if it is still the given one.
 *
 * \param Script The script.
 * \param ScriptHandle The handle a call failed with.
 */
static VOID KphpHwCloseScript(
    _Inout_ PKPH_HW_SCRIPT Script,
    _In_ HANDLE ScriptHandle
    )
{
    PhAcquireQueuedLockExclusive(&Script->Lock);

    if (Script->Handle == ScriptHandle)
    {
        KphClose(Script->Handle);
        Script->Handle = NULL;
    }

    PhReleaseQueuedLockExclusive(&Script->Lock);
}

static NTSTATUS KphpHwScriptCall(
    _Inout_ PKPH_HW_SCRIPT Script,
    _In_ PANSI_STRING Routine,
    _In_reads_bytes_opt_(InputLength) PVOID Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_opt_(OutputLength) PVOID Output,
    _In_ ULONG OutputLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    HANDLE scriptHandle;

    *ReturnLength = 0;

    for (ULONG attempt = 0; attempt < 2; attempt++)
    {
        if (!NT_SUCCESS(status = KphpHwAcquireScript(Script, &scriptHandle)))
            return status;

        status = KphScriptCall(
            scriptHandle,
            Routine,
            Input,
            InputLength,
            Output,
            OutputLength,
            ReturnLength
            );

        PhReleaseQueuedLockShared(&Script->Lock);

        if (status != STATUS_SI_SCRIPT_FAULTED && status != STATUS_INVALID_HANDLE)
            break;

        KphpHwCloseScript(Script, scriptHandle);
    }

    return status;
}

/**
 * Chooses the mode scripts are loaded in from now on, and closes the scripts
 * that are loaded.
 *
 * \param Flags KPH_SCRIPT_VOLATILE or KPH_SCRIPT_NON_VOLATILE.
 */
VOID KphHwSetScriptMode(
    _In_ ULONG Flags
    )
{
    PKPH_HW_SCRIPT scripts[] = { &KphpHwCpuScript, &KphpHwSuperIoScript };

    for (ULONG i = 0; i < RTL_NUMBER_OF(scripts); i++)
    {
        PhAcquireQueuedLockExclusive(&scripts[i]->Lock);

        scripts[i]->Flags = Flags;

        if (scripts[i]->Handle)
        {
            KphClose(scripts[i]->Handle);
            scripts[i]->Handle = NULL;
        }

        PhReleaseQueuedLockExclusive(&scripts[i]->Lock);
    }
}

/**
 * Identifies the processor.
 *
 * \param Identity Receives the identity.
 * \return Successful or errant status.
 */
NTSTATUS KphHwCpuIdentify(
    _Out_ PKPH_HW_CPU_IDENTITY Identity
    )
{
    NTSTATUS status;
    ANSI_STRING routine;
    KPH_HW_CPU_IDENTITY identity;
    ULONG returnLength;

    RtlInitAnsiString(&routine, "identify");

    status = KphpHwScriptCall(
        &KphpHwCpuScript,
        &routine,
        NULL,
        0,
        &identity,
        sizeof(identity),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(identity))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Reads model-specific registers, each on the processor its item names.
 * Items for the same processor must be adjacent; the script pins once per run
 * of the same processor. The helper does not sort.
 *
 * \param Items The registers to read.
 * \param Count The number of items.
 * \param Values Receives a status and a value per item, in the order of the
 * items.
 * \return Successful or errant status.
 */
NTSTATUS KphHwCpuRead(
    _In_reads_(Count) PCKPH_HW_CPU_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    )
{
    NTSTATUS status;
    ANSI_STRING routine;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PKPH_HW_VALUE64 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_CPU_ITEM), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE64), &outputLength)))
        return status;

    RtlInitAnsiString(&routine, "read");

    values = PhAllocate(outputLength ? outputLength : 1);

    status = KphpHwScriptCall(
        &KphpHwCpuScript,
        &routine,
        (PVOID)Items,
        inputLength,
        values,
        outputLength,
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength != outputLength)
        status = STATUS_INFO_LENGTH_MISMATCH;

    if (NT_SUCCESS(status))
        RtlCopyMemory(Values, values, outputLength);

    PhFree(values);

    return status;
}

typedef enum _KPH_HW_BUS
{
    KphHwBusPci,
    KphHwBusIsa,
    KphHwBusSmbus,
    KphHwBusMaximum
} KPH_HW_BUS;

#define KPH_HW_BUS_TIMEOUT 100

static CONST PH_STRINGREF KphpHwBusNames[KphHwBusMaximum] =
{
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_PCI"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_ISABUS.HTP.Method"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_SMBUS.HTP.Method"),
};
static PH_INITONCE KphpHwBusInitOnce[KphHwBusMaximum] = { PH_INITONCE_INIT, PH_INITONCE_INIT, PH_INITONCE_INIT };
static HANDLE KphpHwBusHandles[KphHwBusMaximum] = { NULL, NULL, NULL };
static NTSTATUS KphpHwBusStatus[KphHwBusMaximum] = { STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS };

/**
 * Creates or opens a mutant hardware monitors share to serialize a bus.
 *
 * \details A created mutant grants SYSTEM and Administrators full access and
 * Everyone only the rights to wait on and query it.
 *
 * \param MutantHandle Receives the handle.
 * \param ObjectName The full name of the mutant.
 * \return Successful or errant status.
 */
static NTSTATUS KphpHwCreateBusMutant(
    _Out_ PHANDLE MutantHandle,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    UCHAR securityDescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH + sizeof(ACL) + (3 * (sizeof(ACCESS_ALLOWED_ACE) + SECURITY_MAX_SID_SIZE))];
    PSECURITY_DESCRIPTOR securityDescriptor = (PSECURITY_DESCRIPTOR)securityDescriptorBuffer;
    PACL dacl = (PACL)PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE mutantHandle;
    NTSTATUS status;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    if (!NT_SUCCESS(status = PhCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION)))
        return status;
    if (!NT_SUCCESS(status = PhCreateAcl(dacl, sizeof(ACL) + (3 * (sizeof(ACCESS_ALLOWED_ACE) + SECURITY_MAX_SID_SIZE)), ACL_REVISION)))
        return status;
    if (!NT_SUCCESS(status = PhAddAccessAllowedAce(dacl, ACL_REVISION, MUTANT_ALL_ACCESS, (PSID)&PhSeLocalSystemSid)))
        return status;
    if (!NT_SUCCESS(status = PhAddAccessAllowedAce(dacl, ACL_REVISION, MUTANT_ALL_ACCESS, PhSeAdministratorsSid())))
        return status;
    if (!NT_SUCCESS(status = PhAddAccessAllowedAce(dacl, ACL_REVISION, SYNCHRONIZE | READ_CONTROL | MUTANT_QUERY_STATE, (PSID)&PhSeEveryoneSid)))
        return status;
    if (!NT_SUCCESS(status = PhSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE)))
        return status;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_OPENIF | OBJ_DONT_REPARSE,
        NULL,
        securityDescriptor
        );

    status = NtCreateMutant(
        &mutantHandle,
        SYNCHRONIZE | MUTANT_QUERY_STATE,
        &objectAttributes,
        FALSE
        );

    // A tool that created the mutant with a narrower DACL may still grant SYNCHRONIZE.
    if (status == STATUS_ACCESS_DENIED)
        status = NtOpenMutant(&mutantHandle, SYNCHRONIZE, &objectAttributes);

    if (NT_SUCCESS(status))
        *MutantHandle = mutantHandle;

    return status;
}

/**
 * Acquires the mutant hardware monitors share to serialize a bus.
 *
 * \details The mutant is created or opened the first time the bus is
 * acquired. An abandoned mutant counts as acquired.
 *
 * \param Bus The bus.
 * \param Milliseconds The longest time to wait.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus past the time.
 */
static NTSTATUS KphpHwAcquireBus(
    _In_ KPH_HW_BUS Bus,
    _In_ ULONG Milliseconds
    )
{
    LARGE_INTEGER timeout;
    NTSTATUS status;

    if (PhBeginInitOnce(&KphpHwBusInitOnce[Bus]))
    {
        KphpHwBusStatus[Bus] = KphpHwCreateBusMutant(&KphpHwBusHandles[Bus], &KphpHwBusNames[Bus]);
        PhEndInitOnce(&KphpHwBusInitOnce[Bus]);
    }

    if (!NT_SUCCESS(KphpHwBusStatus[Bus]))
        return KphpHwBusStatus[Bus];

    status = NtWaitForSingleObject(KphpHwBusHandles[Bus], FALSE, PhTimeoutFromMilliseconds(&timeout, Milliseconds));

    if (status == STATUS_WAIT_0 || status == STATUS_ABANDONED_WAIT_0)
        return STATUS_SUCCESS;
    if (status == STATUS_TIMEOUT)
        return STATUS_RETRY;

    return NT_SUCCESS(status) ? STATUS_UNSUCCESSFUL : status;
}

/**
 * Releases a bus acquired with KphpHwAcquireBus.
 *
 * \param Bus The bus.
 */
static VOID KphpHwReleaseBus(
    _In_ KPH_HW_BUS Bus
    )
{
    NT_VERIFY(NT_SUCCESS(NtReleaseMutant(KphpHwBusHandles[Bus], NULL)));
}

/**
 * Identifies the Super I/O chip at a configuration port.
 *
 * \param Slot The configuration port: 0 for 2Eh, 1 for 4Eh.
 * \param Chip Receives the family, chip and customer.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSuperIoIdentify(
    _In_ ULONG Slot,
    _Out_ PKPH_HW_SUPERIO_CHIP Chip
    )
{
    NTSTATUS status;
    ANSI_STRING routine;
    KPH_HW_SUPERIO_CHIP chip;
    ULONG returnLength;

    RtlInitAnsiString(&routine, "identify");

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusIsa, KPH_HW_BUS_TIMEOUT)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwSuperIoScript,
        &routine,
        &Slot,
        sizeof(Slot),
        &chip,
        sizeof(chip),
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusIsa);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(chip))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Chip = chip;

    return status;
}

/**
 * Reads hardware monitor registers of the Super I/O chip at a configuration
 * port.
 *
 * \param Slot The configuration port: 0 for 2Eh, 1 for 4Eh.
 * \param Registers The registers to read.
 * \param Count The number of registers.
 * \param Values Receives a status and a value per register, in the order of
 * the registers.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSuperIoRead(
    _In_ ULONG Slot,
    _In_reads_(Count) PCULONG Registers,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    NTSTATUS status;
    ANSI_STRING routine;
    ULONG registersLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE32 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(ULONG), &registersLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(registersLength, sizeof(ULONG), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE32), &outputLength)))
        return status;

    RtlInitAnsiString(&routine, "read");

    input = PhAllocate(inputLength);
    input[0] = Slot;
    RtlCopyMemory(&input[1], Registers, registersLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    if (NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusIsa, KPH_HW_BUS_TIMEOUT)))
    {
        status = KphpHwScriptCall(
            &KphpHwSuperIoScript,
            &routine,
            input,
            inputLength,
            values,
            outputLength,
            &returnLength
            );

        KphpHwReleaseBus(KphHwBusIsa);

        if (NT_SUCCESS(status) && returnLength != outputLength)
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (NT_SUCCESS(status))
            RtlCopyMemory(Values, values, outputLength);
    }

    PhFree(values);
    PhFree(input);

    return status;
}
