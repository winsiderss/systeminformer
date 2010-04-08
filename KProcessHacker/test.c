/*
 * Process Hacker Driver - 
 *   testing code
 * 
 * Copyright (C) 2009 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "include/kph.h"

static EX_PUSH_LOCK TestLock;

VOID KphpTestPushLockThreadStart(
    __in PVOID Context
    );

VOID KphTestPushLock()
{
    ULONG i;
    
    ExInitializePushLock(&TestLock);
    
    for (i = 0; i < 10; i++)
    {
        HANDLE threadHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        
        InitializeObjectAttributes(&objectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
        PsCreateSystemThread(&threadHandle, 0, &objectAttributes, NULL, NULL, KphpTestPushLockThreadStart, NULL);
    }
}

VOID KphpTestPushLockThreadStart(
    __in PVOID Context
    )
{
    ULONG i, j;
    
    for (i = 0; i < 400000; i++)
    {
        ExAcquirePushLockShared(&TestLock);
        
        for (j = 0; j < 1000; j++)
            YieldProcessor();
        
        ExReleasePushLock(&TestLock);
        
        ExAcquirePushLockExclusive(&TestLock);
        
        for (j = 0; j < 9000; j++)
            YieldProcessor();
        
        ExReleasePushLock(&TestLock);
    }
    
    PsTerminateSystemThread(STATUS_SUCCESS);
}
