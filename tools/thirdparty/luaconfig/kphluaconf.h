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

#pragma once

#include "luaconf.h"
#include <sal.h>

#define LUA_USER_H "kphluauser.h"

struct lua_State;
struct lua_longjmp;

//
// Numbers
//

#undef LUA_NUMBER
#define LUA_NUMBER LUA_INTEGER

#undef LUAI_UACNUMBER
#define LUAI_UACNUMBER LUAI_UACINT

#undef LUA_NUMBER_FRMLEN
#define LUA_NUMBER_FRMLEN LUA_INTEGER_FRMLEN

#undef LUA_NUMBER_FMT
#define LUA_NUMBER_FMT LUA_INTEGER_FMT

#undef LUA_NUMBER_FMT_N
#define LUA_NUMBER_FMT_N LUA_INTEGER_FMT

#define KPH_LUA_FLT_DIG 18
#define KPH_LUA_FLT_MANT_DIG 64
#define KPH_LUA_FLT_MAX_10_EXP 18

#undef l_floatatt
#define l_floatatt(n) (KPH_LUA_FLT_##n)

#undef l_floor
#define l_floor(x) (x)

LUA_NUMBER KphLuaStringToNumber(
    _In_z_ const char* String,
    _Out_opt_ char** End
    );

#undef lua_str2number
#define lua_str2number(s,p) KphLuaStringToNumber((s), (p))

#undef lua_strx2number
#define lua_strx2number(s,p) KphLuaStringToNumber((s), (p))

#undef lua_number2strx
#define lua_number2strx(L,b,sz,f,n) \
    ((void)(L), (void)(sz), (void)(f), (void)(n), (b)[0] = '\0', 0)

#define luai_numidiv(L,a,b) luaV_idiv(L,a,b)
#define luai_nummod(L,a,b,m) { (m) = luaV_mod(L,a,b); }
#define luai_numdiv(L,a,b) ((void)(L), (void)(a), (void)(b), (LUA_NUMBER)0)
#define luai_numpow(L,a,b) ((void)(L), (void)(a), (void)(b), (LUA_NUMBER)0)

#define l_hashfloat(n) ((int)((LUA_UNSIGNED)(n) % (unsigned int)INT_MAX))

//
// No coercion of strings to numbers in arithmetic.
//
#define LUA_NOCVTS2N

#undef LUAI_MAXALIGN
#define LUAI_MAXALIGN lua_Number n; void* s; lua_Integer i; long l

#if defined(_KERNEL_MODE)
#define LUAI_MAXCCALLS 24
#define MAXCCALLS 32
#undef LUAL_BUFFERSIZE
#define LUAL_BUFFERSIZE 256
#endif

//
// Formatting
//

int KphLuaFormat(
    _Out_writes_z_(Size) char* Buffer,
    _In_ size_t Size,
    _In_z_ _Printf_format_string_ const char* Format,
    ...
    );

#undef l_sprintf
#define l_sprintf(s,sz,f,i) KphLuaFormat((s), (sz), (f), (i))

#undef lua_getlocaledecpoint
#define lua_getlocaledecpoint() '.'

#define l_strcoll strcmp

//
// Errors
//

__declspec(noreturn)
void KphLuaThrow(
    _In_ struct lua_State* State,
    _In_ int Status
    );

typedef void (*KPH_LUA_PROTECTED_ROUTINE)(
    _In_ struct lua_State* State,
    _In_opt_ void* Context
    );

void KphLuaTry(
    _In_ struct lua_State* State,
    _In_ struct lua_longjmp* Jump,
    _In_ KPH_LUA_PROTECTED_ROUTINE Routine,
    _In_opt_ void* Context
    );

#define LUAI_THROW(L,c) KphLuaThrow((L), (c)->status)
#define LUAI_TRY(L,c,f,ud) KphLuaTry((L), (c), (f), (ud))

//
// Environment
//

#define l_signalT int

unsigned int KphLuaSeed(
    void
    );

#define luai_makeseed() KphLuaSeed()

#define lua_writestring(s,l) ((void)(s), (void)(l))
#define lua_writeline() ((void)0)
#define lua_writestringerror(s,p) ((void)(s), (void)(p))
