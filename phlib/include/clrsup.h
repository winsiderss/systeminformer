#ifndef _PH_CLRSUP_H
#define _PH_CLRSUP_H

typedef PVOID PPH_CLR_PUBLISH;
typedef PVOID PPH_CLR_PUBLISH_PROCESS;
typedef PVOID PPH_CLR_PUBLISH_APPDOMAIN;

PPH_CLR_PUBLISH PhClrCreatePublish();

VOID PhClrFreePublish(
    __in PPH_CLR_PUBLISH Publish
    );

PPH_CLR_PUBLISH_PROCESS PhClrGetProcessPublish(
    __in PPH_CLR_PUBLISH Publish,
    __in HANDLE ProcessId
    );

VOID PhClrFreePublishProcess(
    __in PPH_CLR_PUBLISH_PROCESS Process
    );

BOOLEAN PhClrIsManagedPublishProcess(
    __in PPH_CLR_PUBLISH_PROCESS Process
    );

BOOLEAN PhClrEnumAppDomainsPublishProcess(
    __in PPH_CLR_PUBLISH_PROCESS Process,
    __out PPH_CLR_PUBLISH_APPDOMAIN *AppDomains,
    __out PULONG NumberOfAppDomains
    );

VOID PhClrFreePublishAppDomain(
    __in PPH_CLR_PUBLISH_APPDOMAIN AppDomain
    );

PPH_STRING PhClrGetNamePublishAppDomain(
    __in PPH_CLR_PUBLISH_APPDOMAIN AppDomain
    );

#endif
