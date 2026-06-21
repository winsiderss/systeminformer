#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "resource.h"
    "updater.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES
    "Updater.rc"
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "downloadbits.cpp"
    "downloadmsix.cpp"
    "downloadp2p.cpp"
    "main.c"
    "options.c"
    "page1.c"
    "page2.c"
    "page3.c"
    "page4.c"
    "page5.c"
    "toastmain.c"
    "toastnotify.c"
    "updater.c"
    "utils.c"
    "verify.c"
)
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
)
si_add_plugin(Updater ${ALL_FILES})

target_link_libraries(Updater PRIVATE
    delayimp
    user32
    gdi32
    bcrypt
    Bits
    ole32
    oleaut32
    uuid
)

target_link_options(Updater PRIVATE
    /DELAYLOAD:bcrypt.dll
)
