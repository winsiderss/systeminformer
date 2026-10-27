//
// symcrypt_atomics.h
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//
// Portable atomic operation macros for SymCrypt.
//
// This header provides cross-platform atomic primitives that can be consumed by
// both the SymCrypt core library (lib/) and companion static libraries (lib_plus/)
// without requiring the full internal header (sc_lib.h).
//
// Prerequisites: This header requires that symcrypt.h (or symcrypt_internal.h) has
// already been included, as it depends on SYMCRYPT_MS_VC, SYMCRYPT_GNUC,
// SYMCRYPT_CPU_* macros, FORCEINLINE, and SYMCRYPT_FORCE_READ* macros.
//

#ifndef _SYMCRYPT_ATOMICS_H_
#define _SYMCRYPT_ATOMICS_H_

//
// SymCrypt atomic macros
//
// The SymCrypt atomics take the form SYMCRYPT_ATOMIC_<Operation><Bitsize>_<Return>_<Ordering>
//
// <Operation> is the atomic operation (i.e. LOAD, OR, XOR, AND, ADD, INC, etc.)
// <Bitsize> indicates the bitsize of the values that the atomic operation operates on. Pointers to
// values which atomics operate on must be aligned to the size of the value.
// <Return> takes the value PRE or POST, indicating whether the return value of the atomic is the
// value of the destination before (PRE) or after (POST) the operation was performed. Not used when
// operation is LOAD!
// <Ordering> specifies the memory ordering of the atomic operation in relation to other loads/stores
// and can take one of the following values:
//   RELAXED corresponds to relaxed memory ordering in C++11
//   SEQ_CST corresponds to sequentially consistent memory ordering in C++11
//   ACQUIRE corresponds to acquire memory ordering in C++11
//   RELEASE corresponds to release memory ordering in C++11
//

#if SYMCRYPT_MS_VC
#include <intrin.h>

#if SYMCRYPT_CPU_ARM64
// 64b loads are naturally atomic on Arm64
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           SYMCRYPT_FORCE_READ64(_dest)
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) _InterlockedAdd64_nf( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) _InterlockedAdd( (volatile LONG *)(_dest), (LONG)(_val) )

#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)__ldar64( (volatile UINT64 *)(_dest) ))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   __stlr64( (volatile UINT64 *)(_dest), (UINT64)(_val) )

// For ARM/ARM64, MSVC does not have a dedicated acquire-release CAS intrinsic.
#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#elif SYMCRYPT_CPU_ARM
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           _InterlockedOr64_nf( (volatile LONG64 *)(_dest), 0 )
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd_nf( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) _InterlockedAdd64_nf( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) _InterlockedAdd( (volatile LONG *)(_dest), (LONG)(_val) )

#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)_InterlockedOr32_acq( (volatile LONG *)(_dest), 0 ))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   _InterlockedExchangePointer_rel( (volatile PVOID *)(_dest), (PVOID)(_val) )

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#elif SYMCRYPT_CPU_AMD64
// For MSVC on AMD64, there are no _nf atomic intrinsics
// 64b loads are naturally atomic on AMD64
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           SYMCRYPT_FORCE_READ64(_dest)
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) (_InterlockedExchangeAdd64( (volatile LONG64 *)(_dest), (LONG64)(_val) ) + (LONG64)(_val))

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) (_InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) ) + (LONG)(_val))

// Volatile load / store are sufficient for acquire-release semantics on AMD64
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)SYMCRYPT_FORCE_READ64(_dest))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   SYMCRYPT_FORCE_WRITE64(_dest, ((UINT64)(_val)))

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#elif SYMCRYPT_CPU_X86
// For MSVC on x86, there is no 64b atomic load intrinsic - use expected to fail CAS, attempting to set from 0 to 0
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           _InterlockedCompareExchange64( (volatile LONG64 *)(_dest), 0, 0 )
// For MSVC on x86, there are no _nf atomic intrinsics
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) )
// For MSVC on x86, there is no 64b atomic add intrinsic
// We could use InterlockedAdd64 function from windows.h if we are using MSVC for Windows, but
// to remove dependency we just define our own inline function using _InterlockedCompareExchange64
FORCEINLINE
LONG64
SymCryptInlineInterlockedAdd64( volatile LONG64* destination, LONG64 value )
{
    LONG64 preValue;
    do {
        preValue = *destination;
    } while (_InterlockedCompareExchange64(destination, preValue + value, preValue) != preValue);

    return preValue + value;
}
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) SymCryptInlineInterlockedAdd64( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) (_InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) ) + (LONG)(_val))

// Volatile load / store are sufficient for acquire-release semantics on x86
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)SYMCRYPT_FORCE_READ32(_dest))
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   SYMCRYPT_FORCE_WRITE32(_dest, ((UINT32)(_val)))

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#else

// Fallback intended to generically work across all supported platforms for cases where
// we do not make decisions based on CPU architecture, such as no ASM builds. For the most
// part the same as x86 except in cases where the underlying definition relies on pointer size.

#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           _InterlockedCompareExchange64( (volatile LONG64 *)(_dest), 0, 0 )
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   _InterlockedOr( (volatile LONG *)(_dest), (LONG)(_val) )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  _InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) )

FORCEINLINE
LONG64
SymCryptInlineInterlockedAdd64( volatile LONG64* destination, LONG64 value )
{
    LONG64 preValue;
    do {
        preValue = *destination;
    } while (_InterlockedCompareExchange64(destination, preValue + value, preValue) != preValue);

    return preValue + value;
}
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) SymCryptInlineInterlockedAdd64( (volatile LONG64 *)(_dest), (LONG64)(_val) )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) (_InterlockedExchangeAdd( (volatile LONG *)(_dest), (LONG)(_val) ) + (LONG)(_val))

#if defined(_WIN64)
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)_InterlockedOr64( (volatile LONG64 *)(_dest), 0 ))
#else
#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          ((PVOID)_InterlockedOr( (volatile LONG *)(_dest), 0 ))
#endif

#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   _InterlockedExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_val) )

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    _InterlockedCompareExchangePointer( (volatile PVOID *)(_dest), (PVOID)(_exchange), (PVOID)(_comp) )

#endif

#elif SYMCRYPT_GNUC
#define SYMCRYPT_ATOMIC_LOAD64_RELAXED(_dest)           __atomic_load_n( (volatile uint64_t *)(_dest), __ATOMIC_RELAXED )
#define SYMCRYPT_ATOMIC_OR32_PRE_RELAXED(_dest, _val)   __atomic_fetch_or( (volatile uint32_t *)(_dest), (uint32_t)(_val), __ATOMIC_RELAXED )
#define SYMCRYPT_ATOMIC_ADD32_PRE_RELAXED(_dest, _val)  __atomic_fetch_add( (volatile uint32_t *)(_dest), (uint32_t)(_val), __ATOMIC_RELAXED )
#define SYMCRYPT_ATOMIC_ADD64_POST_RELAXED(_dest, _val) __atomic_add_fetch( (volatile uint64_t *)(_dest), (uint64_t)(_val), __ATOMIC_RELAXED )

#define SYMCRYPT_ATOMIC_ADD32_POST_SEQ_CST(_dest, _val) __atomic_add_fetch( (volatile uint32_t *)(_dest), (uint32_t)(_val), __ATOMIC_ACQ_REL )

#define SYMCRYPT_ATOMIC_LOADPTR_ACQUIRE(_dest)          __atomic_load_n( (volatile void* *)(_dest), __ATOMIC_ACQUIRE )
#define SYMCRYPT_ATOMIC_STOREPTR_RELEASE(_dest, _val)   __atomic_store_n( (volatile void* *)(_dest), (void*)(_val), __ATOMIC_RELEASE )

FORCEINLINE
void*
SymCryptAtomicCasPtrAcqRel(
    void** dest,
    void* desired,
    void* expected)
{
    __atomic_compare_exchange_n(
        dest,               // ptr
        &expected,
        desired,
        FALSE,              // weak (set to FALSE => strong)
        __ATOMIC_RELEASE,   // success_memorder
        __ATOMIC_ACQUIRE ); // failure_memorder
    return expected;
}

#define SYMCRYPT_ATOMIC_CAS_PTR_ACQUIRE_RELEASE( _dest, _exchange, _comp ) \
    SymCryptAtomicCasPtrAcqRel( (volatile void **)(_dest), (void *)(_exchange), (void *)(_comp) )

#endif


#endif // _SYMCRYPT_ATOMICS_H_
