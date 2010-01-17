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

VOID PhEditSecurity(
    __in HWND hWnd,
    __in PWSTR ObjectName
    )
{
    ISecurityInformation *info;

    info = PhSecurityInformation_Create(ObjectName);

    EditSecurity_I(hWnd, info);

    PhSecurityInformation_Release(info);
}

ISecurityInformation *PhSecurityInformation_Create(
    __in PWSTR ObjectName
    )
{
    PhSecurityInformation *info;

    info = PhAllocate(sizeof(PhSecurityInformation));
    info->VTable = &PhSecurityInformation_VTable;
    info->RefCount = 1;

    info->ObjectName = PhCreateString(ObjectName);

    return (ISecurityInformation *)info;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_QueryInterface(
    __in ISecurityInformation *This,
    __in REFIID Riid,
    __out PVOID *Object
    )
{
    if (IsEqualIID(Riid, &IID_ISecurityInformation))
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
    memset(ObjectInfo, 0, sizeof(SI_OBJECT_INFO));
    ObjectInfo->dwFlags =
        SI_EDIT_AUDITS |
        SI_EDIT_OWNER |
        SI_EDIT_PERMS |
        SI_ADVANCED |
        SI_NO_ACL_PROTECT |
        SI_NO_TREE_APPLY;
    ObjectInfo->hInstance = NULL;
    ObjectInfo->pszObjectName = L"Object Name Here";

    return S_OK;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetSecurity(
    __in ISecurityInformation *This,
    __in SECURITY_INFORMATION RequestedInformation,
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in BOOL Default
    )
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_SetSecurity(
    __in ISecurityInformation *This,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    return E_NOTIMPL;
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
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE PhSecurityInformation_MapGeneric(
    __in ISecurityInformation *This,
    __in const GUID *ObjectType,
    __in PUCHAR AceFlags,
    __in PACCESS_MASK Mask
    )
{
    return E_NOTIMPL;
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
