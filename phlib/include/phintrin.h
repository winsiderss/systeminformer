/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2023
 *
 */

#ifndef _PH_PHINTRIN_H
#define _PH_PHINTRIN_H

#ifdef _ARM64_
#define PhHasPopulationCount TRUE
#define PhHasIntrinsics TRUE
#else
extern int __isa_available;
extern long __isa_enabled;

#define ISA_AVAILABLE_X86 0
#define ISA_AVAILABLE_SSE2 1
#define ISA_AVAILABLE_SSE42 2
#define ISA_AVAILABLE_AVX 3
#define ISA_AVAILABLE_ENFSTRG 4
#define ISA_AVAILABLE_AVX2 5
#define ISA_AVAILABLE_AVX512 6

#define PhHasPopulationCount \
    (__isa_available >= ISA_AVAILABLE_SSE42)
#define PhHasIntrinsics \
    (__isa_available >= ISA_AVAILABLE_SSE2)
#define PhHasAVX \
    (__isa_available >= ISA_AVAILABLE_AVX2)
#endif

#include <intrin.h>

FORCEINLINE
ULONG
PhPopulationCount32(
    _In_ ULONG Value
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
    uint32_t count = 0;
    uint8x8_t input_val, count8x8_val;
    uint16x4_t count16x4_val;
    uint32x2_t count32x2_val;

    input_val = vld1_u8((uint8_t*)&Value);
    count8x8_val = vcnt_u8(input_val);
    count16x4_val = vpaddl_u8(count8x8_val);
    count32x2_val = vpaddl_u16(count16x4_val);

    vst1_u32(&count, count32x2_val);
    return count;
#else
    return (ULONG)_mm_popcnt_u32(Value);
#endif
}

#ifdef _WIN64
FORCEINLINE
ULONG
PhPopulationCount64(
    _In_ ULONG64 Value
    )
{
#ifdef _ARM64_
    // https://github.com/DLTcollab/sse2neon/blob/master/sse2neon.h
    uint64_t count = 0;
    uint8x8_t input_val, count8x8_val;
    uint16x4_t count16x4_val;
    uint32x2_t count32x2_val;
    uint64x1_t count64x1_val;

    input_val = vld1_u8((uint8_t*)&Value);
    count8x8_val = vcnt_u8(input_val);
    count16x4_val = vpaddl_u8(count8x8_val);
    count32x2_val = vpaddl_u16(count16x4_val);
    count64x1_val = vpaddl_u32(count32x2_val);
    vst1_u64(&count, count64x1_val);
    return (ULONG)count;
#else
    return (ULONG)_mm_popcnt_u64(Value);
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

FORCEINLINE
PH_INT128 
PhLoadINT128(
    _In_reads_bytes_(2 * sizeof(LONG)) PLONG Memory 
    )
{
#ifdef _ARM64_
   return vld1q_s32(Memory);
#else
    return _mm_loadu_si128((__m128i*)Memory);
#endif
}

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

FORCEINLINE
VOID
PhStoreFLOAT128(
    _Out_writes_bytes_(2 * sizeof(FLOAT)) PFLOAT Target,
    PH_FLOAT128 Value
    )
{
#ifdef _ARM64_
    vst1q_f32(Target, Value);
#else
    _mm_store_ps(Target, Value);
#endif
}

#endif
