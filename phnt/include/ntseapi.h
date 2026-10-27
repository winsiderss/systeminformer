/*
 * Authorization functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTSEAPI_H
#define _NTSEAPI_H

//
// Privileges
//

#define SE_MIN_WELL_KNOWN_PRIVILEGE (2L)
#define SE_CREATE_TOKEN_PRIVILEGE (2L)              // Required to create a primary token.
#define SE_ASSIGNPRIMARYTOKEN_PRIVILEGE (3L)        // Required to assign the primary token of a process.
#define SE_LOCK_MEMORY_PRIVILEGE (4L)               // Required to lock physical pages in memory.
#define SE_INCREASE_QUOTA_PRIVILEGE (5L)            // Required to increase the quota assigned to a process.
#define SE_MACHINE_ACCOUNT_PRIVILEGE (6L)           // Required to create a computer account.
#define SE_TCB_PRIVILEGE (7L)                       // Required to act as part of the Trusted Computer Base.
#define SE_SECURITY_PRIVILEGE (8L)                  // Required to perform a number of security-related functions, such as controlling and viewing audit messages. // Security operator.
#define SE_TAKE_OWNERSHIP_PRIVILEGE (9L)            // Required to take ownership of an object without being granted discretionary access.
#define SE_LOAD_DRIVER_PRIVILEGE (10L)              // Required to load or unload a device driver.
#define SE_SYSTEM_PROFILE_PRIVILEGE (11L)           // Required to gather profiling information for the entire system.
#define SE_SYSTEMTIME_PRIVILEGE (12L)               // Required to modify the system time.
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE (13L)      // Required to gather profiling information for a single process.
#define SE_INC_BASE_PRIORITY_PRIVILEGE (14L)        // Required to increase the base priority of a process.
#define SE_CREATE_PAGEFILE_PRIVILEGE (15L)          // Required to create a paging file.
#define SE_CREATE_PERMANENT_PRIVILEGE (16L)         // Required to create a permanent object.
#define SE_BACKUP_PRIVILEGE (17L)                   // Required to perform backup operations. This privilege causes the system to grant all read access control to any file.
#define SE_RESTORE_PRIVILEGE (18L)                  // Required to perform restore operations. This privilege causes the system to grant all write access control to any file.
#define SE_SHUTDOWN_PRIVILEGE (19L)                 // Required to shut down a local system.
#define SE_DEBUG_PRIVILEGE (20L)                    // Required to debug and adjust memory of any process, ignoring the DACL for the process.
#define SE_AUDIT_PRIVILEGE (21L)                    // Required to generate audit-log entries.
#define SE_SYSTEM_ENVIRONMENT_PRIVILEGE (22L)       // Required to modify UEFI variables of systems that use this type of memory to store configuration information.
#define SE_CHANGE_NOTIFY_PRIVILEGE (23L)            // Required to receive notifications of changes to files or directories and skip all traversal access checks. It is enabled by default for all users.
#define SE_REMOTE_SHUTDOWN_PRIVILEGE (24L)          // Required to shut down a system using a network request.
#define SE_UNDOCK_PRIVILEGE (25L)                   // Required to undock a laptop.
#define SE_SYNC_AGENT_PRIVILEGE (26L)               // Required for a domain controller to use the Lightweight Directory Access Protocol (LDAP) directory synchronization services.
#define SE_ENABLE_DELEGATION_PRIVILEGE (27L)        // Required to mark user and computer accounts as trusted for delegation.
#define SE_MANAGE_VOLUME_PRIVILEGE (28L)            // Required to enable volume management privileges.
#define SE_IMPERSONATE_PRIVILEGE (29L)              // Required to impersonate a client after authentication.
#define SE_CREATE_GLOBAL_PRIVILEGE (30L)            // Required to create named file mapping objects in the global namespace during Terminal Services sessions. It is enabled by default for all administrators.
#define SE_TRUSTED_CREDMAN_ACCESS_PRIVILEGE (31L)   // Required to access Credential Manager as a trusted caller.
#define SE_RELABEL_PRIVILEGE (32L)                  // Required to modify the mandatory integrity level of an object.
#define SE_INC_WORKING_SET_PRIVILEGE (33L)          // Required to allocate more memory for applications that run in the context of users.
#define SE_TIME_ZONE_PRIVILEGE (34L)                // Required to adjust the time zone associated with the computer's internal clock.
#define SE_CREATE_SYMBOLIC_LINK_PRIVILEGE (35L)     // Required to create a symbolic link.
#define SE_DELEGATE_SESSION_USER_IMPERSONATE_PRIVILEGE (36L) // Required to obtain an impersonation token for another user in the same session.
#define SE_MAX_WELL_KNOWN_PRIVILEGE SE_DELEGATE_SESSION_USER_IMPERSONATE_PRIVILEGE

//
// Authz
//

// begin_rev

#if (PHNT_MODE == PHNT_MODE_KERNEL)
/**
 * The TOKEN_INFORMATION_CLASS enumeration contains values that specify the type of information
 * being assigned to or retrieved from an access token.
 * \remarks The q/s annotations identify NtQueryInformationToken and NtSetInformationToken respectively.
 * Query access is TOKEN_QUERY, except TokenSource requires TOKEN_QUERY_SOURCE. Set access is generally
 * TOKEN_ADJUST_DEFAULT; TokenSessionId additionally requires TOKEN_ADJUST_SESSIONID and TokenLinkedToken
 * additionally requires TOKEN_QUERY. Privilege requirements marked s: apply to setting, not querying.
 * Individual classes can be unavailable or feature-gated even when their enum value is declared.
 *
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ne-winnt-token_information_class
 */
typedef enum _TOKEN_INFORMATION_CLASS
{
    TokenUser = 1,                        // q: TOKEN_USER, SE_TOKEN_USER
    TokenGroups,                          // q: TOKEN_GROUPS
    TokenPrivileges,                      // q: TOKEN_PRIVILEGES
    TokenOwner,                           // qs: TOKEN_OWNER
    TokenPrimaryGroup,                    // qs: TOKEN_PRIMARY_GROUP
    TokenDefaultDacl,                     // qs: TOKEN_DEFAULT_DACL
    TokenSource,                          // q: TOKEN_SOURCE
    TokenType,                            // q: TOKEN_TYPE_INFORMATION
    TokenImpersonationLevel,              // q: TOKEN_IMPERSONATION_LEVEL_INFORMATION
    TokenStatistics,                      // q: TOKEN_STATISTICS // 10
    TokenRestrictedSids,                  // q: TOKEN_GROUPS
    TokenSessionId,                       // qs: TOKEN_SESSION_ID (s: requires SeTcbPrivilege)
    TokenGroupsAndPrivileges,             // q: TOKEN_GROUPS_AND_PRIVILEGES
    TokenSessionReference,                // s: TOKEN_SESSION_REFERENCE (requires SeTcbPrivilege)
    TokenSandBoxInert,                    // q: TOKEN_SANDBOX_INERT
    TokenAuditPolicy,                     // qs: TOKEN_AUDIT_POLICY (requires SeSecurityPrivilege/SeTcbPrivilege)
    TokenOrigin,                          // qs: TOKEN_ORIGIN (s: requires SeTcbPrivilege)
    TokenElevationType,                   // q: TOKEN_ELEVATION_TYPE_INFORMATION
    TokenLinkedToken,                     // qs: TOKEN_LINKED_TOKEN (s: requires SeCreateTokenPrivilege)
    TokenElevation,                       // q: TOKEN_ELEVATION // 20
    TokenHasRestrictions,                 // q: TOKEN_RESTRICTION_INFORMATION (1 byte; ReturnLength = 1)
    TokenAccessInformation,               // q: TOKEN_ACCESS_INFORMATION
    TokenVirtualizationAllowed,           // qs: TOKEN_VIRTUALIZATION_ALLOWED (s: requires SeCreateTokenPrivilege)
    TokenVirtualizationEnabled,           // qs: TOKEN_VIRTUALIZATION_ENABLED
    TokenIntegrityLevel,                  // qs: TOKEN_MANDATORY_LABEL
    TokenUIAccess,                        // qs: TOKEN_UI_ACCESS (s: requires SeTcbPrivilege)
    TokenMandatoryPolicy,                 // qs: TOKEN_MANDATORY_POLICY (s: requires SeTcbPrivilege)
    TokenLogonSid,                        // q: TOKEN_GROUPS
    TokenIsAppContainer,                  // q: TOKEN_IS_APP_CONTAINER // since WIN8
    TokenCapabilities,                    // q: TOKEN_GROUPS // 30
    TokenAppContainerSid,                 // q: TOKEN_APPCONTAINER_INFORMATION
    TokenAppContainerNumber,              // q: TOKEN_APPCONTAINER_NUMBER
    TokenUserClaimAttributes,             // q: CLAIM_SECURITY_ATTRIBUTES_INFORMATION
    TokenDeviceClaimAttributes,           // q: CLAIM_SECURITY_ATTRIBUTES_INFORMATION
    TokenRestrictedUserClaimAttributes,   // q: CLAIM_SECURITY_ATTRIBUTES_INFORMATION
    TokenRestrictedDeviceClaimAttributes, // q: CLAIM_SECURITY_ATTRIBUTES_INFORMATION
    TokenDeviceGroups,                    // q: TOKEN_GROUPS
    TokenRestrictedDeviceGroups,          // q: TOKEN_GROUPS
    TokenSecurityAttributes,              // q: TOKEN_SECURITY_ATTRIBUTES_INFORMATION; s: TOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION (requires SeTcbPrivilege)
    TokenIsRestricted,                    // q: TOKEN_RESTRICTED_INFORMATION // 40; unsupported by the WoW64 thunk
    TokenProcessTrustLevel,               // q: TOKEN_PROCESS_TRUST_LEVEL // since WINBLUE
    TokenPrivateNameSpace,                // qs: TOKEN_PRIVATE_NAMESPACE (s: requires SeTcbPrivilege) // since THRESHOLD
    TokenSingletonAttributes,             // q: TOKEN_SECURITY_ATTRIBUTES_INFORMATION // since REDSTONE
    TokenBnoIsolation,                    // q: TOKEN_BNO_ISOLATION_INFORMATION // since REDSTONE2
    TokenChildProcessFlags,               // s: TOKEN_CHILD_PROCESS_FLAGS  (requires SeTcbPrivilege) // since REDSTONE3
    TokenIsLessPrivilegedAppContainer,    // q: TOKEN_LESS_PRIVILEGED_APPCONTAINER_INFORMATION // since REDSTONE5
    TokenIsSandboxed,                     // q: TOKEN_SANDBOXED_INFORMATION // since 19H1
    TokenIsAppSilo,                       // q: TOKEN_APP_SILO_INFORMATION // since WIN11 22H2 // previously TokenOriginatingProcessTrustLevel // q: TOKEN_PROCESS_TRUST_LEVEL
    TokenLoggingInformation,              // q: TOKEN_LOGGING_INFORMATION // since 24H2
    TokenLearningMode,                    // q: TOKEN_LEARNING_MODE // since 25H2
    TokenIsSystemManagedAdmin,            // q: TOKEN_SYSTEM_MANAGED_ADMIN_INFORMATION
    TokenIsInstaller,                     // s: TOKEN_INSTALLER_INFORMATION (exactly 4 bytes; feature-gated); query not dispatched 
    TokenHasTamperProtection,             // q: TOKEN_TAMPER_PROTECTION_INFORMATION
    TokenAppIdentitySid,                  // q: TOKEN_APPCONTAINER_INFORMATION
    TokenAppInstanceSid,                  // q: TOKEN_APPCONTAINER_INFORMATION
    TokenAppSuiteSid,                     // q: TOKEN_APPCONTAINER_INFORMATION
    TokenEntitlements,                    // q: not implemented (NTDDI 26200)
    TokenAgentId,                         // qs: TOKEN_AGENT_ID (feature-gated); query is null-terminated, set is counted UTF-16 (even byte length 2..0xfffc)
    MaxTokenInfoClass
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

//
// Token information structures
//

/**
 * The TOKEN_USER structure identifies the user associated with an access token.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_user
 */
typedef struct _TOKEN_USER
{
    SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;

typedef struct _SE_TOKEN_USER
{
    union
    {
        TOKEN_USER TokenUser;
        SID_AND_ATTRIBUTES User;
    } DUMMYUNIONNAME;
    union
    {
        SID Sid;
        BYTE Buffer[SECURITY_MAX_SID_SIZE];
    } DUMMYUNIONNAME2;
} SE_TOKEN_USER, PSE_TOKEN_USER;

#define TOKEN_USER_MAX_SIZE (sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE)

/**
 * The TOKEN_GROUPS structure contains information about the group security identifiers (SIDs) in an access token.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_groups
 */
typedef struct _TOKEN_GROUPS
{
    ULONG GroupCount;
    SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY];
} TOKEN_GROUPS, *PTOKEN_GROUPS;

/**
 * The TOKEN_PRIVILEGES structure contains information about a set of privileges for an access token.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_privileges
 */
typedef struct _TOKEN_PRIVILEGES
{
    ULONG PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

/**
 * The TOKEN_OWNER structure contains the default owner security identifier (SID) that will be applied to newly created objects.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_owner
 */
typedef struct _TOKEN_OWNER
{
    PSID Owner;
} TOKEN_OWNER, *PTOKEN_OWNER;

#define TOKEN_OWNER_MAX_SIZE (sizeof(TOKEN_OWNER) + SECURITY_MAX_SID_SIZE)

/**
 * The TOKEN_PRIMARY_GROUP structure specifies a group security identifier (SID) for an access token.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_primary_group
 */
typedef struct _TOKEN_PRIMARY_GROUP
{
    PSID PrimaryGroup;
} TOKEN_PRIMARY_GROUP, *PTOKEN_PRIMARY_GROUP;

/**
 * The TOKEN_DEFAULT_DACL structure specifies a discretionary access control list (DACL).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_default_dacl
 */
typedef struct _TOKEN_DEFAULT_DACL
{
    PACL DefaultDacl;
} TOKEN_DEFAULT_DACL, *PTOKEN_DEFAULT_DACL;

#define TOKEN_SOURCE_LENGTH 8

/**
 * The TOKEN_SOURCE structure identifies the source of an access token.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_source
 */
typedef struct _TOKEN_SOURCE
{
    CHAR SourceName[TOKEN_SOURCE_LENGTH];
    LUID SourceIdentifier;
} TOKEN_SOURCE, *PTOKEN_SOURCE;

/**
 * The TOKEN_TYPE enumeration contains values that differentiate between a primary token and an impersonation token.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ne-winnt-token_type
 */
typedef enum _TOKEN_TYPE
{
    TokenPrimary = 1,
    TokenImpersonation
} TOKEN_TYPE;

typedef struct _TOKEN_USER_CLAIMS
{
    PCLAIMS_BLOB UserClaims;
} TOKEN_USER_CLAIMS, *PTOKEN_USER_CLAIMS;

typedef struct _TOKEN_DEVICE_CLAIMS
{
    PCLAIMS_BLOB DeviceClaims;
} TOKEN_DEVICE_CLAIMS, *PTOKEN_DEVICE_CLAIMS;

typedef struct _TOKEN_GROUPS_AND_PRIVILEGES
{
    ULONG SidCount;
    ULONG SidLength;
    PSID_AND_ATTRIBUTES Sids;
    ULONG RestrictedSidCount;
    ULONG RestrictedSidLength;
    PSID_AND_ATTRIBUTES RestrictedSids;
    ULONG PrivilegeCount;
    ULONG PrivilegeLength;
    PLUID_AND_ATTRIBUTES Privileges;
    LUID AuthenticationId;
} TOKEN_GROUPS_AND_PRIVILEGES, *PTOKEN_GROUPS_AND_PRIVILEGES;

typedef struct _TOKEN_LINKED_TOKEN
{
    HANDLE LinkedToken;
} TOKEN_LINKED_TOKEN, *PTOKEN_LINKED_TOKEN;

typedef struct _TOKEN_ELEVATION
{
    ULONG TokenIsElevated;
} TOKEN_ELEVATION, *PTOKEN_ELEVATION;

/**
 * The TOKEN_MANDATORY_LABEL structure specifies the integrity SID and its attributes for a token.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-token_mandatory_label
 */
typedef struct _TOKEN_MANDATORY_LABEL
{
    SID_AND_ATTRIBUTES Label;
} TOKEN_MANDATORY_LABEL, *PTOKEN_MANDATORY_LABEL;

#define TOKEN_MANDATORY_POLICY_OFF             0x0
#define TOKEN_MANDATORY_POLICY_NO_WRITE_UP     0x1
#define TOKEN_MANDATORY_POLICY_NEW_PROCESS_MIN 0x2
#define TOKEN_MANDATORY_POLICY_VALID_MASK      (TOKEN_MANDATORY_POLICY_NO_WRITE_UP | \
                                                TOKEN_MANDATORY_POLICY_NEW_PROCESS_MIN)

#define TOKEN_INTEGRITY_LEVEL_MAX_SIZE \
    ((((ULONG)(sizeof(TOKEN_MANDATORY_LABEL)) + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1)) + SECURITY_MAX_SID_SIZE)

typedef struct _TOKEN_MANDATORY_POLICY
{
    ULONG Policy;
} TOKEN_MANDATORY_POLICY, *PTOKEN_MANDATORY_POLICY;

typedef PVOID PSECURITY_ATTRIBUTES_OPAQUE;

/**
 * The TOKEN_ACCESS_INFORMATION structure is the fixed header returned for TokenAccessInformation.
 * \remarks The fixed layout is 0x58 bytes on x64 and 0x38 bytes on x86 and ends with SecurityAttributes.
 * Query ReturnLength includes the additional SID hashes, SID arrays, privileges and attribute storage.
 * The pointer members refer into the returned buffer. There is no trailing SECURITY_QUALITY_OF_SERVICE field.
 */
typedef struct _TOKEN_ACCESS_INFORMATION
{
    PSID_AND_ATTRIBUTES_HASH SidHash;
    PSID_AND_ATTRIBUTES_HASH RestrictedSidHash;
    PTOKEN_PRIVILEGES Privileges;
    LUID AuthenticationId;
    TOKEN_TYPE TokenType;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    TOKEN_MANDATORY_POLICY MandatoryPolicy;
    ULONG Flags;
    ULONG AppContainerNumber;
    PSID PackageSid;
    PSID_AND_ATTRIBUTES_HASH CapabilitiesHash;
    PSID TrustLevelSid;
    PSECURITY_ATTRIBUTES_OPAQUE SecurityAttributes;
} TOKEN_ACCESS_INFORMATION, *PTOKEN_ACCESS_INFORMATION;

typedef struct _TOKEN_LOGGING_INFORMATION
{
    TOKEN_TYPE TokenType;
    TOKEN_ELEVATION TokenElevation;
    TOKEN_ELEVATION_TYPE TokenElevationType;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    ULONG IntegrityLevel;
    SID_AND_ATTRIBUTES User;
    PSID TrustLevelSid;
    ULONG SessionId;
    ULONG AppContainerNumber;
    LUID AuthenticationId;
    ULONG GroupCount;
    ULONG GroupsLength;
    PSID_AND_ATTRIBUTES Groups;
} TOKEN_LOGGING_INFORMATION, *PTOKEN_LOGGING_INFORMATION;

#endif // (PHNT_MODE == PHNT_MODE_KERNEL)

//
// Scalar token information wrappers. Each member preserves the native payload
// type and size; these structures can be used in both user and kernel mode.
// TOKEN_AGENT_ID contains inline UTF-16 data, with no length or pointer header.
// Query buffers include a terminator; set lengths count the supplied UTF-16 bytes.
//

typedef struct _TOKEN_TYPE_INFORMATION
{
    TOKEN_TYPE TokenType;
} TOKEN_TYPE_INFORMATION, *PTOKEN_TYPE_INFORMATION;

typedef struct _TOKEN_IMPERSONATION_LEVEL_INFORMATION
{
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
} TOKEN_IMPERSONATION_LEVEL_INFORMATION, *PTOKEN_IMPERSONATION_LEVEL_INFORMATION;

typedef struct _TOKEN_SESSION_ID
{
    ULONG SessionId;
} TOKEN_SESSION_ID, *PTOKEN_SESSION_ID;

typedef struct _TOKEN_SESSION_REFERENCE
{
    ULONG SessionReference;
} TOKEN_SESSION_REFERENCE, *PTOKEN_SESSION_REFERENCE;

typedef struct _TOKEN_SANDBOX_INERT
{
    ULONG SandboxInert;
} TOKEN_SANDBOX_INERT, *PTOKEN_SANDBOX_INERT;

typedef struct _TOKEN_ELEVATION_TYPE_INFORMATION
{
    TOKEN_ELEVATION_TYPE ElevationType;
} TOKEN_ELEVATION_TYPE_INFORMATION, *PTOKEN_ELEVATION_TYPE_INFORMATION;

typedef struct _TOKEN_RESTRICTION_INFORMATION
{
    BOOLEAN HasRestrictions;
} TOKEN_RESTRICTION_INFORMATION, *PTOKEN_RESTRICTION_INFORMATION;

typedef struct _TOKEN_VIRTUALIZATION_ALLOWED
{
    ULONG VirtualizationAllowed;
} TOKEN_VIRTUALIZATION_ALLOWED, *PTOKEN_VIRTUALIZATION_ALLOWED;

typedef struct _TOKEN_VIRTUALIZATION_ENABLED
{
    ULONG VirtualizationEnabled;
} TOKEN_VIRTUALIZATION_ENABLED, *PTOKEN_VIRTUALIZATION_ENABLED;

typedef struct _TOKEN_UI_ACCESS
{
    ULONG UIAccess;
} TOKEN_UI_ACCESS, *PTOKEN_UI_ACCESS;

typedef struct _TOKEN_IS_APP_CONTAINER
{
    ULONG IsAppContainer;
} TOKEN_IS_APP_CONTAINER, *PTOKEN_IS_APP_CONTAINER;

typedef struct _TOKEN_APPCONTAINER_NUMBER
{
    ULONG AppContainerNumber;
} TOKEN_APPCONTAINER_NUMBER, *PTOKEN_APPCONTAINER_NUMBER;

typedef struct _TOKEN_RESTRICTED_INFORMATION
{
    ULONG IsRestricted;
} TOKEN_RESTRICTED_INFORMATION, *PTOKEN_RESTRICTED_INFORMATION;

typedef struct _TOKEN_PRIVATE_NAMESPACE
{
    ULONG PrivateNamespace;
} TOKEN_PRIVATE_NAMESPACE, *PTOKEN_PRIVATE_NAMESPACE;

typedef struct _TOKEN_CHILD_PROCESS_FLAGS
{
    ULONG ChildProcessFlags;
} TOKEN_CHILD_PROCESS_FLAGS, *PTOKEN_CHILD_PROCESS_FLAGS;

typedef struct _TOKEN_LESS_PRIVILEGED_APPCONTAINER_INFORMATION
{
    ULONG IsLessPrivilegedAppContainer;
} TOKEN_LESS_PRIVILEGED_APPCONTAINER_INFORMATION, *PTOKEN_LESS_PRIVILEGED_APPCONTAINER_INFORMATION;

typedef struct _TOKEN_SANDBOXED_INFORMATION
{
    ULONG IsSandboxed;
} TOKEN_SANDBOXED_INFORMATION, *PTOKEN_SANDBOXED_INFORMATION;

typedef struct _TOKEN_APP_SILO_INFORMATION
{
    ULONG IsAppSilo;
} TOKEN_APP_SILO_INFORMATION, *PTOKEN_APP_SILO_INFORMATION;

typedef struct _TOKEN_LEARNING_MODE
{
    ULONG LearningMode;
} TOKEN_LEARNING_MODE, *PTOKEN_LEARNING_MODE;

typedef struct _TOKEN_SYSTEM_MANAGED_ADMIN_INFORMATION
{
    ULONG IsSystemManagedAdmin;
} TOKEN_SYSTEM_MANAGED_ADMIN_INFORMATION, *PTOKEN_SYSTEM_MANAGED_ADMIN_INFORMATION;

typedef struct _TOKEN_INSTALLER_INFORMATION
{
    ULONG IsInstaller;
} TOKEN_INSTALLER_INFORMATION, *PTOKEN_INSTALLER_INFORMATION;

typedef struct _TOKEN_TAMPER_PROTECTION_INFORMATION
{
    ULONG HasTamperProtection;
} TOKEN_TAMPER_PROTECTION_INFORMATION, *PTOKEN_TAMPER_PROTECTION_INFORMATION;

typedef struct _TOKEN_AGENT_ID
{
    WCHAR AgentId[ANYSIZE_ARRAY];
} TOKEN_AGENT_ID, *PTOKEN_AGENT_ID;

//
// Types
//

#define TOKEN_SECURITY_ATTRIBUTE_TYPE_INVALID 0x00
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_INT64 0x01
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_UINT64 0x02
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING 0x03 // Case insensitive attribute value string by default. Unless the flag TOKEN_SECURITY_ATTRIBUTE_VALUE_CASE_SENSITIVE is set.
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_FQBN 0x04 // Fully-qualified binary name.
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_SID 0x05
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_BOOLEAN 0x06
#define TOKEN_SECURITY_ATTRIBUTE_TYPE_OCTET_STRING 0x10

//
// Flags
//

// Attribute must not be inherited across process spawns.
#define TOKEN_SECURITY_ATTRIBUTE_NON_INHERITABLE 0x0001
// Attribute value is compared in a case sensitive way. It is valid with string value
// or composite type containing string value. For other types of value, this flag
// will be ignored. Currently, it is valid with the two types:
// TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING and TOKEN_SECURITY_ATTRIBUTE_TYPE_FQBN.
#define TOKEN_SECURITY_ATTRIBUTE_VALUE_CASE_SENSITIVE 0x0002
#define TOKEN_SECURITY_ATTRIBUTE_USE_FOR_DENY_ONLY 0x0004 // Attribute is considered only for Deny Aces.
#define TOKEN_SECURITY_ATTRIBUTE_DISABLED_BY_DEFAULT 0x0008 // Attribute is disabled by default.
#define TOKEN_SECURITY_ATTRIBUTE_DISABLED 0x0010 // Attribute is disabled.
#define TOKEN_SECURITY_ATTRIBUTE_MANDATORY 0x0020 // Attribute is mandatory.
#define TOKEN_SECURITY_ATTRIBUTE_COMPARE_IGNORE 0x0040 // Attribute is ignored.

#define TOKEN_SECURITY_ATTRIBUTE_VALID_FLAGS ( \
    TOKEN_SECURITY_ATTRIBUTE_NON_INHERITABLE | \
    TOKEN_SECURITY_ATTRIBUTE_VALUE_CASE_SENSITIVE | \
    TOKEN_SECURITY_ATTRIBUTE_USE_FOR_DENY_ONLY | \
    TOKEN_SECURITY_ATTRIBUTE_DISABLED_BY_DEFAULT | \
    TOKEN_SECURITY_ATTRIBUTE_DISABLED | \
    TOKEN_SECURITY_ATTRIBUTE_MANDATORY)

// Reserve upper 16 bits for custom flags. These should be preserved but not
// validated as they do not affect security in any way.
#define TOKEN_SECURITY_ATTRIBUTE_CUSTOM_FLAGS 0xffff0000

// end_rev

// private // CLAIM_SECURITY_ATTRIBUTE_FQBN_VALUE
typedef struct _TOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE
{
    ULONG64 Version;
    UNICODE_STRING Name;
} TOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE, *PTOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE;

// private // CLAIM_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE
typedef struct _TOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE
{
    PVOID Value; // Pointer is BYTE aligned.
    ULONG ValueLength; // In bytes
} TOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE, *PTOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE;

// private
/**
 * Describes a native token security attribute. Name and string values are UNICODE_STRING descriptors,
 * unlike the PWSTR-based CLAIM_SECURITY_ATTRIBUTE_V1 representation.
 * \remarks The fixed structure is 40 bytes on x64 and 24 bytes on x86. ValueCount counts elements,
 * not bytes; Values points to storage interpreted according to ValueType.
 */
typedef struct _TOKEN_SECURITY_ATTRIBUTE_V1
{
    UNICODE_STRING Name;
    USHORT ValueType;
    USHORT Reserved;
    ULONG Flags;
    ULONG ValueCount;
    union
    {
        PLONG64 Int64;
        PULONG64 Uint64;
        PUNICODE_STRING String;
        PTOKEN_SECURITY_ATTRIBUTE_FQBN_VALUE Fqbn;
        PTOKEN_SECURITY_ATTRIBUTE_OCTET_STRING_VALUE OctetString;
    } Values;
} TOKEN_SECURITY_ATTRIBUTE_V1, *PTOKEN_SECURITY_ATTRIBUTE_V1;

// private
typedef struct _TOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1
{
    UNICODE_STRING Name;
    USHORT ValueType;
    USHORT Reserved;
    ULONG Flags;
    ULONG ValueCount;
    union
    {
        ULONG Int64[ANYSIZE_ARRAY];
        ULONG Uint64[ANYSIZE_ARRAY];
        ULONG String[ANYSIZE_ARRAY];
        ULONG Fqbn[ANYSIZE_ARRAY];
        ULONG OctetString[ANYSIZE_ARRAY];
    } Values;
} TOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1, *PTOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1;

// rev
#define TOKEN_SECURITY_ATTRIBUTES_INFORMATION_VERSION_V1 1
// rev
#define TOKEN_SECURITY_ATTRIBUTES_INFORMATION_VERSION TOKEN_SECURITY_ATTRIBUTES_INFORMATION_VERSION_V1

// private
typedef struct _TOKEN_SECURITY_ATTRIBUTES_INFORMATION
{
    USHORT Version;
    USHORT Reserved;
    ULONG AttributeCount;
    union
    {
        PTOKEN_SECURITY_ATTRIBUTE_V1 AttributeV1;
    };
} TOKEN_SECURITY_ATTRIBUTES_INFORMATION, *PTOKEN_SECURITY_ATTRIBUTES_INFORMATION;

// private
typedef enum _TOKEN_SECURITY_ATTRIBUTE_OPERATION
{
    TOKEN_SECURITY_ATTRIBUTE_OPERATION_NONE,
    TOKEN_SECURITY_ATTRIBUTE_OPERATION_REPLACE_ALL,
    TOKEN_SECURITY_ATTRIBUTE_OPERATION_ADD,
    TOKEN_SECURITY_ATTRIBUTE_OPERATION_DELETE,
    TOKEN_SECURITY_ATTRIBUTE_OPERATION_REPLACE
} TOKEN_SECURITY_ATTRIBUTE_OPERATION, *PTOKEN_SECURITY_ATTRIBUTE_OPERATION;

// private
/**
 * Supplies attributes and operations when setting TokenSecurityAttributes (39).
 * \remarks This two-pointer header is 16 bytes on x64 and 8 bytes on x86. It is not the query format,
 * TOKEN_SECURITY_ATTRIBUTES_INFORMATION. The set operation requires TOKEN_ADJUST_DEFAULT and SeTcbPrivilege.
 * Operations must not be NULL. A NULL Attributes pointer is accepted only for REPLACE_ALL.
 */
typedef struct _TOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION
{
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION Attributes;
    PTOKEN_SECURITY_ATTRIBUTE_OPERATION Operations;
} TOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION, *PTOKEN_SECURITY_ATTRIBUTES_AND_OPERATION_INFORMATION;

// rev
/**
 * The TOKEN_PROCESS_TRUST_LEVEL structure contains information about
 * the trust level assigned to a process token. The trust level is
 * represented by a SID (Security Identifier) pointed to by TrustLevelSid.
 */
typedef struct _TOKEN_PROCESS_TRUST_LEVEL
{
    PSID TrustLevelSid;
} TOKEN_PROCESS_TRUST_LEVEL, *PTOKEN_PROCESS_TRUST_LEVEL;

#if (PHNT_MODE != PHNT_MODE_KERNEL) && (!defined(NTDDI_WIN11_GE) || (NTDDI_VERSION < NTDDI_WIN11_GE))
typedef struct _TOKEN_LOGGING_INFORMATION
{
    TOKEN_TYPE TokenType;
    TOKEN_ELEVATION TokenElevation;
    TOKEN_ELEVATION_TYPE TokenElevationType;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    ULONG IntegrityLevel;
    SID_AND_ATTRIBUTES User;
    PSID TrustLevelSid;
    ULONG SessionId;
    ULONG AppContainerNumber;
    LUID AuthenticationId;
    ULONG GroupCount;
    ULONG GroupsLength;
    PSID_AND_ATTRIBUTES Groups;
} TOKEN_LOGGING_INFORMATION, *PTOKEN_LOGGING_INFORMATION;
#endif // (PHNT_MODE != PHNT_MODE_KERNEL) && (!defined(NTDDI_WIN11_GE) || (NTDDI_VERSION < NTDDI_WIN11_GE))

//
// Tokens
//

/**
 * The NtCreateToken routine creates a new access token.
 *
 * \param TokenHandle Pointer to a variable that receives the handle to the newly created token.
 * \param DesiredAccess Specifies the requested access rights for the new token.
 * \param ObjectAttributes Optional pointer to an OBJECT_ATTRIBUTES structure specifying object attributes.
 * \param Type Specifies the type of token to be created (primary or impersonation).
 * \param AuthenticationId Pointer to a locally unique identifier (LUID) for the token.
 * \param ExpirationTime Pointer to a LARGE_INTEGER specifying the expiration time of the token.
 * \param User Pointer to a TOKEN_USER structure specifying the user account for the token.
 * \param Groups Pointer to a TOKEN_GROUPS structure specifying the group accounts for the token.
 * \param Privileges Pointer to a TOKEN_PRIVILEGES structure specifying the privileges for the token.
 * \param Owner Optional pointer to a TOKEN_OWNER structure specifying the owner SID for the token.
 * \param PrimaryGroup Pointer to a TOKEN_PRIMARY_GROUP structure specifying the primary group SID for the token.
 * \param DefaultDacl Optional pointer to a TOKEN_DEFAULT_DACL structure specifying the default DACL for the token.
 * \param Source Pointer to a TOKEN_SOURCE structure specifying the source of the token.
 * \return NTSTATUS code indicating success or failure.
 * \remarks NtCreateToken forwards to NtCreateTokenEx with the four extended claim/device inputs absent. This is
 * privileged token construction, not duplication of an ordinary process token. The Ex path checks
 * SeCreateTokenPrivilege; callers must still supply valid token type, security QoS and captured
 * SID/group/privilege data. Close a successfully returned token handle with NtClose.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TOKEN_TYPE Type,
    _In_ PLUID AuthenticationId,
    _In_ PLARGE_INTEGER ExpirationTime,
    _In_ PTOKEN_USER User,
    _In_ PTOKEN_GROUPS Groups,
    _In_ PTOKEN_PRIVILEGES Privileges,
    _In_opt_ PTOKEN_OWNER Owner,
    _In_ PTOKEN_PRIMARY_GROUP PrimaryGroup,
    _In_opt_ PTOKEN_DEFAULT_DACL DefaultDacl,
    _In_ PTOKEN_SOURCE Source
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtCreateLowBoxToken routine creates a new lowbox access token based on an existing token.
 *
 * \param TokenHandle Pointer to a variable that receives the handle to the newly created lowbox token.
 * \param ExistingTokenHandle Handle to an existing token to base the new token on.
 * \param DesiredAccess Specifies the requested access rights for the new token.
 * \param ObjectAttributes Optional pointer to an OBJECT_ATTRIBUTES structure specifying object attributes.
 * \param PackageSid Pointer to a SID structure specifying the package SID for the lowbox token.
 * \param CapabilityCount Number of capabilities in the Capabilities array.
 * \param Capabilities Optional pointer to an array of SID_AND_ATTRIBUTES structures specifying capabilities.
 * \param HandleCount Number of handles in the Handles array.
 * \param Handles Optional pointer to an array of handles to be associated with the token.
 * \return NTSTATUS code indicating success or failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/secauthz/ntcreatelowboxtoken
 * \remarks Creates an AppContainer/lowbox token from an existing token, using TOKEN_DUPLICATE (0x2) on the source.
 * Package SID, capability array and captured handle list have separate validation; this is not equivalent to
 * lowering an integrity label alone. The implementation has a feature-gated
 * AppContainer/integrity path, so successful entry capture does not establish every policy outcome.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateLowBoxToken(
    _Out_ PHANDLE TokenHandle,
    _In_ HANDLE ExistingTokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PSID PackageSid,
    _In_ ULONG CapabilityCount,
    _In_reads_opt_(CapabilityCount) PSID_AND_ATTRIBUTES Capabilities,
    _In_ ULONG HandleCount,
    _In_reads_opt_(HandleCount) HANDLE *Handles
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtCreateTokenEx routine creates a new access token with extended attributes.
 *
 * \param TokenHandle Pointer to a variable that receives the handle to the newly created token.
 * \param DesiredAccess Specifies the requested access rights for the new token.
 * \param ObjectAttributes Optional pointer to an OBJECT_ATTRIBUTES structure specifying object attributes.
 * \param Type Specifies the type of token to be created (primary or impersonation).
 * \param AuthenticationId Pointer to a locally unique identifier (LUID) for the token.
 * \param ExpirationTime Pointer to a LARGE_INTEGER specifying the expiration time of the token.
 * \param User Pointer to a TOKEN_USER structure specifying the user account for the token.
 * \param Groups Pointer to a TOKEN_GROUPS structure specifying the group accounts for the token.
 * \param Privileges Pointer to a TOKEN_PRIVILEGES structure specifying the privileges for the token.
 * \param UserAttributes Optional pointer to a TOKEN_SECURITY_ATTRIBUTES_INFORMATION structure specifying user claims.
 * \param DeviceAttributes Optional pointer to a TOKEN_SECURITY_ATTRIBUTES_INFORMATION structure specifying device claims.
 * \param DeviceGroups Optional pointer to a TOKEN_GROUPS structure specifying device groups.
 * \param MandatoryPolicy Optional pointer to a TOKEN_MANDATORY_POLICY structure specifying the mandatory policy.
 * \param Owner Optional pointer to a TOKEN_OWNER structure specifying the owner SID for the token.
 * \param PrimaryGroup Pointer to a TOKEN_PRIMARY_GROUP structure specifying the primary group SID for the token.
 * \param DefaultDacl Optional pointer to a TOKEN_DEFAULT_DACL structure specifying the default DACL for the token.
 * \param Source Pointer to a TOKEN_SOURCE structure specifying the source of the token.
 * \return NTSTATUS code indicating success or failure.
 * \remarks The entry checks SeCreateTokenPrivilege before token construction. User-mode TokenType is restricted to
 * primary or impersonation; the latter requires captured security QoS. Pointer-bearing SID/group/default-DACL
 * and claim inputs are captured independently. WoW64 converts these records rather than treating their 32-bit
 * layouts as native buffers. Creation policy, claim semantics and privileged success paths require separate
 * validation.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTokenEx(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TOKEN_TYPE Type,
    _In_ PLUID AuthenticationId,
    _In_ PLARGE_INTEGER ExpirationTime,
    _In_ PTOKEN_USER User,
    _In_ PTOKEN_GROUPS Groups,
    _In_ PTOKEN_PRIVILEGES Privileges,
    _In_opt_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION UserAttributes,
    _In_opt_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION DeviceAttributes,
    _In_opt_ PTOKEN_GROUPS DeviceGroups,
    _In_opt_ PTOKEN_MANDATORY_POLICY MandatoryPolicy,
    _In_opt_ PTOKEN_OWNER Owner,
    _In_ PTOKEN_PRIMARY_GROUP PrimaryGroup,
    _In_opt_ PTOKEN_DEFAULT_DACL DefaultDacl,
    _In_ PTOKEN_SOURCE Source
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

/**
 * The NtOpenProcessToken routine opens the access token associated with a process, and returns a handle that can be used to access that token.
 *
 * \param ProcessHandle Handle to the process whose access token is to be opened. The handle must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param DesiredAccess ACCESS_MASK structure specifying the requested types of access to the access token.
 * \param TokenHandle Pointer to a caller-allocated variable that receives a handle to the newly opened access token.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntopenprocesstoken
 * \remarks Opens the process primary token, not a thread impersonation token. It forwards to NtOpenProcessTokenEx with
 * HandleAttributes = 0. The process reference requires PROCESS_QUERY_LIMITED_INFORMATION (0x1000) on this
 * build; DesiredAccess applies separately to the returned token. The token handle is caller-owned and must be
 * closed.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle
    );

/**
 * The NtOpenProcessTokenEx routine opens the access token associated with a process, and returns a handle that can be used to access that token.
 *
 * \param ProcessHandle Handle to the process whose access token is to be opened. The handle must have PROCESS_QUERY_LIMITED_INFORMATION access.
 * \param DesiredAccess ACCESS_MASK structure specifying the requested types of access to the access token.
 * \param HandleAttributes Object-handle attributes. The kernel masks these with 0x1df2 for user-mode callers and 0x11ff2 for kernel-mode callers; OBJ_INHERIT is supported.
 * \param TokenHandle Pointer to a caller-allocated variable that receives a handle to the newly opened access token.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntopenprocesstokenex
 * \remarks The native implementation masks user HandleAttributes with 0x1DF2 before the token is opened. The process
 * handle and requested token rights are checked separately. Native failures need not initialize TokenHandle,
 * whereas the WoW64 thunk copies its zero-initialized temporary back after the native call. Consume the output
 * only on success.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcessTokenEx(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _Out_ PHANDLE TokenHandle
    );

/**
 * The NtOpenThreadToken routine opens the access token associated with a thread, and returns a handle that can be used to access that token.
 *
 * \param ThreadHandle Handle to the thread whose access token is to be opened. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param DesiredAccess ACCESS_MASK structure specifying the requested types of access to the access token.
 * \param OpenAsSelf Boolean value specifying whether the access check is to be made against the security context of the thread calling NtOpenThreadToken or against the security context of the process for the calling thread.
 * \param TokenHandle Pointer to a caller-allocated variable that receives a handle to the newly opened access token.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntopenthreadtoken
 * \remarks Opens the thread impersonation token and returns STATUS_NO_TOKEN if the thread is not impersonating; it does
 * not fall back to the process primary token. OpenAsSelf selects the security context for opening the token,
 * not which token is returned. This wrapper uses the Ex path with handle attributes 0.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle
    );

/**
 * The NtOpenThreadTokenEx routine opens the access token associated with a thread, and returns a handle that can be used to access that token.
 *
 * \param ThreadHandle Handle to the thread whose access token is to be opened. The handle must have THREAD_QUERY_LIMITED_INFORMATION access.
 * \param DesiredAccess ACCESS_MASK structure specifying the requested types of access to the access token.
 * \param OpenAsSelf Boolean value specifying whether the access check is to be made against the security context of the thread calling NtOpenThreadToken or against the security context of the process for the calling thread.
 * \param HandleAttributes Object-handle attributes. The kernel masks these with 0x1df2 for user-mode callers and 0x11ff2 for kernel-mode callers; OBJ_INHERIT is supported.
 * \param TokenHandle Pointer to a caller-allocated variable that receives a handle to the newly opened access token.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntopenthreadtokenex
 * \remarks The native implementation references the thread with THREAD_QUERY_LIMITED_INFORMATION (0x800), then separately
 * opens its impersonation token with DesiredAccess. No token and anonymous-level impersonation have distinct
 * failures. OpenAsSelf changes access-check context; it does not install or replace impersonation. WoW64
 * converts the handle output via a zero-initialized native temporary.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThreadTokenEx(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _In_ ULONG HandleAttributes,
    _Out_ PHANDLE TokenHandle
    );

/**
 * The NtDuplicateToken function creates a handle to a new access token that duplicates an existing token.
 *
 * \param ExistingTokenHandle A handle to an existing access token that was opened with the TOKEN_DUPLICATE access right.
 * \param DesiredAccess ACCESS_MASK structure specifying the requested types of access to the access token.
 * \param ObjectAttributes Pointer to an OBJECT_ATTRIBUTES structure that describes the requested properties for the new token.
 * \param EffectiveOnly A Boolean value that indicates whether the entire existing token should be duplicated into the new token or just the effective (currently enabled) part of the token.
 * \param Type Specifies the type of token to create either a primary token or an impersonation token.
 * \param NewTokenHandle Pointer to a caller-allocated variable that receives a handle to the newly duplicated token.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntduplicatetoken
 * \remarks Requires TOKEN_DUPLICATE (0x2) on the source. DesiredAccess = 0 requests the source handle granted access,
 * not a token handle with no rights. TokenType, EffectiveOnly and security-QoS impersonation level are
 * separate choices. The result is a new token object; merely creating it does not attach it to a process or
 * thread. Close the returned handle independently of the source handle.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDuplicateToken(
    _In_ HANDLE ExistingTokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN EffectiveOnly,
    _In_ TOKEN_TYPE Type,
    _Out_ PHANDLE NewTokenHandle
    );

/**
 * The NtQueryInformationToken routine retrieves a specified type of information about an access token. The calling process must have appropriate access rights to obtain the information.
 *
 * \param TokenHandle A handle to an existing access token from which information is to be retrieved. If TokenInformationClass is set to TokenSource, the handle must have TOKEN_QUERY_SOURCE access.
 * For all other TokenInformationClass values, the handle must have TOKEN_QUERY access.
 * \param TokenInformationClass A value from the TOKEN_INFORMATION_CLASS enumerated type identifying the type of information to be retrieved.
 * \param TokenInformation Caller-owned output buffer, probed with 4-byte alignment for user-mode callers. NULL with zero length can be used to obtain the required size.
 * \param TokenInformationLength Length, in bytes, of the caller-allocated TokenInformation buffer.
 * \param ReturnLength Required writable pointer receiving the returned or required size in bytes. On STATUS_BUFFER_TOO_SMALL this can exceed TokenInformationLength.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntqueryinformationtoken
 * \remarks Class sizing is not uniform. TokenHasRestrictions (21) writes a 1-byte BOOLEAN and sets
 * ReturnLength = 1, leaving later bytes untouched even when 4 bytes were supplied. TokenElevation (20)
 * requires exactly 4 bytes and returns STATUS_INFO_LENGTH_MISMATCH otherwise. TokenIsRestricted (40) supports
 * a 4-byte native query but the matching WoW64 thunk returns STATUS_INVALID_INFO_CLASS without changing
 * ReturnLength. Pointer-bearing classes have separate WoW64 conversion rules.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_writes_bytes_to_opt_(TokenInformationLength, *ReturnLength) PVOID TokenInformation,
    _In_ ULONG TokenInformationLength,
    _Out_ PULONG ReturnLength
    );

/**
 * The NtSetInformationToken routine modifies information in a specified token. The calling process must have appropriate access rights to set the information.
 *
 * \param TokenHandle Token handle normally requiring TOKEN_ADJUST_DEFAULT. TokenSessionId also requires TOKEN_ADJUST_SESSIONID; TokenLinkedToken also requires TOKEN_QUERY.
 * \param TokenInformationClass A value from the TOKEN_INFORMATION_CLASS enumerated type identifying the type of information to be modified.
 * \param TokenInformation Pointer to a caller-allocated buffer containing the information to be modified in the token.
 * \param TokenInformationLength Class-specific input length in bytes. On x64, owner, primary-group and default-DACL inputs require at least 8 bytes; linked-token input requires exactly 8 bytes and session-ID input exactly 4 bytes.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntsetinformationtoken
 * \remarks The information class selects independently validated input data and access. Most accepted selectors start
 * with TOKEN_ADJUST_DEFAULT (0x80); TokenSessionId also requests TOKEN_ADJUST_SESSIONID (0x100), and
 * additional privilege/policy checks still apply. TokenDefaultDacl expects a TOKEN_DEFAULT_DACL containing a
 * pointer, not raw ACL bytes. Changing it affects the token default DACL, not the security descriptor
 * protecting the token object itself.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
    _In_ ULONG TokenInformationLength
    );

/**
 * The NtAdjustPrivilegesToken routine enables, disables, or removes privileges in the specified access token.
 *
 * \param TokenHandle Handle to the token that contains the privileges to be modified. The handle must have TOKEN_ADJUST_PRIVILEGES access and also TOKEN_QUERY when PreviousState is supplied.
 * \param DisableAllPrivileges Specifies whether the function disables all of the token's privileges. If this value is TRUE, the function disables all privileges and ignores the NewState parameter.
 * If it is FALSE, the function modifies privileges based on the information pointed to by the NewState parameter.
 * \param NewState A pointer to a TOKEN_PRIVILEGES structure that specifies an array of privileges and their attributes. If DisableAllPrivileges is TRUE, the function ignores this parameter.
 * \param BufferLength Specifies the size, in bytes, of the buffer pointed to by the PreviousState parameter. This parameter can be zero if the PreviousState parameter is NULL.
 * \param PreviousState A pointer to a buffer that the function fills with a TOKEN_PRIVILEGES structure that contains the previous state of any privileges that the function modifies.
 * \param ReturnLength A pointer to a variable that receives the required size, in bytes, of the buffer pointed to by the PreviousState parameter. This parameter can be NULL if PreviousState is NULL.
 * \return STATUS_SUCCESS on complete adjustment; STATUS_NOT_ALL_ASSIGNED (0x00000106) can report partial adjustment even though NT_SUCCESS is true. STATUS_BUFFER_TOO_SMALL reports the required previous-state size without changing privileges.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-adjusttokenprivileges
 * \remarks Use TOKEN_ADJUST_PRIVILEGES (0x20), plus TOKEN_QUERY (0x8) when PreviousState is requested. ReturnLength is
 * required when PreviousState is non-NULL. An unknown privilege can produce STATUS_NOT_ALL_ASSIGNED (0x106)
 * even though NT_SUCCESS is true. This operation cannot add privileges that the token does not possess. An empty previous-state result clears the first declared LUID_AND_ATTRIBUTES slot as well
 * as PrivilegeCount; retain storage for the declared TOKEN_PRIVILEGES structure.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PTOKEN_PRIVILEGES PreviousState,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * The NtAdjustGroupsToken routine enables or disables groups in the specified access token.
 *
 * \param TokenHandle Handle to the token that contains the groups to be modified. The handle must have TOKEN_ADJUST_GROUPS access and also TOKEN_QUERY when PreviousState is supplied.
 * \param ResetToDefault Specifies whether the function resets the groups to the default state. If this value is TRUE, the function resets all groups to their default state and ignores the NewState parameter.
 * \param NewState A pointer to a TOKEN_GROUPS structure that specifies an array of groups and their attributes. If ResetToDefault is TRUE, the function ignores this parameter.
 * \param BufferLength Specifies the size, in bytes, of the buffer pointed to by the PreviousState parameter. This parameter can be zero if the PreviousState parameter is NULL.
 * \param PreviousState A pointer to a buffer that the function fills with a TOKEN_GROUPS structure that contains the previous state of any groups that the function modifies.
 * \param ReturnLength A pointer to a variable that receives the required size, in bytes, of the buffer pointed to by the PreviousState parameter. This parameter can be NULL if PreviousState is NULL.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-adjusttokengroups
 * \remarks Requires TOKEN_ADJUST_GROUPS (0x40), plus TOKEN_QUERY (0x8) when PreviousState is requested. Without
 * ResetToDefault, NewState is required. Changes apply to eligible group attributes, not arbitrary new SIDs,
 * and mandatory/deny-only constraints remain in the group helper. TOKEN_GROUPS contains pointer-sized SID
 * entries and therefore requires WoW64 conversion unlike the fixed LUID_AND_ATTRIBUTES privilege entries.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustGroupsToken(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN ResetToDefault,
    _In_opt_ PTOKEN_GROUPS NewState,
    _In_opt_ ULONG BufferLength,
    _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PTOKEN_GROUPS PreviousState,
    _Out_opt_ PULONG ReturnLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtAdjustTokenClaimsAndDeviceGroups routine adjusts the user claims, device claims, and device groups in the specified access token.
 *
 * \param TokenHandle Handle to the token that contains the claims and device groups to be modified.
 * \param UserResetToDefault Specifies whether the function resets the user claims to the default state. If this value is TRUE, the function resets all user claims to their default state and ignores the NewUserState parameter.
 * \param DeviceResetToDefault Specifies whether the function resets the device claims to the default state. If this value is TRUE, the function resets all device claims to their default state and ignores the NewDeviceState parameter.
 * \param DeviceGroupsResetToDefault Specifies whether the function resets the device groups to the default state. If this value is TRUE, the function resets all device groups to their default state and ignores the NewDeviceGroupsState parameter.
 * \param NewUserState A pointer to a TOKEN_SECURITY_ATTRIBUTES_INFORMATION structure that specifies an array of user claims and their attributes. If UserResetToDefault is TRUE, the function ignores this parameter.
 * \param NewDeviceState A pointer to a TOKEN_SECURITY_ATTRIBUTES_INFORMATION structure that specifies an array of device claims and their attributes. If DeviceResetToDefault is TRUE, the function ignores this parameter.
 * \param NewDeviceGroupsState A pointer to a TOKEN_GROUPS structure that specifies an array of device groups and their attributes. If DeviceGroupsResetToDefault is TRUE, the function ignores this parameter.
 * \param UserBufferLength Specifies the size, in bytes, of the buffer pointed to by the PreviousUserState parameter. This parameter can be zero if the PreviousUserState parameter is NULL.
 * \param PreviousUserState A pointer to a buffer that the function fills with a TOKEN_SECURITY_ATTRIBUTES_INFORMATION structure that contains the previous state of any user claims that the function modifies.
 * \param DeviceBufferLength Specifies the size, in bytes, of the buffer pointed to by the PreviousDeviceState parameter. This parameter can be zero if the PreviousDeviceState parameter is NULL.
 * \param PreviousDeviceState A pointer to a buffer that the function fills with a TOKEN_SECURITY_ATTRIBUTES_INFORMATION structure that contains the previous state of any device claims that the function modifies.
 * \param DeviceGroupsBufferLength Specifies the size, in bytes, of the buffer pointed to by the PreviousDeviceGroups parameter. This parameter can be zero if the PreviousDeviceGroups parameter is NULL.
 * \param PreviousDeviceGroups A pointer to a buffer that the function fills with a TOKEN_GROUPS structure that contains the previous state of any device groups that the function modifies.
 * \param UserReturnLength A pointer to a variable that receives the required size, in bytes, of the buffer pointed to by the PreviousUserState parameter. This parameter can be NULL if PreviousUserState is NULL.
 * \param DeviceReturnLength A pointer to a variable that receives the required size, in bytes, of the buffer pointed to by the PreviousDeviceState parameter. This parameter can be NULL if PreviousDeviceState is NULL.
 * \param DeviceGroupsReturnBufferLength A pointer to a variable that receives the required size, in bytes, of the buffer pointed to by the PreviousDeviceGroups parameter. This parameter can be NULL if PreviousDeviceGroups is NULL.
 * \return NTSTATUS Successful or errant status.
 * \remarks The analyzed native implementation resolves to a shared STATUS_NOT_IMPLEMENTED (0xC0000002) stub. The
 * declaration describes the intended extended interface, not a working claim/device-group mutation on this
 * build. WoW64 still performs input/output conversion around the call; do not submit oversized arrays merely
 * to test the stub.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAdjustTokenClaimsAndDeviceGroups(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN UserResetToDefault,
    _In_ BOOLEAN DeviceResetToDefault,
    _In_ BOOLEAN DeviceGroupsResetToDefault,
    _In_opt_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION NewUserState,
    _In_opt_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION NewDeviceState,
    _In_opt_ PTOKEN_GROUPS NewDeviceGroupsState,
    _In_ ULONG UserBufferLength,
    _Out_writes_bytes_to_opt_(UserBufferLength, *UserReturnLength) PTOKEN_SECURITY_ATTRIBUTES_INFORMATION PreviousUserState,
    _In_ ULONG DeviceBufferLength,
    _Out_writes_bytes_to_opt_(DeviceBufferLength, *DeviceReturnLength) PTOKEN_SECURITY_ATTRIBUTES_INFORMATION PreviousDeviceState,
    _In_ ULONG DeviceGroupsBufferLength,
    _Out_writes_bytes_to_opt_(DeviceGroupsBufferLength, *DeviceGroupsReturnBufferLength) PTOKEN_GROUPS PreviousDeviceGroups,
    _Out_opt_ PULONG UserReturnLength,
    _Out_opt_ PULONG DeviceReturnLength,
    _Out_opt_ PULONG DeviceGroupsReturnBufferLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

// NtFilterToken Flags
#define DISABLE_MAX_PRIVILEGE   0x1 // Disables all privileges in the new token except SE_CHANGE_NOTIFY_PRIVILEGE.
#define SANDBOX_INERT           0x2 // Stores the TOKEN_SANDBOX_INERT flag in the token.
#define LUA_TOKEN               0x4
#define WRITE_RESTRICTED        0x8

/**
 * The NtFilterToken routine creates a new access token that is a restricted version of an existing access token.
 *
 * \param ExistingTokenHandle Handle to a primary or impersonation token. The token can also be a restricted token. This token must already be open for TOKEN_DUPLICATE access.
 * \param Flags Specifies additional privilege options.
 * \param SidsToDisable The deny-only SIDs to include in the restricted token. The system uses a deny-only SID to deny access to a securable object. The absence of a deny-only SID does not allow access.
 * \param PrivilegesToDelete The privileges to delete in the restricted token. This parameter is optional and can be NULL.
 * \param RestrictedSids The list of restricting SIDs for the new token. This parameter is optional and can be NULL.
 * \param NewTokenHandle The new restricted token. The new token is the same type, primary or impersonation, as the existing token.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-sefiltertoken
 * \remarks Creates an independently owned filtered token using TOKEN_DUPLICATE (0x2), rather than modifying the source
 * token in place. SID arrays and privilege-deletion arrays are captured separately; restricting SIDs are not
 * equivalent to the deny-only SID list. A zero-flags/all-NULL-list call still creates a token and is not proof
 * that any restriction was added. Close the resulting handle.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFilterToken(
    _In_ HANDLE ExistingTokenHandle,
    _In_ ULONG Flags,
    _In_opt_ PTOKEN_GROUPS SidsToDisable,
    _In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
    _In_opt_ PTOKEN_GROUPS RestrictedSids,
    _Out_ PHANDLE NewTokenHandle
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtFilterTokenEx routine creates a new access token that is a restricted version of an existing access token.
 *
 * \param ExistingTokenHandle Handle to a primary or impersonation token. The token can also be a restricted token. This token must already be open for TOKEN_DUPLICATE access.
 * \param Flags Specifies additional privilege options.
 * \param SidsToDisable The deny-only SIDs to include in the restricted token. The system uses a deny-only SID to deny access to a securable object. The absence of a deny-only SID does not allow access.
 * \param PrivilegesToDelete The privileges to delete in the restricted token. This parameter is optional and can be NULL.
 * \param RestrictedSids The list of restricting SIDs for the new token. This parameter is optional and can be NULL.
 * \param DisableUserClaimsCount The number of user claims to disable.
 * \param UserClaimsToDisable The list of user claims to disable in the restricted token. This parameter is optional and can be NULL.
 * \param DisableDeviceClaimsCount The number of device claims to disable.
 * \param DeviceClaimsToDisable The list of device claims to disable in the restricted token. This parameter is optional and can be NULL.
 * \param DeviceGroupsToDisable The list of device groups to disable in the restricted token. This parameter is optional and can be NULL.
 * \param RestrictedUserAttributes The list of restricted user attributes for the new token. This parameter is optional and can be NULL.
 * \param RestrictedDeviceAttributes The list of restricted device attributes for the new token. This parameter is optional and can be NULL.
 * \param RestrictedDeviceGroups The list of restricted device groups for the new token. This parameter is optional and can be NULL.
 * \param NewTokenHandle The new restricted token. The new token is the same type, primary or impersonation, as the existing token.
 * \return NTSTATUS Successful or errant status.
 * \remarks The analyzed native implementation is a shared STATUS_NOT_IMPLEMENTED (0xC0000002) stub, unlike the
 * working ordinary NtFilterToken path. WoW64 can capture the supplied arrays and copy its temporary output
 * handle despite native rejection. A present export or matching declaration is not evidence of supported
 * claims filtering.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtFilterTokenEx(
    _In_ HANDLE ExistingTokenHandle,
    _In_ ULONG Flags,
    _In_opt_ PTOKEN_GROUPS SidsToDisable,
    _In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
    _In_opt_ PTOKEN_GROUPS RestrictedSids,
    _In_ ULONG DisableUserClaimsCount,
    _In_opt_ PCUNICODE_STRING UserClaimsToDisable,
    _In_ ULONG DisableDeviceClaimsCount,
    _In_opt_ PCUNICODE_STRING DeviceClaimsToDisable,
    _In_opt_ PTOKEN_GROUPS DeviceGroupsToDisable,
    _In_opt_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION RestrictedUserAttributes,
    _In_opt_ PTOKEN_SECURITY_ATTRIBUTES_INFORMATION RestrictedDeviceAttributes,
    _In_opt_ PTOKEN_GROUPS RestrictedDeviceGroups,
    _Out_ PHANDLE NewTokenHandle
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

/**
 * The NtCompareTokens routine compares two access tokens to determine whether they are equivalent.
 *
 * \param FirstTokenHandle Handle to the first access token to compare. The handle must have TOKEN_QUERY access.
 * \param SecondTokenHandle Handle to the second access token to compare. The handle must have TOKEN_QUERY access.
 * \param Equal Pointer to a BOOLEAN variable that receives TRUE if the tokens are equivalent, or FALSE otherwise.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/secauthz/ntcomparetokens
 * \remarks Returns a comparison execution status separately from the BOOLEAN equality decision; STATUS_SUCCESS alone
 * does not mean equal. Both handles require TOKEN_QUERY (0x8). The native entry recognizes identical
 * handles/objects and otherwise calls SeCompareTokens with a live output pointer, even when decompiler
 * inference omits that argument. Equality is not token-handle identity or necessarily equality of every
 * mutable metadata field.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompareTokens(
    _In_ HANDLE FirstTokenHandle,
    _In_ HANDLE SecondTokenHandle,
    _Out_ PBOOLEAN Equal
    );

/**
 * The NtPrivilegeCheck routine determines whether a specified set of privileges are enabled in the access token of a client.
 *
 * \param ClientToken Handle to the access token of the client whose privileges are to be checked. The handle must have TOKEN_QUERY access.
 * \param RequiredPrivileges Pointer to a PRIVILEGE_SET structure that specifies the set of privileges to be checked. The array is input/output: privileges found enabled are marked SE_PRIVILEGE_USED_FOR_ACCESS (0x80000000).
 * \param Result Pointer to a BOOLEAN variable that receives the privilege decision. PRIVILEGE_SET_ALL_NECESSARY (1) requires all listed privileges; otherwise, at least one must be enabled.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-privilegecheck
 * \remarks Result is a BOOLEAN decision, separate from the NTSTATUS execution result. The privilege array is
 * input/output and enabled matches can acquire SE_PRIVILEGE_USED_FOR_ACCESS (0x80000000). The native path
 * requires 4-byte alignment and TOKEN_QUERY (0x8); an impersonation token below SecurityIdentification is
 * rejected. It checks privileges already enabled, without enabling them.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeCheck(
    _In_ HANDLE ClientToken,
    _Inout_ PPRIVILEGE_SET RequiredPrivileges,
    _Out_ PBOOLEAN Result
    );

/**
 * The NtImpersonateAnonymousToken routine causes a thread to impersonate the anonymous token.
 *
 * \param ThreadHandle Handle to the thread that will impersonate the anonymous token. The handle must have THREAD_DIRECT_IMPERSONATION access.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateAnonymousToken(
    _In_ HANDLE ThreadHandle
    );

/**
 * The NtQuerySecurityAttributesToken routine retrieves security attribute information from an access token.
 *
 * \param TokenHandle Handle to the access token from which to retrieve security attributes. The handle must have TOKEN_QUERY access.
 * \param Attributes Pointer to an array of UNICODE_STRING structures specifying the names of the attributes to query. This parameter can be NULL if NumberOfAttributes is zero.
 * \param NumberOfAttributes The number of attributes specified in the Attributes array.
 * \param Buffer Optional output buffer receiving TOKEN_SECURITY_ATTRIBUTES_INFORMATION and its variable data. Buffer must be NULL exactly when Length is zero.
 * \param Length The size, in bytes, of the Buffer parameter.
 * \param ReturnLength Required writable pointer receiving the complete required size in bytes; use Buffer = NULL and Length = 0 for a sizing query.
 * \return NTSTATUS Successful or errant status.
 * \remarks The native path captures an array of counted attribute names and references the token with TOKEN_QUERY
 * (0x8). The result is TOKEN_SECURITY_ATTRIBUTES_INFORMATION plus variable data, not a flat array of values.
 * WoW64 widens names and converts the pointer-bearing output; native required size and 32-bit packed output
 * must not be assumed identical. Keep name-count and byte-capacity arithmetic separate.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySecurityAttributesToken(
    _In_ HANDLE TokenHandle,
    _In_reads_opt_(NumberOfAttributes) PCUNICODE_STRING Attributes,
    _In_ ULONG NumberOfAttributes,
    _Out_writes_bytes_opt_(Length) PVOID Buffer, // PTOKEN_SECURITY_ATTRIBUTES_INFORMATION
    _In_ ULONG Length,
    _Out_ PULONG ReturnLength
    );

//
// Access checking
//

/**
 * The NtAccessCheck routine determines whether a security descriptor grants a specified set of access rights to the client represented by an access token.
 *
 * \param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure against which access is checked.
 * \param ClientToken Handle to the access token representing the client. The handle must have TOKEN_QUERY access.
 * \param DesiredAccess Object-specific access mask to check. Map GENERIC_READ/WRITE/EXECUTE/ALL first; unmapped generic bits return STATUS_GENERIC_NOT_MAPPED.
 * \param GenericMapping Pointer to the GENERIC_MAPPING structure associated with the object for which access is being checked.
 * \param PrivilegeSet Pointer to a PRIVILEGE_SET structure that receives the privileges required to access the object. The buffer must be large enough to hold the privilege set.
 * \param PrivilegeSetLength Pointer to a variable that specifies the size, in bytes, of the PrivilegeSet buffer. On input, this is the size of the buffer; on output, it receives the number of bytes required.
 * \param GrantedAccess Pointer to an access mask that receives the granted access rights.
 * \param AccessStatus Pointer to a variable that receives the results of the access check.
 * \return NTSTATUS for execution of the check. A successful return does not itself grant access: inspect AccessStatus and GrantedAccess (or their result arrays).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-accesscheck
 * \remarks For an ordinary token handle, the user-mode path requires an impersonation token at SecurityIdentification
 * or higher and TOKEN_QUERY (0x8); a primary token is rejected with STATUS_NO_IMPERSONATION_TOKEN.
 * AccessStatus is itself NTSTATUS, not the Win32 BOOL result. An empty DACL can yield syscall STATUS_SUCCESS
 * with AccessStatus = STATUS_ACCESS_DENIED and GrantedAccess = 0. Map generic bits before calling; do not
 * equate successful execution with authorization.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_bytes_(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus
    );

/**
 * The NtAccessCheckByType routine determines whether a security descriptor grants a specified set of access rights to the client represented by an access token, taking into account object type information.
 *
 * \param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure against which access is checked.
 * \param PrincipalSelfSid Optional pointer to a SID structure representing the principal self SID, or NULL.
 * \param ClientToken Handle to the access token representing the client. The handle must have TOKEN_QUERY access.
 * \param DesiredAccess Object-specific access mask to check. Map GENERIC_READ/WRITE/EXECUTE/ALL first; unmapped generic bits return STATUS_GENERIC_NOT_MAPPED.
 * \param ObjectTypeList Pointer to an array of OBJECT_TYPE_LIST structures that specify the hierarchy of object types for the object being accessed.
 * \param ObjectTypeListLength The number of elements in the ObjectTypeList array.
 * \param GenericMapping Pointer to the GENERIC_MAPPING structure associated with the object for which access is being checked.
 * \param PrivilegeSet Pointer to a PRIVILEGE_SET structure that receives the privileges required to access the object. The buffer must be large enough to hold the privilege set.
 * \param PrivilegeSetLength Pointer to a variable that specifies the size, in bytes, of the PrivilegeSet buffer. On input, this is the size of the buffer; on output, it receives the number of bytes required.
 * \param GrantedAccess Pointer to an access mask that receives the granted access rights.
 * \param AccessStatus Pointer to a variable that receives the results of the access check.
 * \return NTSTATUS for execution of the check. A successful return does not itself grant access: inspect AccessStatus and GrantedAccess (or their result arrays).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-accesscheckbytype
 * \remarks Uses SeAccessCheckByType in scalar-result mode. ObjectTypeList describes a GUID hierarchy for
 * object-specific ACE evaluation, not a list of handles to open. Supply an impersonation token, map generic
 * access bits, and inspect the separate AccessStatus decision. The scalar output is not a per-entry result
 * array; privilege-set byte sizing is independent of ObjectTypeListLength.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByType(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_reads_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_bytes_(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus
    );

/**
 * The NtAccessCheckByTypeResultList routine determines whether a security descriptor grants a specified set of access rights to the client represented by an access token, and returns the results for each object type in a list.
 *
 * \param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure against which access is checked.
 * \param PrincipalSelfSid Optional pointer to a SID structure representing the principal self SID, or NULL.
 * \param ClientToken Handle to the access token representing the client. The handle must have TOKEN_QUERY access.
 * \param DesiredAccess Object-specific access mask to check. Map GENERIC_READ/WRITE/EXECUTE/ALL first; unmapped generic bits return STATUS_GENERIC_NOT_MAPPED.
 * \param ObjectTypeList Pointer to an array of OBJECT_TYPE_LIST structures that specify the hierarchy of object types for the object being accessed.
 * \param ObjectTypeListLength The number of elements in the ObjectTypeList array.
 * \param GenericMapping Pointer to the GENERIC_MAPPING structure associated with the object for which access is being checked.
 * \param PrivilegeSet Pointer to a PRIVILEGE_SET structure that receives the privileges required to access the object. The buffer must be large enough to hold the privilege set.
 * \param PrivilegeSetLength Pointer to a variable that specifies the size, in bytes, of the PrivilegeSet buffer. On input, this is the size of the buffer; on output, it receives the number of bytes required.
 * \param GrantedAccess Pointer to an array of access masks that receive the granted access rights for each object type.
 * \param AccessStatus Pointer to an array of NTSTATUS values that receive the results of the access check for each object type.
 * \return NTSTATUS for execution of the check. A successful return does not itself grant access: inspect AccessStatus and GrantedAccess (or their result arrays).
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-accesscheckbytyperesultlist
 * \remarks Uses the same access-check helper with result-list mode enabled. GrantedAccess and AccessStatus must each
 * provide one element per ObjectTypeList entry; AccessStatus elements are NTSTATUS values.
 * ObjectTypeListLength is a count while PrivilegeSetLength is bytes. A successful syscall does not mean all
 * object-type decisions granted access.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultList(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_reads_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_bytes_(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatus
    );

//
// Signing
//

#define SIGNING_LEVEL_FILE_CACHE_FLAG_NOT_VALIDATED  0x01
#define SIGNING_LEVEL_FILE_CACHE_FLAG_VALIDATE_ONLY  0x04

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The NtGetCachedSigningLevel routine retrieves the cached signing level of a file.
 *
 * \param File File handle with FILE_READ_DATA access.
 * \param Flags Pointer to the flags set on the file.
 * \param SigningLevel Pointer to the signing level.
 * \param Thumbprint Pointer to the thumbprint.
 * \param ThumbprintSize Optional input/output byte count: input capacity of Thumbprint, output required or returned size. A short buffer returns STATUS_BUFFER_TOO_SMALL.
 * \param ThumbprintAlgorithm Pointer to the thumbprint algorithm.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-getcachedsigninglevel
 * \remarks Reads cached Code Integrity metadata; it is not a fresh signature-verification operation. The native path
 * requires FILE_READ_DATA (0x1). Flags and the 1-byte SigningLevel are written after the policy callback
 * succeeds; thumbprint copying has its own capacity check and can then return STATUS_BUFFER_TOO_SMALL. Do not
 * assume all outputs stay unchanged on a later thumbprint-size failure.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetCachedSigningLevel(
    _In_ HANDLE File,
    _Out_ PULONG Flags,
    _Out_ PSE_SIGNING_LEVEL SigningLevel,
    _Out_writes_bytes_to_opt_(*ThumbprintSize, *ThumbprintSize) PUCHAR Thumbprint,
    _Inout_opt_ PULONG ThumbprintSize,
    _Out_opt_ PULONG ThumbprintAlgorithm
    );

/**
 * The NtSetCachedSigningLevel routine sets the cached signing level of a file.
 *
 * \param Flags Input signing-cache flags, passed by value.
 * \param InputSigningLevel Requested signing level, passed by value.
 * \param SourceFiles Pointer to a set of source file handles.
 * \param SourceFileCount The source file count.
 * \param TargetFile The target file.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-setcachedsigninglevel
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetCachedSigningLevel(
    _In_ ULONG Flags,
    _In_ SE_SIGNING_LEVEL InputSigningLevel,
    _In_reads_(SourceFileCount) PHANDLE SourceFiles,
    _In_ ULONG SourceFileCount,
    _In_opt_ HANDLE TargetFile
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8)

// rev
typedef struct _SE_FILE_CACHE_CLAIM_INFORMATION
{
    ULONG Size;
    PVOID Claim;
} SE_FILE_CACHE_CLAIM_INFORMATION, *PSE_FILE_CACHE_CLAIM_INFORMATION;

// rev
typedef struct _SE_SET_FILE_CACHE_INFORMATION
{
    ULONG Size;
    UNICODE_STRING CatalogDirectoryPath;
    SE_FILE_CACHE_CLAIM_INFORMATION OriginClaimInfo;
} SE_SET_FILE_CACHE_INFORMATION, *PSE_SET_FILE_CACHE_INFORMATION;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
/**
 * The NtSetCachedSigningLevel2 routine sets the cached signing level of a file.
 *
 * \param Flags Input signing-cache flags, passed by value.
 * \param InputSigningLevel Requested signing level, passed by value.
 * \param SourceFiles Pointer to a set of source file handles.
 * \param SourceFileCount The source file count.
 * \param TargetFile The target file.
 * \param CacheInformation Pointer to the cache information.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetCachedSigningLevel2(
    _In_ ULONG Flags,
    _In_ SE_SIGNING_LEVEL InputSigningLevel,
    _In_reads_(SourceFileCount) PHANDLE SourceFiles,
    _In_ ULONG SourceFileCount,
    _In_opt_ HANDLE TargetFile,
    _In_opt_ SE_SET_FILE_CACHE_INFORMATION* CacheInformation
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
// rev
/**
 * The NtCompareSigningLevels routine compares two signing levels.
 *
 * \param FirstSigningLevel The first signing level to compare.
 * \param SecondSigningLevel The second signing level to compare.
 * \return STATUS_SUCCESS when the Code Integrity comparison accepts the levels; STATUS_INVALID_IMAGE_HASH (0xc0000428) otherwise. This is not a Boolean or three-way comparison result.
 * \remarks Returns NTSTATUS from a Code Integrity comparison callback decision: success when accepted, otherwise
 * STATUS_INVALID_IMAGE_HASH (0xC0000428), including when the callback is absent. The result is not a signed
 * numeric ordering of the two level bytes, and comparing levels does not validate a file signature.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCompareSigningLevels(
    _In_ SE_SIGNING_LEVEL FirstSigningLevel,
    _In_ SE_SIGNING_LEVEL SecondSigningLevel
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)

//
// Audit alarm
//

/**
 * The NtAccessCheckAndAuditAlarm routine determines whether a security descriptor grants a specified set of access rights to the client being impersonated by the calling thread.
 * If the security descriptor has a SACL with ACEs that apply to the client, the function generates any necessary audit messages in the security event log.
 *
 * \param SubsystemName A pointer to a counted UNICODE_STRING specifying the subsystem name.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param ObjectTypeName A pointer to a counted UNICODE_STRING specifying the object type name.
 * \param ObjectName A pointer to a counted UNICODE_STRING specifying the object name.
 * \param SecurityDescriptor A pointer to the SECURITY_DESCRIPTOR structure against which access is checked.
 * \param DesiredAccess Access mask that specifies the access rights to check. This mask must have been mapped by the MapGenericMask function to contain no generic access rights.
 * \param GenericMapping A pointer to the GENERIC_MAPPING structure associated with the object for which access is being checked.
 * \param ObjectCreation Specifies a flag that determines whether the calling application will create a new object when access is granted.
 * \param GrantedAccess A pointer to an access mask that receives the granted access rights.
 * \param AccessStatus A pointer to a variable that receives the results of the access check.
 * \param GenerateOnClose A pointer to a flag set by the audit-generation routine when the function returns.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-accesscheckandauditalarma
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckAndAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PCUNICODE_STRING ObjectTypeName,
    _In_ PCUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose
    );

/**
 * The NtAccessCheckByTypeAndAuditAlarm routine determines whether a security descriptor grants a specified set of access rights to the client being impersonated by the calling thread.
 * If the security descriptor has a SACL with ACEs that apply to the client, the function generates any necessary audit messages in the security event log.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param ObjectTypeName A pointer to a UNICODE_STRING specifying the type of object being created or accessed.
 * \param ObjectName A pointer to a UNICODE_STRING specifying the name of the object being created or accessed.
 * \param SecurityDescriptor A pointer to the SECURITY_DESCRIPTOR structure against which access is checked.
 * \param PrincipalSelfSid A pointer to a SID structure representing the principal self SID, or NULL.
 * \param DesiredAccess Access mask that specifies the access rights to check. This mask must have been mapped by the MapGenericMask function to contain no generic access rights.
 * \param AuditType Specifies the type of audit event to be generated.
 * \param Flags Audit event flags.
 * \param ObjectTypeList A pointer to an array of OBJECT_TYPE_LIST structures that specify the hierarchy of object types for the object being accessed.
 * \param ObjectTypeListLength The number of elements in the ObjectTypeList array.
 * \param GenericMapping A pointer to the GENERIC_MAPPING structure associated with the object for which access is being checked.
 * \param ObjectCreation Specifies a flag that determines whether the calling application will create a new object when access is granted.
 * \param GrantedAccess A pointer to an access mask that receives the granted access rights.
 * \param AccessStatus A pointer to a variable that receives the results of the access check.
 * \param GenerateOnClose A pointer to a flag set by the audit-generation routine when the function returns.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-accesscheckbytypeandauditalarma
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeAndAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PCUNICODE_STRING ObjectTypeName,
    _In_ PCUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ ULONG Flags,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose
    );

/**
 * The NtAccessCheckByTypeResultListAndAuditAlarm routine determines whether a security descriptor grants a specified set of access rights to the client being impersonated by the calling thread.
 * It also generates audit messages for each object type in the hierarchy, and returns the results for each object type in a list.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param ObjectTypeName A pointer to a UNICODE_STRING specifying the type of object being created or accessed.
 * \param ObjectName A pointer to a UNICODE_STRING specifying the name of the object being created or accessed.
 * \param SecurityDescriptor A pointer to the SECURITY_DESCRIPTOR structure against which access is checked.
 * \param PrincipalSelfSid A pointer to a SID structure representing the principal self SID, or NULL.
 * \param DesiredAccess Access mask that specifies the access rights to check.
 * \param AuditType Specifies the type of audit event to be generated.
 * \param Flags Audit event flags.
 * \param ObjectTypeList A pointer to an array of OBJECT_TYPE_LIST structures that specify the hierarchy of object types for the object being accessed.
 * \param ObjectTypeListLength The number of elements in the ObjectTypeList array.
 * \param GenericMapping A pointer to the GENERIC_MAPPING structure associated with the object for which access is being checked.
 * \param ObjectCreation Specifies a flag that determines whether the calling application will create a new object when access is granted.
 * \param GrantedAccess A pointer to an array of access masks that receive the granted access rights for each object type.
 * \param AccessStatus A pointer to an array of NTSTATUS values that receive the results of the access check for each object type.
 * \param GenerateOnClose A pointer to a flag set by the audit-generation routine when the function returns.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-accesscheckbytyperesultlistandauditalarma
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PCUNICODE_STRING ObjectTypeName,
    _In_ PCUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ ULONG Flags,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose
    );

/**
 * The NtAccessCheckByTypeResultListAndAuditAlarmByHandle routine determines whether a security descriptor grants a specified set of access rights to the client represented by a specified access token.
 * It also generates audit messages for each object type in the hierarchy, and returns the results for each object type in a list.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param ClientToken Handle to the access token representing the client.
 * \param ObjectTypeName A pointer to a UNICODE_STRING specifying the type of object being created or accessed.
 * \param ObjectName A pointer to a UNICODE_STRING specifying the name of the object being created or accessed.
 * \param SecurityDescriptor A pointer to the SECURITY_DESCRIPTOR structure against which access is checked.
 * \param PrincipalSelfSid A pointer to a SID structure representing the principal self SID, or NULL.
 * \param DesiredAccess Access mask that specifies the access rights to check.
 * \param AuditType Specifies the type of audit event to be generated.
 * \param Flags Audit event flags.
 * \param ObjectTypeList A pointer to an array of OBJECT_TYPE_LIST structures that specify the hierarchy of object types for the object being accessed.
 * \param ObjectTypeListLength The number of elements in the ObjectTypeList array.
 * \param GenericMapping A pointer to the GENERIC_MAPPING structure associated with the object for which access is being checked.
 * \param ObjectCreation Specifies a flag that determines whether the calling application will create a new object when access is granted.
 * \param GrantedAccess A pointer to an array of access masks that receive the granted access rights for each object type.
 * \param AccessStatus A pointer to an array of NTSTATUS values that receive the results of the access check for each object type.
 * \param GenerateOnClose A pointer to a flag set by the audit-generation routine when the function returns.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-accesscheckbytyperesultlistandauditalarmbyhandlea
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ HANDLE ClientToken,
    _In_ PCUNICODE_STRING ObjectTypeName,
    _In_ PCUNICODE_STRING ObjectName,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ AUDIT_EVENT_TYPE AuditType,
    _In_ ULONG Flags,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ BOOLEAN ObjectCreation,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatus,
    _Out_ PBOOLEAN GenerateOnClose
    );

/**
 * The NtOpenObjectAuditAlarm routine generates an audit message in the security event log when an object is opened.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param ObjectTypeName A pointer to a UNICODE_STRING specifying the type of object being opened.
 * \param ObjectName A pointer to a UNICODE_STRING specifying the name of the object being opened.
 * \param SecurityDescriptor A pointer to the SECURITY_DESCRIPTOR structure for the object.
 * \param ClientToken Handle to the access token representing the client.
 * \param DesiredAccess Access mask that specifies the access rights requested.
 * \param GrantedAccess Access mask that specifies the access rights granted.
 * \param Privileges A pointer to a PRIVILEGE_SET structure that specifies the privileges used to gain access, or NULL.
 * \param ObjectCreation Specifies a flag that determines whether the object is being created.
 * \param AccessGranted Specifies a flag that determines whether access was granted.
 * \param GenerateOnClose A pointer to a flag set by the audit-generation routine when the function returns.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenObjectAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ PCUNICODE_STRING ObjectTypeName,
    _In_ PCUNICODE_STRING ObjectName,
    _In_opt_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ACCESS_MASK GrantedAccess,
    _In_opt_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN ObjectCreation,
    _In_ BOOLEAN AccessGranted,
    _Out_ PBOOLEAN GenerateOnClose
    );

/**
 * The NtPrivilegeObjectAuditAlarm routine generates an audit message in the security event log when a privilege is used to access an object.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param ClientToken Handle to the access token representing the client.
 * \param DesiredAccess Access mask that specifies the access rights requested.
 * \param Privileges A pointer to a PRIVILEGE_SET structure that specifies the privileges used to gain access.
 * \param AccessGranted Specifies a flag that determines whether access was granted.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegeObjectAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted
    );

/**
 * The NtCloseObjectAuditAlarm routine generates an audit message in the security event log when an object handle is closed.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param GenerateOnClose Specifies a flag that determines whether to generate an audit on close.
 * \return NTSTATUS Successful or errant status.
 * \remarks GenerateOnClose = FALSE is an immediate STATUS_SUCCESS no-op in the native entry. TRUE takes
 * the audit-privilege/string-capture path. The routine reports an audit event and does not invoke NtClose on
 * HandleId; object-handle ownership remains the caller responsibility.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCloseObjectAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ BOOLEAN GenerateOnClose
    );

/**
 * The NtDeleteObjectAuditAlarm routine generates an audit message in the security event log when an object is deleted.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param HandleId A pointer to a unique value representing the client's handle to the object.
 * \param GenerateOnClose Specifies a flag that determines whether to generate an audit on close.
 * \return NTSTATUS Successful or errant status.
 * \remarks GenerateOnClose = FALSE is an immediate STATUS_SUCCESS no-op. TRUE performs audit
 * privilege/context handling. It reports deletion to auditing; it does not delete the object or close the
 * opaque HandleId correlation value.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteObjectAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_opt_ PVOID HandleId,
    _In_ BOOLEAN GenerateOnClose
    );

/**
 * The NtPrivilegedServiceAuditAlarm routine generates an audit message in the security event log when a privileged service is accessed.
 *
 * \param SubsystemName A pointer to a UNICODE_STRING specifying the name of the subsystem calling the function.
 * \param ServiceName A pointer to a UNICODE_STRING specifying the name of the service being accessed.
 * \param ClientToken Handle to the access token representing the client.
 * \param Privileges A pointer to a PRIVILEGE_SET structure that specifies the privileges used to access the service.
 * \param AccessGranted Specifies a flag that determines whether access was granted.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-privilegedserviceauditalarma
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPrivilegedServiceAuditAlarm(
    _In_ PCUNICODE_STRING SubsystemName,
    _In_ PCUNICODE_STRING ServiceName,
    _In_ HANDLE ClientToken,
    _In_ PPRIVILEGE_SET Privileges,
    _In_ BOOLEAN AccessGranted
    );

//
// KSecDD FS control definitions
//
#define KSEC_DEVICE_NAME L"\\Device\\KSecDD"
#define IOCTL_KSEC_CONNECT_LSA                      CTL_CODE(FILE_DEVICE_KSEC,  0, METHOD_BUFFERED,     FILE_WRITE_ACCESS )
#define IOCTL_KSEC_RNG                              CTL_CODE(FILE_DEVICE_KSEC,  1, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_RNG_REKEY                        CTL_CODE(FILE_DEVICE_KSEC,  2, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_ENCRYPT_MEMORY                   CTL_CODE(FILE_DEVICE_KSEC,  3, METHOD_OUT_DIRECT,   FILE_ANY_ACCESS )
#define IOCTL_KSEC_DECRYPT_MEMORY                   CTL_CODE(FILE_DEVICE_KSEC,  4, METHOD_OUT_DIRECT,   FILE_ANY_ACCESS )
#define IOCTL_KSEC_ENCRYPT_MEMORY_CROSS_PROC        CTL_CODE(FILE_DEVICE_KSEC,  5, METHOD_OUT_DIRECT,   FILE_ANY_ACCESS )
#define IOCTL_KSEC_DECRYPT_MEMORY_CROSS_PROC        CTL_CODE(FILE_DEVICE_KSEC,  6, METHOD_OUT_DIRECT,   FILE_ANY_ACCESS )
#define IOCTL_KSEC_ENCRYPT_MEMORY_SAME_LOGON        CTL_CODE(FILE_DEVICE_KSEC,  7, METHOD_OUT_DIRECT,   FILE_ANY_ACCESS )
#define IOCTL_KSEC_DECRYPT_MEMORY_SAME_LOGON        CTL_CODE(FILE_DEVICE_KSEC,  8, METHOD_OUT_DIRECT,   FILE_ANY_ACCESS )
#define IOCTL_KSEC_FIPS_GET_FUNCTION_TABLE          CTL_CODE(FILE_DEVICE_KSEC,  9, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_ALLOC_POOL                       CTL_CODE(FILE_DEVICE_KSEC, 10, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_FREE_POOL                        CTL_CODE(FILE_DEVICE_KSEC, 11, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_COPY_POOL                        CTL_CODE(FILE_DEVICE_KSEC, 12, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_DUPLICATE_HANDLE                 CTL_CODE(FILE_DEVICE_KSEC, 13, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_REGISTER_EXTENSION               CTL_CODE(FILE_DEVICE_KSEC, 14, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_CLIENT_CALLBACK                  CTL_CODE(FILE_DEVICE_KSEC, 15, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_GET_BCRYPT_EXTENSION             CTL_CODE(FILE_DEVICE_KSEC, 16, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_GET_SSL_EXTENSION                CTL_CODE(FILE_DEVICE_KSEC, 17, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_GET_DEVICECONTROL_EXTENSION      CTL_CODE(FILE_DEVICE_KSEC, 18, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_ALLOC_VM                         CTL_CODE(FILE_DEVICE_KSEC, 19, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_FREE_VM                          CTL_CODE(FILE_DEVICE_KSEC, 20, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_COPY_VM                          CTL_CODE(FILE_DEVICE_KSEC, 21, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_CLIENT_FREE_VM                   CTL_CODE(FILE_DEVICE_KSEC, 22, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_INSERT_PROTECTED_PROCESS_ADDRESS CTL_CODE(FILE_DEVICE_KSEC, 23, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_REMOVE_PROTECTED_PROCESS_ADDRESS CTL_CODE(FILE_DEVICE_KSEC, 24, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_GET_BCRYPT_EXTENSION2            CTL_CODE(FILE_DEVICE_KSEC, 25, METHOD_BUFFERED,     FILE_ANY_ACCESS )
#define IOCTL_KSEC_IPC_GET_QUEUED_FUNCTION_CALLS    CTL_CODE(FILE_DEVICE_KSEC, 26, METHOD_OUT_DIRECT,   FILE_ANY_ACCESS)
#define IOCTL_KSEC_IPC_SET_FUNCTION_RETURN          CTL_CODE(FILE_DEVICE_KSEC, 27, METHOD_NEITHER,      FILE_ANY_ACCESS)

#endif // _NTSEAPI_H
