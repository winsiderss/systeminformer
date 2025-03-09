#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(SI_CUSTOM_BUILD_TOOL "${SI_ROOT}/tools/CustomBuildTool/bin/Release/$ENV{PROCESSOR_ARCHITECTURE}/CustomBuildTool.exe")

if (NOT EXISTS "${SI_CUSTOM_BUILD_TOOL}")
    message(FATAL_ERROR "CustomBuildTool.exe not found. Run build\\build_init.cmd first.")
endif()

function(si_sdkbuild target)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${SI_CUSTOM_BUILD_TOOL}" -sdk -$<CONFIG> -${SI_PLATFORM}
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
