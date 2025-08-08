#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

#
# Helper function for adding WPP trace custom commands.
#
function(_si_tracewpp_command tmh_files src_files)
    set(options WPP_KERNEL_MODE WPP_USER_MODE WPP_PRESERVE_EXT)
    set(oneValueArgs WPP_EXT WPP_SCAN WPP_CONFIG_DIR TMH_OUT_DIR)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 2 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(arg_WPP_KERNEL_MODE)
        set(_mode "-km")
    elseif(arg_WPP_USER_MODE)
        set(_mode "-um")
    else()
        message(FATAL_ERROR "WPP_KERNEL_MODE or WPP_USER_MODE must be provided for wpptrace generation.")
    endif()

    if(arg_WPP_EXT)
        set(_ext "-ext:${arg_WPP_EXT}")
    else()
        message(FATAL_ERROR "WPP_EXT must be provided for wpptrace generation.")
    endif()

    if(arg_WPP_PRESERVE_EXT)
        set(_preserveext "-preserveext:${arg_WPP_EXT}")
    else()
        set(_preserveext "")
    endif()

    if(arg_WPP_SCAN)
        cmake_path(NATIVE_PATH arg_WPP_SCAN NORMALIZE _scan_file)
        set(_scan "-scan:\"${_scan_file}\"")
    else()
        set(_scan "")
    endif()

    if(arg_WPP_CONFIG_DIR)
        cmake_path(NATIVE_PATH arg_WPP_CONFIG_DIR NORMALIZE _cfg_dir)
        set(_cfgdir "-cfgdir:\"${_cfg_dir}\"")
    else()
        set(_cfgdir "")
    endif()

    if(arg_TMH_OUT_DIR)
        cmake_path(NATIVE_PATH arg_TMH_OUT_DIR NORMALIZE _out_dir)
    else()
        message(FATAL_ERROR "TMH_OUT_DIR must be provided for wpptrace generation.")
    endif()

    set(_tmh_files)
    set(_src_files)
    foreach(_source_file ${arg_UNPARSED_ARGUMENTS})
        #
        # Check that the source file has a relevant extension.
        #
        cmake_path(GET _source_file EXTENSION _file_ext)
        string(REPLACE "." "\\." _file_ext_match ${_file_ext})
        if(NOT ${arg_WPP_EXT} MATCHES ${_file_ext_match})
            continue()
        endif()

        cmake_path(GET _source_file FILENAME _file_name)
        if(NOT arg_WPP_PRESERVE_EXT)
            cmake_path(REMOVE_EXTENSION _file_name)
        endif()

        if(IS_ABSOLUTE "${_source_file}")
            set(_source_file_abs "${_source_file}")
        else()
            set(_source_file_abs "${CMAKE_CURRENT_SOURCE_DIR}/${_source_file}")
        endif()
        cmake_path(NATIVE_PATH _source_file_abs NORMALIZE _source_file_abs)

        #
        # Build the output directory. To avoid collisions the output directory
        # is appended with the relative path from the target's current source
        # directory. Also sanitize the path for insertion into the tracewpp
        # outdir parameter.
        #
        cmake_path(GET _source_file_abs PARENT_PATH _tmf_out_dir)
        cmake_path(RELATIVE_PATH _tmf_out_dir OUTPUT_VARIABLE _tmf_out_dir)
        cmake_path(APPEND _out_dir "${_tmf_out_dir}" OUTPUT_VARIABLE _tmf_out_dir)
        cmake_path(NATIVE_PATH _tmf_out_dir NORMALIZE _tmf_out_dir)
        file(MAKE_DIRECTORY "${_tmf_out_dir}")
        string(REGEX REPLACE "[\\\\/]+$" "" _tmf_out_dir "${_tmf_out_dir}")
        set(_odir "-odir:\"${_tmf_out_dir}\"")

        cmake_path(
            APPEND _tmf_out_dir "${_file_name}.tmh"
            OUTPUT_VARIABLE _tmh_file
        )
        cmake_path(
            RELATIVE_PATH _tmh_file
            BASE_DIRECTORY ${CMAKE_BINARY_DIR}
            OUTPUT_VARIABLE _tmh_comment
        )
        cmake_path(NATIVE_PATH _tmh_file NORMALIZE _tmh_file)

        add_custom_command(
            OUTPUT ${_tmh_file}
            COMMAND tracewpp ${_mode}
                             ${_ext}
                             ${_preserveext}
                             ${_cfgdir}
                             ${_odir}
                             ${_scan}
                             "\"${_source_file_abs}\""
            DEPENDS "${_source_file_abs}" "${_scan_file}"
            COMMENT "Generating WPP trace header ${_tmh_comment}"
        )

        list(APPEND _tmh_files ${_tmh_file})
        list(APPEND _src_files ${_source_file})
    endforeach()

    set(${tmh_files} ${_tmh_files} PARENT_SCOPE)
    set(${src_files} ${_src_files} PARENT_SCOPE)
endfunction()

#
# Configures a target for WPP trace generation.
#
function(si_target_tracewpp target)
    set(options WPP_KERNEL_MODE WPP_USER_MODE WPP_PRESERVE_EXT)
    set(oneValueArgs WPP_EXT WPP_SCAN)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")

    if(arg_WPP_KERNEL_MODE)
        set(_mode WPP_KERNEL_MODE)
    else()
        set(_mode WPP_USER_MODE)
    endif()

    set(_preserve_ext)
    if(arg_WPP_PRESERVE_EXT)
        set(_preserve_ext WPP_PRESERVE_EXT)
    endif()

    #
    # Try to locate the WPP config directory.
    #
    set(_config_dir "$ENV{WDKBinRoot}/WppConfig/Rev1")
    if(NOT EXISTS "${_config_dir}")
        set(_config_dir "$ENV{WindowsSdkVerBinPath}/WppConfig/Rev1")
        if(NOT EXISTS "${_config_dir}")
            message(FATAL_ERROR "WPP config directory not found.")
        endif()
    endif()

    get_target_property(_target_binary_dir ${target} BINARY_DIR)
    set(_tmh_out_dir "${_target_binary_dir}/tmh")

    _si_tracewpp_command(_tmh_files _src_files
        ${_mode} ${_preserve_ext}
        WPP_EXT ${arg_WPP_EXT}
        WPP_SCAN ${arg_WPP_SCAN}
        WPP_CONFIG_DIR ${_config_dir}
        TMH_OUT_DIR ${_tmh_out_dir}
        ${arg_UNPARSED_ARGUMENTS}
    )

    #
    # N.B. _si_tracewpp_command guarantees that _tmh_files to _src_files
    # have the same number of elements. The work in this block is setting the
    # required TMH_FILE compile definition for the individual source files.
    # Along the way, gather the include directories to be added to the target.
    #
    set(_include_dirs)
    list(LENGTH _tmh_files _list_length)
    if(_list_length GREATER 0)
        math(EXPR _max_index "${_list_length} - 1")
        foreach(_index RANGE ${_max_index})
            list(GET _src_files ${_index} _src_file)
            list(GET _tmh_files ${_index} _tmh_file)
            cmake_path(GET _tmh_file PARENT_PATH _tmh_dir)
            list(APPEND _include_dirs "${_tmh_dir}")
            cmake_path(GET _tmh_file FILENAME _tmh_file)
            set_source_files_properties(${_src_file} PROPERTIES
                TARGET_DIRECTORY ${target}
                COMPILE_DEFINITIONS "TMH_FILE=${_tmh_file}")
        endforeach()
    endif()

    #
    # This custom target associates the custom commands per source file with
    # actual target. Here we also set the include paths.
    #
    list(REMOVE_DUPLICATES _include_dirs)
    if(_include_dirs)
        add_custom_target(${target}_tracewpp
            DEPENDS ${_tmh_files}
            COMMENT "Generating WPP trace headers for ${target}"
        )
        add_dependencies(${target} ${target}_tracewpp)
        target_include_directories(${target} PRIVATE ${_include_dirs})
    endif()
endfunction()
