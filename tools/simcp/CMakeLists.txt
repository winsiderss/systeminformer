#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "include/simcp.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "version.rc"
    "simcp.manifest"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "main.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_executable(simcp ${ALL_FILES})

target_include_directories(simcp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${SI_ROOT}/phlib/include"
    "${SI_ROOT}/kphlib/include"
)

target_compile_definitions(simcp PRIVATE
    _PHLIB_
)

target_link_libraries(simcp PRIVATE
    bcrypt
    crypt32
    gdiplus
    phlib
    ntdll
    userenv
    winhttp
)

if(MSVC)
    target_link_options(simcp PRIVATE /SUBSYSTEM:CONSOLE,10.0)
endif()
