//
// rngforkdetection.c
// Defines fork detection functions using getpid system call and a global variable.
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include <unistd.h>

pid_t g_pid = 0;

// Sets the initial pid
VOID
SYMCRYPT_CALL
SymCryptRngForkDetectionInit(void)
{
    g_pid = getpid();
}

// Returns true if pid has changed since init or last call
BOOLEAN
SYMCRYPT_CALL
SymCryptRngForkDetect(void)
{
    BOOLEAN forkDetected = FALSE;
    pid_t currPid = getpid();

    if( currPid != g_pid )
    {
        forkDetected = TRUE;
        g_pid = currPid;
    }

    return forkDetected;
}