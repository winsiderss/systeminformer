/*
 * Process Hacker - 
 *   object security data
 * 
 * Copyright (C) 2010 wj32
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

#include <phgui.h>

#define ACCESS_ENTRIES(Type) PH_ACCESS_ENTRY Ph##Type##AccessEntries[] =
#define ACCESS_ENTRY(Type, HasSynchronize) \
    { L#Type, Ph##Type##AccessEntries, sizeof(Ph##Type##AccessEntries), HasSynchronize }

typedef struct _PH_SPECIFIC_TYPE
{
    PWSTR Type;
    PPH_ACCESS_ENTRY AccessEntries;
    ULONG SizeOfAccessEntries;
    BOOLEAN HasSynchronize;
} PH_SPECIFIC_TYPE, *PPH_SPECIFIC_TYPE;

ACCESS_ENTRIES(Standard)
{
    { L"Synchronize", SYNCHRONIZE, FALSE, TRUE },
    { L"Delete", DELETE, FALSE, TRUE },
    { L"Read permissions", READ_CONTROL, FALSE, TRUE },
    { L"Change permissions", WRITE_DAC, FALSE, TRUE },
    { L"Take ownership", WRITE_OWNER, FALSE, TRUE }
};

ACCESS_ENTRIES(AlpcPort)
{
    { L"Full control", PORT_ALL_ACCESS, TRUE, TRUE },
    { L"Connect", PORT_CONNECT, TRUE, TRUE }
};

ACCESS_ENTRIES(DebugObject)
{
    { L"Full control", DEBUG_ALL_ACCESS, TRUE, TRUE },
    { L"Read events", DEBUG_READ_EVENT, TRUE, TRUE }, 
    { L"Assign processes", DEBUG_PROCESS_ASSIGN, TRUE, TRUE },
    { L"Query information", DEBUG_QUERY_INFORMATION, TRUE, TRUE },
    { L"Set information", DEBUG_SET_INFORMATION, TRUE, TRUE }
};

ACCESS_ENTRIES(Desktop)
{
    { L"Full control", DESKTOP_ALL_ACCESS, TRUE, TRUE },
    { L"Read", DESKTOP_GENERIC_READ, TRUE, FALSE },
    { L"Write", DESKTOP_GENERIC_WRITE, TRUE, FALSE },
    { L"Execute", DESKTOP_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Enumerate", DESKTOP_ENUMERATE, FALSE, TRUE },
    { L"Read objects", DESKTOP_READOBJECTS, FALSE, TRUE },
    { L"Playback journals", DESKTOP_JOURNALPLAYBACK, FALSE, TRUE },
    { L"Write objects", DESKTOP_WRITEOBJECTS, FALSE, TRUE },
    { L"Create windows", DESKTOP_CREATEWINDOW, FALSE, TRUE },
    { L"Create menus", DESKTOP_CREATEMENU, FALSE, TRUE },
    { L"Create window hooks", DESKTOP_HOOKCONTROL, FALSE, TRUE },
    { L"Record journals", DESKTOP_JOURNALRECORD, FALSE, TRUE },
    { L"Switch desktop", DESKTOP_SWITCHDESKTOP, FALSE, TRUE }
};

ACCESS_ENTRIES(Directory)
{
    { L"Full control", DIRECTORY_ALL_ACCESS, TRUE, TRUE },
    { L"Query", DIRECTORY_QUERY, TRUE, TRUE},
    { L"Traverse", DIRECTORY_TRAVERSE, TRUE, TRUE},
    { L"Create objects", DIRECTORY_CREATE_OBJECT, TRUE, TRUE},
    { L"Create subdirectories", DIRECTORY_CREATE_SUBDIRECTORY, TRUE, TRUE}
};

ACCESS_ENTRIES(Event)
{
    { L"Full control", EVENT_ALL_ACCESS, TRUE, TRUE },
    { L"Query", EVENT_QUERY_STATE, TRUE, TRUE },
    { L"Modify", EVENT_MODIFY_STATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Process)
{
    { L"Full control", PROCESS_ALL_ACCESS & ~PROCESS_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Query information", PROCESS_QUERY_INFORMATION, TRUE, TRUE },
    { L"Set information", PROCESS_SET_INFORMATION, TRUE, TRUE },
    { L"Set quotas", PROCESS_SET_QUOTA, TRUE, TRUE },
    { L"Set session ID", PROCESS_SET_SESSIONID, TRUE, TRUE },
    { L"Create threads", PROCESS_CREATE_THREAD, TRUE, TRUE },
    { L"Create processes", PROCESS_CREATE_PROCESS, TRUE, TRUE },
    { L"Modify memory", PROCESS_VM_OPERATION, TRUE, TRUE },
    { L"Read memory", PROCESS_VM_READ, TRUE, TRUE },
    { L"Write memory", PROCESS_VM_WRITE, TRUE, TRUE },
    { L"Duplicate handles", PROCESS_DUP_HANDLE, TRUE, TRUE },
    { L"Suspend / resume / set port", PROCESS_SUSPEND_RESUME, TRUE, TRUE },
    { L"Terminate", PROCESS_TERMINATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Process60)
{
    { L"Full control", PROCESS_ALL_ACCESS, TRUE, TRUE },
    { L"Query limited information", PROCESS_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Query information", PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Set information", PROCESS_SET_INFORMATION, TRUE, TRUE },
    { L"Set quotas", PROCESS_SET_QUOTA, TRUE, TRUE },
    { L"Set session ID", PROCESS_SET_SESSIONID, TRUE, TRUE },
    { L"Create threads", PROCESS_CREATE_THREAD, TRUE, TRUE },
    { L"Create processes", PROCESS_CREATE_PROCESS, TRUE, TRUE },
    { L"Modify memory", PROCESS_VM_OPERATION, TRUE, TRUE },
    { L"Read memory", PROCESS_VM_READ, TRUE, TRUE },
    { L"Write memory", PROCESS_VM_WRITE, TRUE, TRUE },
    { L"Duplicate handles", PROCESS_DUP_HANDLE, TRUE, TRUE },
    { L"Suspend / resume / set port", PROCESS_SUSPEND_RESUME, TRUE, TRUE },
    { L"Terminate", PROCESS_TERMINATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Service)
{
    { L"Full control", SERVICE_ALL_ACCESS, TRUE, TRUE },
    { L"Query status", SERVICE_QUERY_STATUS, TRUE, TRUE },
    { L"Query configuration", SERVICE_QUERY_CONFIG, TRUE, TRUE },
    { L"Modify configuration", SERVICE_CHANGE_CONFIG, TRUE, TRUE },
    { L"Enumerate dependents", SERVICE_ENUMERATE_DEPENDENTS, TRUE, TRUE },
    { L"Start", SERVICE_START, TRUE, TRUE },
    { L"Stop", SERVICE_STOP, TRUE, TRUE },
    { L"Pause / continue", SERVICE_PAUSE_CONTINUE, TRUE, TRUE },
    { L"Interrogate", SERVICE_INTERROGATE, TRUE, TRUE },
    { L"User-defined control", SERVICE_USER_DEFINED_CONTROL, TRUE, TRUE }
};

ACCESS_ENTRIES(Thread)
{
    { L"Full control", THREAD_ALL_ACCESS & ~THREAD_QUERY_LIMITED_INFORMATION & ~THREAD_SET_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Query information", THREAD_QUERY_INFORMATION, TRUE, TRUE },
    { L"Set information", THREAD_SET_INFORMATION, TRUE, TRUE },
    { L"Get context", THREAD_GET_CONTEXT, TRUE, TRUE },
    { L"Set context", THREAD_SET_CONTEXT, TRUE, TRUE },
    { L"Set token", THREAD_SET_THREAD_TOKEN, TRUE, TRUE },
    { L"Alert", THREAD_ALERT, TRUE, TRUE },
    { L"Impersonate", THREAD_IMPERSONATE, TRUE, TRUE },
    { L"Direct impersonate", THREAD_DIRECT_IMPERSONATION, TRUE, TRUE },
    { L"Suspend / resume", THREAD_SUSPEND_RESUME, TRUE, TRUE },
    { L"Terminate", THREAD_TERMINATE, TRUE, TRUE },
};

ACCESS_ENTRIES(Thread60)
{
    { L"Full control", THREAD_ALL_ACCESS, TRUE, TRUE },
    { L"Query limited information", THREAD_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Query information", THREAD_QUERY_INFORMATION, TRUE, TRUE },
    { L"Set limited information", THREAD_SET_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Set information", THREAD_SET_INFORMATION, TRUE, TRUE },
    { L"Get context", THREAD_GET_CONTEXT, TRUE, TRUE },
    { L"Set context", THREAD_SET_CONTEXT, TRUE, TRUE },
    { L"Set token", THREAD_SET_THREAD_TOKEN, TRUE, TRUE },
    { L"Alert", THREAD_ALERT, TRUE, TRUE },
    { L"Impersonate", THREAD_IMPERSONATE, TRUE, TRUE },
    { L"Direct impersonate", THREAD_DIRECT_IMPERSONATION, TRUE, TRUE },
    { L"Suspend / resume", THREAD_SUSPEND_RESUME, TRUE, TRUE },
    { L"Terminate", THREAD_TERMINATE, TRUE, TRUE },
};

PH_SPECIFIC_TYPE PhSpecificTypes[] =
{
    ACCESS_ENTRY(AlpcPort, TRUE),
    ACCESS_ENTRY(DebugObject, FALSE),
    ACCESS_ENTRY(Desktop, FALSE),
    ACCESS_ENTRY(Directory, FALSE),
    ACCESS_ENTRY(Event, TRUE),
    ACCESS_ENTRY(Process, TRUE),
    ACCESS_ENTRY(Process60, TRUE),
    ACCESS_ENTRY(Service, FALSE),
    ACCESS_ENTRY(Thread, TRUE),
    ACCESS_ENTRY(Thread60, TRUE)
};

BOOLEAN PhGetAccessEntries(
    __in PWSTR Type,
    __out PPH_ACCESS_ENTRY *AccessEntries,
    __out PULONG NumberOfAccessEntries
    )
{
    ULONG i;
    PPH_SPECIFIC_TYPE specificType = NULL;
    PPH_ACCESS_ENTRY accessEntries;

    if (WSTR_IEQUAL(Type, L"ALPC Port"))
    {
        Type = L"AlpcPort";
    }
    else if (WSTR_IEQUAL(Type, L"Port"))
    {
        Type = L"AlpcPort";
    }
    else if (WSTR_IEQUAL(Type, L"WaitablePort"))
    {
        Type = L"AlpcPort";
    }
    else if (WSTR_IEQUAL(Type, L"Process"))
    {
        if (WindowsVersion >= WINDOWS_VISTA)
            Type = L"Process60";
    }
    else if (WSTR_IEQUAL(Type, L"Thread"))
    {
        if (WindowsVersion >= WINDOWS_VISTA)
            Type = L"Thread60";
    }

    // Find the specific type.
    for (i = 0; i < sizeof(PhSpecificTypes) / sizeof(PH_SPECIFIC_TYPE); i++)
    {
        if (WSTR_IEQUAL(PhSpecificTypes[i].Type, Type))
        {
            specificType = &PhSpecificTypes[i];
            break;
        }
    }

    if (specificType)
    {
        ULONG sizeOfEntries;

        // Copy the specific access entries and append the standard access entries.

        if (specificType->HasSynchronize)
            sizeOfEntries = specificType->SizeOfAccessEntries + sizeof(PhStandardAccessEntries);
        else
            sizeOfEntries = specificType->SizeOfAccessEntries + sizeof(PhStandardAccessEntries) - sizeof(PH_ACCESS_ENTRY);

        accessEntries = PhAllocate(sizeOfEntries);
        memcpy(accessEntries, specificType->AccessEntries, specificType->SizeOfAccessEntries);

        if (specificType->HasSynchronize)
        {
            memcpy(
                PTR_ADD_OFFSET(accessEntries, specificType->SizeOfAccessEntries),
                PhStandardAccessEntries,
                sizeof(PhStandardAccessEntries)
                );
        }
        else
        {
            memcpy(
                PTR_ADD_OFFSET(accessEntries, specificType->SizeOfAccessEntries),
                &PhStandardAccessEntries[1],
                sizeof(PhStandardAccessEntries) - sizeof(PH_ACCESS_ENTRY)
                );
        }

        *AccessEntries = accessEntries;
        *NumberOfAccessEntries = sizeOfEntries / sizeof(PH_ACCESS_ENTRY);
    }
    else
    {
        accessEntries = PhAllocate(sizeof(PhStandardAccessEntries));
        memcpy(accessEntries, PhStandardAccessEntries, sizeof(PhStandardAccessEntries));

        *AccessEntries = accessEntries;
        *NumberOfAccessEntries = sizeof(PhStandardAccessEntries) / sizeof(PH_ACCESS_ENTRY);
    }

    return TRUE;
}
