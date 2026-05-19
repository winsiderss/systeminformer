




















































































SymCryptModulusNdigitsOffsetAmd64 EQU 4;
SymCryptModulusInv64OffsetAmd64 EQU 24;
SymCryptModulusValueOffsetAmd64 EQU 128;
SymCryptNegDivisorSingleDigitOffsetAmd64 EQU 256;

SymCryptModulusNdigitsOffsetX86 EQU 4;
SymCryptModulusInv64OffsetX86 EQU 24;
SymCryptModulusValueOffsetX86 EQU 96;

SymCryptModulusNdigitsOffsetArm64 EQU 4;
SymCryptModulusInv64OffsetArm64 EQU 24;
SymCryptModulusValueOffsetArm64 EQU 128;

SymCryptModulusNdigitsOffsetArm EQU 4;
SymCryptModulusInv64OffsetArm EQU 24;
SymCryptModulusValueOffsetArm EQU 96;















































    



        
        
    




    
        include ksamd64.inc
    



    
    
    
    
    
    
    






























extern  SymCryptAesSboxMatrixMult:DWORD
extern  SymCryptAesInvSboxMatrixMult:DWORD
extern  SymCryptAesInvSbox:BYTE
extern  SymCryptFatal:NEAR








SYMCRYPT_CODE_VERSION EQU ((103 * 65536) + 11)
SYMCRYPT_MAGIC_CONSTANT EQU 53316D76h + SYMCRYPT_CODE_VERSION 

SYMCRYPT_CHECK_MAGIC MACRO  check_magic_label, ptr, struct_magic_offset, arg_1
        mov     rax, [ptr + struct_magic_offset]
        sub     rax, ptr
        cmp     rax, SYMCRYPT_MAGIC_CONSTANT
        jz      check_magic_label
        mov     arg_1, 6D616763h 
        call    SymCryptFatal
check_magic_label:
ENDM
















N_ROUND_KEYS_IN_AESKEY EQU 29
lastEncRoundKeyOffset EQU (29*16)
lastDecRoundKeyOffset EQU (29*16 + 8)
magicFieldOffset EQU (29*16 + 8 + 8)















ENC_MIX MACRO  keyptr
        
        
        
        
        
        
        

        
        
        
        
        
        
        
        
        
        
        
        

        movzx   esi,al
        mov     esi,[(r11 + 0) + 4 * rsi]
        movzx   edi,ah
        shr     eax,16
        mov     r8d,[(r11 + 1024) + 4 * rdi]
        movzx   ebp,al
        mov     ebp,[(r11 + 2048) + 4 * rbp]
        movzx   edi,ah
        mov     edi,[(r11 + 3072) + 4 * rdi]

        movzx   eax,bl
        xor     edi,[(r11 + 0) + 4 * rax]
        movzx   eax,bh
        shr     ebx,16
        xor     esi,[(r11 + 1024) + 4 * rax]
        movzx   eax,bl
        xor     r8d,[(r11 + 2048) + 4 * rax]
        movzx   eax,bh
        xor     ebp,[(r11 + 3072) + 4 * rax]

        movzx   eax,cl
        xor     ebp,[(r11 + 0) + 4 * rax]
        movzx   ebx,ch
        shr     ecx,16
        xor     edi,[(r11 + 1024) + 4 * rbx]
        movzx   eax,cl
        xor     esi,[(r11 + 2048) + 4 * rax]
        movzx   ebx,ch
        xor     r8d,[(r11 + 3072) + 4 * rbx]

        movzx   eax,dl
        xor     r8d,[(r11 + 0) + 4 * rax]
        movzx   ebx,dh
        shr     edx,16
        xor     ebp,[(r11 + 1024) + 4 * rbx]
        movzx   eax,dl
        xor     edi,[(r11 + 2048) + 4 * rax]
        movzx   ebx,dh
        xor     esi,[(r11 + 3072) + 4 * rbx]

        mov     eax, [keyptr]
        mov     ebx, [keyptr + 4]
        xor     eax, esi
        mov     ecx, [keyptr + 8]
        xor     ebx, edi
        mov     edx, [keyptr + 12]
        xor     ecx, ebp
        xor     edx, r8d
ENDM


DEC_MIX MACRO  keyptr
        
        
        
        
        
        

        movzx   esi,al
        mov     esi,[(r11 + 0) + 4 * rsi]
        movzx   edi,ah
        shr     eax,16
        mov     edi,[(r11 + 1024) + 4 * rdi]
        movzx   ebp,al
        mov     ebp,[(r11 + 2048) + 4 * rbp]
        movzx   eax,ah
        mov     r8d,[(r11 + 3072) + 4 * rax]

        movzx   eax,bl
        xor     edi,[(r11 + 0) + 4 * rax]
        movzx   eax,bh
        shr     ebx,16
        xor     ebp,[(r11 + 1024) + 4 * rax]
        movzx   eax,bl
        xor     r8d,[(r11 + 2048) + 4 * rax]
        movzx   eax,bh
        xor     esi,[(r11 + 3072) + 4 * rax]

        movzx   eax,cl
        xor     ebp,[(r11 + 0) + 4 * rax]
        movzx   ebx,ch
        shr     ecx,16
        xor     r8d,[(r11 + 1024) + 4 * rbx]
        movzx   eax,cl
        xor     esi,[(r11 + 2048) + 4 * rax]
        movzx   ebx,ch
        xor     edi,[(r11 + 3072) + 4 * rbx]

        movzx   eax,dl
        xor     r8d,[(r11 + 0) + 4 * rax]
        movzx   ebx,dh
        shr     edx,16
        xor     esi,[(r11 + 1024) + 4 * rbx]
        movzx   eax,dl
        xor     edi,[(r11 + 2048) + 4 * rax]
        movzx   ebx,dh
        xor     ebp,[(r11 + 3072) + 4 * rbx]

        mov     eax, [keyptr]
        mov     ebx, [keyptr + 4]
        xor     eax, esi
        mov     ecx, [keyptr + 8]
        xor     ebx, edi
        mov     edx, [keyptr + 12]
        xor     ecx, ebp
        xor     edx, r8d
ENDM

AES_ENCRYPT_MACRO MACRO  AesEncryptMacroLoopLabel
        
        
        
        
        
        
        
        
        
        

        
        
        
        xor     eax,[r9]
        xor     ebx,[r9+4]
        xor     ecx,[r9+8]
        xor     edx,[r9+12]

        add     r9,32

        
        

AesEncryptMacroLoopLabel:
        
        

        ENC_MIX r9-16

        cmp     r9,r10
        lea     r9,[r9+16]
        jc      AesEncryptMacroLoopLabel

        
        
        
        
        
        
        

        movzx   esi,al
        movzx   esi,byte ptr[r11 + 1 + 4*rsi]
        movzx   edi,ah
        shr     eax,16
        movzx   r8d,byte ptr[r11 + 1 + 4*rdi]
        movzx   ebp,al
        shl     r8d,8
        movzx   ebp,byte ptr[r11 + 1 + 4*rbp]
        shl     ebp,16
        movzx   edi,ah
        movzx   edi,byte ptr[r11 + 1 + 4*rdi]
        shl     edi,24

        movzx   eax,bl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        or      edi,eax
        movzx   eax,bh
        shr     ebx,16
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        shl     eax,8
        or      esi,eax
        movzx   eax,bl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        movzx   ebx,bh
        shl     eax,16
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        or      r8d,eax
        shl     ebx,24
        or      ebp,ebx

        movzx   eax,cl
        movzx   ebx,ch
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        shr     ecx,16
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     ebx,8
        or      ebp,eax
        or      edi,ebx
        movzx   eax,cl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        movzx   ebx,ch
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     eax,16
        shl     ebx,24
        or      esi,eax
        or      r8d,ebx

        movzx   eax,dl
        movzx   ebx,dh
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        shr     edx,16
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     ebx,8
        or      r8d,eax
        or      ebp,ebx
        movzx   eax,dl
        movzx   eax,byte ptr[r11 + 1 + 4*rax]
        movzx   ebx,dh
        movzx   ebx,byte ptr[r11 + 1 + 4*rbx]
        shl     eax,16
        shl     ebx,24
        or      edi,eax
        or      esi,ebx

        
        
        

        xor     r8d,[r10+12]
        xor     esi,[r10]
        xor     edi,[r10+4]
        xor     ebp,[r10+8]
ENDM

AES_DECRYPT_MACRO MACRO  AesDecryptMacroLoopLabel
        
        
        
        
        
        
        
        

        
        
        
        xor     eax,[r9]
        xor     ebx,[r9+4]
        xor     ecx,[r9+8]
        xor     edx,[r9+12]

        add     r9,32

        
        
AesDecryptMacroLoopLabel:
        
        

        DEC_MIX r9-16

        cmp     r9,r10
        lea     r9,[r9+16]
        jc      AesDecryptMacroLoopLabel

        
        
        
        

        movzx   esi,al
        movzx   esi,byte ptr[r12 + rsi]
        movzx   edi,ah
        shr     eax,16
        movzx   edi,byte ptr[r12 + rdi]
        movzx   ebp,al
        shl     edi,8
        movzx   ebp,byte ptr[r12 + rbp]
        shl     ebp,16
        movzx   eax,ah
        movzx   r8d,byte ptr[r12 + rax]
        shl     r8d,24

        movzx   eax,bl
        movzx   eax,byte ptr[r12 + rax]
        or      edi,eax
        movzx   eax,bh
        shr     ebx,16
        movzx   eax,byte ptr[r12 + rax]
        shl     eax,8
        or      ebp,eax
        movzx   eax,bl
        movzx   eax,byte ptr[r12 + rax]
        movzx   ebx,bh
        shl     eax,16
        movzx   ebx,byte ptr[r12 + rbx]
        or      r8d,eax
        shl     ebx,24
        or      esi,ebx

        movzx   eax,cl
        movzx   ebx,ch
        movzx   eax,byte ptr[r12 + rax]
        shr     ecx,16
        movzx   ebx,byte ptr[r12 + rbx]
        shl     ebx,8
        or      ebp,eax
        or      r8d,ebx
        movzx   eax,cl
        movzx   eax,byte ptr[r12 + rax]
        movzx   ebx,ch
        movzx   ebx,byte ptr[r12 + rbx]
        shl     eax,16
        shl     ebx,24
        or      esi,eax
        or      edi,ebx

        movzx   eax,dl
        movzx   ebx,dh
        movzx   eax,byte ptr[r12 + rax]
        shr     edx,16
        movzx   ebx,byte ptr[r12 + rbx]
        shl     ebx,8
        or      r8d,eax
        or      esi,ebx
        movzx   eax,dl
        movzx   eax,byte ptr[r12 + rax]
        movzx   ebx,dh
        movzx   ebx,byte ptr[r12 + rbx]
        shl     eax,16
        shl     ebx,24
        or      edi,eax
        or      ebp,ebx

        
        
        

        xor     esi,[r10]
        xor     edi,[r10+4]
        xor     ebp,[r10+8]
        xor     r8d,[r10+12]
ENDM



        
        
        

AES_ENCRYPT MACRO  loopLabel
        call    SymCryptAesEncryptAsmInternal
ENDM

AES_DECRYPT MACRO  loopLabel
        call    SymCryptAesDecryptAsmInternal
ENDM











LEAF_ENTRY SymCryptAesEncryptAsmInternal, _TEXT


        AES_ENCRYPT_MACRO SymCryptAesEncryptAsmInternalLoop


BEGIN_EPILOGUE
ret
LEAF_END SymCryptAesEncryptAsmInternal, _TEXT
















LEAF_ENTRY SymCryptAesDecryptAsmInternal, _TEXT


        AES_DECRYPT_MACRO SymCryptAesDecryptAsmInternalLoop


BEGIN_EPILOGUE
ret
LEAF_END SymCryptAesDecryptAsmInternal, _TEXT

























































































NESTED_ENTRY SymCryptAesEncryptAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 40

END_PROLOGUE


        SYMCRYPT_CHECK_MAGIC SymCryptAesEncryptAsmCheckMagic, rcx, magicFieldOffset, rcx

        
        
        
        
        
        
        

        mov     r10, [rcx + lastEncRoundKeyOffset]
        mov     r9, rcx

        mov     [rsp + 112 ], r8

        
        
        
        mov     eax,[rdx     ]
        mov     ebx,[rdx +  4]
        mov     ecx,[rdx +  8]
        mov     edx,[rdx + 12]

        lea     r11,[SymCryptAesSboxMatrixMult]

        AES_ENCRYPT SymCryptAesEncryptAsmLoop
        
        
        
        
        

        
        mov     rax,[rsp + 112 ]
        mov     [rax     ], esi
        mov     [rax +  4], edi
        mov     [rax +  8], ebp
        mov     [rax + 12], r8d


add rsp, 40
BEGIN_EPILOGUE
pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptAesEncryptAsm, _TEXT

































































































































NESTED_ENTRY SymCryptAesDecryptAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 40

END_PROLOGUE


        SYMCRYPT_CHECK_MAGIC SymCryptAesDecryptAsmCheckMagic, rcx, magicFieldOffset, rcx

        
        
        
        
        
        
        

        mov     r9,[rcx + lastEncRoundKeyOffset]
        mov     r10,[rcx + lastDecRoundKeyOffset]

        mov     [rsp + 112 ], r8

        mov     eax,[rdx     ]
        mov     ebx,[rdx +  4]
        mov     ecx,[rdx +  8]
        mov     edx,[rdx + 12]

        lea     r11,[SymCryptAesInvSboxMatrixMult]
        lea     r12,[SymCryptAesInvSbox]

        AES_DECRYPT SymCryptAesDecryptAsmLoop
        
        
        
        
        
        

        
        mov     rax,[rsp + 112 ]
        mov     [rax     ], esi
        mov     [rax +  4], edi
        mov     [rax +  8], ebp
        mov     [rax + 12], r8d


add rsp, 40
BEGIN_EPILOGUE
pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptAesDecryptAsm, _TEXT


































































































































NESTED_ENTRY SymCryptAesCbcEncryptAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 40

END_PROLOGUE

mov r10, [rsp + 144]

        
        
        
        
        
        
        

        SYMCRYPT_CHECK_MAGIC SymCryptAesCbcEncryptAsmCheckMagic, rcx, magicFieldOffset, rcx

        and     r10, NOT 15      
        jz      SymCryptAesCbcEncryptNoData

        mov     [rsp + 112 ], rdx    
        mov     rax, rdx                 
        mov     r13, r8                 

        mov     r15, r10                 
        mov     r14, r9                 

        add     r15, r8                 

        mov     r10,[rcx + lastEncRoundKeyOffset]    
        mov     r12,rcx                              

        
        
        
        mov     esi,[rax     ]
        mov     edi,[rax +  4]
        mov     ebp,[rax +  8]
        mov     r8d,[rax + 12]

        lea     r11,[SymCryptAesSboxMatrixMult]

align 16
SymCryptAesCbcEncryptAsmLoop:
        
        
        
        
        
        

        

        mov     eax, [r13]
        mov     r9, r12
        mov     ebx, [r13+4]
        xor     eax, esi
        mov     ecx, [r13+8]
        xor     ebx, edi
        xor     ecx, ebp
        mov     edx, [r13+12]
        xor     edx, r8d

        add     r13, 16


        AES_ENCRYPT SymCryptAesCbcEncryptAsmInnerLoop
        
        
        
        
        
        
        

        mov     [r14], esi
        mov     [r14+4], edi
        mov     [r14+8], ebp
        mov     [r14+12], r8d

        add     r14, 16

        cmp     r13, r15

        jb      SymCryptAesCbcEncryptAsmLoop


        
        
        
        mov     rax,[rsp + 112 ]
        mov     [rax], esi
        mov     [rax+4], edi
        mov     [rax+8], ebp
        mov     [rax+12], r8d

SymCryptAesCbcEncryptNoData:


add rsp, 40
BEGIN_EPILOGUE
pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptAesCbcEncryptAsm, _TEXT



































































































































NESTED_ENTRY SymCryptAesCbcDecryptAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 40

END_PROLOGUE

mov r10, [rsp + 144]

        
        
        
        
        
        
        

        SYMCRYPT_CHECK_MAGIC SymCryptAesCbcDecryptAsmCheckMagic, rcx, magicFieldOffset, rcx

        and     r10, NOT 15
        jz      SymCryptAesCbcDecryptNoData

        mov     [rsp + 112 ], rdx   
        mov     [rsp + 120 ], r8   

        lea     r14, [r10 - 16]
        lea     r15, [r9 + r14]         
        add     r14, r8                 

        mov     r13,[rcx + lastEncRoundKeyOffset]
        mov     r10,[rcx + lastDecRoundKeyOffset]

        lea     r11,[SymCryptAesInvSboxMatrixMult]
        lea     r12,[SymCryptAesInvSbox]

        
        
        
        mov     eax,[r14]
        mov     ebx,[r14+4]
        mov     ecx,[r14+8]
        mov     edx,[r14+12]

        mov     [rsp + 128   ], eax
        mov     [rsp + 128 +4], ebx
        mov     [rsp + 136   ], ecx
        mov     [rsp + 136 +4], edx

        jmp     SymCryptAesCbcDecryptAsmLoopEntry

align 16

SymCryptAesCbcDecryptAsmLoop:
        
        
        
        
        

        

        mov     eax,[r14-16]
        mov     ebx,[r14-12]
        xor     esi,eax
        mov     ecx,[r14-8]
        xor     edi,ebx
        mov     [r15],esi
        mov     edx,[r14-4]
        xor     ebp,ecx
        mov     [r15+4],edi
        xor     r8d,edx
        mov     [r15+8],ebp
        mov     [r15+12],r8d

        sub     r14,16
        sub     r15,16

SymCryptAesCbcDecryptAsmLoopEntry:

        mov     r9, r13

        AES_DECRYPT SymCryptAesCbcDecryptAsmInnerLoop
        
        
        
        
        
        
        
        

        cmp     r14, [rsp + 120 ]  
        ja      SymCryptAesCbcDecryptAsmLoop

        mov     rbx,[rsp + 112 ]   
        xor     esi,[rbx]
        xor     edi,[rbx+4]
        xor     ebp,[rbx+8]
        xor     r8d,[rbx+12]

        mov     [r15], esi
        mov     [r15+4], edi
        mov     [r15+8], ebp
        mov     [r15+12], r8d

        
        
        
        mov     rax,[rsp + 128 ]
        mov     rcx,[rsp + 136 ]
        mov     [rbx], rax
        mov     [rbx+8], rcx

SymCryptAesCbcDecryptNoData:


add rsp, 40
BEGIN_EPILOGUE
pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptAesCbcDecryptAsm, _TEXT


































































































































NESTED_ENTRY SymCryptAesCtrMsb64Asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 40

END_PROLOGUE

mov r10, [rsp + 144]

        
        
        
        
        
        
        

        SYMCRYPT_CHECK_MAGIC SymCryptAesCtrMsb64AsmCheckMagic, rcx, magicFieldOffset, rcx

        and     r10, NOT 15      
        jz      SymCryptAesCtrMsb64NoData

        mov     [rsp + 112 ], rdx   
        mov     rax, rdx         
        mov     r13, r8         
        mov     r14, r10         
        mov     r15, r9         
        add     r14, r8         

        mov     r10,[rcx + lastEncRoundKeyOffset]        
        mov     r12,rcx                                  


        lea     r11,[SymCryptAesSboxMatrixMult]

        
        
        
        mov     rcx, [rax + 8]
        mov     rax, [rax    ]

        
        
        
        mov     [rsp + 120 ], rax
        mov     [rsp + 128 ], rcx

        
        
        
        mov     rbx, rax
        mov     rdx, rcx
        shr     rbx, 32
        shr     rdx, 32

align 16
SymCryptAesCtrMsb64AsmLoop:
        
        
        
        
        
        
        
        
        

        mov     r9, r12

        AES_ENCRYPT SymCryptAesCtrMsb64AsmInnerLoop
        
        
        
        
        
        
        

        
        
        

        mov     eax,dword ptr [rsp + 120  + 0]
        mov     ebx,dword ptr [rsp + 120  + 4]
        mov     rcx,[rsp +  128  ]
        bswap   rcx
        add     rcx, 1
        bswap   rcx
        mov     [rsp + 128  ], rcx
        mov     rdx, rcx
        shr     rdx, 32

        
        
        
        
        
        
        

        xor     esi,[r13 + 0 ]
        xor     edi,[r13 + 4 ]
        xor     ebp,[r13 + 8 ]
        xor     r8d,[r13 + 12]

        

        mov     [r15 + 0], esi
        mov     [r15 + 4], edi
        mov     [r15 + 8], ebp
        mov     [r15 + 12], r8d

        add     r13, 16     
        add     r15, 16     

        cmp     r13, r14

        jb      SymCryptAesCtrMsb64AsmLoop

        
        
        
        mov     rsi,[rsp + 112 ]   
        mov     [rsi + 8], ecx
        mov     [rsi + 12], edx

        
        
        
        xor     rax, rax
        mov     [rsp + 120 ], rax
        mov     [rsp + 128 ], rax

SymCryptAesCtrMsb64NoData:


add rsp, 40
BEGIN_EPILOGUE
pop r15
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptAesCtrMsb64Asm, _TEXT





























































END
