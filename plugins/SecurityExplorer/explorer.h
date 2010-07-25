#ifndef EXPLORER_H
#define EXPLORER_H

#include <phdk.h>
#include "resource.h"

#ifndef SX_MAIN_PRIVATE
extern PPH_PLUGIN PluginInstance;
#endif

VOID SxShowExplorer();

__callback NTSTATUS SxStdGetObjectSecurity(
    __out PSECURITY_DESCRIPTOR *SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    );

__callback NTSTATUS SxStdSetObjectSecurity(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_INFORMATION SecurityInformation,
    __in PVOID Context
    );

#endif
