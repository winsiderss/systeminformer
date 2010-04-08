/*
 * Process Hacker Driver - 
 *   system service logging (data)
 * 
 * Copyright (C) 2009 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SYSSERVICEDATA_H
#define _SYSSERVICEDATA_H

#include "kph.h"

#define TAG_CALL_ENTRY ('cShP')

typedef enum _KPHSS_ARGUMENT_TYPE
{
    /* Having argument info for out variables is very rare 
     * because usually the caller does not fill in anything 
     * in the variable. In some cases, however, the caller 
     * does specify a length (usually Length, or MaximumLength).
     * 
     * Note that with the exception of a few types such as 
     * HANDLE, all types listed here are POINTER TYPES 
     * (although a handle is the size of a pointer). This 
     * is because non-pointer arguments are already recorded 
     * in the event block.
     */
    
    /* Anything passed by value */
    NormalArgument = 0,
    
    /* PBOOLEAN */
    Int8Argument,
    /* P(U)SHORT */
    Int16Argument,
    /* P(U)LONG */
    Int32Argument,
    /* P(U)LARGE_INTEGER */
    Int64Argument,
    /* HANDLE */
    /* Only object manager handles, no fake handles. */
    HandleArgument,
    /* PSTR */
    StringArgument,
    /* PWSTR */
    WStringArgument,
    /* PANSI_STRING */
    AnsiStringArgument,
    /* PUNICODE_STRING */
    UnicodeStringArgument,
    /* POBJECT_ATTRIBUTES */
    ObjectAttributesArgument,
    /* PCLIENT_ID */
    ClientIdArgument,
    /* PCONTEXT */
    ContextArgument,
    /* PINITIAL_TEB */
    InitialTebArgument,
    /* PGUID */
    GuidArgument,
    /* PVOID */
    BytesArgument
} KPHSS_ARGUMENT_TYPE;

typedef struct _KPHSS_HANDLE
{
    CLIENT_ID ClientId;
    USHORT TypeNameOffset; /* KPHSS_WSTRING */
    USHORT NameOffset; /* KPHSS_WSTRING */
} KPHSS_HANDLE, *PKPHSS_HANDLE;

typedef struct _KPHSS_STRING
{
    USHORT Length;
    CHAR Buffer[1];
} KPHSS_STRING, *PKPHSS_STRING;

typedef struct _KPHSS_WSTRING
{
    USHORT Length;
    WCHAR Buffer[1];
} KPHSS_WSTRING, *PKPHSS_WSTRING;

typedef struct _KPHSS_ANSI_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PSTR Pointer;
    CHAR Buffer[1];
} KPHSS_ANSI_STRING, *PKPHSS_ANSI_STRING;

typedef struct _KPHSS_UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Pointer;
    WCHAR Buffer[1];
} KPHSS_UNICODE_STRING, *PKPHSS_UNICODE_STRING;

typedef struct _KPHSS_OBJECT_ATTRIBUTES
{
    union
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        struct
        {
            ULONG Length;
            HANDLE RootDirectory;
            PUNICODE_STRING ObjectName;
            ULONG Attributes;
            PVOID SecurityDescriptor;
            PVOID SecurityQualityOfService;
        };
    };
    
    USHORT RootDirectoryOffset; /* KPHSS_HANDLE */
    USHORT ObjectNameOffset; /* KPHSS_UNICODE_STRING */
} KPHSS_OBJECT_ATTRIBUTES, *PKPHSS_OBJECT_ATTRIBUTES;

typedef struct _KPHSS_INITIAL_TEB
{
    struct
    {
        PVOID OldStackBase;
        PVOID OldStackLimit;
    } OldInitialTeb;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID StackAllocationBase;
} KPHSS_INITIAL_TEB, *PKPHSS_INITIAL_TEB;

typedef struct _KPHSS_BYTES
{
    USHORT Length;
    CHAR Buffer[1];
} KPHSS_BYTES, *PKPHSS_BYTES;

#ifndef _SYSSERVICEDATA_PRIVATE
extern RTL_GENERIC_TABLE KphSsCallTable;
#endif

#define KPHSS_MAXIMUM_ARGUMENT_BLOCKS 20

typedef struct _KPHSS_CALL_ENTRY
{
    PULONG Number;
    PSTR Name;
    ULONG NumberOfArguments;
    KPHSS_ARGUMENT_TYPE Arguments[KPHSS_MAXIMUM_ARGUMENT_BLOCKS];
} KPHSS_CALL_ENTRY, *PKPHSS_CALL_ENTRY;

VOID KphSsDataInit();
VOID KphSsDataDeinit();

PKPHSS_CALL_ENTRY KphSsLookupCallEntry(
    __in ULONG Number
    );

#endif