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
    "main.c"
    "mcp.c"
    "options.c"
    "schema.c"
    "server.c"
    "target.c"
    "toolmod.c"
    "toolproc.c"
    "tools.c"
    "toolmem.c"
    "toolpe.c"
    "toolstart.c"
    "toolnet.c"
    "toolsvc.c"
    "toolsys.c"
    "toolthrd.c"
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
)
