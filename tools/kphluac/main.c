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
#include <lobject.h>
#include <lopcodes.h>
#include <lstate.h>

#define KPHLUAC_ENVIRONMENT "_ENV"
#define KPHLUAC_REGISTERS 256

typedef struct _BUILD_CONTEXT
{
    const char* Script;
    const char* Source;
    const char** Routines;
    int* RoutineLines;
    int RoutineCount;
    int Errors;
} BUILD_CONTEXT, *PBUILD_CONTEXT;

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
    SOURCE_TEXT source;
    char chunkName[LUA_IDSIZE];
    const LClosure* closure;
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

    //
    // A chunk named "=<script>" is shown as "<script>" in messages, in the
    // build and in the driver.
    //
    if (strlen(Context->Script) > (sizeof(chunkName) - 2))
    {
        Report(Context, 0, "the script name is too long");
        result = 1;
        goto Exit;
    }

    chunkName[0] = '=';
    strcpy_s(chunkName + 1, sizeof(chunkName) - 1, Context->Script);

    source.Text = Text;
    source.Length = Length;

    if (lua_load(state, ReadPiece, &source, chunkName, "t") != LUA_OK)
    {
        ReportCompileError(Context, state);
        result = 1;
        goto Exit;
    }

    closure = clLvalue(s2v(state->top.p - 1));

    CheckFunction(Context, closure->p, 1);

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
    // The chunk keeps its line information for the trace of the driver.
    //
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
    int count;
    char* text;
    const char* start;
    size_t length;
    int result;

    memset(&context, 0, sizeof(context));
    sourcePath = NULL;
    outputPath = NULL;

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
                "[-r <routine>]...\n"
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
