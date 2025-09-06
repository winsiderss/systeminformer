#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

include(commands)

#
# Enables PREfast analysis using a props file (for VS generators).
#
function(_si_target_prefast_props target)
    set(options)
    set(oneValueArgs RULE_SET STACK_SIZE)
    set(multiValueArgs EXCLUDE_PATHS)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    set(_ruleset)
    if(DEFINED arg_RULE_SET)
        if(IS_ABSOLUTE "${arg_RULE_SET}")
            set(_ruleset "${arg_RULE_SET}")
        else()
            set(_ruleset "${CMAKE_CURRENT_SOURCE_DIR}/${arg_RULE_SET}")
        endif()
        cmake_path(NATIVE_PATH _ruleset NORMALIZE _ruleset)
    endif()

    if(DEFINED arg_STACK_SIZE)
        set(_stack_size "stacksize${arg_STACK_SIZE}")
    else()
        set(_stack_size "stacksize1024")
    endif()

    set(_exclude_paths)
    foreach(_arg ${arg_EXCLUDE_PATHS})
        if(IS_ABSOLUTE "${_arg}")
            set(_exclude_path "${_arg}")
        else()
            set(_exclude_path "${CMAKE_CURRENT_SOURCE_DIR}/${_arg}")
        endif()
        cmake_path(NATIVE_PATH _exclude_path NORMALIZE _exclude_path)
        list(APPEND _exclude_paths ${_exclude_path})
    endforeach()

    set(_props_file "${CMAKE_CURRENT_BINARY_DIR}/${target}_prefast.props")

    string(APPEND _props "<Project>\n")
    string(APPEND _props "  <PropertyGroup>\n")
    string(APPEND _props "    <RunCodeAnalysis>true</RunCodeAnalysis>\n")
    string(APPEND _props "    <CodeAnalysisTreatWarningsAsErrors>true</CodeAnalysisTreatWarningsAsErrors>\n")
    if(_ruleset)
    string(APPEND _props "    <CodeAnalysisRuleset>${_ruleset}</CodeAnalysisRuleset>\n")
    endif()
    foreach(_exclude_path ${_exclude_paths})
    string(APPEND _props "    <CAExcludePath>${_exclude_path};$(CAExcludePath)</CAExcludePath>\n")
    endforeach()
    string(APPEND _props "    <PREfastAdditionalOptions>${_stack_size};$(PREfastAdditionalOptions)</PREfastAdditionalOptions>\n")
    string(APPEND _props "  </PropertyGroup>\n")
    string(APPEND _props "</Project>\n")

    file(GENERATE OUTPUT "${_props_file}" CONTENT "${_props}")

    set_target_properties(${target} PROPERTIES VS_USER_PROPS "${_props_file}")
endfunction()

#
# Enables PREfast analysis compiler configuration (for non-VS generators).
#
function(_si_target_prefast_opts target)
    set(options)
    set(oneValueArgs RULE_SET STACK_SIZE)
    set(multiValueArgs EXCLUDE_PATHS)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    #
    # Doing this without the MSBuild system requires looking up some necessary
    # parts to pass to the compiler ourself:
    # 1. The project directory.
    # 2. The directories which contain predefined rulesets.
    # 3. Plugins for analysis.
    #

    cmake_path(NATIVE_PATH CMAKE_CURRENT_SOURCE_DIR NORMALIZE _project_dir)
    string(REGEX REPLACE "[\\\\/]+$" "" _project_dir "${_project_dir}")

    find_path(_rulesets_dir "AllRules.ruleset" PATHS
        ENV "VSINSTALLDIR"
        PATH_SUFFIXES "Team Tools/Static Analysis Tools/Rule Sets"
        REQUIRED
    )
    string(REGEX REPLACE "[\\\\/]+$" "" _rulesets_dir "${_rulesets_dir}")

    #
    # Plugins other than EspXEngine are disabled for now. Only EspXEngine is
    # included by MSBuild for user mode code analysis. Including the others in
    # user mode causes some interesting analysis errors. If this path ever gets
    # used for kernel mode builds this section should be revisited. (jxy-s)
    #
    set(_plugins)
    find_program(_plugin "EspXEngine.dll" PATHS "VC/Tools/MSVC" REQUIRED)
    cmake_path(NATIVE_PATH _plugin NORMALIZE _plugin)
    list(APPEND _plugins ${_plugin})
    #find_program(_plugin "WindowsPrefast.dll" PATHS "Windows Kits/10/bin" REQUIRED)
    #cmake_path(NATIVE_PATH _plugin NORMALIZE _plugin)
    #list(APPEND _plugins ${_plugin})
    #find_program(_plugin "drivers.dll" PATHS "Windows Kits/10/bin" REQUIRED)
    #cmake_path(NATIVE_PATH _plugin NORMALIZE _plugin)
    #list(APPEND _plugins ${_plugin})

    #
    # Handle arguments - N.B. exclude paths will be handled later.
    #

    set(_ruleset)
    if(DEFINED arg_RULE_SET)
        if(IS_ABSOLUTE "${arg_RULE_SET}")
            set(_ruleset "${arg_RULE_SET}")
        else()
            set(_ruleset "${CMAKE_CURRENT_SOURCE_DIR}/${arg_RULE_SET}")
        endif()
        cmake_path(NATIVE_PATH _ruleset NORMALIZE _ruleset)
    endif()

    if(DEFINED arg_STACK_SIZE)
        set(_stack_size "stacksize${arg_STACK_SIZE}")
    else()
        set(_stack_size "stacksize1024")
    endif()

    #
    # Create a response file to bypass CMake's automatic escaping.
    #

    set(_response_file "${CMAKE_CURRENT_BINARY_DIR}/${target}_prefast.rsp")
    string(APPEND _analyze "/analyze\n")
    string(APPEND _analyze "/analyze:projectdirectory\"${_project_dir}\"\n")
    string(APPEND _analyze "/analyze:rulesetdirectory\";${_rulesets_dir};\"\n")
    if(_ruleset)
    string(APPEND _analyze "/analyze:ruleset\"${_ruleset}\"\n")
    endif()
    string(APPEND _analyze "/analyze:${_stack_size}\n")
    foreach(_plugin ${_plugins})
    string(APPEND _analyze "/analyze:plugin\"${_plugin}\"\n")
    endforeach()

    file(GENERATE OUTPUT "${_response_file}" CONTENT "${_analyze}")
    cmake_path(NATIVE_PATH _response_file NORMALIZE _response_file)

    #
    # Configure the compiler for analysis.
    #

    target_compile_definitions(${target} PRIVATE CODE_ANALYSIS)
    target_compile_options(${target} PRIVATE "@${_response_file}")

    #
    # Handle exclude path arguments.
    #
    # The analyzer expects the CAExcludePath environment variable to be set to
    # exclude certain paths from code analysis. So generate a compiler wrapper
    # script that sets the environment variable to exclude files from analysis
    # that should be.
    #

    set(_exclude_paths)
    foreach(_arg ${arg_EXCLUDE_PATHS})
        if(IS_ABSOLUTE "${_arg}")
            set(_exclude_path "${_arg}")
        else()
            set(_exclude_path "${CMAKE_CURRENT_SOURCE_DIR}/${_arg}")
        endif()
        cmake_path(NATIVE_PATH _exclude_path NORMALIZE _exclude_path)
        list(APPEND _exclude_paths ${_exclude_path})
    endforeach()

    if (_exclude_paths)
        string(JOIN ";" _exclude_paths ${_exclude_paths})
        set(_wrapper_file "${CMAKE_CURRENT_BINARY_DIR}/${target}_prefast_wrapper.cmd")

        string(APPEND _wrapper "@echo off\n")
        string(APPEND _wrapper "set \"CAExcludePath=${_exclude_paths}\"\n")
        string(APPEND _wrapper "%*\n")

        file(GENERATE OUTPUT "${_wrapper_file}" CONTENT "${_wrapper}")

        set_target_properties(${target} PROPERTIES
            CXX_COMPILER_LAUNCHER "${_wrapper_file}"
            C_COMPILER_LAUNCHER "${_wrapper_file}"
        )
    endif()
endfunction()

#
# Enables PREfast analysis for a given target.
#
function(si_target_prefast target)
    set(options)
    set(oneValueArgs RULE_SET STACK_SIZE)
    set(multiValueArgs EXCLUDE_PATHS)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(NOT MSVC_NOT_CLANG)
        message(NOTICE "PREfast skipped for ${target} because it is not compatible with Clang.")
        return()
    endif()

    #
    # Append exclude paths that are excluded from PREfast by MSBuild by default.
    # Note that using EXTERNAL_INCLUDE and INCLUDE is a cheat here, but it's
    # close enough. See: Microsoft.CodeAnalysis.Targets CAExcludePath
    #
    set(_exclude_paths ${arg_EXCLUDE_PATHS})
    list(APPEND _exclude_paths $ENV{EXTERNAL_INCLUDE})
    list(APPEND _exclude_paths $ENV{INCLUDE})
    list(REMOVE_DUPLICATES _exclude_paths)

    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        _si_target_prefast_props(${target}
            RULE_SET ${arg_RULE_SET}
            STACK_SIZE ${arg_STACK_SIZE}
            EXCLUDE_PATHS ${_exclude_paths}
        )
    else()
        _si_target_prefast_opts(${target}
            RULE_SET ${arg_RULE_SET}
            STACK_SIZE ${arg_STACK_SIZE}
            EXCLUDE_PATHS ${_exclude_paths}
         )
    endif()
endfunction()
