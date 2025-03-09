#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(defaults)
include(tools)

function(si_set_target_defaults target)
    if (MSVC)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            set_target_properties(${target} PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreadedDebug"
                MSVC_DEBUG_INFORMATION_FORMAT "EditAndContinue"
            )
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            set_target_properties(${target} PROPERTIES
                MSVC_RUNTIME_LIBRARY "MultiThreadedDebug"
                MSVC_DEBUG_INFORMATION_FORMAT "ProgramDatabase"
            )
        endif()
    endif()
    target_compile_features(${target} PRIVATE
        c_std_17   # CMake needs to add MSVC support for C23
        cxx_std_23
    )
endfunction()

#
# add_library for user mode System Informer libraries
#
function(si_add_library target)
    add_library(${target} ${ARGN})
    si_set_target_defaults(${target})
    target_compile_options(${target} PRIVATE ${SI_COMPILE_FLAGS_USER})
    target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFS_USER})
endfunction()

#
# add_executable for user mode System Informer executables
#
function(si_add_executable target)
    add_executable(${target} ${ARGN})
    si_set_target_defaults(${target})
    target_compile_options(${target} PRIVATE ${SI_COMPILE_FLAGS_USER})
    target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFS_USER})
    target_link_options(${target} PRIVATE ${SI_LINK_FLAGS_USER})
endfunction()

#
# add_library for System Informer plugins
#
function(si_add_plugin target)
    add_library(${target} SHARED ${ARGN})
    si_set_target_defaults(${target})
    target_compile_options(${target} PRIVATE ${SI_COMPILE_FLAGS_USER})
    target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFS_USER})
    target_link_options(${target} PRIVATE ${SI_LINK_FLAGS_USER})
    target_link_libraries(${target} PRIVATE SystemInformer)
    target_include_directories(${target} PRIVATE "${SYSTEM_INFORMER_ROOT}/plugins/include")
    si_kphsign(${target})
endfunction()

#
# add_library for kernel mode System Informer libraries
#
function(si_add_kernel_library target)
    add_library(${target} ${ARGN})
    si_set_target_defaults(${target})
    target_compile_options(${target} PRIVATE ${SI_COMPILE_OPTIONS_KERNEL})
    target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFINITIONS_KERNEL})
endfunction()

#
# add_executable for kernel mode System Informer drivers
#
function(si_add_kernel_driver target)
    add_executable(${target} ${ARGN})
    si_clear_target_defaults(${target})
    set_target_properties(${target} PROPERTIES SUFFIX ".sys")
    target_compile_options(${target} PRIVATE ${SI_COMPILE_OPTIONS_KERNEL})
    target_compile_definitions(${target} PRIVATE ${SI_COMPILE_DEFINITIONS_KERNEL})
    target_link_options(${target} PRIVATE ${SI_LINK_OPTIONS_KERNEL})
endfunction()