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
} PV_APISCAN_PAGE_CONTEXT, *PPV_APISCAN_PAGE_CONTEXT;

// Sliding window (keep last N instructions)
#define PV_BACKSLICE_WINDOW_SIZE 400

typedef struct _PV_INSTRUCTION_WINDOW
{
    ZydisDecodedInstruction Instructions[PV_BACKSLICE_WINDOW_SIZE];
    ZydisDecodedOperand Operands[PV_BACKSLICE_WINDOW_SIZE][ZYDIS_MAX_OPERAND_COUNT];
    ULONG64 InstructionPointers[PV_BACKSLICE_WINDOW_SIZE];
    ULONG Count;
    ULONG Head;
} PV_INSTRUCTION_WINDOW, *PPV_INSTRUCTION_WINDOW;

static VOID PvEnsureRowsCapacity(
    _Inout_ PPV_API_SCAN_ROWS Rows,
    _In_ ULONG Add
    )
{
    ULONG newCapacity;

    if (Rows->Count + Add <= Rows->Capacity)
        return;

    newCapacity = Rows->Capacity ? Rows->Capacity * 2 : 64;

    while (newCapacity < Rows->Count + Add)
        newCapacity *= 2;

    Rows->Items = PhReAllocate(Rows->Items, sizeof(PV_API_SCAN_ROW) * newCapacity);
    Rows->Capacity = newCapacity;
}

static VOID PvEnsureIatCapacity(
    _Inout_ PPV_IAT_MAP Map,
    _In_ ULONG Add
    )
{
    ULONG newCapacity;

    if (Map->Count + Add <= Map->Capacity)
        return;

    newCapacity = Map->Capacity ? Map->Capacity * 2 : 64;

    while (newCapacity < Map->Count + Add)
        newCapacity *= 2;

    Map->Items = PhReAllocate(Map->Items, sizeof(PV_IAT_MAP_ENTRY) * newCapacity);
    Map->Capacity = newCapacity;
}

PCWSTR PvFindIatName(
    _In_ PPV_IAT_MAP Map,
    _In_ ULONG64 SlotVa
    )
{
    for (ULONG i = 0; i < Map->Count; i++)
    {
        if (Map->Items[i].SlotVa == SlotVa)
            return Map->Items[i].Name;
    }

    return NULL;
}

ULONG64 PvGetImageBase(
    VOID
    )
{
    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return (ULONG64)PvMappedImage.NtHeaders->OptionalHeader.ImageBase;
    else
        return (ULONG64)PvMappedImage.NtHeaders32->OptionalHeader.ImageBase;
}

BOOLEAN PvReadAsciiStringAtVa(
    _In_ ULONG64 Va,
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ ULONG BufferCount
    )
{
    ULONG64 imageBase;
    ULONG rva;
    PCSTR string;
    ULONG length;

    imageBase = PvGetImageBase();

    if (Va < imageBase)
        return FALSE;

    rva = (ULONG)(Va - imageBase);
    string = PhMappedImageRvaToVa(&PvMappedImage, rva, NULL);

    if (!string)
        return FALSE;

    length = 0;

    while (length + 1 < BufferCount)
    {
        CHAR c = string[length];

        if (IMAGE_SNAP_BY_ORDINAL64(c))
        {
            if (!isprint(c))
                return FALSE;
        }

        if (c == ANSI_NULL)
            break;
        if (!isprint(c))
            return FALSE;

        Buffer[length] = (WCHAR)(UCHAR)c;
        length++;
    }

    Buffer[length] = UNICODE_NULL;

    return (length >= 3);
}

BOOLEAN PvReadUtf16StringAtVa(
    _In_ ULONG64 Va,
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ ULONG BufferCount
    )
{
    ULONG64 imageBase;
    ULONG rva;
    PCWSTR string;
    ULONG length;

    imageBase = PvGetImageBase();

    if (Va < imageBase)
        return FALSE;

    rva = (ULONG)(Va - imageBase);
    string = PhMappedImageRvaToVa(&PvMappedImage, rva, NULL);

    if (!string)
        return FALSE;

    length = 0;

    while (length + 1 < BufferCount)
    {
        WCHAR c = string[length];

        if (IMAGE_SNAP_BY_ORDINAL64(c))
        {
            if (!iswprint(c))
                return FALSE;
        }

        if (c == UNICODE_NULL)
            break;
        if (!iswprint(c))
            return FALSE;

        Buffer[length] = c;
        length++;
    }

    Buffer[length] = UNICODE_NULL;

    return (length >= 3);
}

VOID PvReadStringAtVa(
    _In_ ULONG64 Va,
    _Out_writes_(BufferCount) PWSTR Buffer,
    _In_ ULONG BufferCount
    )
{
    Buffer[0] = UNICODE_NULL;

    if (PvReadAsciiStringAtVa(Va, Buffer, BufferCount))
        return;

    PvReadUtf16StringAtVa(Va, Buffer, BufferCount);
}

BOOLEAN PvIsTargetForPage(
    _In_ BOOLEAN IsGetProcAddressPage,
    _In_ PCSTR DllName,
    _In_ PCSTR FunctionName
    )
{
    if (IsGetProcAddressPage)
    {
        if (
            PhEqualBytesZ(DllName, "kernel32.dll", TRUE) || 
            PhEqualBytesZ(DllName, "kernelbase.dll", TRUE)
            )
        {
            if (PhEqualBytesZ(FunctionName, "GetProcAddress", TRUE))
            {
                return TRUE;
            }
        }

        if (PhEqualBytesZ(DllName, "ntdll.dll", TRUE))
        {
            if (
                PhEqualBytesZ(FunctionName, "LdrGetProcedureAddress", TRUE) ||
                PhEqualBytesZ(FunctionName, "LdrGetProcedureAddressEx", TRUE) ||
                PhEqualBytesZ(FunctionName, "LdrGetProcedureAddressForCaller", TRUE)
                )
            {
                return TRUE;
            }
        }

        return FALSE;
    }
    else
    {
        if (
            PhEqualBytesZ(DllName, "kernel32.dll", TRUE) || 
            PhEqualBytesZ(DllName, "kernelbase.dll", TRUE)
            )
        {
            if (PhEqualBytesZ(FunctionName, "LoadLibraryA", TRUE) ||
                PhEqualBytesZ(FunctionName, "LoadLibraryW", TRUE) ||
                PhEqualBytesZ(FunctionName, "LoadLibraryExA", TRUE) ||
                PhEqualBytesZ(FunctionName, "LoadLibraryExW", TRUE))
            {
                return TRUE;
            }
        }

        if (PhEqualBytesZ(DllName, "ntdll.dll", TRUE))
        {
            if (PhEqualBytesZ(FunctionName, "LdrLoadDll", TRUE))
            {
                return TRUE;
            }
        }

        return FALSE;
    }
}

VOID PvBuildIatMapForPage(
    _Inout_ PPV_APISCAN_PAGE_CONTEXT Context
    )
{
    PH_MAPPED_IMAGE_IMPORTS imports;
    ULONG64 imageBase;

    if (!NT_SUCCESS(PhGetMappedImageImports(&imports, &PvMappedImage)))
        return;

    imageBase = PvGetImageBase();

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

            //PPH_STRING importDllString = PhZeroExtendToUtf16(importDll.Name);
            NTSTATUS status;
            WCHAR dllName[0x100];
            WCHAR functionName[0x100];

            status = PhCopyStringZFromUtf8(
                importDll.Name,
                SIZE_MAX,
                dllName,
                RTL_NUMBER_OF(dllName),
                NULL
                );

            status = PhCopyStringZFromUtf8(
                entry.Name,
                SIZE_MAX,
                functionName,
                RTL_NUMBER_OF(functionName),
                NULL
                );

            _snwprintf_s(
                Context->IatMap.Items[Context->IatMap.Count].Name,
                RTL_NUMBER_OF(Context->IatMap.Items[Context->IatMap.Count].Name),
                _TRUNCATE,
                L"%s!%s",
                dllName,
                functionName
                );

            Context->IatMap.Count++;
        }
    }
}

// Resolve a register to constant/addr 
// mov reg, imm / lea reg, [rip+disp] / xor reg, reg / mov reg, [rip+disp]

typedef enum _PV_VAL_KIND
{
    PvValUnknown,
    PvValConst,
    PvValAddr,
    PvValIatSlot
} PV_VAL_KIND;

typedef struct _PV_VAL
{
    PV_VAL_KIND Kind;
    ULONG64 U;
} PV_VAL, *PPV_VAL;

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

BOOLEAN PvIsBarrierCategory(
    _In_ ZydisInstructionCategory Category
    )
{
    return Category == ZYDIS_CATEGORY_CALL ||
           Category == ZYDIS_CATEGORY_RET ||
           Category == ZYDIS_CATEGORY_COND_BR ||
           Category == ZYDIS_CATEGORY_UNCOND_BR ||
           Category == ZYDIS_CATEGORY_SYSTEM;
}

ULONG64 PvRipTarget(
    _In_ ULONG64 InstructionPointer,
    _In_ UINT8 Length,
    _In_ INT64 Displacement
    )
{
    return InstructionPointer + (ULONG64)Length + (ULONG64)Displacement;
}

PV_VAL PvBackSliceResolveReg(
    _In_reads_(Count) const ZydisDecodedInstruction* Instructions,
    _In_reads_(Count * ZYDIS_MAX_OPERAND_COUNT) const ZydisDecodedOperand* Operands,
    _In_reads_(Count) const ULONG64* InstructionPointers,
    _In_ ULONG Count,
    _In_ ULONG CallIndex,
    _In_ ZydisRegister Register,
    _In_ PPV_IAT_MAP IatMap
    )
{
    INT scanned = 0;

    for (INT i = (INT)CallIndex - 1; i >= 0 && scanned < 160; i--, scanned++)
    {
        const ZydisDecodedInstruction* instruction = &Instructions[i];
        const ZydisDecodedOperand* operands = &Operands[i * ZYDIS_MAX_OPERAND_COUNT];

        if (PvIsBarrierCategory(instruction->meta.category))
            break;

        if (instruction->operand_count_visible < 2)
            continue;

        const ZydisDecodedOperand* dst = &operands[0];
        const ZydisDecodedOperand* src = &operands[1];

        if (dst->type != ZYDIS_OPERAND_TYPE_REGISTER || dst->reg.value != Register)
            continue;

        // xor reg, reg -> 0
        if (instruction->mnemonic == ZYDIS_MNEMONIC_XOR &&
            src->type == ZYDIS_OPERAND_TYPE_REGISTER &&
            src->reg.value == Register)
        {
            return PvValConstType(0);
        }

        if (instruction->mnemonic == ZYDIS_MNEMONIC_MOV)
        {
            if (src->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                return PvValConstType((ULONG64)src->imm.value.u);

            if (src->type == ZYDIS_OPERAND_TYPE_MEMORY && src->mem.base == ZYDIS_REGISTER_RIP)
            {
                ULONG64 memoryVa = PvRipTarget(InstructionPointers[i], instruction->length, src->mem.disp.value);

                if (PvFindIatName(IatMap, memoryVa))
                    return PvValIatType(memoryVa);

                return PvValAddrType(memoryVa);
            }
        }

        if (instruction->mnemonic == ZYDIS_MNEMONIC_LEA &&
            src->type == ZYDIS_OPERAND_TYPE_MEMORY &&
            src->mem.base == ZYDIS_REGISTER_RIP)
        {
            ULONG64 address = PvRipTarget(InstructionPointers[i], instruction->length, src->mem.disp.value);

            if (PvFindIatName(IatMap, address))
                return PvValIatType(address);

            return PvValAddrType(address);
        }

        return PvValUnknownType();
    }

    return PvValUnknownType();
}

VOID PvAddInstructionToWindow(
    _Inout_ PPV_INSTRUCTION_WINDOW Window,
    _In_ const ZydisDecodedInstruction* Instruction,
    _In_reads_(ZYDIS_MAX_OPERAND_COUNT) const ZydisDecodedOperand* Operands,
    _In_ ULONG64 InstructionPointer
    )
{
    ULONG index = Window->Head;

    Window->Instructions[index] = *Instruction;
    RtlCopyMemory(Window->Operands[index], Operands, sizeof(ZydisDecodedOperand) * ZYDIS_MAX_OPERAND_COUNT);
    Window->InstructionPointers[index] = InstructionPointer;

    Window->Head = (Window->Head + 1) % PV_BACKSLICE_WINDOW_SIZE;
    
    if (Window->Count < PV_BACKSLICE_WINDOW_SIZE)
        Window->Count++;
}

// Stack-slot tracking for resolving common compiler patterns
// mov rdx, [rsp + disp] / mov rdx, [rbp + disp]
// mov [rsp+disp], reg (recursively resolved)
// mov [rsp+disp], [rip+disp]
PV_VAL PvBackSliceResolveRegWindow(
    _In_ PPV_INSTRUCTION_WINDOW Window,
    _In_ ULONG CurrentIndex,
    _In_ ZydisRegister Register,
    _In_ PPV_IAT_MAP IatMap
    )
{
    ULONG maxScan = min(Window->Count - 1, 160); // -1 to exclude current instruction

    // Search backwards from instruction before current
    for (ULONG i = 1; i <= maxScan; i++)
    {
        ULONG windowIndex = (CurrentIndex + PV_BACKSLICE_WINDOW_SIZE - i) % PV_BACKSLICE_WINDOW_SIZE;

        // Early termination if we've exhausted the window
        if (i > Window->Count)
            break;

        const ZydisDecodedInstruction* instruction = &Window->Instructions[windowIndex];
        const ZydisDecodedOperand* ops = Window->Operands[windowIndex];

        if (PvIsBarrierCategory(instruction->meta.category))
            break;

        if (instruction->operand_count_visible < 2)
            continue;

        const ZydisDecodedOperand* dst = &ops[0];
        const ZydisDecodedOperand* src = &ops[1];

        if (dst->type != ZYDIS_OPERAND_TYPE_REGISTER || dst->reg.value != Register)
            continue;

        // xor reg, reg -> 0
        if (
            instruction->mnemonic == ZYDIS_MNEMONIC_XOR &&
            src->type == ZYDIS_OPERAND_TYPE_REGISTER &&
            src->reg.value == Register
            )
        {
            return PvValConstType(0);
        }

        if (instruction->mnemonic == ZYDIS_MNEMONIC_MOV)
        {
            if (src->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            {
                return PvValConstType((ULONG64)src->imm.value.u);
            }

            if (src->type == ZYDIS_OPERAND_TYPE_REGISTER)
            {
                // Recursively resolve the source register (mov reg, reg)
                return PvBackSliceResolveRegWindow(Window, windowIndex, src->reg.value, IatMap);
            }

            // mov reg, [rip+disp]
            if (src->type == ZYDIS_OPERAND_TYPE_MEMORY && src->mem.base == ZYDIS_REGISTER_RIP)
            {
                ULONG64 memoryVa = PvRipTarget(
                    Window->InstructionPointers[windowIndex],
                    instruction->length,
                    src->mem.disp.value
                    );

                if (PvFindIatName(IatMap, memoryVa))
                {
                    return PvValIatType(memoryVa);
                }

                return PvValAddrType(memoryVa);
            }

            // mov reg, [rsp+disp] / [rbp+disp]
            // Common compiler pattern for argument setup/spills.
            if (
                src->type == ZYDIS_OPERAND_TYPE_MEMORY &&
                (src->mem.base == ZYDIS_REGISTER_RSP || src->mem.base == ZYDIS_REGISTER_RBP)
                )
            {
                // Only handle simple base+disp, no index/scale.
                if (src->mem.index != ZYDIS_REGISTER_NONE)
                    return PvValUnknownType();

                // Find the last store to the same stack slot.
                for (ULONG j = i + 1; j <= maxScan; j++)
                {
                    ULONG prevIndex = (CurrentIndex + PV_BACKSLICE_WINDOW_SIZE - j) % PV_BACKSLICE_WINDOW_SIZE;

                    if (j > Window->Count)
                        break;

                    const ZydisDecodedInstruction* prevInstruction = &Window->Instructions[prevIndex];
                    const ZydisDecodedOperand* prevOps = Window->Operands[prevIndex];

                    if (PvIsBarrierCategory(prevInstruction->meta.category))
                        break;

                    if (prevInstruction->operand_count_visible < 2)
                        continue;

                    const ZydisDecodedOperand* prevDst = &prevOps[0];
                    const ZydisDecodedOperand* prevSrc = &prevOps[1];

                    if (
                        prevInstruction->mnemonic != ZYDIS_MNEMONIC_MOV ||
                        prevDst->type != ZYDIS_OPERAND_TYPE_MEMORY ||
                        prevDst->mem.base != src->mem.base ||
                        prevDst->mem.index != ZYDIS_REGISTER_NONE ||
                        prevDst->mem.disp.value != src->mem.disp.value
                        )
                    {
                        continue;
                    }

                    if (prevSrc->type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
                        return PvValConstType((ULONG64)prevSrc->imm.value.u);

                    if (prevSrc->type == ZYDIS_OPERAND_TYPE_REGISTER)
                        return PvBackSliceResolveRegWindow(Window, prevIndex, prevSrc->reg.value, IatMap);

                    if (prevSrc->type == ZYDIS_OPERAND_TYPE_MEMORY && prevSrc->mem.base == ZYDIS_REGISTER_RIP)
                    {
                        ULONG64 memoryVa = PvRipTarget(
                            Window->InstructionPointers[prevIndex],
                            prevInstruction->length,
                            prevSrc->mem.disp.value
                            );

                        if (PvFindIatName(IatMap, memoryVa))
                            return PvValIatType(memoryVa);

                        return PvValAddrType(memoryVa);
                    }

                    return PvValUnknownType();
                }

                return PvValUnknownType();
            }
        }

        if (
            instruction->mnemonic == ZYDIS_MNEMONIC_LEA &&
            src->type == ZYDIS_OPERAND_TYPE_MEMORY &&
            src->mem.base == ZYDIS_REGISTER_RIP
            )
        {
            ULONG64 address = PvRipTarget(
                Window->InstructionPointers[windowIndex],
                instruction->length,
                src->mem.disp.value
                );

            if (PvFindIatName(IatMap, address))
            {
                return PvValIatType(address);
            }

            return PvValAddrType(address);
        }

        return PvValUnknownType();
    }

    return PvValUnknownType();
}

VOID PvScanImageForPage(
    _Inout_ PPV_APISCAN_PAGE_CONTEXT Context
    )
{
    ZydisDecoder decoder;
    ULONG64 imageBase;
    PIMAGE_SECTION_HEADER sections;
    ULONG sectionCount;
    PV_INSTRUCTION_WINDOW window;

    PvBuildIatMapForPage(Context);

    if (Context->IatMap.Count == 0)
        return;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);
    else
        ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

    imageBase = PvGetImageBase();
    sections = PvMappedImage.Sections;
    sectionCount = PvMappedImage.NumberOfSections;

    for (ULONG si = 0; si < sectionCount; si++)
    {
        PIMAGE_SECTION_HEADER section = &sections[si];
        ULONG rva;
        ULONG sectionSize;
        PVOID code;
        ULONG64 ip;
        ULONG offset;
        ULONG64 registerLastIat[ZYDIS_REGISTER_MAX_VALUE + 1];
        INT registerTtl[ZYDIS_REGISTER_MAX_VALUE + 1];

        if (!FlagOn(section->Characteristics, IMAGE_SCN_MEM_EXECUTE))
            continue;

        rva = section->VirtualAddress;
        sectionSize = section->Misc.VirtualSize ? section->Misc.VirtualSize : section->SizeOfRawData;
        code = PhMappedImageRvaToVa(&PvMappedImage, rva, NULL);

        if (!code || sectionSize < 8)
            continue;

        // Reset window for each section
        RtlZeroMemory(&window, sizeof(window));
        RtlZeroMemory(registerLastIat, sizeof(registerLastIat));
        RtlZeroMemory(registerTtl, sizeof(registerTtl));

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

            // Add to sliding window
            currentWindowIndex = window.Head;
            PvAddInstructionToWindow(&window, &instruction, operands, ip);

            // Decay TTLs
            for (ULONG r = 0; r <= ZYDIS_REGISTER_MAX_VALUE; r++)
            {
                if (registerTtl[r] > 0)
                    registerTtl[r]--;
                if (registerTtl[r] == 0)
                    registerLastIat[r] = 0;
            }

            // Track mov reg, [rip+disp] where target is IAT slot
            if (
                instruction.mnemonic == ZYDIS_MNEMONIC_MOV &&
                instruction.operand_count_visible >= 2 &&
                operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
                operands[1].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                operands[1].mem.base == ZYDIS_REGISTER_RIP
                )
            {
                ULONG64 memoryVa = PvRipTarget(ip, instruction.length, operands[1].mem.disp.value);

                if (PvFindIatName(&Context->IatMap, memoryVa))
                {
                    ZydisRegister reg = operands[0].reg.value;
                    registerLastIat[reg] = memoryVa;
                    registerTtl[reg] = 3;
                }
            }

            // CALL instructions
            if (
                instruction.meta.category == ZYDIS_CATEGORY_CALL &&
                instruction.operand_count_visible >= 1
                )
            {
                PCWSTR targetName = NULL;
                ULONG64 slotVa = 0;

                // Resolve call target
                if (operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY && operands[0].mem.base == ZYDIS_REGISTER_RIP)
                {
                    // call [rip+disp]
                    ULONG64 memoryVa = PvRipTarget(ip, instruction.length, operands[0].mem.disp.value);
                    targetName = PvFindIatName(&Context->IatMap, memoryVa);
                    slotVa = memoryVa;
                }
                else if (operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER)
                {
                    // call reg
                    ZydisRegister reg = operands[0].reg.value;

                    if (registerLastIat[reg])
                    {
                        targetName = PvFindIatName(&Context->IatMap, registerLastIat[reg]);
                        slotVa = registerLastIat[reg];
                    }
                }

                if (targetName)
                {
                    WCHAR argument[512];

                    RtlZeroMemory(argument, sizeof(argument));

                    if (Context->IsGetProcAddressPage)
                    {
                        // GetProcAddress: RDX contains procedure name pointer or ordinal
                        PV_VAL value = PvBackSliceResolveRegWindow(
                            &window,
                            currentWindowIndex,
                            (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) ? ZYDIS_REGISTER_RDX : ZYDIS_REGISTER_EDX,
                            &Context->IatMap
                            );

                        if (value.Kind == PvValConst && value.U <= 0xFFFF)
                        {
                            _snwprintf_s(argument, RTL_NUMBER_OF(argument), _TRUNCATE, L"<ordinal:%llu>", value.U);
                        }
                        else if ((value.Kind == PvValAddr || value.Kind == PvValConst) && value.U)
                        {
                            PvReadStringAtVa(value.U, argument, RTL_NUMBER_OF(argument));

                            if (argument[0] == UNICODE_NULL)
                                wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                        }
                        else
                        {
                            wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                        }
                    }
                    else
                    {
                        // LoadLibrary*: RCX contains DLL name string (ANSI/Unicode)
                        PV_VAL value = PvBackSliceResolveRegWindow(
                            &window,
                            currentWindowIndex,
                            (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) ? ZYDIS_REGISTER_RCX : ZYDIS_REGISTER_ECX,
                            &Context->IatMap
                            );

                        if ((value.Kind == PvValAddr || value.Kind == PvValConst) && value.U)
                        {
                            PvReadStringAtVa(value.U, argument, RTL_NUMBER_OF(argument));

                            if (argument[0] == UNICODE_NULL)
                                wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                        }
                        else
                        {
                            wcscpy_s(argument, RTL_NUMBER_OF(argument), L"<unknown>");
                        }
                    }

                    // Store result row
                    PvEnsureRowsCapacity(&Context->Rows, 1);

                    {
                        PPV_API_SCAN_ROW row = &Context->Rows.Items[Context->Rows.Count++];

                        row->CallVa = ip;
                        row->IatSlotVa = slotVa;
                        wcsncpy_s(row->Target, RTL_NUMBER_OF(row->Target), targetName, _TRUNCATE);
                        wcsncpy_s(row->Argument, RTL_NUMBER_OF(row->Argument), argument, _TRUNCATE);
                    }
                }
            }

            offset += instruction.length;
            ip += instruction.length;
        }
    }
}

VOID PvPopulateListView(
    _In_ PPV_APISCAN_PAGE_CONTEXT ctx
    )
{
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

    PvScanImageForPage(Context);
    PvPopulateListView(Context);

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
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->IatMap.Items)
            {
                PhFree(context->IatMap.Items);
            }

            if (context->Rows.Items)
            {
                PhFree(context->Rows.Items);
            }

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

    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->IatMap.Items)
            {
                PhFree(context->IatMap.Items);
            }

            if (context->Rows.Items)
            {
                PhFree(context->Rows.Items);
            }

            PhFree(context);
        }
        break;
    }

    return FALSE;
}
