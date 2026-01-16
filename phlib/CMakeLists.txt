#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

project(phlib)

set(HEADERS
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
    "include/phafd.h"
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
source_group("Header Files" FILES ${HEADERS})

set(SOURCES
    "apiimport.c"
    "appresolver.c"
    "appruntime.c"
    "avltree.c"
    "basestring.c"
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
    "nativefile.c"
    "nativeflt.c"
    "nativemodule.c"
    "nativejob.c"
    "nativekey.c"
    "nativetxn.c"
    "nativeprocess.c"
    "nativetoken.c"
    "nativepipe.c"
    "nativesocket.c"
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
source_group("Source Files" FILES ${SOURCES})

set(ALL_FILES
    ${HEADERS}
    ${SOURCES}
)

si_add_library(phlib STATIC ${ALL_FILES})

target_include_directories(phlib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(phlib PUBLIC
    _PHLIB_
)

target_link_libraries(phlib PRIVATE
    phnt
    kphlib_um
    thirdparty
    gdiplus
)

add_library(phlib_interface INTERFACE)

target_include_directories(phlib_interface INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
