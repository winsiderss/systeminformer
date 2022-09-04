/*++

Copyright (c) 2004 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiTypes.h

Abstract:

  EFI defined types. Use these types when ever possible!

--*/

#ifndef _EFI_TYPES_H_
#define _EFI_TYPES_H_

//
// EFI Data Types based on ANSI C integer types in EfiBind.h
//
typedef uint8_t BOOLEAN;
//typedef intn_t INTN;
//typedef uintn_t UINTN;
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef uint8_t CHAR8;
typedef uint16_t CHAR16;
typedef UINT64 EFI_LBA;

//
// Modifiers for EFI Data Types used to self document code.
// Please see EFI coding convention for proper usage.
//
#ifndef IN
//
// Some other envirnments use this construct, so #ifndef to prevent
// mulitple definition.
//
#define IN
#define OUT
#define OPTIONAL
#endif

//#define UNALIGNED

//
// Modifiers for EFI Runtime and Boot Services
//
#define EFI_RUNTIMESERVICE
#define EFI_BOOTSERVICE

//
// Boot Service add in EFI 1.1
//
#define EFI_BOOTSERVICE11

//
// Modifiers to absract standard types to aid in debug of problems
//
#define CONST     const
#define STATIC    static
#define VOID      void
#define VOLATILE  volatile

//
// Modifier to ensure that all protocol member functions and EFI intrinsics
// use the correct C calling convention. All protocol member functions and
// EFI intrinsics are required to modify thier member functions with EFIAPI.
//
#define EFIAPI  _EFIAPI

//
// EFI Constants. They may exist in other build structures, so #ifndef them.
//
#ifndef TRUE
#define TRUE  ((BOOLEAN) (1 == 1))
#endif

#ifndef FALSE
#define FALSE ((BOOLEAN) (0 == 1))
#endif

#ifndef NULL
#define NULL  ((VOID *) 0)
#endif
//
// EFI Data Types derived from other EFI data types.
//
typedef UINT32 EFI_STATUS; // UINTN

typedef VOID *EFI_HANDLE;
#define NULL_HANDLE ((VOID *) 0)

typedef VOID *EFI_EVENT;
typedef UINT32 EFI_TPL; // UINTN

typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;

typedef union {
  EFI_GUID  Guid;
  UINT8     Raw[16];
} EFI_GUID_UNION;

//
// EFI Time Abstraction:
//  Year:       2000 - 20XX
//  Month:      1 - 12
//  Day:        1 - 31
//  Hour:       0 - 23
//  Minute:     0 - 59
//  Second:     0 - 59
//  Nanosecond: 0 - 999,999,999
//  TimeZone:   -1440 to 1440 or 2047
//
typedef struct {
  UINT16  Year;
  UINT8   Month;
  UINT8   Day;
  UINT8   Hour;
  UINT8   Minute;
  UINT8   Second;
  UINT8   Pad1;
  UINT32  Nanosecond;
  INT16   TimeZone;
  UINT8   Daylight;
  UINT8   Pad2;
} EFI_TIME;

//
// Bit definitions for EFI_TIME.Daylight
//
#define EFI_TIME_ADJUST_DAYLIGHT  0x01
#define EFI_TIME_IN_DAYLIGHT      0x02

//
// Value definition for EFI_TIME.TimeZone
//
#define EFI_UNSPECIFIED_TIMEZONE  0x07FF

//
// Networking
//
typedef struct {
  UINT8 Addr[4];
} EFI_IPv4_ADDRESS;

typedef struct {
  UINT8 Addr[16];
} EFI_IPv6_ADDRESS;

typedef struct {
  UINT8 Addr[32];
} EFI_MAC_ADDRESS;

typedef union {
  UINT32            Addr[4];
  EFI_IPv4_ADDRESS  v4;
  EFI_IPv6_ADDRESS  v6;
} EFI_IP_ADDRESS;

typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef enum {
  AllocateAnyPages,
  AllocateMaxAddress,
  AllocateAddress,
  MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef struct {
  UINT64  Signature;
  UINT32  Revision;
  UINT32  HeaderSize;
  UINT32  CRC32;
  UINT32  Reserved;
} EFI_TABLE_HEADER;

//
// possible caching types for the memory range
//
#define EFI_MEMORY_UC   0x0000000000000001
#define EFI_MEMORY_WC   0x0000000000000002
#define EFI_MEMORY_WT   0x0000000000000004
#define EFI_MEMORY_WB   0x0000000000000008
#define EFI_MEMORY_UCE  0x0000000000000010

//
// physical memory protection on range
//
#define EFI_MEMORY_WP 0x0000000000001000
#define EFI_MEMORY_RP 0x0000000000002000
#define EFI_MEMORY_XP 0x0000000000004000

//
// range requires a runtime mapping
//
#define EFI_MEMORY_RUNTIME  0x8000000000000000

typedef UINT64  EFI_PHYSICAL_ADDRESS;
typedef UINT64  EFI_VIRTUAL_ADDRESS;

#define EFI_MEMORY_DESCRIPTOR_VERSION 1
typedef struct {
  UINT32                Type;
  UINT32                Pad;
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  EFI_VIRTUAL_ADDRESS   VirtualStart;
  UINT64                NumberOfPages;
  UINT64                Attribute;
} EFI_MEMORY_DESCRIPTOR;

//
// The EFI memory allocation functions work in units of EFI_PAGEs that are
// 4K. This should in no way be confused with the page size of the processor.
// An EFI_PAGE is just the quanta of memory in EFI.
//
#define EFI_PAGE_SIZE         4096
#define EFI_PAGE_MASK         0xFFF
#define EFI_PAGE_SHIFT        12

#define EFI_SIZE_TO_PAGES(a)  (((a) >> EFI_PAGE_SHIFT) + (((a) & EFI_PAGE_MASK) ? 1 : 0))

#define EFI_PAGES_TO_SIZE(a)   ( (a) << EFI_PAGE_SHIFT)

//
//  ALIGN_POINTER - aligns a pointer to the lowest boundry
//
#define ALIGN_POINTER(p, s) ((VOID *) (p + ((s - ((UINTN) p)) & (s - 1))))

//
//  ALIGN_VARIABLE - aligns a variable up to the next natural boundry for int size of a processor
//
#define ALIGN_VARIABLE(Value, Adjustment) \
  (UINTN) Adjustment = 0; \
  if ((UINTN) Value % sizeof (UINTN)) { \
    (UINTN) Adjustment = sizeof (UINTN) - ((UINTN) Value % sizeof (UINTN)); \
  } \
  Value = (UINTN) Value + (UINTN) Adjustment

//
//  EFI_FIELD_OFFSET - returns the byte offset to a field within a structure
//
#define EFI_FIELD_OFFSET(TYPE,Field) ((UINTN)(&(((TYPE *) 0)->Field)))

//
//  CONTAINING_RECORD - returns a pointer to the structure
//      from one of it's elements.
//
#define _CR(Record, TYPE, Field)  ((TYPE *) ((CHAR8 *) (Record) - (CHAR8 *) &(((TYPE *) 0)->Field)))

//
// Define macros to build data structure signatures from characters.
//
#define EFI_SIGNATURE_16(A, B)        ((A) | (B << 8))
#define EFI_SIGNATURE_32(A, B, C, D)  (EFI_SIGNATURE_16 (A, B) | (EFI_SIGNATURE_16 (C, D) << 16))
#define EFI_SIGNATURE_64(A, B, C, D, E, F, G, H) \
    (EFI_SIGNATURE_32 (A, B, C, D) | ((UINT64) (EFI_SIGNATURE_32 (E, F, G, H)) << 32))

#endif
