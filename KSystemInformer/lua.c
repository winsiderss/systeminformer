/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <kph.h>
#include <script.h>

#include <lobject.h>
#include <lstate.h>
#include <ldo.h>
#include <llex.h>
#include <lparser.h>
#include <lundump.h>
#include <lzio.h>

#include <trace.h>

#define KPH_LUA_EXCEPTION_ERROR       ((NTSTATUS)0xE04C5541L)
#define KPH_LUA_EXCEPTION_UNPROTECTED ((NTSTATUS)0xE04C5542L)

typedef struct _KPH_LUA_BLOCK
{
    LIST_ENTRY Link;
} KPH_LUA_BLOCK, *PKPH_LUA_BLOCK;

C_ASSERT(IS_ALIGNED(sizeof(KPH_LUA_BLOCK), MEMORY_ALLOCATION_ALIGNMENT));

typedef struct _KPH_LUA_CALLOUT
{
    PKPH_LUA_CALLOUT_ROUTINE Routine;
    PVOID Context;
    NTSTATUS Status;
} KPH_LUA_CALLOUT, *PKPH_LUA_CALLOUT;

typedef struct _KPH_LUA_PROTECTED_CALL
{
    PKPH_LUA_PROTECTED_CALL_ROUTINE Routine;
    PVOID Context;
} KPH_LUA_PROTECTED_CALL, *PKPH_LUA_PROTECTED_CALL;

KPH_PAGED_FILE();

static int KphpLuaErrno = 0;

/**
 * \brief Finds the first floating point conversion of a format.
 *
 * \param[in] Format The format.
 *
 * \return The conversion character, or ANSI_NULL when there is none.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
CHAR KphpLuaFormatRefused(
    _In_z_ PCSTR Format
    )
{
    BOOLEAN conversion;

    KPH_PAGED_CODE_PASSIVE();

    conversion = FALSE;

    for (PCSTR c = Format; *c != ANSI_NULL; c++)
    {
        if (!conversion)
        {
            if (*c == '%')
            {
                conversion = TRUE;
            }

            continue;
        }

        switch (*c)
        {
            case 'a':
            case 'A':
            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
            {
                return *c;
            }
            case '%':
            case 'c':
            case 'd':
            case 'i':
            case 'o':
            case 'p':
            case 's':
            case 'u':
            case 'x':
            case 'X':
            {
                conversion = FALSE;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    return ANSI_NULL;
}

/**
 * \brief Selects the exception a handler in this file catches.
 *
 * \param[in] Code The code of the exception.
 * \param[in] Expected The code the handler catches.
 *
 * \return EXCEPTION_EXECUTE_HANDLER for the expected code, otherwise
 * EXCEPTION_CONTINUE_SEARCH.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
LONG KphpLuaExceptionFilter(
    _In_ NTSTATUS Code,
    _In_ NTSTATUS Expected
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Code == Expected)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

/**
 * \brief Adds the reserve of an allocator to its limit, once.
 *
 * \param[in,out] Allocator The allocator.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaReleaseReserve(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator
    )
{
    NTSTATUS status;
    SIZE_T limit;

    KPH_PAGED_CODE_PASSIVE();

    status = RtlSIZETAdd(Allocator->Limit, Allocator->Reserve, &limit);
    if (NT_SUCCESS(status))
    {
        Allocator->Limit = limit;
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlSIZETAdd failed: %!STATUS!",
                      status);
    }

    Allocator->Reserve = 0;
}

/**
 * \brief Takes the reserve of an allocator back out of its limit.
 *
 * \param[in,out] Allocator The allocator.
 * \param[in] Reserve The bytes KphLuaReleaseReserve added to the limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaRestoreReserve(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator,
    _In_ SIZE_T Reserve
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Allocator->Reserve == 0);
    NT_ASSERT(Allocator->Limit >= Reserve);

    Allocator->Limit -= Reserve;
    Allocator->Reserve = Reserve;
}

// Declared by Lua and its configuration without SAL; these contracts are ours.
#pragma warning(push)
#pragma warning(disable: 28253 28301)

/**
 * \brief Converts text to a non-integer number, which never succeeds.
 *
 * \param[in] String The text.
 * \param[out] End Optionally receives the start of the text.
 *
 * \return Zero.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
LUA_NUMBER KphLuaStringToNumber(
    _In_z_ const char* String,
    _Out_opt_ char** End
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (End)
    {
        *End = (char*)String;
    }

    return 0;
}

/**
 * \brief Formats text for Lua, without floating point conversions.
 *
 * \param[out] Buffer Receives the text, always terminated.
 * \param[in] Size The size of the buffer in bytes.
 * \param[in] Format The format.
 * \param[in] ... The arguments.
 *
 * \return The length of the text, not counting the terminator.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphLuaFormat(
    _Out_writes_z_(Size) char* Buffer,
    _In_ size_t Size,
    _In_z_ _Printf_format_string_ const char* Format,
    ...
    )
{
    NTSTATUS status;
    va_list args;
    size_t length;
    CHAR refused;
    int result;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Size > 0);

    Buffer[0] = ANSI_NULL;

    refused = KphpLuaFormatRefused(Format);
    if (refused != ANSI_NULL)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Floating point format refused: %hs",
                      Format);

        return 0;
    }

    va_start(args, Format);
    status = RtlStringCbVPrintfA(Buffer, Size, Format, args);
    va_end(args);

    if (!NT_SUCCESS(status) && (status != STATUS_BUFFER_OVERFLOW))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Formatting failed: %!STATUS!",
                      status);

        Buffer[0] = ANSI_NULL;

        return 0;
    }

    status = RtlStringCbLengthA(Buffer, Size, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlStringCbLengthA failed: %!STATUS!",
                      status);

        Buffer[0] = ANSI_NULL;

        return 0;
    }

    status = RtlSizeTToInt(length, &result);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlSizeTToInt failed: %!STATUS!",
                      status);

        Buffer[0] = ANSI_NULL;

        return 0;
    }

    return result;
}

/**
 * \brief Finds the first character of a string that is in a set.
 *
 * \param[in] String The string.
 * \param[in] Accept The set of characters.
 *
 * \return The first character in the set, NULL if there is none.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Ret_maybenull_
char* KphLuaStringBreak(
    _In_z_ const char* String,
    _In_z_ const char* Accept
    )
{
    ANSI_STRING accept;

    KPH_PAGED_CODE_PASSIVE();

    RtlInitAnsiString(&accept, Accept);

    for (PCSTR c = String; *c != ANSI_NULL; c++)
    {
        for (USHORT i = 0; i < accept.Length; i++)
        {
            if (*c == accept.Buffer[i])
            {
                return (char*)c;
            }
        }
    }

    return NULL;
}

/**
 * \brief Checks if a character is in an ASCII character class.
 *
 * \param[in] Character The character.
 * \param[in] Class The class (KPH_LUA_CLASS_*).
 *
 * \return Non-zero if the character is in the class.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphLuaIsClass(
    _In_ int Character,
    _In_ int Class
    )
{
    BOOLEAN upper;
    BOOLEAN lower;
    BOOLEAN digit;
    BOOLEAN graph;
    BOOLEAN hex;
    BOOLEAN result;

    KPH_PAGED_CODE_PASSIVE();

    upper = ((Character >= 'A') && (Character <= 'Z'));
    lower = ((Character >= 'a') && (Character <= 'z'));
    digit = ((Character >= '0') && (Character <= '9'));
    graph = ((Character > ' ') && (Character < 0x7f));

    switch (Class)
    {
        case KPH_LUA_CLASS_ALNUM:
        {
            result = (upper || lower || digit);
            break;
        }
        case KPH_LUA_CLASS_ALPHA:
        {
            result = (upper || lower);
            break;
        }
        case KPH_LUA_CLASS_CNTRL:
        {
            result = ((Character >= 0) && (Character < ' '));
            if (Character == 0x7f)
            {
                result = TRUE;
            }
            break;
        }
        case KPH_LUA_CLASS_DIGIT:
        {
            result = digit;
            break;
        }
        case KPH_LUA_CLASS_GRAPH:
        {
            result = graph;
            break;
        }
        case KPH_LUA_CLASS_LOWER:
        {
            result = lower;
            break;
        }
        case KPH_LUA_CLASS_PUNCT:
        {
            result = graph;
            if (upper || lower || digit)
            {
                result = FALSE;
            }
            break;
        }
        case KPH_LUA_CLASS_SPACE:
        {
            result = ((Character >= '\t') && (Character <= '\r'));
            if (Character == ' ')
            {
                result = TRUE;
            }
            break;
        }
        case KPH_LUA_CLASS_UPPER:
        {
            result = upper;
            break;
        }
        case KPH_LUA_CLASS_XDIGIT:
        {
            hex = ((Character >= 'a') && (Character <= 'f'));
            if ((Character >= 'A') && (Character <= 'F'))
            {
                hex = TRUE;
            }
            result = (digit || hex);
            break;
        }
        default:
        {
            result = FALSE;
            break;
        }
    }

    return result;
}

/**
 * \brief Converts an ASCII upper case letter to lower case.
 *
 * \param[in] Character The character.
 *
 * \return The lower case letter, or the character unchanged.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphLuaToLower(
    _In_ int Character
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if ((Character >= 'A') && (Character <= 'Z'))
    {
        return (Character + ('a' - 'A'));
    }

    return Character;
}

/**
 * \brief Converts an ASCII lower case letter to upper case.
 *
 * \param[in] Character The character.
 *
 * \return The upper case letter, or the character unchanged.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphLuaToUpper(
    _In_ int Character
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if ((Character >= 'a') && (Character <= 'z'))
    {
        return (Character - ('a' - 'A'));
    }

    return Character;
}

/**
 * \brief Retrieves the storage for the errno of Lua's unused file helpers.
 *
 * \return The storage.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Ret_notnull_
int* KphLuaErrno(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return &KphpLuaErrno;
}

/**
 * \brief Retrieves the seed for Lua's string hashes and sort pivot, a constant.
 *
 * \return The seed.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
unsigned int KphLuaSeed(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return 'LISK';
}

/**
 * \brief Raises a Lua error to the nearest KphLuaTry. Does not return.
 *
 * \details A memory error releases the reserve of the allocator, so that what
 * runs while the error unwinds has memory to do it.
 *
 * \param[in] State The Lua state.
 * \param[in] Status The status of the error (LUA_ERR*).
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
__declspec(noreturn)
VOID KphLuaThrow(
    _In_ struct lua_State* State,
    _In_ int Status
    )
{
    PKPH_LUA_ALLOCATOR allocator;
    PVOID context;

    KPH_PAGED_CODE_PASSIVE();

    if (Status == LUA_ERRMEM)
    {
        lua_getallocf(State, &context);

        allocator = context;

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Lua state out of memory: %Iu of %Iu bytes",
                      allocator->Used,
                      allocator->Limit);

        allocator->Failures++;

        KphLuaReleaseReserve(allocator);
    }

    ExRaiseStatus(KPH_LUA_EXCEPTION_ERROR);
}

/**
 * \brief Raises the unprotected error of Lua. Does not return.
 *
 * \details Reached only if Lua raises an error outside every protected call.
 * KphLuaCallout catches it.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
__declspec(noreturn)
VOID KphLuaUnprotectedError(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    ExRaiseStatus(KPH_LUA_EXCEPTION_UNPROTECTED);
}

/**
 * \brief Calls a routine for Lua, returning when it returns or raises a Lua
 * error.
 *
 * \param[in] State The Lua state.
 * \param[in] Jump The recovery point of Lua, which receives the status.
 * \param[in] Routine The routine to call.
 * \param[in] Context The context for the routine.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaTry(
    _In_ struct lua_State* State,
    _In_ struct lua_longjmp* Jump,
    _In_ KPH_LUA_PROTECTED_ROUTINE Routine,
    _In_opt_ void* Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Jump);

    __try
    {
        Routine(State, Context);
    }
    __except (KphpLuaExceptionFilter(GetExceptionCode(),
                                     KPH_LUA_EXCEPTION_ERROR))
    {
        NOTHING;
    }
}

/**
 * \brief Stands in for the lexer's initialization: there is no compiler.
 *
 * \param[in] State The Lua state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
void luaX_init(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(State);
}

/**
 * \brief Stands in for the parser: there is no compiler.
 *
 * \details Raises a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] z Unused.
 * \param[in] anchor Unused.
 * \param[in] buff Unused.
 * \param[in] dyd Unused.
 * \param[in] name Unused.
 * \param[in] firstchar Unused.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
LClosure* luaY_parser(
    _In_ lua_State* State,
    _In_ ZIO* z,
    _In_ Table* anchor,
    _In_ Mbuffer* buff,
    _In_ Dyndata* dyd,
    _In_z_ const char* name,
    _In_ int firstchar
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(z);
    UNREFERENCED_PARAMETER(anchor);
    UNREFERENCED_PARAMETER(buff);
    UNREFERENCED_PARAMETER(dyd);
    UNREFERENCED_PARAMETER(name);
    UNREFERENCED_PARAMETER(firstchar);

    lua_pushliteral(State, "source is not supported");
    luaD_throw(State, LUA_ERRSYNTAX);
}

/**
 * \brief Stands in for the chunk writer: there is no compiler.
 *
 * \param[in] State Unused.
 * \param[in] f Unused.
 * \param[in] w Unused.
 * \param[in] data Unused.
 * \param[in] strip Unused.
 *
 * \return Non-zero, which is a failure to write.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int luaU_dump(
    _In_ lua_State* State,
    _In_ const Proto* f,
    _In_ lua_Writer w,
    _In_opt_ void* data,
    _In_ int strip
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(State);
    UNREFERENCED_PARAMETER(f);
    UNREFERENCED_PARAMETER(w);
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(strip);

    return 1;
}

#pragma warning(pop)

/**
 * \brief Initializes an allocator for a Lua state.
 *
 * \param[out] Allocator The allocator.
 * \param[in] Limit The most bytes the state may hold.
 * \param[in] Reserve The bytes a memory error adds to the limit.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeAllocator(
    _Out_ PKPH_LUA_ALLOCATOR Allocator,
    _In_ SIZE_T Limit,
    _In_ SIZE_T Reserve
    )
{
    KPH_PAGED_CODE_PASSIVE();

    InitializeListHead(&Allocator->Blocks);
    Allocator->Limit = Limit;
    Allocator->Reserve = Reserve;
    Allocator->Used = 0;
    Allocator->CollectAt = Limit - (Limit / 4);
    Allocator->Failures = 0;
    Allocator->Held = NULL;
    Allocator->HeldSize = Reserve;
    Allocator->HeldUsed = 0;
}

/**
 * \brief Deletes an allocator, freeing whatever its state left allocated.
 *
 * \param[in,out] Allocator The allocator.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaDeleteAllocator(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (!IsListEmpty(&Allocator->Blocks))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Lua state left %Iu bytes allocated",
                      Allocator->Used);
    }

    while (!IsListEmpty(&Allocator->Blocks))
    {
        PLIST_ENTRY entry;
        PKPH_LUA_BLOCK block;

        entry = RemoveHeadList(&Allocator->Blocks);
        block = CONTAINING_RECORD(entry, KPH_LUA_BLOCK, Link);

        KphFree(block, KPH_TAG_LUA_BLOCK);
    }

    if (Allocator->Held)
    {
        KphFree(Allocator->Held, KPH_TAG_LUA_RESERVE);
        Allocator->Held = NULL;
    }

    Allocator->Used = 0;
}

/**
 * \brief Allocates, resizes or frees memory for a Lua state.
 *
 * \param[in] Context The allocator.
 * \param[in] Block The block to resize or free, NULL to allocate.
 * \param[in] OldSize The size of the block, when there is one.
 * \param[in] NewSize The size to make the block, zero to free it.
 *
 * \return The block, NULL when freed or when the allocation fails.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Ret_maybenull_
void* KphpLuaAllocate(
    _In_ void* Context,
    _In_opt_ void* Block,
    _In_ size_t OldSize,
    _In_ size_t NewSize
    )
{
    NTSTATUS status;
    PKPH_LUA_ALLOCATOR allocator;
    PKPH_LUA_BLOCK block;
    PKPH_LUA_BLOCK newBlock;
    PVOID memory;
    SIZE_T oldSize;
    SIZE_T used;
    SIZE_T size;
    BOOLEAN held;

    KPH_PAGED_CODE_PASSIVE();

    allocator = Context;

    //
    // Lua passes a type tag as the old size when there is no block.
    //
    if (Block)
    {
        block = Add2Ptr(Block, -(LONG_PTR)sizeof(KPH_LUA_BLOCK));
        oldSize = OldSize;
    }
    else
    {
        block = NULL;
        oldSize = 0;
    }

    NT_ASSERT(allocator->Used >= oldSize);

    held = (block &&
            allocator->Held &&
            ((ULONG_PTR)block >= (ULONG_PTR)allocator->Held) &&
            ((ULONG_PTR)block < ((ULONG_PTR)allocator->Held +
                                 allocator->HeldSize)));

    if (NewSize == 0)
    {
        if (block)
        {
            if (!held)
            {
                RemoveEntryList(&block->Link);
                KphFree(block, KPH_TAG_LUA_BLOCK);
            }

            allocator->Used -= oldSize;
        }

        return NULL;
    }

    //
    // Lua requires that shrinking a block never fails: it stays in place.
    //
    if (block && (NewSize <= oldSize))
    {
        allocator->Used -= (oldSize - NewSize);

        return Block;
    }

    used = allocator->Used - oldSize;

    status = RtlSIZETAdd(used, NewSize, &used);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlSIZETAdd failed: %!STATUS!",
                      status);

        return NULL;
    }

    //
    // Lua collects and asks again, and raises a memory error if that fails.
    //
    if (used > allocator->Limit)
    {
        return NULL;
    }

    status = RtlSIZETAdd(NewSize, sizeof(KPH_LUA_BLOCK), &size);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "RtlSIZETAdd failed: %!STATUS!",
                      status);

        return NULL;
    }

    //
    // A pool refusal is final until the reserve is released.
    //
    if ((allocator->Failures != 0) && (allocator->Reserve != 0))
    {
        return NULL;
    }

    newBlock = KphAllocatePaged(size, KPH_TAG_LUA_BLOCK);
    if (!newBlock)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Failed to allocate Lua block");

        allocator->Failures++;

        if (allocator->Reserve != 0)
        {
            return NULL;
        }

        size = ALIGN_UP_BY(size, MEMORY_ALLOCATION_ALIGNMENT);

        if ((allocator->HeldSize - allocator->HeldUsed) < size)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Lua reserve exhausted");

            return NULL;
        }

        newBlock = Add2Ptr(allocator->Held, allocator->HeldUsed);
        allocator->HeldUsed += size;
    }
    else
    {
        InsertTailList(&allocator->Blocks, &newBlock->Link);
    }

    memory = Add2Ptr(newBlock, sizeof(KPH_LUA_BLOCK));

    if (block)
    {
        RtlCopyMemory(memory, Block, oldSize);

        if (!held)
        {
            RemoveEntryList(&block->Link);
            KphFree(block, KPH_TAG_LUA_BLOCK);
        }
    }

    allocator->Used = used;

    return memory;
}

/**
 * \brief Runs a callout routine, catching the unprotected error of Lua.
 *
 * \param[in,out] Parameter The callout.
 */
_Function_class_(EXPAND_STACK_CALLOUT)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID NTAPI KphpLuaCallout(
    _Inout_opt_ PVOID Parameter
    )
{
    PKPH_LUA_CALLOUT callout;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    callout = Parameter;

    __try
    {
        callout->Routine(callout->Context);
        callout->Status = STATUS_SUCCESS;
    }
    __except (KphpLuaExceptionFilter(GetExceptionCode(),
                                     KPH_LUA_EXCEPTION_UNPROTECTED))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "Lua raised an error outside a protected call");

        callout->Status = STATUS_INTERNAL_ERROR;
    }
}

/**
 * \brief Runs a routine that uses Lua on an expanded stack.
 *
 * \details Everything done with a Lua state happens inside the routine. An
 * unprotected error of Lua ends the routine and fails the callout; the state
 * is then abandoned and its allocator deleted.
 *
 * \param[in] Routine The routine to run.
 * \param[in,out] Context The context for the routine.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaCallout(
    _In_ PKPH_LUA_CALLOUT_ROUTINE Routine,
    _Inout_opt_ PVOID Context
    )
{
    NTSTATUS status;
    KPH_LUA_CALLOUT callout;

    KPH_PAGED_CODE_PASSIVE();

    callout.Routine = Routine;
    callout.Context = Context;
    callout.Status = STATUS_UNSUCCESSFUL;

    status = KeExpandKernelStackAndCalloutEx(KphpLuaCallout,
                                             &callout,
                                             MAXIMUM_EXPANSION_SIZE,
                                             TRUE,
                                             NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "KeExpandKernelStackAndCalloutEx failed: %!STATUS!",
                      status);

        return status;
    }

    return callout.Status;
}

/**
 * \brief Creates a Lua state.
 *
 * \param[in,out] Allocator The allocator for the state, which must outlive it.
 * \param[out] State Receives the state, close with KphLuaCloseState.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaCreateState(
    _Inout_ PKPH_LUA_ALLOCATOR Allocator,
    _Outptr_ lua_State** State
    )
{
    lua_State* state;
    unsigned int seed;

    KPH_PAGED_CODE_PASSIVE();

    *State = NULL;

    if (!Allocator->Held && (Allocator->HeldSize != 0))
    {
        Allocator->Held = KphAllocatePaged(Allocator->HeldSize,
                                           KPH_TAG_LUA_RESERVE);
        if (!Allocator->Held)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          SCRIPT,
                          "Failed to allocate the Lua reserve");

            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    seed = KphLuaSeed();

    state = lua_newstate(KphpLuaAllocate, Allocator, seed);
    if (!state)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      SCRIPT,
                      "lua_newstate failed");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *State = state;

    return STATUS_SUCCESS;
}

/**
 * \brief Closes a Lua state.
 *
 * \param[in] State The state.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCloseState(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    lua_close(State);
}

/**
 * \brief Runs the routine of a protected call.
 *
 * \param[in] State The Lua state, with the protected call on its stack.
 *
 * \return Zero, the number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaProtectedCall(
    _In_ lua_State* State
    )
{
    PKPH_LUA_PROTECTED_CALL call;

    KPH_PAGED_CODE_PASSIVE();

    call = lua_touserdata(State, 1);

    NT_ASSERT(call);

    lua_settop(State, 0);

    call->Routine(State, call->Context);

    return 0;
}

/**
 * \brief Calls a routine so that a Lua error it raises ends it and is
 * returned as a status.
 *
 * \param[in] State The Lua state.
 * \param[in] Routine The routine to call.
 * \param[in,out] Context The context for the routine.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaProtectedCall(
    _In_ lua_State* State,
    _In_ PKPH_LUA_PROTECTED_CALL_ROUTINE Routine,
    _Inout_opt_ PVOID Context
    )
{
    NTSTATUS status;
    KPH_LUA_PROTECTED_CALL call;
    int result;
    PCSTR message;

    KPH_PAGED_CODE_PASSIVE();

    call.Routine = Routine;
    call.Context = Context;

    //
    // Neither push allocates, so neither can raise.
    //
    lua_pushcfunction(State, KphpLuaProtectedCall);
    lua_pushlightuserdata(State, &call);

    result = lua_pcall(State, 1, 0, 0);
    if (result == LUA_OK)
    {
        return STATUS_SUCCESS;
    }

    if (result == LUA_ERRMEM)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        status = STATUS_SI_SCRIPT_ERROR;
    }

    //
    // Reading an error that is a string does not allocate.
    //
    if (lua_type(State, -1) == LUA_TSTRING)
    {
        message = lua_tolstring(State, -1, NULL);
    }
    else
    {
        message = "(error is not a string)";
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  SCRIPT,
                  "Lua error %ld: %hs",
                  (LONG)result,
                  message);

    lua_settop(State, 0);

    return status;
}

/**
 * \brief Loads a precompiled chunk, leaving its function on the stack.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Chunk The chunk.
 * \param[in] Length The length of the chunk in bytes.
 * \param[in] Name The name of the chunk, for messages.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaLoadChunk(
    _In_ lua_State* State,
    _In_reads_bytes_(Length) PCVOID Chunk,
    _In_ SIZE_T Length,
    _In_ PCANSI_STRING Name
    )
{
    int result;
    PCSTR name;

    KPH_PAGED_CODE_PASSIVE();

    //
    // A name starting with '=' is shown as it is in messages.
    //
    lua_pushliteral(State, "=");
    lua_pushlstring(State, Name->Buffer, Name->Length);
    lua_concat(State, 2);

    name = lua_tolstring(State, -1, NULL);

    result = luaL_loadbufferx(State, Chunk, Length, name, "b");
    if (result != LUA_OK)
    {
        lua_error(State);
    }

    lua_remove(State, -2);
}
