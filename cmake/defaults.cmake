#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

if(NOT MSVC)
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if(CMAKE_GENERATOR_PLATFORM)
    if(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
        set(SI_PLATFORM "x64")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "AMD64")
        set(SI_PLATFORM "x64")
    elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
        set(SI_PLATFORM "ARM64")
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "i[3-6]86|x86")
        set(SI_PLATFORM "Win32")
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "Win32")
        set(SI_PLATFORM "Win32")
    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
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

set(SI_COMPILE_FLAGS_USER
    ${SI_COMPILE_FLAGS_COMMON}
    /W3               # Warning level 3
    /sdl              # Additional security checks
    /Gz               # __stdcall calling convention
    /Od               # Disable optimizations (debug)
    /fp:precise       # Precise floating point
    /RTC1             # Run-time error checks
    /EHsc             # Exception handling model
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND SI_COMPILE_FLAGS_USER
        /Od           # Disable optimizations (debug)
        /RTC1         # Run-time error checks
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND SI_COMPILE_FLAGS_USER
        /O2           # Maximum optimizations (release)
        /d1nodatetime
        /guard:cf     # Control flow guard
    )
    if(SI_PLATFORM STREQUAL "x64")
        list(APPEND SI_COMPILE_FLAGS_USER
            /guard:xcf # Extended flow guard
        )
    endif()
endif()

set(SI_COMPILE_FLAGS_KERNEL
    ${SI_COMPILE_FLAGS_COMMON}
    /W4               # Warning level 4
    /kernel           # Kernel mode compilation
)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND SI_COMPILE_FLAGS_KERNEL
        /Od           # Disable optimizations (debug)
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    list(APPEND SI_COMPILE_FLAGS_KERNEL
        /O2           # Maximum optimizations (release)
        /d1nodatetime
    )
endif()

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
