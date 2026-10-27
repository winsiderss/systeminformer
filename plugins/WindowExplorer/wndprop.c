/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2018-2026
 *
 */

#include "wndexp.h"

// Internal callback context for property enumeration
typedef struct _WE_PROPS_ENUM_CONTEXT
{
    WE_PROPSENUMPROC Callback;
    PVOID Context;
    NTSTATUS Status;
} WE_PROPS_ENUM_CONTEXT, *PWE_PROPS_ENUM_CONTEXT;

// Internal enumeration callback used by EnumPropsEx
BOOL CALLBACK WeInternalEnumPropsExCallback(
    _In_ HWND WindowHandle,
    _In_ PWSTR Name,
    _In_ HANDLE Value,
    _In_ ULONG_PTR Context
    );

// Uxtheme atom name mapping
static struct {
    WE_UXTHEME_ATOM Atom;
    PCWSTR Name;
    PCWSTR Description;
} WeUxthemeAtomMap[] =
{
    { WeUxthemeAtomThemeWnd, L"THEMEWND", L"Theme window object" },
    { WeUxthemeAtomThemeMetrics, L"THEMEMETRICS", L"Theme metrics object" },
    { WeUxthemeAtomCShakeWnd, L"CSHAKEWND", L"CShakeWnd object" },
    { WeUxthemeAtomCThemeWnd, L"CTHEMEWND", L"CThemeWnd object" },
    { WeUxthemeAtomThemeProps, L"THEMEPROPS", L"Theme properties object" },
};

/**
 * Determines if a property name is an atom reference.
 *
 * \param PropertyNameOrAtom The property name or atom string.
 * \param AtomValue Optional pointer to receive the atom value.
 *
 * \return TRUE if the property is an atom, FALSE otherwise.
 */
BOOLEAN WeIsPropertyAtom(
    _In_ PCWSTR PropertyNameOrAtom,
    _Out_opt_ PUSHORT AtomValue
    )
{
    SIZE_T length;
    PWSTR endPtr;
    ULONG value;

    if (!PropertyNameOrAtom)
        return FALSE;

    // Check if it's already a numeric atom pointer
    // Windows atoms are in the range 0xC000-0xFFFF for global atoms
    // Local atoms are 0x0001-0xBFFF, but IS_INTRESOURCE checks for high-bit set
    if (IS_INTRESOURCE(PropertyNameOrAtom))
    {
        USHORT atom = PtrToUshort(PropertyNameOrAtom);
        
        if (AtomValue)
            *AtomValue = atom;
        
        return TRUE;
    }

    // Check if it's in "#123" format
    if (PropertyNameOrAtom[0] != L'#')
        return FALSE;

    length = PhCountStringZ(PropertyNameOrAtom);
    if (length <= 1 || length > 7) // # + up to 5 digits + null
        return FALSE;

    value = wcstoul(PropertyNameOrAtom + 1, &endPtr, 10);
    
    if (*endPtr != UNICODE_NULL || value > USHORT_MAX)
        return FALSE;

    if (AtomValue)
        *AtomValue = (USHORT)value;

    return TRUE;
}

/**
 * Formats an atom value as a string.
 *
 * \param AtomValue The atom value to format.
 *
 * \return A formatted string in "#123" format.
 */
PPH_STRING WeFormatAtomString(
    _In_ USHORT AtomValue
    )
{
    return PhFormatString(L"#%hu", AtomValue);
}

/**
 * Gets the symbolic name for a uxtheme atom.
 *
 * \param Atom The uxtheme atom value.
 *
 * \return The symbolic name of the atom, or NULL if not found.
 */
PPH_STRING WeGetUxthemeAtomName(
    _In_ WE_UXTHEME_ATOM Atom
    )
{
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(WeUxthemeAtomMap); i++)
    {
        if (WeUxthemeAtomMap[i].Atom == Atom)
            return PhCreateString(WeUxthemeAtomMap[i].Name);
    }

    return NULL;
}

/**
 * Retrieves a window property value.
 *
 * \param WindowHandle Handle to the window.
 * \param PropertyNameOrAtom Property name or atom string.
 *
 * \return The property value, or NULL if the property does not exist.
 */
HANDLE WeGetWindowProperty(
    _In_ HWND WindowHandle,
    _In_ PCWSTR PropertyNameOrAtom
    )
{
    USHORT atomValue;
    PCWSTR propertyName = PropertyNameOrAtom;

    if (!PropertyNameOrAtom)
        return NULL;

    if (!IsWindow(WindowHandle))
        return NULL;

    // Check if it's an atom reference
    if (WeIsPropertyAtom(PropertyNameOrAtom, &atomValue))
    {
        // Convert atom value to atom pointer
        propertyName = MAKEINTRESOURCEW(atomValue);
    }

    return GetProp(WindowHandle, propertyName);
}

/**
 * Sets a window property value.
 *
 * \param WindowHandle Handle to the window.
 * \param PropertyNameOrAtom Property name or atom string.
 * \param PropertyValue The property value to set.
 *
 * \return TRUE if successful, FALSE otherwise.
 */
BOOL WeSetWindowProperty(
    _In_ HWND WindowHandle,
    _In_ PCWSTR PropertyNameOrAtom,
    _In_ HANDLE PropertyValue
    )
{
    USHORT atomValue;
    PCWSTR propertyName = PropertyNameOrAtom;

    if (!PropertyNameOrAtom)
        return FALSE;

    if (!IsWindow(WindowHandle))
        return FALSE;

    // Check if it's an atom reference
    if (WeIsPropertyAtom(PropertyNameOrAtom, &atomValue))
    {
        // Convert atom value to atom pointer
        propertyName = MAKEINTRESOURCEW(atomValue);
    }

    return SetProp(WindowHandle, propertyName, PropertyValue);
}

/**
 * Removes a window property.
 *
 * \param WindowHandle Handle to the window.
 * \param PropertyNameOrAtom Property name or atom string.
 *
 * \return The property value that was removed, or NULL if the property did not exist.
 */
HANDLE WeRemoveWindowProperty(
    _In_ HWND WindowHandle,
    _In_ PCWSTR PropertyNameOrAtom
    )
{
    USHORT atomValue;
    PCWSTR propertyName = PropertyNameOrAtom;
    HANDLE result;

    if (!PropertyNameOrAtom)
        return NULL;

    if (!IsWindow(WindowHandle))
        return NULL;

    // Check if it's an atom reference
    if (WeIsPropertyAtom(PropertyNameOrAtom, &atomValue))
    {
        // Convert atom value to atom pointer
        propertyName = MAKEINTRESOURCEW(atomValue);
    }

    result = RemoveProp(WindowHandle, propertyName);
    
    // RemoveProp returns the property value or NULL on error
    // We need to check GetLastError to distinguish between "not found" and "error"
    if (!result && GetLastError() != ERROR_SUCCESS)
    {
        // Actual error occurred
        return NULL;
    }

    return result;
}

/**
 * Retrieves detailed information about a window property.
 *
 * \param WindowHandle Handle to the window.
 * \param PropertyNameOrAtom Property name or atom string.
 * \param PropertyName Optional pointer to receive the property name.
 * \param PropertyValue Optional pointer to receive the property value.
 * \param IsAtom Optional pointer to receive whether the property is an atom.
 * \param AtomValue Optional pointer to receive the atom value.
 *
 * \return STATUS_SUCCESS on success, or an error code on failure.
 */
NTSTATUS WeGetWindowPropertyInfo(
    _In_ HWND WindowHandle,
    _In_ PCWSTR PropertyNameOrAtom,
    _Out_opt_ PPH_STRING *PropertyName,
    _Out_opt_ PHANDLE PropertyValue,
    _Out_opt_ PBOOLEAN IsAtom,
    _Out_opt_ PUSHORT AtomValue
    )
{
    USHORT atomValue = 0;
    BOOLEAN isAtom = FALSE;
    PCWSTR propertyName = PropertyNameOrAtom;
    HANDLE value;

    if (!PropertyNameOrAtom)
        return STATUS_INVALID_PARAMETER;

    if (!IsWindow(WindowHandle))
        return STATUS_INVALID_HANDLE;

    // Determine if it's an atom
    isAtom = WeIsPropertyAtom(PropertyNameOrAtom, &atomValue);
    
    if (isAtom)
    {
        // Convert atom value to atom pointer
        propertyName = MAKEINTRESOURCEW(atomValue);
    }

    // Get the property value
    value = GetProp(WindowHandle, propertyName);
    
    if (!value && GetLastError() != ERROR_SUCCESS)
        return PhGetLastWin32ErrorAsNtStatus();

    // Return the results
    if (PropertyName)
    {
        if (isAtom)
            *PropertyName = WeFormatAtomString(atomValue);
        else
            *PropertyName = PhCreateString(PropertyNameOrAtom);
    }

    if (PropertyValue)
        *PropertyValue = value;

    if (IsAtom)
        *IsAtom = isAtom;

    if (AtomValue)
        *AtomValue = atomValue;

    return STATUS_SUCCESS;
}

/**
 * Internal callback function for EnumPropsEx.
 */
BOOL CALLBACK WeInternalEnumPropsExCallback(
    _In_ HWND WindowHandle,
    _In_ PWSTR Name,
    _In_ HANDLE Value,
    _In_ ULONG_PTR Context
    )
{
    PWE_PROPS_ENUM_CONTEXT enumContext = (PWE_PROPS_ENUM_CONTEXT)Context;
    BOOLEAN isAtom;
    USHORT atomValue = 0;
    if (!enumContext || !enumContext->Callback)
        return FALSE;

    if (!NT_SUCCESS(enumContext->Status))
        return FALSE;

    // Determine if it's an atom
    if (IS_INTRESOURCE(Name))
    {
        isAtom = TRUE;
        atomValue = PtrToUshort(Name);
    }
    else
    {
        isAtom = FALSE;
    }

    // Call the user's callback
    if (!enumContext->Callback(WindowHandle, Name, Value, isAtom, atomValue, enumContext->Context))
    {
        enumContext->Status = STATUS_CANCELLED;
        return FALSE;
    }

    return TRUE;
}

/**
 * Enumerates all properties of a window.
 *
 * \param WindowHandle Handle to the window.
 * \param Callback Callback function called for each property.
 * \param Context User-defined context passed to the callback.
 *
 * \return STATUS_SUCCESS on success, or an error code on failure.
 */
NTSTATUS WeEnumWindowProperties(
    _In_ HWND WindowHandle,
    _In_ WE_PROPSENUMPROC Callback,
    _In_opt_ PVOID Context
    )
{
    WE_PROPS_ENUM_CONTEXT enumContext;

    if (!IsWindow(WindowHandle))
        return STATUS_INVALID_HANDLE;

    if (!Callback)
        return STATUS_INVALID_PARAMETER;

    enumContext.Callback = Callback;
    enumContext.Context = Context;
    enumContext.Status = STATUS_SUCCESS;

    EnumPropsEx(WindowHandle, WeInternalEnumPropsExCallback, (LPARAM)&enumContext);

    return enumContext.Status;
}

/**
 * Retrieves a uxtheme-specific property.
 *
 * \param WindowHandle Handle to the window.
 * \param Atom The uxtheme atom to retrieve.
 * \param ObjectType Optional pointer to receive the object type description.
 * \param PropertyValue Optional pointer to receive the property value.
 *
 * \return STATUS_SUCCESS on success, or an error code on failure.
 */
NTSTATUS WeGetUxthemeProperty(
    _In_ HWND WindowHandle,
    _In_ WE_UXTHEME_ATOM Atom,
    _Out_opt_ PPH_STRING *ObjectType,
    _Out_opt_ PHANDLE PropertyValue
    )
{
    HANDLE value;
    ULONG i;

    if (!IsWindow(WindowHandle))
        return STATUS_INVALID_HANDLE;

    // Check if atom is in valid uxtheme range
    if (Atom < WeUxthemeAtomMin || Atom > WeUxthemeAtomMax)
        return STATUS_INVALID_PARAMETER;

    // Get the property value
    value = GetProp(WindowHandle, MAKEINTRESOURCEW(Atom));
    
    if (!value && GetLastError() != ERROR_SUCCESS)
        return PhGetLastWin32ErrorAsNtStatus();

    // Return object type if requested
    if (ObjectType)
    {
        for (i = 0; i < RTL_NUMBER_OF(WeUxthemeAtomMap); i++)
        {
            if (WeUxthemeAtomMap[i].Atom == Atom)
            {
                *ObjectType = PhCreateString(WeUxthemeAtomMap[i].Description);
                break;
            }
        }
        
        if (!*ObjectType)
            *ObjectType = PhCreateString(L"Unknown uxtheme object");
    }

    if (PropertyValue)
        *PropertyValue = value;

    return STATUS_SUCCESS;
}

/**
 * Checks if a window has uxtheme properties.
 *
 * \param WindowHandle Handle to the window.
 *
 * \return TRUE if the window has uxtheme properties, FALSE otherwise.
 */
BOOLEAN WeWindowHasUxthemeProperties(
    _In_ HWND WindowHandle
    )
{
    WE_UXTHEME_ATOM atom;
    HANDLE value;

    if (!IsWindow(WindowHandle))
        return FALSE;

    // Check for any uxtheme atom
    for (atom = WeUxthemeAtomMin; atom <= WeUxthemeAtomMax; atom++)
    {
        value = GetProp(WindowHandle, MAKEINTRESOURCEW(atom));
        if (value)
            return TRUE;
    }

    return FALSE;
}

/**
 * Callback for collecting property information during analysis.
 */
BOOL CALLBACK WeAnalyzePropertiesCallback(
    _In_ HWND WindowHandle,
    _In_ PCWSTR PropertyName,
    _In_ HANDLE PropertyValue,
    _In_ BOOLEAN IsAtom,
    _In_ USHORT AtomValue,
    _In_opt_ PVOID Context
    )
{
    PWE_PROPERTY_INFO propertyInfo;
    PPH_LIST propertyList = (PPH_LIST)Context;

    if (!propertyList)
        return FALSE;

    propertyInfo = PhAllocate(sizeof(WE_PROPERTY_INFO));
    if (!propertyInfo)
        return FALSE;

    memset(propertyInfo, 0, sizeof(WE_PROPERTY_INFO));

    if (IsAtom)
    {
        propertyInfo->Name = WeFormatAtomString(AtomValue);
        propertyInfo->IsAtom = TRUE;
        propertyInfo->AtomValue = AtomValue;
    }
    else
    {
        propertyInfo->Name = PhCreateString(PropertyName);
        propertyInfo->IsAtom = FALSE;
    }

    propertyInfo->Value = PropertyValue;

    PhAddItemList(propertyList, propertyInfo);

    return TRUE;
}

/**
 * Analyzes all properties of a window.
 *
 * \param WindowHandle Handle to the window.
 * \param PropertyList Pointer to receive the list of property information.
 * \param UxthemePropertyCount Optional pointer to receive count of uxtheme properties.
 * \param StringPropertyCount Optional pointer to receive count of string properties.
 * \param AtomPropertyCount Optional pointer to receive count of atom properties.
 *
 * \return STATUS_SUCCESS on success, or an error code on failure.
 */
NTSTATUS WeAnalyzeWindowProperties(
    _In_ HWND WindowHandle,
    _Out_ PPH_LIST *PropertyList,
    _Out_opt_ PULONG UxthemePropertyCount,
    _Out_opt_ PULONG StringPropertyCount,
    _Out_opt_ PULONG AtomPropertyCount
    )
{
    NTSTATUS status;
    PPH_LIST propertyList;
    ULONG i;
    PWE_PROPERTY_INFO propertyInfo;
    ULONG uxthemeCount = 0;
    ULONG stringCount = 0;
    ULONG atomCount = 0;

    if (!IsWindow(WindowHandle))
        return STATUS_INVALID_HANDLE;

    if (!PropertyList)
        return STATUS_INVALID_PARAMETER;

    propertyList = PhCreateList(10);
    if (!propertyList)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = WeEnumWindowProperties(WindowHandle, WeAnalyzePropertiesCallback, propertyList);
    
    if (!NT_SUCCESS(status))
    {
        // Clean up any allocated items
        for (i = 0; i < propertyList->Count; i++)
        {
            propertyInfo = propertyList->Items[i];
            if (propertyInfo->Name)
                PhDereferenceObject(propertyInfo->Name);
            PhFree(propertyInfo);
        }
        
        PhDereferenceObject(propertyList);
        return status;
    }

    // Analyze the collected properties
    for (i = 0; i < propertyList->Count; i++)
    {
        propertyInfo = propertyList->Items[i];
        
        if (propertyInfo->IsAtom)
        {
            atomCount++;
            
            // Check if it's a uxtheme atom
            if (propertyInfo->AtomValue >= WeUxthemeAtomMin && 
                propertyInfo->AtomValue <= WeUxthemeAtomMax)
            {
                uxthemeCount++;
            }
        }
        else
        {
            stringCount++;
        }
    }

    if (UxthemePropertyCount)
        *UxthemePropertyCount = uxthemeCount;
    
    if (StringPropertyCount)
        *StringPropertyCount = stringCount;
    
    if (AtomPropertyCount)
        *AtomPropertyCount = atomCount;

    *PropertyList = propertyList;

    return STATUS_SUCCESS;
}

/**
 * Calculates property statistics for a window.
 *
 * \param WindowHandle Handle to the window.
 * \param Statistics Pointer to receive the property statistics.
 *
 * \return STATUS_SUCCESS on success, or an error code on failure.
 */
NTSTATUS WeGetWindowPropertyStatistics(
    _In_ HWND WindowHandle,
    _Out_ PWE_PROPERTY_STATISTICS Statistics
    )
{
    NTSTATUS status;
    PPH_LIST propertyList = NULL;
    ULONG uxthemeCount = 0;
    ULONG stringCount = 0;
    ULONG atomCount = 0;
    ULONG i;
    PWE_PROPERTY_INFO propertyInfo;
    SIZE_T totalMemory = 0;
    ULONG maxNameLength = 0;

    if (!IsWindow(WindowHandle))
        return STATUS_INVALID_HANDLE;

    if (!Statistics)
        return STATUS_INVALID_PARAMETER;

    memset(Statistics, 0, sizeof(WE_PROPERTY_STATISTICS));

    status = WeAnalyzeWindowProperties(
        WindowHandle,
        &propertyList,
        &uxthemeCount,
        &stringCount,
        &atomCount
        );

    if (!NT_SUCCESS(status))
        return status;

    // Calculate statistics
    for (i = 0; i < propertyList->Count; i++)
    {
        propertyInfo = propertyList->Items[i];
        
        // Estimate memory usage
        if (propertyInfo->Name)
        {
            SIZE_T nameLength = propertyInfo->Name->Length / sizeof(WCHAR);
            totalMemory += sizeof(WE_PROPERTY_INFO);
            totalMemory += propertyInfo->Name->Length;
            
            if (nameLength > maxNameLength)
                maxNameLength = (ULONG)nameLength;
        }
        
        // Value is just a handle, minimal memory impact
        totalMemory += sizeof(HANDLE);
    }

    Statistics->TotalProperties = propertyList->Count;
    Statistics->StringProperties = stringCount;
    Statistics->AtomProperties = atomCount;
    Statistics->UxthemeProperties = uxthemeCount;
    Statistics->TotalMemoryUsed = totalMemory;
    Statistics->MaxPropertyNameLength = maxNameLength;

    // Clean up
    for (i = 0; i < propertyList->Count; i++)
    {
        propertyInfo = propertyList->Items[i];
        if (propertyInfo->Name)
            PhDereferenceObject(propertyInfo->Name);
        PhFree(propertyInfo);
    }
    
    PhDereferenceObject(propertyList);

    return STATUS_SUCCESS;
}
