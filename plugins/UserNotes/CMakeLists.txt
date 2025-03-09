#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "db.h"
    "resource.h"
    "usernotes.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES,
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "db.c"
    "main.c"
    "options.c"
    "prpcmpage.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_plugin(UserNotes ${ALL_FILES})

target_link_libraries(UserNotes PRIVATE
    delayimp
    comdlg32
)

target_link_options(UserNotes PRIVATE
    /DELAYLOAD:comdlg32.dll
)
