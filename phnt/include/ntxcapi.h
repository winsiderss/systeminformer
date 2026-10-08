/*
 * Exception support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTXCAPI_H
#define _NTXCAPI_H

/**
 * Dispatches an exception to the registered exception handlers for the current thread.
 *
 * \param ExceptionRecord A pointer to the exception record. The dispatcher can update its flags while processing the exception.
 * \param ContextRecord A pointer to the processor context for the exception. An exception handler can modify this context.
 * \return TRUE if the exception was handled; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDispatchException(
    _Inout_ PEXCEPTION_RECORD ExceptionRecord,
    _Inout_ PCONTEXT ContextRecord
    );

/**
 * Raises an exception whose exception code is the specified NTSTATUS value.
 *
 * \param Status The NTSTATUS value to raise as an exception.
 * \return This function does not return.
 */
_Analysis_noreturn_
NTSYSAPI
DECLSPEC_NORETURN
VOID
NTAPI
RtlRaiseStatus(
    _In_ NTSTATUS Status
    );

/**
 * Raises the supplied exception in the calling thread.
 *
 * \param ExceptionRecord A pointer to an exception record that describes the exception. The routine updates the exception flags and address during dispatch.
 * \return This function does not return a value.
 * \remarks The routine returns to its caller only if an exception handler handles the exception and resumes execution.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-raiseexception
 */
NTSYSAPI
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
/**
 * Terminates the calling process immediately by raising a fast-fail exception when a return address hijack is detected.
 *
 * \details This function is typically invoked by security mitigations, such as Hardware-enforced Stack Protection (Intel CET / Shadow Stack),
 * upon detecting an inconsistency between the expected and actual return addresses on the stack.
 * It internally triggers a fatal exception (typically STATUS_STACK_BUFFER_OVERRUN) and does not return to the caller.
 */
NTSYSAPI
VOID
NTAPI
RtlRaiseExceptionForReturnAddressHijack(
    VOID
    );

/**
 * Captures a processor context and raises the supplied noncontinuable exception.
 *
 * \param ExceptionRecord A pointer to the exception record to dispatch or raise. If its exception address is NULL, the routine supplies the caller's return address.
 * \param ContextRecord A pointer to a context record that receives the captured processor state.
 * \param FirstChance TRUE to dispatch the exception directly as a first-chance exception; FALSE to raise it through the system service.
 * \return The status returned by the selected exception path if execution resumes.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRaiseNoncontinuableException(
    _Inout_ PEXCEPTION_RECORD ExceptionRecord,
    _Out_ PCONTEXT ContextRecord,
    _In_ BOOLEAN FirstChance
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * Identifies the continuation operation requested from NtContinueEx.
 */
typedef enum _KCONTINUE_TYPE
{
    KCONTINUE_UNWIND,   // Continue an unwind operation.
    KCONTINUE_RESUME,   // Resume execution from the supplied context.
    KCONTINUE_LONGJUMP, // Perform a long-jump continuation.
    KCONTINUE_SET,      // Set the supplied continuation context.
    KCONTINUE_LAST,     // Maximum continuation type value.
} KCONTINUE_TYPE;

/**
 * Describes an extended continuation operation for NtContinueEx.
 */
typedef struct _KCONTINUE_ARGUMENT
{
    KCONTINUE_TYPE ContinueType; // The requested continuation operation.
    ULONG ContinueFlags;         // A combination of KCONTINUE_FLAG_* values.
    ULONGLONG Reserved[2];        // Reserved; initialize to zero.
} KCONTINUE_ARGUMENT, *PKCONTINUE_ARGUMENT;

/** Requests alert testing while continuing execution. */
#define KCONTINUE_FLAG_TEST_ALERT 0x00000001
/** Requests delivery of a pending user-mode APC while continuing execution. */
#define KCONTINUE_FLAG_DELIVER_APC 0x00000002

/**
 * Continues execution from a supplied processor context using extended continuation arguments.
 *
 * \param ContextRecord A pointer to the processor context from which execution is to continue.
 * \param ContinueArgument Either a pointer to a KCONTINUE_ARGUMENT structure or a BOOLEAN value encoded as a pointer for the legacy TestAlert form.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinueEx(
    _In_ PCONTEXT ContextRecord,
    _In_ PVOID ContinueArgument // PKCONTINUE_ARGUMENT and BOOLEAN are valid
    );

/**
 * Continues execution from a supplied processor context.
 *
 * \param ContextRecord A pointer to the processor context from which execution is to continue.
 * \param TestAlert TRUE to test the current thread for pending alerts before continuing; otherwise, FALSE.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinue(
    _In_ PCONTEXT ContextRecord,
    _In_ BOOLEAN TestAlert
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

/**
 * Raises an exception using the supplied exception and processor context records.
 *
 * \param ExceptionRecord A pointer to the exception record that describes the exception.
 * \param ContextRecord A pointer to the processor context at which the exception occurred.
 * \param FirstChance TRUE to process the exception as a first-chance exception; FALSE to process it as a second-chance exception.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord,
    _In_ BOOLEAN FirstChance
    );

#ifndef _PHNT_RTLASSERT_DECLARED
#define _PHNT_RTLASSERT_DECLARED
/**
 * The RtlAssert routine reports an assertion failure and prompts the debugger or user for a response.
 *
 * \param VoidFailedAssertion A pointer to the null-terminated ANSI expression that failed.
 * \param VoidFileName A pointer to the null-terminated ANSI source file name.
 * \param LineNumber The source line number at which the assertion failed.
 * \param MutableMessage An optional pointer to a null-terminated ANSI message associated with the assertion.
 * \return This function does not return a value.
 * \remarks Depending on the selected response, the routine can return, break into the debugger, or terminate the current thread or process.
 */
NTSYSAPI
VOID
NTAPI
RtlAssert(
    _In_ PVOID VoidFailedAssertion,
    _In_ PVOID VoidFileName,
    _In_ ULONG LineNumber,
    _In_opt_ PSTR MutableMessage
    );
#endif

#endif // _NTXCAPI_H
