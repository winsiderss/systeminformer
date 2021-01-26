/*
 * random_seed.c
 *
 * Copyright (c) 2013 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

#include "random_seed.h"
#include "config.h"
#include "strerror_override.h"
#include <stdio.h>

#define DEBUG_SEED(s)

#if defined ENABLE_RDRAND

/* cpuid */

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)
#define HAS_X86_CPUID 1

static void do_cpuid(int regs[], int h)
{
    /* clang-format off */
    __asm__ __volatile__("cpuid"
                         : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
                         : "a"(h));
    /* clang-format on */
}

#elif defined _MSC_VER

#define HAS_X86_CPUID 1
#define do_cpuid __cpuid

#endif

/* has_rdrand */

#if HAS_X86_CPUID

static int get_rdrand_seed(void);

/* Valid values are -1 (haven't tested), 0 (no), and 1 (yes). */
static int _has_rdrand = -1;

static int has_rdrand(void)
{
    if (_has_rdrand != -1)
    {
        return _has_rdrand;
    }

    /* CPUID.01H:ECX.RDRAND[bit 30] == 1 */
    int regs[4];
    do_cpuid(regs, 1);
    if (!(regs[2] & (1 << 30)))
    {
        _has_rdrand = 0;
        return 0;
    }

    /*
     * Some CPUs advertise RDRAND in CPUID, but return 0xFFFFFFFF
     * unconditionally. To avoid locking up later, test RDRAND here. If over
     * 3 trials RDRAND has returned the same value, declare it broken.
     * Example CPUs are AMD Ryzen 3000 series
     * and much older AMD APUs, such as the E1-1500
     * https://github.com/systemd/systemd/issues/11810
     * https://linuxreviews.org/RDRAND_stops_returning_random_values_on_older_AMD_CPUs_after_suspend
     */
    _has_rdrand = 0;
    int prev = get_rdrand_seed();
    for (int i = 0; i < 3; i++)
    {
        int temp = get_rdrand_seed();
        if (temp != prev)
        {
            _has_rdrand = 1;
            break;
        }

        prev = temp;
    }

    return _has_rdrand;
}

#endif

/* get_rdrand_seed - GCC x86 and X64 */

#if defined __GNUC__ && (defined __i386__ || defined __x86_64__)

#define HAVE_RDRAND 1

static int get_rdrand_seed(void)
{
    DEBUG_SEED("get_rdrand_seed");
    int _eax;
    /* rdrand eax */
    /* clang-format off */
    __asm__ __volatile__("1: .byte 0x0F\n"
                         "   .byte 0xC7\n"
                         "   .byte 0xF0\n"
                         "   jnc 1b;\n"
                         : "=a" (_eax));
    /* clang-format on */
    return _eax;
}

#endif

#if defined _MSC_VER

#if _MSC_VER >= 1700
#define HAVE_RDRAND 1

/* get_rdrand_seed - Visual Studio 2012 and above */

static int get_rdrand_seed(void)
{
    DEBUG_SEED("get_rdrand_seed");
    int r;
    while (_rdrand32_step(&r) == 0)
        ;
    return r;
}

#elif defined _M_IX86
#define HAVE_RDRAND 1

/* get_rdrand_seed - Visual Studio 2010 and below - x86 only */

/* clang-format off */
static int get_rdrand_seed(void)
{
    DEBUG_SEED("get_rdrand_seed");
    int _eax;
retry:
    /* rdrand eax */
    __asm _emit 0x0F __asm _emit 0xC7 __asm _emit 0xF0
    __asm jnc retry
    __asm mov _eax, eax
    return _eax;
}
/* clang-format on */

#endif
#endif

#endif /* defined ENABLE_RDRAND */

/* has_dev_urandom */

#if defined(__APPLE__) || defined(__unix__) || defined(__linux__)

#include <fcntl.h>
#include <string.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdlib.h>
#include <sys/stat.h>

#define HAVE_DEV_RANDOM 1

static const char *dev_random_file = "/dev/urandom";

static int has_dev_urandom(void)
{
    struct stat buf;
    if (stat(dev_random_file, &buf))
    {
        return 0;
    }
    return ((buf.st_mode & S_IFCHR) != 0);
}

/* get_dev_random_seed */

static int get_dev_random_seed(void)
{
    DEBUG_SEED("get_dev_random_seed");

    int fd = open(dev_random_file, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "error opening %s: %s", dev_random_file, strerror(errno));
        exit(1);
    }

    int r;
    ssize_t nread = read(fd, &r, sizeof(r));
    if (nread != sizeof(r))
    {
        fprintf(stderr, "error short read %s: %s", dev_random_file, strerror(errno));
        exit(1);
    }

    close(fd);
    return r;
}

#endif

/* get_cryptgenrandom_seed */

//#ifdef WIN32
//
//#define HAVE_CRYPTGENRANDOM 1
//
///* clang-format off */
//#include <windows.h>
//
///* Caution: these blank lines must remain so clang-format doesn't reorder
//   includes to put windows.h after wincrypt.h */
//
//#include <wincrypt.h>
///* clang-format on */
//#ifndef __GNUC__
//#pragma comment(lib, "advapi32.lib")
//#endif
//
//static int get_time_seed(void);
//
//static int get_cryptgenrandom_seed(void)
//{
//    HCRYPTPROV hProvider = 0;
//    DWORD dwFlags = CRYPT_VERIFYCONTEXT;
//    int r;
//
//    DEBUG_SEED("get_cryptgenrandom_seed");
//
//    /* WinNT 4 and Win98 do no support CRYPT_SILENT */
//    if (LOBYTE(LOWORD(GetVersion())) > 4)
//        dwFlags |= CRYPT_SILENT;
//
//    if (!CryptAcquireContextA(&hProvider, 0, 0, PROV_RSA_FULL, dwFlags))
//    {
//        fprintf(stderr, "error CryptAcquireContextA 0x%08lx", GetLastError());
//        r = get_time_seed();
//    }
//    else
//    {
//        BOOL ret = CryptGenRandom(hProvider, sizeof(r), (BYTE*)&r);
//        CryptReleaseContext(hProvider, 0);
//        if (!ret)
//        {
//            fprintf(stderr, "error CryptGenRandom 0x%08lx", GetLastError());
//            r = get_time_seed();
//        }
//    }
//
//    return r;
//}
//
//#endif

/* get_time_seed */

#include <time.h>

static int get_time_seed(void)
{
    DEBUG_SEED("get_time_seed");

    return (int)time(NULL) * 433494437;
}

/* json_c_get_random_seed */

int json_c_get_random_seed(void)
{
#ifdef OVERRIDE_GET_RANDOM_SEED
    OVERRIDE_GET_RANDOM_SEED;
#endif
#if defined HAVE_RDRAND && HAVE_RDRAND
    if (has_rdrand())
        return get_rdrand_seed();
#endif
#if defined HAVE_DEV_RANDOM && HAVE_DEV_RANDOM
    if (has_dev_urandom())
        return get_dev_random_seed();
#endif
#if defined HAVE_CRYPTGENRANDOM && HAVE_CRYPTGENRANDOM
    return get_cryptgenrandom_seed();
#endif
    return get_time_seed();
}
