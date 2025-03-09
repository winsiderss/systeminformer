#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "disktabp.h"
    "efi_guid_list.h"
    "etwmini.h"
    "etwmon.h"
    "etwsys.h"
    "extension/plugin.h"
    "exttools.h"
    "framemon.h"
    "gpumini.h"
    "gpumon.h"
    "gpusys.h"
    "npumini.h"
    "npumon.h"
    "npusys.h"
    "poolmon.h"
    "resource.h"
    "tpm.h"
)
source_group("Header Files" FILES ${HEADERS})

set(HEADERS_PRESENTMON
    "PresentMon/ETW/Microsoft_Windows_D3D9.h"
    "PresentMon/ETW/Microsoft_Windows_Dwm_Core.h"
    "PresentMon/ETW/Microsoft_Windows_DXGI.h"
    "PresentMon/ETW/Microsoft_Windows_DxgKrnl.h"
    "PresentMon/ETW/Microsoft_Windows_EventMetadata.h"
    "PresentMon/ETW/Microsoft_Windows_Win32k.h"
    "PresentMon/PresentMon.hpp"
    "PresentMon/PresentMonTraceConsumer.hpp"
    "PresentMon/TraceConsumer.hpp"
)
source_group("Header Files\\PresentMon" FILES ${HEADERS_PRESENTMON})

set(SOURCES
    "counters.c"
    "disktab.c"
    "etwdisk.c"
    "etwmini.c"
    "etwmon.c"
    "etwprprp.c"
    "etwstat.c"
    "etwsys.c"
    "firmware.c"
    "firmware_editor.c"
    "framemon.cpp"
    "frameprp.c"
    "fwmon.c"
    "fwtab.c"
    "iconext.c"
    "main.c"
    "modsrv.c"
    "namedpipes.c"
    "objmgr.c"
    "objprp.c"
    "options.c"
    "pooldb.c"
    "pooldialog.c"
    "pooldialogbig.c"
    "pooltree.c"
    "reparse.c"
    "smbios.c"
    "svcext.c"
    "thrdact.c"
    "tpm.c"
    "tpm_editor.c"
    "treeext.c"
    "unldll.c"
    "utils.c"
    "waitchain.c"
    "wswatch.c"
)
source_group("Source Files" FILES ${SOURCES})

set(RESOURCES,
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES_NEURAL
    "npudetails.c"
    "npumini.c"
    "npumon.c"
    "npunodes.c"
    "npuprprp.c"
    "npusys.c"
)
source_group("Source Files\\Neural" FILES ${SOURCES_NEURAL})

set(SOURCES_PRESENTMON
    "PresentMon/PresentMon.cpp"
    "PresentMon/PresentMonTraceConsumer.cpp"
    "PresentMon/TraceConsumer.cpp"
    "PresentMon/TraceSession.cpp"
)
source_group("Source Files\\PresentMon" FILES ${SOURCES_PRESENTMON})

set(SOURCES_VIDEO
    "gpudetails.c"
    "gpumini.c"
    "gpumon.c"
    "gpunodes.c"
    "gpuprprp.c"
    "gpusys.c"
)
source_group("Source Files\\Video" FILES ${SOURCES_VIDEO})

set(ALL_FILES
    ${HEADERS}
    ${HEADERS_PRESENTMON}
    ${RESOURCES}
    ${SOURCES}
    ${SOURCES_NEURAL}
    ${SOURCES_PRESENTMON}
    ${SOURCES_VIDEO}
)

si_add_plugin(ExtendedTools ${ALL_FILES})

target_link_libraries(ExtendedTools PRIVATE
    delayimp
    cfgmgr32
    oleaut32
    tdh
    tbs
    winsta
)

target_link_options(ExtendedTools PRIVATE
    /DELAYLOAD:cfgmgr32.dll
    /DELAYLOAD:oleaut32.dll
    /DELAYLOAD:tdh.dll
    /DELAYLOAD:tbs.dll
    /DELAYLOAD:winsta.dll
)
