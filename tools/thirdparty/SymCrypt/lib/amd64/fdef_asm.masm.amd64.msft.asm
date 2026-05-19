









































































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
    



    
    
    
    
    
    
    




























MULT_SINGLEADD_128 MACRO  index, src_reg, dst_reg, Q0, QH, mul_word, even_carry, odd_carry
        
        
        
        
        
        
        

        mov     Q0, [src_reg + 8*index]
        mul     mul_word
        mov     odd_carry, QH
        add     Q0, even_carry
        mov     [dst_reg + 8*index], Q0
        adc     odd_carry, 0

        mov     Q0, [src_reg + 8*(index+1)]
        mul     mul_word
        mov     even_carry, QH
        add     Q0, odd_carry
        mov     [dst_reg + 8*(index+1)], Q0
        adc     even_carry, 0
ENDM

MULT_DOUBLEADD_128 MACRO  index, src_reg, dst_reg, Q0, QH, mul_word, even_carry, odd_carry
        
        
        
        
        
        
        

        mov     Q0, [src_reg + 8*index]
        mul     mul_word
        mov     odd_carry, QH
        add     Q0, [dst_reg + 8*index]
        adc     odd_carry, 0
        add     Q0, even_carry
        mov     [dst_reg + 8*index], Q0
        adc     odd_carry, 0

        mov     Q0, [src_reg + 8*(index+1)]
        mul     mul_word
        mov     even_carry, QH
        add     Q0, [dst_reg + 8*(index+1)]
        adc     even_carry, 0
        add     Q0, odd_carry
        mov     [dst_reg + 8*(index+1)], Q0
        adc     even_carry, 0
ENDM



SQR_SINGLEADD_64 MACRO  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
        
        
        
        
        
        
        

        mov     Q0, [src_reg + 8*index]
        mul     mul_word
        mov     dst_carry, QH
        add     Q0, src_carry
        mov     [dst_reg + 8*index], Q0
        adc     dst_carry, 0
ENDM

SQR_DOUBLEADD_64 MACRO  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
        
        
        
        
        
        
        

        mov     Q0, [src_reg + 8*index]
        mul     mul_word
        mov     dst_carry, QH
        add     Q0, [dst_reg + 8*index]
        adc     dst_carry, 0
        add     Q0, src_carry
        mov     [dst_reg + 8*index], Q0
        adc     dst_carry, 0
ENDM

SQR_SHIFT_LEFT MACRO  index, Q0, src_reg
    mov     Q0, [src_reg + 8*index]
    adc     Q0, Q0                 
    mov     [src_reg + 8*index], Q0
ENDM

SQR_DIAGONAL_PROP MACRO  index, src_reg, dst_reg, Q0, QH, carry
    
    mov     Q0, [src_reg + 8*index]     
    mul     Q0                     

    
    add     Q0, [dst_reg + 16*index]
    adc     QH, 0
    add     Q0, carry
    adc     QH, 0
    mov     [dst_reg + 16*index], Q0

    
    mov     Q0, QH
    xor     QH, QH

    add     Q0, [dst_reg + 16*index + 8]
    adc     QH, 0
    mov     [dst_reg + 16*index + 8], Q0
    mov     carry, QH
ENDM

MONTGOMERY14 MACRO  Q0, QH, mul_word, pA, R0, R1, R2, R3, Cy
    
    
    

    mov     Q0, [pA]
    mul     mul_word
    add     R0, -1  
    adc     QH, 0
    mov     Cy, QH

    mov     Q0, [pA + 8]
    mul     mul_word
    add     R1, Q0
    adc     QH, 0
    add     R1, Cy
    adc     QH, 0
    mov     Cy, QH

    mov     Q0, [pA + 16]
    mul     mul_word
    add     R2, Q0
    adc     QH, 0
    add     R2, Cy
    adc     QH, 0
    mov     Cy, QH

    mov     Q0, [pA + 24]
    mul     mul_word
    add     R3, Q0
    adc     QH, 0
    add     R3, Cy
    adc     QH, 0
ENDM

MUL14 MACRO  Q0, QH, mul_word, pA, R0, R1, R2, R3, Cy
    
    

    mov     Q0, [pA]
    mul     mul_word
    add     R0, Q0
    adc     QH, 0
    mov     Cy, QH

    mov     Q0, [pA + 8]
    mul     mul_word
    add     R1, Q0
    adc     QH, 0
    add     R1, Cy
    adc     QH, 0
    mov     Cy, QH

    mov     Q0, [pA + 16]
    mul     mul_word
    add     R2, Q0
    adc     QH, 0
    add     R2, Cy
    adc     QH, 0
    mov     Cy, QH

    mov     Q0, [pA + 24]
    mul     mul_word
    add     R3, Q0
    adc     QH, 0
    add     R3, Cy
    adc     QH, 0
ENDM


SQR_DOUBLEADD_64_2 MACRO  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64    (index),     src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64    (index + 1), src_reg, dst_reg, Q0, QH, mul_word, dst_carry, src_carry
ENDM

SQR_DOUBLEADD_64_4 MACRO  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64_2  (index),     src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64_2  (index + 2), src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
ENDM

SQR_DOUBLEADD_64_8 MACRO  index, src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64_4  (index),     src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
    SQR_DOUBLEADD_64_4  (index + 4), src_reg, dst_reg, Q0, QH, mul_word, src_carry, dst_carry
ENDM

SQR_SIZE_SPECIFIC_INIT MACRO  outer_src_reg, outer_dst_reg, inner_src_reg, inner_dst_reg, mul_word
    lea     outer_src_reg, [outer_src_reg + 8]  
    lea     outer_dst_reg, [outer_dst_reg + 16] 

    mov     inner_src_reg, outer_src_reg        
    mov     inner_dst_reg, outer_dst_reg        

    mov     mul_word, [outer_src_reg]           
    lea     inner_src_reg, [inner_src_reg + 8]  
ENDM





























LEAF_ENTRY SymCryptFdefRawAddAsm, _TEXT


        
        add     r9d, r9d
        xor     rax, rax

SymCryptFdefRawAddAsmLoop:
        
        mov     rax,[rcx]
        adc     rax,[rdx]
        mov     [r8],rax

        mov     rax,[rcx + 8]
        adc     rax,[rdx + 8]
        mov     [r8 + 8], rax

        mov     rax,[rcx + 16]
        adc     rax,[rdx + 16]
        mov     [r8 + 16], rax

        mov     rax,[rcx + 24]
        adc     rax,[rdx + 24]
        mov     [r8 + 24], rax

        lea     rcx, [rcx + 32]
        lea     rdx, [rdx + 32]
        lea     r8, [r8 + 32]
        dec     r9d
        jnz     SymCryptFdefRawAddAsmLoop

        mov     rax, 0
        adc     rax, rax


BEGIN_EPILOGUE
ret
LEAF_END SymCryptFdefRawAddAsm, _TEXT

















































LEAF_ENTRY SymCryptFdefRawSubAsm, _TEXT


        
        add     r9d, r9d
        xor     rax, rax

SymCryptFdefRawSubAsmLoop:
        
        mov     rax,[rcx]
        sbb     rax,[rdx]
        mov     [r8],rax

        mov     rax,[rcx + 8]
        sbb     rax,[rdx + 8]
        mov     [r8 + 8], rax

        mov     rax,[rcx + 16]
        sbb     rax,[rdx + 16]
        mov     [r8 + 16], rax

        mov     rax,[rcx + 24]
        sbb     rax,[rdx + 24]
        mov     [r8 + 24], rax

        lea     rcx,[rcx + 32]
        lea     rdx,[rdx + 32]
        lea     r8,[r8 + 32]
        dec     r9d
        jnz     SymCryptFdefRawSubAsmLoop

        mov     rax, 0
        adc     rax, rax


BEGIN_EPILOGUE
ret
LEAF_END SymCryptFdefRawSubAsm, _TEXT

















































LEAF_ENTRY SymCryptFdefMaskedCopyAsm, _TEXT


        add     r8d, r8d              

        movd    xmm0, r9d            
        pcmpeqd xmm1, xmm1          
        pshufd  xmm0, xmm0, 0       
        pxor    xmm1, xmm0          

SymCryptFdefMaskedCopyAsmLoop:
        movdqa  xmm2, [rcx]          
        movdqa  xmm3, [rdx]          
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [rdx], xmm2

        movdqa  xmm2, [rcx + 16]     
        movdqa  xmm3, [rdx + 16]     
        pand    xmm2, xmm0
        pand    xmm3, xmm1
        por     xmm2, xmm3
        movdqa  [rdx + 16], xmm2

        

        add     rcx, 32
        add     rdx, 32
        dec     r8d
        jnz     SymCryptFdefMaskedCopyAsmLoop


BEGIN_EPILOGUE
ret
LEAF_END SymCryptFdefMaskedCopyAsm, _TEXT



















































































NESTED_ENTRY SymCryptFdefRawMulAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13

END_PROLOGUE

mov r10, rdx
mov r11, [rsp + 88]

        shl     r10, 3           

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        


        

        mov     rsi, r8          
        mov     rdi, r11          
        mov     rbp, [rcx]        
        xor     rbx, rbx
        mov     r12, r9

        

align 16

SymCryptFdefRawMulAsmLoop1:
        MULT_SINGLEADD_128 0, rsi, rdi, rax, rdx, rbp, rbx, r13
        MULT_SINGLEADD_128 2, rsi, rdi, rax, rdx, rbp, rbx, r13
        MULT_SINGLEADD_128 4, rsi, rdi, rax, rdx, rbp, rbx, r13
        MULT_SINGLEADD_128 6, rsi, rdi, rax, rdx, rbp, rbx, r13

        lea     rsi,[rsi + 64]
        lea     rdi,[rdi + 64]

        dec     r12
        jnz     SymCryptFdefRawMulAsmLoop1

        mov     [rdi], rbx        

        dec     r10

align 16

SymCryptFdefRawMulAsmLoopOuter:

        add     rcx, 8           
        add     r11, 8           
        mov     rbp, [rcx]
        mov     rsi, r8
        mov     rdi, r11
        xor     rbx, rbx
        mov     r12, r9

align 16

SymCryptFdefRawMulAsmLoop2:
        MULT_DOUBLEADD_128 0, rsi, rdi, rax, rdx, rbp, rbx, r13
        MULT_DOUBLEADD_128 2, rsi, rdi, rax, rdx, rbp, rbx, r13
        MULT_DOUBLEADD_128 4, rsi, rdi, rax, rdx, rbp, rbx, r13
        MULT_DOUBLEADD_128 6, rsi, rdi, rax, rdx, rbp, rbx, r13

        lea     rsi,[rsi + 64]
        lea     rdi,[rdi + 64]

        dec     r12
        jnz     SymCryptFdefRawMulAsmLoop2

        mov     [rdi], rbx        

        dec     r10
        jnz     SymCryptFdefRawMulAsmLoopOuter


BEGIN_EPILOGUE
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefRawMulAsm, _TEXT




















































































































NESTED_ENTRY SymCryptFdefRawSquareAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14

END_PROLOGUE

mov r10, rdx

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        
        
        
        
        
        
        
        
        
        
        
        

        mov     [rsp + 64 ], rcx   

        mov     r12, r10             
        shl     r12, 3              
        mov     rbx, r8              

        mov     r9, rcx              
        mov     r11, r8              

        

        mov     rsi, [rcx]            

        xor     rdi, rdi              
        xor     rbp, rbp              

        mov     r13, r12            
        mov     [r11], rdi            

        
        jmp     SymCryptFdefRawSquareAsmInnerLoopInit_Word1

align 16
SymCryptFdefRawSquareAsmInnerLoopInit_Word0:
        SQR_SINGLEADD_64 0, r9, r11, rax, rdx, rsi, rdi, rbp

align 16
SymCryptFdefRawSquareAsmInnerLoopInit_Word1:
        SQR_SINGLEADD_64 1, r9, r11, rax, rdx, rsi, rbp, rdi

        SQR_SINGLEADD_64 2, r9, r11, rax, rdx, rsi, rdi, rbp

        SQR_SINGLEADD_64 3, r9, r11, rax, rdx, rsi, rbp, rdi

        lea     r9, [r9 + 32]
        lea     r11, [r11 + 32]
        sub     r13, 4
        jnz     SymCryptFdefRawSquareAsmInnerLoopInit_Word0

        mov     [r11], rdi                

        dec     r12                     
        mov     r14, 1                  

align 16
SymCryptFdefRawSquareAsmLoopOuter:

        add     rbx, 8                   

        mov     r9, rcx                  
        mov     r11, rbx                  

        mov     rsi, [rcx + 8*r14]        

        inc     r14b                     

        mov     r13, r12                
        add     r13, 2
        and     r13, -4                 

        xor     rdi, rdi                  
        xor     rbp, rbp                  

        
        cmp     r14b, 3
        je      SymCryptFdefRawSquareAsmInnerLoop_Word3
        cmp     r14b, 2
        je      SymCryptFdefRawSquareAsmInnerLoop_Word2
        cmp     r14b, 1
        je      SymCryptFdefRawSquareAsmInnerLoop_Word1

        
        xor     r14b, r14b                

        add     rcx, 32                  
        add     rbx, 32                  

        mov     r9, rcx                  
        mov     r11, rbx                  

align 16
SymCryptFdefRawSquareAsmInnerLoop_Word0:
        SQR_DOUBLEADD_64 0, r9, r11, rax, rdx, rsi, rdi, rbp

align 16
SymCryptFdefRawSquareAsmInnerLoop_Word1:
        SQR_DOUBLEADD_64 1, r9, r11, rax, rdx, rsi, rbp, rdi

align 16
SymCryptFdefRawSquareAsmInnerLoop_Word2:
        SQR_DOUBLEADD_64 2, r9, r11, rax, rdx, rsi, rdi, rbp

align 16
SymCryptFdefRawSquareAsmInnerLoop_Word3:
        SQR_DOUBLEADD_64 3, r9, r11, rax, rdx, rsi, rbp, rdi

        lea     r9, [r9 + 32]
        lea     r11, [r11 + 32]
        sub     r13, 4
        jnz     SymCryptFdefRawSquareAsmInnerLoop_Word0

        mov     [r11], rdi            

        dec     r12
        cmp     r12, 1
        jne     SymCryptFdefRawSquareAsmLoopOuter

        xor     rdx, rdx
        mov     [rbx + 40], rdx       


        
        
        

        mov     r12, r10             
        mov     r11, r8              
        shl     r12, 1              

align 16
SymCryptFdefRawSquareAsmSecondPass:
        SQR_SHIFT_LEFT 0, rax, r11
        SQR_SHIFT_LEFT 1, rax, r11
        SQR_SHIFT_LEFT 2, rax, r11
        SQR_SHIFT_LEFT 3, rax, r11

        SQR_SHIFT_LEFT 4, rax, r11
        SQR_SHIFT_LEFT 5, rax, r11
        SQR_SHIFT_LEFT 6, rax, r11
        SQR_SHIFT_LEFT 7, rax, r11

        lea     r11, [r11 + 64]
        dec     r12
        jnz     SymCryptFdefRawSquareAsmSecondPass

        
        
        

        mov     rcx, [rsp + 64 ]  

SymCryptFdefRawSquareAsmThirdPass:
        SQR_DIAGONAL_PROP 0, rcx, r8, rax, rdx, r12
        SQR_DIAGONAL_PROP 1, rcx, r8, rax, rdx, r12
        SQR_DIAGONAL_PROP 2, rcx, r8, rax, rdx, r12
        SQR_DIAGONAL_PROP 3, rcx, r8, rax, rdx, r12
        SQR_DIAGONAL_PROP 4, rcx, r8, rax, rdx, r12
        SQR_DIAGONAL_PROP 5, rcx, r8, rax, rdx, r12
        SQR_DIAGONAL_PROP 6, rcx, r8, rax, rdx, r12
        SQR_DIAGONAL_PROP 7, rcx, r8, rax, rdx, r12

        add     rcx, 64              
        add     r8, 128             
        dec     r10
        jnz     SymCryptFdefRawSquareAsmThirdPass


BEGIN_EPILOGUE
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefRawSquareAsm, _TEXT



























































































































NESTED_ENTRY SymCryptFdefMontgomeryReduceAsm, _TEXT

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

        mov     r9d, [rcx + SymCryptModulusNdigitsOffsetAmd64]    
        mov     r11, [rcx + SymCryptModulusInv64OffsetAmd64]      

        lea     rcx, [rcx + SymCryptModulusValueOffsetAmd64]      

        mov     r15d, r9d         
        shl     r15d, 3          

        xor     ebx, ebx

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

align 16

SymCryptFdefMontgomeryReduceAsmOuterLoop:

        
        

        mov     rsi, [r10]                        
        mov     r13, r10
        mov     r12, rcx

        imul    rsi, r11                          

        mov     r14d, r9d
        xor     edi, edi

align 16

SymCryptFdefMontgomeryReduceAsmInnerloop:
        
        
        
        
        
        
        
        
        
        

        MULT_DOUBLEADD_128 0, r12, r13, rax, rdx, rsi, rdi, rbp
        MULT_DOUBLEADD_128 2, r12, r13, rax, rdx, rsi, rdi, rbp
        MULT_DOUBLEADD_128 4, r12, r13, rax, rdx, rsi, rdi, rbp
        MULT_DOUBLEADD_128 6, r12, r13, rax, rdx, rsi, rdi, rbp

        lea     r12,[r12 + 64]
        lea     r13,[r13 + 64]

        dec     r14d
        jnz     SymCryptFdefMontgomeryReduceAsmInnerloop

        add     rdi, rbx
        mov     ebx, 0
        adc     rbx, 0
        add     rdi, [r13]
        adc     rbx, 0
        mov     [r13], rdi

        lea     r10,[r10 + 8]

        dec     r15d
        jnz     SymCryptFdefMontgomeryReduceAsmOuterLoop

        
        
        

        
        mov     r14d, r9d         
        mov     r13, r10         
        mov     r12, rcx         
        mov     rdi, r8          

        

align 16

SymCryptFdefMontgomeryReduceAsmSubLoop:
        mov     rax,[r13]
        sbb     rax,[r12]
        mov     [rdi], rax

        mov     rax,[r13 + 8]
        sbb     rax,[r12 + 8]
        mov     [rdi + 8], rax

        mov     rax,[r13 + 16]
        sbb     rax,[r12 + 16]
        mov     [rdi + 16], rax

        mov     rax,[r13 + 24]
        sbb     rax,[r12 + 24]
        mov     [rdi + 24], rax

        mov     rax,[r13 + 32]
        sbb     rax,[r12 + 32]
        mov     [rdi + 32], rax

        mov     rax,[r13 + 40]
        sbb     rax,[r12 + 40]
        mov     [rdi + 40], rax

        mov     rax,[r13 + 48]
        sbb     rax,[r12 + 48]
        mov     [rdi + 48], rax

        mov     rax,[r13 + 56]
        sbb     rax,[r12 + 56]
        mov     [rdi + 56], rax

        lea     r13,[r13 + 64]
        lea     r12,[r12 + 64]
        lea     rdi,[rdi + 64]

        dec     r14d
        jnz     SymCryptFdefMontgomeryReduceAsmSubLoop

        
        
        sbb     ebx, 0

        movd    xmm0, ebx            
        pcmpeqd xmm1, xmm1          
        pshufd  xmm0, xmm0, 0       
        pxor    xmm1, xmm0          

align 16

SymCryptFdefMontgomeryReduceAsmMaskedCopyLoop:
        movdqa  xmm2, [r10]          
        movdqa  xmm3, [r8]          
        pand    xmm2, xmm0          
        pand    xmm3, xmm1          
        por     xmm2, xmm3
        movdqa  [r8], xmm2

        movdqa  xmm2, [r10 + 16]     
        movdqa  xmm3, [r8 + 16]     
        pand    xmm2, xmm0          
        pand    xmm3, xmm1          
        por     xmm2, xmm3
        movdqa  [r8 + 16], xmm2

        movdqa  xmm2, [r10 + 32]     
        movdqa  xmm3, [r8 + 32]     
        pand    xmm2, xmm0          
        pand    xmm3, xmm1          
        por     xmm2, xmm3
        movdqa  [r8 + 32], xmm2

        movdqa  xmm2, [r10 + 48]     
        movdqa  xmm3, [r8 + 48]     
        pand    xmm2, xmm0          
        pand    xmm3, xmm1          
        por     xmm2, xmm3
        movdqa  [r8 + 48], xmm2

        
        lea     r10,[r10 + 64]
        lea     r8,[r8 + 64]

        dec     r9d
        jnz     SymCryptFdefMontgomeryReduceAsmMaskedCopyLoop


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
NESTED_END SymCryptFdefMontgomeryReduceAsm, _TEXT






































































































NESTED_ENTRY SymCryptFdefModSub256Asm, _TEXT

rex_push_reg rsi
push_reg rdi

END_PROLOGUE


        
        
        
        

        add     rcx, SymCryptModulusValueOffsetAmd64

        

        mov     rax, [rdx + 0*8]
        sub     rax, [r8 + 0*8]
        mov     r10, [rdx + 1*8]
        sbb     r10, [r8 + 1*8]
        mov     r11, [rdx + 2*8]
        sbb     r11, [r8 + 2*8]
        mov     rsi, [rdx + 3*8]
        sbb     rsi, [r8 + 3*8]

        
        

        mov     rdx, 0
        cmovb   rdx, [rcx + 0*8]
        mov     r8, 0
        cmovb   r8, [rcx + 1*8]
        mov     rdi, 0
        cmovb   rdi, [rcx + 2*8]
        mov     rcx, [rcx + 3*8]
        cmovnb  rcx, rdx

        
        add     rax, rdx
        adc     r10, r8
        adc     r11, rdi
        adc     rsi, rcx

        mov     [r9 + 0*8], rax
        mov     [r9 + 1*8], r10
        mov     [r9 + 2*8], r11
        mov     [r9 + 3*8], rsi


BEGIN_EPILOGUE
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefModSub256Asm, _TEXT


























































































NESTED_ENTRY SymCryptFdefModSub384Asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13

END_PROLOGUE


        
        
        
        

        add     rcx, SymCryptModulusValueOffsetAmd64

        

        mov     rax, [rdx + 0*8]
        sub     rax, [r8 + 0*8]
        mov     r10, [rdx + 1*8]
        sbb     r10, [r8 + 1*8]
        mov     r11, [rdx + 2*8]
        sbb     r11, [r8 + 2*8]
        mov     rsi, [rdx + 3*8]
        sbb     rsi, [r8 + 3*8]
        mov     rdi, [rdx + 4*8]
        sbb     rdi, [r8 + 4*8]
        mov     rbp, [rdx + 5*8]
        sbb     rbp, [r8 + 5*8]

        
        

        mov     rdx,  0
        cmovb   rdx,  [rcx + 0*8]
        mov     r8,  0
        cmovb   r8,  [rcx + 1*8]
        mov     rbx, 0
        cmovb   rbx, [rcx + 2*8]
        mov     r12, 0
        cmovb   r12, [rcx + 3*8]
        mov     r13, 0
        cmovb   r13, [rcx + 4*8]
        mov     rcx,  [rcx + 5*8]
        cmovnb  rcx,  rdx

        
        add     rax, rdx
        adc     r10, r8
        adc     r11, rbx
        adc     rsi, r12
        adc     rdi, r13
        adc     rbp, rcx

        mov     [r9 + 0*8], rax
        mov     [r9 + 1*8], r10
        mov     [r9 + 2*8], r11
        mov     [r9 + 3*8], rsi
        mov     [r9 + 4*8], rdi
        mov     [r9 + 5*8], rbp


BEGIN_EPILOGUE
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdefModSub384Asm, _TEXT


























































altentry SymCryptFdefMontgomeryReduce256AsmInternal









































































NESTED_ENTRY SymCryptFdefModMulMontgomery256Asm, _TEXT

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

        
        
        
        

        
        

        mov     r11, [r10]
        xor     rbp, rbp
        xor     rbx, rbx
        xor     r12, r12

        mov     rax, [r8]
        mul     r11
        mov     rsi, rax
        mov     rdi, rdx

        mov     rax, [r8 + 8]
        mul     r11
        add     rdi, rax
        adc     rbp, rdx

        mov     rax, [r8 + 16]
        mul     r11
        add     rbp, rax
        adc     rbx, rdx

        mov     rax, [r8 + 24]
        mul     r11
        add     rbx, rax
        adc     r12, rdx

        
        mov     r11, [r10 + 8]
        MUL14   rax, rdx, r11, r8, rdi, rbp, rbx, r12, r15
        mov     r13, rdx

        
        mov     r11, [r10 + 16]
        MUL14   rax, rdx, r11, r8, rbp, rbx, r12, r13, r15
        mov     r14, rdx

        
        mov     r11, [r10 + 24]
        MUL14   rax, rdx, r11, r8, rbx, r12, r13, r14, r15
        mov     r15, rdx

ALTERNATE_ENTRY SymCryptFdefMontgomeryReduce256AsmInternal
        
        
        
        
        

        mov     r8, [rcx + SymCryptModulusInv64OffsetAmd64]      
        add     rcx, SymCryptModulusValueOffsetAmd64

        mov     r11, rsi
        imul    r11, r8             
        MONTGOMERY14 rax, rdx, r11, rcx, rsi, rdi, rbp, rbx, rsi
        mov     rsi, rdx            

        mov     r11, rdi
        imul    r11, r8
        MONTGOMERY14 rax, rdx, r11, rcx, rdi, rbp, rbx, r12, rdi
        mov     rdi, rdx            

        mov     r11, rbp
        imul    r11, r8
        MONTGOMERY14 rax, rdx, r11, rcx, rbp, rbx, r12, r13, rbp
        mov     rbp, rdx

        mov     r11, rbx
        imul    r11, r8
        MONTGOMERY14 rax, rdx, r11, rcx, rbx, r12, r13, r14, rbx
        

        add     r12, rsi
        adc     r13, rdi
        adc     r14, rbp
        adc     r15, rdx

        sbb     r11, r11        

        

        mov     rsi, r12
        sub     rsi, [rcx]
        mov     rdi, r13
        sbb     rdi, [rcx + 8]
        mov     rbp, r14
        sbb     rbp, [rcx + 16]
        mov     rbx, r15
        sbb     rbx, [rcx + 24]

        sbb     rcx, rcx        

        
        
        
        

        xor     rcx, r11        

        cmovz   r12, rsi
        cmovz   r13, rdi
        cmovz   r14, rbp
        cmovz   r15, rbx

        mov     [r9 +  0], r12
        mov     [r9 +  8], r13
        mov     [r9 + 16], r14
        mov     [r9 + 24], r15


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
NESTED_END SymCryptFdefModMulMontgomery256Asm, _TEXT


































































































































NESTED_ENTRY SymCryptFdefMontgomeryReduce256Asm, _TEXT

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
        mov     rsi,  [r10 +  0]
        mov     rdi,  [r10 +  8]
        mov     rbp,  [r10 + 16]
        mov     rbx,  [r10 + 24]
        mov     r12, [r10 + 32]
        mov     r13, [r10 + 40]
        mov     r14, [r10 + 48]
        mov     r15, [r10 + 56]

        
        
        

        
        test    rsp,rsp
        jne     SymCryptFdefMontgomeryReduce256AsmInternal       

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
NESTED_END SymCryptFdefMontgomeryReduce256Asm, _TEXT


















































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































END
