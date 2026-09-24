#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#
# Shared clang-cl + LLVM lld-link configuration. Intended for DEBUG / local
# iteration only. lld-link cannot emit /GUARD:XFG or /GUARD:EHCONT, so shipping
# binaries must continue to be produced by the clang-msvc-* toolchains, which
# link with MSVC link.exe.
#
# Per-architecture toolchain files set the compiler, target triple and resource
# tools, then include this file, then include finalize-msvc.cmake. Ordering
# matters: finalize-msvc.cmake joins the SI_*_INIT lists into CMAKE_*_FLAGS, so
# every removal below must happen before it runs.
#

include(${CMAKE_CURRENT_LIST_DIR}/clang-msvc.cmake)

set(CMAKE_LINKER          lld-link)
set(CMAKE_AR              llvm-lib)
set(CMAKE_C_COMPILER_AR   llvm-lib)
set(CMAKE_CXX_COMPILER_AR llvm-lib)

#
# Strip link flags that lld-link does not implement. CMake invokes CMAKE_LINKER
# (lld-link) directly, so these are passed verbatim; lld-link treats the unknown
# /BASERELOCCLUSTERING as an input file and errors.
#
list(REMOVE_ITEM SI_LINK_FLAGS_INIT /BASERELOCCLUSTERING)

#
# The Release-only mitigations below have no lld-link implementation. They are
# removed rather than diagnosed as a hard error because the Ninja Multi-Config
# generator does not resolve the configuration until build time, so a configure
# time check on the build type cannot see it. Warn instead, so that a Release
# build through this toolchain is obviously not a shippable one.
#
list(REMOVE_ITEM SI_COMPILE_FLAGS_RELEASE_INIT
    /guard:xfg
    /guard:ehcont
)
list(REMOVE_ITEM SI_LINK_FLAGS_RELEASE_INIT
    /GUARD:XFG
    /GUARD:EHCONT
    /CETCOMPAT
    /LTCG
)
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    list(REMOVE_ITEM SI_LINK_FLAGS_RELEASE_INIT /SAFESEH)
endif()

message(WARNING
    "clang-lld is a Debug-only toolchain. lld-link cannot emit XFG or EHCONT, "
    "so /guard:xfg, /guard:ehcont, /GUARD:XFG, /GUARD:EHCONT, /CETCOMPAT and "
    "/LTCG have been dropped from the Release configuration. Use a "
    "clang-msvc-* or msvc-* toolchain for binaries that will be signed or "
    "shipped."
)
