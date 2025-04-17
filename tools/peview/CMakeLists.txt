set(PROJECT_NAME peview)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "colmgr.h"
    "include/peview.h"
    "include/prpsh.h"
    "resource.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Resource_Files
    "resources/case_sensitive_modern_dark.svg"
    "resources/case_sensitive_modern_light.svg"
    "resources/regex_modern_dark.svg"
    "resources/regex_modern_light.svg"
    "resources/search_modern_dark.svg"
    "resources/search_modern_light.svg"
    "resources/search_stop_modern_dark.svg"
    "resources/search_stop_modern_light.svg"
)
source_group("Resource Files" FILES ${Resource_Files})

set(Source_Files
    "chcol.c"
    "colmgr.c"
    "delayhook.c"
    "main.c"
    "misc.c"
    "options.c"
    "prpsh.c"
    "searchbox.c"
    "settings.c"
)
source_group("Source Files" FILES ${Source_Files})

set(Source_Files__Pages
    "attributes.c"
    "layout.c"
    "mappings.c"
    "propstore.c"
    "streams.c"
    "strings.c"
)
source_group("Source Files\\Pages" FILES ${Source_Files__Pages})

set(Source_Files__Pages__Linux
    "exlfdynamic.c"
    "exlfexports.c"
    "exlfimports.c"
    "exlfprp.c"
)
source_group("Source Files\\Pages\\Linux" FILES ${Source_Files__Pages__Linux})

set(Source_Files__Pages__Windows
    "cfgprp.c"
    "clrprp.c"
    "clrprptables.c"
    "clrtableimportprp.c"
    "clrtableimports.cpp"
    "debugprp.c"
    "ehcontprp.c"
    "expprp.c"
    "hashprp.c"
    "impprp.c"
    "ldprp.c"
    "libprp.c"
    "links.c"
    "pdb.c"
    "pdbprp.c"
    "pechpeprp.c"
    "pedirprp.c"
    "pedynrelocprp.c"
    "peexceptprp.c"
    "peheaderprp.c"
    "pemuiprp.c"
    "pepogoprp.c"
    "peprp.c"
    "peprpwnd.c"
    "perelocprp.c"
    "pesectionprp.c"
    "previewprp.c"
    "processes.c"
    "resprp.c"
    "richprp.c"
    "secprp.c"
    "tlsprp.c"
    "versioninfoprp.c"
    "volatileprp.c"
)
source_group("Source Files\\Pages\\Windows" FILES ${Source_Files__Pages__Windows})

set(ALL_FILES
    ${Header_Files}
    ${Resource_Files}
    ${Source_Files}
    ${Source_Files__Pages}
    ${Source_Files__Pages__Linux}
    ${Source_Files__Pages__Windows}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE peview)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
################################################################################
# Output directory
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>$ENV{PlatformArchitecture}/"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../kphlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../kphlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../kphlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
endif()

################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG;"
            "DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_PHLIB_;"
        "_WINDOWS;"
        "WIN64;"
        "$ENV{ExternalCompilerOptions};"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG;"
            "DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_PHLIB_;"
        "_WINDOWS;"
        "WIN32;"
        "$ENV{ExternalCompilerOptions};"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG;"
            "DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "_PHLIB_;"
        "_WINDOWS;"
        "WIN64;"
        "$ENV{ExternalCompilerOptions};"
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
                /Od;
                /Zi
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy;
                ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
                /d1nodatetime;
                /guard:cf
            >
            /MP;
            /std:c++latest;
            /sdl;
            /GF;
            /W3;
            /WX;
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od;
                /Zi
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy;
                ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
                /d1nodatetime;
                /guard:cf
            >
            /MP;
            /std:c++latest;
            /sdl;
            /GF;
            /Gz;
            /W3;
            /WX;
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Debug>:
                /Od;
                /Zi
            >
            $<$<CONFIG:Release>:
                /O2;
                /Oi;
                /Gy;
                ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
                /d1nodatetime;
                /guard:cf
            >
            /MP;
            /std:c++latest;
            /sdl;
            /GF;
            /Gz;
            /W3;
            /WX;
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /LTCG;
                /OPT:ICF;
                /INCREMENTAL:NO
            >
            /DELAYLOAD:aclui.dll;
            /DELAYLOAD:advapi32.dll;
            /DELAYLOAD:bcrypt.dll;
            /DELAYLOAD:comctl32.dll;
            /DELAYLOAD:comdlg32.dll;
            /DELAYLOAD:cryptui.dll;
            /DELAYLOAD:crypt32.dll;
            /DELAYLOAD:gdi32.dll;
            /DELAYLOAD:ole32.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:propsys.dll;
            /DELAYLOAD:shell32.dll;
            /DELAYLOAD:user32.dll;
            /DEBUG;
            /MACHINE:ARM64;
            /SUBSYSTEM:WINDOWS;
            /NATVIS:$(SolutionDir)/SystemInformer.natvis;
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /LTCG;
                /OPT:ICF;
                /INCREMENTAL:NO
            >
            /DELAYLOAD:aclui.dll;
            /DELAYLOAD:advapi32.dll;
            /DELAYLOAD:bcrypt.dll;
            /DELAYLOAD:comctl32.dll;
            /DELAYLOAD:comdlg32.dll;
            /DELAYLOAD:cryptui.dll;
            /DELAYLOAD:crypt32.dll;
            /DELAYLOAD:gdi32.dll;
            /DELAYLOAD:ole32.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:propsys.dll;
            /DELAYLOAD:shell32.dll;
            /DELAYLOAD:user32.dll;
            /DEBUG;
            /MACHINE:X86;
            /SUBSYSTEM:WINDOWS;
            /NATVIS:$(SolutionDir)/SystemInformer.natvis;
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /OPT:REF;
                /LTCG;
                /OPT:ICF;
                /INCREMENTAL:NO
            >
            /DELAYLOAD:aclui.dll;
            /DELAYLOAD:advapi32.dll;
            /DELAYLOAD:bcrypt.dll;
            /DELAYLOAD:comctl32.dll;
            /DELAYLOAD:comdlg32.dll;
            /DELAYLOAD:cryptui.dll;
            /DELAYLOAD:crypt32.dll;
            /DELAYLOAD:gdi32.dll;
            /DELAYLOAD:ole32.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:propsys.dll;
            /DELAYLOAD:shell32.dll;
            /DELAYLOAD:user32.dll;
            /DEBUG;
            /MACHINE:X64;
            /SUBSYSTEM:WINDOWS;
            /NATVIS:$(SolutionDir)/SystemInformer.natvis;
        )
    endif()
endif()

################################################################################
# Post build events
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
    )
endif()

################################################################################
# Dependencies
################################################################################
# Link with other targets.
target_link_libraries(${PROJECT_NAME} PRIVATE
    phlib
)

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "aclui;"
        "bcrypt;"
        "cryptui;"
        "crypt32;"
        "propsys;"
        "phlib;"
        "ntdll;"
        "userenv;"
        "windowscodecs;"
        "winhttp;"
        "winsta;"
        "kphlib_um"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "aclui;"
        "bcrypt;"
        "cryptui;"
        "crypt32;"
        "propsys;"
        "phlib;"
        "ntdll;"
        "userenv;"
        "windowscodecs;"
        "winhttp;"
        "winsta;"
        "kphlib_um"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "aclui;"
        "bcrypt;"
        "cryptui;"
        "crypt32;"
        "propsys;"
        "phlib;"
        "ntdll;"
        "userenv;"
        "windowscodecs;"
        "winhttp;"
        "winsta;"
        "kphlib_um"
    )
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/phlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/phlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/phlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
endif()

