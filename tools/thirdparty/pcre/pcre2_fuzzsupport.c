/***************************************************************************
Fuzzer driver for PCRE2. Given an arbitrary string of bytes and a length, it
tries to compile and match it, deriving options from the string itself. If
STANDALONE is defined, a main program that calls the driver with the contents
of specified files is compiled, and commentary on what is happening is output.
If an argument starts with '=' the rest of it it is taken as a literal string
rather than a file name. This allows easy testing of short strings.

Written by Philip Hazel, October 2016
Updated February 2024 (Addison Crump added 16-bit/32-bit and JIT support)
Further updates March/April/May 2024 by PH
***************************************************************************/

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* stack size adjustment */
#include <sys/time.h>
#include <sys/resource.h>

#define STACK_SIZE_MB 256
#define JIT_SIZE_LIMIT (200 * 1024)

#ifndef PCRE2_CODE_UNIT_WIDTH
#define PCRE2_CODE_UNIT_WIDTH 8
#endif

#include "config.h"
#include "pcre2.h"
#include "pcre2_internal.h"

#define MAX_MATCH_SIZE 1000

#define DFA_WORKSPACE_COUNT 100

/* When adding new compile or match options, remember to update the functions
below that output them. */

#define ALLOWED_COMPILE_OPTIONS \
  (PCRE2_ANCHORED|PCRE2_ALLOW_EMPTY_CLASS|PCRE2_ALT_BSUX|PCRE2_ALT_CIRCUMFLEX| \
   PCRE2_ALT_EXTENDED_CLASS|PCRE2_ALT_VERBNAMES|PCRE2_AUTO_CALLOUT| \
   PCRE2_CASELESS|PCRE2_DOLLAR_ENDONLY| \
   PCRE2_DOTALL|PCRE2_DUPNAMES|PCRE2_ENDANCHORED|PCRE2_EXTENDED| \
   PCRE2_EXTENDED_MORE|PCRE2_FIRSTLINE| \
   PCRE2_MATCH_UNSET_BACKREF|PCRE2_MULTILINE|PCRE2_NEVER_BACKSLASH_C| \
   PCRE2_NO_AUTO_CAPTURE| \
   PCRE2_NO_AUTO_POSSESS|PCRE2_NO_DOTSTAR_ANCHOR|PCRE2_NO_START_OPTIMIZE| \
   PCRE2_UCP|PCRE2_UNGREEDY|PCRE2_USE_OFFSET_LIMIT| \
   PCRE2_UTF)

#define ALLOWED_MATCH_OPTIONS \
  (PCRE2_ANCHORED|PCRE2_ENDANCHORED|PCRE2_NOTBOL|PCRE2_NOTEOL|PCRE2_NOTEMPTY| \
   PCRE2_NOTEMPTY_ATSTART|PCRE2_PARTIAL_HARD| \
   PCRE2_PARTIAL_SOFT)

#define BASE_MATCH_OPTIONS \
  (PCRE2_NO_JIT|PCRE2_DISABLE_RECURSELOOP_CHECK)


#if defined(SUPPORT_DIFF_FUZZ) || defined(STANDALONE)
static void print_compile_options(FILE *stream, uint32_t compile_options)
{
fprintf(stream, "Compile options %s%.8x =",
  (compile_options == PCRE2_NEVER_BACKSLASH_C)? "(base) " : "",
  compile_options);

fprintf(stream, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
  ((compile_options & PCRE2_ALT_BSUX) != 0)? " alt_bsux" : "",
  ((compile_options & PCRE2_ALT_CIRCUMFLEX) != 0)? " alt_circumflex" : "",
  ((compile_options & PCRE2_ALT_EXTENDED_CLASS) != 0)? "alt_extended_class" : "",
  ((compile_options & PCRE2_ALT_VERBNAMES) != 0)? " alt_verbnames" : "",
  ((compile_options & PCRE2_ALLOW_EMPTY_CLASS) != 0)? " allow_empty_class" : "",
  ((compile_options & PCRE2_ANCHORED) != 0)? " anchored" : "",
  ((compile_options & PCRE2_AUTO_CALLOUT) != 0)? " auto_callout" : "",
  ((compile_options & PCRE2_CASELESS) != 0)? " caseless" : "",
  ((compile_options & PCRE2_DOLLAR_ENDONLY) != 0)? " dollar_endonly" : "",
  ((compile_options & PCRE2_DOTALL) != 0)? " dotall" : "",
  ((compile_options & PCRE2_DUPNAMES) != 0)? " dupnames" : "",
  ((compile_options & PCRE2_ENDANCHORED) != 0)? " endanchored" : "",
  ((compile_options & PCRE2_EXTENDED) != 0)? " extended" : "",
  ((compile_options & PCRE2_EXTENDED_MORE) != 0)? " extended_more" : "",
  ((compile_options & PCRE2_FIRSTLINE) != 0)? " firstline" : "",
  ((compile_options & PCRE2_MATCH_UNSET_BACKREF) != 0)? " match_unset_backref" : "",
  ((compile_options & PCRE2_MULTILINE) != 0)? " multiline" : "",
  ((compile_options & PCRE2_NEVER_BACKSLASH_C) != 0)? " never_backslash_c" : "",
  ((compile_options & PCRE2_NEVER_UCP) != 0)? " never_ucp" : "",
  ((compile_options & PCRE2_NEVER_UTF) != 0)? " never_utf" : "",
  ((compile_options & PCRE2_NO_AUTO_CAPTURE) != 0)? " no_auto_capture" : "",
  ((compile_options & PCRE2_NO_AUTO_POSSESS) != 0)? " no_auto_possess" : "",
  ((compile_options & PCRE2_NO_DOTSTAR_ANCHOR) != 0)? " no_dotstar_anchor" : "",
  ((compile_options & PCRE2_NO_UTF_CHECK) != 0)? " no_utf_check" : "",
  ((compile_options & PCRE2_NO_START_OPTIMIZE) != 0)? " no_start_optimize" : "",
  ((compile_options & PCRE2_UCP) != 0)? " ucp" : "",
  ((compile_options & PCRE2_UNGREEDY) != 0)? " ungreedy" : "",
  ((compile_options & PCRE2_USE_OFFSET_LIMIT) != 0)? " use_offset_limit" : "",
  ((compile_options & PCRE2_UTF) != 0)? " utf" : "");
}

static void print_match_options(FILE *stream, uint32_t match_options)
{
fprintf(stream, "Match options %s%.8x =",
  (match_options == BASE_MATCH_OPTIONS)? "(base) " : "", match_options);

fprintf(stream, "%s%s%s%s%s%s%s%s%s%s%s\n",
  ((match_options & PCRE2_ANCHORED) != 0)? " anchored" : "",
  ((match_options & PCRE2_DISABLE_RECURSELOOP_CHECK) != 0)? " disable_recurseloop_check" : "",
  ((match_options & PCRE2_ENDANCHORED) != 0)? " endanchored" : "",
  ((match_options & PCRE2_NO_JIT) != 0)? " no_jit" : "",
  ((match_options & PCRE2_NO_UTF_CHECK) != 0)? " no_utf_check" : "",
  ((match_options & PCRE2_NOTBOL) != 0)? " notbol" : "",
  ((match_options & PCRE2_NOTEMPTY) != 0)? " notempty" : "",
  ((match_options & PCRE2_NOTEMPTY_ATSTART) != 0)? " notempty_atstart" : "",
  ((match_options & PCRE2_NOTEOL) != 0)? " noteol" : "",
  ((match_options & PCRE2_PARTIAL_HARD) != 0)? " partial_hard" : "",
  ((match_options & PCRE2_PARTIAL_SOFT) != 0)? " partial_soft" : "");
}


/* This function can print an error message at all code unit widths. */

static void print_error(FILE *f, int errorcode, const char *text, ...)
{
PCRE2_UCHAR buffer[256];
PCRE2_UCHAR *p = buffer;
va_list ap;
va_start(ap, text);
vfprintf(f, text, ap);
va_end(ap);
pcre2_get_error_message(errorcode, buffer, 256);
while (*p != 0) fprintf(f, "%c", *p++);
printf("\n");
}
#endif /* defined(SUPPORT_DIFF_FUZZ || defined(STANDALONE) */


#ifdef SUPPORT_JIT
#ifdef SUPPORT_DIFF_FUZZ
static void dump_matches(FILE *stream, int count, pcre2_match_data *match_data)
{
int errorcode;

for (int index = 0; index < count; index++)
  {
  PCRE2_UCHAR *bufferptr = NULL;
  PCRE2_SIZE bufflen = 0;

  errorcode = pcre2_substring_get_bynumber(match_data, index, &bufferptr,
    &bufflen);

  if (errorcode >= 0)
    {
    fprintf(stream, "Match %d (hex encoded): ", index);
    for (PCRE2_SIZE i = 0; i < bufflen; i++)
      {
      fprintf(stream, "%02x", bufferptr[i]);
      }
    fprintf(stream, "\n");
    }
  else
    {
    print_error(stream, errorcode, "Match %d failed: ", index);
    }
  }
}

/* This function describes the current test case being evaluated, then aborts */

static void describe_failure(
  const char *task,
  const PCRE2_UCHAR *data,
  PCRE2_SIZE size,
  uint32_t compile_options,
  uint32_t match_options,
  int errorcode,
  int errorcode_jit,
  int matches,
  int matches_jit,
  pcre2_match_data *match_data,
  pcre2_match_data *match_data_jit
) {

fprintf(stderr, "Encountered failure while performing %s; context:\n", task);

fprintf(stderr, "Pattern/sample string (hex encoded): ");
for (size_t i = 0; i < size; i++)
  {
  fprintf(stderr, "%02x", data[i]);
  }
fprintf(stderr, "\n");

print_compile_options(stderr, compile_options);
print_match_options(stderr, match_options);

if (errorcode < 0)
  {
  print_error(stderr, errorcode, "Non-JIT'd operation emitted an error: ");
  }

if (matches >= 0)
  {
  fprintf(stderr, "Non-JIT'd operation did not emit an error.\n");
  if (match_data != NULL)
    {
    fprintf(stderr, "%d matches discovered by non-JIT'd regex:\n", matches);
    dump_matches(stderr, matches, match_data);
    fprintf(stderr, "\n");
    }
  }

if (errorcode_jit < 0)
  {
  print_error(stderr, errorcode_jit, "JIT'd operation emitted error %d:",
    errorcode_jit);
  }

if (matches_jit >= 0)
  {
  fprintf(stderr, "JIT'd operation did not emit an error.\n");
  if (match_data_jit != NULL)
    {
    fprintf(stderr, "%d matches discovered by JIT'd regex:\n", matches_jit);
    dump_matches(stderr, matches_jit, match_data_jit);
    fprintf(stderr, "\n");
    }
  }

abort();
}
#endif  /* SUPPORT_DIFF_FUZZ */
#endif  /* SUPPORT_JIT */

/* This is the callout function. Its only purpose is to halt matching if there
are more than 100 callouts, as one way of stopping too much time being spent on
fruitless matches. The callout data is a pointer to the counter. */

static int callout_function(pcre2_callout_block *cb, void *callout_data)
{
(void)cb;  /* Avoid unused parameter warning */
*((uint32_t *)callout_data) += 1;
return (*((uint32_t *)callout_data) > 100)? PCRE2_ERROR_CALLOUT : 0;
}

/* Putting in this apparently unnecessary prototype prevents gcc from giving a
"no previous prototype" warning when compiling at high warning level. */

int LLVMFuzzerInitialize(int *, char ***);

int LLVMFuzzerTestOneInput(unsigned char *, size_t);

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
int rc;
struct rlimit rlim;
getrlimit(RLIMIT_STACK, &rlim);
rlim.rlim_cur = STACK_SIZE_MB * 1024 * 1024;
if (rlim.rlim_cur > rlim.rlim_max)
  {
  fprintf(stderr, "Hard stack size limit is too small\n");
  _exit(1);
  }
rc = setrlimit(RLIMIT_STACK, &rlim);
if (rc != 0)
  {
  fprintf(stderr, "Failed to expand stack size\n");
  _exit(1);
  }

(void)argc;  /* Avoid "unused parameter" warnings */
(void)argv;
return 0;
}

/* Here's the driving function. */

int LLVMFuzzerTestOneInput(unsigned char *data, size_t size)
{
PCRE2_UCHAR *wdata;
PCRE2_UCHAR *newwdata = NULL;
uint32_t compile_options;
uint32_t match_options;
uint64_t random_options;
pcre2_match_data *match_data = NULL;
#ifdef SUPPORT_JIT
pcre2_match_data *match_data_jit = NULL;
#endif
pcre2_compile_context *compile_context = NULL;
pcre2_match_context *match_context = NULL;
size_t match_size;
int dfa_workspace[DFA_WORKSPACE_COUNT];

if (size < sizeof(random_options)) return -1;

random_options = *(uint64_t *)(data);
data += sizeof(random_options);
wdata = (PCRE2_UCHAR *)data;
size -= sizeof(random_options);
size /= PCRE2_CODE_UNIT_WIDTH / 8;

/* PCRE2 compiles quantified groups by replicating them. In certain cases of
very large quantifiers this can lead to unacceptably long JIT compile times. To
get around this, we scan the data string for large quantifiers that follow a
closing parenthesis, and reduce the value of the quantifier to 10, assuming
that this will make minimal difference to the detection of bugs.

Do the same for quantifiers that follow a closing square bracket, because
classes that contain a number of non-ascii characters can take a lot of time
when matching.

We have to make a copy of the input because oss-fuzz complains if we overwrite
the original. Start the scan at the second character so there can be a
lookbehind for a backslash, and end it before the end so that the next
character can be checked for an opening brace. */

if (size > 3)
  {
  newwdata = malloc(size * sizeof(PCRE2_UCHAR));
  memcpy(newwdata, wdata, size * sizeof(PCRE2_UCHAR));
  wdata = newwdata;

  for (size_t i = 1; i < size - 2; i++)
    {
    size_t j;

    if ((wdata[i] != ')' && wdata[i] != ']') || wdata[i-1] == '\\' ||
         wdata[i+1] != '{')
      continue;
    i++;  /* Points to '{' */

    /* Loop for two values in a quantifier. Offset i points to brace or comma
    at the start of the loop. */

    for (int ii = 0; ii < 2; ii++)
      {
      int q = 0;

      if (i >= size - 1) goto END_QSCAN;  /* Can happen for , */

      /* Ignore leading spaces. */

      while (wdata[i+1] == ' ' || wdata[i+1] == '\t')
        {
        i++;
        if (i >= size - 1) goto END_QSCAN;
        }

      /* Ignore non-significant leading zeros. */

      while (wdata[i+1] == '0' && i+2 < size && wdata[i+2] >= '0' &&
             wdata[i+2] <= '9')
        {
        i++;
        if (i >= size - 1) goto END_QSCAN;
        }

      /* Scan for a number ending in brace, or comma in the first iteration,
      optionally preceded by space. */

      for (j = i + 1; j < size && j < i + 7; j++)
        {
        if (wdata[j] == ' ' || wdata[j] == '\t')
          {
          j++;
          while (j < size && (wdata[j] == ' ' || wdata[j] == '\t')) j++;
          if (j >= size) goto OUTERLOOP;
          if (wdata[j] != '}' && wdata[j] != ',') goto OUTERLOOP;
          }
        if (wdata[j] == '}' || (ii == 0 && wdata[j] == ',')) break;

        if (wdata[j] < '0' || wdata[j] > '9')
          {
          j--;               /* Ensure this character is checked next. The */
          goto OUTERLOOP;    /* string might be (e.g.) "){9){234}" */
          }
        q = q * 10 + (wdata[j] - '0');
        }

      if (j >= size) goto END_QSCAN;  /* End of data */

      /* Hit ',' or '}' or read 6 digits. Six digits is a number > 65536 which
      is the maximum quantifier. Leave such numbers alone. */

      if (j >= i + 7 || q > 65535) goto OUTERLOOP;

      /* Limit the quantifier size to 10 */

      if (q > 10)
        {
#ifdef STANDALONE
        printf("Reduced quantifier value %d to 10.\n", q);
#endif
        for (size_t k = i + 1; k < j; k++) wdata[k] = '0';
        wdata[j - 2] = '1';
        }

      /* Advance to end of number and break if reached closing brace (continue
      after comma, which is only valid in the first time round this loop). */

      i = j;
      if (wdata[i] == '}') break;
      }

    /* Continue along the data string */

    OUTERLOOP:
    i = j;
    continue;
    }
  }
END_QSCAN:

/* Limiting the length of the subject for matching stops fruitless searches
in large trees taking too much time. */

match_size = (size > MAX_MATCH_SIZE)? MAX_MATCH_SIZE : size;

/* Create a compile context, and set a limit on the size of the compiled
pattern. This stops the fuzzer using vast amounts of memory. */

compile_context = pcre2_compile_context_create(NULL);
if (compile_context == NULL)
  {
#ifdef STANDALONE
  fprintf(stderr, "** Failed to create compile context block\n");
#endif
  abort();
  }
pcre2_set_max_pattern_compiled_length(compile_context, 10*1024*1024);

/* Ensure that all undefined option bits are zero (waste of time trying them)
and also that PCRE2_NO_UTF_CHECK is unset, as there is no guarantee that the
input is valid UTF. Also unset PCRE2_NEVER_UTF and PCRE2_NEVER_UCP as there is
no reason to disallow UTF and UCP. Force PCRE2_NEVER_BACKSLASH_C to be set
because \C in random patterns is highly likely to cause a crash. */

compile_options = ((random_options >> 32) & ALLOWED_COMPILE_OPTIONS) |
  PCRE2_NEVER_BACKSLASH_C;
match_options = (((uint32_t)random_options) & ALLOWED_MATCH_OPTIONS) |
  BASE_MATCH_OPTIONS;

/* Discard partial matching if PCRE2_ENDANCHORED is set, because they are not
allowed together and just give an immediate error return. */

if (((compile_options|match_options) & PCRE2_ENDANCHORED) != 0)
  match_options &= ~(PCRE2_PARTIAL_HARD|PCRE2_PARTIAL_SOFT);

/* Do the compile with and without the options, and after a successful compile,
likewise do the match with and without the options. */

for (int i = 0; i < 2; i++)
  {
  uint32_t callout_count;
  int errorcode;
#ifdef SUPPORT_JIT
  int errorcode_jit;
#ifdef SUPPORT_DIFF_FUZZ
  int matches = 0;
  int matches_jit = 0;
#endif
#endif
  PCRE2_SIZE erroroffset;
  pcre2_code *code;

#ifdef STANDALONE
  printf("\n");
  print_compile_options(stdout, compile_options);
#endif

  code = pcre2_compile((PCRE2_SPTR)wdata, (PCRE2_SIZE)size, compile_options,
    &errorcode, &erroroffset, compile_context);

  /* Compilation succeeded */

  if (code != NULL)
    {
    int j;
    uint32_t save_match_options = match_options;

    /* Call JIT compile only if the compiled pattern is not too big. */

#ifdef SUPPORT_JIT
    int jit_ret = -1;
    if (((struct pcre2_real_code *)code)->blocksize <= JIT_SIZE_LIMIT)
      {
#ifdef STANDALONE
      printf("Compile succeeded; calling JIT compile\n");
#endif
      jit_ret = pcre2_jit_compile(code, PCRE2_JIT_COMPLETE);
#ifdef STANDALONE
      if (jit_ret < 0) printf("JIT compile error %d\n", jit_ret);
#endif
      }
    else
      {
#ifdef STANDALONE
      printf("Not calling JIT: compiled pattern is too long "
        "(%ld bytes; limit=%d)\n",
        ((struct pcre2_real_code *)code)->blocksize, JIT_SIZE_LIMIT);
#endif
      }
#endif  /* SUPPORT_JIT */

    /* Create match data and context blocks only when we first need them. Set
    low match and depth limits to avoid wasting too much searching large
    pattern trees. Almost all matches are going to fail. */

    if (match_data == NULL)
      {
      match_data = pcre2_match_data_create(32, NULL);
#ifdef SUPPORT_JIT
      match_data_jit = pcre2_match_data_create(32, NULL);
      if (match_data == NULL || match_data_jit == NULL)
#else
      if (match_data == NULL)
#endif
        {
#ifdef STANDALONE
        fprintf(stderr, "** Failed to create match data block\n");
#endif
        abort();
        }
      }

    if (match_context == NULL)
      {
      match_context = pcre2_match_context_create(NULL);
      if (match_context == NULL)
        {
#ifdef STANDALONE
        fprintf(stderr, "** Failed to create match context block\n");
#endif
        abort();
        }
      (void)pcre2_set_match_limit(match_context, 100);
      (void)pcre2_set_depth_limit(match_context, 100);
      (void)pcre2_set_callout(match_context, callout_function, &callout_count);
      }

    /* Match twice, with and without options. */

#ifdef STANDALONE
    printf("\n");
#endif
    for (j = 0; j < 2; j++)
      {
#ifdef STANDALONE
      print_match_options(stdout, match_options);
#endif

      callout_count = 0;
      errorcode = pcre2_match(code, (PCRE2_SPTR)wdata, (PCRE2_SIZE)match_size, 0,
        match_options, match_data, match_context);

#ifdef STANDALONE
      if (errorcode >= 0) printf("Match returned %d\n", errorcode); else
        print_error(stdout, errorcode, "Match failed: error %d: ", errorcode);
#endif

/* If JIT is enabled, do a JIT match and, if appropriately compiled, compare
with the interpreter. */

#ifdef SUPPORT_JIT
      if (jit_ret >= 0)
        {
#ifdef STANDALONE
        printf("Matching with JIT\n");
#endif
        callout_count = 0;
        errorcode_jit = pcre2_match(code, (PCRE2_SPTR)wdata, (PCRE2_SIZE)match_size, 0,
          match_options & ~PCRE2_NO_JIT, match_data_jit, match_context);

#ifdef STANDALONE
        if (errorcode_jit >= 0)
          printf("Match returned %d\n", errorcode_jit);
        else
          print_error(stdout, errorcode_jit, "JIT match failed: error %d: ",
            errorcode_jit);
#else
        (void)errorcode_jit;   /* Avoid compiler warning */
#endif  /* STANDALONE */

/* With differential matching enabled, compare with interpreter. */

#ifdef SUPPORT_DIFF_FUZZ
        matches = errorcode;
        matches_jit = errorcode_jit;

        if (errorcode_jit != errorcode)
          {
          if (!(errorcode < 0 && errorcode_jit < 0) &&
                errorcode != PCRE2_ERROR_MATCHLIMIT && errorcode != PCRE2_ERROR_CALLOUT &&
                errorcode_jit != PCRE2_ERROR_MATCHLIMIT && errorcode_jit != PCRE2_ERROR_JIT_STACKLIMIT && errorcode_jit != PCRE2_ERROR_CALLOUT)
            {
            describe_failure("match errorcode comparison", wdata, size, compile_options, match_options, errorcode, errorcode_jit, matches, matches_jit, match_data, match_data_jit);
            }
          }
        else
          {
          for (int index = 0; index < errorcode; index++)
            {
            PCRE2_UCHAR *bufferptr, *bufferptr_jit;
            PCRE2_SIZE bufflen, bufflen_jit;

            bufferptr = bufferptr_jit = NULL;
            bufflen = bufflen_jit = 0;

            errorcode = pcre2_substring_get_bynumber(match_data, (uint32_t) index, &bufferptr, &bufflen);
            errorcode_jit = pcre2_substring_get_bynumber(match_data_jit, (uint32_t) index, &bufferptr_jit, &bufflen_jit);

            if (errorcode != errorcode_jit)
              {
              describe_failure("match entry errorcode comparison", wdata, size,
                compile_options, match_options, errorcode, errorcode_jit,
                matches, matches_jit, match_data, match_data_jit);
              }

            if (errorcode >= 0)
              {
              if (bufflen != bufflen_jit)
                {
                describe_failure("match entry length comparison", wdata, size,
                  compile_options, match_options, errorcode, errorcode_jit,
                  matches, matches_jit, match_data, match_data_jit);
                }

              if (memcmp(bufferptr, bufferptr_jit, bufflen) != 0)
                {
                describe_failure("match entry content comparison", wdata, size,
                  compile_options, match_options, errorcode, errorcode_jit,
                  matches, matches_jit, match_data, match_data_jit);
                }
              }

              pcre2_substring_free(bufferptr);
              pcre2_substring_free(bufferptr_jit);
            }
          }
#endif  /* SUPPORT_DIFF_FUZZ */
        }
#endif  /* SUPPORT_JIT */

      if (match_options == BASE_MATCH_OPTIONS) break;  /* Don't do same twice */
      match_options = BASE_MATCH_OPTIONS;              /* For second time */
      }

    /* Match with DFA twice, with and without options, but remove options that
    are not allowed with DFA. */

    match_options = save_match_options & ~BASE_MATCH_OPTIONS;

#ifdef STANDALONE
    printf("\n");
#endif

    for (j = 0; j < 2; j++)
      {
#ifdef STANDALONE
      printf("DFA match options %.8x =", match_options);
      printf("%s%s%s%s%s%s%s%s%s\n",
        ((match_options & PCRE2_ANCHORED) != 0)? " anchored" : "",
        ((match_options & PCRE2_ENDANCHORED) != 0)? " endanchored" : "",
        ((match_options & PCRE2_NO_UTF_CHECK) != 0)? " no_utf_check" : "",
        ((match_options & PCRE2_NOTBOL) != 0)? " notbol" : "",
        ((match_options & PCRE2_NOTEMPTY) != 0)? " notempty" : "",
        ((match_options & PCRE2_NOTEMPTY_ATSTART) != 0)? " notempty_atstart" : "",
        ((match_options & PCRE2_NOTEOL) != 0)? " noteol" : "",
        ((match_options & PCRE2_PARTIAL_HARD) != 0)? " partial_hard" : "",
        ((match_options & PCRE2_PARTIAL_SOFT) != 0)? " partial_soft" : "");
#endif

      callout_count = 0;
      errorcode = pcre2_dfa_match(code, (PCRE2_SPTR)wdata,
        (PCRE2_SIZE)match_size, 0, match_options, match_data,
        match_context, dfa_workspace, DFA_WORKSPACE_COUNT);

#ifdef STANDALONE
      if (errorcode >= 0)
        printf("Match returned %d\n", errorcode);
      else
        print_error(stdout, errorcode, "DFA match failed: error %d: ", errorcode);
#endif

      if (match_options == 0) break;  /* No point doing same twice */
      match_options = 0;              /* For second time */
      }

    match_options = save_match_options;  /* Reset for the second compile */
    pcre2_code_free(code);
    }

  /* Compilation failed */

  else
    {
#ifdef STANDALONE
    print_error(stdout, errorcode, "Error %d at offset %lu: ", errorcode,
      erroroffset);
#else
    if (errorcode == PCRE2_ERROR_INTERNAL) abort();
#endif
    }

  if (compile_options == PCRE2_NEVER_BACKSLASH_C) break;  /* Avoid same twice */
  compile_options = PCRE2_NEVER_BACKSLASH_C;              /* For second time */
  }

/* Tidy up before exiting */

if (match_data != NULL) pcre2_match_data_free(match_data);
#ifdef SUPPORT_JIT
if (match_data_jit != NULL) pcre2_match_data_free(match_data_jit);
#endif
free(newwdata);
if (match_context != NULL) pcre2_match_context_free(match_context);
if (compile_context != NULL) pcre2_compile_context_free(compile_context);
return 0;
}


/* Optional main program.  */

#ifdef STANDALONE
int main(int argc, char **argv)
{
LLVMFuzzerInitialize(&argc, &argv);

if (argc < 2)
  {
  printf("** No arguments given\n");
  return 0;
  }

for (int i = 1; i < argc; i++)
  {
  size_t filelen;
  size_t readsize;
  unsigned char *buffer;
  FILE *f;

  /* Handle a literal string. Copy to an exact size buffer so that checks for
  overrunning work. */

  if (argv[i][0] == '=')
    {
    readsize = strlen(argv[i]) - 1;
    printf("------ <Literal> ------\n");
    printf("Length = %lu\n", readsize);
    printf("%.*s\n", (int)readsize, argv[i]+1);
    buffer = (unsigned char *)malloc(readsize);
    if (buffer == NULL)
      printf("** Failed to allocate %lu bytes of memory\n", readsize);
    else
      {
      memcpy(buffer, argv[i]+1, readsize);
      LLVMFuzzerTestOneInput(buffer, readsize);
      free(buffer);
      }
    continue;
    }

  /* Handle a string given in a file */

  f = fopen(argv[i], "rb");
  if (f == NULL)
    {
    printf("** Failed to open %s: %s\n", argv[i], strerror(errno));
    continue;
    }

  printf("------ %s ------\n", argv[i]);

  fseek(f, 0, SEEK_END);
  filelen = ftell(f);
  fseek(f, 0, SEEK_SET);

  buffer = (unsigned char *)malloc(filelen);
  if (buffer == NULL)
    {
    printf("** Failed to allocate %lu bytes of memory\n", filelen);
    fclose(f);
    continue;
    }

  readsize = fread(buffer, 1, filelen, f);
  fclose(f);

  if (readsize != filelen)
    printf("** File size is %lu but fread() returned %lu\n", filelen, readsize);
  else
    {
    printf("Length = %lu\n", filelen);
    LLVMFuzzerTestOneInput(buffer, filelen);
    }
  free(buffer);
  }

return 0;
}
#endif  /* STANDALONE */

/* End */
