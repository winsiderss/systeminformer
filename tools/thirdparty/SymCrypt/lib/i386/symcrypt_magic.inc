;
; SymCrypt_magic.inc
; Copyright (c) Microsoft Corporation. Licensed under the MIT license.
;
; Include file to define the support macros for the Magic field
;
        extern  @SymCryptFatal@4:near


SYMCRYPT_MAGIC_FIELD    MACRO

#if SYMCRYPT_DEBUG
        align   4
        magic   dd      ?
#endif

                        ENDM

SYMCRYPT_CODE_VERSION   EQU     ((SYMCRYPT_CODE_VERSION_API SHL 16) OR SYMCRYPT_CODE_VERSION_MINOR )
SYMCRYPT_MAGIC_CONSTANT EQU     ('S1mv' + SYMCRYPT_CODE_VERSION)

SYMCRYPT_CHECK_MAGIC    MACRO   ptr, struct_name

#if SYMCRYPT_DEBUG
        lea     eax, [ptr + SYMCRYPT_MAGIC_CONSTANT]
        cmp     eax, [ptr + struct_name.magic]
        jz      @F
        mov     ecx, 'magc'
        call    @SymCryptFatal@4
@@:
#endif

        ENDM



