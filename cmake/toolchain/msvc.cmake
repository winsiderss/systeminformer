#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

set(SI_C_STANDARD_FLAG   /std:clatest)
set(SI_CXX_STANDARD_FLAG /std:c++latest)

#
# Compiler flags
#
set(SI_COMPILE_FLAGS_INIT
    /diagnostics:caret                  # Quality of life compiler output
    /W3                                 # Warning level 3
    /MP                                 # Multi-processor compilation
    /WX                                 # Treat warnings as errors
    /Oi                                 # Enable intrinsic functions
    /GF                                 # Enable string pooling
    /Gm-                                # Disable minimal rebuild
    /GS                                 # Buffer security check
    /Gy                                 # Enable function-level linking
    /Zc:wchar_t                         # wchar_t is native type
    /Zc:forScope                        # Force conformance in for loop scope
    /Zc:inline                          # Remove unreferenced functions
    /Zc:rvalueCast                      # Enforce type conversion rules
    /Zc:preprocessor                    # New preprocessor conformance
    /permissive-                        # Standards conformance
    /external:W3                        # Warning level for external headers
    /utf-8                              # Set source and execution charsets to UTF-8
    /DEBUG                              # Generate debug info
    /fp:precise                         # Precise floating point
    /EHsc                               # Exception handling model
    /sdl                                # Additional security checks
    /d1nodatetime                       # Disable date/time in debug info
)
set(SI_COMPILE_FLAGS_DEBUG_INIT
    /ZI                                 # Debug information format (edit and continue)
    /MTd                                # Static runtime library (debug)
    /Od                                 # Optimizations (disabled)
    /RTC1                               # Run-time error checks
)
set(SI_COMPILE_FLAGS_RELEASE_INIT
    /Z7                                 # Debug information format (full)
    /MT                                 # Static runtime library
    /O2                                 # Optimizations (maximum speed)
    /Qspectre                           # Enable spectre mitigations
    /guard:cf                           # Control flow guard
)

#
# Linker flags
#
set(SI_STATIC_LINK_FLAGS_INIT
    /MACHINE:${CMAKE_SYSTEM_PROCESSOR}  # Target platform
)
set(SI_LINK_FLAGS_INIT
    /MACHINE:${CMAKE_SYSTEM_PROCESSOR}  # Target platform
    /DEBUG                              # Generate debug info
    /DEBUGTYPE:CV,PDATA
    /DYNAMICBASE                        # Enable ASLR
    /NXCOMPAT                           # Enable DEP
    /DEPENDENTLOADFLAG:0x800            # LOAD_LIBRARY_SEARCH_SYSTEM32
    /FILEALIGN:0x1000                   # Align sections to 4K
    /BREPRO                             # Reproducible builds
    /LARGEADDRESSAWARE                  # Handles addresses larger than 2GB
    /BASERELOCCLUSTERING                # Better relocation compression
)
set(SI_LINK_FLAGS_DEBUG_INIT
    /INCREMENTAL                        # Incremental linking
)
set(SI_LINK_FLAGS_RELEASE_INIT
    /RELEASE                            # Set release checksum
    /INCREMENTAL:NO                     # Incremental linking off
    /OPT:REF                            # Eliminate unreferenced
    /OPT:ICF                            # COMDAT folding
    /LTCG                               # Link-time code generation
    /GUARD:CF                           # Control flow guard
)

#
# Processor specific options
#
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
    list(APPEND SI_COMPILE_FLAGS_RELEASE_INIT
        /guard:signret                  # Signed return instruction generation
        /guard:ehcont                   # EH continuation guard
    )
    list(APPEND SI_LINK_FLAGS_RELEASE_INIT
        /GUARD:EHCONT                   # EH continuation guard
    )
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    list(APPEND SI_COMPILE_FLAGS_RELEASE_INIT
        /guard:xfg                      # Extended flow guard
        /guard:ehcont                   # EH continuation guard
        /QIntel-jcc-erratum             # Intel erratum 1279
    )
    list(APPEND SI_LINK_FLAGS_RELEASE_INIT
        /GUARD:XFG                      # Extended flow guard
        /GUARD:EHCONT                   # EH continuation guard
        /CETCOMPAT                      # CET compatibility
    )
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86")
    list(APPEND SI_COMPILE_FLAGS_RELEASE_INIT
        /QIntel-jcc-erratum             # Intel erratum 1279
    )
    list(APPEND SI_LINK_FLAGS_RELEASE_INIT
        /SAFESEH                        # EH continuation guard
        /CETCOMPAT                      # CET compatibility
    )
else()
    message(FATAL_ERROR "Unknown system processor: ${CMAKE_SYSTEM_PROCESSOR}")
endif()
