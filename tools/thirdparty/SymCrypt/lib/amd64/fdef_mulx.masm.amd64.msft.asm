










































































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
    



    
    
    
    
    
    
    



























ZEROREG MACRO  R
        xor     R,R
ENDM

ZEROREG_8 MACRO  R0, R1, R2, R3, R4, R5, R6, R7
    ZEROREG R0
    ZEROREG R1
    ZEROREG R2
    ZEROREG R3
    ZEROREG R4
    ZEROREG R5
    ZEROREG R6
    ZEROREG R7
ENDM

MULADD18 MACRO  R0, R1, R2, R3, R4, R5, R6, R7, pD, pA, pB, T0, T1, QH
    
    
    

    xor     T0, T0      
                        

    mov     QH, [pB]
    adox    R0, [pD]

    mulx    T1, T0, [pA + 0 * 8]
    adcx    R0, T0
    adox    R1, T1

    mov     [pD], R0

    mulx    T1, T0, [pA + 1 * 8]
    adcx    R1, T0
    adox    R2, T1

    mulx    T1, T0, [pA + 2 * 8]
    adcx    R2, T0
    adox    R3, T1

    mulx    T1, T0, [pA + 3 * 8]
    adcx    R3, T0
    adox    R4, T1

    mulx    T1, T0, [pA + 4 * 8]
    adcx    R4, T0
    adox    R5, T1

    mulx    T1, T0, [pA + 5 * 8]
    adcx    R5, T0
    adox    R6, T1

    mulx    T1, T0, [pA + 6 * 8]
    adcx    R6, T0
    adox    R7, T1

    mulx    T1, T0, [pA + 7 * 8]
    adcx    R7, T0

    mov     R0, 0
    adox    R0, R0

    adcx    R0, T1
ENDM

MULADD88 MACRO  R0, R1, R2, R3, R4, R5, R6, R7, pD, pA, pB, T0, T1, QH
    
    

    MULADD18    R0, R1, R2, R3, R4, R5, R6, R7, pD     , pA, pB     , T0, T1, QH
    MULADD18    R1, R2, R3, R4, R5, R6, R7, R0, pD +  8, pA, pB +  8, T0, T1, QH
    MULADD18    R2, R3, R4, R5, R6, R7, R0, R1, pD + 16, pA, pB + 16, T0, T1, QH
    MULADD18    R3, R4, R5, R6, R7, R0, R1, R2, pD + 24, pA, pB + 24, T0, T1, QH
    MULADD18    R4, R5, R6, R7, R0, R1, R2, R3, pD + 32, pA, pB + 32, T0, T1, QH
    MULADD18    R5, R6, R7, R0, R1, R2, R3, R4, pD + 40, pA, pB + 40, T0, T1, QH
    MULADD18    R6, R7, R0, R1, R2, R3, R4, R5, pD + 48, pA, pB + 48, T0, T1, QH
    MULADD18    R7, R0, R1, R2, R3, R4, R5, R6, pD + 56, pA, pB + 56, T0, T1, QH
ENDM


HALF_SQUARE_NODIAG8 MACRO  R0, R1, R2, R3, R4, R5, R6, R7, pD, pA, T0, T1, QH
    
    
    

    

    mov     QH, [pA + 0 * 8]            
    mov     R1, [pD + 1 * 8]
    mov     R2, [pD + 2 * 8]
    mov     R3, [pD + 3 * 8]
    mov     R4, [pD + 4 * 8]
    mov     R5, [pD + 5 * 8]
    mov     R6, [pD + 6 * 8]
    mov     R7, [pD + 7 * 8]
    xor     R0, R0

    mulx    T1, T0, [pA + 1 * 8]
    adcx    R1, T0
    adox    R2, T1

    mulx    T1, T0, [pA + 2 * 8]
    adcx    R2, T0
    adox    R3, T1

    mulx    T1, T0, [pA + 3 * 8]
    adcx    R3, T0
    adox    R4, T1

    mulx    T1, T0, [pA + 4 * 8]
    adcx    R4, T0
    adox    R5, T1

    mulx    T1, T0, [pA + 5 * 8]
    adcx    R5, T0
    adox    R6, T1

    mulx    T1, T0, [pA + 6 * 8]
    adcx    R6, T0
    adox    R7, T1

    mulx    T1, T0, [pA + 7 * 8]
    adcx    R7, T0
    mov     [pD + 1 * 8], R1

    adox    R0, R0
    adcx    R0, T1
    mov     [pD + 2 * 8], R2
    mov     QH, [pA + 1 * 8]        

    
    xor     T0, T0      
                        

    mulx    T1, T0, [pA + 2 * 8]
    adcx    R3, T0
    adox    R4, T1

    mulx    T1, T0, [pA + 3 * 8]
    adcx    R4, T0
    adox    R5, T1

    mulx    T1, T0, [pA + 4 * 8]
    adcx    R5, T0
    adox    R6, T1

    mulx    T1, T0, [pA + 5 * 8]
    adcx    R6, T0
    adox    R7, T1

    mulx    T1, T0, [pA + 6 * 8]
    adcx    R7, T0
    adox    R0, T1

    mov     QH, [pA + 7 * 8]        
    mov     R1, 0
    mov     R2, 0
    mov     [pD + 3 * 8], R3

    mulx    T1, T0, [pA + 1 * 8]
    adcx    R0, T0
    adox    R1, T1                  

    mulx    T1, T0, [pA + 2 * 8]
    adcx    R1, T0
    mov     [pD + 4 * 8], R4

    adcx    R2, T1
    mov     QH, [pA + 2 * 8]        

    
    xor     T0, T0      
                        

    mulx    T1, T0, [pA + 3 * 8]
    adcx    R5, T0
    adox    R6, T1

    mulx    T1, T0, [pA + 4 * 8]
    adcx    R6, T0
    adox    R7, T1

    mulx    T1, T0, [pA + 5 * 8]
    adcx    R7, T0
    adox    R0, T1

    mulx    T1, T0, [pA + 6 * 8]
    adcx    R0, T0
    adox    R1, T1

    mov     QH, [pA + 4 * 8]        
    mov     R3, 0
    mov     R4, 0

    mulx    T1, T0, [pA + 5 * 8]
    adcx    R1, T0
    adox    R2, T1

    mulx    T1,T0, [pA + 6 * 8]
    adcx    R2, T0
    adox    R3, T1                  

    mov     QH, [pA + 5 * 8]        
    mov     [pD + 5 * 8], R5

    mulx    T1, T0, [pA + 6 * 8]
    adcx    R3, T0
    adcx    R4, T1

    mov     QH, [pA + 3 * 8]        
    mov     [pD + 6 * 8], R6

    
    xor     T0, T0      
                        

    mulx    T1, T0, [pA + 4 * 8]
    adcx    R7, T0
    adox    R0, T1

    mulx    T1, T0, [pA + 5 * 8]
    adcx    R0, T0
    adox    R1, T1

    mulx    T1, T0, [pA + 6 * 8]
    adcx    R1, T0
    adox    R2, T1

    mulx    T1, T0, [pA + 7 * 8]
    adcx    R2, T0
    adox    R3, T1

    mov     QH, [pA + 7 * 8]        
    mov     R5, 0
    mov     R6, 0
    mov     [pD + 7 * 8], R7

    mulx    T1, T0, [pA + 4 * 8]
    adcx    R3, T0
    adox    R4, T1

    mulx    T1, T0, [pA + 5 * 8]
    adcx    R4, T0
    adox    R5, T1                  

    mulx    T1, T0, [pA + 6 * 8]
    adcx    R5, T0
    adcx    R6, T1

    xor     R7, R7
ENDM

MONTGOMERY18 MACRO  R0, R1, R2, R3, R4, R5, R6, R7, modInv, pMod, pMont, T0, T1, QH
    
    
    
    

    mov     QH, R0
    imul    QH, modInv

    
    
    

    
    
    
    
    
    or      T0, -1          
    adcx    R0, T0          
    mov     R0, 0
    mov     [pMont], QH

    mulx    T1, T1, [pMod + 0 * 8]
    adox    R1, T1

    mulx    T1, T0, [pMod + 1 * 8]
    adcx    R1, T0
    adox    R2, T1

    mulx    T1, T0, [pMod + 2 * 8]
    adcx    R2, T0
    adox    R3, T1

    mulx    T1, T0, [pMod + 3 * 8]
    adcx    R3, T0
    adox    R4, T1

    mulx    T1, T0, [pMod + 4 * 8]
    adcx    R4, T0
    adox    R5, T1

    mulx    T1, T0, [pMod + 5 * 8]
    adcx    R5, T0
    adox    R6, T1

    mulx    T1, T0, [pMod + 6 * 8]
    adcx    R6, T0
    adox    R7, T1

    mulx    T1, T0, [pMod + 7 * 8]
    adcx    R7, T0

    adcx    R0, R0
    adox    R0, T1
ENDM

SYMCRYPT_SQUARE_DIAG MACRO  index, src_reg, dest_reg, T0, T1, T2, T3, QH
    mov     QH, [src_reg + 8 * index]
    mov     T0, [dest_reg + 16 * index]
    mov     T1, [dest_reg + 16 * index + 8]
    mulx    T3, T2, QH
    adcx    T2, T0
    adox    T2, T0
    adcx    T3, T1
    adox    T3, T1
    mov     [dest_reg + 16 * index], T2
    mov     [dest_reg + 16 * index + 8], T3
ENDM






































































NESTED_ENTRY SymCryptFdefRawMulMulx, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15

END_PROLOGUE

mov r10, rdx
mov r11, [rsp + 104]

        shl     r9, 6
        mov     [rsp + 72 ], r9
        mov     [rsp + 80 ], r10d

        
        mov     rsi, r11

        
        xorps   xmm0,xmm0               
        mov     rax, r9

SymCryptFdefRawMulMulxWipeLoop:
        movaps      [rsi],xmm0
        movaps      [rsi+16],xmm0            
        movaps      [rsi+32],xmm0            
        movaps      [rsi+48],xmm0            
        add         rsi, 64
        sub         rax, 64
        jnz         SymCryptFdefRawMulMulxWipeLoop


SymCryptFdefRawMulxOuterLoop:

        ZEROREG_8   rsi, rdi, rbp, rbx, r12, r13, r14, r15      

SymCryptFdefRawMulMulxInnerLoop:

        
        
        
        
        
        
        
        
        

        MULADD88  rsi, rdi, rbp, rbx, r12, r13, r14, r15, r11, rcx, r8, rax, r10, rdx

        add     r8, 64              
        add     r11, 64

        sub     r9d, 64                            
        jnz     SymCryptFdefRawMulMulxInnerLoop

        
        mov     [r11 + 0*8], rsi
        mov     [r11 + 1*8], rdi
        mov     [r11 + 2*8], rbp
        mov     [r11 + 3*8], rbx
        mov     [r11 + 4*8], r12
        mov     [r11 + 5*8], r13
        mov     [r11 + 6*8], r14
        mov     [r11 + 7*8], r15

        
        
        mov     r9, [rsp + 72 ]

        
        sub     r11, r9
        add     r11, 64
        
        sub     r8, r9

        
        add     rcx, 64

        
        mov     r10d, [rsp + 80 ]
        sub     r10d, 1                              
        mov     [rsp + 80 ], r10d
        jnz     SymCryptFdefRawMulxOuterLoop


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
NESTED_END SymCryptFdefRawMulMulx, _TEXT
































































































































NESTED_ENTRY SymCryptFdefRawSquareMulx, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15

END_PROLOGUE

mov r10, rdx

        
        
        

        

        mov     [rsp + 72 ], rcx   
        mov     [rsp + 80 ], r10   
        mov     [rsp + 88 ], r8   

        shl     r10, 6       
        mov     [rsp + 96 ], r10   

        
        xor     rax, rax
        mov     r11, r8
        mov     r9, r10

SymCryptFdefRawSquareMulxWipeLoop:
        
        
        mov     [r11     ], rax
        mov     [r11 +  8], rax
        mov     [r11 + 16], rax
        mov     [r11 + 24], rax
        mov     [r11 + 32], rax
        mov     [r11 + 40], rax
        mov     [r11 + 48], rax
        mov     [r11 + 56], rax
        add     r11, 64
        sub     r9, 64
        jnz     SymCryptFdefRawSquareMulxWipeLoop

        

SymCryptFdefRawSquareMulxOuterLoop:

        HALF_SQUARE_NODIAG8 rsi, rdi, rbp, rbx, r12, r13, r14, r15, r8, rcx, rax, r9, rdx

        sub     r10, 64
        jz      SymCryptFdefRawSquareMulxPhase2     

        lea     r11, [rcx + 64]
        lea     r8, [r8 + 64]

SymCryptFdefRawSquareMulxInnerLoop:
        
        
        
        
        
        
        

        
        
        
        

        MULADD88    rsi, rdi, rbp, rbx, r12, r13, r14, r15, r8, rcx, r11, rax, r9, rdx

        add     r8, 64
        add     r11, 64

        sub     r10, 64                  
        jnz     SymCryptFdefRawSquareMulxInnerLoop

        
        mov     [r8 + 0*8], rsi
        mov     [r8 + 1*8], rdi
        mov     [r8 + 2*8], rbp
        mov     [r8 + 3*8], rbx
        mov     [r8 + 4*8], r12
        mov     [r8 + 5*8], r13
        mov     [r8 + 6*8], r14
        mov     [r8 + 7*8], r15

        mov     r10, [rsp + 96 ]   

        add     rcx, 64                                  
        sub     r8, r10                                  
        add     r8, 128                                 

        sub     r10, 64                                  
        mov     [rsp + 96 ], r10

        jmp     SymCryptFdefRawSquareMulxOuterLoop


SymCryptFdefRawSquareMulxPhase2:
        

        
        mov     [r8 +  8*8], rsi
        mov     [r8 +  9*8], rdi
        mov     [r8 + 10*8], rbp
        mov     [r8 + 11*8], rbx
        mov     [r8 + 12*8], r12
        mov     [r8 + 13*8], r13
        mov     [r8 + 14*8], r14
        mov     [r8 + 15*8], r15

        

        mov     rcx, [rsp + 72 ]
        mov     r10, [rsp + 80 ]
        mov     r8, [rsp + 88 ]

        
        
        
        xor     rax, rax
        xor     r9, r9

SymCryptFdefRawSquareMulxDiagonalsLoop:
        
        
        

        
        
        mov     rdx, [rcx]
        mov     r11, [r8]
        mov     rsi, [r8 + 8]
        mulx    rbp, rdi, rdx
        adcx    rdi, rax              
        adcx    rbp, r9              

        adcx    rdi, r11
        adox    rdi, r11
        adcx    rbp, rsi
        adox    rbp, rsi
        mov     [r8], rdi
        mov     [r8 + 8], rbp

        SYMCRYPT_SQUARE_DIAG 1, rcx, r8, r11, rsi, rdi, rbp, rdx
        SYMCRYPT_SQUARE_DIAG 2, rcx, r8, r11, rsi, rdi, rbp, rdx
        SYMCRYPT_SQUARE_DIAG 3, rcx, r8, r11, rsi, rdi, rbp, rdx
        SYMCRYPT_SQUARE_DIAG 4, rcx, r8, r11, rsi, rdi, rbp, rdx
        SYMCRYPT_SQUARE_DIAG 5, rcx, r8, r11, rsi, rdi, rbp, rdx
        SYMCRYPT_SQUARE_DIAG 6, rcx, r8, r11, rsi, rdi, rbp, rdx
        SYMCRYPT_SQUARE_DIAG 7, rcx, r8, r11, rsi, rdi, rbp, rdx

        
        mov     eax, r9d
        adox    eax, r9d

        
        
        

        lea     rcx, [rcx + 64]
        lea     r8, [r8 + 128]
        dec     r10
        jnz     SymCryptFdefRawSquareMulxDiagonalsLoop


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
NESTED_END SymCryptFdefRawSquareMulx, _TEXT































































































































NESTED_ENTRY SymCryptFdefMontgomeryReduceMulx, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15

END_PROLOGUE

mov r10, rdx

        mov     [rsp + 72 ], rcx
        mov     [rsp + 80 ], r10
        mov     [rsp + 88 ], r8

        mov     eax, [rcx + SymCryptModulusNdigitsOffsetAmd64]
        mov     [rsp + 96 ], eax
        

        xor     r9d, r9d
        mov     [rsp + 96  + 4], r9d
        

SymCryptFdefMontgomeryReduceMulxOuterLoop:
        
        
        mov     rsi, [r10 + 0 * 8]
        mov     rdi, [r10 + 1 * 8]
        mov     rbp, [r10 + 2 * 8]
        mov     rbx, [r10 + 3 * 8]
        mov     r12, [r10 + 4 * 8]
        mov     r13, [r10 + 5 * 8]
        mov     r14, [r10 + 6 * 8]
        mov     r15, [r10 + 7 * 8]

        mov     r8, [rcx + SymCryptModulusInv64OffsetAmd64]      
        mov     r9d, [rcx + SymCryptModulusNdigitsOffsetAmd64]
        lea     rcx, [rcx + SymCryptModulusValueOffsetAmd64]      

        
        
        
        

        MONTGOMERY18    rsi, rdi, rbp, rbx, r12, r13, r14, r15,  r8, rcx, r10 + (0 * 8), rax, r11, rdx
        MONTGOMERY18    rdi, rbp, rbx, r12, r13, r14, r15, rsi,  r8, rcx, r10 + (1 * 8), rax, r11, rdx
        MONTGOMERY18    rbp, rbx, r12, r13, r14, r15, rsi, rdi,  r8, rcx, r10 + (2 * 8), rax, r11, rdx
        MONTGOMERY18    rbx, r12, r13, r14, r15, rsi, rdi, rbp,  r8, rcx, r10 + (3 * 8), rax, r11, rdx
        MONTGOMERY18    r12, r13, r14, r15, rsi, rdi, rbp, rbx,  r8, rcx, r10 + (4 * 8), rax, r11, rdx
        MONTGOMERY18    r13, r14, r15, rsi, rdi, rbp, rbx, r12,  r8, rcx, r10 + (5 * 8), rax, r11, rdx
        MONTGOMERY18    r14, r15, rsi, rdi, rbp, rbx, r12, r13,  r8, rcx, r10 + (6 * 8), rax, r11, rdx
        MONTGOMERY18    r15, rsi, rdi, rbp, rbx, r12, r13, r14,  r8, rcx, r10 + (7 * 8), rax, r11, rdx

        
        

        mov     r8, r10         
        add     rcx, 64
        add     r10, 64

        dec     r9d
        jz      SymCryptFdefMontgomeryReduceMulxInnerLoopDone

SymCryptFdefMontgomeryReduceMulxInnerLoop:

        
        
        
        
        
        
        

        MULADD88    rsi, rdi, rbp, rbx, r12, r13, r14, r15,  r10, rcx, r8, rax, r11, rdx
            
            
            

        add     rcx, 64
        add     r10, 64
        dec     r9d
        jnz     SymCryptFdefMontgomeryReduceMulxInnerLoop


SymCryptFdefMontgomeryReduceMulxInnerLoopDone:

        
        
        mov     r11d, [rsp + 96  + 4]
        
        neg     r11d

        
        mov     rax, [r10 + 0 * 8]
        adc     rax, rsi
        mov     [r10 + 0 * 8], rax

        mov     r11, [r10 + 1 * 8]
        adc     r11, rdi
        mov     [r10 + 1 * 8], r11

        mov     rax, [r10 + 2 * 8]
        adc     rax, rbp
        mov     [r10 + 2 * 8], rax

        mov     r11, [r10 + 3 * 8]
        adc     r11, rbx
        mov     [r10 + 3 * 8], r11

        mov     rax, [r10 + 4 * 8]
        adc     rax, r12
        mov     [r10 + 4 * 8], rax

        mov     r11, [r10 + 5 * 8]
        adc     r11, r13
        mov     [r10 + 5 * 8], r11

        mov     rax, [r10 + 6 * 8]
        adc     rax, r14
        mov     [r10 + 6 * 8], rax

        mov     r11, [r10 + 7 * 8]
        adc     r11, r15
        mov     [r10 + 7 * 8], r11

        adc     r9d, r9d                
        mov     [rsp + 96  + 4], r9d

        mov     r10, [rsp + 80 ]
        add     r10, 64
        mov     [rsp + 80 ], r10

        mov     rcx, [rsp + 72 ]

        mov     eax, [rsp + 96 ]
        dec     eax
        mov     [rsp + 96 ], eax

        jnz     SymCryptFdefMontgomeryReduceMulxOuterLoop

        

        mov     esi, [rcx + SymCryptModulusNdigitsOffsetAmd64]
        lea     rcx, [rcx + SymCryptModulusValueOffsetAmd64]                    

        mov     r8, [rsp + 88 ]

        
        
        
        

        
        mov     edi, esi      
        mov     rbp, r10      
        mov     rbx, r8      

        

SymCryptFdefMontgomeryReduceMulxSubLoop:
        mov     rax,[r10 + 0 * 8]
        sbb     rax,[rcx + 0 * 8]
        mov     [r8 + 0 * 8], rax

        mov     r11,[r10 + 1 * 8]
        sbb     r11,[rcx + 1 * 8]
        mov     [r8 + 1 * 8], r11

        mov     rax,[r10 + 2 * 8]
        sbb     rax,[rcx + 2 * 8]
        mov     [r8 + 2 * 8], rax

        mov     r11,[r10 + 3 * 8]
        sbb     r11,[rcx + 3 * 8]
        mov     [r8 + 3 * 8], r11

        mov     rax,[r10 + 4 * 8]
        sbb     rax,[rcx + 4 * 8]
        mov     [r8 + 4 * 8], rax

        mov     r11,[r10 + 5 * 8]
        sbb     r11,[rcx + 5 * 8]
        mov     [r8 + 5 * 8], r11

        mov     rax,[r10 + 6 * 8]
        sbb     rax,[rcx + 6 * 8]
        mov     [r8 + 6 * 8], rax

        mov     r11,[r10 + 7 * 8]
        sbb     r11,[rcx + 7 * 8]
        mov     [r8 + 7 * 8], r11

        lea     r10, [r10 + 64]
        lea     rcx, [rcx + 64]
        lea     r8, [r8 + 64]
        dec     esi
        jnz     SymCryptFdefMontgomeryReduceMulxSubLoop

        
        
        sbb     r9d, 0
        

        movd    xmm0, r9d           
        pcmpeqd xmm1, xmm1          
        pshufd  xmm0, xmm0, 0       
        pxor    xmm1, xmm0          

SymCryptFdefMontgomeryReduceMulxMaskedCopyLoop:
        movdqa  xmm2, [rbp + 0 * 16]    
        movdqa  xmm3, [rbx + 0 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [rbx + 0 * 16], xmm2

        movdqa  xmm2, [rbp + 1 * 16]    
        movdqa  xmm3, [rbx + 1 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [rbx + 1 * 16], xmm2

        movdqa  xmm2, [rbp + 2 * 16]    
        movdqa  xmm3, [rbx + 2 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [rbx + 2 * 16], xmm2

        movdqa  xmm2, [rbp + 3 * 16]    
        movdqa  xmm3, [rbx + 3 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [rbx + 3 * 16], xmm2

        

        add     rbp, 64
        add     rbx, 64
        dec     edi
        jnz     SymCryptFdefMontgomeryReduceMulxMaskedCopyLoop


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
NESTED_END SymCryptFdefMontgomeryReduceMulx, _TEXT





























































MULADD_LOADSTORE18 MACRO  pS, pM, pD, QH, Tc, T0, T1
    
    
    
    
    

    xor     T0, T0 

    mulx    T1, T0, [pM + 0 * 8]
    adox    T0, Tc
    adcx    T0, [pS + 0 * 8]
    mov     [pD + 0 * 8], T0

    mulx    Tc, T0, [pM + 1 * 8]
    adox    T0, T1
    adcx    T0, [pS + 1 * 8]
    mov     [pD + 1 * 8], T0

    mulx    T1, T0, [pM + 2 * 8]
    adox    T0, Tc
    adcx    T0, [pS + 2 * 8]
    mov     [pD + 2 * 8], T0

    mulx    Tc, T0, [pM + 3 * 8]
    adox    T0, T1
    adcx    T0, [pS + 3 * 8]
    mov     [pD + 3 * 8], T0

    mulx    T1, T0, [pM + 4 * 8]
    adox    T0, Tc
    adcx    T0, [pS + 4 * 8]
    mov     [pD + 4 * 8], T0

    mulx    Tc, T0, [pM + 5 * 8]
    adox    T0, T1
    adcx    T0, [pS + 5 * 8]
    mov     [pD + 5 * 8], T0

    mulx    T1, T0, [pM + 6 * 8]
    adox    T0, Tc
    adcx    T0, [pS + 6 * 8]
    mov     [pD + 6 * 8], T0

    mulx    Tc, T0, [pM + 7 * 8]
    adox    T0, T1
    adcx    T0, [pS + 7 * 8]
    mov     [pD + 7 * 8], T0

    mov     T1, 0
    adox    Tc, T1 
    adcx    Tc, T1 
ENDM

SHIFTRIGHT2 MACRO  pD, index, shrVal, shrMask, shlVal, Tc, T0, T1
    
    
    
    
    
    
    
    
    
    
    
    

    mov     T0, [pD + ((index+1)*8)]
    shrx    T1, T0, shrVal
    shlx    Tc, Tc, shlVal
    and     T1, shrMask
    or      T1, Tc
    mov     [pD + ((index+1)*8)], T1

    mov     Tc, [pD + (index*8)]
    shrx    T1, Tc, shrVal
    shlx    T0, T0, shlVal
    and     T1, shrMask
    or      T1, T0
    mov     [pD + (index*8)], T1
ENDM

















































NESTED_ENTRY SymCryptFdefModDivSmallPow2Mulx, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp

END_PROLOGUE

mov r10, rdx

        mov     rdi, [r10]
        mov     rdx, [rcx + SymCryptModulusInv64OffsetAmd64]      

        imul    rdx, rdi  
                        
                        

        
        xor     eax, eax
        mov     rdi, -1
        sub     eax, r8d
        shrx    rdi, rdi, rax      
                                
                                

        and     rdx, rdi          
                                
                                

        mov     eax, [rcx + SymCryptModulusNdigitsOffsetAmd64]            
        lea     r11, [rcx + SymCryptModulusValueOffsetAmd64]              

        xor     rsi, rsi 

SymCryptFdefModDivSmallPow2MulxMulAddLoop:
        
        
        
        
        
        
        
        
        
        

        MULADD_LOADSTORE18 r10, r11, r9, rdx, rsi, rdi, rbp

        
        add     r10, 64
        add     r11, 64
        add     r9, 64

        dec     eax
        jnz     SymCryptFdefModDivSmallPow2MulxMulAddLoop

        mov     eax, [rcx + SymCryptModulusNdigitsOffsetAmd64]            
        mov     ecx, 64
        xor     rdi, rdi
        sub     ecx, r8d  
        mov     r10, -1
        cmovz   r10, rdi  

        sub     r9, 64  

SymCryptFdefModDivSmallPow2MulxShiftRightLoop:
        
        
        
        
        
        
        
        
        
        
        

        SHIFTRIGHT2 r9, 6, r8, r10, rcx, rsi, rdi, rbp
        SHIFTRIGHT2 r9, 4, r8, r10, rcx, rsi, rdi, rbp
        SHIFTRIGHT2 r9, 2, r8, r10, rcx, rsi, rdi, rbp
        SHIFTRIGHT2 r9, 0, r8, r10, rcx, rsi, rdi, rbp
        
        sub     r9, 64

        dec     eax
        jnz     SymCryptFdefModDivSmallPow2MulxShiftRightLoop


BEGIN_EPILOGUE
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefModDivSmallPow2Mulx, _TEXT



























































































NESTED_ENTRY SymCryptFdefModAddMulx256Asm, _TEXT

rex_push_reg rsi
push_reg rdi

END_PROLOGUE


        
        
        
        

        add     rcx, SymCryptNegDivisorSingleDigitOffsetAmd64

        
        

        xor     rax, rax 

        mov     rax, [rdx + 0*8]
        adcx    rax, [r8 + 0*8]

        mov     r10, [rdx + 1*8]
        adcx    r10, [r8 + 1*8]

        mov     r11, [rdx + 2*8]
        adcx    r11, [r8 + 2*8]

        mov     rdx, [rdx + 3*8]
        adcx    rdx, [r8 + 3*8]

        mov     rsi, [rcx + 0*8]
        adox    rsi, rax

        mov     rdi, [rcx + 1*8]
        adox    rdi, r10

        mov     r8, [rcx + 2*8]
        adox    r8, r11

        mov     rcx, [rcx + 3*8]
        adox    rcx, rdx

        
        
        
        

        cmovc   rax, rsi
        cmovc   r10, rdi
        cmovc   r11, r8
        cmovc   rdx, rcx

        cmovo   rax, rsi
        cmovo   r10, rdi
        cmovo   r11, r8
        cmovo   rdx, rcx

        mov     [r9 + 0*8], rax
        mov     [r9 + 1*8], r10
        mov     [r9 + 2*8], r11
        mov     [r9 + 3*8], rdx


BEGIN_EPILOGUE
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefModAddMulx256Asm, _TEXT





































MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE MACRO  T0, T1, QH, pA, Aoff, pB, pM, K, R0, R1, R2, R3, R4, R5
    
    
    
    
    
    
    
    

    mov     QH, [pA + Aoff]
    xor     T0, T0      
                        

    mulx    T1, T0, [pB + 0*8]
    adox    R1, T0
    adcx    R2, T1

    mulx    T1, T0, [pB + 1*8]
    adox    R2, T0
    adcx    R3, T1

    mulx    T1, T0, [pB + 2*8]
    adox    R3, T0
    adcx    R4, T1

    mulx    T1, T0, [pB + 3*8]
    adox    R4, T0
    mov     T0, 0
    adcx    T1, T0      

    adox    R5, T0      
                        
                        

    xor     T0, T0      

    mov     QH, K       

    mulx    K, T0, [pM + 0*8]
    adcx    R0, T0      
    adox    R1, K

    mulx    K, T0, [pM + 1*8]
    adcx    R1, T0
    adox    R2, K

    mulx    K, T0, [pM + 2*8]
    adcx    R2, T0
    adox    R3, K

    mulx    K, T0, [pM + 3*8]
    adcx    R3, T0
    adox    R4, K       

    adcx    R4, R0      
    adox    R5, R0      

    adc     R5, T1      

    adc     R0, R0

    mov     K, R1
    imul    K, [pM - SymCryptModulusValueOffsetAmd64 + SymCryptModulusInv64OffsetAmd64]
ENDM












altentry SymCryptFdefModMulMontgomeryMulx256AsmInternal



























































NESTED_ENTRY SymCryptFdefModMulMontgomeryMulx256Asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14

END_PROLOGUE

mov r10, rdx

        
        
        
        

ALTERNATE_ENTRY SymCryptFdefModMulMontgomeryMulx256AsmInternal

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        xor     r13, r13  
        mov     rdx, [r10]

        mulx    rdi, rsi,  [r8 + 0*8]

        mulx    rbp, rax,  [r8 + 1*8]
        adc     rdi, rax        
                              
                              

        mulx    rbx, rax,  [r8 + 2*8]
        adc     rbp, rax

        mulx    r12, rax, [r8 + 3*8]
        adc     rbx, rax

        adc     r12, r13      

        
        mov     r14, rsi
        imul    r14, [rcx + SymCryptModulusInv64OffsetAmd64]
        add     rcx, SymCryptModulusValueOffsetAmd64

        MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE rax, r11, rdx, r10,  8, r8, rcx, r14, rsi, rdi, rbp, rbx, r12, r13

        MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE rax, r11, rdx, r10, 16, r8, rcx, r14, rdi, rbp, rbx, r12, r13, rsi

        MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE rax, r11, rdx, r10, 24, r8, rcx, r14, rbp, rbx, r12, r13, rsi, rdi

        xor     rax, rax      
        mov     rdx, r14

        mulx    r11, rax, [rcx + 0*8]
        adcx    rbx, rax      
        adox    r12, r11

        mulx    r11, rax, [rcx + 1*8]
        adcx    r12, rax
        adox    r13, r11

        mulx    r11, rax, [rcx + 2*8]
        adcx    r13, rax
        adox    rsi, r11

        mulx    r11, rax, [rcx + 3*8]
        adcx    rsi, rax
        adox    rdi, r11      

        mov     rax, 0
        adcx    rdi, rax      
        adox    rbp, rax

        adc     rbp, rax

        
        

        add     rcx, SymCryptNegDivisorSingleDigitOffsetAmd64 - SymCryptModulusValueOffsetAmd64

        mov     rax, [rcx + 0*8]
        add     rax, r12
        mov     r10, [rcx + 1*8]
        adc     r10, r13
        mov     r8, [rcx + 2*8]
        adc     r8, rsi
        mov     r11, [rcx + 3*8]
        adc     r11, rdi

        
        
        
        

        adc     rbp, rbp  
                        
                        
                        

        cmovnz  r12, rax
        cmovnz  r13, r10
        cmovnz  rsi, r8
        cmovnz  rdi, r11

        mov     [r9 + 0*8], r12
        mov     [r9 + 1*8], r13
        mov     [r9 + 2*8], rsi
        mov     [r9 + 3*8], rdi


BEGIN_EPILOGUE
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefModMulMontgomeryMulx256Asm, _TEXT



























































































































NESTED_ENTRY SymCryptFdefModSquareMontgomeryMulx256Asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14

END_PROLOGUE

mov r10, rdx

        
        
        

        mov     r9, r8
        mov     r8, r10

        
        
        

        
        test    rsp,rsp
        jne     SymCryptFdefModMulMontgomeryMulx256AsmInternal       

        int     3       
                        
                        


BEGIN_EPILOGUE
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefModSquareMontgomeryMulx256Asm, _TEXT



























































































































NESTED_ENTRY SymCryptFdefModAddMulx384Asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13

END_PROLOGUE


        
        
        
        

        add     rcx, SymCryptNegDivisorSingleDigitOffsetAmd64

        
        

        xor     rax, rax  

        mov     rax, [rdx + 0*8]
        adcx    rax, [r8 + 0*8]

        mov     r10, [rdx + 1*8]
        adcx    r10, [r8 + 1*8]

        mov     r11, [rdx + 2*8]
        adcx    r11, [r8 + 2*8]

        mov     rsi, [rdx + 3*8]
        adcx    rsi, [r8 + 3*8]

        mov     rdi, [rdx + 4*8]
        adcx    rdi, [r8 + 4*8]

        mov     rdx, [rdx + 5*8]
        adcx    rdx, [r8 + 5*8]

        mov     rbp,  [rcx + 0*8]
        adox    rbp,  rax

        mov     rbx, [rcx + 1*8]
        adox    rbx, r10

        mov     r12, [rcx + 2*8]
        adox    r12, r11

        mov     r13, [rcx + 3*8]
        adox    r13, rsi

        mov     r8,  [rcx + 4*8]
        adox    r8,  rdi

        mov     rcx,  [rcx + 5*8]
        adox    rcx,  rdx

        
        
        
        

        cmovc   rax, rbp
        cmovc   r10, rbx
        cmovc   r11, r12
        cmovc   rsi, r13
        cmovc   rdi, r8
        cmovc   rdx, rcx

        cmovo   rax, rbp
        cmovo   r10, rbx
        cmovo   r11, r12
        cmovo   rsi, r13
        cmovo   rdi, r8
        cmovo   rdx, rcx

        mov     [r9 + 0*8], rax
        mov     [r9 + 1*8], r10
        mov     [r9 + 2*8], r11
        mov     [r9 + 3*8], rsi
        mov     [r9 + 4*8], rdi
        mov     [r9 + 5*8], rdx


BEGIN_EPILOGUE
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefModAddMulx384Asm, _TEXT





















































MUL16_P384 MACRO  T0, T1, QH, pA, Aoff, pB, R0, R1, R2, R3, R4, R5, R6, R7
    
    
    
    

    mov     QH, [pA + Aoff]
    xor     R7, R7      
                        

    mulx    T1, T0, [pB + 0*8]
    adcx    R0, T0
    adox    R1, T1

    mulx    T1, T0, [pB + 1*8]
    adcx    R1, T0
    adox    R2, T1

    mulx    T1, T0, [pB + 2*8]
    adcx    R2, T0
    adox    R3, T1

    mulx    T1, T0, [pB + 3*8]
    adcx    R3, T0
    adox    R4, T1

    mulx    T1, T0, [pB + 4*8]
    adcx    R4, T0
    adox    R5, T1

    mulx    T1, T0, [pB + 5*8]
    adcx    R5, T0
    adox    R6, R7      


    adc     R6, T1      
    adc     R7, R7
ENDM

MONT16_P384 MACRO  T0, T1, QH, pM, N4, R0, R1, R2, R3, R4, R5, R6, R7
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    mov     QH, R0
    imul    QH, [pM - SymCryptModulusValueOffsetAmd64 + SymCryptModulusInv64OffsetAmd64]

    add     N4, -1
    sbb     R3, QH
    sbb     N4, N4      

    xor     T0, T0      

    mulx    T1, T0, [pM + 0*8]
    adcx    R0, T0      
    adox    R1, T1

    mulx    T1, T0, [pM + 1*8]
    adcx    R1, T0
    adox    R2, T1

    mulx    T1, T0, [pM + 2*8]
    adcx    R2, T0
    adox    R3, T1

    adcx    R3, R0
    adox    N4, R0

    adc     N4, R0      
                        

    add     R6, QH
    adc     R7, R0

ENDM


altentry SymCryptFdefModMulMontgomeryMulxP384AsmInternal









































































NESTED_ENTRY SymCryptFdefModMulMontgomeryMulxP384Asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15

END_PROLOGUE

mov r10, rdx

        
        
        
        

ALTERNATE_ENTRY SymCryptFdefModMulMontgomeryMulxP384AsmInternal

        mov     [rsp + 72 ], r9   

        
        
        
        
        
        
        
        

        xor     r15, r15  
        mov     rdx, [r10]

        mulx    rdi, rsi,  [r8 + 0*8]

        mulx    rbp, rax,  [r8 + 1*8]
        adc     rdi, rax        
                              
                              

        mulx    rbx, rax,  [r8 + 2*8]
        adc     rbp, rax

        mulx    r12, rax, [r8 + 3*8]
        adc     rbx, rax

        mulx    r13, rax, [r8 + 4*8]
        adc     r12, rax

        mulx    r14, rax, [r8 + 5*8]
        adc     r13, rax

        adc     r14, r15        
        add     rcx, SymCryptModulusValueOffsetAmd64
        xor     r9, r9

        MONT16_P384     rax, r11, rdx, rcx, r9,     rsi, rdi, rbp, rbx, r12, r13, r14, r15

        MUL16_P384      rax, r11, rdx, r10,  8, r8, rdi, rbp, rbx, r12, r13, r14, r15, rsi
        MONT16_P384     rax, r11, rdx, rcx, r9,     rdi, rbp, rbx, r12, r13, r14, r15, rsi

        MUL16_P384      rax, r11, rdx, r10, 16, r8, rbp, rbx, r12, r13, r14, r15, rsi, rdi
        MONT16_P384     rax, r11, rdx, rcx, r9,     rbp, rbx, r12, r13, r14, r15, rsi, rdi

        MUL16_P384      rax, r11, rdx, r10, 24, r8, rbx, r12, r13, r14, r15, rsi, rdi, rbp
        MONT16_P384     rax, r11, rdx, rcx, r9,     rbx, r12, r13, r14, r15, rsi, rdi, rbp

        MUL16_P384      rax, r11, rdx, r10, 32, r8, r12, r13, r14, r15, rsi, rdi, rbp, rbx
        MONT16_P384     rax, r11, rdx, rcx, r9,     r12, r13, r14, r15, rsi, rdi, rbp, rbx

        MUL16_P384      rax, r11, rdx, r10, 40, r8, r13, r14, r15, rsi, rdi, rbp, rbx, r12
        MONT16_P384     rax, r11, rdx, rcx, r9,     r13, r14, r15, rsi, rdi, rbp, rbx, r12

        xor     rax, rax

        adox    rdi, r9  
                        
        adox    rbp, r9
        adox    rbx, r9
        adox    r12, r9

        
        
        

        mov     r10,  [rcx + SymCryptNegDivisorSingleDigitOffsetAmd64 - SymCryptModulusValueOffsetAmd64 + 0*8]
        add     r10,  r14
        mov     r8,  [rcx + 0*8] 
        adc     r8,  r15
        mov     r11,  1  
        adc     r11,  rsi
        mov     rcx,  rax 
        adc     rax,  rdi
        adc     rcx,  rbp
        adc     r13, rbx

        
        
        
        

        adc     r12, r12        
                                
                                
                                

        cmovnz  r14, r10
        cmovnz  r15, r8
        cmovnz  rsi,  r11
        cmovnz  rdi,  rax
        cmovnz  rbp,  rcx
        cmovnz  rbx,  r13

        mov     r9, [rsp + 72 ]   

        mov     [r9 + 0*8], r14
        mov     [r9 + 1*8], r15
        mov     [r9 + 2*8], rsi
        mov     [r9 + 3*8], rdi
        mov     [r9 + 4*8], rbp
        mov     [r9 + 5*8], rbx


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
NESTED_END SymCryptFdefModMulMontgomeryMulxP384Asm, _TEXT



































































































































NESTED_ENTRY SymCryptFdefModSquareMontgomeryMulxP384Asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15

END_PROLOGUE

mov r10, rdx

        
        
        

        mov     r9, r8
        mov     r8, r10

        
        
        

        
        test    rsp,rsp
        jne     SymCryptFdefModMulMontgomeryMulxP384AsmInternal       

        int     3       
                        
                        


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
NESTED_END SymCryptFdefModSquareMontgomeryMulxP384Asm, _TEXT


































































































































NESTED_ENTRY SymCryptFdefRawMulMulx1024, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14

END_PROLOGUE

mov r10, rdx

        
        
        
        
        

        
        xorps       xmm0,xmm0               

        movaps      [r9],xmm0
        movaps      [r9+16],xmm0            
        movaps      [r9+32],xmm0            
        movaps      [r9+48],xmm0            

        movaps      [r9+64],xmm0
        movaps      [r9+80],xmm0            
        movaps      [r9+96],xmm0            
        movaps      [r9+112],xmm0           

        

        ZEROREG_8   r11, rsi, rdi, rbp, rbx, r12, r13, r14      

        MULADD88  r11, rsi, rdi, rbp, rbx, r12, r13, r14, r9, rcx, r10, rax, r8, rdx

        add     r10, 64              
        add     r9, 64
        xor     rax, rax              

        MULADD88  r11, rsi, rdi, rbp, rbx, r12, r13, r14, r9, rcx, r10, rax, r8, rdx

        add     r9, 64

        
        mov     [r9 + 0*8], r11
        mov     [r9 + 1*8], rsi
        mov     [r9 + 2*8], rdi
        mov     [r9 + 3*8], rbp
        mov     [r9 + 4*8], rbx
        mov     [r9 + 5*8], r12
        mov     [r9 + 6*8], r13
        mov     [r9 + 7*8], r14

        

        

        
        sub     r9, 64

        
        sub     r10, 64

        
        add     rcx, 64

        ZEROREG_8   r11, rsi, rdi, rbp, rbx, r12, r13, r14      

        MULADD88  r11, rsi, rdi, rbp, rbx, r12, r13, r14, r9, rcx, r10, rax, r8, rdx

        add     r10, 64              
        add     r9, 64
        xor     rax, rax              

        MULADD88  r11, rsi, rdi, rbp, rbx, r12, r13, r14, r9, rcx, r10, rax, r8, rdx

        add     r9, 64

        
        mov     [r9 + 0*8], r11
        mov     [r9 + 1*8], rsi
        mov     [r9 + 2*8], rdi
        mov     [r9 + 3*8], rbp
        mov     [r9 + 4*8], rbx
        mov     [r9 + 5*8], r12
        mov     [r9 + 6*8], r13
        mov     [r9 + 7*8], r14


BEGIN_EPILOGUE
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefRawMulMulx1024, _TEXT
























































































































NESTED_ENTRY SymCryptFdefRawSquareMulx1024, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14

END_PROLOGUE

mov r10, rdx

        
        xorps       xmm0,xmm0               

        movaps      [r8],xmm0
        movaps      [r8+16],xmm0
        movaps      [r8+32],xmm0
        movaps      [r8+48],xmm0

        movaps      [r8+64],xmm0
        movaps      [r8+80],xmm0
        movaps      [r8+96],xmm0
        movaps      [r8+112],xmm0

        xor     rax, rax                      

        HALF_SQUARE_NODIAG8 r11, rsi, rdi, rbp, rbx, r12, r13, r14,  r8, rcx, rax, r10, rdx

        lea     r9, [rcx + 64]               
        lea     r8, [r8 + 64]               

        
        
        
        
        
        

        MULADD88    r11, rsi, rdi, rbp, rbx, r12, r13, r14, r8, rcx, r9, rax, r10, rdx

        add     r8, 64                      

        
        mov     [r8 + 0*8], r11
        mov     [r8 + 1*8], rsi
        mov     [r8 + 2*8], rdi
        mov     [r8 + 3*8], rbp
        mov     [r8 + 4*8], rbx
        mov     [r8 + 5*8], r12
        mov     [r8 + 6*8], r13
        mov     [r8 + 7*8], r14

        

        xor     rax, rax                        

        HALF_SQUARE_NODIAG8 r11, rsi, rdi, rbp, rbx, r12, r13, r14,  r8, r9, rax, r10, rdx

        
        mov     [r8 +  8*8], r11
        mov     [r8 +  9*8], rsi
        mov     [r8 + 10*8], rdi
        mov     [r8 + 11*8], rbp
        mov     [r8 + 12*8], rbx
        mov     [r8 + 13*8], r12
        mov     [r8 + 14*8], r13
        mov     [r8 + 15*8], r14

        

        sub     r8, 128         

        SYMCRYPT_SQUARE_DIAG 0, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 1, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 2, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 3, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 4, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 5, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 6, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 7, rcx, r8, rax, r10, r9, r11, rdx

        SYMCRYPT_SQUARE_DIAG 8, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 9, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 10, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 11, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 12, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 13, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 14, rcx, r8, rax, r10, r9, r11, rdx
        SYMCRYPT_SQUARE_DIAG 15, rcx, r8, rax, r10, r9, r11, rdx


BEGIN_EPILOGUE
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefRawSquareMulx1024, _TEXT



























































































































NESTED_ENTRY SymCryptFdefMontgomeryReduceMulx1024, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15

END_PROLOGUE

mov r10, rdx

        mov     [rsp + 72 ], r8

        mov     eax, 2
        mov     [rsp + 80 ], eax
        

        xor     r9d, r9d
        lea     rcx, [rcx + SymCryptModulusValueOffsetAmd64]                      

SymCryptFdefMontgomeryReduceMulx1024OuterLoop:
        
        
        mov     rsi, [r10 + 0 * 8]
        mov     rdi, [r10 + 1 * 8]
        mov     rbp, [r10 + 2 * 8]
        mov     rbx, [r10 + 3 * 8]
        mov     r12, [r10 + 4 * 8]
        mov     r13, [r10 + 5 * 8]
        mov     r14, [r10 + 6 * 8]
        mov     r15, [r10 + 7 * 8]

        mov     r8, [rcx - SymCryptModulusValueOffsetAmd64 + SymCryptModulusInv64OffsetAmd64]    

        
        
        
        

        MONTGOMERY18    rsi, rdi, rbp, rbx, r12, r13, r14, r15,  r8, rcx, r10 + (0 * 8), rax, r11, rdx
        MONTGOMERY18    rdi, rbp, rbx, r12, r13, r14, r15, rsi,  r8, rcx, r10 + (1 * 8), rax, r11, rdx
        MONTGOMERY18    rbp, rbx, r12, r13, r14, r15, rsi, rdi,  r8, rcx, r10 + (2 * 8), rax, r11, rdx
        MONTGOMERY18    rbx, r12, r13, r14, r15, rsi, rdi, rbp,  r8, rcx, r10 + (3 * 8), rax, r11, rdx
        MONTGOMERY18    r12, r13, r14, r15, rsi, rdi, rbp, rbx,  r8, rcx, r10 + (4 * 8), rax, r11, rdx
        MONTGOMERY18    r13, r14, r15, rsi, rdi, rbp, rbx, r12,  r8, rcx, r10 + (5 * 8), rax, r11, rdx
        MONTGOMERY18    r14, r15, rsi, rdi, rbp, rbx, r12, r13,  r8, rcx, r10 + (6 * 8), rax, r11, rdx
        MONTGOMERY18    r15, rsi, rdi, rbp, rbx, r12, r13, r14,  r8, rcx, r10 + (7 * 8), rax, r11, rdx

        
        

        mov     r8, r10         
        add     rcx, 64
        add     r10, 64

        
        
        
        
        
        
        

        MULADD88    rsi, rdi, rbp, rbx, r12, r13, r14, r15,  r10, rcx, r8, rax, r11, rdx
            
            
            

        add     rcx, 64
        add     r10, 64

        
        
        
        neg     r9d
        mov     r9d, 0

        
        mov     rax, [r10 + 0 * 8]
        adc     rax, rsi
        mov     [r10 + 0 * 8], rax

        mov     r11, [r10 + 1 * 8]
        adc     r11, rdi
        mov     [r10 + 1 * 8], r11

        mov     rax, [r10 + 2 * 8]
        adc     rax, rbp
        mov     [r10 + 2 * 8], rax

        mov     r11, [r10 + 3 * 8]
        adc     r11, rbx
        mov     [r10 + 3 * 8], r11

        mov     rax, [r10 + 4 * 8]
        adc     rax, r12
        mov     [r10 + 4 * 8], rax

        mov     r11, [r10 + 5 * 8]
        adc     r11, r13
        mov     [r10 + 5 * 8], r11

        mov     rax, [r10 + 6 * 8]
        adc     rax, r14
        mov     [r10 + 6 * 8], rax

        mov     r11, [r10 + 7 * 8]
        adc     r11, r15
        mov     [r10 + 7 * 8], r11

        adc     r9d, r9d                  

        sub     r10, 64                  
        sub     rcx, 128                 

        mov     eax, [rsp + 80 ]
        sub     eax, 1
        mov     [rsp + 80 ], eax

        jnz     SymCryptFdefMontgomeryReduceMulx1024OuterLoop

        

        mov     r8, [rsp + 72 ]

        
        
        

        

        mov     rax,[r10 + 0 * 8]
        sbb     rax,[rcx + 0 * 8]
        mov     [r8 + 0 * 8], rax

        mov     r11,[r10 + 1 * 8]
        sbb     r11,[rcx + 1 * 8]
        mov     [r8 + 1 * 8], r11

        mov     rax,[r10 + 2 * 8]
        sbb     rax,[rcx + 2 * 8]
        mov     [r8 + 2 * 8], rax

        mov     r11,[r10 + 3 * 8]
        sbb     r11,[rcx + 3 * 8]
        mov     [r8 + 3 * 8], r11

        mov     rax,[r10 + 4 * 8]
        sbb     rax,[rcx + 4 * 8]
        mov     [r8 + 4 * 8], rax

        mov     r11,[r10 + 5 * 8]
        sbb     r11,[rcx + 5 * 8]
        mov     [r8 + 5 * 8], r11

        mov     rax,[r10 + 6 * 8]
        sbb     rax,[rcx + 6 * 8]
        mov     [r8 + 6 * 8], rax

        mov     r11,[r10 + 7 * 8]
        sbb     r11,[rcx + 7 * 8]
        mov     [r8 + 7 * 8], r11

        mov     rax,[r10 + 8 * 8]
        sbb     rax,[rcx + 8 * 8]
        mov     [r8 + 8 * 8], rax

        mov     r11,[r10 + 9 * 8]
        sbb     r11,[rcx + 9 * 8]
        mov     [r8 + 9 * 8], r11

        mov     rax,[r10 + 10 * 8]
        sbb     rax,[rcx + 10 * 8]
        mov     [r8 + 10 * 8], rax

        mov     r11,[r10 + 11 * 8]
        sbb     r11,[rcx + 11 * 8]
        mov     [r8 + 11 * 8], r11

        mov     rax,[r10 + 12 * 8]
        sbb     rax,[rcx + 12 * 8]
        mov     [r8 + 12 * 8], rax

        mov     r11,[r10 + 13 * 8]
        sbb     r11,[rcx + 13 * 8]
        mov     [r8 + 13 * 8], r11

        mov     rax,[r10 + 14 * 8]
        sbb     rax,[rcx + 14 * 8]
        mov     [r8 + 14 * 8], rax

        mov     r11,[r10 + 15 * 8]
        sbb     r11,[rcx + 15 * 8]
        mov     [r8 + 15 * 8], r11

        
        
        sbb     r9d, 0
        

        movd    xmm0, r9d            
        pcmpeqd xmm1, xmm1          
        pshufd  xmm0, xmm0, 0       
        pxor    xmm1, xmm0          


        movdqa  xmm2, [r10 + 0 * 16]    
        movdqa  xmm3, [r8 + 0 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 0 * 16], xmm2

        movdqa  xmm2, [r10 + 1 * 16]    
        movdqa  xmm3, [r8 + 1 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 1 * 16], xmm2

        movdqa  xmm2, [r10 + 2 * 16]    
        movdqa  xmm3, [r8 + 2 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 2 * 16], xmm2

        movdqa  xmm2, [r10 + 3 * 16]    
        movdqa  xmm3, [r8 + 3 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 3 * 16], xmm2

        movdqa  xmm2, [r10 + 4 * 16]    
        movdqa  xmm3, [r8 + 4 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 4 * 16], xmm2

        movdqa  xmm2, [r10 + 5 * 16]    
        movdqa  xmm3, [r8 + 5 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 5 * 16], xmm2

        movdqa  xmm2, [r10 + 6 * 16]    
        movdqa  xmm3, [r8 + 6 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 6 * 16], xmm2

        movdqa  xmm2, [r10 + 7 * 16]    
        movdqa  xmm3, [r8 + 7 * 16]    
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [r8 + 7 * 16], xmm2


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
NESTED_END SymCryptFdefMontgomeryReduceMulx1024, _TEXT

























































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































END
