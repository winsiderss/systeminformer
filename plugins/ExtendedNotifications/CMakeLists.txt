#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "extnoti.h"
    "resource.h"
)
source_group("Header Files" FILES ${HEADERS})

set(SOURCES
    "filelog.c"
    "main.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${SOURCES}
)

si_add_plugin(ExtendedNotifications ${ALL_FILES})
