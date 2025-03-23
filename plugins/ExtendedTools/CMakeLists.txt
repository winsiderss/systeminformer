set(PROJECT_NAME ExtendedTools)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "disktabp.h"
    "efi_guid_list.h"
    "etwmini.h"
    "etwmon.h"
    "etwsys.h"
    "extension/plugin.h"
    "exttools.h"
    "framemon.h"
    "gpumini.h"
    "gpumon.h"
    "gpusys.h"
    "npumini.h"
    "npumon.h"
    "npusys.h"
    "poolmon.h"
    "resource.h"
    "tpm.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Header_Files__PresentMon
    "PresentMon/ETW/Microsoft_Windows_D3D9.h"
    "PresentMon/ETW/Microsoft_Windows_Dwm_Core.h"
    "PresentMon/ETW/Microsoft_Windows_DXGI.h"
    "PresentMon/ETW/Microsoft_Windows_DxgKrnl.h"
    "PresentMon/ETW/Microsoft_Windows_EventMetadata.h"
    "PresentMon/ETW/Microsoft_Windows_Win32k.h"
    "PresentMon/PresentMon.hpp"
    "PresentMon/PresentMonTraceConsumer.hpp"
    "PresentMon/TraceConsumer.hpp"
)
source_group("Header Files\\PresentMon" FILES ${Header_Files__PresentMon})

source_group("Resource Files" FILES ${Resource_Files})

set(Source_Files
    "counters.c"
    "disktab.c"
    "etwdisk.c"
    "etwmini.c"
    "etwmon.c"
    "etwprprp.c"
    "etwstat.c"
    "etwsys.c"
    "firmware.c"
    "firmware_editor.c"
    "framemon.cpp"
    "frameprp.c"
    "fwmon.c"
    "fwtab.c"
    "iconext.c"
    "main.c"
    "modsrv.c"
    "namedpipes.c"
    "objmgr.c"
    "objprp.c"
    "options.c"
    "pooldb.c"
    "pooldialog.c"
    "pooldialogbig.c"
    "pooltree.c"
    "reparse.c"
    "smbios.c"
    "svcext.c"
    "thrdact.c"
    "tpm.c"
    "tpm_editor.c"
    "treeext.c"
    "unldll.c"
    "utils.c"
    "waitchain.c"
    "wswatch.c"
)
source_group("Source Files" FILES ${Source_Files})

set(Source_Files__Neural
    "npudetails.c"
    "npumini.c"
    "npumon.c"
    "npunodes.c"
    "npuprprp.c"
    "npusys.c"
)
source_group("Source Files\\Neural" FILES ${Source_Files__Neural})

set(Source_Files__PresentMon
    "PresentMon/PresentMon.cpp"
    "PresentMon/PresentMonTraceConsumer.cpp"
    "PresentMon/TraceConsumer.cpp"
    "PresentMon/TraceSession.cpp"
)
source_group("Source Files\\PresentMon" FILES ${Source_Files__PresentMon})

set(Source_Files__Video
    "gpudetails.c"
    "gpumini.c"
    "gpumon.c"
    "gpunodes.c"
    "gpuprprp.c"
    "gpusys.c"
)
source_group("Source Files\\Video" FILES ${Source_Files__Video})

set(ALL_FILES
    ${Header_Files}
    ${Header_Files__PresentMon}
    ${Resource_Files}
    ${Source_Files}
    ${Source_Files__Neural}
    ${Source_Files__PresentMon}
    ${Source_Files__Video}
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

set(ROOT_NAMESPACE ExtendedTools)

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
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:tdh.dll;
            /DELAYLOAD:tbs.dll;
            /DELAYLOAD:winsta.dll;
            /DEBUG:FULL
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:tdh.dll;
            /DELAYLOAD:tbs.dll;
            /DELAYLOAD:winsta.dll;
            /DEBUG:FULL
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:tdh.dll;
            /DELAYLOAD:tbs.dll;
            /DELAYLOAD:winsta.dll;
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
        "pdh;"
        "tbs;"
        "winsta"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "cfgmgr32;"
        "pdh;"
        "tbs;"
        "winsta"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "cfgmgr32;"
        "pdh;"
        "tbs;"
        "winsta"
    )
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

