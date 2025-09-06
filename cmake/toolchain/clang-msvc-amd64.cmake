#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

set(CMAKE_C_COMPILER          clang-cl)
set(CMAKE_CXX_COMPILER        clang-cl)
set(CMAKE_LINKER              link)
set(CMAKE_AR                  lib)
set(CMAKE_C_COMPILER_AR       lib)
set(CMAKE_CXX_COMPILER_AR     lib)
set(CMAKE_NINJA_CMCLDEPS_RC   OFF)
set(CMAKE_MC_COMPILER         mc)
set(CMAKE_RC_COMPILER         rc)
set(CMAKE_C_COMPILER_TARGET   "x86_64-pc-windows-msvc")
set(CMAKE_CXX_COMPILER_TARGET "x86_64-pc-windows-msvc")

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/override-msvc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/clang-msvc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/finalize-msvc.cmake)
