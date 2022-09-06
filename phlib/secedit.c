/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
 *
 */

#include <ph.h>
#include <appresolver.h>
#include <secedit.h>
#include <lsasup.h>

#include <guisup.h>
#include <hndlinfo.h>
#include <settings.h>
#include <seceditp.h>

static ISecurityInformationVtbl PhSecurityInformation_VTable =
{
    PhSecurityInformation_QueryInterface,
    PhSecurityInformation_AddRef,
    PhSecurityInformation_Release,
    PhSecurityInformation_GetObjectInformation,
    PhSecurityInformation_GetSecurity,
    PhSecurityInformation_SetSecurity,
    PhSecurityInformation_GetAccessRights,
    PhSecurityInformation_MapGeneric,
    PhSecurityInformation_GetInheritTypes,
    PhSecurityInformation_PropertySheetPageCallback
};

static ISecurityInformation2Vtbl PhSecurityInformation_VTable2 =
{
    PhSecurityInformation2_QueryInterface,
    PhSecurityInformation2_AddRef,
    PhSecurityInformation2_Release,
    PhSecurityInformation2_IsDaclCanonical,
    PhSecurityInformation2_LookupSids
};

static ISecurityInformation3Vtbl PhSecurityInformation_VTable3 =
{
    PhSecurityInformation3_QueryInterface,
    PhSecurityInformation3_AddRef,
    PhSecurityInformation3_Release,
    PhSecurityInformation3_GetFullResourceName,
    PhSecurityInformation3_OpenElevatedEditor
};

static IDataObjectVtbl PhDataObject_VTable =
{
    PhSecurityDataObject_QueryInterface,
    PhSecurityDataObject_AddRef,
    PhSecurityDataObject_Release,
    PhSecurityDataObject_GetData,
    PhSecurityDataObject_GetDataHere,
    PhSecurityDataObject_QueryGetData,
    PhSecurityDataObject_GetCanonicalFormatEtc,
    PhSecurityDataObject_SetData,
    PhSecurityDataObject_EnumFormatEtc,
    PhSecurityDataObject_DAdvise,
    PhSecurityDataObject_DUnadvise,
    PhSecurityDataObject_EnumDAdvise
};

static ISecurityObjectTypeInfoExVtbl PhSecurityObjectTypeInfo_VTable3 =
{
    PhSecurityObjectTypeInfo_QueryInterface,
    PhSecurityObjectTypeInfo_AddRef,
    PhSecurityObjectTypeInfo_Release,
    PhSecurityObjectTypeInfo_GetInheritSource
};

static IEffectivePermissionVtbl PhEffectivePermission_VTable =
{
    PhEffectivePermission_QueryInterface,
    PhEffectivePermission_AddRef,
    PhEffectivePermission_Release,
    PhEffectivePermission_GetEffectivePermission
};

/**
 * Creates a security editor page.
 *
 * \param ObjectName The name of the object.
 * \param ObjectType The type name of the object.
 * \param Context A user-defined value to pass to the callback functions.
 */
HPROPSHEETPAGE PhCreateSecurityPage(
    _In_ PWSTR ObjectName,
    _In_ PWSTR ObjectType,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    )
{
    ISecurityInformation *info;
    HPROPSHEETPAGE page;

    info = PhSecurityInformation_Create(
        NULL,
        ObjectName,
        ObjectType,
        OpenObject,
        CloseObject,
        Context,
        TRUE
        );

    page = CreateSecurityPage(info);

    PhSecurityInformation_Release(info);

    return page;
}

static NTSTATUS PhpEditSecurityInformationThread(
    _In_ PVOID Context
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)Context;

    // The EditSecurityAdvanced function on Windows 7 doesn't handle the SI_PAGE_TYPE
    // parameter correctly and also doesn't show the Audit and Owner tabs... (dmex)
    if (WindowsVersion >= WINDOWS_8 && PhGetIntegerSetting(L"EnableSecurityAdvancedDialog"))
        EditSecurityAdvanced(this->WindowHandle, Context, COMBINE_PAGE_ACTIVATION(SI_PAGE_PERM, SI_SHOW_PERM_ACTIVATED));
    else
        EditSecurity(this->WindowHandle, Context);

    PhSecurityInformation_Release(Context);

    return STATUS_SUCCESS;
}

/**
 * Displays a security editor dialog.
 *
 * \param hWnd The parent window of the dialog.
 * \param ObjectName The name of the object.
 * \param Context A user-defined value to pass to the callback functions.
 */
VOID PhEditSecurity(
    _In_opt_ HWND WindowHandle,
    _In_ PWSTR ObjectName,
    _In_ PWSTR ObjectType,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context
    )
{
    ISecurityInformation *info;

    info = PhSecurityInformation_Create(
        WindowHandle,
        ObjectName,
        ObjectType,
        OpenObject,
        CloseObject,
        Context,
        FALSE
        );

    PhCreateThread2(PhpEditSecurityInformationThread, info);
}

ISecurityInformation *PhSecurityInformation_Create(
    _In_opt_ HWND WindowHandle,
    _In_ PWSTR ObjectName,
    _In_ PWSTR ObjectType,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN IsPage
    )
{
    PhSecurityInformation *info;
    ULONG i;

    info = PhAllocateZero(sizeof(PhSecurityInformation));
    info->VTable = &PhSecurityInformation_VTable;
    info->RefCount = 1;

    info->WindowHandle = WindowHandle;
    info->ObjectName = PhCreateString(ObjectName);
    info->ObjectType = PhCreateString(ObjectType);
    info->OpenObject = OpenObject;
    info->CloseObject = CloseObject;
    info->Context = Context;  
    info->IsPage = IsPage;

    if (PhGetAccessEntries(ObjectType, &info->AccessEntriesArray, &info->NumberOfAccessEntries))
    {
        info->AccessEntries = PhAllocateZero(sizeof(SI_ACCESS) * info->NumberOfAccessEntries);

        for (i = 0; i < info->NumberOfAccessEntries; i++)
        {
            info->AccessEntries[i].pszName = info->AccessEntriesArray[i].Name;
            info->AccessEntries[i].mask = info->AccessEntriesArray[i].Access;

            if (info->AccessEntriesArray[i].General)
                info->AccessEntries[i].dwFlags |= SI_ACCESS_GENERAL;
            if (info->AccessEntriesArray[i].Specific)
                info->AccessEntries[i].dwFlags |= SI_ACCESS_SPECIFIC;

            if (PhEqualString2(info->ObjectType, L"FileObject", TRUE)) // TODO: Remove PhEqualString2 (dmex)
                info->AccessEntries[i].dwFlags |= OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        }
    }

    return (ISecurityInformation *)info;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_QueryInterface(
    _In_ ISecurityInformation *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_ISecurityInformation)
        )
    {
        PhSecurityInformation_AddRef(This);
        *Object = This;
        return S_OK;
    }
    else if (IsEqualGUID(Riid, &IID_ISecurityInformation2))
    {
        if (WindowsVersion >= WINDOWS_8)
        {
            PhSecurityInformation2 *info;

            info = PhAllocateZero(sizeof(PhSecurityInformation2));
            info->VTable = &PhSecurityInformation_VTable2;
            info->Context = this;
            info->RefCount = 1;

            *Object = info;
            return S_OK;
        }
    }
    else if (IsEqualGUID(Riid, &IID_ISecurityInformation3))
    {
        if (WindowsVersion >= WINDOWS_8)
        {
            PhSecurityInformation3 *info;

            info = PhAllocateZero(sizeof(PhSecurityInformation3));
            info->VTable = &PhSecurityInformation_VTable3;
            info->Context = this;
            info->RefCount = 1;

            *Object = info;
            return S_OK;
        }
    }
    else if (IsEqualGUID(Riid, &IID_ISecurityObjectTypeInfo))
    {
        if (WindowsVersion >= WINDOWS_8)
        {
            PhSecurityObjectTypeInfo* info;

            info = PhAllocateZero(sizeof(PhSecurityObjectTypeInfo));
            info->VTable = &PhSecurityObjectTypeInfo_VTable3;
            info->Context = this;
            info->RefCount = 1;

            *Object = info;
            return S_OK;
        }
    }
    else if (IsEqualGUID(Riid, &IID_IEffectivePermission))
    {
        PhEffectivePermission* info;

        info = PhAllocateZero(sizeof(PhEffectivePermission));
        info->VTable = &PhEffectivePermission_VTable;
        info->Context = this;
        info->RefCount = 1;

        *Object = info;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation_AddRef(
    _In_ ISecurityInformation *This
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation_Release(
    _In_ ISecurityInformation *This
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        if (this->CloseObject)
            this->CloseObject(this->Context);

        if (this->ObjectName) 
            PhDereferenceObject(this->ObjectName);
        if (this->ObjectType)
            PhDereferenceObject(this->ObjectType);
        if (this->AccessEntries) 
            PhFree(this->AccessEntries);
        if (this->AccessEntriesArray)
            PhFree(this->AccessEntriesArray);

        PhFree(this);

        return 0;
    }

    return this->RefCount;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetObjectInformation(
    _In_ ISecurityInformation *This,
    _Out_ PSI_OBJECT_INFO ObjectInfo
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    memset(ObjectInfo, 0, sizeof(SI_OBJECT_INFO));
    ObjectInfo->dwFlags = SI_EDIT_ALL | SI_ADVANCED | (WindowsVersion >= WINDOWS_8 ? SI_VIEW_ONLY : 0) | SI_EDIT_EFFECTIVE;
    ObjectInfo->pszObjectName = PhGetString(this->ObjectName);

    if (PhEqualString2(this->ObjectType, L"FileObject", TRUE))
    {
        ObjectInfo->dwFlags |= SI_ENABLE_EDIT_ATTRIBUTE_CONDITION | SI_MAY_WRITE; // SI_RESET | SI_READONLY
        //if (Folder) ObjectInfo->dwFlags |= SI_CONTAINER;
    }

    if (PhEqualString2(this->ObjectType, L"TokenDefault", TRUE))
    {
        ObjectInfo->dwFlags &= ~(SI_EDIT_OWNER | SI_EDIT_AUDITS);
    }

    if (PhEqualString2(this->ObjectType, L"PowerDefault", TRUE))
    {
        ObjectInfo->dwFlags &= ~SI_EDIT_AUDITS;
        ObjectInfo->dwFlags |= SI_NO_ACL_PROTECT | SI_NO_TREE_APPLY | SI_CONTAINER | SI_OWNER_READONLY;
    }

    if (PhEqualString2(this->ObjectType, L"RdpDefault", TRUE))
    {
        ObjectInfo->dwFlags &= ~SI_EDIT_OWNER;
        ObjectInfo->dwFlags |= SI_NO_ACL_PROTECT | SI_NO_TREE_APPLY;
    }

    if (PhEqualString2(this->ObjectType, L"WmiDefault", TRUE))
    {
        ObjectInfo->dwFlags |= SI_CONTAINER | SI_OWNER_READONLY;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetSecurity(
    _In_ ISecurityInformation *This,
    _In_ SECURITY_INFORMATION RequestedInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ BOOL Default
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;
    NTSTATUS status;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG sdLength;
    PSECURITY_DESCRIPTOR newSd;

    //if (Default)
    //{
    //    securityDescriptor = PhAllocateZero(SECURITY_DESCRIPTOR_MIN_LENGTH);
    //
    //    status = RtlCreateSecurityDescriptor(
    //        securityDescriptor,
    //        SECURITY_DESCRIPTOR_REVISION
    //        );
    //
    //    if (!NT_SUCCESS(status))
    //        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    //
    //    status = RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, NULL, FALSE);
    //
    //    if (!NT_SUCCESS(status))
    //        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    //}
    //else
    {
        status = PhStdGetObjectSecurity(
            &securityDescriptor,
            RequestedInformation,
            this
            );

        if (!NT_SUCCESS(status))
            return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    }

    sdLength = RtlLengthSecurityDescriptor(securityDescriptor);
    newSd = LocalAlloc(LPTR, sdLength);
    memcpy(newSd, securityDescriptor, sdLength);
    PhFree(securityDescriptor);

    *SecurityDescriptor = newSd;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_SetSecurity(
    _In_ ISecurityInformation *This,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;
    NTSTATUS status;

    status = PhStdSetObjectSecurity(
        SecurityDescriptor,
        SecurityInformation,
        this
        );

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetAccessRights(
    _In_ ISecurityInformation *This,
    _In_ const GUID *ObjectType,
    _In_ ULONG Flags,
    _Out_ PSI_ACCESS *Access,
    _Out_ PULONG Accesses,
    _Out_ PULONG DefaultAccess
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    *Access = this->AccessEntries;
    *Accesses = this->NumberOfAccessEntries;
    *DefaultAccess = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_MapGeneric(
    _In_ ISecurityInformation *This,
    _In_ const GUID *ObjectType,
    _In_ PUCHAR AceFlags,
    _Inout_ PACCESS_MASK Mask
    )
{
    PhSecurityInformation* this = (PhSecurityInformation*)This;

    if (PhEqualString2(this->ObjectType, L"FileObject", TRUE))
    {
        static GENERIC_MAPPING genericMappings =
        {
            FILE_GENERIC_READ,
            FILE_GENERIC_WRITE,
            FILE_GENERIC_EXECUTE,
            FILE_ALL_ACCESS
        };

        RtlMapGenericMask(Mask, &genericMappings);
    }

    // TODO we're supposed to lookup the GenericMapping for the object type. (dmex)

    //POBJECT_TYPES_INFORMATION objectTypes;
    //POBJECT_TYPE_INFORMATION objectType;

    //if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
    //{
    //    objectType = PH_FIRST_OBJECT_TYPE(objectTypes);
    //
    //    for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
    //    {
    //        RtlMapGenericMask(Mask, &objectType->GenericMapping);
    //    }
    //
    //    PhFree(objectTypes);
    //}

    // TODO
    // NtQuerySystemInformation(SystemObjectInformation);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetInheritTypes(
    _In_ ISecurityInformation *This,
    _Out_ PSI_INHERIT_TYPE *InheritTypes,
    _Out_ PULONG InheritTypesCount
    )
{

    PhSecurityInformation* this = (PhSecurityInformation*)This;

    if (PhEqualString2(this->ObjectType, L"WmiDefault", TRUE))
    {
        static SI_INHERIT_TYPE inheritTypes[] =
        {
            0, 0, L"This namespace only",
            0, CONTAINER_INHERIT_ACE, L"This namespace and subnamespaces",
            0, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, L"Subnamespaces only",
        };

        *InheritTypes = inheritTypes;
        *InheritTypesCount = RTL_NUMBER_OF(inheritTypes);
        return S_OK;
    }
    else
    {
        static SI_INHERIT_TYPE inheritTypes[] =
        {
            0, 0, L"This folder only",
            0, CONTAINER_INHERIT_ACE, L"This folder, subfolders and files",
            0, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, L"Subfolders and files only",
        };

        *InheritTypes = inheritTypes;
        *InheritTypesCount = RTL_NUMBER_OF(inheritTypes);
        return S_OK;
    }

    //if (Folder-Container)
    //*InheritTypes = inheritTypes;
    //*InheritTypesCount = RTL_NUMBER_OF(inheritTypes);
    //return S_OK;
    //else
    //return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_PropertySheetPageCallback(
    _In_ ISecurityInformation *This,
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ SI_PAGE_TYPE uPage
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    if (uMsg == PSPCB_SI_INITDIALOG)
    {
        // Center the security editor window.
        if (!this->IsPage)
            PhCenterWindow(GetParent(hwnd), GetParent(GetParent(hwnd)));

        PhInitializeWindowTheme(hwnd, !!PhGetIntegerSetting(L"EnableThemeSupport"));
    }

    return E_NOTIMPL;
}

// ISecurityInformation2

HRESULT STDMETHODCALLTYPE PhSecurityInformation2_QueryInterface(
    _In_ ISecurityInformation2 *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    )
{
    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_ISecurityInformation2)
        )
    {
        PhSecurityInformation2_AddRef(This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation2_AddRef(
    _In_ ISecurityInformation2 *This
    )
{
    PhSecurityInformation2 *this = (PhSecurityInformation2 *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation2_Release(
    _In_ ISecurityInformation2 *This
    )
{
    PhSecurityInformation2 *this = (PhSecurityInformation2 *)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        PhFree(this);
        return 0;
    }

    return this->RefCount;
}

BOOL STDMETHODCALLTYPE PhSecurityInformation2_IsDaclCanonical(
    _In_ ISecurityInformation2 *This,
    _In_ PACL pDacl
    )
{
    return TRUE;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation2_LookupSids(
    _In_ ISecurityInformation2 *This,
    _In_ ULONG cSids,
    _In_ PSID *rgpSids,
    _Out_ LPDATAOBJECT *ppdo
    )
{
    PhSecurityInformation2 *this = (PhSecurityInformation2 *)This;
    PhSecurityIDataObject *dataObject;

    dataObject = PhAllocateZero(sizeof(PhSecurityInformation));
    dataObject->VTable = &PhDataObject_VTable;
    dataObject->Context = this->Context;
    dataObject->RefCount = 1;

    dataObject->SidCount = cSids;
    dataObject->Sids = rgpSids;
    dataObject->NameCache = PhCreateList(1);

    *ppdo = (LPDATAOBJECT)dataObject;

    return S_OK;
}

// ISecurityInformation3

HRESULT STDMETHODCALLTYPE PhSecurityInformation3_QueryInterface(
    _In_ ISecurityInformation3 *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    )
{
    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_ISecurityInformation3)
        )
    {
        PhSecurityInformation3_AddRef(This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation3_AddRef(
    _In_ ISecurityInformation3 *This
    )
{
    PhSecurityInformation3 *this = (PhSecurityInformation3 *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation3_Release(
    _In_ ISecurityInformation3 *This
    )
{
    PhSecurityInformation3 *this = (PhSecurityInformation3 *)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        PhFree(this);
        return 0;
    }

    return this->RefCount;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation3_GetFullResourceName(
    _In_ ISecurityInformation3 *This,
    _Outptr_ PWSTR *ppszResourceName
    )
{
    PhSecurityInformation3 *this = (PhSecurityInformation3 *)This;

    if (PhIsNullOrEmptyString(this->Context->ObjectName))
        *ppszResourceName = PhGetString(this->Context->ObjectType);
    else
        *ppszResourceName = PhGetString(this->Context->ObjectName);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation3_OpenElevatedEditor(
    _In_ ISecurityInformation3 *This,
    _In_ HWND hWnd,
    _In_ SI_PAGE_TYPE uPage
    )
{
    PhSecurityInformation3 *this = (PhSecurityInformation3 *)This;

    return E_NOTIMPL;
}

// IDataObject

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_QueryInterface(
    _In_ IDataObject *This,
    _In_ REFIID Riid,
    _COM_Outptr_ PVOID *Object
    )
{
    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_IDataObject)
        )
    {
        PhSecurityDataObject_AddRef(This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PhSecurityDataObject_AddRef(
    _In_ IDataObject *This
    )
{
    PhSecurityIDataObject *this = (PhSecurityIDataObject *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE PhSecurityDataObject_Release(
    _In_ IDataObject *This
    )
{
    PhSecurityIDataObject *this = (PhSecurityIDataObject *)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        PhDereferenceObjects(this->NameCache->Items, this->NameCache->Count);
        PhDereferenceObject(this->NameCache);

        PhFree(this);
        return 0;
    }

    return this->RefCount;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_GetData(
    _In_ IDataObject *This,
    _In_ FORMATETC *pformatetcIn,
    _Out_ STGMEDIUM *pmedium
    )
{
    PhSecurityIDataObject *this = (PhSecurityIDataObject *)This;
    PSID_INFO_LIST sidInfoList;

    sidInfoList = (PSID_INFO_LIST)GlobalAlloc(GMEM_ZEROINIT, sizeof(SID_INFO_LIST) + (sizeof(SID_INFO) * this->SidCount));

    if (!sidInfoList)
    {
        pmedium = NULL;
        return S_FALSE;
    }

    sidInfoList->cItems = this->SidCount;

    for (ULONG i = 0; i < this->SidCount; i++)
    {
        SID_INFO sidInfo;
        PPH_STRING sidString;
        SID_NAME_USE sidNameUse;

        memset(&sidInfo, 0, sizeof(SID_INFO));
        sidInfo.pSid = this->Sids[i];

        if (sidString = PhGetSidFullName(sidInfo.pSid, FALSE, &sidNameUse))
        {
            switch (sidNameUse)
            {
            case SidTypeUser:
            case SidTypeLogonSession:
                sidInfo.pwzClass = L"User";
                break;
            case SidTypeAlias:
            case SidTypeGroup:
                sidInfo.pwzClass = L"Group";
                break;
            case SidTypeComputer:
                sidInfo.pwzClass = L"Computer";
                break;
            }

            sidInfo.pwzCommonName = PhGetString(sidString);
            PhAddItemList(this->NameCache, sidString);
        }
        else if (sidString = PhGetAppContainerPackageName(sidInfo.pSid))
        {
            PhMoveReference(&sidString, PhConcatStringRefZ(&sidString->sr, L" (APP_PACKAGE)"));
            sidInfo.pwzCommonName = PhGetString(sidString);
            PhAddItemList(this->NameCache, sidString);
        }
        else if (sidString = PhGetAppContainerName(sidInfo.pSid))
        {
            PhMoveReference(&sidString, PhConcatStringRefZ(&sidString->sr, L" (APP_CONTAINER)"));
            sidInfo.pwzCommonName = PhGetString(sidString);
            PhAddItemList(this->NameCache, sidString);
        }
        else if (sidString = PhGetCapabilitySidName(sidInfo.pSid))
        {
            PhMoveReference(&sidString, PhConcatStringRefZ(&sidString->sr, L" (APP_CAPABILITY)"));
            sidInfo.pwzCommonName = PhGetString(sidString);
            PhAddItemList(this->NameCache, sidString);
        }

        sidInfoList->aSidInfo[i] = sidInfo;
    }

    pmedium->tymed = TYMED_HGLOBAL;
    pmedium->hGlobal = (HGLOBAL)sidInfoList;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_GetDataHere(
    _In_ IDataObject *This,
    _In_  FORMATETC *pformatetc,
    _Inout_ STGMEDIUM *pmedium
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_QueryGetData(
    _In_ IDataObject *This,
    _In_opt_ FORMATETC *pformatetc
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_GetCanonicalFormatEtc(
    _In_ IDataObject * This,
    _In_opt_ FORMATETC *pformatectIn,
    _Out_ FORMATETC *pformatetcOut
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_SetData(
    _In_ IDataObject *This,
    _In_ FORMATETC *pformatetc,
    _In_ STGMEDIUM *pmedium,
    _In_ BOOL fRelease
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_EnumFormatEtc(
    _In_ IDataObject *This,
    _In_ ULONG dwDirection,
    _Out_opt_ IEnumFORMATETC **ppenumFormatEtc
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_DAdvise(
    _In_ IDataObject *This,
    _In_ FORMATETC *pformatetc,
    _In_ ULONG advf,
    _In_opt_ IAdviseSink *pAdvSink,
    _Out_ ULONG *pdwConnection
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_DUnadvise(
    _In_ IDataObject *This,
    _In_ ULONG dwConnection
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_EnumDAdvise(
    _In_ IDataObject *This,
    _Out_opt_ IEnumSTATDATA **ppenumAdvise
    )
{
    return E_NOTIMPL;
}

// ISecurityObjectTypeInfo

HRESULT STDMETHODCALLTYPE PhSecurityObjectTypeInfo_QueryInterface(
    _In_ ISecurityObjectTypeInfoEx* This,
    _In_ REFIID Riid,
    _Out_ PVOID* Object
    )
{
    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_ISecurityObjectTypeInfo)
        )
    {
        PhSecurityObjectTypeInfo_AddRef(This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PhSecurityObjectTypeInfo_AddRef(
    _In_ ISecurityObjectTypeInfoEx* This
    )
{
    PhSecurityObjectTypeInfo* this = (PhSecurityObjectTypeInfo*)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE PhSecurityObjectTypeInfo_Release(
    _In_ ISecurityObjectTypeInfoEx* This
    )
{
    PhSecurityObjectTypeInfo* this = (PhSecurityObjectTypeInfo*)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        PhFree(this);
        return 0;
    }

    return this->RefCount;
}

HRESULT STDMETHODCALLTYPE PhSecurityObjectTypeInfo_GetInheritSource(
    _In_ ISecurityObjectTypeInfoEx* This,
    _In_ SECURITY_INFORMATION SecurityInfo,
    _In_ PACL Acl,
    _Out_ PINHERITED_FROM* InheritArray
    )
{
    static GENERIC_MAPPING genericMappings =
    {
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
    };
    PhSecurityObjectTypeInfo* this = (PhSecurityObjectTypeInfo*)This;
    PINHERITED_FROM result;
    ULONG status;

    if (!PhEqualString2(this->Context->ObjectType, L"FileObject", TRUE)) // TODO: Remove PhEqualString2 (dmex)
        return S_FALSE;

    result = (PINHERITED_FROM)LocalAlloc(LPTR, ((ULONGLONG)Acl->AceCount + 1) * sizeof(INHERITED_FROM));

    if (!result)
        return S_FALSE;

    if ((status = GetInheritanceSource(
        PhGetString(this->Context->ObjectName),
        SE_FILE_OBJECT,
        SecurityInfo,
        TRUE, // Container
        NULL,
        0,
        Acl,
        NULL,
        &genericMappings,
        result
        )) == ERROR_SUCCESS)
    {
        *InheritArray = result;
    }
    else
    {
        LocalFree(result);
    }

    return HRESULT_FROM_WIN32(status);
}

// IEffectivePermission

HRESULT STDMETHODCALLTYPE PhEffectivePermission_QueryInterface(
    _In_ IEffectivePermission* This,
    _In_ REFIID Riid,
    _Out_ PVOID* Object
    )
{
    if (
        IsEqualIID(Riid, &IID_IUnknown) ||
        IsEqualIID(Riid, &IID_IEffectivePermission)
        )
    {
        PhEffectivePermission_AddRef(This);
        *Object = This;
        return S_OK;
    }

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PhEffectivePermission_AddRef(
    _In_ IEffectivePermission* This
    )
{
    PhEffectivePermission* this = (PhEffectivePermission*)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE PhEffectivePermission_Release(
    _In_ IEffectivePermission* This
    )
{
    PhEffectivePermission* this = (PhEffectivePermission*)This;

    this->RefCount--;

    if (this->RefCount == 0)
    {
        PhFree(this);
        return 0;
    }

    return this->RefCount;
}

HRESULT STDMETHODCALLTYPE PhEffectivePermission_GetEffectivePermission(
    _In_ IEffectivePermission* This,
    _In_ const GUID* GuidObjectType,
    _In_ PSID UserSid,
    _In_ LPCWSTR ServerName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ POBJECT_TYPE_LIST* ObjectTypeList,
    _Out_ PULONG ObjectTypeListLength,
    _Out_ PACCESS_MASK* GrantedAccessList,
    _Out_ PULONG GrantedAccessListLength
    )
{
    ULONG status;
    OBJECT_TYPE_LIST defaultObjectTypeList[1] = { 0 };
    BOOLEAN present = FALSE;
    BOOLEAN defaulted = FALSE;
    PACL dacl = NULL;
    PACCESS_MASK accessRights;
    TRUSTEE trustee = { 0 };

    status = RtlGetDaclSecurityDescriptor(
        SecurityDescriptor,
        &present,
        &dacl,
        &defaulted
        );

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));
    // Note: RtlGetDaclSecurityDescriptor returns success for security descriptors with a NULL dacl. (dmex)
    if (NT_SUCCESS(status) && !dacl)
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(STATUS_INVALID_SECURITY_DESCR));

    accessRights = (PACCESS_MASK)LocalAlloc(LPTR, sizeof(PACCESS_MASK) + sizeof(ACCESS_MASK));

    if (!accessRights)
        return S_FALSE;

    BuildTrusteeWithSid(&trustee, UserSid);
    status = GetEffectiveRightsFromAcl(dacl, &trustee, accessRights);

    if (status != ERROR_SUCCESS)
    {
        LocalFree(accessRights);
        return HRESULT_FROM_WIN32(status);
    }

    *ObjectTypeList = (POBJECT_TYPE_LIST)defaultObjectTypeList;
    *ObjectTypeListLength = 1;
    *GrantedAccessList = accessRights;
    *GrantedAccessListLength = 1;
    return S_OK;
}

NTSTATUS PhpGetObjectSecurityWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);
    // This is required (especially for File objects) because some drivers don't seem to handle
    // QuerySecurity properly. (wj32)
    memset(buffer, 0, bufferSize);

    status = PhCallNtQuerySecurityObjectWithTimeout(
        Handle,
        SecurityInformation,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);
        memset(buffer, 0, bufferSize);

        status = PhCallNtQuerySecurityObjectWithTimeout(
            Handle,
            SecurityInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *SecurityDescriptor = (PSECURITY_DESCRIPTOR)buffer;

    return status;
}

/**
 * Retrieves the security descriptor of an object.
 *
 * \param SecurityDescriptor A variable which receives a pointer to the security descriptor of the
 * object. The security descriptor must be freed using PhFree() when no longer needed.
 * \param SecurityInformation The security information to retrieve.
 * \param Context A pointer to a PH_STD_OBJECT_SECURITY structure describing the object.
 *
 * \remarks This function may be used for the \a GetObjectSecurity callback in
 * PhCreateSecurityPage() or PhEditSecurity().
 */
_Callback_ NTSTATUS PhStdGetObjectSecurity(
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)Context;
    NTSTATUS status;
    HANDLE handle;

    if (!this)
        return STATUS_UNSUCCESSFUL;

    status = this->OpenObject(
        &handle,
        PhGetAccessForGetSecurity(SecurityInformation),
        this->Context
        );

    if (!NT_SUCCESS(status))
        return status;

    if (PhEqualString2(this->ObjectType, L"Service", TRUE) || PhEqualString2(this->ObjectType, L"SCManager", TRUE))
    {
        status = PhGetSeObjectSecurity(handle, SE_SERVICE, SecurityInformation, SecurityDescriptor);
        CloseServiceHandle(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"File", TRUE))
    {
        status = PhpGetObjectSecurityWithTimeout(handle, SecurityInformation, SecurityDescriptor);
        NtClose(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"FileObject", TRUE))
    {
        status = PhGetSeObjectSecurity(handle, SE_FILE_OBJECT, SecurityInformation, SecurityDescriptor);
        NtClose(handle);
    }
    else if (
        PhEqualString2(this->ObjectType, L"LsaAccount", TRUE) ||
        PhEqualString2(this->ObjectType, L"LsaPolicy", TRUE) ||
        PhEqualString2(this->ObjectType, L"LsaSecret", TRUE) ||
        PhEqualString2(this->ObjectType, L"LsaTrusted", TRUE)
        )
    {
        PSECURITY_DESCRIPTOR securityDescriptor;

        status = LsaQuerySecurityObject(
            handle,
            SecurityInformation,
            &securityDescriptor
            );

        if (NT_SUCCESS(status))
        {
            *SecurityDescriptor = PhAllocateCopy(
                securityDescriptor,
                RtlLengthSecurityDescriptor(securityDescriptor)
                );
            LsaFreeMemory(securityDescriptor);
        }

        LsaClose(handle);
    }
    else if (
        PhEqualString2(this->ObjectType, L"SamAlias", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamDomain", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamGroup", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamServer", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamUser", TRUE)
        )
    {
        //PSECURITY_DESCRIPTOR securityDescriptor;
        //
        //status = SamQuerySecurityObject(
        //    handle,
        //    SecurityInformation,
        //    &securityDescriptor
        //    );
        //
        //if (NT_SUCCESS(status))
        //{
        //    *SecurityDescriptor = PhAllocateCopy(
        //        securityDescriptor,
        //        RtlLengthSecurityDescriptor(securityDescriptor)
        //        );
        //    SamFreeMemory(securityDescriptor);
        //}
        //
        //SamCloseHandle(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"TokenDefault", TRUE))
    {
        PTOKEN_DEFAULT_DACL defaultDacl = NULL;

        status = PhQueryTokenVariableSize(
            handle,
            TokenDefaultDacl,
            &defaultDacl
            );

        // Note: NtQueryInformationToken returns success for processes with a NULL DefaultDacl. (dmex)
        if (NT_SUCCESS(status) && !defaultDacl->DefaultDacl)
            status = STATUS_INVALID_SECURITY_DESCR;

        if (NT_SUCCESS(status))
        {
            ULONG allocationLength;
            PSECURITY_DESCRIPTOR securityDescriptor;

            allocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH + defaultDacl->DefaultDacl->AclSize;

            securityDescriptor = PhAllocateZero(allocationLength);
            RtlCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
            RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, defaultDacl->DefaultDacl, FALSE);

            assert(allocationLength == RtlLengthSecurityDescriptor(securityDescriptor));

            *SecurityDescriptor = PhAllocateCopy(
                securityDescriptor,
                RtlLengthSecurityDescriptor(securityDescriptor)
                );
            PhFree(securityDescriptor);
        }

        if (defaultDacl)
            PhFree(defaultDacl);

        NtClose(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"PowerDefault", TRUE))
    {
        PSECURITY_DESCRIPTOR securityDescriptor;
        PPH_STRING powerPolicySddl;

        if (NT_SUCCESS(status = PhpGetPowerPolicySecurityDescriptor(&powerPolicySddl)))
        {
            if (securityDescriptor = PhGetSecurityDescriptorFromString(PhGetString(powerPolicySddl)))
            {
                *SecurityDescriptor = PhAllocateCopy(
                    securityDescriptor,
                    RtlLengthSecurityDescriptor(securityDescriptor)
                    );
                PhFree(securityDescriptor);
            }
            else
            {
                status = STATUS_INVALID_SECURITY_DESCR;
            }

            PhDereferenceObject(powerPolicySddl);
        }
    }
    else if (PhEqualString2(this->ObjectType, L"RdpDefault", TRUE))
    {
        PSECURITY_DESCRIPTOR securityDescriptor;

        if (NT_SUCCESS(status = PhpGetRemoteDesktopSecurityDescriptor(&securityDescriptor)))
        {
            *SecurityDescriptor = PhAllocateCopy(
                securityDescriptor,
                RtlLengthSecurityDescriptor(securityDescriptor)
                );
            PhFree(securityDescriptor);
        }
    }
    else if (PhEqualString2(this->ObjectType, L"WmiDefault", TRUE))
    {
        PSECURITY_DESCRIPTOR securityDescriptor;

        if (NT_SUCCESS(status = PhGetWmiNamespaceSecurityDescriptor(&securityDescriptor)))
        {
            *SecurityDescriptor = PhAllocateCopy(
                securityDescriptor,
                RtlLengthSecurityDescriptor(securityDescriptor)
                );
            PhFree(securityDescriptor);
        }
    }
    else
    {
        status = PhGetObjectSecurity(handle, SecurityInformation, SecurityDescriptor);
        NtClose(handle);
    }

    return status;
}

/**
 * Modifies the security descriptor of an object.
 *
 * \param SecurityDescriptor A security descriptor containing security information to set.
 * \param SecurityInformation The security information to retrieve.
 * \param Context A pointer to a PH_STD_OBJECT_SECURITY structure describing the object.
 *
 * \remarks This function may be used for the \a SetObjectSecurity callback in
 * PhCreateSecurityPage() or PhEditSecurity().
 */
_Callback_ NTSTATUS PhStdSetObjectSecurity(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)Context;
    NTSTATUS status;
    HANDLE handle;

    if (!this)
        return STATUS_UNSUCCESSFUL;

    status = this->OpenObject(
        &handle,
        PhGetAccessForSetSecurity(SecurityInformation),
        this->Context
        );

    if (!NT_SUCCESS(status))
        return status;

    if (PhEqualString2(this->ObjectType, L"Service", TRUE) || PhEqualString2(this->ObjectType, L"SCManager", TRUE))
    {
        status = PhSetSeObjectSecurity(handle, SE_SERVICE, SecurityInformation, SecurityDescriptor);
        CloseServiceHandle(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"File", TRUE))
    {
        status = PhSetObjectSecurity(handle, SecurityInformation, SecurityDescriptor);
        NtClose(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"FileObject", TRUE))
    {
        status = PhSetSeObjectSecurity(handle, SE_FILE_OBJECT, SecurityInformation, SecurityDescriptor);
        NtClose(handle);
    }
    else if (
        PhEqualString2(this->ObjectType, L"LsaAccount", TRUE) ||
        PhEqualString2(this->ObjectType, L"LsaPolicy", TRUE) ||
        PhEqualString2(this->ObjectType, L"LsaSecret", TRUE) ||
        PhEqualString2(this->ObjectType, L"LsaTrusted", TRUE)
        )
    {
        status = LsaSetSecurityObject(
            handle,
            SecurityInformation,
            SecurityDescriptor
            );
        
        LsaClose(handle);
    }
    else if (
        PhEqualString2(this->ObjectType, L"SamAlias", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamDomain", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamGroup", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamServer", TRUE) ||
        PhEqualString2(this->ObjectType, L"SamUser", TRUE)
        )
    {
        //status = SamSetSecurityObject(
        //    handle,
        //    SecurityInformation,
        //    SecurityDescriptor
        //    );
        //
        //SamCloseHandle(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"TokenDefault", TRUE))
    {
        BOOLEAN present = FALSE;
        BOOLEAN defaulted = FALSE;
        PACL dacl = NULL;

        status = RtlGetDaclSecurityDescriptor(
            SecurityDescriptor,
            &present,
            &dacl,
            &defaulted
            );

        // Note: RtlGetDaclSecurityDescriptor returns success for security descriptors with a NULL dacl. (dmex)
        if (NT_SUCCESS(status) && !dacl)
            status = STATUS_INVALID_SECURITY_DESCR;

        if (NT_SUCCESS(status))
        {
            TOKEN_DEFAULT_DACL defaultDacl;

            defaultDacl.DefaultDacl = dacl;

            status = NtSetInformationToken(
                handle,
                TokenDefaultDacl,
                &defaultDacl,
                sizeof(TOKEN_DEFAULT_DACL)
                );
        }

        NtClose(handle);
    }
    else if (PhEqualString2(this->ObjectType, L"PowerDefault", TRUE))
    {
        PPH_STRING powerPolicySddl;
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
        UCHAR administratorsSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * 2];
        PSID administratorsSid;

        // kludge the descriptor into the correct SDDL format required by powercfg (dmex)
        // 1) The owner must always be the built-in domain administrator.
        // 2) The group must always be NT AUTHORITY\SYSTEM.
        // 3) Remove the INHERIT_REQ flag (not required but makes the sddl consistent with powercfg).

        administratorsSid = (PSID)administratorsSidBuffer;
        RtlInitializeSid(administratorsSid, &ntAuthority, 2);
        *RtlSubAuthoritySid(administratorsSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
        *RtlSubAuthoritySid(administratorsSid, 1) = DOMAIN_ALIAS_RID_ADMINS;
        RtlSetOwnerSecurityDescriptor(SecurityDescriptor, administratorsSid, TRUE);
        RtlSetGroupSecurityDescriptor(SecurityDescriptor, &PhSeLocalSystemSid, TRUE);
        RtlSetControlSecurityDescriptor(SecurityDescriptor, SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ, SE_DACL_PROTECTED);

        if (powerPolicySddl = PhGetSecurityDescriptorAsString(
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
            DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
            SecurityDescriptor
            ))
        {
            status = PhpSetPowerPolicySecurityDescriptor(powerPolicySddl);
            PhDereferenceObject(powerPolicySddl);
        }
    }
    else if (PhEqualString2(this->ObjectType, L"RdpDefault", TRUE))
    {
        status = PhpSetRemoteDesktopSecurityDescriptor(SecurityDescriptor);
    }
    else if (PhEqualString2(this->ObjectType, L"WmiDefault", TRUE))
    {
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
        UCHAR administratorsSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * 2];
        PSID administratorsSid;

        // kludge the descriptor into the correct format required by wmimgmt (dmex)
        // 1) The owner must always be the built-in domain administrator.
        // 2) The group must always be the built-in domain administrator.

        administratorsSid = (PSID)administratorsSidBuffer;
        RtlInitializeSid(administratorsSid, &ntAuthority, 2);
        *RtlSubAuthoritySid(administratorsSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
        *RtlSubAuthoritySid(administratorsSid, 1) = DOMAIN_ALIAS_RID_ADMINS;
        RtlSetOwnerSecurityDescriptor(SecurityDescriptor, administratorsSid, TRUE);
        RtlSetGroupSecurityDescriptor(SecurityDescriptor, administratorsSid, TRUE);

        status = PhSetWmiNamespaceSecurityDescriptor(SecurityDescriptor);
    }
    else
    {
        status = PhSetObjectSecurity(handle, SecurityInformation, SecurityDescriptor);
        NtClose(handle);
    }

    return status;
}

NTSTATUS PhGetSeObjectSecurity(
    _In_ HANDLE Handle,
    _In_ ULONG ObjectType,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
{
    ULONG win32Result;
    PSECURITY_DESCRIPTOR securityDescriptor;

    win32Result = GetSecurityInfo(
        Handle,
        ObjectType,
        SecurityInformation,
        NULL,
        NULL,
        NULL,
        NULL,
        &securityDescriptor
        );

    if (win32Result != ERROR_SUCCESS)
        return NTSTATUS_FROM_WIN32(win32Result);

    *SecurityDescriptor = PhAllocateCopy(securityDescriptor, RtlLengthSecurityDescriptor(securityDescriptor));
    LocalFree(securityDescriptor);

    return STATUS_SUCCESS;
}

NTSTATUS PhSetSeObjectSecurity(
    _In_ HANDLE Handle,
    _In_ ULONG ObjectType,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    ULONG win32Result = NO_ERROR;
    SECURITY_INFORMATION securityInformation = 0;
    BOOLEAN present = FALSE;
    BOOLEAN defaulted = FALSE;
    PSID owner = NULL;
    PSID group = NULL;
    PACL dacl = NULL;
    PACL sacl = NULL;

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        if (NT_SUCCESS(RtlGetOwnerSecurityDescriptor(SecurityDescriptor, &owner, &defaulted)))
            securityInformation |= OWNER_SECURITY_INFORMATION;
    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        if (NT_SUCCESS(RtlGetGroupSecurityDescriptor(SecurityDescriptor, &group, &defaulted)))
            securityInformation |= GROUP_SECURITY_INFORMATION;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        if (NT_SUCCESS(RtlGetDaclSecurityDescriptor(SecurityDescriptor, &present, &dacl, &defaulted)) && present)
            securityInformation |= DACL_SECURITY_INFORMATION;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        if (NT_SUCCESS(RtlGetSaclSecurityDescriptor(SecurityDescriptor, &present, &sacl, &defaulted)) && present)
            securityInformation |= SACL_SECURITY_INFORMATION;
    }

    if (ObjectType == SE_FILE_OBJECT) // probably works with other types but haven't checked (dmex)
    {
        SECURITY_DESCRIPTOR_CONTROL control;
        ULONG revision;

        if (NT_SUCCESS(RtlGetControlSecurityDescriptor(SecurityDescriptor, &control, &revision)))
        {
            if (SecurityInformation & DACL_SECURITY_INFORMATION)
            {
                if (control & SE_DACL_PROTECTED)
                    securityInformation |= PROTECTED_DACL_SECURITY_INFORMATION;
                else
                    securityInformation |= UNPROTECTED_DACL_SECURITY_INFORMATION;
            }

            if (SecurityInformation & SACL_SECURITY_INFORMATION)
            {
                if (control & SE_SACL_PROTECTED)
                    securityInformation |= PROTECTED_SACL_SECURITY_INFORMATION;
                else
                    securityInformation |= UNPROTECTED_SACL_SECURITY_INFORMATION;
            }
        }
    }

    win32Result = SetSecurityInfo(
        Handle,
        ObjectType,
        securityInformation, // SecurityInformation
        owner,
        group,
        dacl,
        sacl
        );

    if (win32Result != ERROR_SUCCESS)
        return NTSTATUS_FROM_WIN32(win32Result);

    return STATUS_SUCCESS;
}

// Power policy security descriptors
// HACK - nightly build feature testing (dmex)
// TODO - move definitions to proper locations.

#include <powrprof.h>

ULONG (WINAPI* PowerGetActiveScheme_I)(
    _In_ HKEY UserRootPowerKey,
    _Out_ PGUID* ActivePolicyGuid
    ) = NULL;
ULONG (WINAPI* PowerSetActiveScheme_I)(
    _In_ HKEY UserRootPowerKey,
    _In_ PGUID SchemeGuid
    ) = NULL;
ULONG (WINAPI* PowerRestoreDefaultPowerSchemes_I)(
    VOID
    ) = NULL;
// rev
ULONG (WINAPI* PowerReadSecurityDescriptor)(
    _In_ POWER_DATA_ACCESSOR AccessFlags,
    _In_ PGUID PowerGuid,
    _Out_ PWSTR* StringSecurityDescriptor
    ) = NULL;
// rev
ULONG (WINAPI* PowerWriteSecurityDescriptor)(
    _In_ POWER_DATA_ACCESSOR AccessFlags,
    _In_ PGUID PowerGuid,
    _In_ PWSTR StringSecurityDescriptor
    ) = NULL;

PVOID PhpInitializePowerPolicyApi(VOID)
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        imageBaseAddress = PhLoadLibrary(L"powrprof.dll");
        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

NTSTATUS PhpGetPowerPolicySecurityDescriptor(
    _Out_ PPH_STRING* StringSecurityDescriptor
    )
{
    ULONG status;
    PVOID imageBaseAddress;
    PWSTR stringSecurityDescriptor;
    PGUID policyGuid;

    if (!(imageBaseAddress = PhpInitializePowerPolicyApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!PowerGetActiveScheme_I)
        PowerGetActiveScheme_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerGetActiveScheme", 0);
    if (!PowerReadSecurityDescriptor)
        PowerReadSecurityDescriptor = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerReadSecurityDescriptor", 0);

    if (!(PowerGetActiveScheme_I && PowerReadSecurityDescriptor))
        return STATUS_PROCEDURE_NOT_FOUND;

    // We can use GUIDs for schemes, policies or other values but we only query the default scheme. (dmex)
    status = PowerGetActiveScheme_I(
        NULL,
        &policyGuid
        );

    if (status != ERROR_SUCCESS)
        return NTSTATUS_FROM_WIN32(status);

    // PowerReadSecurityDescriptor/PowerWriteSecurityDescriptor both use the same SDDL
    // format as registry keys and are RPC wrappers for this registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Power\SecurityDescriptors

    status = PowerReadSecurityDescriptor(
        ACCESS_DEFAULT_SECURITY_DESCRIPTOR, // ACCESS_ACTIVE_SCHEME?
        policyGuid,
        &stringSecurityDescriptor
        );

    if (status == ERROR_SUCCESS)
    {
        if (StringSecurityDescriptor)
        {
            *StringSecurityDescriptor = PhCreateString(stringSecurityDescriptor);
        }

        LocalFree(stringSecurityDescriptor);
    }

    LocalFree(policyGuid);

    return NTSTATUS_FROM_WIN32(status);
}

NTSTATUS PhpSetPowerPolicySecurityDescriptor(
    _In_ PPH_STRING StringSecurityDescriptor
    )
{
    ULONG status;
    PVOID imageBaseAddress;
    PGUID policyGuid;

    if (!(imageBaseAddress = PhpInitializePowerPolicyApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!PowerGetActiveScheme_I)
        PowerGetActiveScheme_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerGetActiveScheme", 0);
    if (!PowerWriteSecurityDescriptor)
        PowerWriteSecurityDescriptor = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerWriteSecurityDescriptor", 0);

    if (!(PowerGetActiveScheme_I && PowerWriteSecurityDescriptor))
        return STATUS_PROCEDURE_NOT_FOUND;

    status = PowerGetActiveScheme_I(
        NULL,
        &policyGuid
        );

    if (status != ERROR_SUCCESS)
        return NTSTATUS_FROM_WIN32(status);

    status = PowerWriteSecurityDescriptor(
        ACCESS_DEFAULT_SECURITY_DESCRIPTOR,
        policyGuid, // Todo: Pass the GUID from the original query.
        PhGetString(StringSecurityDescriptor)
        );

    //if (status == ERROR_SUCCESS)
    //{
    //    // refresh/reapply the current scheme.
    //    status = PowerSetActiveScheme_I(NULL, policyGuid);
    //}

    LocalFree(policyGuid);

    return NTSTATUS_FROM_WIN32(status);
}

// Terminal server security descriptors
// HACK - nightly build feature testing (dmex)
// TODO - move definitions to proper locations.

#include <wtsapi32.h>

BOOL (WINAPI* WTSGetListenerSecurity_I)(
    _In_ HANDLE ServerHandle,
    _In_ PVOID Reserved1,
    _In_ ULONG Reserved2,
    _In_ PWSTR ListenerName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    ) = NULL;

BOOL (WINAPI* WTSSetListenerSecurity_I)(
    _In_ HANDLE ServerHandle,
    _In_ PVOID Reserved1,
    _In_ ULONG Reserved2,
    _In_ PWSTR ListenerName,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    ) = NULL;

PVOID PhpInitializeRemoteDesktopServiceApi(VOID)
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        imageBaseAddress = PhLoadLibrary(L"wtsapi32.dll");
        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

NTSTATUS PhpGetRemoteDesktopSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    BOOL status;
    PVOID imageBaseAddress;
    ULONG securityDescriptorLength = 0;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;

    if (!(imageBaseAddress = PhpInitializeRemoteDesktopServiceApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!WTSGetListenerSecurity_I)
        WTSGetListenerSecurity_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "WTSGetListenerSecurityW", 0);

    if (!WTSGetListenerSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    // Todo: Add support for SI_RESET using the default security descriptor: 
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Terminal Server\DefaultSecurity

    status = WTSGetListenerSecurity_I(
        WTS_CURRENT_SERVER_HANDLE,
        NULL,
        0,
        L"Rdp-Tcp", // or "Console"
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
        securityDescriptor,
        0,
        &securityDescriptorLength
        );

    if (!(!status && securityDescriptorLength))
        return STATUS_INVALID_SECURITY_DESCR;

    securityDescriptor = PhAllocateZero(securityDescriptorLength);

    status = WTSGetListenerSecurity_I(
        WTS_CURRENT_SERVER_HANDLE,
        NULL,
        0,
        L"Rdp-Tcp", // or "Console"
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
        securityDescriptor,
        securityDescriptorLength,
        &securityDescriptorLength
        );

    if (status)
    {
        if (SecurityDescriptor)
        {
            *SecurityDescriptor = securityDescriptor;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_INVALID_SECURITY_DESCR;
}

NTSTATUS PhpSetRemoteDesktopSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PVOID imageBaseAddress;

    if (!(imageBaseAddress = PhpInitializeRemoteDesktopServiceApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!WTSSetListenerSecurity_I)
        WTSSetListenerSecurity_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "WTSSetListenerSecurityW", 0);

    if (!WTSSetListenerSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    if (WTSSetListenerSecurity_I(
        WTS_CURRENT_SERVER_HANDLE,
        NULL,
        0,
        L"RDP-Tcp",
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
        SecurityDescriptor
        ))
    {
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_SECURITY_DESCR;
}

// WBEM security descriptors
// HACK - nightly build feature testing (dmex)
// TODO - This was located in prpgwmi.c but VS2019
// complained about LNK1120 even though it linked correctly.

#include <wbemidl.h>

DEFINE_GUID(CLSID_WbemLocator, 0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);
DEFINE_GUID(IID_IWbemLocator, 0xdc12a687, 0x737f, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);

PVOID PhGetWbemProxDllBase(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING systemDirectory;
        PPH_STRING systemFileName;

        if (systemDirectory = PhGetSystemDirectory())
        {
            if (systemFileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\wbem\\wbemprox.dll"))
            {
                if (!(imageBaseAddress = PhGetLoaderEntryStringRefDllBase(&systemFileName->sr, NULL)))
                    imageBaseAddress = PhLoadLibrary(PhGetString(systemFileName));

                PhDereferenceObject(systemFileName);
            }

            PhDereferenceObject(systemDirectory);
        }

        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

NTSTATUS PhGetWmiNamespaceSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    HRESULT status;
    PVOID imageBaseAddress;
    PVOID securityDescriptor = NULL;
    PVOID securityDescriptorData = NULL;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemQueryString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemGetSDClassObject = 0;
    VARIANT variantReturnValue = { VT_EMPTY };
    VARIANT variantArrayValue = { VT_EMPTY };
   
    if (!(imageBaseAddress = PhGetWbemProxDllBase()))
        return STATUS_UNSUCCESSFUL;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocString(L"Root");
    status = IWbemLocator_ConnectServer(
        wbemLocator,
        wbemResourceString,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &wbemServices
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemQueryString = SysAllocString(L"__SystemSecurity");
    status = IWbemServices_GetObject(
        wbemServices,
        wbemQueryString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemMethodString = SysAllocString(L"GetSD");
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemQueryString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemGetSDClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemGetSDClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    if (V_UI4(&variantReturnValue) != ERROR_SUCCESS)
    {
        status = HRESULT_FROM_WIN32(V_UI4(&variantReturnValue));
        goto CleanupExit;
    }

    status = IWbemClassObject_Get(
        wbemGetSDClassObject,
        L"SD",
        0,
        &variantArrayValue,
        NULL,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    status = SafeArrayAccessData(V_ARRAY(&variantArrayValue), &securityDescriptorData);

    if (FAILED(status))
        goto CleanupExit;

    // The Wbem security descriptor is relative but the ACUI
    // dialog automatically converts the descriptor for us
    // so we don't have to convert just validate. (dmex)

    if (RtlValidSecurityDescriptor(securityDescriptorData))
    {
        securityDescriptor = PhAllocateCopy(
            securityDescriptorData,
            RtlLengthSecurityDescriptor(securityDescriptorData)
            );
    }

    SafeArrayUnaccessData(V_ARRAY(&variantArrayValue));

CleanupExit:
    if (wbemGetSDClassObject)
        IWbemClassObject_Release(wbemGetSDClassObject);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemClassObject)
        IWbemClassObject_Release(wbemClassObject);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    VariantClear(&variantArrayValue);
    VariantClear(&variantReturnValue);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);

    if (SUCCEEDED(status) && securityDescriptor)
    {
        *SecurityDescriptor = securityDescriptor;
        return STATUS_SUCCESS;
    }

    if (securityDescriptor)
    {
        PhFree(securityDescriptor);
    }

    if (status == WBEM_E_ACCESS_DENIED)
        return STATUS_ACCESS_DENIED;
    if (status == WBEM_E_INVALID_PARAMETER)
        return STATUS_INVALID_PARAMETER;

    return STATUS_INVALID_SECURITY_DESCR;
}

NTSTATUS PhSetWmiNamespaceSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemQueryString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemSetSDClassObject = NULL;
    PVOID safeArrayData = NULL;
    LPSAFEARRAY safeArray = NULL;
    SAFEARRAYBOUND safeArrayBounds;
    PSECURITY_DESCRIPTOR relativeSecurityDescriptor = 0;
    ULONG relativeSecurityDescriptorLength = 0;
    BOOLEAN freeSecurityDescriptor = FALSE;
    VARIANT variantArrayValue = { VT_EMPTY };
    VARIANT variantReturnValue = { VT_EMPTY };

    if (!(imageBaseAddress = PhGetWbemProxDllBase()))
        return STATUS_UNSUCCESSFUL;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocString(L"Root");
    status = IWbemLocator_ConnectServer(
        wbemLocator,
        wbemResourceString,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &wbemServices
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemQueryString = SysAllocString(L"__SystemSecurity");
    status = IWbemServices_GetObject(
        wbemServices,
        wbemQueryString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    if (RtlValidRelativeSecurityDescriptor(
        SecurityDescriptor,
        RtlLengthSecurityDescriptor(SecurityDescriptor),
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION
        ))
    {
        relativeSecurityDescriptor = SecurityDescriptor;
        relativeSecurityDescriptorLength = RtlLengthSecurityDescriptor(SecurityDescriptor);
    }
    else
    {
        NTSTATUS ntstatus;

        ntstatus = RtlAbsoluteToSelfRelativeSD(
            SecurityDescriptor,
            NULL,
            &relativeSecurityDescriptorLength
            );

        if (ntstatus != STATUS_BUFFER_TOO_SMALL)
        {
            status = HRESULT_FROM_NT(ntstatus);
            goto CleanupExit;
        }

        relativeSecurityDescriptor = PhAllocate(relativeSecurityDescriptorLength);
        ntstatus = RtlAbsoluteToSelfRelativeSD(
            SecurityDescriptor,
            relativeSecurityDescriptor,
            &relativeSecurityDescriptorLength
            );

        if (!NT_SUCCESS(ntstatus))
        {
            PhFree(relativeSecurityDescriptor);
            status = HRESULT_FROM_NT(ntstatus);
            goto CleanupExit;
        }

        freeSecurityDescriptor = TRUE;
    }

    safeArrayBounds.lLbound = 0;
    safeArrayBounds.cElements = relativeSecurityDescriptorLength;

    if (!(safeArray = SafeArrayCreate(VT_UI1, 1, &safeArrayBounds)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    status = SafeArrayAccessData(safeArray, &safeArrayData);

    if (FAILED(status))
        goto CleanupExit;

    memcpy(
        safeArrayData,
        relativeSecurityDescriptor,
        relativeSecurityDescriptorLength
        );

    status = SafeArrayUnaccessData(safeArray);

    if (FAILED(status))
        goto CleanupExit;

    V_VT(&variantArrayValue) = VT_ARRAY | VT_UI1;
    V_ARRAY(&variantArrayValue) = safeArray;

    status = IWbemClassObject_Put(
        wbemClassObject,
        L"SD",
        0,
        &variantArrayValue,
        CIM_EMPTY
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemMethodString = SysAllocString(L"SetSD");
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemQueryString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemSetSDClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemSetSDClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    if (V_UI4(&variantReturnValue) != ERROR_SUCCESS)
    {
        status = HRESULT_FROM_WIN32(V_UI4(&variantReturnValue));
    }

CleanupExit:
    if (wbemSetSDClassObject)
        IWbemClassObject_Release(wbemSetSDClassObject);
    if (wbemClassObject)
        IWbemClassObject_Release(wbemClassObject);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    if (freeSecurityDescriptor && relativeSecurityDescriptor)
        PhFree(relativeSecurityDescriptor);

    VariantClear(&variantReturnValue);
    VariantClear(&variantArrayValue);
    //if (safeArray) SafeArrayDestroy(safeArray);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);

    if (SUCCEEDED(status))
    {
        return STATUS_SUCCESS;
    }

    if (status == WBEM_E_ACCESS_DENIED)
        return STATUS_ACCESS_DENIED;
    if (status == WBEM_E_INVALID_PARAMETER)
        return STATUS_INVALID_PARAMETER;

    return STATUS_INVALID_SECURITY_DESCR;
}

HRESULT PhRestartDefenderOfflineScan(
    VOID
    )
{
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemQueryString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemStartClassObject = 0;
    VARIANT variantReturnValue = { VT_EMPTY };

    if (!(imageBaseAddress = PhGetWbemProxDllBase()))
        return STATUS_UNSUCCESSFUL;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocString(L"Root\\Microsoft\\Windows\\Defender");
    status = IWbemLocator_ConnectServer(
        wbemLocator,
        wbemResourceString,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &wbemServices
        );

    if (FAILED(status))
        goto CleanupExit;

    status = CoSetProxyBlanket(
        (IUnknown*)wbemServices,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemQueryString = SysAllocString(L"MSFT_MpWDOScan");
    status = IWbemServices_GetObject(
        wbemServices,
        wbemQueryString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemMethodString = SysAllocString(L"Start");
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemQueryString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemStartClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemStartClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    if (V_UI4(&variantReturnValue) != ERROR_SUCCESS)
    {
        status = HRESULT_FROM_WIN32(V_UI4(&variantReturnValue));
        goto CleanupExit;
    }

CleanupExit:
    if (wbemStartClassObject)
        IWbemClassObject_Release(wbemStartClassObject);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemClassObject)
        IWbemClassObject_Release(wbemClassObject);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    VariantClear(&variantReturnValue);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);

    return status;
}
