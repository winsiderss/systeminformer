#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

project(phnt)

add_library(phnt INTERFACE)

target_include_directories(phnt INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
