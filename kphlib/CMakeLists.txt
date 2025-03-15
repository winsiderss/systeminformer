set(PROJECT_NAME kphlib)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "kphdyn.xml"
    "sistatus.mc"
)
source_group("" FILES ${no_group_source_files})

set(Header_Files
    "include/kphapi.h"
    "include/kphdyn.h"
    "include/kphdyndata.h"
    "include/kphlibbase.h"
    "include/kphmsg.h"
    "include/kphmsgdefs.h"
    "include/kphmsgdyn.h"
    "include/sistatus.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Resource_Files
    "include/sistatus_MSG00001.bin"
)
source_group("Resource Files" FILES ${Resource_Files})

set(Source_Files
    "kphdyn.c"
    "kphdyndata.c"
    "kphmsg.c"
    "kphmsgdyn.c"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${no_group_source_files}
    ${Header_Files}
    ${Resource_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE kphlib_um)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
################################################################################
# Target name
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG   "kphlib_um"
        TARGET_NAME_RELEASE "kphlib_um"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG   "kphlib_um"
        TARGET_NAME_RELEASE "kphlib_um"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG   "kphlib_um"
        TARGET_NAME_RELEASE "kphlib_um"
    )
endif()
################################################################################
# Output directory
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
endif()
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
            MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
            MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
            MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
            MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
            MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
            MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
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
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
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
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Release>:
                /O2;
                /d1nodatetime;
                /guard:cf
            >
            /std:c++latest;
            /W4;
            /WX;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Release>:
                /O2;
                /d1nodatetime;
                /guard:xfg;
                /guard:cf
            >
            /std:c++latest;
            /W4;
            /WX;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od
            >
            $<$<CONFIG:Release>:
                /O2;
                /d1nodatetime;
                /guard:xfg;
                /guard:cf
            >
            /std:c++latest;
            /W4;
            /WX;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /MACHINE:ARM64
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            /MACHINE:X86
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /MACHINE:X64
        )
    endif()
endif()

################################################################################
# Pre build events
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMANDS
        COMMAND $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -dyndata "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>bin\\$<CONFIG>${CMAKE_VS_PLATFORM_NAME}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -dyndata "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>bin\\$<CONFIG>${CMAKE_VS_PLATFORM_NAME}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMANDS
        COMMAND $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -dyndata "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>bin\\$<CONFIG>${CMAKE_VS_PLATFORM_NAME}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -dyndata "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>bin\\$<CONFIG>${CMAKE_VS_PLATFORM_NAME}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMANDS
        COMMAND $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -dyndata "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>bin\\$<CONFIG>${CMAKE_VS_PLATFORM_NAME}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -dyndata "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>bin\\$<CONFIG>${CMAKE_VS_PLATFORM_NAME}"
    )
endif()
