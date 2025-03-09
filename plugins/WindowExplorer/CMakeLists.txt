#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "prpsh.h"
    "resource.h"
    "wndexp.h"
    "wndtree.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES,
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "main.c"
    "prpsh.c"
    "utils.c"
    "wnddlg.c"
    "wndprp.c"
    "wndtree.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_plugin(WindowExplorer ${ALL_FILES})

target_link_libraries(WindowExplorer PRIVATE
    delayimp
    comctl32
    comctl32
    gdi32
    ole32
    propsys
    shell32
    user32
)

target_link_options(WindowExplorer PRIVATE
    /DELAYLOAD:comctl32.dll
    /DELAYLOAD:comctl32.dll
    /DELAYLOAD:gdi32.dll
    /DELAYLOAD:ole32.dll
    /DELAYLOAD:propsys.dll
    /DELAYLOAD:shell32.dll
    /DELAYLOAD:user32.dll
)
