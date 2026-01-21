/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2025-2026
 *
 */

#include <peview.h>
#include <thirdparty.h>
#include "..\thirdparty\zydis\Zydis.h"

// Sliding window (keep last N instructions)
#define PV_BACKSLICE_WINDOW_SIZE 400
// Scan on a worker thread and incrementally update UI
#define PV_APISCAN_STREAMING_DEFAULT 0
// Optional decoder streaming support (do not keep prior decoded instructions).
// When enabled, the scanner re-decodes backwards from the section bytes on CALL to resolve arguments.
#define PV_APISCAN_DECODER_STREAMING_DEFAULT 0

#define PV_BACKSLICE_REDECODE_MAX_INSTRUCTIONS 400
#define PV_MAX_INSTRUCTION_LENGTH 15
#define PV_REDECODE_SKIP_LIMIT 12

#define WM_PV_APISCAN_APPEND (WM_APP + 840)
#define WM_PV_APISCAN_DONE   (WM_APP + 841)

typedef struct _PV_API_SCAN_ROW
{
    ULONG64 CallVa;
    ULONG64 IatSlotVa;
    WCHAR Target[256];
    WCHAR Argument[512];
} PV_API_SCAN_ROW, *PPV_API_SCAN_ROW;

typedef struct _PV_API_SCAN_ROWS
{
    PPV_API_SCAN_ROW Items;
    ULONG Count;
    ULONG Capacity;
} PV_API_SCAN_ROWS, *PPV_API_SCAN_ROWS;

typedef struct _PV_IAT_MAP_ENTRY
{
    ULONG64 SlotVa;
    WCHAR Name[256];
} PV_IAT_MAP_ENTRY, *PPV_IAT_MAP_ENTRY;

typedef struct _PV_IAT_MAP
{
    PPV_IAT_MAP_ENTRY Items;
    ULONG Count;
    ULONG Capacity;
} PV_IAT_MAP, *PPV_IAT_MAP;

typedef struct _PV_APISCAN_PAGE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PPV_PROPPAGECONTEXT PropSheetContext;
    PH_LAYOUT_MANAGER LayoutManager;

    BOOLEAN IsGetProcAddressPage;

    PV_IAT_MAP IatMap;
    PV_API_SCAN_ROWS Rows;


    // Streaming support
    BOOLEAN StreamingEnabled;
    BOOLEAN DecoderStreamingEnabled;
    HANDLE ScanThreadHandle;
    ULONG FlushedCount;
    PH_QUEUED_LOCK RowsLock;
} PV_APISCAN_PAGE_CONTEXT, *PPV_APISCAN_PAGE_CONTEXT;

typedef struct _PV_INSTRUCTION_WINDOW
{
    ZydisDecodedInstruction Instructions[PV_BACKSLICE_WINDOW_SIZE];
    ZydisDecodedOperand Operands[PV_BACKSLICE_WINDOW_SIZE][ZYDIS_MAX_OPERAND_COUNT];
    ULONG64 InstructionPointers[PV_BACKSLICE_WINDOW_SIZE];
    ULONG Count;
    ULONG Head;
} PV_INSTRUCTION_WINDOW, *PPV_INSTRUCTION_WINDOW;

typedef enum _PV_VAL_KIND
{
    PvValUnknown,
    PvValConst,
    PvValAddr,
    PvValIatSlot,
    PvValStackSlot,
    PvValImport
} PV_VAL_KIND;

typedef struct _PV_VAL
{
    PV_VAL_KIND Kind;
    ULONG64 U;
} PV_VAL, *PPV_VAL;

PV_VAL PvBackSliceResolveRegRedecodeInternal(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG FromOffset,
    _In_ ZydisRegister Register,
    _In_ ULONG Depth,
    _In_ LONG64 SpDelta
    );

PV_VAL PvBackSliceResolveStackSlotStoreRedecode(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG FromOffset,
    _In_ ZydisRegister BaseReg,
    _In_ LONG64 SlotDispAtCall,
    _In_ ULONG Depth,
    _In_ LONG64 SpDelta
    );

VOID PvBuildIatMapForPage(
    _Inout_ PPV_APISCAN_PAGE_CONTEXT Context
    );

//
// Utilities and resolution helpers.
//

ZydisRegister PvNormalizeGprRegister(
    _In_ ZydisRegister Reg,
    _In_ BOOLEAN Is64
    )
{
    switch (Reg)
    {
    case ZYDIS_REGISTER_AL: case ZYDIS_REGISTER_AH: case ZYDIS_REGISTER_AX: case ZYDIS_REGISTER_EAX: case ZYDIS_REGISTER_RAX:
        return Is64 ? ZYDIS_REGISTER_RAX : ZYDIS_REGISTER_EAX;
    case ZYDIS_REGISTER_BL: case ZYDIS_REGISTER_BH: case ZYDIS_REGISTER_BX: case ZYDIS_REGISTER_EBX: case ZYDIS_REGISTER_RBX:
        return Is64 ? ZYDIS_REGISTER_RBX : ZYDIS_REGISTER_EBX;
    case ZYDIS_REGISTER_CL: case ZYDIS_REGISTER_CH: case ZYDIS_REGISTER_CX: case ZYDIS_REGISTER_ECX: case ZYDIS_REGISTER_RCX:
        return Is64 ? ZYDIS_REGISTER_RCX : ZYDIS_REGISTER_ECX;
    case ZYDIS_REGISTER_DL: case ZYDIS_REGISTER_DH: case ZYDIS_REGISTER_DX: case ZYDIS_REGISTER_EDX: case ZYDIS_REGISTER_RDX:
        return Is64 ? ZYDIS_REGISTER_RDX : ZYDIS_REGISTER_EDX;
    case ZYDIS_REGISTER_SPL: case ZYDIS_REGISTER_SP: case ZYDIS_REGISTER_ESP: case ZYDIS_REGISTER_RSP:
        return Is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP;
    case ZYDIS_REGISTER_BPL: case ZYDIS_REGISTER_BP: case ZYDIS_REGISTER_EBP: case ZYDIS_REGISTER_RBP:
        return Is64 ? ZYDIS_REGISTER_RBP : ZYDIS_REGISTER_EBP;
    case ZYDIS_REGISTER_SIL: case ZYDIS_REGISTER_SI: case ZYDIS_REGISTER_ESI: case ZYDIS_REGISTER_RSI:
        return Is64 ? ZYDIS_REGISTER_RSI : ZYDIS_REGISTER_ESI;
    case ZYDIS_REGISTER_DIL: case ZYDIS_REGISTER_DI: case ZYDIS_REGISTER_EDI: case ZYDIS_REGISTER_RDI:
        return Is64 ? ZYDIS_REGISTER_RDI : ZYDIS_REGISTER_EDI;
    case ZYDIS_REGISTER_R8B: case ZYDIS_REGISTER_R8W: case ZYDIS_REGISTER_R8D: case ZYDIS_REGISTER_R8:
        return ZYDIS_REGISTER_R8;
    case ZYDIS_REGISTER_R9B: case ZYDIS_REGISTER_R9W: case ZYDIS_REGISTER_R9D: case ZYDIS_REGISTER_R9:
        return ZYDIS_REGISTER_R9;
    case ZYDIS_REGISTER_R10B: case ZYDIS_REGISTER_R10W: case ZYDIS_REGISTER_R10D: case ZYDIS_REGISTER_R10:
        return ZYDIS_REGISTER_R10;
    case ZYDIS_REGISTER_R11B: case ZYDIS_REGISTER_R11W: case ZYDIS_REGISTER_R11D: case ZYDIS_REGISTER_R11:
        return ZYDIS_REGISTER_R11;
    case ZYDIS_REGISTER_R12B: case ZYDIS_REGISTER_R12W: case ZYDIS_REGISTER_R12D: case ZYDIS_REGISTER_R12:
        return ZYDIS_REGISTER_R12;
    case ZYDIS_REGISTER_R13B: case ZYDIS_REGISTER_R13W: case ZYDIS_REGISTER_R13D: case ZYDIS_REGISTER_R13:
        return ZYDIS_REGISTER_R13;
    case ZYDIS_REGISTER_R14B: case ZYDIS_REGISTER_R14W: case ZYDIS_REGISTER_R14D: case ZYDIS_REGISTER_R14:
        return ZYDIS_REGISTER_R14;
    case ZYDIS_REGISTER_R15B: case ZYDIS_REGISTER_R15W: case ZYDIS_REGISTER_R15D: case ZYDIS_REGISTER_R15:
        return ZYDIS_REGISTER_R15;
    }

    return Reg;
}

FORCEINLINE BOOLEAN PvIsSameRegister(
    _In_ ZydisRegister A,
    _In_ ZydisRegister B,
    _In_ BOOLEAN Is64
    )
{
    return PvNormalizeGprRegister(A, Is64) == PvNormalizeGprRegister(B, Is64);
}

_Success_(return)
BOOLEAN PvRedecodePreviousInstruction(
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _Inout_ PULONG CurrentOffset,
    _Out_ PULONG InstructionOffset,
    _Out_ ZydisDecodedInstruction* Instruction,
    _Out_writes_(ZYDIS_MAX_OPERAND_COUNT) ZydisDecodedOperand* Operands
    )
{
    ULONG offset = *CurrentOffset;

    for (ULONG skip = 0; skip < PV_REDECODE_SKIP_LIMIT; skip++)
    {
        if (offset == 0)
            return FALSE;

        ULONG cur = offset;

        for (ULONG back = 1; back <= PV_MAX_INSTRUCTION_LENGTH && back <= cur; back++)
        {
            ULONG start = cur - back;

            if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(Decoder, SectionBytes + start, SectionSize - start, Instruction, Operands)))
                continue;

            if (start + Instruction->length == cur)
            {
                *InstructionOffset = start;
                *CurrentOffset = start;
                return TRUE;
            }
        }

        offset--;
    }

    return FALSE;
}

VOID PvUpdateStackDeltaBackward(
    _In_ BOOLEAN Is64,
    _In_ const ZydisDecodedInstruction* Instruction,
    _In_reads_(ZYDIS_MAX_OPERAND_COUNT) const ZydisDecodedOperand* Operands,
    _In_ LONG64 SpDeltaPost,
    _Out_ PINT64 SpDeltaPre,
    _Out_ PBOOLEAN IsPush,
    _Out_ PBOOLEAN IsPop
    )
{
    LONG64 deltaPre = SpDeltaPost;
    BOOLEAN push = FALSE;
    BOOLEAN pop = FALSE;
    ULONG ptrsize = Is64 ? 8 : 4;

    switch (Instruction->mnemonic)
    {
    case ZYDIS_MNEMONIC_PUSH:
        push = TRUE;
        deltaPre = SpDeltaPost + ptrsize;
        break;
    case ZYDIS_MNEMONIC_POP:
        pop = TRUE;
        deltaPre = SpDeltaPost - ptrsize;
        break;
    case ZYDIS_MNEMONIC_CALL:
        deltaPre = SpDeltaPost + ptrsize;
        break;
    case ZYDIS_MNEMONIC_SUB:
        if (Instruction->operand_count_visible >= 2 &&
            Operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            PvIsSameRegister(Operands[0].reg.value, Is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, Is64) &&
            Operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        {
            deltaPre = SpDeltaPost + (LONG64)Operands[1].imm.value.u;
        }
        break;
    case ZYDIS_MNEMONIC_ADD:
        if (Instruction->operand_count_visible >= 2 &&
            Operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            PvIsSameRegister(Operands[0].reg.value, Is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, Is64) &&
            Operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        {
            deltaPre = SpDeltaPost - (LONG64)Operands[1].imm.value.u;
        }
        break;
    case ZYDIS_MNEMONIC_LEA:
        if (Instruction->operand_count_visible >= 2 &&
            Operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
            PvIsSameRegister(Operands[0].reg.value, Is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, Is64) &&
            Operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
            PvIsSameRegister(Operands[1].mem.base, Is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, Is64) &&
            Operands[1].mem.index == ZYDIS_REGISTER_NONE)
        {
            deltaPre = SpDeltaPost - (LONG64)Operands[1].mem.disp.value;
        }
        break;
    }

    *SpDeltaPre = deltaPre;
    *IsPush = push;
    *IsPop = pop;
}

BOOLEAN PvIsBarrierCategory(
    _In_ ZydisInstructionCategory Category
    )
{
    return Category == ZYDIS_CATEGORY_RET ||
        Category == ZYDIS_CATEGORY_SYSTEM;
}

ULONG64 PvRipTarget(
    _In_ ULONG64 InstructionPointer,
    _In_ UCHAR Length,
    _In_ LONG64 Displacement
    )
{
    return (ULONG64)((LONG64)InstructionPointer + (LONG64)Length + Displacement);
}

PV_VAL PvValUnknownType(
    VOID
    )
{
    PV_VAL value;
    value.Kind = PvValUnknown;
    value.U = 0;
    return value;
}

PV_VAL PvValConstType(
    _In_ ULONG64 Value
    )
{
    PV_VAL value;
    value.Kind = PvValConst;
    value.U = Value;
    return value;
}

PV_VAL PvValAddrType(
    _In_ ULONG64 Value
    )
{
    PV_VAL value;
    value.Kind = PvValAddr;
    value.U = Value;
    return value;
}

PV_VAL PvValIatType(
    _In_ ULONG64 Value
    )
{
    PV_VAL value;
    value.Kind = PvValIatSlot;
    value.U = Value;
    return value;
}

PV_VAL PvValStackSlotType(
    _In_ LONG64 DispAtCall
    )
{
    PV_VAL value;
    value.Kind = PvValStackSlot;
    value.U = (ULONG64)DispAtCall;
    return value;
}

PV_VAL PvValImportType(
    _In_ ULONG64 IatSlotVa
    )
{
    PV_VAL value;
    value.Kind = PvValImport;
    value.U = IatSlotVa;
    return value;
}

ULONG64 PvGetImageBase(
    VOID
    )
{
    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return PvMappedImage.NtHeaders64->OptionalHeader.ImageBase;
    else
        return PvMappedImage.NtHeaders32->OptionalHeader.ImageBase;
}

_Success_(return)
BOOLEAN PvReadIntegerAtVa(
    _In_ ULONG64 Va,
    _In_ ULONG SizeBits,
    _Out_ PULONG64 Value
    )
{
    PVOID va;

    va = PhMappedImageVaToVa(&PvMappedImage, Va, NULL);

    if (!va)
    {
        dprintf("PvReadIntegerAtVa: Failed to map VA 0x%llx\n", Va);
        return FALSE;
    }

    switch (SizeBits)
    {
    case 8: *Value = *(PUCHAR)va; break;
    case 16: *Value = *(PUSHORT)va; break;
    case 32: *Value = *(PULONG)va; break;
    case 64: *Value = *(PULONG64)va; break;
    default: 
        dprintf("PvReadIntegerAtVa: Unsupported size %u bits\n", SizeBits);
        return FALSE;
    }

    return TRUE;
}

PCWSTR PvFindIatName(
    _In_ PPV_IAT_MAP IatMap,
    _In_ ULONG64 Va
    )
{
    for (ULONG i = 0; i < IatMap->Count; i++)
    {
        if (IatMap->Items[i].SlotVa == Va)
            return IatMap->Items[i].Name;
    }

    return NULL;
}

VOID PvEnsureRowsCapacity(
    _Inout_ PPV_API_SCAN_ROWS Rows,
    _In_ ULONG ExtraCapacity
    )
{
    if (Rows->Count + ExtraCapacity > Rows->Capacity)
    {
        Rows->Capacity = max(Rows->Capacity * 2, Rows->Count + ExtraCapacity);
        Rows->Capacity = max(Rows->Capacity, 16);
        Rows->Items = PhReAllocate(Rows->Items, Rows->Capacity * sizeof(PV_API_SCAN_ROW));
    }
}

VOID PvEnsureIatCapacity(
    _Inout_ PPV_IAT_MAP IatMap,
    _In_ ULONG ExtraCapacity
    )
{
    if (IatMap->Count + ExtraCapacity > IatMap->Capacity)
    {
        IatMap->Capacity = max(IatMap->Capacity * 2, IatMap->Count + ExtraCapacity);
        IatMap->Capacity = max(IatMap->Capacity, 16);
        IatMap->Items = PhReAllocate(IatMap->Items, IatMap->Capacity * sizeof(PV_IAT_MAP_ENTRY));
    }
}

BOOLEAN PvIsTargetForPage(
    _In_ BOOLEAN IsGetProcAddressPage,
    _In_ PCSTR DllName,
    _In_ PCSTR ExportName
    )
{
    if (IsGetProcAddressPage)
    {
        return
            _stricmp(ExportName, "GetProcAddress") == 0 ||
            _stricmp(ExportName, "LdrGetProcedureAddress") == 0 ||
            _stricmp(ExportName, "LdrGetProcedureAddressForCaller") == 0;
    }
    else
    {
        return
            _stricmp(ExportName, "LoadLibraryA") == 0 ||
            _stricmp(ExportName, "LoadLibraryW") == 0 ||
            _stricmp(ExportName, "LoadLibraryExA") == 0 ||
            _stricmp(ExportName, "LoadLibraryExW") == 0 ||
            _stricmp(ExportName, "LdrLoadDll") == 0;
    }
}

VOID PvReadStringAtVa(
    _In_ ULONG64 Va,
    _Out_writes_(BufferChars) PWSTR Buffer,
    _In_ ULONG BufferChars
    )
{
    PVOID va;

    Buffer[0] = UNICODE_NULL;

    va = PhMappedImageVaToVa(&PvMappedImage, Va, NULL);

    if (!va)
    {
        dprintf("PvReadStringAtVa: Failed to map VA 0x%llx\n", Va);
        return;
    }

    // Detect if this is a Unicode string or ANSI string.
    if (((PUCHAR)va)[0] != 0 && ((PUCHAR)va)[1] == 0)
    {
        // Likely Unicode (UTF-16LE)
        SIZE_T length = 0;
        PWCHAR s = (PWCHAR)va;
        while (s[length] != UNICODE_NULL && length < BufferChars - 1)
        {
            Buffer[length] = s[length];
            length++;
        }
        Buffer[length] = UNICODE_NULL;
    }
    else
    {
        // Likely ANSI
        PhCopyStringZFromUtf8(va, SIZE_MAX, Buffer, BufferChars, NULL);
    }

    dprintf("PvReadStringAtVa: Resolved 0x%llx to \"%S\"\n", Va, Buffer);
}

_Success_(return)
BOOLEAN PvReadAnsiStringStructAtVa(
    _In_ ULONG64 Va,
    _Out_writes_(BufferChars) PWSTR Buffer,
    _In_ ULONG BufferChars
    )
{
    PVOID va;
    BOOLEAN is64 = (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
    ULONG64 bufferVa;
    USHORT length;

    va = PhMappedImageVaToVa(&PvMappedImage, Va, NULL);
    if (!va)
    {
        dprintf("PvReadAnsiStringStructAtVa: Failed to map struct at 0x%llx\n", Va);
        return FALSE;
    }

    if (is64)
    {
        length = *(PUSHORT)va;
        bufferVa = *(PULONG64)PTR_ADD_OFFSET(va, 8);
    }
    else
    {
        length = *(PUSHORT)va;
        bufferVa = *(PULONG)PTR_ADD_OFFSET(va, 4);
    }

    dprintf("PvReadAnsiStringStructAtVa: Struct at 0x%llx has BufferVa 0x%llx, Length %u\n", Va, bufferVa, length);

    if (!bufferVa) return FALSE;

    PVOID stringData = PhMappedImageVaToVa(&PvMappedImage, bufferVa, NULL);
    if (!stringData)
    {
        dprintf("PvReadAnsiStringStructAtVa: Failed to map string data at 0x%llx\n", bufferVa);
        return FALSE;
    }

    PhCopyStringZFromUtf8(stringData, length, Buffer, BufferChars, NULL);
    dprintf("PvReadAnsiStringStructAtVa: Resolved to \"%S\"\n", Buffer);
    return TRUE;
}

_Success_(return)
BOOLEAN PvReadUnicodeStringStructAtVa(
    _In_ ULONG64 Va,
    _Out_writes_(BufferChars) PWSTR Buffer,
    _In_ ULONG BufferChars
    )
{
    PVOID va;
    BOOLEAN is64 = (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
    ULONG64 bufferVa;
    USHORT length;

    va = PhMappedImageVaToVa(&PvMappedImage, Va, NULL);
    if (!va)
    {
        dprintf("PvReadUnicodeStringStructAtVa: Failed to map struct at 0x%llx\n", Va);
        return FALSE;
    }

    if (is64)
    {
        length = *(PUSHORT)va;
        bufferVa = *(PULONG64)PTR_ADD_OFFSET(va, 8);
    }
    else
    {
        length = *(PUSHORT)va;
        bufferVa = *(PULONG)PTR_ADD_OFFSET(va, 4);
    }

    dprintf("PvReadUnicodeStringStructAtVa: Struct at 0x%llx has BufferVa 0x%llx, Length %u\n", Va, bufferVa, length);

    if (!bufferVa) return FALSE;

    PVOID stringData = PhMappedImageVaToVa(&PvMappedImage, bufferVa, NULL);
    if (!stringData)
    {
        dprintf("PvReadUnicodeStringStructAtVa: Failed to map string data at 0x%llx\n", bufferVa);
        return FALSE;
    }

    length /= sizeof(WCHAR);
    if (length >= BufferChars) length = (USHORT)BufferChars - 1;

    memcpy(Buffer, stringData, length * sizeof(WCHAR));
    Buffer[length] = UNICODE_NULL;
    dprintf("PvReadUnicodeStringStructAtVa: Resolved to \"%S\"\n", Buffer);
    return TRUE;
}

VOID PvAddInstructionToWindow(
    _Inout_ PPV_INSTRUCTION_WINDOW Window,
    _In_ const ZydisDecodedInstruction* Instruction,
    _In_reads_(ZYDIS_MAX_OPERAND_COUNT) const ZydisDecodedOperand* Operands,
    _In_ ULONG64 IP
    )
{
    Window->Instructions[Window->Head] = *Instruction;
    memcpy(Window->Operands[Window->Head], Operands, sizeof(ZydisDecodedOperand) * ZYDIS_MAX_OPERAND_COUNT);
    Window->InstructionPointers[Window->Head] = IP;

    Window->Head = (Window->Head + 1) % PV_BACKSLICE_WINDOW_SIZE;
    if (Window->Count < PV_BACKSLICE_WINDOW_SIZE)
        Window->Count++;
}

// Implementation of resolution logic.

PV_VAL PvBackSliceResolveStackSlotStoreRedecode(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG FromOffset,
    _In_ ZydisRegister BaseReg,
    _In_ LONG64 SlotDispAtCall,
    _In_ ULONG Depth,
    _In_ LONG64 SpDelta
    )
{
    ULONG scanned = 0;
    ULONG cur = FromOffset;
    BOOLEAN is64 = (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);

    if (Depth > 20)
    {
        dprintf("PvBackSliceResolveStackSlotStoreRedecode: Depth limit reached\n");
        return PvValUnknownType();
    }

    dprintf("PvBackSliceResolveStackSlotStoreRedecode: Resolving [BaseReg %u + 0x%llx] FromOffset 0x%x SpDelta 0x%llx\n", 
        BaseReg, SlotDispAtCall, FromOffset, SpDelta);

    while (scanned < PV_BACKSLICE_REDECODE_MAX_INSTRUCTIONS)
    {
        ZydisDecodedInstruction instruction;
        ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
        ULONG instOffset;
        LONG64 spDeltaPre;
        BOOLEAN isPush, isPop;
        LONG64 spDeltaPost = SpDelta;

        if (!PvRedecodePreviousInstruction(Decoder, SectionBytes, SectionSize, &cur, &instOffset, &instruction, ops))
            break;

        PvUpdateStackDeltaBackward(
            is64,
            &instruction,
            ops,
            spDeltaPost,
            &spDeltaPre,
            &isPush,
            &isPop
            );
        SpDelta = spDeltaPre;
        scanned++;

        if (PvIsBarrierCategory(instruction.meta.category))
        {
            dprintf("PvBackSliceResolveStackSlotStoreRedecode: Barrier reached (%s) at 0x%llx\n", 
                ZydisMnemonicGetString(instruction.mnemonic), SectionBaseVa + instOffset);
            break;
        }

        if (instruction.mnemonic == ZYDIS_MNEMONIC_PUSH)
        {
            LONG64 storeDispAtCall = spDeltaPost;

            if (PvIsSameRegister(
                BaseReg,
                is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP,
                is64
                ) && storeDispAtCall == SlotDispAtCall)
            {
                const ZydisDecodedOperand* pushOp = &ops[0];

                dprintf("PvBackSliceResolveStackSlotStoreRedecode: Found PUSH to target slot at offset 0x%x (0x%llx)\n", 
                    instOffset, SectionBaseVa + instOffset);

                if (pushOp->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                {
                    return PvValConstType((ULONG64)pushOp->imm.value.u);
                }

                if (pushOp->type == ZYDIS_OPERAND_TYPE_REGISTER)
                {
                    return PvBackSliceResolveRegRedecodeInternal(
                        Context,
                        Decoder,
                        SectionBytes,
                        SectionSize,
                        SectionBaseVa,
                        instOffset,
                        pushOp->reg.value,
                        Depth + 1,
                        SpDelta
                        );
                }
            }

            continue;
        }

        if (instruction.operand_count_visible < 2)
            continue;

        const ZydisDecodedOperand* dst = &ops[0];
        const ZydisDecodedOperand* src = &ops[1];

        if (dst->type != ZYDIS_OPERAND_TYPE_MEMORY)
            continue;

        if (!PvIsSameRegister(dst->mem.base, BaseReg, is64))
            continue;

        if (dst->mem.index != ZYDIS_REGISTER_NONE)
            continue;

        LONG64 storeDispAtCall = (LONG64)dst->mem.disp.value;
        if (PvIsSameRegister(BaseReg, is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, is64))
            storeDispAtCall += SpDelta;

        if (storeDispAtCall != SlotDispAtCall)
            continue;

        dprintf("PvBackSliceResolveStackSlotStoreRedecode: Found MOV to target slot at offset 0x%x (0x%llx)\n", 
            instOffset, SectionBaseVa + instOffset);

        if (instruction.mnemonic == ZYDIS_MNEMONIC_MOV)
        {
            if (src->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                return PvValConstType((ULONG64)src->imm.value.u);

            if (src->type == ZYDIS_OPERAND_TYPE_REGISTER)
                return PvBackSliceResolveRegRedecodeInternal(Context, Decoder, SectionBytes, SectionSize, SectionBaseVa, instOffset, src->reg.value, Depth + 1, SpDelta);
        }
    }

    return PvValUnknownType();
}

PV_VAL PvBackSliceResolveRegRedecodeInternal(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG FromOffset,
    _In_ ZydisRegister Register,
    _In_ ULONG Depth,
    _In_ LONG64 SpDelta
    )
{
    ULONG scanned = 0;
    ULONG cur = FromOffset;
    BOOLEAN is64 = (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
    ZydisRegister regNorm = PvNormalizeGprRegister(Register, is64);

    if (Depth > 20)
    {
        dprintf("PvBackSliceResolveRegRedecodeInternal: Depth limit reached\n");
        return PvValUnknownType();
    }

    while (scanned < PV_BACKSLICE_REDECODE_MAX_INSTRUCTIONS)
    {
        ZydisDecodedInstruction instruction;
        ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
        ULONG instOffset;
        LONG64 spDeltaPre;
        BOOLEAN isPush, isPop;
        LONG64 spDeltaPost = SpDelta;

        if (!PvRedecodePreviousInstruction(Decoder, SectionBytes, SectionSize, &cur, &instOffset, &instruction, ops))
            break;

        PvUpdateStackDeltaBackward(
            is64,
            &instruction,
            ops,
            spDeltaPost,
            &spDeltaPre,
            &isPush,
            &isPop
            );
        SpDelta = spDeltaPre;
        scanned++;

        if (PvIsBarrierCategory(instruction.meta.category))
        {
            dprintf("PvBackSliceResolveRegRedecodeInternal: Barrier reached (%s) at 0x%llx\n", 
                ZydisMnemonicGetString(instruction.mnemonic), SectionBaseVa + instOffset);
            break;
        }

        if (
            instruction.mnemonic == ZYDIS_MNEMONIC_CALL &&
            (regNorm == ZYDIS_REGISTER_RAX || regNorm == ZYDIS_REGISTER_EAX)
            )
        {
            if (ops[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                ULONG64 memoryVa = 0;

                if (is64 && ops[0].mem.base == ZYDIS_REGISTER_RIP)
                    memoryVa = PvRipTarget(SectionBaseVa + (ULONG64)instOffset, instruction.length, ops[0].mem.disp.value);
                else if (ops[0].mem.base == ZYDIS_REGISTER_NONE)
                    memoryVa = (ULONG64)ops[0].mem.disp.value;

                if (memoryVa && PvFindIatName(&Context->IatMap, memoryVa))
                {
                    dprintf("PvBackSliceResolveRegRedecodeInternal: Found CALL to IAT at 0x%llx (RAX is now PvValImport)\n", memoryVa);
                    return PvValImportType(memoryVa);
                }
            }
            else if (ops[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                PV_VAL targetVal = PvBackSliceResolveRegRedecodeInternal(
                    Context,
                    Decoder,
                    SectionBytes,
                    SectionSize,
                    SectionBaseVa,
                    instOffset,
                    ops[0].reg.value,
                    Depth + 1,
                    SpDelta
                    );

                if (targetVal.Kind == PvValIatSlot)
                {
                    dprintf("PvBackSliceResolveRegRedecodeInternal: Found register CALL to IAT at 0x%llx (RAX is now PvValImport)\n", targetVal.U);
                    return PvValImportType(targetVal.U);
                }
            }
        }

        if (instruction.operand_count_visible < 2)
            continue;

        const ZydisDecodedOperand* dst = &ops[0];
        const ZydisDecodedOperand* src = &ops[1];

        if (dst->type != ZYDIS_OPERAND_TYPE_REGISTER || !PvIsSameRegister(dst->reg.value, regNorm, is64))
            continue;

        dprintf("PvBackSliceResolveRegRedecodeInternal: Found write to target register at offset 0x%x (0x%llx) (%s)\n", 
            instOffset, SectionBaseVa + instOffset, ZydisMnemonicGetString(instruction.mnemonic));

        if (
            instruction.mnemonic == ZYDIS_MNEMONIC_XOR &&
            src->type == ZYDIS_OPERAND_TYPE_REGISTER &&
            PvIsSameRegister(src->reg.value, regNorm, is64)
            )
        {
            return PvValConstType(0);
        }

        if (
            instruction.mnemonic == ZYDIS_MNEMONIC_MOV ||
            instruction.mnemonic == ZYDIS_MNEMONIC_MOVZX || 
            instruction.mnemonic == ZYDIS_MNEMONIC_MOVSX || 
            instruction.mnemonic == ZYDIS_MNEMONIC_MOVSXD
            )
        {
            if (src->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            {
                return PvValConstType((ULONG64)src->imm.value.u);
            }

            if (src->type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                return PvBackSliceResolveRegRedecodeInternal(
                    Context,
                    Decoder,
                    SectionBytes,
                    SectionSize,
                    SectionBaseVa,
                    instOffset,
                    src->reg.value,
                    Depth + 1,
                    SpDelta
                    );
            }

            if (src->type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                ULONG64 memoryVa = 0;
                BOOLEAN vaResolved = FALSE;

                if (is64 && src->mem.base == ZYDIS_REGISTER_RIP)
                {
                    memoryVa = PvRipTarget(SectionBaseVa + (ULONG64)instOffset, instruction.length, src->mem.disp.value);
                    vaResolved = TRUE;
                }
                else if (src->mem.base == ZYDIS_REGISTER_NONE)
                {
                    memoryVa = (ULONG64)src->mem.disp.value;
                    vaResolved = TRUE;
                }
                else if (src->mem.index == ZYDIS_REGISTER_NONE)
                {
                    if (
                        PvIsSameRegister(src->mem.base, is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, is64) ||
                        PvIsSameRegister(src->mem.base, is64 ? ZYDIS_REGISTER_RBP : ZYDIS_REGISTER_EBP, is64)
                        )
                    {
                        LONG64 slotDispAtCall = (LONG64)src->mem.disp.value;

                        if (PvIsSameRegister(src->mem.base, is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, is64))
                            slotDispAtCall += SpDelta;

                        return PvBackSliceResolveStackSlotStoreRedecode(
                            Context,
                            Decoder,
                            SectionBytes,
                            SectionSize,
                            SectionBaseVa,
                            instOffset,
                            src->mem.base,
                            slotDispAtCall,
                            Depth + 1,
                            SpDelta
                            );
                    }
                    else
                    {
                        PV_VAL base = PvBackSliceResolveRegRedecodeInternal(
                            Context,
                            Decoder,
                            SectionBytes,
                            SectionSize,
                            SectionBaseVa,
                            instOffset,
                            src->mem.base,
                            Depth + 1,
                            SpDelta
                            );

                        if (base.Kind == PvValAddr || base.Kind == PvValConst || base.Kind == PvValStackSlot)
                        {
                            memoryVa = (ULONG64)((LONG64)base.U + (LONG64)src->mem.disp.value);
                            vaResolved = TRUE;
                        }
                    }
                }

                if (vaResolved)
                {
                    if (PvFindIatName(&Context->IatMap, memoryVa))
                        return PvValIatType(memoryVa);

                    ULONG64 pointerValue;
                    if (PvReadIntegerAtVa(memoryVa, src->size, &pointerValue))
                    {
                        dprintf("PvBackSliceResolveRegRedecodeInternal: Resolved load from 0x%llx to 0x%llx\n", memoryVa, pointerValue);
                        return PvValAddrType(pointerValue);
                    }

                    return PvValAddrType(memoryVa);
                }
            }

            return PvValUnknownType();
        }

        if (instruction.mnemonic == ZYDIS_MNEMONIC_MOVZX || instruction.mnemonic == ZYDIS_MNEMONIC_MOVSX || instruction.mnemonic == ZYDIS_MNEMONIC_MOVSXD)
        {
            if (src->type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                return PvBackSliceResolveRegRedecodeInternal(
                    Context,
                    Decoder,
                    SectionBytes,
                    SectionSize,
                    SectionBaseVa,
                    instOffset,
                    src->reg.value,
                    Depth + 1,
                    SpDelta
                    );
            }

            if (src->type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                ULONG64 memoryVa = 0;

                if (is64 && src->mem.base == ZYDIS_REGISTER_RIP)
                    memoryVa = PvRipTarget(SectionBaseVa + (ULONG64)instOffset, instruction.length, src->mem.disp.value);
                else if (src->mem.base == ZYDIS_REGISTER_NONE)
                    memoryVa = (ULONG64)src->mem.disp.value;

                if (memoryVa)
                {
                    ULONG64 v;
                    if (PvReadIntegerAtVa(memoryVa, src->size, &v))
                        return PvValConstType(v);
                }
            }

            return PvValUnknownType();
        }

        if (instruction.mnemonic == ZYDIS_MNEMONIC_LEA && src->type == ZYDIS_OPERAND_TYPE_MEMORY)
        {
            if (src->mem.index == ZYDIS_REGISTER_NONE &&
                (PvIsSameRegister(src->mem.base, is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, is64) ||
                PvIsSameRegister(src->mem.base, is64 ? ZYDIS_REGISTER_RBP : ZYDIS_REGISTER_EBP, is64)))
            {
                LONG64 dispAtCall = (LONG64)src->mem.disp.value;

                if (PvIsSameRegister(src->mem.base, is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, is64))
                    dispAtCall += SpDelta;

                return PvValStackSlotType(dispAtCall);
            }

            if (is64 && src->mem.base == ZYDIS_REGISTER_RIP)
            {
                ULONG64 address = PvRipTarget(
                    SectionBaseVa + (ULONG64)instOffset,
                    instruction.length,
                    src->mem.disp.value
                    );

                if (PvFindIatName(&Context->IatMap, address))
                    return PvValIatType(address);

                return PvValAddrType(address);
            }

            if (src->mem.base == ZYDIS_REGISTER_NONE)
            {
                ULONG64 address = (ULONG64)src->mem.disp.value;

                if (PvFindIatName(&Context->IatMap, address))
                    return PvValIatType(address);

                return PvValAddrType(address);
            }

            if (src->mem.index == ZYDIS_REGISTER_NONE && src->mem.base != ZYDIS_REGISTER_NONE)
            {
                PV_VAL base = PvBackSliceResolveRegRedecodeInternal(
                    Context,
                    Decoder,
                    SectionBytes,
                    SectionSize,
                    SectionBaseVa,
                    instOffset,
                    src->mem.base,
                    Depth + 1,
                    SpDelta
                    );

                if (base.Kind == PvValAddr || base.Kind == PvValConst || base.Kind == PvValStackSlot)
                {
                    base.U = (ULONG64)((LONG64)base.U + (LONG64)src->mem.disp.value);
                    return base;
                }
            }

            return PvValUnknownType();
        }

        if ((
            instruction.mnemonic == ZYDIS_MNEMONIC_ADD || 
            instruction.mnemonic == ZYDIS_MNEMONIC_SUB ||
            instruction.mnemonic == ZYDIS_MNEMONIC_AND
            ) && src->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        {
            PV_VAL base = PvBackSliceResolveRegRedecodeInternal(
                Context,
                Decoder,
                SectionBytes,
                SectionSize,
                SectionBaseVa,
                instOffset,
                regNorm,
                Depth + 1,
                SpDelta
                );

            if (base.Kind == PvValAddr || base.Kind == PvValConst || base.Kind == PvValStackSlot)
            {
                LONG64 imm = (LONG64)src->imm.value.u;

                if (instruction.mnemonic == ZYDIS_MNEMONIC_ADD)
                    base.U = (ULONG64)((LONG64)base.U + imm);
                else if (instruction.mnemonic == ZYDIS_MNEMONIC_SUB)
                    base.U = (ULONG64)((LONG64)base.U - imm);
                else // AND
                    base.U = (ULONG64)((LONG64)base.U & imm);

                return base;
            }

            return PvValUnknownType();
        }

        dprintf("PvBackSliceResolveRegRedecodeInternal: Instruction at 0x%llx handled but not fully resolved, stopping scan.\n", SectionBaseVa + instOffset);
        return PvValUnknownType();
    }

    return PvValUnknownType();
}

PV_VAL PvBackSliceResolveRegRedecode(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG CallOffset,
    _In_ ZydisRegister Register
    )
{
    return PvBackSliceResolveRegRedecodeInternal(
        Context,
        Decoder,
        SectionBytes,
        SectionSize,
        SectionBaseVa,
        CallOffset,
        Register,
        0,
        0
        );
}

PV_VAL PvBackSliceResolveRegWindow(
    _In_ PPV_INSTRUCTION_WINDOW Window,
    _In_ ULONG CurrentIndex,
    _In_ ZydisRegister Register,
    _In_ PPV_IAT_MAP IatMap,
    _In_ ULONG Depth
    )
{
    ULONG maxScan = min(Window->Count - 1, PV_BACKSLICE_WINDOW_SIZE);
    BOOLEAN is64 = (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
    ZydisRegister regNorm = PvNormalizeGprRegister(Register, is64);

    if (Depth > 20)
    {
        dprintf("PvBackSliceResolveRegWindow: Depth limit reached\n");
        return PvValUnknownType();
    }

    dprintf("PvBackSliceResolveRegWindow: Resolving %s, maxScan %u, Depth %u\n", 
        ZydisRegisterGetString(Register), maxScan, Depth);

    for (ULONG i = 1; i <= maxScan; i++)
    {
        ULONG windowIndex = (CurrentIndex + PV_BACKSLICE_WINDOW_SIZE - i) % PV_BACKSLICE_WINDOW_SIZE;

        if (i > Window->Count)
            break;

        const ZydisDecodedInstruction* instruction = &Window->Instructions[windowIndex];
        const ZydisDecodedOperand* ops = Window->Operands[windowIndex];

        if (PvIsBarrierCategory(instruction->meta.category))
        {
            dprintf("PvBackSliceResolveRegWindow: Barrier reached (%s) at 0x%llx\n", ZydisMnemonicGetString(instruction->mnemonic), Window->InstructionPointers[windowIndex]);
            break;
        }

        if (
            instruction->mnemonic == ZYDIS_MNEMONIC_CALL &&
            (regNorm == ZYDIS_REGISTER_RAX || regNorm == ZYDIS_REGISTER_EAX)
            )
        {
            if (ops[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                ULONG64 memoryVa = 0;

                if (is64 && ops[0].mem.base == ZYDIS_REGISTER_RIP)
                    memoryVa = PvRipTarget(Window->InstructionPointers[windowIndex], instruction->length, ops[0].mem.disp.value);
                else if (ops[0].mem.base == ZYDIS_REGISTER_NONE)
                    memoryVa = (ULONG64)ops[0].mem.disp.value;

                if (memoryVa && PvFindIatName(IatMap, memoryVa))
                {
                    dprintf("PvBackSliceResolveRegWindow: Found CALL to IAT at 0x%llx (RAX is now PvValImport)\n", memoryVa);
                    return PvValImportType(memoryVa);
                }
            }
            else if (ops[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                PV_VAL targetVal = PvBackSliceResolveRegWindow(
                    Window,
                    windowIndex,
                    ops[0].reg.value,
                    IatMap,
                    Depth + 1
                    );

                if (targetVal.Kind == PvValIatSlot)
                {
                    dprintf("PvBackSliceResolveRegWindow: Found register CALL to IAT at 0x%llx (RAX is now PvValImport)\n", targetVal.U);
                    return PvValImportType(targetVal.U);
                }
            }
        }

        if (instruction->operand_count_visible < 2)
            continue;

        const ZydisDecodedOperand* dst = &ops[0];
        const ZydisDecodedOperand* src = &ops[1];

        if (dst->type != ZYDIS_OPERAND_TYPE_REGISTER || !PvIsSameRegister(dst->reg.value, regNorm, is64))
            continue;

        dprintf("PvBackSliceResolveRegWindow: Found write to target %s at window index %u (%s) at 0x%llx\n", 
            ZydisRegisterGetString(dst->reg.value), windowIndex, ZydisMnemonicGetString(instruction->mnemonic), Window->InstructionPointers[windowIndex]);

        if (
            instruction->mnemonic == ZYDIS_MNEMONIC_XOR &&
            src->type == ZYDIS_OPERAND_TYPE_REGISTER &&
            PvIsSameRegister(src->reg.value, regNorm, is64)
            )
        {
            return PvValConstType(0);
        }

        if (
            instruction->mnemonic == ZYDIS_MNEMONIC_MOV ||
            instruction->mnemonic == ZYDIS_MNEMONIC_MOVZX || 
            instruction->mnemonic == ZYDIS_MNEMONIC_MOVSX || 
            instruction->mnemonic == ZYDIS_MNEMONIC_MOVSXD
            )
        {
            if (src->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            {
                dprintf("PvBackSliceResolveRegWindow: %s %s, 0x%llx\n", 
                    ZydisMnemonicGetString(instruction->mnemonic), ZydisRegisterGetString(dst->reg.value), src->imm.value.u);
                return PvValConstType((ULONG64)src->imm.value.u);
            }

            if (src->type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                dprintf("PvBackSliceResolveRegWindow: %s %s, %s (recursing)\n", 
                    ZydisMnemonicGetString(instruction->mnemonic), ZydisRegisterGetString(dst->reg.value), ZydisRegisterGetString(src->reg.value));
                return PvBackSliceResolveRegWindow(Window, windowIndex, src->reg.value, IatMap, Depth + 1);
            }

            if (src->type == ZYDIS_OPERAND_TYPE_MEMORY)
            {
                ULONG64 memoryVa = 0;
                BOOLEAN vaResolved = FALSE;

                dprintf(
                    "PvBackSliceResolveRegWindow: %s %s, [%s + 0x%llx]\n", 
                    ZydisMnemonicGetString(instruction->mnemonic),
                    ZydisRegisterGetString(dst->reg.value),
                    ZydisRegisterGetString(src->mem.base),
                    src->mem.disp.value
                    );

                if (src->mem.base == ZYDIS_REGISTER_RIP)
                {
                    memoryVa = PvRipTarget(
                        Window->InstructionPointers[windowIndex],
                        instruction->length,
                        src->mem.disp.value
                        );
                    vaResolved = TRUE;
                }
                else if (src->mem.base == ZYDIS_REGISTER_NONE)
                {
                    memoryVa = (ULONG64)src->mem.disp.value;
                    vaResolved = TRUE;
                }
                else if (src->mem.index == ZYDIS_REGISTER_NONE)
                {
                    PV_VAL base = PvBackSliceResolveRegWindow(
                        Window,
                        windowIndex,
                        src->mem.base,
                        IatMap,
                        Depth + 1
                        );

                    if (base.Kind == PvValAddr || base.Kind == PvValConst || base.Kind == PvValStackSlot)
                    {
                        memoryVa = (ULONG64)((LONG64)base.U + (LONG64)src->mem.disp.value);
                        vaResolved = TRUE;
                    }
                }

                if (vaResolved)
                {
                    if (PvFindIatName(IatMap, memoryVa))
                    {
                        dprintf("PvBackSliceResolveRegWindow: Resolved to IatSlot 0x%llx\n", memoryVa);
                        return PvValIatType(memoryVa);
                    }
                    
                    ULONG64 pointerValue;
                    if (PvReadIntegerAtVa(memoryVa, src->size, &pointerValue))
                    {
                        dprintf("PvBackSliceResolveRegWindow: Resolved load from 0x%llx to 0x%llx\n", memoryVa, pointerValue);
                        return PvValAddrType(pointerValue);
                    }

                    dprintf("PvBackSliceResolveRegWindow: Resolved to Addr 0x%llx (load failed)\n", memoryVa);
                    return PvValAddrType(memoryVa);
                }
            }
        }

        if (
            instruction->mnemonic == ZYDIS_MNEMONIC_LEA &&
            src->type == ZYDIS_OPERAND_TYPE_MEMORY
            )
        {
            dprintf(
                "PvBackSliceResolveRegWindow: LEA %s, [%s + 0x%llx]\n", 
                ZydisRegisterGetString(dst->reg.value),
                ZydisRegisterGetString(src->mem.base),
                src->mem.disp.value
                );

            if (src->mem.base == ZYDIS_REGISTER_RIP)
            {
                ULONG64 address = PvRipTarget(
                    Window->InstructionPointers[windowIndex],
                    instruction->length,
                    src->mem.disp.value
                    );

                if (PvFindIatName(IatMap, address))
                    return PvValIatType(address);

                return PvValAddrType(address);
            }
            
            if (src->mem.base == ZYDIS_REGISTER_NONE)
            {
                ULONG64 address = (ULONG64)src->mem.disp.value;

                if (PvFindIatName(IatMap, address))
                    return PvValIatType(address);

                return PvValAddrType(address);
            }

            if (src->mem.index == ZYDIS_REGISTER_NONE && src->mem.base != ZYDIS_REGISTER_NONE)
            {
                PV_VAL base = PvBackSliceResolveRegWindow(
                    Window,
                    windowIndex,
                    src->mem.base,
                    IatMap,
                    Depth + 1
                    );

                if (base.Kind == PvValAddr || base.Kind == PvValConst || base.Kind == PvValStackSlot)
                {
                    base.U = (ULONG64)((LONG64)base.U + (LONG64)src->mem.disp.value);
                    dprintf("PvBackSliceResolveRegWindow: Resolved reg-relative LEA to 0x%llx\n", base.U);
                    return base;
                }
            }
        }

        if ((
            instruction->mnemonic == ZYDIS_MNEMONIC_ADD || 
            instruction->mnemonic == ZYDIS_MNEMONIC_SUB ||
            instruction->mnemonic == ZYDIS_MNEMONIC_AND
            ) && src->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        {
            PV_VAL base = PvBackSliceResolveRegWindow(Window, windowIndex, regNorm, IatMap, Depth + 1);

            if (base.Kind == PvValAddr || base.Kind == PvValConst || base.Kind == PvValStackSlot)
            {
                LONG64 imm = (LONG64)src->imm.value.u;

                if (instruction->mnemonic == ZYDIS_MNEMONIC_ADD)
                    base.U = (ULONG64)((LONG64)base.U + imm);
                else if (instruction->mnemonic == ZYDIS_MNEMONIC_SUB)
                    base.U = (ULONG64)((LONG64)base.U - imm);
                else // AND
                    base.U = (ULONG64)((LONG64)base.U & imm);

                dprintf("PvBackSliceResolveRegWindow: Resolved %s through arithmetic/logic to 0x%llx\n", 
                    ZydisRegisterGetString(regNorm), base.U);
                return base;
            }
        }

        dprintf("PvBackSliceResolveRegWindow: Instruction at 0x%llx handled but not fully resolved, stopping scan for this register.\n", Window->InstructionPointers[windowIndex]);
        return PvValUnknownType();
    }

    return PvValUnknownType();
}

BOOLEAN PvStoreImmediateToBuffer(
    _Inout_updates_bytes_(BufferSize) PUCHAR Buffer,
    _In_ ULONG BufferSize,
    _Inout_updates_(BufferSize) PBOOLEAN ValidMask,
    _In_ ULONG Offset,
    _In_ ULONG SizeBytes,
    _In_ ULONG64 Imm
    )
{
    if (Offset + SizeBytes > BufferSize)
        return FALSE;

    memcpy(Buffer + Offset, &Imm, SizeBytes);

    for (ULONG i = 0; i < SizeBytes; i++)
        ValidMask[Offset + i] = TRUE;

    return TRUE;
}

VOID PvTryDecodeMaterializedString(
    _In_reads_bytes_(BufferSize) const UCHAR* Bytes,
    _In_reads_(BufferSize) const BOOLEAN* ValidMask,
    _In_ ULONG BufferSize,
    _Out_writes_(OutChars) PWSTR Out,
    _In_ ULONG OutChars
    )
{
    ULONG i;

    for (i = 0; i < BufferSize && ValidMask[i] && Bytes[i] != 0; i++);

    if (i > 0)
    {
        PhCopyStringZFromUtf8(Bytes, i, Out, OutChars, NULL);
    }
    else
    {
        Out[0] = UNICODE_NULL;
    }
}

BOOLEAN PvMaterializeStackStringRedecode(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG CallOffset,
    _In_ ZydisRegister ArgRegister,
    _In_ LONG64 BaseDispAtCall,
    _Out_writes_(OutChars) PWSTR Out,
    _In_ ULONG OutChars
    )
{
    UCHAR buffer[512];
    BOOLEAN valid[512];
    ULONG scanned = 0;
    ULONG cur = CallOffset;
    BOOLEAN is64 = (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
    LONG64 spDelta = 0;

    RtlZeroMemory(buffer, sizeof(buffer));
    RtlZeroMemory(valid, sizeof(valid));

    while (scanned < PV_BACKSLICE_REDECODE_MAX_INSTRUCTIONS)
    {
        ZydisDecodedInstruction instruction;
        ZydisDecodedOperand ops[ZYDIS_MAX_OPERAND_COUNT];
        ULONG instOffset;
        LONG64 spDeltaPre;
        BOOLEAN isPush, isPop;
        LONG64 spDeltaPost = spDelta;

        if (!PvRedecodePreviousInstruction(
            Decoder,
            SectionBytes,
            SectionSize,
            &cur,
            &instOffset,
            &instruction,
            ops
            ))
        {
            break;
        }

        PvUpdateStackDeltaBackward(
            is64,
            &instruction,
            ops,
            spDeltaPost,
            &spDeltaPre,
            &isPush,
            &isPop
            );
        spDelta = spDeltaPre;
        scanned++;

        if (PvIsBarrierCategory(instruction.meta.category))
            break;

        if (instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
            instruction.operand_count_visible >= 2 &&
            ops[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
            ops[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
        {
            const ZydisDecodedOperand* dst = &ops[0];
            const ZydisDecodedOperand* src = &ops[1];

            if (
                dst->mem.index == ZYDIS_REGISTER_NONE &&
                PvIsSameRegister(dst->mem.base, is64 ? ZYDIS_REGISTER_RSP : ZYDIS_REGISTER_ESP, is64)
                )
            {
                LONG64 storeDispAtCall = (LONG64)dst->mem.disp.value + spDeltaPost;
                LONG64 offsetInString = storeDispAtCall - BaseDispAtCall;

                if (offsetInString >= 0 && offsetInString < sizeof(buffer))
                {
                    PvStoreImmediateToBuffer(buffer, sizeof(buffer), valid, (ULONG)offsetInString, dst->size / 8, src->imm.value.u);
                }
            }
        }
    }

    PvTryDecodeMaterializedString(buffer, valid, sizeof(buffer), Out, OutChars);
    return Out[0] != UNICODE_NULL;
}

PV_VAL PvResolveArg(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG CallOffset,
    _In_ PPV_INSTRUCTION_WINDOW Window,
    _In_ ULONG CurrentWindowIndex,
    _In_ ULONG ArgIndex
    )
{
    BOOLEAN is64 = (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
    PV_VAL result;

    if (is64)
    {
        ZydisRegister reg;
        const char* regName;

        switch (ArgIndex)
        {
        case 0: reg = ZYDIS_REGISTER_RCX; regName = "RCX"; break;
        case 1: reg = ZYDIS_REGISTER_RDX; regName = "RDX"; break;
        case 2: reg = ZYDIS_REGISTER_R8; regName = "R8"; break;
        case 3: reg = ZYDIS_REGISTER_R9; regName = "R9"; break;
        default:
            dprintf("PvResolveArg: ArgIndex %u out of range for x64\n", ArgIndex);
            return PvValUnknownType();
        }

        dprintf("PvResolveArg: x64 ArgIndex %u (%s) CallOffset 0x%x\n", ArgIndex, regName, CallOffset);

        if (Context->DecoderStreamingEnabled)
        {
            result = PvBackSliceResolveRegRedecode(
                Context,
                Decoder,
                SectionBytes,
                SectionSize,
                SectionBaseVa,
                CallOffset,
                reg
                );
        }
        else
        {
            result = PvBackSliceResolveRegWindow(
                Window,
                CurrentWindowIndex,
                reg,
                &Context->IatMap,
                0
                );
        }

        dprintf("PvResolveArg: result Kind %u, U 0x%llx\n", result.Kind, result.U);
        return result;
    }
    else
    {
        // x86 stdcall: Arg 0 is at [ESP], Arg 1 at [ESP+4], etc.
        LONG64 disp = (LONG64)ArgIndex * 4;

        dprintf("PvResolveArg: x86 ArgIndex %u (ESP+0x%llx) CallOffset 0x%x\n", ArgIndex, disp, CallOffset);

        result = PvBackSliceResolveStackSlotStoreRedecode(
            Context,
            Decoder,
            SectionBytes,
            SectionSize,
            SectionBaseVa,
            CallOffset,
            ZYDIS_REGISTER_ESP,
            disp,
            0,
            0
            );

        dprintf("PvResolveArg: result Kind %u, U 0x%llx\n", result.Kind, result.U);
        return result;
    }
}

PV_VAL PvResolveRegForCall(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ ZydisDecoder* Decoder,
    _In_reads_bytes_(SectionSize) PUCHAR SectionBytes,
    _In_ ULONG SectionSize,
    _In_ ULONG64 SectionBaseVa,
    _In_ ULONG CallOffset,
    _In_ PPV_INSTRUCTION_WINDOW Window,
    _In_ ULONG CurrentWindowIndex,
    _In_ ZydisRegister Register
    )
{
    if (Context->DecoderStreamingEnabled)
    {
        return PvBackSliceResolveRegRedecode(
            Context,
            Decoder,
            SectionBytes,
            SectionSize,
            SectionBaseVa,
            CallOffset,
            Register
            );
    }

    return PvBackSliceResolveRegWindow(
        Window,
        CurrentWindowIndex,
        Register,
        &Context->IatMap,
        0
        );
}

VOID PvAppendListView(
    _In_ PPV_APISCAN_PAGE_CONTEXT ctx
    )
{
    ULONG startIndex;
    ULONG endIndex;

    if (!ctx->ListViewHandle)
        return;

    PhAcquireQueuedLockExclusive(&ctx->RowsLock);
    startIndex = ctx->FlushedCount;
    endIndex = ctx->Rows.Count;
    ctx->FlushedCount = endIndex;

    if (startIndex >= endIndex)
    {
        PhReleaseQueuedLockExclusive(&ctx->RowsLock);
        return;
    }

    ExtendedListView_SetRedraw(ctx->ListViewHandle, FALSE);

    for (ULONG i = startIndex; i < endIndex; i++)
    {
        PV_API_SCAN_ROW row = ctx->Rows.Items[i];

        WCHAR bufferVa[64];
        WCHAR bufferSlot[64];
        INT index;

        _snwprintf_s(bufferVa, ARRAYSIZE(bufferVa), _TRUNCATE, L"0x%I64X", row.CallVa);
        index = PhAddListViewItem(ctx->ListViewHandle, MAXINT, bufferVa, NULL);
        PhSetListViewSubItem(ctx->ListViewHandle, index, 1, row.Target);
        PhSetListViewSubItem(ctx->ListViewHandle, index, 2, row.Argument);
        _snwprintf_s(bufferSlot, ARRAYSIZE(bufferSlot), _TRUNCATE, L"0x%I64X", row.IatSlotVa);
        PhSetListViewSubItem(ctx->ListViewHandle, index, 3, bufferSlot);
    }

    PhReleaseQueuedLockExclusive(&ctx->RowsLock);
    ExtendedListView_SetRedraw(ctx->ListViewHandle, TRUE);
}

VOID PvScanImageForPage(
    _Inout_ PPV_APISCAN_PAGE_CONTEXT Context
    )
{
    ZydisDecoder decoder;
    BOOLEAN is64Machine;
    ULONG64 imageBase;
    PIMAGE_SECTION_HEADER sections;
    ULONG sectionCount;
    PV_INSTRUCTION_WINDOW window;
    ULONG instructionsProcessed = 0;

    PvBuildIatMapForPage(Context);

    if (Context->IatMap.Count == 0)
        return;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        is64Machine = TRUE;
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    }
    else
    {
        is64Machine = FALSE;
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);
    }

    imageBase = PvGetImageBase();
    sections = PvMappedImage.Sections;
    sectionCount = PvMappedImage.NumberOfSections;

    dprintf("PvScanImageForPage: Scanning %u sections\n", sectionCount);

    for (ULONG si = 0; si < sectionCount; si++)
    {
        PIMAGE_SECTION_HEADER section = &sections[si];
        ULONG rva;
        ULONG sectionSize;
        PVOID code;
        ULONG64 ip;
        ULONG offset;

        if (!FlagOn(section->Characteristics, IMAGE_SCN_MEM_EXECUTE))
        {
            continue;
        }

        rva = section->VirtualAddress;
        sectionSize = section->Misc.VirtualSize ? section->Misc.VirtualSize : section->SizeOfRawData;
        code = PhMappedImageRvaToVa(&PvMappedImage, rva, NULL);

        if (!code || sectionSize < 8)
        {
            continue;
        }

        dprintf("PvScanImageForPage: Scanning section %u (RVA 0x%x, Size 0x%x)\n", si, rva, sectionSize);

        if (!Context->DecoderStreamingEnabled)
        {
            RtlZeroMemory(&window, sizeof(window));
        }

        ip = imageBase + (ULONG64)rva;
        offset = 0;

        while (offset < sectionSize)
        {
            ZydisDecodedInstruction instruction;
            ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
            ULONG currentWindowIndex;

            if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(
                &decoder,
                PTR_ADD_OFFSET(code, offset),
                sectionSize - offset,
                &instruction,
                operands
                )))
            {
                offset++;
                ip++;
                continue;
            }

            if (instruction.length == 0)
            {
                offset++;
                ip++;
                continue;
            }

            if (!Context->DecoderStreamingEnabled)
            {
                currentWindowIndex = window.Head;
                PvAddInstructionToWindow(&window, &instruction, operands, ip);
            }
            else
            {
                currentWindowIndex = 0;
            }

            if (
                instruction.meta.category == ZYDIS_CATEGORY_CALL &&
                instruction.operand_count_visible >= 1
                )
            {
                PCWSTR targetName = NULL;
                ULONG64 slotVa = 0;

                // dprintf("PvScanImageForPage: Encountered CALL at 0x%llx\n", ip);

                if (
                    operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                    ((is64Machine && operands[0].mem.base == ZYDIS_REGISTER_RIP) || (operands[0].mem.base == ZYDIS_REGISTER_NONE))
                    )
                {
                    ULONG64 memoryVa = is64Machine ?
                        PvRipTarget(ip, instruction.length, operands[0].mem.disp.value) :
                        (ULONG64)operands[0].mem.disp.value;

                    targetName = PvFindIatName(&Context->IatMap, memoryVa);
                    slotVa = memoryVa;

                    if (targetName)
                    {
                        dprintf("PvScanImageForPage: Found direct IAT call: %S at 0x%llx\n", targetName, ip);
                    }
                }
                else if (operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
                {
                    ZydisRegister reg = operands[0].reg.value;
                    PV_VAL targetVal = PvResolveRegForCall(
                        Context,
                        &decoder,
                        (PUCHAR)code,
                        sectionSize,
                        imageBase + (ULONG64)rva,
                        offset,
                        &window,
                        currentWindowIndex,
                        reg
                        );

                    if (targetVal.Kind == PvValIatSlot)
                    {
                        targetName = PvFindIatName(&Context->IatMap, targetVal.U);
                        slotVa = targetVal.U;

                        if (targetName)
                        {
                            dprintf("PvScanImageForPage: Found register IAT call: %S at 0x%llx\n", targetName, ip);
                        }
                    }
                }
                else if (operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                {
                    // Follow jump thunk
                    ULONG64 thunkVa = is64Machine ?
                        PvRipTarget(ip, instruction.length, operands[0].imm.value.s) :
                        (ULONG64)operands[0].imm.value.u;

                    ULONG thunkRva = (ULONG)(thunkVa - imageBase);
                    PVOID thunkCode = PhMappedImageRvaToVa(&PvMappedImage, thunkRva, NULL);

                    if (thunkCode)
                    {
                        ZydisDecodedInstruction thunkInstr;
                        ZydisDecodedOperand thunkOps[ZYDIS_MAX_OPERAND_COUNT];

                        if (ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, thunkCode, 15, &thunkInstr, thunkOps)))
                        {
                            if (thunkInstr.mnemonic == ZYDIS_MNEMONIC_JMP && thunkOps[0].type == ZYDIS_OPERAND_TYPE_MEMORY)
                            {
                                ULONG64 iatVa = is64Machine ?
                                    PvRipTarget(thunkVa, thunkInstr.length, thunkOps[0].mem.disp.value) :
                                    (ULONG64)thunkOps[0].mem.disp.value;

                                targetName = PvFindIatName(&Context->IatMap, iatVa);
                                slotVa = iatVa;

                                if (targetName)
                                {
                                    dprintf("PvScanImageForPage: Found thunk IAT call: %S at 0x%llx\n", targetName, ip);
                                }
                            }
                        }
                    }
                }

                if (targetName)
                {
                    WCHAR argument[512];

                    RtlZeroMemory(argument, sizeof(argument));

                    dprintf("PvScanImageForPage: Resolving argument for %S at 0x%llx\n", targetName, ip);

                    if (Context->IsGetProcAddressPage)
                    {
                        PV_VAL value = PvResolveArg(
                            Context,
                            &decoder,
                            (PUCHAR)code,
                            sectionSize,
                            imageBase + (ULONG64)rva,
                            offset,
                            &window,
                            currentWindowIndex,
                            1
                            );

                        if (value.Kind == PvValConst && value.U <= 0xFFFF)
                        {
                            _snwprintf_s(
                                argument,
                                RTL_NUMBER_OF(argument),
                                _TRUNCATE,
                                L"<ordinal:%llu>",
                                value.U
                                );
                        }
                        else if (value.Kind == PvValStackSlot)
                        {
                            if (!PvMaterializeStackStringRedecode(
                                Context,
                                &decoder,
                                (PUCHAR)code,
                                sectionSize,
                                imageBase + (ULONG64)rva,
                                offset,
                                ZYDIS_REGISTER_RDX,
                                (LONG64)value.U,
                                argument,
                                RTL_NUMBER_OF(argument)
                                ))
                            {
                                wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                            }
                        }
                        else if (value.Kind == PvValImport)
                        {
                            PCWSTR importName = PvFindIatName(&Context->IatMap, value.U);

                            if (importName)
                                _snwprintf_s(argument, RTL_NUMBER_OF(argument), _TRUNCATE, L"<result of %s>", importName);
                            else
                                _snwprintf_s(argument, RTL_NUMBER_OF(argument), _TRUNCATE, L"<result of 0x%llx>", value.U);
                        }
                        else if ((value.Kind == PvValAddr || value.Kind == PvValConst) && value.U)
                        {
                            if (PhEqualStringZ(targetName, L"LdrGetProcedureAddress", TRUE))
                            {
                                if (!PvReadAnsiStringStructAtVa(value.U, argument, RTL_NUMBER_OF(argument)))
                                {
                                    wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                                }
                            }
                            else
                            {
                                PvReadStringAtVa(value.U, argument, RTL_NUMBER_OF(argument));

                                if (argument[0] == UNICODE_NULL)
                                {
                                    wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                                }
                            }
                        }
                        else
                        {
                            wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                        }
                    }
                    else
                    {
                        ULONG argIdx = 0;
                        BOOLEAN isUnicodeStruct = FALSE;

                        if (PhEqualStringZ(targetName, L"LdrLoadDll", TRUE))
                        {
                            argIdx = 2;
                            isUnicodeStruct = TRUE;
                        }

                        PV_VAL value = PvResolveArg(
                            Context,
                            &decoder,
                            (PUCHAR)code,
                            sectionSize,
                            imageBase + (ULONG64)rva,
                            offset,
                            &window,
                            currentWindowIndex,
                            argIdx
                            );

                        if (value.Kind == PvValStackSlot)
                        {
                            if (!PvMaterializeStackStringRedecode(
                                Context,
                                &decoder,
                                (PUCHAR)code,
                                sectionSize,
                                imageBase + (ULONG64)rva,
                                offset, ZYDIS_REGISTER_RCX,
                                (LONG64)value.U,
                                argument,
                                RTL_NUMBER_OF(argument)
                                ))
                            {
                                wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                            }
                        }
                        else if (value.Kind == PvValImport)
                        {
                            PCWSTR importName = PvFindIatName(&Context->IatMap, value.U);

                            if (importName)
                                _snwprintf_s(argument, RTL_NUMBER_OF(argument), _TRUNCATE, L"<result of %s>", importName);
                            else
                                _snwprintf_s(argument, RTL_NUMBER_OF(argument), _TRUNCATE, L"<result of 0x%llx>", value.U);
                        }
                        else if ((value.Kind == PvValAddr || value.Kind == PvValConst) && value.U)
                        {
                            if (isUnicodeStruct)
                            {
                                if (!PvReadUnicodeStringStructAtVa(value.U, argument, RTL_NUMBER_OF(argument)))
                                {
                                    wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                                }
                            }
                            else
                            {
                                PvReadStringAtVa(value.U, argument, RTL_NUMBER_OF(argument));

                                if (argument[0] == UNICODE_NULL)
                                {
                                    wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                                }
                            }
                        }
                        else
                        {
                            wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                        }
                    }

                    PhAcquireQueuedLockExclusive(&Context->RowsLock);
                    PvEnsureRowsCapacity(&Context->Rows, 1);
                    {
                        PPV_API_SCAN_ROW row = &Context->Rows.Items[Context->Rows.Count++];
                        row->CallVa = ip;
                        row->IatSlotVa = slotVa;
                        wcsncpy_s(row->Target, RTL_NUMBER_OF(row->Target), targetName, _TRUNCATE);
                        wcsncpy_s(row->Argument, RTL_NUMBER_OF(row->Argument), argument, _TRUNCATE);
                    }
                    PhReleaseQueuedLockExclusive(&Context->RowsLock);

                    if (Context->StreamingEnabled && (Context->Rows.Count == 1 || ((Context->Rows.Count & 0x3F) == 0)))
                    {
                        PostMessage(Context->WindowHandle, WM_PV_APISCAN_APPEND, 0, 0);
                    }
                }
            }

            offset += instruction.length;
            ip += instruction.length;
        }
    }
}

static VOID PvBuildIatMapForPage(
    _Inout_ PPV_APISCAN_PAGE_CONTEXT Context
    )
{
    PH_MAPPED_IMAGE_IMPORTS imports;
    ULONG64 imageBase;

    if (!NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)))
    {
        dprintf("PvBuildIatMapForPage: PhGetMappedImageImports failed\n");
        return;
    }

    imageBase = PvGetImageBase();
    dprintf("PvBuildIatMapForPage: Building IAT map for image with %u DLLs\n", imports.NumberOfDlls);

    for (ULONG i = 0; i < imports.NumberOfDlls; i++)
    {
        PH_MAPPED_IMAGE_IMPORT_DLL importDll;

        if (!NT_SUCCESS(PhGetMappedImageImportDll(&imports, i, &importDll)))
            continue;

        for (ULONG j = 0; j < importDll.NumberOfEntries; j++)
        {
            PH_MAPPED_IMAGE_IMPORT_ENTRY entry;
            ULONG iatRva;
            ULONG64 slotVa;

            if (!NT_SUCCESS(PhGetMappedImageImportEntry(&importDll, j, &entry)))
                continue;

            if (!entry.Name)
                continue;

            if (!PvIsTargetForPage(Context->IsGetProcAddressPage, importDll.Name, entry.Name))
                continue;

            iatRva = PhGetMappedImageImportEntryRva(&importDll, j, FALSE);
            slotVa = imageBase + (ULONG64)iatRva;

            PvEnsureIatCapacity(&Context->IatMap, 1);

            Context->IatMap.Items[Context->IatMap.Count].SlotVa = slotVa;

            NTSTATUS status;
            WCHAR dllName[0x100];
            WCHAR functionName[0x100];

            dllName[0] = UNICODE_NULL;
            functionName[0] = UNICODE_NULL;

            status = PhCopyStringZFromUtf8(
                importDll.Name,
                SIZE_MAX,
                dllName,
                RTL_NUMBER_OF(dllName),
                NULL
                );

            if (!NT_SUCCESS(status))
            {
                wcscpy_s(dllName, RTL_NUMBER_OF(dllName), L"<invalid>");
            }

            status = PhCopyStringZFromUtf8(
                entry.Name,
                SIZE_MAX,
                functionName,
                RTL_NUMBER_OF(functionName),
                NULL
                );

            if (!NT_SUCCESS(status))
            {
                wcscpy_s(functionName, RTL_NUMBER_OF(functionName), L"<invalid>");
            }

            _snwprintf_s(
                Context->IatMap.Items[Context->IatMap.Count].Name,
                RTL_NUMBER_OF(Context->IatMap.Items[Context->IatMap.Count].Name),
                _TRUNCATE,
                L"%s!%s",
                dllName,
                functionName
                );

            // dprintf("PvBuildIatMapForPage: Mapped %S to 0x%llx\n", Context->IatMap.Items[Context->IatMap.Count].Name, slotVa);
            Context->IatMap.Count++;
        }
    }

    dprintf("PvBuildIatMapForPage: Map built with %u entries\n", Context->IatMap.Count);
}

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS NTAPI PvApiScanWorkerThread(
    _In_ PVOID Parameter
    )
{
    PPV_APISCAN_PAGE_CONTEXT context = (PPV_APISCAN_PAGE_CONTEXT)Parameter;
    PvScanImageForPage(context);
    PostMessage(context->WindowHandle, WM_PV_APISCAN_DONE, 0, 0);
    return STATUS_SUCCESS;
}

VOID PvPopulateListView(
    _In_ PPV_APISCAN_PAGE_CONTEXT ctx
    )
{
    ctx->FlushedCount = 0;
    ExtendedListView_SetRedraw(ctx->ListViewHandle, FALSE);
    ListView_DeleteAllItems(ctx->ListViewHandle);

    for (ULONG i = 0; i < ctx->Rows.Count; i++)
    {
        WCHAR bufferVa[64];
        _snwprintf_s(bufferVa, ARRAYSIZE(bufferVa), _TRUNCATE, L"0x%I64X", ctx->Rows.Items[i].CallVa);

        INT index = PhAddListViewItem(ctx->ListViewHandle, MAXINT, bufferVa, NULL);
        PhSetListViewSubItem(ctx->ListViewHandle, index, 1, ctx->Rows.Items[i].Target);
        PhSetListViewSubItem(ctx->ListViewHandle, index, 2, ctx->Rows.Items[i].Argument);

        WCHAR bufferSlot[64];
        _snwprintf_s(bufferSlot, ARRAYSIZE(bufferSlot), _TRUNCATE, L"0x%I64X", ctx->Rows.Items[i].IatSlotVa);
        PhSetListViewSubItem(ctx->ListViewHandle, index, 3, bufferSlot);
    }

    ExtendedListView_SetRedraw(ctx->ListViewHandle, TRUE);
}

VOID PvInitCommonListView(
    _In_ PPV_APISCAN_PAGE_CONTEXT Context,
    _In_ HWND WindowHandle
    )
{
    Context->WindowHandle = WindowHandle;
    Context->ListViewHandle = GetDlgItem(WindowHandle, IDC_LIST);

    PhSetListViewStyle(Context->ListViewHandle, TRUE, TRUE);
    PhSetControlTheme(Context->ListViewHandle, L"explorer");
    PhSetExtendedListView(Context->ListViewHandle);
    PvConfigTreeBorders(Context->ListViewHandle);

    PhAddListViewColumn(Context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Call VA");
    PhAddListViewColumn(Context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 240, L"Target");
    PhAddListViewColumn(Context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 280, L"String/Ordinal");
    PhAddListViewColumn(Context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 120, L"IAT Slot");

    PhInitializeLayoutManager(&Context->LayoutManager, WindowHandle);
    PhAddLayoutItem(&Context->LayoutManager, Context->ListViewHandle, NULL, PH_ANCHOR_ALL);

    Context->StreamingEnabled = PV_APISCAN_STREAMING_DEFAULT ? TRUE : FALSE;
    Context->DecoderStreamingEnabled = PV_APISCAN_DECODER_STREAMING_DEFAULT ? TRUE : FALSE;

    if (Context->StreamingEnabled)
    {
        Context->ScanThreadHandle = PhCreateThread(0, PvApiScanWorkerThread, Context);
    }
    else
    {
        PvScanImageForPage(Context);
        PvPopulateListView(Context);
    }

    PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);
}

INT_PTR CALLBACK PvGetProcAddressDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_APISCAN_PAGE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_APISCAN_PAGE_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            const LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->IsGetProcAddressPage = TRUE;
            context->IatMap.Items = NULL;
            context->IatMap.Count = 0;
            context->IatMap.Capacity = 0;
            context->Rows.Items = NULL;
            context->Rows.Count = 0;
            context->Rows.Capacity = 0;
            PvInitCommonListView(context, hwndDlg);
        }
        return TRUE;

    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);
                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_PV_APISCAN_APPEND:
        {
            PvAppendListView(context);
        }
        break;
    case WM_PV_APISCAN_DONE:
        {
            PvAppendListView(context);
        }
        break;
    case WM_DESTROY:
        {
            if (context->ScanThreadHandle)
            {
                PhWaitForSingleObject(context->ScanThreadHandle, INFINITE);
                NtClose(context->ScanThreadHandle);
                context->ScanThreadHandle = NULL;
            }

            PhDeleteLayoutManager(&context->LayoutManager);
            if (context->IatMap.Items) PhFree(context->IatMap.Items);
            if (context->Rows.Items) PhFree(context->Rows.Items);
            PhFree(context);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PvGetLoadLibraryDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_APISCAN_PAGE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_APISCAN_PAGE_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            const LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->IsGetProcAddressPage = FALSE;
            context->IatMap.Items = NULL;
            context->IatMap.Count = 0;
            context->IatMap.Capacity = 0;
            context->Rows.Items = NULL;
            context->Rows.Count = 0;
            context->Rows.Capacity = 0;
            PvInitCommonListView(context, hwndDlg);
        }
        return TRUE;

    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);
                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_PV_APISCAN_APPEND:
        {
            PvAppendListView(context);
        }
        break;
    case WM_PV_APISCAN_DONE:
        {
            PvAppendListView(context);
        }
        break;
    case WM_DESTROY:
        {
            if (context->ScanThreadHandle)
            {
                PhWaitForSingleObject(context->ScanThreadHandle, INFINITE);
                NtClose(context->ScanThreadHandle);
                context->ScanThreadHandle = NULL;
            }

            PhDeleteLayoutManager(&context->LayoutManager);
            if (context->IatMap.Items) PhFree(context->IatMap.Items);
            if (context->Rows.Items) PhFree(context->Rows.Items);
            PhFree(context);
        }
        break;
    }

    return FALSE;
}
