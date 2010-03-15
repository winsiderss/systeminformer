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
    { L"Read permissions", READ_CONTROL, FALSE, TRUE, L"Read control" },
    { L"Change permissions", WRITE_DAC, FALSE, TRUE, L"Write DAC" },
    { L"Take ownership", WRITE_OWNER, FALSE, TRUE, L"Write owner" }
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

ACCESS_ENTRIES(EventPair)
{
    { L"Full control", EVENT_PAIR_ALL_ACCESS, TRUE, TRUE }
};

ACCESS_ENTRIES(File)
{
    { L"Full control", FILE_ALL_ACCESS, TRUE, TRUE },
    { L"Read & execute", FILE_GENERIC_READ | FILE_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Read", FILE_GENERIC_READ, TRUE, FALSE },
    { L"Write", FILE_GENERIC_WRITE, TRUE, FALSE },
    { L"Traverse folder / execute file", FILE_EXECUTE, FALSE, TRUE, L"Execute" },
    { L"List folder / read data", FILE_READ_DATA, FALSE, TRUE, L"Read data" },
    { L"Read attributes", FILE_READ_ATTRIBUTES, FALSE, TRUE },
    { L"Read extended attributes", FILE_READ_EA, FALSE, TRUE, L"Read EA" },
    { L"Create files / write data", FILE_WRITE_DATA, FALSE, TRUE, L"Write data" },
    { L"Create folders / append data", FILE_APPEND_DATA, FALSE, TRUE, L"Append data" },
    { L"Write attributes", FILE_WRITE_ATTRIBUTES, FALSE, TRUE },
    { L"Write extended attributes", FILE_WRITE_EA, FALSE, TRUE, L"Write EA" },
    { L"Delete subfolders and files", FILE_DELETE_CHILD, FALSE, TRUE, L"Delete child" }
};

ACCESS_ENTRIES(FilterConnectionPort)
{
    { L"Full control", FLT_PORT_ALL_ACCESS, TRUE, TRUE },
    { L"Connect", FLT_PORT_CONNECT, TRUE, TRUE }
};

ACCESS_ENTRIES(IoCompletion)
{
    { L"Full control", IO_COMPLETION_ALL_ACCESS, TRUE, TRUE },
    { L"Query", IO_COMPLETION_QUERY_STATE, TRUE, TRUE },
    { L"Modify", IO_COMPLETION_MODIFY_STATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Job)
{
    { L"Full control", JOB_OBJECT_ALL_ACCESS, TRUE, TRUE },
    { L"Query", JOB_OBJECT_QUERY, TRUE, TRUE },
    { L"Assign processes", JOB_OBJECT_ASSIGN_PROCESS, TRUE, TRUE },
    { L"Set attributes", JOB_OBJECT_SET_ATTRIBUTES, TRUE, TRUE },
    { L"Set security attributes", JOB_OBJECT_SET_SECURITY_ATTRIBUTES, TRUE, TRUE },
    { L"Terminate", JOB_OBJECT_TERMINATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Key)
{
    { L"Full control", KEY_ALL_ACCESS, TRUE, TRUE },
    { L"Read", KEY_READ, TRUE, FALSE },
    { L"Write", KEY_WRITE, TRUE, FALSE },
    { L"Execute", KEY_EXECUTE, TRUE, FALSE },
    { L"Enumerate subkeys", KEY_ENUMERATE_SUB_KEYS, FALSE, TRUE },
    { L"Query values", KEY_QUERY_VALUE, FALSE, TRUE },
    { L"Notify", KEY_NOTIFY, FALSE, TRUE },
    { L"Set values", KEY_SET_VALUE, FALSE, TRUE },
    { L"Create subkeys", KEY_CREATE_SUB_KEY, FALSE, TRUE },
    { L"Create links", KEY_CREATE_LINK, FALSE, TRUE }
};

ACCESS_ENTRIES(KeyedEvent)
{
    { L"Full control", KEYEDEVENT_ALL_ACCESS, TRUE, TRUE },
    { L"Wait", KEYEDEVENT_WAIT, TRUE, TRUE },
    { L"Wake", KEYEDEVENT_WAKE, TRUE, TRUE }
};

ACCESS_ENTRIES(Mutant)
{
    { L"Full control", MUTANT_ALL_ACCESS, TRUE, TRUE },
    { L"Query", MUTANT_QUERY_STATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Process)
{
    { L"Full control", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xfff, TRUE, TRUE },
    { L"Query information", PROCESS_QUERY_INFORMATION, TRUE, TRUE },
    { L"Set information", PROCESS_SET_INFORMATION, TRUE, TRUE },
    { L"Set quotas", PROCESS_SET_QUOTA, TRUE, TRUE },
    { L"Set session ID", PROCESS_SET_SESSIONID, TRUE, TRUE },
    { L"Create threads", PROCESS_CREATE_THREAD, TRUE, TRUE },
    { L"Create processes", PROCESS_CREATE_PROCESS, TRUE, TRUE },
    { L"Modify memory", PROCESS_VM_OPERATION, TRUE, TRUE, L"VM operation" },
    { L"Read memory", PROCESS_VM_READ, TRUE, TRUE, L"VM read" },
    { L"Write memory", PROCESS_VM_WRITE, TRUE, TRUE, L"VM write" },
    { L"Duplicate handles", PROCESS_DUP_HANDLE, TRUE, TRUE },
    { L"Suspend / resume / set port", PROCESS_SUSPEND_RESUME, TRUE, TRUE, L"Suspend/resume" },
    { L"Terminate", PROCESS_TERMINATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Process60)
{
    { L"Full control", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xffff, TRUE, TRUE }, // PROCESS_ALL_ACCESS
    { L"Query limited information", PROCESS_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Query information", PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Set information", PROCESS_SET_INFORMATION, TRUE, TRUE },
    { L"Set quotas", PROCESS_SET_QUOTA, TRUE, TRUE },
    { L"Set session ID", PROCESS_SET_SESSIONID, TRUE, TRUE },
    { L"Create threads", PROCESS_CREATE_THREAD, TRUE, TRUE },
    { L"Create processes", PROCESS_CREATE_PROCESS, TRUE, TRUE },
    { L"Modify memory", PROCESS_VM_OPERATION, TRUE, TRUE, L"VM operation" },
    { L"Read memory", PROCESS_VM_READ, TRUE, TRUE, L"VM read" },
    { L"Write memory", PROCESS_VM_WRITE, TRUE, TRUE, L"VM write" },
    { L"Duplicate handles", PROCESS_DUP_HANDLE, TRUE, TRUE },
    { L"Suspend / resume / set port", PROCESS_SUSPEND_RESUME, TRUE, TRUE, L"Suspend/resume" },
    { L"Terminate", PROCESS_TERMINATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Profile)
{
    { L"Full control", PROFILE_ALL_ACCESS, TRUE, TRUE },
    { L"Control", PROFILE_CONTROL, TRUE, TRUE }
};

ACCESS_ENTRIES(Section)
{
    { L"Full control", SECTION_ALL_ACCESS, TRUE, TRUE },
    { L"Query", SECTION_QUERY, TRUE, TRUE },
    { L"Map for read", SECTION_MAP_READ, TRUE, TRUE, L"Map read" },
    { L"Map for write", SECTION_MAP_WRITE, TRUE, TRUE, L"Map write" },
    { L"Map for execute", SECTION_MAP_EXECUTE, TRUE, TRUE, L"Map execute" },
    { L"Map for execute (explicit)", SECTION_MAP_EXECUTE_EXPLICIT, TRUE, TRUE, L"Map execute explicit" },
    { L"Extend size", SECTION_EXTEND_SIZE, TRUE, TRUE }
};

ACCESS_ENTRIES(Semaphore)
{
    { L"Full control", SEMAPHORE_ALL_ACCESS, TRUE, TRUE },
    { L"Query", SEMAPHORE_QUERY_STATE, TRUE, TRUE },
    { L"Modify", SEMAPHORE_MODIFY_STATE, TRUE, TRUE }
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
    { L"Pause / continue", SERVICE_PAUSE_CONTINUE, TRUE, TRUE, L"Pause/continue" },
    { L"Interrogate", SERVICE_INTERROGATE, TRUE, TRUE },
    { L"User-defined control", SERVICE_USER_DEFINED_CONTROL, TRUE, TRUE }
};

ACCESS_ENTRIES(Session)
{
    { L"Full control", SESSION_ALL_ACCESS, TRUE, TRUE },
    { L"Query", SESSION_QUERY_ACCESS, TRUE, TRUE },
    { L"Modify", SESSION_MODIFY_ACCESS, TRUE, TRUE }
};

ACCESS_ENTRIES(SymbolicLink)
{
    { L"Full control", SYMBOLIC_LINK_ALL_ACCESS, TRUE, TRUE },
    { L"Query", SYMBOLIC_LINK_QUERY, TRUE, TRUE }
};

ACCESS_ENTRIES(Thread)
{
    { L"Full control", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff, TRUE, TRUE },
    { L"Query information", THREAD_QUERY_INFORMATION, TRUE, TRUE },
    { L"Set information", THREAD_SET_INFORMATION, TRUE, TRUE },
    { L"Get context", THREAD_GET_CONTEXT, TRUE, TRUE },
    { L"Set context", THREAD_SET_CONTEXT, TRUE, TRUE },
    { L"Set token", THREAD_SET_THREAD_TOKEN, TRUE, TRUE },
    { L"Alert", THREAD_ALERT, TRUE, TRUE },
    { L"Impersonate", THREAD_IMPERSONATE, TRUE, TRUE },
    { L"Direct impersonate", THREAD_DIRECT_IMPERSONATION, TRUE, TRUE },
    { L"Suspend / resume", THREAD_SUSPEND_RESUME, TRUE, TRUE, L"Suspend/resume" },
    { L"Terminate", THREAD_TERMINATE, TRUE, TRUE },
};

ACCESS_ENTRIES(Thread60)
{
    { L"Full control", STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xffff, TRUE, TRUE }, // THREAD_ALL_ACCESS
    { L"Query limited information", THREAD_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Query information", THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Set limited information", THREAD_SET_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Set information", THREAD_SET_INFORMATION | THREAD_SET_LIMITED_INFORMATION, TRUE, TRUE },
    { L"Get context", THREAD_GET_CONTEXT, TRUE, TRUE },
    { L"Set context", THREAD_SET_CONTEXT, TRUE, TRUE },
    { L"Set token", THREAD_SET_THREAD_TOKEN, TRUE, TRUE },
    { L"Alert", THREAD_ALERT, TRUE, TRUE },
    { L"Impersonate", THREAD_IMPERSONATE, TRUE, TRUE },
    { L"Direct impersonate", THREAD_DIRECT_IMPERSONATION, TRUE, TRUE },
    { L"Suspend / resume", THREAD_SUSPEND_RESUME, TRUE, TRUE, L"Suspend/resume" },
    { L"Terminate", THREAD_TERMINATE, TRUE, TRUE },
};

ACCESS_ENTRIES(Timer)
{
    { L"Full control", TIMER_ALL_ACCESS, TRUE, TRUE },
    { L"Query", TIMER_QUERY_STATE, TRUE, TRUE },
    { L"Modify", TIMER_MODIFY_STATE, TRUE, TRUE }
};

ACCESS_ENTRIES(TmEn)
{
    { L"Full control", ENLISTMENT_ALL_ACCESS, TRUE, TRUE },
    { L"Read", ENLISTMENT_GENERIC_READ, TRUE, FALSE },
    { L"Write", ENLISTMENT_GENERIC_WRITE, TRUE, FALSE },
    { L"Execute", ENLISTMENT_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Query information", ENLISTMENT_QUERY_INFORMATION, FALSE, TRUE },
    { L"Set information", ENLISTMENT_SET_INFORMATION, FALSE, TRUE },
    { L"Recover", ENLISTMENT_RECOVER, FALSE, TRUE },
    { L"Subordinate rights", ENLISTMENT_SUBORDINATE_RIGHTS, FALSE, TRUE },
    { L"Superior rights", ENLISTMENT_SUPERIOR_RIGHTS, FALSE, TRUE }
};

ACCESS_ENTRIES(TmRm)
{
    { L"Full control", RESOURCEMANAGER_ALL_ACCESS, TRUE, TRUE },
    { L"Read", RESOURCEMANAGER_GENERIC_READ, TRUE, FALSE },
    { L"Write", RESOURCEMANAGER_GENERIC_WRITE, TRUE, FALSE },
    { L"Execute", RESOURCEMANAGER_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Query information", RESOURCEMANAGER_QUERY_INFORMATION, FALSE, TRUE },
    { L"Set information", RESOURCEMANAGER_SET_INFORMATION, FALSE, TRUE },
    { L"Get notifications", RESOURCEMANAGER_GET_NOTIFICATION, FALSE, TRUE },
    { L"Enlist", RESOURCEMANAGER_ENLIST, FALSE, TRUE },
    { L"Recover", RESOURCEMANAGER_RECOVER, FALSE, TRUE },
    { L"Register protocols", RESOURCEMANAGER_REGISTER_PROTOCOL, FALSE, TRUE },
    { L"Complete propagation", RESOURCEMANAGER_COMPLETE_PROPAGATION, FALSE, TRUE }
};

ACCESS_ENTRIES(TmTm)
{
    { L"Full control", TRANSACTIONMANAGER_ALL_ACCESS, TRUE, TRUE },
    { L"Read", TRANSACTIONMANAGER_GENERIC_READ, TRUE, FALSE },
    { L"Write", TRANSACTIONMANAGER_GENERIC_WRITE, TRUE, FALSE },
    { L"Execute", TRANSACTIONMANAGER_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Query information", TRANSACTIONMANAGER_QUERY_INFORMATION, FALSE, TRUE },
    { L"Set information", TRANSACTIONMANAGER_SET_INFORMATION, FALSE, TRUE },
    { L"Recover", TRANSACTIONMANAGER_RECOVER, FALSE, TRUE },
    { L"Rename", TRANSACTIONMANAGER_RENAME, FALSE, TRUE },
    { L"Create resource manager", TRANSACTIONMANAGER_CREATE_RM, FALSE, TRUE },
    { L"Bind transactions", TRANSACTIONMANAGER_BIND_TRANSACTION, FALSE, TRUE }
};

ACCESS_ENTRIES(TmTx)
{
    { L"Full control", TRANSACTION_ALL_ACCESS, TRUE, TRUE },
    { L"Read", TRANSACTION_GENERIC_READ, TRUE, FALSE },
    { L"Write", TRANSACTION_GENERIC_WRITE, TRUE, FALSE },
    { L"Execute", TRANSACTION_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Query information", TRANSACTION_QUERY_INFORMATION, FALSE, TRUE },
    { L"Set information", TRANSACTION_SET_INFORMATION, FALSE, TRUE },
    { L"Enlist", TRANSACTION_ENLIST, FALSE, TRUE },
    { L"Commit", TRANSACTION_COMMIT, FALSE, TRUE },
    { L"Rollback", TRANSACTION_ROLLBACK, FALSE, TRUE },
    { L"Propagate", TRANSACTION_PROPAGATE, FALSE, TRUE }
};

ACCESS_ENTRIES(Token)
{
    { L"Full control", TOKEN_ALL_ACCESS, TRUE, TRUE },
    { L"Read", TOKEN_READ, TRUE, FALSE },
    { L"Write", TOKEN_WRITE, TRUE, FALSE },
    { L"Execute", TOKEN_EXECUTE, TRUE, FALSE },
    { L"Adjust privileges", TOKEN_ADJUST_PRIVILEGES, FALSE, TRUE },
    { L"Adjust groups", TOKEN_ADJUST_GROUPS, FALSE, TRUE },
    { L"Adjust defaults", TOKEN_ADJUST_DEFAULT, FALSE, TRUE },
    { L"Adjust session ID", TOKEN_ADJUST_SESSIONID, FALSE, TRUE },
    { L"Assign as primary token", TOKEN_ASSIGN_PRIMARY, FALSE, TRUE, L"Assign primary" },
    { L"Duplicate", TOKEN_DUPLICATE, FALSE, TRUE },
    { L"Impersonate", TOKEN_IMPERSONATE, FALSE, TRUE },
    { L"Query", TOKEN_QUERY, FALSE, TRUE },
    { L"Query source", TOKEN_QUERY_SOURCE, FALSE, TRUE }
};

ACCESS_ENTRIES(Type)
{
    { L"Full control", OBJECT_TYPE_ALL_ACCESS, TRUE, TRUE },
    { L"Create", OBJECT_TYPE_CREATE, TRUE, TRUE }
};

ACCESS_ENTRIES(WindowStation)
{
    { L"Full control", WINSTA_ALL_ACCESS | STANDARD_RIGHTS_REQUIRED, TRUE, TRUE },
    { L"Read", WINSTA_GENERIC_READ, TRUE, FALSE },
    { L"Write", WINSTA_GENERIC_WRITE, TRUE, FALSE },
    { L"Execute", WINSTA_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Enumerate", WINSTA_ENUMERATE, FALSE, TRUE },
    { L"Enumerate desktops", WINSTA_ENUMDESKTOPS, FALSE, TRUE },
    { L"Read attributes", WINSTA_READATTRIBUTES, FALSE, TRUE },
    { L"Read screen", WINSTA_READSCREEN, FALSE, TRUE },
    { L"Access clipboard", WINSTA_ACCESSCLIPBOARD, FALSE, TRUE },
    { L"Access global atoms", WINSTA_ACCESSGLOBALATOMS, FALSE, TRUE },
    { L"Create desktop", WINSTA_CREATEDESKTOP, FALSE, TRUE },
    { L"Write attributes", WINSTA_WRITEATTRIBUTES, FALSE, TRUE },
    { L"Exit windows", WINSTA_EXITWINDOWS, FALSE, TRUE }
};

PH_SPECIFIC_TYPE PhSpecificTypes[] =
{
    ACCESS_ENTRY(AlpcPort, TRUE),
    ACCESS_ENTRY(DebugObject, TRUE),
    ACCESS_ENTRY(Desktop, FALSE),
    ACCESS_ENTRY(Directory, FALSE),
    ACCESS_ENTRY(Event, TRUE),
    ACCESS_ENTRY(EventPair, TRUE),
    ACCESS_ENTRY(File, TRUE),
    ACCESS_ENTRY(FilterConnectionPort, FALSE),
    ACCESS_ENTRY(IoCompletion, TRUE),
    ACCESS_ENTRY(Job, TRUE),
    ACCESS_ENTRY(Key, FALSE),
    ACCESS_ENTRY(KeyedEvent, FALSE),
    ACCESS_ENTRY(Mutant, TRUE),
    ACCESS_ENTRY(Process, TRUE),
    ACCESS_ENTRY(Process60, TRUE),
    ACCESS_ENTRY(Profile, FALSE),
    ACCESS_ENTRY(Section, FALSE),
    ACCESS_ENTRY(Semaphore, TRUE),
    ACCESS_ENTRY(Service, FALSE),
    ACCESS_ENTRY(Session, FALSE),
    ACCESS_ENTRY(SymbolicLink, FALSE),
    ACCESS_ENTRY(Thread, TRUE),
    ACCESS_ENTRY(Thread60, TRUE),
    ACCESS_ENTRY(Timer, TRUE),
    ACCESS_ENTRY(TmEn, FALSE),
    ACCESS_ENTRY(TmRm, FALSE),
    ACCESS_ENTRY(TmTm, FALSE),
    ACCESS_ENTRY(TmTx, FALSE),
    ACCESS_ENTRY(Token, FALSE),
    ACCESS_ENTRY(Type, FALSE),
    ACCESS_ENTRY(WindowStation, FALSE)
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

static int __cdecl PhpAccessEntryCompare(
    __in const void *elem1,
    __in const void *elem2
    )
{
    PPH_ACCESS_ENTRY entry1 = (PPH_ACCESS_ENTRY)elem1;
    PPH_ACCESS_ENTRY entry2 = (PPH_ACCESS_ENTRY)elem2;

    return intcmp(PhCountBits(entry2->Access), PhCountBits(entry1->Access));
}

PPH_STRING PhGetAccessString(
    __in ACCESS_MASK Access,
    __in PPH_ACCESS_ENTRY AccessEntries,
    __in ULONG NumberOfAccessEntries
    )
{
    PPH_STRING_BUILDER stringBuilder;
    PPH_STRING string;
    PPH_ACCESS_ENTRY accessEntries;
    PBOOLEAN matched;
    ULONG i;
    ULONG j;

    stringBuilder = PhCreateStringBuilder(10);

    // Sort the access entries according to how many access rights they 
    // include.
    accessEntries = PhAllocateCopy(AccessEntries, NumberOfAccessEntries * sizeof(PH_ACCESS_ENTRY));
    qsort(accessEntries, NumberOfAccessEntries, sizeof(PH_ACCESS_ENTRY), PhpAccessEntryCompare);

    matched = PhAllocate(NumberOfAccessEntries * sizeof(BOOLEAN));
    memset(matched, 0, NumberOfAccessEntries * sizeof(BOOLEAN));

    for (i = 0; i < NumberOfAccessEntries; i++)
    {
        // We make sure we haven't matched this access entry yet. 
        // This ensures that we won't get duplicates, e.g. 
        // FILE_GENERIC_READ includes FILE_READ_DATA, and we 
        // don't want to display both to the user.
        if (
            !matched[i] &&
            ((Access & accessEntries[i].Access) == accessEntries[i].Access)
            )
        {
            if (accessEntries[i].ShortName)
                PhStringBuilderAppend2(stringBuilder, accessEntries[i].ShortName);
            else
                PhStringBuilderAppend2(stringBuilder, accessEntries[i].Name);

            PhStringBuilderAppend2(stringBuilder, L", ");

            // Disable equal or more specific entries.
            for (j = i; j < NumberOfAccessEntries; j++)
            {
                if ((accessEntries[i].Access | accessEntries[j].Access) == accessEntries[i].Access)
                    matched[j] = TRUE;
            }
        }
    }

    // Remove the trailing ", ".
    if (PhStringEndsWith2(stringBuilder->String, L", ", FALSE))
        PhStringBuilderRemove(stringBuilder, stringBuilder->String->Length / 2 - 2, 2);

    PhFree(matched);
    PhFree(accessEntries);

    string = PhReferenceStringBuilderString(stringBuilder);
    PhDereferenceObject(stringBuilder);

    return string;
}
