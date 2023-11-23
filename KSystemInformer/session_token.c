/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <kph.h>

#include <trace.h>

typedef struct _KPH_SESSION_TOKEN_INIT
{
    LARGE_INTEGER Expiry;
    ULONG Privileges;
    LONG Uses;
} KPH_SESSION_TOKEN_INIT, *PKPH_SESSION_TOKEN_INIT;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpSessionTokenTypeName = RTL_CONSTANT_STRING(L"KphSessionToken");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpSessionTokenType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

PAGED_FILE();

/**
 * \brief Allocate a session token object.
 *
 * \param[in] Size The size of the object to allocate.
 *
 * \return A pointer to the allocated object, NULL on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateSessionToken(
    _In_ SIZE_T Size
    )
{
    PAGED_CODE();

    //
    // N.B. The session token object is allocated from non-paged pool because it
    // is stored in an atomic object reference in process and thread contexts.
    //

    return KphAllocateNPaged(Size, KPH_TAG_SESSION_TOKEN_OBJECT);
}

/**
 * \brief Free a session token object.
 *
 * \param[in] Object The session token object to free.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpFreeSessionToken(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    KphFree(Object, KPH_TAG_SESSION_TOKEN_OBJECT);
}

/**
 * \brief Initialize a session token object.
 *
 * \param[in] Object The session token object to initialize.
 * \param[in] Parameter The session token object initialization parameters.
 *
 * \return Successful or errant status.
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeSessionToken(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_SESSION_TOKEN token;
    PKPH_SESSION_TOKEN_INIT init;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    token = Object;
    init = Parameter;

    status = ExUuidCreate(&token->AccessToken.Identifier);
    if (status != STATUS_SUCCESS)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ExUuidCreate failed: %!STATUS!",
                      status);

        if (NT_SUCCESS(status))
        {
            status = STATUS_UNSUCCESSFUL;
        }

        return status;
    }

    status = BCryptGenRandom(NULL,
                             token->AccessToken.Material,
                             sizeof(token->AccessToken.Material),
                             BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "BCryptGenRandom failed: %!STATUS!",
                      status);

        return status;
    }

    if (init->Expiry.QuadPart < 0)
    {
        KeQuerySystemTime(&token->AccessToken.Expiry);

        token->AccessToken.Expiry.QuadPart += -(init->Expiry.QuadPart);
    }
    else
    {
        token->AccessToken.Expiry = init->Expiry;
    }

    token->AccessToken.Privileges = init->Privileges;
    token->AccessToken.Uses = init->Uses;

    token->UseCount = 0;

    return STATUS_SUCCESS;
}

/**
 * \brief Requests an access session token.
 *
 * \details After a request for an access session token has been made, the
 * current thread is responsible for assigning the session token to a process
 * or thread later by providing the signature for the access session token
 * content.
 *
 * \param[out] AccessToken Populated with the access session token.
 * \param[in] Expiry The expiry time of the session token.
 * \param[in] Privileges The privileges of the session token.
 * \param[in] Uses The number of uses of the session token.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphRequestSessionAccessToken(
    _Out_ PKPH_SESSION_ACCESS_TOKEN AccessToken,
    _In_ PLARGE_INTEGER Expiry,
    _In_ ULONG Privileges,
    _In_ LONG Uses
    )
{
    NTSTATUS status;
    PKPH_THREAD_CONTEXT thread;
    KPH_SESSION_TOKEN_INIT tokenInit;
    PKPH_SESSION_TOKEN token;

    PAGED_CODE_PASSIVE();

    RtlZeroMemory(AccessToken, sizeof(KPH_SESSION_ACCESS_TOKEN));

    thread = NULL;
    token = NULL;

    if (!Privileges || (Privileges & ~KPH_TOKEN_VALID_PRIVILEGES))
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    thread = KphGetCurrentThreadContext();
    if (!thread)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    tokenInit.Expiry = *Expiry;
    tokenInit.Privileges = Privileges;
    tokenInit.Uses = Uses;

    status = KphCreateObject(KphpSessionTokenType,
                             sizeof(KPH_SESSION_TOKEN),
                             &token,
                             &tokenInit);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    RtlCopyMemory(AccessToken,
                  &token->AccessToken,
                  sizeof(KPH_SESSION_ACCESS_TOKEN));

    KphAtomicAssignObjectReference(&thread->RequestSessionToken.Atomic, token);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  GENERAL,
                  "[%!GUID!] %!TIME! %ld",
                  &token->AccessToken.Identifier,
                  token->AccessToken.Expiry.QuadPart,
                  token->AccessToken.Uses);

Exit:

    if (token)
    {
        KphDereferenceObject(token);
    }

    if (thread)
    {
        KphDereferenceObject(thread);
    }

    return status;
}

/**
 * \brief Verifies an access session token.
 *
 * \param[in] Actor The actor process setting the session token.
 * \param[in] Target The target process receiving the session token.
 * \param[in] Token The session token to verify.
 * \param[in] Signature The signature of the session token.
 * \param[in] SignatureLength The length of the signature.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpVerifySessionToken(
    _In_ PKPH_PROCESS_CONTEXT Actor,
    _In_ PKPH_PROCESS_CONTEXT Target,
    _In_ PKPH_SESSION_TOKEN Token,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    KPH_PROCESS_STATE actorState;
    KPH_PROCESS_STATE targetState;
    PBYTE signature;

    PAGED_CODE_PASSIVE();

    signature = NULL;
    dyn = NULL;

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->SessionTokenPublicKeyHandle)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        signature = KphAllocatePaged(SignatureLength,
                                     KPH_TAG_SESSION_TOKEN_SIGNATURE);
        if (!signature)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Failed to allocate signature buffer.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            ProbeForRead(Signature, SignatureLength, 1);
            RtlCopyMemory(signature, Signature, SignatureLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        signature = Signature;
    }

    status = KphVerifyBufferEx(dyn->SessionTokenPublicKeyHandle,
                               (PBYTE)&Token->AccessToken,
                               sizeof(Token->AccessToken),
                               signature,
                               SignatureLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphVerifyBufferEx failed: %!STATUS!",
                      status);

        goto Exit;
    }

    actorState = KphGetProcessState(Actor);
    targetState = KphGetProcessState(Target);

    if (((actorState & KPH_PROCESS_STATE_MAXIMUM) != KPH_PROCESS_STATE_MAXIMUM) ||
        ((actorState & KPH_PROCESS_STATE_MAXIMUM) != KPH_PROCESS_STATE_MAXIMUM))
    {
        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  GENERAL,
                  "[%!GUID!] %!TIME! %ld",
                  &Token->AccessToken.Identifier,
                  Token->AccessToken.Expiry.QuadPart,
                  Token->AccessToken.Uses);

    status = STATUS_SUCCESS;

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (signature && (signature != Signature))
    {
        KphFree(signature, KPH_TAG_SESSION_TOKEN_SIGNATURE);
    }

    return status;
}

/**
 * \brief Assigns an access session token to a process.
 *
 * \param[in] Process The process to assign the session token to.
 * \param[in] Signature The signature of the session token.
 * \param[in] SignatureLength The length of the signature.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAssignProcessSessionToken(
    _Inout_ PKPH_PROCESS_CONTEXT Process,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_THREAD_CONTEXT actor;
    PKPH_SESSION_TOKEN token;

    PAGED_CODE_PASSIVE();

    token = NULL;

    actor = KphGetCurrentThreadContext();
    if (!actor)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    token = KphAtomicMoveObjectReference(&actor->RequestSessionToken.Atomic,
                                         NULL);
    if (!token)
    {
        status = STATUS_INVALID_STATE_TRANSITION;
        goto Exit;
    }

    if (!actor->ProcessContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpVerifySessionToken(actor->ProcessContext,
                                    Process,
                                    token,
                                    Signature,
                                    SignatureLength,
                                    AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphpVerifyToken failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphAtomicAssignObjectReference(&Process->SessionToken.Atomic, token);

    status = STATUS_SUCCESS;

Exit:

    if (token)
    {
        KphDereferenceObject(token);
    }

    if (actor)
    {
        KphDereferenceObject(actor);
    }

    return status;
}

/**
 * \brief Assigns an access session token to a process.
 *
 * \param[in] ProcessHandle Handle to the process to assign the session token.
 * \param[in] Signature The signature of the session token.
 * \param[in] SignatureLength The length of the signature.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphAssignProcessSessionToken(
    _In_ HANDLE ProcessHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT processContext;

    PAGED_CODE_PASSIVE();

    processObject = NULL;
    processContext = NULL;

    if (!Signature || !SignatureLength)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &processObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        processObject = NULL;
        goto Exit;
    }

    processContext = KphGetEProcessContext(processObject);
    if (!processContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpAssignProcessSessionToken(processContext,
                                           Signature,
                                           SignatureLength,
                                           AccessMode);

Exit:

    if (processContext)
    {
        KphDereferenceObject(processContext);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    return status;
}

/**
 * \brief Assigns an access session token to a thread.
 *
 * \param[in] Thread The thread to assign the session token to.
 * \param[in] Signature The signature of the session token.
 * \param[in] SignatureLength The length of the signature.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpAssignThreadSessionToken(
    _Inout_ PKPH_THREAD_CONTEXT Thread,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_THREAD_CONTEXT actor;
    PKPH_SESSION_TOKEN token;

    PAGED_CODE_PASSIVE();

    actor = NULL;
    token = NULL;

    actor = KphGetCurrentThreadContext();
    if (!actor)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    token = KphAtomicMoveObjectReference(&actor->RequestSessionToken.Atomic,
                                         NULL);
    if (!token)
    {
        status = STATUS_INVALID_STATE_TRANSITION;
        goto Exit;
    }

    if (!actor->ProcessContext || !Thread->ProcessContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpVerifySessionToken(actor->ProcessContext,
                                    Thread->ProcessContext,
                                    token,
                                    Signature,
                                    SignatureLength,
                                    AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphpVerifyToken failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphAtomicAssignObjectReference(&Thread->SessionToken.Atomic, token);

    status = STATUS_SUCCESS;

Exit:

    if (token)
    {
        KphDereferenceObject(token);
    }

    if (actor)
    {
        KphDereferenceObject(actor);
    }

    return status;
}

/**
 * \brief Assigns an access session token to a thread.
 *
 * \param[in] ThreadHandle Handle to the thread to assign the session token.
 * \param[in] Signature The signature of the session token.
 * \param[in] SignatureLength The length of the signature.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphAssignThreadSessionToken(
    _In_ HANDLE ThreadHandle,
    _In_ PBYTE Signature,
    _In_ ULONG SignatureLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PETHREAD threadObject;
    PKPH_THREAD_CONTEXT threadContext;

    PAGED_CODE_PASSIVE();

    threadObject = NULL;
    threadContext = NULL;

    if (!Signature || !SignatureLength)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(ThreadHandle,
                                       0,
                                       *PsThreadType,
                                       AccessMode,
                                       &threadObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        threadObject = NULL;
        goto Exit;
    }

    threadContext = KphGetEThreadContext(threadObject);
    if (!threadContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpAssignThreadSessionToken(threadContext,
                                          Signature,
                                          SignatureLength,
                                          AccessMode);

Exit:

    if (threadContext)
    {
        KphDereferenceObject(threadContext);
    }

    if (threadObject)
    {
        ObDereferenceObject(threadObject);
    }

    return status;
}

/**
 * \brief Performs a session token privilege check.
 *
 * \param[in,out] Token The session token to perform the check on.
 * \param[in] Privileges The privileges to check.
 *
 * \return TRUE if the privilege is granted, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphpSessionTokenPrivilegeCheck(
    _Inout_ PKPH_SESSION_TOKEN Token,
    _In_ ULONG Privileges
    )
{
    NTSTATUS status;
    LARGE_INTEGER systemTime;
    LONG useCount;

    PAGED_CODE();

    KeQuerySystemTime(&systemTime);

    useCount = Token->UseCount;
    MemoryBarrier();

    if (Token->AccessToken.Expiry.QuadPart <= systemTime.QuadPart)
    {
        status = STATUS_TIMEOUT;
        goto Exit;
    }

    if ((Token->AccessToken.Privileges & Privileges) != Privileges)
    {
        status = STATUS_PRIVILEGE_NOT_HELD;
        goto Exit;
    }

    if (Token->AccessToken.Uses < 0)
    {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    for (;;)
    {
        if (useCount >= Token->AccessToken.Uses)
        {
            status = STATUS_QUOTA_EXCEEDED;
            goto Exit;
        }

        if (InterlockedCompareExchange(&Token->UseCount,
                                       useCount + 1,
                                       useCount) == useCount)
        {
            useCount++;
            status = STATUS_SUCCESS;
            break;
        }

        useCount = Token->UseCount;
        MemoryBarrier();
    }

Exit:

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  GENERAL,
                  "[%!GUID!] %!TIME! %ld/%ld %!STATUS!",
                  &Token->AccessToken.Identifier,
                  Token->AccessToken.Expiry.QuadPart,
                  useCount,
                  Token->AccessToken.Uses,
                  status);

    return (status == STATUS_SUCCESS);
}

/**
 * \brief Performs a session token privilege check on a thread.
 *
 * \param[in] Thread The thread to perform the session token check on.
 * \param[in] Privileges The privileges to check.
 *
 * \return TRUE if the privilege is granted, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphpThreadSessionTokenPrivilegeCheck(
    _In_ PKPH_THREAD_CONTEXT Thread,
    _In_ ULONG Privileges
    )
{
    BOOLEAN result;
    PKPH_SESSION_TOKEN token;

    PAGED_CODE();

    result = FALSE;

    token = KphAtomicReferenceObject(&Thread->SessionToken.Atomic);
    if (token)
    {
        result = KphpSessionTokenPrivilegeCheck(token, Privileges);

        KphDereferenceObject(token);
    }

    return result;
}

/**
 * \brief Performs a session token privilege check on a process.
 *
 * \param[in] Process The process to perform the session token check on.
 * \param[in] Privileges The privileges to check.
 *
 * \return TRUE if the privilege is granted, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphpProcessSessionTokenPrivilegeCheck(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ ULONG Privileges
    )
{
    BOOLEAN result;
    PKPH_SESSION_TOKEN token;

    PAGED_CODE();

    result = FALSE;

    token = KphAtomicReferenceObject(&Process->SessionToken.Atomic);
    if (token)
    {
        result = KphpSessionTokenPrivilegeCheck(token, Privileges);

        KphDereferenceObject(token);
    }

    return result;
}

/**
 * \brief Performs a session token privilege check.
 *
 * \param[in] Thread The thread to perform the session token check on.
 * \param[in] Privileges The privileges to check.
 *
 * \return TRUE if the privilege is granted, FALSE otherwise.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
BOOLEAN KphSessionTokenPrivilegeCheck(
    _In_ PKPH_THREAD_CONTEXT Thread,
    _In_ ULONG Privileges
    )
{
    PAGED_CODE();

    if (KphpThreadSessionTokenPrivilegeCheck(Thread, Privileges))
    {
        return TRUE;
    }

    if (!Thread->ProcessContext)
    {
        return FALSE;
    }

    return KphpProcessSessionTokenPrivilegeCheck(Thread->ProcessContext,
                                                 Privileges);
}

/**
 * \brief Initialize the session token infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeSessionToken(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    PAGED_CODE_PASSIVE();

    typeInfo.Allocate = KphpAllocateSessionToken;
    typeInfo.Initialize = KphpInitializeSessionToken;
    typeInfo.Delete = NULL;
    typeInfo.Free = KphpFreeSessionToken;

    KphCreateObjectType(&KphpSessionTokenTypeName,
                        &typeInfo,
                        &KphpSessionTokenType);
}
