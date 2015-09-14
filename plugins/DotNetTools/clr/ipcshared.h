//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.
//

// dmex: This header has been highly modified.
// Original: https://github.com/dotnet/coreclr/blob/master/src/ipcman/ipcshared.h

#ifndef _IPC_SHARED_H_
#define _IPC_SHARED_H_

// This is the name of the file backed session's name on the LS (debuggee)
// Name of the LegacyPrivate (per-process) block. %d resolved to a PID
#define CorLegacyPrivateIPCBlock            L"Cor_Private_IPCBlock_%d"
#define CorLegacyPrivateIPCBlockTempV4      L"Cor_Private_IPCBlock_v4_%d"
#define CorLegacyPublicIPCBlock             L"Cor_Public_IPCBlock_%d"
#define CorSxSPublicIPCBlock                L"Cor_SxSPublic_IPCBlock_%d"
#define CorSxSBoundaryDescriptor            L"Cor_CLR_IPCBlock_%d"
#define CorSxSWriterPrivateNamespacePrefix  L"Cor_CLR_WRITER"
#define CorSxSReaderPrivateNamespacePrefix  L"Cor_CLR_READER"
#define CorSxSVistaPublicIPCBlock           L"Cor_SxSPublic_IPCBlock"

#define CorLegacyPrivateIPCBlock_RS         L"CLR_PRIVATE_RS_IPCBlock_%d"
#define CorLegacyPrivateIPCBlock_RSTempV4   L"CLR_PRIVATE_RS_IPCBlock_v4_%d"
#define CorLegacyPublicIPCBlock_RS          L"CLR_PUBLIC_IPCBlock_%d"
#define CorSxSPublicIPCBlock_RS             L"CLR_SXSPUBLIC_IPCBlock_%d"

#define CorSxSPublicInstanceName            L"%s_p%d_r%d"
#define CorSxSPublicInstanceNameWhidbey     L"%s_p%d"

#endif _IPC_SHARED_H_