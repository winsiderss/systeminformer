#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

project(kphlib)

set(HEADERS
    "include/kphapi.h"
    "include/kphdyn.h"
    "include/kphdyndata.h"
    "include/kphlibbase.h"
    "include/kphmsg.h"
    "include/kphmsgdefs.h"
    "include/kphmsgdyn.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "kphdyn.xml"
    "sistatus.mc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "kphdyn.c"
    "kphdyndata.c"
    "kphmsg.c"
    "kphmsgdyn.c"
    "kphringbuff.c"
)
source_group("Source Files" FILES SOURCES)

#
# sistatus message compilation.
#
# sistatus.h is generated from sistatus.mc by the Windows SDK message compiler
# (mc.exe). This mirrors the CustomBuild step in kphlib_um.vcxproj: the kernel
# side uses the WDK MessageCompile task; the user-mode side uses the SDK mc.exe
# directly (no WDK requirement). The output is placed in a per-config,
# per-platform directory to match the $(Configuration)$(PlatformArchitecture)
# layout used by the MSBuild projects.
#
find_program(SI_MESSAGE_COMPILER
    NAMES mc
    PATHS ENV "WindowsSdkVerBinPath"
    PATH_SUFFIXES x64 x86 arm64
)
if(NOT SI_MESSAGE_COMPILER)
    message(FATAL_ERROR "mc.exe (message compiler) not found in the Windows SDK.")
endif()

set(SI_SISTATUS_DIR "${CMAKE_CURRENT_BINARY_DIR}/sistatus/$<CONFIG>${SI_PLATFORM_SHORT}")
set(SI_SISTATUS_HEADER "${SI_SISTATUS_DIR}/sistatus.h")

add_custom_command(
    OUTPUT
        "${SI_SISTATUS_HEADER}"
        "${SI_SISTATUS_DIR}/sistatus.rc"
        "${SI_SISTATUS_DIR}/sistatus_MSG00001.bin"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SI_SISTATUS_DIR}"
    COMMAND "${SI_MESSAGE_COMPILER}" -c -u -U -n -b
            -h "${SI_SISTATUS_DIR}" -r "${SI_SISTATUS_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/sistatus.mc"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/sistatus.mc"
    COMMENT "Compiling messages sistatus.mc"
    VERBATIM
)

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
    "${SI_SISTATUS_HEADER}"
)

si_add_library(kphlib_um STATIC ${ALL_FILES})

target_include_directories(kphlib_um PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/$<CONFIG>${SI_PLATFORM_SHORT}"
    "${SI_SISTATUS_DIR}"
)

target_link_libraries(kphlib_um PRIVATE
    phnt
)
