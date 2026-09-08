#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "agenttools.h"
    "resource.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "version.rc"
    "AgentTools.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "consent.c"
    "events.c"
    "main.c"
    "mcp.c"
    "options.c"
    "schema.c"
    "server.c"
    "snapshot.c"
    "target.c"
    "tooladpt.c"
    "tooldev.c"
    "tooldisk.c"
    "tooldnet.c"
    "toolegrs.c"
    "toolfind.c"
    "toolfw.c"
    "toolfile.c"
    "toolgpu.c"
    "toolhndl.c"
    "toolimg.c"
    "toolhist.c"
    "toolio.c"
    "toolmod.c"
    "toolmtgn.c"
    "toolnote.c"
    "toolproc.c"
    "toolwnd.c"
    "tools.c"
    "toolmem.c"
    "toolpe.c"
    "toolstart.c"
    "toolnet.c"
    "toolsvc.c"
    "toolsess.c"
    "toolsys.c"
    "toolthrd.c"
    "toolwait.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_plugin(AgentTools ${ALL_FILES})

target_include_directories(AgentTools PRIVATE
    "${SI_ROOT}/tools/simcp/include"
)

target_link_libraries(AgentTools PRIVATE
    user32
    gdi32
    winsta
)
