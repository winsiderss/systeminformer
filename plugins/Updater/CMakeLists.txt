#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "resource.h"
    "updater.h"
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
    "page5.c"
    "updater.c"
    "verify.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_plugin(Updater ${ALL_FILES})

target_link_libraries(Updater PRIVATE
    delayimp
    bcrypt
    kphlib_um
)

target_link_options(Updater PRIVATE
    /DELAYLOAD:bcrypt.dll
)

