#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#
# clang-cl + LLVM lld-link. Intended for DEBUG / local iteration only. See
# clang-lld.cmake for the rationale and the list of dropped mitigations.
#

set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM64)
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_PLATFORM ARM64)
endif()

set(CMAKE_C_COMPILER          clang-cl)
set(CMAKE_CXX_COMPILER        clang-cl)
set(CMAKE_NINJA_CMCLDEPS_RC   OFF)
set(CMAKE_MC_COMPILER         mc)
# llvm-rc cannot concatenate adjacent string literals in a resource VALUE, which
# PHAPP_VERSION_STRING (include/phappres.h) relies on, so rc.exe is required.
set(CMAKE_RC_COMPILER         rc)
set(CMAKE_C_COMPILER_TARGET   "arm64-pc-windows-msvc")
set(CMAKE_CXX_COMPILER_TARGET "arm64-pc-windows-msvc")

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/override-msvc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/clang-lld.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/finalize-msvc.cmake)
