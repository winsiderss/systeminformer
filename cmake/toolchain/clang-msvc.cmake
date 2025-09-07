#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

include(${CMAKE_CURRENT_LIST_DIR}/msvc.cmake)

set(SI_C_STANDARD_FLAG   /std:c17)
set(SI_CXX_STANDARD_FLAG /std:c++latest)

set(CMAKE_RC_FLAGS_INIT "/nologo")

set(_remove "__SI_REMOVE")
set(SI_CLANG_MSVC_REPLACE_COMPILE_FLAGS
    /MP                 ${_remove}
    /Gm-                ${_remove}
    /Zc:preprocessor    ${_remove}
    /d1nodatetime       ${_remove}
    /guard:xfg          ${_remove}
    /ZI                 /Z7
    # TODO(jxy-s) Investigate failures related to this flag.
    /Qspectre           ${_remove} # -mspeculative-load-hardening
    # TODO(jxy-s) Needs investigation. Works locally, but fails on CI.
    /guard:signret      ${_remove}
)
list(LENGTH SI_CLANG_MSVC_REPLACE_COMPILE_FLAGS _replace_total)
math(EXPR _max_idx "${_replace_total} - 1")
foreach(_idx RANGE 0 ${_max_idx} 2)
    math(EXPR _next_idx "${_idx} + 1")
    list(GET SI_CLANG_MSVC_REPLACE_COMPILE_FLAGS ${_idx} _old_flag)
    list(GET SI_CLANG_MSVC_REPLACE_COMPILE_FLAGS ${_next_idx} _new_flag)

    if(_new_flag STREQUAL "${_remove}")
        message(VERBOSE "clang-msvc removing: ${_old_flag}")
        list(REMOVE_ITEM SI_COMPILE_FLAGS_INIT ${_old_flag})
        list(REMOVE_ITEM SI_COMPILE_FLAGS_DEBUG_INIT ${_old_flag})
        list(REMOVE_ITEM SI_COMPILE_FLAGS_RELEASE_INIT ${_old_flag})
    else()
        message(VERBOSE "clang-msvc replacing: ${_old_flag} ${_new_flag}")
        list(TRANSFORM SI_COMPILE_FLAGS_INIT REPLACE "${_old_flag}" "${_new_flag}")
        list(TRANSFORM SI_COMPILE_FLAGS_DEBUG_INIT REPLACE "${_old_flag}" "${_new_flag}")
        list(TRANSFORM SI_COMPILE_FLAGS_RELEASE_INIT REPLACE "${_old_flag}" "${_new_flag}")
    endif()
endforeach()

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
    list(APPEND SI_COMPILE_FLAGS_INIT
        -mcrc                         # Enable ARM64 CRC32 instructions
    )
else()
    list(APPEND SI_COMPILE_FLAGS_INIT
        -mavx                         # Enable AVX instructions
        -mavx2                        # Enable AVX2 instructions
        -mavx512vl                    # Enable AVX512VL instructions
        -mrdrnd                       # Enable RDRAND instructions
    )
endif()

#
# Only suppress here if Microsoft code ends up needing it.
#
list(APPEND SI_COMPILE_FLAGS_INIT
    -fms-compatibility
    -fms-extensions
    -Wno-comment
    -Wno-extern-c-compat
    -Wno-extern-initializer
    -Wno-ignored-attributes
    -Wno-ignored-pragma-intrinsic
    -Wno-microsoft-anon-tag
    -Wno-microsoft-enum-forward-reference
    -Wno-microsoft-include
    -Wno-overloaded-virtual
    -Wno-pragma-pack
    -Wno-unknown-pragmas
    -Wno-unused-local-typedef
    -Wno-unused-value

    # TODO(jxy-s) Needs investigation. Works locally, but fails on CI.
    -Wno-int-to-void-pointer-cast
    -Wno-void-pointer-to-int-cast
)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
    list(APPEND SI_COMPILE_FLAGS_INIT
        -Wno-implicit-function-declaration
    )
endif()
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    list(APPEND SI_COMPILE_FLAGS_INIT
        -Wno-missing-prototype-for-cc
    )
endif()
