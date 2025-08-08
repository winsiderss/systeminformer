#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(tools)
include(platform)
include(tracewpp)

#
# Helper function for setting up a System Informer target
#
function(_si_set_target_defaults target)
    set(options PLUGIN)
    set(oneValueArgs TYPE)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(NOT arg_TYPE MATCHES "^(UM|KM)(_LIB|_BIN)$")
        message(FATAL_ERROR "Invalid target type: ${arg_TYPE}")
    endif()

    #
    # Specifying /NATVIS on the linker options doesn't work, it gets omitted by
    # the generators. Inject the dependency into the sources instead, which is
    # supported by cmake.
    #
    source_group("Resource Files" FILES ${SI_ROOT}/SystemInformer.natvis)
    target_sources(${target} PRIVATE ${SI_ROOT}/SystemInformer.natvis)

    if(NOT SI_OUTPUT_DIR STREQUAL "" AND NOT SI_OUTPUT_DIR STREQUAL "OFF")
        if(arg_PLUGIN)
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}/plugins"
                PDB_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}/plugins"
            )
        else()
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}"
                PDB_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}"
            )
        endif()
    endif()

    get_target_property(_target_sources ${target} SOURCES)
    if(NOT _target_sources)
        set(_target_sources "")
    endif()

    if(arg_TYPE STREQUAL "UM_LIB")
        target_compile_options(${target} PRIVATE /Gz) # __stcall calling convention
        if(SI_WITH_WPP_USER)
            si_target_tracewpp(${target} WPP_USER_MODE
                WPP_EXT ".c.cpp.h.hpp" WPP_PRESERVE_EXT
                WPP_SCAN "${CMAKE_SOURCE_DIR}/phlib/include/trace.h"
                ${_target_sources}
            )
            target_link_libraries(${target} PRIVATE advapi32)
        else()
            target_compile_definitions(${target} PRIVATE SI_NO_WPP)
        endif()
    elseif(arg_TYPE STREQUAL "UM_BIN")
        target_compile_options(${target} PRIVATE /Gz) # __stcall calling convention
        if(SI_WITH_WPP_USER)
            si_target_tracewpp(${target} WPP_USER_MODE
                WPP_EXT ".c.cpp.h.hpp" WPP_PRESERVE_EXT
                WPP_SCAN "${CMAKE_SOURCE_DIR}/phlib/include/trace.h"
                ${_target_sources}
            )
            target_link_libraries(${target} PRIVATE advapi32)
        else()
            target_compile_definitions(${target} PRIVATE SI_NO_WPP)
        endif()
        si_kphsign(${target})
    elseif(arg_TYPE STREQUAL "KM_LIB")
        if(SI_WITH_WPP_KERNEL)
            si_target_tracewpp(${target} WPP_KERNEL_MODE
                WPP_EXT ".c.cpp.h.hpp" WPP_PRESERVE_EXT
                WPP_SCAN "${CMAKE_SOURCE_DIR}/KSystemInformer/include/trace.h"
                ${_target_sources}
            )
        else()
            target_compile_definitions(${target} PRIVATE KSI_NO_WPP)
        endif()
    elseif(arg_TYPE STREQUAL "KM_BIN")
        set_target_properties(${target} PROPERTIES SUFFIX ".sys")
        if(SI_WITH_WPP_KERNEL)
            si_target_tracewpp(${target} WPP_KERNEL_MODE
                WPP_EXT ".c.cpp.h.hpp" WPP_PRESERVE_EXT
                WPP_SCAN "${CMAKE_SOURCE_DIR}/KSystemInformer/include/trace.h"
                ${_target_sources}
            )
        else()
            target_compile_definitions(${target} PRIVATE KSI_NO_WPP)
        endif()
    endif()

    if (arg_PLUGIN)
        target_link_libraries(${target} PRIVATE SystemInformer)
        target_include_directories(${target} PRIVATE "${SI_ROOT}/plugins/include")
    endif()

    if(SI_WITH_PREFAST)
        # TODO configure this with the additional analysis options and ruleset
        # once the kph-staging branch is merged into master.
        target_compile_options(${target} PRIVATE /analyze)
    endif()
endfunction()

#
# add_library for user mode System Informer libraries
#
function(si_add_library target)
    add_library(${target} ${ARGN})
    _si_set_target_defaults(${target} TYPE UM_LIB)
endfunction()

#
# add_executable for user mode System Informer executables
#
function(si_add_executable target)
    add_executable(${target} ${ARGN})
    _si_set_target_defaults(${target} TYPE UM_BIN)
endfunction()

#
# add_library for System Informer plugins
#
function(si_add_plugin target)
    add_library(${target} SHARED ${ARGN})
    _si_set_target_defaults(${target} TYPE UM_LIB PLUGIN)
endfunction()

#
# add_library for kernel mode System Informer libraries
#
function(si_add_kernel_library target)
    add_library(${target} ${ARGN})
    _si_set_target_defaults(${target} TYPE KM_LIB)
endfunction()

#
# add_executable for kernel mode System Informer drivers
#
function(si_add_kernel_driver target)
    add_executable(${target} ${ARGN})
    _si_set_target_defaults(${target} TYPE KM_BIN)
endfunction()
