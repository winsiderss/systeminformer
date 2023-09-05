/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <appresolver.h>
#include <secedit.h>
#include <svcsup.h>
#include <lsasup.h>

#include <aclui.h>
#include <aclapi.h>

#include <guisup.h>
#include <hndlinfo.h>
#include <seceditp.h>
#include <secwmi.h>

BOOLEAN PhEnableSecurityAdvancedDialog = FALSE;
BOOLEAN PhEnableSecurityDialogThread = TRUE;

static const ISecurityInformationVtbl PhSecurityInformation_VTable =
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

static const ISecurityInformation2Vtbl PhSecurityInformation_VTable2 =
{
    PhSecurityInformation2_QueryInterface,
    PhSecurityInformation2_AddRef,
    PhSecurityInformation2_Release,
    PhSecurityInformation2_IsDaclCanonical,
    PhSecurityInformation2_LookupSids
};

static const ISecurityInformation3Vtbl PhSecurityInformation_VTable3 =
{
    PhSecurityInformation3_QueryInterface,
    PhSecurityInformation3_AddRef,
    PhSecurityInformation3_Release,
    PhSecurityInformation3_GetFullResourceName,
    PhSecurityInformation3_OpenElevatedEditor
};

static const IDataObjectVtbl PhDataObject_VTable =
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

static const ISecurityObjectTypeInfoExVtbl PhSecurityObjectTypeInfo_VTable3 =
{
    PhSecurityObjectTypeInfo_QueryInterface,
    PhSecurityObjectTypeInfo_AddRef,
    PhSecurityObjectTypeInfo_Release,
    PhSecurityObjectTypeInfo_GetInheritSource
};

static const IEffectivePermissionVtbl PhEffectivePermission_VTable =
{
    PhEffectivePermission_QueryInterface,
    PhEffectivePermission_AddRef,
    PhEffectivePermission_Release,
    PhEffectivePermission_GetEffectivePermission
};

static VOID PhEditSecurityAdvanced(
    _In_ PVOID Context
    )
{
    PhSecurityInformation* this = (PhSecurityInformation*)Context;

    if (WindowsVersion > WINDOWS_7 && PhEnableSecurityAdvancedDialog)
        EditSecurityAdvanced(this->WindowHandle, Context, COMBINE_PAGE_ACTIVATION(SI_PAGE_PERM, SI_SHOW_PERM_ACTIVATED));
    else
        EditSecurity(this->WindowHandle, Context);

    PhSecurityInformation_Release(Context);
}

static NTSTATUS PhEditSecurityAdvancedThread(
    _In_ PVOID Context
    )
{
    PhEditSecurityAdvanced(Context);
    return STATUS_SUCCESS;
}

/**
 * Creates a security editor page.
 *
 * \param ObjectName The name of the object.
 * \param ObjectType The type name of the object.
 * \param OpenObject An optional procedure for opening the object.
 * \param CloseObject An optional procedure for closing the object.
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
        NULL,
        NULL,
        Context,
        TRUE
        );

    page = CreateSecurityPage(info);

    PhSecurityInformation_Release(info);

    return page;
}

/**
 * Displays a security editor dialog.
 *
 * \param WindowHandle The parent window of the dialog.
 * \param ObjectName The name of the object.
 * \param ObjectType The type of object.
 * \param OpenObject An optional procedure for opening the object.
 * \param CloseObject An optional procedure for closing the object.
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
        NULL,
        NULL,
        Context,
        FALSE
        );

    if (PhEnableSecurityDialogThread)
        PhCreateThread2(PhEditSecurityAdvancedThread, info);
    else
        PhEditSecurityAdvanced(info);
}

/**
 * Displays a security editor dialog.
 *
 * \param WindowHandle The parent window of the dialog.
 * \param ObjectName The name of the object.
 * \param ObjectType The type of object.
 * \param OpenObject An optional procedure for opening the object.
 * \param CloseObject An optional procedure for closing the object.
 * \param GetObjectSecurity A callback function executed to retrieve the security descriptor of the object.
 * \param SetObjectSecurity A callback function executed to modify the security descriptor of the object.
 * \param Context A user-defined value to pass to the callback functions.
 */
VOID PhEditSecurityEx(
    _In_opt_ HWND WindowHandle,
    _In_ PWSTR ObjectName,
    _In_ PWSTR ObjectType,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    _In_opt_ PPH_SET_OBJECT_SECURITY SetObjectSecurity,
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
        GetObjectSecurity,
        SetObjectSecurity,
        Context,
        FALSE
        );

    if (PhEnableSecurityDialogThread)
        PhCreateThread2(PhEditSecurityAdvancedThread, info);
    else
        PhEditSecurityAdvanced(info);
}

ISecurityInformation *PhSecurityInformation_Create(
    _In_opt_ HWND WindowHandle,
    _In_ PWSTR ObjectName,
    _In_ PWSTR ObjectType,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    _In_opt_ PPH_SET_OBJECT_SECURITY SetObjectSecurity,
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
    info->GetObjectSecurity = GetObjectSecurity;
    info->SetObjectSecurity = SetObjectSecurity;
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

    {
        GENERIC_MAPPING genericMapping;

        if (NT_SUCCESS(PhGetObjectTypeMask(&info->ObjectType->sr, &genericMapping)))
        {
            memcpy(&info->GenericMapping, &genericMapping, sizeof(genericMapping));
            info->HaveGenericMapping = TRUE;
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
    else if (PhEqualString2(this->ObjectType, L"TokenDefault", TRUE))
    {
        ObjectInfo->dwFlags &= ~(SI_EDIT_OWNER | SI_EDIT_AUDITS);
    }
    else if (PhEqualString2(this->ObjectType, L"PowerDefault", TRUE))
    {
        ObjectInfo->dwFlags &= ~SI_EDIT_AUDITS;
        ObjectInfo->dwFlags |= SI_NO_ACL_PROTECT | SI_NO_TREE_APPLY | SI_CONTAINER | SI_OWNER_READONLY;
    }
    else if (PhEqualString2(this->ObjectType, L"RdpDefault", TRUE))
    {
        ObjectInfo->dwFlags &= ~SI_EDIT_OWNER;
        ObjectInfo->dwFlags |= SI_NO_ACL_PROTECT | SI_NO_TREE_APPLY;
    }
    else if (PhEqualString2(this->ObjectType, L"WmiDefault", TRUE))
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
        if (this->GetObjectSecurity)
        {
            PH_STD_OBJECT_SECURITY objectSecurity;

            objectSecurity.ObjectContext = this;
            objectSecurity.Context = this->Context;

            status = this->GetObjectSecurity(
                &securityDescriptor,
                RequestedInformation,
                &objectSecurity
                );
        }
        else
        {
            status = PhStdGetObjectSecurity(
                &securityDescriptor,
                RequestedInformation,
                this
                );
        }

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

    if (this->SetObjectSecurity)
    {
        PH_STD_OBJECT_SECURITY objectSecurity;

        objectSecurity.ObjectContext = this;
        objectSecurity.Context = this->Context;

        status = this->SetObjectSecurity(
            SecurityDescriptor,
            SecurityInformation,
            &objectSecurity
            );
    }
    else
    {
        status = PhStdSetObjectSecurity(
            SecurityDescriptor,
            SecurityInformation,
            this
            );
    }

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetAccessRights(
    _In_ ISecurityInformation *This,
    _In_ PCGUID ObjectType,
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
    _In_ PCGUID ObjectType,
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

        PhMapGenericMask(Mask, &genericMappings);
    }
    else
    {
        if (this->HaveGenericMapping)
        {
            PhMapGenericMask(Mask, &this->GenericMapping);
        }
    }

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
            { 0, 0, L"This namespace only" },
            { 0, CONTAINER_INHERIT_ACE, L"This namespace and subnamespaces" },
            { 0, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, L"Subnamespaces only" },
        };

        *InheritTypes = inheritTypes;
        *InheritTypesCount = RTL_NUMBER_OF(inheritTypes);
        return S_OK;
    }
    else
    {
        static SI_INHERIT_TYPE inheritTypes[] =
        {
            { 0, 0, L"This folder only" },
            { 0, CONTAINER_INHERIT_ACE, L"This folder, subfolders and files" },
            { 0, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, L"Subfolders and files only" },
        };

        *InheritTypes = inheritTypes;
        *InheritTypesCount = RTL_NUMBER_OF(inheritTypes);
        return S_OK;
    }

    return E_NOTIMPL;
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

        PhInitializeWindowTheme(hwnd, PhEnableThemeSupport);
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
    _In_ LPCGUID GuidObjectType,
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
    TRUSTEE trustee;

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

    PhBuildTrusteeWithSid(&trustee, UserSid);
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
        status = PhGetServiceObjectSecurity(handle, SecurityInformation, SecurityDescriptor);
        PhCloseServiceHandle(handle);
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
        status = PhSetServiceObjectSecurity(handle, SecurityInformation, SecurityDescriptor);
        PhCloseServiceHandle(handle);
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

        // kludge the descriptor into the correct SDDL format required by powercfg (dmex)
        // 1) The owner must always be the built-in domain administrator.
        // 2) The group must always be NT AUTHORITY\SYSTEM.
        // 3) Remove the INHERIT_REQ flag (not required but makes the sddl consistent with powercfg).

        RtlSetOwnerSecurityDescriptor(SecurityDescriptor, PhSeAdministratorsSid(), TRUE);
        RtlSetGroupSecurityDescriptor(SecurityDescriptor, (PSID)&PhSeLocalSystemSid, TRUE);
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
        PSID administratorsSid;

        // kludge the descriptor into the correct format required by wmimgmt (dmex)
        // 1) The owner must always be the built-in domain administrator.
        // 2) The group must always be the built-in domain administrator.

        administratorsSid = PhSeAdministratorsSid();
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
        return PhDosErrorToNtStatus(win32Result);

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
        return PhDosErrorToNtStatus(win32Result);

    return STATUS_SUCCESS;
}
