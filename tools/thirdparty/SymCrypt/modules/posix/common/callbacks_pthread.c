//
// callbacks_pthread.c
// Contains symcrypt call back functions for pthreads
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include <pthread.h>
#include "precomp.h"

PVOID
SYMCRYPT_CALL
SymCryptCallbackAlloc( SIZE_T nBytes )
{
    // aligned_alloc requires size to be integer multiple of alignment
    SIZE_T cbAllocation = (nBytes + (SYMCRYPT_ASYM_ALIGN_VALUE - 1)) & ~(SYMCRYPT_ASYM_ALIGN_VALUE - 1);

    return aligned_alloc(SYMCRYPT_ASYM_ALIGN_VALUE, cbAllocation);
}

VOID
SYMCRYPT_CALL
SymCryptCallbackFree( VOID * pMem )
{
    free( pMem );
}

SYMCRYPT_ERROR
SYMCRYPT_CALL
SymCryptCallbackRandom( PBYTE pbBuffer, SIZE_T cbBuffer )
{
    SymCryptRandom( pbBuffer, cbBuffer );
    return SYMCRYPT_NO_ERROR;
}


PVOID
SYMCRYPT_CALL
SymCryptCallbackAllocateMutexFastInproc(void)
{
    PVOID ptr = malloc(sizeof(pthread_mutex_t));

    if( ptr )
    {
        if( pthread_mutex_init( (pthread_mutex_t *)ptr, NULL ) != 0 )
        {
            free(ptr);
            ptr = NULL;
        }
    }
    
    return ptr;
}

VOID
SYMCRYPT_CALL
SymCryptCallbackFreeMutexFastInproc( PVOID pMutex )
{
    pthread_mutex_destroy( (pthread_mutex_t *)pMutex );

    free(pMutex);
}

VOID
SYMCRYPT_CALL
SymCryptCallbackAcquireMutexFastInproc( PVOID pMutex )
{
    pthread_mutex_lock( (pthread_mutex_t *)pMutex );
}

VOID
SYMCRYPT_CALL
SymCryptCallbackReleaseMutexFastInproc( PVOID pMutex )
{
    pthread_mutex_unlock( (pthread_mutex_t *)pMutex );
}