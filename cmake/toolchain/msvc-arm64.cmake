#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM64)
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_GENERATOR_PLATFORM ARM64)
endif()

set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)
set(CMAKE_LINKER link)
set(CMAKE_MC_COMPILER mc)
set(CMAKE_RC_COMPILER rc)

set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_LIST_DIR}/overrides-msvc.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/msvc.cmake)
