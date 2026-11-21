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
} BUILD_CONTEXT, *PBUILD_CONTEXT;

typedef struct _TEXT_BUFFER
{
    char* Data;
    size_t Length;
    size_t Capacity;
} TEXT_BUFFER, *PTEXT_BUFFER;

static const char* KphluacOpeningWords[] = { "function", "do", "if", "repeat" };
static const char* KphluacClosingWords[] = { "end", "until" };
static const char KphluacPlaceholderStart[] =
    "(function(...) local _ = '" KPHLUAC_MARKER;
static const char KphluacPlaceholderEnd[] = "' end)()";

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
 * \param[in] L Unused.
 * \param[in,out] Context The source text, emptied by the call.
 * \param[out] Size Receives the length of the piece.
 *
 * \return The piece, or NULL at the end.
 */
static const char* ReadPiece(
    _In_ lua_State* L,
    _Inout_ void* Context,
    _Out_ size_t* Size
    )
{
    PSOURCE_TEXT source;
    const char* text;

    (void)L;

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
 * \param[in] L Unused.
 * \param[in] Piece The piece.
 * \param[in] Size The length of the piece.
 * \param[in] Context The output file.
 *
 * \return Zero on success.
 */
static int WritePiece(
    _In_ lua_State* L,
    _In_reads_bytes_(Size) const void* Piece,
    _In_ size_t Size,
    _In_ void* Context
    )
{
    (void)L;

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
 * \param[in] L The Lua state, with the message on top of the stack.
 */
static void ReportCompileError(
    _Inout_ PBUILD_CONTEXT Context,
    _In_ lua_State* L
    )
{
    const char* message;
    size_t nameLength;
    char* end;
    long line;

    message = lua_tolstring(L, -1, NULL);
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
                    i++;
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

    if (Context->Kind != KPHLUAC_KIND_SCRIPT)
    {
        CheckLibraryReturn(Context, function);
    }

    for (int i = 0; i < Context->RequireCount; i++)
    {
        memset(&child, 0, sizeof(child));
        child.Script = Context->Requires[i].Name;
        child.LibraryFolder = Context->LibraryFolder;
        child.Kind = (Context->Kind == KPHLUAC_KIND_SCRIPT) ?
            KPHLUAC_KIND_ENTRY : KPHLUAC_KIND_PART;

        if ((swprintf_s(path,
                        ARRAYSIZE(path),
                        L"%ls\\%hs.lua",
                        Context->LibraryFolder,
                        child.Script) < 0) ||
            (sprintf_s(display,
                       sizeof(display),
                       "%ls\\%s.lua",
                       Context->LibraryFolder,
                       child.Script) < 0))
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

int __cdecl main(
    void
    )
{
    BUILD_CONTEXT context;
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
    sourcePath = NULL;
    outputPath = NULL;
    libraryFolder = NULL;

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
                "[-r <routine>]... [-L <library folder>]\n"
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
