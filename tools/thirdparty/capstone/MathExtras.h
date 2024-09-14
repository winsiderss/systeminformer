//===-- llvm/Support/MathExtras.h - Useful math functions -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains some functions that are useful for math stuff.
//
//===----------------------------------------------------------------------===//

/* Capstone Disassembly Engine */
/* By Nguyen Anh Quynh <aquynh@gmail.com>, 2013-2015 */

#ifndef CS_LLVM_SUPPORT_MATHEXTRAS_H
#define CS_LLVM_SUPPORT_MATHEXTRAS_H

#if defined(_WIN32_WCE) && (_WIN32_WCE < 0x800)
#include "windowsce/intrin.h"
#elif defined(_MSC_VER)
#include <intrin.h>
#endif

#ifndef __cplusplus
#if defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64)
#define inline /* inline */
#endif
#endif

// NOTE: The following support functions use the _32/_64 extensions instead of
// type overloading so that signed and unsigned integers can be used without
// ambiguity.

/// Hi_32 - This function returns the high 32 bits of a 64 bit value.
static inline uint32_t Hi_32(uint64_t Value) {
	return (uint32_t)(Value >> 32);
}

/// Lo_32 - This function returns the low 32 bits of a 64 bit value.
static inline uint32_t Lo_32(uint64_t Value) {
	return (uint32_t)(Value);
}

/// isUIntN - Checks if an unsigned integer fits into the given (dynamic)
/// bit width.
static inline bool isUIntN(unsigned N, uint64_t x) {
	return x == (x & (~0ULL >> (64 - N)));
}

/// isIntN - Checks if an signed integer fits into the given (dynamic)
/// bit width.
//static inline bool isIntN(unsigned N, int64_t x) {
//  return N >= 64 || (-(INT64_C(1)<<(N-1)) <= x && x < (INT64_C(1)<<(N-1)));
//}

/// isMask_32 - This function returns true if the argument is a sequence of ones
/// starting at the least significant bit with the remainder zero (32 bit
/// version).   Ex. isMask_32(0x0000FFFFU) == true.
static inline bool isMask_32(uint32_t Value) {
	return Value && ((Value + 1) & Value) == 0;
}

/// isMask_64 - This function returns true if the argument is a sequence of ones
/// starting at the least significant bit with the remainder zero (64 bit
/// version).
static inline bool isMask_64(uint64_t Value) {
	return Value && ((Value + 1) & Value) == 0;
}

/// isShiftedMask_32 - This function returns true if the argument contains a
/// sequence of ones with the remainder zero (32 bit version.)
/// Ex. isShiftedMask_32(0x0000FF00U) == true.
static inline bool isShiftedMask_32(uint32_t Value) {
	return isMask_32((Value - 1) | Value);
}

/// isShiftedMask_64 - This function returns true if the argument contains a
/// sequence of ones with the remainder zero (64 bit version.)
static inline bool isShiftedMask_64(uint64_t Value) {
	return isMask_64((Value - 1) | Value);
}

/// isPowerOf2_32 - This function returns true if the argument is a power of
/// two > 0. Ex. isPowerOf2_32(0x00100000U) == true (32 bit edition.)
static inline bool isPowerOf2_32(uint32_t Value) {
	return Value && !(Value & (Value - 1));
}

/// CountLeadingZeros_32 - this function performs the platform optimal form of
/// counting the number of zeros from the most significant bit to the first one
/// bit.  Ex. CountLeadingZeros_32(0x00F000FF) == 8.
/// Returns 32 if the word is zero.
static inline unsigned CountLeadingZeros_32(uint32_t Value) {
	unsigned Count; // result
#if __GNUC__ >= 4
	// PowerPC is defined for __builtin_clz(0)
#if !defined(__ppc__) && !defined(__ppc64__)
	if (!Value) return 32;
#endif
	Count = __builtin_clz(Value);
#else
	unsigned Shift;
	if (!Value) return 32;
	Count = 0;
	// bisection method for count leading zeros
	for (Shift = 32 >> 1; Shift; Shift >>= 1) {
		uint32_t Tmp = Value >> Shift;
		if (Tmp) {
			Value = Tmp;
		} else {
			Count |= Shift;
		}
	}
#endif
	return Count;
}

/// CountLeadingOnes_32 - this function performs the operation of
/// counting the number of ones from the most significant bit to the first zero
/// bit.  Ex. CountLeadingOnes_32(0xFF0FFF00) == 8.
/// Returns 32 if the word is all ones.
static inline unsigned CountLeadingOnes_32(uint32_t Value) {
	return CountLeadingZeros_32(~Value);
}

/// CountLeadingZeros_64 - This function performs the platform optimal form
/// of counting the number of zeros from the most significant bit to the first
/// one bit (64 bit edition.)
/// Returns 64 if the word is zero.
static inline unsigned CountLeadingZeros_64(uint64_t Value) {
	unsigned Count; // result
#if __GNUC__ >= 4
	// PowerPC is defined for __builtin_clzll(0)
#if !defined(__ppc__) && !defined(__ppc64__)
	if (!Value) return 64;
#endif
	Count = __builtin_clzll(Value);
#else
#ifndef _MSC_VER
	unsigned Shift;
	if (sizeof(long) == sizeof(int64_t))
	{
		if (!Value) return 64;
		Count = 0;
		// bisection method for count leading zeros
		for (Shift = 64 >> 1; Shift; Shift >>= 1) {
			uint64_t Tmp = Value >> Shift;
			if (Tmp) {
				Value = Tmp;
			} else {
				Count |= Shift;
			}
		}
	}
	else
#endif
	{
		// get hi portion
		uint32_t Hi = Hi_32(Value);

		// if some bits in hi portion
		if (Hi) {
			// leading zeros in hi portion plus all bits in lo portion
			Count = CountLeadingZeros_32(Hi);
		} else {
			// get lo portion
			uint32_t Lo = Lo_32(Value);
			// same as 32 bit value
			Count = CountLeadingZeros_32(Lo)+32;
		}
	}
#endif
	return Count;
}

/// CountLeadingOnes_64 - This function performs the operation
/// of counting the number of ones from the most significant bit to the first
/// zero bit (64 bit edition.)
/// Returns 64 if the word is all ones.
static inline unsigned CountLeadingOnes_64(uint64_t Value) {
	return CountLeadingZeros_64(~Value);
}

/// CountTrailingZeros_32 - this function performs the platform optimal form of
/// counting the number of zeros from the least significant bit to the first one
/// bit.  Ex. CountTrailingZeros_32(0xFF00FF00) == 8.
/// Returns 32 if the word is zero.
static inline unsigned CountTrailingZeros_32(uint32_t Value) {
#if __GNUC__ >= 4
	return Value ? __builtin_ctz(Value) : 32;
#else
	static const unsigned Mod37BitPosition[] = {
		32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13,
		4, 7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9,
		5, 20, 8, 19, 18
	};
	// Replace "-Value" by "1+~Value" in the following commented code to avoid 
	// MSVC warning C4146
	//    return Mod37BitPosition[(-Value & Value) % 37];
	return Mod37BitPosition[((1 + ~Value) & Value) % 37];
#endif
}

/// CountTrailingOnes_32 - this function performs the operation of
/// counting the number of ones from the least significant bit to the first zero
/// bit.  Ex. CountTrailingOnes_32(0x00FF00FF) == 8.
/// Returns 32 if the word is all ones.
static inline unsigned CountTrailingOnes_32(uint32_t Value) {
	return CountTrailingZeros_32(~Value);
}

/// CountTrailingZeros_64 - This function performs the platform optimal form
/// of counting the number of zeros from the least significant bit to the first
/// one bit (64 bit edition.)
/// Returns 64 if the word is zero.
static inline unsigned CountTrailingZeros_64(uint64_t Value) {
#if __GNUC__ >= 4
	return Value ? __builtin_ctzll(Value) : 64;
#else
	static const unsigned Mod67Position[] = {
		64, 0, 1, 39, 2, 15, 40, 23, 3, 12, 16, 59, 41, 19, 24, 54,
		4, 64, 13, 10, 17, 62, 60, 28, 42, 30, 20, 51, 25, 44, 55,
		47, 5, 32, 65, 38, 14, 22, 11, 58, 18, 53, 63, 9, 61, 27,
		29, 50, 43, 46, 31, 37, 21, 57, 52, 8, 26, 49, 45, 36, 56,
		7, 48, 35, 6, 34, 33, 0
	};
	// Replace "-Value" by "1+~Value" in the following commented code to avoid 
	// MSVC warning C4146
	//    return Mod67Position[(-Value & Value) % 67];
	return Mod67Position[((1 + ~Value) & Value) % 67];
#endif
}

/// CountTrailingOnes_64 - This function performs the operation
/// of counting the number of ones from the least significant bit to the first
/// zero bit (64 bit edition.)
/// Returns 64 if the word is all ones.
static inline unsigned CountTrailingOnes_64(uint64_t Value) {
	return CountTrailingZeros_64(~Value);
}

/// CountPopulation_32 - this function counts the number of set bits in a value.
/// Ex. CountPopulation(0xF000F000) = 8
/// Returns 0 if the word is zero.
static inline unsigned CountPopulation_32(uint32_t Value) {
#if __GNUC__ >= 4
	return __builtin_popcount(Value);
#else
	uint32_t v = Value - ((Value >> 1) & 0x55555555);
	v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
	return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
}

/// CountPopulation_64 - this function counts the number of set bits in a value,
/// (64 bit edition.)
static inline unsigned CountPopulation_64(uint64_t Value) {
#if __GNUC__ >= 4
	return __builtin_popcountll(Value);
#else
	uint64_t v = Value - ((Value >> 1) & 0x5555555555555555ULL);
	v = (v & 0x3333333333333333ULL) + ((v >> 2) & 0x3333333333333333ULL);
	v = (v + (v >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	return (uint64_t)((v * 0x0101010101010101ULL) >> 56);
#endif
}

/// Log2_32 - This function returns the floor log base 2 of the specified value,
/// -1 if the value is zero. (32 bit edition.)
/// Ex. Log2_32(32) == 5, Log2_32(1) == 0, Log2_32(0) == -1, Log2_32(6) == 2
static inline unsigned Log2_32(uint32_t Value) {
	return 31 - CountLeadingZeros_32(Value);
}

/// Log2_64 - This function returns the floor log base 2 of the specified value,
/// -1 if the value is zero. (64 bit edition.)
static inline unsigned Log2_64(uint64_t Value) {
	return 63 - CountLeadingZeros_64(Value);
}

/// Log2_32_Ceil - This function returns the ceil log base 2 of the specified
/// value, 32 if the value is zero. (32 bit edition).
/// Ex. Log2_32_Ceil(32) == 5, Log2_32_Ceil(1) == 0, Log2_32_Ceil(6) == 3
static inline unsigned Log2_32_Ceil(uint32_t Value) {
	return 32-CountLeadingZeros_32(Value-1);
}

/// Log2_64_Ceil - This function returns the ceil log base 2 of the specified
/// value, 64 if the value is zero. (64 bit edition.)
static inline unsigned Log2_64_Ceil(uint64_t Value) {
	return 64-CountLeadingZeros_64(Value-1);
}

/// GreatestCommonDivisor64 - Return the greatest common divisor of the two
/// values using Euclid's algorithm.
static inline uint64_t GreatestCommonDivisor64(uint64_t A, uint64_t B) {
	while (B) {
		uint64_t T = B;
		B = A % B;
		A = T;
	}
	return A;
}

/// BitsToDouble - This function takes a 64-bit integer and returns the bit
/// equivalent double.
static inline double BitsToDouble(uint64_t Bits) {
	union {
		uint64_t L;
		double D;
	} T;
	T.L = Bits;
	return T.D;
}

/// BitsToFloat - This function takes a 32-bit integer and returns the bit
/// equivalent float.
static inline float BitsToFloat(uint32_t Bits) {
	union {
		uint32_t I;
		float F;
	} T;
	T.I = Bits;
	return T.F;
}

/// DoubleToBits - This function takes a double and returns the bit
/// equivalent 64-bit integer.  Note that copying doubles around
/// changes the bits of NaNs on some hosts, notably x86, so this
/// routine cannot be used if these bits are needed.
static inline uint64_t DoubleToBits(double Double) {
	union {
		uint64_t L;
		double D;
	} T;
	T.D = Double;
	return T.L;
}

/// FloatToBits - This function takes a float and returns the bit
/// equivalent 32-bit integer.  Note that copying floats around
/// changes the bits of NaNs on some hosts, notably x86, so this
/// routine cannot be used if these bits are needed.
static inline uint32_t FloatToBits(float Float) {
	union {
		uint32_t I;
		float F;
	} T;
	T.F = Float;
	return T.I;
}

/// MinAlign - A and B are either alignments or offsets.  Return the minimum
/// alignment that may be assumed after adding the two together.
static inline uint64_t MinAlign(uint64_t A, uint64_t B) {
	// The largest power of 2 that divides both A and B.
	//
	// Replace "-Value" by "1+~Value" in the following commented code to avoid 
	// MSVC warning C4146
	//    return (A | B) & -(A | B);
	return (A | B) & (1 + ~(A | B));
}

/// NextPowerOf2 - Returns the next power of two (in 64-bits)
/// that is strictly greater than A.  Returns zero on overflow.
static inline uint64_t NextPowerOf2(uint64_t A) {
	A |= (A >> 1);
	A |= (A >> 2);
	A |= (A >> 4);
	A |= (A >> 8);
	A |= (A >> 16);
	A |= (A >> 32);
	return A + 1;
}

/// Returns the next integer (mod 2**64) that is greater than or equal to
/// \p Value and is a multiple of \p Align. \p Align must be non-zero.
///
/// Examples:
/// \code
///   RoundUpToAlignment(5, 8) = 8
///   RoundUpToAlignment(17, 8) = 24
///   RoundUpToAlignment(~0LL, 8) = 0
/// \endcode
static inline uint64_t RoundUpToAlignment(uint64_t Value, uint64_t Align) {
	return ((Value + Align - 1) / Align) * Align;
}

/// Returns the offset to the next integer (mod 2**64) that is greater than
/// or equal to \p Value and is a multiple of \p Align. \p Align must be
/// non-zero.
static inline uint64_t OffsetToAlignment(uint64_t Value, uint64_t Align) {
	return RoundUpToAlignment(Value, Align) - Value;
}

/// abs64 - absolute value of a 64-bit int.  Not all environments support
/// "abs" on whatever their name for the 64-bit int type is.  The absolute
/// value of the largest negative number is undefined, as with "abs".
static inline int64_t abs64(int64_t x) {
	return (x < 0) ? -x : x;
}

/// \brief Sign extend number in the bottom B bits of X to a 32-bit int.
/// Requires 0 < B <= 32.
static inline int32_t SignExtend32(uint32_t X, unsigned B) {
	return (int32_t)(X << (32 - B)) >> (32 - B);
}

/// \brief Sign extend number in the bottom B bits of X to a 64-bit int.
/// Requires 0 < B <= 64.
static inline int64_t SignExtend64(uint64_t X, unsigned B) {
	return (int64_t)(X << (64 - B)) >> (64 - B);
}

/// \brief Count number of 0's from the most significant bit to the least
///   stopping at the first 1.
///
/// Only unsigned integral types are allowed.
///
/// \param ZB the behavior on an input of 0. Only ZB_Width and ZB_Undefined are
///   valid arguments.
static inline unsigned int countLeadingZeros(int x)
{
	int i;
	const unsigned bits = sizeof(x) * 8;
	unsigned count = bits;

	if (x < 0) {
		return 0;
	}
	for (i = bits; --i; ) {
		if (x == 0) break;
		count--;
		x >>= 1;
	}

	return count;
}

#endif
