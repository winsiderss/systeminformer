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
#include <cpusensor.h>
#include <sistatus.h>

typedef struct _KPH_HW_SCRIPT
{
    PH_QUEUED_LOCK Lock;
    HANDLE Handle;
    ULONG Flags;
    PCPH_STRINGREF FileName;
    NTSTATUS MissingStatus;
} KPH_HW_SCRIPT, *PKPH_HW_SCRIPT;

static CONST PH_STRINGREF KphpHwCpuFileName = PH_STRINGREF_INIT(L"scripts\\cpu.bin");
static CONST PH_STRINGREF KphpHwSuperIoFileName = PH_STRINGREF_INIT(L"scripts\\superio.bin");
static CONST PH_STRINGREF KphpHwAcpiFileName = PH_STRINGREF_INIT(L"scripts\\acpi.bin");
static CONST PH_STRINGREF KphpHwSmnFileName = PH_STRINGREF_INIT(L"scripts\\smn.bin");
static CONST PH_STRINGREF KphpHwSmuFileName = PH_STRINGREF_INIT(L"scripts\\smu.bin");
static CONST PH_STRINGREF KphpHwSmbusFileName = PH_STRINGREF_INIT(L"scripts\\smbus.bin");
static CONST PH_STRINGREF KphpHwPciMmioFileName = PH_STRINGREF_INIT(L"scripts\\pcimmio.bin");
static CONST PH_STRINGREF KphpHwHsmpFileName = PH_STRINGREF_INIT(L"scripts\\hsmp.bin");
static CONST PH_STRINGREF KphpHwPlatformFileName = PH_STRINGREF_INIT(L"scripts\\platform.bin");
static CONST PH_STRINGREF KphpHwPciConfigFileName = PH_STRINGREF_INIT(L"scripts\\pciconfig.bin");
static CONST PH_STRINGREF KphpHwCrosEcFileName = PH_STRINGREF_INIT(L"scripts\\crosec.bin");
static CONST PH_STRINGREF KphpHwVsecFileName = PH_STRINGREF_INIT(L"scripts\\vsec.bin");
static CONST PH_STRINGREF KphpHwEcFileName = PH_STRINGREF_INIT(L"scripts\\ec.bin");
static CONST PH_STRINGREF KphpHwTsensFileName = PH_STRINGREF_INIT(L"scripts\\tsens.bin");
static CONST PH_STRINGREF KphpHwSpbmFileName = PH_STRINGREF_INIT(L"scripts\\spbm.bin");
static CONST PH_STRINGREF KphpHwGpdFileName = PH_STRINGREF_INIT(L"scripts\\gpd.bin");
static CONST PH_STRINGREF KphpHwLenovoWsFileName = PH_STRINGREF_INIT(L"scripts\\lenovows.bin");
static CONST PH_STRINGREF KphpHwDellFileName = PH_STRINGREF_INIT(L"scripts\\dell.bin");
static CONST PH_STRINGREF KphpHwAsusWmiFileName = PH_STRINGREF_INIT(L"scripts\\asuswmi.bin");
static CONST PH_STRINGREF KphpHwGigabyteFileName = PH_STRINGREF_INIT(L"scripts\\gigabytewmi.bin");
static CONST PH_STRINGREF KphpHwAlienwareFileName = PH_STRINGREF_INIT(L"scripts\\alienware.bin");
static CONST PH_STRINGREF KphpHwHpFileName = PH_STRINGREF_INIT(L"scripts\\hpwmi.bin");
static CONST PH_STRINGREF KphpHwProm21FileName = PH_STRINGREF_INIT(L"scripts\\prom21.bin");
static CONST PH_STRINGREF KphpHwOcMailboxFileName = PH_STRINGREF_INIT(L"scripts\\ocmailbox.bin");
static CONST PH_STRINGREF KphpHwUsbDiskFileName = PH_STRINGREF_INIT(L"scripts\\usbdisk.bin");
static CONST PH_STRINGREF KphpHwHidFileName = PH_STRINGREF_INIT(L"scripts\\hid.bin");
static CONST PH_STRINGREF KphpHwAxiFileName = PH_STRINGREF_INIT(L"scripts\\axi.bin");
static CONST PH_STRINGREF KphpHwAqtionFileName = PH_STRINGREF_INIT(L"scripts\\aqtion.bin");

static KPH_HW_SCRIPT KphpHwCpuScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwCpuFileName };
static KPH_HW_SCRIPT KphpHwSuperIoScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwSuperIoFileName };
static KPH_HW_SCRIPT KphpHwAcpiScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwAcpiFileName };
static KPH_HW_SCRIPT KphpHwSmnScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwSmnFileName };
static KPH_HW_SCRIPT KphpHwSmuScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwSmuFileName };
static KPH_HW_SCRIPT KphpHwSmbusScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwSmbusFileName };
static KPH_HW_SCRIPT KphpHwPciMmioScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwPciMmioFileName };
static KPH_HW_SCRIPT KphpHwHsmpScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwHsmpFileName };
static KPH_HW_SCRIPT KphpHwPlatformScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwPlatformFileName };
static KPH_HW_SCRIPT KphpHwPciConfigScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwPciConfigFileName };
static KPH_HW_SCRIPT KphpHwCrosEcScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwCrosEcFileName };
static KPH_HW_SCRIPT KphpHwVsecScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwVsecFileName };
static KPH_HW_SCRIPT KphpHwEcScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwEcFileName };
static KPH_HW_SCRIPT KphpHwTsensScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwTsensFileName };
static KPH_HW_SCRIPT KphpHwSpbmScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwSpbmFileName };
static KPH_HW_SCRIPT KphpHwGpdScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwGpdFileName };
static KPH_HW_SCRIPT KphpHwLenovoWsScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwLenovoWsFileName };
static KPH_HW_SCRIPT KphpHwDellScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwDellFileName };
static KPH_HW_SCRIPT KphpHwAsusWmiScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwAsusWmiFileName };
static KPH_HW_SCRIPT KphpHwGigabyteScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwGigabyteFileName };
static KPH_HW_SCRIPT KphpHwAlienwareScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwAlienwareFileName };
static KPH_HW_SCRIPT KphpHwHpScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwHpFileName };
static KPH_HW_SCRIPT KphpHwProm21Script = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwProm21FileName };
static KPH_HW_SCRIPT KphpHwOcMailboxScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwOcMailboxFileName };
static KPH_HW_SCRIPT KphpHwUsbDiskScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwUsbDiskFileName };
static KPH_HW_SCRIPT KphpHwHidScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwHidFileName };
static KPH_HW_SCRIPT KphpHwAxiScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwAxiFileName };
static KPH_HW_SCRIPT KphpHwAqtionScript = { PH_QUEUED_LOCK_INIT, NULL, KPH_SCRIPT_NON_VOLATILE, &KphpHwAqtionFileName };

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

        status = Script->MissingStatus;

        PhReleaseQueuedLockShared(&Script->Lock);

        if (!NT_SUCCESS(status))
            return status;

        PhAcquireQueuedLockExclusive(&Script->Lock);

        if (Script->Handle)
        {
            status = STATUS_SUCCESS;
        }
        else if (!NT_SUCCESS(Script->MissingStatus))
        {
            status = Script->MissingStatus;
        }
        else if (!(fileName = PhGetApplicationDirectoryFileName(Script->FileName, TRUE)))
        {
            status = STATUS_NO_SUCH_FILE;
        }
        else
        {
            status = KphLoadScript(&Script->Handle, &fileName->sr, Script->Flags);
            PhDereferenceObject(fileName);

            if (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND)
                Script->MissingStatus = status;
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
    _In_ PCANSI_STRING Routine,
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
    PKPH_HW_SCRIPT scripts[] =
    {
        &KphpHwCpuScript,
        &KphpHwSuperIoScript,
        &KphpHwAcpiScript,
        &KphpHwSmnScript,
        &KphpHwSmuScript,
        &KphpHwSmbusScript,
        &KphpHwPciMmioScript,
        &KphpHwHsmpScript,
        &KphpHwPlatformScript,
        &KphpHwPciConfigScript,
        &KphpHwCrosEcScript,
        &KphpHwEcScript,
        &KphpHwVsecScript,
        &KphpHwTsensScript,
        &KphpHwSpbmScript,
        &KphpHwGpdScript,
        &KphpHwLenovoWsScript,
        &KphpHwDellScript,
        &KphpHwAsusWmiScript,
        &KphpHwGigabyteScript,
        &KphpHwAlienwareScript,
        &KphpHwHpScript,
        &KphpHwProm21Script,
        &KphpHwOcMailboxScript,
        &KphpHwUsbDiskScript,
        &KphpHwHidScript,
        &KphpHwAxiScript,
        &KphpHwAqtionScript,
    };

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
 * Identifies a processor, on that processor.
 *
 * \param Group The processor group.
 * \param Number The processor number within the group.
 * \param Identity Receives the identity. On ARM64 the signature is the
 * processor's MIDR and the other fields are zero.
 * \return Successful or errant status.
 */
NTSTATUS KphHwCpuIdentify(
    _In_ USHORT Group,
    _In_ USHORT Number,
    _Out_ PKPH_HW_CPU_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    USHORT processor[2];
    KPH_HW_CPU_IDENTITY identity;
    ULONG returnLength;

    processor[0] = Group;
    processor[1] = Number;

    status = KphpHwScriptCall(
        &KphpHwCpuScript,
        &routine,
        processor,
        sizeof(processor),
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
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PKPH_HW_VALUE64 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_CPU_ITEM), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE64), &outputLength)))
        return status;

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
    KphHwBusIntelOcMailbox,
    KphHwBusEc,
    KphHwBusDimm,
    KphHwBusJMicronSmart,
    KphHwBusCorsair,
    KphHwBusMaximum
} KPH_HW_BUS;

#define KPH_HW_BUS_TIMEOUT 100

static CONST PH_STRINGREF KphpHwBusNames[KphHwBusMaximum] =
{
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_PCI"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_ISABUS.HTP.Method"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_SMBUS.HTP.Method"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_Intel_OC_Mailbox"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_EC"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_DIMM"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\Access_JMicron_SMART"),
    PH_STRINGREF_INIT(L"\\BaseNamedObjects\\CorsairLinkReadWriteGuardMutex"),
};
static PH_INITONCE KphpHwBusInitOnce[KphHwBusMaximum] = { PH_INITONCE_INIT, PH_INITONCE_INIT, PH_INITONCE_INIT, PH_INITONCE_INIT, PH_INITONCE_INIT, PH_INITONCE_INIT, PH_INITONCE_INIT, PH_INITONCE_INIT };
static HANDLE KphpHwBusHandles[KphHwBusMaximum] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static NTSTATUS KphpHwBusStatus[KphHwBusMaximum] = { STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS, STATUS_SUCCESS };

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
 * Acquires the buses an embedded controller is reached through: the ISA bus,
 * then the EC.
 *
 * \return Successful or errant status; STATUS_RETRY when another tool holds a
 * bus.
 */
static NTSTATUS KphpHwAcquireEc(
    VOID
    )
{
    NTSTATUS status;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusIsa, KPH_HW_BUS_TIMEOUT)))
        return status;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusEc, KPH_HW_BUS_TIMEOUT)))
    {
        KphpHwReleaseBus(KphHwBusIsa);
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * Releases the buses acquired with KphpHwAcquireEc.
 */
static VOID KphpHwReleaseEc(
    VOID
    )
{
    KphpHwReleaseBus(KphHwBusEc);
    KphpHwReleaseBus(KphHwBusIsa);
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
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_SUPERIO_CHIP chip;
    ULONG returnLength;

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
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
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

/**
 * Identifies the root complex at a PCI location and the processor.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Identity Receives the root complex's vendor and device and the
 * processor's family, model and stepping.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSmnIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMN_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_SMN_IDENTITY identity;
    ULONG returnLength;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwSmnScript,
        &routine,
        &Locator,
        sizeof(Locator),
        &identity,
        sizeof(identity),
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusPci);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(identity))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Reads registers of the system management network through the root complex
 * at a PCI location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Addresses The addresses to read.
 * \param Count The number of addresses.
 * \param Values Receives a status and a value per address, in the order of the
 * addresses.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSmnRead(
    _In_ ULONG Locator,
    _In_reads_(Count) PCULONG Addresses,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG addressesLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE32 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(ULONG), &addressesLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(addressesLength, sizeof(ULONG), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE32), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Locator;
    RtlCopyMemory(&input[1], Addresses, addressesLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    if (NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
    {
        status = KphpHwScriptCall(
            &KphpHwSmnScript,
            &routine,
            input,
            inputLength,
            values,
            outputLength,
            &returnLength
            );

        KphpHwReleaseBus(KphHwBusPci);

        if (NT_SUCCESS(status) && returnLength != outputLength)
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (NT_SUCCESS(status))
            RtlCopyMemory(Values, values, outputLength);
    }

    PhFree(values);
    PhFree(input);

    return status;
}

/**
 * Identifies the firmware mailbox behind the root complex at a PCI location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Identity Receives the firmware version, and the version and length of
 * the power-management table (0 when the row has none or does not list the
 * version).
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSmuIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMU_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_SMU_IDENTITY identity;
    ULONG returnLength;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwSmuScript,
        &routine,
        &Locator,
        sizeof(Locator),
        &identity,
        sizeof(identity),
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusPci);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(identity))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Copies the power-management table of the firmware mailbox behind the root
 * complex at a PCI location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Header Receives the table's version and length.
 * \param Table Receives the table.
 * \param Length The length of Table.
 * \param ReturnLength Receives the length of the table, also when Table is too
 * small.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus; STATUS_BUFFER_TOO_SMALL when Table does
 * not hold the table.
 */
NTSTATUS KphHwSmuReadTable(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMU_TABLE Header,
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Table,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("table");
    NTSTATUS status;
    ULONG outputLength;
    ULONG returnLength;
    PKPH_HW_SMU_TABLE output;

    if (!NT_SUCCESS(status = RtlULongAdd(Length, sizeof(KPH_HW_SMU_TABLE), &outputLength)))
        return status;

    output = PhAllocate(outputLength);

    if (NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
    {
        status = KphpHwScriptCall(
            &KphpHwSmuScript,
            &routine,
            &Locator,
            sizeof(Locator),
            output,
            outputLength,
            &returnLength
            );

        KphpHwReleaseBus(KphHwBusPci);

        if ((NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL) &&
            returnLength < sizeof(KPH_HW_SMU_TABLE))
        {
            status = STATUS_INFO_LENGTH_MISMATCH;
        }

        if (NT_SUCCESS(status) && output->Length != returnLength - sizeof(KPH_HW_SMU_TABLE))
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (status == STATUS_BUFFER_TOO_SMALL)
            *ReturnLength = returnLength - sizeof(KPH_HW_SMU_TABLE);

        if (NT_SUCCESS(status))
        {
            *Header = *output;
            RtlCopyMemory(Table, PTR_ADD_OFFSET(output, sizeof(KPH_HW_SMU_TABLE)), output->Length);
            *ReturnLength = output->Length;
        }
    }

    PhFree(output);

    return status;
}

/**
 * Sends commands to the firmware mailbox behind the root complex at a PCI
 * location; only the commands the script's row permits are sent.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Commands The commands.
 * \param Count The number of commands.
 * \param Results Receives a status, the mailbox's response and the result words
 * per command, in the order of the commands.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSmuCommand(
    _In_ ULONG Locator,
    _In_reads_(Count) PCKPH_HW_SMU_COMMAND Commands,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_SMU_RESULT Results
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("command");
    NTSTATUS status;
    ULONG commandsLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_SMU_RESULT results;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_SMU_COMMAND), &commandsLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(commandsLength, sizeof(ULONG), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_SMU_RESULT), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Locator;
    RtlCopyMemory(&input[1], Commands, commandsLength);

    results = PhAllocate(outputLength ? outputLength : 1);

    if (NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
    {
        status = KphpHwScriptCall(
            &KphpHwSmuScript,
            &routine,
            input,
            inputLength,
            results,
            outputLength,
            &returnLength
            );

        KphpHwReleaseBus(KphHwBusPci);

        if (NT_SUCCESS(status) && returnLength != outputLength)
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (NT_SUCCESS(status))
            RtlCopyMemory(Results, results, outputLength);
    }

    PhFree(results);
    PhFree(input);

    return status;
}

/**
 * Identifies the HSMP mailbox of a socket: in the memory window of the
 * socket's AMDI0097 device when it has one, otherwise behind the root complex
 * at a PCI location.
 *
 * \param Locator The location of the socket's root complex: (segment << 16) |
 * (bus << 8) | (device << 3) | function.
 * \param Socket The socket, as its AMDI0097 device's _UID names it.
 * \param Identity Receives the mailbox's version.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwHsmpIdentify(
    _In_ ULONG Locator,
    _In_ ULONG Socket,
    _Out_ PKPH_HW_HSMP_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    ULONG input[2];
    KPH_HW_HSMP_IDENTITY identity;
    ULONG returnLength;

    input[0] = Locator;
    input[1] = Socket;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwHsmpScript,
        &routine,
        input,
        sizeof(input),
        &identity,
        sizeof(identity),
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusPci);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(identity))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Sends commands to the HSMP mailbox of a socket: in the memory window of the
 * socket's AMDI0097 device when it has one, otherwise behind the root complex
 * at a PCI location. Only the commands the script's row permits are sent.
 *
 * \param Locator The location of the socket's root complex: (segment << 16) |
 * (bus << 8) | (device << 3) | function.
 * \param Socket The socket, as its AMDI0097 device's _UID names it.
 * \param Commands The commands.
 * \param Count The number of commands.
 * \param Results Receives a status, the mailbox's response and the result words
 * per command, in the order of the commands.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwHsmpCommand(
    _In_ ULONG Locator,
    _In_ ULONG Socket,
    _In_reads_(Count) PCKPH_HW_HSMP_COMMAND Commands,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_HSMP_RESULT Results
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("command");
    NTSTATUS status;
    ULONG commandsLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_HSMP_RESULT results;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_HSMP_COMMAND), &commandsLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(commandsLength, sizeof(ULONG) * 2, &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_HSMP_RESULT), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Locator;
    input[1] = Socket;
    RtlCopyMemory(&input[2], Commands, commandsLength);

    results = PhAllocate(outputLength ? outputLength : 1);

    if (NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
    {
        status = KphpHwScriptCall(
            &KphpHwHsmpScript,
            &routine,
            input,
            inputLength,
            results,
            outputLength,
            &returnLength
            );

        KphpHwReleaseBus(KphHwBusPci);

        if (NT_SUCCESS(status) && returnLength != outputLength)
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (NT_SUCCESS(status))
            RtlCopyMemory(Results, results, outputLength);
    }

    PhFree(results);
    PhFree(input);

    return status;
}

/**
 * Reads the metrics table of a client processor's socket (family 1Ah models
 * 80h to 8Fh) through its AMDI0097 device, after the firmware writes it.
 *
 * \param Socket The socket, as its AMDI0097 device's _UID names it.
 * \param Table Receives the table.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a processor
 * or table size the script does not list, or without an AMDI0097 device;
 * STATUS_RETRY when another tool holds the bus.
 */
NTSTATUS KphHwHsmpReadMetrics(
    _In_ ULONG Socket,
    _Out_writes_bytes_(KPH_HW_HSMP_METRICS_SIZE) PVOID Table
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("metrics");
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
        return status;

    buffer = PhAllocate(KPH_HW_HSMP_METRICS_SIZE);

    status = KphpHwScriptCall(
        &KphpHwHsmpScript,
        &routine,
        &Socket,
        sizeof(ULONG),
        buffer,
        KPH_HW_HSMP_METRICS_SIZE,
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusPci);

    if (NT_SUCCESS(status) && returnLength != KPH_HW_HSMP_METRICS_SIZE)
        status = STATUS_INFO_LENGTH_MISMATCH;

    if (NT_SUCCESS(status))
        memcpy(Table, buffer, KPH_HW_HSMP_METRICS_SIZE);

    PhFree(buffer);

    return status;
}

/**
 * Acquires the buses an SMBus host is reached through.
 *
 * \details The host of a processor that is not Intel's is reached through the
 * chipset's power management index and data port pair, which other tools
 * serialize on the PCI bus, so the ISA and PCI buses are held as well.
 *
 * \param Dimm Whether the memory modules' devices are reached; the DIMM bus is
 * then held inside the others.
 * \param Isa Receives whether the ISA and PCI buses are held.
 * \return Successful or errant status; STATUS_RETRY when another tool holds a
 * bus.
 */
static NTSTATUS KphpHwAcquireSmbus(
    _In_ BOOLEAN Dimm,
    _Out_ PBOOLEAN Isa
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN isa = FALSE;
    NTSTATUS status;

    *Isa = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        PH_CPU_IDENTITY identity;

        if (PhGetCpuIdentity(&identity))
            isa = !PhCpuIdentityIsIntel(&identity);

        PhEndInitOnce(&initOnce);
    }

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusSmbus, KPH_HW_BUS_TIMEOUT)))
        return status;

    if (isa)
    {
        if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusIsa, KPH_HW_BUS_TIMEOUT)))
        {
            KphpHwReleaseBus(KphHwBusSmbus);
            return status;
        }

        if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
        {
            KphpHwReleaseBus(KphHwBusIsa);
            KphpHwReleaseBus(KphHwBusSmbus);
            return status;
        }

        *Isa = TRUE;
    }

    if (Dimm && !NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusDimm, KPH_HW_BUS_TIMEOUT)))
    {
        if (*Isa)
        {
            KphpHwReleaseBus(KphHwBusPci);
            KphpHwReleaseBus(KphHwBusIsa);
        }

        KphpHwReleaseBus(KphHwBusSmbus);
        *Isa = FALSE;
        return status;
    }

    return STATUS_SUCCESS;
}

/**
 * Releases the buses acquired with KphpHwAcquireSmbus.
 *
 * \param Dimm Whether the DIMM bus is held.
 * \param Isa Whether the ISA and PCI buses are held.
 */
static VOID KphpHwReleaseSmbus(
    _In_ BOOLEAN Dimm,
    _In_ BOOLEAN Isa
    )
{
    if (Dimm)
        KphpHwReleaseBus(KphHwBusDimm);

    if (Isa)
    {
        KphpHwReleaseBus(KphHwBusPci);
        KphpHwReleaseBus(KphHwBusIsa);
    }

    KphpHwReleaseBus(KphHwBusSmbus);
}

/**
 * Identifies the SMBus host at a PCI location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Host Receives the host's vendor, device and revision, its kind (1 for
 * the AMD FCH, 2 for the Intel host) and its SMBus base.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSmbusIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_SMBUS_HOST Host
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_SMBUS_HOST host;
    ULONG returnLength;
    BOOLEAN isa;

    if (!NT_SUCCESS(status = KphpHwAcquireSmbus(FALSE, &isa)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwSmbusScript,
        &routine,
        &Locator,
        sizeof(Locator),
        &host,
        sizeof(host),
        &returnLength
        );

    KphpHwReleaseSmbus(FALSE, isa);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(host))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Host = host;

    return status;
}

/**
 * Reads bytes and words from devices on a port of the SMBus host at a PCI
 * location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Port The host's port.
 * \param Items The reads: address, command, protocol (0 byte, 1 word).
 * \param Count The number of reads.
 * \param Values Receives a status and a value per read, in the order of the
 * reads.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwSmbusRead(
    _In_ ULONG Locator,
    _In_ ULONG Port,
    _In_reads_(Count) PCKPH_HW_SMBUS_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG itemsLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE32 values;
    BOOLEAN isa;
    BOOLEAN dimm = FALSE;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_SMBUS_ITEM), &itemsLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(itemsLength, sizeof(ULONG) * 2, &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE32), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Locator;
    input[1] = Port;
    RtlCopyMemory(&input[2], Items, itemsLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    // A memory module's thermal sensor (18h-1Fh), PMIC (48h-4Fh) or SPD hub (50h-57h).
    for (ULONG i = 0; i < Count; i++)
    {
        UCHAR address = Items[i].Address;

        if ((address >= 0x18 && address <= 0x1F) || (address >= 0x48 && address <= 0x57))
            dimm = TRUE;
    }

    if (NT_SUCCESS(status = KphpHwAcquireSmbus(dimm, &isa)))
    {
        status = KphpHwScriptCall(
            &KphpHwSmbusScript,
            &routine,
            input,
            inputLength,
            values,
            outputLength,
            &returnLength
            );

        KphpHwReleaseSmbus(dimm, isa);

        if (NT_SUCCESS(status) && returnLength != outputLength)
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (NT_SUCCESS(status))
            RtlCopyMemory(Values, values, outputLength);
    }

    PhFree(values);
    PhFree(input);

    return status;
}

/**
 * Reads a PMBus controller's revision and its MFR_ID, MFR_MODEL and
 * IC_DEVICE_ID blocks.
 *
 * \param Locator The SMBus host's location.
 * \param Port The host's port.
 * \param Address The device's 7-bit address.
 * \param Identity Receives the revision and the blocks read (a block not read
 * has length 0).
 * \return Successful or errant status; STATUS_NO_SUCH_DEVICE when nothing
 * answers; STATUS_ACCESS_DENIED for an address the script does not permit.
 */
NTSTATUS KphHwPmbusIdentify(
    _In_ ULONG Locator,
    _In_ ULONG Port,
    _In_ UCHAR Address,
    _Out_ PKPH_HW_PMBUS_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    static const UCHAR commands[4] = { 0x98, 0x99, 0x9A, 0xAD };
    NTSTATUS status;
    struct
    {
        ULONG Locator;
        ULONG Port;
        KPH_HW_SMBUS_ITEM Items[4];
    } input;
    UCHAR output[4 * sizeof(KPH_HW_VALUE32) + 3 * KPH_HW_PMBUS_BLOCK];
    KPH_HW_VALUE32 records[4];
    KPH_HW_PMBUS_IDENTITY identity;
    PUCHAR blocks[3];
    PUCHAR lengths[3];
    ULONG returnLength = 0;
    ULONG at = sizeof(records);
    BOOLEAN isa;

    input.Locator = Locator;
    input.Port = Port;

    for (ULONG i = 0; i < 4; i++)
    {
        input.Items[i].Address = Address;
        input.Items[i].Command = commands[i];
        input.Items[i].Protocol = i ? KPH_HW_SMBUS_BLOCK : KPH_HW_SMBUS_BYTE;
        input.Items[i].Reserved = 0;
    }

    if (!NT_SUCCESS(status = KphpHwAcquireSmbus(FALSE, &isa)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwSmbusScript,
        &routine,
        &input,
        sizeof(input),
        output,
        sizeof(output),
        &returnLength
        );

    KphpHwReleaseSmbus(FALSE, isa);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength < sizeof(records))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(records, output, sizeof(records));

    if (!NT_SUCCESS(records[0].Status))
        return records[0].Status;

    memset(&identity, 0, sizeof(identity));
    identity.Revision = (UCHAR)records[0].Value;
    blocks[0] = identity.MfrId;
    blocks[1] = identity.Model;
    blocks[2] = identity.DeviceId;
    lengths[0] = &identity.MfrIdLength;
    lengths[1] = &identity.ModelLength;
    lengths[2] = &identity.DeviceIdLength;

    for (ULONG i = 0; i < 3; i++)
    {
        ULONG length;

        if (!NT_SUCCESS(records[i + 1].Status))
            continue;

        length = records[i + 1].Value;

        if (length == 0 || length > KPH_HW_PMBUS_BLOCK || returnLength - at < length)
            return STATUS_INFO_LENGTH_MISMATCH;

        memcpy(blocks[i], output + at, length);
        *lengths[i] = (UCHAR)length;
        at += length;
    }

    if (at != returnLength)
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Identifies the PCI function at a location and the memory windows it has.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Device Receives the vendor and device IDs and a mask of the windows:
 * bit (KPH_HW_WINDOW_MCHBAR - 1) and bit (KPH_HW_WINDOW_BAR0 - 1).
 * \return Successful or errant status.
 */
NTSTATUS KphHwPciMmioIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_PCI_MMIO Device
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_PCI_MMIO device;
    ULONG returnLength;

    status = KphpHwScriptCall(
        &KphpHwPciMmioScript,
        &routine,
        &Locator,
        sizeof(Locator),
        &device,
        sizeof(device),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(device))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Device = device;

    return status;
}

/**
 * Reads registers of a memory window of the PCI function at a location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Window KPH_HW_WINDOW_MCHBAR or KPH_HW_WINDOW_BAR0.
 * \param Items The reads: offset in the window and width (1, 2, 4 or 8).
 * \param Count The number of reads.
 * \param Values Receives a status and a value per read, in the order of the
 * reads.
 * \return Successful or errant status.
 */
NTSTATUS KphHwPciMmioRead(
    _In_ ULONG Locator,
    _In_ ULONG Window,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG itemsLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE64 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_WINDOW_ITEM), &itemsLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(itemsLength, sizeof(ULONG) * 2, &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE64), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Locator;
    input[1] = Window;
    RtlCopyMemory(&input[2], Items, itemsLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    status = KphpHwScriptCall(
        &KphpHwPciMmioScript,
        &routine,
        input,
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
    PhFree(input);

    return status;
}

/**
 * Identifies the PCI function at a location whose configuration space may be
 * read.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Device Receives the vendor and device IDs and the number of ranges
 * that may be read.
 * \return Successful or errant status.
 */
NTSTATUS KphHwPciConfigIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_PCI_CONFIG Device
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_PCI_CONFIG device;
    ULONG returnLength;

    status = KphpHwScriptCall(
        &KphpHwPciConfigScript,
        &routine,
        &Locator,
        sizeof(Locator),
        &device,
        sizeof(device),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(device))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Device = device;

    return status;
}

/**
 * Reads registers of the configuration space of the PCI function at a
 * location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Items The reads: offset in configuration space and width (1, 2 or 4).
 * \param Count The number of reads.
 * \param Values Receives a status and a value per read, in the order of the
 * reads.
 * \return Successful or errant status.
 */
NTSTATUS KphHwPciConfigRead(
    _In_ ULONG Locator,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG itemsLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE32 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_WINDOW_ITEM), &itemsLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(itemsLength, sizeof(ULONG), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE32), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Locator;
    RtlCopyMemory(&input[1], Items, itemsLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    status = KphpHwScriptCall(
        &KphpHwPciConfigScript,
        &routine,
        input,
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
    PhFree(input);

    return status;
}

/**
 * Reads a range of the configuration space of the PCI function at a location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Offset The offset of the range in configuration space.
 * \param Buffer Receives the range.
 * \param Length The length of the range and of Buffer.
 * \return Successful or errant status.
 */
NTSTATUS KphHwPciConfigReadBlock(
    _In_ ULONG Locator,
    _In_ ULONG Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read_block");
    NTSTATUS status;
    ULONG input[3];
    ULONG returnLength;
    PVOID output;

    input[0] = Locator;
    input[1] = Offset;
    input[2] = Length;

    output = PhAllocate(Length ? Length : 1);

    status = KphpHwScriptCall(
        &KphpHwPciConfigScript,
        &routine,
        input,
        sizeof(input),
        output,
        Length,
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength != Length)
        status = STATUS_INFO_LENGTH_MISMATCH;

    if (NT_SUCCESS(status))
        RtlCopyMemory(Buffer, output, Length);

    PhFree(output);

    return status;
}

/**
 * Identifies the memory map of the Chrome EC embedded controller of the board.
 *
 * \param Identity Receives the map's signature and the version and flag bytes
 * after it.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwCrosEcIdentify(
    _Out_ PKPH_HW_CROSEC_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_CROSEC_IDENTITY identity;
    ULONG returnLength;

    if (!NT_SUCCESS(status = KphpHwAcquireEc()))
        return status;

    status = KphpHwScriptCall(
        &KphpHwCrosEcScript,
        &routine,
        NULL,
        0,
        &identity,
        sizeof(identity),
        &returnLength
        );

    KphpHwReleaseEc();

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(identity))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Reads bytes of the memory map of the Chrome EC embedded controller of the
 * board.
 *
 * \param Offsets The offsets in the map to read.
 * \param Count The number of offsets.
 * \param Values Receives a status and a byte per offset, in the order of the
 * offsets.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwCrosEcRead(
    _In_reads_(Count) PCULONG Offsets,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE32 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(ULONG), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE32), &outputLength)))
        return status;

    input = PhAllocate(inputLength ? inputLength : 1);
    RtlCopyMemory(input, Offsets, inputLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    if (NT_SUCCESS(status = KphpHwAcquireEc()))
    {
        status = KphpHwScriptCall(
            &KphpHwCrosEcScript,
            &routine,
            input,
            inputLength,
            values,
            outputLength,
            &returnLength
            );

        KphpHwReleaseEc();

        if (NT_SUCCESS(status) && returnLength != outputLength)
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (NT_SUCCESS(status))
            RtlCopyMemory(Values, values, outputLength);
    }

    PhFree(values);
    PhFree(input);

    return status;
}

/**
 * Identifies an ACPI embedded controller.
 *
 * \param Ordinal The controller, an ordinal of the PNP0C09 devices.
 * \param Identity Receives its data and command ports and KPH_HW_EC_* flags.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwEcIdentify(
    _In_ ULONG Ordinal,
    _Out_ PKPH_HW_EC_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_EC_IDENTITY identity;
    ULONG returnLength;

    if (!NT_SUCCESS(status = KphpHwAcquireEc()))
        return status;

    status = KphpHwScriptCall(
        &KphpHwEcScript,
        &routine,
        &Ordinal,
        sizeof(Ordinal),
        &identity,
        sizeof(identity),
        &returnLength
        );

    KphpHwReleaseEc();

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(identity))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Identity = identity;

    return status;
}

/**
 * Lists the sensors of the board's embedded controller.
 *
 * \param Sensors Receives the sensors.
 * \param Capacity The number of elements in Sensors.
 * \param Count Receives the number of sensors.
 * \return Successful or errant status; STATUS_NOT_FOUND on a board the script
 * has no row for; STATUS_BUFFER_TOO_SMALL when the board has more sensors than
 * Capacity.
 */
NTSTATUS KphHwEcSensors(
    _Out_writes_to_(Capacity, *Count) PKPH_HW_EC_SENSOR Sensors,
    _In_ ULONG Capacity,
    _Out_ PULONG Count
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("sensors");
    NTSTATUS status;
    UCHAR output[0x1000];
    ULONG returnLength;
    ULONG count = 0;

    status = KphpHwScriptCall(
        &KphpHwEcScript,
        &routine,
        NULL,
        0,
        output,
        sizeof(output),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    for (ULONG at = 0; at < returnLength; count++)
    {
        if (returnLength - at < 6 || returnLength - at - 6 < output[at + 5] || output[at + 5] >= KPH_HW_EC_LABEL_LENGTH)
            return STATUS_INFO_LENGTH_MISMATCH;

        at += 6 + output[at + 5];
    }

    if (count > Capacity)
        return STATUS_BUFFER_TOO_SMALL;

    for (ULONG i = 0, at = 0; i < count; i++)
    {
        Sensors[i].Kind = output[at];
        Sensors[i].Size = output[at + 1];
        Sensors[i].Bank = output[at + 2];
        Sensors[i].Register = output[at + 3];
        Sensors[i].Form = output[at + 4];
        RtlZeroMemory(Sensors[i].Label, sizeof(Sensors[i].Label));
        RtlCopyMemory(Sensors[i].Label, &output[at + 6], output[at + 5]);
        at += 6 + output[at + 5];
    }

    *Count = count;

    return status;
}

/**
 * Lists the fans and temperature sensors of an Alienware or Dell G machine
 * through its firmware's WMI method.
 *
 * \param Identity Receives the counts and the ids.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a machine the
 * script does not list; STATUS_NOT_FOUND when no device lists the method.
 */
NTSTATUS KphHwAlienwareIdentify(
    _Out_ PKPH_HW_ALIENWARE_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    UCHAR output[2 + 2 * KPH_HW_ALIENWARE_RESOURCES];
    KPH_HW_ALIENWARE_IDENTITY identity;
    ULONG returnLength = 0;

    status = KphpHwScriptCall(
        &KphpHwAlienwareScript,
        &routine,
        NULL,
        0,
        output,
        sizeof(output),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength < 2 ||
        output[0] > KPH_HW_ALIENWARE_RESOURCES ||
        output[1] > KPH_HW_ALIENWARE_RESOURCES ||
        returnLength != 2UL + output[0] + output[1])
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    memset(&identity, 0, sizeof(identity));
    identity.FanCount = output[0];
    identity.TemperatureCount = output[1];
    memcpy(identity.Fans, output + 2, output[0]);
    memcpy(identity.Temperatures, output + 2 + output[0], output[1]);
    *Identity = identity;

    return status;
}

/**
 * Reads temperatures and fan speeds of an Alienware or Dell G machine through
 * its firmware's WMI method.
 *
 * \param Sensors Each sensor's kind and id.
 * \param Count The number of sensors.
 * \param Values Receives a status and the value (degrees C or RPM) per sensor.
 * \return Successful or errant status.
 */
NTSTATUS KphHwAlienwareRead(
    _In_reads_(Count) PCKPH_HW_ALIENWARE_SENSOR Sensors,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_VALUE32 values[2 * KPH_HW_ALIENWARE_RESOURCES];
    ULONG returnLength = 0;

    if (Count == 0 || Count > RTL_NUMBER_OF(values))
        return STATUS_INVALID_PARAMETER;

    status = KphpHwScriptCall(
        &KphpHwAlienwareScript,
        &routine,
        (PVOID)Sensors,
        Count * sizeof(KPH_HW_ALIENWARE_SENSOR),
        values,
        Count * sizeof(KPH_HW_VALUE32),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != Count * sizeof(KPH_HW_VALUE32))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Values, values, Count * sizeof(KPH_HW_VALUE32));

    return status;
}

/**
 * Lists the numeric sensors of an HP machine's firmware data block that are a
 * temperature, voltage, current or fan.
 *
 * \param Sensors Receives each sensor's kind, instance and name.
 * \param Capacity The number of elements in Sensors.
 * \param Count Receives the number of sensors.
 * \return Successful or errant status; STATUS_NOT_FOUND when no device lists
 * the block; STATUS_NOT_SUPPORTED when it has none of those sensors.
 */
NTSTATUS KphHwHpIdentify(
    _Out_writes_to_(Capacity, *Count) PKPH_HW_HP_SENSOR Sensors,
    _In_ ULONG Capacity,
    _Out_ PULONG Count
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_HP_SENSOR sensors[KPH_HW_HP_SENSORS];
    ULONG returnLength = 0;
    ULONG count;

    status = KphpHwScriptCall(
        &KphpHwHpScript,
        &routine,
        NULL,
        0,
        sensors,
        sizeof(sensors),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength % sizeof(KPH_HW_HP_SENSOR) != 0)
        return STATUS_INFO_LENGTH_MISMATCH;

    count = returnLength / sizeof(KPH_HW_HP_SENSOR);

    for (ULONG i = 0; i < count; i++)
    {
        if (!memchr(sensors[i].Name, 0, sizeof(sensors[i].Name)))
            return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (count > Capacity)
        return STATUS_BUFFER_TOO_SMALL;

    memcpy(Sensors, sensors, count * sizeof(KPH_HW_HP_SENSOR));
    *Count = count;

    return status;
}

/**
 * Reads numeric sensors of an HP machine's firmware data block.
 *
 * \param Items Each sensor's kind and instance.
 * \param Count The number of sensors.
 * \param Values Receives a status and the signed value per sensor in its kind's
 * unit.
 * \return Successful or errant status.
 */
NTSTATUS KphHwHpRead(
    _In_reads_(Count) PCKPH_HW_HP_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_VALUE32 values[KPH_HW_HP_SENSORS];
    ULONG returnLength = 0;

    if (Count == 0 || Count > RTL_NUMBER_OF(values))
        return STATUS_INVALID_PARAMETER;

    status = KphpHwScriptCall(
        &KphpHwHpScript,
        &routine,
        (PVOID)Items,
        Count * sizeof(KPH_HW_HP_ITEM),
        values,
        Count * sizeof(KPH_HW_VALUE32),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != Count * sizeof(KPH_HW_VALUE32))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Values, values, Count * sizeof(KPH_HW_VALUE32));

    return status;
}

/**
 * Reads the die temperature of AMD Promontory 21 chipsets through their USB
 * controllers.
 *
 * \param Locators Each controller's location: (segment << 16) | (bus << 8) |
 * (device << 3) | function.
 * \param Count The number of controllers.
 * \param Values Receives a status and the raw byte per controller; 0 is no
 * reading.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwProm21Read(
    _In_reads_(Count) const ULONG* Locators,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_VALUE32 values[KPH_HW_PROM21_DIES];
    ULONG returnLength = 0;

    if (Count == 0 || Count > RTL_NUMBER_OF(values))
        return STATUS_INVALID_PARAMETER;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwProm21Script,
        &routine,
        (PVOID)Locators,
        Count * sizeof(ULONG),
        values,
        Count * sizeof(KPH_HW_VALUE32),
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusPci);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != Count * sizeof(KPH_HW_VALUE32))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Values, values, Count * sizeof(KPH_HW_VALUE32));

    return status;
}

/**
 * Calls a script routine that fills a record per device.
 *
 * \param Script The script.
 * \param Routine The routine.
 * \param Bus The bus to hold, or KphHwBusMaximum for none.
 * \param Devices Receives a record per device.
 * \param Count Receives the number of records.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
static NTSTATUS KphpHwHidCall(
    _In_ PKPH_HW_SCRIPT Script,
    _In_ PCANSI_STRING Routine,
    _In_ KPH_HW_BUS Bus,
    _Out_writes_to_(KPH_HW_HID_DEVICES, *Count) PKPH_HW_HID_DEVICE Devices,
    _Out_ PULONG Count
    )
{
    NTSTATUS status;
    PKPH_HW_HID_DEVICE devices;
    ULONG returnLength = 0;

    devices = PhAllocate(KPH_HW_HID_DEVICES * sizeof(KPH_HW_HID_DEVICE));

    if (Bus == KphHwBusMaximum || NT_SUCCESS(status = KphpHwAcquireBus(Bus, KPH_HW_BUS_TIMEOUT)))
    {
        status = KphpHwScriptCall(
            Script,
            Routine,
            NULL,
            0,
            devices,
            KPH_HW_HID_DEVICES * sizeof(KPH_HW_HID_DEVICE),
            &returnLength
            );

        if (Bus != KphHwBusMaximum)
            KphpHwReleaseBus(Bus);

        if (NT_SUCCESS(status) && returnLength % sizeof(KPH_HW_HID_DEVICE))
            status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        memcpy(Devices, devices, returnLength);
        *Count = returnLength / sizeof(KPH_HW_HID_DEVICE);
    }

    PhFree(devices);

    return status;
}

/**
 * Reads the coolers that push their status as HID input reports; each waits
 * for its next report.
 *
 * \param Devices Receives a record per device.
 * \param Count Receives the number of records.
 * \return Successful or errant status.
 */
NTSTATUS KphHwHidRead(
    _Out_writes_to_(KPH_HW_HID_DEVICES, *Count) PKPH_HW_HID_DEVICE Devices,
    _Out_ PULONG Count
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");

    return KphpHwHidCall(&KphpHwHidScript, &routine, KphHwBusMaximum, Devices, Count);
}

/**
 * Reads the fan controllers and power supplies that answer HID requests.
 *
 * \param Devices Receives a record per device.
 * \param Count Receives the number of records.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the devices.
 */
NTSTATUS KphHwCorsairRead(
    _Out_writes_to_(KPH_HW_HID_DEVICES, *Count) PKPH_HW_HID_DEVICE Devices,
    _Out_ PULONG Count
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("corsair");

    return KphpHwHidCall(&KphpHwHidScript, &routine, KphHwBusCorsair, Devices, Count);
}

/**
 * Reads the power supplies behind their USB serial dongle.
 *
 * \param Devices Receives a record per supply.
 * \param Count Receives the number of records.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the supplies.
 */
NTSTATUS KphHwAxiRead(
    _Out_writes_to_(KPH_HW_HID_DEVICES, *Count) PKPH_HW_HID_DEVICE Devices,
    _Out_ PULONG Count
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");

    return KphpHwHidCall(&KphpHwAxiScript, &routine, KphHwBusCorsair, Devices, Count);
}

/**
 * Reads the SMART data of the drives behind USB bridges.
 *
 * \param Disks Receives a record per disk of the USB storage drivers.
 * \param Count Receives the number of records.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bridges.
 */
NTSTATUS KphHwUsbDiskRead(
    _Out_writes_to_(KPH_HW_USB_DISKS, *Count) PKPH_HW_USB_DISK Disks,
    _Out_ PULONG Count
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    PKPH_HW_USB_DISK disks;
    ULONG returnLength = 0;

    disks = PhAllocate(KPH_HW_USB_DISKS * sizeof(KPH_HW_USB_DISK));

    if (NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusJMicronSmart, KPH_HW_BUS_TIMEOUT)))
    {
        status = KphpHwScriptCall(
            &KphpHwUsbDiskScript,
            &routine,
            NULL,
            0,
            disks,
            KPH_HW_USB_DISKS * sizeof(KPH_HW_USB_DISK),
            &returnLength
            );

        KphpHwReleaseBus(KphHwBusJMicronSmart);

        if (NT_SUCCESS(status) && returnLength % sizeof(KPH_HW_USB_DISK))
            status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status))
    {
        memcpy(Disks, disks, returnLength);
        *Count = returnLength / sizeof(KPH_HW_USB_DISK);
    }

    PhFree(disks);

    return status;
}

/**
 * Reads the temperature of AQtion network controllers through their firmware.
 *
 * \param Locators Each controller's location: (segment << 16) | (bus << 8) |
 * (device << 3) | function.
 * \param Count The number of controllers.
 * \param Values Receives a status and the temperature in millidegrees C (signed)
 * per controller.
 * \return Successful or errant status; STATUS_RETRY when another tool holds
 * the bus.
 */
NTSTATUS KphHwAqtionRead(
    _In_reads_(Count) const ULONG* Locators,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_VALUE32 values[KPH_HW_AQTION_CONTROLLERS];
    ULONG returnLength = 0;

    if (Count == 0 || Count > RTL_NUMBER_OF(values))
        return STATUS_INVALID_PARAMETER;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusPci, KPH_HW_BUS_TIMEOUT)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwAqtionScript,
        &routine,
        (PVOID)Locators,
        Count * sizeof(ULONG),
        values,
        Count * sizeof(KPH_HW_VALUE32),
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusPci);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != Count * sizeof(KPH_HW_VALUE32))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Values, values, Count * sizeof(KPH_HW_VALUE32));

    return status;
}

/**
 * Reads the six temperatures of a Gigabyte board through its firmware's WMI
 * method.
 *
 * \param Values Receives a status and the method's integer per sensor; its
 * low byte is signed degrees C, 0 a sensor that is not there.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a board that is
 * not Gigabyte's; STATUS_NOT_FOUND when no device lists the method.
 */
NTSTATUS KphHwGigabyteRead(
    _Out_writes_(KPH_HW_GIGABYTE_SENSORS) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_VALUE32 values[KPH_HW_GIGABYTE_SENSORS];
    ULONG returnLength = 0;

    status = KphpHwScriptCall(
        &KphpHwGigabyteScript,
        &routine,
        NULL,
        0,
        values,
        sizeof(values),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(values))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Values, values, sizeof(values));

    return status;
}

/**
 * Reads registers of an ACPI embedded controller.
 *
 * \param Ordinal The controller, an ordinal of the PNP0C09 devices.
 * \param Registers The bank and register of each byte to read.
 * \param Count The number of registers.
 * \param Values Receives a status and a byte per register, in their order.
 * \return Successful or errant status; STATUS_DEVICE_BUSY when the controller
 * holds a byte of another transaction; STATUS_RETRY when another tool holds the
 * bus.
 */
NTSTATUS KphHwEcRead(
    _In_ ULONG Ordinal,
    _In_reads_(Count) PCKPH_HW_EC_REGISTER Registers,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PUCHAR input;
    PKPH_HW_VALUE32 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_EC_REGISTER), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(inputLength, sizeof(ULONG), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE32), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    RtlCopyMemory(input, &Ordinal, sizeof(ULONG));
    RtlCopyMemory(input + sizeof(ULONG), Registers, inputLength - sizeof(ULONG));

    values = PhAllocate(outputLength ? outputLength : 1);

    if (NT_SUCCESS(status = KphpHwAcquireEc()))
    {
        status = KphpHwScriptCall(
            &KphpHwEcScript,
            &routine,
            input,
            inputLength,
            values,
            outputLength,
            &returnLength
            );

        KphpHwReleaseEc();

        if (NT_SUCCESS(status) && returnLength != outputLength)
            status = STATUS_INFO_LENGTH_MISMATCH;

        if (NT_SUCCESS(status))
            RtlCopyMemory(Values, values, outputLength);
    }

    PhFree(values);
    PhFree(input);

    return status;
}

/**
 * Reads the fan speed of a GPD handheld from its embedded controller.
 *
 * \param Rpm Receives the speed in RPM.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a machine the
 * script does not list; STATUS_RETRY when another tool holds the bus.
 */
NTSTATUS KphHwGpdReadFan(
    _Out_ PULONG Rpm
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG rpm;
    ULONG returnLength;

    if (!NT_SUCCESS(status = KphpHwAcquireEc()))
        return status;

    status = KphpHwScriptCall(
        &KphpHwGpdScript,
        &routine,
        NULL,
        0,
        &rpm,
        sizeof(rpm),
        &returnLength
        );

    KphpHwReleaseEc();

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(rpm))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Rpm = rpm;

    return status;
}

/**
 * Reads the temperatures and fan speeds of a Lenovo ThinkStation from its
 * embedded controller.
 *
 * \param Buffer Receives the model number (4 bytes), each of the model's
 * temperatures in degrees C (4 bytes, signed; -1 for none) and each of its fan
 * speeds in RPM (4 bytes).
 * \param Length The size of Buffer.
 * \param ReturnLength Receives the number of bytes written.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a machine the
 * script does not list; STATUS_DEVICE_BUSY when another agent holds the
 * controller's window; STATUS_RETRY when another tool holds the bus.
 */
NTSTATUS KphHwLenovoWsRead(
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG returnLength = 0;

    *ReturnLength = 0;

    if (!NT_SUCCESS(status = KphpHwAcquireEc()))
        return status;

    status = KphpHwScriptCall(
        &KphpHwLenovoWsScript,
        &routine,
        NULL,
        0,
        Buffer,
        Length,
        &returnLength
        );

    KphpHwReleaseEc();

    if (NT_SUCCESS(status))
        *ReturnLength = returnLength;

    return status;
}

/**
 * Identifies the fans and temperature sensors of a Dell machine through its
 * firmware's SMM interface.
 *
 * \param Identity Receives the fan speed multiplier, each fan's type and each
 * sensor's type (FFh for none).
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a machine the
 * script does not list; STATUS_IO_TIMEOUT once a command was too slow.
 */
NTSTATUS KphHwDellIdentify(
    _Out_ PKPH_HW_DELL_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_DELL_IDENTITY identity;
    ULONG returnLength;

    status = KphpHwScriptCall(
        &KphpHwDellScript,
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
 * Reads fans and temperature sensors of a Dell machine through its firmware's
 * SMM interface.
 *
 * \param FanMask The fans to read, a bit each.
 * \param TempMask The temperature sensors to read, a bit each.
 * \param Values Receives each fan's speed before the multiplier, then each
 * sensor's degrees C; -1 for one not read or without a reading.
 * \return Successful or errant status; STATUS_IO_TIMEOUT once a command was too
 * slow.
 */
NTSTATUS KphHwDellRead(
    _In_ ULONG FanMask,
    _In_ ULONG TempMask,
    _Out_writes_(KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS) PLONG Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG input[2];
    LONG values[KPH_HW_DELL_FANS + KPH_HW_DELL_TEMPS];
    ULONG returnLength;

    input[0] = FanMask;
    input[1] = TempMask;

    status = KphpHwScriptCall(
        &KphpHwDellScript,
        &routine,
        input,
        sizeof(input),
        values,
        sizeof(values),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(values))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Values, values, sizeof(values));

    return status;
}

/**
 * Identifies the hardware monitor sensors of an ASUS board through its
 * firmware's WMI method.
 *
 * \param Identity Receives the interface version, the sensor count and each
 * sensor's name and enums.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a board the
 * script does not list or an interface version below 2.
 */
NTSTATUS KphHwAsusWmiIdentify(
    _Out_ PKPH_HW_ASUSWMI_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_ASUSWMI_IDENTITY identity;
    ULONG returnLength = 0;

    memset(&identity, 0, sizeof(KPH_HW_ASUSWMI_IDENTITY));

    status = KphpHwScriptCall(
        &KphpHwAsusWmiScript,
        &routine,
        NULL,
        0,
        &identity,
        sizeof(KPH_HW_ASUSWMI_IDENTITY),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength < RTL_SIZEOF_THROUGH_FIELD(KPH_HW_ASUSWMI_IDENTITY, Count) ||
        identity.Count > KPH_HW_ASUSWMI_SENSORS ||
        returnLength != UFIELD_OFFSET(KPH_HW_ASUSWMI_IDENTITY, Sensors[identity.Count]))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    *Identity = identity;

    return status;
}

/**
 * Reads the hardware monitor sensors of an ASUS board through its firmware's
 * WMI method.
 *
 * \param Sources Each sensor's source (1 Super I/O, 2 embedded controller),
 * sensor 0 first.
 * \param Count The number of sensors.
 * \param Values Receives a status and the raw value per sensor.
 * \return Successful or errant status.
 */
NTSTATUS KphHwAsusWmiRead(
    _In_reads_(Count) const UCHAR* Sources,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE32 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_VALUE32 values[KPH_HW_ASUSWMI_SENSORS];
    ULONG returnLength = 0;

    if (Count == 0 || Count > KPH_HW_ASUSWMI_SENSORS)
        return STATUS_INVALID_PARAMETER;

    status = KphpHwScriptCall(
        &KphpHwAsusWmiScript,
        &routine,
        (PVOID)Sources,
        Count,
        values,
        Count * sizeof(KPH_HW_VALUE32),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != Count * sizeof(KPH_HW_VALUE32))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Values, values, Count * sizeof(KPH_HW_VALUE32));

    return status;
}

/**
 * Reads the voltage settings of the Intel overclocking mailbox, plane by plane.
 *
 * \param Planes Receives per plane (core, GPU, cache, system agent, analog I/O) a status, the completion code
 * and the data of the read command.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED on a processor the script does not list,
 * STATUS_RETRY when another tool holds the mailbox.
 */
NTSTATUS KphHwOcMailboxRead(
    _Out_writes_(MaxKphHwOcPlane) PKPH_HW_OC_PLANE Planes
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_OC_PLANE planes[MaxKphHwOcPlane];
    ULONG returnLength = 0;

    if (!NT_SUCCESS(status = KphpHwAcquireBus(KphHwBusIntelOcMailbox, KPH_HW_BUS_TIMEOUT)))
        return status;

    status = KphpHwScriptCall(
        &KphpHwOcMailboxScript,
        &routine,
        NULL,
        0,
        planes,
        sizeof(planes),
        &returnLength
        );

    KphpHwReleaseBus(KphHwBusIntelOcMailbox);

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(planes))
        return STATUS_INFO_LENGTH_MISMATCH;

    memcpy(Planes, planes, sizeof(planes));

    return status;
}


/**
 * Identifies the PCI function at a location whose Intel telemetry regions may
 * be read.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Device Receives the vendor and device IDs and the number of
 * operations of the function.
 * \return Successful or errant status.
 */
NTSTATUS KphHwVsecIdentify(
    _In_ ULONG Locator,
    _Out_ PKPH_HW_VSEC Device
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_VSEC device;
    ULONG returnLength;

    status = KphpHwScriptCall(
        &KphpHwVsecScript,
        &routine,
        &Locator,
        sizeof(Locator),
        &device,
        sizeof(device),
        &returnLength
        );

    if (!NT_SUCCESS(status))
        return status;

    if (returnLength != sizeof(device))
        return STATUS_INFO_LENGTH_MISMATCH;

    *Device = device;

    return status;
}

/**
 * Lists the Intel telemetry regions of every operation of the PCI function at
 * a location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Regions Receives an operation, discovery kind, index, ID, size and
 * position per region.
 * \param Count The number of entries Regions holds.
 * \param ReturnCount Receives the number of regions, also when Regions is too
 * small.
 * \return Successful or errant status; STATUS_BUFFER_TOO_SMALL when Regions
 * does not hold every region.
 */
NTSTATUS KphHwVsecRegions(
    _In_ ULONG Locator,
    _Out_writes_to_opt_(Count, *ReturnCount) PKPH_HW_VSEC_REGION Regions,
    _In_ ULONG Count,
    _Out_ PULONG ReturnCount
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("regions");
    NTSTATUS status;
    ULONG outputLength;
    ULONG returnLength;
    PKPH_HW_VSEC_REGION regions;

    if (!Regions)
        Count = 0;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VSEC_REGION), &outputLength)))
        return status;

    regions = PhAllocate(outputLength ? outputLength : 1);

    status = KphpHwScriptCall(
        &KphpHwVsecScript,
        &routine,
        &Locator,
        sizeof(Locator),
        outputLength ? regions : NULL,
        outputLength,
        &returnLength
        );

    if ((NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL) &&
        (returnLength % sizeof(KPH_HW_VSEC_REGION)) != 0)
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL)
        *ReturnCount = returnLength / sizeof(KPH_HW_VSEC_REGION);

    if (NT_SUCCESS(status))
        RtlCopyMemory(Regions, regions, returnLength);

    PhFree(regions);

    return status;
}

/**
 * Reads registers of an Intel telemetry region of the PCI function at a
 * location.
 *
 * \param Locator The location: (segment << 16) | (bus << 8) | (device << 3) |
 * function.
 * \param Operation The operation of the region, as KphHwVsecRegions lists it.
 * \param Position The position of the region, as KphHwVsecRegions lists it.
 * \param Id The ID of the region, as KphHwVsecRegions lists it.
 * \param Items The reads: offset in the region and width (4 or 8).
 * \param Count The number of reads.
 * \param Values Receives a status and a value per read, in the order of the
 * reads.
 * \return Successful or errant status.
 */
NTSTATUS KphHwVsecRead(
    _In_ ULONG Locator,
    _In_ ULONG Operation,
    _In_ ULONG Position,
    _In_ ULONG Id,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG itemsLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE64 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_WINDOW_ITEM), &itemsLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(itemsLength, sizeof(ULONG) * 4, &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE64), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Locator;
    input[1] = Operation;
    input[2] = Position;
    input[3] = Id;
    RtlCopyMemory(&input[4], Items, itemsLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    status = KphpHwScriptCall(
        &KphpHwVsecScript,
        &routine,
        input,
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
    PhFree(input);

    return status;
}

/**
 * Reads registers of an Intel platform window found through the ACPI low power
 * idle table.
 *
 * \param Window KPH_HW_PLATFORM_LPIT or KPH_HW_PLATFORM_PWRM.
 * \param Items The reads: offset in the window and width (1, 2, 4 or 8).
 * \param Count The number of reads.
 * \param Values Receives a status and a value per read, in the order of the
 * reads.
 * \return Successful or errant status.
 */
NTSTATUS KphHwPlatformRead(
    _In_ ULONG Window,
    _In_reads_(Count) PCKPH_HW_WINDOW_ITEM Items,
    _In_ ULONG Count,
    _Out_writes_(Count) PKPH_HW_VALUE64 Values
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG itemsLength;
    ULONG inputLength;
    ULONG outputLength;
    ULONG returnLength;
    PULONG input;
    PKPH_HW_VALUE64 values;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_WINDOW_ITEM), &itemsLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongAdd(itemsLength, sizeof(ULONG), &inputLength)))
        return status;
    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_VALUE64), &outputLength)))
        return status;

    input = PhAllocate(inputLength);
    input[0] = Window;
    RtlCopyMemory(&input[1], Items, itemsLength);

    values = PhAllocate(outputLength ? outputLength : 1);

    status = KphpHwScriptCall(
        &KphpHwPlatformScript,
        &routine,
        input,
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
    PhFree(input);

    return status;
}

/**
 * Lists the ACPI devices the acpi script's rows cover.
 *
 * \param Devices Receives a row, ordinal, matching ID and namespace path per
 * device.
 * \param Count The number of entries Devices holds.
 * \param ReturnCount Receives the number of devices, also when Devices is too
 * small.
 * \return Successful or errant status; STATUS_BUFFER_TOO_SMALL when Devices
 * does not hold every device.
 */
NTSTATUS KphHwAcpiIdentify(
    _Out_writes_to_opt_(Count, *ReturnCount) PKPH_HW_ACPI_DEVICE Devices,
    _In_ ULONG Count,
    _Out_ PULONG ReturnCount
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    ULONG outputLength;
    ULONG returnLength;
    PKPH_HW_ACPI_DEVICE devices;

    if (!Devices)
        Count = 0;

    if (!NT_SUCCESS(status = RtlULongMult(Count, sizeof(KPH_HW_ACPI_DEVICE), &outputLength)))
        return status;

    devices = PhAllocate(outputLength ? outputLength : 1);

    status = KphpHwScriptCall(
        &KphpHwAcpiScript,
        &routine,
        NULL,
        0,
        outputLength ? devices : NULL,
        outputLength,
        &returnLength
        );

    if ((NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL) &&
        (returnLength % sizeof(KPH_HW_ACPI_DEVICE)) != 0)
    {
        status = STATUS_INFO_LENGTH_MISMATCH;
    }

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL)
        *ReturnCount = returnLength / sizeof(KPH_HW_ACPI_DEVICE);

    if (NT_SUCCESS(status))
        RtlCopyMemory(Devices, devices, returnLength);

    PhFree(devices);

    return status;
}

/**
 * Evaluates ACPI methods the acpi script's rows permit.
 *
 * \param Milliseconds The time each evaluation may take.
 * \param Evals The records, built with KphHwAcpiAppendEval.
 * \param EvalsLength The length of the records.
 * \param Output Receives a status, a length and the raw output buffer per
 * record, each padded to a multiple of 8 bytes.
 * \param Length The length of Output.
 * \param ReturnLength Receives the length of the output, also when Output is
 * too small.
 * \return Successful or errant status; STATUS_BUFFER_TOO_SMALL when Output
 * does not hold every record.
 */
NTSTATUS KphHwAcpiEval(
    _In_ ULONG Milliseconds,
    _In_reads_bytes_(EvalsLength) PCVOID Evals,
    _In_ ULONG EvalsLength,
    _Out_writes_bytes_to_(Length, *ReturnLength) PVOID Output,
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("eval");
    NTSTATUS status;
    ULONG inputLength;
    ULONG returnLength;
    PUCHAR input;
    PVOID output;

    if (!NT_SUCCESS(status = RtlULongAdd(EvalsLength, sizeof(ULONG), &inputLength)))
        return status;

    input = PhAllocate(inputLength);
    RtlCopyMemory(input, &Milliseconds, sizeof(ULONG));
    RtlCopyMemory(input + sizeof(ULONG), Evals, EvalsLength);

    output = PhAllocate(Length ? Length : 1);

    status = KphpHwScriptCall(
        &KphpHwAcpiScript,
        &routine,
        input,
        inputLength,
        Length ? output : NULL,
        Length,
        &returnLength
        );

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_TOO_SMALL)
        *ReturnLength = returnLength;

    if (NT_SUCCESS(status))
        RtlCopyMemory(Output, output, returnLength);

    PhFree(output);
    PhFree(input);

    return status;
}

/**
 * Identifies the Qualcomm TSENS controllers of the part: each controller's
 * version and control words and the role of each of its sensors.
 *
 * \param Identity Receives the controllers.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED when the script
 * has no row for the part.
 */
NTSTATUS KphHwTsensIdentify(
    _Out_ PKPH_HW_TSENS_IDENTITY Identity
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("identify");
    NTSTATUS status;
    KPH_HW_TSENS_IDENTITY identity;
    ULONG returnLength;

    status = KphpHwScriptCall(
        &KphpHwTsensScript,
        &routine,
        NULL,
        0,
        &identity,
        sizeof(identity),
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength != sizeof(identity))
        status = STATUS_INFO_LENGTH_MISMATCH;

    if (NT_SUCCESS(status) && identity.Count > KPH_HW_TSENS_CONTROLLERS)
        status = STATUS_DEVICE_PROTOCOL_ERROR;

    if (NT_SUCCESS(status))
        *Identity = identity;

    return status;
}

/**
 * Reads the status registers of the Qualcomm TSENS sensors of the part.
 *
 * \param Count The number of controllers KphHwTsensIdentify gave.
 * \param Status Receives KPH_HW_TSENS_SENSORS status registers per controller,
 * 0 for a sensor not enabled.
 * \return Successful or errant status.
 */
NTSTATUS KphHwTsensRead(
    _In_ ULONG Count,
    _Out_writes_(Count * KPH_HW_TSENS_SENSORS) PULONG Status
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    ULONG length;
    ULONG returnLength;

    if (Count > KPH_HW_TSENS_CONTROLLERS)
        return STATUS_INVALID_PARAMETER;

    length = Count * KPH_HW_TSENS_SENSORS * sizeof(ULONG);

    status = KphpHwScriptCall(
        &KphpHwTsensScript,
        &routine,
        NULL,
        0,
        Status,
        length,
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength != length)
        status = STATUS_INFO_LENGTH_MISMATCH;

    return status;
}

/**
 * Reads the NVIDIA N1X power budget page: its temperatures, powers, energies
 * and power limits.
 *
 * \param Sample Receives the sample.
 * \return Successful or errant status; STATUS_NOT_SUPPORTED when the script
 * has no row for the part.
 */
NTSTATUS KphHwSpbmRead(
    _Out_ PKPH_HW_SPBM_SAMPLE Sample
    )
{
    static const ANSI_STRING routine = RTL_CONSTANT_STRING("read");
    NTSTATUS status;
    KPH_HW_SPBM_SAMPLE sample;
    ULONG returnLength;

    status = KphpHwScriptCall(
        &KphpHwSpbmScript,
        &routine,
        NULL,
        0,
        &sample,
        sizeof(sample),
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength != sizeof(sample))
        status = STATUS_INFO_LENGTH_MISMATCH;

    if (NT_SUCCESS(status))
        *Sample = sample;

    return status;
}

/**
 * Appends an evaluation record for KphHwAcpiEval.
 *
 * \param Evals The records.
 * \param Row The row, a KPH_HW_ACPI_ROW_* value.
 * \param Ordinal The device among the devices of the row, as
 * KphHwAcpiIdentify numbers them.
 * \param Name The method name or a path relative to the device.
 * \param Arguments The argument records as the ACPI driver takes them.
 */
VOID KphHwAcpiAppendEval(
    _Inout_ PPH_BYTES_BUILDER Evals,
    _In_ ULONG Row,
    _In_ ULONG Ordinal,
    _In_ PCPH_BYTESREF Name,
    _In_opt_ PCPH_BYTESREF Arguments
    )
{
    static CONST UCHAR padding[8] = { 0 };
    ULONG header[4];
    SIZE_T length;

    header[0] = Row;
    header[1] = Ordinal;
    header[2] = (ULONG)Name->Length;
    header[3] = Arguments ? (ULONG)Arguments->Length : 0;

    PhAppendBytesBuilderEx(Evals, header, sizeof(header), 0, NULL);
    PhAppendBytesBuilderEx(Evals, Name->Buffer, Name->Length, 0, NULL);

    if (Arguments && Arguments->Length)
        PhAppendBytesBuilderEx(Evals, Arguments->Buffer, Arguments->Length, 0, NULL);

    length = sizeof(header) + Name->Length + header[3];

    if (length % sizeof(padding))
        PhAppendBytesBuilderEx(Evals, (PVOID)padding, sizeof(padding) - (length % sizeof(padding)), 0, NULL);
}

