#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

#
# Select the host-architecture build of CustomBuildTool. Do not use
# $ENV{PROCESSOR_ARCHITECTURE} here: it reports the architecture of the running
# process, so a 32-bit cmake.exe (the one Visual Studio ships under
# Program Files (x86)) resolves to "x86" on an AMD64 host and picks the wrong
# build. CMAKE_HOST_SYSTEM_PROCESSOR always reports the real host.
#
string(TOLOWER "${CMAKE_HOST_SYSTEM_PROCESSOR}" _si_host_arch)
set(SI_CUSTOM_BUILD_TOOL "${SI_ROOT}/tools/CustomBuildTool/bin/Release/${_si_host_arch}/CustomBuildTool.exe")
unset(_si_host_arch)

if (NOT EXISTS "${SI_CUSTOM_BUILD_TOOL}")
    message(FATAL_ERROR "CustomBuildTool.exe not found. Run build\\build_tools.cmd first.")
endif()

function(si_sdkbuild target)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${SI_CUSTOM_BUILD_TOOL}" -sdk -$<CONFIG> -${SI_PLATFORM} -cmake
        WORKING_DIRECTORY "${SI_ROOT}"
        VERBATIM
    )
endfunction()

function(si_kphsign target)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${SI_CUSTOM_BUILD_TOOL}" -kphsign "$<SHELL_PATH:$<TARGET_FILE:${target}>>"
        WORKING_DIRECTORY "${SI_ROOT}"
        VERBATIM
    )
endfunction()
