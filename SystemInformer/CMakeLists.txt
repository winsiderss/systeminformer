set(PROJECT_NAME SystemInformer)

################################################################################
# Source groups
################################################################################
set(Headers
    "include/actions.h"
    "include/appsup.h"
    "include/colmgr.h"
    "include/colsetmgr.h"
    "include/devprv.h"
    "include/extmgr.h"
    "include/extmgri.h"
    "include/heapstruct.h"
    "include/hidnproc.h"
    "include/hndllist.h"
    "include/hndlmenu.h"
    "include/hndlprv.h"
    "include/informer.h"
    "include/ksisup.h"
    "include/mainwnd.h"
    "include/mainwndp.h"
    "include/memlist.h"
    "include/memprv.h"
    "include/memsrch.h"
    "include/miniinfo.h"
    "include/miniinfop.h"
    "include/modlist.h"
    "include/modprv.h"
    "include/netlist.h"
    "include/netprv.h"
    "include/notifico.h"
    "include/notificop.h"
    "include/notiftoast.h"
    "include/phapp.h"
    "include/phappres.h"
    "include/phfwddef.h"
    "include/phplug.h"
    "include/phsettings.h"
    "include/phsvc.h"
    "include/phsvcapi.h"
    "include/phsvccl.h"
    "include/phuisup.h"
    "include/procgrp.h"
    "include/procmtgn.h"
    "include/procprp.h"
    "include/procprpp.h"
    "include/procprv.h"
    "include/proctree.h"
    "include/srvlist.h"
    "include/srvprv.h"
    "include/sysinfo.h"
    "include/sysinfop.h"
    "include/thrdlist.h"
    "include/thrdprv.h"
    "resource.h"
    "sdk/phdk.h"
)
source_group("Headers" FILES ${Headers})

set(Module
    "SystemInformer.def"
)
source_group("Module" FILES ${Module})

set(Resources
    "resources/application.ico"
    "resources/capslist.txt"
    "resources/case_sensitive_modern_dark.svg"
    "resources/case_sensitive_modern_light.svg"
    "resources/cog.ico"
    "resources/etwguids.txt"
    "resources/pooltag.txt"
    "resources/regex_modern_dark.svg"
    "resources/regex_modern_light.svg"
    "resources/search_modern_dark.svg"
    "resources/search_modern_light.svg"
    "resources/search_stop_modern_dark.svg"
    "resources/search_stop_modern_light.svg"
    "resources/systeminformer.png"
    "SystemInformer.ico"
)
source_group("Resources" FILES ${Resources})

set(System_Informer
    "about.c"
    "actions.c"
    "admintask.c"
    "affinity.c"
    "anawait.c"
    "appsup.c"
    "chcol.c"
    "chdlg.c"
    "chproc.c"
    "colmgr.c"
    "colsetmgr.c"
    "dbgcon.c"
    "delayhook.c"
    "delayload.c"
    "devprv.c"
    "extmgr.c"
    "findobj.c"
    "gdihndl.c"
    "heapinfo.c"
    "hidnproc.c"
    "hndllist.c"
    "hndlmenu.c"
    "hndlprp.c"
    "hndlprv.c"
    "hndlstat.c"
    "infodlg.c"
    "informer.c"
    "itemtips.c"
    "jobprp.c"
    "kdump.c"
    "ksidbg.c"
    "ksisup.c"
    "ksyscall.c"
    "log.c"
    "logwnd.c"
    "main.c"
    "mainwnd.c"
    "mdump.c"
    "memedit.c"
    "memlist.c"
    "memlists.c"
    "memmod.c"
    "memprot.c"
    "memprv.c"
    "memrslt.c"
    "memsrch.c"
    "memsrcht.c"
    "miniinfo.c"
    "modlist.c"
    "modprv.c"
    "mtgndlg.c"
    "mwpgdev.c"
    "mwpgnet.c"
    "mwpgproc.c"
    "mwpgsrv.c"
    "netlist.c"
    "netprv.c"
    "notifico.c"
    "notiftoast.cpp"
    "ntobjprp.c"
    "options.c"
    "pagfiles.c"
    "plugin.c"
    "plugman.c"
    "procgrp.c"
    "procmtgn.c"
    "procprp.c"
    "procprv.c"
    "procrec.c"
    "proctree.c"
    "prpgenv.c"
    "prpggen.c"
    "prpghndl.c"
    "prpgjob.c"
    "prpgmem.c"
    "prpgmod.c"
    "prpgperf.c"
    "prpgsrv.c"
    "prpgstat.c"
    "prpgthrd.c"
    "prpgtok.c"
    "prpgvdm.c"
    "prpgwmi.c"
    "runas.c"
    "searchbox.c"
    "sessmsg.c"
    "sessprp.c"
    "sessshad.c"
    "settings.c"
    "srvcr.c"
    "srvctl.c"
    "srvlist.c"
    "srvprp.c"
    "srvprv.c"
    "sysinfo.c"
    "syssccpu.c"
    "sysscio.c"
    "sysscmem.c"
    "thrdlist.c"
    "thrdprv.c"
    "thrdstk.c"
    "thrdstks.c"
    "tokprp.c"
)
source_group("System Informer" FILES ${System_Informer})

set(phsvc
    "phsvc/clapi.c"
    "phsvc/svcapi.c"
    "phsvc/svcapiport.c"
    "phsvc/svcclient.c"
    "phsvc/svcmain.c"
)
source_group("phsvc" FILES ${phsvc})

set(ALL_FILES
    ${Headers}
    ${Module}
    ${Resources}
    ${System_Informer}
    ${phsvc}
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MTd_StaticDebug")

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE SystemInformer)

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
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../kphlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../kphlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../phnt/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../phlib/include;"
        "${CMAKE_CURRENT_SOURCE_DIR}/../kphlib/include;"
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
        "_PHAPP_;"
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
        "_PHAPP_;"
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
        "_PHAPP_;"
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
            $<$<CONFIG:Release>:
                /d1nodatetime;
                /guard:cf
            >
            /MP;
            /std:c++latest;
            /sdl;
            /GF;
            /Gz;
            /Gy;
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
            /MP;
            /std:c++latest;
            /sdl;
            /GF;
            /Gz;
            /Gy;
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
                /guard:xfg;
                /guard:cf
            >
            /MP;
            /std:c++latest;
            /sdl;
            /GF;
            /Gz;
            /Gy;
            /W3;
            /WX;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            /utf-8;
            ${DEFAULT_CXX_EXCEPTION_HANDLING};
            /Zc:preprocessor
        )
    endif()
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:setupapi.dll;
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:aclui.dll;
            /DELAYLOAD:comdlg32.dll;
            /DELAYLOAD:gdiplus.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:winhttp.dll;
            /DELAYLOAD:winsta.dll;
            /DEBUG;
            /MACHINE:ARM64;
            /SUBSYSTEM:WINDOWS;
            /NATVIS:$(SolutionDir)/SystemInformer.natvis;
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:setupapi.dll;
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:aclui.dll;
            /DELAYLOAD:comdlg32.dll;
            /DELAYLOAD:gdiplus.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:winhttp.dll;
            /DELAYLOAD:winsta.dll;
            /DEBUG;
            /MACHINE:X86;
            /SUBSYSTEM:WINDOWS;
            /NATVIS:$(SolutionDir)/SystemInformer.natvis;
        )
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        target_link_options(${PROJECT_NAME} PRIVATE
            /DELAYLOAD:setupapi.dll;
            /DELAYLOAD:cfgmgr32.dll;
            /DELAYLOAD:aclui.dll;
            /DELAYLOAD:comdlg32.dll;
            /DELAYLOAD:gdiplus.dll;
            /DELAYLOAD:oleaut32.dll;
            /DELAYLOAD:winhttp.dll;
            /DELAYLOAD:winsta.dll;
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
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -sdk -$<CONFIG> -${CMAKE_VS_PLATFORM_NAME}
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -sdk -$<CONFIG> -${CMAKE_VS_PLATFORM_NAME}
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -sdk -$<CONFIG> -${CMAKE_VS_PLATFORM_NAME}
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -sdk -$<CONFIG> -${CMAKE_VS_PLATFORM_NAME}
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    add_custom_command_if(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMANDS
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND   $<CONFIG:Debug> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -sdk -$<CONFIG> -${CMAKE_VS_PLATFORM_NAME}
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -kphsign "$<SHELL_PATH:${OUTPUT_DIRECTORY}>${TARGET_NAME}$ENV{TargetExt}"
        COMMAND $<CONFIG:Release> "$<SHELL_PATH:${CMAKE_SOURCE_DIR}/>tools\\CustomBuildTool\\bin\\Release\\$ENV{PROCESSOR_ARCHITECTURE}\\CustomBuildTool.exe" -sdk -$<CONFIG> -${CMAKE_VS_PLATFORM_NAME}
    )
endif()

################################################################################
# Dependencies
################################################################################
add_dependencies(${PROJECT_NAME}
    kphlib
    phlib
)

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "setupapi;"
        "cfgmgr32;"
        "aclui;"
        "comctl32;"
        "dnsapi;"
        "ntdll;"
        "phlib;"
        "userenv;"
        "wbemuuid;"
        "windowscodecs;"
        "winhttp;"
        "winsta;"
        "kphlib_um"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "setupapi;"
        "cfgmgr32;"
        "aclui;"
        "comctl32;"
        "dnsapi;"
        "ntdll;"
        "phlib;"
        "userenv;"
        "wbemuuid;"
        "windowscodecs;"
        "winhttp;"
        "winsta;"
        "kphlib_um"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "setupapi;"
        "cfgmgr32;"
        "aclui;"
        "comctl32;"
        "dnsapi;"
        "ntdll;"
        "phlib;"
        "userenv;"
        "wbemuuid;"
        "windowscodecs;"
        "winhttp;"
        "winsta;"
        "kphlib_um"
    )
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_SOURCE_DIR}/phlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_SOURCE_DIR}/phlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/kphlib/bin/$<CONFIG>$ENV{PlatformArchitecture};"
        "${CMAKE_SOURCE_DIR}/phlib/bin/$<CONFIG>$ENV{PlatformArchitecture}"
    )
endif()
