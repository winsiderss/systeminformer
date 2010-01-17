#ifndef SECEDITP_H
#define SECEDITP_H

#include <aclui.h>

typedef struct
{
    ISecurityInformationVtbl *VTable;

    ULONG RefCount;

    PPH_STRING ObjectName;
} PhSecurityInformation;

ISecurityInformation *PhSecurityInformation_Create(
    __in PWSTR ObjectName
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_QueryInterface(
    __in ISecurityInformation *This,
    __in REFIID Riid,
    __out PVOID *Object
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation_AddRef(
    __in ISecurityInformation *This
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation_Release(
    __in ISecurityInformation *This
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetObjectInformation(
    __in ISecurityInformation *This,
    __out PSI_OBJECT_INFO ObjectInfo
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetSecurity(
    __in ISecurityInformation *This,
    __in SECURITY_INFORMATION RequestedInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in BOOL Default
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_SetSecurity(
    __in ISecurityInformation *This,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetAccessRights(
    __in ISecurityInformation *This,
    __in const GUID *ObjectType,
    __in ULONG Flags,
    __out PSI_ACCESS *Access,
    __out PULONG Accesses,
    __out PULONG DefaultAccess
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_MapGeneric(
    __in ISecurityInformation *This,
    __in const GUID *ObjectType,
    __in PUCHAR AceFlags,
    __in PACCESS_MASK Mask
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetInheritTypes(
    __in ISecurityInformation *This,
    __out PSI_INHERIT_TYPE *InheritTypes,
    __out PULONG InheritTypesCount
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_PropertySheetPageCallback(
    __in ISecurityInformation *This,
    __in HWND hwnd,
    __in UINT uMsg,
    __in SI_PAGE_TYPE uPage
    );

typedef HPROPSHEETPAGE (WINAPI *_CreateSecurityPage)(
    __in LPSECURITYINFO psi
    );

typedef BOOL (WINAPI *_EditSecurity)(
    __in HWND hwndOwner,
    __in LPSECURITYINFO psi
    );

#endif
