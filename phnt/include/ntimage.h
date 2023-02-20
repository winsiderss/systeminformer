/*
 * PE format support
 *
 * This file is part of System Informer.
 */

#ifndef _NTIMAGE_H
#define _NTIMAGE_H

#ifndef _MAC
#include <pshpack4.h>
#else
#include <pshpack1.h>
#endif

typedef struct _IMAGE_DEBUG_POGO_ENTRY
{
    ULONG Rva;
    ULONG Size;
    CHAR Name[1];
} IMAGE_DEBUG_POGO_ENTRY, *PIMAGE_DEBUG_POGO_ENTRY;

typedef struct _IMAGE_DEBUG_POGO_SIGNATURE
{
    ULONG Signature;
} IMAGE_DEBUG_POGO_SIGNATURE, *PIMAGE_DEBUG_POGO_SIGNATURE;

#define IMAGE_DEBUG_POGO_SIGNATURE_LTCG 'LTCG' // coffgrp LTCG (0x4C544347)
#define IMAGE_DEBUG_POGO_SIGNATURE_PGU 'PGU\0' // coffgrp PGU (0x50475500)

typedef struct _IMAGE_BASE_RELOCATION_ENTRY
{
    USHORT Offset : 12;
    USHORT Type : 4;
} IMAGE_BASE_RELOCATION_ENTRY, *PIMAGE_BASE_RELOCATION_ENTRY;

typedef struct _IMAGE_CHPE_METADATA_X86 
{
	ULONG  Version;
	ULONG  CHPECodeAddressRangeOffset;
	ULONG  CHPECodeAddressRangeCount;
	ULONG  WowA64ExceptionHandlerFunctionPointer;
	ULONG  WowA64DispatchCallFunctionPointer;
	ULONG  WowA64DispatchIndirectCallFunctionPointer;
	ULONG  WowA64DispatchIndirectCallCfgFunctionPointer;
	ULONG  WowA64DispatchRetFunctionPointer;
	ULONG  WowA64DispatchRetLeafFunctionPointer;
	ULONG  WowA64DispatchJumpFunctionPointer;
	ULONG  CompilerIATPointer;         // Present if Version >= 2
	ULONG  WowA64RdtscFunctionPointer; // Present if Version >= 3
} IMAGE_CHPE_METADATA_X86, *PIMAGE_CHPE_METADATA_X86;

typedef struct _IMAGE_CHPE_RANGE_ENTRY 
{
	union 
    {
		ULONG StartOffset;
		struct 
        {
			ULONG NativeCode : 1;
			ULONG AddressBits : 31;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;

	ULONG Length;
} IMAGE_CHPE_RANGE_ENTRY, *PIMAGE_CHPE_RANGE_ENTRY;

typedef struct _IMAGE_ARM64EC_METADATA 
{
	ULONG  Version;
	ULONG  CodeMap;
	ULONG  CodeMapCount;
	ULONG  CodeRangesToEntryPoints;
	ULONG  RedirectionMetadata;
	ULONG  tbd__os_arm64x_dispatch_call_no_redirect;
	ULONG  tbd__os_arm64x_dispatch_ret;
	ULONG  tbd__os_arm64x_dispatch_call;
	ULONG  tbd__os_arm64x_dispatch_icall;
	ULONG  tbd__os_arm64x_dispatch_icall_cfg;
	ULONG  AlternateEntryPoint;
	ULONG  AuxiliaryIAT;
	ULONG  CodeRangesToEntryPointsCount;
	ULONG  RedirectionMetadataCount;
	ULONG  GetX64InformationFunctionPointer;
	ULONG  SetX64InformationFunctionPointer;
	ULONG  ExtraRFETable;
	ULONG  ExtraRFETableSize;
	ULONG  __os_arm64x_dispatch_fptr;
	ULONG  AuxiliaryIATCopy;
} IMAGE_ARM64EC_METADATA, *PIMAGE_ARM64EC_METADATA;

typedef struct _IMAGE_ARM64EC_REDIRECTION_ENTRY 
{
	ULONG Source;
	ULONG Destination;
} IMAGE_ARM64EC_REDIRECTION_ENTRY, *PIMAGE_ARM64EC_REDIRECTION_ENTRY;

typedef struct _IMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT 
{
    ULONG StartRva;
    ULONG EndRva;
    ULONG EntryPoint;
} IMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT, *PIMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT;

#define IMAGE_DVRT_ARM64X_FIXUP_TYPE_ZEROFILL   0
#define IMAGE_DVRT_ARM64X_FIXUP_TYPE_VALUE      1
#define IMAGE_DVRT_ARM64X_FIXUP_TYPE_DELTA      2

#define IMAGE_DVRT_ARM64X_FIXUP_SIZE_2BYTES     1
#define IMAGE_DVRT_ARM64X_FIXUP_SIZE_4BYTES     2
#define IMAGE_DVRT_ARM64X_FIXUP_SIZE_8BYTES     3

typedef struct _IMAGE_DVRT_ARM64X_FIXUP_RECORD 
{
    USHORT Offset : 12;
    USHORT Type   :  2;
    USHORT Size   :  2;
	// Value of varaible Size when IMAGE_DVRT_ARM64X_FIXUP_TYPE_VALUE
} IMAGE_DVRT_ARM64X_FIXUP_RECORD, *PIMAGE_DVRT_ARM64X_FIXUP_RECORD;

typedef struct _IMAGE_DVRT_ARM64X_DELTA_FIXUP_RECORD 
{
    USHORT Offset : 12;
    USHORT Type   :  2; // IMAGE_DVRT_ARM64X_FIXUP_TYPE_DELTA
    USHORT Sign   :  1; // 1 = -, 0 = +
    USHORT Scale  :  1; // 1 = 8, 0 = 4
	// USHORT Value; // Delta = Value * Scale * Sign 
} IMAGE_DVRT_ARM64X_DELTA_FIXUP_RECORD, *PIMAGE_DVRT_ARM64X_DELTA_FIXUP_RECORD;

#include <poppack.h>

#define IMAGE_DYNAMIC_RELOCATION_ARM64X                         0x00000006
#define IMAGE_DYNAMIC_RELOCATION_MM_SHARED_USER_DATA_VA         0x7FFE0000
#define IMAGE_DYNAMIC_RELOCATION_KI_USER_SHARED_DATA64          0xFFFFF78000000000UI64

#endif