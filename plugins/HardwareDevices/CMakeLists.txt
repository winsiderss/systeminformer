#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(HEADERS
    "devices.h"
    "prpsh.h"
    "resource.h"
)
source_group("Header Files" FILES ${HEADERS})

set(RESOURCES,
    "version.rc"
)
source_group("Resource Files" FILES ${RESOURCES})

set(SOURCES
    "main.c"
    "prpsh.c"
)
source_group("Source Files" FILES ${SOURCES})

set(SOURCES_DEVICES
    "deviceprops.c"
    "devicetree.c"
)
source_group("Source Files\\Devices" FILES ${SOURCES_DEVICES})

set(SOURCES_DISK
    "disk.c"
    "diskdetails.c"
    "diskgraph.c"
    "disknotify.c"
    "diskoptions.c"
    "fmifs.h"
    "storage.c"
)
source_group("Source Files\\Disk" FILES ${SOURCES_DISK})

set(SOURCES_GRAPHICS
    "gpu.c"
    "gpudetails.c"
    "gpugraph.c"
    "gpunodes.c"
    "gpuoptions.c"
    "graphics.c"
)
source_group("Source Files\\Graphics" FILES ${SOURCES_GRAPHICS})

set(SOURCES_NETWORK
    "adapter.c"
    "ndis.c"
    "netdetails.c"
    "netgraph.c"
    "netoptions.c"
)
source_group("Source Files\\Network" FILES ${SOURCES_NETWORK})

set(SOURCES_RAPL
    "power.c"
    "powergraph.c"
    "poweroptions.c"
)
source_group("Source Files\\RAPL" FILES ${SOURCES_RAPL})

si_add_plugin(HardwareDevices
    ${HEADERS}
    ${RESOURCES}
    ${SOURCES}
    ${SOURCES_DEVICES}
    ${SOURCES_DISK}
    ${SOURCES_GRAPHICS}
    ${SOURCES_NETWORK}
    ${SOURCES_RAPL}
)

target_link_libraries(HardwareDevices PRIVATE
    delayimp
    cfgmgr32
    iphlpapi
    wlanapi
)

target_link_options(HardwareDevices PRIVATE
    /DELAYLOAD:cfgmgr32.dll
    /DELAYLOAD:iphlpapi.dll
)
