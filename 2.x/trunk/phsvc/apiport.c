#include <phsvc.h>

HANDLE PhSvcApiPortHandle;

NTSTATUS PhApiPortInitialization()
{
    static SID_IDENTIFIER_AUTHORITY worldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING portName;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG sdAllocationLength;
    PACL dacl;
    PSID worldSid;

    RtlInitUnicodeString(&portName, PHSVC_PORT_NAME);

    worldSid = PhAllocate(RtlLengthRequiredSid(1));
    RtlInitializeSid(worldSid, &worldSidAuthority, 1);
    *(RtlSubAuthoritySid(worldSid, 0)) = SECURITY_WORLD_RID;

    sdAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
        (ULONG)sizeof(ACL) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(worldSid) +
        32;

    securityDescriptor = PhAllocate(sdAllocationLength);
    dacl = (PACL)PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);

    RtlCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    RtlCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION2);

    RtlAddAccessAllowedAce(dacl, ACL_REVISION2, PORT_ALL_ACCESS, worldSid);
    RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

    InitializeObjectAttributes(
        &oa,
        &portName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        securityDescriptor
        );

    status = NtCreatePort(
        &PhSvcApiPortHandle,
        &oa,
        sizeof(PHSVC_API_CONNECTINFO),
        sizeof(PHSVC_API_MSG),
        0
        );

    PhFree(securityDescriptor);
    PhFree(worldSid);

    return status;
}
