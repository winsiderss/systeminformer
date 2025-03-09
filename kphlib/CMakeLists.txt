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
    "include/sistatus.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "include/sistatus_MSG00001.bin"
    "kphdyn.xml"
    "sistatus.mc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "kphdyn.c"
    "kphdyndata.c"
    "kphmsg.c"
    "kphmsgdyn.c"
)
source_group("Source Files" FILES SOURCES)

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)

si_add_library(kphlib_um STATIC ${ALL_FILES})

target_include_directories(kphlib_um PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(kphlib_um PRIVATE
    phnt
)

if(SI_WITH_KERNEL)
    si_add_kernel_library(kphlib_um STATIC ${HEADERS} ${RESOURCES} ${SOURCES})

    target_include_directories(kphlib_km PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
endif()
