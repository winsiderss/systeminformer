#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "nettools.h"
    "resource.h"
    "tracert.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(RESOURCES_IMAGES
    "resources/licence.txt"
)
source_group("Resource Files\\Images" FILES ${RESOURCES_IMAGES})

set(SOURCES
    "country.c"
    "main.c"
    "options.c"
    "pages.c"
    "ping.c"
    "ports.c"
    "tracert.c"
    "tracetree.c"
    "update.c"
    "whois.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${RESOURCES_IMAGES}
    ${SOURCES}
)

si_add_plugin(NetworkTools ${ALL_FILES})

target_link_libraries(NetworkTools PRIVATE
    thirdparty
    delayimp
    iphlpapi
    ws2_32
)

target_link_options(NetworkTools PRIVATE
    /DELAYLOAD:iphlpapi.dll
    /DELAYLOAD:ws2_32.dll
)
