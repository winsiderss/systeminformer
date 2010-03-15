#ifndef PHSVC_H
#define PHSVC_H

#include <ph.h>
#include <phsvcapi.h>

#define PHSVC_PORT_NAME (L"\\BaseNamedObjects\\PhSvcApiPort")

NTSTATUS PhApiPortInitialization();

#endif
