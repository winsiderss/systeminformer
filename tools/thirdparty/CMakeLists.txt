#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(SOURCES_JSON_C
    "jsonc/arraylist.c"
    "jsonc/debug.c"
    "jsonc/json_c_version.c"
    "jsonc/json_object.c"
    "jsonc/json_object_iterator.c"
    "jsonc/json_pointer.c"
    "jsonc/json_tokener.c"
    "jsonc/json_util.c"
    "jsonc/json_visit.c"
    "jsonc/libjson.c"
    "jsonc/linkhash.c"
    "jsonc/printbuf.c"
    "jsonc/random_seed.c"
    "jsonc/strerror_override.c"
)
source_group("Json-C" FILES ${SOURCES_JSON_C})

set(HEADERS_JSON_C
    "jsonc/arraylist.h"
    "jsonc/config.h"
    "jsonc/debug.h"
    "jsonc/json.h"
    "jsonc/json_c_version.h"
    "jsonc/json_config.h"
    "jsonc/json_inttypes.h"
    "jsonc/json_object.h"
    "jsonc/json_object_iterator.h"
    "jsonc/json_object_private.h"
    "jsonc/json_pointer.h"
    "jsonc/json_pointer_private.h"
    "jsonc/json_tokener.h"
    "jsonc/json_types.h"
    "jsonc/json_util.h"
    "jsonc/json_visit.h"
    "jsonc/linkhash.h"
    "jsonc/math_compat.h"
    "jsonc/printbuf.h"
    "jsonc/random_seed.h"
    "jsonc/snprintf_compat.h"
    "jsonc/strdup_compat.h"
    "jsonc/strerror_override.h"
    "jsonc/vasprintf_compat.h"
)
source_group("Json-C\\Headers" FILES ${HEADERS_JSON_C})

set(SOURCES_MINI_XML
    "mxml/mxml-attr.c"
    "mxml/mxml-file.c"
    "mxml/mxml-get.c"
    "mxml/mxml-index.c"
    "mxml/mxml-node.c"
    "mxml/mxml-options.c"
    "mxml/mxml-private.c"
    "mxml/mxml-search.c"
    "mxml/mxml-set.c"
)
source_group("Mini-XML" FILES ${SOURCES_MINI_XML})

set(HEADERS_MINI_XML
    "mxml/config.h"
    "mxml/mxml-private.h"
    "mxml/mxml.h"
)
source_group("Mini-XML\\Headers" FILES ${HEADERS_MINI_XML})

set(SOURCES_PCRE
    "pcre/pcre2_auto_possess.c"
    "pcre/pcre2_chartables.c"
    "pcre/pcre2_chkdint.c"
    "pcre/pcre2_compile.c"
    "pcre/pcre2_compile_class.c"
    "pcre/pcre2_chkdint.c"
    "pcre/pcre2_config.c"
    "pcre/pcre2_context.c"
    "pcre/pcre2_convert.c"
    "pcre/pcre2_dfa_match.c"
    "pcre/pcre2_error.c"
    "pcre/pcre2_extuni.c"
    "pcre/pcre2_find_bracket.c"
    "pcre/pcre2_jit_compile.c"
    "pcre/pcre2_maketables.c"
    "pcre/pcre2_match.c"
    "pcre/pcre2_match_data.c"
    "pcre/pcre2_newline.c"
    "pcre/pcre2_ord2utf.c"
    "pcre/pcre2_pattern_info.c"
    "pcre/pcre2_script_run.c"
    "pcre/pcre2_serialize.c"
    "pcre/pcre2_string_utils.c"
    "pcre/pcre2_study.c"
    "pcre/pcre2_substitute.c"
    "pcre/pcre2_substring.c"
    "pcre/pcre2_tables.c"
    "pcre/pcre2_ucd.c"
    "pcre/pcre2_ucptables.c"
    "pcre/pcre2_valid_utf.c"
    "pcre/pcre2_xclass.c"
    "pcre/pcre2posix.c"
)
source_group("PCRE" FILES ${SOURCES_PCRE})

set(HEADERS_PCRE
    "pcre/config.h"
    "pcre/pcre2.h"
    "pcre/pcre2_internal.h"
    "pcre/pcre2_intmodedep.h"
    "pcre/pcre2_ucp.h"
    "pcre/pcre2posix.h"
    "pcre/pcre2_compile.h"
    "pcre/pcre2_util.h"
)
source_group("PCRE\\Headers" FILES ${HEADERS_PCRE})

set(SOURCES_DETOURS
    "detours/detours.cpp"
    "detours/detours.h"
    "detours/disasm.cpp"
    "detours/disolarm.cpp"
    "detours/disolarm64.cpp"
    "detours/disolia64.cpp"
    "detours/disolx64.cpp"
    "detours/disolx86.cpp"
    "detours/modules.cpp"
)
source_group("detours" FILES ${SOURCES_DETOURS})

set(SOURCES_MAXMINDDB
    "maxminddb/data-pool.c"
    "maxminddb/data-pool.h"
    "maxminddb/maxminddb-compat-util.h"
    "maxminddb/maxminddb.c"
    "maxminddb/maxminddb.h"
    "maxminddb/maxminddb_config.h"
)
source_group("maxminddb" FILES ${SOURCES_MAXMINDDB})

set(SOURCES_MD5
    "md5/md5.c"
    "md5/md5.h"
)
source_group("md5" FILES ${SOURCES_MD5})

set(SOURCES_MINIZ
    "miniz/miniz.c"
    "miniz/miniz.h"
    "miniz/miniz_tdef.c"
    "miniz/miniz_tinfl.c"
    "miniz/miniz_zip.c"
    "miniz/miniz_common.h"
    "miniz/miniz_tdef.h"
    "miniz/miniz_tinfl.h"
    "miniz/miniz_zip.h"
)
source_group("miniz" FILES ${SOURCES_MINIZ})

set(SOURCES_SHA
    "sha/sha.c"
    "sha/sha.h"
)
source_group("sha" FILES ${SOURCES_SHA})

set(SOURCES_SHA256
    "sha256/sha256.c"
    "sha256/sha256.h"
)
source_group("sha256" FILES ${SOURCES_SHA256})

set(SOURCES_SSDEEP
    "ssdeep/fuzzy.c"
    "ssdeep/fuzzy.h"
)
source_group("ssdeep" FILES ${SOURCES_SSDEEP})

set(SOURCES_TLSH
    "tlsh/tlsh.cpp"
    "tlsh/tlsh.h"
    "tlsh/tlsh_impl.cpp"
    "tlsh/tlsh_impl.h"
    "tlsh/tlsh_util.cpp"
    "tlsh/tlsh_util.h"
    "tlsh/tlsh_win_version.h"
    "tlsh/tlsh_wrapper.cpp"
    "tlsh/tlsh_wrapper.h"
)
source_group("tlsh" FILES ${SOURCES_TLSH})

set(SOURCES_XXHASH
    "xxhash/xxhash.h"
    "xxhash/xxhashwrapper.c"
    "xxhash/xxhashwrapper.h"
)
source_group("xxhash" FILES ${SOURCES_XXHASH})

set(SOURCES_ZYDIS
    "zydis/Zydis.h"
    "zydis/Zydis.c"
)
source_group("zydis" FILES ${SOURCES_ZYDIS})

set(ALL_FILES
    ${SOURCES_JSON_C}
    ${HEADERS_JSON_C}
    ${SOURCES_MINI_XML}
    ${HEADERS_MINI_XML}
    ${SOURCES_PCRE}
    ${HEADERS_PCRE}
    ${SOURCES_DETOURS}
    ${SOURCES_MAXMINDDB}
    ${SOURCES_MD5}
    ${SOURCES_MINIZ}
    ${SOURCES_SHA}
    ${SOURCES_SHA256}
    ${SOURCES_SSDEEP}
    ${SOURCES_TLSH}
    ${SOURCES_XXHASH}
    ${SOURCES_ZYDIS}
)

si_add_library(thirdparty STATIC ${ALL_FILES})

target_link_libraries(thirdparty PRIVATE
    phnt
    phlib_interface
)

target_compile_definitions(thirdparty PRIVATE
    _CRT_SECURE_NO_WARNINGS
    HAVE_CONFIG_H
    MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS=0
    CAPSTONE_X86_ATT_DISABLE_NO
    CAPSTONE_DIET_NO
    CAPSTONE_X86_REDUCE_NO
    CAPSTONE_HAS_ARM
    CAPSTONE_HAS_ARM64
    CAPSTONE_HAS_X86
    CAPSTONE_USE_SYS_DYN_MEM
)

if(MSVC_CLANG)
    set_source_files_properties(
        "detours/disasm.cpp"
        "detours/disolx64.cpp"
        "detours/disolx86.cpp"
        PROPERTIES
        TARGET_DIRECTORY thirdparty
        COMPILE_OPTIONS "-Wno-reorder-ctor"
    )
    set_source_files_properties(
        "jsonc/arraylist.c"
        "jsonc/json_object.c"
        "ssdeep/fuzzy.c"
        PROPERTIES
        TARGET_DIRECTORY thirdparty
        COMPILE_OPTIONS "-Wno-tautological-constant-out-of-range-compare"
    )
endif()
