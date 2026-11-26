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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>

#define KPH_LUA_EXCEPTION_ERROR 0xE04C5541ul

/**
 * \brief Converts text to a number that is not an integer: there are none.
 *
 * \param[in] String The text.
 * \param[out] End Receives where the conversion stopped: at the start.
 *
 * \return Zero.
 */
LUA_NUMBER KphLuaStringToNumber(
    _In_z_ const char* String,
    _Out_opt_ char** End
    )
{
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
int KphLuaFormat(
    _Out_writes_z_(Size) char* Buffer,
    _In_ size_t Size,
    _In_z_ _Printf_format_string_ const char* Format,
    ...
    )
{
    va_list args;
    int length;

    Buffer[0] = '\0';

    for (const char* c = Format; *c != '\0'; c++)
    {
        if (*c != '%')
        {
            continue;
        }

        c++;

        while ((*c != '\0') && !strchr("diouxXcspaAeEfFgG%", *c))
        {
            c++;
        }

        if (*c == '\0')
        {
            break;
        }

        if (strchr("aAeEfFgG", *c))
        {
            return 0;
        }
    }

    va_start(args, Format);
    length = vsnprintf(Buffer, Size, Format, args);
    va_end(args);

    if (length < 0)
    {
        Buffer[0] = '\0';

        return 0;
    }

    if ((size_t)length >= Size)
    {
        length = (int)(Size - 1);
    }

    return length;
}

/**
 * \brief Finds the first character of a string that is one of a set.
 *
 * \param[in] String The string.
 * \param[in] Accept The set.
 *
 * \return The character in the string, or NULL.
 */
_Ret_maybenull_
char* KphLuaStringBreak(
    _In_z_ const char* String,
    _In_z_ const char* Accept
    )
{
    return strpbrk(String, Accept);
}

/**
 * \brief Raises a Lua error to the nearest KphLuaTry. Does not return.
 *
 * \param[in] State Unused.
 * \param[in] Status Unused.
 */
void KphLuaThrow(
    _In_ struct lua_State* State,
    _In_ int Status
    )
{
    (void)State;
    (void)Status;

    RaiseException(KPH_LUA_EXCEPTION_ERROR, 0, 0, NULL);
}

/**
 * \brief Calls a routine and returns when it returns or when it raises a Lua
 * error.
 *
 * \param[in] State The Lua state.
 * \param[in] Jump Unused.
 * \param[in] Routine The routine.
 * \param[in] Context The context for the routine.
 */
void KphLuaTry(
    _In_ struct lua_State* State,
    _In_ struct lua_longjmp* Jump,
    _In_ KPH_LUA_PROTECTED_ROUTINE Routine,
    _In_opt_ void* Context
    )
{
    (void)Jump;

    __try
    {
        Routine(State, Context);
    }
    __except ((GetExceptionCode() == KPH_LUA_EXCEPTION_ERROR) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        ;
    }
}

/**
 * \brief Ends the tool: Lua raised an error outside every protected call.
 * Does not return.
 */
__declspec(noreturn)
void KphLuaUnprotectedError(
    void
    )
{
    fprintf(stderr, "kphluac: internal error (unprotected Lua error)\n");

    exit(2);
}
