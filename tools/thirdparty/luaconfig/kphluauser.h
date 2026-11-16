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

#if defined(_KERNEL_MODE) && (defined(LUA_CORE) || defined(LUA_LIB))
#pragma bss_seg("PAGEBBS")
#pragma code_seg("PAGE")
#pragma data_seg("PAGEDATA")
#pragma const_seg("PAGERO")
#endif

__declspec(noreturn)
void KphLuaUnprotectedError(
    void
    );

_Ret_maybenull_
char* KphLuaStringBreak(
    _In_z_ const char* String,
    _In_z_ const char* Accept
    );

#define KPH_LUA_CLASS_ALNUM  0
#define KPH_LUA_CLASS_ALPHA  1
#define KPH_LUA_CLASS_CNTRL  2
#define KPH_LUA_CLASS_DIGIT  3
#define KPH_LUA_CLASS_GRAPH  4
#define KPH_LUA_CLASS_LOWER  5
#define KPH_LUA_CLASS_PUNCT  6
#define KPH_LUA_CLASS_SPACE  7
#define KPH_LUA_CLASS_UPPER  8
#define KPH_LUA_CLASS_XDIGIT 9

int KphLuaIsClass(
    _In_ int Character,
    _In_ int Class
    );

int KphLuaToLower(
    _In_ int Character
    );

int KphLuaToUpper(
    _In_ int Character
    );

_Ret_notnull_
int* KphLuaErrno(
    void
    );

#if !defined(lapi_c)
#define lua_pushnumber(L,n) lua_pushinteger((L), (lua_Integer)(n))
#endif

#if defined(ldo_c)
#define abort() KphLuaUnprotectedError()
#endif

#if defined(lobject_c) || defined(lstrlib_c)
#define strpbrk(s,a) KphLuaStringBreak((s), (a))
#endif

#if defined(lstrlib_c) || defined(lbaselib_c)
#undef isalnum
#undef isalpha
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit
#undef tolower
#undef toupper
#define isalnum(c) KphLuaIsClass((c), KPH_LUA_CLASS_ALNUM)
#define isalpha(c) KphLuaIsClass((c), KPH_LUA_CLASS_ALPHA)
#define iscntrl(c) KphLuaIsClass((c), KPH_LUA_CLASS_CNTRL)
#define isdigit(c) KphLuaIsClass((c), KPH_LUA_CLASS_DIGIT)
#define isgraph(c) KphLuaIsClass((c), KPH_LUA_CLASS_GRAPH)
#define islower(c) KphLuaIsClass((c), KPH_LUA_CLASS_LOWER)
#define ispunct(c) KphLuaIsClass((c), KPH_LUA_CLASS_PUNCT)
#define isspace(c) KphLuaIsClass((c), KPH_LUA_CLASS_SPACE)
#define isupper(c) KphLuaIsClass((c), KPH_LUA_CLASS_UPPER)
#define isxdigit(c) KphLuaIsClass((c), KPH_LUA_CLASS_XDIGIT)
#define tolower(c) KphLuaToLower((c))
#define toupper(c) KphLuaToUpper((c))
#endif

#if defined(lstrlib_c)
#define float int
#define double long long
#undef HUGE_VAL
#define HUGE_VAL LUA_MAXINTEGER
#endif

#if defined(lauxlib_c)
#undef errno
#define errno (*KphLuaErrno())
#define strerror(e) ((void)(e), "error")
#undef stdin
#define stdin ((FILE*)0)
#define fopen(n,m) ((void)(n), (void)(m), (FILE*)0)
#define freopen(n,m,f) ((void)(n), (void)(m), (void)(f), (FILE*)0)
#undef getc
#define getc(f) ((void)(f), -1)
#define fread(b,s,n,f) ((void)(b), (void)(s), (void)(n), (void)(f), (size_t)0)
#undef feof
#define feof(f) ((void)(f), 1)
#undef ferror
#define ferror(f) ((void)(f), 1)
#define fclose(f) ((void)(f), 0)
#define free(p) ((void)(p))
#define realloc(p,n) ((void)(p), (void)(n), (void*)0)
#endif
