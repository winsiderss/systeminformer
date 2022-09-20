

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __mscoree_h__
#define __mscoree_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IObjectHandle_FWD_DEFINED__
#define __IObjectHandle_FWD_DEFINED__
typedef interface IObjectHandle IObjectHandle;

#endif 	/* __IObjectHandle_FWD_DEFINED__ */


#ifndef __IAppDomainBinding_FWD_DEFINED__
#define __IAppDomainBinding_FWD_DEFINED__
typedef interface IAppDomainBinding IAppDomainBinding;

#endif 	/* __IAppDomainBinding_FWD_DEFINED__ */


#ifndef __IGCThreadControl_FWD_DEFINED__
#define __IGCThreadControl_FWD_DEFINED__
typedef interface IGCThreadControl IGCThreadControl;

#endif 	/* __IGCThreadControl_FWD_DEFINED__ */


#ifndef __IGCHostControl_FWD_DEFINED__
#define __IGCHostControl_FWD_DEFINED__
typedef interface IGCHostControl IGCHostControl;

#endif 	/* __IGCHostControl_FWD_DEFINED__ */


#ifndef __ICorThreadpool_FWD_DEFINED__
#define __ICorThreadpool_FWD_DEFINED__
typedef interface ICorThreadpool ICorThreadpool;

#endif 	/* __ICorThreadpool_FWD_DEFINED__ */


#ifndef __IDebuggerThreadControl_FWD_DEFINED__
#define __IDebuggerThreadControl_FWD_DEFINED__
typedef interface IDebuggerThreadControl IDebuggerThreadControl;

#endif 	/* __IDebuggerThreadControl_FWD_DEFINED__ */


#ifndef __IDebuggerInfo_FWD_DEFINED__
#define __IDebuggerInfo_FWD_DEFINED__
typedef interface IDebuggerInfo IDebuggerInfo;

#endif 	/* __IDebuggerInfo_FWD_DEFINED__ */


#ifndef __ICorConfiguration_FWD_DEFINED__
#define __ICorConfiguration_FWD_DEFINED__
typedef interface ICorConfiguration ICorConfiguration;

#endif 	/* __ICorConfiguration_FWD_DEFINED__ */


#ifndef __ICorRuntimeHost_FWD_DEFINED__
#define __ICorRuntimeHost_FWD_DEFINED__
typedef interface ICorRuntimeHost ICorRuntimeHost;

#endif 	/* __ICorRuntimeHost_FWD_DEFINED__ */


#ifndef __ICLRMemoryNotificationCallback_FWD_DEFINED__
#define __ICLRMemoryNotificationCallback_FWD_DEFINED__
typedef interface ICLRMemoryNotificationCallback ICLRMemoryNotificationCallback;

#endif 	/* __ICLRMemoryNotificationCallback_FWD_DEFINED__ */


#ifndef __IHostMalloc_FWD_DEFINED__
#define __IHostMalloc_FWD_DEFINED__
typedef interface IHostMalloc IHostMalloc;

#endif 	/* __IHostMalloc_FWD_DEFINED__ */


#ifndef __IHostMemoryManager_FWD_DEFINED__
#define __IHostMemoryManager_FWD_DEFINED__
typedef interface IHostMemoryManager IHostMemoryManager;

#endif 	/* __IHostMemoryManager_FWD_DEFINED__ */


#ifndef __ICLRTask_FWD_DEFINED__
#define __ICLRTask_FWD_DEFINED__
typedef interface ICLRTask ICLRTask;

#endif 	/* __ICLRTask_FWD_DEFINED__ */


#ifndef __ICLRTask2_FWD_DEFINED__
#define __ICLRTask2_FWD_DEFINED__
typedef interface ICLRTask2 ICLRTask2;

#endif 	/* __ICLRTask2_FWD_DEFINED__ */


#ifndef __IHostTask_FWD_DEFINED__
#define __IHostTask_FWD_DEFINED__
typedef interface IHostTask IHostTask;

#endif 	/* __IHostTask_FWD_DEFINED__ */


#ifndef __ICLRTaskManager_FWD_DEFINED__
#define __ICLRTaskManager_FWD_DEFINED__
typedef interface ICLRTaskManager ICLRTaskManager;

#endif 	/* __ICLRTaskManager_FWD_DEFINED__ */


#ifndef __IHostTaskManager_FWD_DEFINED__
#define __IHostTaskManager_FWD_DEFINED__
typedef interface IHostTaskManager IHostTaskManager;

#endif 	/* __IHostTaskManager_FWD_DEFINED__ */


#ifndef __IHostThreadpoolManager_FWD_DEFINED__
#define __IHostThreadpoolManager_FWD_DEFINED__
typedef interface IHostThreadpoolManager IHostThreadpoolManager;

#endif 	/* __IHostThreadpoolManager_FWD_DEFINED__ */


#ifndef __ICLRIoCompletionManager_FWD_DEFINED__
#define __ICLRIoCompletionManager_FWD_DEFINED__
typedef interface ICLRIoCompletionManager ICLRIoCompletionManager;

#endif 	/* __ICLRIoCompletionManager_FWD_DEFINED__ */


#ifndef __IHostIoCompletionManager_FWD_DEFINED__
#define __IHostIoCompletionManager_FWD_DEFINED__
typedef interface IHostIoCompletionManager IHostIoCompletionManager;

#endif 	/* __IHostIoCompletionManager_FWD_DEFINED__ */


#ifndef __ICLRDebugManager_FWD_DEFINED__
#define __ICLRDebugManager_FWD_DEFINED__
typedef interface ICLRDebugManager ICLRDebugManager;

#endif 	/* __ICLRDebugManager_FWD_DEFINED__ */


#ifndef __ICLRErrorReportingManager_FWD_DEFINED__
#define __ICLRErrorReportingManager_FWD_DEFINED__
typedef interface ICLRErrorReportingManager ICLRErrorReportingManager;

#endif 	/* __ICLRErrorReportingManager_FWD_DEFINED__ */


#ifndef __IHostCrst_FWD_DEFINED__
#define __IHostCrst_FWD_DEFINED__
typedef interface IHostCrst IHostCrst;

#endif 	/* __IHostCrst_FWD_DEFINED__ */


#ifndef __IHostAutoEvent_FWD_DEFINED__
#define __IHostAutoEvent_FWD_DEFINED__
typedef interface IHostAutoEvent IHostAutoEvent;

#endif 	/* __IHostAutoEvent_FWD_DEFINED__ */


#ifndef __IHostManualEvent_FWD_DEFINED__
#define __IHostManualEvent_FWD_DEFINED__
typedef interface IHostManualEvent IHostManualEvent;

#endif 	/* __IHostManualEvent_FWD_DEFINED__ */


#ifndef __IHostSemaphore_FWD_DEFINED__
#define __IHostSemaphore_FWD_DEFINED__
typedef interface IHostSemaphore IHostSemaphore;

#endif 	/* __IHostSemaphore_FWD_DEFINED__ */


#ifndef __ICLRSyncManager_FWD_DEFINED__
#define __ICLRSyncManager_FWD_DEFINED__
typedef interface ICLRSyncManager ICLRSyncManager;

#endif 	/* __ICLRSyncManager_FWD_DEFINED__ */


#ifndef __IHostSyncManager_FWD_DEFINED__
#define __IHostSyncManager_FWD_DEFINED__
typedef interface IHostSyncManager IHostSyncManager;

#endif 	/* __IHostSyncManager_FWD_DEFINED__ */


#ifndef __ICLRPolicyManager_FWD_DEFINED__
#define __ICLRPolicyManager_FWD_DEFINED__
typedef interface ICLRPolicyManager ICLRPolicyManager;

#endif 	/* __ICLRPolicyManager_FWD_DEFINED__ */


#ifndef __IHostPolicyManager_FWD_DEFINED__
#define __IHostPolicyManager_FWD_DEFINED__
typedef interface IHostPolicyManager IHostPolicyManager;

#endif 	/* __IHostPolicyManager_FWD_DEFINED__ */


#ifndef __IActionOnCLREvent_FWD_DEFINED__
#define __IActionOnCLREvent_FWD_DEFINED__
typedef interface IActionOnCLREvent IActionOnCLREvent;

#endif 	/* __IActionOnCLREvent_FWD_DEFINED__ */


#ifndef __ICLROnEventManager_FWD_DEFINED__
#define __ICLROnEventManager_FWD_DEFINED__
typedef interface ICLROnEventManager ICLROnEventManager;

#endif 	/* __ICLROnEventManager_FWD_DEFINED__ */


#ifndef __IHostGCManager_FWD_DEFINED__
#define __IHostGCManager_FWD_DEFINED__
typedef interface IHostGCManager IHostGCManager;

#endif 	/* __IHostGCManager_FWD_DEFINED__ */


#ifndef __ICLRAssemblyReferenceList_FWD_DEFINED__
#define __ICLRAssemblyReferenceList_FWD_DEFINED__
typedef interface ICLRAssemblyReferenceList ICLRAssemblyReferenceList;

#endif 	/* __ICLRAssemblyReferenceList_FWD_DEFINED__ */


#ifndef __ICLRReferenceAssemblyEnum_FWD_DEFINED__
#define __ICLRReferenceAssemblyEnum_FWD_DEFINED__
typedef interface ICLRReferenceAssemblyEnum ICLRReferenceAssemblyEnum;

#endif 	/* __ICLRReferenceAssemblyEnum_FWD_DEFINED__ */


#ifndef __ICLRProbingAssemblyEnum_FWD_DEFINED__
#define __ICLRProbingAssemblyEnum_FWD_DEFINED__
typedef interface ICLRProbingAssemblyEnum ICLRProbingAssemblyEnum;

#endif 	/* __ICLRProbingAssemblyEnum_FWD_DEFINED__ */


#ifndef __ICLRAssemblyIdentityManager_FWD_DEFINED__
#define __ICLRAssemblyIdentityManager_FWD_DEFINED__
typedef interface ICLRAssemblyIdentityManager ICLRAssemblyIdentityManager;

#endif 	/* __ICLRAssemblyIdentityManager_FWD_DEFINED__ */


#ifndef __ICLRHostBindingPolicyManager_FWD_DEFINED__
#define __ICLRHostBindingPolicyManager_FWD_DEFINED__
typedef interface ICLRHostBindingPolicyManager ICLRHostBindingPolicyManager;

#endif 	/* __ICLRHostBindingPolicyManager_FWD_DEFINED__ */


#ifndef __ICLRGCManager_FWD_DEFINED__
#define __ICLRGCManager_FWD_DEFINED__
typedef interface ICLRGCManager ICLRGCManager;

#endif 	/* __ICLRGCManager_FWD_DEFINED__ */


#ifndef __ICLRGCManager2_FWD_DEFINED__
#define __ICLRGCManager2_FWD_DEFINED__
typedef interface ICLRGCManager2 ICLRGCManager2;

#endif 	/* __ICLRGCManager2_FWD_DEFINED__ */


#ifndef __IHostAssemblyStore_FWD_DEFINED__
#define __IHostAssemblyStore_FWD_DEFINED__
typedef interface IHostAssemblyStore IHostAssemblyStore;

#endif 	/* __IHostAssemblyStore_FWD_DEFINED__ */


#ifndef __IHostAssemblyManager_FWD_DEFINED__
#define __IHostAssemblyManager_FWD_DEFINED__
typedef interface IHostAssemblyManager IHostAssemblyManager;

#endif 	/* __IHostAssemblyManager_FWD_DEFINED__ */


#ifndef __IHostControl_FWD_DEFINED__
#define __IHostControl_FWD_DEFINED__
typedef interface IHostControl IHostControl;

#endif 	/* __IHostControl_FWD_DEFINED__ */


#ifndef __ICLRControl_FWD_DEFINED__
#define __ICLRControl_FWD_DEFINED__
typedef interface ICLRControl ICLRControl;

#endif 	/* __ICLRControl_FWD_DEFINED__ */


#ifndef __ICLRRuntimeHost_FWD_DEFINED__
#define __ICLRRuntimeHost_FWD_DEFINED__
typedef interface ICLRRuntimeHost ICLRRuntimeHost;

#endif 	/* __ICLRRuntimeHost_FWD_DEFINED__ */


#ifndef __ICLRHostProtectionManager_FWD_DEFINED__
#define __ICLRHostProtectionManager_FWD_DEFINED__
typedef interface ICLRHostProtectionManager ICLRHostProtectionManager;

#endif 	/* __ICLRHostProtectionManager_FWD_DEFINED__ */


#ifndef __ICLRDomainManager_FWD_DEFINED__
#define __ICLRDomainManager_FWD_DEFINED__
typedef interface ICLRDomainManager ICLRDomainManager;

#endif 	/* __ICLRDomainManager_FWD_DEFINED__ */


#ifndef __ITypeName_FWD_DEFINED__
#define __ITypeName_FWD_DEFINED__
typedef interface ITypeName ITypeName;

#endif 	/* __ITypeName_FWD_DEFINED__ */


#ifndef __ITypeNameBuilder_FWD_DEFINED__
#define __ITypeNameBuilder_FWD_DEFINED__
typedef interface ITypeNameBuilder ITypeNameBuilder;

#endif 	/* __ITypeNameBuilder_FWD_DEFINED__ */


#ifndef __ITypeNameFactory_FWD_DEFINED__
#define __ITypeNameFactory_FWD_DEFINED__
typedef interface ITypeNameFactory ITypeNameFactory;

#endif 	/* __ITypeNameFactory_FWD_DEFINED__ */


#ifndef __IApartmentCallback_FWD_DEFINED__
#define __IApartmentCallback_FWD_DEFINED__
typedef interface IApartmentCallback IApartmentCallback;

#endif 	/* __IApartmentCallback_FWD_DEFINED__ */


#ifndef __IManagedObject_FWD_DEFINED__
#define __IManagedObject_FWD_DEFINED__
typedef interface IManagedObject IManagedObject;

#endif 	/* __IManagedObject_FWD_DEFINED__ */


#ifndef __ICatalogServices_FWD_DEFINED__
#define __ICatalogServices_FWD_DEFINED__
typedef interface ICatalogServices ICatalogServices;

#endif 	/* __ICatalogServices_FWD_DEFINED__ */


#ifndef __ComCallUnmarshal_FWD_DEFINED__
#define __ComCallUnmarshal_FWD_DEFINED__

#ifdef __cplusplus
typedef class ComCallUnmarshal ComCallUnmarshal;
#else
typedef struct ComCallUnmarshal ComCallUnmarshal;
#endif /* __cplusplus */

#endif 	/* __ComCallUnmarshal_FWD_DEFINED__ */


#ifndef __ComCallUnmarshalV4_FWD_DEFINED__
#define __ComCallUnmarshalV4_FWD_DEFINED__

#ifdef __cplusplus
typedef class ComCallUnmarshalV4 ComCallUnmarshalV4;
#else
typedef struct ComCallUnmarshalV4 ComCallUnmarshalV4;
#endif /* __cplusplus */

#endif 	/* __ComCallUnmarshalV4_FWD_DEFINED__ */


#ifndef __CorRuntimeHost_FWD_DEFINED__
#define __CorRuntimeHost_FWD_DEFINED__

#ifdef __cplusplus
typedef class CorRuntimeHost CorRuntimeHost;
#else
typedef struct CorRuntimeHost CorRuntimeHost;
#endif /* __cplusplus */

#endif 	/* __CorRuntimeHost_FWD_DEFINED__ */


#ifndef __CLRRuntimeHost_FWD_DEFINED__
#define __CLRRuntimeHost_FWD_DEFINED__

#ifdef __cplusplus
typedef class CLRRuntimeHost CLRRuntimeHost;
#else
typedef struct CLRRuntimeHost CLRRuntimeHost;
#endif /* __cplusplus */

#endif 	/* __CLRRuntimeHost_FWD_DEFINED__ */


#ifndef __TypeNameFactory_FWD_DEFINED__
#define __TypeNameFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class TypeNameFactory TypeNameFactory;
#else
typedef struct TypeNameFactory TypeNameFactory;
#endif /* __cplusplus */

#endif 	/* __TypeNameFactory_FWD_DEFINED__ */


#ifndef __IHostSecurityContext_FWD_DEFINED__
#define __IHostSecurityContext_FWD_DEFINED__
typedef interface IHostSecurityContext IHostSecurityContext;

#endif 	/* __IHostSecurityContext_FWD_DEFINED__ */


#ifndef __IHostSecurityManager_FWD_DEFINED__
#define __IHostSecurityManager_FWD_DEFINED__
typedef interface IHostSecurityManager IHostSecurityManager;

#endif 	/* __IHostSecurityManager_FWD_DEFINED__ */


#ifndef __ICLRAppDomainResourceMonitor_FWD_DEFINED__
#define __ICLRAppDomainResourceMonitor_FWD_DEFINED__
typedef interface ICLRAppDomainResourceMonitor ICLRAppDomainResourceMonitor;

#endif 	/* __ICLRAppDomainResourceMonitor_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "gchost.h"
#include "ivalidator.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_mscoree_0000_0000 */
/* [local] */ 

#ifndef USE_DEPRECATED_CLR_API_WITHOUT_WARNING
#define DEPRECATED_CLR_API_MESG "This API has been deprecated. Refer to https://go.microsoft.com/fwlink/?LinkId=143720 for more details."
#define DECLARE_DEPRECATED __declspec(deprecated(DEPRECATED_CLR_API_MESG))
#define DEPRECATED_CLR_STDAPI EXTERN_C DECLARE_DEPRECATED HRESULT STDAPICALLTYPE
#else // USE_DEPRECATED_CLR_API_WITHOUT_WARNING
#define DECLARE_DEPRECATED 
#define DEPRECATED_CLR_STDAPI STDAPI
#endif // !USE_DEPRECATED_CLR_API_WITHOUT_WARNING

#define	CLR_MAJOR_VERSION	( 4 )

#define	CLR_MINOR_VERSION	( 0 )

#define	CLR_BUILD_VERSION	( 30319 )

#define	CLR_ASSEMBLY_MAJOR_VERSION	( 4 )

#define	CLR_ASSEMBLY_MINOR_VERSION	( 0 )

#define	CLR_ASSEMBLY_BUILD_VERSION	( 0 )

EXTERN_GUID(LIBID_mscoree, 0x5477469e,0x83b1,0x11d2,0x8b,0x49,0x00,0xa0,0xc9,0xb7,0xc9,0xc4);
EXTERN_GUID(CLSID_CorRuntimeHost, 0xcb2f6723, 0xab3a, 0x11d2, 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e);
EXTERN_GUID(CLSID_TypeNameFactory, 0xB81FF171, 0x20F3, 0x11d2, 0x8d, 0xcc, 0x00, 0xa0, 0xc9, 0xb0, 0x05, 0x25);
EXTERN_GUID(CLSID_CLRRuntimeHost, 0x90F1A06E, 0x7712, 0x4762, 0x86, 0xB5, 0x7A, 0x5E, 0xBA, 0x6B, 0xDB, 0x02);
EXTERN_GUID(CLSID_ComCallUnmarshal, 0x3F281000,0xE95A,0x11d2,0x88,0x6B,0x00,0xC0,0x4F,0x86,0x9F,0x04);
EXTERN_GUID(CLSID_ComCallUnmarshalV4, 0x45fb4600,0xe6e8,0x4928,0xb2,0x5e,0x50,0x47,0x6f,0xf7,0x94,0x25);
EXTERN_GUID(IID_IObjectHandle, 0xc460e2b4, 0xe199, 0x412a, 0x84, 0x56, 0x84, 0xdc, 0x3e, 0x48, 0x38, 0xc3);
EXTERN_GUID(IID_IManagedObject, 0xc3fcc19e, 0xa970, 0x11d2, 0x8b, 0x5a, 0x00, 0xa0, 0xc9, 0xb7, 0xc9, 0xc4);
EXTERN_GUID(IID_IApartmentCallback, 0x178e5337, 0x1528, 0x4591, 0xb1, 0xc9, 0x1c, 0x6e, 0x48, 0x46, 0x86, 0xd8);
EXTERN_GUID(IID_ICatalogServices, 0x04c6be1e, 0x1db1, 0x4058, 0xab, 0x7a, 0x70, 0x0c, 0xcc, 0xfb, 0xf2, 0x54);
EXTERN_GUID(IID_ICorRuntimeHost, 0xcb2f6722, 0xab3a, 0x11d2, 0x9c, 0x40, 0x00, 0xc0, 0x4f, 0xa3, 0x0a, 0x3e);
EXTERN_GUID(IID_ICorThreadpool, 0x84680D3A, 0xB2C1, 0x46e8, 0xAC, 0xC2, 0xDB, 0xC0, 0xA3, 0x59, 0x15, 0x9A);
EXTERN_GUID(IID_ICLRDebugManager, 0xdcaec6, 0x2ac0, 0x43a9, 0xac, 0xf9, 0x1e, 0x36, 0xc1, 0x39, 0xb1, 0xd);
EXTERN_GUID(IID_IHostMemoryNeededCallback, 0x47EB8E57, 0x0846, 0x4546, 0xAF, 0x76, 0x6F, 0x42, 0xFC, 0xFC, 0x26, 0x49);
EXTERN_GUID(IID_IHostMalloc, 0x1831991C, 0xCC53, 0x4A31, 0xB2, 0x18, 0x04, 0xE9, 0x10, 0x44, 0x64, 0x79);
EXTERN_GUID(IID_IHostMemoryManager, 0x7BC698D1, 0xF9E3, 0x4460, 0x9C, 0xDE, 0xD0, 0x42, 0x48, 0xE9, 0xFA, 0x25);
EXTERN_GUID(IID_ICLRTask, 0x28E66A4A, 0x9906, 0x4225, 0xB2, 0x31, 0x91, 0x87, 0xc3, 0xeb, 0x86, 0x11);
EXTERN_GUID(IID_ICLRTask2, 0x28E66A4A, 0x9906, 0x4225, 0xB2, 0x31, 0x91, 0x87, 0xc3, 0xeb, 0x86, 0x12);
EXTERN_GUID(IID_IHostTask, 0xC2275828, 0xC4B1, 0x4B55, 0x82, 0xC9, 0x92, 0x13, 0x5F, 0x74, 0xDF, 0x1A);
EXTERN_GUID(IID_ICLRTaskManager, 0x4862efbe, 0x3ae5, 0x44f8, 0x8F, 0xEB, 0x34, 0x61, 0x90, 0xeE, 0x8A, 0x34);
EXTERN_GUID(IID_IHostTaskManager, 0x997FF24C, 0x43B7, 0x4352, 0x86, 0x67, 0x0D, 0xC0, 0x4F, 0xAF, 0xD3, 0x54);
EXTERN_GUID(IID_IHostThreadpoolManager, 0x983D50E2, 0xCB15, 0x466B, 0x80, 0xFC, 0x84, 0x5D, 0xC6, 0xE8, 0xC5, 0xFD);
EXTERN_GUID(IID_ICLRIoCompletionManager, 0x2D74CE86, 0xB8D6, 0x4C84, 0xB3, 0xA7, 0x97, 0x68, 0x93, 0x3B, 0x3C, 0x12);
EXTERN_GUID(IID_IHostIoCompletionManager, 0x8BDE9D80, 0xEC06, 0x41D6, 0x83, 0xE6, 0x22, 0x58, 0x0E, 0xFF, 0xCC, 0x20);
EXTERN_GUID(IID_IHostSyncManager, 0x234330c7, 0x5f10, 0x4f20, 0x96, 0x15, 0x51, 0x22, 0xda, 0xb7, 0xa0, 0xac);
EXTERN_GUID(IID_IHostCrst, 0x6DF710A6, 0x26A4, 0x4a65, 0x8c, 0xd5, 0x72, 0x37, 0xb8, 0xbd, 0xa8, 0xdc);
EXTERN_GUID(IID_IHostAutoEvent, 0x50B0CFCE, 0x4063, 0x4278, 0x96, 0x73, 0xe5, 0xcb, 0x4e, 0xd0, 0xbd, 0xb8);
EXTERN_GUID(IID_IHostManualEvent, 0x1BF4EC38, 0xAFFE, 0x4fb9, 0x85, 0xa6, 0x52, 0x52, 0x68, 0xf1, 0x5b, 0x54);
EXTERN_GUID(IID_IHostSemaphore, 0x855efd47, 0xcc09, 0x463a, 0xa9, 0x7d, 0x16, 0xac, 0xab, 0x88, 0x26, 0x61);
EXTERN_GUID(IID_ICLRSyncManager, 0x55FF199D, 0xAD21, 0x48f9, 0xa1, 0x6c, 0xf2, 0x4e, 0xbb, 0xb8, 0x72, 0x7d);
EXTERN_GUID(IID_ICLRAppDomainResourceMonitor, 0XC62DE18C, 0X2E23, 0X4AEA, 0X84, 0X23, 0XB4, 0X0C, 0X1F, 0XC5, 0X9E, 0XAE);
EXTERN_GUID(IID_ICLRPolicyManager, 0x7D290010, 0xD781, 0x45da, 0xA6, 0xF8, 0xAA, 0x5D, 0x71, 0x1A, 0x73, 0x0E);
EXTERN_GUID(IID_ICLRGCManager, 0x54D9007E, 0xA8E2, 0x4885, 0xB7, 0xBF, 0xF9, 0x98, 0xDE, 0xEE, 0x4F, 0x2A);
EXTERN_GUID(IID_ICLRGCManager2, 0x0603B793, 0xA97A, 0x4712, 0x9C, 0xB4, 0x0C, 0xD1, 0xC7, 0x4C, 0x0F, 0x7C);
EXTERN_GUID(IID_ICLRErrorReportingManager, 0x980d2f1a, 0xbf79, 0x4c08, 0x81, 0x2a, 0xbb, 0x97, 0x78, 0x92, 0x8f, 0x78);
EXTERN_GUID(IID_IHostPolicyManager, 0x7AE49844, 0xB1E3, 0x4683, 0xBA, 0x7C, 0x1E, 0x82, 0x12, 0xEA, 0x3B, 0x79);
EXTERN_GUID(IID_IHostGCManager, 0x5D4EC34E, 0xF248, 0x457B, 0xB6, 0x03, 0x25, 0x5F, 0xAA, 0xBA, 0x0D, 0x21);
EXTERN_GUID(IID_IActionOnCLREvent, 0x607BE24B, 0xD91B, 0x4E28, 0xA2, 0x42, 0x61, 0x87, 0x1C, 0xE5, 0x6E, 0x35);
EXTERN_GUID(IID_ICLROnEventManager, 0x1D0E0132, 0xE64F, 0x493D, 0x92, 0x60, 0x02, 0x5C, 0x0E, 0x32, 0xC1, 0x75);
EXTERN_GUID(IID_ICLRRuntimeHost, 0x90F1A06C, 0x7712, 0x4762, 0x86, 0xB5, 0x7A, 0x5E, 0xBA, 0x6B, 0xDB, 0x02);
EXTERN_GUID(IID_ICLRHostProtectionManager, 0x89f25f5c, 0xceef, 0x43e1, 0x9c, 0xfa, 0xa6, 0x8c, 0xe8, 0x63, 0xaa, 0xac);
EXTERN_GUID(IID_IHostAssemblyStore, 0x7b102a88, 0x3f7f, 0x496d, 0x8f, 0xa2, 0xc3, 0x53, 0x74, 0xe0, 0x1a, 0xf3);
EXTERN_GUID(IID_IHostAssemblyManager, 0x613dabd7, 0x62b2, 0x493e, 0x9e, 0x65, 0xc1, 0xe3, 0x2a, 0x1e, 0x0c, 0x5e);
EXTERN_GUID(IID_IHostSecurityManager, 0x75ad2468, 0xa349, 0x4d02, 0xa7, 0x64, 0x76, 0xa6, 0x8a, 0xee, 0x0c, 0x4f);
EXTERN_GUID(IID_IHostSecurityContext, 0x7e573ce4, 0x343, 0x4423, 0x98, 0xd7, 0x63, 0x18, 0x34, 0x8a, 0x1d, 0x3c);
EXTERN_GUID(IID_ICLRAssemblyIdentityManager, 0x15f0a9da, 0x3ff6, 0x4393, 0x9d, 0xa9, 0xfd, 0xfd, 0x28, 0x4e, 0x69, 0x72);
EXTERN_GUID(IID_ICLRDomainManager, 0x270d00a2, 0x8e15, 0x4d0b, 0xad, 0xeb, 0x37, 0xbc, 0x3e, 0x47, 0xdf, 0x77);
EXTERN_GUID(IID_ITypeName, 0xB81FF171, 0x20F3, 0x11d2, 0x8d, 0xcc, 0x00, 0xa0, 0xc9, 0xb0, 0x05, 0x22);
EXTERN_GUID(IID_ICLRAssemblyReferenceList, 0x1b2c9750, 0x2e66, 0x4bda, 0x8b, 0x44, 0x0a, 0x64, 0x2c, 0x5c, 0xd7, 0x33);
EXTERN_GUID(IID_ICLRReferenceAssemblyEnum, 0xd509cb5d, 0xcf32, 0x4876, 0xae, 0x61, 0x67, 0x77, 0x0c, 0xf9, 0x19, 0x73);
EXTERN_GUID(IID_ICLRProbingAssemblyEnum, 0xd0c5fb1f, 0x416b, 0x4f97, 0x81, 0xf4, 0x7a, 0xc7, 0xdc, 0x24, 0xdd, 0x5d);
EXTERN_GUID(IID_ICLRHostBindingPolicyManager, 0x4b3545e7, 0x1856, 0x48c9, 0xa8, 0xba, 0x24, 0xb2, 0x1a, 0x75, 0x3c, 0x09);
EXTERN_GUID(IID_ITypeNameBuilder, 0xB81FF171, 0x20F3, 0x11d2, 0x8d, 0xcc, 0x00, 0xa0, 0xc9, 0xb0, 0x05, 0x23);
EXTERN_GUID(IID_ITypeNameFactory, 0xB81FF171, 0x20F3, 0x11d2, 0x8d, 0xcc, 0x00, 0xa0, 0xc9, 0xb0, 0x05, 0x21);
DEPRECATED_CLR_STDAPI GetCORSystemDirectory(_Out_writes_to_(cchBuffer, *dwLength) LPWSTR pbuffer, DWORD  cchBuffer, DWORD* dwLength);
DEPRECATED_CLR_STDAPI GetCORVersion(_Out_writes_to_(cchBuffer, *dwLength) LPWSTR pbBuffer, DWORD cchBuffer, DWORD* dwLength);
DEPRECATED_CLR_STDAPI GetFileVersion(LPCWSTR szFilename, _Out_writes_to_opt_(cchBuffer, *dwLength) LPWSTR szBuffer, DWORD cchBuffer, DWORD* dwLength);
DEPRECATED_CLR_STDAPI GetCORRequiredVersion(_Out_writes_to_(cchBuffer, *dwLength) LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwLength);
DEPRECATED_CLR_STDAPI GetRequestedRuntimeInfo(LPCWSTR pExe, LPCWSTR pwszVersion, LPCWSTR pConfigurationFile, DWORD startupFlags, DWORD runtimeInfoFlags, _Out_writes_opt_(dwDirectory) LPWSTR pDirectory, DWORD dwDirectory, _Out_opt_ DWORD *dwDirectoryLength, _Out_writes_opt_(cchBuffer) LPWSTR pVersion, DWORD cchBuffer, _Out_opt_ DWORD* dwlength);
DEPRECATED_CLR_STDAPI GetRequestedRuntimeVersion(_In_ LPWSTR pExe, _Out_writes_to_(cchBuffer, *pdwLength) LPWSTR pVersion, DWORD cchBuffer, _Out_ DWORD* dwLength);
DEPRECATED_CLR_STDAPI CorBindToRuntimeHost(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, LPCWSTR pwszHostConfigFile, VOID* pReserved, DWORD startupFlags, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
DEPRECATED_CLR_STDAPI CorBindToRuntimeEx(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, DWORD startupFlags, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
DEPRECATED_CLR_STDAPI CorBindToRuntimeByCfg(IStream* pCfgStream, DWORD reserved, DWORD startupFlags, REFCLSID rclsid,REFIID riid, LPVOID FAR* ppv);
DEPRECATED_CLR_STDAPI CorBindToRuntime(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
DEPRECATED_CLR_STDAPI CorBindToCurrentRuntime(LPCWSTR pwszFileName, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);
DEPRECATED_CLR_STDAPI ClrCreateManagedInstance(LPCWSTR pTypeName, REFIID riid, void **ppObject);
DECLARE_DEPRECATED void STDMETHODCALLTYPE CorMarkThreadInThreadPool();
DEPRECATED_CLR_STDAPI RunDll32ShimW(HWND hwnd, HINSTANCE hinst, LPCWSTR lpszCmdLine, int nCmdShow);
DEPRECATED_CLR_STDAPI LoadLibraryShim(LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE *phModDll);
DEPRECATED_CLR_STDAPI CallFunctionShim(LPCWSTR szDllName, LPCSTR szFunctionName, LPVOID lpvArgument1, LPVOID lpvArgument2, LPCWSTR szVersion, LPVOID pvReserved);
DEPRECATED_CLR_STDAPI GetRealProcAddress(LPCSTR pwszProcName, VOID** ppv);
DECLARE_DEPRECATED void STDMETHODCALLTYPE CorExitProcess(int exitCode);
DEPRECATED_CLR_STDAPI LoadStringRC(UINT iResouceID, _Out_writes_z_(iMax) LPWSTR szBuffer, int iMax, int bQuiet);
DEPRECATED_CLR_STDAPI LoadStringRCEx(LCID lcid, UINT iResouceID, _Out_writes_z_(iMax) LPWSTR szBuffer, int iMax, int bQuiet, int *pcwchUsed);
typedef HRESULT (__stdcall *FLockClrVersionCallback) ();
DEPRECATED_CLR_STDAPI LockClrVersion(FLockClrVersionCallback hostCallback,FLockClrVersionCallback *pBeginHostSetup,FLockClrVersionCallback *pEndHostSetup);
DEPRECATED_CLR_STDAPI CreateDebuggingInterfaceFromVersion(int iDebuggerVersion, LPCWSTR szDebuggeeVersion, IUnknown ** ppCordb);
DEPRECATED_CLR_STDAPI GetVersionFromProcess(HANDLE hProcess, _Out_writes_to_(cchBuffer, *pdwLength) LPWSTR pVersion, DWORD cchBuffer, _Out_ DWORD* dwLength);
typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0000_0001
    {
        HOST_TYPE_DEFAULT	= 0,
        HOST_TYPE_APPLAUNCH	= 0x1,
        HOST_TYPE_CORFLAG	= 0x2
    } 	HOST_TYPE;

STDAPI CorLaunchApplication(HOST_TYPE dwClickOnceHost, LPCWSTR pwzAppFullName, DWORD dwManifestPaths, LPCWSTR* ppwzManifestPaths, DWORD dwActivationData, LPCWSTR* ppwzActivationData, LPPROCESS_INFORMATION lpProcessInformation);
typedef HRESULT ( __stdcall *FExecuteInAppDomainCallback )( 
    void *cookie);

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0000_0002
    {
        STARTUP_CONCURRENT_GC	= 0x1,
        STARTUP_LOADER_OPTIMIZATION_MASK	= ( 0x3 << 1 ) ,
        STARTUP_LOADER_OPTIMIZATION_SINGLE_DOMAIN	= ( 0x1 << 1 ) ,
        STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN	= ( 0x2 << 1 ) ,
        STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST	= ( 0x3 << 1 ) ,
        STARTUP_LOADER_SAFEMODE	= 0x10,
        STARTUP_LOADER_SETPREFERENCE	= 0x100,
        STARTUP_SERVER_GC	= 0x1000,
        STARTUP_HOARD_GC_VM	= 0x2000,
        STARTUP_SINGLE_VERSION_HOSTING_INTERFACE	= 0x4000,
        STARTUP_LEGACY_IMPERSONATION	= 0x10000,
        STARTUP_DISABLE_COMMITTHREADSTACK	= 0x20000,
        STARTUP_ALWAYSFLOW_IMPERSONATION	= 0x40000,
        STARTUP_TRIM_GC_COMMIT	= 0x80000,
        STARTUP_ETW	= 0x100000,
        STARTUP_ARM	= 0x400000
    } 	STARTUP_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0000_0003
    {
        CLSID_RESOLUTION_DEFAULT	= 0,
        CLSID_RESOLUTION_REGISTERED	= 0x1
    } 	CLSID_RESOLUTION_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0000_0004
    {
        RUNTIME_INFO_UPGRADE_VERSION	= 0x1,
        RUNTIME_INFO_REQUEST_IA64	= 0x2,
        RUNTIME_INFO_REQUEST_AMD64	= 0x4,
        RUNTIME_INFO_REQUEST_X86	= 0x8,
        RUNTIME_INFO_DONT_RETURN_DIRECTORY	= 0x10,
        RUNTIME_INFO_DONT_RETURN_VERSION	= 0x20,
        RUNTIME_INFO_DONT_SHOW_ERROR_DIALOG	= 0x40,
        RUNTIME_INFO_IGNORE_ERROR_MODE	= 0x1000
    } 	RUNTIME_INFO_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0000_0005
    {
        APPDOMAIN_SECURITY_DEFAULT	= 0,
        APPDOMAIN_SECURITY_SANDBOXED	= 0x1,
        APPDOMAIN_SECURITY_FORBID_CROSSAD_REVERSE_PINVOKE	= 0x2,
        APPDOMAIN_FORCE_TRIVIAL_WAIT_OPERATIONS	= 0x8
    } 	APPDOMAIN_SECURITY_FLAGS;

STDAPI GetRequestedRuntimeVersionForCLSID(REFCLSID rclsid, _Out_writes_opt_(cchBuffer) LPWSTR pVersion, DWORD cchBuffer, _Out_opt_ DWORD* dwLength, CLSID_RESOLUTION_FLAGS dwResolutionFlags);


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0000_v0_0_s_ifspec;

#ifndef __IObjectHandle_INTERFACE_DEFINED__
#define __IObjectHandle_INTERFACE_DEFINED__

/* interface IObjectHandle */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IObjectHandle;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C460E2B4-E199-412a-8456-84DC3E4838C3")
    IObjectHandle : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Unwrap( 
            /* [retval][out] */ __RPC__out VARIANT *ppv) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IObjectHandleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IObjectHandle * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IObjectHandle * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IObjectHandle * This);
        
        HRESULT ( STDMETHODCALLTYPE *Unwrap )( 
            __RPC__in IObjectHandle * This,
            /* [retval][out] */ __RPC__out VARIANT *ppv);
        
        END_INTERFACE
    } IObjectHandleVtbl;

    interface IObjectHandle
    {
        CONST_VTBL struct IObjectHandleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectHandle_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IObjectHandle_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IObjectHandle_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IObjectHandle_Unwrap(This,ppv)	\
    ( (This)->lpVtbl -> Unwrap(This,ppv) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IObjectHandle_INTERFACE_DEFINED__ */


#ifndef __IAppDomainBinding_INTERFACE_DEFINED__
#define __IAppDomainBinding_INTERFACE_DEFINED__

/* interface IAppDomainBinding */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IAppDomainBinding;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C2B07A7-1E98-11d3-872F-00C04F79ED0D")
    IAppDomainBinding : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnAppDomain( 
            /* [in] */ IUnknown *pAppdomain) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IAppDomainBindingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAppDomainBinding * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAppDomainBinding * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAppDomainBinding * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnAppDomain )( 
            IAppDomainBinding * This,
            /* [in] */ IUnknown *pAppdomain);
        
        END_INTERFACE
    } IAppDomainBindingVtbl;

    interface IAppDomainBinding
    {
        CONST_VTBL struct IAppDomainBindingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAppDomainBinding_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IAppDomainBinding_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IAppDomainBinding_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IAppDomainBinding_OnAppDomain(This,pAppdomain)	\
    ( (This)->lpVtbl -> OnAppDomain(This,pAppdomain) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IAppDomainBinding_INTERFACE_DEFINED__ */


#ifndef __IGCThreadControl_INTERFACE_DEFINED__
#define __IGCThreadControl_INTERFACE_DEFINED__

/* interface IGCThreadControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IGCThreadControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F31D1788-C397-4725-87A5-6AF3472C2791")
    IGCThreadControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ThreadIsBlockingForSuspension( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspensionStarting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspensionEnding( 
            DWORD Generation) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IGCThreadControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGCThreadControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGCThreadControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGCThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadIsBlockingForSuspension )( 
            IGCThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuspensionStarting )( 
            IGCThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuspensionEnding )( 
            IGCThreadControl * This,
            DWORD Generation);
        
        END_INTERFACE
    } IGCThreadControlVtbl;

    interface IGCThreadControl
    {
        CONST_VTBL struct IGCThreadControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGCThreadControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IGCThreadControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IGCThreadControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IGCThreadControl_ThreadIsBlockingForSuspension(This)	\
    ( (This)->lpVtbl -> ThreadIsBlockingForSuspension(This) ) 

#define IGCThreadControl_SuspensionStarting(This)	\
    ( (This)->lpVtbl -> SuspensionStarting(This) ) 

#define IGCThreadControl_SuspensionEnding(This,Generation)	\
    ( (This)->lpVtbl -> SuspensionEnding(This,Generation) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IGCThreadControl_INTERFACE_DEFINED__ */


#ifndef __IGCHostControl_INTERFACE_DEFINED__
#define __IGCHostControl_INTERFACE_DEFINED__

/* interface IGCHostControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IGCHostControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5513D564-8374-4cb9-AED9-0083F4160A1D")
    IGCHostControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestVirtualMemLimit( 
            /* [in] */ SIZE_T sztMaxVirtualMemMB,
            /* [out][in] */ SIZE_T *psztNewMaxVirtualMemMB) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IGCHostControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGCHostControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGCHostControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGCHostControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *RequestVirtualMemLimit )( 
            IGCHostControl * This,
            /* [in] */ SIZE_T sztMaxVirtualMemMB,
            /* [out][in] */ SIZE_T *psztNewMaxVirtualMemMB);
        
        END_INTERFACE
    } IGCHostControlVtbl;

    interface IGCHostControl
    {
        CONST_VTBL struct IGCHostControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGCHostControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IGCHostControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IGCHostControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IGCHostControl_RequestVirtualMemLimit(This,sztMaxVirtualMemMB,psztNewMaxVirtualMemMB)	\
    ( (This)->lpVtbl -> RequestVirtualMemLimit(This,sztMaxVirtualMemMB,psztNewMaxVirtualMemMB) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IGCHostControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0004 */
/* [local] */ 

#ifdef __midl
typedef VOID ( __stdcall *WAITORTIMERCALLBACK )( 
    PVOID __MIDL____MIDL_itf_mscoree_0000_00040000,
    BOOL __MIDL____MIDL_itf_mscoree_0000_00040001);

#endif // __midl
#ifdef __midl
typedef DWORD ( __stdcall *LPTHREAD_START_ROUTINE )( 
    LPVOID lpThreadParameter);

typedef VOID ( *LPOVERLAPPED_COMPLETION_ROUTINE )( 
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPVOID lpOverlapped);

#endif // __midl
typedef VOID ( __stdcall *PTLS_CALLBACK_FUNCTION )( 
    PVOID __MIDL____MIDL_itf_mscoree_0000_00040005);



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0004_v0_0_s_ifspec;

#ifndef __ICorThreadpool_INTERFACE_DEFINED__
#define __ICorThreadpool_INTERFACE_DEFINED__

/* interface ICorThreadpool */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICorThreadpool;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("84680D3A-B2C1-46e8-ACC2-DBC0A359159A")
    ICorThreadpool : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CorRegisterWaitForSingleObject( 
            /* [in] */ HANDLE *phNewWaitObject,
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Context,
            /* [in] */ ULONG timeout,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorUnregisterWait( 
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorQueueUserWorkItem( 
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorCreateTimer( 
            /* [in] */ HANDLE *phNewTimer,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Parameter,
            /* [in] */ DWORD DueTime,
            /* [in] */ DWORD Period,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorChangeTimer( 
            /* [in] */ HANDLE Timer,
            /* [in] */ ULONG DueTime,
            /* [in] */ ULONG Period,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorDeleteTimer( 
            /* [in] */ HANDLE Timer,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorBindIoCompletionCallback( 
            /* [in] */ HANDLE fileHandle,
            /* [in] */ LPOVERLAPPED_COMPLETION_ROUTINE callback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorCallOrQueueUserWorkItem( 
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [out] */ BOOL *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorSetMaxThreads( 
            /* [in] */ DWORD MaxWorkerThreads,
            /* [in] */ DWORD MaxIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorGetMaxThreads( 
            /* [out] */ DWORD *MaxWorkerThreads,
            /* [out] */ DWORD *MaxIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CorGetAvailableThreads( 
            /* [out] */ DWORD *AvailableWorkerThreads,
            /* [out] */ DWORD *AvailableIOCompletionThreads) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICorThreadpoolVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorThreadpool * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorThreadpool * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorThreadpool * This);
        
        HRESULT ( STDMETHODCALLTYPE *CorRegisterWaitForSingleObject )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE *phNewWaitObject,
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Context,
            /* [in] */ ULONG timeout,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorUnregisterWait )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE hWaitObject,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorQueueUserWorkItem )( 
            ICorThreadpool * This,
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [in] */ BOOL executeOnlyOnce,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorCreateTimer )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE *phNewTimer,
            /* [in] */ WAITORTIMERCALLBACK Callback,
            /* [in] */ PVOID Parameter,
            /* [in] */ DWORD DueTime,
            /* [in] */ DWORD Period,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorChangeTimer )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE Timer,
            /* [in] */ ULONG DueTime,
            /* [in] */ ULONG Period,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorDeleteTimer )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE Timer,
            /* [in] */ HANDLE CompletionEvent,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorBindIoCompletionCallback )( 
            ICorThreadpool * This,
            /* [in] */ HANDLE fileHandle,
            /* [in] */ LPOVERLAPPED_COMPLETION_ROUTINE callback);
        
        HRESULT ( STDMETHODCALLTYPE *CorCallOrQueueUserWorkItem )( 
            ICorThreadpool * This,
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [out] */ BOOL *result);
        
        HRESULT ( STDMETHODCALLTYPE *CorSetMaxThreads )( 
            ICorThreadpool * This,
            /* [in] */ DWORD MaxWorkerThreads,
            /* [in] */ DWORD MaxIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *CorGetMaxThreads )( 
            ICorThreadpool * This,
            /* [out] */ DWORD *MaxWorkerThreads,
            /* [out] */ DWORD *MaxIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *CorGetAvailableThreads )( 
            ICorThreadpool * This,
            /* [out] */ DWORD *AvailableWorkerThreads,
            /* [out] */ DWORD *AvailableIOCompletionThreads);
        
        END_INTERFACE
    } ICorThreadpoolVtbl;

    interface ICorThreadpool
    {
        CONST_VTBL struct ICorThreadpoolVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorThreadpool_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICorThreadpool_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICorThreadpool_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICorThreadpool_CorRegisterWaitForSingleObject(This,phNewWaitObject,hWaitObject,Callback,Context,timeout,executeOnlyOnce,result)	\
    ( (This)->lpVtbl -> CorRegisterWaitForSingleObject(This,phNewWaitObject,hWaitObject,Callback,Context,timeout,executeOnlyOnce,result) ) 

#define ICorThreadpool_CorUnregisterWait(This,hWaitObject,CompletionEvent,result)	\
    ( (This)->lpVtbl -> CorUnregisterWait(This,hWaitObject,CompletionEvent,result) ) 

#define ICorThreadpool_CorQueueUserWorkItem(This,Function,Context,executeOnlyOnce,result)	\
    ( (This)->lpVtbl -> CorQueueUserWorkItem(This,Function,Context,executeOnlyOnce,result) ) 

#define ICorThreadpool_CorCreateTimer(This,phNewTimer,Callback,Parameter,DueTime,Period,result)	\
    ( (This)->lpVtbl -> CorCreateTimer(This,phNewTimer,Callback,Parameter,DueTime,Period,result) ) 

#define ICorThreadpool_CorChangeTimer(This,Timer,DueTime,Period,result)	\
    ( (This)->lpVtbl -> CorChangeTimer(This,Timer,DueTime,Period,result) ) 

#define ICorThreadpool_CorDeleteTimer(This,Timer,CompletionEvent,result)	\
    ( (This)->lpVtbl -> CorDeleteTimer(This,Timer,CompletionEvent,result) ) 

#define ICorThreadpool_CorBindIoCompletionCallback(This,fileHandle,callback)	\
    ( (This)->lpVtbl -> CorBindIoCompletionCallback(This,fileHandle,callback) ) 

#define ICorThreadpool_CorCallOrQueueUserWorkItem(This,Function,Context,result)	\
    ( (This)->lpVtbl -> CorCallOrQueueUserWorkItem(This,Function,Context,result) ) 

#define ICorThreadpool_CorSetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads)	\
    ( (This)->lpVtbl -> CorSetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads) ) 

#define ICorThreadpool_CorGetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads)	\
    ( (This)->lpVtbl -> CorGetMaxThreads(This,MaxWorkerThreads,MaxIOCompletionThreads) ) 

#define ICorThreadpool_CorGetAvailableThreads(This,AvailableWorkerThreads,AvailableIOCompletionThreads)	\
    ( (This)->lpVtbl -> CorGetAvailableThreads(This,AvailableWorkerThreads,AvailableIOCompletionThreads) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICorThreadpool_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0005 */
/* [local] */ 

EXTERN_GUID(IID_IDebuggerThreadControl, 0x23d86786, 0x0bb5, 0x4774, 0x8f, 0xb5, 0xe3, 0x52, 0x2a, 0xdd, 0x62, 0x46);


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0005_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0005_v0_0_s_ifspec;

#ifndef __IDebuggerThreadControl_INTERFACE_DEFINED__
#define __IDebuggerThreadControl_INTERFACE_DEFINED__

/* interface IDebuggerThreadControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IDebuggerThreadControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("23D86786-0BB5-4774-8FB5-E3522ADD6246")
    IDebuggerThreadControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ThreadIsBlockingForDebugger( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseAllRuntimeThreads( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartBlockingForDebugger( 
            DWORD dwUnused) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDebuggerThreadControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDebuggerThreadControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDebuggerThreadControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDebuggerThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadIsBlockingForDebugger )( 
            IDebuggerThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseAllRuntimeThreads )( 
            IDebuggerThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartBlockingForDebugger )( 
            IDebuggerThreadControl * This,
            DWORD dwUnused);
        
        END_INTERFACE
    } IDebuggerThreadControlVtbl;

    interface IDebuggerThreadControl
    {
        CONST_VTBL struct IDebuggerThreadControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebuggerThreadControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDebuggerThreadControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDebuggerThreadControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDebuggerThreadControl_ThreadIsBlockingForDebugger(This)	\
    ( (This)->lpVtbl -> ThreadIsBlockingForDebugger(This) ) 

#define IDebuggerThreadControl_ReleaseAllRuntimeThreads(This)	\
    ( (This)->lpVtbl -> ReleaseAllRuntimeThreads(This) ) 

#define IDebuggerThreadControl_StartBlockingForDebugger(This,dwUnused)	\
    ( (This)->lpVtbl -> StartBlockingForDebugger(This,dwUnused) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDebuggerThreadControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0006 */
/* [local] */ 

EXTERN_GUID(IID_IDebuggerInfo, 0xbf24142d, 0xa47d, 0x4d24, 0xa6, 0x6d, 0x8c, 0x21, 0x41, 0x94, 0x4e, 0x44);


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0006_v0_0_s_ifspec;

#ifndef __IDebuggerInfo_INTERFACE_DEFINED__
#define __IDebuggerInfo_INTERFACE_DEFINED__

/* interface IDebuggerInfo */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IDebuggerInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BF24142D-A47D-4d24-A66D-8C2141944E44")
    IDebuggerInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsDebuggerAttached( 
            /* [out] */ BOOL *pbAttached) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IDebuggerInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDebuggerInfo * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDebuggerInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDebuggerInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsDebuggerAttached )( 
            IDebuggerInfo * This,
            /* [out] */ BOOL *pbAttached);
        
        END_INTERFACE
    } IDebuggerInfoVtbl;

    interface IDebuggerInfo
    {
        CONST_VTBL struct IDebuggerInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDebuggerInfo_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDebuggerInfo_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDebuggerInfo_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDebuggerInfo_IsDebuggerAttached(This,pbAttached)	\
    ( (This)->lpVtbl -> IsDebuggerAttached(This,pbAttached) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDebuggerInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0007 */
/* [local] */ 

EXTERN_GUID(IID_ICorConfiguration, 0x5c2b07a5, 0x1e98, 0x11d3, 0x87, 0x2f, 0x00, 0xc0, 0x4f, 0x79, 0xed, 0x0d);


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0007_v0_0_s_ifspec;

#ifndef __ICorConfiguration_INTERFACE_DEFINED__
#define __ICorConfiguration_INTERFACE_DEFINED__

/* interface ICorConfiguration */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICorConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C2B07A5-1E98-11d3-872F-00C04F79ED0D")
    ICorConfiguration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGCThreadControl( 
            /* [in] */ IGCThreadControl *pGCThreadControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGCHostControl( 
            /* [in] */ IGCHostControl *pGCHostControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDebuggerThreadControl( 
            /* [in] */ IDebuggerThreadControl *pDebuggerThreadControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddDebuggerSpecialThread( 
            /* [in] */ DWORD dwSpecialThreadId) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICorConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorConfiguration * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorConfiguration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorConfiguration * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCThreadControl )( 
            ICorConfiguration * This,
            /* [in] */ IGCThreadControl *pGCThreadControl);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCHostControl )( 
            ICorConfiguration * This,
            /* [in] */ IGCHostControl *pGCHostControl);
        
        HRESULT ( STDMETHODCALLTYPE *SetDebuggerThreadControl )( 
            ICorConfiguration * This,
            /* [in] */ IDebuggerThreadControl *pDebuggerThreadControl);
        
        HRESULT ( STDMETHODCALLTYPE *AddDebuggerSpecialThread )( 
            ICorConfiguration * This,
            /* [in] */ DWORD dwSpecialThreadId);
        
        END_INTERFACE
    } ICorConfigurationVtbl;

    interface ICorConfiguration
    {
        CONST_VTBL struct ICorConfigurationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorConfiguration_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICorConfiguration_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICorConfiguration_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICorConfiguration_SetGCThreadControl(This,pGCThreadControl)	\
    ( (This)->lpVtbl -> SetGCThreadControl(This,pGCThreadControl) ) 

#define ICorConfiguration_SetGCHostControl(This,pGCHostControl)	\
    ( (This)->lpVtbl -> SetGCHostControl(This,pGCHostControl) ) 

#define ICorConfiguration_SetDebuggerThreadControl(This,pDebuggerThreadControl)	\
    ( (This)->lpVtbl -> SetDebuggerThreadControl(This,pDebuggerThreadControl) ) 

#define ICorConfiguration_AddDebuggerSpecialThread(This,dwSpecialThreadId)	\
    ( (This)->lpVtbl -> AddDebuggerSpecialThread(This,dwSpecialThreadId) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICorConfiguration_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0008 */
/* [local] */ 

typedef void *HDOMAINENUM;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0008_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0008_v0_0_s_ifspec;

#ifndef __ICorRuntimeHost_INTERFACE_DEFINED__
#define __ICorRuntimeHost_INTERFACE_DEFINED__

/* interface ICorRuntimeHost */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICorRuntimeHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CB2F6722-AB3A-11d2-9C40-00C04FA30A3E")
    ICorRuntimeHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateLogicalThreadState( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteLogicalThreadState( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SwitchInLogicalThreadState( 
            /* [in] */ DWORD *pFiberCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SwitchOutLogicalThreadState( 
            /* [out] */ DWORD **pFiberCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LocksHeldByLogicalThread( 
            /* [out] */ DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapFile( 
            /* [in] */ HANDLE hFile,
            /* [out] */ HMODULE *hMapAddress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfiguration( 
            /* [out] */ ICorConfiguration **pConfiguration) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDomain( 
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pIdentityArray,
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultDomain( 
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumDomains( 
            /* [out] */ HDOMAINENUM *hEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NextDomain( 
            /* [in] */ HDOMAINENUM hEnum,
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseEnum( 
            /* [in] */ HDOMAINENUM hEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDomainEx( 
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pSetup,
            /* [in] */ IUnknown *pEvidence,
            /* [out] */ IUnknown **pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDomainSetup( 
            /* [out] */ IUnknown **pAppDomainSetup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateEvidence( 
            /* [out] */ IUnknown **pEvidence) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadDomain( 
            /* [in] */ IUnknown *pAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CurrentDomain( 
            /* [out] */ IUnknown **pAppDomain) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICorRuntimeHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICorRuntimeHost * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICorRuntimeHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateLogicalThreadState )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteLogicalThreadState )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchInLogicalThreadState )( 
            ICorRuntimeHost * This,
            /* [in] */ DWORD *pFiberCookie);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchOutLogicalThreadState )( 
            ICorRuntimeHost * This,
            /* [out] */ DWORD **pFiberCookie);
        
        HRESULT ( STDMETHODCALLTYPE *LocksHeldByLogicalThread )( 
            ICorRuntimeHost * This,
            /* [out] */ DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *MapFile )( 
            ICorRuntimeHost * This,
            /* [in] */ HANDLE hFile,
            /* [out] */ HMODULE *hMapAddress);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfiguration )( 
            ICorRuntimeHost * This,
            /* [out] */ ICorConfiguration **pConfiguration);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICorRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomain )( 
            ICorRuntimeHost * This,
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pIdentityArray,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultDomain )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *EnumDomains )( 
            ICorRuntimeHost * This,
            /* [out] */ HDOMAINENUM *hEnum);
        
        HRESULT ( STDMETHODCALLTYPE *NextDomain )( 
            ICorRuntimeHost * This,
            /* [in] */ HDOMAINENUM hEnum,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *CloseEnum )( 
            ICorRuntimeHost * This,
            /* [in] */ HDOMAINENUM hEnum);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainEx )( 
            ICorRuntimeHost * This,
            /* [in] */ LPCWSTR pwzFriendlyName,
            /* [in] */ IUnknown *pSetup,
            /* [in] */ IUnknown *pEvidence,
            /* [out] */ IUnknown **pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDomainSetup )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pAppDomainSetup);
        
        HRESULT ( STDMETHODCALLTYPE *CreateEvidence )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pEvidence);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadDomain )( 
            ICorRuntimeHost * This,
            /* [in] */ IUnknown *pAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *CurrentDomain )( 
            ICorRuntimeHost * This,
            /* [out] */ IUnknown **pAppDomain);
        
        END_INTERFACE
    } ICorRuntimeHostVtbl;

    interface ICorRuntimeHost
    {
        CONST_VTBL struct ICorRuntimeHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICorRuntimeHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICorRuntimeHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICorRuntimeHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICorRuntimeHost_CreateLogicalThreadState(This)	\
    ( (This)->lpVtbl -> CreateLogicalThreadState(This) ) 

#define ICorRuntimeHost_DeleteLogicalThreadState(This)	\
    ( (This)->lpVtbl -> DeleteLogicalThreadState(This) ) 

#define ICorRuntimeHost_SwitchInLogicalThreadState(This,pFiberCookie)	\
    ( (This)->lpVtbl -> SwitchInLogicalThreadState(This,pFiberCookie) ) 

#define ICorRuntimeHost_SwitchOutLogicalThreadState(This,pFiberCookie)	\
    ( (This)->lpVtbl -> SwitchOutLogicalThreadState(This,pFiberCookie) ) 

#define ICorRuntimeHost_LocksHeldByLogicalThread(This,pCount)	\
    ( (This)->lpVtbl -> LocksHeldByLogicalThread(This,pCount) ) 

#define ICorRuntimeHost_MapFile(This,hFile,hMapAddress)	\
    ( (This)->lpVtbl -> MapFile(This,hFile,hMapAddress) ) 

#define ICorRuntimeHost_GetConfiguration(This,pConfiguration)	\
    ( (This)->lpVtbl -> GetConfiguration(This,pConfiguration) ) 

#define ICorRuntimeHost_Start(This)	\
    ( (This)->lpVtbl -> Start(This) ) 

#define ICorRuntimeHost_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#define ICorRuntimeHost_CreateDomain(This,pwzFriendlyName,pIdentityArray,pAppDomain)	\
    ( (This)->lpVtbl -> CreateDomain(This,pwzFriendlyName,pIdentityArray,pAppDomain) ) 

#define ICorRuntimeHost_GetDefaultDomain(This,pAppDomain)	\
    ( (This)->lpVtbl -> GetDefaultDomain(This,pAppDomain) ) 

#define ICorRuntimeHost_EnumDomains(This,hEnum)	\
    ( (This)->lpVtbl -> EnumDomains(This,hEnum) ) 

#define ICorRuntimeHost_NextDomain(This,hEnum,pAppDomain)	\
    ( (This)->lpVtbl -> NextDomain(This,hEnum,pAppDomain) ) 

#define ICorRuntimeHost_CloseEnum(This,hEnum)	\
    ( (This)->lpVtbl -> CloseEnum(This,hEnum) ) 

#define ICorRuntimeHost_CreateDomainEx(This,pwzFriendlyName,pSetup,pEvidence,pAppDomain)	\
    ( (This)->lpVtbl -> CreateDomainEx(This,pwzFriendlyName,pSetup,pEvidence,pAppDomain) ) 

#define ICorRuntimeHost_CreateDomainSetup(This,pAppDomainSetup)	\
    ( (This)->lpVtbl -> CreateDomainSetup(This,pAppDomainSetup) ) 

#define ICorRuntimeHost_CreateEvidence(This,pEvidence)	\
    ( (This)->lpVtbl -> CreateEvidence(This,pEvidence) ) 

#define ICorRuntimeHost_UnloadDomain(This,pAppDomain)	\
    ( (This)->lpVtbl -> UnloadDomain(This,pAppDomain) ) 

#define ICorRuntimeHost_CurrentDomain(This,pAppDomain)	\
    ( (This)->lpVtbl -> CurrentDomain(This,pAppDomain) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICorRuntimeHost_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0009 */
/* [local] */ 

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0009_0001
    {
        eMemoryAvailableLow	= 1,
        eMemoryAvailableNeutral	= 2,
        eMemoryAvailableHigh	= 3
    } 	EMemoryAvailable;

typedef /* [public][public][public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0009_0002
    {
        eTaskCritical	= 0,
        eAppDomainCritical	= 1,
        eProcessCritical	= 2
    } 	EMemoryCriticalLevel;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0009_0003
    {
        WAIT_MSGPUMP	= 0x1,
        WAIT_ALERTABLE	= 0x2,
        WAIT_NOTINDEADLOCK	= 0x4
    } 	WAIT_OPTION;

EXTERN_GUID(IID_ICLRMemoryNotificationCallback, 0x47EB8E57, 0x0846, 0x4546, 0xAF, 0x76, 0x6F, 0x42, 0xFC, 0xFC, 0x26, 0x49);


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0009_v0_0_s_ifspec;

#ifndef __ICLRMemoryNotificationCallback_INTERFACE_DEFINED__
#define __ICLRMemoryNotificationCallback_INTERFACE_DEFINED__

/* interface ICLRMemoryNotificationCallback */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRMemoryNotificationCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("47EB8E57-0846-4546-AF76-6F42FCFC2649")
    ICLRMemoryNotificationCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnMemoryNotification( 
            /* [in] */ EMemoryAvailable eMemoryAvailable) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRMemoryNotificationCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRMemoryNotificationCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRMemoryNotificationCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRMemoryNotificationCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnMemoryNotification )( 
            ICLRMemoryNotificationCallback * This,
            /* [in] */ EMemoryAvailable eMemoryAvailable);
        
        END_INTERFACE
    } ICLRMemoryNotificationCallbackVtbl;

    interface ICLRMemoryNotificationCallback
    {
        CONST_VTBL struct ICLRMemoryNotificationCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRMemoryNotificationCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRMemoryNotificationCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRMemoryNotificationCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRMemoryNotificationCallback_OnMemoryNotification(This,eMemoryAvailable)	\
    ( (This)->lpVtbl -> OnMemoryNotification(This,eMemoryAvailable) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRMemoryNotificationCallback_INTERFACE_DEFINED__ */


#ifndef __IHostMalloc_INTERFACE_DEFINED__
#define __IHostMalloc_INTERFACE_DEFINED__

/* interface IHostMalloc */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostMalloc;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1831991C-CC53-4A31-B218-04E910446479")
    IHostMalloc : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Alloc( 
            /* [in] */ SIZE_T cbSize,
            /* [in] */ EMemoryCriticalLevel eCriticalLevel,
            /* [out] */ void **ppMem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DebugAlloc( 
            /* [in] */ SIZE_T cbSize,
            /* [in] */ EMemoryCriticalLevel eCriticalLevel,
            /* [annotation][in] */ 
            _In_   char *pszFileName,
            /* [in] */ int iLineNo,
            /* [annotation][out] */ 
            _Outptr_result_maybenull_  void **ppMem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Free( 
            /* [in] */ void *pMem) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostMallocVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostMalloc * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostMalloc * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostMalloc * This);
        
        HRESULT ( STDMETHODCALLTYPE *Alloc )( 
            IHostMalloc * This,
            /* [in] */ SIZE_T cbSize,
            /* [in] */ EMemoryCriticalLevel eCriticalLevel,
            /* [out] */ void **ppMem);
        
        HRESULT ( STDMETHODCALLTYPE *DebugAlloc )( 
            IHostMalloc * This,
            /* [in] */ SIZE_T cbSize,
            /* [in] */ EMemoryCriticalLevel eCriticalLevel,
            /* [annotation][in] */ 
            _In_   char *pszFileName,
            /* [in] */ int iLineNo,
            /* [annotation][out] */ 
            _Outptr_result_maybenull_  void **ppMem);
        
        HRESULT ( STDMETHODCALLTYPE *Free )( 
            IHostMalloc * This,
            /* [in] */ void *pMem);
        
        END_INTERFACE
    } IHostMallocVtbl;

    interface IHostMalloc
    {
        CONST_VTBL struct IHostMallocVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostMalloc_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostMalloc_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostMalloc_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostMalloc_Alloc(This,cbSize,eCriticalLevel,ppMem)	\
    ( (This)->lpVtbl -> Alloc(This,cbSize,eCriticalLevel,ppMem) ) 

#define IHostMalloc_DebugAlloc(This,cbSize,eCriticalLevel,pszFileName,iLineNo,ppMem)	\
    ( (This)->lpVtbl -> DebugAlloc(This,cbSize,eCriticalLevel,pszFileName,iLineNo,ppMem) ) 

#define IHostMalloc_Free(This,pMem)	\
    ( (This)->lpVtbl -> Free(This,pMem) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostMalloc_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0011 */
/* [local] */ 

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0011_0001
    {
        MALLOC_THREADSAFE	= 0x1,
        MALLOC_EXECUTABLE	= 0x2
    } 	MALLOC_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0011_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0011_v0_0_s_ifspec;

#ifndef __IHostMemoryManager_INTERFACE_DEFINED__
#define __IHostMemoryManager_INTERFACE_DEFINED__

/* interface IHostMemoryManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostMemoryManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7BC698D1-F9E3-4460-9CDE-D04248E9FA25")
    IHostMemoryManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateMalloc( 
            /* [in] */ DWORD dwMallocType,
            /* [out] */ IHostMalloc **ppMalloc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VirtualAlloc( 
            /* [in] */ void *pAddress,
            /* [in] */ SIZE_T dwSize,
            /* [in] */ DWORD flAllocationType,
            /* [in] */ DWORD flProtect,
            /* [in] */ EMemoryCriticalLevel eCriticalLevel,
            /* [out] */ void **ppMem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VirtualFree( 
            /* [in] */ LPVOID lpAddress,
            /* [in] */ SIZE_T dwSize,
            /* [in] */ DWORD dwFreeType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VirtualQuery( 
            /* [in] */ void *lpAddress,
            /* [out] */ void *lpBuffer,
            /* [in] */ SIZE_T dwLength,
            /* [out] */ SIZE_T *pResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VirtualProtect( 
            /* [in] */ void *lpAddress,
            /* [in] */ SIZE_T dwSize,
            /* [in] */ DWORD flNewProtect,
            /* [out] */ DWORD *pflOldProtect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMemoryLoad( 
            /* [out] */ DWORD *pMemoryLoad,
            /* [out] */ SIZE_T *pAvailableBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterMemoryNotificationCallback( 
            /* [in] */ ICLRMemoryNotificationCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NeedsVirtualAddressSpace( 
            /* [in] */ LPVOID startAddress,
            /* [in] */ SIZE_T size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AcquiredVirtualAddressSpace( 
            /* [in] */ LPVOID startAddress,
            /* [in] */ SIZE_T size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleasedVirtualAddressSpace( 
            /* [in] */ LPVOID startAddress) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostMemoryManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostMemoryManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostMemoryManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostMemoryManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateMalloc )( 
            IHostMemoryManager * This,
            /* [in] */ DWORD dwMallocType,
            /* [out] */ IHostMalloc **ppMalloc);
        
        HRESULT ( STDMETHODCALLTYPE *VirtualAlloc )( 
            IHostMemoryManager * This,
            /* [in] */ void *pAddress,
            /* [in] */ SIZE_T dwSize,
            /* [in] */ DWORD flAllocationType,
            /* [in] */ DWORD flProtect,
            /* [in] */ EMemoryCriticalLevel eCriticalLevel,
            /* [out] */ void **ppMem);
        
        HRESULT ( STDMETHODCALLTYPE *VirtualFree )( 
            IHostMemoryManager * This,
            /* [in] */ LPVOID lpAddress,
            /* [in] */ SIZE_T dwSize,
            /* [in] */ DWORD dwFreeType);
        
        HRESULT ( STDMETHODCALLTYPE *VirtualQuery )( 
            IHostMemoryManager * This,
            /* [in] */ void *lpAddress,
            /* [out] */ void *lpBuffer,
            /* [in] */ SIZE_T dwLength,
            /* [out] */ SIZE_T *pResult);
        
        HRESULT ( STDMETHODCALLTYPE *VirtualProtect )( 
            IHostMemoryManager * This,
            /* [in] */ void *lpAddress,
            /* [in] */ SIZE_T dwSize,
            /* [in] */ DWORD flNewProtect,
            /* [out] */ DWORD *pflOldProtect);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemoryLoad )( 
            IHostMemoryManager * This,
            /* [out] */ DWORD *pMemoryLoad,
            /* [out] */ SIZE_T *pAvailableBytes);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterMemoryNotificationCallback )( 
            IHostMemoryManager * This,
            /* [in] */ ICLRMemoryNotificationCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *NeedsVirtualAddressSpace )( 
            IHostMemoryManager * This,
            /* [in] */ LPVOID startAddress,
            /* [in] */ SIZE_T size);
        
        HRESULT ( STDMETHODCALLTYPE *AcquiredVirtualAddressSpace )( 
            IHostMemoryManager * This,
            /* [in] */ LPVOID startAddress,
            /* [in] */ SIZE_T size);
        
        HRESULT ( STDMETHODCALLTYPE *ReleasedVirtualAddressSpace )( 
            IHostMemoryManager * This,
            /* [in] */ LPVOID startAddress);
        
        END_INTERFACE
    } IHostMemoryManagerVtbl;

    interface IHostMemoryManager
    {
        CONST_VTBL struct IHostMemoryManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostMemoryManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostMemoryManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostMemoryManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostMemoryManager_CreateMalloc(This,dwMallocType,ppMalloc)	\
    ( (This)->lpVtbl -> CreateMalloc(This,dwMallocType,ppMalloc) ) 

#define IHostMemoryManager_VirtualAlloc(This,pAddress,dwSize,flAllocationType,flProtect,eCriticalLevel,ppMem)	\
    ( (This)->lpVtbl -> VirtualAlloc(This,pAddress,dwSize,flAllocationType,flProtect,eCriticalLevel,ppMem) ) 

#define IHostMemoryManager_VirtualFree(This,lpAddress,dwSize,dwFreeType)	\
    ( (This)->lpVtbl -> VirtualFree(This,lpAddress,dwSize,dwFreeType) ) 

#define IHostMemoryManager_VirtualQuery(This,lpAddress,lpBuffer,dwLength,pResult)	\
    ( (This)->lpVtbl -> VirtualQuery(This,lpAddress,lpBuffer,dwLength,pResult) ) 

#define IHostMemoryManager_VirtualProtect(This,lpAddress,dwSize,flNewProtect,pflOldProtect)	\
    ( (This)->lpVtbl -> VirtualProtect(This,lpAddress,dwSize,flNewProtect,pflOldProtect) ) 

#define IHostMemoryManager_GetMemoryLoad(This,pMemoryLoad,pAvailableBytes)	\
    ( (This)->lpVtbl -> GetMemoryLoad(This,pMemoryLoad,pAvailableBytes) ) 

#define IHostMemoryManager_RegisterMemoryNotificationCallback(This,pCallback)	\
    ( (This)->lpVtbl -> RegisterMemoryNotificationCallback(This,pCallback) ) 

#define IHostMemoryManager_NeedsVirtualAddressSpace(This,startAddress,size)	\
    ( (This)->lpVtbl -> NeedsVirtualAddressSpace(This,startAddress,size) ) 

#define IHostMemoryManager_AcquiredVirtualAddressSpace(This,startAddress,size)	\
    ( (This)->lpVtbl -> AcquiredVirtualAddressSpace(This,startAddress,size) ) 

#define IHostMemoryManager_ReleasedVirtualAddressSpace(This,startAddress)	\
    ( (This)->lpVtbl -> ReleasedVirtualAddressSpace(This,startAddress) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostMemoryManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0012 */
/* [local] */ 

typedef UINT64 TASKID;

typedef DWORD CONNID;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0012_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0012_v0_0_s_ifspec;

#ifndef __ICLRTask_INTERFACE_DEFINED__
#define __ICLRTask_INTERFACE_DEFINED__

/* interface ICLRTask */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRTask;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28E66A4A-9906-4225-B231-9187C3EB8611")
    ICLRTask : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SwitchIn( 
            /* [in] */ HANDLE threadHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SwitchOut( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMemStats( 
            /* [out] */ COR_GC_THREAD_STATS *memUsage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            BOOL fFull) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExitTask( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RudeAbort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NeedsPriorityScheduling( 
            /* [out] */ BOOL *pbNeedsPriorityScheduling) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE YieldTask( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LocksHeld( 
            /* [out] */ SIZE_T *pLockCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTaskIdentifier( 
            /* [in] */ TASKID asked) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRTaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRTask * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRTask * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchIn )( 
            ICLRTask * This,
            /* [in] */ HANDLE threadHandle);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchOut )( 
            ICLRTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemStats )( 
            ICLRTask * This,
            /* [out] */ COR_GC_THREAD_STATS *memUsage);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICLRTask * This,
            BOOL fFull);
        
        HRESULT ( STDMETHODCALLTYPE *ExitTask )( 
            ICLRTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ICLRTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *RudeAbort )( 
            ICLRTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *NeedsPriorityScheduling )( 
            ICLRTask * This,
            /* [out] */ BOOL *pbNeedsPriorityScheduling);
        
        HRESULT ( STDMETHODCALLTYPE *YieldTask )( 
            ICLRTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *LocksHeld )( 
            ICLRTask * This,
            /* [out] */ SIZE_T *pLockCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetTaskIdentifier )( 
            ICLRTask * This,
            /* [in] */ TASKID asked);
        
        END_INTERFACE
    } ICLRTaskVtbl;

    interface ICLRTask
    {
        CONST_VTBL struct ICLRTaskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRTask_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRTask_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRTask_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRTask_SwitchIn(This,threadHandle)	\
    ( (This)->lpVtbl -> SwitchIn(This,threadHandle) ) 

#define ICLRTask_SwitchOut(This)	\
    ( (This)->lpVtbl -> SwitchOut(This) ) 

#define ICLRTask_GetMemStats(This,memUsage)	\
    ( (This)->lpVtbl -> GetMemStats(This,memUsage) ) 

#define ICLRTask_Reset(This,fFull)	\
    ( (This)->lpVtbl -> Reset(This,fFull) ) 

#define ICLRTask_ExitTask(This)	\
    ( (This)->lpVtbl -> ExitTask(This) ) 

#define ICLRTask_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define ICLRTask_RudeAbort(This)	\
    ( (This)->lpVtbl -> RudeAbort(This) ) 

#define ICLRTask_NeedsPriorityScheduling(This,pbNeedsPriorityScheduling)	\
    ( (This)->lpVtbl -> NeedsPriorityScheduling(This,pbNeedsPriorityScheduling) ) 

#define ICLRTask_YieldTask(This)	\
    ( (This)->lpVtbl -> YieldTask(This) ) 

#define ICLRTask_LocksHeld(This,pLockCount)	\
    ( (This)->lpVtbl -> LocksHeld(This,pLockCount) ) 

#define ICLRTask_SetTaskIdentifier(This,asked)	\
    ( (This)->lpVtbl -> SetTaskIdentifier(This,asked) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRTask_INTERFACE_DEFINED__ */


#ifndef __ICLRTask2_INTERFACE_DEFINED__
#define __ICLRTask2_INTERFACE_DEFINED__

/* interface ICLRTask2 */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRTask2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28E66A4A-9906-4225-B231-9187C3EB8612")
    ICLRTask2 : public ICLRTask
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginPreventAsyncAbort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndPreventAsyncAbort( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRTask2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRTask2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRTask2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRTask2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchIn )( 
            ICLRTask2 * This,
            /* [in] */ HANDLE threadHandle);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchOut )( 
            ICLRTask2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMemStats )( 
            ICLRTask2 * This,
            /* [out] */ COR_GC_THREAD_STATS *memUsage);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICLRTask2 * This,
            BOOL fFull);
        
        HRESULT ( STDMETHODCALLTYPE *ExitTask )( 
            ICLRTask2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            ICLRTask2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *RudeAbort )( 
            ICLRTask2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *NeedsPriorityScheduling )( 
            ICLRTask2 * This,
            /* [out] */ BOOL *pbNeedsPriorityScheduling);
        
        HRESULT ( STDMETHODCALLTYPE *YieldTask )( 
            ICLRTask2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *LocksHeld )( 
            ICLRTask2 * This,
            /* [out] */ SIZE_T *pLockCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetTaskIdentifier )( 
            ICLRTask2 * This,
            /* [in] */ TASKID asked);
        
        HRESULT ( STDMETHODCALLTYPE *BeginPreventAsyncAbort )( 
            ICLRTask2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndPreventAsyncAbort )( 
            ICLRTask2 * This);
        
        END_INTERFACE
    } ICLRTask2Vtbl;

    interface ICLRTask2
    {
        CONST_VTBL struct ICLRTask2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRTask2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRTask2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRTask2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRTask2_SwitchIn(This,threadHandle)	\
    ( (This)->lpVtbl -> SwitchIn(This,threadHandle) ) 

#define ICLRTask2_SwitchOut(This)	\
    ( (This)->lpVtbl -> SwitchOut(This) ) 

#define ICLRTask2_GetMemStats(This,memUsage)	\
    ( (This)->lpVtbl -> GetMemStats(This,memUsage) ) 

#define ICLRTask2_Reset(This,fFull)	\
    ( (This)->lpVtbl -> Reset(This,fFull) ) 

#define ICLRTask2_ExitTask(This)	\
    ( (This)->lpVtbl -> ExitTask(This) ) 

#define ICLRTask2_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define ICLRTask2_RudeAbort(This)	\
    ( (This)->lpVtbl -> RudeAbort(This) ) 

#define ICLRTask2_NeedsPriorityScheduling(This,pbNeedsPriorityScheduling)	\
    ( (This)->lpVtbl -> NeedsPriorityScheduling(This,pbNeedsPriorityScheduling) ) 

#define ICLRTask2_YieldTask(This)	\
    ( (This)->lpVtbl -> YieldTask(This) ) 

#define ICLRTask2_LocksHeld(This,pLockCount)	\
    ( (This)->lpVtbl -> LocksHeld(This,pLockCount) ) 

#define ICLRTask2_SetTaskIdentifier(This,asked)	\
    ( (This)->lpVtbl -> SetTaskIdentifier(This,asked) ) 


#define ICLRTask2_BeginPreventAsyncAbort(This)	\
    ( (This)->lpVtbl -> BeginPreventAsyncAbort(This) ) 

#define ICLRTask2_EndPreventAsyncAbort(This)	\
    ( (This)->lpVtbl -> EndPreventAsyncAbort(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRTask2_INTERFACE_DEFINED__ */


#ifndef __IHostTask_INTERFACE_DEFINED__
#define __IHostTask_INTERFACE_DEFINED__

/* interface IHostTask */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostTask;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C2275828-C4B1-4B55-82C9-92135F74DF1A")
    IHostTask : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Alert( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Join( 
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPriority( 
            /* [in] */ int newPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ int *pPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCLRTask( 
            /* [in] */ ICLRTask *pCLRTask) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostTaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostTask * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostTask * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IHostTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *Alert )( 
            IHostTask * This);
        
        HRESULT ( STDMETHODCALLTYPE *Join )( 
            IHostTask * This,
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option);
        
        HRESULT ( STDMETHODCALLTYPE *SetPriority )( 
            IHostTask * This,
            /* [in] */ int newPriority);
        
        HRESULT ( STDMETHODCALLTYPE *GetPriority )( 
            IHostTask * This,
            /* [out] */ int *pPriority);
        
        HRESULT ( STDMETHODCALLTYPE *SetCLRTask )( 
            IHostTask * This,
            /* [in] */ ICLRTask *pCLRTask);
        
        END_INTERFACE
    } IHostTaskVtbl;

    interface IHostTask
    {
        CONST_VTBL struct IHostTaskVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostTask_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostTask_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostTask_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostTask_Start(This)	\
    ( (This)->lpVtbl -> Start(This) ) 

#define IHostTask_Alert(This)	\
    ( (This)->lpVtbl -> Alert(This) ) 

#define IHostTask_Join(This,dwMilliseconds,option)	\
    ( (This)->lpVtbl -> Join(This,dwMilliseconds,option) ) 

#define IHostTask_SetPriority(This,newPriority)	\
    ( (This)->lpVtbl -> SetPriority(This,newPriority) ) 

#define IHostTask_GetPriority(This,pPriority)	\
    ( (This)->lpVtbl -> GetPriority(This,pPriority) ) 

#define IHostTask_SetCLRTask(This,pCLRTask)	\
    ( (This)->lpVtbl -> SetCLRTask(This,pCLRTask) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostTask_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0015 */
/* [local] */ 

typedef 
enum ETaskType
    {
        TT_DEBUGGERHELPER	= 0x1,
        TT_GC	= 0x2,
        TT_FINALIZER	= 0x4,
        TT_THREADPOOL_TIMER	= 0x8,
        TT_THREADPOOL_GATE	= 0x10,
        TT_THREADPOOL_WORKER	= 0x20,
        TT_THREADPOOL_IOCOMPLETION	= 0x40,
        TT_ADUNLOAD	= 0x80,
        TT_USER	= 0x100,
        TT_THREADPOOL_WAIT	= 0x200,
        TT_UNKNOWN	= 0x80000000
    } 	ETaskType;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0015_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0015_v0_0_s_ifspec;

#ifndef __ICLRTaskManager_INTERFACE_DEFINED__
#define __ICLRTaskManager_INTERFACE_DEFINED__

/* interface ICLRTaskManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRTaskManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4862efbe-3ae5-44f8-8feb-346190ee8a34")
    ICLRTaskManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTask( 
            /* [out] */ ICLRTask **pTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentTask( 
            /* [out] */ ICLRTask **pTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUILocale( 
            /* [in] */ LCID lcid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLocale( 
            /* [in] */ LCID lcid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentTaskType( 
            /* [out] */ ETaskType *pTaskType) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRTaskManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRTaskManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRTaskManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTask )( 
            ICLRTaskManager * This,
            /* [out] */ ICLRTask **pTask);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentTask )( 
            ICLRTaskManager * This,
            /* [out] */ ICLRTask **pTask);
        
        HRESULT ( STDMETHODCALLTYPE *SetUILocale )( 
            ICLRTaskManager * This,
            /* [in] */ LCID lcid);
        
        HRESULT ( STDMETHODCALLTYPE *SetLocale )( 
            ICLRTaskManager * This,
            /* [in] */ LCID lcid);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentTaskType )( 
            ICLRTaskManager * This,
            /* [out] */ ETaskType *pTaskType);
        
        END_INTERFACE
    } ICLRTaskManagerVtbl;

    interface ICLRTaskManager
    {
        CONST_VTBL struct ICLRTaskManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRTaskManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRTaskManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRTaskManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRTaskManager_CreateTask(This,pTask)	\
    ( (This)->lpVtbl -> CreateTask(This,pTask) ) 

#define ICLRTaskManager_GetCurrentTask(This,pTask)	\
    ( (This)->lpVtbl -> GetCurrentTask(This,pTask) ) 

#define ICLRTaskManager_SetUILocale(This,lcid)	\
    ( (This)->lpVtbl -> SetUILocale(This,lcid) ) 

#define ICLRTaskManager_SetLocale(This,lcid)	\
    ( (This)->lpVtbl -> SetLocale(This,lcid) ) 

#define ICLRTaskManager_GetCurrentTaskType(This,pTaskType)	\
    ( (This)->lpVtbl -> GetCurrentTaskType(This,pTaskType) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRTaskManager_INTERFACE_DEFINED__ */


#ifndef __IHostTaskManager_INTERFACE_DEFINED__
#define __IHostTaskManager_INTERFACE_DEFINED__

/* interface IHostTaskManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostTaskManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("997FF24C-43B7-4352-8667-0DC04FAFD354")
    IHostTaskManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCurrentTask( 
            /* [out] */ IHostTask **pTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTask( 
            /* [in] */ DWORD dwStackSize,
            /* [in] */ LPTHREAD_START_ROUTINE pStartAddress,
            /* [in] */ PVOID pParameter,
            /* [out] */ IHostTask **ppTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Sleep( 
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SwitchToTask( 
            /* [in] */ DWORD option) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUILocale( 
            /* [in] */ LCID lcid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLocale( 
            /* [in] */ LCID lcid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CallNeedsHostHook( 
            /* [in] */ SIZE_T target,
            /* [out] */ BOOL *pbCallNeedsHostHook) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LeaveRuntime( 
            /* [in] */ SIZE_T target) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnterRuntime( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReverseLeaveRuntime( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReverseEnterRuntime( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginDelayAbort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndDelayAbort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginThreadAffinity( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndThreadAffinity( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStackGuarantee( 
            /* [in] */ ULONG guarantee) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStackGuarantee( 
            /* [out] */ ULONG *pGuarantee) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCLRTaskManager( 
            /* [in] */ ICLRTaskManager *ppManager) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostTaskManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostTaskManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostTaskManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentTask )( 
            IHostTaskManager * This,
            /* [out] */ IHostTask **pTask);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTask )( 
            IHostTaskManager * This,
            /* [in] */ DWORD dwStackSize,
            /* [in] */ LPTHREAD_START_ROUTINE pStartAddress,
            /* [in] */ PVOID pParameter,
            /* [out] */ IHostTask **ppTask);
        
        HRESULT ( STDMETHODCALLTYPE *Sleep )( 
            IHostTaskManager * This,
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option);
        
        HRESULT ( STDMETHODCALLTYPE *SwitchToTask )( 
            IHostTaskManager * This,
            /* [in] */ DWORD option);
        
        HRESULT ( STDMETHODCALLTYPE *SetUILocale )( 
            IHostTaskManager * This,
            /* [in] */ LCID lcid);
        
        HRESULT ( STDMETHODCALLTYPE *SetLocale )( 
            IHostTaskManager * This,
            /* [in] */ LCID lcid);
        
        HRESULT ( STDMETHODCALLTYPE *CallNeedsHostHook )( 
            IHostTaskManager * This,
            /* [in] */ SIZE_T target,
            /* [out] */ BOOL *pbCallNeedsHostHook);
        
        HRESULT ( STDMETHODCALLTYPE *LeaveRuntime )( 
            IHostTaskManager * This,
            /* [in] */ SIZE_T target);
        
        HRESULT ( STDMETHODCALLTYPE *EnterRuntime )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReverseLeaveRuntime )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReverseEnterRuntime )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginDelayAbort )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndDelayAbort )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginThreadAffinity )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndThreadAffinity )( 
            IHostTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetStackGuarantee )( 
            IHostTaskManager * This,
            /* [in] */ ULONG guarantee);
        
        HRESULT ( STDMETHODCALLTYPE *GetStackGuarantee )( 
            IHostTaskManager * This,
            /* [out] */ ULONG *pGuarantee);
        
        HRESULT ( STDMETHODCALLTYPE *SetCLRTaskManager )( 
            IHostTaskManager * This,
            /* [in] */ ICLRTaskManager *ppManager);
        
        END_INTERFACE
    } IHostTaskManagerVtbl;

    interface IHostTaskManager
    {
        CONST_VTBL struct IHostTaskManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostTaskManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostTaskManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostTaskManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostTaskManager_GetCurrentTask(This,pTask)	\
    ( (This)->lpVtbl -> GetCurrentTask(This,pTask) ) 

#define IHostTaskManager_CreateTask(This,dwStackSize,pStartAddress,pParameter,ppTask)	\
    ( (This)->lpVtbl -> CreateTask(This,dwStackSize,pStartAddress,pParameter,ppTask) ) 

#define IHostTaskManager_Sleep(This,dwMilliseconds,option)	\
    ( (This)->lpVtbl -> Sleep(This,dwMilliseconds,option) ) 

#define IHostTaskManager_SwitchToTask(This,option)	\
    ( (This)->lpVtbl -> SwitchToTask(This,option) ) 

#define IHostTaskManager_SetUILocale(This,lcid)	\
    ( (This)->lpVtbl -> SetUILocale(This,lcid) ) 

#define IHostTaskManager_SetLocale(This,lcid)	\
    ( (This)->lpVtbl -> SetLocale(This,lcid) ) 

#define IHostTaskManager_CallNeedsHostHook(This,target,pbCallNeedsHostHook)	\
    ( (This)->lpVtbl -> CallNeedsHostHook(This,target,pbCallNeedsHostHook) ) 

#define IHostTaskManager_LeaveRuntime(This,target)	\
    ( (This)->lpVtbl -> LeaveRuntime(This,target) ) 

#define IHostTaskManager_EnterRuntime(This)	\
    ( (This)->lpVtbl -> EnterRuntime(This) ) 

#define IHostTaskManager_ReverseLeaveRuntime(This)	\
    ( (This)->lpVtbl -> ReverseLeaveRuntime(This) ) 

#define IHostTaskManager_ReverseEnterRuntime(This)	\
    ( (This)->lpVtbl -> ReverseEnterRuntime(This) ) 

#define IHostTaskManager_BeginDelayAbort(This)	\
    ( (This)->lpVtbl -> BeginDelayAbort(This) ) 

#define IHostTaskManager_EndDelayAbort(This)	\
    ( (This)->lpVtbl -> EndDelayAbort(This) ) 

#define IHostTaskManager_BeginThreadAffinity(This)	\
    ( (This)->lpVtbl -> BeginThreadAffinity(This) ) 

#define IHostTaskManager_EndThreadAffinity(This)	\
    ( (This)->lpVtbl -> EndThreadAffinity(This) ) 

#define IHostTaskManager_SetStackGuarantee(This,guarantee)	\
    ( (This)->lpVtbl -> SetStackGuarantee(This,guarantee) ) 

#define IHostTaskManager_GetStackGuarantee(This,pGuarantee)	\
    ( (This)->lpVtbl -> GetStackGuarantee(This,pGuarantee) ) 

#define IHostTaskManager_SetCLRTaskManager(This,ppManager)	\
    ( (This)->lpVtbl -> SetCLRTaskManager(This,ppManager) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostTaskManager_INTERFACE_DEFINED__ */


#ifndef __IHostThreadpoolManager_INTERFACE_DEFINED__
#define __IHostThreadpoolManager_INTERFACE_DEFINED__

/* interface IHostThreadpoolManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostThreadpoolManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("983D50E2-CB15-466B-80FC-845DC6E8C5FD")
    IHostThreadpoolManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueueUserWorkItem( 
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [in] */ ULONG Flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxThreads( 
            /* [in] */ DWORD dwMaxWorkerThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxThreads( 
            /* [out] */ DWORD *pdwMaxWorkerThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableThreads( 
            /* [out] */ DWORD *pdwAvailableWorkerThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMinThreads( 
            /* [in] */ DWORD dwMinIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMinThreads( 
            /* [out] */ DWORD *pdwMinIOCompletionThreads) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostThreadpoolManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostThreadpoolManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostThreadpoolManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostThreadpoolManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueueUserWorkItem )( 
            IHostThreadpoolManager * This,
            /* [in] */ LPTHREAD_START_ROUTINE Function,
            /* [in] */ PVOID Context,
            /* [in] */ ULONG Flags);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaxThreads )( 
            IHostThreadpoolManager * This,
            /* [in] */ DWORD dwMaxWorkerThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxThreads )( 
            IHostThreadpoolManager * This,
            /* [out] */ DWORD *pdwMaxWorkerThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetAvailableThreads )( 
            IHostThreadpoolManager * This,
            /* [out] */ DWORD *pdwAvailableWorkerThreads);
        
        HRESULT ( STDMETHODCALLTYPE *SetMinThreads )( 
            IHostThreadpoolManager * This,
            /* [in] */ DWORD dwMinIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetMinThreads )( 
            IHostThreadpoolManager * This,
            /* [out] */ DWORD *pdwMinIOCompletionThreads);
        
        END_INTERFACE
    } IHostThreadpoolManagerVtbl;

    interface IHostThreadpoolManager
    {
        CONST_VTBL struct IHostThreadpoolManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostThreadpoolManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostThreadpoolManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostThreadpoolManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostThreadpoolManager_QueueUserWorkItem(This,Function,Context,Flags)	\
    ( (This)->lpVtbl -> QueueUserWorkItem(This,Function,Context,Flags) ) 

#define IHostThreadpoolManager_SetMaxThreads(This,dwMaxWorkerThreads)	\
    ( (This)->lpVtbl -> SetMaxThreads(This,dwMaxWorkerThreads) ) 

#define IHostThreadpoolManager_GetMaxThreads(This,pdwMaxWorkerThreads)	\
    ( (This)->lpVtbl -> GetMaxThreads(This,pdwMaxWorkerThreads) ) 

#define IHostThreadpoolManager_GetAvailableThreads(This,pdwAvailableWorkerThreads)	\
    ( (This)->lpVtbl -> GetAvailableThreads(This,pdwAvailableWorkerThreads) ) 

#define IHostThreadpoolManager_SetMinThreads(This,dwMinIOCompletionThreads)	\
    ( (This)->lpVtbl -> SetMinThreads(This,dwMinIOCompletionThreads) ) 

#define IHostThreadpoolManager_GetMinThreads(This,pdwMinIOCompletionThreads)	\
    ( (This)->lpVtbl -> GetMinThreads(This,pdwMinIOCompletionThreads) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostThreadpoolManager_INTERFACE_DEFINED__ */


#ifndef __ICLRIoCompletionManager_INTERFACE_DEFINED__
#define __ICLRIoCompletionManager_INTERFACE_DEFINED__

/* interface ICLRIoCompletionManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRIoCompletionManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2d74ce86-b8d6-4c84-b3a7-9768933b3c12")
    ICLRIoCompletionManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnComplete( 
            /* [in] */ DWORD dwErrorCode,
            /* [in] */ DWORD NumberOfBytesTransferred,
            /* [in] */ void *pvOverlapped) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRIoCompletionManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRIoCompletionManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRIoCompletionManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRIoCompletionManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnComplete )( 
            ICLRIoCompletionManager * This,
            /* [in] */ DWORD dwErrorCode,
            /* [in] */ DWORD NumberOfBytesTransferred,
            /* [in] */ void *pvOverlapped);
        
        END_INTERFACE
    } ICLRIoCompletionManagerVtbl;

    interface ICLRIoCompletionManager
    {
        CONST_VTBL struct ICLRIoCompletionManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRIoCompletionManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRIoCompletionManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRIoCompletionManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRIoCompletionManager_OnComplete(This,dwErrorCode,NumberOfBytesTransferred,pvOverlapped)	\
    ( (This)->lpVtbl -> OnComplete(This,dwErrorCode,NumberOfBytesTransferred,pvOverlapped) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRIoCompletionManager_INTERFACE_DEFINED__ */


#ifndef __IHostIoCompletionManager_INTERFACE_DEFINED__
#define __IHostIoCompletionManager_INTERFACE_DEFINED__

/* interface IHostIoCompletionManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostIoCompletionManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8bde9d80-ec06-41d6-83e6-22580effcc20")
    IHostIoCompletionManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateIoCompletionPort( 
            /* [out] */ HANDLE *phPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseIoCompletionPort( 
            /* [in] */ HANDLE hPort) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxThreads( 
            /* [in] */ DWORD dwMaxIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxThreads( 
            /* [out] */ DWORD *pdwMaxIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableThreads( 
            /* [out] */ DWORD *pdwAvailableIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHostOverlappedSize( 
            /* [out] */ DWORD *pcbSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCLRIoCompletionManager( 
            /* [in] */ ICLRIoCompletionManager *pManager) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeHostOverlapped( 
            /* [in] */ void *pvOverlapped) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Bind( 
            /* [in] */ HANDLE hPort,
            /* [in] */ HANDLE hHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMinThreads( 
            /* [in] */ DWORD dwMinIOCompletionThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMinThreads( 
            /* [out] */ DWORD *pdwMinIOCompletionThreads) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostIoCompletionManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostIoCompletionManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostIoCompletionManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostIoCompletionManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateIoCompletionPort )( 
            IHostIoCompletionManager * This,
            /* [out] */ HANDLE *phPort);
        
        HRESULT ( STDMETHODCALLTYPE *CloseIoCompletionPort )( 
            IHostIoCompletionManager * This,
            /* [in] */ HANDLE hPort);
        
        HRESULT ( STDMETHODCALLTYPE *SetMaxThreads )( 
            IHostIoCompletionManager * This,
            /* [in] */ DWORD dwMaxIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxThreads )( 
            IHostIoCompletionManager * This,
            /* [out] */ DWORD *pdwMaxIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetAvailableThreads )( 
            IHostIoCompletionManager * This,
            /* [out] */ DWORD *pdwAvailableIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostOverlappedSize )( 
            IHostIoCompletionManager * This,
            /* [out] */ DWORD *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetCLRIoCompletionManager )( 
            IHostIoCompletionManager * This,
            /* [in] */ ICLRIoCompletionManager *pManager);
        
        HRESULT ( STDMETHODCALLTYPE *InitializeHostOverlapped )( 
            IHostIoCompletionManager * This,
            /* [in] */ void *pvOverlapped);
        
        HRESULT ( STDMETHODCALLTYPE *Bind )( 
            IHostIoCompletionManager * This,
            /* [in] */ HANDLE hPort,
            /* [in] */ HANDLE hHandle);
        
        HRESULT ( STDMETHODCALLTYPE *SetMinThreads )( 
            IHostIoCompletionManager * This,
            /* [in] */ DWORD dwMinIOCompletionThreads);
        
        HRESULT ( STDMETHODCALLTYPE *GetMinThreads )( 
            IHostIoCompletionManager * This,
            /* [out] */ DWORD *pdwMinIOCompletionThreads);
        
        END_INTERFACE
    } IHostIoCompletionManagerVtbl;

    interface IHostIoCompletionManager
    {
        CONST_VTBL struct IHostIoCompletionManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostIoCompletionManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostIoCompletionManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostIoCompletionManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostIoCompletionManager_CreateIoCompletionPort(This,phPort)	\
    ( (This)->lpVtbl -> CreateIoCompletionPort(This,phPort) ) 

#define IHostIoCompletionManager_CloseIoCompletionPort(This,hPort)	\
    ( (This)->lpVtbl -> CloseIoCompletionPort(This,hPort) ) 

#define IHostIoCompletionManager_SetMaxThreads(This,dwMaxIOCompletionThreads)	\
    ( (This)->lpVtbl -> SetMaxThreads(This,dwMaxIOCompletionThreads) ) 

#define IHostIoCompletionManager_GetMaxThreads(This,pdwMaxIOCompletionThreads)	\
    ( (This)->lpVtbl -> GetMaxThreads(This,pdwMaxIOCompletionThreads) ) 

#define IHostIoCompletionManager_GetAvailableThreads(This,pdwAvailableIOCompletionThreads)	\
    ( (This)->lpVtbl -> GetAvailableThreads(This,pdwAvailableIOCompletionThreads) ) 

#define IHostIoCompletionManager_GetHostOverlappedSize(This,pcbSize)	\
    ( (This)->lpVtbl -> GetHostOverlappedSize(This,pcbSize) ) 

#define IHostIoCompletionManager_SetCLRIoCompletionManager(This,pManager)	\
    ( (This)->lpVtbl -> SetCLRIoCompletionManager(This,pManager) ) 

#define IHostIoCompletionManager_InitializeHostOverlapped(This,pvOverlapped)	\
    ( (This)->lpVtbl -> InitializeHostOverlapped(This,pvOverlapped) ) 

#define IHostIoCompletionManager_Bind(This,hPort,hHandle)	\
    ( (This)->lpVtbl -> Bind(This,hPort,hHandle) ) 

#define IHostIoCompletionManager_SetMinThreads(This,dwMinIOCompletionThreads)	\
    ( (This)->lpVtbl -> SetMinThreads(This,dwMinIOCompletionThreads) ) 

#define IHostIoCompletionManager_GetMinThreads(This,pdwMinIOCompletionThreads)	\
    ( (This)->lpVtbl -> GetMinThreads(This,pdwMinIOCompletionThreads) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostIoCompletionManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0020 */
/* [local] */ 

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0020_0001
    {
        eSymbolReadingNever	= 0,
        eSymbolReadingAlways	= 1,
        eSymbolReadingFullTrustOnly	= 2
    } 	ESymbolReadingPolicy;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0020_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0020_v0_0_s_ifspec;

#ifndef __ICLRDebugManager_INTERFACE_DEFINED__
#define __ICLRDebugManager_INTERFACE_DEFINED__

/* interface ICLRDebugManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRDebugManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00DCAEC6-2AC0-43a9-ACF9-1E36C139B10D")
    ICLRDebugManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE BeginConnection( 
            /* [in] */ CONNID dwConnectionId,
            /* [annotation][string][in] */ 
            _In_  wchar_t *szConnectionName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConnectionTasks( 
            /* [in] */ CONNID id,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ ICLRTask **ppCLRTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndConnection( 
            /* [in] */ CONNID dwConnectionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDacl( 
            /* [in] */ PACL pacl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDacl( 
            /* [out] */ PACL *pacl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDebuggerAttached( 
            /* [out] */ BOOL *pbAttached) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSymbolReadingPolicy( 
            /* [in] */ ESymbolReadingPolicy policy) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRDebugManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRDebugManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRDebugManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRDebugManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *BeginConnection )( 
            ICLRDebugManager * This,
            /* [in] */ CONNID dwConnectionId,
            /* [annotation][string][in] */ 
            _In_  wchar_t *szConnectionName);
        
        HRESULT ( STDMETHODCALLTYPE *SetConnectionTasks )( 
            ICLRDebugManager * This,
            /* [in] */ CONNID id,
            /* [in] */ DWORD dwCount,
            /* [size_is][in] */ ICLRTask **ppCLRTask);
        
        HRESULT ( STDMETHODCALLTYPE *EndConnection )( 
            ICLRDebugManager * This,
            /* [in] */ CONNID dwConnectionId);
        
        HRESULT ( STDMETHODCALLTYPE *SetDacl )( 
            ICLRDebugManager * This,
            /* [in] */ PACL pacl);
        
        HRESULT ( STDMETHODCALLTYPE *GetDacl )( 
            ICLRDebugManager * This,
            /* [out] */ PACL *pacl);
        
        HRESULT ( STDMETHODCALLTYPE *IsDebuggerAttached )( 
            ICLRDebugManager * This,
            /* [out] */ BOOL *pbAttached);
        
        HRESULT ( STDMETHODCALLTYPE *SetSymbolReadingPolicy )( 
            ICLRDebugManager * This,
            /* [in] */ ESymbolReadingPolicy policy);
        
        END_INTERFACE
    } ICLRDebugManagerVtbl;

    interface ICLRDebugManager
    {
        CONST_VTBL struct ICLRDebugManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRDebugManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRDebugManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRDebugManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRDebugManager_BeginConnection(This,dwConnectionId,szConnectionName)	\
    ( (This)->lpVtbl -> BeginConnection(This,dwConnectionId,szConnectionName) ) 

#define ICLRDebugManager_SetConnectionTasks(This,id,dwCount,ppCLRTask)	\
    ( (This)->lpVtbl -> SetConnectionTasks(This,id,dwCount,ppCLRTask) ) 

#define ICLRDebugManager_EndConnection(This,dwConnectionId)	\
    ( (This)->lpVtbl -> EndConnection(This,dwConnectionId) ) 

#define ICLRDebugManager_SetDacl(This,pacl)	\
    ( (This)->lpVtbl -> SetDacl(This,pacl) ) 

#define ICLRDebugManager_GetDacl(This,pacl)	\
    ( (This)->lpVtbl -> GetDacl(This,pacl) ) 

#define ICLRDebugManager_IsDebuggerAttached(This,pbAttached)	\
    ( (This)->lpVtbl -> IsDebuggerAttached(This,pbAttached) ) 

#define ICLRDebugManager_SetSymbolReadingPolicy(This,policy)	\
    ( (This)->lpVtbl -> SetSymbolReadingPolicy(This,policy) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRDebugManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0021 */
/* [local] */ 

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0021_0001
    {
        DUMP_FLAVOR_Mini	= 0,
        DUMP_FLAVOR_CriticalCLRState	= 1,
        DUMP_FLAVOR_NonHeapCLRState	= 2,
        DUMP_FLAVOR_Default	= DUMP_FLAVOR_Mini
    } 	ECustomDumpFlavor;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0021_0002
    {
        DUMP_ITEM_None	= 0
    } 	ECustomDumpItemKind;

typedef /* [public][public] */ struct __MIDL___MIDL_itf_mscoree_0000_0021_0003
    {
    ECustomDumpItemKind itemKind;
    union 
        {
        UINT_PTR pReserved;
        } 	;
    } 	CustomDumpItem;

#define	BucketParamsCount	( 10 )

#define	BucketParamLength	( 255 )

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0021_0005
    {
        Parameter1	= 0,
        Parameter2	= ( Parameter1 + 1 ) ,
        Parameter3	= ( Parameter2 + 1 ) ,
        Parameter4	= ( Parameter3 + 1 ) ,
        Parameter5	= ( Parameter4 + 1 ) ,
        Parameter6	= ( Parameter5 + 1 ) ,
        Parameter7	= ( Parameter6 + 1 ) ,
        Parameter8	= ( Parameter7 + 1 ) ,
        Parameter9	= ( Parameter8 + 1 ) ,
        InvalidBucketParamIndex	= ( Parameter9 + 1 ) 
    } 	BucketParameterIndex;

typedef struct _BucketParameters
    {
    BOOL fInited;
    WCHAR pszEventTypeName[ 255 ];
    WCHAR pszParams[ 10 ][ 255 ];
    } 	BucketParameters;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0021_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0021_v0_0_s_ifspec;

#ifndef __ICLRErrorReportingManager_INTERFACE_DEFINED__
#define __ICLRErrorReportingManager_INTERFACE_DEFINED__

/* interface ICLRErrorReportingManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRErrorReportingManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("980D2F1A-BF79-4c08-812A-BB9778928F78")
    ICLRErrorReportingManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBucketParametersForCurrentException( 
            /* [out] */ BucketParameters *pParams) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginCustomDump( 
            /* [in] */ ECustomDumpFlavor dwFlavor,
            /* [in] */ DWORD dwNumItems,
            /* [length_is][size_is][in] */ CustomDumpItem *items,
            DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndCustomDump( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRErrorReportingManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRErrorReportingManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRErrorReportingManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRErrorReportingManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBucketParametersForCurrentException )( 
            ICLRErrorReportingManager * This,
            /* [out] */ BucketParameters *pParams);
        
        HRESULT ( STDMETHODCALLTYPE *BeginCustomDump )( 
            ICLRErrorReportingManager * This,
            /* [in] */ ECustomDumpFlavor dwFlavor,
            /* [in] */ DWORD dwNumItems,
            /* [length_is][size_is][in] */ CustomDumpItem *items,
            DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *EndCustomDump )( 
            ICLRErrorReportingManager * This);
        
        END_INTERFACE
    } ICLRErrorReportingManagerVtbl;

    interface ICLRErrorReportingManager
    {
        CONST_VTBL struct ICLRErrorReportingManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRErrorReportingManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRErrorReportingManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRErrorReportingManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRErrorReportingManager_GetBucketParametersForCurrentException(This,pParams)	\
    ( (This)->lpVtbl -> GetBucketParametersForCurrentException(This,pParams) ) 

#define ICLRErrorReportingManager_BeginCustomDump(This,dwFlavor,dwNumItems,items,dwReserved)	\
    ( (This)->lpVtbl -> BeginCustomDump(This,dwFlavor,dwNumItems,items,dwReserved) ) 

#define ICLRErrorReportingManager_EndCustomDump(This)	\
    ( (This)->lpVtbl -> EndCustomDump(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRErrorReportingManager_INTERFACE_DEFINED__ */


#ifndef __IHostCrst_INTERFACE_DEFINED__
#define __IHostCrst_INTERFACE_DEFINED__

/* interface IHostCrst */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostCrst;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6DF710A6-26A4-4a65-8CD5-7237B8BDA8DC")
    IHostCrst : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Enter( 
            /* [in] */ DWORD option) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Leave( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TryEnter( 
            /* [in] */ DWORD option,
            /* [out] */ BOOL *pbSucceeded) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSpinCount( 
            /* [in] */ DWORD dwSpinCount) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostCrstVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostCrst * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostCrst * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostCrst * This);
        
        HRESULT ( STDMETHODCALLTYPE *Enter )( 
            IHostCrst * This,
            /* [in] */ DWORD option);
        
        HRESULT ( STDMETHODCALLTYPE *Leave )( 
            IHostCrst * This);
        
        HRESULT ( STDMETHODCALLTYPE *TryEnter )( 
            IHostCrst * This,
            /* [in] */ DWORD option,
            /* [out] */ BOOL *pbSucceeded);
        
        HRESULT ( STDMETHODCALLTYPE *SetSpinCount )( 
            IHostCrst * This,
            /* [in] */ DWORD dwSpinCount);
        
        END_INTERFACE
    } IHostCrstVtbl;

    interface IHostCrst
    {
        CONST_VTBL struct IHostCrstVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostCrst_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostCrst_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostCrst_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostCrst_Enter(This,option)	\
    ( (This)->lpVtbl -> Enter(This,option) ) 

#define IHostCrst_Leave(This)	\
    ( (This)->lpVtbl -> Leave(This) ) 

#define IHostCrst_TryEnter(This,option,pbSucceeded)	\
    ( (This)->lpVtbl -> TryEnter(This,option,pbSucceeded) ) 

#define IHostCrst_SetSpinCount(This,dwSpinCount)	\
    ( (This)->lpVtbl -> SetSpinCount(This,dwSpinCount) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostCrst_INTERFACE_DEFINED__ */


#ifndef __IHostAutoEvent_INTERFACE_DEFINED__
#define __IHostAutoEvent_INTERFACE_DEFINED__

/* interface IHostAutoEvent */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostAutoEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("50B0CFCE-4063-4278-9673-E5CB4ED0BDB8")
    IHostAutoEvent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Wait( 
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostAutoEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostAutoEvent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostAutoEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostAutoEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *Wait )( 
            IHostAutoEvent * This,
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            IHostAutoEvent * This);
        
        END_INTERFACE
    } IHostAutoEventVtbl;

    interface IHostAutoEvent
    {
        CONST_VTBL struct IHostAutoEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostAutoEvent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostAutoEvent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostAutoEvent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostAutoEvent_Wait(This,dwMilliseconds,option)	\
    ( (This)->lpVtbl -> Wait(This,dwMilliseconds,option) ) 

#define IHostAutoEvent_Set(This)	\
    ( (This)->lpVtbl -> Set(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostAutoEvent_INTERFACE_DEFINED__ */


#ifndef __IHostManualEvent_INTERFACE_DEFINED__
#define __IHostManualEvent_INTERFACE_DEFINED__

/* interface IHostManualEvent */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostManualEvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1BF4EC38-AFFE-4fb9-85A6-525268F15B54")
    IHostManualEvent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Wait( 
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostManualEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostManualEvent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostManualEvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostManualEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *Wait )( 
            IHostManualEvent * This,
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IHostManualEvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *Set )( 
            IHostManualEvent * This);
        
        END_INTERFACE
    } IHostManualEventVtbl;

    interface IHostManualEvent
    {
        CONST_VTBL struct IHostManualEventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostManualEvent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostManualEvent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostManualEvent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostManualEvent_Wait(This,dwMilliseconds,option)	\
    ( (This)->lpVtbl -> Wait(This,dwMilliseconds,option) ) 

#define IHostManualEvent_Reset(This)	\
    ( (This)->lpVtbl -> Reset(This) ) 

#define IHostManualEvent_Set(This)	\
    ( (This)->lpVtbl -> Set(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostManualEvent_INTERFACE_DEFINED__ */


#ifndef __IHostSemaphore_INTERFACE_DEFINED__
#define __IHostSemaphore_INTERFACE_DEFINED__

/* interface IHostSemaphore */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostSemaphore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("855efd47-cc09-463a-a97d-16acab882661")
    IHostSemaphore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Wait( 
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseSemaphore( 
            /* [in] */ LONG lReleaseCount,
            /* [out] */ LONG *lpPreviousCount) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostSemaphoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostSemaphore * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostSemaphore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostSemaphore * This);
        
        HRESULT ( STDMETHODCALLTYPE *Wait )( 
            IHostSemaphore * This,
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ DWORD option);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseSemaphore )( 
            IHostSemaphore * This,
            /* [in] */ LONG lReleaseCount,
            /* [out] */ LONG *lpPreviousCount);
        
        END_INTERFACE
    } IHostSemaphoreVtbl;

    interface IHostSemaphore
    {
        CONST_VTBL struct IHostSemaphoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostSemaphore_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostSemaphore_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostSemaphore_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostSemaphore_Wait(This,dwMilliseconds,option)	\
    ( (This)->lpVtbl -> Wait(This,dwMilliseconds,option) ) 

#define IHostSemaphore_ReleaseSemaphore(This,lReleaseCount,lpPreviousCount)	\
    ( (This)->lpVtbl -> ReleaseSemaphore(This,lReleaseCount,lpPreviousCount) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostSemaphore_INTERFACE_DEFINED__ */


#ifndef __ICLRSyncManager_INTERFACE_DEFINED__
#define __ICLRSyncManager_INTERFACE_DEFINED__

/* interface ICLRSyncManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRSyncManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("55FF199D-AD21-48f9-A16C-F24EBBB8727D")
    ICLRSyncManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMonitorOwner( 
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostTask **ppOwnerHostTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRWLockOwnerIterator( 
            /* [in] */ SIZE_T Cookie,
            /* [out] */ SIZE_T *pIterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRWLockOwnerNext( 
            /* [in] */ SIZE_T Iterator,
            /* [out] */ IHostTask **ppOwnerHostTask) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRWLockOwnerIterator( 
            /* [in] */ SIZE_T Iterator) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRSyncManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRSyncManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRSyncManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRSyncManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMonitorOwner )( 
            ICLRSyncManager * This,
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostTask **ppOwnerHostTask);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRWLockOwnerIterator )( 
            ICLRSyncManager * This,
            /* [in] */ SIZE_T Cookie,
            /* [out] */ SIZE_T *pIterator);
        
        HRESULT ( STDMETHODCALLTYPE *GetRWLockOwnerNext )( 
            ICLRSyncManager * This,
            /* [in] */ SIZE_T Iterator,
            /* [out] */ IHostTask **ppOwnerHostTask);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteRWLockOwnerIterator )( 
            ICLRSyncManager * This,
            /* [in] */ SIZE_T Iterator);
        
        END_INTERFACE
    } ICLRSyncManagerVtbl;

    interface ICLRSyncManager
    {
        CONST_VTBL struct ICLRSyncManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRSyncManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRSyncManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRSyncManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRSyncManager_GetMonitorOwner(This,Cookie,ppOwnerHostTask)	\
    ( (This)->lpVtbl -> GetMonitorOwner(This,Cookie,ppOwnerHostTask) ) 

#define ICLRSyncManager_CreateRWLockOwnerIterator(This,Cookie,pIterator)	\
    ( (This)->lpVtbl -> CreateRWLockOwnerIterator(This,Cookie,pIterator) ) 

#define ICLRSyncManager_GetRWLockOwnerNext(This,Iterator,ppOwnerHostTask)	\
    ( (This)->lpVtbl -> GetRWLockOwnerNext(This,Iterator,ppOwnerHostTask) ) 

#define ICLRSyncManager_DeleteRWLockOwnerIterator(This,Iterator)	\
    ( (This)->lpVtbl -> DeleteRWLockOwnerIterator(This,Iterator) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRSyncManager_INTERFACE_DEFINED__ */


#ifndef __IHostSyncManager_INTERFACE_DEFINED__
#define __IHostSyncManager_INTERFACE_DEFINED__

/* interface IHostSyncManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostSyncManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("234330c7-5f10-4f20-9615-5122dab7a0ac")
    IHostSyncManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetCLRSyncManager( 
            /* [in] */ ICLRSyncManager *pManager) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCrst( 
            /* [out] */ IHostCrst **ppCrst) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateCrstWithSpinCount( 
            /* [in] */ DWORD dwSpinCount,
            /* [out] */ IHostCrst **ppCrst) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAutoEvent( 
            /* [out] */ IHostAutoEvent **ppEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateManualEvent( 
            /* [in] */ BOOL bInitialState,
            /* [out] */ IHostManualEvent **ppEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateMonitorEvent( 
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostAutoEvent **ppEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRWLockWriterEvent( 
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostAutoEvent **ppEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateRWLockReaderEvent( 
            /* [in] */ BOOL bInitialState,
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostManualEvent **ppEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateSemaphore( 
            /* [in] */ DWORD dwInitial,
            /* [in] */ DWORD dwMax,
            /* [out] */ IHostSemaphore **ppSemaphore) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostSyncManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostSyncManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostSyncManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostSyncManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetCLRSyncManager )( 
            IHostSyncManager * This,
            /* [in] */ ICLRSyncManager *pManager);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCrst )( 
            IHostSyncManager * This,
            /* [out] */ IHostCrst **ppCrst);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCrstWithSpinCount )( 
            IHostSyncManager * This,
            /* [in] */ DWORD dwSpinCount,
            /* [out] */ IHostCrst **ppCrst);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAutoEvent )( 
            IHostSyncManager * This,
            /* [out] */ IHostAutoEvent **ppEvent);
        
        HRESULT ( STDMETHODCALLTYPE *CreateManualEvent )( 
            IHostSyncManager * This,
            /* [in] */ BOOL bInitialState,
            /* [out] */ IHostManualEvent **ppEvent);
        
        HRESULT ( STDMETHODCALLTYPE *CreateMonitorEvent )( 
            IHostSyncManager * This,
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostAutoEvent **ppEvent);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRWLockWriterEvent )( 
            IHostSyncManager * This,
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostAutoEvent **ppEvent);
        
        HRESULT ( STDMETHODCALLTYPE *CreateRWLockReaderEvent )( 
            IHostSyncManager * This,
            /* [in] */ BOOL bInitialState,
            /* [in] */ SIZE_T Cookie,
            /* [out] */ IHostManualEvent **ppEvent);
        
        HRESULT ( STDMETHODCALLTYPE *CreateSemaphore )( 
            IHostSyncManager * This,
            /* [in] */ DWORD dwInitial,
            /* [in] */ DWORD dwMax,
            /* [out] */ IHostSemaphore **ppSemaphore);
        
        END_INTERFACE
    } IHostSyncManagerVtbl;

    interface IHostSyncManager
    {
        CONST_VTBL struct IHostSyncManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostSyncManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostSyncManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostSyncManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostSyncManager_SetCLRSyncManager(This,pManager)	\
    ( (This)->lpVtbl -> SetCLRSyncManager(This,pManager) ) 

#define IHostSyncManager_CreateCrst(This,ppCrst)	\
    ( (This)->lpVtbl -> CreateCrst(This,ppCrst) ) 

#define IHostSyncManager_CreateCrstWithSpinCount(This,dwSpinCount,ppCrst)	\
    ( (This)->lpVtbl -> CreateCrstWithSpinCount(This,dwSpinCount,ppCrst) ) 

#define IHostSyncManager_CreateAutoEvent(This,ppEvent)	\
    ( (This)->lpVtbl -> CreateAutoEvent(This,ppEvent) ) 

#define IHostSyncManager_CreateManualEvent(This,bInitialState,ppEvent)	\
    ( (This)->lpVtbl -> CreateManualEvent(This,bInitialState,ppEvent) ) 

#define IHostSyncManager_CreateMonitorEvent(This,Cookie,ppEvent)	\
    ( (This)->lpVtbl -> CreateMonitorEvent(This,Cookie,ppEvent) ) 

#define IHostSyncManager_CreateRWLockWriterEvent(This,Cookie,ppEvent)	\
    ( (This)->lpVtbl -> CreateRWLockWriterEvent(This,Cookie,ppEvent) ) 

#define IHostSyncManager_CreateRWLockReaderEvent(This,bInitialState,Cookie,ppEvent)	\
    ( (This)->lpVtbl -> CreateRWLockReaderEvent(This,bInitialState,Cookie,ppEvent) ) 

#define IHostSyncManager_CreateSemaphore(This,dwInitial,dwMax,ppSemaphore)	\
    ( (This)->lpVtbl -> CreateSemaphore(This,dwInitial,dwMax,ppSemaphore) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostSyncManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0028 */
/* [local] */ 

typedef /* [public][public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0028_0001
    {
        OPR_ThreadAbort	= 0,
        OPR_ThreadRudeAbortInNonCriticalRegion	= ( OPR_ThreadAbort + 1 ) ,
        OPR_ThreadRudeAbortInCriticalRegion	= ( OPR_ThreadRudeAbortInNonCriticalRegion + 1 ) ,
        OPR_AppDomainUnload	= ( OPR_ThreadRudeAbortInCriticalRegion + 1 ) ,
        OPR_AppDomainRudeUnload	= ( OPR_AppDomainUnload + 1 ) ,
        OPR_ProcessExit	= ( OPR_AppDomainRudeUnload + 1 ) ,
        OPR_FinalizerRun	= ( OPR_ProcessExit + 1 ) ,
        MaxClrOperation	= ( OPR_FinalizerRun + 1 ) 
    } 	EClrOperation;

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0028_0002
    {
        FAIL_NonCriticalResource	= 0,
        FAIL_CriticalResource	= ( FAIL_NonCriticalResource + 1 ) ,
        FAIL_FatalRuntime	= ( FAIL_CriticalResource + 1 ) ,
        FAIL_OrphanedLock	= ( FAIL_FatalRuntime + 1 ) ,
        FAIL_StackOverflow	= ( FAIL_OrphanedLock + 1 ) ,
        FAIL_AccessViolation	= ( FAIL_StackOverflow + 1 ) ,
        FAIL_CodeContract	= ( FAIL_AccessViolation + 1 ) ,
        MaxClrFailure	= ( FAIL_CodeContract + 1 ) 
    } 	EClrFailure;

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0028_0003
    {
        eRuntimeDeterminedPolicy	= 0,
        eHostDeterminedPolicy	= ( eRuntimeDeterminedPolicy + 1 ) 
    } 	EClrUnhandledException;

typedef /* [public][public][public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0028_0004
    {
        eNoAction	= 0,
        eThrowException	= ( eNoAction + 1 ) ,
        eAbortThread	= ( eThrowException + 1 ) ,
        eRudeAbortThread	= ( eAbortThread + 1 ) ,
        eUnloadAppDomain	= ( eRudeAbortThread + 1 ) ,
        eRudeUnloadAppDomain	= ( eUnloadAppDomain + 1 ) ,
        eExitProcess	= ( eRudeUnloadAppDomain + 1 ) ,
        eFastExitProcess	= ( eExitProcess + 1 ) ,
        eRudeExitProcess	= ( eFastExitProcess + 1 ) ,
        eDisableRuntime	= ( eRudeExitProcess + 1 ) ,
        MaxPolicyAction	= ( eDisableRuntime + 1 ) 
    } 	EPolicyAction;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0028_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0028_v0_0_s_ifspec;

#ifndef __ICLRPolicyManager_INTERFACE_DEFINED__
#define __ICLRPolicyManager_INTERFACE_DEFINED__

/* interface ICLRPolicyManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRPolicyManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7D290010-D781-45da-A6F8-AA5D711A730E")
    ICLRPolicyManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDefaultAction( 
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimeout( 
            /* [in] */ EClrOperation operation,
            /* [in] */ DWORD dwMilliseconds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActionOnTimeout( 
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimeoutAndAction( 
            /* [in] */ EClrOperation operation,
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ EPolicyAction action) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActionOnFailure( 
            /* [in] */ EClrFailure failure,
            /* [in] */ EPolicyAction action) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUnhandledExceptionPolicy( 
            /* [in] */ EClrUnhandledException policy) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRPolicyManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRPolicyManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRPolicyManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRPolicyManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultAction )( 
            ICLRPolicyManager * This,
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action);
        
        HRESULT ( STDMETHODCALLTYPE *SetTimeout )( 
            ICLRPolicyManager * This,
            /* [in] */ EClrOperation operation,
            /* [in] */ DWORD dwMilliseconds);
        
        HRESULT ( STDMETHODCALLTYPE *SetActionOnTimeout )( 
            ICLRPolicyManager * This,
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action);
        
        HRESULT ( STDMETHODCALLTYPE *SetTimeoutAndAction )( 
            ICLRPolicyManager * This,
            /* [in] */ EClrOperation operation,
            /* [in] */ DWORD dwMilliseconds,
            /* [in] */ EPolicyAction action);
        
        HRESULT ( STDMETHODCALLTYPE *SetActionOnFailure )( 
            ICLRPolicyManager * This,
            /* [in] */ EClrFailure failure,
            /* [in] */ EPolicyAction action);
        
        HRESULT ( STDMETHODCALLTYPE *SetUnhandledExceptionPolicy )( 
            ICLRPolicyManager * This,
            /* [in] */ EClrUnhandledException policy);
        
        END_INTERFACE
    } ICLRPolicyManagerVtbl;

    interface ICLRPolicyManager
    {
        CONST_VTBL struct ICLRPolicyManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRPolicyManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRPolicyManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRPolicyManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRPolicyManager_SetDefaultAction(This,operation,action)	\
    ( (This)->lpVtbl -> SetDefaultAction(This,operation,action) ) 

#define ICLRPolicyManager_SetTimeout(This,operation,dwMilliseconds)	\
    ( (This)->lpVtbl -> SetTimeout(This,operation,dwMilliseconds) ) 

#define ICLRPolicyManager_SetActionOnTimeout(This,operation,action)	\
    ( (This)->lpVtbl -> SetActionOnTimeout(This,operation,action) ) 

#define ICLRPolicyManager_SetTimeoutAndAction(This,operation,dwMilliseconds,action)	\
    ( (This)->lpVtbl -> SetTimeoutAndAction(This,operation,dwMilliseconds,action) ) 

#define ICLRPolicyManager_SetActionOnFailure(This,failure,action)	\
    ( (This)->lpVtbl -> SetActionOnFailure(This,failure,action) ) 

#define ICLRPolicyManager_SetUnhandledExceptionPolicy(This,policy)	\
    ( (This)->lpVtbl -> SetUnhandledExceptionPolicy(This,policy) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRPolicyManager_INTERFACE_DEFINED__ */


#ifndef __IHostPolicyManager_INTERFACE_DEFINED__
#define __IHostPolicyManager_INTERFACE_DEFINED__

/* interface IHostPolicyManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostPolicyManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7AE49844-B1E3-4683-BA7C-1E8212EA3B79")
    IHostPolicyManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnDefaultAction( 
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnTimeout( 
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnFailure( 
            /* [in] */ EClrFailure failure,
            /* [in] */ EPolicyAction action) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostPolicyManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostPolicyManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostPolicyManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostPolicyManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnDefaultAction )( 
            IHostPolicyManager * This,
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimeout )( 
            IHostPolicyManager * This,
            /* [in] */ EClrOperation operation,
            /* [in] */ EPolicyAction action);
        
        HRESULT ( STDMETHODCALLTYPE *OnFailure )( 
            IHostPolicyManager * This,
            /* [in] */ EClrFailure failure,
            /* [in] */ EPolicyAction action);
        
        END_INTERFACE
    } IHostPolicyManagerVtbl;

    interface IHostPolicyManager
    {
        CONST_VTBL struct IHostPolicyManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostPolicyManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostPolicyManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostPolicyManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostPolicyManager_OnDefaultAction(This,operation,action)	\
    ( (This)->lpVtbl -> OnDefaultAction(This,operation,action) ) 

#define IHostPolicyManager_OnTimeout(This,operation,action)	\
    ( (This)->lpVtbl -> OnTimeout(This,operation,action) ) 

#define IHostPolicyManager_OnFailure(This,failure,action)	\
    ( (This)->lpVtbl -> OnFailure(This,failure,action) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostPolicyManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0030 */
/* [local] */ 

typedef /* [public][public][public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0030_0001
    {
        Event_DomainUnload	= 0,
        Event_ClrDisabled	= ( Event_DomainUnload + 1 ) ,
        Event_MDAFired	= ( Event_ClrDisabled + 1 ) ,
        Event_StackOverflow	= ( Event_MDAFired + 1 ) ,
        MaxClrEvent	= ( Event_StackOverflow + 1 ) 
    } 	EClrEvent;

typedef struct _MDAInfo
    {
    LPCWSTR lpMDACaption;
    LPCWSTR lpMDAMessage;
    LPCWSTR lpStackTrace;
    } 	MDAInfo;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0030_0002
    {
        SO_Managed	= 0,
        SO_ClrEngine	= ( SO_Managed + 1 ) ,
        SO_Other	= ( SO_ClrEngine + 1 ) 
    } 	StackOverflowType;

typedef struct _StackOverflowInfo
{
    StackOverflowType soType;
    EXCEPTION_POINTERS *pExceptionInfo;
} StackOverflowInfo;


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0030_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0030_v0_0_s_ifspec;

#ifndef __IActionOnCLREvent_INTERFACE_DEFINED__
#define __IActionOnCLREvent_INTERFACE_DEFINED__

/* interface IActionOnCLREvent */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IActionOnCLREvent;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("607BE24B-D91B-4E28-A242-61871CE56E35")
    IActionOnCLREvent : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnEvent( 
            /* [in] */ EClrEvent event,
            /* [in] */ PVOID data) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IActionOnCLREventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IActionOnCLREvent * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IActionOnCLREvent * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IActionOnCLREvent * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnEvent )( 
            IActionOnCLREvent * This,
            /* [in] */ EClrEvent event,
            /* [in] */ PVOID data);
        
        END_INTERFACE
    } IActionOnCLREventVtbl;

    interface IActionOnCLREvent
    {
        CONST_VTBL struct IActionOnCLREventVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActionOnCLREvent_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IActionOnCLREvent_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IActionOnCLREvent_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IActionOnCLREvent_OnEvent(This,event,data)	\
    ( (This)->lpVtbl -> OnEvent(This,event,data) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IActionOnCLREvent_INTERFACE_DEFINED__ */


#ifndef __ICLROnEventManager_INTERFACE_DEFINED__
#define __ICLROnEventManager_INTERFACE_DEFINED__

/* interface ICLROnEventManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLROnEventManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D0E0132-E64F-493D-9260-025C0E32C175")
    ICLROnEventManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RegisterActionOnEvent( 
            /* [in] */ EClrEvent event,
            /* [in] */ IActionOnCLREvent *pAction) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterActionOnEvent( 
            /* [in] */ EClrEvent event,
            /* [in] */ IActionOnCLREvent *pAction) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLROnEventManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLROnEventManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLROnEventManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLROnEventManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *RegisterActionOnEvent )( 
            ICLROnEventManager * This,
            /* [in] */ EClrEvent event,
            /* [in] */ IActionOnCLREvent *pAction);
        
        HRESULT ( STDMETHODCALLTYPE *UnregisterActionOnEvent )( 
            ICLROnEventManager * This,
            /* [in] */ EClrEvent event,
            /* [in] */ IActionOnCLREvent *pAction);
        
        END_INTERFACE
    } ICLROnEventManagerVtbl;

    interface ICLROnEventManager
    {
        CONST_VTBL struct ICLROnEventManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLROnEventManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLROnEventManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLROnEventManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLROnEventManager_RegisterActionOnEvent(This,event,pAction)	\
    ( (This)->lpVtbl -> RegisterActionOnEvent(This,event,pAction) ) 

#define ICLROnEventManager_UnregisterActionOnEvent(This,event,pAction)	\
    ( (This)->lpVtbl -> UnregisterActionOnEvent(This,event,pAction) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLROnEventManager_INTERFACE_DEFINED__ */


#ifndef __IHostGCManager_INTERFACE_DEFINED__
#define __IHostGCManager_INTERFACE_DEFINED__

/* interface IHostGCManager */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostGCManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5D4EC34E-F248-457B-B603-255FAABA0D21")
    IHostGCManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ThreadIsBlockingForSuspension( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspensionStarting( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SuspensionEnding( 
            DWORD Generation) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostGCManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostGCManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostGCManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostGCManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *ThreadIsBlockingForSuspension )( 
            IHostGCManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuspensionStarting )( 
            IHostGCManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SuspensionEnding )( 
            IHostGCManager * This,
            DWORD Generation);
        
        END_INTERFACE
    } IHostGCManagerVtbl;

    interface IHostGCManager
    {
        CONST_VTBL struct IHostGCManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostGCManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostGCManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostGCManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostGCManager_ThreadIsBlockingForSuspension(This)	\
    ( (This)->lpVtbl -> ThreadIsBlockingForSuspension(This) ) 

#define IHostGCManager_SuspensionStarting(This)	\
    ( (This)->lpVtbl -> SuspensionStarting(This) ) 

#define IHostGCManager_SuspensionEnding(This,Generation)	\
    ( (This)->lpVtbl -> SuspensionEnding(This,Generation) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostGCManager_INTERFACE_DEFINED__ */


#ifndef __ICLRAssemblyReferenceList_INTERFACE_DEFINED__
#define __ICLRAssemblyReferenceList_INTERFACE_DEFINED__

/* interface ICLRAssemblyReferenceList */
/* [local][unique][helpstring][uuid][version][object] */ 


EXTERN_C const IID IID_ICLRAssemblyReferenceList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1b2c9750-2e66-4bda-8b44-0a642c5cd733")
    ICLRAssemblyReferenceList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsStringAssemblyReferenceInList( 
            /* [in] */ LPCWSTR pwzAssemblyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsAssemblyReferenceInList( 
            /* [in] */ IUnknown *pName) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRAssemblyReferenceListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRAssemblyReferenceList * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRAssemblyReferenceList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRAssemblyReferenceList * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsStringAssemblyReferenceInList )( 
            ICLRAssemblyReferenceList * This,
            /* [in] */ LPCWSTR pwzAssemblyName);
        
        HRESULT ( STDMETHODCALLTYPE *IsAssemblyReferenceInList )( 
            ICLRAssemblyReferenceList * This,
            /* [in] */ IUnknown *pName);
        
        END_INTERFACE
    } ICLRAssemblyReferenceListVtbl;

    interface ICLRAssemblyReferenceList
    {
        CONST_VTBL struct ICLRAssemblyReferenceListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRAssemblyReferenceList_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRAssemblyReferenceList_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRAssemblyReferenceList_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRAssemblyReferenceList_IsStringAssemblyReferenceInList(This,pwzAssemblyName)	\
    ( (This)->lpVtbl -> IsStringAssemblyReferenceInList(This,pwzAssemblyName) ) 

#define ICLRAssemblyReferenceList_IsAssemblyReferenceInList(This,pName)	\
    ( (This)->lpVtbl -> IsAssemblyReferenceInList(This,pName) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRAssemblyReferenceList_INTERFACE_DEFINED__ */


#ifndef __ICLRReferenceAssemblyEnum_INTERFACE_DEFINED__
#define __ICLRReferenceAssemblyEnum_INTERFACE_DEFINED__

/* interface ICLRReferenceAssemblyEnum */
/* [local][unique][helpstring][uuid][version][object] */ 


EXTERN_C const IID IID_ICLRReferenceAssemblyEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d509cb5d-cf32-4876-ae61-67770cf91973")
    ICLRReferenceAssemblyEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ DWORD dwIndex,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRReferenceAssemblyEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRReferenceAssemblyEnum * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRReferenceAssemblyEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRReferenceAssemblyEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            ICLRReferenceAssemblyEnum * This,
            /* [in] */ DWORD dwIndex,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize);
        
        END_INTERFACE
    } ICLRReferenceAssemblyEnumVtbl;

    interface ICLRReferenceAssemblyEnum
    {
        CONST_VTBL struct ICLRReferenceAssemblyEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRReferenceAssemblyEnum_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRReferenceAssemblyEnum_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRReferenceAssemblyEnum_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRReferenceAssemblyEnum_Get(This,dwIndex,pwzBuffer,pcchBufferSize)	\
    ( (This)->lpVtbl -> Get(This,dwIndex,pwzBuffer,pcchBufferSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRReferenceAssemblyEnum_INTERFACE_DEFINED__ */


#ifndef __ICLRProbingAssemblyEnum_INTERFACE_DEFINED__
#define __ICLRProbingAssemblyEnum_INTERFACE_DEFINED__

/* interface ICLRProbingAssemblyEnum */
/* [local][unique][helpstring][uuid][version][object] */ 


EXTERN_C const IID IID_ICLRProbingAssemblyEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d0c5fb1f-416b-4f97-81f4-7ac7dc24dd5d")
    ICLRProbingAssemblyEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ DWORD dwIndex,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRProbingAssemblyEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRProbingAssemblyEnum * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRProbingAssemblyEnum * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRProbingAssemblyEnum * This);
        
        HRESULT ( STDMETHODCALLTYPE *Get )( 
            ICLRProbingAssemblyEnum * This,
            /* [in] */ DWORD dwIndex,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize);
        
        END_INTERFACE
    } ICLRProbingAssemblyEnumVtbl;

    interface ICLRProbingAssemblyEnum
    {
        CONST_VTBL struct ICLRProbingAssemblyEnumVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRProbingAssemblyEnum_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRProbingAssemblyEnum_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRProbingAssemblyEnum_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRProbingAssemblyEnum_Get(This,dwIndex,pwzBuffer,pcchBufferSize)	\
    ( (This)->lpVtbl -> Get(This,dwIndex,pwzBuffer,pcchBufferSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRProbingAssemblyEnum_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0036 */
/* [local] */ 

typedef 
enum _CLRAssemblyIdentityFlags
    {
        CLR_ASSEMBLY_IDENTITY_FLAGS_DEFAULT	= 0
    } 	ECLRAssemblyIdentityFlags;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0036_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0036_v0_0_s_ifspec;

#ifndef __ICLRAssemblyIdentityManager_INTERFACE_DEFINED__
#define __ICLRAssemblyIdentityManager_INTERFACE_DEFINED__

/* interface ICLRAssemblyIdentityManager */
/* [local][unique][helpstring][uuid][version][object] */ 


EXTERN_C const IID IID_ICLRAssemblyIdentityManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("15f0a9da-3ff6-4393-9da9-fdfd284e6972")
    ICLRAssemblyIdentityManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCLRAssemblyReferenceList( 
            /* [in] */ LPCWSTR *ppwzAssemblyReferences,
            /* [in] */ DWORD dwNumOfReferences,
            /* [out] */ ICLRAssemblyReferenceList **ppReferenceList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBindingIdentityFromFile( 
            /* [in] */ LPCWSTR pwzFilePath,
            /* [in] */ DWORD dwFlags,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBindingIdentityFromStream( 
            /* [in] */ IStream *pStream,
            /* [in] */ DWORD dwFlags,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferencedAssembliesFromFile( 
            /* [in] */ LPCWSTR pwzFilePath,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ICLRAssemblyReferenceList *pExcludeAssembliesList,
            /* [out] */ ICLRReferenceAssemblyEnum **ppReferenceEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetReferencedAssembliesFromStream( 
            /* [in] */ IStream *pStream,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ICLRAssemblyReferenceList *pExcludeAssembliesList,
            /* [out] */ ICLRReferenceAssemblyEnum **ppReferenceEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProbingAssembliesFromReference( 
            /* [in] */ DWORD dwMachineType,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pwzReferenceIdentity,
            /* [out] */ ICLRProbingAssemblyEnum **ppProbingAssemblyEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsStronglyNamed( 
            /* [in] */ LPCWSTR pwzAssemblyIdentity,
            /* [out] */ BOOL *pbIsStronglyNamed) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRAssemblyIdentityManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRAssemblyIdentityManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRAssemblyIdentityManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCLRAssemblyReferenceList )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ LPCWSTR *ppwzAssemblyReferences,
            /* [in] */ DWORD dwNumOfReferences,
            /* [out] */ ICLRAssemblyReferenceList **ppReferenceList);
        
        HRESULT ( STDMETHODCALLTYPE *GetBindingIdentityFromFile )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ LPCWSTR pwzFilePath,
            /* [in] */ DWORD dwFlags,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetBindingIdentityFromStream )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ IStream *pStream,
            /* [in] */ DWORD dwFlags,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchBufferSize)  LPWSTR pwzBuffer,
            /* [out][in] */ DWORD *pcchBufferSize);
        
        HRESULT ( STDMETHODCALLTYPE *GetReferencedAssembliesFromFile )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ LPCWSTR pwzFilePath,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ICLRAssemblyReferenceList *pExcludeAssembliesList,
            /* [out] */ ICLRReferenceAssemblyEnum **ppReferenceEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetReferencedAssembliesFromStream )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ IStream *pStream,
            /* [in] */ DWORD dwFlags,
            /* [in] */ ICLRAssemblyReferenceList *pExcludeAssembliesList,
            /* [out] */ ICLRReferenceAssemblyEnum **ppReferenceEnum);
        
        HRESULT ( STDMETHODCALLTYPE *GetProbingAssembliesFromReference )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ DWORD dwMachineType,
            /* [in] */ DWORD dwFlags,
            /* [in] */ LPCWSTR pwzReferenceIdentity,
            /* [out] */ ICLRProbingAssemblyEnum **ppProbingAssemblyEnum);
        
        HRESULT ( STDMETHODCALLTYPE *IsStronglyNamed )( 
            ICLRAssemblyIdentityManager * This,
            /* [in] */ LPCWSTR pwzAssemblyIdentity,
            /* [out] */ BOOL *pbIsStronglyNamed);
        
        END_INTERFACE
    } ICLRAssemblyIdentityManagerVtbl;

    interface ICLRAssemblyIdentityManager
    {
        CONST_VTBL struct ICLRAssemblyIdentityManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRAssemblyIdentityManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRAssemblyIdentityManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRAssemblyIdentityManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRAssemblyIdentityManager_GetCLRAssemblyReferenceList(This,ppwzAssemblyReferences,dwNumOfReferences,ppReferenceList)	\
    ( (This)->lpVtbl -> GetCLRAssemblyReferenceList(This,ppwzAssemblyReferences,dwNumOfReferences,ppReferenceList) ) 

#define ICLRAssemblyIdentityManager_GetBindingIdentityFromFile(This,pwzFilePath,dwFlags,pwzBuffer,pcchBufferSize)	\
    ( (This)->lpVtbl -> GetBindingIdentityFromFile(This,pwzFilePath,dwFlags,pwzBuffer,pcchBufferSize) ) 

#define ICLRAssemblyIdentityManager_GetBindingIdentityFromStream(This,pStream,dwFlags,pwzBuffer,pcchBufferSize)	\
    ( (This)->lpVtbl -> GetBindingIdentityFromStream(This,pStream,dwFlags,pwzBuffer,pcchBufferSize) ) 

#define ICLRAssemblyIdentityManager_GetReferencedAssembliesFromFile(This,pwzFilePath,dwFlags,pExcludeAssembliesList,ppReferenceEnum)	\
    ( (This)->lpVtbl -> GetReferencedAssembliesFromFile(This,pwzFilePath,dwFlags,pExcludeAssembliesList,ppReferenceEnum) ) 

#define ICLRAssemblyIdentityManager_GetReferencedAssembliesFromStream(This,pStream,dwFlags,pExcludeAssembliesList,ppReferenceEnum)	\
    ( (This)->lpVtbl -> GetReferencedAssembliesFromStream(This,pStream,dwFlags,pExcludeAssembliesList,ppReferenceEnum) ) 

#define ICLRAssemblyIdentityManager_GetProbingAssembliesFromReference(This,dwMachineType,dwFlags,pwzReferenceIdentity,ppProbingAssemblyEnum)	\
    ( (This)->lpVtbl -> GetProbingAssembliesFromReference(This,dwMachineType,dwFlags,pwzReferenceIdentity,ppProbingAssemblyEnum) ) 

#define ICLRAssemblyIdentityManager_IsStronglyNamed(This,pwzAssemblyIdentity,pbIsStronglyNamed)	\
    ( (This)->lpVtbl -> IsStronglyNamed(This,pwzAssemblyIdentity,pbIsStronglyNamed) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRAssemblyIdentityManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0037 */
/* [local] */ 

typedef 
enum _hostBiningPolicyModifyFlags
    {
        HOST_BINDING_POLICY_MODIFY_DEFAULT	= 0,
        HOST_BINDING_POLICY_MODIFY_CHAIN	= 1,
        HOST_BINDING_POLICY_MODIFY_REMOVE	= 2,
        HOST_BINDING_POLICY_MODIFY_MAX	= 3
    } 	EHostBindingPolicyModifyFlags;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0037_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0037_v0_0_s_ifspec;

#ifndef __ICLRHostBindingPolicyManager_INTERFACE_DEFINED__
#define __ICLRHostBindingPolicyManager_INTERFACE_DEFINED__

/* interface ICLRHostBindingPolicyManager */
/* [local][unique][helpstring][uuid][version][object] */ 


EXTERN_C const IID IID_ICLRHostBindingPolicyManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4b3545e7-1856-48c9-a8ba-24b21a753c09")
    ICLRHostBindingPolicyManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ModifyApplicationPolicy( 
            /* [in] */ LPCWSTR pwzSourceAssemblyIdentity,
            /* [in] */ LPCWSTR pwzTargetAssemblyIdentity,
            /* [in] */ BYTE *pbApplicationPolicy,
            /* [in] */ DWORD cbAppPolicySize,
            /* [in] */ DWORD dwPolicyModifyFlags,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcbNewAppPolicySize)  BYTE *pbNewApplicationPolicy,
            /* [out][in] */ DWORD *pcbNewAppPolicySize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EvaluatePolicy( 
            /* [in] */ LPCWSTR pwzReferenceIdentity,
            /* [in] */ BYTE *pbApplicationPolicy,
            /* [in] */ DWORD cbAppPolicySize,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchPostPolicyReferenceIdentity)  LPWSTR pwzPostPolicyReferenceIdentity,
            /* [out][in] */ DWORD *pcchPostPolicyReferenceIdentity,
            /* [out] */ DWORD *pdwPoliciesApplied) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRHostBindingPolicyManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRHostBindingPolicyManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRHostBindingPolicyManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRHostBindingPolicyManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *ModifyApplicationPolicy )( 
            ICLRHostBindingPolicyManager * This,
            /* [in] */ LPCWSTR pwzSourceAssemblyIdentity,
            /* [in] */ LPCWSTR pwzTargetAssemblyIdentity,
            /* [in] */ BYTE *pbApplicationPolicy,
            /* [in] */ DWORD cbAppPolicySize,
            /* [in] */ DWORD dwPolicyModifyFlags,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcbNewAppPolicySize)  BYTE *pbNewApplicationPolicy,
            /* [out][in] */ DWORD *pcbNewAppPolicySize);
        
        HRESULT ( STDMETHODCALLTYPE *EvaluatePolicy )( 
            ICLRHostBindingPolicyManager * This,
            /* [in] */ LPCWSTR pwzReferenceIdentity,
            /* [in] */ BYTE *pbApplicationPolicy,
            /* [in] */ DWORD cbAppPolicySize,
            /* [annotation][size_is][out] */ 
            _Out_writes_all_(*pcchPostPolicyReferenceIdentity)  LPWSTR pwzPostPolicyReferenceIdentity,
            /* [out][in] */ DWORD *pcchPostPolicyReferenceIdentity,
            /* [out] */ DWORD *pdwPoliciesApplied);
        
        END_INTERFACE
    } ICLRHostBindingPolicyManagerVtbl;

    interface ICLRHostBindingPolicyManager
    {
        CONST_VTBL struct ICLRHostBindingPolicyManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRHostBindingPolicyManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRHostBindingPolicyManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRHostBindingPolicyManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRHostBindingPolicyManager_ModifyApplicationPolicy(This,pwzSourceAssemblyIdentity,pwzTargetAssemblyIdentity,pbApplicationPolicy,cbAppPolicySize,dwPolicyModifyFlags,pbNewApplicationPolicy,pcbNewAppPolicySize)	\
    ( (This)->lpVtbl -> ModifyApplicationPolicy(This,pwzSourceAssemblyIdentity,pwzTargetAssemblyIdentity,pbApplicationPolicy,cbAppPolicySize,dwPolicyModifyFlags,pbNewApplicationPolicy,pcbNewAppPolicySize) ) 

#define ICLRHostBindingPolicyManager_EvaluatePolicy(This,pwzReferenceIdentity,pbApplicationPolicy,cbAppPolicySize,pwzPostPolicyReferenceIdentity,pcchPostPolicyReferenceIdentity,pdwPoliciesApplied)	\
    ( (This)->lpVtbl -> EvaluatePolicy(This,pwzReferenceIdentity,pbApplicationPolicy,cbAppPolicySize,pwzPostPolicyReferenceIdentity,pcchPostPolicyReferenceIdentity,pdwPoliciesApplied) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRHostBindingPolicyManager_INTERFACE_DEFINED__ */


#ifndef __ICLRGCManager_INTERFACE_DEFINED__
#define __ICLRGCManager_INTERFACE_DEFINED__

/* interface ICLRGCManager */
/* [object][local][unique][version][uuid] */ 


EXTERN_C const IID IID_ICLRGCManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("54D9007E-A8E2-4885-B7BF-F998DEEE4F2A")
    ICLRGCManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Collect( 
            /* [in] */ LONG Generation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStats( 
            /* [out][in] */ COR_GC_STATS *pStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGCStartupLimits( 
            /* [in] */ DWORD SegmentSize,
            /* [in] */ DWORD MaxGen0Size) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRGCManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRGCManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRGCManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRGCManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *Collect )( 
            ICLRGCManager * This,
            /* [in] */ LONG Generation);
        
        HRESULT ( STDMETHODCALLTYPE *GetStats )( 
            ICLRGCManager * This,
            /* [out][in] */ COR_GC_STATS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCStartupLimits )( 
            ICLRGCManager * This,
            /* [in] */ DWORD SegmentSize,
            /* [in] */ DWORD MaxGen0Size);
        
        END_INTERFACE
    } ICLRGCManagerVtbl;

    interface ICLRGCManager
    {
        CONST_VTBL struct ICLRGCManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRGCManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRGCManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRGCManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRGCManager_Collect(This,Generation)	\
    ( (This)->lpVtbl -> Collect(This,Generation) ) 

#define ICLRGCManager_GetStats(This,pStats)	\
    ( (This)->lpVtbl -> GetStats(This,pStats) ) 

#define ICLRGCManager_SetGCStartupLimits(This,SegmentSize,MaxGen0Size)	\
    ( (This)->lpVtbl -> SetGCStartupLimits(This,SegmentSize,MaxGen0Size) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRGCManager_INTERFACE_DEFINED__ */


#ifndef __ICLRGCManager2_INTERFACE_DEFINED__
#define __ICLRGCManager2_INTERFACE_DEFINED__

/* interface ICLRGCManager2 */
/* [object][local][unique][version][uuid] */ 


EXTERN_C const IID IID_ICLRGCManager2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0603B793-A97A-4712-9CB4-0CD1C74C0F7C")
    ICLRGCManager2 : public ICLRGCManager
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGCStartupLimitsEx( 
            /* [in] */ SIZE_T SegmentSize,
            /* [in] */ SIZE_T MaxGen0Size) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRGCManager2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRGCManager2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRGCManager2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRGCManager2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Collect )( 
            ICLRGCManager2 * This,
            /* [in] */ LONG Generation);
        
        HRESULT ( STDMETHODCALLTYPE *GetStats )( 
            ICLRGCManager2 * This,
            /* [out][in] */ COR_GC_STATS *pStats);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCStartupLimits )( 
            ICLRGCManager2 * This,
            /* [in] */ DWORD SegmentSize,
            /* [in] */ DWORD MaxGen0Size);
        
        HRESULT ( STDMETHODCALLTYPE *SetGCStartupLimitsEx )( 
            ICLRGCManager2 * This,
            /* [in] */ SIZE_T SegmentSize,
            /* [in] */ SIZE_T MaxGen0Size);
        
        END_INTERFACE
    } ICLRGCManager2Vtbl;

    interface ICLRGCManager2
    {
        CONST_VTBL struct ICLRGCManager2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRGCManager2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRGCManager2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRGCManager2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRGCManager2_Collect(This,Generation)	\
    ( (This)->lpVtbl -> Collect(This,Generation) ) 

#define ICLRGCManager2_GetStats(This,pStats)	\
    ( (This)->lpVtbl -> GetStats(This,pStats) ) 

#define ICLRGCManager2_SetGCStartupLimits(This,SegmentSize,MaxGen0Size)	\
    ( (This)->lpVtbl -> SetGCStartupLimits(This,SegmentSize,MaxGen0Size) ) 


#define ICLRGCManager2_SetGCStartupLimitsEx(This,SegmentSize,MaxGen0Size)	\
    ( (This)->lpVtbl -> SetGCStartupLimitsEx(This,SegmentSize,MaxGen0Size) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRGCManager2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0040 */
/* [local] */ 

typedef /* [public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0040_0001
    {
        ePolicyLevelNone	= 0,
        ePolicyLevelRetargetable	= 0x1,
        ePolicyUnifiedToCLR	= 0x2,
        ePolicyLevelApp	= 0x4,
        ePolicyLevelPublisher	= 0x8,
        ePolicyLevelHost	= 0x10,
        ePolicyLevelAdmin	= 0x20,
        ePolicyPortability	= 0x40
    } 	EBindPolicyLevels;

typedef struct _AssemblyBindInfo
    {
    DWORD dwAppDomainId;
    LPCWSTR lpReferencedIdentity;
    LPCWSTR lpPostPolicyIdentity;
    DWORD ePolicyLevel;
    } 	AssemblyBindInfo;

typedef struct _ModuleBindInfo
    {
    DWORD dwAppDomainId;
    LPCWSTR lpAssemblyIdentity;
    LPCWSTR lpModuleName;
    } 	ModuleBindInfo;

typedef 
enum _HostApplicationPolicy
    {
        HOST_APPLICATION_BINDING_POLICY	= 1
    } 	EHostApplicationPolicy;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0040_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0040_v0_0_s_ifspec;

#ifndef __IHostAssemblyStore_INTERFACE_DEFINED__
#define __IHostAssemblyStore_INTERFACE_DEFINED__

/* interface IHostAssemblyStore */
/* [unique][helpstring][uuid][version][object][local] */ 


EXTERN_C const IID IID_IHostAssemblyStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7b102a88-3f7f-496d-8fa2-c35374e01af3")
    IHostAssemblyStore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ProvideAssembly( 
            /* [in] */ AssemblyBindInfo *pBindInfo,
            /* [out] */ UINT64 *pAssemblyId,
            /* [out] */ UINT64 *pContext,
            /* [out] */ IStream **ppStmAssemblyImage,
            /* [out] */ IStream **ppStmPDB) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ProvideModule( 
            /* [in] */ ModuleBindInfo *pBindInfo,
            /* [out] */ DWORD *pdwModuleId,
            /* [out] */ IStream **ppStmModuleImage,
            /* [out] */ IStream **ppStmPDB) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostAssemblyStoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostAssemblyStore * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostAssemblyStore * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostAssemblyStore * This);
        
        HRESULT ( STDMETHODCALLTYPE *ProvideAssembly )( 
            IHostAssemblyStore * This,
            /* [in] */ AssemblyBindInfo *pBindInfo,
            /* [out] */ UINT64 *pAssemblyId,
            /* [out] */ UINT64 *pContext,
            /* [out] */ IStream **ppStmAssemblyImage,
            /* [out] */ IStream **ppStmPDB);
        
        HRESULT ( STDMETHODCALLTYPE *ProvideModule )( 
            IHostAssemblyStore * This,
            /* [in] */ ModuleBindInfo *pBindInfo,
            /* [out] */ DWORD *pdwModuleId,
            /* [out] */ IStream **ppStmModuleImage,
            /* [out] */ IStream **ppStmPDB);
        
        END_INTERFACE
    } IHostAssemblyStoreVtbl;

    interface IHostAssemblyStore
    {
        CONST_VTBL struct IHostAssemblyStoreVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostAssemblyStore_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostAssemblyStore_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostAssemblyStore_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostAssemblyStore_ProvideAssembly(This,pBindInfo,pAssemblyId,pContext,ppStmAssemblyImage,ppStmPDB)	\
    ( (This)->lpVtbl -> ProvideAssembly(This,pBindInfo,pAssemblyId,pContext,ppStmAssemblyImage,ppStmPDB) ) 

#define IHostAssemblyStore_ProvideModule(This,pBindInfo,pdwModuleId,ppStmModuleImage,ppStmPDB)	\
    ( (This)->lpVtbl -> ProvideModule(This,pBindInfo,pdwModuleId,ppStmModuleImage,ppStmPDB) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostAssemblyStore_INTERFACE_DEFINED__ */


#ifndef __IHostAssemblyManager_INTERFACE_DEFINED__
#define __IHostAssemblyManager_INTERFACE_DEFINED__

/* interface IHostAssemblyManager */
/* [unique][helpstring][uuid][version][object][local] */ 


EXTERN_C const IID IID_IHostAssemblyManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("613dabd7-62b2-493e-9e65-c1e32a1e0c5e")
    IHostAssemblyManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNonHostStoreAssemblies( 
            /* [out] */ ICLRAssemblyReferenceList **ppReferenceList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyStore( 
            /* [out] */ IHostAssemblyStore **ppAssemblyStore) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostAssemblyManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostAssemblyManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostAssemblyManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostAssemblyManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNonHostStoreAssemblies )( 
            IHostAssemblyManager * This,
            /* [out] */ ICLRAssemblyReferenceList **ppReferenceList);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyStore )( 
            IHostAssemblyManager * This,
            /* [out] */ IHostAssemblyStore **ppAssemblyStore);
        
        END_INTERFACE
    } IHostAssemblyManagerVtbl;

    interface IHostAssemblyManager
    {
        CONST_VTBL struct IHostAssemblyManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostAssemblyManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostAssemblyManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostAssemblyManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostAssemblyManager_GetNonHostStoreAssemblies(This,ppReferenceList)	\
    ( (This)->lpVtbl -> GetNonHostStoreAssemblies(This,ppReferenceList) ) 

#define IHostAssemblyManager_GetAssemblyStore(This,ppAssemblyStore)	\
    ( (This)->lpVtbl -> GetAssemblyStore(This,ppAssemblyStore) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostAssemblyManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0042 */
/* [local] */ 

STDAPI GetCLRIdentityManager(REFIID riid, IUnknown **ppManager);
EXTERN_GUID(IID_IHostControl, 0x02CA073C, 0x7079, 0x4860, 0x88, 0x0A, 0xC2, 0xF7, 0xA4, 0x49, 0xC9, 0x91);


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0042_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0042_v0_0_s_ifspec;

#ifndef __IHostControl_INTERFACE_DEFINED__
#define __IHostControl_INTERFACE_DEFINED__

/* interface IHostControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_IHostControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("02CA073C-7079-4860-880A-C2F7A449C991")
    IHostControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetHostManager( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAppDomainManager( 
            /* [in] */ DWORD dwAppDomainID,
            /* [in] */ IUnknown *pUnkAppDomainManager) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostManager )( 
            IHostControl * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppObject);
        
        HRESULT ( STDMETHODCALLTYPE *SetAppDomainManager )( 
            IHostControl * This,
            /* [in] */ DWORD dwAppDomainID,
            /* [in] */ IUnknown *pUnkAppDomainManager);
        
        END_INTERFACE
    } IHostControlVtbl;

    interface IHostControl
    {
        CONST_VTBL struct IHostControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostControl_GetHostManager(This,riid,ppObject)	\
    ( (This)->lpVtbl -> GetHostManager(This,riid,ppObject) ) 

#define IHostControl_SetAppDomainManager(This,dwAppDomainID,pUnkAppDomainManager)	\
    ( (This)->lpVtbl -> SetAppDomainManager(This,dwAppDomainID,pUnkAppDomainManager) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0043 */
/* [local] */ 

EXTERN_GUID(IID_ICLRControl, 0x9065597E, 0xD1A1, 0x4fb2, 0xB6, 0xBA, 0x7E, 0x1F, 0xCE, 0x23, 0x0F, 0x61);


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0043_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0043_v0_0_s_ifspec;

#ifndef __ICLRControl_INTERFACE_DEFINED__
#define __ICLRControl_INTERFACE_DEFINED__

/* interface ICLRControl */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9065597E-D1A1-4fb2-B6BA-7E1FCE230F61")
    ICLRControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCLRManager( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAppDomainManagerType( 
            /* [in] */ LPCWSTR pwzAppDomainManagerAssembly,
            /* [in] */ LPCWSTR pwzAppDomainManagerType) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCLRManager )( 
            ICLRControl * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppObject);
        
        HRESULT ( STDMETHODCALLTYPE *SetAppDomainManagerType )( 
            ICLRControl * This,
            /* [in] */ LPCWSTR pwzAppDomainManagerAssembly,
            /* [in] */ LPCWSTR pwzAppDomainManagerType);
        
        END_INTERFACE
    } ICLRControlVtbl;

    interface ICLRControl
    {
        CONST_VTBL struct ICLRControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRControl_GetCLRManager(This,riid,ppObject)	\
    ( (This)->lpVtbl -> GetCLRManager(This,riid,ppObject) ) 

#define ICLRControl_SetAppDomainManagerType(This,pwzAppDomainManagerAssembly,pwzAppDomainManagerType)	\
    ( (This)->lpVtbl -> SetAppDomainManagerType(This,pwzAppDomainManagerAssembly,pwzAppDomainManagerType) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRControl_INTERFACE_DEFINED__ */


#ifndef __ICLRRuntimeHost_INTERFACE_DEFINED__
#define __ICLRRuntimeHost_INTERFACE_DEFINED__

/* interface ICLRRuntimeHost */
/* [object][local][unique][helpstring][version][uuid] */ 


EXTERN_C const IID IID_ICLRRuntimeHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("90F1A06C-7712-4762-86B5-7A5EBA6BDB02")
    ICLRRuntimeHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetHostControl( 
            /* [in] */ IHostControl *pHostControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCLRControl( 
            /* [out] */ ICLRControl **pCLRControl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadAppDomain( 
            /* [in] */ DWORD dwAppDomainId,
            /* [in] */ BOOL fWaitUntilDone) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteInAppDomain( 
            /* [in] */ DWORD dwAppDomainId,
            /* [in] */ FExecuteInAppDomainCallback pCallback,
            /* [in] */ void *cookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentAppDomainId( 
            /* [out] */ DWORD *pdwAppDomainId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteApplication( 
            /* [in] */ LPCWSTR pwzAppFullName,
            /* [in] */ DWORD dwManifestPaths,
            /* [in] */ LPCWSTR *ppwzManifestPaths,
            /* [in] */ DWORD dwActivationData,
            /* [in] */ LPCWSTR *ppwzActivationData,
            /* [out] */ int *pReturnValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteInDefaultAppDomain( 
            /* [in] */ LPCWSTR pwzAssemblyPath,
            /* [in] */ LPCWSTR pwzTypeName,
            /* [in] */ LPCWSTR pwzMethodName,
            /* [in] */ LPCWSTR pwzArgument,
            /* [out] */ DWORD *pReturnValue) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRRuntimeHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRRuntimeHost * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRRuntimeHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            ICLRRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            ICLRRuntimeHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetHostControl )( 
            ICLRRuntimeHost * This,
            /* [in] */ IHostControl *pHostControl);
        
        HRESULT ( STDMETHODCALLTYPE *GetCLRControl )( 
            ICLRRuntimeHost * This,
            /* [out] */ ICLRControl **pCLRControl);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadAppDomain )( 
            ICLRRuntimeHost * This,
            /* [in] */ DWORD dwAppDomainId,
            /* [in] */ BOOL fWaitUntilDone);
        
        HRESULT ( STDMETHODCALLTYPE *ExecuteInAppDomain )( 
            ICLRRuntimeHost * This,
            /* [in] */ DWORD dwAppDomainId,
            /* [in] */ FExecuteInAppDomainCallback pCallback,
            /* [in] */ void *cookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentAppDomainId )( 
            ICLRRuntimeHost * This,
            /* [out] */ DWORD *pdwAppDomainId);
        
        HRESULT ( STDMETHODCALLTYPE *ExecuteApplication )( 
            ICLRRuntimeHost * This,
            /* [in] */ LPCWSTR pwzAppFullName,
            /* [in] */ DWORD dwManifestPaths,
            /* [in] */ LPCWSTR *ppwzManifestPaths,
            /* [in] */ DWORD dwActivationData,
            /* [in] */ LPCWSTR *ppwzActivationData,
            /* [out] */ int *pReturnValue);
        
        HRESULT ( STDMETHODCALLTYPE *ExecuteInDefaultAppDomain )( 
            ICLRRuntimeHost * This,
            /* [in] */ LPCWSTR pwzAssemblyPath,
            /* [in] */ LPCWSTR pwzTypeName,
            /* [in] */ LPCWSTR pwzMethodName,
            /* [in] */ LPCWSTR pwzArgument,
            /* [out] */ DWORD *pReturnValue);
        
        END_INTERFACE
    } ICLRRuntimeHostVtbl;

    interface ICLRRuntimeHost
    {
        CONST_VTBL struct ICLRRuntimeHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRRuntimeHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRRuntimeHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRRuntimeHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRRuntimeHost_Start(This)	\
    ( (This)->lpVtbl -> Start(This) ) 

#define ICLRRuntimeHost_Stop(This)	\
    ( (This)->lpVtbl -> Stop(This) ) 

#define ICLRRuntimeHost_SetHostControl(This,pHostControl)	\
    ( (This)->lpVtbl -> SetHostControl(This,pHostControl) ) 

#define ICLRRuntimeHost_GetCLRControl(This,pCLRControl)	\
    ( (This)->lpVtbl -> GetCLRControl(This,pCLRControl) ) 

#define ICLRRuntimeHost_UnloadAppDomain(This,dwAppDomainId,fWaitUntilDone)	\
    ( (This)->lpVtbl -> UnloadAppDomain(This,dwAppDomainId,fWaitUntilDone) ) 

#define ICLRRuntimeHost_ExecuteInAppDomain(This,dwAppDomainId,pCallback,cookie)	\
    ( (This)->lpVtbl -> ExecuteInAppDomain(This,dwAppDomainId,pCallback,cookie) ) 

#define ICLRRuntimeHost_GetCurrentAppDomainId(This,pdwAppDomainId)	\
    ( (This)->lpVtbl -> GetCurrentAppDomainId(This,pdwAppDomainId) ) 

#define ICLRRuntimeHost_ExecuteApplication(This,pwzAppFullName,dwManifestPaths,ppwzManifestPaths,dwActivationData,ppwzActivationData,pReturnValue)	\
    ( (This)->lpVtbl -> ExecuteApplication(This,pwzAppFullName,dwManifestPaths,ppwzManifestPaths,dwActivationData,ppwzActivationData,pReturnValue) ) 

#define ICLRRuntimeHost_ExecuteInDefaultAppDomain(This,pwzAssemblyPath,pwzTypeName,pwzMethodName,pwzArgument,pReturnValue)	\
    ( (This)->lpVtbl -> ExecuteInDefaultAppDomain(This,pwzAssemblyPath,pwzTypeName,pwzMethodName,pwzArgument,pReturnValue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRRuntimeHost_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0045 */
/* [local] */ 

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0045_0001
    {
        eNoChecks	= 0,
        eSynchronization	= 0x1,
        eSharedState	= 0x2,
        eExternalProcessMgmt	= 0x4,
        eSelfAffectingProcessMgmt	= 0x8,
        eExternalThreading	= 0x10,
        eSelfAffectingThreading	= 0x20,
        eSecurityInfrastructure	= 0x40,
        eUI	= 0x80,
        eMayLeakOnAbort	= 0x100,
        eAll	= 0x1ff
    } 	EApiCategories;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0045_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0045_v0_0_s_ifspec;

#ifndef __ICLRHostProtectionManager_INTERFACE_DEFINED__
#define __ICLRHostProtectionManager_INTERFACE_DEFINED__

/* interface ICLRHostProtectionManager */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICLRHostProtectionManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89F25F5C-CEEF-43e1-9CFA-A68CE863AAAC")
    ICLRHostProtectionManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProtectedCategories( 
            /* [in] */ EApiCategories categories) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEagerSerializeGrantSets( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRHostProtectionManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ICLRHostProtectionManager * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ICLRHostProtectionManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ICLRHostProtectionManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProtectedCategories )( 
            __RPC__in ICLRHostProtectionManager * This,
            /* [in] */ EApiCategories categories);
        
        HRESULT ( STDMETHODCALLTYPE *SetEagerSerializeGrantSets )( 
            __RPC__in ICLRHostProtectionManager * This);
        
        END_INTERFACE
    } ICLRHostProtectionManagerVtbl;

    interface ICLRHostProtectionManager
    {
        CONST_VTBL struct ICLRHostProtectionManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRHostProtectionManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRHostProtectionManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRHostProtectionManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRHostProtectionManager_SetProtectedCategories(This,categories)	\
    ( (This)->lpVtbl -> SetProtectedCategories(This,categories) ) 

#define ICLRHostProtectionManager_SetEagerSerializeGrantSets(This)	\
    ( (This)->lpVtbl -> SetEagerSerializeGrantSets(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRHostProtectionManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0046 */
/* [local] */ 

typedef /* [public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0046_0001
    {
        eInitializeNewDomainFlags_None	= 0,
        eInitializeNewDomainFlags_NoSecurityChanges	= 0x2
    } 	EInitializeNewDomainFlags;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0046_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0046_v0_0_s_ifspec;

#ifndef __ICLRDomainManager_INTERFACE_DEFINED__
#define __ICLRDomainManager_INTERFACE_DEFINED__

/* interface ICLRDomainManager */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICLRDomainManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("270D00A2-8E15-4d0b-ADEB-37BC3E47DF77")
    ICLRDomainManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAppDomainManagerType( 
            /* [in] */ __RPC__in LPCWSTR wszAppDomainManagerAssembly,
            /* [in] */ __RPC__in LPCWSTR wszAppDomainManagerType,
            /* [in] */ EInitializeNewDomainFlags dwInitializeDomainFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPropertiesForDefaultAppDomain( 
            /* [in] */ DWORD nProperties,
            /* [in] */ __RPC__deref_in_opt LPCWSTR *pwszPropertyNames,
            /* [in] */ __RPC__deref_in_opt LPCWSTR *pwszPropertyValues) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRDomainManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ICLRDomainManager * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ICLRDomainManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ICLRDomainManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAppDomainManagerType )( 
            __RPC__in ICLRDomainManager * This,
            /* [in] */ __RPC__in LPCWSTR wszAppDomainManagerAssembly,
            /* [in] */ __RPC__in LPCWSTR wszAppDomainManagerType,
            /* [in] */ EInitializeNewDomainFlags dwInitializeDomainFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetPropertiesForDefaultAppDomain )( 
            __RPC__in ICLRDomainManager * This,
            /* [in] */ DWORD nProperties,
            /* [in] */ __RPC__deref_in_opt LPCWSTR *pwszPropertyNames,
            /* [in] */ __RPC__deref_in_opt LPCWSTR *pwszPropertyValues);
        
        END_INTERFACE
    } ICLRDomainManagerVtbl;

    interface ICLRDomainManager
    {
        CONST_VTBL struct ICLRDomainManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRDomainManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRDomainManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRDomainManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRDomainManager_SetAppDomainManagerType(This,wszAppDomainManagerAssembly,wszAppDomainManagerType,dwInitializeDomainFlags)	\
    ( (This)->lpVtbl -> SetAppDomainManagerType(This,wszAppDomainManagerAssembly,wszAppDomainManagerType,dwInitializeDomainFlags) ) 

#define ICLRDomainManager_SetPropertiesForDefaultAppDomain(This,nProperties,pwszPropertyNames,pwszPropertyValues)	\
    ( (This)->lpVtbl -> SetPropertiesForDefaultAppDomain(This,nProperties,pwszPropertyNames,pwszPropertyValues) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRDomainManager_INTERFACE_DEFINED__ */



#ifndef __mscoree_LIBRARY_DEFINED__
#define __mscoree_LIBRARY_DEFINED__

/* library mscoree */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_mscoree;

#ifndef __ITypeName_INTERFACE_DEFINED__
#define __ITypeName_INTERFACE_DEFINED__

/* interface ITypeName */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_ITypeName;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B81FF171-20F3-11d2-8DCC-00A0C9B00522")
    ITypeName : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetNameCount( 
            /* [retval][out] */ __RPC__out DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ DWORD count,
            /* [out] */ __RPC__deref_out_opt BSTR *rgbszNames,
            /* [retval][out] */ __RPC__out DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeArgumentCount( 
            /* [retval][out] */ __RPC__out DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeArguments( 
            /* [in] */ DWORD count,
            /* [out] */ __RPC__deref_out_opt ITypeName **rgpArguments,
            /* [retval][out] */ __RPC__out DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModifierLength( 
            /* [retval][out] */ __RPC__out DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetModifiers( 
            /* [in] */ DWORD count,
            /* [out] */ __RPC__out DWORD *rgModifiers,
            /* [retval][out] */ __RPC__out DWORD *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAssemblyName( 
            /* [retval][out] */ __RPC__deref_out_opt BSTR *rgbszAssemblyNames) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ITypeNameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ITypeName * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ITypeName * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ITypeName * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetNameCount )( 
            __RPC__in ITypeName * This,
            /* [retval][out] */ __RPC__out DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetNames )( 
            __RPC__in ITypeName * This,
            /* [in] */ DWORD count,
            /* [out] */ __RPC__deref_out_opt BSTR *rgbszNames,
            /* [retval][out] */ __RPC__out DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeArgumentCount )( 
            __RPC__in ITypeName * This,
            /* [retval][out] */ __RPC__out DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeArguments )( 
            __RPC__in ITypeName * This,
            /* [in] */ DWORD count,
            /* [out] */ __RPC__deref_out_opt ITypeName **rgpArguments,
            /* [retval][out] */ __RPC__out DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetModifierLength )( 
            __RPC__in ITypeName * This,
            /* [retval][out] */ __RPC__out DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetModifiers )( 
            __RPC__in ITypeName * This,
            /* [in] */ DWORD count,
            /* [out] */ __RPC__out DWORD *rgModifiers,
            /* [retval][out] */ __RPC__out DWORD *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetAssemblyName )( 
            __RPC__in ITypeName * This,
            /* [retval][out] */ __RPC__deref_out_opt BSTR *rgbszAssemblyNames);
        
        END_INTERFACE
    } ITypeNameVtbl;

    interface ITypeName
    {
        CONST_VTBL struct ITypeNameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeName_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ITypeName_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ITypeName_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ITypeName_GetNameCount(This,pCount)	\
    ( (This)->lpVtbl -> GetNameCount(This,pCount) ) 

#define ITypeName_GetNames(This,count,rgbszNames,pCount)	\
    ( (This)->lpVtbl -> GetNames(This,count,rgbszNames,pCount) ) 

#define ITypeName_GetTypeArgumentCount(This,pCount)	\
    ( (This)->lpVtbl -> GetTypeArgumentCount(This,pCount) ) 

#define ITypeName_GetTypeArguments(This,count,rgpArguments,pCount)	\
    ( (This)->lpVtbl -> GetTypeArguments(This,count,rgpArguments,pCount) ) 

#define ITypeName_GetModifierLength(This,pCount)	\
    ( (This)->lpVtbl -> GetModifierLength(This,pCount) ) 

#define ITypeName_GetModifiers(This,count,rgModifiers,pCount)	\
    ( (This)->lpVtbl -> GetModifiers(This,count,rgModifiers,pCount) ) 

#define ITypeName_GetAssemblyName(This,rgbszAssemblyNames)	\
    ( (This)->lpVtbl -> GetAssemblyName(This,rgbszAssemblyNames) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ITypeName_INTERFACE_DEFINED__ */


#ifndef __ITypeNameBuilder_INTERFACE_DEFINED__
#define __ITypeNameBuilder_INTERFACE_DEFINED__

/* interface ITypeNameBuilder */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_ITypeNameBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B81FF171-20F3-11d2-8DCC-00A0C9B00523")
    ITypeNameBuilder : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenGenericArguments( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseGenericArguments( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenGenericArgument( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseGenericArgument( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddName( 
            /* [in] */ __RPC__in LPCWSTR szName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPointer( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddByRef( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddSzArray( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddArray( 
            /* [in] */ DWORD rank) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddAssemblySpec( 
            /* [in] */ __RPC__in LPCWSTR szAssemblySpec) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ToString( 
            /* [retval][out] */ __RPC__deref_out_opt BSTR *pszStringRepresentation) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ITypeNameBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ITypeNameBuilder * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ITypeNameBuilder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenGenericArguments )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *CloseGenericArguments )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenGenericArgument )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *CloseGenericArgument )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddName )( 
            __RPC__in ITypeNameBuilder * This,
            /* [in] */ __RPC__in LPCWSTR szName);
        
        HRESULT ( STDMETHODCALLTYPE *AddPointer )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddByRef )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddSzArray )( 
            __RPC__in ITypeNameBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddArray )( 
            __RPC__in ITypeNameBuilder * This,
            /* [in] */ DWORD rank);
        
        HRESULT ( STDMETHODCALLTYPE *AddAssemblySpec )( 
            __RPC__in ITypeNameBuilder * This,
            /* [in] */ __RPC__in LPCWSTR szAssemblySpec);
        
        HRESULT ( STDMETHODCALLTYPE *ToString )( 
            __RPC__in ITypeNameBuilder * This,
            /* [retval][out] */ __RPC__deref_out_opt BSTR *pszStringRepresentation);
        
        HRESULT ( STDMETHODCALLTYPE *Clear )( 
            __RPC__in ITypeNameBuilder * This);
        
        END_INTERFACE
    } ITypeNameBuilderVtbl;

    interface ITypeNameBuilder
    {
        CONST_VTBL struct ITypeNameBuilderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeNameBuilder_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ITypeNameBuilder_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ITypeNameBuilder_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ITypeNameBuilder_OpenGenericArguments(This)	\
    ( (This)->lpVtbl -> OpenGenericArguments(This) ) 

#define ITypeNameBuilder_CloseGenericArguments(This)	\
    ( (This)->lpVtbl -> CloseGenericArguments(This) ) 

#define ITypeNameBuilder_OpenGenericArgument(This)	\
    ( (This)->lpVtbl -> OpenGenericArgument(This) ) 

#define ITypeNameBuilder_CloseGenericArgument(This)	\
    ( (This)->lpVtbl -> CloseGenericArgument(This) ) 

#define ITypeNameBuilder_AddName(This,szName)	\
    ( (This)->lpVtbl -> AddName(This,szName) ) 

#define ITypeNameBuilder_AddPointer(This)	\
    ( (This)->lpVtbl -> AddPointer(This) ) 

#define ITypeNameBuilder_AddByRef(This)	\
    ( (This)->lpVtbl -> AddByRef(This) ) 

#define ITypeNameBuilder_AddSzArray(This)	\
    ( (This)->lpVtbl -> AddSzArray(This) ) 

#define ITypeNameBuilder_AddArray(This,rank)	\
    ( (This)->lpVtbl -> AddArray(This,rank) ) 

#define ITypeNameBuilder_AddAssemblySpec(This,szAssemblySpec)	\
    ( (This)->lpVtbl -> AddAssemblySpec(This,szAssemblySpec) ) 

#define ITypeNameBuilder_ToString(This,pszStringRepresentation)	\
    ( (This)->lpVtbl -> ToString(This,pszStringRepresentation) ) 

#define ITypeNameBuilder_Clear(This)	\
    ( (This)->lpVtbl -> Clear(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ITypeNameBuilder_INTERFACE_DEFINED__ */


#ifndef __ITypeNameFactory_INTERFACE_DEFINED__
#define __ITypeNameFactory_INTERFACE_DEFINED__

/* interface ITypeNameFactory */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_ITypeNameFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B81FF171-20F3-11d2-8DCC-00A0C9B00521")
    ITypeNameFactory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ParseTypeName( 
            /* [in] */ __RPC__in LPCWSTR szName,
            /* [out] */ __RPC__out DWORD *pError,
            /* [retval][out] */ __RPC__deref_out_opt ITypeName **ppTypeName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeNameBuilder( 
            /* [retval][out] */ __RPC__deref_out_opt ITypeNameBuilder **ppTypeBuilder) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ITypeNameFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ITypeNameFactory * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ITypeNameFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ITypeNameFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *ParseTypeName )( 
            __RPC__in ITypeNameFactory * This,
            /* [in] */ __RPC__in LPCWSTR szName,
            /* [out] */ __RPC__out DWORD *pError,
            /* [retval][out] */ __RPC__deref_out_opt ITypeName **ppTypeName);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeNameBuilder )( 
            __RPC__in ITypeNameFactory * This,
            /* [retval][out] */ __RPC__deref_out_opt ITypeNameBuilder **ppTypeBuilder);
        
        END_INTERFACE
    } ITypeNameFactoryVtbl;

    interface ITypeNameFactory
    {
        CONST_VTBL struct ITypeNameFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITypeNameFactory_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ITypeNameFactory_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ITypeNameFactory_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ITypeNameFactory_ParseTypeName(This,szName,pError,ppTypeName)	\
    ( (This)->lpVtbl -> ParseTypeName(This,szName,pError,ppTypeName) ) 

#define ITypeNameFactory_GetTypeNameBuilder(This,ppTypeBuilder)	\
    ( (This)->lpVtbl -> GetTypeNameBuilder(This,ppTypeBuilder) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ITypeNameFactory_INTERFACE_DEFINED__ */


#ifndef __IApartmentCallback_INTERFACE_DEFINED__
#define __IApartmentCallback_INTERFACE_DEFINED__

/* interface IApartmentCallback */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IApartmentCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("178E5337-1528-4591-B1C9-1C6E484686D8")
    IApartmentCallback : public IUnknown
    {
    public:
        virtual HRESULT __stdcall DoCallback( 
            /* [in] */ SIZE_T pFunc,
            /* [in] */ SIZE_T pData) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IApartmentCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IApartmentCallback * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IApartmentCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IApartmentCallback * This);
        
        HRESULT ( __stdcall *DoCallback )( 
            __RPC__in IApartmentCallback * This,
            /* [in] */ SIZE_T pFunc,
            /* [in] */ SIZE_T pData);
        
        END_INTERFACE
    } IApartmentCallbackVtbl;

    interface IApartmentCallback
    {
        CONST_VTBL struct IApartmentCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApartmentCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IApartmentCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IApartmentCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IApartmentCallback_DoCallback(This,pFunc,pData)	\
    ( (This)->lpVtbl -> DoCallback(This,pFunc,pData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IApartmentCallback_INTERFACE_DEFINED__ */


#ifndef __IManagedObject_INTERFACE_DEFINED__
#define __IManagedObject_INTERFACE_DEFINED__

/* interface IManagedObject */
/* [proxy][unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IManagedObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C3FCC19E-A970-11d2-8B5A-00A0C9B7C9C4")
    IManagedObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSerializedBuffer( 
            /* [out] */ __RPC__deref_out_opt BSTR *pBSTR) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectIdentity( 
            /* [out] */ __RPC__deref_out_opt BSTR *pBSTRGUID,
            /* [out] */ __RPC__out int *AppDomainID,
            /* [out] */ __RPC__out int *pCCW) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IManagedObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IManagedObject * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IManagedObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IManagedObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetSerializedBuffer )( 
            __RPC__in IManagedObject * This,
            /* [out] */ __RPC__deref_out_opt BSTR *pBSTR);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectIdentity )( 
            __RPC__in IManagedObject * This,
            /* [out] */ __RPC__deref_out_opt BSTR *pBSTRGUID,
            /* [out] */ __RPC__out int *AppDomainID,
            /* [out] */ __RPC__out int *pCCW);
        
        END_INTERFACE
    } IManagedObjectVtbl;

    interface IManagedObject
    {
        CONST_VTBL struct IManagedObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IManagedObject_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IManagedObject_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IManagedObject_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IManagedObject_GetSerializedBuffer(This,pBSTR)	\
    ( (This)->lpVtbl -> GetSerializedBuffer(This,pBSTR) ) 

#define IManagedObject_GetObjectIdentity(This,pBSTRGUID,AppDomainID,pCCW)	\
    ( (This)->lpVtbl -> GetObjectIdentity(This,pBSTRGUID,AppDomainID,pCCW) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IManagedObject_INTERFACE_DEFINED__ */


#ifndef __ICatalogServices_INTERFACE_DEFINED__
#define __ICatalogServices_INTERFACE_DEFINED__

/* interface ICatalogServices */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_ICatalogServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("04C6BE1E-1DB1-4058-AB7A-700CCCFBF254")
    ICatalogServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Autodone( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotAutodone( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICatalogServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in ICatalogServices * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in ICatalogServices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in ICatalogServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *Autodone )( 
            __RPC__in ICatalogServices * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotAutodone )( 
            __RPC__in ICatalogServices * This);
        
        END_INTERFACE
    } ICatalogServicesVtbl;

    interface ICatalogServices
    {
        CONST_VTBL struct ICatalogServicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICatalogServices_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICatalogServices_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICatalogServices_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICatalogServices_Autodone(This)	\
    ( (This)->lpVtbl -> Autodone(This) ) 

#define ICatalogServices_NotAutodone(This)	\
    ( (This)->lpVtbl -> NotAutodone(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICatalogServices_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ComCallUnmarshal;

#ifdef __cplusplus

class DECLSPEC_UUID("3F281000-E95A-11d2-886B-00C04F869F04")
ComCallUnmarshal;
#endif

EXTERN_C const CLSID CLSID_ComCallUnmarshalV4;

#ifdef __cplusplus

class DECLSPEC_UUID("45FB4600-E6E8-4928-B25E-50476FF79425")
ComCallUnmarshalV4;
#endif

EXTERN_C const CLSID CLSID_CorRuntimeHost;

#ifdef __cplusplus

class DECLSPEC_UUID("CB2F6723-AB3A-11d2-9C40-00C04FA30A3E")
CorRuntimeHost;
#endif

EXTERN_C const CLSID CLSID_CLRRuntimeHost;

#ifdef __cplusplus

class DECLSPEC_UUID("90F1A06E-7712-4762-86B5-7A5EBA6BDB02")
CLRRuntimeHost;
#endif

EXTERN_C const CLSID CLSID_TypeNameFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("B81FF171-20F3-11d2-8DCC-00A0C9B00525")
TypeNameFactory;
#endif
#endif /* __mscoree_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_mscoree_0000_0048 */
/* [local] */ 

typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_mscoree_0000_0048_0001
    {
        eCurrentContext	= 0,
        eRestrictedContext	= 0x1
    } 	EContextType;



extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0048_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0048_v0_0_s_ifspec;

#ifndef __IHostSecurityContext_INTERFACE_DEFINED__
#define __IHostSecurityContext_INTERFACE_DEFINED__

/* interface IHostSecurityContext */
/* [local][unique][helpstring][uuid][version][object] */ 


EXTERN_C const IID IID_IHostSecurityContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7E573CE4-0343-4423-98D7-6318348A1D3C")
    IHostSecurityContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Capture( 
            /* [out] */ IHostSecurityContext **ppClonedContext) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostSecurityContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostSecurityContext * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostSecurityContext * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostSecurityContext * This);
        
        HRESULT ( STDMETHODCALLTYPE *Capture )( 
            IHostSecurityContext * This,
            /* [out] */ IHostSecurityContext **ppClonedContext);
        
        END_INTERFACE
    } IHostSecurityContextVtbl;

    interface IHostSecurityContext
    {
        CONST_VTBL struct IHostSecurityContextVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostSecurityContext_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostSecurityContext_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostSecurityContext_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostSecurityContext_Capture(This,ppClonedContext)	\
    ( (This)->lpVtbl -> Capture(This,ppClonedContext) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostSecurityContext_INTERFACE_DEFINED__ */


#ifndef __IHostSecurityManager_INTERFACE_DEFINED__
#define __IHostSecurityManager_INTERFACE_DEFINED__

/* interface IHostSecurityManager */
/* [local][unique][helpstring][uuid][version][object] */ 


EXTERN_C const IID IID_IHostSecurityManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("75ad2468-a349-4d02-a764-76a68aee0c4f")
    IHostSecurityManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ImpersonateLoggedOnUser( 
            /* [in] */ HANDLE hToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevertToSelf( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OpenThreadToken( 
            /* [in] */ DWORD dwDesiredAccess,
            /* [in] */ BOOL bOpenAsSelf,
            /* [out] */ HANDLE *phThreadToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreadToken( 
            /* [in] */ HANDLE hToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSecurityContext( 
            /* [in] */ EContextType eContextType,
            /* [out] */ IHostSecurityContext **ppSecurityContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSecurityContext( 
            /* [in] */ EContextType eContextType,
            /* [in] */ IHostSecurityContext *pSecurityContext) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IHostSecurityManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHostSecurityManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHostSecurityManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHostSecurityManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *ImpersonateLoggedOnUser )( 
            IHostSecurityManager * This,
            /* [in] */ HANDLE hToken);
        
        HRESULT ( STDMETHODCALLTYPE *RevertToSelf )( 
            IHostSecurityManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenThreadToken )( 
            IHostSecurityManager * This,
            /* [in] */ DWORD dwDesiredAccess,
            /* [in] */ BOOL bOpenAsSelf,
            /* [out] */ HANDLE *phThreadToken);
        
        HRESULT ( STDMETHODCALLTYPE *SetThreadToken )( 
            IHostSecurityManager * This,
            /* [in] */ HANDLE hToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetSecurityContext )( 
            IHostSecurityManager * This,
            /* [in] */ EContextType eContextType,
            /* [out] */ IHostSecurityContext **ppSecurityContext);
        
        HRESULT ( STDMETHODCALLTYPE *SetSecurityContext )( 
            IHostSecurityManager * This,
            /* [in] */ EContextType eContextType,
            /* [in] */ IHostSecurityContext *pSecurityContext);
        
        END_INTERFACE
    } IHostSecurityManagerVtbl;

    interface IHostSecurityManager
    {
        CONST_VTBL struct IHostSecurityManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHostSecurityManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IHostSecurityManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IHostSecurityManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IHostSecurityManager_ImpersonateLoggedOnUser(This,hToken)	\
    ( (This)->lpVtbl -> ImpersonateLoggedOnUser(This,hToken) ) 

#define IHostSecurityManager_RevertToSelf(This)	\
    ( (This)->lpVtbl -> RevertToSelf(This) ) 

#define IHostSecurityManager_OpenThreadToken(This,dwDesiredAccess,bOpenAsSelf,phThreadToken)	\
    ( (This)->lpVtbl -> OpenThreadToken(This,dwDesiredAccess,bOpenAsSelf,phThreadToken) ) 

#define IHostSecurityManager_SetThreadToken(This,hToken)	\
    ( (This)->lpVtbl -> SetThreadToken(This,hToken) ) 

#define IHostSecurityManager_GetSecurityContext(This,eContextType,ppSecurityContext)	\
    ( (This)->lpVtbl -> GetSecurityContext(This,eContextType,ppSecurityContext) ) 

#define IHostSecurityManager_SetSecurityContext(This,eContextType,pSecurityContext)	\
    ( (This)->lpVtbl -> SetSecurityContext(This,eContextType,pSecurityContext) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IHostSecurityManager_INTERFACE_DEFINED__ */


#ifndef __ICLRAppDomainResourceMonitor_INTERFACE_DEFINED__
#define __ICLRAppDomainResourceMonitor_INTERFACE_DEFINED__

/* interface ICLRAppDomainResourceMonitor */
/* [object][local][unique][helpstring][uuid][version] */ 


EXTERN_C const IID IID_ICLRAppDomainResourceMonitor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c62de18c-2e23-4aea-8423-b40c1fc59eae")
    ICLRAppDomainResourceMonitor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCurrentAllocated( 
            /* [in] */ DWORD dwAppDomainId,
            /* [out] */ ULONGLONG *pBytesAllocated) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentSurvived( 
            /* [in] */ DWORD dwAppDomainId,
            /* [out] */ ULONGLONG *pAppDomainBytesSurvived,
            /* [out] */ ULONGLONG *pTotalBytesSurvived) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentCpuTime( 
            /* [in] */ DWORD dwAppDomainId,
            /* [out] */ ULONGLONG *pMilliseconds) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICLRAppDomainResourceMonitorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICLRAppDomainResourceMonitor * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICLRAppDomainResourceMonitor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICLRAppDomainResourceMonitor * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentAllocated )( 
            ICLRAppDomainResourceMonitor * This,
            /* [in] */ DWORD dwAppDomainId,
            /* [out] */ ULONGLONG *pBytesAllocated);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentSurvived )( 
            ICLRAppDomainResourceMonitor * This,
            /* [in] */ DWORD dwAppDomainId,
            /* [out] */ ULONGLONG *pAppDomainBytesSurvived,
            /* [out] */ ULONGLONG *pTotalBytesSurvived);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentCpuTime )( 
            ICLRAppDomainResourceMonitor * This,
            /* [in] */ DWORD dwAppDomainId,
            /* [out] */ ULONGLONG *pMilliseconds);
        
        END_INTERFACE
    } ICLRAppDomainResourceMonitorVtbl;

    interface ICLRAppDomainResourceMonitor
    {
        CONST_VTBL struct ICLRAppDomainResourceMonitorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICLRAppDomainResourceMonitor_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICLRAppDomainResourceMonitor_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICLRAppDomainResourceMonitor_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICLRAppDomainResourceMonitor_GetCurrentAllocated(This,dwAppDomainId,pBytesAllocated)	\
    ( (This)->lpVtbl -> GetCurrentAllocated(This,dwAppDomainId,pBytesAllocated) ) 

#define ICLRAppDomainResourceMonitor_GetCurrentSurvived(This,dwAppDomainId,pAppDomainBytesSurvived,pTotalBytesSurvived)	\
    ( (This)->lpVtbl -> GetCurrentSurvived(This,dwAppDomainId,pAppDomainBytesSurvived,pTotalBytesSurvived) ) 

#define ICLRAppDomainResourceMonitor_GetCurrentCpuTime(This,dwAppDomainId,pMilliseconds)	\
    ( (This)->lpVtbl -> GetCurrentCpuTime(This,dwAppDomainId,pMilliseconds) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICLRAppDomainResourceMonitor_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_mscoree_0000_0051 */
/* [local] */ 

#undef DEPRECATED_CLR_STDAPI
#undef DECLARE_DEPRECATED
#undef DEPRECATED_CLR_API_MESG


extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0051_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mscoree_0000_0051_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  VARIANT_UserSize(     __RPC__in unsigned long *, unsigned long            , __RPC__in VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  __RPC__in unsigned long *, __RPC__inout_xcount(0) unsigned char *, __RPC__in VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(__RPC__in unsigned long *, __RPC__in_xcount(0) unsigned char *, __RPC__out VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     __RPC__in unsigned long *, __RPC__in VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


