#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "resource.h"
    "toolstatus.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES,
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "customizesb.c"
    "customizetb.c"
    "filter.c"
    "graph.c"
    "main.c"
    "options.c"
    "statusbar.c"
    "taskbar.c"
    "toolbar.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_plugin(ToolStatus ${ALL_FILES})
