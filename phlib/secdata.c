/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <secedit.h>
#include <wmistr.h>
#include <wbemcli.h>
#include <wtsapi32.h>

#define ACCESS_ENTRIES(Type) static PH_ACCESS_ENTRY Ph##Type##AccessEntries[] =
#define ACCESS_ENTRY(Type, HasSynchronize) \
   { TEXT(#Type), Ph##Type##AccessEntries, sizeof(Ph##Type##AccessEntries), HasSynchronize }

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

ACCESS_ENTRIES(EtwConsumer)
{
    { L"Full control", WMIGUID_ALL_ACCESS, TRUE, TRUE },
    { L"Query", WMIGUID_QUERY, TRUE, TRUE },
    { L"Read", WMIGUID_SET, TRUE, TRUE },
    { L"Notification", WMIGUID_NOTIFICATION, TRUE, TRUE },
    { L"Read description", WMIGUID_READ_DESCRIPTION, TRUE, TRUE },
    { L"Execute", WMIGUID_EXECUTE, TRUE, TRUE },
    { L"Create realtime", TRACELOG_CREATE_REALTIME, TRUE, TRUE },
    { L"Create logfile", TRACELOG_CREATE_ONDISK, TRUE, TRUE },
    { L"GUID enable", TRACELOG_GUID_ENABLE, TRUE, TRUE },
    { L"Access kernel logger", TRACELOG_ACCESS_KERNEL_LOGGER, TRUE, TRUE },
    { L"Log events", TRACELOG_LOG_EVENT, TRUE, TRUE },
    { L"Access realtime", TRACELOG_ACCESS_REALTIME, TRUE, TRUE },
    { L"Register guids", TRACELOG_REGISTER_GUIDS, TRUE, TRUE },
    { L"Join group", TRACELOG_JOIN_GROUP, TRUE, TRUE }
};

ACCESS_ENTRIES(EtwRegistration)
{
    { L"Full control", WMIGUID_ALL_ACCESS, TRUE, TRUE },
    { L"Query", WMIGUID_QUERY, TRUE, TRUE },
    { L"Read", WMIGUID_SET, TRUE, TRUE },
    { L"Notification", WMIGUID_NOTIFICATION, TRUE, TRUE },
    { L"Read description", WMIGUID_READ_DESCRIPTION, TRUE, TRUE },
    { L"Execute", WMIGUID_EXECUTE, TRUE, TRUE },
    { L"Create realtime", TRACELOG_CREATE_REALTIME, TRUE, TRUE },
    { L"Create logfile", TRACELOG_CREATE_ONDISK, TRUE, TRUE },
    { L"GUID enable", TRACELOG_GUID_ENABLE, TRUE, TRUE },
    { L"Access kernel logger", TRACELOG_ACCESS_KERNEL_LOGGER, TRUE, TRUE },
    { L"Log events", TRACELOG_LOG_EVENT, TRUE, TRUE },
    { L"Access realtime", TRACELOG_ACCESS_REALTIME, TRUE, TRUE },
    { L"Register guids", TRACELOG_REGISTER_GUIDS, TRUE, TRUE },
    { L"Join group", TRACELOG_JOIN_GROUP, TRUE, TRUE }
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
    //{ L"Execute", KEY_EXECUTE, TRUE, FALSE }, // KEY_EXECUTE has the same value as KEY_READ (dmex)
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

ACCESS_ENTRIES(LsaAccount)
{
    { L"Full control", ACCOUNT_ALL_ACCESS, TRUE, TRUE },
    { L"Read", ACCOUNT_READ, TRUE, FALSE },
    { L"Write", ACCOUNT_WRITE, TRUE, FALSE },
    { L"Execute", ACCOUNT_EXECUTE, TRUE, FALSE },
    { L"View", ACCOUNT_VIEW, FALSE, TRUE },
    { L"Adjust privileges", ACCOUNT_ADJUST_PRIVILEGES, FALSE, TRUE },
    { L"Adjust quotas", ACCOUNT_ADJUST_QUOTAS, FALSE, TRUE },
    { L"Adjust system access", ACCOUNT_ADJUST_SYSTEM_ACCESS, FALSE, TRUE }
};

ACCESS_ENTRIES(LsaPolicy)
{
    { L"Full control", POLICY_ALL_ACCESS | POLICY_NOTIFICATION, TRUE, TRUE },
    { L"Read", POLICY_READ, TRUE, FALSE },
    { L"Write", POLICY_WRITE, TRUE, FALSE },
    { L"Execute", POLICY_EXECUTE | POLICY_NOTIFICATION, TRUE, FALSE },
    { L"View local information", POLICY_VIEW_LOCAL_INFORMATION, FALSE, TRUE },
    { L"View audit information", POLICY_VIEW_AUDIT_INFORMATION, FALSE, TRUE },
    { L"Get private information", POLICY_GET_PRIVATE_INFORMATION, FALSE, TRUE },
    { L"Administer trust", POLICY_TRUST_ADMIN, FALSE, TRUE },
    { L"Create account", POLICY_CREATE_ACCOUNT, FALSE, TRUE },
    { L"Create secret", POLICY_CREATE_SECRET, FALSE, TRUE },
    { L"Create privilege", POLICY_CREATE_PRIVILEGE, FALSE, TRUE },
    { L"Set default quota limits", POLICY_SET_DEFAULT_QUOTA_LIMITS, FALSE, TRUE },
    { L"Set audit requirements", POLICY_SET_AUDIT_REQUIREMENTS, FALSE, TRUE },
    { L"Administer audit log", POLICY_AUDIT_LOG_ADMIN, FALSE, TRUE },
    { L"Administer server", POLICY_SERVER_ADMIN, FALSE, TRUE },
    { L"Lookup names", POLICY_LOOKUP_NAMES, FALSE, TRUE },
    { L"Get notifications", POLICY_NOTIFICATION, FALSE, TRUE }
};

ACCESS_ENTRIES(LsaSecret)
{
    { L"Full control", SECRET_ALL_ACCESS, TRUE, TRUE },
    { L"Read", SECRET_READ, TRUE, FALSE },
    { L"Write", SECRET_WRITE, TRUE, FALSE },
    { L"Execute", SECRET_EXECUTE, TRUE, FALSE },
    { L"Set value", SECRET_SET_VALUE, FALSE, TRUE },
    { L"Query value", SECRET_QUERY_VALUE, FALSE, TRUE }
};

ACCESS_ENTRIES(LsaTrusted)
{
    { L"Full control", TRUSTED_ALL_ACCESS, TRUE, TRUE },
    { L"Read", TRUSTED_READ, TRUE, FALSE },
    { L"Write", TRUSTED_WRITE, TRUE, FALSE },
    { L"Execute", TRUSTED_EXECUTE, TRUE, FALSE },
    { L"Query domain name", TRUSTED_QUERY_DOMAIN_NAME, FALSE, TRUE },
    { L"Query controllers", TRUSTED_QUERY_CONTROLLERS, FALSE, TRUE },
    { L"Set controllers", TRUSTED_SET_CONTROLLERS, FALSE, TRUE },
    { L"Query POSIX", TRUSTED_QUERY_POSIX, FALSE, TRUE },
    { L"Set POSIX", TRUSTED_SET_POSIX, FALSE, TRUE },
    { L"Query authentication", TRUSTED_QUERY_AUTH, FALSE, TRUE },
    { L"Set authentication", TRUSTED_SET_AUTH, FALSE, TRUE }
};

ACCESS_ENTRIES(Mutant)
{
    { L"Full control", MUTANT_ALL_ACCESS, TRUE, TRUE },
    { L"Query", MUTANT_QUERY_STATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Partition)
{
    { L"Full control", MEMORY_PARTITION_ALL_ACCESS, TRUE, TRUE },
    { L"Query", MEMORY_PARTITION_QUERY_ACCESS, TRUE, TRUE },
    { L"Modify", MEMORY_PARTITION_MODIFY_ACCESS, TRUE, TRUE }
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
    { L"Set limited information", PROCESS_SET_LIMITED_INFORMATION, TRUE, TRUE },
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

ACCESS_ENTRIES(SamAlias)
{
    { L"Full control", ALIAS_ALL_ACCESS, TRUE, TRUE },
    { L"Read", ALIAS_READ, TRUE, FALSE },
    { L"Write", ALIAS_WRITE, TRUE, FALSE },
    { L"Execute", ALIAS_EXECUTE, TRUE, FALSE },
    { L"Read information", ALIAS_READ_INFORMATION, FALSE, TRUE },
    { L"Write account", ALIAS_WRITE_ACCOUNT, FALSE, TRUE },
    { L"Add member", ALIAS_ADD_MEMBER, FALSE, TRUE },
    { L"Remove member", ALIAS_REMOVE_MEMBER, FALSE, TRUE },
    { L"List members", ALIAS_LIST_MEMBERS, FALSE, TRUE }
};

ACCESS_ENTRIES(SamDomain)
{
    { L"Full control", DOMAIN_ALL_ACCESS, TRUE, TRUE },
    { L"Read", DOMAIN_READ, TRUE, FALSE },
    { L"Write", DOMAIN_WRITE, TRUE, FALSE },
    { L"Execute", DOMAIN_EXECUTE, TRUE, FALSE },
    { L"Read password parameters", DOMAIN_READ_PASSWORD_PARAMETERS, FALSE, TRUE },
    { L"Write password parameters", DOMAIN_WRITE_PASSWORD_PARAMS, FALSE, TRUE },
    { L"Read other parameters", DOMAIN_READ_OTHER_PARAMETERS, FALSE, TRUE },
    { L"Write other parameters", DOMAIN_WRITE_OTHER_PARAMETERS, FALSE, TRUE },
    { L"Create user", DOMAIN_CREATE_USER, FALSE, TRUE },
    { L"Create group", DOMAIN_CREATE_GROUP, FALSE, TRUE },
    { L"Create alias", DOMAIN_CREATE_ALIAS, FALSE, TRUE },
    { L"Get alias membership", DOMAIN_GET_ALIAS_MEMBERSHIP, FALSE, TRUE },
    { L"List accounts", DOMAIN_LIST_ACCOUNTS, FALSE, TRUE },
    { L"Lookup", DOMAIN_LOOKUP, FALSE, TRUE },
    { L"Administer server", DOMAIN_ADMINISTER_SERVER, FALSE, TRUE }
};

ACCESS_ENTRIES(SamGroup)
{
    { L"Full control", GROUP_ALL_ACCESS, TRUE, TRUE },
    { L"Read", GROUP_READ, TRUE, FALSE },
    { L"Write", GROUP_WRITE, TRUE, FALSE },
    { L"Execute", GROUP_EXECUTE, TRUE, FALSE },
    { L"Read information", GROUP_READ_INFORMATION, FALSE, TRUE },
    { L"Write account", GROUP_WRITE_ACCOUNT, FALSE, TRUE },
    { L"Add member", GROUP_ADD_MEMBER, FALSE, TRUE },
    { L"Remove member", GROUP_REMOVE_MEMBER, FALSE, TRUE },
    { L"List members", GROUP_LIST_MEMBERS, FALSE, TRUE }
};

ACCESS_ENTRIES(SamServer)
{
    { L"Full control", SAM_SERVER_ALL_ACCESS, TRUE, TRUE },
    { L"Read", SAM_SERVER_READ, TRUE, FALSE },
    { L"Write", SAM_SERVER_WRITE, TRUE, FALSE },
    { L"Execute", SAM_SERVER_EXECUTE, TRUE, FALSE },
    { L"Connect", SAM_SERVER_CONNECT, FALSE, TRUE },
    { L"Shutdown", SAM_SERVER_SHUTDOWN, FALSE, TRUE },
    { L"Initialize", SAM_SERVER_INITIALIZE, FALSE, TRUE },
    { L"Create domain", SAM_SERVER_CREATE_DOMAIN, FALSE, TRUE },
    { L"Enumerate domains", SAM_SERVER_ENUMERATE_DOMAINS, FALSE, TRUE },
    { L"Lookup domain", SAM_SERVER_LOOKUP_DOMAIN, FALSE, TRUE }
};

ACCESS_ENTRIES(SamUser)
{
    { L"Full control", USER_ALL_ACCESS, TRUE, TRUE },
    { L"Read", USER_READ, TRUE, FALSE },
    { L"Write", USER_WRITE, TRUE, FALSE },
    { L"Execute", USER_EXECUTE, TRUE, FALSE },
    { L"Read general", USER_READ_GENERAL, FALSE, TRUE },
    { L"Read preferences", USER_READ_PREFERENCES, FALSE, TRUE },
    { L"Write preferences", USER_WRITE_PREFERENCES, FALSE, TRUE },
    { L"Read logon", USER_READ_LOGON, FALSE, TRUE },
    { L"Read account", USER_READ_ACCOUNT, FALSE, TRUE },
    { L"Write account", USER_WRITE_ACCOUNT, FALSE, TRUE },
    { L"Change password", USER_CHANGE_PASSWORD, FALSE, TRUE },
    { L"Force password change", USER_FORCE_PASSWORD_CHANGE, FALSE, TRUE },
    { L"List groups", USER_LIST_GROUPS, FALSE, TRUE },
    { L"Read group information", USER_READ_GROUP_INFORMATION, FALSE, TRUE },
    { L"Write group information", USER_WRITE_GROUP_INFORMATION, FALSE, TRUE }
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

ACCESS_ENTRIES(SCManager)
{
    { L"Full control", SC_MANAGER_ALL_ACCESS, TRUE, TRUE },
    { L"Create service", SC_MANAGER_CREATE_SERVICE, TRUE, TRUE },
    { L"Connect", SC_MANAGER_CONNECT, TRUE, TRUE },
    { L"Enumerate services", SC_MANAGER_ENUMERATE_SERVICE, TRUE, TRUE },
    { L"Lock", SC_MANAGER_LOCK, TRUE, TRUE },
    { L"Modify boot config", SC_MANAGER_MODIFY_BOOT_CONFIG, TRUE, TRUE },
    { L"Query lock status", SC_MANAGER_QUERY_LOCK_STATUS, TRUE, TRUE }
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
    { L"Read", TOKEN_READ, FALSE, FALSE },
    { L"Write", TOKEN_WRITE, FALSE, FALSE },
    { L"Execute", TOKEN_EXECUTE, FALSE, FALSE },
    { L"Adjust privileges", TOKEN_ADJUST_PRIVILEGES, TRUE, TRUE },
    { L"Adjust groups", TOKEN_ADJUST_GROUPS, TRUE, TRUE },
    { L"Adjust defaults", TOKEN_ADJUST_DEFAULT, TRUE, TRUE },
    { L"Adjust session ID", TOKEN_ADJUST_SESSIONID, TRUE, TRUE },
    { L"Assign as primary token", TOKEN_ASSIGN_PRIMARY, TRUE, TRUE, L"Assign primary" },
    { L"Duplicate", TOKEN_DUPLICATE, TRUE, TRUE },
    { L"Impersonate", TOKEN_IMPERSONATE, TRUE, TRUE },
    { L"Query", TOKEN_QUERY, TRUE, TRUE },
    { L"Query source", TOKEN_QUERY_SOURCE, FALSE, TRUE }
};

ACCESS_ENTRIES(TokenDefault)
{
    { L"Full control", GENERIC_ALL, TRUE, TRUE },
    { L"Read", GENERIC_READ, TRUE, TRUE },
    { L"Write", GENERIC_WRITE, TRUE, TRUE },
    { L"Execute", GENERIC_EXECUTE, TRUE, TRUE }
};

ACCESS_ENTRIES(TpWorkerFactory)
{
    { L"Full control", WORKER_FACTORY_ALL_ACCESS, TRUE, TRUE },
    { L"Release worker", WORKER_FACTORY_RELEASE_WORKER, FALSE, TRUE },
    { L"Ready worker", WORKER_FACTORY_READY_WORKER, FALSE, TRUE },
    { L"Wait", WORKER_FACTORY_WAIT, FALSE, TRUE },
    { L"Set information", WORKER_FACTORY_SET_INFORMATION, FALSE, TRUE },
    { L"Query information", WORKER_FACTORY_QUERY_INFORMATION, FALSE, TRUE },
    { L"Shutdown", WORKER_FACTORY_SHUTDOWN, FALSE, TRUE }
};

ACCESS_ENTRIES(Type)
{
    { L"Full control", OBJECT_TYPE_ALL_ACCESS, TRUE, TRUE },
    { L"Create", OBJECT_TYPE_CREATE, TRUE, TRUE }
};

ACCESS_ENTRIES(WaitCompletionPacket)
{
    { L"Full control", OBJECT_TYPE_ALL_ACCESS, TRUE, TRUE },
    { L"Modify state", OBJECT_TYPE_CREATE, TRUE, TRUE }
};

ACCESS_ENTRIES(Wbem)
{
    { L"Enable account", WBEM_ENABLE, TRUE, TRUE },
    { L"Execute methods", WBEM_METHOD_EXECUTE, TRUE, TRUE },
    { L"Full write", WBEM_FULL_WRITE_REP, TRUE, TRUE },
    { L"Partial write", WBEM_PARTIAL_WRITE_REP, TRUE, TRUE },
    { L"Provider write", WBEM_WRITE_PROVIDER, TRUE, TRUE },
    { L"Remote enable", WBEM_REMOTE_ACCESS, TRUE, TRUE },
    { L"Get notifications", WBEM_RIGHT_SUBSCRIBE, TRUE, TRUE },
    { L"Read description", WBEM_RIGHT_PUBLISH, TRUE, TRUE }
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

ACCESS_ENTRIES(WmiGuid)
{
    { L"Full control", WMIGUID_ALL_ACCESS, TRUE, TRUE },
    { L"Read", WMIGUID_GENERIC_READ, TRUE, FALSE },
    { L"Write", WMIGUID_GENERIC_WRITE, TRUE, FALSE },
    { L"Execute", WMIGUID_GENERIC_EXECUTE, TRUE, FALSE },
    { L"Query information", WMIGUID_QUERY, FALSE, TRUE },
    { L"Set information", WMIGUID_SET, FALSE, TRUE },
    { L"Get notifications", WMIGUID_NOTIFICATION, FALSE, TRUE },
    { L"Read description", WMIGUID_READ_DESCRIPTION, FALSE, TRUE },
    { L"Execute", WMIGUID_EXECUTE, FALSE, TRUE },
    { L"Create real-time logs", TRACELOG_CREATE_REALTIME, FALSE, TRUE, L"Create real-time" },
    { L"Create on disk logs", TRACELOG_CREATE_ONDISK, FALSE, TRUE, L"Create on disk" },
    { L"Enable provider GUIDs", TRACELOG_GUID_ENABLE, FALSE, TRUE, L"Enable GUIDs" },
    { L"Access kernel logger", TRACELOG_ACCESS_KERNEL_LOGGER, FALSE, TRUE },
    { L"Log events", TRACELOG_LOG_EVENT, FALSE, TRUE },
    { L"Access real-time events", TRACELOG_ACCESS_REALTIME, FALSE, TRUE, L"Access real-time" },
    { L"Register provider GUIDs", TRACELOG_REGISTER_GUIDS, FALSE, TRUE, L"Register GUIDs" }
};

ACCESS_ENTRIES(Rdp)
{
    { L"Full control", WTS_SECURITY_ALL_ACCESS, TRUE, TRUE },
    { L"Query information", WTS_SECURITY_QUERY_INFORMATION, TRUE, TRUE },
    { L"Set information", WTS_SECURITY_SET_INFORMATION, TRUE, TRUE },
    { L"Reset", WTS_SECURITY_RESET, FALSE, TRUE },
    { L"Virtual channels", WTS_SECURITY_VIRTUAL_CHANNELS, FALSE, TRUE },
    { L"Remote control", WTS_SECURITY_REMOTE_CONTROL, FALSE, TRUE },
    { L"Logon", WTS_SECURITY_LOGON, FALSE, TRUE },
    { L"Logoff", WTS_SECURITY_LOGOFF, FALSE, TRUE },
    { L"Message", WTS_SECURITY_MESSAGE, FALSE, TRUE },
    { L"Connect", WTS_SECURITY_CONNECT, FALSE, TRUE },
    { L"Disconnect", WTS_SECURITY_DISCONNECT, FALSE, TRUE },
    { L"Guest access", WTS_SECURITY_GUEST_ACCESS, FALSE, TRUE },
    { L"Guest access (current)", WTS_SECURITY_CURRENT_GUEST_ACCESS, FALSE, TRUE },
    { L"User access", WTS_SECURITY_USER_ACCESS, FALSE, TRUE },
    { L"User access (current)", WTS_SECURITY_SET_INFORMATION | WTS_SECURITY_RESET | WTS_SECURITY_VIRTUAL_CHANNELS | WTS_SECURITY_LOGOFF | WTS_SECURITY_DISCONNECT, FALSE, TRUE }, // WTS_SECURITY_CURRENT_USER_ACCESS
};

static PH_SPECIFIC_TYPE PhSpecificTypes[] =
{
    ACCESS_ENTRY(AlpcPort, TRUE),
    ACCESS_ENTRY(DebugObject, TRUE),
    ACCESS_ENTRY(Desktop, FALSE),
    ACCESS_ENTRY(Directory, FALSE),
    ACCESS_ENTRY(EtwConsumer, FALSE),
    ACCESS_ENTRY(EtwRegistration, FALSE),
    ACCESS_ENTRY(Event, TRUE),
    ACCESS_ENTRY(EventPair, TRUE),
    ACCESS_ENTRY(File, TRUE),
    ACCESS_ENTRY(FilterConnectionPort, FALSE),
    ACCESS_ENTRY(IoCompletion, TRUE),
    ACCESS_ENTRY(Job, TRUE),
    ACCESS_ENTRY(Key, FALSE),
    ACCESS_ENTRY(KeyedEvent, FALSE),
    ACCESS_ENTRY(LsaAccount, FALSE),
    ACCESS_ENTRY(LsaPolicy, FALSE),
    ACCESS_ENTRY(LsaSecret, FALSE),
    ACCESS_ENTRY(LsaTrusted, FALSE),
    ACCESS_ENTRY(Mutant, TRUE),
    ACCESS_ENTRY(Partition, TRUE),
    ACCESS_ENTRY(Process, TRUE),
    ACCESS_ENTRY(Process60, TRUE),
    ACCESS_ENTRY(Profile, FALSE),
    ACCESS_ENTRY(SamAlias, FALSE),
    ACCESS_ENTRY(SamDomain, FALSE),
    ACCESS_ENTRY(SamGroup, FALSE),
    ACCESS_ENTRY(SamServer, FALSE),
    ACCESS_ENTRY(SamUser, FALSE),
    ACCESS_ENTRY(Section, FALSE),
    ACCESS_ENTRY(Semaphore, TRUE),
    ACCESS_ENTRY(Service, FALSE),
    ACCESS_ENTRY(SCManager, FALSE),
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
    ACCESS_ENTRY(TokenDefault, FALSE),
    ACCESS_ENTRY(TpWorkerFactory, FALSE),
    ACCESS_ENTRY(Type, FALSE),
    ACCESS_ENTRY(WaitCompletionPacket, FALSE),
    ACCESS_ENTRY(Wbem, FALSE),
    ACCESS_ENTRY(WindowStation, FALSE),
    ACCESS_ENTRY(WmiGuid, TRUE),
    ACCESS_ENTRY(Rdp, FALSE),
};

/**
 * Gets access entries for an object type.
 *
 * \param Type The name of the object type.
 * \param AccessEntries A variable which receives an array of access entry structures. You must free
 * the buffer with PhFree() when you no longer need it.
 * \param NumberOfAccessEntries A variable which receives the number of access entry structures
 * returned in
 * \a AccessEntries.
 */
BOOLEAN PhGetAccessEntries(
    _In_ PWSTR Type,
    _Out_ PPH_ACCESS_ENTRY *AccessEntries,
    _Out_ PULONG NumberOfAccessEntries
    )
{
    ULONG i;
    PPH_SPECIFIC_TYPE specificType = NULL;
    PPH_ACCESS_ENTRY accessEntries;

    if (PhEqualStringZ(Type, L"ALPC Port", TRUE))
    {
        Type = L"AlpcPort";
    }
    else if (PhEqualStringZ(Type, L"Port", TRUE))
    {
        Type = L"AlpcPort";
    }
    else if (PhEqualStringZ(Type, L"WaitablePort", TRUE))
    {
        Type = L"AlpcPort";
    }
    else if (PhEqualStringZ(Type, L"Process", TRUE))
    {
        Type = L"Process60";
    }
    else if (PhEqualStringZ(Type, L"Thread", TRUE))
    {
        Type = L"Thread60";
    }
    else if (PhEqualStringZ(Type, L"FileObject", TRUE))
    {
        Type = L"File";
    }
    else if (PhEqualStringZ(Type, L"PowerDefault", TRUE))
    {
        Type = L"Key";
    }
    else if (PhEqualStringZ(Type, L"RdpDefault", TRUE))
    {
        Type = L"Rdp";
    }
    else if (PhEqualStringZ(Type, L"WmiDefault", TRUE))
    {
        // WBEM doesn't allow StandardAccessEntries (dmex)
        for (i = 0; i < RTL_NUMBER_OF(PhSpecificTypes); i++)
        {
            if (PhEqualStringZ(PhSpecificTypes[i].Type, L"Wbem", TRUE))
            {
                specificType = &PhSpecificTypes[i];
                break;
            }
        }

        if (specificType)
        {
            accessEntries = PhAllocate(specificType->SizeOfAccessEntries);
            memcpy(accessEntries, specificType->AccessEntries, specificType->SizeOfAccessEntries);

            *AccessEntries = accessEntries;
            *NumberOfAccessEntries = specificType->SizeOfAccessEntries / sizeof(PH_ACCESS_ENTRY);
            return TRUE;
        }
    }

    // Find the specific type.
    for (i = 0; i < sizeof(PhSpecificTypes) / sizeof(PH_SPECIFIC_TYPE); i++)
    {
        if (PhEqualStringZ(PhSpecificTypes[i].Type, Type, TRUE))
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
        *AccessEntries = PhAllocateCopy(PhStandardAccessEntries, sizeof(PhStandardAccessEntries));
        *NumberOfAccessEntries = sizeof(PhStandardAccessEntries) / sizeof(PH_ACCESS_ENTRY);
    }

    return TRUE;
}

static int __cdecl PhpAccessEntryCompare(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_ACCESS_ENTRY entry1 = (PPH_ACCESS_ENTRY)elem1;
    PPH_ACCESS_ENTRY entry2 = (PPH_ACCESS_ENTRY)elem2;

    return intcmp(PhCountBits(entry2->Access), PhCountBits(entry1->Access));
}

/**
 * Creates a string representation of an access mask.
 *
 * \param Access The access mask.
 * \param AccessEntries An array of access entry structures. You can call PhGetAccessEntries() to
 * retrieve the access entry structures for a standard object type.
 * \param NumberOfAccessEntries The number of elements in \a AccessEntries.
 *
 * \return The string representation of \a Access.
 */
PPH_STRING PhGetAccessString(
    _In_ ACCESS_MASK Access,
    _In_ PPH_ACCESS_ENTRY AccessEntries,
    _In_ ULONG NumberOfAccessEntries
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_ACCESS_ENTRY accessEntries;
    PBOOLEAN matched;
    ULONG i;
    ULONG j;

    PhInitializeStringBuilder(&stringBuilder, 32);

    // Sort the access entries according to how many access rights they include.
    accessEntries = PhAllocateCopy(AccessEntries, NumberOfAccessEntries * sizeof(PH_ACCESS_ENTRY));
    qsort(accessEntries, NumberOfAccessEntries, sizeof(PH_ACCESS_ENTRY), PhpAccessEntryCompare);

    matched = PhAllocate(NumberOfAccessEntries * sizeof(BOOLEAN));
    memset(matched, 0, NumberOfAccessEntries * sizeof(BOOLEAN));

    for (i = 0; i < NumberOfAccessEntries; i++)
    {
        // We make sure we haven't matched this access entry yet. This ensures that we won't get
        // duplicates, e.g. FILE_GENERIC_READ includes FILE_READ_DATA, and we don't want to display
        // both to the user.
        if (
            !matched[i] &&
            ((Access & accessEntries[i].Access) == accessEntries[i].Access)
            )
        {
            if (accessEntries[i].ShortName)
                PhAppendStringBuilder2(&stringBuilder, accessEntries[i].ShortName);
            else
                PhAppendStringBuilder2(&stringBuilder, accessEntries[i].Name);

            PhAppendStringBuilder2(&stringBuilder, L", ");

            // Disable equal or more specific entries.
            for (j = i; j < NumberOfAccessEntries; j++)
            {
                if ((accessEntries[i].Access | accessEntries[j].Access) == accessEntries[i].Access)
                    matched[j] = TRUE;
            }
        }
    }

    // Remove the trailing ", ".
    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhFree(matched);
    PhFree(accessEntries);

    return PhFinalStringBuilderString(&stringBuilder);
}
