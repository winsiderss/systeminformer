set(PROJECT_NAME HardwareDevices)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "devices.h"
    "prpsh.h"
    "resource.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "main.c"
    "prpsh.c"
)
source_group("Source Files" FILES ${Source_Files})

set(Source_Files__Devices
    "deviceprops.c"
    "devicetree.c"
)
source_group("Source Files\\Devices" FILES ${Source_Files__Devices})

set(Source_Files__Disk
    "disk.c"
    "diskdetails.c"
    "diskgraph.c"
    "disknotify.c"
    "diskoptions.c"
    "fmifs.h"
    "storage.c"
)
source_group("Source Files\\Disk" FILES ${Source_Files__Disk})

set(Source_Files__Graphics
    "gpu.c"
    "gpudetails.c"
    "gpugraph.c"
    "gpunodes.c"
    "gpuoptions.c"
    "graphics.c"
)
source_group("Source Files\\Graphics" FILES ${Source_Files__Graphics})

set(Source_Files__Network
    "adapter.c"
    "ndis.c"
    "netdetails.c"
    "netgraph.c"
    "netoptions.c"
)
source_group("Source Files\\Network" FILES ${Source_Files__Network})

set(Source_Files__RAPL
    "power.c"
    "powergraph.c"
    "poweroptions.c"
)
source_group("Source Files\\RAPL" FILES ${Source_Files__RAPL})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
    ${Source_Files__Devices}
    ${Source_Files__Disk}
    ${Source_Files__Graphics}
    ${Source_Files__Network}
    ${Source_Files__RAPL}
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

set(ROOT_NAMESPACE HardwareDevices)

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
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:iphlpapi.dll;
            /DEBUG:FULL
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:iphlpapi.dll;
            /DEBUG:FULL
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:iphlpapi.dll;
            /DEBUG:FULL
        )
    endif()
endif()

################################################################################
# Dependencies
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "cfgmgr32;"
        "iphlpapi;"
        "wlanapi"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "cfgmgr32;"
        "iphlpapi;"
        "wlanapi"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "cfgmgr32;"
        "iphlpapi;"
        "wlanapi"
    )
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

