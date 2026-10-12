#
# Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
#
# This file is part of System Informer.
#

# Enable ASM_MASM for assembly files
if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x64|x86|i386|i686")
    enable_language(ASM_MASM)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    enable_language(ASM_MARMASM)
endif()

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
    "pcre/pcre2_jit_char_inc.h"
    "pcre/pcre2_jit_neon_inc.h"
    "pcre/pcre2_jit_simd_inc.h"
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

# The 7-Zip wrapper and the vendored 7-Zip 26.01 sources it reads 7z/ZIP archives with.
set(SOURCES_SEVENZIP
    "7zip/si_7zip.cpp"
    "7zip/si_7zip.h"
    "7zip/C/7zBuf2.c"
    "7zip/C/7zCrc.c"
    "7zip/C/7zCrcOpt.c"
    "7zip/C/7zStream.c"
    "7zip/C/Aes.c"
    "7zip/C/AesOpt.c"
    "7zip/C/Alloc.c"
    "7zip/C/Bcj2.c"
    "7zip/C/Bcj2Enc.c"
    "7zip/C/Bra.c"
    "7zip/C/Bra86.c"
    "7zip/C/BraIA64.c"
    "7zip/C/BwtSort.c"
    "7zip/C/CpuArch.c"
    "7zip/C/Delta.c"
    "7zip/C/HuffEnc.c"
    "7zip/C/LzFind.c"
    "7zip/C/LzFindMt.c"
    "7zip/C/LzFindOpt.c"
    "7zip/C/Lzma2Dec.c"
    "7zip/C/Lzma2DecMt.c"
    "7zip/C/Lzma2Enc.c"
    "7zip/C/LzmaDec.c"
    "7zip/C/LzmaEnc.c"
    "7zip/C/Md5.c"
    "7zip/C/MtCoder.c"
    "7zip/C/MtDec.c"
    "7zip/C/Ppmd7.c"
    "7zip/C/Ppmd7Dec.c"
    "7zip/C/Ppmd7Enc.c"
    "7zip/C/Ppmd8.c"
    "7zip/C/Ppmd8Dec.c"
    "7zip/C/Ppmd8Enc.c"
    "7zip/C/Sha1.c"
    "7zip/C/Sha1Opt.c"
    "7zip/C/Sha256.c"
    "7zip/C/Sha256Opt.c"
    "7zip/C/Sha3.c"
    "7zip/C/Sha512.c"
    "7zip/C/Sha512Opt.c"
    "7zip/C/Sort.c"
    "7zip/C/SwapBytes.c"
    "7zip/C/Threads.c"
    "7zip/C/Xxh64.c"
    "7zip/C/Xz.c"
    "7zip/C/XzCrc64.c"
    "7zip/C/XzCrc64Opt.c"
    "7zip/C/XzDec.c"
    "7zip/C/XzEnc.c"
    "7zip/C/XzIn.c"
    "7zip/C/ZstdDec.c"
    "7zip/CPP/7zip/Archive/7z/7zCompressionMode.cpp"
    "7zip/CPP/7zip/Archive/7z/7zDecode.cpp"
    "7zip/CPP/7zip/Archive/7z/7zEncode.cpp"
    "7zip/CPP/7zip/Archive/7z/7zExtract.cpp"
    "7zip/CPP/7zip/Archive/7z/7zFolderInStream.cpp"
    "7zip/CPP/7zip/Archive/7z/7zHandler.cpp"
    "7zip/CPP/7zip/Archive/7z/7zHandlerOut.cpp"
    "7zip/CPP/7zip/Archive/7z/7zHeader.cpp"
    "7zip/CPP/7zip/Archive/7z/7zIn.cpp"
    "7zip/CPP/7zip/Archive/7z/7zOut.cpp"
    "7zip/CPP/7zip/Archive/7z/7zProperties.cpp"
    "7zip/CPP/7zip/Archive/7z/7zSpecStream.cpp"
    "7zip/CPP/7zip/Archive/7z/7zUpdate.cpp"
    "7zip/CPP/7zip/Archive/Common/CoderMixer2.cpp"
    "7zip/CPP/7zip/Archive/Common/DummyOutStream.cpp"
    "7zip/CPP/7zip/Archive/Common/FindSignature.cpp"
    "7zip/CPP/7zip/Archive/Common/HandlerOut.cpp"
    "7zip/CPP/7zip/Archive/Common/InStreamWithCRC.cpp"
    "7zip/CPP/7zip/Archive/Common/ItemNameUtils.cpp"
    "7zip/CPP/7zip/Archive/Common/MultiStream.cpp"
    "7zip/CPP/7zip/Archive/Common/OutStreamWithCRC.cpp"
    "7zip/CPP/7zip/Archive/Common/OutStreamWithSha1.cpp"
    "7zip/CPP/7zip/Archive/Common/ParseProperties.cpp"
    "7zip/CPP/7zip/Archive/Zip/ZipAddCommon.cpp"
    "7zip/CPP/7zip/Archive/Zip/ZipHandler.cpp"
    "7zip/CPP/7zip/Archive/Zip/ZipHandlerOut.cpp"
    "7zip/CPP/7zip/Archive/Zip/ZipIn.cpp"
    "7zip/CPP/7zip/Archive/Zip/ZipItem.cpp"
    "7zip/CPP/7zip/Archive/Zip/ZipOut.cpp"
    "7zip/CPP/7zip/Archive/Zip/ZipUpdate.cpp"
    "7zip/CPP/7zip/Common/CreateCoder.cpp"
    "7zip/CPP/7zip/Common/CWrappers.cpp"
    "7zip/CPP/7zip/Common/FilterCoder.cpp"
    "7zip/CPP/7zip/Common/InBuffer.cpp"
    "7zip/CPP/7zip/Common/InOutTempBuffer.cpp"
    "7zip/CPP/7zip/Common/LimitedStreams.cpp"
    "7zip/CPP/7zip/Common/LockedStream.cpp"
    "7zip/CPP/7zip/Common/MemBlocks.cpp"
    "7zip/CPP/7zip/Common/MethodId.cpp"
    "7zip/CPP/7zip/Common/MethodProps.cpp"
    "7zip/CPP/7zip/Common/OffsetStream.cpp"
    "7zip/CPP/7zip/Common/OutBuffer.cpp"
    "7zip/CPP/7zip/Common/OutMemStream.cpp"
    "7zip/CPP/7zip/Common/ProgressMt.cpp"
    "7zip/CPP/7zip/Common/ProgressUtils.cpp"
    "7zip/CPP/7zip/Common/PropId.cpp"
    "7zip/CPP/7zip/Common/StreamBinder.cpp"
    "7zip/CPP/7zip/Common/StreamObjects.cpp"
    "7zip/CPP/7zip/Common/StreamUtils.cpp"
    "7zip/CPP/7zip/Common/UniqBlocks.cpp"
    "7zip/CPP/7zip/Common/VirtThread.cpp"
    "7zip/CPP/7zip/Compress/Bcj2Coder.cpp"
    "7zip/CPP/7zip/Compress/BcjCoder.cpp"
    "7zip/CPP/7zip/Compress/BitlDecoder.cpp"
    "7zip/CPP/7zip/Compress/BranchMisc.cpp"
    "7zip/CPP/7zip/Compress/ByteSwap.cpp"
    "7zip/CPP/7zip/Compress/BZip2Crc.cpp"
    "7zip/CPP/7zip/Compress/BZip2Decoder.cpp"
    "7zip/CPP/7zip/Compress/BZip2Encoder.cpp"
    "7zip/CPP/7zip/Compress/BZip2Register.cpp"
    "7zip/CPP/7zip/Compress/CopyCoder.cpp"
    "7zip/CPP/7zip/Compress/CopyRegister.cpp"
    "7zip/CPP/7zip/Compress/Deflate64Register.cpp"
    "7zip/CPP/7zip/Compress/DeflateDecoder.cpp"
    "7zip/CPP/7zip/Compress/DeflateEncoder.cpp"
    "7zip/CPP/7zip/Compress/DeflateRegister.cpp"
    "7zip/CPP/7zip/Compress/DeltaFilter.cpp"
    "7zip/CPP/7zip/Compress/ImplodeDecoder.cpp"
    "7zip/CPP/7zip/Compress/ImplodeHuffmanDecoder.cpp"
    "7zip/CPP/7zip/Compress/LzfseDecoder.cpp"
    "7zip/CPP/7zip/Compress/Lzma2Decoder.cpp"
    "7zip/CPP/7zip/Compress/Lzma2Encoder.cpp"
    "7zip/CPP/7zip/Compress/Lzma2Register.cpp"
    "7zip/CPP/7zip/Compress/LzmaDecoder.cpp"
    "7zip/CPP/7zip/Compress/LzmaEncoder.cpp"
    "7zip/CPP/7zip/Compress/LzmaRegister.cpp"
    "7zip/CPP/7zip/Compress/LzOutWindow.cpp"
    "7zip/CPP/7zip/Compress/PpmdDecoder.cpp"
    "7zip/CPP/7zip/Compress/PpmdEncoder.cpp"
    "7zip/CPP/7zip/Compress/PpmdRegister.cpp"
    "7zip/CPP/7zip/Compress/PpmdZip.cpp"
    "7zip/CPP/7zip/Compress/ShrinkDecoder.cpp"
    "7zip/CPP/7zip/Compress/XzDecoder.cpp"
    "7zip/CPP/7zip/Compress/XzEncoder.cpp"
    "7zip/CPP/7zip/Compress/ZstdDecoder.cpp"
    "7zip/CPP/7zip/Crypto/7zAes.cpp"
    "7zip/CPP/7zip/Crypto/7zAesRegister.cpp"
    "7zip/CPP/7zip/Crypto/HmacSha1.cpp"
    "7zip/CPP/7zip/Crypto/HmacSha256.cpp"
    "7zip/CPP/7zip/Crypto/MyAes.cpp"
    "7zip/CPP/7zip/Crypto/MyAesReg.cpp"
    "7zip/CPP/7zip/Crypto/Pbkdf2HmacSha1.cpp"
    "7zip/CPP/7zip/Crypto/RandGen.cpp"
    "7zip/CPP/7zip/Crypto/WzAes.cpp"
    "7zip/CPP/7zip/Crypto/ZipCrypto.cpp"
    "7zip/CPP/7zip/Crypto/ZipStrong.cpp"
    "7zip/CPP/Common/7zCRC.cpp"
    "7zip/CPP/Common/CrcReg.cpp"
    "7zip/CPP/Common/DynLimBuf.cpp"
    "7zip/CPP/Common/IntToString.cpp"
    "7zip/CPP/Common/LzFindPrepare.cpp"
    "7zip/CPP/Common/Md5Reg.cpp"
    "7zip/CPP/Common/MyString.cpp"
    "7zip/CPP/Common/MyVector.cpp"
    "7zip/CPP/Common/NewHandler.cpp"
    "7zip/CPP/Common/Sha1Prepare.cpp"
    "7zip/CPP/Common/Sha1Reg.cpp"
    "7zip/CPP/Common/Sha256Prepare.cpp"
    "7zip/CPP/Common/Sha256Reg.cpp"
    "7zip/CPP/Common/Sha3Reg.cpp"
    "7zip/CPP/Common/Sha512Prepare.cpp"
    "7zip/CPP/Common/Sha512Reg.cpp"
    "7zip/CPP/Common/StringConvert.cpp"
    "7zip/CPP/Common/StringToInt.cpp"
    "7zip/CPP/Common/UTFConvert.cpp"
    "7zip/CPP/Common/Wildcard.cpp"
    "7zip/CPP/Common/Xxh64Reg.cpp"
    "7zip/CPP/Common/XzCrc64Init.cpp"
    "7zip/CPP/Common/XzCrc64Reg.cpp"
    "7zip/CPP/Windows/FileDir.cpp"
    "7zip/CPP/Windows/FileFind.cpp"
    "7zip/CPP/Windows/FileIO.cpp"
    "7zip/CPP/Windows/FileName.cpp"
    "7zip/CPP/Windows/PropVariant.cpp"
    "7zip/CPP/Windows/PropVariantConv.cpp"
    "7zip/CPP/Windows/PropVariantUtils.cpp"
    "7zip/CPP/Windows/Synchronization.cpp"
    "7zip/CPP/Windows/System.cpp"
    "7zip/CPP/Windows/TimeUtils.cpp"
)
source_group("7zip" FILES ${SOURCES_SEVENZIP})

set(SOURCES_SYMCRYPT_ASM_X64
    "SymCrypt/lib/amd64/aesasm.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/fdef_asm.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/fdef_mulx.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/fdef369_asm.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/sha256xmm_asm.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/sha256ymm_asm.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/sha512ymm_asm.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/sha512ymm_avx512vl_asm.masm.amd64.msft.asm"
    "SymCrypt/lib/amd64/wipe.masm.amd64.msft.asm"
)

set(SOURCES_SYMCRYPT_ASM_X86
    "SymCrypt/lib/i386/aesasm.asm"
    "SymCrypt/lib/i386/fdef_asm.asm"
    "SymCrypt/lib/i386/wipe.asm"
)

set(SOURCES_SYMCRYPT_ASM_ARM64
    "SymCrypt/lib/arm64/fdef_asm.armasm64.arm64.aapcs64.asm"
    "SymCrypt/lib/arm64/fdef369_asm.armasm64.arm64.aapcs64.asm"
    "SymCrypt/lib/arm64/wipe.armasm64.arm64.aapcs64.asm"
)

set(SOURCES_SYMCRYPT
    "SymCrypt/modules/windows/user/module.c"
    "SymCrypt/lib/3des.c"
    "SymCrypt/lib/aes-asm.c"
    "SymCrypt/lib/aes-c.c"
    "SymCrypt/lib/aes-default-bc.c"
    "SymCrypt/lib/aes-default.c"
    "SymCrypt/lib/aes-key.c"
    "SymCrypt/lib/aes-neon.c"
    "SymCrypt/lib/aes-selftest.c"
    "SymCrypt/lib/aes-xmm.c"
    "SymCrypt/lib/aes-ymm.c"
    "SymCrypt/lib/aescmac.c"
    "SymCrypt/lib/aesCtrDrbg.c"
    "SymCrypt/lib/aeskw.c"
    "SymCrypt/lib/AesTables.c"
    "SymCrypt/lib/a_dispatch.c"
    "SymCrypt/lib/blockciphermodes.c"
    "SymCrypt/lib/ccm.c"
    "SymCrypt/lib/chacha20.c"
    "SymCrypt/lib/chacha20_poly1305.c"
    "SymCrypt/lib/composite_encoding.c"
    "SymCrypt/lib/composite_mlkem.c"
    "SymCrypt/lib/cpuid.c"
    "SymCrypt/lib/cpuid_notry.c"
    "SymCrypt/lib/cpuid_um.c"
    "SymCrypt/lib/crt.c"
    "SymCrypt/lib/data_accessors.c"
    "SymCrypt/lib/DesTables.c"
    "SymCrypt/lib/desx.c"
    "SymCrypt/lib/dh.c"
    "SymCrypt/lib/dlgroup.c"
    "SymCrypt/lib/dlkey.c"
    "SymCrypt/lib/dl_internal_groups.c"
    "SymCrypt/lib/dsa.c"
    "SymCrypt/lib/eckey.c"
    "SymCrypt/lib/ecpoint.c"
    "SymCrypt/lib/ecurve.c"
    "SymCrypt/lib/ec_dh.c"
    "SymCrypt/lib/ec_dispatch.c"
    "SymCrypt/lib/ec_dsa.c"
    "SymCrypt/lib/ec_internal_curve_params.c"
    "SymCrypt/lib/ec_internal_curves.c"
    "SymCrypt/lib/ec_montgomery.c"
    "SymCrypt/lib/ec_mul.c"
    "SymCrypt/lib/ec_short_weierstrass.c"
    "SymCrypt/lib/ec_twisted_edwards.c"
    "SymCrypt/lib/env_generic.c"
    "SymCrypt/lib/env_windowsUserModeWin7.c"
    "SymCrypt/lib/env_windowsUserModeWin8_1.c"
    "SymCrypt/lib/equal.c"
    "SymCrypt/lib/FatalIntercept.c"
    "SymCrypt/lib/fdef369_mod.c"
    "SymCrypt/lib/fdef_general.c"
    "SymCrypt/lib/fdef_int.c"
    "SymCrypt/lib/fdef_mod.c"
    "SymCrypt/lib/fips_selftest.c"
    "SymCrypt/lib/gcm.c"
    "SymCrypt/lib/gen_int.c"
    "SymCrypt/lib/ghash.c"
    "SymCrypt/lib/hash.c"
    "SymCrypt/lib/hkdf.c"
    "SymCrypt/lib/hkdf_selftest.c"
    "SymCrypt/lib/hmac.c"
    "SymCrypt/lib/hmacmd5.c"
    "SymCrypt/lib/hmacsha1.c"
    "SymCrypt/lib/hmacsha224.c"
    "SymCrypt/lib/hmacsha256.c"
    "SymCrypt/lib/hmacsha384.c"
    "SymCrypt/lib/hmacsha512.c"
    "SymCrypt/lib/hmacsha512_224.c"
    "SymCrypt/lib/hmacsha512_256.c"
    "SymCrypt/lib/hmacsha3_224.c"
    "SymCrypt/lib/hmacsha3_256.c"
    "SymCrypt/lib/hmacsha3_384.c"
    "SymCrypt/lib/hmacsha3_512.c"
    "SymCrypt/lib/IEEE802_11SaeCustom.c"
    "SymCrypt/lib/kmac.c"
    "SymCrypt/lib/libmain.c"
    "SymCrypt/lib/lms.c"
    "SymCrypt/lib/marvin32.c"
    "SymCrypt/lib/md2.c"
    "SymCrypt/lib/md4.c"
    "SymCrypt/lib/md5.c"
    "SymCrypt/lib/mldsa.c"
    "SymCrypt/lib/mldsa_primitives.c"
    "SymCrypt/lib/mlkem.c"
    "SymCrypt/lib/mlkem_primitives.c"
    "SymCrypt/lib/modexp.c"
    "SymCrypt/lib/paddingPkcs7.c"
    "SymCrypt/lib/parhash.c"
    "SymCrypt/lib/pbkdf2.c"
    "SymCrypt/lib/pbkdf2_hmacsha1.c"
    "SymCrypt/lib/pbkdf2_hmacsha256.c"
    "SymCrypt/lib/poly1305.c"
    "SymCrypt/lib/primes.c"
    "SymCrypt/lib/rc2.c"
    "SymCrypt/lib/rc4.c"
    "SymCrypt/lib/rdrand.c"
    "SymCrypt/lib/rdseed.c"
    "SymCrypt/lib/recoding.c"
    "SymCrypt/lib/rsakey.c"
    "SymCrypt/lib/rsa_enc.c"
    "SymCrypt/lib/rsa_padding.c"
    "SymCrypt/lib/ScsTable.c"
    "SymCrypt/lib/scsTools.c"
    "SymCrypt/lib/selftest.c"
    "SymCrypt/lib/service_indicator.c"
    "SymCrypt/lib/session.c"
    "SymCrypt/lib/sha1.c"
    "SymCrypt/lib/sha256-xmm.c"
    "SymCrypt/lib/sha256-ymm.c"
    "SymCrypt/lib/sha256.c"
    "SymCrypt/lib/sha256Par-ymm.c"
    "SymCrypt/lib/sha256Par.c"
    "SymCrypt/lib/sha3.c"
    "SymCrypt/lib/sha3_224.c"
    "SymCrypt/lib/sha3_256.c"
    "SymCrypt/lib/sha3_384.c"
    "SymCrypt/lib/sha3_512.c"
    "SymCrypt/lib/sha512-ymm.c"
    "SymCrypt/lib/sha512.c"
    "SymCrypt/lib/sha512Par-ymm.c"
    "SymCrypt/lib/sha512Par.c"
    "SymCrypt/lib/shake.c"
    "SymCrypt/lib/sp800_108.c"
    "SymCrypt/lib/sp800_108_hmacsha1.c"
    "SymCrypt/lib/sp800_108_hmacsha256.c"
    "SymCrypt/lib/sp800_108_hmacsha512.c"
    "SymCrypt/lib/srtp_kdf.c"
    "SymCrypt/lib/srtp_kdf_selftest.c"
    "SymCrypt/lib/ssh_kdf.c"
    "SymCrypt/lib/ssh_kdf_sha256.c"
    "SymCrypt/lib/ssh_kdf_sha512.c"
    "SymCrypt/lib/sskdf.c"
    "SymCrypt/lib/sskdf_selftest.c"
    "SymCrypt/lib/tlsCbcVerify.c"
    "SymCrypt/lib/tlsprf.c"
    "SymCrypt/lib/tlsprf_selftest.c"
    "SymCrypt/lib/xmss.c"
    "SymCrypt/lib/xtsaes.c"
)
source_group("SymCrypt" FILES ${SOURCES_SYMCRYPT})

# Add platform-specific assembly files to SymCrypt sources
if(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x64")
    list(APPEND SOURCES_SYMCRYPT ${SOURCES_SYMCRYPT_ASM_X64})
    source_group("SymCrypt\\asm\\amd64" FILES ${SOURCES_SYMCRYPT_ASM_X64})
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i386|i686")
    list(APPEND SOURCES_SYMCRYPT ${SOURCES_SYMCRYPT_ASM_X86})
    source_group("SymCrypt\\asm\\i386" FILES ${SOURCES_SYMCRYPT_ASM_X86})
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    list(APPEND SOURCES_SYMCRYPT ${SOURCES_SYMCRYPT_ASM_ARM64})
    source_group("SymCrypt\\asm\\arm64" FILES ${SOURCES_SYMCRYPT_ASM_ARM64})
endif()

set(HEADERS_SYMCRYPT
    "SymCrypt/inc/buildInfo.h"
    "SymCrypt/inc/symcrypt.h"
    "SymCrypt/inc/symcrypt_internal.h"
    "SymCrypt/inc/symcrypt_low_level.h"
    "SymCrypt/lib/ghash_definitions.h"
    "SymCrypt/lib/precomp.h"
    "SymCrypt/lib/sc_lib.h"
    "SymCrypt/lib/sc_lib_mldsa.h"
    "SymCrypt/lib/sc_lib_mlkem.h"
    "SymCrypt/lib/smallPrimes32.h"
    "SymCrypt/lib/smallPrimes64.h"
    "SymCrypt/lib/xtsaes_definitions.h"
    "SymCrypt/modules/statusindicator_common.h"
    "SymCrypt/modules/symcrypt_modules_common.h"
)
source_group("SymCrypt\\Headers" FILES ${HEADERS_SYMCRYPT})

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
    ${SOURCES_SEVENZIP}
    ${SOURCES_SYMCRYPT}
    ${HEADERS_SYMCRYPT}
)

si_add_library(thirdparty STATIC ${ALL_FILES})

target_include_directories(thirdparty PRIVATE
    SymCrypt/inc
    SymCrypt/lib
)

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

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86|i386|i686")
    target_compile_definitions(thirdparty PRIVATE
        _X86_=1
    )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x64")
    target_compile_definitions(thirdparty PRIVATE
        _AMD64_=1
    )
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    target_compile_definitions(thirdparty PRIVATE
        _ARM64_=1
    )
endif()

if(MSVC_CLANG)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
        target_compile_definitions(thirdparty PRIVATE
            SYMCRYPT_IGNORE_PLATFORM
        )
    endif()

    target_compile_options(thirdparty PRIVATE
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-macro-redefined>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-multichar>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-reorder-ctor>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-ignored-pragmas>
        $<$<COMPILE_LANGUAGE:C,CXX>:-Wno-tautological-constant-out-of-range-compare>
    )

    if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
        target_compile_options(thirdparty PRIVATE
            $<$<COMPILE_LANGUAGE:C,CXX>:-mcx16>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mssse3>
            $<$<COMPILE_LANGUAGE:C,CXX>:-maes>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mpclmul>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mavx>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mavx2>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mbmi2>
            $<$<COMPILE_LANGUAGE:C,CXX>:-msha>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mvaes>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mvpclmulqdq>
            $<$<COMPILE_LANGUAGE:C,CXX>:-mrdseed>
        )
    endif()
endif()
