#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

cmake_minimum_required(VERSION 3.30.0 FATAL_ERROR)

project(SystemInformer)

set(SI_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(commands)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

option(SI_WITH_CORE "configure core projects" ON)
option(SI_WITH_PLUGINS "configure plugin projects" OFF)
option(SI_WITH_KERNEL "configure kernel projects" OFF)
option(SI_WITH_PREFAST "enables prefast analysis" OFF)
option(SI_WITH_WPP_USER "enables user mode WPP trace" OFF)
option(SI_WITH_WPP_KERNEL "enables kernel mode WPP trace" ON)
# TODO change this to bin once we fully transition to cmake
# set up to allow user to disable with "" or "OFF"
set(SI_OUTPUT_DIR "${SI_ROOT}/bin-cmake" CACHE PATH "output directory")

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
    message(FATAL_ERROR "Kernel mode projects are not supported")
    add_subdirectory(KSystemInformer)
endif()

if(SI_WITH_CORE OR SI_WITH_KERNEL)
    add_subdirectory(kphlib)
endif()
