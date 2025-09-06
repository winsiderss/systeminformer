/*************************************************
*      Perl-Compatible Regular Expressions       *
*************************************************/

/* PCRE2 is a library of functions to support regular expressions whose syntax
and semantics are as close as possible to those of the Perl 5 language. This is
the public header file to be #included by applications that call PCRE2 via the
POSIX wrapper interface.

                       Written by Philip Hazel
     Original API code Copyright (c) 1997-2012 University of Cambridge
          New API code Copyright (c) 2016-2023 University of Cambridge

-----------------------------------------------------------------------------
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

    * Neither the name of the University of Cambridge nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-----------------------------------------------------------------------------
*/

#ifndef PCRE2POSIX_H_IDEMPOTENT_GUARD
#define PCRE2POSIX_H_IDEMPOTENT_GUARD

/* Have to include stdlib.h in order to ensure that size_t is defined. */

#include <stdlib.h>

/* Allow for C++ users */

#ifdef __cplusplus
extern "C" {
#endif

/* Options, mostly defined by POSIX, but with some extras. */

#define REG_ICASE     0x0001  /* Maps to PCRE2_CASELESS */
#define REG_NEWLINE   0x0002  /* Maps to PCRE2_MULTILINE */
#define REG_NOTBOL    0x0004  /* Maps to PCRE2_NOTBOL */
#define REG_NOTEOL    0x0008  /* Maps to PCRE2_NOTEOL */
#define REG_DOTALL    0x0010  /* NOT defined by POSIX; maps to PCRE2_DOTALL */
#define REG_NOSUB     0x0020  /* Do not report what was matched */
#define REG_UTF       0x0040  /* NOT defined by POSIX; maps to PCRE2_UTF */
#define REG_STARTEND  0x0080  /* BSD feature: pass subject string by so,eo */
#define REG_NOTEMPTY  0x0100  /* NOT defined by POSIX; maps to PCRE2_NOTEMPTY */
#define REG_UNGREEDY  0x0200  /* NOT defined by POSIX; maps to PCRE2_UNGREEDY */
#define REG_UCP       0x0400  /* NOT defined by POSIX; maps to PCRE2_UCP */
#define REG_PEND      0x0800  /* GNU feature: pass end pattern by re_endp */
#define REG_NOSPEC    0x1000  /* Maps to PCRE2_LITERAL */

/* This is not used by PCRE2, but by defining it we make it easier
to slot PCRE2 into existing programs that make POSIX calls. */

#define REG_EXTENDED  0

/* Error values. Not all these are relevant or used by the wrapper. */

enum {
  REG_ASSERT = 1,  /* internal error ? */
  REG_BADBR,       /* invalid repeat counts in {} */
  REG_BADPAT,      /* pattern error */
  REG_BADRPT,      /* ? * + invalid */
  REG_EBRACE,      /* unbalanced {} */
  REG_EBRACK,      /* unbalanced [] */
  REG_ECOLLATE,    /* collation error - not relevant */
  REG_ECTYPE,      /* bad class */
  REG_EESCAPE,     /* bad escape sequence */
  REG_EMPTY,       /* empty expression */
  REG_EPAREN,      /* unbalanced () */
  REG_ERANGE,      /* bad range inside [] */
  REG_ESIZE,       /* expression too big */
  REG_ESPACE,      /* failed to get memory */
  REG_ESUBREG,     /* bad back reference */
  REG_INVARG,      /* bad argument */
  REG_NOMATCH      /* match failed */
};


/* The structure representing a compiled regular expression. It is also used
for passing the pattern end pointer when REG_PEND is set. */

typedef struct {
  void *re_pcre2_code;
  void *re_match_data;
  const char *re_endp;
  size_t re_nsub;
  size_t re_erroffset;
  int re_cflags;
} regex_t;

/* The structure in which a captured offset is returned. */

typedef int regoff_t;

typedef struct {
  regoff_t rm_so;
  regoff_t rm_eo;
} regmatch_t;

/* When compiling with the MSVC compiler, it is sometimes necessary to include
a "calling convention" before exported function names. (This is secondhand
information; I know nothing about MSVC myself). For example, something like

  void __cdecl function(....)

might be needed. In order to make this easy, all the exported functions have
PCRE2_CALL_CONVENTION just before their names. It is rarely needed; if not
set, we ensure here that it has no effect. */

#ifndef PCRE2_CALL_CONVENTION
#define PCRE2_CALL_CONVENTION
#endif

#ifndef PCRE2_EXPORT
#define PCRE2_EXPORT
#endif

/* When an application links to a PCRE2 DLL in Windows, the symbols that are
imported have to be identified as such. When building PCRE2, the appropriate
export settings are needed, and are set in pcre2posix.c before including this
file. */

/* By default, we use the standard "extern" declarations. */

#ifndef PCRE2POSIX_EXP_DECL
#  if defined(_WIN32) && defined(PCRE2POSIX_SHARED) && !defined(PCRE2_STATIC)
#    define PCRE2POSIX_EXP_DECL		extern __declspec(dllimport)
#    define PCRE2POSIX_EXP_DEFN		__declspec(dllimport)
#  else
#    define PCRE2POSIX_EXP_DECL		extern PCRE2_EXPORT
#    define PCRE2POSIX_EXP_DEFN
#  endif
#endif

/* The functions. The actual code is in functions with pcre2_xxx names for
uniqueness. POSIX names are provided as macros for API compatibility with POSIX
regex functions. It's done this way to ensure to they are always linked from
the PCRE2 library and not by accident from elsewhere (regex_t differs in size
elsewhere). */

PCRE2POSIX_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_regcomp(regex_t *, const char *, int);
PCRE2POSIX_EXP_DECL int PCRE2_CALL_CONVENTION pcre2_regexec(const regex_t *, const char *, size_t,
                     regmatch_t *, int);
PCRE2POSIX_EXP_DECL size_t PCRE2_CALL_CONVENTION pcre2_regerror(int, const regex_t *, char *, size_t);
PCRE2POSIX_EXP_DECL void PCRE2_CALL_CONVENTION pcre2_regfree(regex_t *);

#define regcomp  pcre2_regcomp
#define regexec  pcre2_regexec
#define regerror pcre2_regerror
#define regfree  pcre2_regfree

/* Debian had a patch that used different names. These are now here to save
them having to maintain their own patch, but are not documented by PCRE2. */

#define PCRE2regcomp  pcre2_regcomp
#define PCRE2regexec  pcre2_regexec
#define PCRE2regerror pcre2_regerror
#define PCRE2regfree  pcre2_regfree

#ifdef __cplusplus
}   /* extern "C" */
#endif

#endif /* PCRE2POSIX_H_IDEMPOTENT_GUARD */

/* End of pcre2posix.h */
