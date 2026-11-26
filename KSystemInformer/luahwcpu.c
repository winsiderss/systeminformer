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

#include <kph.h>
#include <script.h>

#include <trace.h>

typedef enum _KPH_LUA_HW_BATCH_KIND
{
#if defined(_M_X64)
    KphLuaHwBatchMsr,
    KphLuaHwBatchMsrWrite,
    KphLuaHwBatchCpuid,
    KphLuaHwBatchMsrSelect,
    KphLuaHwBatchCr,
    KphLuaHwBatchXcr,
    KphLuaHwBatchPmc,
    KphLuaHwBatchSmi,
#elif defined(_M_ARM64)
    KphLuaHwBatchSysreg,
    KphLuaHwBatchSysregWrite,
    KphLuaHwBatchSysregSelect,
    KphLuaHwBatchSmc,
    KphLuaHwBatchHvc,
#endif
} KPH_LUA_HW_BATCH_KIND, *PKPH_LUA_HW_BATCH_KIND;

typedef struct _KPH_LUA_HW_BATCH_ITEM
{
    USHORT Group;
    USHORT Number;
} KPH_LUA_HW_BATCH_ITEM, *PKPH_LUA_HW_BATCH_ITEM;

typedef struct _KPH_LUA_HW_REG_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Register;
} KPH_LUA_HW_REG_ITEM, *PKPH_LUA_HW_REG_ITEM;

typedef struct _KPH_LUA_HW_REG_RESULT
{
    NTSTATUS Status;
    ULONG Reserved;
    ULONG64 Value;
} KPH_LUA_HW_REG_RESULT, *PKPH_LUA_HW_REG_RESULT;

typedef struct _KPH_LUA_HW_REG_WRITE_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Register;
    ULONG64 Value;
} KPH_LUA_HW_REG_WRITE_ITEM, *PKPH_LUA_HW_REG_WRITE_ITEM;

typedef struct _KPH_LUA_HW_REG_WRITE_RESULT
{
    NTSTATUS Status;
    ULONG Reserved;
} KPH_LUA_HW_REG_WRITE_RESULT, *PKPH_LUA_HW_REG_WRITE_RESULT;

#if defined(_M_X64)

typedef struct _KPH_LUA_HW_CPUID_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Leaf;
    ULONG Subleaf;
} KPH_LUA_HW_CPUID_ITEM, *PKPH_LUA_HW_CPUID_ITEM;

typedef struct _KPH_LUA_HW_CPUID_RESULT
{
    NTSTATUS Status;
    ULONG Registers[4];
} KPH_LUA_HW_CPUID_RESULT, *PKPH_LUA_HW_CPUID_RESULT;

typedef struct _KPH_LUA_HW_SMI_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Port;
    ULONG64 Registers[6];
} KPH_LUA_HW_SMI_ITEM, *PKPH_LUA_HW_SMI_ITEM;

typedef struct _KPH_LUA_HW_SMI_RESULT
{
    NTSTATUS Status;
    ULONG Reserved;
    ULONG64 Registers[7];
} KPH_LUA_HW_SMI_RESULT, *PKPH_LUA_HW_SMI_RESULT;

C_ASSERT(sizeof(KPH_LUA_HW_CPUID_ITEM) == 12);
C_ASSERT(sizeof(KPH_LUA_HW_CPUID_RESULT) == 20);
C_ASSERT(sizeof(KPH_LUA_HW_SMI_ITEM) == 56);
C_ASSERT(sizeof(KPH_LUA_HW_SMI_RESULT) == 64);

#endif

#include <pshpack4.h>
typedef struct _KPH_LUA_HW_SELECT_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Selector;
    ULONG Data;
    ULONG64 Value;
} KPH_LUA_HW_SELECT_ITEM, *PKPH_LUA_HW_SELECT_ITEM;
#include <poppack.h>

C_ASSERT(sizeof(KPH_LUA_HW_SELECT_ITEM) == 20);
C_ASSERT(sizeof(KPH_LUA_HW_REG_ITEM) == 8);
C_ASSERT(sizeof(KPH_LUA_HW_REG_RESULT) == 16);
C_ASSERT(sizeof(KPH_LUA_HW_REG_WRITE_ITEM) == 16);
C_ASSERT(sizeof(KPH_LUA_HW_REG_WRITE_RESULT) == 8);

#if defined(_M_ARM64)

typedef struct _KPH_LUA_HW_CALL_ITEM
{
    USHORT Group;
    USHORT Number;
    ULONG Reserved;
    ULONG64 Registers[18];
} KPH_LUA_HW_CALL_ITEM, *PKPH_LUA_HW_CALL_ITEM;

typedef struct _KPH_LUA_HW_CALL_RESULT
{
    NTSTATUS Status;
    ULONG Reserved;
    ULONG64 Registers[18];
} KPH_LUA_HW_CALL_RESULT, *PKPH_LUA_HW_CALL_RESULT;

C_ASSERT(sizeof(KPH_LUA_HW_CALL_ITEM) == 152);
C_ASSERT(sizeof(KPH_LUA_HW_CALL_RESULT) == 152);

#endif

typedef struct _KPH_LUA_HW_BATCH KPH_LUA_HW_BATCH, *PKPH_LUA_HW_BATCH;

typedef struct _KPH_LUA_HW_BATCH_PROCESSOR
{
    KSI_KDPC Dpc;
    PKPH_LUA_HW_BATCH Batch;
    ULONG First;
    ULONG Count;
} KPH_LUA_HW_BATCH_PROCESSOR, *PKPH_LUA_HW_BATCH_PROCESSOR;

struct _KPH_LUA_HW_BATCH
{
    KEVENT Done;
    LONG Pending;
    LONG Active;
    KPH_LUA_HW_BATCH_KIND Kind;
    ULONG Count;
    ULONG ProcessorCount;
    PVOID Items;
    PVOID Results;
    PULONG Order;
    PULONG ItemProcessors;
    KPH_LUA_HW_BATCH_PROCESSOR Processors[ANYSIZE_ARRAY];
};

#if defined(_M_X64)

/**
 * \brief Reads an MSR of the current processor.
 *
 * \param[in] Index The MSR.
 * \param[out] Value Receives the value, zero when the read faults.
 *
 * \return TRUE when the read did not fault, FALSE otherwise.
 */
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwBatchReadMsr(
    _In_ ULONG Index,
    _Out_ PULONG64 Value
    )
{
    ULONG64 value;

    KPH_NPAGED_CODE_DISPATCH_MIN();

    *Value = 0;

    __try
    {
        value = __readmsr(Index);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    *Value = value;

    return TRUE;
}

/**
 * \brief Writes an MSR of the current processor.
 *
 * \param[in] Index The MSR.
 * \param[in] Value The value.
 *
 * \return TRUE when the write did not fault, FALSE otherwise.
 */
_IRQL_requires_min_(DISPATCH_LEVEL)
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwBatchWriteMsr(
    _In_ ULONG Index,
    _In_ ULONG64 Value
    )
{
    KPH_NPAGED_CODE_DISPATCH_MIN();

    __try
    {
        __writemsr(Index, Value);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Reads a control register of the current processor.
 *
 * \param[in] Number The control register: 0, 2, 3, 4 or 8.
 * \param[out] Value Receives the value, zero for another number.
 *
 * \return TRUE for a control register of the list, FALSE otherwise.
 */
_IRQL_requires_(DISPATCH_LEVEL)
BOOLEAN KphpLuaHwBatchReadCr(
    _In_ ULONG Number,
    _Out_ PULONG64 Value
    )
{
    KPH_NPAGED_CODE_DISPATCH();

    *Value = 0;

    switch (Number)
    {
        case 0:
        {
            *Value = __readcr0();
            return TRUE;
        }
        case 2:
        {
            *Value = __readcr2();
            return TRUE;
        }
        case 3:
        {
            *Value = __readcr3();
            return TRUE;
        }
        case 4:
        {
            *Value = __readcr4();
            return TRUE;
        }
        case 8:
        {
            *Value = __readcr8();
            return TRUE;
        }
        default:
        {
            return FALSE;
        }
    }
}

/**
 * \brief Reads an extended control register of the current processor.
 *
 * \param[in] Index The extended control register.
 * \param[out] Value Receives the value, zero when the read faults.
 *
 * \return TRUE when the read did not fault, FALSE otherwise.
 */
_IRQL_requires_(DISPATCH_LEVEL)
BOOLEAN KphpLuaHwBatchReadXcr(
    _In_ ULONG Index,
    _Out_ PULONG64 Value
    )
{
    ULONG64 value;

    KPH_NPAGED_CODE_DISPATCH();

    *Value = 0;

    __try
    {
        value = _xgetbv(Index);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    *Value = value;

    return TRUE;
}

/**
 * \brief Reads a performance counter of the current processor.
 *
 * \param[in] Index The counter.
 * \param[out] Value Receives the value, zero when the read faults.
 *
 * \return TRUE when the read did not fault, FALSE otherwise.
 */
_IRQL_requires_(DISPATCH_LEVEL)
BOOLEAN KphpLuaHwBatchReadPmc(
    _In_ ULONG Index,
    _Out_ PULONG64 Value
    )
{
    ULONG64 value;

    KPH_NPAGED_CODE_DISPATCH();

    *Value = 0;

    __try
    {
        value = __readpmc(Index);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    *Value = value;

    return TRUE;
}

/**
 * \brief Reads a data MSR through a selector MSR of the current processor,
 * with interrupts masked for the four accesses, and puts the selector back.
 *
 * \param[in] Selector The selector MSR.
 * \param[in] Data The data MSR.
 * \param[in] SelectorValue The value written to the selector.
 * \param[out] Value Receives the value of the data MSR.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_(DISPATCH_LEVEL)
NTSTATUS KphpLuaHwBatchSelect(
    _In_ ULONG Selector,
    _In_ ULONG Data,
    _In_ ULONG64 SelectorValue,
    _Out_ PULONG64 Value
    )
{
    NTSTATUS status;
    KIRQL previous;
    ULONG64 kept;
    ULONG64 value;

    KPH_NPAGED_CODE_DISPATCH();

    *Value = 0;

    KeRaiseIrql(HIGH_LEVEL, &previous);

    if (!KphpLuaHwBatchReadMsr(Selector, &kept))
    {
        status = STATUS_NOT_SUPPORTED;
    }
    else if (!KphpLuaHwBatchWriteMsr(Selector, SelectorValue))
    {
        status = STATUS_NOT_SUPPORTED;
    }
    else if (!KphpLuaHwBatchReadMsr(Data, &value))
    {
        KphpLuaHwBatchWriteMsr(Selector, kept);
        status = STATUS_NOT_SUPPORTED;
    }
    else if (!KphpLuaHwBatchWriteMsr(Selector, kept))
    {
        *Value = value;
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        *Value = value;
        status = STATUS_SUCCESS;
    }

    KeLowerIrql(previous);

    return status;
}

#elif defined(_M_ARM64)

/**
 * \brief Reads a data register through a selector register of the current
 * processor, with interrupts masked for the four accesses, and puts the
 * selector back.
 *
 * \param[in] Selector The selector register code.
 * \param[in] Data The data register code.
 * \param[in] SelectorValue The value written to the selector.
 * \param[out] Value Receives the value of the data register.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_(DISPATCH_LEVEL)
NTSTATUS KphpLuaHwBatchSelect(
    _In_ ULONG Selector,
    _In_ ULONG Data,
    _In_ ULONG64 SelectorValue,
    _Out_ PULONG64 Value
    )
{
    NTSTATUS status;
    KIRQL previous;
    ULONG64 kept;
    ULONG64 value;
    BOOLEAN listed;

    KPH_NPAGED_CODE_DISPATCH();

    *Value = 0;

    if (!KphpLuaHwIsSysregListed(Selector) || !KphpLuaHwIsSysregListed(Data))
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    KeRaiseIrql(HIGH_LEVEL, &previous);

    if (!KphpLuaHwReadSysreg(Selector, &kept, &listed))
    {
        status = STATUS_NOT_SUPPORTED;
    }
    else if (!KphpLuaHwWriteSysreg(Selector, SelectorValue, &listed))
    {
        status = STATUS_NOT_SUPPORTED;
    }
    else if (!KphpLuaHwReadSysreg(Data, &value, &listed))
    {
        KphpLuaHwWriteSysreg(Selector, kept, &listed);
        status = STATUS_NOT_SUPPORTED;
    }
    else if (!KphpLuaHwWriteSysreg(Selector, kept, &listed))
    {
        *Value = value;
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        *Value = value;
        status = STATUS_SUCCESS;
    }

    KeLowerIrql(previous);

    return status;
}

/**
 * \brief Calls into firmware or the hypervisor on the current processor.
 *
 * \param[in] Hvc TRUE for HVC #0, FALSE for SMC #0.
 * \param[in,out] Registers x0 to x17 for the call, receives x0 to x17 after
 * it, zero when the call faults.
 *
 * \return TRUE when the call did not fault, FALSE otherwise.
 */
_IRQL_requires_(DISPATCH_LEVEL)
BOOLEAN KphpLuaHwBatchCall(
    _In_ BOOLEAN Hvc,
    _Inout_updates_(18) PULONG64 Registers
    )
{
    KPH_NPAGED_CODE_DISPATCH();

    __try
    {
        if (Hvc)
        {
            KphHvc(Registers);
        }
        else
        {
            KphSmc(Registers);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RtlZeroMemory(Registers, sizeof(ULONG64) * 18);
        return FALSE;
    }

    return TRUE;
}

#endif

/**
 * \brief Marks one processor of a batch finished.
 *
 * \details The decrement of Active is the last access to the batch; the
 * waiter frees it only after Active reaches zero.
 *
 * \param[in,out] Batch The batch.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpLuaHwBatchFinish(
    _Inout_ PKPH_LUA_HW_BATCH Batch
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (InterlockedDecrement(&Batch->Pending) == 0)
    {
        KeSetEvent(&Batch->Done, IO_NO_INCREMENT, FALSE);
    }

    InterlockedDecrement(&Batch->Active);
}

/**
 * \brief Does the items of one processor, on that processor.
 *
 * \param[in] Dpc Unused.
 * \param[in] DeferredContext The processor's entry of the batch.
 * \param[in] SystemArgument1 Unused.
 * \param[in] SystemArgument2 Unused.
 */
_Function_class_(KDEFERRED_ROUTINE)
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID KphpLuaHwBatchDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2
    )
{
    PKPH_LUA_HW_BATCH_PROCESSOR processor;
    PKPH_LUA_HW_BATCH batch;
    ULONG index;

    KPH_NPAGED_CODE_DISPATCH();

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    NT_ASSERT(DeferredContext);

    processor = DeferredContext;
    batch = processor->Batch;

    for (ULONG i = 0; i < processor->Count; i++)
    {
        index = batch->Order[processor->First + i];

        switch (batch->Kind)
        {
#if defined(_M_X64)
            case KphLuaHwBatchMsr:
            {
                PKPH_LUA_HW_REG_ITEM item;
                PKPH_LUA_HW_REG_RESULT result;
                ULONG64 value;

                item = &((PKPH_LUA_HW_REG_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_RESULT)batch->Results)[index];

                if (KphpLuaHwBatchReadMsr(item->Register, &value))
                {
                    result->Status = STATUS_SUCCESS;
                }
                else
                {
                    result->Status = STATUS_NOT_SUPPORTED;
                }

                result->Value = value;
                break;
            }
            case KphLuaHwBatchMsrWrite:
            {
                PKPH_LUA_HW_REG_WRITE_ITEM item;
                PKPH_LUA_HW_REG_WRITE_RESULT result;

                item = &((PKPH_LUA_HW_REG_WRITE_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_WRITE_RESULT)batch->Results)[index];

                if (KphpLuaHwBatchWriteMsr(item->Register, item->Value))
                {
                    result->Status = STATUS_SUCCESS;
                }
                else
                {
                    result->Status = STATUS_NOT_SUPPORTED;
                }
                break;
            }
            case KphLuaHwBatchCpuid:
            {
                PKPH_LUA_HW_CPUID_ITEM item;
                PKPH_LUA_HW_CPUID_RESULT result;
                int registers[4];

                item = &((PKPH_LUA_HW_CPUID_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_CPUID_RESULT)batch->Results)[index];

                __cpuidex(registers, (int)item->Leaf, (int)item->Subleaf);

                result->Status = STATUS_SUCCESS;
                result->Registers[0] = (ULONG)registers[0];
                result->Registers[1] = (ULONG)registers[1];
                result->Registers[2] = (ULONG)registers[2];
                result->Registers[3] = (ULONG)registers[3];
                break;
            }
            case KphLuaHwBatchMsrSelect:
            {
                PKPH_LUA_HW_SELECT_ITEM item;
                PKPH_LUA_HW_REG_RESULT result;
                ULONG64 value;

                item = &((PKPH_LUA_HW_SELECT_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_RESULT)batch->Results)[index];

                result->Status = KphpLuaHwBatchSelect(item->Selector,
                                                      item->Data,
                                                      item->Value,
                                                      &value);
                result->Value = value;
                break;
            }
            case KphLuaHwBatchCr:
            {
                PKPH_LUA_HW_REG_ITEM item;
                PKPH_LUA_HW_REG_RESULT result;
                ULONG64 value;

                item = &((PKPH_LUA_HW_REG_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_RESULT)batch->Results)[index];

                if (KphpLuaHwBatchReadCr(item->Register, &value))
                {
                    result->Status = STATUS_SUCCESS;
                }
                else
                {
                    result->Status = STATUS_INVALID_PARAMETER;
                }

                result->Value = value;
                break;
            }
            case KphLuaHwBatchXcr:
            {
                PKPH_LUA_HW_REG_ITEM item;
                PKPH_LUA_HW_REG_RESULT result;
                ULONG64 value;

                item = &((PKPH_LUA_HW_REG_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_RESULT)batch->Results)[index];

                if (KphpLuaHwBatchReadXcr(item->Register, &value))
                {
                    result->Status = STATUS_SUCCESS;
                }
                else
                {
                    result->Status = STATUS_NOT_SUPPORTED;
                }

                result->Value = value;
                break;
            }
            case KphLuaHwBatchPmc:
            {
                PKPH_LUA_HW_REG_ITEM item;
                PKPH_LUA_HW_REG_RESULT result;
                ULONG64 value;

                item = &((PKPH_LUA_HW_REG_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_RESULT)batch->Results)[index];

                if (KphpLuaHwBatchReadPmc(item->Register, &value))
                {
                    result->Status = STATUS_SUCCESS;
                }
                else
                {
                    result->Status = STATUS_NOT_SUPPORTED;
                }

                result->Value = value;
                break;
            }
            case KphLuaHwBatchSmi:
            {
                PKPH_LUA_HW_SMI_ITEM item;
                PKPH_LUA_HW_SMI_RESULT result;

                item = &((PKPH_LUA_HW_SMI_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_SMI_RESULT)batch->Results)[index];

                NT_ASSERT(item->Port <= USHORT_MAX);

                RtlCopyMemory(result->Registers,
                              item->Registers,
                              sizeof(item->Registers));

                KphSmi((USHORT)item->Port, result->Registers);

                result->Status = STATUS_SUCCESS;
                break;
            }
#elif defined(_M_ARM64)
            case KphLuaHwBatchSysreg:
            {
                PKPH_LUA_HW_REG_ITEM item;
                PKPH_LUA_HW_REG_RESULT result;
                ULONG64 value;
                BOOLEAN listed;

                item = &((PKPH_LUA_HW_REG_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_RESULT)batch->Results)[index];

                if (!KphpLuaHwReadSysreg(item->Register, &value, &listed))
                {
                    result->Status = STATUS_NOT_SUPPORTED;
                }
                else if (!listed)
                {
                    result->Status = STATUS_NOT_IMPLEMENTED;
                }
                else
                {
                    result->Status = STATUS_SUCCESS;
                }

                result->Value = value;
                break;
            }
            case KphLuaHwBatchSysregWrite:
            {
                PKPH_LUA_HW_REG_WRITE_ITEM item;
                PKPH_LUA_HW_REG_WRITE_RESULT result;
                BOOLEAN listed;

                item = &((PKPH_LUA_HW_REG_WRITE_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_WRITE_RESULT)batch->Results)[index];

                if (!KphpLuaHwWriteSysreg(item->Register, item->Value, &listed))
                {
                    result->Status = STATUS_NOT_SUPPORTED;
                }
                else if (!listed)
                {
                    result->Status = STATUS_NOT_IMPLEMENTED;
                }
                else
                {
                    result->Status = STATUS_SUCCESS;
                }
                break;
            }
            case KphLuaHwBatchSysregSelect:
            {
                PKPH_LUA_HW_SELECT_ITEM item;
                PKPH_LUA_HW_REG_RESULT result;
                ULONG64 value;

                item = &((PKPH_LUA_HW_SELECT_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_REG_RESULT)batch->Results)[index];

                result->Status = KphpLuaHwBatchSelect(item->Selector,
                                                      item->Data,
                                                      item->Value,
                                                      &value);
                result->Value = value;
                break;
            }
            case KphLuaHwBatchSmc:
            case KphLuaHwBatchHvc:
            {
                PKPH_LUA_HW_CALL_ITEM item;
                PKPH_LUA_HW_CALL_RESULT result;

                item = &((PKPH_LUA_HW_CALL_ITEM)batch->Items)[index];
                result = &((PKPH_LUA_HW_CALL_RESULT)batch->Results)[index];

                RtlCopyMemory(result->Registers,
                              item->Registers,
                              sizeof(item->Registers));

                if (KphpLuaHwBatchCall(batch->Kind == KphLuaHwBatchHvc,
                                       result->Registers))
                {
                    result->Status = STATUS_SUCCESS;
                }
                else
                {
                    result->Status = STATUS_NOT_SUPPORTED;
                }
                break;
            }
#endif
            DEFAULT_UNREACHABLE;
        }
    }

    KphpLuaHwBatchFinish(batch);
}

KPH_PAGED_FILE();

/**
 * \brief Frees a batch.
 *
 * \param[in] Context The batch.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwFreeBatch(
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    KphFree(Context, KPH_TAG_LUA_HARDWARE_BATCH);
}

/**
 * \brief Sorts the items of a batch by processor, keeping the order of the
 * items of each processor.
 *
 * \param[in,out] Batch The batch, with its items copied in.
 * \param[in] ItemSize The size of an item.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwBatchSort(
    _Inout_ PKPH_LUA_HW_BATCH Batch,
    _In_ ULONG ItemSize
    )
{
    PKPH_LUA_HW_BATCH_ITEM item;
    PKPH_LUA_HW_BATCH_PROCESSOR processor;
    PROCESSOR_NUMBER number;
    ULONG index;
    ULONG first;

    KPH_PAGED_CODE_PASSIVE();

    for (ULONG i = 0; i < Batch->Count; i++)
    {
        item = Add2Ptr(Batch->Items, (SIZE_T)i * ItemSize);

        Batch->ItemProcessors[i] = INVALID_PROCESSOR_INDEX;

        if (item->Number > UCHAR_MAX)
        {
            continue;
        }

        RtlZeroMemory(&number, sizeof(number));
        number.Group = item->Group;
        number.Number = (UCHAR)item->Number;

        index = KeGetProcessorIndexFromNumber(&number);
        if (index >= Batch->ProcessorCount)
        {
            continue;
        }

        Batch->ItemProcessors[i] = index;
        Batch->Processors[index].Count++;
    }

    first = 0;

    for (ULONG i = 0; i < Batch->ProcessorCount; i++)
    {
        Batch->Processors[i].First = first;
        first += Batch->Processors[i].Count;
        Batch->Processors[i].Count = 0;
    }

    for (ULONG i = 0; i < Batch->Count; i++)
    {
        index = Batch->ItemProcessors[i];
        if (index == INVALID_PROCESSOR_INDEX)
        {
            continue;
        }

        processor = &Batch->Processors[index];
        Batch->Order[processor->First + processor->Count] = i;
        processor->Count++;
    }
}

/**
 * \brief Queues the deferred routine of each processor of a range that has
 * items.
 *
 * \param[in,out] Batch The sorted batch.
 * \param[in] First The first processor index of the range.
 * \param[in] Last The processor index after the range.
 *
 * \return TRUE when at least one was queued or failed to queue, so that the
 * caller waits, FALSE when no processor of the range has items.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpLuaHwBatchQueue(
    _Inout_ PKPH_LUA_HW_BATCH Batch,
    _In_ ULONG First,
    _In_ ULONG Last
    )
{
    NTSTATUS status;
    PKPH_LUA_HW_BATCH_PROCESSOR processor;
    PROCESSOR_NUMBER number;
    LONG pending;

    KPH_PAGED_CODE_PASSIVE();

    pending = 0;

    for (ULONG i = First; i < Last; i++)
    {
        if (Batch->Processors[i].Count)
        {
            pending++;
        }
    }

    if (!pending)
    {
        return FALSE;
    }

    KeClearEvent(&Batch->Done);
    Batch->Pending = pending;
    Batch->Active = pending;

    for (ULONG i = First; i < Last; i++)
    {
        processor = &Batch->Processors[i];

        if (!processor->Count)
        {
            continue;
        }

        processor->Batch = Batch;

        KsiInitializeDpc(&processor->Dpc,
                         KphDriverObject,
                         KphpLuaHwBatchDpc,
                         processor);

        status = KeGetProcessorNumberFromIndex(i, &number);
        if (NT_SUCCESS(status))
        {
            status = KeSetTargetProcessorDpcEx(&processor->Dpc.Dpc, &number);
        }

        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to target processor %lu: %!STATUS!",
                          i,
                          status);

            KphpLuaHwBatchFinish(Batch);
            continue;
        }

        KeSetImportanceDpc(&processor->Dpc.Dpc, HighImportance);

        if (!KsiInsertQueueDpc(&processor->Dpc, NULL, NULL))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to queue processor %lu",
                          i);

            KphpLuaHwBatchFinish(Batch);
        }
    }

    return TRUE;
}

/**
 * \brief Waits for the queued deferred routines of a batch to finish touching
 * it.
 *
 * \param[in] Batch The batch.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwBatchWait(
    _In_ PKPH_LUA_HW_BATCH Batch
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KeWaitForSingleObject(&Batch->Done, Executive, KernelMode, FALSE, NULL);

    while (ReadAcquire(&Batch->Active))
    {
        YieldProcessor();
    }
}

/**
 * \brief Does a list of items, each on its own processor.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Kind What the items do.
 * \param[in] ItemSize The size of an item.
 * \param[in] ResultSize The size of a result.
 * \param[in] Serial TRUE to run the processors one after another, FALSE to
 * run them at once.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwBatchRun(
    _In_ lua_State* State,
    _In_ KPH_LUA_HW_BATCH_KIND Kind,
    _In_ ULONG ItemSize,
    _In_ ULONG ResultSize,
    _In_ BOOLEAN Serial
    )
{
    int results;
    NTSTATUS status;
    PCSTR items;
    SIZE_T length;
    ULONG count;
    ULONG processorCount;
    SIZE_T processorsSize;
    SIZE_T itemsSize;
    SIZE_T resultsSize;
    SIZE_T orderSize;
    SIZE_T size;
    PKPH_LUA_HW_BATCH batch;
    int cleanup;

    KPH_PAGED_CODE_PASSIVE();

    items = luaL_checklstring(State, 1, &length);

    luaL_argcheck(State, (length % ItemSize) == 0, 1, "bad length");

    status = RtlSizeTToULong(length / ItemSize, &count);
    if (!NT_SUCCESS(status))
    {
        results = luaL_argerror(State, 1, "too long");
        goto Exit;
    }

    if (!count)
    {
        lua_pushliteral(State, "");
        results = 1;
        goto Exit;
    }

    KphScriptCharge(State, count);

#if defined(_M_X64)
    if (Kind == KphLuaHwBatchCr)
    {
        for (ULONG i = 0; i < count; i++)
        {
            ULONG number;

            number = ((PKPH_LUA_HW_REG_ITEM)items)[i].Register;

            luaL_argcheck(State,
                          (number == 0) || (number == 2) || (number == 3) ||
                          (number == 4) || (number == 8),
                          1,
                          "bad control register");
        }
    }
    else if (Kind == KphLuaHwBatchSmi)
    {
        for (ULONG i = 0; i < count; i++)
        {
            luaL_argcheck(State,
                          ((PKPH_LUA_HW_SMI_ITEM)items)[i].Port <= USHORT_MAX,
                          1,
                          "bad port");
        }
    }
#endif

    processorCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    NT_VERIFY(NT_SUCCESS(RtlSizeTMult(processorCount,
                                      sizeof(KPH_LUA_HW_BATCH_PROCESSOR),
                                      &processorsSize)));
    NT_VERIFY(NT_SUCCESS(RtlSizeTMult(count, ItemSize, &itemsSize)));
    NT_VERIFY(NT_SUCCESS(RtlSizeTMult(count, ResultSize, &resultsSize)));
    NT_VERIFY(NT_SUCCESS(RtlSizeTMult(count,
                                      sizeof(ULONG) * 2,
                                      &orderSize)));

    size = FIELD_OFFSET(KPH_LUA_HW_BATCH, Processors);

    NT_VERIFY(NT_SUCCESS(RtlSIZETAdd(size, processorsSize, &size)));
    NT_VERIFY(NT_SUCCESS(RtlSIZETAdd(size, resultsSize, &size)));
    NT_VERIFY(NT_SUCCESS(RtlSIZETAdd(size, itemsSize, &size)));
    NT_VERIFY(NT_SUCCESS(RtlSIZETAdd(size, orderSize, &size)));

    batch = KphAllocateNPaged(size, KPH_TAG_LUA_HARDWARE_BATCH);
    if (!batch)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a batch of %lu items",
                      count);

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    KphScriptPushCleanup(State, KphpLuaHwFreeBatch, batch);
    cleanup = lua_gettop(State);

    KeInitializeEvent(&batch->Done, NotificationEvent, FALSE);
    batch->Kind = Kind;
    batch->Count = count;
    batch->ProcessorCount = processorCount;
    batch->Results = Add2Ptr(batch->Processors, processorsSize);
    batch->Items = Add2Ptr(batch->Results, resultsSize);
    batch->Order = Add2Ptr(batch->Items, itemsSize);
    batch->ItemProcessors = &batch->Order[count];

    RtlCopyMemory(batch->Items, items, itemsSize);

    for (ULONG i = 0; i < count; i++)
    {
        *(PNTSTATUS)Add2Ptr(batch->Results, (SIZE_T)i * ResultSize) =
            STATUS_INVALID_PARAMETER;
    }

    KphpLuaHwBatchSort(batch, ItemSize);

    if (Serial)
    {
        for (ULONG i = 0; i < processorCount; i++)
        {
            if (KphpLuaHwBatchQueue(batch, i, i + 1))
            {
                KphpLuaHwBatchWait(batch);
            }
        }
    }
    else if (KphpLuaHwBatchQueue(batch, 0, processorCount))
    {
        KphpLuaHwBatchWait(batch);
    }

    lua_pushlstring(State, batch->Results, resultsSize);

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);

    results = 1;

Exit:

    return results;
}

#if defined(_M_X64)

/**
 * \brief Reads MSRs, each on its own processor (ksi.hw.msr_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMsrRead(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchMsr,
                             sizeof(KPH_LUA_HW_REG_ITEM),
                             sizeof(KPH_LUA_HW_REG_RESULT),
                             FALSE);
}

/**
 * \brief Writes MSRs, each on its own processor (ksi.hw.msr_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMsrWrite(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchMsrWrite,
                             sizeof(KPH_LUA_HW_REG_WRITE_ITEM),
                             sizeof(KPH_LUA_HW_REG_WRITE_RESULT),
                             FALSE);
}

/**
 * \brief Executes CPUID, each on its own processor (ksi.hw.cpuid).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwCpuid(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchCpuid,
                             sizeof(KPH_LUA_HW_CPUID_ITEM),
                             sizeof(KPH_LUA_HW_CPUID_RESULT),
                             FALSE);
}

/**
 * \brief Reads data MSRs through selector MSRs, one processor after another
 * (ksi.hw.msr_select_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwMsrSelectRead(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchMsrSelect,
                             sizeof(KPH_LUA_HW_SELECT_ITEM),
                             sizeof(KPH_LUA_HW_REG_RESULT),
                             TRUE);
}

/**
 * \brief Reads control registers, each on its own processor
 * (ksi.hw.cr_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwCrRead(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchCr,
                             sizeof(KPH_LUA_HW_REG_ITEM),
                             sizeof(KPH_LUA_HW_REG_RESULT),
                             FALSE);
}

/**
 * \brief Reads extended control registers, each on its own processor
 * (ksi.hw.xcr_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwXcrRead(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchXcr,
                             sizeof(KPH_LUA_HW_REG_ITEM),
                             sizeof(KPH_LUA_HW_REG_RESULT),
                             FALSE);
}

/**
 * \brief Reads performance counters, each on its own processor
 * (ksi.hw.pmc_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPmcRead(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchPmc,
                             sizeof(KPH_LUA_HW_REG_ITEM),
                             sizeof(KPH_LUA_HW_REG_RESULT),
                             FALSE);
}

/**
 * \brief Calls into firmware through a port write, each on its own processor
 * (ksi.hw.smi).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwSmi(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchSmi,
                             sizeof(KPH_LUA_HW_SMI_ITEM),
                             sizeof(KPH_LUA_HW_SMI_RESULT),
                             FALSE);
}

#elif defined(_M_ARM64)

/**
 * \brief Reads system registers, each on its own processor
 * (ksi.hw.sysreg_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwSysregRead(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchSysreg,
                             sizeof(KPH_LUA_HW_REG_ITEM),
                             sizeof(KPH_LUA_HW_REG_RESULT),
                             FALSE);
}

/**
 * \brief Writes system registers, each on its own processor
 * (ksi.hw.sysreg_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwSysregWrite(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchSysregWrite,
                             sizeof(KPH_LUA_HW_REG_WRITE_ITEM),
                             sizeof(KPH_LUA_HW_REG_WRITE_RESULT),
                             FALSE);
}

/**
 * \brief Reads data registers through selector registers, one processor after
 * another (ksi.hw.sysreg_select_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwSysregSelectRead(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchSysregSelect,
                             sizeof(KPH_LUA_HW_SELECT_ITEM),
                             sizeof(KPH_LUA_HW_REG_RESULT),
                             TRUE);
}

/**
 * \brief Calls into firmware through SMC #0, each on its own processor
 * (ksi.hw.smc).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwSmc(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchSmc,
                             sizeof(KPH_LUA_HW_CALL_ITEM),
                             sizeof(KPH_LUA_HW_CALL_RESULT),
                             FALSE);
}

/**
 * \brief Calls into firmware or the hypervisor through HVC #0, each on its
 * own processor (ksi.hw.hvc).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the string of results, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwHvc(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphpLuaHwBatchRun(State,
                             KphLuaHwBatchHvc,
                             sizeof(KPH_LUA_HW_CALL_ITEM),
                             sizeof(KPH_LUA_HW_CALL_RESULT),
                             FALSE);
}

#endif

/**
 * \brief Lists the active processors (ksi.hw.processors).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return One, a string of a group and a number for each active processor.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwProcessors(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    ULONG count;
    PROCESSOR_NUMBER processor;
    USHORT number;
    luaL_Buffer buffer;

    KPH_PAGED_CODE_PASSIVE();

    count = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    KphScriptCharge(State, count);

    luaL_buffinit(State, &buffer);

    for (ULONG i = 0; i < count; i++)
    {
        status = KeGetProcessorNumberFromIndex(i, &processor);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "KeGetProcessorNumberFromIndex failed: %!STATUS!",
                          status);
            continue;
        }

        number = processor.Number;

        luaL_addlstring(&buffer,
                        (PCSTR)&processor.Group,
                        sizeof(processor.Group));
        luaL_addlstring(&buffer, (PCSTR)&number, sizeof(number));
    }

    luaL_pushresult(&buffer);

    return 1;
}

/**
 * \brief Retrieves the processor the thread is running on
 * (ksi.hw.current_processor).
 *
 * \details The thread may move at any time, the result is a hint.
 *
 * \param[in] State The Lua state.
 *
 * \return Two, the group and the number of the processor.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwCurrentProcessor(
    _In_ lua_State* State
    )
{
    PROCESSOR_NUMBER processor;

    KPH_PAGED_CODE_PASSIVE();

    KeGetCurrentProcessorNumberEx(&processor);

    lua_pushinteger(State, processor.Group);
    lua_pushinteger(State, processor.Number);

    return 2;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwCpuLibrary[] =
{
#if defined(_M_X64)
    { "cpuid", KphpLuaHwCpuid },
    { "cr_read", KphpLuaHwCrRead },
    { "msr_read", KphpLuaHwMsrRead },
    { "msr_select_read", KphpLuaHwMsrSelectRead },
    { "msr_write", KphpLuaHwMsrWrite },
    { "pmc_read", KphpLuaHwPmcRead },
    { "smi", KphpLuaHwSmi },
    { "xcr_read", KphpLuaHwXcrRead },
#elif defined(_M_ARM64)
    { "hvc", KphpLuaHwHvc },
    { "smc", KphpLuaHwSmc },
    { "sysreg_read", KphpLuaHwSysregRead },
    { "sysreg_select_read", KphpLuaHwSysregSelectRead },
    { "sysreg_write", KphpLuaHwSysregWrite },
#endif
    { "current_processor", KphpLuaHwCurrentProcessor },
    { "processors", KphpLuaHwProcessors },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Initializes the processor functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareCpu(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();
}

/**
 * \brief Cleans up the processor functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareCpu(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();
}

/**
 * \brief Adds the processor functions to the hardware library.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state, with the library table on top.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareCpu(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_setfuncs(State, KphpLuaHwCpuLibrary, 0);
}
