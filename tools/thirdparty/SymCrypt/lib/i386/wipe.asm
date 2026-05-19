;
; Wipe.asm
;
; Copyright (c) Microsoft Corporation. Licensed under the MIT license.
;

        TITLE   wipe.asm


        .686
        .xmm

_TEXT   SEGMENT PARA PUBLIC USE32 'CODE'
        ASSUME  CS:_TEXT, DS:FLAT, SS:FLAT

        PUBLIC  @SymCryptWipeAsm@8

;VOID
;SYMCRYPT_CALL
;SymCryptWipe( _Out_writes_bytes_( cbData )   PVOID  pbData,
;                                       SIZE_T cbData )

@SymCryptWipeAsm@8    PROC

        ;
        ; The .FPO provides debugging information for stack frames that do not use
        ; ebp as a base pointer.
        ; This stuff not well documented, 
        ; but here is the information I've gathered about the arguments to .FPO
        ; 
        ; In order:
        ; cdwLocals: Size of local variables, in DWords
        ; cdwParams: Size of parameters, in DWords. Given that this is all about
        ;            stack stuff, I'm assuming this is only about parameters passed
        ;            on the stack.
        ; cbProlog : Number of bytes in the prolog code. We sometimes interleaved the
        ;            prolog code with work for better performance. Most uses of
        ;            .FPO seem to set this value to 0.
        ;            The debugger seems to work if the prolog defined by this value
        ;            contains all the stack adjustments.
        ; cbRegs   : # registers saved in the prolog. 4 in our case
        ; fUseBP   : 0 if EBP is not used as base pointer, 1 if EBP is used as base pointer
        ; cbFrame  : Type of frame.
        ;            0 = FPO frame (no frame pointer)
        ;            1 = Trap frame (result of a CPU trap event)
        ;            2 = TSS frame
        ;
        ; Having looked at various occurrences of .FPO in the Windows code it
        ; seems to be used fairly sloppy, with lots of arguments left 0 even when
        ; they probably shouldn't be according to the spec.
        ;
        .FPO(0,0,0,0,0,0)       ; 3 byte prolog (covers esp adjustment only)
        
        ; ecx = pbData
        ; edx = cbData

        ;       
        ; This function will handle any alignment of pbData and any size, but it is optimized for
        ; the case where pbData is 4-aligned, and cbData is a multiple of 4.
        ;
        
        xor     eax,eax
        cmp     edx,8
        jb      SymCryptWipeAsmSmall     ; if cbData < 8, this is a rare case
        
        test    ecx,7
        jnz     SymCryptWipeAsmUnAligned; if data pointer is unaligned, we jump to the code that aligns the pointer
                                        ; For well-optimized code the aligned case is the common one, and that is
                                        ; the fall-through.
        
SymCryptWipeAsmAligned:
        ;
        ; Here ecx is aligned, and edx contains the # bytes left to wipe, and edx >= 8
        ;
        ; Our loop wipes in 8-byte increments; 
        ;
        sub     edx,8

        align   16
SymCryptWipeAsmLoop:
        mov     [ecx],eax
        mov     [ecx+4],eax           ; Wipe 8 bytes
        add     ecx,8
        sub     edx,8               
        ja      SymCryptWipeAsmLoop   ; Loop if we have >8 byte to wipe. <= 8 bytes is handled by the code below

        ;        
        ; The end of the buffer to wipe is at ecx + edx + 8 as 
        ; the number of bytes we wiped is 8 less than the number we subtracted from edx.
        ;
        ; We know we had 8 or more bytes to wipe, so we just wipe the last 8 bytes.
        ; This avoids a lot of conditional jumping etc.
        ;
        mov     [ecx+edx+8-8],eax
        mov     [ecx+edx+8-4],eax

SymCryptWipeAsmDone:        
        ret

SymCryptWipeAsmUnaligned:
        ;
        ; At this point we know that cbData(rdx) >= 8 and pbData(rcx) is unaligned. 
        ; We wipe 4 bytes and set our pointer & length to the remaining aligned buffer
        ;
        mov     [ecx],eax
        mov     [ecx+4],eax
        sub     eax,ecx
        and     eax,7
        add     ecx,eax
        sub     edx,eax
        xor     eax,eax
        
        ;
        ; edx = cbData could be less than 4 now. If it is, go directly to the
        ; tail wiping.
        ;        
        cmp     edx,8
        jae     SymCryptWipeAsmAligned  ; if cbData >= 8, do aligned wipes
        
SymCryptWipeAsmSmall:
        jmp     dword ptr [SymCryptWipeAsmSmallTable + 4*edx]
        
SymCryptWipeAsmT4:
        mov     [ecx],eax
SymCryptWipeAsmT0:        
        ret
        
SymCryptWipeAsmT1:
        mov     [ecx],al
        ret

SymCryptWipeAsmT3:
        mov     [ecx+2],al        
SymCryptWipeAsmT2:
        mov     [ecx],ax
        ret
        
SymCryptWipeAsmT5:
        mov     [ecx],eax
        mov     [ecx+1],eax
        ret
        
SymCryptWipeAsmT6:
        mov     [ecx],eax
        mov     [ecx+2],eax
        ret
        
SymCryptWipeAsmT7:
        mov     [ecx],eax
        mov     [ecx+3],eax
        ret
        
        
        align   4
SymCryptWipeAsmSmallTable   label   dword      
        dd      SymCryptWipeAsmT0
        dd      SymCryptWipeAsmT1
        dd      SymCryptWipeAsmT2
        dd      SymCryptWipeAsmT3
        dd      SymCryptWipeAsmT4
        dd      SymCryptWipeAsmT5
        dd      SymCryptWipeAsmT6
        dd      SymCryptWipeAsmT7
        
                                        
        
@SymCryptWipeAsm@8    ENDP

_TEXT           ENDS

END

