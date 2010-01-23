/*
 * Process Hacker - 
 *   object security editor
 * 
 * Copyright (C) 2010 wj32
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

#include <phgui.h>
#include <seceditp.h>

ISecurityInformationVtbl PhSecurityInformation_VTable =
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

_CreateSecurityPage CreateSecurityPage_I;
_EditSecurity EditSecurity_I;

VOID PhSecurityEditorInitialization()
{
    LoadLibrary(L"aclui.dll");

    CreateSecurityPage_I = PhGetProcAddress(L"aclui.dll", "CreateSecurityPage");
    EditSecurity_I = PhGetProcAddress(L"aclui.dll", "EditSecurity"); 
}

HPROPSHEETPAGE PhCreateSecurityPage(
    __in PWSTR ObjectName,
    __in PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    __in PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    __in PVOID Context,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
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
        NumberOfAccessEntries
        );

    page = CreateSecurityPage_I(info);

    PhSecurityInformation_Release(info);

    return page;
}

VOID PhEditSecurity(
    __in HWND hWnd,
    __in PWSTR ObjectName,
    __in PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    __in PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    __in PVOID Context,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
    )
{
    ISecurityInformation *info;

    info = PhSecurityInformation_Create(
        ObjectName,
        GetObjectSecurity,
        SetObjectSecurity,
        Context,
        AccessEntries,
        NumberOfAccessEntries
        );

    EditSecurity_I(hWnd, info);

    PhSecurityInformation_Release(info);
}

ISecurityInformation *PhSecurityInformation_Create(
    __in PWSTR ObjectName,
    __in PPH_GET_OBJECT_SECURITY GetObjectSecurity,
    __in PPH_SET_OBJECT_SECURITY SetObjectSecurity,
    __in PVOID Context,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
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
    __in ISecurityInformation *This,
    __in REFIID Riid,
    __out PVOID *Object
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

    *Object = NULL;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation_AddRef(
    __in ISecurityInformation *This
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    this->RefCount++;

    return this->RefCount;
}

ULONG STDMETHODCALLTYPE PhSecurityInformation_Release(
    __in ISecurityInformation *This
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
    __in ISecurityInformation *This,
    __out PSI_OBJECT_INFO ObjectInfo
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    memset(ObjectInfo, 0, sizeof(SI_OBJECT_INFO));
    ObjectInfo->dwFlags =
        SI_EDIT_AUDITS |
        SI_EDIT_OWNER |
        SI_EDIT_PERMS |
        SI_ADVANCED |
        SI_NO_ACL_PROTECT |
        SI_NO_TREE_APPLY;
    ObjectInfo->hInstance = NULL;
    ObjectInfo->pszObjectName = this->ObjectName->Buffer;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetSecurity(
    __in ISecurityInformation *This,
    __in SECURITY_INFORMATION RequestedInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in BOOL Default
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
        return HRESULT_FROM_NT(status);

    sdLength = GetSecurityDescriptorLength(securityDescriptor);
    newSd = LocalAlloc(0, sdLength);
    memcpy(newSd, securityDescriptor, sdLength);
    PhFree(securityDescriptor);

    *SecurityDescriptor = newSd;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_SetSecurity(
    __in ISecurityInformation *This,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
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
        return HRESULT_FROM_NT(status);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetAccessRights(
    __in ISecurityInformation *This,
    __in const GUID *ObjectType,
    __in ULONG Flags,
    __out PSI_ACCESS *Access,
    __out PULONG Accesses,
    __out PULONG DefaultAccess
    )
{
    PhSecurityInformation *this = (PhSecurityInformation *)This;

    *Access = this->AccessEntries;
    *Accesses = this->NumberOfAccessEntries;
    *DefaultAccess = 0;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_MapGeneric(
    __in ISecurityInformation *This,
    __in const GUID *ObjectType,
    __in PUCHAR AceFlags,
    __inout PACCESS_MASK Mask
    )
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetInheritTypes(
    __in ISecurityInformation *This,
    __out PSI_INHERIT_TYPE *InheritTypes,
    __out PULONG InheritTypesCount
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_PropertySheetPageCallback(
    __in ISecurityInformation *This,
    __in HWND hwnd,
    __in UINT uMsg,
    __in SI_PAGE_TYPE uPage
    )
{
    return E_NOTIMPL;
}

NTSTATUS PhStdGetObjectSecurity(
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    )
{
    NTSTATUS status;
    PPH_STD_OBJECT_SECURITY stdObjectSecurity;
    HANDLE handle;

    stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context;

    status = stdObjectSecurity->OpenObject(
        &handle,
        READ_CONTROL |
        ((SecurityInformation & SACL_SECURITY_INFORMATION) ? ACCESS_SYSTEM_SECURITY : 0),
        stdObjectSecurity->Context
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetObjectSecurity(handle, SecurityInformation, SecurityDescriptor);

    CloseHandle(handle);

    return status;
}

NTSTATUS PhStdSetObjectSecurity(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    )
{
    NTSTATUS status;
    PPH_STD_OBJECT_SECURITY stdObjectSecurity;
    HANDLE handle;

    stdObjectSecurity = (PPH_STD_OBJECT_SECURITY)Context;

    status = stdObjectSecurity->OpenObject(
        &handle,
        ((SecurityInformation & DACL_SECURITY_INFORMATION) ? WRITE_DAC : 0) |
        ((SecurityInformation & OWNER_SECURITY_INFORMATION) ? WRITE_OWNER : 0) |
        ((SecurityInformation & SACL_SECURITY_INFORMATION) ? ACCESS_SYSTEM_SECURITY : 0),
        stdObjectSecurity->Context
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhSetObjectSecurity(handle, SecurityInformation, SecurityDescriptor);

    CloseHandle(handle);

    return status;
}
