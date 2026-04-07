/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2023
 *     dmex     2023
 *
 */

/**
 * Intrinsics and SIMD wrapper header for cross-platform support.
 *
 * This header provides a unified interface for SIMD (Single Instruction Multiple Data) operations
 * and intrinsic functions across different platforms and architectures. It abstracts platform-specific
 * differences between ARM64 (using NEON) and x86/x64 (using SSE2, SSE4.2, AVX, AVX2) instruction sets.
 *
 * Key Features:
 * - Population count (bit counting) operations for 32-bit and 64-bit integers
 * - 128-bit integer vector operations (load, store, compare, arithmetic, bitwise)
 * - 128-bit floating-point vector operations (arithmetic, conversion, manipulation)
 * - Cross-platform type definitions (PH_INT128, PH_FLOAT128)
 * - ISA (Instruction Set Architecture) feature detection for x86/x64
 *
 * Platform Support:
 * - ARM64: Uses ARM NEON intrinsics via <arm_acle.h>
 * - x86/x64: Uses SSE2, SSE4.2, AVX, AVX2 intrinsics via <intrin.h>
 *
 * Design Patterns:
 * - All functions are marked FORCEINLINE for performance-critical operations
 * - Conditional compilation (#ifdef _ARM64_) selects appropriate intrinsics per platform
 * - Operations support various element sizes: 8-bit, 16-bit, 32-bit, and 64-bit lanes
 * - Memory operations support both aligned and unaligned access variants
 *
 * \note Runtime ISA feature detection on x86/x64 uses MSCRT-provided __isa_available and __isa_enabled variables.
 * \warning Some operations require specific CPU features (SSE4.2 for popcnt, AVX2 for advanced operations).
 */

#ifndef _PH_PHINTRIN_H
#define _PH_PHINTRIN_H

#include <intrin.h>

#if defined(_M_ARM64) && defined(__clang__)
#include <arm_acle.h>
#endif

#ifdef _ARM64_
#define PhHasIntrinsics TRUE
#define PhHasPopulationCount TRUE
#define PhHasAVX FALSE
#else
/**
 * @var __isa_available
 * 
 * External variable indicating the availability of ISA (Instruction Set Architecture) features.
 * 
 * \note This is an external variable initialized by the MSCRT (Microsoft C Runtime).
 */
extern int __isa_available;
#define ISA_AVAILABLE_X86 0
#define ISA_AVAILABLE_SSE2 1
#define ISA_AVAILABLE_SSE42 2
#define ISA_AVAILABLE_AVX 3
#define ISA_AVAILABLE_ENFSTRG 4
#define ISA_AVAILABLE_AVX2 5
#define ISA_AVAILABLE_AVX512 6

/**
 * @var __isa_enabled
 *
 * External variable indicating which ISA (Instruction Set Architecture) 
 * extensions are enabled and available on the current system (e.g., SSE, AVX, etc.).
 * 
 * \note This is an external variable initialized by the MSCRT (Microsoft C Runtime).
 */
extern long __isa_enabled;
#define ISA_ENABLED_X86 0x00000001
#define ISA_ENABLED_SSE2 0x00000002
#define ISA_ENABLED_SSE42 0x00000004
#define ISA_ENABLED_AVX 0x00000008
//#define ISA_ENABLED_ENFSTRG 0x00000010
#define ISA_ENABLED_AVX2 0x00000020
#define ISA_ENABLED_AVX512 0x00000040

#ifdef _M_IX86
#define PhHasIntrinsics \
    FlagOn(__isa_enabled, ISA_ENABLED_SSE2)
#else
#define PhHasIntrinsics TRUE
#endif
#define PhHasPopulationCount \
    FlagOn(__isa_enabled, ISA_ENABLED_SSE42)
#define PhHasAVX \
    FlagOn(__isa_enabled, ISA_ENABLED_AVX2)
#define PhHasAVX512 \
    FlagOn(__isa_enabled, ISA_ENABLED_AVX512)
#endif

/**
 * Counts the number of set bits in a 32-bit unsigned integer.
 *
 * This function calculates the population count (Hamming weight) of the given
 * 32-bit value, returning the total number of '1' bits present in its binary
 * representation.
 *
 * \param[in] Value The 32-bit unsigned integer to count bits from.
 * \return The number of set bits in the input value.
 * \remarks
 * On ARM64 platforms, this function uses the native _CountOneBits intrinsic.
 * On other platforms (x86/x64), it uses the SSE4.2 _mm_popcnt_u32 intrinsic.
 * Both provide efficient hardware-accelerated bit counting when available.
 */
FORCEINLINE
ULONG
PhPopulationCount32(
    _In_ ULONG Value
    )
{
#ifdef _ARM64_
    return _CountOneBits(Value);
#else
    return (ULONG)_mm_popcnt_u32(Value);
#endif
}

#ifdef _WIN64
/**
 * Counts the number of set bits (population count) in a 64-bit unsigned integer.
 * 
 * This function calculates the Hamming weight (number of 1-bits) in the given 64-bit value.
 * It uses platform-specific intrinsics for optimal performance:
 * - On ARM64: uses _CountOneBits64() intrinsic
 * - On other platforms: uses SSE4.2 _mm_popcnt_u64() intrinsic
 * 
 * \param[in] Value The 64-bit unsigned integer to count bits in.
 * \return The number of set bits (1s) in the Value parameter.
 * \note This function requires SSE4.2 support on x86/x64 platforms or ARM64 platform.
 * \see _CountOneBits64, _mm_popcnt_u64
 */
FORCEINLINE
ULONG64
PhPopulationCount64(
    _In_ ULONG64 Value
    )
{
#ifdef _ARM64_
    return _CountOneBits64(Value);
#else
    return (ULONG64)_mm_popcnt_u64(Value);
#endif
}
#endif

#ifdef _ARM64_
typedef int64x2_t PH_INT128;
typedef float32x4_t PH_FLOAT128;
#else
typedef __m128i PH_INT128;
typedef __m128  PH_FLOAT128;
#endif

typedef PH_INT128* PPH_INT128;
typedef PH_FLOAT128* PPH_FLOAT128;

/**
 * The PhSetZeroINT128 function initializes a 128-bit integer
 * value to zero using platform-specific intrinsic functions.
 * 
 * \return A PH_INT128 value with all bits set to zero.
 */
FORCEINLINE
PH_INT128
PhSetZeroINT128(
    VOID
    )
{
#ifdef _ARM64_
    return vdupq_n_s32(0);
#else
    return _mm_setzero_si128();
#endif
}

/**
 * Loads a 128-bit integer value from an unaligned memory address.
 *
 * This function reads a 128-bit value from memory without requiring the memory
 * address to be aligned.
 *
 * \param[in] Memory Pointer to the memory location to load from. Does not need to be aligned.
 * \return A PH_INT128 value loaded from the specified memory address.
 * \remarks This is useful when the alignment of the source data cannot be guaranteed.
 */
FORCEINLINE
PH_INT128
PhLoadINT128U(
    _In_reads_bytes_(2 * sizeof(LONG)) PLONG Memory
    )
{
#ifdef _ARM64_
    return vld1q_s32(Memory);
#else
    return _mm_loadu_si128((__m128i const*)Memory);
#endif
}

/**
 * Loads a 128-bit integer value from an aligned memory address.
 *
 * \param[in] Memory Pointer to the memory location to load from. Must be 16-byte aligned.
 * \return A PH_INT128 value loaded from the specified memory address.
 * \remarks The memory address must be properly aligned for optimal performance.
 */
FORCEINLINE
PH_INT128
PhLoadINT128(
    _In_reads_bytes_(2 * sizeof(LONG)) PLONG Memory
    )
{
#ifdef _ARM64_
    return vld1q_s32(Memory);
#else
    return _mm_load_si128((__m128i const*)Memory);
#endif
}

/**
 * The PhStoreINT128 function stores a 128-bit integer value to the specified target address.
 *
 * \param[out] Target A pointer to a memory location where the 128-bit value will be stored.
 * Must be aligned to at least 16 bytes and writable for 16 bytes (2 * sizeof(LONG)).
 * \param[in] Value The 128-bit integer value to be stored.
 */
FORCEINLINE
VOID
PhStoreINT128(
    _Out_writes_bytes_(2 * sizeof(LONG)) PLONG Target,
    _In_ PH_INT128 Value
    )
{
#ifdef _ARM64_
    vst1q_s32(Target, Value);
#else
    _mm_store_si128((__m128i*)Target, Value);
#endif
}

/**
 * The PhStoreINT128U function stores a 128-bit integer value to the specified unaligned target address
 * without alignment requirements.
 * 
 * \param[out] Target A pointer to a memory location where the 128-bit value will be stored.
 * The memory does not need to be aligned and must be at least 2 * sizeof(LONG) bytes.
 * \param[in] Value The 128-bit integer value to store.
 */
FORCEINLINE
VOID
PhStoreINT128U(
    _Out_writes_bytes_(2 * sizeof(LONG)) PLONG Target,
    _In_ PH_INT128 Value
    )
{
#ifdef _ARM64_
    vst1q_s32(Target, Value);
#else
    _mm_storeu_si128((__m128i*)Target, Value);
#endif
}

/**
 * Compares two 128-bit integers element-wise for equality using 16-bit signed integer elements.
 * 
 * \param Left The left operand as a 128-bit integer.
 * \param Right The right operand as a 128-bit integer.
 * \return A 128-bit integer where each 16-bit element is set to all 1s (0xFFFF) if the 
 * corresponding elements in Left and Right are equal, or all 0s (0x0000) if they are not equal.
 */
FORCEINLINE
PH_INT128
PhCompareEqINT128by16(
    _In_ PH_INT128 Left,
    _In_ PH_INT128 Right
    )
{
#ifdef _ARM64_
    return vceqq_s16(Left, Right);
#else
    return _mm_cmpeq_epi16(Left, Right);
#endif
}

/**
 * Compares two 128-bit integer vectors element-wise for equality using 32-bit elements.
 *
 * \param[in] Left Left operand vector.
 * \param[in] Right Right operand vector.
 * \return A PH_INT128 where each 32-bit lane is 0xFFFFFFFF if equal, otherwise 0.
 */
FORCEINLINE
PH_INT128
PhCompareEqINT128by32(
    _In_ PH_INT128 Left,
    _In_ PH_INT128 Right
    )
{
#ifdef _ARM64_
    return vceqq_s32(Left, Right);
#else
    return _mm_cmpeq_epi32(Left, Right);
#endif
}

/**
 * Creates a movemask from the top bit of each 8-bit lane in a 128-bit vector.
 *
 * \param[in] Value Input vector.
 * \return A mask in the low 16 bits where bit i corresponds to the top bit of byte i.
 */
FORCEINLINE
ULONG
PhMoveMaskINT128by8(
    _In_ PH_INT128 Value
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
    uint8x16_t input = Value;
    uint16x8_t high_bits = vshrq_n_u8(input, 7);
    uint32x4_t paired16 = vsraq_n_u16(high_bits, high_bits, 7);
    uint64x2_t paired32 = vsraq_n_u32(paired16, paired16, 14);
    uint8x16_t paired64 = vsraq_n_u64(paired32, paired32, 28);
    return vgetq_lane_u8(paired64, 0) | ((int) vgetq_lane_u8(paired64, 8) << 8);
#else
    return _mm_movemask_epi8(Value);
#endif
}

/**
 * Broadcasts a single float into all lanes of a 128-bit float vector.
 *
 * \param[in] Value Float value to broadcast.
 * \return A PH_FLOAT128 containing Value in every lane.
 */
FORCEINLINE
PH_FLOAT128
PhSetFLOAT128bySingle(
    _In_ FLOAT Value
    )
{
#ifdef _ARM64_
    return vdupq_n_f32(Value);
#else
    return _mm_set1_ps(Value);
#endif
}

/**
 * Broadcasts a 16-bit signed integer into all 16-bit lanes of a 128-bit vector.
 *
 * \param[in] Value 16-bit value to broadcast.
 * \return A PH_INT128 with Value replicated in each 16-bit lane.
 */
FORCEINLINE
PH_INT128
PhSetINT128by16(
    _In_ SHORT Value
    )
{
#ifdef _ARM64_
    return vdupq_n_s16(Value);
#else
    return _mm_set1_epi16(Value);
#endif
}

/**
 * Broadcasts a 32-bit signed integer into all 32-bit lanes of a 128-bit vector.
 *
 * \param[in] Value 32-bit value to broadcast.
 * \return A PH_INT128 with Value replicated in each 32-bit lane.
 */
FORCEINLINE
PH_INT128
PhSetINT128by32(
    _In_ LONG Value
    )
{
#ifdef _ARM64_
    return vdupq_n_s32(Value);
#else
    return _mm_set1_epi32(Value);
#endif
}

/**
 * Broadcasts a 32-bit unsigned integer into all 32-bit lanes of a 128-bit vector.
 *
 * \param[in] Value 32-bit unsigned value to broadcast.
 * \return A PH_INT128 with Value replicated in each 32-bit lane.
 */
FORCEINLINE
PH_INT128
PhSetUINT128by32(
    _In_ ULONG Value
    )
{
#ifdef _ARM64_
    return vdupq_n_u32(Value);
#else
    return _mm_set1_epi32(Value);
#endif
}

/**
 * Broadcasts a single float by loading it as a replicated scalar into a 128-bit vector.
 *
 * \param[in] Value Float value to replicate.
 * \return A PH_FLOAT128 with Value in every lane.
 */
FORCEINLINE
PH_FLOAT128
PhSetFLOAT128by32(
    _In_ FLOAT Value
    )
{
#ifdef _ARM64_
    return vld1q_dup_f32(&Value);
#else
    return _mm_load_ps1(&Value);
#endif
}

/**
 * Loads four floats from memory into a 128-bit float vector.
 *
 * \param[in] Memory Pointer to 4 floats (must be readable).
 * \return A PH_FLOAT128 loaded from Memory.
 */
FORCEINLINE
PH_FLOAT128
PhLoadFLOAT128(
    _In_reads_bytes_(2 * sizeof(PFLOAT)) PFLOAT Memory
    )
{
#ifdef _ARM64_
    return vld1q_f32(Memory);
#else
    return _mm_load_ps(Memory);
#endif
}

/**
 * Adds two 128-bit float vectors lane-wise.
 *
 * \param[in] A First addend vector.
 * \param[in] B Second addend vector.
 * \return The lane-wise sum vector.
 */
FORCEINLINE
PH_FLOAT128
PhAddFLOAT128(
    _In_ PH_FLOAT128 A,
    _In_ PH_FLOAT128 B
    )
{
#ifdef _ARM64_
    return vaddq_f32(A, B);
#else
    return _mm_add_ps(A, B);
#endif
}

/**
 * Divides one 128-bit floating-point vector by another.
 *
 * \param Divisor The dividend vector to be divided.
 * \param Dividend The divisor vector used to divide the Divisor.
 * \return A PH_FLOAT128 containing the lane-wise results of the division.
 */
FORCEINLINE
PH_FLOAT128
PhDivideFLOAT128(
    _In_ PH_FLOAT128 Divisor,
    _In_ PH_FLOAT128 Dividend
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
    float32x4_t recip = vrecpeq_f32(Dividend);
    recip = vmulq_f32(recip, vrecpsq_f32(recip, Dividend));
    recip = vmulq_f32(recip, vrecpsq_f32(recip, Dividend));
    return vmulq_f32(Divisor, recip);
#else
    return _mm_div_ps(Divisor, Dividend);
#endif
}

/**
 * Returns the lane-wise maximum of two 128-bit float vectors.
 *
 * \param[in] A First vector.
 * \param[in] B Second vector.
 * \return A vector containing the maximum of A and B per lane.
 */
FORCEINLINE
PH_FLOAT128
PhMaxFLOAT128(
    _In_ PH_FLOAT128 A,
    _In_ PH_FLOAT128 B
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
#if SSE2NEON_PRECISE_MINMAX
    float32x4_t _a = A;
    float32x4_t _b = B;
    return vbslq_f32(vcgtq_f32(_a, _b), _a, _b);
#else
    return vmaxq_f32(A, B);
#endif
#else
    return _mm_max_ps(A, B);
#endif
}

/**
 * Multiplies two 128-bit float vectors lane-wise.
 *
 * \param[in] A First multiplicand vector.
 * \param[in] B Second multiplicand vector.
 * \return The lane-wise product vector.
 */
FORCEINLINE
PH_FLOAT128
PhMultiplyFLOAT128(
    _In_ PH_FLOAT128 A,
    _In_ PH_FLOAT128 B
    )
{
#ifdef _ARM64_
    return vmulq_f32(A, B);
#else
    return _mm_mul_ps(A, B);
#endif
}

/**
 * Stores four floats from a 128-bit vector to memory.
 *
 * \param[out] Target Destination pointer to 4 floats (writable).
 * \param[in] Value Vector to store.
 */
FORCEINLINE
VOID
PhStoreFLOAT128(
    _Out_writes_bytes_(2 * sizeof(FLOAT)) PFLOAT Target,
    _In_ PH_FLOAT128 Value
    )
{
#ifdef _ARM64_
    vst1q_f32(Target, Value);
#else
    _mm_store_ps(Target, Value);
#endif
}

/**
 * Stores the lowest single float lane of a 128-bit vector to memory.
 *
 * \param[out] Target Destination pointer to a float.
 * \param[in] Value Vector containing the value to store in lane 0.
 */
FORCEINLINE
VOID
PhStoreFLOAT128LowSingle(
    _Out_writes_bytes_(2 * sizeof(FLOAT)) PFLOAT Target,
    _In_ PH_FLOAT128 Value
    )
{
#ifdef _ARM64_
    vst1q_lane_f32(Target, Value, 0);
#else
    _mm_store_ss(Target, Value);
#endif
}

/**
 * Returns a 128-bit float vector set to zero.
 *
 * \return A PH_FLOAT128 with all lanes set to 0.0f.
 */
FORCEINLINE
PH_FLOAT128
PhZeroFLOAT128(
    VOID
    )
{
#ifdef _ARM64_
    return vdupq_n_f32(0);
#else
    return _mm_setzero_ps();
#endif
}

#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | ((fp0)))
#endif

/**
 * Shuffle/combine lanes from two 128-bit float vectors in the 2,1,0,3 order.
 *
 * \param[in] A First input vector.
 * \param[in] B Second input vector.
 * \return A shuffled PH_FLOAT128 composed of selected lanes from A and B.
 */
FORCEINLINE
PH_FLOAT128
PhShuffleFLOAT128_2103(
    _In_ PH_FLOAT128 A,
    _In_ PH_FLOAT128 B
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
    float32x2_t a03 = vget_low_f32(vextq_f32(A, A, 3));
    float32x2_t b21 = vget_high_f32(vextq_f32(B, B, 3));
    return vcombine_f32(a03, b21);
#else
    return _mm_shuffle_ps(A, B, _MM_SHUFFLE(2, 1, 0, 3));
#endif
}

/**
 * Shuffle/combine lanes from two 128-bit float vectors in the 2,3,0,1 order.
 *
 * \param[in] A First input vector.
 * \param[in] B Second input vector.
 * \return A shuffled PH_FLOAT128 composed of selected lanes from A and B.
 */
FORCEINLINE
PH_FLOAT128
PhShuffleFLOAT128_2301(
    _In_ PH_FLOAT128 A,
    _In_ PH_FLOAT128 B
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
    float32x2_t a03 = vget_low_f32(vextq_f32(A, A, 3));
    float32x2_t b21 = vget_high_f32(vextq_f32(B, B, 3));
    return vcombine_f32(a03, b21);
#else
    return _mm_shuffle_ps(A, B, _MM_SHUFFLE(2, 3, 0, 1));
#endif
}

/**
 * Shuffle/combine lanes from two 128-bit float vectors in the 1,0,3,2 order.
 *
 * \param[in] A First input vector.
 * \param[in] B Second input vector.
 * \return A shuffled PH_FLOAT128 composed of selected lanes from A and B.
 */
FORCEINLINE
PH_FLOAT128
PhShuffleFLOAT128_1032(
    _In_ PH_FLOAT128 A,
    _In_ PH_FLOAT128 B
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
    float32x2_t a03 = vget_low_f32(vextq_f32(A, A, 3));
    float32x2_t b21 = vget_high_f32(vextq_f32(B, B, 3));
    return vcombine_f32(a03, b21);
#else
    return _mm_shuffle_ps(A, B, _MM_SHUFFLE(1, 0, 3, 2));
#endif
}
/**
 * Shift each 32-bit lane left by a constant count.
 *
 * \param[in] A Vector to shift.
 * \param[in] Count Shift count in bits (0-255).
 * \return The shifted vector.
 */
FORCEINLINE
PH_INT128
PhShiftLeftINT128(
    _In_ PH_INT128 A,
    _In_ _In_range_(0, 255) INT32 Count
    )
{
#ifdef _ARM64_
    return vshlq_s32(A, vdupq_n_s32(Count));
#else
    return _mm_slli_epi32(A, Count);
#endif
}

/**
 * Shift each 32-bit lane right logically by a constant count.
 *
 * \param[in] A Vector to shift.
 * \param[in] Count Shift count in bits (0-255).
 * \return The shifted vector.
 */
FORCEINLINE
PH_INT128
PhShiftRightINT128(
    _In_ PH_INT128 A,
    _In_ _In_range_(0, 255) INT32 Count
    )
{
#ifdef _ARM64_
    return vshlq_u32(A, vdupq_n_s32(-Count));
#else
    return _mm_srli_epi32(A, Count);
#endif
}

/**
 * Compares two 128-bit integer vectors lane-wise treating lanes as signed 16-bit values.
 *
 * \param[in] A Left operand.
 * \param[in] B Right operand.
 * \return A mask vector with 0xFFFF in lanes where A > B, otherwise 0.
 */
FORCEINLINE
PH_INT128
PhCompareGtINT128by16(
    _In_ PH_INT128 A,
    _In_ PH_INT128 B
    )
{
#ifdef _ARM64_
    return vcgtq_s16(A, B);
#else
    return _mm_cmpgt_epi16(A, B);
#endif
}

/**
 * Adds two 128-bit integer vectors lane-wise using 16-bit signed lanes.
 *
 * \param[in] A First addend.
 * \param[in] B Second addend.
 * \return Lane-wise sum vector.
 */
FORCEINLINE
PH_INT128
PhAddINT128by16(
    _In_ PH_INT128 A,
    _In_ PH_INT128 B
    )
{
#ifdef _ARM64_
    return vaddq_s16(A, B);
#else
    return _mm_add_epi16(A, B);
#endif
}

/**
 * Subtracts two 128-bit integer vectors lane-wise using 16-bit signed lanes.
 *
 * \param[in] A Minuend.
 * \param[in] B Subtrahend.
 * \return Lane-wise difference vector.
 */
FORCEINLINE
PH_INT128
PhSubINT128by16(
    _In_ PH_INT128 A,
    _In_ PH_INT128 B
    )
{
#ifdef _ARM64_
    return vsubq_s16(A, B);
#else
    return _mm_sub_epi16(A, B);
#endif
}

/**
 * Bitwise AND of two 128-bit integer vectors.
 *
 * \param[in] A First operand.
 * \param[in] B Second operand.
 * \return Result of A & B.
 */
FORCEINLINE
PH_INT128
PhAndINT128(
    _In_ PH_INT128 A,
    _In_ PH_INT128 B
    )
{
#ifdef _ARM64_
    return vandq_s32(A, B);
#else
    return _mm_and_si128(A, B);
#endif
}

/**
 * Bitwise OR of two 128-bit integer vectors.
 *
 * \param[in] A First operand.
 * \param[in] B Second operand.
 * \return Result of A | B.
 */
FORCEINLINE
PH_INT128
PhOrINT128(
    _In_ PH_INT128 A,
    _In_ PH_INT128 B
    )
{
#ifdef _ARM64_
    return vorrq_s32(A, B);
#else
    return _mm_or_si128(A, B);
#endif
}

/**
 * Convert 128-bit float vector to unsigned 32-bit integers per lane.
 *
 * \param[in] A Float vector to convert.
 * \return A PH_INT128 containing converted unsigned 32-bit integers.
 */
FORCEINLINE
PH_INT128
PhConvertFLOAT128ToUINT128(
    _In_ PH_FLOAT128 A
    )
{
#ifdef _ARM64_
    return vcvtq_u32_f32(A);
#else
    return _mm_cvtps_epu32(A); // _mm_cvtps_epi32
#endif
}

/**
 * Convert 128-bit signed 32-bit integer vector to float per lane.
 *
 * \param[in] A Integer vector to convert.
 * \return A PH_FLOAT128 containing converted floats.
 */
FORCEINLINE
PH_FLOAT128
PhConvertINT128ToFLOAT128(
    _In_ PH_INT128 A
    )
{
#ifdef _ARM64_
    return vcvtq_f32_s32(A);
#else
    return _mm_cvtepi32_ps(A);
#endif
}

/**
 * Convert ASCII lower-case letters (a-z) to upper-case (A-Z) for 16-bit lanes.
 *
 * \param[in] Input Vector containing UTF-16 characters in 16-bit lanes.
 * \return Vector with ASCII letters converted to upper-case; other values unchanged.
 */
FORCEINLINE
PH_INT128
PhUppercaseLatin1INT128by16(
    _In_ PH_INT128 Input
    )
{
#ifdef _ARM64_
    // NEON: convert a-z (0x61-0x7A) and à-þ (0xE0-0xFE) excluding ÷ (0xF7)
    uint16x8_t ge_a = vcgeq_u16(Input, vdupq_n_u16(0x0061));
    uint16x8_t le_z = vcleq_u16(Input, vdupq_n_u16(0x007A));
    uint16x8_t mask1 = vandq_u16(ge_a, le_z);
    uint16x8_t ge_lat = vcgeq_u16(Input, vdupq_n_u16(0x00E0));
    uint16x8_t le_lat = vcleq_u16(Input, vdupq_n_u16(0x00FE));
    uint16x8_t mask2 = vandq_u16(ge_lat, le_lat);
    uint16x8_t not_div = vceqq_u16(Input, vdupq_n_u16(0x00F7));
    mask2 = vbicq_u16(mask2, not_div);
    uint16x8_t final_mask = vorrq_u16(mask1, mask2);
    return vsubq_u16(Input, vandq_u16(final_mask, vdupq_n_u16(0x0020)));
#else
    // SSE2: convert a-z (0x61-0x7A) and à-þ (0xE0-0xFE) excluding ÷ (0xF7)
    __m128i ge_a = _mm_cmpgt_epi16(Input, _mm_set1_epi16(0x0060));
    __m128i le_z = _mm_cmpgt_epi16(_mm_set1_epi16(0x007B), Input);
    __m128i mask1 = _mm_and_si128(ge_a, le_z);
    __m128i ge_lat = _mm_cmpgt_epi16(Input, _mm_set1_epi16(0x00DF));
    __m128i le_lat = _mm_cmpgt_epi16(_mm_set1_epi16(0x00FF), Input);
    __m128i mask2 = _mm_and_si128(ge_lat, le_lat);
    __m128i is_div = _mm_cmpeq_epi16(Input, _mm_set1_epi16(0x00F7));
    mask2 = _mm_andnot_si128(is_div, mask2);
    __m128i final_mask = _mm_or_si128(mask1, mask2);
    return _mm_sub_epi16(Input, _mm_and_si128(final_mask, _mm_set1_epi16(0x0020)));
#endif
}

#ifndef _ARM64_
/**
 * Convert Latin-1 lowercase letters to upper-case (AVX2).
 *
 * \param[in] Input 256-bit vector containing UTF-16 characters in 16-bit lanes.
 * \return Vector with letters converted to upper-case; other values unchanged.
 */
FORCEINLINE
__m256i
PhUppercaseLatin1INT256by16(
    _In_ __m256i Input
    )
{
    // convert a-z (0x61-0x7A) and à-þ (0xE0-0xFE) excluding ÷ (0xF7)
    __m256i ge_a = _mm256_cmpgt_epi16(Input, _mm256_set1_epi16(0x0060));
    __m256i le_z = _mm256_cmpgt_epi16(_mm256_set1_epi16(0x007B), Input);
    __m256i mask1 = _mm256_and_si256(ge_a, le_z);
    __m256i ge_lat = _mm256_cmpgt_epi16(Input, _mm256_set1_epi16(0x00DF));
    __m256i le_lat = _mm256_cmpgt_epi16(_mm256_set1_epi16(0x00FF), Input);
    __m256i mask2 = _mm256_and_si256(ge_lat, le_lat);
    __m256i is_div = _mm256_cmpeq_epi16(Input, _mm256_set1_epi16(0x00F7));
    mask2 = _mm256_andnot_si256(is_div, mask2);
    __m256i final_mask = _mm256_or_si256(mask1, mask2);
    return _mm256_sub_epi16(Input, _mm256_and_si256(final_mask, _mm256_set1_epi16(0x0020)));
}
#endif

FORCEINLINE
PH_INT128
PhUppercaseASCIIINT128by16(
    _In_ PH_INT128 Input
    )
{
#ifdef _ARM64_
    // NEON:  convert a-z to A-Z
    int16x8_t ge_a = vcgeq_s16(Input, vdupq_n_s16(L'a'));
    int16x8_t le_z = vcleq_s16(Input, vdupq_n_s16(L'z'));
    int16x8_t mask = vandq_s16(ge_a, le_z);
    return vsubq_s16(Input, vandq_s16(mask, vdupq_n_s16(0x20)));
#else
    // SSE2: convert a-z to A-Z (8 UTF-16 chars)
    __m128i ge_a = _mm_cmpgt_epi16(Input, _mm_set1_epi16(L'a' - 1));
    __m128i le_z = _mm_cmpgt_epi16(_mm_set1_epi16(L'z' + 1), Input);
    __m128i mask = _mm_and_si128(ge_a, le_z);
    return _mm_sub_epi16(Input, _mm_and_si128(mask, _mm_set1_epi16(0x20)));
#endif
}

#ifndef _ARM64_
/**
 * Convert ASCII lower-case letters (a-z) to upper-case (A-Z) for 16-bit lanes (AVX2).
 *
 * \param[in] Input 256-bit vector containing UTF-16 characters in 16-bit lanes.
 * \return Vector with ASCII letters converted to upper-case; other values unchanged.
 */
FORCEINLINE
__m256i
PhUppercaseASCIIINT256by16(
    _In_ __m256i Input
    )
{
    __m256i ge_a = _mm256_cmpgt_epi16(Input, _mm256_set1_epi16(L'a' - 1));
    __m256i le_z = _mm256_cmpgt_epi16(_mm256_set1_epi16(L'z' + 1), Input);
    __m256i mask = _mm256_and_si256(ge_a, le_z);
    return _mm256_sub_epi16(Input, _mm256_and_si256(mask, _mm256_set1_epi16(0x20)));
}

/**
 * Convert 8 unsigned 32-bit integers in a 256-bit vector to floats.
 *
 * \param[in] Value 256-bit integer vector containing 8 uint32 values.
 * \return A __m256 of floats representing the unsigned conversion of Value.
 */
FORCEINLINE
__m256
_mm256_cvtf_epu32(
    _In_ __m256i Value
    )
{
    const __m256 mul = _mm256_set1_ps(0x1.0p16f);
    const __m256i hi = _mm256_srli_epi32(Value, 16);
    const __m256i lo = _mm256_srli_epi32(_mm256_slli_epi32(Value, 16), 16);
    const __m256 fHi = _mm256_mul_ps(_mm256_cvtepi32_ps(hi), mul);
    const __m256 fLo = _mm256_cvtepi32_ps(lo);

    return _mm256_add_ps(fHi, fLo);
}
#endif

/**
 * Convert a 128-bit vector of unsigned 32-bit integers to floats accurately.
 *
 * \param[in] Value Integer vector to convert.
 * \return Float vector with converted values.
 */
FORCEINLINE
PH_FLOAT128
PhConvertUINT128ToFLOAT128(
    _In_ PH_INT128 Value
    )
{
    // https://stackoverflow.com/questions/9151711/most-efficient-way-to-convert-vector-of-uint32-to-vector-of-float

    const PH_FLOAT128 mul = PhSetFLOAT128bySingle(0x1.0p16f); // 65536

    // Avoid double rounding by doing two exact conversions of high and low 16-bit segments.

    const PH_INT128 hi = PhShiftRightINT128(Value, 16);
    const PH_INT128 lo = PhShiftRightINT128(PhShiftLeftINT128(Value, 16), 16); // PhAndINT128(Value, PhSetINT128by32(0x0000FFFF));
    const PH_FLOAT128 fHi = PhMultiplyFLOAT128(PhConvertINT128ToFLOAT128(hi), mul);
    const PH_FLOAT128 fLo = PhConvertINT128ToFLOAT128(lo);

    return PhAddFLOAT128(fHi, fLo);
}

#endif
