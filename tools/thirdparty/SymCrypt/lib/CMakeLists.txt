set(SOURCES_COMMON
    3des.c
    a_dispatch.c
    aes-asm.c
    aes-c.c
    aes-default-bc.c
    aes-default.c
    aes-key.c
    aes-neon.c
    aes-selftest.c
    aes-xmm.c
    aes-ymm.c
    aescmac.c
    aesCtrDrbg.c
    aeskw.c
    AesTables.c
    blockciphermodes.c
    ccm.c
    chacha20_poly1305.c
    chacha20.c
    composite_encoding.c
    composite_mlkem.c
    cpuid_notry.c
    cpuid_um.c
    cpuid.c
    crt.c
    data_accessors.c
    DesTables.c
    desx.c
    dh.c
    dl_internal_groups.c
    dlgroup.c
    dlkey.c
    dsa.c
    ec_dh.c
    ec_dispatch.c
    ec_dsa.c
    ec_internal_curve_params.c
    ec_internal_curves.c
    ec_montgomery.c
    ec_mul.c
    ec_short_weierstrass.c
    ec_twisted_edwards.c
    eckey.c
    ecpoint.c
    ecurve.c
    equal.c
    FatalIntercept.c
    fdef_general.c
    fdef_int.c
    fdef_mod.c
    fdef369_mod.c
    fips_selftest.c
    gcm.c
    gen_int.c
    ghash.c
    hash.c
    hkdf_selftest.c
    hkdf.c
    hmac.c
    hmacmd5.c
    hmacsha1.c
    hmacsha224.c
    hmacsha256.c
    hmacsha384.c
    hmacsha512.c
    hmacsha512_224.c
    hmacsha512_256.c
    hmacsha3_224.c
    hmacsha3_256.c
    hmacsha3_384.c
    hmacsha3_512.c
    kmac.c
    libmain.c
    lms.c
    marvin32.c
    md2.c
    md4.c
    md5.c
    mldsa.c
    mldsa_primitives.c
    mlkem.c
    mlkem_primitives.c
    modexp.c
    paddingPkcs7.c
    parhash.c
    pbkdf2_hmacsha1.c
    pbkdf2_hmacsha256.c
    pbkdf2.c
    poly1305.c
    primes.c
    rc2.c
    rc4.c
    rdrand.c
    rdseed.c
    recoding.c
    rsa_enc.c
    rsa_padding.c
    rsakey.c
    ScsTable.c
    scsTools.c
    selftest.c
    service_indicator.c
    session.c
    sha1.c
    sha256.c
    sha256Par.c
    sha256Par-ymm.c
    sha256-xmm.c
    sha256-ymm.c
    sha512.c
    sha512Par.c
    sha512Par-ymm.c
    sha512-ymm.c
    sha3.c
    sha3_224.c
    sha3_256.c
    sha3_384.c
    sha3_512.c
    shake.c
    sp800_108_hmacsha1.c
    sp800_108_hmacsha256.c
    sp800_108_hmacsha512.c
    sp800_108.c
    srtp_kdf.c
    srtp_kdf_selftest.c
    ssh_kdf.c
    ssh_kdf_sha256.c
    ssh_kdf_sha512.c
    sskdf.c
    sskdf_selftest.c
    tlsCbcVerify.c
    tlsprf_selftest.c
    tlsprf.c
    xmss.c
    xtsaes.c
  )

function(process_cppasm filepath outformat archdefine)
    get_filename_component(fileextension ${filepath} EXT)
    if(NOT fileextension STREQUAL .cppasm)
        message(FATAL_ERROR "cppasm processing invoked on file with incorrect extension (${filepath} -> ${fileextension})")
    endif()
    if((NOT outformat STREQUAL gas) AND (NOT outformat STREQUAL masm) AND (NOT outformat STREQUAL armasm64))
        message(FATAL_ERROR "cppasm processing invoked with unrecognized outformat (${outformat})")
    endif()
    if((NOT archdefine STREQUAL amd64) AND (NOT archdefine STREQUAL x86) AND (NOT archdefine STREQUAL arm64) AND (NOT archdefine STREQUAL arm))
        message(FATAL_ERROR "cppasm processing invoked with unrecognized archdefine (${archdefine})")
    endif()

    get_filename_component(rootpath ${filepath} DIRECTORY)

    get_filename_component(filestem ${filepath} NAME_WE) # filestem is the filename w/out extension
    string(TOUPPER ${archdefine} archdefineupper)
    string(FIND ${rootpath} ${CMAKE_CURRENT_BINARY_DIR} findindex) # check whether input is in the output directory
    if(findindex EQUAL -1) # input in the source directory
        set(filepath ${CMAKE_CURRENT_SOURCE_DIR}/${filepath})
        set(output_asm ${CMAKE_CURRENT_BINARY_DIR}/${rootpath}/${filestem}-${outformat}.asm)
    else() # input in the output directory
        set(output_directory ${rootpath})
        set(output_asm ${rootpath}/${filestem}.asm)
    endif()

    set(dbg_definition "")
    if(CMAKE_BUILD_TYPE MATCHES "Debug|Sanitize")
        set(dbg_definition "-DDBG=1")
    endif()

    if(outformat STREQUAL gas)
        # assume gas => GCC compatible C compiler
        # On macOS, add -Wno-unicode to suppress warnings about incomplete universal character names
        # which can occur in assembly comments and are harmless
        set(extra_flags "")
        if(APPLE)
            set(extra_flags "-Wno-unicode")
        endif()
        add_custom_command(
            OUTPUT ${output_asm}
            COMMAND "${CMAKE_C_COMPILER}" -E -P -x c ${filepath} -o ${output_asm}
                -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/${rootpath} -I${SYMCRYPT_SOURCE_DIR}/inc
                -DSYMCRYPT_GAS -DSYMCRYPT_CPU_${archdefineupper} ${dbg_definition} ${extra_flags}
            MAIN_DEPENDENCY ${filepath}
            DEPENDS ${SYMCRYPT_SOURCE_DIR}/inc/C_asm_shared.inc ${filepath} symcryptasm_shared.cppasm
            COMMENT "C preprocessing ${filepath} to ${outformat} (${output_asm})"
            VERBATIM)
    elseif((outformat STREQUAL masm) OR (outformat STREQUAL armasm64))
        # assume masm / armasm64 => MSVC C compiler
        add_custom_command(
            OUTPUT ${output_asm}
            COMMAND "${CMAKE_C_COMPILER}" /nologo /EP /P /Fi${output_asm} ${filepath}
                -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/${rootpath} -I${SYMCRYPT_SOURCE_DIR}/inc
                -DSYMCRYPT_MASM -DSYMCRYPT_CPU_${archdefineupper} ${dbg_definition}
            MAIN_DEPENDENCY ${filepath}
            DEPENDS ${SYMCRYPT_SOURCE_DIR}/inc/C_asm_shared.inc ${filepath} symcryptasm_shared.cppasm
            COMMENT "C preprocessing ${filepath} to ${outformat} (${output_asm})"
            VERBATIM)
    endif()
endfunction()

function(process_symcryptasm filepath outformat archdefine callingconvention)
    get_filename_component(fileextension ${filepath} EXT)
    if(NOT fileextension STREQUAL .symcryptasm)
        message(FATAL_ERROR "symcryptasm processing invoked on file with incorrect extension (${filepath} -> ${fileextension})")
    endif()
    if((NOT outformat STREQUAL gas) AND (NOT outformat STREQUAL masm) AND (NOT outformat STREQUAL armasm64))
        message(FATAL_ERROR "symcryptasm processing invoked with unrecognized outformat (${outformat})")
    endif()
    if((NOT archdefine STREQUAL amd64) AND (NOT archdefine STREQUAL x86) AND (NOT archdefine STREQUAL arm64) AND (NOT archdefine STREQUAL arm))
        message(FATAL_ERROR "symcryptasm processing invoked with unrecognized archdefine (${archdefine})")
    endif()
    if((NOT callingconvention STREQUAL msft) AND (NOT callingconvention STREQUAL systemv) AND (NOT callingconvention STREQUAL aapcs64) AND (NOT callingconvention STREQUAL arm64ec) AND (NOT callingconvention STREQUAL aapcs32))
        message(FATAL_ERROR "symcryptasm processing invoked with unrecognized callingconvention (${callingconvention})")
    endif()
    get_filename_component(rootpath ${filepath} DIRECTORY)
    get_filename_component(filestem ${filepath} NAME_WE) # filestem is the filename w/out extension
    set(filepath ${CMAKE_CURRENT_SOURCE_DIR}/${filepath})
    set(output_directory ${CMAKE_CURRENT_BINARY_DIR}/${rootpath})
    set(output_cppasm ${output_directory}/${filestem}-${outformat}.cppasm)

    # Use gas-macho assembler for macOS (Mach-O object format)
    if(APPLE AND outformat STREQUAL gas)
        set(assembler_format "gas-macho")
    else()
        set(assembler_format ${outformat})
    endif()

    add_custom_command(
        OUTPUT ${output_cppasm}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${output_directory}
        COMMAND ${Python3_EXECUTABLE} ${SYMCRYPT_SOURCE_DIR}/scripts/symcryptasm_processor.py ${assembler_format} ${archdefine} ${callingconvention} ${filepath} ${output_cppasm}
        MAIN_DEPENDENCY ${filepath}
        DEPENDS ${SYMCRYPT_SOURCE_DIR}/scripts/symcryptasm_processor.py
        COMMENT "Python preprocessing ${filepath} to ${assembler_format} (${output_cppasm})"
        VERBATIM)

    process_cppasm(${output_cppasm} ${outformat} ${archdefine})
endfunction()

if(WIN32)
    list(APPEND SOURCES_COMMON IEEE802_11SaeCustom.c)
else()
    if(SYMCRYPT_TARGET_ARCH MATCHES "AMD64|X86")
        list(APPEND SOURCES_COMMON linux/intrinsics.c)
    endif()
endif()

if(WIN32 AND SYMCRYPT_USE_ASM)
    if(SYMCRYPT_TARGET_ARCH MATCHES "AMD64")
        process_symcryptasm(amd64/aesasm.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/fdef_asm.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/fdef369_asm.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/fdef_mulx.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/wipe.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/sha256xmm_asm.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/sha256ymm_asm.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/sha512ymm_asm.symcryptasm masm amd64 msft)
        process_symcryptasm(amd64/sha512ymm_avx512vl_asm.symcryptasm masm amd64 msft)

        list(APPEND SOURCES_COMMON
            amd64/aesasm-masm.asm
            amd64/fdef_asm-masm.asm
            amd64/fdef369_asm-masm.asm
            amd64/fdef_mulx-masm.asm
            amd64/wipe-masm.asm
            amd64/sha256xmm_asm-masm.asm
            amd64/sha256ymm_asm-masm.asm
            amd64/sha512ymm_asm-masm.asm
            amd64/sha512ymm_avx512vl_asm-masm.asm)
        set_source_files_properties(
            amd64/aesasm-masm.asm
            amd64/fdef_asm-masm.asm
            amd64/fdef369_asm-masm.asm
            amd64/fdef_mulx-masm.asm
            amd64/wipe-masm.asm
            amd64/sha256xmm_asm-masm.asm
            amd64/sha256ymm_asm-masm.asm
            amd64/sha512ymm_asm-masm.asm
            amd64/sha512ymm_avx512vl_asm-masm.asm
            PROPERTIES LANGUAGE ASM_MASM)

    elseif(SYMCRYPT_TARGET_ARCH MATCHES "X86")
        process_cppasm(i386/aesasm.cppasm masm x86)
        process_cppasm(i386/fdef_asm.cppasm masm x86)

        set(CMAKE_ASMMASM_FLAGS "${CMAKE_ASMMASM_FLAGS} /safeseh")

        list(APPEND SOURCES_COMMON
            i386/aesasm-masm.asm
            i386/fdef_asm-masm.asm
            i386/wipe.asm)
        set_source_files_properties(
            i386/aesasm-masm.asm
            i386/fdef_asm-masm.asm
            i386/wipe.asm
            PROPERTIES
            LANGUAGE ASM_MASM
            COMPILE_OPTIONS "/safeseh")
        set_source_files_properties(
            i386/fdef_asm-masm.asm PROPERTIES INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/i386)
    elseif(SYMCRYPT_TARGET_ARCH STREQUAL "ARM64")
        # TODO: Investigate supporting Arm64X with CMake Windows build
        process_symcryptasm(arm64/fdef_asm.symcryptasm armasm64 arm64 aapcs64)
        process_symcryptasm(arm64/fdef369_asm.symcryptasm armasm64 arm64 aapcs64)
        process_symcryptasm(arm64/wipe.symcryptasm armasm64 arm64 aapcs64)

        list(APPEND SOURCES_COMMON
            arm64/fdef_asm-armasm64.asm
            arm64/fdef369_asm-armasm64.asm
            arm64/wipe-armasm64.asm)
        set_source_files_properties(
            arm64/fdef_asm-armasm64.asm
            arm64/fdef369_asm-armasm64.asm
            arm64/wipe-armasm64.asm
            PROPERTIES LANGUAGE ASM_MARMASM)
    endif()
elseif(NOT WIN32 AND SYMCRYPT_USE_ASM)
    if(SYMCRYPT_TARGET_ARCH MATCHES "AMD64")
        process_symcryptasm(amd64/aesasm.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/fdef_asm.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/fdef369_asm.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/fdef_mulx.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/wipe.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/sha256xmm_asm.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/sha256ymm_asm.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/sha512ymm_asm.symcryptasm gas amd64 systemv)
        process_symcryptasm(amd64/sha512ymm_avx512vl_asm.symcryptasm gas amd64 systemv)

        list(APPEND SOURCES_COMMON
            amd64/aesasm-gas.asm
            amd64/fdef_asm-gas.asm
            amd64/fdef369_asm-gas.asm
            amd64/fdef_mulx-gas.asm
            amd64/wipe-gas.asm
            amd64/sha256xmm_asm-gas.asm
            amd64/sha256ymm_asm-gas.asm
            amd64/sha512ymm_asm-gas.asm
            amd64/sha512ymm_avx512vl_asm-gas.asm)
        set_source_files_properties(
            amd64/aesasm-gas.asm
            amd64/fdef_asm-gas.asm
            amd64/fdef369_asm-gas.asm
            amd64/fdef_mulx-gas.asm
            amd64/wipe-gas.asm
            amd64/sha256xmm_asm-gas.asm
            amd64/sha256ymm_asm-gas.asm
            amd64/sha512ymm_asm-gas.asm
            amd64/sha512ymm_avx512vl_asm-gas.asm
            PROPERTIES LANGUAGE ASM)
    elseif(SYMCRYPT_TARGET_ARCH STREQUAL "ARM64")
        process_symcryptasm(arm64/fdef_asm.symcryptasm gas arm64 aapcs64)
        process_symcryptasm(arm64/fdef369_asm.symcryptasm gas arm64 aapcs64)
        process_symcryptasm(arm64/wipe.symcryptasm gas arm64 aapcs64)

        list(APPEND SOURCES_COMMON
            arm64/fdef_asm-gas.asm
            arm64/fdef369_asm-gas.asm
            arm64/wipe-gas.asm)
        set_source_files_properties(
            arm64/fdef_asm-gas.asm
            arm64/fdef369_asm-gas.asm
            arm64/wipe-gas.asm
            PROPERTIES LANGUAGE ASM)
    elseif(SYMCRYPT_TARGET_ARCH STREQUAL "ARM")
        process_symcryptasm(arm/aesasm.symcryptasm gas arm aapcs32)
        process_symcryptasm(arm/fdef_asm.symcryptasm gas arm aapcs32)
        process_symcryptasm(arm/wipe.symcryptasm gas arm aapcs32)

        list(APPEND SOURCES_COMMON
            arm/aesasm-gas.asm
            arm/fdef_asm-gas.asm
            arm/wipe-gas.asm)
        set_source_files_properties(
            arm/aesasm-gas.asm
            arm/fdef_asm-gas.asm
            arm/wipe-gas.asm
            PROPERTIES LANGUAGE ASM)
        set_source_files_properties(
            arm/fdef_asm-gas.asm PROPERTIES INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/arm)
    endif()
endif()

if(SYMCRYPT_TARGET_ARCH MATCHES "AMD64|X86")
    if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        set_source_files_properties(aes-ymm.c PROPERTIES COMPILE_OPTIONS "-mavx;-mavx2;-mvaes;-mvpclmulqdq")
        set_source_files_properties(sha256Par-ymm.c PROPERTIES COMPILE_OPTIONS "-mavx;-mavx2")
        set_source_files_properties(sha512Par-ymm.c PROPERTIES COMPILE_OPTIONS "-mavx;-mavx2")
        set_source_files_properties(sha256-xmm.c PROPERTIES COMPILE_OPTIONS "-mssse3")
        set_source_files_properties(sha256-ymm.c PROPERTIES COMPILE_OPTIONS "-mavx;-mavx2;-mbmi2")
        set_source_files_properties(sha512-ymm.c PROPERTIES COMPILE_OPTIONS "-mavx;-mavx2;-mbmi2")
    endif()
endif()

if(WIN32)
    # Disable run-time type information for SymCrypt library - we do not use it
    add_compile_options($<$<COMPILE_LANGUAGE:C,CXX>:/GR->)

    add_library(symcrypt_usermodewin7 STATIC env_windowsUserModeWin7.c)
    set_target_properties(symcrypt_usermodewin7 PROPERTIES PREFIX "")
    target_link_libraries(symcrypt_usermodewin7 symcrypt_common)

    add_library(symcrypt_usermodewin8_1 STATIC env_windowsUserModeWin8_1.c)
    set_target_properties(symcrypt_usermodewin8_1 PROPERTIES PREFIX "")
    target_link_libraries(symcrypt_usermodewin8_1 symcrypt_common)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    # Ignore the incompatible pointer types void * to PSYMCRYPT_XXX
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-Wno-incompatible-pointer-types>)
endif()

if(NOT WIN32)
    if (SYMCRYPT_STACK_PROTECTION)
        # Mitigate stack buffer overruns, and stack clash attacks
        add_compile_options(-fstack-protector-strong)
        add_compile_options(-Wstack-protector)
        add_compile_options(--param ssp-buffer-size=4)

        if(CMAKE_C_COMPILER_ID MATCHES "GNU")
            add_compile_options(-fstack-clash-protection)
        endif()
    endif()

    if(SYMCRYPT_TARGET_ARCH MATCHES "AMD64" AND
        CMAKE_C_COMPILER_ID MATCHES "Clang" AND
        NOT CMAKE_BUILD_TYPE MATCHES "Debug|Sanitize")
        # Spectre/Meltdown mitigations - these cause segfaults on AMD64 debug due to a Clang bug
        # https://github.com/llvm/llvm-project/issues/93898
        add_compile_options(-mllvm -x86-speculative-load-hardening)
    endif()

    if(SYMCRYPT_TARGET_ENV MATCHES "PosixUserMode")
        add_library(symcrypt_posixusermode STATIC env_posixUserMode.c)
        set_target_properties(symcrypt_posixusermode PROPERTIES PREFIX "")
        target_link_libraries(symcrypt_posixusermode symcrypt_common)
    elseif(SYMCRYPT_TARGET_ENV MATCHES "PosixOPTEE")
        if(SYMCRYPT_TARGET_ARCH MATCHES "ARM64")
            # Remove files from symcrypt_common that are not supported in ARM64 optee env.
            # cpuid_um.c - include auxv.h and use getauxval
            # session.c - use atomic operations (__atomic_compare_exchange)
            list(REMOVE_ITEM SOURCES_COMMON
                cpuid_um.c
                session.c)
        endif()

        add_library(symcrypt_envOpteeTa STATIC env_opteeTa.c)
        set_target_properties(symcrypt_envOpteeTa PROPERTIES PREFIX "")
        target_link_libraries(symcrypt_envOpteeTa symcrypt_common)
    endif()
endif()

include_directories(${SYMCRYPT_SOURCE_DIR}/inc)

add_library(symcrypt_common STATIC ${SOURCES_COMMON})

add_library(symcrypt_generic STATIC env_generic.c)
set_target_properties(symcrypt_generic PROPERTIES PREFIX "")
target_link_libraries(symcrypt_generic symcrypt_common)
