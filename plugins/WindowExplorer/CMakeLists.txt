set(PROJECT_NAME WindowExplorer)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "prpsh.h"
    "resource.h"
    "wndexp.h"
    "wndtree.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "main.c"
    "prpsh.c"
    "utils.c"
    "wnddlg.c"
    "wndprp.c"
    "wndtree.c"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
################################################################################
# Includes for CMake from *.props
################################################################################
#use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "../Plugins.cmake")

set(ROOT_NAMESPACE WindowExplorer)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
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
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
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
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:comctl32.dll;
            /DELAYLOAD:gdi32.dll;
            /DELAYLOAD:ole32.dll;
            /DELAYLOAD:SystemInformer.exe;
            /DELAYLOAD:propsys.dll;
            /DELAYLOAD:shell32.dll;
            /DELAYLOAD:user32.dll;
            /DEBUG:FULL
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:comctl32.dll;
            /DELAYLOAD:gdi32.dll;
            /DELAYLOAD:ole32.dll;
            /DELAYLOAD:propsys.dll;
            /DELAYLOAD:shell32.dll;
            /DELAYLOAD:user32.dll;
            /DEBUG:FULL
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:comctl32.dll;
            /DELAYLOAD:gdi32.dll;
            /DELAYLOAD:ole32.dll;
            /DELAYLOAD:propsys.dll;
            /DELAYLOAD:shell32.dll;
            /DELAYLOAD:user32.dll;
            /DEBUG:FULL
        )
    endif()
endif()

################################################################################
# Dependencies
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "propsys"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "propsys"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "propsys"
    )
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

