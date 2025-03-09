#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "clretw.h"
    "clrsup.h"
    "dn.h"
    "resource.h"
    "svcext.h"
)
source_group("Header Files" FILES ${HEADERS})

set(HEADERS_CLR
    "clr/dacprivate.h"
    "clr/dbgappdomain.h"
    "clr/ipcenums.h"
    "clr/ipcheader.h"
    "clr/ipcshared.h"
    "clr/perfcounterdefs.h"
    "clr/sospriv.h"
    "clr/xclrdata.h"
)
source_group("Header Files\\clr" FILES ${HEADERS_CLR})

set(RESOURCES,
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "asmpage.c"
    "clrsup.c"
    "counters.c"
    "main.c"
    "perfpage.c"
    "stackext.c"
    "svcext.c"
    "treeext.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${HEADERS_CLR}
    ${RESOURCES}
    ${SOURCES}
)

si_add_plugin(DotNetTools ${ALL_FILES})

target_link_libraries(DotNetTools PRIVATE
    delayimp
    advapi32
)

target_link_options(DotNetTools PRIVATE
    /DELAYLOAD:advapi32.dll
)
