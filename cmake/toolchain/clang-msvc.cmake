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

# TODO(jxy-s) Many of these should be moved out of the toolchain and into the
# project and/or fixed in the project sources.
# Only suppress here if Microsoft code ends up needing it...
list(APPEND SI_COMPILE_FLAGS_INIT
    -mavx                         # Enable AVX instructions
    -mavx2                        # Enable AVX2 instructions
    -mavx512vl                    # Enable AVX512VL instructions
    -mrdrnd                       # Enable RDRAND instructions

    # Required for Microsoft headers
    -Wno-microsoft-enum-forward-reference
    -Wno-microsoft-static-assert
    -Wno-microsoft-anon-tag
    -Wno-microsoft-include
    -Wno-ignored-pragma-intrinsic
    -Wno-ignored-attributes
    -Wno-pragma-pack
    -Wno-unused-local-typedef
    -Wno-overloaded-virtual
    -Wno-comment
    -Wno-extern-c-compat

    # TODO(jxy-s) Common pattern in project, move to project configuration.
    -Wno-parentheses
    -Wno-incompatible-pointer-types
    -Wno-unused-variable
    -Wno-switch
    -Wno-missing-braces

    # TODO(jxy-s) Prevalent in project, but should maybe be addressed.
    -Wno-visibility
    -Wno-pointer-sign

    # TODO(jxy-s) Likely fixable with some small/medium refactors.
    -Wno-unused-but-set-variable
    -Wno-int-to-void-pointer-cast

    # TODO(jxy-s) Should be narrowly suppressed in specific places.
    -Wno-unused-value
    -Wno-reorder-ctor

    # TODO(jxy-s) Requires significant refactor of use of static_assert. Or wait
    # for c23 to be available for clang-msvc.
    # error: '_Static_assert' with no message is a C23 extension [-Werror,-Wc23-extensions]
    -Wno-c23-extensions

    # TODO(jxy-s) Needs investigation
    #C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\lib\clang\19\include\xmmintrin.h(2187,9): error: '_MM_HINT_T2' macro redefined [-Werror,-Wmacro-redefined]
    # 2187 | #define _MM_HINT_T2  1
    #      |         ^
    #C:\Program Files (x86)\Windows Kits\10\\include\10.0.26100.0\\um\winnt.h(3651,9): note: previous definition is here
    # 3651 | #define _MM_HINT_T2     3
    #      |         ^
    -Wno-macro-redefined

    # TODO(jxy-s) Fairly large refactor, but should be able to address this.
    # Or might be acceptable to suppress this in project configuration.
    # #pragma deprecated
    # #pragma prefast
    -Wno-unknown-pragmas

    # TODO(jxy-s) Investigate, maybe fixable? However, unused functions are a
    # common pattern. But the only errors are from here:
    #C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\lib\clang\19\include\amxcomplexintrin.h(139,13): error: unused function '__tile_cmmimfp16ps' [-Werror,-Wunused-function]
    #  139 | static void __tile_cmmimfp16ps(__tile1024i *dst, __tile1024i src0,
    #      |             ^~~~~~~~~~~~~~~~~~
    #C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\lib\clang\19\include\amxcomplexintrin.h(162,13): error: unused function '__tile_cmmrlfp16ps' [-Werror,-Wunused-function]
    #  162 | static void __tile_cmmrlfp16ps(__tile1024i *dst, __tile1024i src0,
    #      |             ^~~~~~~~~~~~~~~~~~
    -Wno-unused-function
)
