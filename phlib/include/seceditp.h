/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2016-2023
 *
 */

#ifndef _PH_SECEDITP_H
#define _PH_SECEDITP_H

typedef struct
{
    ISecurityInformationVtbl *VTable;

    ULONG RefCount;

    HWND WindowHandle;
    BOOLEAN IsPage;
    BOOLEAN HaveGenericMapping;
    PPH_ACCESS_ENTRY AccessEntriesArray;
    PSI_ACCESS AccessEntries;
    ULONG NumberOfAccessEntries;
    GENERIC_MAPPING GenericMapping;

    PPH_STRING ObjectName;
    PPH_STRING ObjectType;
    PPH_OPEN_OBJECT OpenObject;
    PPH_CLOSE_OBJECT CloseObject;
    PVOID Context;
} PhSecurityInformation;

typedef struct
{
    ISecurityInformation2Vtbl *VTable;

    PhSecurityInformation *Context;
    ULONG RefCount;
} PhSecurityInformation2;

typedef struct
{
    ISecurityInformation3Vtbl *VTable;

    PhSecurityInformation *Context;
    ULONG RefCount;
} PhSecurityInformation3;

typedef struct
{
    IDataObjectVtbl *VTable;

    PhSecurityInformation *Context;
    ULONG RefCount;
    ULONG SidCount;
    PSID *Sids;
    PPH_LIST NameCache;
} PhSecurityIDataObject;

typedef struct
{
    IEffectivePermissionVtbl *VTable;

    PhSecurityInformation *Context;
    ULONG RefCount;
} PhEffectivePermission;

#undef INTERFACE
#define INTERFACE   ISecurityObjectTypeInfoEx
DECLARE_INTERFACE_IID_(ISecurityObjectTypeInfoEx, IUnknown, "FC3066EB-79EF-444b-9111-D18A75EBF2FA")
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ _In_ REFIID riid, _Outptr_ void** ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;

    // *** ISecurityInformation methods ***
    STDMETHOD(GetInheritSource)(THIS_ SECURITY_INFORMATION si,
        PACL pACL,
        PINHERITED_FROM * ppInheritArray) PURE;
};
typedef ISecurityObjectTypeInfoEx* LPSecurityObjectTypeInfoEx;

typedef struct
{
    ISecurityObjectTypeInfoExVtbl* VTable;

    PhSecurityInformation* Context;
    ULONG RefCount;
} PhSecurityObjectTypeInfo;

// ISecurityInformation

ISecurityInformation *PhSecurityInformation_Create(
    _In_opt_ HWND WindowHandle,
    _In_ PWSTR ObjectName,
    _In_ PWSTR ObjectType,
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PPH_CLOSE_OBJECT CloseObject,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN IsPage
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_QueryInterface(
    _In_ ISecurityInformation *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation_AddRef(
    _In_ ISecurityInformation *This
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation_Release(
    _In_ ISecurityInformation *This
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetObjectInformation(
    _In_ ISecurityInformation *This,
    _Out_ PSI_OBJECT_INFO ObjectInfo
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetSecurity(
    _In_ ISecurityInformation *This,
    _In_ SECURITY_INFORMATION RequestedInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ BOOL Default
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_SetSecurity(
    _In_ ISecurityInformation *This,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetAccessRights(
    _In_ ISecurityInformation *This,
    _In_ const GUID *ObjectType,
    _In_ ULONG Flags,
    _Out_ PSI_ACCESS *Access,
    _Out_ PULONG Accesses,
    _Out_ PULONG DefaultAccess
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_MapGeneric(
    _In_ ISecurityInformation *This,
    _In_ const GUID *ObjectType,
    _In_ PUCHAR AceFlags,
    _Inout_ PACCESS_MASK Mask
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_GetInheritTypes(
    _In_ ISecurityInformation *This,
    _Out_ PSI_INHERIT_TYPE *InheritTypes,
    _Out_ PULONG InheritTypesCount
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation_PropertySheetPageCallback(
    _In_ ISecurityInformation *This,
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ SI_PAGE_TYPE uPage
    );

// ISecurityInformation2

HRESULT STDMETHODCALLTYPE PhSecurityInformation2_QueryInterface(
    _In_ ISecurityInformation2 *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation2_AddRef(
    _In_ ISecurityInformation2 *This
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation2_Release(
    _In_ ISecurityInformation2 *This
    );

BOOL STDMETHODCALLTYPE PhSecurityInformation2_IsDaclCanonical(
    _In_ ISecurityInformation2 *This,
    _In_ PACL pDacl
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation2_LookupSids(
    _In_ ISecurityInformation2 *This,
    _In_ ULONG cSids,
    _In_ PSID *rgpSids,
    _Out_ LPDATAOBJECT *ppdo
    );

// ISecurityInformation3

HRESULT STDMETHODCALLTYPE PhSecurityInformation3_QueryInterface(
    _In_ ISecurityInformation3 *This,
    _In_ REFIID Riid,
    _Out_ PVOID *Object
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation3_AddRef(
    _In_ ISecurityInformation3 *This
    );

ULONG STDMETHODCALLTYPE PhSecurityInformation3_Release(
    _In_ ISecurityInformation3 *This
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation3_GetFullResourceName(
    _In_ ISecurityInformation3 *This,
    _Outptr_ PWSTR *ppszResourceName
    );

HRESULT STDMETHODCALLTYPE PhSecurityInformation3_OpenElevatedEditor(
    _In_ ISecurityInformation3 *This,
    _In_ HWND hWnd,
    _In_ SI_PAGE_TYPE uPage
    );

// IDataObject

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_QueryInterface(
    _In_ IDataObject *This,
    _In_ REFIID Riid,
    _COM_Outptr_ PVOID *Object
    );

ULONG STDMETHODCALLTYPE PhSecurityDataObject_AddRef(
    _In_ IDataObject *This
    );

ULONG STDMETHODCALLTYPE PhSecurityDataObject_Release(
    _In_ IDataObject *This
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_GetData(
    _In_ IDataObject *This,
    _In_ FORMATETC *pformatetcIn,
    _Out_ STGMEDIUM *pmedium);

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_GetDataHere(
    _In_ IDataObject *This,
    _In_ FORMATETC *pformatetc,
    _Inout_ STGMEDIUM *pmedium
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_QueryGetData(
    _In_ IDataObject *This,
    _In_opt_ FORMATETC *pformatetc
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_GetCanonicalFormatEtc(
    _In_ IDataObject *This,
    _In_opt_ FORMATETC *pformatectIn,
    _Out_ FORMATETC *pformatetcOut
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_SetData(
    _In_ IDataObject *This,
    _In_ FORMATETC *pformatetc,
    _In_ STGMEDIUM *pmedium,
    _In_ BOOL fRelease
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_EnumFormatEtc(
    _In_ IDataObject *This,
    _In_ ULONG dwDirection,
    _Out_opt_ IEnumFORMATETC **ppenumFormatEtc
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_DAdvise(
    _In_ IDataObject *This,
    _In_ FORMATETC *pformatetc,
    _In_ ULONG advf,
    _In_opt_ IAdviseSink *pAdvSink,
    _Out_ ULONG *pdwConnection
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_DUnadvise(
    _In_ IDataObject *This,
    _In_ ULONG dwConnection
    );

HRESULT STDMETHODCALLTYPE PhSecurityDataObject_EnumDAdvise(
    _In_ IDataObject *This,
    _Out_opt_ IEnumSTATDATA **ppenumAdvise
    );

// ISecurityObjectTypeInfo

HRESULT STDMETHODCALLTYPE PhSecurityObjectTypeInfo_QueryInterface(
    _In_ ISecurityObjectTypeInfoEx* This,
    _In_ REFIID Riid,
    _Out_ PVOID* Object
    );

ULONG STDMETHODCALLTYPE PhSecurityObjectTypeInfo_AddRef(
    _In_ ISecurityObjectTypeInfoEx* This
    );

ULONG STDMETHODCALLTYPE PhSecurityObjectTypeInfo_Release(
    _In_ ISecurityObjectTypeInfoEx* This
    );

HRESULT STDMETHODCALLTYPE PhSecurityObjectTypeInfo_GetInheritSource(
    _In_ ISecurityObjectTypeInfoEx* This,
    _In_ SECURITY_INFORMATION SecurityInfo,
    _In_ PACL Acl,
    _Out_ PINHERITED_FROM *InheritArray
    );

// IEffectivePermission

HRESULT STDMETHODCALLTYPE PhEffectivePermission_QueryInterface(
    _In_ IEffectivePermission* This,
    _In_ REFIID Riid,
    _Out_ PVOID* Object
    );

ULONG STDMETHODCALLTYPE PhEffectivePermission_AddRef(
    _In_ IEffectivePermission* This
    );

ULONG STDMETHODCALLTYPE PhEffectivePermission_Release(
    _In_ IEffectivePermission* This
    );

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
    );

#endif
