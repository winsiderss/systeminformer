#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(defaults)
include(tools)

function(si_set_target_defaults target)
    set(options PLUGIN)
    set(oneValueArgs TYPE)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(NOT arg_TYPE MATCHES "^(UM|KM)(_LIB|_BIN)$")
        message(FATAL_ERROR "Invalid target type: ${arg_TYPE}")
    endif()

    target_compile_features(${target} PRIVATE
        c_std_17   # CMake needs to add MSVC support for C23
        cxx_std_23
    )

    #
    # Specifying /NATVIS on the linker options doesn't work, it gets omitted by
    # the generators. Inject the dependency into the sources instead, which is
    # supported by cmake.
    #
    source_group("Resource Files" FILES ${SI_ROOT}/SystemInformer.natvis)
    target_sources(${target} PRIVATE ${SI_ROOT}/SystemInformer.natvis)

    set_target_properties(${target} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug>:EditAndContinue>$<$<NOT:$<CONFIG:Debug>>:ProgramDatabase>"
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    )

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

    if(arg_TYPE STREQUAL "UM_LIB")
        target_compile_options(${target} PRIVATE ${SI_COMPILE_FLAGS_USER})
        target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFS_USER})
    elseif(arg_TYPE STREQUAL "UM_BIN")
        target_compile_options(${target} PRIVATE ${SI_COMPILE_FLAGS_USER})
        target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFS_USER})
        target_link_options(${target} PRIVATE ${SI_LINK_FLAGS_USER})
        si_kphsign(${target})
    elseif(arg_TYPE STREQUAL "KM_LIB")
        target_compile_options(${target} PRIVATE ${SI_COMPILE_FLAGS_KERNEL})
        target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFS_KERNEL})
    elseif(arg_TYPE STREQUAL "KM_BIN")
        target_compile_options(${target} PRIVATE ${SI_COMPILE_FLAGS_KERNEL})
        target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFS_KERNEL})
        target_link_options(${target} PRIVATE ${SI_LINK_FLAGS_KERNEL})
        set_target_properties(${target} PROPERTIES SUFFIX ".sys")
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
    si_set_target_defaults(${target} TYPE UM_LIB)
endfunction()

#
# add_executable for user mode System Informer executables
#
function(si_add_executable target)
    add_executable(${target} ${ARGN})
    si_set_target_defaults(${target} TYPE UM_BIN)
endfunction()

#
# add_library for System Informer plugins
#
function(si_add_plugin target)
    add_library(${target} SHARED ${ARGN})
    si_set_target_defaults(${target} TYPE UM_LIB PLUGIN)
endfunction()

#
# add_library for kernel mode System Informer libraries
#
function(si_add_kernel_library target)
    add_library(${target} ${ARGN})
    si_set_target_defaults(${target} TYPE KM_LIB)
endfunction()

#
# add_executable for kernel mode System Informer drivers
#
function(si_add_kernel_driver target)
    add_executable(${target} ${ARGN})
    si_set_target_defaults(${target} TYPE KM_BIN)
endfunction()
