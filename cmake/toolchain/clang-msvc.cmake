#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

include(${CMAKE_CURRENT_LIST_DIR}/msvc.cmake)

set(SI_C_STANDARD_FLAG   /std:c17)
set(SI_CXX_STANDARD_FLAG /std:c++latest)

set(CMAKE_RC_FLAGS_INIT "/nologo")

list(REMOVE_ITEM SI_COMPILE_FLAGS_INIT
    /MP
    /Gm-
    /Zc:preprocessor
    /d1nodatetime
)

list(TRANSFORM SI_COMPILE_FLAGS_DEBUG_INIT REPLACE "/ZI" "/Z7")

# TODO(jxy-s) Many of these should be moved out of the toolchain and into the
# project and/or fixed in the project sources.
# Only suppress here if Microsoft code ends up needing it...
list(APPEND SI_COMPILE_FLAGS_INIT
    "-mavx"                         # Enable AVX instructions
    "-mavx2"                        # Enable AVX2 instructions
    "-mavx512vl"                    # Enable AVX512VL instructions
    "-mrdrnd"                       # Enable RDRAND instructions
    "-Wno-c23-extensions"
    "-Wno-ignored-pragma-intrinsic"
    "-Wno-ignored-attributes"
    "-Wno-macro-redefined"
    "-Wno-microsoft-static-assert"
    "-Wno-microsoft-anon-tag"
    "-Wno-microsoft-enum-forward-reference"
    "-Wno-parentheses"
    "-Wno-pragma-pack"
    "-Wno-unknown-pragmas"
    "-Wno-unused-function"
    "-Wno-unused-local-typedef"
    "-Wno-unused-variable"
    "-Wno-incompatible-pointer-types"
    "-Wno-visibility"
    "-Wno-overloaded-virtual"
    "-Wno-switch"
    "-Wno-pointer-sign"
    "-Wno-unused-but-set-variable"
    "-Wno-unused-value"
    "-Wno-reorder-ctor"
    "-Wno-missing-braces"
    "-Wno-sizeof-array-div"
    "-Wno-invalid-noreturn"
    "-Wno-enum-conversion"
    "-Wno-single-bit-bitfield-constant-conversion"
    "-Wno-tautological-constant-out-of-range-compare"
    "-Wno-implicit-const-int-float-conversion"
    "-Wno-misleading-indentation"
    "-Wno-extern-c-compat"
    "-Wno-void-pointer-to-int-cast"
    "-Wno-int-to-void-pointer-cast"
    "-Wno-microsoft-include"
    "-Wno-extra-tokens"
    "-Wno-comment"
    "-Wno-invalid-offsetof"
)
