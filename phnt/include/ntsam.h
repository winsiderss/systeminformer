/*
 * Security Account Manager support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTSAM_H
#define _NTSAM_H

//
// Types
//

#define SAM_MAXIMUM_LOOKUP_COUNT (1000)
#define SAM_MAXIMUM_LOOKUP_LENGTH (32000)
#define SAM_MAX_PASSWORD_LENGTH (256)
#define SAM_PASSWORD_ENCRYPTION_SALT_LEN (16)

/**
 * A handle to a SAM object (server, domain, user, group, alias, etc.).
 *
 * Use PSAM_HANDLE for a pointer to a SAM_HANDLE.
 */
typedef PVOID SAM_HANDLE, *PSAM_HANDLE;

/**
 * A SAM enumeration handle used to continue enumeration operations.
 */
typedef ULONG SAM_ENUMERATE_HANDLE, *PSAM_ENUMERATE_HANDLE;

/**
 * The SAM_RID_ENUMERATION structure associates a relative identifier (RID)
 * with a name. It is used when enumerating accounts by RID.
 */
typedef struct _SAM_RID_ENUMERATION
{
    ULONG RelativeId;
    UNICODE_STRING Name;
} SAM_RID_ENUMERATION, *PSAM_RID_ENUMERATION;

/**
 * The SAM_SID_ENUMERATION structure associates a SID with a name. It is used
 * when enumerating accounts by SID.
 */
typedef struct _SAM_SID_ENUMERATION
{
    PSID Sid;
    UNICODE_STRING Name;
} SAM_SID_ENUMERATION, *PSAM_SID_ENUMERATION;

/**
 * A variable-length byte array used by SAM APIs.
 *
 * Size specifies the number of valid bytes in Data.
 */
typedef struct _SAM_BYTE_ARRAY
{
    ULONG Size;
    _Field_size_bytes_(Size) PUCHAR Data;
} SAM_BYTE_ARRAY, *PSAM_BYTE_ARRAY;

/**
 * A SAM byte array constrained to a 32K maximum size.
 */
typedef struct _SAM_BYTE_ARRAY_32K
{
    ULONG Size;
    _Field_size_bytes_(Size) PUCHAR Data;
} SAM_BYTE_ARRAY_32K, *PSAM_BYTE_ARRAY_32K;

/**
 * Alias for SAM_BYTE_ARRAY_32K used for shell object properties.
 */
typedef SAM_BYTE_ARRAY_32K SAM_SHELL_OBJECT_PROPERTIES, *PSAM_SHELL_OBJECT_PROPERTIES;

//
// Basic
//

/**
 * Frees a buffer allocated by a SAM function.
 *
 * \param Buffer The buffer to free.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamFreeMemory(
    _In_ PVOID Buffer
    );

/**
 * The SamCloseHandle method closes (that is, releases server-side resources used by) any handle.
 *
 * \param SamHandle The object handle.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/55d134df-e257-48ad-8afa-cb2ca45cd3cc
 */
NTSYSAPI
NTSTATUS
NTAPI
SamCloseHandle(
    _In_ SAM_HANDLE SamHandle
    );

/**
 * Sets security information for a SAM object.
 *
 * \param ObjectHandle A handle to the SAM object.
 * \param SecurityInformation The security descriptor components to set.
 * \param SecurityDescriptor The security descriptor containing the requested information.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamSetSecurityObject(
    _In_ SAM_HANDLE ObjectHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

/**
 * The SamQuerySecurityObject method queries the access control on a server, domain, user, group, or alias object.
 *
 * \param ObjectHandle The "Domain", "User", "Group", or "Alias" object handle.
 * \param SecurityInformation A bit field that specifies which fields of SecurityDescriptor the client is requesting to be returned.
 * \param SecurityDescriptor A security descriptor expressing accesses that are specific to the ObjectHandle and the owner and group of the object.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/0ecf8fec-d17e-4a88-b7f1-e0f0f66790db
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQuerySecurityObject(
    _In_ SAM_HANDLE ObjectHandle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Outptr_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    );

/**
 * The SamRidToSid method obtains the SID of an account, given a RID.
 *
 * \param ObjectHandle The "Domain", "User", "Group", or "Alias" object handle.
 * \param Rid The RID of the object.
 * \param Sid The SID of the object referenced by Rid.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/00ff8192-a4f6-45ba-9f65-917e46b6a693
 */
NTSYSAPI
NTSTATUS
NTAPI
SamRidToSid(
    _In_ SAM_HANDLE ObjectHandle,
    _In_ ULONG Rid,
    _Outptr_ PSID *Sid
    );

/**
 * Queries the SID of the account managed by Windows LAPS.
 *
 * \param ObjectHandle A handle to the SAM object to query.
 * \param AccountSid Receives a pointer to the managed account SID.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQueryLapsManagedAccount(
    _In_ SAM_HANDLE ObjectHandle,
    _Outptr_ PSID *AccountSid
    );

//
// Server
//

#define SAM_SERVER_CONNECT 0x0001
#define SAM_SERVER_SHUTDOWN 0x0002
#define SAM_SERVER_INITIALIZE 0x0004
#define SAM_SERVER_CREATE_DOMAIN 0x0008
#define SAM_SERVER_ENUMERATE_DOMAINS 0x0010
#define SAM_SERVER_LOOKUP_DOMAIN 0x0020

#define SAM_SERVER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED     | \
    SAM_SERVER_CONNECT | \
    SAM_SERVER_INITIALIZE | \
    SAM_SERVER_CREATE_DOMAIN | \
    SAM_SERVER_SHUTDOWN | \
    SAM_SERVER_ENUMERATE_DOMAINS | \
    SAM_SERVER_LOOKUP_DOMAIN)

#define SAM_SERVER_READ (STANDARD_RIGHTS_READ | \
    SAM_SERVER_ENUMERATE_DOMAINS)

#define SAM_SERVER_WRITE (STANDARD_RIGHTS_WRITE | \
    SAM_SERVER_INITIALIZE | \
    SAM_SERVER_CREATE_DOMAIN | \
    SAM_SERVER_SHUTDOWN)

#define SAM_SERVER_EXECUTE (STANDARD_RIGHTS_EXECUTE | \
    SAM_SERVER_CONNECT | \
    SAM_SERVER_LOOKUP_DOMAIN)

/**
 * Opaque RPC auth identity handle used to pass credentials to SamConnectWithCreds.
 */
typedef struct _RPC_AUTH_IDENTITY_HANDLE *PRPC_AUTH_IDENTITY_HANDLE;

//
// Functions
//

/**
 * The SamConnect method returns a handle to a server.
 *
 * \param ServerName The NETBIOS name of the server; this parameter MAY be ignored on receipt.
 * \param ServerHandle A handle representing a server.
 * \param DesiredAccess The access requested for ServerHandle upon output. 
 * \param ObjectAttributes The OBJECT_ATTRIBUTES structure that specifies the properties of the server handle to be opened.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/47492d59-e095-4398-b03e-8a062b989123
 */
NTSYSAPI
NTSTATUS
NTAPI
SamConnect(
    _In_opt_ PCUNICODE_STRING ServerName,
    _Out_ PSAM_HANDLE ServerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * Connects to a SAM server using the supplied credentials.
 *
 * \param ServerName The name of the server to connect to.
 * \param ServerHandle Receives a handle to the server.
 * \param DesiredAccess The access requested for the returned server handle.
 * \param ObjectAttributes The object attributes for the connection.
 * \param Creds The RPC authentication identity containing the connection credentials.
 * \param Spn The service principal name used for authentication.
 * \param DestinationIsWindows2K Receives the destination's Windows 2000 compatibility indicator.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamConnectWithCreds(
    _In_ PCUNICODE_STRING ServerName,
    _Out_ PSAM_HANDLE ServerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ PRPC_AUTH_IDENTITY_HANDLE Creds,
    _In_ PWCHAR Spn,
    _Out_ PBOOL DestinationIsWindows2K
    );

/**
 * Requests shutdown of the SAM server.
 *
 * \param ServerHandle A handle to the server to shut down.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamShutdownSamServer(
    _In_ SAM_HANDLE ServerHandle
    );

//
// Domain
//

#define DOMAIN_READ_PASSWORD_PARAMETERS 0x0001
#define DOMAIN_WRITE_PASSWORD_PARAMS 0x0002
#define DOMAIN_READ_OTHER_PARAMETERS 0x0004
#define DOMAIN_WRITE_OTHER_PARAMETERS 0x0008
#define DOMAIN_CREATE_USER 0x0010
#define DOMAIN_CREATE_GROUP 0x0020
#define DOMAIN_CREATE_ALIAS 0x0040
#define DOMAIN_GET_ALIAS_MEMBERSHIP 0x0080
#define DOMAIN_LIST_ACCOUNTS 0x0100
#define DOMAIN_LOOKUP 0x0200
#define DOMAIN_ADMINISTER_SERVER 0x0400

#define DOMAIN_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
    DOMAIN_READ_OTHER_PARAMETERS | \
    DOMAIN_WRITE_OTHER_PARAMETERS | \
    DOMAIN_WRITE_PASSWORD_PARAMS | \
    DOMAIN_CREATE_USER | \
    DOMAIN_CREATE_GROUP | \
    DOMAIN_CREATE_ALIAS | \
    DOMAIN_GET_ALIAS_MEMBERSHIP | \
    DOMAIN_LIST_ACCOUNTS | \
    DOMAIN_READ_PASSWORD_PARAMETERS | \
    DOMAIN_LOOKUP | \
    DOMAIN_ADMINISTER_SERVER)

#define DOMAIN_READ (STANDARD_RIGHTS_READ | \
    DOMAIN_GET_ALIAS_MEMBERSHIP | \
    DOMAIN_READ_OTHER_PARAMETERS)

#define DOMAIN_WRITE (STANDARD_RIGHTS_WRITE | \
    DOMAIN_WRITE_OTHER_PARAMETERS | \
    DOMAIN_WRITE_PASSWORD_PARAMS | \
    DOMAIN_CREATE_USER | \
    DOMAIN_CREATE_GROUP | \
    DOMAIN_CREATE_ALIAS | \
    DOMAIN_ADMINISTER_SERVER)

#define DOMAIN_EXECUTE (STANDARD_RIGHTS_EXECUTE | \
    DOMAIN_READ_PASSWORD_PARAMETERS | \
    DOMAIN_LIST_ACCOUNTS | \
    DOMAIN_LOOKUP)

#define DOMAIN_PROMOTION_INCREMENT { 0x0, 0x10 }
#define DOMAIN_PROMOTION_MASK { 0x0, 0xfffffff0 }

//
// SamQueryInformationDomain/SamSetInformationDomain types
//

/**
 * The DOMAIN_INFORMATION_CLASS enumeration specifies the type of information
 * returned or set by SamQueryInformationDomain / SamSetInformationDomain.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
typedef enum _DOMAIN_INFORMATION_CLASS
{
    DomainPasswordInformation = 1,      // qs: DOMAIN_PASSWORD_INFORMATION
    DomainGeneralInformation,           // q: DOMAIN_GENERAL_INFORMATION
    DomainLogoffInformation,            // qs: DOMAIN_LOGOFF_INFORMATION
    DomainOemInformation,               // qs: DOMAIN_OEM_INFORMATION
    DomainNameInformation,              // q: DOMAIN_NAME_INFORMATION
    DomainReplicationInformation,       // qs: DOMAIN_REPLICATION_INFORMATION
    DomainServerRoleInformation,        // qs: DOMAIN_SERVER_ROLE_INFORMATION
    DomainModifiedInformation,          // q: DOMAIN_MODIFIED_INFORMATION
    DomainStateInformation,             // qs: DOMAIN_STATE_INFORMATION
    DomainUasInformation,               // qs: DOMAIN_UAS_INFORMATION
    DomainGeneralInformation2,          // q: DOMAIN_GENERAL_INFORMATION2
    DomainLockoutInformation,           // qs: DOMAIN_LOCKOUT_INFORMATION
    DomainModifiedInformation2,         // q: DOMAIN_MODIFIED_INFORMATION2
    DomainMaxInformation
} DOMAIN_INFORMATION_CLASS;

/**
 * DOMAIN_SERVER_ENABLE_STATE indicates whether the domain server is enabled
 * or disabled.
 */
typedef enum _DOMAIN_SERVER_ENABLE_STATE
{
    DomainServerEnabled = 1,
    DomainServerDisabled
} DOMAIN_SERVER_ENABLE_STATE, *PDOMAIN_SERVER_ENABLE_STATE;

/**
 * DOMAIN_SERVER_ROLE indicates whether the server is primary or backup for
 * the domain.
 */
typedef enum _DOMAIN_SERVER_ROLE
{
    DomainServerRoleBackup = 2,
    DomainServerRolePrimary
} DOMAIN_SERVER_ROLE, *PDOMAIN_SERVER_ROLE;

/**
 * DOMAIN_GENERAL_INFORMATION contains general properties of a domain such as
 * name, counts and server role/state.
 */
typedef struct _DOMAIN_GENERAL_INFORMATION
{
    LARGE_INTEGER ForceLogoff;
    UNICODE_STRING OemInformation;
    UNICODE_STRING DomainName;
    UNICODE_STRING ReplicaSourceNodeName;
    LARGE_INTEGER DomainModifiedCount;
    DOMAIN_SERVER_ENABLE_STATE DomainServerState;
    DOMAIN_SERVER_ROLE DomainServerRole;
    BOOLEAN UasCompatibilityRequired;
    ULONG UserCount;
    ULONG GroupCount;
    ULONG AliasCount;
} DOMAIN_GENERAL_INFORMATION, *PDOMAIN_GENERAL_INFORMATION;

/**
 * DOMAIN_GENERAL_INFORMATION2 extends DOMAIN_GENERAL_INFORMATION with lockout
 * related parameters.
 */
typedef struct _DOMAIN_GENERAL_INFORMATION2
{
    DOMAIN_GENERAL_INFORMATION I1;
    LARGE_INTEGER LockoutDuration; // delta time
    LARGE_INTEGER LockoutObservationWindow; // delta time
    USHORT LockoutThreshold;
} DOMAIN_GENERAL_INFORMATION2, *PDOMAIN_GENERAL_INFORMATION2;

/**
 * DOMAIN_UAS_INFORMATION indicates whether UAS compatibility is required for the domain.
 */
typedef struct _DOMAIN_UAS_INFORMATION
{
    BOOLEAN UasCompatibilityRequired;
} DOMAIN_UAS_INFORMATION;

#ifndef _DOMAIN_PASSWORD_INFORMATION_DEFINED // defined in ntsecapi.h
#define _DOMAIN_PASSWORD_INFORMATION_DEFINED

/**
 * DOMAIN_PASSWORD_INFORMATION contains password policy parameters for the domain.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
typedef struct _DOMAIN_PASSWORD_INFORMATION
{
    USHORT MinPasswordLength;
    USHORT PasswordHistoryLength;
    ULONG PasswordProperties;
    LARGE_INTEGER MaxPasswordAge;
    LARGE_INTEGER MinPasswordAge;
} DOMAIN_PASSWORD_INFORMATION, *PDOMAIN_PASSWORD_INFORMATION;

//
// PasswordProperties flags
//
#define DOMAIN_PASSWORD_COMPLEX 0x00000001L
#define DOMAIN_PASSWORD_NO_ANON_CHANGE 0x00000002L
#define DOMAIN_PASSWORD_NO_CLEAR_CHANGE 0x00000004L
#define DOMAIN_LOCKOUT_ADMINS 0x00000008L
#define DOMAIN_PASSWORD_STORE_CLEARTEXT 0x00000010L
#define DOMAIN_REFUSE_PASSWORD_CHANGE 0x00000020L
#define DOMAIN_NO_LM_OWF_CHANGE 0x00000040L

#endif // _DOMAIN_PASSWORD_INFORMATION_DEFINED

/**
 * DOMAIN_PASSWORD_CONSTRUCTION indicates password complexity requirements.
 */
typedef enum _DOMAIN_PASSWORD_CONSTRUCTION
{
    DomainPasswordSimple = 1,
    DomainPasswordComplex
} DOMAIN_PASSWORD_CONSTRUCTION;

/**
 * DOMAIN_LOGOFF_INFORMATION specifies force logoff time used by the domain.
 */
typedef struct _DOMAIN_LOGOFF_INFORMATION
{
    LARGE_INTEGER ForceLogoff;
} DOMAIN_LOGOFF_INFORMATION, *PDOMAIN_LOGOFF_INFORMATION;

/**
 * DOMAIN_OEM_INFORMATION carries OEM-specific information for the domain.
 */
typedef struct _DOMAIN_OEM_INFORMATION
{
    UNICODE_STRING OemInformation;
} DOMAIN_OEM_INFORMATION, *PDOMAIN_OEM_INFORMATION;

/**
 * DOMAIN_NAME_INFORMATION contains the domain's name.
 */
typedef struct _DOMAIN_NAME_INFORMATION
{
    UNICODE_STRING DomainName;
} DOMAIN_NAME_INFORMATION, *PDOMAIN_NAME_INFORMATION;

/**
 * DOMAIN_SERVER_ROLE_INFORMATION reports the server role for the domain.
 */
typedef struct _DOMAIN_SERVER_ROLE_INFORMATION
{
    DOMAIN_SERVER_ROLE DomainServerRole;
} DOMAIN_SERVER_ROLE_INFORMATION, *PDOMAIN_SERVER_ROLE_INFORMATION;

/**
 * DOMAIN_REPLICATION_INFORMATION contains the replication source node name.
 */
typedef struct _DOMAIN_REPLICATION_INFORMATION
{
    UNICODE_STRING ReplicaSourceNodeName;
} DOMAIN_REPLICATION_INFORMATION, *PDOMAIN_REPLICATION_INFORMATION;

/**
 * DOMAIN_MODIFIED_INFORMATION reports modification counters and creation time.
 */
typedef struct _DOMAIN_MODIFIED_INFORMATION
{
    LARGE_INTEGER DomainModifiedCount;
    LARGE_INTEGER CreationTime;
} DOMAIN_MODIFIED_INFORMATION, *PDOMAIN_MODIFIED_INFORMATION;

/**
 * DOMAIN_MODIFIED_INFORMATION2 extends DOMAIN_MODIFIED_INFORMATION with promotion data.
 */
typedef struct _DOMAIN_MODIFIED_INFORMATION2
{
    LARGE_INTEGER DomainModifiedCount;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER ModifiedCountAtLastPromotion;
} DOMAIN_MODIFIED_INFORMATION2, *PDOMAIN_MODIFIED_INFORMATION2;

/**
 * DOMAIN_STATE_INFORMATION reports the enabled/disabled state of the domain server.
 */
typedef struct _DOMAIN_STATE_INFORMATION
{
    DOMAIN_SERVER_ENABLE_STATE DomainServerState;
} DOMAIN_STATE_INFORMATION, *PDOMAIN_STATE_INFORMATION;

typedef struct _DOMAIN_LOCKOUT_INFORMATION
{
    LARGE_INTEGER LockoutDuration; // delta time
    LARGE_INTEGER LockoutObservationWindow; // delta time
    USHORT LockoutThreshold; // zero means no lockout
} DOMAIN_LOCKOUT_INFORMATION, *PDOMAIN_LOCKOUT_INFORMATION;

//
// SamQueryDisplayInformation types
//

typedef enum _DOMAIN_DISPLAY_INFORMATION
{
    DomainDisplayUser = 1,      // DOMAIN_DISPLAY_USER
    DomainDisplayMachine,       // DOMAIN_DISPLAY_MACHINE
    DomainDisplayGroup,         // DOMAIN_DISPLAY_GROUP
    DomainDisplayOemUser,       // DOMAIN_DISPLAY_OEM_USER
    DomainDisplayOemGroup,      // DOMAIN_DISPLAY_OEM_GROUP
    DomainDisplayServer,        // DOMAIN_DISPLAY_MACHINE
    DomainDisplayMax
} DOMAIN_DISPLAY_INFORMATION, *PDOMAIN_DISPLAY_INFORMATION;

typedef struct _DOMAIN_DISPLAY_USER
{
    ULONG Index;
    ULONG Rid;
    ULONG AccountControl;
    UNICODE_STRING LogonName;
    UNICODE_STRING AdminComment;
    UNICODE_STRING FullName;
} DOMAIN_DISPLAY_USER, *PDOMAIN_DISPLAY_USER;

typedef struct _DOMAIN_DISPLAY_MACHINE
{
    ULONG Index;
    ULONG Rid;
    ULONG AccountControl;
    UNICODE_STRING Machine;
    UNICODE_STRING Comment;
} DOMAIN_DISPLAY_MACHINE, *PDOMAIN_DISPLAY_MACHINE;

typedef struct _DOMAIN_DISPLAY_GROUP
{
    ULONG Index;
    ULONG Rid;
    ULONG Attributes;
    UNICODE_STRING Group;
    UNICODE_STRING Comment;
} DOMAIN_DISPLAY_GROUP, *PDOMAIN_DISPLAY_GROUP;

typedef struct _DOMAIN_DISPLAY_OEM_USER
{
    ULONG Index;
    OEM_STRING User;
} DOMAIN_DISPLAY_OEM_USER, *PDOMAIN_DISPLAY_OEM_USER;

typedef struct _DOMAIN_DISPLAY_OEM_GROUP
{
    ULONG Index;
    OEM_STRING Group;
} DOMAIN_DISPLAY_OEM_GROUP, *PDOMAIN_DISPLAY_OEM_GROUP;

//
// SamQueryLocalizableAccountsInDomain types
//

typedef enum _DOMAIN_LOCALIZABLE_ACCOUNTS_INFORMATION
{
    DomainLocalizableAccountsBasic = 1,
} DOMAIN_LOCALIZABLE_ACCOUNTS_INFORMATION, *PDOMAIN_LOCALIZABLE_ACCOUNTS_INFORMATION;

typedef struct _DOMAIN_LOCALIZABLE_ACCOUNTS_ENTRY
{
    ULONG Rid;
    SID_NAME_USE Use;
    UNICODE_STRING Name;
    UNICODE_STRING AdminComment;
} DOMAIN_LOCALIZABLE_ACCOUNT_ENTRY, *PDOMAIN_LOCALIZABLE_ACCOUNT_ENTRY;

typedef struct _DOMAIN_LOCALIZABLE_ACCOUNTS
{
    ULONG Count;
    _Field_size_(Count) DOMAIN_LOCALIZABLE_ACCOUNT_ENTRY *Entries;
} DOMAIN_LOCALIZABLE_ACCOUNTS_BASIC, *PDOMAIN_LOCALIZABLE_ACCOUNTS_BASIC;

typedef union _DOMAIN_LOCALIZABLE_INFO_BUFFER
{
    DOMAIN_LOCALIZABLE_ACCOUNTS_BASIC Basic;
} DOMAIN_LOCALIZABLE_ACCOUNTS_INFO_BUFFER, *PDOMAIN_LOCALIZABLE_ACCOUNTS_INFO_BUFFER;

//
// Functions
//

/**
 * The SamLookupDomainInSamServer method obtains the SID of a domain, given the object's name.
 *
 * \param ServerHandle A handle representing a server.
 * \param Name A UTF-16 encoded string.
 * \param DomainId A SID value of a domain that corresponds to the Name.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/47492d59-e095-4398-b03e-8a062b989123
 */
NTSYSAPI
NTSTATUS
NTAPI
SamLookupDomainInSamServer(
    _In_ SAM_HANDLE ServerHandle,
    _In_ PCUNICODE_STRING Name,
    _Outptr_ PSID *DomainId
    );

/**
 * The SamEnumerateDomainsInSamServer method obtains a listing of all domains hosted by the server side of this protocol.
 *
 * \param ServerHandle A handle representing a server.
 * \param EnumerationContext An opaque value that the server can use to continue an enumeration on a subsequent call.
 * \param Buffer A listing of domain information.
 * \param PreferedMaximumLength The requested maximum number of bytes to return in Buffer.
 * \param CountReturned The count of domain elements returned in Buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/2142fd2d-0854-42c1-a9fb-2fe964e381ce
 */
NTSYSAPI
NTSTATUS
NTAPI
SamEnumerateDomainsInSamServer(
    _In_ SAM_HANDLE ServerHandle,
    _Inout_ PSAM_ENUMERATE_HANDLE EnumerationContext,
    _Outptr_ PVOID *Buffer, // PSAM_SID_ENUMERATION *Buffer
    _In_ ULONG PreferedMaximumLength,
    _Out_ PULONG CountReturned
    );

/**
 * The SamOpenDomain method obtains a handle to a domain, given a SID.
 *
 * \param ServerHandle A handle representing a server.
 * \param DesiredAccess The desired access to the domain.
 * \param DomainId A SID value of a domain hosted by the server.
 * \param DomainHandle A handle to the requested domain.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/ba710c90-5b12-42f8-9e5a-d4aacc1329fa
 */
NTSYSAPI
NTSTATUS
NTAPI
SamOpenDomain(
    _In_ SAM_HANDLE ServerHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PSID DomainId,
    _Out_ PSAM_HANDLE DomainHandle
    );

/**
 * The SamQueryInformationDomain method obtains attributes from a domain object.
 *
 * \param DomainHandle A handle representing a domain.
 * \param DomainInformationClass An enumeration indicating which attributes to return.
 * \param Buffer The requested attributes on output.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/5d6a2817-caa9-41ca-a269-fd13ecbb4fa8
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQueryInformationDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ DOMAIN_INFORMATION_CLASS DomainInformationClass,
    _Outptr_ PVOID *Buffer
    );

/**
 * The SamSetInformationDomain method updates attributes of a domain object.
 *
 * \param DomainHandle A handle representing a domain.
 * \param DomainInformationClass An enumeration indicating which attributes to update.
 * \param Buffer The provided attributes on output.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamSetInformationDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ DOMAIN_INFORMATION_CLASS DomainInformationClass,
    _In_ PVOID Buffer
    );

/**
 * The SamLookupNamesInDomain method translates a set of account names into a set of RIDs.
 *
 * \param DomainHandle A handle representing a domain.
 * \param Count The number of elements in Names.
 * \param Names An array of strings that are to be mapped to RIDs.
 * \param RelativeIds An array of RIDs of accounts that correspond to the elements in Names.
 * \param Use An array of SID_NAME_USE enumeration values that describe the type of account for each entry in RelativeIds.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/d91271c6-7b2e-4194-9927-8fabfa429f90
 */
NTSYSAPI
NTSTATUS
NTAPI
SamLookupNamesInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ULONG Count,
    _In_reads_(Count) PCUNICODE_STRING Names,
    _Out_ _Deref_post_count_(Count) PULONG *RelativeIds,
    _Out_ _Deref_post_count_(Count) PSID_NAME_USE *Use
    );

/**
 * Translates account names in a domain into SIDs.
 *
 * \param DomainHandle A handle to the domain.
 * \param Count The number of account names in Names.
 * \param Names The account names to look up.
 * \param Sids Receives the SIDs corresponding to Names.
 * \param Use Receives the account types corresponding to Names.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamLookupNamesInDomain2(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ULONG Count,
    _In_reads_(Count) PCUNICODE_STRING Names,
    _Out_ _Deref_post_count_(Count) PSID* Sids,
    _Out_ _Deref_post_count_(Count) PSID_NAME_USE* Use
    );

/**
 * The SamLookupIdsInDomain method translates a set of RIDs into account names.
 *
 * \param DomainHandle A handle representing a domain.
 * \param Count The number of elements in RelativeIds.
 * \param RelativeIds An array of RIDs that are to be mapped to account names.
 * \param Names An array of account names that correspond to the elements in RelativeIds.
 * \param Use An array of SID_NAME_USE enumeration values that describe the type of account for each entry in RelativeIds.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/c870951c-74b3-4714-9857-224595ffc61a
 */
NTSYSAPI
NTSTATUS
NTAPI
SamLookupIdsInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ULONG Count,
    _In_reads_(Count) PULONG RelativeIds,
    _Out_ _Deref_post_count_(Count) PUNICODE_STRING *Names,
    _Out_ _Deref_post_opt_count_(Count) PSID_NAME_USE *Use
    );

/**
 * The SamRemoveMemberFromForeignDomain method removes a member from all aliases.
 *
 * \param DomainHandle A handle representing a domain.
 * \param MemberId The SID to remove from the membership.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/03afc843-584d-473b-834a-3f5a1ac86cce
 */
NTSYSAPI
NTSTATUS
NTAPI
SamRemoveMemberFromForeignDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PSID MemberId
    );

/**
 * Queries localizable account information in a domain.
 *
 * \param Domain A handle to the domain.
 * \param Flags Flags controlling the query.
 * \param LanguageId The language identifier for the requested account information.
 * \param Class The class of localizable account information to retrieve.
 * \param Buffer Receives a pointer to the information selected by Class.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQueryLocalizableAccountsInDomain(
    _In_ SAM_HANDLE Domain,
    _In_ ULONG Flags,
    _In_ ULONG LanguageId,
    _In_ DOMAIN_LOCALIZABLE_ACCOUNTS_INFORMATION Class,
    _Outptr_ PVOID *Buffer
    );

//
// Group
//

#define GROUP_READ_INFORMATION 0x0001
#define GROUP_WRITE_ACCOUNT 0x0002
#define GROUP_ADD_MEMBER 0x0004
#define GROUP_REMOVE_MEMBER 0x0008
#define GROUP_LIST_MEMBERS 0x0010

#define GROUP_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
    GROUP_LIST_MEMBERS | \
    GROUP_WRITE_ACCOUNT | \
    GROUP_ADD_MEMBER | \
    GROUP_REMOVE_MEMBER | \
    GROUP_READ_INFORMATION)

#define GROUP_READ (STANDARD_RIGHTS_READ | \
    GROUP_LIST_MEMBERS)

#define GROUP_WRITE (STANDARD_RIGHTS_WRITE | \
    GROUP_WRITE_ACCOUNT | \
    GROUP_ADD_MEMBER | \
    GROUP_REMOVE_MEMBER)

#define GROUP_EXECUTE (STANDARD_RIGHTS_EXECUTE | \
    GROUP_READ_INFORMATION)

/**
 * The GROUP_MEMBERSHIP structure describes a single membership entry for a group.
 *
 * Fields:
 * - RelativeId: The RID of the member account.
 * - Attributes: Membership attributes (group-specific flags).
 */
typedef struct _GROUP_MEMBERSHIP
{
    ULONG RelativeId;
    ULONG Attributes;
} GROUP_MEMBERSHIP, *PGROUP_MEMBERSHIP;

//
// SamQueryInformationGroup/SamSetInformationGroup types
//

/**
 * The GROUP_INFORMATION_CLASS enumeration specifies information classes
 * returned or set by SamQueryInformationGroup / SamSetInformationGroup.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
typedef enum _GROUP_INFORMATION_CLASS
{
    GroupGeneralInformation = 1,        // q: GROUP_GENERAL_INFORMATION
    GroupNameInformation,               // qs: GROUP_NAME_INFORMATION
    GroupAttributeInformation,          // qs: GROUP_ATTRIBUTE_INFORMATION
    GroupAdminCommentInformation,       // qs: GROUP_ADM_COMMENT_INFORMATION
    GroupReplicationInformation,        // q: GROUP_REPLICATION_INFORMATION
    GroupMaxInformation
} GROUP_INFORMATION_CLASS;

/**
 * GROUP_GENERAL_INFORMATION contains common properties for a group.
 *
 * Fields:
 * - Name: Group name.
 * - Attributes: Group attribute flags.
 * - MemberCount: Number of members in the group.
 * - AdminComment: Administrative comment string.
 */
typedef struct _GROUP_GENERAL_INFORMATION
{
    UNICODE_STRING Name;
    ULONG Attributes;
    ULONG MemberCount;
    UNICODE_STRING AdminComment;
} GROUP_GENERAL_INFORMATION, *PGROUP_GENERAL_INFORMATION;

/**
 * GROUP_NAME_INFORMATION contains the group's name.
 */
typedef struct _GROUP_NAME_INFORMATION
{
    UNICODE_STRING Name;
} GROUP_NAME_INFORMATION, *PGROUP_NAME_INFORMATION;

/**
 * GROUP_ATTRIBUTE_INFORMATION contains group attribute flags.
 */
typedef struct _GROUP_ATTRIBUTE_INFORMATION
{
    ULONG Attributes;
} GROUP_ATTRIBUTE_INFORMATION, *PGROUP_ATTRIBUTE_INFORMATION;

/**
 * GROUP_ADM_COMMENT_INFORMATION contains the administrative comment for a group.
 */
typedef struct _GROUP_ADM_COMMENT_INFORMATION
{
    UNICODE_STRING AdminComment;
} GROUP_ADM_COMMENT_INFORMATION, *PGROUP_ADM_COMMENT_INFORMATION;

/**
 * GROUP_REPLICATION_INFORMATION contains replication metadata for a group.
 */
typedef struct _GROUP_REPLICATION_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
} GROUP_REPLICATION_INFORMATION, *PGROUP_REPLICATION_INFORMATION;

//
// Functions
//

/**
 * The SamEnumerateGroupsInDomain method enumerates all groups.
 *
 * \param DomainHandle A handle representing a domain.
 * \param EnumerationContext An opaque value that the server can use to continue an enumeration on a subsequent call.
 * \param Buffer A listing of group information.
 * \param PreferedMaximumLength The requested maximum number of bytes to return in Buffer.
 * \param CountReturned The number of group entries returned in Buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/e0b7a4b7-ecfc-405f-9d7d-32b3cd2cd6c8
 */
NTSYSAPI
NTSTATUS
NTAPI
SamEnumerateGroupsInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _Inout_ PSAM_ENUMERATE_HANDLE EnumerationContext,
    _Outptr_ PVOID *Buffer, // PSAM_RID_ENUMERATION *
    _In_ ULONG PreferedMaximumLength,
    _Out_ PULONG CountReturned
    );

/**
 * The SamCreateGroupInDomain method creates a new group account in the specified domain.
 *
 * \param DomainHandle A handle representing the domain.
 * \param AccountName The name of the group to create.
 * \param DesiredAccess The requested access for the output group handle.
 * \param GroupHandle Receives the handle for the created group.
 * \param RelativeId Receives the RID of the created group.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamCreateGroupInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PCUNICODE_STRING AccountName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PSAM_HANDLE GroupHandle,
    _Out_ PULONG RelativeId
    );

/**
 * The SamOpenGroup method opens an existing group account by RID.
 *
 * \param DomainHandle A handle representing the domain containing the group.
 * \param DesiredAccess The access requested for GroupHandle.
 * \param GroupId The RID of the group to open.
 * \param GroupHandle Receives a handle to the opened group.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamOpenGroup(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG GroupId,
    _Out_ PSAM_HANDLE GroupHandle
    );

/**
 * The SamDeleteGroup method deletes a group account represented by GroupHandle.
 *
 * \param GroupHandle A handle to the group to delete.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamDeleteGroup(
    _In_ SAM_HANDLE GroupHandle
    );

/**
 * The SamQueryInformationGroup method queries information about a group.
 *
 * \param GroupHandle A handle to the group.
 * \param GroupInformationClass The information class to retrieve.
 * \param Buffer Receives a pointer to the returned information structure.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQueryInformationGroup(
    _In_ SAM_HANDLE GroupHandle,
    _In_ GROUP_INFORMATION_CLASS GroupInformationClass,
    _Outptr_ PVOID *Buffer
    );

/**
 * The SamSetInformationGroup method sets information for a group.
 *
 * \param GroupHandle A handle to the group.
 * \param GroupInformationClass The information class to set.
 * \param Buffer Pointer to the information structure to apply.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamSetInformationGroup(
    _In_ SAM_HANDLE GroupHandle,
    _In_ GROUP_INFORMATION_CLASS GroupInformationClass,
    _In_ PVOID Buffer
    );

/**
 * The SamAddMemberToGroup method adds a member to a group.
 *
 * \param GroupHandle A handle to the group.
 * \param MemberId The RID of the member to add.
 * \param Attributes Membership attributes for the member.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamAddMemberToGroup(
    _In_ SAM_HANDLE GroupHandle,
    _In_ ULONG MemberId,
    _In_ ULONG Attributes
    );

/**
 * The SamRemoveMemberFromGroup method removes a member from a group.
 *
 * \param GroupHandle A handle to the group.
 * \param MemberId The RID of the member to remove.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamRemoveMemberFromGroup(
    _In_ SAM_HANDLE GroupHandle,
    _In_ ULONG MemberId
    );

/**
 * The SamGetMembersInGroup method retrieves the members of a group.
 *
 * \param GroupHandle A handle to the group.
 * \param MemberIds Receives an array of member RIDs (allocated by the server).
 * \param Attributes Receives member attributes array parallel to MemberIds.
 * \param MemberCount Receives the number of members returned.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamGetMembersInGroup(
    _In_ SAM_HANDLE GroupHandle,
    _Out_ _Deref_post_count_(*MemberCount) PULONG *MemberIds,
    _Out_ _Deref_post_count_(*MemberCount) PULONG *Attributes,
    _Out_ PULONG MemberCount
    );

/**
 * The SamSetMemberAttributesOfGroup method sets membership attributes for a group member.
 *
 * \param GroupHandle A handle to the group.
 * \param MemberId The RID of the member.
 * \param Attributes The new membership attributes.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamSetMemberAttributesOfGroup(
    _In_ SAM_HANDLE GroupHandle,
    _In_ ULONG MemberId,
    _In_ ULONG Attributes
    );

//
// Alias
//

#define ALIAS_ADD_MEMBER 0x0001
#define ALIAS_REMOVE_MEMBER 0x0002
#define ALIAS_LIST_MEMBERS 0x0004
#define ALIAS_READ_INFORMATION 0x0008
#define ALIAS_WRITE_ACCOUNT 0x0010

#define ALIAS_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
    ALIAS_READ_INFORMATION | \
    ALIAS_WRITE_ACCOUNT | \
    ALIAS_LIST_MEMBERS | \
    ALIAS_ADD_MEMBER | \
    ALIAS_REMOVE_MEMBER)

#define ALIAS_READ (STANDARD_RIGHTS_READ | \
    ALIAS_LIST_MEMBERS)

#define ALIAS_WRITE (STANDARD_RIGHTS_WRITE | \
    ALIAS_WRITE_ACCOUNT | \
    ALIAS_ADD_MEMBER | \
    ALIAS_REMOVE_MEMBER)

#define ALIAS_EXECUTE (STANDARD_RIGHTS_EXECUTE | \
    ALIAS_READ_INFORMATION)

//
// SamQueryInformationAlias/SamSetInformationAlias types
//

/**
 * The ALIAS_INFORMATION_CLASS enumeration specifies information classes
 * returned or set by SamQueryInformationAlias / SamSetInformationAlias.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
typedef enum _ALIAS_INFORMATION_CLASS
{
    AliasGeneralInformation = 1,        // q: ALIAS_GENERAL_INFORMATION
    AliasNameInformation,               // qs: ALIAS_NAME_INFORMATION
    AliasAdminCommentInformation,       // qs: ALIAS_ADM_COMMENT_INFORMATION
    AliasReplicationInformation,        // q: ALIAS_REPLICATION_INFORMATION
    AliasExtendedInformation,           // q: ALIAS_EXTENDED_INFORMATION
    AliasMaxInformation
} ALIAS_INFORMATION_CLASS;

/**
 * ALIAS_GENERAL_INFORMATION contains common properties for an alias.
 */
typedef struct _ALIAS_GENERAL_INFORMATION
{
    UNICODE_STRING Name;
    ULONG MemberCount;
    UNICODE_STRING AdminComment;
} ALIAS_GENERAL_INFORMATION, *PALIAS_GENERAL_INFORMATION;

/**
 * ALIAS_NAME_INFORMATION contains the alias name.
 */
typedef struct _ALIAS_NAME_INFORMATION
{
    UNICODE_STRING Name;
} ALIAS_NAME_INFORMATION, *PALIAS_NAME_INFORMATION;

/**
 * ALIAS_ADM_COMMENT_INFORMATION contains the administrative comment for an alias.
 */
typedef struct _ALIAS_ADM_COMMENT_INFORMATION
{
    UNICODE_STRING AdminComment;
} ALIAS_ADM_COMMENT_INFORMATION, *PALIAS_ADM_COMMENT_INFORMATION;

/**
 * ALIAS_REPLICATION_INFORMATION contains replication metadata for an alias.
 */
typedef struct _ALIAS_REPLICATION_INFORMATION
{
    LARGE_INTEGER LastWriteTime;
} ALIAS_REPLICATION_INFORMATION, *PALIAS_REPLICATION_INFORMATION;

#define ALIAS_ALL_NAME (0x00000001L)
#define ALIAS_ALL_MEMBER_COUNT (0x00000002L)
#define ALIAS_ALL_ADMIN_COMMENT (0x00000004L)
#define ALIAS_ALL_SHELL_ADMIN_OBJECT_PROPERTIES (0x00000008L)

/**
 * ALIAS_EXTENDED_INFORMATION contains optional extended fields for an alias.
 *
 * WhichFields indicates which of the extended fields are present.
 */
typedef struct _ALIAS_EXTENDED_INFORMATION
{
    ULONG WhichFields;
    SAM_SHELL_OBJECT_PROPERTIES ShellAdminObjectProperties;
} ALIAS_EXTENDED_INFORMATION, *PALIAS_EXTENDED_INFORMATION;

//
// Functions
//

/**
 * The SamEnumerateAliasesInDomain method enumerates aliases in a domain.
 *
 * \param DomainHandle A handle representing the domain.
 * \param EnumerationContext Opaque continuation value for enumeration.
 * \param Buffer Receives an array of SAM_RID_ENUMERATION entries.
 * \param PreferedMaximumLength Preferred maximum bytes to return.
 * \param CountReturned Receives the number of entries returned.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamEnumerateAliasesInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _Inout_ PSAM_ENUMERATE_HANDLE EnumerationContext,
    _Outptr_ PVOID *Buffer, // PSAM_RID_ENUMERATION *Buffer
    _In_ ULONG PreferedMaximumLength,
    _Out_ PULONG CountReturned
    );

/**
 * The SamCreateAliasInDomain method creates an alias in the specified domain.
 *
 * \param DomainHandle A handle representing the domain.
 * \param AccountName The alias name to create.
 * \param DesiredAccess Requested access for the output alias handle.
 * \param AliasHandle Receives the created alias handle.
 * \param RelativeId Receives the RID of the new alias.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamCreateAliasInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PCUNICODE_STRING AccountName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PSAM_HANDLE AliasHandle,
    _Out_ PULONG RelativeId
    );

/**
 * The SamOpenAlias method opens an existing alias by RID.
 *
 * \param DomainHandle A handle representing the domain.
 * \param DesiredAccess Requested access for the returned alias handle.
 * \param AliasId The RID of the alias to open.
 * \param AliasHandle Receives the opened alias handle.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamOpenAlias(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG AliasId,
    _Out_ PSAM_HANDLE AliasHandle
    );

/**
 * The SamDeleteAlias method deletes an alias identified by AliasHandle.
 *
 * \param AliasHandle A handle to the alias to delete.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamDeleteAlias(
    _In_ SAM_HANDLE AliasHandle
    );

/**
 * The SamQueryInformationAlias method queries information about an alias.
 *
 * \param AliasHandle A handle to the alias.
 * \param AliasInformationClass The information class to query.
 * \param Buffer Receives a pointer to the returned information structure.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQueryInformationAlias(
    _In_ SAM_HANDLE AliasHandle,
    _In_ ALIAS_INFORMATION_CLASS AliasInformationClass,
    _Outptr_ PVOID *Buffer
    );

/**
 * The SamSetInformationAlias method sets information for an alias.
 *
 * \param AliasHandle A handle to the alias.
 * \param AliasInformationClass The information class to set.
 * \param Buffer Pointer to the information structure to apply.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamSetInformationAlias(
    _In_ SAM_HANDLE AliasHandle,
    _In_ ALIAS_INFORMATION_CLASS AliasInformationClass,
    _In_ PVOID Buffer
    );

/**
 * The SamAddMemberToAlias method adds a security principal to an alias.
 *
 * \param AliasHandle A handle to the alias.
 * \param MemberId The SID of the member to add.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamAddMemberToAlias(
    _In_ SAM_HANDLE AliasHandle,
    _In_ PSID MemberId
    );

/**
 * The SamAddMultipleMembersToAlias method adds multiple members to an alias.
 *
 * \param AliasHandle A handle to the alias.
 * \param MemberIds Array of member SIDs to add.
 * \param MemberCount Number of members in MemberIds.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamAddMultipleMembersToAlias(
    _In_ SAM_HANDLE AliasHandle,
    _In_reads_(MemberCount) PSID *MemberIds,
    _In_ ULONG MemberCount
    );

/**
 * The SamRemoveMemberFromAlias method removes a member from an alias.
 *
 * \param AliasHandle A handle to the alias.
 * \param MemberId The SID of the member to remove.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamRemoveMemberFromAlias(
    _In_ SAM_HANDLE AliasHandle,
    _In_ PSID MemberId
    );

/**
 * The SamRemoveMultipleMembersFromAlias method removes multiple members from an alias.
 *
 * \param AliasHandle A handle to the alias.
 * \param MemberIds Array of member SIDs to remove.
 * \param MemberCount Number of members in MemberIds.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamRemoveMultipleMembersFromAlias(
    _In_ SAM_HANDLE AliasHandle,
    _In_reads_(MemberCount) PSID *MemberIds,
    _In_ ULONG MemberCount
    );

/**
 * The SamGetMembersInAlias method retrieves the members of an alias.
 *
 * \param AliasHandle A handle to the alias.
 * \param MemberIds Receives an allocated array of member SIDs.
 * \param MemberCount Receives the number of members returned.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamGetMembersInAlias(
    _In_ SAM_HANDLE AliasHandle,
    _Out_ _Deref_post_count_(*MemberCount) PSID **MemberIds,
    _Out_ PULONG MemberCount
    );

/**
 * The SamGetAliasMembership method determines alias membership for a set of SIDs.
 *
 * \param DomainHandle A handle representing the domain.
 * \param PassedCount Number of SIDs in the Sids array.
 * \param Sids Array of SIDs to check for membership.
 * \param MembershipCount Receives the number of alias entries returned.
 * \param Aliases Receives an array of alias RIDs for which membership applies.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
NTSYSAPI
NTSTATUS
NTAPI
SamGetAliasMembership(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ULONG PassedCount,
    _In_reads_(PassedCount) PSID *Sids,
    _Out_ PULONG MembershipCount,
    _Out_ _Deref_post_count_(*MembershipCount) PULONG *Aliases
    );

//
// Group types
//

#define GROUP_TYPE_BUILTIN_LOCAL_GROUP 0x00000001
#define GROUP_TYPE_ACCOUNT_GROUP 0x00000002
#define GROUP_TYPE_RESOURCE_GROUP 0x00000004
#define GROUP_TYPE_UNIVERSAL_GROUP 0x00000008
#define GROUP_TYPE_APP_BASIC_GROUP 0x00000010
#define GROUP_TYPE_APP_QUERY_GROUP 0x00000020
#define GROUP_TYPE_SECURITY_ENABLED 0x80000000

#define GROUP_TYPE_RESOURCE_BEHAVOIR (GROUP_TYPE_RESOURCE_GROUP | \
    GROUP_TYPE_APP_BASIC_GROUP | \
    GROUP_TYPE_APP_QUERY_GROUP)

//
// User
//

#define USER_READ_GENERAL 0x0001
#define USER_READ_PREFERENCES 0x0002
#define USER_WRITE_PREFERENCES 0x0004
#define USER_READ_LOGON 0x0008
#define USER_READ_ACCOUNT 0x0010
#define USER_WRITE_ACCOUNT 0x0020
#define USER_CHANGE_PASSWORD 0x0040
#define USER_FORCE_PASSWORD_CHANGE 0x0080
#define USER_LIST_GROUPS 0x0100
#define USER_READ_GROUP_INFORMATION 0x0200
#define USER_WRITE_GROUP_INFORMATION 0x0400

#define USER_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | \
    USER_READ_PREFERENCES | \
    USER_READ_LOGON | \
    USER_LIST_GROUPS | \
    USER_READ_GROUP_INFORMATION | \
    USER_WRITE_PREFERENCES | \
    USER_CHANGE_PASSWORD | \
    USER_FORCE_PASSWORD_CHANGE | \
    USER_READ_GENERAL | \
    USER_READ_ACCOUNT | \
    USER_WRITE_ACCOUNT | \
    USER_WRITE_GROUP_INFORMATION)

#define USER_READ (STANDARD_RIGHTS_READ | \
    USER_READ_PREFERENCES | \
    USER_READ_LOGON | \
    USER_READ_ACCOUNT | \
    USER_LIST_GROUPS | \
    USER_READ_GROUP_INFORMATION)

#define USER_WRITE (STANDARD_RIGHTS_WRITE | \
    USER_WRITE_PREFERENCES | \
    USER_CHANGE_PASSWORD)

#define USER_EXECUTE (STANDARD_RIGHTS_EXECUTE | \
    USER_READ_GENERAL | \
    USER_CHANGE_PASSWORD)

//
// User account control flags
//

#define USER_ACCOUNT_DISABLED (0x00000001)
#define USER_HOME_DIRECTORY_REQUIRED (0x00000002)
#define USER_PASSWORD_NOT_REQUIRED (0x00000004)
#define USER_TEMP_DUPLICATE_ACCOUNT (0x00000008)
#define USER_NORMAL_ACCOUNT (0x00000010)
#define USER_MNS_LOGON_ACCOUNT (0x00000020)
#define USER_INTERDOMAIN_TRUST_ACCOUNT (0x00000040)
#define USER_WORKSTATION_TRUST_ACCOUNT (0x00000080)
#define USER_SERVER_TRUST_ACCOUNT (0x00000100)
#define USER_DONT_EXPIRE_PASSWORD (0x00000200)
#define USER_ACCOUNT_AUTO_LOCKED (0x00000400)
#define USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED (0x00000800)
#define USER_SMARTCARD_REQUIRED (0x00001000)
#define USER_TRUSTED_FOR_DELEGATION (0x00002000)
#define USER_NOT_DELEGATED (0x00004000)
#define USER_USE_DES_KEY_ONLY (0x00008000)
#define USER_DONT_REQUIRE_PREAUTH (0x00010000)
#define USER_PASSWORD_EXPIRED (0x00020000)
#define USER_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION (0x00040000)
#define USER_NO_AUTH_DATA_REQUIRED (0x00080000)
#define USER_PARTIAL_SECRETS_ACCOUNT (0x00100000)
#define USER_USE_AES_KEYS (0x00200000)

#define NEXT_FREE_ACCOUNT_CONTROL_BIT (USER_USE_AES_KEYS << 1)

#define USER_MACHINE_ACCOUNT_MASK ( \
    USER_INTERDOMAIN_TRUST_ACCOUNT | \
    USER_WORKSTATION_TRUST_ACCOUNT | \
    USER_SERVER_TRUST_ACCOUNT \
    )

#define USER_ACCOUNT_TYPE_MASK ( \
    USER_TEMP_DUPLICATE_ACCOUNT | \
    USER_NORMAL_ACCOUNT | \
    USER_MACHINE_ACCOUNT_MASK \
    )

#define USER_COMPUTED_ACCOUNT_CONTROL_BITS ( \
    USER_ACCOUNT_AUTO_LOCKED | \
    USER_PASSWORD_EXPIRED \
    )

// Logon times may be expressed in day, hour, or minute granularity.

#define SAM_DAYS_PER_WEEK (7)
#define SAM_HOURS_PER_WEEK (24 * SAM_DAYS_PER_WEEK)
#define SAM_MINUTES_PER_WEEK (60 * SAM_HOURS_PER_WEEK)

/**
 * LOGON_HOURS specifies weekly logon time windows using a bitmask.
 *
 * UnitsPerWeek is the number of equal-length time units the week is divided into.
 * LogonHours is a bit map where each bit represents a time unit in the week. A
 * NULL LogonHours pointer indicates DONT_CHANGE when used with SamSetInformationUser().
 */
typedef struct _LOGON_HOURS
{
    USHORT UnitsPerWeek;

    // UnitsPerWeek is the number of equal length time units the week is
    // divided into. This value is used to compute the length of the bit
    // string in logon_hours. Must be less than or equal to
    // SAM_UNITS_PER_WEEK (10080) for this release.
    //
    // LogonHours is a bit map of valid logon times. Each bit represents
    // a unique division in a week. The largest bit map supported is 1260
    // bytes (10080 bits), which represents minutes per week. In this case
    // the first bit (bit 0, byte 0) is Sunday, 00:00:00 - 00-00:59; bit 1,
    // byte 0 is Sunday, 00:01:00 - 00:01:59, etc. A NULL pointer means
    // DONT_CHANGE for SamSetInformationUser() calls.

    PUCHAR LogonHours;
} LOGON_HOURS, *PLOGON_HOURS;

/**
 * The SR_SECURITY_DESCRIPTOR structure contains information about the security privileges of the user.
 *
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/subauth/ns-subauth-sr_security_descriptor
 */
typedef struct _SR_SECURITY_DESCRIPTOR
{
    ULONG Length;                   // Indicates the size in bytes of the structure.
    PUCHAR SecurityDescriptor;      // Indicates the user's security privileges.
} SR_SECURITY_DESCRIPTOR, *PSR_SECURITY_DESCRIPTOR;

//
// SamQueryInformationUser/SamSetInformationUser types
//

/**
 * The USER_INFORMATION_CLASS enumeration specifies information classes
 * for SamQueryInformationUser / SamSetInformationUser.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/
 */
typedef enum _USER_INFORMATION_CLASS
{
    UserGeneralInformation = 1,     // q: USER_GENERAL_INFORMATION
    UserPreferencesInformation,     // qs: USER_PREFERENCES_INFORMATION
    UserLogonInformation,           // q: USER_LOGON_INFORMATION
    UserLogonHoursInformation,      // qs: USER_LOGON_HOURS_INFORMATION
    UserAccountInformation,         // q: USER_ACCOUNT_INFORMATION
    UserNameInformation,            // qs: USER_NAME_INFORMATION
    UserAccountNameInformation,     // qs: USER_ACCOUNT_NAME_INFORMATION
    UserFullNameInformation,        // qs: USER_FULL_NAME_INFORMATION
    UserPrimaryGroupInformation,    // qs: USER_PRIMARY_GROUP_INFORMATION
    UserHomeInformation,            // qs: USER_HOME_INFORMATION // 10
    UserScriptInformation,          // qs: USER_SCRIPT_INFORMATION
    UserProfileInformation,         // qs: USER_PROFILE_INFORMATION
    UserAdminCommentInformation,    // qs: USER_ADMIN_COMMENT_INFORMATION
    UserWorkStationsInformation,    // qs: USER_WORKSTATIONS_INFORMATION
    UserSetPasswordInformation,     // s: USER_SET_PASSWORD_INFORMATION
    UserControlInformation,         // qs: USER_CONTROL_INFORMATION
    UserExpiresInformation,         // qs: USER_EXPIRES_INFORMATION
    UserInternal1Information,       // qs: USER_INTERNAL1_INFORMATION
    UserInternal2Information,       // qs: USER_INTERNAL2_INFORMATION
    UserParametersInformation,      // qs: USER_PARAMETERS_INFORMATION // 20
    UserAllInformation,             // qs: USER_ALL_INFORMATION
    UserInternal3Information,       // qs: USER_INTERNAL3_INFORMATION
    UserInternal4Information,       // qs: USER_INTERNAL4_INFORMATION
    UserInternal5Information,       // qs: USER_INTERNAL5_INFORMATION
    UserInternal4InformationNew,    // qs: USER_INTERNAL4_INFORMATION_NEW
    UserInternal5InformationNew,    // qs: USER_INTERNAL5_INFORMATION_NEW
    UserInternal6Information,       // qs: USER_INTERNAL6_INFORMATION
    UserExtendedInformation,        // qs: USER_EXTENDED_INFORMATION
    UserLogonUIInformation,         // q: USER_LOGON_UI_INFORMATION // since VISTA
    UserAuthInformation,            // qs: USER_AUTH_INFORMATION // since WIN10 // 30
    UserInternal7Information,       // qs: USER_INTERNAL7_INFORMATION // since 20H1
    UserInternal8Information,       // qs: USER_INTERNAL8_INFORMATION
    UserMaxInformation
} USER_INFORMATION_CLASS, *PUSER_INFORMATION_CLASS;

/**
 * USER_GENERAL_INFORMATION contains basic user account properties.
 *
 * Fields:
 * - UserName: The account name.
 * - FullName: The full display name.
 * - PrimaryGroupId: RID of the primary group.
 * - AdminComment: Administrative comment string.
 * - UserComment: User-provided comment string.
 */
typedef struct _USER_GENERAL_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG PrimaryGroupId;
    UNICODE_STRING AdminComment;
    UNICODE_STRING UserComment;
} USER_GENERAL_INFORMATION, *PUSER_GENERAL_INFORMATION;

/**
 * USER_PREFERENCES_INFORMATION contains locale/preferences for a user.
 */
typedef struct _USER_PREFERENCES_INFORMATION
{
    UNICODE_STRING UserComment;
    UNICODE_STRING Reserved1;
    USHORT CountryCode;
    USHORT CodePage;
} USER_PREFERENCES_INFORMATION, *PUSER_PREFERENCES_INFORMATION;

/**
 * USER_LOGON_INFORMATION contains logon-related data for a user account.
 */
typedef struct _USER_LOGON_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG UserId;
    ULONG PrimaryGroupId;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING WorkStations;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    ULONG UserAccountControl;
} USER_LOGON_INFORMATION, *PUSER_LOGON_INFORMATION;

/**
 * USER_LOGON_HOURS_INFORMATION wraps LOGON_HOURS for user queries/sets.
 */
typedef struct _USER_LOGON_HOURS_INFORMATION
{
    LOGON_HOURS LogonHours;
} USER_LOGON_HOURS_INFORMATION, *PUSER_LOGON_HOURS_INFORMATION;

/**
 * USER_ACCOUNT_INFORMATION is a comprehensive user account info structure
 * used by SamQueryInformationUser / SamSetInformationUser.
 */
typedef struct _USER_ACCOUNT_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    ULONG UserId;
    ULONG PrimaryGroupId;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING AdminComment;
    UNICODE_STRING WorkStations;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    ULONG UserAccountControl;
} USER_ACCOUNT_INFORMATION, *PUSER_ACCOUNT_INFORMATION;

/**
 * USER_NAME_INFORMATION contains user and full names.
 */
typedef struct _USER_NAME_INFORMATION
{
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
} USER_NAME_INFORMATION, *PUSER_NAME_INFORMATION;

/**
 * USER_ACCOUNT_NAME_INFORMATION contains the account name only.
 */
typedef struct _USER_ACCOUNT_NAME_INFORMATION
{
    UNICODE_STRING UserName;
} USER_ACCOUNT_NAME_INFORMATION, *PUSER_ACCOUNT_NAME_INFORMATION;

/**
 * USER_FULL_NAME_INFORMATION contains the full display name for the user.
 */
typedef struct _USER_FULL_NAME_INFORMATION
{
    UNICODE_STRING FullName;
} USER_FULL_NAME_INFORMATION, *PUSER_FULL_NAME_INFORMATION;

/**
 * USER_PRIMARY_GROUP_INFORMATION specifies the user's primary group RID.
 */
typedef struct _USER_PRIMARY_GROUP_INFORMATION
{
    ULONG PrimaryGroupId;
} USER_PRIMARY_GROUP_INFORMATION, *PUSER_PRIMARY_GROUP_INFORMATION;

/**
 * USER_HOME_INFORMATION contains the user's home directory and drive.
 */
typedef struct _USER_HOME_INFORMATION
{
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
} USER_HOME_INFORMATION, *PUSER_HOME_INFORMATION;

/**
 * USER_SCRIPT_INFORMATION contains the login script path.
 */
typedef struct _USER_SCRIPT_INFORMATION
{
    UNICODE_STRING ScriptPath;
} USER_SCRIPT_INFORMATION, *PUSER_SCRIPT_INFORMATION;

/**
 * USER_PROFILE_INFORMATION contains the profile path for the user.
 */
typedef struct _USER_PROFILE_INFORMATION
{
    UNICODE_STRING ProfilePath;
} USER_PROFILE_INFORMATION, *PUSER_PROFILE_INFORMATION;

/**
 * USER_ADMIN_COMMENT_INFORMATION contains administrative comment text.
 */
typedef struct _USER_ADMIN_COMMENT_INFORMATION
{
    UNICODE_STRING AdminComment;
} USER_ADMIN_COMMENT_INFORMATION, *PUSER_ADMIN_COMMENT_INFORMATION;

/**
 * USER_WORKSTATIONS_INFORMATION contains a list of allowed workstations.
 */
typedef struct _USER_WORKSTATIONS_INFORMATION
{
    UNICODE_STRING WorkStations;
} USER_WORKSTATIONS_INFORMATION, *PUSER_WORKSTATIONS_INFORMATION;

/**
 * USER_SET_PASSWORD_INFORMATION is used to set a user's password.
 */
typedef struct _USER_SET_PASSWORD_INFORMATION
{
    UNICODE_STRING Password;
    BOOLEAN PasswordExpired;
} USER_SET_PASSWORD_INFORMATION, *PUSER_SET_PASSWORD_INFORMATION;

/**
 * USER_CONTROL_INFORMATION contains user account control flags.
 */
typedef struct _USER_CONTROL_INFORMATION
{
    ULONG UserAccountControl;
} USER_CONTROL_INFORMATION, *PUSER_CONTROL_INFORMATION;

/**
 * USER_EXPIRES_INFORMATION contains the account expiration time.
 */
typedef struct _USER_EXPIRES_INFORMATION
{
    LARGE_INTEGER AccountExpires;
} USER_EXPIRES_INFORMATION, *PUSER_EXPIRES_INFORMATION;

#define CYPHER_BLOCK_LENGTH 8

/**
 * CYPHER_BLOCK is a fixed-size block used for password encryption.
 */
typedef struct _CYPHER_BLOCK
{
    CHAR data[CYPHER_BLOCK_LENGTH];
} CYPHER_BLOCK, *PCYPHER_BLOCK;

/**
 * ENCRYPTED_NT_OWF_PASSWORD holds the encrypted NT OWF password blocks.
 */
typedef struct _ENCRYPTED_NT_OWF_PASSWORD
{
    CYPHER_BLOCK data[2];
} ENCRYPTED_NT_OWF_PASSWORD, *PENCRYPTED_NT_OWF_PASSWORD;

/**
 * ENCRYPTED_LM_OWF_PASSWORD holds the encrypted LM OWF password blocks.
 */
typedef struct _ENCRYPTED_LM_OWF_PASSWORD
{
    CYPHER_BLOCK data[2];
} ENCRYPTED_LM_OWF_PASSWORD, *PENCRYPTED_LM_OWF_PASSWORD;

typedef struct _USER_INTERNAL1_INFORMATION
{
    ENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword;
    ENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword;
    BOOLEAN NtPasswordPresent;
    BOOLEAN LmPasswordPresent;
    BOOLEAN PasswordExpired;
} USER_INTERNAL1_INFORMATION, *PUSER_INTERNAL1_INFORMATION;

typedef struct _USER_INTERNAL2_INFORMATION
{
    ULONG StatisticsToApply;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    USHORT BadPasswordCount;
    USHORT LogonCount;
} USER_INTERNAL2_INFORMATION, *PUSER_INTERNAL2_INFORMATION;

typedef struct _USER_PARAMETERS_INFORMATION
{
    UNICODE_STRING Parameters;
} USER_PARAMETERS_INFORMATION, *PUSER_PARAMETERS_INFORMATION;

//
// Flags for WhichFields in USER_ALL_INFORMATION
//

#define USER_ALL_USERNAME 0x00000001
#define USER_ALL_FULLNAME 0x00000002
#define USER_ALL_USERID 0x00000004
#define USER_ALL_PRIMARYGROUPID 0x00000008
#define USER_ALL_ADMINCOMMENT 0x00000010
#define USER_ALL_USERCOMMENT 0x00000020
#define USER_ALL_HOMEDIRECTORY 0x00000040
#define USER_ALL_HOMEDIRECTORYDRIVE 0x00000080
#define USER_ALL_SCRIPTPATH 0x00000100
#define USER_ALL_PROFILEPATH 0x00000200
#define USER_ALL_WORKSTATIONS 0x00000400
#define USER_ALL_LASTLOGON 0x00000800
#define USER_ALL_LASTLOGOFF 0x00001000
#define USER_ALL_LOGONHOURS 0x00002000
#define USER_ALL_BADPASSWORDCOUNT 0x00004000
#define USER_ALL_LOGONCOUNT 0x00008000
#define USER_ALL_PASSWORDCANCHANGE 0x00010000
#define USER_ALL_PASSWORDMUSTCHANGE 0x00020000
#define USER_ALL_PASSWORDLASTSET 0x00040000
#define USER_ALL_ACCOUNTEXPIRES 0x00080000
#define USER_ALL_USERACCOUNTCONTROL 0x00100000
#define USER_ALL_PARAMETERS 0x00200000
#define USER_ALL_COUNTRYCODE 0x00400000
#define USER_ALL_CODEPAGE 0x00800000
#define USER_ALL_NTPASSWORDPRESENT 0x01000000 // field AND boolean
#define USER_ALL_LMPASSWORDPRESENT 0x02000000 // field AND boolean
#define USER_ALL_PRIVATEDATA 0x04000000 // field AND boolean
#define USER_ALL_PASSWORDEXPIRED 0x08000000
#define USER_ALL_SECURITYDESCRIPTOR 0x10000000
#define USER_ALL_OWFPASSWORD 0x20000000 // boolean

#define USER_ALL_UNDEFINED_MASK 0xc0000000

//
// Fields that require USER_READ_GENERAL access to read.
//
#define USER_ALL_READ_GENERAL_MASK \
    (USER_ALL_USERNAME | \
    USER_ALL_FULLNAME | \
    USER_ALL_USERID | \
    USER_ALL_PRIMARYGROUPID | \
    USER_ALL_ADMINCOMMENT | \
    USER_ALL_USERCOMMENT)

//
// Fields that require USER_READ_LOGON access to read.
//
#define USER_ALL_READ_LOGON_MASK \
   (USER_ALL_HOMEDIRECTORY | \
    USER_ALL_HOMEDIRECTORYDRIVE | \
    USER_ALL_SCRIPTPATH | \
    USER_ALL_PROFILEPATH | \
    USER_ALL_WORKSTATIONS | \
    USER_ALL_LASTLOGON | \
    USER_ALL_LASTLOGOFF | \
    USER_ALL_LOGONHOURS | \
    USER_ALL_BADPASSWORDCOUNT | \
    USER_ALL_LOGONCOUNT | \
    USER_ALL_PASSWORDCANCHANGE | \
    USER_ALL_PASSWORDMUSTCHANGE)

//
// Fields that require USER_READ_ACCOUNT access to read.
//
#define USER_ALL_READ_ACCOUNT_MASK \
    (USER_ALL_PASSWORDLASTSET | \
    USER_ALL_ACCOUNTEXPIRES | \
    USER_ALL_USERACCOUNTCONTROL | \
    USER_ALL_PARAMETERS)

//
// Fields that require USER_READ_PREFERENCES access to read.
//
#define USER_ALL_READ_PREFERENCES_MASK \
    (USER_ALL_COUNTRYCODE | USER_ALL_CODEPAGE)

//
// Fields that can only be read by trusted clients.
//
#define USER_ALL_READ_TRUSTED_MASK \
    (USER_ALL_NTPASSWORDPRESENT | \
    USER_ALL_LMPASSWORDPRESENT | \
    USER_ALL_PASSWORDEXPIRED | \
    USER_ALL_SECURITYDESCRIPTOR | \
    USER_ALL_PRIVATEDATA)

//
// Fields that can't be read.
//
#define USER_ALL_READ_CANT_MASK USER_ALL_UNDEFINED_MASK

//
// Fields that require USER_WRITE_ACCOUNT access to write.
//
#define USER_ALL_WRITE_ACCOUNT_MASK \
    (USER_ALL_USERNAME | \
    USER_ALL_FULLNAME | \
    USER_ALL_PRIMARYGROUPID | \
    USER_ALL_HOMEDIRECTORY | \
    USER_ALL_HOMEDIRECTORYDRIVE | \
    USER_ALL_SCRIPTPATH | \
    USER_ALL_PROFILEPATH | \
    USER_ALL_ADMINCOMMENT | \
    USER_ALL_WORKSTATIONS | \
    USER_ALL_LOGONHOURS | \
    USER_ALL_ACCOUNTEXPIRES | \
    USER_ALL_USERACCOUNTCONTROL | \
    USER_ALL_PARAMETERS)

//
// Fields that require USER_WRITE_PREFERENCES access to write.
//
#define USER_ALL_WRITE_PREFERENCES_MASK \
    (USER_ALL_USERCOMMENT | USER_ALL_COUNTRYCODE | USER_ALL_CODEPAGE)

// Fields that require USER_FORCE_PASSWORD_CHANGE access to write.
//
// Note that non-trusted clients only set the NT password as a
// UNICODE string. The wrapper will convert it to an LM password,
// OWF and encrypt both versions. Trusted clients can pass in OWF
// versions of either or both.

#define USER_ALL_WRITE_FORCE_PASSWORD_CHANGE_MASK \
    (USER_ALL_NTPASSWORDPRESENT | \
    USER_ALL_LMPASSWORDPRESENT | \
    USER_ALL_PASSWORDEXPIRED)

//
// Fields that can only be written by trusted clients.
//
#define USER_ALL_WRITE_TRUSTED_MASK \
    (USER_ALL_LASTLOGON | \
    USER_ALL_LASTLOGOFF | \
    USER_ALL_BADPASSWORDCOUNT | \
    USER_ALL_LOGONCOUNT | \
    USER_ALL_PASSWORDLASTSET | \
    USER_ALL_SECURITYDESCRIPTOR | \
    USER_ALL_PRIVATEDATA)

//
// Fields that can't be written.
//
#define USER_ALL_WRITE_CANT_MASK \
    (USER_ALL_USERID | \
    USER_ALL_PASSWORDCANCHANGE | \
    USER_ALL_PASSWORDMUSTCHANGE | \
    USER_ALL_UNDEFINED_MASK)

typedef struct _USER_ALL_INFORMATION
{
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordCanChange;
    LARGE_INTEGER PasswordMustChange;
    UNICODE_STRING UserName;
    UNICODE_STRING FullName;
    UNICODE_STRING HomeDirectory;
    UNICODE_STRING HomeDirectoryDrive;
    UNICODE_STRING ScriptPath;
    UNICODE_STRING ProfilePath;
    UNICODE_STRING AdminComment;
    UNICODE_STRING WorkStations;
    UNICODE_STRING UserComment;
    UNICODE_STRING Parameters;
    UNICODE_STRING LmPassword;
    UNICODE_STRING NtPassword;
    UNICODE_STRING PrivateData;
    SR_SECURITY_DESCRIPTOR SecurityDescriptor;
    ULONG UserId;
    ULONG PrimaryGroupId;
    ULONG UserAccountControl;
    ULONG WhichFields;
    LOGON_HOURS LogonHours;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    USHORT CountryCode;
    USHORT CodePage;
    BOOLEAN LmPasswordPresent;
    BOOLEAN NtPasswordPresent;
    BOOLEAN PasswordExpired;
    BOOLEAN PrivateDataSensitive;
} USER_ALL_INFORMATION, *PUSER_ALL_INFORMATION;

typedef struct _USER_INTERNAL3_INFORMATION
{
    USER_ALL_INFORMATION I1;
    LARGE_INTEGER LastBadPasswordTime;
} USER_INTERNAL3_INFORMATION, *PUSER_INTERNAL3_INFORMATION;

typedef struct _ENCRYPTED_USER_PASSWORD
{
    UCHAR Buffer[(SAM_MAX_PASSWORD_LENGTH * 2) + 4];
} ENCRYPTED_USER_PASSWORD, *PENCRYPTED_USER_PASSWORD;

typedef struct _USER_INTERNAL4_INFORMATION
{
    USER_ALL_INFORMATION I1;
    ENCRYPTED_USER_PASSWORD UserPassword;
} USER_INTERNAL4_INFORMATION, *PUSER_INTERNAL4_INFORMATION;

typedef struct _USER_INTERNAL5_INFORMATION
{
    ENCRYPTED_USER_PASSWORD UserPassword;
    BOOLEAN PasswordExpired;
} USER_INTERNAL5_INFORMATION, *PUSER_INTERNAL5_INFORMATION;

typedef struct _ENCRYPTED_USER_PASSWORD_NEW
{
    UCHAR Buffer[(SAM_MAX_PASSWORD_LENGTH * 2) + 4 + SAM_PASSWORD_ENCRYPTION_SALT_LEN];
} ENCRYPTED_USER_PASSWORD_NEW, *PENCRYPTED_USER_PASSWORD_NEW;

typedef struct _USER_INTERNAL4_INFORMATION_NEW
{
    USER_ALL_INFORMATION I1;
    ENCRYPTED_USER_PASSWORD_NEW UserPassword;
} USER_INTERNAL4_INFORMATION_NEW, *PUSER_INTERNAL4_INFORMATION_NEW;

typedef struct _USER_INTERNAL5_INFORMATION_NEW
{
    ENCRYPTED_USER_PASSWORD_NEW UserPassword;
    BOOLEAN PasswordExpired;
} USER_INTERNAL5_INFORMATION_NEW, *PUSER_INTERNAL5_INFORMATION_NEW;

typedef struct _USER_ALLOWED_TO_DELEGATE_TO_LIST
{
    ULONG Size;
    ULONG NumSPNs;
    UNICODE_STRING SPNList[ANYSIZE_ARRAY];
} USER_ALLOWED_TO_DELEGATE_TO_LIST, *PUSER_ALLOWED_TO_DELEGATE_TO_LIST;

#define USER_EXTENDED_FIELD_UPN 0x00000001L
#define USER_EXTENDED_FIELD_A2D2 0x00000002L

typedef struct _USER_INTERNAL6_INFORMATION
{
    USER_ALL_INFORMATION I1;
    LARGE_INTEGER LastBadPasswordTime;
    ULONG ExtendedFields;
    BOOLEAN UPNDefaulted;
    UNICODE_STRING UPN;
    PUSER_ALLOWED_TO_DELEGATE_TO_LIST A2D2List;
} USER_INTERNAL6_INFORMATION, *PUSER_INTERNAL6_INFORMATION;

typedef SAM_BYTE_ARRAY_32K SAM_USER_TILE, *PSAM_USER_TILE;

// 0xff000fff is reserved for internal callers and implementation.

#define USER_EXTENDED_FIELD_USER_TILE (0x00001000L)
#define USER_EXTENDED_FIELD_PASSWORD_HINT (0x00002000L)
#define USER_EXTENDED_FIELD_DONT_SHOW_IN_LOGON_UI (0x00004000L)
#define USER_EXTENDED_FIELD_SHELL_ADMIN_OBJECT_PROPERTIES (0x00008000L)

typedef struct _USER_EXTENDED_INFORMATION
{
    ULONG ExtendedWhichFields;
    SAM_USER_TILE UserTile;
    UNICODE_STRING PasswordHint;
    BOOLEAN DontShowInLogonUI;
    SAM_SHELL_OBJECT_PROPERTIES ShellAdminObjectProperties;
} USER_EXTENDED_INFORMATION, *PUSER_EXTENDED_INFORMATION;

// For local callers only.
typedef struct _USER_LOGON_UI_INFORMATION
{
    BOOLEAN PasswordIsBlank;
    BOOLEAN AccountIsDisabled;
} USER_LOGON_UI_INFORMATION, *PUSER_LOGON_UI_INFORMATION;

typedef struct _USER_AUTH_INFORMATION
{
    SAM_BYTE_ARRAY_32K AuthData;
} USER_AUTH_INFORMATION, *PUSER_AUTH_INFORMATION;

typedef struct _ENCRYPTED_PASSWORD_AES
{
    UCHAR AuthData[64];
    UCHAR Salt[SAM_PASSWORD_ENCRYPTION_SALT_LEN];
    ULONG cbCipher;
    PUCHAR Cipher;
    ULONGLONG PBKDF2Iterations;
} ENCRYPTED_PASSWORD_AES, *PENCRYPTED_PASSWORD_AES;

typedef struct _USER_INTERNAL7_INFORMATION
{
    ENCRYPTED_PASSWORD_AES UserPassword;
    BOOLEAN PasswordExpired;
} USER_INTERNAL7_INFORMATION, *PUSER_INTERNAL7_INFORMATION;

typedef struct _USER_INTERNAL8_INFORMATION
{
    USER_ALL_INFORMATION I1;
    ENCRYPTED_PASSWORD_AES UserPassword;
} USER_INTERNAL8_INFORMATION, *PUSER_INTERNAL8_INFORMATION;

// SamChangePasswordUser3 types

typedef struct _USER_PWD_CHANGE_FAILURE_INFORMATION
{
    ULONG ExtendedFailureReason;
    UNICODE_STRING FilterModuleName;
} USER_PWD_CHANGE_FAILURE_INFORMATION, *PUSER_PWD_CHANGE_FAILURE_INFORMATION;

//
// ExtendedFailureReason values
//

#define SAM_PWD_CHANGE_NO_ERROR 0
#define SAM_PWD_CHANGE_PASSWORD_TOO_SHORT 1
#define SAM_PWD_CHANGE_PWD_IN_HISTORY 2
#define SAM_PWD_CHANGE_USERNAME_IN_PASSWORD 3
#define SAM_PWD_CHANGE_FULLNAME_IN_PASSWORD 4
#define SAM_PWD_CHANGE_NOT_COMPLEX 5
#define SAM_PWD_CHANGE_MACHINE_PASSWORD_NOT_DEFAULT 6
#define SAM_PWD_CHANGE_FAILED_BY_FILTER 7
#define SAM_PWD_CHANGE_PASSWORD_TOO_LONG 8
#define SAM_PWD_CHANGE_FAILURE_REASON_MAX 8

//
// Functions
//

/**
 * Enumerates user accounts in a domain.
 *
 * \param DomainHandle A handle to the domain.
 * \param EnumerationContext The enumeration continuation value. Set to zero for the first call and reuse the returned value for subsequent calls.
 * \param UserAccountControl The account control flags used to filter users, or zero to enumerate all users.
 * \param Buffer Receives an array of SAM_RID_ENUMERATION entries.
 * \param PreferedMaximumLength The preferred maximum number of bytes to return in Buffer.
 * \param CountReturned Receives the number of entries returned in Buffer.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamEnumerateUsersInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _Inout_ PSAM_ENUMERATE_HANDLE EnumerationContext,
    _In_ ULONG UserAccountControl,
    _Outptr_ PVOID *Buffer, // PSAM_RID_ENUMERATION *
    _In_ ULONG PreferedMaximumLength,
    _Out_ PULONG CountReturned
    );

// rev
/**
 * Enumerates user accounts in a domain with additional enumeration flags.
 *
 * \param DomainHandle A handle to the domain.
 * \param EnumerationContext The enumeration continuation value. Set to zero for the first call and reuse the returned value for subsequent calls.
 * \param UserAccountControl The account control flags used to filter users.
 * \param Flags Flags controlling the enumeration.
 * \param Buffer Receives an array of SAM_RID_ENUMERATION entries.
 * \param PreferedMaximumLength The preferred maximum number of bytes to return in Buffer.
 * \param CountReturned Receives the number of entries returned in Buffer.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamEnumerateUsersInDomain2(
    _In_ SAM_HANDLE DomainHandle,
    _Inout_ PSAM_ENUMERATE_HANDLE EnumerationContext,
    _In_ ULONG UserAccountControl,
    _In_ ULONG Flags,
    _Outptr_ PVOID *Buffer, // PSAM_RID_ENUMERATION *
    _In_ ULONG PreferedMaximumLength,
    _Out_ PULONG CountReturned
    );

/**
 * Creates a user account in a domain.
 *
 * \param DomainHandle A handle to the domain.
 * \param AccountName The name of the user account to create.
 * \param DesiredAccess The access requested for the returned user handle.
 * \param UserHandle Receives a handle to the created user.
 * \param RelativeId Receives the RID of the created user.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamCreateUserInDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PCUNICODE_STRING AccountName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PSAM_HANDLE UserHandle,
    _Out_ PULONG RelativeId
    );

/**
 * Creates a user account of the specified type and returns the granted access.
 *
 * \param DomainHandle A handle to the domain.
 * \param AccountName The name of the user account to create.
 * \param AccountType The user account type to create.
 * \param DesiredAccess The access requested for the returned user handle.
 * \param UserHandle Receives a handle to the created user.
 * \param GrantedAccess Receives the access granted to UserHandle.
 * \param RelativeId Receives the RID of the created user.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamCreateUser2InDomain(
    _In_ SAM_HANDLE DomainHandle,
    _In_ PCUNICODE_STRING AccountName,
    _In_ ULONG AccountType,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PSAM_HANDLE UserHandle,
    _Out_ PULONG GrantedAccess,
    _Out_ PULONG RelativeId
    );

/**
 * Opens an existing user account by RID.
 *
 * \param DomainHandle A handle to the domain containing the user.
 * \param DesiredAccess The access requested for the returned user handle.
 * \param UserId The RID of the user to open.
 * \param UserHandle Receives a handle to the opened user.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamOpenUser(
    _In_ SAM_HANDLE DomainHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG UserId,
    _Out_ PSAM_HANDLE UserHandle
    );

/**
 * Deletes a user account.
 *
 * \param UserHandle A handle to the user to delete.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamDeleteUser(
    _In_ SAM_HANDLE UserHandle
    );

/**
 * Queries information about a user account.
 *
 * \param UserHandle A handle to the user.
 * \param UserInformationClass The user information class to retrieve.
 * \param Buffer Receives a pointer to the requested information structure.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQueryInformationUser(
    _In_ SAM_HANDLE UserHandle,
    _In_ USER_INFORMATION_CLASS UserInformationClass,
    _Outptr_ PVOID *Buffer
    );

/**
 * Sets information for a user account.
 *
 * \param UserHandle A handle to the user.
 * \param UserInformationClass The user information class to set.
 * \param Buffer The information structure to apply.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamSetInformationUser(
    _In_ SAM_HANDLE UserHandle,
    _In_ USER_INFORMATION_CLASS UserInformationClass,
    _In_ PVOID Buffer
    );

/**
 * Retrieves the group memberships of a user account.
 *
 * \param UserHandle A handle to the user.
 * \param Groups Receives an array of GROUP_MEMBERSHIP entries containing group RIDs and membership attributes.
 * \param MembershipCount Receives the number of entries in Groups.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamGetGroupsForUser(
    _In_ SAM_HANDLE UserHandle,
    _Out_ _Deref_post_count_(*MembershipCount) PGROUP_MEMBERSHIP *Groups,
    _Out_ PULONG MembershipCount
    );

/**
 * Changes a user account password using a user handle.
 *
 * \param UserHandle A handle to the user.
 * \param OldPassword The current password.
 * \param NewPassword The new password.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamChangePasswordUser(
    _In_ SAM_HANDLE UserHandle,
    _In_ PCUNICODE_STRING OldPassword,
    _In_ PCUNICODE_STRING NewPassword
    );

/**
 * Changes a user account password using the server and account names.
 *
 * \param ServerName The name of the SAM server.
 * \param UserName The name of the user account.
 * \param OldPassword The current password.
 * \param NewPassword The new password.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamChangePasswordUser2(
    _In_ PCUNICODE_STRING ServerName,
    _In_ PCUNICODE_STRING UserName,
    _In_ PCUNICODE_STRING OldPassword,
    _In_ PCUNICODE_STRING NewPassword
    );

/**
 * Changes a user account password and provides password policy failure information.
 *
 * \param ServerName The name of the SAM server.
 * \param UserName The name of the user account.
 * \param OldPassword The current password.
 * \param NewPassword The new password.
 * \param EffectivePasswordPolicy Receives password policy information when supplied by the server.
 * \param PasswordChangeFailureInfo Receives additional information about a rejected password change when supplied by the server.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamChangePasswordUser3(
    _In_ PCUNICODE_STRING ServerName,
    _In_ PCUNICODE_STRING UserName,
    _In_ PCUNICODE_STRING OldPassword,
    _In_ PCUNICODE_STRING NewPassword,
    _Outptr_ PDOMAIN_PASSWORD_INFORMATION *EffectivePasswordPolicy,
    _Outptr_ PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo
    );

/**
 * Retrieves account display information in ascending account name order.
 *
 * \param DomainHandle A handle to the domain.
 * \param DisplayInformation The class of account display information to retrieve.
 * \param Index The index at which to begin retrieving entries.
 * \param EntryCount The requested maximum number of entries.
 * \param PreferredMaximumLength The preferred maximum size of the returned information, in bytes.
 * \param TotalAvailable Receives the number of bytes required for the complete listing.
 * \param TotalReturned Receives the number of bytes returned.
 * \param ReturnedEntryCount Receives the number of entries in SortedBuffer.
 * \param SortedBuffer Receives the sorted array of entries selected by DisplayInformation.
 * \return NTSTATUS indicating success or failure.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/c1458942-f2d5-4317-a888-abd27abad504
 */
NTSYSAPI
NTSTATUS
NTAPI
SamQueryDisplayInformation(
    _In_ SAM_HANDLE DomainHandle,
    _In_ DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    _In_ ULONG Index,
    _In_ ULONG EntryCount,
    _In_ ULONG PreferredMaximumLength,
    _Out_ PULONG TotalAvailable,
    _Out_ PULONG TotalReturned,
    _Out_ PULONG ReturnedEntryCount,
    _Outptr_ PVOID *SortedBuffer
    );

/**
 * The SamGetDisplayEnumerationIndex method obtains an index into an ascending account-name–sorted list of accounts.
 *
 * \param DomainHandle A handle representing a domain.
 * \param DisplayInformation An enumeration indicating the set of objects for which to return an index.
 * \param Prefix A string matched against the account name to find a starting point for an enumeration.
 * \param Index A value to use as input to SamQueryDisplayInformation in order to control the accounts that are returned from that method.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/bd429624-f2d5-4717-8aa2-659952c3e209
 */
NTSYSAPI
NTSTATUS
NTAPI
SamGetDisplayEnumerationIndex(
    _In_ SAM_HANDLE DomainHandle,
    _In_ DOMAIN_DISPLAY_INFORMATION DisplayInformation,
    _In_ PCUNICODE_STRING Prefix,
    _Out_ PULONG Index
    );

//
// Database replication
//

typedef enum _SECURITY_DB_DELTA_TYPE
{
    SecurityDbNew = 1,
    SecurityDbRename,
    SecurityDbDelete,
    SecurityDbChangeMemberAdd,
    SecurityDbChangeMemberSet,
    SecurityDbChangeMemberDel,
    SecurityDbChange,
    SecurityDbChangePassword
} SECURITY_DB_DELTA_TYPE, *PSECURITY_DB_DELTA_TYPE;

typedef enum _SECURITY_DB_OBJECT_TYPE
{
    SecurityDbObjectSamDomain = 1,
    SecurityDbObjectSamUser,
    SecurityDbObjectSamGroup,
    SecurityDbObjectSamAlias,
    SecurityDbObjectLsaPolicy,
    SecurityDbObjectLsaTDomain,
    SecurityDbObjectLsaAccount,
    SecurityDbObjectLsaSecret
} SECURITY_DB_OBJECT_TYPE, *PSECURITY_DB_OBJECT_TYPE;

typedef enum _SAM_ACCOUNT_TYPE
{
    SamObjectUser = 1,
    SamObjectGroup,
    SamObjectAlias
} SAM_ACCOUNT_TYPE, *PSAM_ACCOUNT_TYPE;

#define SAM_USER_ACCOUNT (0x00000001)
#define SAM_GLOBAL_GROUP_ACCOUNT (0x00000002)
#define SAM_LOCAL_GROUP_ACCOUNT (0x00000004)

typedef struct _SAM_GROUP_MEMBER_ID
{
    ULONG MemberRid;
} SAM_GROUP_MEMBER_ID, *PSAM_GROUP_MEMBER_ID;

typedef struct _SAM_ALIAS_MEMBER_ID
{
    PSID MemberSid;
} SAM_ALIAS_MEMBER_ID, *PSAM_ALIAS_MEMBER_ID;

typedef union _SAM_DELTA_DATA
{
    SAM_GROUP_MEMBER_ID GroupMemberId;
    SAM_ALIAS_MEMBER_ID AliasMemberId;
    ULONG AccountControl;
} SAM_DELTA_DATA, *PSAM_DELTA_DATA;

/**
 * Notifies a consumer of a change to an object in a security database.
 *
 * \param DomainSid The SID of the domain containing the changed object.
 * \param DeltaType The type of change.
 * \param ObjectType The type of object that changed.
 * \param ObjectRid The RID of the changed object.
 * \param ObjectName The name of the changed object, if supplied.
 * \param ModifiedCount The database modification count associated with the change.
 * \param DeltaData Additional information specific to the change, if supplied.
 * \return NTSTATUS indicating success or failure.
 */
typedef _Function_class_(SAM_DELTA_NOTIFICATION_ROUTINE)
NTSTATUS NTAPI SAM_DELTA_NOTIFICATION_ROUTINE(
    _In_ PSID DomainSid,
    _In_ SECURITY_DB_DELTA_TYPE DeltaType,
    _In_ SECURITY_DB_OBJECT_TYPE ObjectType,
    _In_ ULONG ObjectRid,
    _In_opt_ PCUNICODE_STRING ObjectName,
    _In_ PLARGE_INTEGER ModifiedCount,
    _In_opt_ PSAM_DELTA_DATA DeltaData
    );
typedef SAM_DELTA_NOTIFICATION_ROUTINE* PSAM_DELTA_NOTIFICATION_ROUTINE;

#define SAM_DELTA_NOTIFY_ROUTINE "DeltaNotify"

/**
 * Registers an event for notifications of SAM object changes.
 *
 * \param ObjectType The type of object to monitor.
 * \param NotificationEventHandle The event to signal when a matching object changes.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamRegisterObjectChangeNotification(
    _In_ SECURITY_DB_OBJECT_TYPE ObjectType,
    _In_ HANDLE NotificationEventHandle
    );

/**
 * Unregisters an event previously registered for SAM object changes.
 *
 * \param ObjectType The object type specified when registering the event.
 * \param NotificationEventHandle The event previously registered for notifications.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamUnregisterObjectChangeNotification(
    _In_ SECURITY_DB_OBJECT_TYPE ObjectType,
    _In_ HANDLE NotificationEventHandle
    );

//
// Compatibility mode
//

#define SAM_SID_COMPATIBILITY_ALL 0
#define SAM_SID_COMPATIBILITY_LAX 1
#define SAM_SID_COMPATIBILITY_STRICT 2

/**
 * Queries the SID compatibility mode for a SAM object.
 *
 * \param ObjectHandle A handle to the SAM object.
 * \param Mode Receives a SAM_SID_COMPATIBILITY_* value.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamGetCompatibilityMode(
    _In_ SAM_HANDLE ObjectHandle,
    _Out_ ULONG *Mode
    );

//
// Password validation
//

typedef enum _PASSWORD_POLICY_VALIDATION_TYPE
{
    SamValidateAuthentication = 1,
    SamValidatePasswordChange,
    SamValidatePasswordReset
} PASSWORD_POLICY_VALIDATION_TYPE;

typedef struct _SAM_VALIDATE_PASSWORD_HASH
{
    ULONG Length;
    _Field_size_bytes_(Length) PUCHAR Hash;
} SAM_VALIDATE_PASSWORD_HASH, *PSAM_VALIDATE_PASSWORD_HASH;

// Flags for PresentFields in SAM_VALIDATE_PERSISTED_FIELDS

#define SAM_VALIDATE_PASSWORD_LAST_SET 0x00000001
#define SAM_VALIDATE_BAD_PASSWORD_TIME 0x00000002
#define SAM_VALIDATE_LOCKOUT_TIME 0x00000004
#define SAM_VALIDATE_BAD_PASSWORD_COUNT 0x00000008
#define SAM_VALIDATE_PASSWORD_HISTORY_LENGTH 0x00000010
#define SAM_VALIDATE_PASSWORD_HISTORY 0x00000020

typedef struct _SAM_VALIDATE_PERSISTED_FIELDS
{
    ULONG PresentFields;
    LARGE_INTEGER PasswordLastSet;
    LARGE_INTEGER BadPasswordTime;
    LARGE_INTEGER LockoutTime;
    ULONG BadPasswordCount;
    ULONG PasswordHistoryLength;
    _Field_size_bytes_(PasswordHistoryLength) PSAM_VALIDATE_PASSWORD_HASH PasswordHistory;
} SAM_VALIDATE_PERSISTED_FIELDS, *PSAM_VALIDATE_PERSISTED_FIELDS;

typedef enum _SAM_VALIDATE_VALIDATION_STATUS
{
    SamValidateSuccess = 0,
    SamValidatePasswordMustChange,
    SamValidateAccountLockedOut,
    SamValidatePasswordExpired,
    SamValidatePasswordIncorrect,
    SamValidatePasswordIsInHistory,
    SamValidatePasswordTooShort,
    SamValidatePasswordTooLong,
    SamValidatePasswordNotComplexEnough,
    SamValidatePasswordTooRecent,
    SamValidatePasswordFilterError
} SAM_VALIDATE_VALIDATION_STATUS, *PSAM_VALIDATE_VALIDATION_STATUS;

typedef struct _SAM_VALIDATE_STANDARD_OUTPUT_ARG
{
    SAM_VALIDATE_PERSISTED_FIELDS ChangedPersistedFields;
    SAM_VALIDATE_VALIDATION_STATUS ValidationStatus;
} SAM_VALIDATE_STANDARD_OUTPUT_ARG, *PSAM_VALIDATE_STANDARD_OUTPUT_ARG;

typedef struct _SAM_VALIDATE_AUTHENTICATION_INPUT_ARG
{
    SAM_VALIDATE_PERSISTED_FIELDS InputPersistedFields;
    BOOLEAN PasswordMatched;
} SAM_VALIDATE_AUTHENTICATION_INPUT_ARG, *PSAM_VALIDATE_AUTHENTICATION_INPUT_ARG;

typedef struct _SAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG
{
    SAM_VALIDATE_PERSISTED_FIELDS InputPersistedFields;
    UNICODE_STRING ClearPassword;
    UNICODE_STRING UserAccountName;
    SAM_VALIDATE_PASSWORD_HASH HashedPassword;
    BOOLEAN PasswordMatch; // denotes if the old password supplied by user matched or not
} SAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG, *PSAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG;

typedef struct _SAM_VALIDATE_PASSWORD_RESET_INPUT_ARG
{
    SAM_VALIDATE_PERSISTED_FIELDS InputPersistedFields;
    UNICODE_STRING ClearPassword;
    UNICODE_STRING UserAccountName;
    SAM_VALIDATE_PASSWORD_HASH HashedPassword;
    BOOLEAN PasswordMustChangeAtNextLogon; // looked at only for password reset
    BOOLEAN ClearLockout; // can be used to clear user account lockout
} SAM_VALIDATE_PASSWORD_RESET_INPUT_ARG, *PSAM_VALIDATE_PASSWORD_RESET_INPUT_ARG;

typedef union _SAM_VALIDATE_INPUT_ARG
{
    SAM_VALIDATE_AUTHENTICATION_INPUT_ARG ValidateAuthenticationInput;
    SAM_VALIDATE_PASSWORD_CHANGE_INPUT_ARG ValidatePasswordChangeInput;
    SAM_VALIDATE_PASSWORD_RESET_INPUT_ARG ValidatePasswordResetInput;
} SAM_VALIDATE_INPUT_ARG, *PSAM_VALIDATE_INPUT_ARG;

typedef union _SAM_VALIDATE_OUTPUT_ARG
{
    SAM_VALIDATE_STANDARD_OUTPUT_ARG ValidateAuthenticationOutput;
    SAM_VALIDATE_STANDARD_OUTPUT_ARG ValidatePasswordChangeOutput;
    SAM_VALIDATE_STANDARD_OUTPUT_ARG ValidatePasswordResetOutput;
} SAM_VALIDATE_OUTPUT_ARG, *PSAM_VALIDATE_OUTPUT_ARG;

/**
 * Validates authentication or a proposed password change or reset against password policy.
 *
 * \param ServerName The name of the server, or NULL for the local server.
 * \param ValidationType The type of validation to perform.
 * \param InputArg The input union member corresponding to ValidationType.
 * \param OutputArg Receives the validation result, including the policy validation status and changed persisted fields.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamValidatePassword(
    _In_opt_ PCUNICODE_STRING ServerName,
    _In_ PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
    _In_ PSAM_VALIDATE_INPUT_ARG InputArg,
    _Out_ PSAM_VALIDATE_OUTPUT_ARG *OutputArg
    );

//
// Generic operation
//

typedef enum _SAM_GENERIC_OPERATION_TYPE
{
    SamObjectChangeNotificationOperation
} SAM_GENERIC_OPERATION_TYPE, *PSAM_GENERIC_OPERATION_TYPE;

typedef struct _SAM_OPERATION_OBJCHG_INPUT
{
    BOOLEAN Register;
    ULONG64 EventHandle;
    SECURITY_DB_OBJECT_TYPE ObjectType;
    ULONG ProcessID;
} SAM_OPERATION_OBJCHG_INPUT, *PSAM_OPERATION_OBJCHG_INPUT;

typedef struct _SAM_OPERATION_OBJCHG_OUTPUT
{
    ULONG Reserved;
} SAM_OPERATION_OBJCHG_OUTPUT, *PSAM_OPERATION_OBJCHG_OUTPUT;

typedef union _SAM_GENERIC_OPERATION_INPUT
{
    SAM_OPERATION_OBJCHG_INPUT ObjChangeIn;
} SAM_GENERIC_OPERATION_INPUT, *PSAM_GENERIC_OPERATION_INPUT;

typedef union _SAM_GENERIC_OPERATION_OUTPUT
{
    SAM_OPERATION_OBJCHG_OUTPUT ObjChangeOut;
} SAM_GENERIC_OPERATION_OUTPUT, *PSAM_GENERIC_OPERATION_OUTPUT;

/**
 * Performs a SAM operation selected by its operation type.
 *
 * \param ServerName The name of the server, or NULL for the local server.
 * \param OperationType The operation to perform.
 * \param OperationIn The input union member corresponding to OperationType.
 * \param OperationOut Receives the output for the selected operation.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamPerformGenericOperation(
    _In_opt_ PCWSTR ServerName,
    _In_ SAM_GENERIC_OPERATION_TYPE OperationType,
    _In_ PSAM_GENERIC_OPERATION_INPUT OperationIn,
    _Out_ PSAM_GENERIC_OPERATION_OUTPUT *OperationOut
    );

//
// Private SAM exports
//

// rev
/**
 * Performs local Windows LAPS cleanup for Sysprep.
 *
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamLapsSysprepCleanup(
    VOID
    );

// rev
/**
 * Invokes the private domain test entry point.
 *
 * \param DomainHandle The domain handle passed to the test entry point.
 * \return NTSTATUS indicating success or failure.
 * \remarks The inspected implementation returns STATUS_NOT_IMPLEMENTED.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamTestPrivateFunctionsDomain(
    _In_ SAM_HANDLE DomainHandle
    );

// rev
/**
 * Invokes the private user test entry point.
 *
 * \param UserHandle The user handle passed to the test entry point.
 * \return NTSTATUS indicating success or failure.
 * \remarks The inspected implementation returns STATUS_NOT_IMPLEMENTED.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamTestPrivateFunctionsUser(
    _In_ SAM_HANDLE UserHandle
    );

// rev
/**
 * Checks whether an account is a delegated managed service account and whether the caller is authorized to use it.
 *
 * \param ServerName The server name, or NULL for the local server.
 * \param AccountName The account name to query.
 * \param Result Receives whether the account is a delegated managed service account.
 * \param Authorized Receives whether the caller is authorized to use the account.
 * \return NTSTATUS indicating success or failure.
 * \remarks Both output values are initialized to FALSE, including on failure.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-samr/38d8c44c-292a-4a2e-a6a7-75450ab6439f
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiAccountIsDelegateManagedServiceAccount(
    _In_opt_ PCUNICODE_STRING ServerName,
    _In_ PCUNICODE_STRING AccountName,
    _Out_ PBOOLEAN Result,
    _Out_ PBOOLEAN Authorized
    );

// rev
/**
 * Changes the local SAM boot key.
 *
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiChangeKeys(
    VOID
    );

// rev
/**
 * Changes a user password using LM and NT one-way function (OWF) password hashes.
 *
 * \param UserHandle A handle to the user account.
 * \param LmPresent Whether LM password hashes are supplied.
 * \param OldLmOwfPassword The 16-byte old LM password hash, required when LmPresent is TRUE.
 * \param NewLmOwfPassword The 16-byte new LM password hash, required when LmPresent is TRUE.
 * \param NtPresent Whether NT password hashes are supplied.
 * \param OldNtOwfPassword The 16-byte old NT password hash, required when NtPresent is TRUE.
 * \param NewNtOwfPassword The 16-byte new NT password hash, required when NtPresent is TRUE.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiChangePasswordUser(
    _In_ SAM_HANDLE UserHandle,
    _In_ BOOLEAN LmPresent,
    _In_reads_bytes_opt_(16) PCVOID OldLmOwfPassword,
    _In_reads_bytes_opt_(16) PCVOID NewLmOwfPassword,
    _In_ BOOLEAN NtPresent,
    _In_reads_bytes_opt_(16) PCVOID OldNtOwfPassword,
    _In_reads_bytes_opt_(16) PCVOID NewNtOwfPassword
    );

// rev
/**
 * Changes a user password using encrypted password buffers.
 *
 * \param ServerName The server name, or NULL for the local server.
 * \param UserName The user account name.
 * \param NewPasswordEncryptedWithOldNt The new password encrypted with the old NT password hash.
 * \param OldNtOwfPasswordEncryptedWithNewNt The old NT password hash encrypted with the new NT password hash.
 * \param LmPresent Whether the LM password buffers are supplied.
 * \param NewPasswordEncryptedWithOldLm The new password encrypted with the old LM password hash, required when LmPresent is TRUE.
 * \param OldLmOwfPasswordEncryptedWithNewNt The old LM password hash encrypted with the new NT password hash, required when LmPresent is TRUE.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiChangePasswordUser2(
    _In_opt_ PCUNICODE_STRING ServerName,
    _In_ PCUNICODE_STRING UserName,
    _In_ PENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    _In_ PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    _In_ BOOLEAN LmPresent,
    _In_opt_ PENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    _In_opt_ PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewNt
    );

// rev
/**
 * Prepares encrypted password buffers for SamiChangePasswordUser2.
 *
 * \param OldPassword The current password.
 * \param NewPassword The new password, limited to SAM_MAX_PASSWORD_LENGTH characters.
 * \param NewPasswordEncryptedWithOldNt Receives the new password encrypted with the old NT password hash.
 * \param OldNtOwfPasswordEncryptedWithNewNt Receives the old NT password hash encrypted with the new NT password hash.
 * \param LmPresent Receives whether LM password buffers were generated.
 * \param NewPasswordEncryptedWithOldLm Receives the new password encrypted with the old LM password hash when LmPresent is TRUE.
 * \param OldLmOwfPasswordEncryptedWithNewNt Receives the old LM password hash encrypted with the new NT password hash when LmPresent is TRUE.
 * \return NTSTATUS indicating success or failure.
 * \remarks Output buffers and LmPresent may be modified on failure. LM buffers are only valid when LmPresent is TRUE.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiEncryptPasswords(
    _In_ PCUNICODE_STRING OldPassword,
    _In_ PCUNICODE_STRING NewPassword,
    _Out_ PENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    _Out_ PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    _Out_ PBOOLEAN LmPresent,
    _Out_ PENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    _Out_ PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewNt
    );

// rev
/**
 * Finds or creates the local shadow administrator account associated with a user SID.
 *
 * \param UserSid The SID of the user whose shadow administrator account is requested.
 * \param AccountName Receives the allocated account name. Free with SamFreeMemory.
 * \param AccountSid Receives the allocated account SID. Free with SamFreeMemory.
 * \return NTSTATUS indicating success or failure.
 * \remarks Both output pointers are initialized to NULL, including on failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiFindOrCreateShadowAdminAccount(
    _In_ PSID UserSid,
    _Outptr_ PWSTR *AccountName,
    _Outptr_ PSID *AccountSid
    );

// rev
/**
 * Builds a localized message describing the server's password policy requirements.
 *
 * \param ServerName The server name, or NULL for the local server.
 * \param ErrorMessage Receives the allocated message. Free with SamFreeMemory.
 * \return NTSTATUS indicating success or failure.
 * \remarks ErrorMessage is initialized to NULL, including on failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiGetBadPasswordErrorMessage(
    _In_opt_ PCUNICODE_STRING ServerName,
    _Outptr_ PWSTR *ErrorMessage
    );

// rev
/**
 * Checks whether a SID identifies a local shadow administrator account.
 *
 * \param AccountSid The account SID to check.
 * \param IsShadowAdmin Receives whether the SID identifies a shadow administrator account.
 * \param AssociatedAccountName Receives the allocated associated account name. Free with SamFreeMemory.
 * \param AssociatedAccountSid Receives the allocated associated account SID. Free with SamFreeMemory.
 * \return NTSTATUS indicating success or failure.
 * \remarks The Boolean output is initialized to FALSE and both output pointers to NULL, including on failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiIsShadowAdminAccount(
    _In_ PSID AccountSid,
    _Out_ PBOOLEAN IsShadowAdmin,
    _Outptr_ PWSTR *AssociatedAccountName,
    _Outptr_ PSID *AssociatedAccountSid
    );

// rev
/**
 * Changes a user's LM password using mutually encrypted LM password hashes.
 *
 * \param UserHandle A handle to the user account.
 * \param OldLmOwfPasswordEncryptedWithNewLm The old LM password hash encrypted with the new LM password hash.
 * \param NewLmOwfPasswordEncryptedWithOldLm The new LM password hash encrypted with the old LM password hash.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiLmChangePasswordUser(
    _In_ SAM_HANDLE UserHandle,
    _In_ PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLm,
    _In_ PENCRYPTED_LM_OWF_PASSWORD NewLmOwfPasswordEncryptedWithOldLm
    );

// rev
/**
 * Submits boot key information for a SAM domain.
 *
 * \param DomainHandle A handle to the domain.
 * \param Operation The boot key operation selector.
 * \param OldBootKey The counted byte buffer containing the old boot key.
 * \param NewBootKey The counted byte buffer containing the new boot key.
 * \return NTSTATUS indicating success or failure.
 * \remarks The string descriptors contain binary key data rather than text.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiSetBootKeyInformation(
    _In_ SAM_HANDLE DomainHandle,
    _In_ BOOLEAN Operation,
    _In_ PCUNICODE_STRING OldBootKey,
    _In_ PCUNICODE_STRING NewBootKey
    );

// rev
/**
 * Sets a Directory Services Restore Mode account password.
 *
 * \param ServerName The server name, or NULL for the local server.
 * \param UserId The RID of the account whose password is set.
 * \param NewPassword The new password.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiSetDSRMPassword(
    _In_opt_ PCUNICODE_STRING ServerName,
    _In_ ULONG UserId,
    _In_ PCUNICODE_STRING NewPassword
    );

// rev
/**
 * Sets a Directory Services Restore Mode account password from its NT OWF hash.
 *
 * \param ServerName The server name, or NULL for the local server.
 * \param UserId The RID of the account whose password is set.
 * \param NewPasswordOwf The 16-byte NT password hash.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiSetDSRMPasswordOWF(
    _In_opt_ PCUNICODE_STRING ServerName,
    _In_ ULONG UserId,
    _In_reads_bytes_(16) PCVOID NewPasswordOwf
    );

// rev
/**
 * Synchronizes the local Directory Services Restore Mode password from an account.
 *
 * \param UserId The RID of the account supplying the password.
 * \return NTSTATUS indicating success or failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiSyncDSRMPasswordFromAccount(
    _In_ ULONG UserId
    );

// rev
/**
 * Checks whether the caller may reuse a computer account.
 *
 * \param ServerName The server name, or NULL for the local server.
 * \param ComputerSid The SID of the computer account.
 * \param CompatibilityVersion The required server account reuse validation version, 1 or 2.
 * \param Result Receives whether account reuse is permitted.
 * \return NTSTATUS indicating success or failure.
 * \remarks Result is initialized to FALSE, including on failure.
 */
NTSYSAPI
NTSTATUS
NTAPI
SamiValidateComputerAccountReuseAttempt(
    _In_opt_ PCUNICODE_STRING ServerName,
    _In_ PSID ComputerSid,
    _In_ ULONG CompatibilityVersion,
    _Out_ PBOOLEAN Result
    );

// rev
/**
 * Refreshes localized SAM account names for a machine UI language.
 *
 * \param Language The null-terminated hexadecimal language identifier.
 * \param Flags Initialization flags. Bit 0 skips the refresh.
 * \return Zero on success or one on failure.
 * \remarks Returns zero on success or one on failure. The last error is set from the localization status when a refresh is attempted.
 */
NTSYSAPI
ULONG
NTAPI
OnMachineUILanguageInit(
    _In_ PCWSTR Language,
    _In_ ULONG Flags
    );

#endif // _NTSAM_H
