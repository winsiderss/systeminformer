/*
 * Process Hacker Plugins - 
 *   CommonUtil Plugin
 *
 * Copyright (C) 2016 dmex
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
 *
 */

#ifndef _COMMONUTIL_H
#define _COMMONUTIL_H

#define COMMONUTIL_PLUGIN_NAME L"ProcessHacker.CommonUtil"
#define COMMONUTIL_INTERFACE_VERSION 1

typedef PVOID (NTAPI *PUTIL_CREATE_JSON_PARSER)(
    _In_ PSTR JsonString
    );

typedef VOID (NTAPI *PUTIL_CLEANUP_JSON_PARSER)(
    _In_ PVOID Object
    );

typedef PSTR (NTAPI *PUTIL_GET_JSON_VALUE_STRING)(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

typedef INT64 (NTAPI *PUTIL_GET_JSON_VALUE_INT64)(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

typedef PVOID (NTAPI *PUTIL_CREATE_JSON_OBJECT)(
    VOID
    );

typedef PVOID (NTAPI *PUTIL_GET_JSON_OBJECT)(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

typedef INT (NTAPI *PUTIL_GET_JSON_OBJECT_LENGTH)(
    _In_ PVOID Object
    );

typedef BOOL (NTAPI *PUTIL_GET_JSON_OBJECT_BOOL)(
    _In_ PVOID Object,
    _In_ PSTR Key
    );

typedef VOID (NTAPI *PUTIL_ADD_JSON_OBJECT_VALUE)(
    _In_ PVOID Object,
    _In_ PVOID Entry
    );

typedef PVOID (NTAPI *PUTIL_CREATE_JSON_ARRAY)(
    VOID
    );

typedef VOID (NTAPI *PUTIL_ADD_JSON_ARRAY_VALUE)(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ PSTR Value
    );

typedef PSTR (NTAPI *PUTIL_GET_JSON_ARRAY_STRING)(
    _In_ PVOID Object
    );

typedef INT64 (NTAPI *PUTIL_GET_JSON_OBJECT_ARRAY_ULONG)(
    _In_ PVOID Object,
    _In_ INT Index
    );

typedef INT (NTAPI *PUTIL_GET_JSON_ARRAY_LENGTH)(
    _In_ PVOID Object
    );

typedef PVOID (NTAPI *PUTIL_GET_JSON_OBJECT_ARRAY_INDEX)(
    _In_ PVOID Object,
    _In_ INT Index
    );

typedef struct _JSON_ARRAY_LIST_OBJECT
{
    PSTR Key;
    PVOID Entry;
} JSON_ARRAY_LIST_OBJECT, *PJSON_ARRAY_LIST_OBJECT;

typedef PPH_LIST (NTAPI *PUTIL_GET_JSON_OBJECT_ARRAY_LIST)(
    _In_ PVOID Object
    );

typedef struct _COMMONUTIL_INTERFACE
{
    ULONG Version;
    PUTIL_CREATE_JSON_PARSER CreateJsonParser;
    PUTIL_CLEANUP_JSON_PARSER CleanupJsonParser;
    PUTIL_GET_JSON_VALUE_STRING GetJsonValueAsString;
    PUTIL_GET_JSON_VALUE_INT64 GetJsonValueAsUlong;
    PUTIL_GET_JSON_OBJECT_BOOL GetJsonValueAsBool;
    PUTIL_CREATE_JSON_OBJECT CreateJsonObject;
    PUTIL_GET_JSON_OBJECT GetJsonObject;
    PUTIL_GET_JSON_OBJECT_LENGTH GetJsonObjectLength;
    PUTIL_ADD_JSON_ARRAY_VALUE JsonAddObject;
    PUTIL_CREATE_JSON_ARRAY CreateJsonArray;
    PUTIL_ADD_JSON_OBJECT_VALUE JsonArrayAddObject;
    PUTIL_GET_JSON_ARRAY_STRING GetJsonArrayString;
    PUTIL_GET_JSON_OBJECT_ARRAY_ULONG GetJsonArrayUlong;
    PUTIL_GET_JSON_ARRAY_LENGTH JsonGetArrayLength;
    PUTIL_GET_JSON_OBJECT_ARRAY_INDEX JsonGetObjectArrayIndex;
    PUTIL_GET_JSON_OBJECT_ARRAY_LIST JsonGetObjectArrayList;
} COMMONUTIL_INTERFACE, *P_COMMONUTIL_INTERFACE;

FORCEINLINE
PVOID
CreateJsonParser(
    _In_ PSTR JsonString
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->CreateJsonParser(JsonString);
            }
        }
    }

    return NULL;
}

FORCEINLINE
VOID 
CleanupJsonParser(
    _In_ PVOID Object
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                Interface->CleanupJsonParser(Object);
            }
        }
    }
}

FORCEINLINE
PSTR 
GetJsonValueAsString(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->GetJsonValueAsString(Object, Key);
            }
        }
    }

    return NULL;
}

FORCEINLINE
INT64
GetJsonValueAsUlong(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->GetJsonValueAsUlong(Object, Key);
            }
        }
    }

    return 0;
}

FORCEINLINE
BOOL
GetJsonValueAsBool(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->GetJsonValueAsBool(Object, Key);
            }
        }
    }

    return FALSE;
}

FORCEINLINE
PVOID 
CreateJsonArray(
    VOID
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->CreateJsonArray();
            }
        }
    }

    return NULL;
}

FORCEINLINE
PSTR 
GetJsonArrayString(
    _In_ PVOID Object
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->GetJsonArrayString(Object);
            }
        }
    }

    return NULL;
}

FORCEINLINE
INT64
GetJsonArrayUlong(
    _In_ PVOID Object,
    _In_ INT Index
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->GetJsonArrayUlong(Object, Index);
            }
        }
    }

    return 0;
}

FORCEINLINE
INT 
JsonGetArrayLength(
    _In_ PVOID Object
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->JsonGetArrayLength(Object);
            }
        }
    }

    return 0;
}

FORCEINLINE
PVOID
JsonGetObjectArrayIndex(
    _In_ PVOID Object,
    _In_ INT Index
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->JsonGetObjectArrayIndex(Object, Index);
            }
        }
    }

    return NULL;
}

FORCEINLINE
PPH_LIST
JsonGetObjectArrayList(
    _In_ PVOID Object
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->JsonGetObjectArrayList(Object);
            }
        }
    }

    return NULL;
}

FORCEINLINE
PVOID 
CreateJsonObject(
    VOID
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->CreateJsonObject();
            }
        }
    }

    return NULL;
}

FORCEINLINE
PVOID 
JsonGetObject(
    _In_ PVOID Object,
    _In_ PSTR Key
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                return Interface->GetJsonObject(Object, Key);
            }
        }
    }

    return NULL;
}

FORCEINLINE
VOID
JsonArrayAddObject(
    _In_ PVOID Object,
    _In_ PVOID Entry
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                Interface->JsonArrayAddObject(Object, Entry);
            }
        }
    }
}

FORCEINLINE
VOID
JsonAddObject(
    _In_ PVOID Object,
    _In_ PSTR Key,
    _In_ PSTR Value
    )
{
    PPH_PLUGIN toolStatusPlugin;

    if (toolStatusPlugin = PhFindPlugin(COMMONUTIL_PLUGIN_NAME))
    {
        P_COMMONUTIL_INTERFACE Interface;

        if (Interface = PhGetPluginInformation(toolStatusPlugin)->Interface)
        {
            if (Interface->Version <= COMMONUTIL_INTERFACE_VERSION)
            {
                Interface->JsonAddObject(Object, Key, Value);
            }
        }
    }
}

FORCEINLINE
HICON
CommonBitmapToIcon(
    _In_ HBITMAP BitmapHandle,
    _In_ INT Width,
    _In_ INT Height
    )
{
    HICON icon;
    HDC screenDc;
    HBITMAP screenBitmap;
    ICONINFO iconInfo = { 0 };

    screenDc = CreateIC(L"DISPLAY", NULL, NULL, NULL);
    screenBitmap = CreateCompatibleBitmap(screenDc, Width, Height);

    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = BitmapHandle;
    iconInfo.hbmMask = screenBitmap;

    icon = CreateIconIndirect(&iconInfo);

    DeleteObject(screenBitmap);
    DeleteDC(screenDc);

    return icon;
}

/**
 * Creates a Ansi string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ArgPtr A pointer to the list of arguments.
 */
FORCEINLINE
PPH_BYTES 
FormatAnsiString_V(
    _In_ _Printf_format_string_ PSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_BYTES string;
    int length;

    length = _vscprintf(Format, ArgPtr);

    if (length == -1)
        return NULL;

    string = PhCreateBytesEx(NULL, length * sizeof(CHAR));

    _vsnprintf(
        string->Buffer,
        length,
        Format, ArgPtr
        );

    return string;
}

/**
 * Creates a Ansi string using format specifiers.
 *
 * \param Format The format-control string.
 */
FORCEINLINE
PPH_BYTES 
FormatAnsiString(
    _In_ _Printf_format_string_ PSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return FormatAnsiString_V(Format, argptr);
}

FORCEINLINE
HFONT
CommonDuplicateFont(
    _In_ HFONT Font
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
        return CreateFontIndirect(&logFont);
    else
        return NULL;
}

#endif