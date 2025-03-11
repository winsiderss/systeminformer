set(PROJECT_NAME phlib)

################################################################################
# Source groups
################################################################################
set(Header_Files
    "circbuf_i.h"
    "format_i.h"
    "include/apiimport.h"
    "include/appresolver.h"
    "include/appresolverp.h"
    "include/bcd.h"
    "include/circbuf.h"
    "include/circbuf_h.h"
    "include/colorbox.h"
    "include/cpysave.h"
    "include/dltmgr.h"
    "include/dspick.h"
    "include/emenu.h"
    "include/exlf.h"
    "include/exprodid.h"
    "include/fastlock.h"
    "include/filepool.h"
    "include/filepoolp.h"
    "include/filestream.h"
    "include/filestreamp.h"
    "include/graph.h"
    "include/guisup.h"
    "include/guisupp.h"
    "include/guisupview.h"
    "include/handle.h"
    "include/handlep.h"
    "include/hexedit.h"
    "include/hexeditp.h"
    "include/hndlinfo.h"
    "include/hvsocketcontrol.h"
    "include/json.h"
    "include/kphcomms.h"
    "include/kphuser.h"
    "include/lsasup.h"
    "include/mapimg.h"
    "include/mapldr.h"
    "include/ph.h"
    "include/phbase.h"
    "include/phbasesup.h"
    "include/phconfig.h"
    "include/phconsole.h"
    "include/phdata.h"
    "include/phfirmware.h"
    "include/phintrin.h"
    "include/phintrnl.h"
    "include/phnative.h"
    "include/phnativeinl.h"
    "include/phnet.h"
    "include/phsup.h"
    "include/phutil.h"
    "include/provider.h"
    "include/queuedlock.h"
    "include/ref.h"
    "include/refp.h"
    "include/searchbox.h"
    "include/secedit.h"
    "include/seceditp.h"
    "include/secwmi.h"
    "include/settings.h"
    "include/strsrch.h"
    "include/svcsup.h"
    "include/symprv.h"
    "include/symprvp.h"
    "include/templ.h"
    "include/thirdparty.h"
    "include/treenew.h"
    "include/treenewp.h"
    "include/verify.h"
    "include/verifyp.h"
    "include/workqueue.h"
    "include/workqueuep.h"
    "include/wslsup.h"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "apiimport.c"
    "appresolver.c"
    "appruntime.c"
    "avltree.c"
    "basesup.c"
    "bcd.cpp"
    "circbuf.c"
    "colorbox.c"
    "cpysave.c"
    "data.c"
    "directdraw.cpp"
    "dspick.c"
    "emenu.c"
    "error.c"
    "extlv.c"
    "fastlock.c"
    "filepool.c"
    "filestream.c"
    "firmware.c"
    "format.c"
    "format_std.cpp"
    "global.c"
    "graph.c"
    "guisup.c"
    "guisuplistview.cpp"
    "handle.c"
    "hexedit.c"
    "hndlinfo.c"
    "http.c"
    "hvsocketcontrol.c"
    "icotobmp.c"
    "imgcoherency.c"
    "json.c"
    "kph.c"
    "kphcomms.c"
    "lsasup.c"
    "mapexlf.c"
    "mapimg.c"
    "mapldr.c"
    "maplib.c"
    "native.c"
    "nativeflt.c"
    "nativemodule.c"
    "nativepipe.c"
    "nativethread.c"
    "nativeuser.c"
    "provider.c"
    "queuedlock.c"
    "ref.c"
    "searchbox.c"
    "secdata.c"
    "secedit.c"
    "secwmi.c"
    "settings.c"
    "strsrch.c"
    "svcsup.c"
    "symprv.c"
    "symprv_std.cpp"
    "sync.c"
    "theme.c"
    "treenew.c"
    "util.c"
    "verify.c"
    "workqueue.c"
    "wslsup.c"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} STATIC ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE phlib)

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
# Include directories
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../kphlib/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../kphlib/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../kphlib/include"
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
        "_PHLIB_;"
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
        "_PHLIB_;"
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
        "_PHLIB_;"
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
            $<$<CONFIG:Release>:
                /d1nodatetime;
                /guard:cf
            >
            /permissive-;
            /MP;
            /std:c++latest;
            /W3;
            /WX;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /d1nodatetime;
                /guard:cf
            >
            /permissive-;
            /MP;
            /std:c++latest;
            /W3;
            /WX;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_compile_options(${PROJECT_NAME} PRIVATE
            $<$<CONFIG:Release>:
                /d1nodatetime;
                /guard:cf
            >
            /permissive-;
            /MP;
            /std:c++latest;
            /W3;
            /WX;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING}
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
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    kphlib
)

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "kphlib_um;"
        "thirdparty;"
        "gdiplus"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "kphlib_um;"
        "thirdparty;"
        "gdiplus"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "kphlib_um;"
        "thirdparty;"
        "gdiplus"
    )
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tools/thirdparty/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tools/thirdparty/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_link_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_CURRENT_SOURCE_DIR}/../tools/thirdparty/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
endif()
