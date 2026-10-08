#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(tools)
include(platform)
include(tracewpp)

set(SI_UM_CLANG_NO_DIAGNOSTICS
    -Wno-c23-extensions
    -Wno-incompatible-pointer-types
    -Wno-missing-braces
    # With CONST_VTABLE defined, DECLARE_INTERFACE_ expands to
    # 'const struct IFooVtbl { ... };' - a const with no declarator. MSVC accepts
    # it silently; clang reports 'const ignored on this declaration'.
    -Wno-missing-declarations
    -Wno-parentheses
    -Wno-pointer-sign
    -Wno-switch
    -Wno-unused-but-set-variable
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-visibility
    -Wno-defaulted-function-deleted
    -Wno-class-conversion
    -Wno-microsoft-explicit-constructor-call
    -Wno-nontrivial-memaccess
)

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

    if(MSVC)
        target_link_options(${target} PRIVATE /NATVIS:${SI_ROOT}/SystemInformer.natvis)
    endif()

    if(MSVC_NOT_CLANG)
        target_compile_options(${target} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:/std:clatest>
            $<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>
            $<$<COMPILE_LANGUAGE:C,CXX>:/Zc:preprocessor>
            $<$<COMPILE_LANGUAGE:C,CXX>:/permissive->
            $<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>
        )
    endif()

    if(NOT SI_OUTPUT_DIR STREQUAL "" AND NOT SI_OUTPUT_DIR STREQUAL "OFF")
        if(arg_PLUGIN)
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}/plugins"
                ARCHIVE_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}/plugins"
                PDB_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}/plugins"
            )
        else()
            set_target_properties(${target} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}"
                ARCHIVE_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}"
                PDB_OUTPUT_DIRECTORY "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}"
            )
        endif()
    endif()

    get_target_property(_target_sources ${target} SOURCES)
    if(NOT _target_sources)
        set(_target_sources "")
    endif()

    #
    # User-mode preprocessor definitions. These mirror the shared definitions in
    # Common.User.props, which is canonical for MSBuild; keep the two in sync.
    # CONST_VTABLE makes MIDL-generated C interfaces declare lpVtbl as a pointer
    # to const, as required by the const vtables in phlib/webview.
    #
    # thirdparty.vcxproj inherits Common.User.props through tools/Directory.Build.props
    # and preserves its definitions alongside the project-specific architecture macros.
    # Apply the shared definitions to thirdparty here as well. The user-mode clang
    # options above suppress the CONST_VTABLE-related SDK declaration warning.
    if(arg_TYPE MATCHES "^UM_")
        if(SI_PLATFORM STREQUAL "Win32")
            set(_si_um_bitness WIN32)
        else()
            set(_si_um_bitness WIN64)
        endif()
        target_compile_definitions(${target} PRIVATE
            ${_si_um_bitness}
            _WINDOWS
            _USRDLL
            ENABLE_RESTRICTED
            CONST_VTABLE
            $<$<CONFIG:Debug>:DEBUG>
            $<$<CONFIG:Debug>:_DEBUG>
            $<$<CONFIG:Debug>:_DBG_MEMCPY_INLINE_>
            $<$<CONFIG:Release>:NDEBUG>
        )
        unset(_si_um_bitness)
    endif()

    if(arg_TYPE STREQUAL "UM_LIB")
        if(MSVC)
            target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/Gz>) # __stdcall calling convention
            target_link_options(${target} PRIVATE /SUBSYSTEM:WINDOWS,6.1)
        else()
            target_link_options(${target} PRIVATE -mwindows)
        endif()
        if(MSVC_CLANG)
            foreach(noDiagnostic IN LISTS SI_UM_CLANG_NO_DIAGNOSTICS)
                target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:${noDiagnostic}>)
            endforeach()
        endif()
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
        if(MSVC)
            target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/Gz>) # __stdcall calling convention
            target_link_options(${target} PRIVATE /SUBSYSTEM:WINDOWS,6.1)
        else()
            target_link_options(${target} PRIVATE -mwindows)
        endif()
        if(MSVC_CLANG)
            foreach(noDiagnostic IN LISTS SI_UM_CLANG_NO_DIAGNOSTICS)
                target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:${noDiagnostic}>)
            endforeach()
        endif()
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
        target_link_libraries(${target} PRIVATE SystemInformer thirdparty ntdll)
        target_include_directories(${target} PRIVATE
            "${SI_ROOT}/plugins/include"
            "${SI_ROOT}/phnt/include"
            "${SI_ROOT}/sdk/include/$<CONFIG>${SI_PLATFORM_SHORT}"
            "${SI_ROOT}/kphlib/include"
        )
        target_link_directories(${target} PRIVATE
            "${SI_ROOT}/sdk/lib/$<CONFIG>${SI_PLATFORM_SHORT}"
            "${SI_OUTPUT_DIR}/$<CONFIG>${SI_PLATFORM_SHORT}"
        )
    endif()

    if(SI_WITH_PREFAST AND MSVC)
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
