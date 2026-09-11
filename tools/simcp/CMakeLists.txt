#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "envelope.h"
    "include/simcp.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "version.rc"
    "simcp.manifest"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "envelope.c"
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
)

target_link_libraries(simcp PRIVATE
    phnt
    phlib
    kphlib_um
    delayimp
    user32
    gdi32
    comdlg32
    advapi32
    ole32
    bcrypt
    crypt32
    gdiplus
    ntdll
    userenv
    winhttp
)

target_link_options(simcp PRIVATE
    /DELAYLOAD:advapi32.dll
    /DELAYLOAD:comctl32.dll
    /DELAYLOAD:comdlg32.dll
    /DELAYLOAD:gdi32.dll
    /DELAYLOAD:ole32.dll
    /DELAYLOAD:user32.dll
    /DELAYLOAD:winhttp.dll
    /DELAYLOAD:winsta.dll
)

if(MSVC)
    target_link_options(simcp PRIVATE /SUBSYSTEM:CONSOLE,10.0)
endif()
