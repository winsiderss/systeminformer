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

#define LUA_CORE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lua.h>

#include <ldebug.h>
#include <lgc.h>
#include <lobject.h>
#include <lopcodes.h>
#include <lstate.h>

#define KPHLUAC_ENVIRONMENT "_ENV"
#define KPHLUAC_REGISTERS 256
#define KPHLUAC_REQUIRE "require"
#define KPHLUAC_LIBRARY_ENTRY "nt"
#define KPHLUAC_MARKER "kphluac require "
#define KPHLUAC_STATUS_PREFIX "STATUS_"
#define KPHLUAC_STATUS_LINE "M." KPHLUAC_STATUS_PREFIX
#define KPHLUAC_STATUS_FORM " = status(0x"
#define KPHLUAC_STATUS_FUNCTION "local function status(value)"
#define KPHLUAC_STATUS_BODY "return (value ~ 0x80000000) - 0x80000000"
#define KPHLUAC_PENDING_TESTS 64

typedef enum _KPHLUAC_KIND
{
    KPHLUAC_KIND_SCRIPT,
    KPHLUAC_KIND_ENTRY,
    KPHLUAC_KIND_PART
} KPHLUAC_KIND;

typedef struct _REQUIRE
{
    char* Name;
    int Line;
} REQUIRE, *PREQUIRE;

typedef struct _STATUS_NAME
{
    char* Name;
    long long Value;
} STATUS_NAME, *PSTATUS_NAME;

typedef struct _STATUS_TABLE
{
    PSTATUS_NAME Names;
    int Count;
} STATUS_TABLE, *PSTATUS_TABLE;

typedef struct _BUILD_CONTEXT
{
    const char* Script;
    const char* Source;
    const char** Routines;
    int* RoutineLines;
    int RoutineCount;
    int Errors;
    KPHLUAC_KIND Kind;
    const wchar_t* LibraryFolder;
    PREQUIRE Requires;
    int RequireCount;
    int Fold;
    const STATUS_TABLE* Statuses;
    int LibraryUseLine;
} BUILD_CONTEXT, *PBUILD_CONTEXT;

typedef struct _STATUS_TEST
{
    const char* Name;
    const char* Prefix;
    const char* Suffix;
    const char* Function;
    const char* Body;
} STATUS_TEST, *PSTATUS_TEST;

typedef struct _PENDING_TEST
{
    int Depth;
    size_t Start;
    int Line;
    int Arguments;
    const char* Name;
    const char* Suffix;
} PENDING_TEST, *PPENDING_TEST;

typedef struct _TEXT_BUFFER
{
    char* Data;
    size_t Length;
    size_t Capacity;
} TEXT_BUFFER, *PTEXT_BUFFER;

static const char* KphluacOpeningWords[] = { "function", "do", "if", "repeat" };
static const char* KphluacClosingWords[] = { "end", "until" };

//
// The globals the driver leaves in a script's state.
//
static const char* KphluacDriverGlobals[] =
{
    "_G", "_VERSION", "assert", "error", "getmetatable", "ipairs", "ksi", "math", "next", "pairs", "pcall",
    "rawequal", "rawget", "rawlen", "rawset", "select", "setmetatable", "string", "table", "tonumber", "tostring",
    "type",
};
static const char KphluacPlaceholderStart[] =
    "(function(...) local _ = '" KPHLUAC_MARKER;
static const char KphluacPlaceholderEnd[] = "' end)()";

//
// Each is the body of the function of the same name in nt.lua.
//
static const STATUS_TEST KphluacStatusTests[] =
{
    { "NT_SUCCESS", "((", ") >= 0)", "function M.NT_SUCCESS(s)", "return s >= 0" },
    { "NT_INFORMATION", "((((", ") >> 30) & 3) == 1)", "function M.NT_INFORMATION(s)", "return (s >> 30) & 3 == 1" },
    { "NT_WARNING", "((((", ") >> 30) & 3) == 2)", "function M.NT_WARNING(s)", "return (s >> 30) & 3 == 2" },
    { "NT_ERROR", "((((", ") >> 30) & 3) == 3)", "function M.NT_ERROR(s)", "return (s >> 30) & 3 == 3" },
};

typedef struct _SOURCE_TEXT
{
    const char* Text;
    size_t Length;
} SOURCE_TEXT, *PSOURCE_TEXT;

typedef struct _REGISTER_STATE
{
    const char* Constant;
    const char* Field;
    unsigned char Environment;
    unsigned char Closure;
    unsigned char Library;
} REGISTER_STATE, *PREGISTER_STATE;

/**
 * \brief Reports an error in the script.
 *
 * \param[in,out] Context The build.
 * \param[in] Line The line of the source, zero when there is none.
 * \param[in] Format The format of the message.
 * \param[in] ... The arguments.
 */
static void Report(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ int Line,
    _In_z_ _Printf_format_string_ const char* Format,
    ...
    )
{
    va_list args;

    Context->Errors++;

    if (Line > 0)
    {
        fprintf(stderr, "%s(%d): error: ", Context->Source, Line);
    }
    else
    {
        fprintf(stderr, "%s: error: ", Context->Source);
    }

    va_start(args, Format);
    vfprintf(stderr, Format, args);
    va_end(args);

    fputc('\n', stderr);
}

/**
 * \brief Checks a script or routine name: ASCII letters, digits, '_' and '-'.
 *
 * \param[in] Name The name.
 *
 * \return Non-zero when the name is allowed.
 */
static int IsValidName(
    _In_z_ const char* Name
    )
{
    if (Name[0] == '\0')
    {
        return 0;
    }

    for (const char* c = Name; *c != '\0'; c++)
    {
        if ((*c >= 'a') && (*c <= 'z'))
        {
            continue;
        }

        if ((*c >= 'A') && (*c <= 'Z'))
        {
            continue;
        }

        if ((*c >= '0') && (*c <= '9'))
        {
            continue;
        }

        if ((*c == '_') || (*c == '-'))
        {
            continue;
        }

        return 0;
    }

    return 1;
}

/**
 * \brief Reads a whole file.
 *
 * \param[in] Path The path of the file.
 * \param[out] Length Receives the length of the file in bytes.
 *
 * \return The contents, to be freed with free, or NULL on failure.
 */
_Ret_maybenull_
static char* ReadSource(
    _In_z_ const wchar_t* Path,
    _Out_ size_t* Length
    )
{
    FILE* file;
    char* text;
    long size;

    *Length = 0;

    if (_wfopen_s(&file, Path, L"rb") != 0)
    {
        return NULL;
    }

    text = NULL;

    if (fseek(file, 0, SEEK_END) != 0)
    {
        goto Exit;
    }

    size = ftell(file);
    if (size < 0)
    {
        goto Exit;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        goto Exit;
    }

    text = malloc((size_t)size + 1);
    if (!text)
    {
        goto Exit;
    }

    if (fread(text, 1, (size_t)size, file) != (size_t)size)
    {
        free(text);
        text = NULL;
        goto Exit;
    }

    text[size] = '\0';
    *Length = (size_t)size;

Exit:

    fclose(file);

    return text;
}

/**
 * \brief Allocates memory for the Lua state of the compiler.
 *
 * \param[in] Context Unused.
 * \param[in] Block The block to resize or free, if any.
 * \param[in] OldSize Unused.
 * \param[in] NewSize The size wanted, zero to free the block.
 *
 * \return The block, or NULL.
 */
static void* Allocate(
    _In_opt_ void* Context,
    _In_opt_ void* Block,
    _In_ size_t OldSize,
    _In_ size_t NewSize
    )
{
    (void)Context;
    (void)OldSize;

    if (NewSize == 0)
    {
        free(Block);

        return NULL;
    }

    return realloc(Block, NewSize);
}

/**
 * \brief Hands the source to the Lua loader, in one piece.
 *
 * \param[in] State Unused.
 * \param[in,out] Context The source text, emptied by the call.
 * \param[out] Size Receives the length of the piece.
 *
 * \return The piece, or NULL at the end.
 */
static const char* ReadPiece(
    _In_ lua_State* State,
    _Inout_ void* Context,
    _Out_ size_t* Size
    )
{
    PSOURCE_TEXT source;
    const char* text;

    (void)State;

    source = Context;

    text = source->Text;
    *Size = source->Length;

    source->Text = NULL;
    source->Length = 0;

    if (*Size == 0)
    {
        return NULL;
    }

    return text;
}

/**
 * \brief Writes a piece of the chunk to the output file.
 *
 * \param[in] State Unused.
 * \param[in] Piece The piece.
 * \param[in] Size The length of the piece.
 * \param[in] Context The output file.
 *
 * \return Zero on success.
 */
static int WritePiece(
    _In_ lua_State* State,
    _In_reads_bytes_(Size) const void* Piece,
    _In_ size_t Size,
    _In_ void* Context
    )
{
    (void)State;

    if (Size == 0)
    {
        return 0;
    }

    if (fwrite(Piece, 1, Size, Context) != Size)
    {
        return 1;
    }

    return 0;
}

/**
 * \brief Reports the error the Lua compiler left on the stack.
 *
 * \param[in,out] Context The build.
 * \param[in] State The Lua state, with the message on top of the stack.
 */
static void ReportCompileError(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ lua_State* State
    )
{
    const char* message;
    size_t nameLength;
    char* end;
    long line;

    message = lua_tolstring(State, -1, NULL);
    if (!message)
    {
        Report(Context, 0, "the script does not compile");
        return;
    }

    //
    // Lua reports the nesting limit of its parser without a line.
    //
    if (strcmp(message, "C stack overflow") == 0)
    {
        Report(Context,
               0,
               "the script nests too deeply (expressions, blocks, or "
               "functions inside functions)");
        return;
    }

    nameLength = strlen(Context->Script);

    if ((strncmp(message, Context->Script, nameLength) == 0) &&
        (message[nameLength] == ':'))
    {
        line = strtol(message + nameLength + 1, &end, 10);

        if ((line > 0) && (end[0] == ':') && (end[1] == ' '))
        {
            Report(Context, (int)line, "%s", end + 2);
            return;
        }
    }

    Report(Context, 0, "%s", message);
}

/**
 * \brief Retrieves a constant of a function as text, if it is a string.
 *
 * \param[in] Function The function.
 * \param[in] Index The index of the constant.
 *
 * \return The text, or NULL when the constant is not a string.
 */
_Ret_maybenull_
static const char* GetStringConstant(
    _In_ const Proto* Function,
    _In_ int Index
    )
{
    const TValue* constant;

    if ((Index < 0) || (Index >= Function->sizek))
    {
        return NULL;
    }

    constant = &Function->k[Index];

    if (!ttisstring(constant))
    {
        return NULL;
    }

    return getstr(tsvalue(constant));
}

/**
 * \brief Refuses floating point in a function: the driver's Lua has none.
 *
 * \param[in,out] Context The build.
 * \param[in] Function The function.
 */
static void CheckFloat(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ const Proto* Function
    )
{
    Instruction instruction;
    int line;

    for (int pc = 0; pc < Function->sizecode; pc++)
    {
        instruction = Function->code[pc];
        line = luaG_getfuncline(Function, pc);

        switch (GET_OPCODE(instruction))
        {
            case OP_DIV:
            case OP_DIVK:
            {
                Report(Context,
                       line,
                       "'/' is floating point division, which a script does "
                       "not have; use '//'");
                break;
            }
            case OP_POW:
            case OP_POWK:
            {
                Report(Context,
                       line,
                       "'^' is floating point, which a script does not have");
                break;
            }
            case OP_LOADF:
            {
                Report(Context, line, "a floating point value");
                break;
            }
            default:
            {
                break;
            }
        }
    }

    for (int i = 0; i < Function->sizek; i++)
    {
        if (ttisfloat(&Function->k[i]))
        {
            Report(Context,
                   Function->linedefined,
                   "a floating point constant in the function that starts "
                   "here");
        }
    }
}

/**
 * \brief Checks if a format for string.format has a floating point conversion.
 *
 * \param[in] Format The format.
 *
 * \return Non-zero when it has one.
 */
static int HasFloatConversion(
    _In_z_ const char* Format
    )
{
    for (const char* c = Format; *c != '\0'; c++)
    {
        if (*c != '%')
        {
            continue;
        }

        c++;

        while ((*c != '\0') && !strchr("diouxXcspqaAeEfFgG%", *c))
        {
            c++;
        }

        if (*c == '\0')
        {
            return 0;
        }

        if (strchr("aAeEfFgG", *c))
        {
            return 1;
        }
    }

    return 0;
}

/**
 * \brief Finds a routine of the manifest entry by name.
 *
 * \param[in] Context The build.
 * \param[in] Name The name.
 *
 * \return The index of the routine, or -1.
 */
static int FindRoutine(
    _In_ PBUILD_CONTEXT Context,
    _In_z_ const char* Name
    )
{
    for (int i = 0; i < Context->RoutineCount; i++)
    {
        if (strcmp(Context->Routines[i], Name) == 0)
        {
            return i;
        }
    }

    return -1;
}

/**
 * \brief Checks if an upvalue of a function is the table of globals.
 *
 * \param[in] Function The function.
 * \param[in] Index The index of the upvalue.
 *
 * \return Non-zero when it is.
 */
static int IsEnvironment(
    _In_ const Proto* Function,
    _In_ int Index
    )
{
    const TString* name;

    if ((Index < 0) || (Index >= Function->sizeupvalues))
    {
        return 0;
    }

    name = Function->upvalues[Index].name;
    if (!name)
    {
        return 0;
    }

    return (strcmp(getstr(name), KPHLUAC_ENVIRONMENT) == 0);
}

/**
 * \brief Checks if an upvalue of a function is the local that holds nt.
 *
 * \param[in] Function The function.
 * \param[in] Index The index of the upvalue.
 *
 * \return Non-zero when it is.
 */
static int IsLibrary(
    _In_ const Proto* Function,
    _In_ int Index
    )
{
    const TString* name;

    if ((Index < 0) || (Index >= Function->sizeupvalues))
    {
        return 0;
    }

    name = Function->upvalues[Index].name;
    if (!name)
    {
        return 0;
    }

    return (strcmp(getstr(name), KPHLUAC_LIBRARY_ENTRY) == 0);
}

/**
 * \brief Forgets what is known of the registers from one on.
 *
 * \param[in,out] Registers What is known of each register.
 * \param[in] First The first register to forget.
 * \param[in] Count The number of registers to forget.
 */
static void ForgetRegisters(
    _Inout_updates_(KPHLUAC_REGISTERS) PREGISTER_STATE Registers,
    _In_ int First,
    _In_ int Count
    )
{
    for (int i = First; (i < (First + Count)) && (i < KPHLUAC_REGISTERS); i++)
    {
        Registers[i].Constant = NULL;
        Registers[i].Field = NULL;
        Registers[i].Environment = 0;
        Registers[i].Closure = 0;
        Registers[i].Library = 0;
    }
}

/**
 * \brief Follows one instruction: what it leaves in the registers it writes.
 *
 * \details The code is followed in the order it is written, not in the order
 * it may run.
 *
 * \param[in,out] Registers What is known of each register.
 * \param[in] Function The function.
 * \param[in] Pc The index of the instruction.
 */
static void FollowInstruction(
    _Inout_updates_(KPHLUAC_REGISTERS) PREGISTER_STATE Registers,
    _In_ const Proto* Function,
    _In_ int Pc
    )
{
    Instruction instruction;
    const char* text;
    int a;
    int source;

    instruction = Function->code[Pc];
    a = GETARG_A(instruction);

    switch (GET_OPCODE(instruction))
    {
        case OP_MOVE:
        {
            source = GETARG_B(instruction);
            Registers[a] = Registers[source];
            break;
        }
        case OP_LOADK:
        {
            text = GetStringConstant(Function, GETARG_Bx(instruction));
            ForgetRegisters(Registers, a, 1);
            Registers[a].Constant = text;
            break;
        }
        case OP_LOADKX:
        {
            text = NULL;

            if ((Pc + 1) < Function->sizecode)
            {
                text = GetStringConstant(Function,
                                         GETARG_Ax(Function->code[Pc + 1]));
            }

            ForgetRegisters(Registers, a, 1);
            Registers[a].Constant = text;
            break;
        }
        case OP_GETUPVAL:
        {
            ForgetRegisters(Registers, a, 1);

            if (IsEnvironment(Function, GETARG_B(instruction)))
            {
                Registers[a].Environment = 1;
            }
            else if (IsLibrary(Function, GETARG_B(instruction)))
            {
                Registers[a].Library = 1;
            }

            break;
        }
        case OP_GETFIELD:
        {
            text = GetStringConstant(Function, GETARG_C(instruction));
            ForgetRegisters(Registers, a, 1);
            Registers[a].Field = text;
            break;
        }
        case OP_GETTABLE:
        {
            source = GETARG_C(instruction);
            text = Registers[source].Constant;
            ForgetRegisters(Registers, a, 1);
            Registers[a].Field = text;
            break;
        }
        case OP_CLOSURE:
        {
            ForgetRegisters(Registers, a, 1);
            Registers[a].Closure = 1;
            break;
        }
        case OP_CALL:
        case OP_TAILCALL:
        case OP_VARARG:
        case OP_SELF:
        case OP_LOADNIL:
        case OP_FORPREP:
        case OP_FORLOOP:
        case OP_TFORPREP:
        case OP_TFORCALL:
        case OP_TFORLOOP:
        {
            //
            // These write more than one register, from A up.
            //
            ForgetRegisters(Registers, a, KPHLUAC_REGISTERS);
            break;
        }
        default:
        {
            if (testAMode(GET_OPCODE(instruction)))
            {
                ForgetRegisters(Registers, a, 1);
            }

            break;
        }
    }
}

/**
 * \brief Refuses floating point in a format a call passes as a literal to
 * string.pack, string.unpack, string.packsize or string.format.
 *
 * \details This sees a format only when it is written in the call. A format
 * that is computed, or passed through a variable, is not seen.
 *
 * \param[in,out] Context The build.
 * \param[in] Registers What is known of each register.
 * \param[in] Function The function.
 * \param[in] Pc The index of the call instruction.
 */
static void CheckCallFormat(
    _Inout_ PBUILD_CONTEXT Context,
    _In_reads_(KPHLUAC_REGISTERS) const REGISTER_STATE* Registers,
    _In_ const Proto* Function,
    _In_ int Pc
    )
{
    const char* name;
    const char* format;
    int function;
    int line;

    function = GETARG_A(Function->code[Pc]);

    if ((function + 1) >= KPHLUAC_REGISTERS)
    {
        return;
    }

    name = Registers[function].Field;
    format = Registers[function + 1].Constant;

    if (!name || !format)
    {
        return;
    }

    line = luaG_getfuncline(Function, Pc);

    if ((strcmp(name, "pack") == 0) ||
        (strcmp(name, "unpack") == 0) ||
        (strcmp(name, "packsize") == 0))
    {
        if (strpbrk(format, "fdn"))
        {
            Report(Context,
                   line,
                   "the pack format \"%s\" has a floating point item (f, d "
                   "or n)",
                   format);
        }
    }
    else if (strcmp(name, "format") == 0)
    {
        if (HasFloatConversion(format))
        {
            Report(Context,
                   line,
                   "the format \"%s\" has a floating point conversion",
                   format);
        }
    }
}

/**
 * \brief Checks an assignment to a global: the only globals of a script are
 * its routines, each defined once as a function at the top level.
 *
 * \param[in,out] Context The build.
 * \param[in] Function The function.
 * \param[in] Pc The index of the instruction that assigns.
 * \param[in] Name The name of the global.
 * \param[in] IsFunction Non-zero when the value assigned is a new function.
 * \param[in] IsMain Non-zero for the top level of the script.
 */
static void CheckGlobalStore(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ const Proto* Function,
    _In_ int Pc,
    _In_z_ const char* Name,
    _In_ int IsFunction,
    _In_ int IsMain
    )
{
    int routine;
    int line;

    line = luaG_getfuncline(Function, Pc);

    if (Context->Kind != KPHLUAC_KIND_SCRIPT)
    {
        Report(Context,
               line,
               "global '%s': a script library defines no globals (a missing "
               "'local'?)",
               Name);
        return;
    }

    if (!IsMain)
    {
        Report(Context,
               line,
               "global '%s' is assigned inside a function; the only globals "
               "of a script are its routines, defined at the top level (a "
               "missing 'local'?)",
               Name);
        return;
    }

    routine = FindRoutine(Context, Name);
    if (routine < 0)
    {
        Report(Context,
               line,
               "global '%s' is not a routine the manifest lists for script "
               "'%s' (a missing 'local'?)",
               Name,
               Context->Script);
        return;
    }

    if (Context->RoutineLines[routine] != 0)
    {
        Report(Context,
               line,
               "routine '%s' is already defined at line %d",
               Name,
               Context->RoutineLines[routine]);
        return;
    }

    Context->RoutineLines[routine] = line;

    if (!IsFunction)
    {
        Report(Context,
               line,
               "routine '%s' is not defined as a function",
               Name);
    }
}

/**
 * \brief Finds a global the driver leaves in a script's state.
 *
 * \param[in] Name The name.
 *
 * \return Non-zero when it is one.
 */
static int IsDriverGlobal(
    _In_z_ const char* Name
    )
{
    for (size_t i = 0; i < ARRAYSIZE(KphluacDriverGlobals); i++)
    {
        if (strcmp(KphluacDriverGlobals[i], Name) == 0)
        {
            return 1;
        }
    }

    return 0;
}

/**
 * \brief Checks the code of a function: the globals it assigns and the
 * formats it passes.
 *
 * \details A global is seen when it is assigned by name. One created through
 * a table, such as _G or a copy of _ENV, is not seen.
 *
 * \param[in,out] Context The build.
 * \param[in] Function The function.
 * \param[in] IsMain Non-zero for the top level of the script.
 */
static void CheckCode(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ const Proto* Function,
    _In_ int IsMain
    )
{
    REGISTER_STATE registers[KPHLUAC_REGISTERS];
    Instruction instruction;
    const char* name;
    int isFunction;
    int table;
    int key;
    int value;

    ForgetRegisters(registers, 0, KPHLUAC_REGISTERS);

    for (int pc = 0; pc < Function->sizecode; pc++)
    {
        instruction = Function->code[pc];

        //
        // The three ways an assignment to a global is compiled.
        //
        name = NULL;

        switch (GET_OPCODE(instruction))
        {
            case OP_SETTABUP:
            {
                if (IsEnvironment(Function, GETARG_A(instruction)))
                {
                    name = GetStringConstant(Function, GETARG_B(instruction));
                }

                break;
            }
            case OP_GETTABUP:
            {
                if (IsEnvironment(Function, GETARG_B(instruction)))
                {
                    name = GetStringConstant(Function, GETARG_C(instruction));

                    if (name &&
                        ((strcmp(name, "nt") == 0) ||
                         (!IsDriverGlobal(name) && (FindRoutine(Context, name) < 0))))
                    {
                        Report(Context,
                               luaG_getfuncline(Function, pc),
                               "global '%s' is read; a script reads only its "
                               "routines and the driver's globals (a missing "
                               "'local'?)",
                               name);
                    }

                    name = NULL;
                }

                break;
            }
            case OP_SETFIELD:
            {
                table = GETARG_A(instruction);

                if (registers[table].Environment)
                {
                    name = GetStringConstant(Function, GETARG_B(instruction));
                }

                break;
            }
            case OP_SETTABLE:
            {
                table = GETARG_A(instruction);
                key = GETARG_B(instruction);

                if (registers[table].Environment)
                {
                    name = registers[key].Constant;
                }

                break;
            }
            case OP_CALL:
            case OP_TAILCALL:
            {
                CheckCallFormat(Context, registers, Function, pc);
                break;
            }
            default:
            {
                break;
            }
        }

        if (name)
        {
            isFunction = 0;

            if (!TESTARG_k(instruction))
            {
                value = GETARG_C(instruction);
                isFunction = registers[value].Closure;
            }

            CheckGlobalStore(Context, Function, pc, name, isFunction, IsMain);
        }

        FollowInstruction(registers, Function, pc);
    }
}

/**
 * \brief Checks a function and the functions nested in it.
 *
 * \param[in,out] Context The build.
 * \param[in] Function The function.
 * \param[in] IsMain Non-zero for the top level of the script.
 */
static void CheckFunction(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ const Proto* Function,
    _In_ int IsMain
    )
{
    CheckFloat(Context, Function);
    CheckCode(Context, Function, IsMain);

    for (int i = 0; i < Function->sizep; i++)
    {
        CheckFunction(Context, Function->p[i], 0);
    }
}

/**
 * \brief Appends text to a growing buffer.
 *
 * \param[in,out] Buffer The buffer.
 * \param[in] Text The text.
 * \param[in] Length The length of the text.
 *
 * \return Non-zero on success.
 */
static int AppendText(
    _Inout_ PTEXT_BUFFER Buffer,
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length
    )
{
    char* data;
    size_t capacity;

    if ((Buffer->Length + Length + 1) > Buffer->Capacity)
    {
        capacity = (Buffer->Length + Length + 1) * 2;

        data = realloc(Buffer->Data, capacity);
        if (!data)
        {
            return 0;
        }

        Buffer->Data = data;
        Buffer->Capacity = capacity;
    }

    memcpy(Buffer->Data + Buffer->Length, Text, Length);
    Buffer->Length += Length;
    Buffer->Data[Buffer->Length] = '\0';

    return 1;
}

/**
 * \brief Checks if a character can start a Lua name.
 *
 * \param[in] Character The character.
 *
 * \return Non-zero when it can.
 */
static int IsNameStart(
    _In_ char Character
    )
{
    return (((Character >= 'a') && (Character <= 'z')) ||
            ((Character >= 'A') && (Character <= 'Z')) ||
            (Character == '_'));
}

/**
 * \brief Checks if a character can be part of a Lua name.
 *
 * \param[in] Character The character.
 *
 * \return Non-zero when it can.
 */
static int IsNameCharacter(
    _In_ char Character
    )
{
    return (IsNameStart(Character) ||
            ((Character >= '0') && (Character <= '9')));
}

/**
 * \brief Measures the opening long bracket at a position, if there is one.
 *
 * \param[in] Text The source text.
 * \param[in] Length The length of the source text.
 * \param[in] At The position of a '['.
 *
 * \return The level of the bracket plus one, or zero when there is none.
 */
static size_t LongBracketLevel(
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _In_ size_t At
    )
{
    size_t level;
    size_t i;

    level = 0;

    for (i = At + 1; (i < Length) && (Text[i] == '='); i++)
    {
        level++;
    }

    if ((i < Length) && (Text[i] == '['))
    {
        return level + 1;
    }

    return 0;
}

/**
 * \brief Skips a long string or comment.
 *
 * \param[in] Text The source text.
 * \param[in] Length The length of the source text.
 * \param[in] At The position of its opening bracket.
 * \param[in] Level The level of the bracket plus one.
 *
 * \return The position after its closing bracket, or the length.
 */
static size_t SkipLongBracket(
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _In_ size_t At,
    _In_ size_t Level
    )
{
    size_t count;
    size_t j;

    for (size_t i = At + Level + 1; i < Length; i++)
    {
        if (Text[i] != ']')
        {
            continue;
        }

        count = 0;

        for (j = i + 1; (j < Length) && (Text[j] == '='); j++)
        {
            count++;
        }

        if (((count + 1) == Level) && (j < Length) && (Text[j] == ']'))
        {
            return j + 1;
        }
    }

    return Length;
}

/**
 * \brief Counts the line of a position in the source text.
 *
 * \param[in] Text The source text.
 * \param[in] At The position.
 *
 * \return The line, from one.
 */
static int LineOf(
    _In_reads_bytes_(At) const char* Text,
    _In_ size_t At
    )
{
    int line;

    line = 1;

    for (size_t i = 0; i < At; i++)
    {
        if (Text[i] == '\n')
        {
            line++;
        }
    }

    return line;
}

/**
 * \brief Checks if a name is a word that opens or closes a block.
 *
 * \param[in] Name The name.
 * \param[in] Length The length of the name.
 *
 * \return One for a word that opens a block, minus one for one that closes
 * it, zero for any other name.
 */
static int BlockWord(
    _In_reads_bytes_(Length) const char* Name,
    _In_ size_t Length
    )
{
    for (int i = 0; i < (int)ARRAYSIZE(KphluacOpeningWords); i++)
    {
        if ((strlen(KphluacOpeningWords[i]) == Length) &&
            (memcmp(KphluacOpeningWords[i], Name, Length) == 0))
        {
            return 1;
        }
    }

    for (int i = 0; i < (int)ARRAYSIZE(KphluacClosingWords); i++)
    {
        if ((strlen(KphluacClosingWords[i]) == Length) &&
            (memcmp(KphluacClosingWords[i], Name, Length) == 0))
        {
            return -1;
        }
    }

    return 0;
}

/**
 * \brief Records a library a unit requires.
 *
 * \param[in,out] Context The build of the unit.
 * \param[in] Name The name of the library.
 * \param[in] Length The length of the name.
 * \param[in] Line The line of the require.
 *
 * \return Non-zero on success.
 */
static int AddRequire(
    _Inout_ PBUILD_CONTEXT Context,
    _In_reads_bytes_(Length) const char* Name,
    _In_ size_t Length,
    _In_ int Line
    )
{
    PREQUIRE requires;
    char* name;

    for (int i = 0; i < Context->RequireCount; i++)
    {
        if ((strlen(Context->Requires[i].Name) == Length) &&
            (memcmp(Context->Requires[i].Name, Name, Length) == 0))
        {
            Report(Context,
                   Line,
                   "'%s' is already required at line %d",
                   Context->Requires[i].Name,
                   Context->Requires[i].Line);
            return 1;
        }
    }

    requires = realloc(Context->Requires,
                       ((size_t)Context->RequireCount + 1) * sizeof(REQUIRE));
    if (!requires)
    {
        return 0;
    }

    Context->Requires = requires;

    name = malloc(Length + 1);
    if (!name)
    {
        return 0;
    }

    memcpy(name, Name, Length);
    name[Length] = '\0';

    Context->Requires[Context->RequireCount].Name = name;
    Context->Requires[Context->RequireCount].Line = Line;
    Context->RequireCount++;

    return 1;
}

/**
 * \brief Skips an escape in a short string: after \z the white space that
 * follows, line breaks included; after a backslash and a line break the whole
 * break, CR LF or LF CR as one.
 *
 * \param[in] Text The source text.
 * \param[in] Length The length of the source text.
 * \param[in] At The index of the backslash.
 *
 * \return The index after the escape.
 */
static size_t SkipEscape(
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _In_ size_t At
    )
{
    At++;

    if (At >= Length)
    {
        return At;
    }

    if (Text[At] == 'z')
    {
        At++;

        while ((At < Length) && isspace((unsigned char)Text[At]))
        {
            At++;
        }

        return At;
    }

    if ((Text[At] == '\r') || (Text[At] == '\n'))
    {
        if ((At + 1 < Length) &&
            ((Text[At + 1] == '\r') || (Text[At + 1] == '\n')) &&
            (Text[At + 1] != Text[At]))
        {
            return At + 2;
        }
    }

    return At + 1;
}

/**
 * \brief Rewrites each require of a unit, in place on its own line, as a call
 * of an empty function marked with the library's name, and records the
 * libraries required.
 *
 * \details A require is written require('name') in the main body, outside
 * every function and block. Strings and comments are skipped; a name after
 * '.' or ':' is a field, not the global.
 *
 * \param[in,out] Context The build of the unit.
 * \param[in] Text The source text.
 * \param[in] Length The length of the source text.
 * \param[in,out] Output Receives the rewritten text.
 *
 * \return Non-zero on success; errors in the text are reported, not failed.
 */
static int RewriteRequires(
    _Inout_ PBUILD_CONTEXT Context,
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _Inout_ PTEXT_BUFFER Output
    )
{
    size_t copied;
    size_t i;
    size_t level;
    size_t start;
    size_t nameStart;
    size_t nameEnd;
    char quote;
    char previous;
    char beforePrevious;
    int depth;
    int field;
    int line;

    copied = 0;
    i = 0;
    depth = 0;
    previous = 0;
    beforePrevious = 0;

    while (i < Length)
    {
        if ((Text[i] == '-') && ((i + 1) < Length) && (Text[i + 1] == '-'))
        {
            level = 0;

            if (((i + 2) < Length) && (Text[i + 2] == '['))
            {
                level = LongBracketLevel(Text, Length, i + 2);
            }

            if (level != 0)
            {
                i = SkipLongBracket(Text, Length, i + 2, level);
            }
            else
            {
                while ((i < Length) && (Text[i] != '\n'))
                {
                    i++;
                }
            }

            continue;
        }

        if ((Text[i] == '\'') || (Text[i] == '"'))
        {
            quote = Text[i];
            i++;

            while ((i < Length) && (Text[i] != quote) && (Text[i] != '\n'))
            {
                if (Text[i] == '\\')
                {
                    i = SkipEscape(Text, Length, i);
                    continue;
                }

                i++;
            }

            i++;
            beforePrevious = previous;
            previous = quote;
            continue;
        }

        if (Text[i] == '[')
        {
            level = LongBracketLevel(Text, Length, i);

            if (level != 0)
            {
                i = SkipLongBracket(Text, Length, i, level);
                beforePrevious = previous;
                previous = ']';
                continue;
            }
        }

        if ((Text[i] >= '0') && (Text[i] <= '9'))
        {
            while ((i < Length) &&
                   (IsNameCharacter(Text[i]) || (Text[i] == '.')))
            {
                i++;
            }

            beforePrevious = previous;
            previous = '0';
            continue;
        }

        if (!IsNameStart(Text[i]))
        {
            if ((Text[i] != ' ') && (Text[i] != '\t') && (Text[i] != '\r') &&
                (Text[i] != '\n'))
            {
                beforePrevious = previous;
                previous = Text[i];
            }

            i++;
            continue;
        }

        start = i;

        while ((i < Length) && IsNameCharacter(Text[i]))
        {
            i++;
        }

        field = (((previous == '.') && (beforePrevious != '.')) ||
                 (previous == ':'));
        beforePrevious = previous;
        previous = 'a';

        if (field)
        {
            continue;
        }

        //
        // A name followed by '=' is assigned to or a table key, not called.
        //
        nameEnd = i;

        while ((nameEnd < Length) &&
               ((Text[nameEnd] == ' ') || (Text[nameEnd] == '\t')))
        {
            nameEnd++;
        }

        if ((nameEnd < Length) && (Text[nameEnd] == '=') &&
            (((nameEnd + 1) >= Length) || (Text[nameEnd + 1] != '=')))
        {
            continue;
        }

        depth += BlockWord(Text + start, i - start);

        if (((i - start) != (sizeof(KPHLUAC_REQUIRE) - 1)) ||
            (memcmp(Text + start, KPHLUAC_REQUIRE, i - start) != 0))
        {
            continue;
        }

        line = LineOf(Text, start);

        nameStart = i + 2;
        nameEnd = nameStart;

        if (((i + 1) < Length) && (Text[i] == '(') &&
            ((Text[i + 1] == '\'') || (Text[i + 1] == '"')))
        {
            quote = Text[i + 1];

            while ((nameEnd < Length) && IsNameCharacter(Text[nameEnd]))
            {
                nameEnd++;
            }

            if ((nameEnd == nameStart) || ((nameEnd + 1) >= Length) ||
                (Text[nameEnd] != quote) || (Text[nameEnd + 1] != ')'))
            {
                nameEnd = nameStart;
            }
        }

        if (nameEnd == nameStart)
        {
            Report(Context,
                   line,
                   "require takes one name in quotes, as require('nt')");
            continue;
        }

        if (depth != 0)
        {
            Report(Context,
                   line,
                   "require is allowed only in the main body, outside every "
                   "function and block");
        }

        if (!AddRequire(Context, Text + nameStart, nameEnd - nameStart, line))
        {
            return 0;
        }

        if (!AppendText(Output, Text + copied, start - copied) ||
            !AppendText(Output,
                        KphluacPlaceholderStart,
                        sizeof(KphluacPlaceholderStart) - 1) ||
            !AppendText(Output, Text + nameStart, nameEnd - nameStart) ||
            !AppendText(Output,
                        KphluacPlaceholderEnd,
                        sizeof(KphluacPlaceholderEnd) - 1))
        {
            return 0;
        }

        i = nameEnd + 2;
        copied = i;
        beforePrevious = 'a';
        previous = ')';
    }

    return AppendText(Output, Text + copied, Length - copied);
}

/**
 * \brief Finds text in text.
 *
 * \param[in] Text The text to search.
 * \param[in] Length The length of the text to search.
 * \param[in] Needle The text to find.
 *
 * \return Non-zero when it is found.
 */
static int ContainsText(
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _In_z_ const char* Needle
    )
{
    size_t needle;

    needle = strlen(Needle);

    for (size_t i = 0; (i + needle) <= Length; i++)
    {
        if (memcmp(Text + i, Needle, needle) == 0)
        {
            return 1;
        }
    }

    return 0;
}

/**
 * \brief Finds a status of nt by name.
 *
 * \param[in] Table The statuses of nt, or NULL when there are none.
 * \param[in] Name The name, STATUS_ and the rest.
 * \param[in] Length The length of the name.
 *
 * \return The index of the status, or -1.
 */
static int FindStatus(
    _In_opt_ const STATUS_TABLE* Table,
    _In_reads_bytes_(Length) const char* Name,
    _In_ size_t Length
    )
{
    if (!Table)
    {
        return -1;
    }

    for (int i = 0; i < Table->Count; i++)
    {
        if ((strlen(Table->Names[i].Name) == Length) &&
            (memcmp(Table->Names[i].Name, Name, Length) == 0))
        {
            return i;
        }
    }

    return -1;
}

/**
 * \brief Reads one status line of nt.lua: M.STATUS_<name> = status(0x<8 hex
 * digits>), the value made signed as nt's status function does.
 *
 * \param[in,out] Context The build of nt.lua.
 * \param[in] Text The line after "M.".
 * \param[in] Length The length of the line.
 * \param[in] Line The number of the line.
 * \param[in,out] Table Receives the status.
 *
 * \return Non-zero on success; errors in the text are reported, not failed.
 */
static int AddStatus(
    _Inout_ PBUILD_CONTEXT Context,
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _In_ int Line,
    _Inout_ PSTATUS_TABLE Table
    )
{
    PSTATUS_NAME names;
    unsigned long long value;
    size_t nameLength;
    size_t at;
    char* name;
    char digit;

    nameLength = 0;

    while ((nameLength < Length) && IsNameCharacter(Text[nameLength]))
    {
        nameLength++;
    }

    at = nameLength + sizeof(KPHLUAC_STATUS_FORM) - 1;

    if ((Length != (at + 9)) ||
        (memcmp(Text + nameLength,
                KPHLUAC_STATUS_FORM,
                sizeof(KPHLUAC_STATUS_FORM) - 1) != 0) ||
        (Text[Length - 1] != ')'))
    {
        Report(Context,
               Line,
               "a status is written " KPHLUAC_STATUS_LINE "<name> = "
               "status(0x<8 hex digits>), the form kphluac folds");
        return 1;
    }

    value = 0;

    for (size_t i = at; i < (at + 8); i++)
    {
        digit = Text[i];

        if ((digit >= '0') && (digit <= '9'))
        {
            value = (value << 4) | (unsigned long long)(digit - '0');
        }
        else if ((digit >= 'A') && (digit <= 'F'))
        {
            value = (value << 4) | (unsigned long long)(digit - 'A' + 10);
        }
        else if ((digit >= 'a') && (digit <= 'f'))
        {
            value = (value << 4) | (unsigned long long)(digit - 'a' + 10);
        }
        else
        {
            Report(Context,
                   Line,
                   "a status is written " KPHLUAC_STATUS_LINE "<name> = "
                   "status(0x<8 hex digits>), the form kphluac folds");
            return 1;
        }
    }

    if (FindStatus(Table, Text, nameLength) >= 0)
    {
        Report(Context, Line, "'%.*s' is defined twice", (int)nameLength, Text);
        return 1;
    }

    names = realloc(Table->Names,
                    ((size_t)Table->Count + 1) * sizeof(STATUS_NAME));
    if (!names)
    {
        return 0;
    }

    Table->Names = names;

    name = malloc(nameLength + 1);
    if (!name)
    {
        return 0;
    }

    memcpy(name, Text, nameLength);
    name[nameLength] = '\0';

    Table->Names[Table->Count].Name = name;
    Table->Names[Table->Count].Value =
        (long long)(value ^ 0x80000000ull) - 0x80000000ll;
    Table->Count++;

    return 1;
}

/**
 * \brief Reads the statuses of nt.lua, the values kphluac folds.
 *
 * \param[in,out] Context The build of nt.lua.
 * \param[in] Text The source text of nt.lua.
 * \param[in] Length The length of the source text.
 * \param[in,out] Table Receives the statuses.
 *
 * \return Non-zero on success; errors in the text are reported, not failed.
 */
static int LoadStatuses(
    _Inout_ PBUILD_CONTEXT Context,
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _Inout_ PSTATUS_TABLE Table
    )
{
    size_t at;
    size_t end;
    size_t lineEnd;
    int line;

    at = 0;
    line = 1;

    while (at < Length)
    {
        end = at;

        while ((end < Length) && (Text[end] != '\n'))
        {
            end++;
        }

        lineEnd = end;

        if ((lineEnd > at) && (Text[lineEnd - 1] == '\r'))
        {
            lineEnd--;
        }

        if (((lineEnd - at) >= (sizeof(KPHLUAC_STATUS_LINE) - 1)) &&
            (memcmp(Text + at,
                    KPHLUAC_STATUS_LINE,
                    sizeof(KPHLUAC_STATUS_LINE) - 1) == 0))
        {
            if (!AddStatus(Context, Text + at + 2, lineEnd - at - 2, line, Table))
            {
                return 0;
            }
        }

        at = end + 1;
        line++;
    }

    if ((Table->Count != 0) &&
        (!ContainsText(Text, Length, KPHLUAC_STATUS_FUNCTION) ||
         !ContainsText(Text, Length, KPHLUAC_STATUS_BODY)))
    {
        Report(Context,
               0,
               "the status function is not the one kphluac folds: "
               KPHLUAC_STATUS_FUNCTION " " KPHLUAC_STATUS_BODY " end");
    }

    for (size_t i = 0; i < ARRAYSIZE(KphluacStatusTests); i++)
    {
        if (ContainsText(Text, Length, KphluacStatusTests[i].Function) &&
            !ContainsText(Text, Length, KphluacStatusTests[i].Body))
        {
            Report(Context,
                   0,
                   "%s is not the test kphluac folds: %s %s end",
                   KphluacStatusTests[i].Name,
                   KphluacStatusTests[i].Function,
                   KphluacStatusTests[i].Body);
        }
    }

    return 1;
}

/**
 * \brief Finds a test of nt that kphluac writes as its body.
 *
 * \param[in] Name The name.
 * \param[in] Length The length of the name.
 *
 * \return The test, or NULL.
 */
_Ret_maybenull_
static const STATUS_TEST* FindStatusTest(
    _In_reads_bytes_(Length) const char* Name,
    _In_ size_t Length
    )
{
    for (int i = 0; i < (int)ARRAYSIZE(KphluacStatusTests); i++)
    {
        if ((strlen(KphluacStatusTests[i].Name) == Length) &&
            (memcmp(KphluacStatusTests[i].Name, Name, Length) == 0))
        {
            return &KphluacStatusTests[i];
        }
    }

    return NULL;
}

/**
 * \brief Rewrites a unit's uses of nt's statuses and tests, in place on their
 * own lines: nt.STATUS_<name> as its value, nt.NT_SUCCESS(x) and the other
 * tests as their bodies.
 *
 * \details Strings and comments are skipped; a name after '.' or ':' is a
 * field, not the library. A test used as a value, not called, is left to the
 * function in nt.lua. Without folding the unit is only checked: the names, the
 * arguments, the first use of nt.
 *
 * \param[in,out] Context The build of the unit.
 * \param[in] Text The source text.
 * \param[in] Length The length of the source text.
 * \param[in,out] Output Receives the rewritten text.
 *
 * \return Non-zero on success; errors in the text are reported, not failed.
 */
static int RewriteLibraryUses(
    _Inout_ PBUILD_CONTEXT Context,
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _Inout_ PTEXT_BUFFER Output
    )
{
    PENDING_TEST pending[KPHLUAC_PENDING_TESTS];
    PPENDING_TEST top;
    const STATUS_TEST* test;
    char value[32];
    size_t copied;
    size_t i;
    size_t level;
    size_t start;
    size_t at;
    size_t nameStart;
    size_t nameEnd;
    char quote;
    char previous;
    char beforePrevious;
    int pendingCount;
    int depth;
    int field;
    int index;
    int empty;

    copied = 0;
    i = 0;
    depth = 0;
    pendingCount = 0;
    previous = 0;
    beforePrevious = 0;

    while (i < Length)
    {
        if ((Text[i] == '-') && ((i + 1) < Length) && (Text[i + 1] == '-'))
        {
            level = 0;

            if (((i + 2) < Length) && (Text[i + 2] == '['))
            {
                level = LongBracketLevel(Text, Length, i + 2);
            }

            if (level != 0)
            {
                i = SkipLongBracket(Text, Length, i + 2, level);
            }
            else
            {
                while ((i < Length) && (Text[i] != '\n'))
                {
                    i++;
                }
            }

            continue;
        }

        if ((Text[i] == '\'') || (Text[i] == '"'))
        {
            quote = Text[i];
            i++;

            while ((i < Length) && (Text[i] != quote) && (Text[i] != '\n'))
            {
                if (Text[i] == '\\')
                {
                    i = SkipEscape(Text, Length, i);
                    continue;
                }

                i++;
            }

            i++;
            beforePrevious = previous;
            previous = quote;
            continue;
        }

        if (Text[i] == '[')
        {
            level = LongBracketLevel(Text, Length, i);

            if (level != 0)
            {
                i = SkipLongBracket(Text, Length, i, level);
                beforePrevious = previous;
                previous = ']';
                continue;
            }
        }

        if ((Text[i] >= '0') && (Text[i] <= '9'))
        {
            while ((i < Length) &&
                   (IsNameCharacter(Text[i]) || (Text[i] == '.')))
            {
                i++;
            }

            beforePrevious = previous;
            previous = '0';
            continue;
        }

        if (!IsNameStart(Text[i]))
        {
            top = pendingCount ? &pending[pendingCount - 1] : NULL;

            if ((Text[i] == '(') || (Text[i] == '[') || (Text[i] == '{'))
            {
                depth++;
            }
            else if ((Text[i] == ')') || (Text[i] == ']') || (Text[i] == '}'))
            {
                if (top && (top->Depth == depth) && (Text[i] == ')'))
                {
                    empty = 1;

                    for (size_t j = top->Start; j < i; j++)
                    {
                        if ((Text[j] != ' ') && (Text[j] != '\t') &&
                            (Text[j] != '\r') && (Text[j] != '\n'))
                        {
                            empty = 0;
                            break;
                        }
                    }

                    if (empty)
                    {
                        Report(Context,
                               top->Line,
                               "nt.%s takes one argument",
                               top->Name);
                    }

                    if (Context->Fold)
                    {
                        if (!AppendText(Output, Text + copied, i - copied) ||
                            !AppendText(Output, top->Suffix, strlen(top->Suffix)))
                        {
                            return 0;
                        }

                        copied = i + 1;
                    }

                    pendingCount--;
                }

                depth--;
            }
            else if ((Text[i] == ',') && top && (top->Depth == depth))
            {
                if (top->Arguments++ == 0)
                {
                    Report(Context,
                           top->Line,
                           "nt.%s takes one argument",
                           top->Name);
                }
            }

            if ((Text[i] != ' ') && (Text[i] != '\t') && (Text[i] != '\r') &&
                (Text[i] != '\n'))
            {
                beforePrevious = previous;
                previous = Text[i];
            }

            i++;
            continue;
        }

        start = i;

        while ((i < Length) && IsNameCharacter(Text[i]))
        {
            i++;
        }

        field = (((previous == '.') && (beforePrevious != '.')) ||
                 (previous == ':'));
        beforePrevious = previous;
        previous = 'a';

        if (field ||
            ((i - start) != (sizeof(KPHLUAC_LIBRARY_ENTRY) - 1)) ||
            (memcmp(Text + start, KPHLUAC_LIBRARY_ENTRY, i - start) != 0))
        {
            continue;
        }

        if (Context->LibraryUseLine == 0)
        {
            Context->LibraryUseLine = LineOf(Text, start);
        }

        //
        // nt.<name>, with nothing but spaces around the dot.
        //
        at = i;

        while ((at < Length) && ((Text[at] == ' ') || (Text[at] == '\t')))
        {
            at++;
        }

        if ((at >= Length) || (Text[at] != '.') ||
            (((at + 1) < Length) && (Text[at + 1] == '.')))
        {
            continue;
        }

        at++;

        while ((at < Length) && ((Text[at] == ' ') || (Text[at] == '\t')))
        {
            at++;
        }

        if ((at >= Length) || !IsNameStart(Text[at]))
        {
            continue;
        }

        nameStart = at;

        while ((at < Length) && IsNameCharacter(Text[at]))
        {
            at++;
        }

        nameEnd = at;
        i = nameEnd;

        if (((nameEnd - nameStart) > (sizeof(KPHLUAC_STATUS_PREFIX) - 1)) &&
            (memcmp(Text + nameStart,
                    KPHLUAC_STATUS_PREFIX,
                    sizeof(KPHLUAC_STATUS_PREFIX) - 1) == 0))
        {
            //
            // An assignment is left for the check of the code to refuse.
            //
            while ((at < Length) && ((Text[at] == ' ') || (Text[at] == '\t')))
            {
                at++;
            }

            if ((at < Length) && (Text[at] == '=') &&
                (((at + 1) >= Length) || (Text[at + 1] != '=')))
            {
                continue;
            }

            index = FindStatus(Context->Statuses,
                               Text + nameStart,
                               nameEnd - nameStart);
            if (index < 0)
            {
                Report(Context,
                       LineOf(Text, start),
                       "'nt.%.*s' is not a status nt.lua defines",
                       (int)(nameEnd - nameStart),
                       Text + nameStart);
                continue;
            }

            if (Context->Fold)
            {
                if ((sprintf_s(value,
                               sizeof(value),
                               "(%lld)",
                               Context->Statuses->Names[index].Value) < 0) ||
                    !AppendText(Output, Text + copied, start - copied) ||
                    !AppendText(Output, value, strlen(value)))
                {
                    return 0;
                }

                copied = nameEnd;
            }

            beforePrevious = 'a';
            previous = ')';
            continue;
        }

        test = FindStatusTest(Text + nameStart, nameEnd - nameStart);
        if (!test)
        {
            continue;
        }

        while ((at < Length) && ((Text[at] == ' ') || (Text[at] == '\t')))
        {
            at++;
        }

        if ((at >= Length) || (Text[at] != '('))
        {
            continue;
        }

        if (pendingCount == KPHLUAC_PENDING_TESTS)
        {
            Report(Context,
                   LineOf(Text, start),
                   "nt.%s is nested too deeply",
                   test->Name);
            return 1;
        }

        depth++;

        top = &pending[pendingCount++];
        top->Depth = depth;
        top->Start = at + 1;
        top->Line = LineOf(Text, start);
        top->Arguments = 0;
        top->Name = test->Name;
        top->Suffix = test->Suffix;

        if (Context->Fold)
        {
            if (!AppendText(Output, Text + copied, start - copied) ||
                !AppendText(Output, test->Prefix, strlen(test->Prefix)))
            {
                return 0;
            }

            copied = at + 1;
        }

        i = at + 1;
        beforePrevious = 'a';
        previous = '(';
    }

    if (pendingCount != 0)
    {
        Report(Context,
               pending[0].Line,
               "nt.%s has no closing parenthesis",
               pending[0].Name);
    }

    return AppendText(Output, Text + copied, Length - copied);
}

/**
 * \brief Finds the locals named nt in a function and the functions nested in
 * it.
 *
 * \param[in] Function The function.
 * \param[in,out] Found Receives the first two functions that have one.
 * \param[in,out] FoundIndex Receives the index of each one's local.
 * \param[in,out] Count The number found so far, all of them counted.
 */
static void FindLibraryLocals(
    _In_ const Proto* Function,
    _Inout_updates_(2) const Proto** Found,
    _Inout_updates_(2) int* FoundIndex,
    _Inout_ int* Count
    )
{
    const TString* name;

    for (int i = 0; i < Function->sizelocvars; i++)
    {
        name = Function->locvars[i].varname;

        if (!name || (strcmp(getstr(name), KPHLUAC_LIBRARY_ENTRY) != 0))
        {
            continue;
        }

        if (*Count < 2)
        {
            Found[*Count] = Function;
            FoundIndex[*Count] = i;
        }

        (*Count)++;
    }

    for (int i = 0; i < Function->sizep; i++)
    {
        FindLibraryLocals(Function->p[i], Found, FoundIndex, Count);
    }
}

/**
 * \brief Gives the line of an instruction, or of the function's start when the
 * index is outside its code.
 *
 * \param[in] Function The function.
 * \param[in] Pc The index of the instruction.
 *
 * \return The line.
 */
static int LineOfCode(
    _In_ const Proto* Function,
    _In_ int Pc
    )
{
    if ((Pc < 0) || (Pc >= Function->sizecode))
    {
        return Function->linedefined;
    }

    return luaG_getfuncline(Function, Pc);
}

/**
 * \brief Refuses assignments to nt and to its fields: the folded statuses and
 * tests are nt's only while nothing changes it.
 *
 * \details A copy of nt in another local is followed in the order the code is
 * written; a copy held in a table is not.
 *
 * \param[in,out] Context The build.
 * \param[in] Function The function.
 * \param[in] Holder The function whose local is nt.
 * \param[in] Register The register of that local.
 * \param[in] StartPc The first instruction where the local is live.
 * \param[in] EndPc The instruction after the last where it is live.
 */
static void CheckLibraryWrites(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ const Proto* Function,
    _In_ const Proto* Holder,
    _In_ int Register,
    _In_ int StartPc,
    _In_ int EndPc
    )
{
    REGISTER_STATE registers[KPHLUAC_REGISTERS];
    Instruction instruction;
    OpCode opcode;
    int live;
    int a;

    ForgetRegisters(registers, 0, KPHLUAC_REGISTERS);

    for (int pc = 0; pc < Function->sizecode; pc++)
    {
        instruction = Function->code[pc];
        opcode = GET_OPCODE(instruction);
        a = GETARG_A(instruction);
        live = ((Function == Holder) && (pc >= StartPc) && (pc < EndPc));

        if (live)
        {
            registers[Register].Library = 1;
        }

        switch (opcode)
        {
            case OP_SETFIELD:
            case OP_SETTABLE:
            case OP_SETI:
            {
                if (registers[a].Library)
                {
                    Report(Context,
                           LineOfCode(Function, pc),
                           "a field of nt is assigned; nt is not changed");
                }

                break;
            }
            case OP_SETTABUP:
            {
                if (IsLibrary(Function, a))
                {
                    Report(Context,
                           LineOfCode(Function, pc),
                           "a field of nt is assigned; nt is not changed");
                }

                break;
            }
            case OP_SETUPVAL:
            {
                if (IsLibrary(Function, GETARG_B(instruction)))
                {
                    Report(Context,
                           LineOfCode(Function, pc),
                           "nt is assigned; nt is not changed");
                }

                break;
            }
            default:
            {
                if (live && testAMode(opcode) && (a == Register))
                {
                    Report(Context,
                           LineOfCode(Function, pc),
                           "nt is assigned; nt is not changed");
                }

                break;
            }
        }

        FollowInstruction(registers, Function, pc);
    }

    for (int i = 0; i < Function->sizep; i++)
    {
        CheckLibraryWrites(Context,
                           Function->p[i],
                           Holder,
                           Register,
                           StartPc,
                           EndPc);
    }
}

/**
 * \brief Checks that nt in a unit is the library: one local of that name, a
 * script's set by its require in the main body, a part's the parameter of the
 * function it returns; never assigned, nor its fields.
 *
 * \param[in,out] Context The build of the unit.
 * \param[in] Main The main function of the unit.
 */
static void CheckLibrary(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ const Proto* Main
    )
{
    const Proto* found[2];
    const LocVar* local;
    int foundIndex[2];
    int count;
    int line;
    int requireLine;
    int registerIndex;

    count = 0;

    FindLibraryLocals(Main, found, foundIndex, &count);

    if (count == 0)
    {
        if (Context->LibraryUseLine != 0)
        {
            Report(Context,
                   Context->LibraryUseLine,
                   "'nt' is not the library here: a script writes local nt = "
                   "require('nt'), a part returns function(nt)");
        }

        return;
    }

    if (count > 1)
    {
        local = &found[1]->locvars[foundIndex[1]];

        if (foundIndex[1] < found[1]->numparams)
        {
            line = found[1]->linedefined;
        }
        else
        {
            line = LineOfCode(found[1], local->startpc - 1);
        }

        Report(Context,
               line,
               "a second local named 'nt'; the name is the library's");
        return;
    }

    local = &found[0]->locvars[foundIndex[0]];
    line = (foundIndex[0] < found[0]->numparams) ?
        found[0]->linedefined :
        LineOfCode(found[0], local->startpc - 1);

    if (Context->Kind == KPHLUAC_KIND_SCRIPT)
    {
        requireLine = 0;

        for (int i = 0; i < Context->RequireCount; i++)
        {
            if (strcmp(Context->Requires[i].Name, KPHLUAC_LIBRARY_ENTRY) == 0)
            {
                requireLine = Context->Requires[i].Line;
            }
        }

        if ((found[0] != Main) || (requireLine != line))
        {
            Report(Context,
                   line,
                   "'nt' is not the library here: a script writes local nt = "
                   "require('nt') in its main body");
            return;
        }
    }
    else if ((found[0] == Main) || (foundIndex[0] >= found[0]->numparams))
    {
        Report(Context,
               line,
               "'nt' is not the library here: a part returns function(nt)");
        return;
    }

    //
    // A local's register is the number of locals live where it starts that
    // were declared before it.
    //
    registerIndex = 0;

    for (int i = 0; i < foundIndex[0]; i++)
    {
        if ((found[0]->locvars[i].startpc <= local->startpc) &&
            (found[0]->locvars[i].endpc > local->startpc))
        {
            registerIndex++;
        }
    }

    CheckLibraryWrites(Context,
                       Main,
                       found[0],
                       registerIndex,
                       local->startpc,
                       local->endpc);
}

/**
 * \brief Finds the placeholder of a require among the functions of a unit's
 * main body.
 *
 * \param[in] Function The main function of the unit.
 * \param[in] Name The name of the library.
 *
 * \return The index of the placeholder, or -1.
 */
static int FindPlaceholder(
    _In_ const Proto* Function,
    _In_z_ const char* Name
    )
{
    const char* text;
    size_t prefix;

    prefix = sizeof(KPHLUAC_MARKER) - 1;

    for (int i = 0; i < Function->sizep; i++)
    {
        for (int k = 0; k < Function->p[i]->sizek; k++)
        {
            text = GetStringConstant(Function->p[i], k);

            if (text &&
                (strncmp(text, KPHLUAC_MARKER, prefix) == 0) &&
                (strcmp(text + prefix, Name) == 0))
            {
                return i;
            }
        }
    }

    return -1;
}

/**
 * \brief Checks that the main body of a script library ends by returning one
 * value.
 *
 * \param[in,out] Context The build of the library.
 * \param[in] Function The main function of the library.
 */
static void CheckLibraryReturn(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ const Proto* Function
    )
{
    Instruction instruction;

    if (Function->sizecode >= 2)
    {
        instruction = Function->code[Function->sizecode - 2];

        if ((GET_OPCODE(instruction) == OP_RETURN1) ||
            ((GET_OPCODE(instruction) == OP_RETURN) &&
             (GETARG_B(instruction) == 2)))
        {
            return;
        }
    }

    Report(Context,
           luaG_getfuncline(Function, Function->sizecode - 1),
           "a script library ends by returning one value (its table, or for "
           "a part the function that makes it)");
}

/**
 * \brief Compiles and checks a unit (a script, nt or a part) and the libraries
 * it requires, and builds each library into it in place of its require.
 *
 * \details The unit's compiled main function is left on the stack. A library
 * is compiled from its own file under its own name, so that a Lua error in
 * it reports its own name and line. Its main function takes the place of the
 * require's placeholder, and its one upvalue, the table of globals, comes
 * from the unit's main function as a nested function's does.
 *
 * \param[in,out] Context The build of the unit.
 * \param[in] State The Lua state of the build.
 * \param[in] Text The source text.
 * \param[in] Length The length of the source text.
 *
 * \return The main function of the unit, or NULL.
 */
_Ret_maybenull_
static Proto* CompileUnit(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ lua_State* State,
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length
    )
{
    Proto* function;
    Proto* library;
    BUILD_CONTEXT child;
    TEXT_BUFFER rewritten;
    TEXT_BUFFER folded;
    SOURCE_TEXT source;
    char chunkName[LUA_IDSIZE];
    wchar_t path[MAX_PATH];
    char display[MAX_PATH * 3];
    char* text;
    const char* start;
    size_t length;
    int index;

    function = NULL;
    memset(&rewritten, 0, sizeof(rewritten));

    if (!RewriteRequires(Context, Text, Length, &rewritten))
    {
        fprintf(stderr, "kphluac: out of memory\n");
        Context->Errors++;
        goto Exit;
    }

    //
    // nt itself is the definition of what is folded.
    //
    if (Context->Kind != KPHLUAC_KIND_ENTRY)
    {
        const char* rewrittenText = "";

        if (rewritten.Data)
        {
            rewrittenText = rewritten.Data;
        }

        memset(&folded, 0, sizeof(folded));

        if (!RewriteLibraryUses(Context,
                                rewrittenText,
                                rewritten.Length,
                                &folded))
        {
            free(folded.Data);
            fprintf(stderr, "kphluac: out of memory\n");
            Context->Errors++;
            goto Exit;
        }

        if (Context->Fold)
        {
            free(rewritten.Data);
            rewritten = folded;
        }
        else
        {
            free(folded.Data);
        }
    }

    for (int i = 0; i < Context->RequireCount; i++)
    {
        if ((Context->Kind == KPHLUAC_KIND_SCRIPT) &&
            (strcmp(Context->Requires[i].Name, KPHLUAC_LIBRARY_ENTRY) != 0))
        {
            Report(Context,
                   Context->Requires[i].Line,
                   "a script requires only '" KPHLUAC_LIBRARY_ENTRY "'; "
                   "'%s' is not it (a part is reached as "
                   KPHLUAC_LIBRARY_ENTRY ".%s)",
                   Context->Requires[i].Name,
                   Context->Requires[i].Name);
        }
        else if ((Context->Kind == KPHLUAC_KIND_ENTRY) &&
                 (strcmp(Context->Requires[i].Name,
                         KPHLUAC_LIBRARY_ENTRY) == 0))
        {
            Report(Context,
                   Context->Requires[i].Line,
                   "'" KPHLUAC_LIBRARY_ENTRY "' requires only its parts");
        }
        else if (Context->Kind == KPHLUAC_KIND_PART)
        {
            Report(Context,
                   Context->Requires[i].Line,
                   "a part requires nothing; '" KPHLUAC_LIBRARY_ENTRY "' gives "
                   "it what it needs");
        }
    }

    if (Context->Errors != 0)
    {
        goto Exit;
    }

    //
    // A chunk named "=<name>" is shown as "<name>" in messages, in the build
    // and in the driver.
    //
    if (strlen(Context->Script) > (sizeof(chunkName) - 2))
    {
        Report(Context, 0, "the name is too long");
        goto Exit;
    }

    chunkName[0] = '=';
    strcpy_s(chunkName + 1, sizeof(chunkName) - 1, Context->Script);

    source.Text = rewritten.Data ? rewritten.Data : "";
    source.Length = rewritten.Length;

    if (lua_load(State, ReadPiece, &source, chunkName, "t") != LUA_OK)
    {
        ReportCompileError(Context, State);
        goto Exit;
    }

    function = clLvalue(s2v(State->top.p - 1))->p;

    CheckFunction(Context, function, 1);

    for (int i = 0; i < Context->RoutineCount; i++)
    {
        if (Context->RoutineLines[i] == 0)
        {
            Report(Context,
                   0,
                   "routine '%s' is not defined; the manifest lists it for "
                   "script '%s'",
                   Context->Routines[i],
                   Context->Script);
        }
    }

    if (Context->Kind != KPHLUAC_KIND_SCRIPT)
    {
        CheckLibraryReturn(Context, function);
    }

    if (Context->Kind != KPHLUAC_KIND_ENTRY)
    {
        CheckLibrary(Context, function);
    }

    for (int i = 0; i < Context->RequireCount; i++)
    {
        memset(&child, 0, sizeof(child));
        child.Script = Context->Requires[i].Name;
        child.LibraryFolder = Context->LibraryFolder;
        child.Fold = Context->Fold;
        child.Statuses = Context->Statuses;
        child.Kind = (Context->Kind == KPHLUAC_KIND_SCRIPT) ?
            KPHLUAC_KIND_ENTRY : KPHLUAC_KIND_PART;

        if ((swprintf_s(path,
                        ARRAYSIZE(path),
                        L"%ls\\%hs.lua",
                        Context->LibraryFolder,
                        child.Script) < 0) ||
            (WideCharToMultiByte(CP_UTF8,
                                 0,
                                 path,
                                 -1,
                                 display,
                                 sizeof(display),
                                 NULL,
                                 NULL) <= 0))
        {
            Report(Context, Context->Requires[i].Line, "the path is too long");
            continue;
        }

        child.Source = display;

        text = ReadSource(path, &length);
        if (!text)
        {
            Report(Context,
                   Context->Requires[i].Line,
                   "no script library '%s': %s cannot be read",
                   child.Script,
                   display);
            continue;
        }

        start = text;

        if ((length >= 3) &&
            ((unsigned char)text[0] == 0xEF) &&
            ((unsigned char)text[1] == 0xBB) &&
            ((unsigned char)text[2] == 0xBF))
        {
            start += 3;
            length -= 3;
        }

        library = CompileUnit(&child, State, start, length);

        free(text);

        for (int r = 0; r < child.RequireCount; r++)
        {
            free(child.Requires[r].Name);
        }

        free(child.Requires);

        Context->Errors += child.Errors;

        if (!library)
        {
            continue;
        }

        index = FindPlaceholder(function, Context->Requires[i].Name);

        if ((index < 0) ||
            (library->sizeupvalues != 1) ||
            !IsEnvironment(function, 0))
        {
            Report(Context,
                   Context->Requires[i].Line,
                   "the require of '%s' cannot be built in",
                   Context->Requires[i].Name);
            continue;
        }

        library->upvalues[0].instack = 0;
        library->upvalues[0].idx = 0;
        function->p[index] = library;
        luaC_objbarrier(State, function, library);
    }

Exit:

    free(rewritten.Data);

    if (Context->Errors != 0)
    {
        return NULL;
    }

    return function;
}

/**
 * \brief Compiles and checks the script and writes its chunk.
 *
 * \param[in,out] Context The build.
 * \param[in] Text The source text.
 * \param[in] Length The length of the source text.
 * \param[in] Output The path of the chunk to write.
 *
 * \return The exit code.
 */
static int Build(
    _Inout_ PBUILD_CONTEXT Context,
    _In_reads_bytes_(Length) const char* Text,
    _In_ size_t Length,
    _In_z_ const wchar_t* Output
    )
{
    int result;
    lua_State* state;
    FILE* file;
    int failed;

    result = 2;
    file = NULL;

    state = lua_newstate(Allocate, NULL, 0);
    if (!state)
    {
        fprintf(stderr, "kphluac: out of memory\n");
        return 2;
    }

    if (!CompileUnit(Context, state, Text, Length))
    {
        result = 1;
        goto Exit;
    }

    if (Context->Errors != 0)
    {
        result = 1;
        goto Exit;
    }

    if (_wfopen_s(&file, Output, L"wb") != 0)
    {
        fprintf(stderr, "kphluac: cannot write %ls\n", Output);
        file = NULL;
        goto Exit;
    }

    //
    // The chunk keeps its line information for the trace of the driver. The
    // libraries built in leave other functions on the stack: the script's
    // main function is the first.
    //
    lua_settop(state, 1);
    failed = lua_dump(state, WritePiece, file, 0);

    if (fclose(file) != 0)
    {
        failed = 1;
    }

    file = NULL;

    if (failed)
    {
        fprintf(stderr, "kphluac: cannot write %ls\n", Output);
        _wremove(Output);
        goto Exit;
    }

    result = 0;

Exit:

    lua_close(state);

    return result;
}

/**
 * \brief Converts an argument to UTF-8.
 *
 * \param[in] Text The argument.
 *
 * \return The text, never freed, or NULL on failure.
 */
_Ret_maybenull_
static char* ToUtf8(
    _In_z_ const wchar_t* Text
    )
{
    char* buffer;
    int length;

    length = WideCharToMultiByte(CP_UTF8, 0, Text, -1, NULL, 0, NULL, NULL);
    if (length <= 0)
    {
        return NULL;
    }

    buffer = malloc((size_t)length);
    if (!buffer)
    {
        return NULL;
    }

    if (WideCharToMultiByte(CP_UTF8,
                            0,
                            Text,
                            -1,
                            buffer,
                            length,
                            NULL,
                            NULL) <= 0)
    {
        free(buffer);

        return NULL;
    }

    return buffer;
}

/**
 * \brief Reads the statuses of nt.lua in the library folder; a folder without
 * nt.lua has none.
 *
 * \param[in,out] Context The build of the script.
 * \param[in] LibraryFolder The library folder.
 * \param[in,out] Table Receives the statuses.
 *
 * \return Non-zero on success; errors in the text are reported, not failed.
 */
static int ReadStatuses(
    _Inout_ PBUILD_CONTEXT Context,
    _In_z_ const wchar_t* LibraryFolder,
    _Inout_ PSTATUS_TABLE Table
    )
{
    BUILD_CONTEXT library;
    wchar_t path[MAX_PATH];
    char display[MAX_PATH * 3];
    char* text;
    const char* start;
    size_t length;
    int result;

    if ((swprintf_s(path,
                    ARRAYSIZE(path),
                    L"%ls\\%hs.lua",
                    LibraryFolder,
                    KPHLUAC_LIBRARY_ENTRY) < 0) ||
        (WideCharToMultiByte(CP_UTF8,
                             0,
                             path,
                             -1,
                             display,
                             sizeof(display),
                             NULL,
                             NULL) <= 0))
    {
        return 1;
    }

    text = ReadSource(path, &length);
    if (!text)
    {
        return 1;
    }

    start = text;

    if ((length >= 3) &&
        ((unsigned char)text[0] == 0xEF) &&
        ((unsigned char)text[1] == 0xBB) &&
        ((unsigned char)text[2] == 0xBF))
    {
        start += 3;
        length -= 3;
    }

    memset(&library, 0, sizeof(library));
    library.Script = KPHLUAC_LIBRARY_ENTRY;
    library.Source = display;

    result = LoadStatuses(&library, start, length, Table);

    Context->Errors += library.Errors;

    free(text);

    return result;
}

int __cdecl main(
    void
    )
{
    BUILD_CONTEXT context;
    STATUS_TABLE statuses;
    wchar_t** arguments;
    const wchar_t* sourcePath;
    const wchar_t* outputPath;
    wchar_t* libraryFolder;
    wchar_t* slash;
    size_t folderLength;
    int count;
    char* text;
    const char* start;
    size_t length;
    int result;

    memset(&context, 0, sizeof(context));
    memset(&statuses, 0, sizeof(statuses));
    sourcePath = NULL;
    outputPath = NULL;
    libraryFolder = NULL;
    context.Fold = 1;

    //
    // The tools of this tree link without the C runtime's argument processing.
    //
    arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!arguments)
    {
        fprintf(stderr, "kphluac: cannot read the command line\n");
        return 2;
    }

    if ((count == 2) && (wcscmp(arguments[1], L"-v") == 0))
    {
        puts(LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE);
        return 0;
    }

    context.Routines = calloc((size_t)count + 1, sizeof(const char*));
    context.RoutineLines = calloc((size_t)count + 1, sizeof(int));
    if (!context.Routines || !context.RoutineLines)
    {
        fprintf(stderr, "kphluac: out of memory\n");
        return 2;
    }

    for (int i = 1; i < count; i++)
    {
        if ((i + 1) >= count)
        {
            context.Script = NULL;
            break;
        }

        if (wcscmp(arguments[i], L"-n") == 0)
        {
            context.Script = ToUtf8(arguments[++i]);
        }
        else if (wcscmp(arguments[i], L"-i") == 0)
        {
            sourcePath = arguments[++i];
            context.Source = ToUtf8(sourcePath);
        }
        else if (wcscmp(arguments[i], L"-o") == 0)
        {
            outputPath = arguments[++i];
        }
        else if (wcscmp(arguments[i], L"-L") == 0)
        {
            libraryFolder = arguments[++i];
        }
        else if (wcscmp(arguments[i], L"-O") == 0)
        {
            i++;

            if (wcscmp(arguments[i], L"0") == 0)
            {
                context.Fold = 0;
            }
            else if (wcscmp(arguments[i], L"1") == 0)
            {
                context.Fold = 1;
            }
            else
            {
                context.Script = NULL;
                break;
            }
        }
        else if (wcscmp(arguments[i], L"-r") == 0)
        {
            context.Routines[context.RoutineCount] = ToUtf8(arguments[++i]);
            if (!context.Routines[context.RoutineCount])
            {
                context.Script = NULL;
                break;
            }

            context.RoutineCount++;
        }
        else
        {
            context.Script = NULL;
            break;
        }
    }

    if (!context.Script || !context.Source || !sourcePath || !outputPath)
    {
        fprintf(stderr,
                "usage: kphluac -n <script> -i <source> -o <chunk> "
                "[-r <routine>]... [-L <library folder>] [-O 0|1]\n"
                "       kphluac -v\n");
        return 2;
    }

    if (!IsValidName(context.Script))
    {
        Report(&context,
               0,
               "script name '%s': a name is ASCII letters, digits, '_' and "
               "'-'",
               context.Script);
    }

    for (int i = 0; i < context.RoutineCount; i++)
    {
        if (!IsValidName(context.Routines[i]))
        {
            Report(&context,
                   0,
                   "routine name '%s': a name is ASCII letters, digits, '_' "
                   "and '-'",
                   context.Routines[i]);
        }

        if (IsDriverGlobal(context.Routines[i]))
        {
            Report(&context,
                   0,
                   "routine name '%s' is a global the driver gives every "
                   "script",
                   context.Routines[i]);
        }

        for (int j = 0; j < i; j++)
        {
            if (strcmp(context.Routines[i], context.Routines[j]) == 0)
            {
                Report(&context,
                       0,
                       "routine '%s' is listed twice",
                       context.Routines[i]);
            }
        }
    }

    if (context.Errors != 0)
    {
        return 1;
    }

    //
    // The script libraries are in the folder "lib" beside the source, unless
    // another is given.
    //
    if (!libraryFolder)
    {
        folderLength = wcslen(sourcePath) + 8;

        libraryFolder = calloc(folderLength, sizeof(wchar_t));
        if (!libraryFolder)
        {
            fprintf(stderr, "kphluac: out of memory\n");
            return 2;
        }

        wcscpy_s(libraryFolder, folderLength, sourcePath);

        slash = wcsrchr(libraryFolder, L'\\');
        if (!slash)
        {
            slash = wcsrchr(libraryFolder, L'/');
        }

        if (slash)
        {
            wcscpy_s(slash + 1,
                     folderLength - (size_t)(slash + 1 - libraryFolder),
                     L"lib");
        }
        else
        {
            wcscpy_s(libraryFolder, folderLength, L"lib");
        }
    }

    context.Kind = KPHLUAC_KIND_SCRIPT;
    context.LibraryFolder = libraryFolder;
    context.Statuses = &statuses;

    //
    // nt's statuses are folded into the script and its parts as their values,
    // and its tests as their bodies; -O 0 builds without folding.
    //
    if (!ReadStatuses(&context, libraryFolder, &statuses))
    {
        fprintf(stderr, "kphluac: out of memory\n");
        return 2;
    }

    if (context.Errors != 0)
    {
        return 1;
    }

    text = ReadSource(sourcePath, &length);
    if (!text)
    {
        fprintf(stderr, "kphluac: cannot read %s\n", context.Source);
        return 2;
    }

    //
    // A byte order mark is not part of the script.
    //
    start = text;

    if ((length >= 3) &&
        ((unsigned char)text[0] == 0xEF) &&
        ((unsigned char)text[1] == 0xBB) &&
        ((unsigned char)text[2] == 0xBF))
    {
        start += 3;
        length -= 3;
    }

    result = Build(&context, start, length, outputPath);

    free(text);

    return result;
}
