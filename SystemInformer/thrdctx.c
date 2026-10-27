/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <procprv.h>
#include <emenu.h>
#include <mapldr.h>

typedef struct _PH_THREAD_CONTEXT_DIALOG_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;

    // Captured (stack frame) context. When set the live thread context is not queried.
    USHORT FrameMachine;
    PVOID FrameContext;
    PCWSTR FrameTitle;
    PPH_THREAD_FRAME_CONTEXT PriorFrames;
    ULONG PriorFrameCount;
    LONG ValueColumn;
} PH_THREAD_CONTEXT_DIALOG_CONTEXT, *PPH_THREAD_CONTEXT_DIALOG_CONTEXT;

// ListView group identifiers.
#define PH_CONTEXT_GROUP_GENERAL    0
#define PH_CONTEXT_GROUP_CONTROL    1
#define PH_CONTEXT_GROUP_SEGMENTS   2
#define PH_CONTEXT_GROUP_DEBUG      3
#define PH_CONTEXT_GROUP_X87        4
#define PH_CONTEXT_GROUP_XMM        5
#define PH_CONTEXT_GROUP_YMM        6
#define PH_CONTEXT_GROUP_AVX512     7
#define PH_CONTEXT_GROUP_CET        8

// The SDK only defines the combined XSTATE_MASK_AVX512.
#ifndef XSTATE_MASK_AVX512_KMASK
#define XSTATE_MASK_AVX512_KMASK (1ui64 << (XSTATE_AVX512_KMASK))
#endif
#ifndef XSTATE_MASK_AVX512_ZMM_H
#define XSTATE_MASK_AVX512_ZMM_H (1ui64 << (XSTATE_AVX512_ZMM_H))
#endif
#ifndef XSTATE_MASK_AVX512_ZMM
#define XSTATE_MASK_AVX512_ZMM (1ui64 << (XSTATE_AVX512_ZMM))
#endif

#if defined(_M_X64)
static typeof(&RtlGetEnabledExtendedFeatures) RtlGetEnabledExtendedFeatures_I = NULL;
static typeof(&RtlGetExtendedContextLength) RtlGetExtendedContextLength_I = NULL;
static typeof(&RtlInitializeExtendedContext) RtlInitializeExtendedContext_I = NULL;
static typeof(&RtlLocateLegacyContext) RtlLocateLegacyContext_I = NULL;
static typeof(&RtlSetExtendedFeaturesMask) RtlSetExtendedFeaturesMask_I = NULL;
static typeof(&RtlGetExtendedFeaturesMask) RtlGetExtendedFeaturesMask_I = NULL;
static typeof(&RtlLocateExtendedFeature) RtlLocateExtendedFeature_I = NULL;

BOOLEAN PhInitializeThreadXStateImports(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN imported = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhGetLoaderEntryDllBaseZ(L"ntdll.dll"))
        {
            RtlGetEnabledExtendedFeatures_I = PhGetDllBaseProcedureAddress(baseAddress, "RtlGetEnabledExtendedFeatures", 0);
            RtlGetExtendedContextLength_I = PhGetDllBaseProcedureAddress(baseAddress, "RtlGetExtendedContextLength", 0);
            RtlInitializeExtendedContext_I = PhGetDllBaseProcedureAddress(baseAddress, "RtlInitializeExtendedContext", 0);
            RtlLocateLegacyContext_I = PhGetDllBaseProcedureAddress(baseAddress, "RtlLocateLegacyContext", 0);
            RtlSetExtendedFeaturesMask_I = PhGetDllBaseProcedureAddress(baseAddress, "RtlSetExtendedFeaturesMask", 0);
            RtlGetExtendedFeaturesMask_I = PhGetDllBaseProcedureAddress(baseAddress, "RtlGetExtendedFeaturesMask", 0);
            RtlLocateExtendedFeature_I = PhGetDllBaseProcedureAddress(baseAddress, "RtlLocateExtendedFeature", 0);
        }

        imported =
            RtlGetEnabledExtendedFeatures_I &&
            RtlGetExtendedContextLength_I &&
            RtlInitializeExtendedContext_I &&
            RtlLocateLegacyContext_I &&
            RtlSetExtendedFeaturesMask_I &&
            RtlGetExtendedFeaturesMask_I &&
            RtlLocateExtendedFeature_I;

        PhEndInitOnce(&initOnce);
    }

    return imported;
}
#endif

LONG PhThreadFrameContextAddItem(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    )
{
    LONG index;

    LVFINDINFO findInfo = { 0 };

    findInfo.flags = LVFI_STRING;
    findInfo.psz = Name;
    index = ListView_FindItem(Context->ListViewHandle, -1, &findInfo);

    if (index == -1)
    {
        index = PhAddListViewGroupItem(Context->ListViewHandle, GroupId, MAXINT, Name, NULL);

        // A register may exist only in another architecture's context.
        for (LONG column = 1; column < Context->ValueColumn; column++)
            PhSetListViewSubItem(Context->ListViewHandle, index, column, L"Unavailable");
    }

    PhSetListViewSubItem(Context->ListViewHandle, index, Context->ValueColumn, Value);

    return index;
}

// Formats a little-endian byte buffer as a hex string, most significant byte first.
PPH_STRING PhThreadFrameContextFormatHexBytes(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, Length * 2 + 2);
    PhAppendStringBuilder2(&stringBuilder, L"0x");

    for (ULONG i = Length; i != 0; i--)
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"%02x", Buffer[i - 1]);
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PhAddThreadFrameContextVectorItem(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context,
    _In_ LONG GroupId,
    _In_ PCWSTR Name,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    PPH_STRING string;

    string = PhThreadFrameContextFormatHexBytes(Buffer, Length);
    PhThreadFrameContextAddItem(Context, GroupId, Name, string->Buffer);
    PhDereferenceObject(string);
}

PPH_STRING PhThreadFrameContextFormatEFlags(
    _In_ ULONG EFlags
    )
{
    static const struct { ULONG Bit; PCWSTR Name; } flags[] =
    {
        { 0x0001, L"CF" },
        { 0x0004, L"PF" },
        { 0x0010, L"AF" },
        { 0x0040, L"ZF" },
        { 0x0080, L"SF" },
        { 0x0100, L"TF" },
        { 0x0200, L"IF" },
        { 0x0400, L"DF" },
        { 0x0800, L"OF" },
    };
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 64);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%08lx (", EFlags);

    for (ULONG i = 0; i < RTL_NUMBER_OF(flags); i++)
    {
        if (EFlags & flags[i].Bit)
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s ", flags[i].Name);
        }
    }

    if (PhEndsWithStringRef2(&stringBuilder.String->sr, L" ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    PhAppendCharStringBuilder(&stringBuilder, L')');

    return PhFinalStringBuilderString(&stringBuilder);
}

#if defined(_M_X64)
VOID PhAddNativeContextItems(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context,
    _In_ PCONTEXT ThreadContext,
    _In_opt_ PCONTEXT_EX ContextEx
    )
{
    static const struct { PCWSTR Name; ULONG Offset; } generalRegisters[] =
    {
        { L"Rax", FIELD_OFFSET(CONTEXT, Rax) },
        { L"Rbx", FIELD_OFFSET(CONTEXT, Rbx) },
        { L"Rcx", FIELD_OFFSET(CONTEXT, Rcx) },
        { L"Rdx", FIELD_OFFSET(CONTEXT, Rdx) },
        { L"Rsi", FIELD_OFFSET(CONTEXT, Rsi) },
        { L"Rdi", FIELD_OFFSET(CONTEXT, Rdi) },
        { L"Rbp", FIELD_OFFSET(CONTEXT, Rbp) },
        { L"Rsp", FIELD_OFFSET(CONTEXT, Rsp) },
        { L"R8", FIELD_OFFSET(CONTEXT, R8) },
        { L"R9", FIELD_OFFSET(CONTEXT, R9) },
        { L"R10", FIELD_OFFSET(CONTEXT, R10) },
        { L"R11", FIELD_OFFSET(CONTEXT, R11) },
        { L"R12", FIELD_OFFSET(CONTEXT, R12) },
        { L"R13", FIELD_OFFSET(CONTEXT, R13) },
        { L"R14", FIELD_OFFSET(CONTEXT, R14) },
        { L"R15", FIELD_OFFSET(CONTEXT, R15) },
        { L"Rip", FIELD_OFFSET(CONTEXT, Rip) },
    };
    HWND lvHandle = Context->ListViewHandle;
    PPH_STRING string;
    WCHAR name[16];

    ListView_EnableGroupView(lvHandle, TRUE);
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_GENERAL, L"General purpose");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_CONTROL, L"Control and flags");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_SEGMENTS, L"Segments");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_DEBUG, L"Debug registers");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_X87, L"x87 floating point");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_XMM, L"SSE (XMM)");

    for (ULONG i = 0; i < RTL_NUMBER_OF(generalRegisters); i++)
    {
        PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, generalRegisters[i].Name,
            PhaFormatString(L"0x%016I64x", *(PULONG64)PTR_ADD_OFFSET(ThreadContext, generalRegisters[i].Offset))->Buffer);
    }

    string = PhThreadFrameContextFormatEFlags(ThreadContext->EFlags);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"EFlags", string->Buffer);
    PhDereferenceObject(string);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"MxCsr", PhaFormatString(L"0x%08lx", ThreadContext->MxCsr)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"ContextFlags", PhaFormatString(L"0x%08lx", ThreadContext->ContextFlags)->Buffer);

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegCs", PhaFormatString(L"0x%04x", ThreadContext->SegCs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegSs", PhaFormatString(L"0x%04x", ThreadContext->SegSs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegDs", PhaFormatString(L"0x%04x", ThreadContext->SegDs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegEs", PhaFormatString(L"0x%04x", ThreadContext->SegEs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegFs", PhaFormatString(L"0x%04x", ThreadContext->SegFs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegGs", PhaFormatString(L"0x%04x", ThreadContext->SegGs)->Buffer);

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr0", PhaFormatString(L"0x%016I64x", ThreadContext->Dr0)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr1", PhaFormatString(L"0x%016I64x", ThreadContext->Dr1)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr2", PhaFormatString(L"0x%016I64x", ThreadContext->Dr2)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr3", PhaFormatString(L"0x%016I64x", ThreadContext->Dr3)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr6", PhaFormatString(L"0x%016I64x", ThreadContext->Dr6)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr7", PhaFormatString(L"0x%016I64x", ThreadContext->Dr7)->Buffer);

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"ControlWord", PhaFormatString(L"0x%04x", ThreadContext->FltSave.ControlWord)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"StatusWord", PhaFormatString(L"0x%04x", ThreadContext->FltSave.StatusWord)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"TagWord", PhaFormatString(L"0x%02x", ThreadContext->FltSave.TagWord)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"ErrorOffset", PhaFormatString(L"0x%08lx", ThreadContext->FltSave.ErrorOffset)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"ErrorSelector", PhaFormatString(L"0x%04x", ThreadContext->FltSave.ErrorSelector)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"DataOffset", PhaFormatString(L"0x%08lx", ThreadContext->FltSave.DataOffset)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"DataSelector", PhaFormatString(L"0x%04x", ThreadContext->FltSave.DataSelector)->Buffer);

    for (ULONG i = 0; i < 8; i++)
    {
        _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"St%lu", i);
        PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_X87, name, &ThreadContext->FltSave.FloatRegisters[i], sizeof(M128A));
    }

    for (ULONG i = 0; i < 16; i++)
    {
        _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"Xmm%lu", i);
        PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_XMM, name, &ThreadContext->FltSave.XmmRegisters[i], sizeof(M128A));
    }

    if (ContextEx && RtlGetEnabledExtendedFeatures_I && RtlGetExtendedFeaturesMask_I && RtlLocateExtendedFeature_I)
    {
        ULONG64 enabledFeatures = RtlGetEnabledExtendedFeatures_I(MAXULONG64);
        PM128A avx;
        ULONG avxLength = 0;

        // Note: a feature bit clear in the post-get mask means the component is in its
        // INIT (all zero) state. The buffer is zero-initialized, so the located values
        // correctly display as zeros in that case.

        if (FlagOn(enabledFeatures, XSTATE_MASK_AVX) &&
            (avx = RtlLocateExtendedFeature_I((PCONTEXT_EX)ContextEx, XSTATE_AVX, &avxLength)) &&
            avxLength >= 16 * sizeof(M128A))
        {
            if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_YMM, L"AVX (YMM)");

            for (ULONG i = 0; i < 16; i++)
            {
                UCHAR ymm[32];

                memcpy(&ymm[0], &ThreadContext->FltSave.XmmRegisters[i], 16);
                memcpy(&ymm[16], &avx[i], 16);

                _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"Ymm%lu", i);
                PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_YMM, name, ymm, sizeof(ymm));
            }
        }

        if (FlagOn(enabledFeatures, XSTATE_MASK_AVX512_KMASK))
        {
            PULONG64 kmask;
            PUCHAR zmmHigh;
            PUCHAR zmmFull;
            ULONG kmaskLength = 0;
            ULONG zmmHighLength = 0;
            ULONG zmmFullLength = 0;

            if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_AVX512, L"AVX-512");

            if ((kmask = RtlLocateExtendedFeature_I((PCONTEXT_EX)ContextEx, XSTATE_AVX512_KMASK, &kmaskLength)) &&
                kmaskLength >= 8 * sizeof(ULONG64))
            {
                for (ULONG i = 0; i < 8; i++)
                {
                    _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"K%lu", i);
                    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_AVX512, name, PhaFormatString(L"0x%016I64x", kmask[i])->Buffer);
                }
            }

            if (FlagOn(enabledFeatures, XSTATE_MASK_AVX512_ZMM_H) && avxLength >= 16 * sizeof(M128A) &&
                (zmmHigh = RtlLocateExtendedFeature_I((PCONTEXT_EX)ContextEx, XSTATE_AVX512_ZMM_H, &zmmHighLength)) &&
                zmmHighLength >= 16 * 32)
            {
                avx = RtlLocateExtendedFeature_I((PCONTEXT_EX)ContextEx, XSTATE_AVX, NULL);

                for (ULONG i = 0; i < 16; i++)
                {
                    UCHAR zmm[64];

                    memcpy(&zmm[0], &ThreadContext->FltSave.XmmRegisters[i], 16);
                    memcpy(&zmm[16], &avx[i], 16);
                    memcpy(&zmm[32], &zmmHigh[i * 32], 32);

                    _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"Zmm%lu", i);
                    PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_AVX512, name, zmm, sizeof(zmm));
                }
            }

            if (FlagOn(enabledFeatures, XSTATE_MASK_AVX512_ZMM) &&
                (zmmFull = RtlLocateExtendedFeature_I((PCONTEXT_EX)ContextEx, XSTATE_AVX512_ZMM, &zmmFullLength)) &&
                zmmFullLength >= 16 * 64)
            {
                for (ULONG i = 0; i < 16; i++)
                {
                    _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"Zmm%lu", i + 16);
                    PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_AVX512, name, &zmmFull[i * 64], 64);
                }
            }
        }
    }
}

PPH_STRING PhThreadFrameContextFormatCetUMsr(
    _In_ ULONG64 CetUMsr
    )
{
    static const struct { ULONG64 Bit; PCWSTR Name; } flags[] =
    {
        { PH_CET_U_SH_STK_EN, L"SH_STK_EN" },
        { PH_CET_U_WR_SHSTK_EN, L"WR_SHSTK_EN" },
        { PH_CET_U_ENDBR_EN, L"ENDBR_EN" },
        { PH_CET_U_LEG_IW_EN, L"LEG_IW_EN" },
        { PH_CET_U_NO_TRACK_EN, L"NO_TRACK_EN" },
        { PH_CET_U_SUPPRESS_DIS, L"SUPPRESS_DIS" },
        { PH_CET_U_SUPPRESS, L"SUPPRESS" },
        { PH_CET_U_TRACKER, L"TRACKER" },
    };
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 64);
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%016I64x (", CetUMsr);

    for (ULONG i = 0; i < RTL_NUMBER_OF(flags); i++)
    {
        if (CetUMsr & flags[i].Bit)
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s ", flags[i].Name);
        }
    }

    if (CetUMsr & PH_CET_U_LEG_BITMAP_BASE_MASK)
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"Bitmap=0x%I64x ", CetUMsr & PH_CET_U_LEG_BITMAP_BASE_MASK);
    }

    if (PhEndsWithStringRef2(&stringBuilder.String->sr, L" ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    PhAppendCharStringBuilder(&stringBuilder, L')');

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID PhAddNativeCetContextItems(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context,
    _In_ PXSAVE_CET_U_FORMAT CetUserState
    )
{
    PPH_STRING string;

    if (Context->ValueColumn == 1)
        PhAddListViewGroup(Context->ListViewHandle, PH_CONTEXT_GROUP_CET, L"CET shadow stack");

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CET, L"Ssp", PhaFormatString(L"0x%016I64x", CetUserState->Ia32Pl3SspMsr)->Buffer);

    string = PhThreadFrameContextFormatCetUMsr(CetUserState->Ia32CetUMsr);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CET, L"CetUMsr", string->Buffer);
    PhDereferenceObject(string);
}

VOID PhAddWow64ContextItems(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context,
    _In_ PWOW64_CONTEXT ThreadContext
    )
{
    HWND lvHandle = Context->ListViewHandle;
    PPH_STRING string;
    WCHAR name[16];

    ListView_EnableGroupView(lvHandle, TRUE);
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_GENERAL, L"General purpose (WOW64)");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_CONTROL, L"Control and flags");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_SEGMENTS, L"Segments");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_DEBUG, L"Debug registers");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_X87, L"x87 floating point");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_XMM, L"SSE (XMM)");

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Eax", PhaFormatString(L"0x%08lx", ThreadContext->Eax)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Ebx", PhaFormatString(L"0x%08lx", ThreadContext->Ebx)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Ecx", PhaFormatString(L"0x%08lx", ThreadContext->Ecx)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Edx", PhaFormatString(L"0x%08lx", ThreadContext->Edx)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Esi", PhaFormatString(L"0x%08lx", ThreadContext->Esi)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Edi", PhaFormatString(L"0x%08lx", ThreadContext->Edi)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Ebp", PhaFormatString(L"0x%08lx", ThreadContext->Ebp)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Esp", PhaFormatString(L"0x%08lx", ThreadContext->Esp)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Eip", PhaFormatString(L"0x%08lx", ThreadContext->Eip)->Buffer);

    string = PhThreadFrameContextFormatEFlags(ThreadContext->EFlags);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"EFlags", string->Buffer);
    PhDereferenceObject(string);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"ContextFlags", PhaFormatString(L"0x%08lx", ThreadContext->ContextFlags)->Buffer);

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegCs", PhaFormatString(L"0x%04lx", ThreadContext->SegCs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegSs", PhaFormatString(L"0x%04lx", ThreadContext->SegSs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegDs", PhaFormatString(L"0x%04lx", ThreadContext->SegDs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegEs", PhaFormatString(L"0x%04lx", ThreadContext->SegEs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegFs", PhaFormatString(L"0x%04lx", ThreadContext->SegFs)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_SEGMENTS, L"SegGs", PhaFormatString(L"0x%04lx", ThreadContext->SegGs)->Buffer);

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr0", PhaFormatString(L"0x%08lx", ThreadContext->Dr0)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr1", PhaFormatString(L"0x%08lx", ThreadContext->Dr1)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr2", PhaFormatString(L"0x%08lx", ThreadContext->Dr2)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr3", PhaFormatString(L"0x%08lx", ThreadContext->Dr3)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr6", PhaFormatString(L"0x%08lx", ThreadContext->Dr6)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_DEBUG, L"Dr7", PhaFormatString(L"0x%08lx", ThreadContext->Dr7)->Buffer);

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"ControlWord", PhaFormatString(L"0x%08lx", ThreadContext->FloatSave.ControlWord)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"StatusWord", PhaFormatString(L"0x%08lx", ThreadContext->FloatSave.StatusWord)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"TagWord", PhaFormatString(L"0x%08lx", ThreadContext->FloatSave.TagWord)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"ErrorOffset", PhaFormatString(L"0x%08lx", ThreadContext->FloatSave.ErrorOffset)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"ErrorSelector", PhaFormatString(L"0x%08lx", ThreadContext->FloatSave.ErrorSelector)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"DataOffset", PhaFormatString(L"0x%08lx", ThreadContext->FloatSave.DataOffset)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_X87, L"DataSelector", PhaFormatString(L"0x%08lx", ThreadContext->FloatSave.DataSelector)->Buffer);

    for (ULONG i = 0; i < 8; i++)
    {
        _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"St%lu", i);
        PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_X87, name, &ThreadContext->FloatSave.RegisterArea[i * 10], 10);
    }

    // ExtendedRegisters is the FXSAVE area: MxCsr at offset 24, XMM0-7 at offset 160.
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"MxCsr", PhaFormatString(L"0x%08lx", *(PULONG)&ThreadContext->ExtendedRegisters[24])->Buffer);

    for (ULONG i = 0; i < 8; i++)
    {
        _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"Xmm%lu", i);
        PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_XMM, name, &ThreadContext->ExtendedRegisters[160 + i * 16], 16);
    }
}
#elif defined(_M_ARM64)
VOID PhAddNativeContextItems(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context,
    _In_ PCONTEXT ThreadContext
    )
{
    HWND lvHandle = Context->ListViewHandle;
    WCHAR name[16];

    ListView_EnableGroupView(lvHandle, TRUE);
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_GENERAL, L"General purpose");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_CONTROL, L"Control and flags");
    if (Context->ValueColumn == 1)
        PhAddListViewGroup(lvHandle, PH_CONTEXT_GROUP_XMM, L"NEON (V)");

    for (ULONG i = 0; i < 29; i++)
    {
        _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"X%lu", i);
        PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, name, PhaFormatString(L"0x%016I64x", ThreadContext->X[i])->Buffer);
    }

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Fp", PhaFormatString(L"0x%016I64x", ThreadContext->Fp)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Lr", PhaFormatString(L"0x%016I64x", ThreadContext->Lr)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Sp", PhaFormatString(L"0x%016I64x", ThreadContext->Sp)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_GENERAL, L"Pc", PhaFormatString(L"0x%016I64x", ThreadContext->Pc)->Buffer);

    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"Cpsr", PhaFormatString(L"0x%08lx", ThreadContext->Cpsr)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"Fpcr", PhaFormatString(L"0x%08lx", ThreadContext->Fpcr)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"Fpsr", PhaFormatString(L"0x%08lx", ThreadContext->Fpsr)->Buffer);
    PhThreadFrameContextAddItem(Context, PH_CONTEXT_GROUP_CONTROL, L"ContextFlags", PhaFormatString(L"0x%08lx", ThreadContext->ContextFlags)->Buffer);

    for (ULONG i = 0; i < 32; i++)
    {
        _snwprintf_s(name, RTL_NUMBER_OF(name), _TRUNCATE, L"V%lu", i);
        PhAddThreadFrameContextVectorItem(Context, PH_CONTEXT_GROUP_XMM, name, &ThreadContext->V[i], sizeof(ThreadContext->V[i]));
    }
}
#endif

NTSTATUS PhQueryThreadFrameContext(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    status = PhOpenThread(
        &threadHandle,
        THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_LIMITED_INFORMATION,
        Context->ThreadId
        );

    if (!NT_SUCCESS(status))
    {
        status = PhOpenThread(
            &threadHandle,
            THREAD_GET_CONTEXT,
            Context->ThreadId
            );
    }

    if (!NT_SUCCESS(status))
        return status;

#if defined(_M_X64)
    {
        PPH_PROCESS_ITEM processItem;
        BOOLEAN isWow64 = FALSE;

        if (processItem = PhReferenceProcessItem(Context->ProcessId))
        {
            isWow64 = !!processItem->IsWow64Process;
            PhDereferenceObject(processItem);
        }

        if (isWow64)
        {
            WOW64_CONTEXT wow64Context;

            memset(&wow64Context, 0, sizeof(WOW64_CONTEXT));
            wow64Context.ContextFlags = WOW64_CONTEXT_ALL;

            status = RtlWow64GetThreadContext(threadHandle, &wow64Context);

            if (NT_SUCCESS(status))
                PhAddWow64ContextItems(Context, &wow64Context);

            NtClose(threadHandle);
            return status;
        }

        status = STATUS_NOT_SUPPORTED;

        if (PhInitializeThreadXStateImports() && RtlGetEnabledExtendedFeatures_I(MAXULONG64))
        {
            ULONG contextLength = 0;

            status = RtlGetExtendedContextLength_I(CONTEXT_ALL | CONTEXT_XSTATE, &contextLength);

            if (NT_SUCCESS(status))
            {
                PVOID buffer;
                PCONTEXT_EX contextEx = NULL;
                PCONTEXT threadContext = NULL;

                buffer = PhAllocateZero(contextLength);
                status = RtlInitializeExtendedContext_I(buffer, CONTEXT_ALL | CONTEXT_XSTATE, &contextEx);

                if (NT_SUCCESS(status))
                {
                    threadContext = RtlLocateLegacyContext_I(contextEx, NULL);
                    RtlSetExtendedFeaturesMask_I(contextEx, RtlGetEnabledExtendedFeatures_I(MAXULONG64));

                    status = NtGetContextThread(threadHandle, threadContext);

                    if (NT_SUCCESS(status))
                        PhAddNativeContextItems(Context, threadContext, contextEx);
                }

                PhFree(buffer);
            }
        }

        if (!NT_SUCCESS(status))
        {
            // Fall back to the legacy context when extended (XState) support is unavailable.
            CONTEXT threadContext;

            memset(&threadContext, 0, sizeof(CONTEXT));
            threadContext.ContextFlags = CONTEXT_ALL;

            status = PhGetContextThread(threadHandle, &threadContext);

            if (NT_SUCCESS(status))
                PhAddNativeContextItems(Context, &threadContext, NULL);
        }

        if (NT_SUCCESS(status))
        {
            XSAVE_CET_U_FORMAT cetUserState;

            if (NT_SUCCESS(PhGetThreadCetUserState(threadHandle, &cetUserState)) &&
                FlagOn(cetUserState.Ia32CetUMsr, PH_CET_U_SH_STK_EN))
                PhAddNativeCetContextItems(Context, &cetUserState);
        }
    }
#elif defined(_M_ARM64)
    {
        CONTEXT threadContext;

        memset(&threadContext, 0, sizeof(CONTEXT));
        threadContext.ContextFlags = CONTEXT_ALL;

        status = PhGetContextThread(threadHandle, &threadContext);

        if (NT_SUCCESS(status))
            PhAddNativeContextItems(Context, &threadContext);
    }
#else
    status = STATUS_NOT_SUPPORTED;
#endif

    NtClose(threadHandle);
    return status;
}

NTSTATUS PhAddFrameContextItems(
    _In_ PPH_THREAD_CONTEXT_DIALOG_CONTEXT Context
    )
{
#if defined(_M_X64)
    if (Context->FrameMachine == IMAGE_FILE_MACHINE_AMD64)
    {
        PhAddNativeContextItems(Context, Context->FrameContext, NULL);
        return STATUS_SUCCESS;
    }

    if (Context->FrameMachine == IMAGE_FILE_MACHINE_I386)
    {
        PhAddWow64ContextItems(Context, Context->FrameContext);
        return STATUS_SUCCESS;
    }
#elif defined(_M_ARM64)
    // ARM64EC frames are unwound with the native context.
    if (Context->FrameMachine == IMAGE_FILE_MACHINE_ARM64 || Context->FrameMachine == IMAGE_FILE_MACHINE_ARM64EC)
    {
        PhAddNativeContextItems(Context, Context->FrameContext);
        return STATUS_SUCCESS;
    }
#endif

    return STATUS_NOT_SUPPORTED;
}

INT_PTR CALLBACK PhThreadContextDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_THREAD_CONTEXT_DIALOG_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_THREAD_CONTEXT_DIALOG_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            PPH_STRING title;
            CLIENT_ID clientId;

            clientId.UniqueProcess = context->ProcessId;
            clientId.UniqueThread = context->ThreadId;
            if (context->FrameContext && context->FrameTitle)
            {
                title = PhaConcatStrings(
                    4,
                    L"Frame Registers - ",
                    context->FrameTitle,
                    L" - ",
                    PH_AUTO_T(PH_STRING, PhGetClientIdName(&clientId))->Buffer
                    );
            }
            else
            {
                title = PhaConcatStrings2(L"Thread Context - ", PH_AUTO_T(PH_STRING, PhGetClientIdName(&clientId))->Buffer);
            }
            PhSetWindowText(hwndDlg, title->Buffer);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Register");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 380, L"Value");
            context->ValueColumn = 1;

            for (ULONG i = 0; i < context->PriorFrameCount; i++)
            {
                PhAddListViewColumn(context->ListViewHandle, i + 2, i + 2, i + 2, LVCFMT_LEFT, 380,
                    PhaFormatString(L"Frame #%lu", context->PriorFrames[i].Index)->Buffer);
            }
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (context->FrameContext)
                status = PhAddFrameContextItems(context);
            else
                status = PhQueryThreadFrameContext(context);

            if (!NT_SUCCESS(status))
            {
                PhShowStatus(hwndDlg, L"Unable to query the thread context.", status, 0);
            }

            for (ULONG i = 0; i < context->PriorFrameCount; i++)
            {
                PH_THREAD_CONTEXT_DIALOG_CONTEXT frameContext = *context;

                frameContext.FrameMachine = context->PriorFrames[i].Machine;
                frameContext.FrameContext = context->PriorFrames[i].ContextRecord;
                frameContext.ValueColumn = i + 2;

                for (LONG row = 0; row < ListView_GetItemCount(context->ListViewHandle); row++)
                    PhSetListViewSubItem(context->ListViewHandle, row, frameContext.ValueColumn, L"Unavailable");

                if (frameContext.FrameContext)
                    PhAddFrameContextItems(&frameContext);
            }

            PhSetDialogFocus(hwndDlg, context->ListViewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU_ITEM item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                    PhFree(listviewItems);
                }
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhShowThreadContextDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId
    )
{
    PH_THREAD_CONTEXT_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_THREAD_CONTEXT_DIALOG_CONTEXT));
    context.ProcessId = ProcessId;
    context.ThreadId = ThreadId;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDCONTEXT),
        ParentWindowHandle,
        PhThreadContextDlgProc,
        &context
        );
}

/**
 * Shows the registers of a captured (unwound) stack frame context.
 *
 * \param ParentWindowHandle The parent window.
 * \param ProcessId The process ID of the thread.
 * \param ThreadId The thread ID.
 * \param Machine The machine type of the frame (PH_THREAD_STACK_FRAME::Machine).
 * \param FrameContext The captured context record matching \a Machine.
 * \param FrameTitle An optional description of the frame.
 * \param PriorFrames Captured contexts for frames preceding the selected frame.
 * \param PriorFrameCount The number of preceding frames.
 */
VOID PhShowThreadFrameContextDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ HANDLE ThreadId,
    _In_ USHORT Machine,
    _In_ PVOID FrameContext,
    _In_opt_ PCWSTR FrameTitle,
    _In_reads_(PriorFrameCount) PPH_THREAD_FRAME_CONTEXT PriorFrames,
    _In_ ULONG PriorFrameCount
    )
{
    PH_THREAD_CONTEXT_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_THREAD_CONTEXT_DIALOG_CONTEXT));
    context.ProcessId = ProcessId;
    context.ThreadId = ThreadId;
    context.FrameMachine = Machine;
    context.FrameContext = FrameContext;
    context.FrameTitle = FrameTitle;
    context.PriorFrames = PriorFrames;
    context.PriorFrameCount = PriorFrameCount;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_THRDCONTEXT),
        ParentWindowHandle,
        PhThreadContextDlgProc,
        &context
        );
}
