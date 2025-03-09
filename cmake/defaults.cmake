#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

if(NOT MSVC)
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(CMAKE_VS_PLATFORM_NAME)
    message(STATUS "CMAKE_VS_PLATFORM_NAME: ${CMAKE_VS_PLATFORM_NAME}")
    set(SI_PLATFORM "${CMAKE_VS_PLATFORM_NAME}")
else()
    message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(SI_PLATFORM "x64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
        set(SI_PLATFORM "ARM64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i[3-6]86|x86")
        set(SI_PLATFORM "Win32")
    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

#
# Compiler flags
#

set(SI_COMPILE_FLAGS_COMMON
    /WX               # Treat warnings as errors
    /Oi               # Enable intrinsic functions
    /Ot               # Favor speed
    /GF               # Enable string pooling
    /Gm-              # Disable minimal rebuild
    /GS               # Buffer security check
    /Gy               # Enable function-level linking
    /Zc:wchar_t       # wchar_t is native type
    /Zc:forScope      # Force conformance in for loop scope
    /Zc:inline        # Remove unreferenced functions
    /Zc:rvalueCast    # Enforce type conversion rules
    /Zc:preprocessor  # New preprocessor conformance
    /permissive-      # Standards conformance
    /external:W3      # Warning level for external headers
    /utf-8            # Set source and execution charsets to UTF-8
)
if($<CONFIG> STREQUAL "Debug")
    list(APPEND SI_COMPILE_FLAGS_COMMON
        /Od           # Disable optimizations (debug)
    )
elseif($<CONFIG> STREQUAL "Release")
    list(APPEND SI_COMPILE_FLAGS_COMMON
        /O2           # Maximum optimizations (release)
    )
endif()

set(SI_COMPILE_FLAGS_USER
    ${SI_COMPILE_FLAGS_COMMON}
    /W3               # Warning level 3
    /Gz               # __stdcall calling convention
    /fp:precise       # Precise floating point
    /EHsc             # Exception handling model
)
if($<CONFIG> STREQUAL "Debug")
    list(APPEND SI_COMPILE_FLAGS_USER
        /sdl          # Additional security checks
        /RTC1         # Run-time error checks
    )
elseif($<CONFIG> STREQUAL "Release")
    list(APPEND SI_COMPILE_FLAGS_USER
        /d1nodatetime
    )
    if(SI_PLATFORM STREQUAL "x64")
        list(APPEND SI_COMPILE_FLAGS_USER
            /guard:xcf # Extended flow guard
        )
    else()
        list(APPEND SI_COMPILE_FLAGS_USER
            /guard:cf  # Control flow guard
        )
    endif()
endif()

set(SI_COMPILE_FLAGS_KERNEL
    ${SI_COMPILE_FLAGS_COMMON}
    /W4               # Warning level 4
    /kernel           # Kernel mode compilation
)

#
# Linker flags
#

set(SI_LINK_FLAGS_COMMON
    /DYNAMICBASE
    /NXCOMPAT
    /NATVIS:${SYSTEM_INFORMER_ROOT}/SystemInformer.natvis
    /DEPENDENTLOADFLAG:0x800 # LOAD_LIBRARY_SEARCH_SYSTEM32
    /FILEALIGN:0x1000
    /BREPRO
)

set(SI_LINK_FLAGS_USER
    ${SI_LINK_FLAGS_COMMON}
    /INCREMENTAL
    /LARGEADDRESSAWARE
)

set(SI_LINK_FLAGS_KERNEL
    ${SI_LINK_FLAGS_COMMON}
)

#
# Compiler definitions
#

set(SI_COMPILE_DEFS_COMMON
)

set(SI_COMPILE_DEFS_USER
    ${SI_COMPILE_DEFS_COMMON}
)

set(SI_COMPILE_DEFS_KERNEL
    ${SI_COMPILE_DEFS_COMMON}
)
