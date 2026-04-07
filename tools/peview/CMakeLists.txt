#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "colmgr.h"
    "include/peview.h"
    "include/prpsh.h"
    "resource.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "version.rc"
    "peview.rc"
    "peview.manifest"
    "resources/case_sensitive_modern_dark.svg"
    "resources/case_sensitive_modern_light.svg"
    "resources/regex_modern_dark.svg"
    "resources/regex_modern_light.svg"
    "resources/search_modern_dark.svg"
    "resources/search_modern_light.svg"
    "resources/search_stop_modern_dark.svg"
    "resources/search_stop_modern_light.svg"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
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
source_group("Source Files" FILES ${SOURCES})

set(SOURCES_PAGES
    "attributes.c"
    "layout.c"
    "mappings.c"
    "propstore.c"
    "streams.c"
    "strings.c"
)
source_group("Source Files\\Pages" FILES ${SOURCES_PAGES})

set(SOURCES_PAGES_LINUX
    "exlfdynamic.c"
    "exlfexports.c"
    "exlfimports.c"
    "exlfprp.c"
)
source_group("Source Files\\Pages\\Linux" FILES ${SOURCES_PAGES_LINUX})

set(SOURCES_PAGES_WINDOWS
    "appmanifest.c"
    "cfgprp.c"
    "clrprp.c"
    "clrprptables.c"
    "clrtableimportprp.c"
    "clrtableimports.cpp"
    "debugprp.c"
    "disimp.c"
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
source_group("Source Files\\Pages\\Windows" FILES ${SOURCES_PAGES_WINDOWS})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
    ${SOURCES_PAGES}
    ${SOURCES_PAGES_LINUX}
    ${SOURCES_PAGES_WINDOWS}
)

si_add_executable(peview WIN32 ${ALL_FILES})

target_include_directories(peview PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(peview PRIVATE
    phnt
    phlib
    kphlib_um
    delayimp
    user32
    gdi32
    comdlg32
    advapi32
    shell32
    ole32
    oleaut32
    aclui
    bcrypt
    cryptui
    crypt32
    propsys
    phlib
    ntdll
    userenv
    windowscodecs
    winhttp
    winsta
)

target_link_options(peview PRIVATE
    /DELAYLOAD:aclui.dll
    /DELAYLOAD:advapi32.dll
    /DELAYLOAD:bcrypt.dll
    /DELAYLOAD:comctl32.dll
    /DELAYLOAD:comdlg32.dll
    /DELAYLOAD:cryptui.dll
    /DELAYLOAD:crypt32.dll
    /DELAYLOAD:gdi32.dll
    /DELAYLOAD:ole32.dll
    /DELAYLOAD:oleaut32.dll
    /DELAYLOAD:propsys.dll
    /DELAYLOAD:shell32.dll
    /DELAYLOAD:user32.dll
)
