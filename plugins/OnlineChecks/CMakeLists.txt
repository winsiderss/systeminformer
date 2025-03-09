#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "onlnchk.h"
    "resource.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES,
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "main.c"
    "options.c"
    "page1.c"
    "page2.c"
    "page3.c"
    "page4.c"
    "upload.c"
    "virustotal.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_plugin(OnlineChecks ${ALL_FILES})
