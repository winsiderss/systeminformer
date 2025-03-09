#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "extsrv.h"
    "resource.h"
)
source_group("Header Files" FILES ${HEADERS})

set(SOURCES
    "depend.c"
    "main.c"
    "other.c"
    "recovery.c"
    "srvprgrs.c"
    "svcpkg.c"
    "svcpnp.c"
    "trigger.c"
    "triggpg.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${SOURCES}
)

si_add_plugin(ExtendedServices ${ALL_FILES})

target_link_libraries(ExtendedServices PRIVATE
    delayimp
    cfgmgr32
)

target_link_options(DotNetTools PRIVATE
    /DELAYLOAD:cfgmgr32.dll
)
