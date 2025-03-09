#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

cmake_minimum_required(VERSION 3.30.0 FATAL_ERROR)

project(SystemInformer)

set(SYSTEM_INFORMER_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(commands)

option(SI_WITH_CORE "configure core projects" ON)
option(SI_WITH_PLUGINS "configure plugin projects" OFF)
option(SI_WITH_KERNEL "configure kernel projects" OFF)
option(SI_WITH_PREFAST "enables prefast analysis" OFF)

set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "Supported Configurations" FORCE
)

if(SI_WITH_CORE)
    add_subdirectory(SystemInformer)
    add_subdirectory(phnt)
    add_subdirectory(phlib)
    add_subdirectory(tools)
endif()

if(SI_WITH_PLUGINS)
    add_subdirectory(plugins)
endif()

if(SI_WITH_KERNEL)
    add_subdirectory(KSystemInformer)
endif()

if(SI_WITH_CORE OR SI_WITH_KERNEL)
    add_subdirectory(kphlib)
endif()
