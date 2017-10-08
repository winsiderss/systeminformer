/*
 * Process Hacker -
 *   object security editor
 *
 * Copyright (C) 2010-2016 wj32
 * Copyright (C) 2017 dmex
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

#include <ph.h>
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

/**
 * Creates a security editor page.
 *
 * \param ObjectName The name of the object.
 * \param GetObjectSecurity A callback function executed to retrieve the security descriptor of the
 * object.
 * \param SetObjectSecurity A callback function executed to modify the security descriptor of the
 * object.
 * \param Context A user-defined value to pass to the callback functions.
 * \param AccessEntries An array of access mask descriptors.
 * \param NumberOfAccessEntries The number of elements in \a AccessEntries.
 */
HPROPSHEETPAGE PhCreateSecurityPage(
    _In_ PWSTR ObjectName,
    _In_ PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    _In_ PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    _In_opt_ PVOID Context,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    )
{
    ISecurityInformation *info;
    HPROPSHEETPAGE page;

    info = PhSecurityInformation_Create(
        ObjectName,
        GetObjectSecurity,
        SetObjectSecurity,
        Context,
        AccessEntries,
        NumberOfAccessEntries,
        TRUE
        );

    page = CreateSecurityPage(info);

    PhSecurityInformation_Release(info);

    return page;
}

/**
 * Displays a security editor dialog.
 *
 * \param hWnd The parent window of the dialog.
 * \param ObjectName The name of the object.
 * \param GetObjectSecurity A callback function executed to retrieve the security descriptor of the
 * object.
 * \param SetObjectSecurity A callback function executed to modify the security descriptor of the
 * object.
 * \param Context A user-defined value to pass to the callback functions.
 * \param AccessEntries An array of access mask descriptors.
 * \param NumberOfAccessEntries The number of elements in \a AccessEntries.
 */
VOID PhEditSecurity(
    _In_ HWND hWnd,
    _In_ PWSTR ObjectName,
    _In_ PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    _In_ PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    _In_opt_ PVOID Context,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    )
{
    ISecurityInformation *info;

    info = PhSecurityInformation_Create(
        ObjectName,
        GetObjectSecurity,
        SetObjectSecurity,
        Context,
        AccessEntries,
        NumberOfAccessEntries,
        FALSE
        );

    if (WindowsVersion >= WINDOWS_8 && PhGetIntegerSetting(L"EnableSecurityAdvancedDialog"))
        EditSecurityAdvanced(hWnd, info, COMBINE_PAGE_ACTIVATION(SI_PAGE_PERM, SI_SHOW_PERM_ACTIVATED));
    else
        EditSecurity(hWnd, info);

    PhSecurityInformation_Release(info);
}

ISecurityInformation *PhSecurityInformation_Create(
    _In_ PWSTR ObjectName,
    _In_ PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    _In_ PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    _In_opt_ PVOID Context,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries,
    _In_ BOOLEAN IsPage
    )
{
    PhSecurityInformation *info;
    ULONG i;

    info = PhAllocate(sizeof(PhSecurityInformation));
    info->VTable = &PhSecurityInformation_VTable;
    info->RefCount = 1;

    info->ObjectName = PhCreateString(ObjectName);
    info->GetObjectSecurity = GetObjectSecurity;
    info->SetObjectSecurity = SetObjectSecurity;
    info->Context = Context;
    info->AccessEntries = PhAllocate(sizeof(SI_ACCESS) * NumberOfAccessEntries);
    info->NumberOfAccessEntries = NumberOfAccessEntries;
    info->IsPage = IsPage;

    for (i = 0; i < NumberOfAccessEntries; i++)
    {
        memset(&info->AccessEntries[i], 0, sizeof(SI_ACCESS));
        info->AccessEntries[i].pszName = AccessEntries[i].Name;
        info->AccessEntries[i].mask = AccessEntries[i].Access;

        if (AccessEntries[i].General)
            info->AccessEntries[i].dwFlags |= SI_ACCESS_GENERAL;
        if (AccessEntries[i].Specific)
            info->AccessEntries[i].dwFlags |= SI_ACCESS_SPECIFIC;
    }

    return (ISecurityInformation *)info;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_QueryInterface(
    _In_ ISecurityInformation *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    )
{
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

            info = PhAllocate(sizeof(PhSecurityInformation2));
            info->VTable = &PhSecurityInformation_VTable2;
            info->RefCount = 1;

            *Object = info;
            return S_OK;
        }
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
        if (this->ObjectName) PhDereferenceObject(this->ObjectName);
        PhFree(this->AccessEntries);

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
    ObjectInfo->dwFlags =
        SI_EDIT_AUDITS |
        SI_EDIT_OWNER |
        SI_EDIT_PERMS |
        SI_ADVANCED;
        //SI_NO_ACL_PROTECT |
        //SI_NO_TREE_APPLY;
    ObjectInfo->hInstance = NULL;
    ObjectInfo->pszObjectName = this->ObjectName->Buffer;

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

    status = this->GetObjectSecurity(
        &securityDescriptor,
        RequestedInformation,
        this->Context
        );

    if (!NT_SUCCESS(status))
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(status));

    sdLength = RtlLengthSecurityDescriptor(securityDescriptor);
    newSd = LocalAlloc(0, sdLength);
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

    status = this->SetObjectSecurity(
        SecurityDescriptor,
        SecurityInformation,
        this->Context
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
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetInheritTypes(
    _In_ ISecurityInformation *This,
    _Out_ PSI_INHERIT_TYPE *InheritTypes,
    _Out_ PULONG InheritTypesCount
    )
{
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

    if (uMsg == PSPCB_SI_INITDIALOG && !this->IsPage)
    {
        // Center the security editor window.
        PhCenterWindow(GetParent(hwnd), GetParent(GetParent(hwnd)));
    }

    return E_NOTIMPL;
}

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
    PhSecurityIDataObject *dataObject;

    dataObject = PhAllocate(sizeof(PhSecurityInformation));
    dataObject->VTable = &PhDataObject_VTable;
    dataObject->RefCount = 1;

    dataObject->SidCount = cSids;
    dataObject->Sids = rgpSids;
    dataObject->NameCache = PhCreateList(1);

    *ppdo = (LPDATAOBJECT)dataObject;

    return S_OK;
}

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
        for (ULONG i = 0; i < this->NameCache->Count; i++)
            PhDereferenceObject(this->NameCache->Items[i]);
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
    sidInfoList->cItems = this->SidCount;

    for (ULONG i = 0; i < this->SidCount; i++)
    {
        SID_INFO sidInfo;
        PPH_STRING sidString;
        SID_NAME_USE sidUse;

        memset(&sidInfo, 0, sizeof(SID_INFO));

        sidInfo.pSid = this->Sids[i];

        if (sidString = PhGetSidFullName(sidInfo.pSid, FALSE, &sidUse))
        {
            switch (sidUse)
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
        else if (sidString = PhSidToStringSid(sidInfo.pSid))
        {
            static PH_STRINGREF appcontainerMappings = PH_STRINGREF_INIT(L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Mappings\\");
            HANDLE keyHandle;
            PPH_STRING keyPath;
            PPH_STRING packageName = NULL;

            if (PhEqualString2(sidString, L"S-1-15-3-4096", FALSE))
            {
                // Special case for Edge and Internet Explorer objects.
                packageName = PhCreateString(L"InternetExplorer (APP_PACKAGE)");
                sidInfo.pwzCommonName = PhGetString(packageName);;
                PhAddItemList(this->NameCache, packageName);
                sidInfoList->aSidInfo[i] = sidInfo;
                continue;
            }

            keyPath = PhConcatStringRef2(&appcontainerMappings, &sidString->sr);

            if (NT_SUCCESS(PhOpenKey(
                &keyHandle,
                KEY_READ,
                PH_KEY_CURRENT_USER,
                &keyPath->sr,
                0
                )))
            {
                packageName = PhQueryRegistryString(keyHandle, L"Moniker");
                NtClose(keyHandle);
            }

            if (packageName)
            {
                PhMoveReference(&packageName, PhFormatString(L"%s (APP_PACKAGE)", PhGetString(packageName)));
                sidInfo.pwzCommonName = PhGetString(packageName);
                PhAddItemList(this->NameCache, packageName);
            }

            PhDereferenceObject(keyPath);
            PhDereferenceObject(sidString);
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
    // QuerySecurity properly.
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
    NTSTATUS status;
    PPH_STD_OBJECT_SECURITY stdObjectSecurity;
    HANDLE handle;

    stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context;

    status = stdObjectSecurity->OpenObject(
        &handle,
        PhGetAccessForGetSecurity(SecurityInformation),
        stdObjectSecurity->Context
        );

    if (!NT_SUCCESS(status))
        return status;

    if (PhEqualStringZ(stdObjectSecurity->ObjectType, L"Service", TRUE))
    {
        status = PhGetSeObjectSecurity(handle, SE_SERVICE, SecurityInformation, SecurityDescriptor);
        CloseServiceHandle(handle);
    }
    else if (PhEqualStringZ(stdObjectSecurity->ObjectType, L"File", TRUE))
    {
        status = PhpGetObjectSecurityWithTimeout(handle, SecurityInformation, SecurityDescriptor);
        NtClose(handle);
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
    NTSTATUS status;
    PPH_STD_OBJECT_SECURITY stdObjectSecurity;
    HANDLE handle;

    stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context;

    status = stdObjectSecurity->OpenObject(
        &handle,
        PhGetAccessForSetSecurity(SecurityInformation),
        stdObjectSecurity->Context
        );

    if (!NT_SUCCESS(status))
        return status;

    if (PhEqualStringZ(stdObjectSecurity->ObjectType, L"Service", TRUE))
    {
        status = PhSetSeObjectSecurity(handle, SE_SERVICE, SecurityInformation, SecurityDescriptor);
        CloseServiceHandle(handle);
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
    ULONG win32Result;
    SECURITY_INFORMATION securityInformation = 0;
    BOOLEAN present;
    BOOLEAN defaulted;
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

    win32Result = SetSecurityInfo(
        Handle,
        ObjectType,
        SecurityInformation,
        owner,
        group,
        dacl,
        sacl
        );

    if (win32Result != ERROR_SUCCESS)
        return NTSTATUS_FROM_WIN32(win32Result);

    return STATUS_SUCCESS;
}
