/*
 * Process Hacker .NET Tools -
 *   performance counter support functions
 *
 * Copyright (C) 2011 wj32
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

#include "dn.h"

BOOLEAN QueryPerfInfoVariableSize(
    __in HKEY Key,
    __in PWSTR ValueName,
    __out PVOID *Data,
    __out_opt PULONG DataSize
    )
{
    LONG result;
    PVOID data;
    ULONG dataSize;
    ULONG bufferSize;

    dataSize = 0x800;
    data = PhAllocate(dataSize);

    while (TRUE)
    {
        bufferSize = dataSize;
        result = RegQueryValueEx(Key, ValueName, NULL, NULL, data, &bufferSize);

        if (result == ERROR_MORE_DATA)
        {
            PhFree(data);
            dataSize += 4096 * 4;
            data = PhAllocate(dataSize);
        }
        else
        {
            break;
        }
    }

    if (result == ERROR_SUCCESS)
    {
        *Data = data;

        if (DataSize)
            *DataSize = dataSize;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

PWSTR FindPerfTextInTextData(
    __in PVOID TextData,
    __in ULONG Index
    )
{
    PWSTR textData;

    textData = TextData;

    // Skip the first pair.
    textData += wcslen(textData) + 1;
    textData += wcslen(textData) + 1;

    while (*textData)
    {
        ULONG index;

        index = _wtoi(textData);
        textData += wcslen(textData) + 1;

        if (index == Index)
        {
            return textData;
        }

        textData += wcslen(textData) + 1;
    }

    return NULL;
}

ULONG FindPerfIndexInTextData(
    __in PVOID TextData,
    __in PPH_STRINGREF Text
    )
{
    PWSTR textData;

    textData = TextData;

    // Skip the first pair.
    textData += wcslen(textData) + 1;
    textData += wcslen(textData) + 1;

    while (*textData)
    {
        ULONG index;
        PWSTR text;
        SIZE_T length;
        PH_STRINGREF textSr;

        index = _wtoi(textData);
        textData += wcslen(textData) + 1;

        text = textData;
        length = wcslen(textData);
        textData += length + 1;

        textSr.Buffer = text;
        textSr.Length = length * sizeof(WCHAR);

        if (PhEqualStringRef(&textSr, Text, TRUE))
            return index;
    }

    return -1;
}

BOOLEAN GetPerfObjectTypeInfo(
    __in_opt PPH_STRINGREF Filter,
    __out PPERF_OBJECT_TYPE_INFO *Info,
    __out PULONG Count
    )
{
    BOOLEAN result;
    PVOID textData = NULL;
    PVOID data = NULL;
    PPERF_OBJECT_TYPE_INFO info = NULL;
    ULONG infoCount;
    ULONG infoAllocated;
    PPERF_DATA_BLOCK block;
    PPERF_OBJECT_TYPE objectType;
    ULONG i;

    result = FALSE;

    if (!QueryPerfInfoVariableSize(HKEY_PERFORMANCE_DATA, L"Counter 009", &textData, NULL)) // can't support non-English because of filter
        return FALSE;

    if (!QueryPerfInfoVariableSize(HKEY_PERFORMANCE_DATA, L"Global", &data, NULL))
        goto ExitCleanup;

    block = data;

    if (memcmp(block->Signature, L"PERF", sizeof(WCHAR) * 4) != 0)
        goto ExitCleanup;

    objectType = (PPERF_OBJECT_TYPE)((PCHAR)block + block->HeaderLength);
    infoCount = 0;
    infoAllocated = 16;
    info = PhAllocate(sizeof(PERF_OBJECT_TYPE_INFO) * infoAllocated);

    for (i = 0; i < block->NumObjectTypes; i++)
    {
        PWSTR objectTypeName;
        PPERF_OBJECT_TYPE_INFO infoMember;

        objectTypeName = FindPerfTextInTextData(textData, objectType->ObjectNameTitleIndex);

        if (objectTypeName)
        {
            PH_STRINGREF objectTypeNameSr;

            PhInitializeStringRef(&objectTypeNameSr, objectTypeName);

            if (!Filter || PhStartsWithStringRef(&objectTypeNameSr, Filter, TRUE))
            {
                if (infoCount == infoAllocated)
                {
                    infoAllocated *= 2;
                    info = PhReAllocate(info, sizeof(PERF_OBJECT_TYPE_INFO) * infoAllocated);
                }

                infoMember = &info[infoCount++];
                infoMember->NameIndex = objectType->ObjectHelpTitleIndex;
                infoMember->NameBuffer = PhCreateString(objectTypeName);
                infoMember->Name = infoMember->NameBuffer->sr;
            }
        }

        objectType = (PPERF_OBJECT_TYPE)((PCHAR)objectType + objectType->TotalByteLength);
    }

    *Info = info;
    *Count = infoCount;
    result = TRUE;

ExitCleanup:
    if (!result)
    {
        if (info)
            PhFree(info);
    }

    if (data)
        PhFree(data);
    if (textData)
        PhFree(textData);

    return result;
}

BOOLEAN GetPerfObjectTypeInfo2(
    __in PPH_STRINGREF NameList,
    __out PPERF_OBJECT_TYPE_INFO *Info,
    __out PULONG Count,
    __out_opt PVOID *TextData
    )
{
    BOOLEAN result;
    PVOID textData = NULL;
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;
    PPERF_OBJECT_TYPE_INFO info = NULL;
    ULONG infoCount;
    ULONG infoAllocated;

    result = FALSE;

    if (!QueryPerfInfoVariableSize(HKEY_PERFORMANCE_DATA, L"Counter 009", &textData, NULL)) // can't support non-English because of filter
        return FALSE;

    infoCount = 0;
    infoAllocated = 16;
    info = PhAllocate(sizeof(PERF_OBJECT_TYPE_INFO) * infoAllocated);

    remainingPart = *NameList;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, ';', &part, &remainingPart);

        if (part.Length != 0)
        {
            ULONG index;

            index = FindPerfIndexInTextData(textData, &part);

            if (index != -1)
            {
                PPERF_OBJECT_TYPE_INFO infoMember;

                if (infoCount == infoAllocated)
                {
                    infoAllocated *= 2;
                    info = PhReAllocate(info, sizeof(PERF_OBJECT_TYPE_INFO) * infoAllocated);
                }

                infoMember = &info[infoCount++];
                infoMember->NameIndex = index;
                infoMember->Name = part;
                infoMember->NameBuffer = NULL;
            }
        }
    }

    *Info = info;
    *Count = infoCount;
    result = TRUE;

    if (TextData)
    {
        *TextData = textData;
    }
    else
    {
        PhFree(textData);
    }

    return result;
}
