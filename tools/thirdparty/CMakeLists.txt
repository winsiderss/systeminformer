cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(thirdparty C CXX)

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()
message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86"))
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)

################################################################################
# Global compiler options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_C_FLAGS "")
    set(CMAKE_C_FLAGS_DEBUG "/MTd")
    set(CMAKE_C_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
endif()

################################################################################
# Global linker options
################################################################################
if(MSVC)
    # remove default flags provided with CMake for MSVC
    set(CMAKE_EXE_LINKER_FLAGS "")
    set(CMAKE_MODULE_LINKER_FLAGS "")
    set(CMAKE_SHARED_LINKER_FLAGS "")
    set(CMAKE_STATIC_LINKER_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

################################################################################
# Nuget packages function stub.
################################################################################
function(use_package TARGET PACKAGE VERSION)
    message(WARNING "No implementation of use_package. Create yours. "
                    "Package \"${PACKAGE}\" with version \"${VERSION}\" "
                    "for target \"${TARGET}\" is ignored!")
endfunction()

################################################################################
# Common utils
################################################################################
include(CMake/Utils.cmake)

################################################################################
# Additional Global Settings(add specific info there)
################################################################################
include(CMake/GlobalSettingsInclude.cmake OPTIONAL)

################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################
# Sub-projects
################################################################################

set(PROJECT_NAME thirdparty)

################################################################################
# Source groups
################################################################################
set(Json-C
    "jsonc/arraylist.c"
    "jsonc/debug.c"
    "jsonc/json_c_version.c"
    "jsonc/json_object.c"
    "jsonc/json_object_iterator.c"
    "jsonc/json_pointer.c"
    "jsonc/json_tokener.c"
    "jsonc/json_util.c"
    "jsonc/json_visit.c"
    "jsonc/libjson.c"
    "jsonc/linkhash.c"
    "jsonc/printbuf.c"
    "jsonc/random_seed.c"
    "jsonc/strerror_override.c"
)
source_group("Json-C" FILES ${Json-C})

set(Json-C__Headers
    "jsonc/arraylist.h"
    "jsonc/config.h"
    "jsonc/debug.h"
    "jsonc/json.h"
    "jsonc/json_c_version.h"
    "jsonc/json_config.h"
    "jsonc/json_inttypes.h"
    "jsonc/json_object.h"
    "jsonc/json_object_iterator.h"
    "jsonc/json_object_private.h"
    "jsonc/json_pointer.h"
    "jsonc/json_pointer_private.h"
    "jsonc/json_tokener.h"
    "jsonc/json_types.h"
    "jsonc/json_util.h"
    "jsonc/json_visit.h"
    "jsonc/linkhash.h"
    "jsonc/math_compat.h"
    "jsonc/printbuf.h"
    "jsonc/random_seed.h"
    "jsonc/snprintf_compat.h"
    "jsonc/strdup_compat.h"
    "jsonc/strerror_override.h"
    "jsonc/vasprintf_compat.h"
)
source_group("Json-C\\Headers" FILES ${Json-C__Headers})

set(Mini-XML
    "mxml/mxml-attr.c"
    "mxml/mxml-file.c"
    "mxml/mxml-get.c"
    "mxml/mxml-index.c"
    "mxml/mxml-node.c"
    "mxml/mxml-options.c"
    "mxml/mxml-private.c"
    "mxml/mxml-search.c"
    "mxml/mxml-set.c"
)
source_group("Mini-XML" FILES ${Mini-XML})

set(Mini-XML__Headers
    "mxml/config.h"
    "mxml/mxml-private.h"
    "mxml/mxml.h"
)
source_group("Mini-XML\\Headers" FILES ${Mini-XML__Headers})

set(PCRE
    "pcre/pcre2_auto_possess.c"
    "pcre/pcre2_chartables.c"
    "pcre/pcre2_compile.c"
    "pcre/pcre2_config.c"
    "pcre/pcre2_context.c"
    "pcre/pcre2_convert.c"
    "pcre/pcre2_dfa_match.c"
    "pcre/pcre2_error.c"
    "pcre/pcre2_extuni.c"
    "pcre/pcre2_find_bracket.c"
    "pcre/pcre2_jit_compile.c"
    "pcre/pcre2_maketables.c"
    "pcre/pcre2_match.c"
    "pcre/pcre2_match_data.c"
    "pcre/pcre2_newline.c"
    "pcre/pcre2_ord2utf.c"
    "pcre/pcre2_pattern_info.c"
    "pcre/pcre2_script_run.c"
    "pcre/pcre2_serialize.c"
    "pcre/pcre2_string_utils.c"
    "pcre/pcre2_study.c"
    "pcre/pcre2_substitute.c"
    "pcre/pcre2_substring.c"
    "pcre/pcre2_tables.c"
    "pcre/pcre2_ucd.c"
    "pcre/pcre2_valid_utf.c"
    "pcre/pcre2_xclass.c"
    "pcre/pcre2posix.c"
)
source_group("PCRE" FILES ${PCRE})

set(PCRE__Headers
    "pcre/config.h"
    "pcre/pcre2.h"
    "pcre/pcre2_internal.h"
    "pcre/pcre2_intmodedep.h"
    "pcre/pcre2_ucp.h"
    "pcre/pcre2posix.h"
)
source_group("PCRE\\Headers" FILES ${PCRE__Headers})

set(detours
    "detours/detours.cpp"
    "detours/detours.h"
    "detours/disasm.cpp"
    "detours/disolarm.cpp"
    "detours/disolarm64.cpp"
    "detours/disolia64.cpp"
    "detours/disolx64.cpp"
    "detours/disolx86.cpp"
    "detours/modules.cpp"
)
source_group("detours" FILES ${detours})

set(maxminddb
    "maxminddb/data-pool.c"
    "maxminddb/data-pool.h"
    "maxminddb/maxminddb-compat-util.h"
    "maxminddb/maxminddb.c"
    "maxminddb/maxminddb.h"
    "maxminddb/maxminddb_config.h"
)
source_group("maxminddb" FILES ${maxminddb})

set(md5
    "md5/md5.c"
    "md5/md5.h"
)
source_group("md5" FILES ${md5})

set(miniz
    "miniz/miniz.c"
    "miniz/miniz.h"
)
source_group("miniz" FILES ${miniz})

set(sha
    "sha/sha.c"
    "sha/sha.h"
)
source_group("sha" FILES ${sha})

set(sha256
    "sha256/sha256.c"
    "sha256/sha256.h"
)
source_group("sha256" FILES ${sha256})

set(ssdeep
    "ssdeep/fuzzy.c"
    "ssdeep/fuzzy.h"
)
source_group("ssdeep" FILES ${ssdeep})

set(tlsh
    "tlsh/tlsh.cpp"
    "tlsh/tlsh.h"
    "tlsh/tlsh_impl.cpp"
    "tlsh/tlsh_impl.h"
    "tlsh/tlsh_util.cpp"
    "tlsh/tlsh_util.h"
    "tlsh/tlsh_win_version.h"
    "tlsh/tlsh_wrapper.cpp"
    "tlsh/tlsh_wrapper.h"
)
source_group("tlsh" FILES ${tlsh})

set(xxhash
    "xxhash/xxhash.h"
    "xxhash/xxhashwrapper.c"
    "xxhash/xxhashwrapper.h"
)
source_group("xxhash" FILES ${xxhash})

set(ALL_FILES
    ${Json-C}
    ${Json-C__Headers}
    ${Mini-XML}
    ${Mini-XML__Headers}
    ${PCRE}
    ${PCRE__Headers}
    ${detours}
    ${maxminddb}
    ${md5}
    ${miniz}
    ${sha}
    ${sha256}
    ${ssdeep}
    ${tlsh}
    ${xxhash}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE thirdparty)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
################################################################################
# Output directory
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
endif()
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()
################################################################################
# Include directories
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "$<$<CONFIG:Debug>:"
            "${CMAKE_CURRENT_SOURCE_DIR}/jsonc;"
            "${CMAKE_CURRENT_SOURCE_DIR}/mxml;"
            "${CMAKE_CURRENT_SOURCE_DIR}/pcre"
        ">"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
endif()

################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "DEBUG;"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "WIN64;"
            "NDEBUG;"
            "_WINDOWS"
        ">"
        "_CRT_SECURE_NO_WARNINGS;"
        "_PHLIB_;"
        "HAVE_CONFIG_H;"
        "MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS=0;"
        "CAPSTONE_X86_ATT_DISABLE_NO;"
        "CAPSTONE_DIET_NO;"
        "CAPSTONE_X86_REDUCE_NO;"
        "CAPSTONE_HAS_ARM;"
        "CAPSTONE_HAS_ARM64;"
        "CAPSTONE_HAS_X86;"
        "CAPSTONE_USE_SYS_DYN_MEM;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "DEBUG;"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "WIN64;"
            "NDEBUG;"
            "_WINDOWS"
        ">"
        "_CRT_SECURE_NO_WARNINGS;"
        "_PHLIB_;"
        "HAVE_CONFIG_H;"
        "MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS=0;"
        "CAPSTONE_X86_ATT_DISABLE_NO;"
        "CAPSTONE_DIET_NO;"
        "CAPSTONE_X86_REDUCE_NO;"
        "CAPSTONE_HAS_ARM;"
        "CAPSTONE_HAS_ARM64;"
        "CAPSTONE_HAS_X86;"
        "CAPSTONE_USE_SYS_DYN_MEM;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "DEBUG;"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "WIN32;"
            "NDEBUG;"
            "_WINDOWS"
        ">"
        "_CRT_SECURE_NO_WARNINGS;"
        "_PHLIB_;"
        "HAVE_CONFIG_H;"
        "MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS=0;"
        "CAPSTONE_X86_ATT_DISABLE_NO;"
        "CAPSTONE_DIET_NO;"
        "CAPSTONE_X86_REDUCE_NO;"
        "CAPSTONE_HAS_ARM;"
        "CAPSTONE_HAS_ARM64;"
        "CAPSTONE_HAS_X86;"
        "CAPSTONE_USE_SYS_DYN_MEM;"
        "UNICODE;"
        "_UNICODE"
    )
endif()

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
        target_compile_options(${PROJECT_NAME} PRIVATE
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
endif()

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
endif()

