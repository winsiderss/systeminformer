



















































































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
    



    
    
    
    
    
    
    
























































LEAF_ENTRY SymCryptFdef369RawAddAsm, _TEXT


        inc     r9d
        xor     rax, rax

SymCryptFdef369RawAddAsmLoop:
        
        mov     rax,[rcx]
        adc     rax,[rdx]
        mov     [r8],rax

        mov     rax,[rcx + 8]
        adc     rax,[rdx + 8]
        mov     [r8 + 8], rax

        mov     rax,[rcx + 16]
        adc     rax,[rdx + 16]
        mov     [r8 + 16], rax

        lea     rcx, [rcx + 24]
        lea     rdx, [rdx + 24]
        lea     r8, [r8 + 24]
        dec     r9d
        jnz     SymCryptFdef369RawAddAsmLoop

        mov     rax, 0
        adc     rax, rax


BEGIN_EPILOGUE
ret
LEAF_END SymCryptFdef369RawAddAsm, _TEXT

















































LEAF_ENTRY SymCryptFdef369RawSubAsm, _TEXT


        inc     r9d
        xor     rax, rax

SymCryptFdef369RawSubAsmLoop:
        
        mov     rax,[rcx]
        sbb     rax,[rdx]
        mov     [r8],rax

        mov     rax,[rcx + 8]
        sbb     rax,[rdx + 8]
        mov     [r8 + 8], rax

        mov     rax,[rcx + 16]
        sbb     rax,[rdx + 16]
        mov     [r8 + 16], rax

        lea     rcx, [rcx + 24]
        lea     rdx, [rdx + 24]
        lea     r8, [r8 + 24]
        dec     r9d
        jnz     SymCryptFdef369RawSubAsmLoop

        mov     rax, 0
        adc     rax, rax


BEGIN_EPILOGUE
ret
LEAF_END SymCryptFdef369RawSubAsm, _TEXT





















































LEAF_ENTRY SymCryptFdef369MaskedCopyAsm, _TEXT


        inc     r8d
        movsxd  r9, r9d

SymCryptFdef369MaskedCopyAsmLoop:
        mov     rax, [rcx]
        mov     r10, [rdx]
        xor     rax, r10
        and     rax, r9
        xor     rax, r10
        mov     [rdx], rax

        mov     rax, [rcx + 8]
        mov     r10, [rdx + 8]
        xor     rax, r10
        and     rax, r9
        xor     rax, r10
        mov     [rdx + 8], rax

        mov     rax, [rcx + 16]
        mov     r10, [rdx + 16]
        xor     rax, r10
        and     rax, r9
        xor     rax, r10
        mov     [rdx + 16], rax

        

        add     rcx, 24
        add     rdx, 24
        dec     r8d
        jnz     SymCryptFdef369MaskedCopyAsmLoop


BEGIN_EPILOGUE
ret
LEAF_END SymCryptFdef369MaskedCopyAsm, _TEXT


















































































NESTED_ENTRY SymCryptFdef369RawMulAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12

END_PROLOGUE

mov r10, rdx
mov r11, [rsp + 80]

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        inc     r10d
        inc     r9d
        lea     r10d, [r10d + 2*r10d]     

        

        mov     rsi, r8              
        mov     rdi, r11              
        mov     rbp, [rcx]            
        xor     rbx, rbx
        mov     r12d, r9d

        

align 16

SymCryptFdef369RawMulAsmLoop1:
        mov     rax, [rsi]
        mul     rbp
        add     rax, rbx
        adc     rdx, 0
        mov     [rdi], rax
        mov     rbx, rdx

        mov     rax, [rsi + 8]
        mul     rbp
        add     rax, rbx
        adc     rdx, 0
        mov     [rdi + 8], rax
        mov     rbx, rdx

        mov     rax, [rsi + 16]
        mul     rbp
        add     rax, rbx
        adc     rdx, 0
        mov     [rdi + 16], rax
        mov     rbx, rdx

        add     rsi, 24
        add     rdi, 24
        dec     r12d
        jnz     SymCryptFdef369RawMulAsmLoop1

        mov     [rdi], rdx                

        dec     r10d

align 16

SymCryptFdef369RawMulAsmLoopOuter:

        add     rcx, 8                   
        add     r11, 8                   
        mov     rbp, [rcx]
        mov     rsi, r8
        mov     rdi, r11
        xor     rbx, rbx
        mov     r12d, r9d

align 16

SymCryptFdef369RawMulAsmLoop2:
        mov     rax, [rsi]
        mul     rbp
        add     rax, [rdi]
        adc     rdx, 0
        add     rax, rbx
        adc     rdx, 0
        mov     [rdi], rax
        mov     rbx, rdx

        mov     rax, [rsi + 8]
        mul     rbp
        add     rax, [rdi + 8]
        adc     rdx, 0
        add     rax, rbx
        adc     rdx, 0
        mov     [rdi + 8], rax
        mov     rbx, rdx

        mov     rax, [rsi + 16]
        mul     rbp
        add     rax, [rdi + 16]
        adc     rdx, 0
        add     rax, rbx
        adc     rdx, 0
        mov     [rdi + 16], rax
        mov     rbx, rdx

        add     rsi, 24
        add     rdi, 24
        dec     r12d
        jnz     SymCryptFdef369RawMulAsmLoop2

        mov     [rdi], rdx           

        dec     r10d
        jnz     SymCryptFdef369RawMulAsmLoopOuter


BEGIN_EPILOGUE
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdef369RawMulAsm, _TEXT
















































































































NESTED_ENTRY SymCryptFdef369MontgomeryReduceAsm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14

END_PROLOGUE

mov r10, rdx

        mov     r9d, [rcx + SymCryptModulusNdigitsOffsetAmd64]    
        inc     r9d
        mov     r11, [rcx + SymCryptModulusInv64OffsetAmd64]      

        lea     rcx, [rcx + SymCryptModulusValueOffsetAmd64]      

        lea     r14d, [r9d + 2*r9d]  

        xor     ebp, ebp

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

align 16

SymCryptFdef369MontgomeryReduceAsmOuterLoop:

        
        

        mov     rsi, [r10]                      
        mov     r12, r10
        mov     rbx, rcx

        imul    rsi, r11                        

        mov     r13d, r9d
        xor     edi, edi

align 16

SymCryptFdef369MontgomeryReduceAsmInnerloop:
        
        
        
        
        
        
        
        
        

        mov     rax, [rbx]
        mul     rsi
        add     rax, [r12]
        adc     rdx, 0
        add     rax, rdi
        adc     rdx, 0
        mov     [r12], rax
        mov     rdi, rdx

        mov     rax, [rbx + 8]
        mul     rsi
        add     rax, [r12 + 8]
        adc     rdx, 0
        add     rax, rdi
        adc     rdx, 0
        mov     [r12 + 8], rax
        mov     rdi, rdx

        mov     rax, [rbx + 16]
        mul     rsi
        add     rax, [r12 + 16]
        adc     rdx, 0
        add     rax, rdi
        adc     rdx, 0
        mov     [r12 + 16], rax
        mov     rdi, rdx

        add     rbx, 24
        add     r12, 24
        dec     r13d
        jnz     SymCryptFdef369MontgomeryReduceAsmInnerloop

        add     rdi, rbp
        mov     ebp, 0
        adc     rbp, 0
        add     rdi, [r12]
        adc     rbp, 0
        mov     [r12], rdi

        add     r10, 8

        dec     r14d
        jnz     SymCryptFdef369MontgomeryReduceAsmOuterLoop

        
        
        

        
        mov     r13d, r9d         
        mov     r12, r10         
        mov     rbx, rcx          
        mov     rdi, r8          

        

align 16

SymCryptFdef369MontgomeryReduceAsmSubLoop:
        mov     rax,[r12]
        sbb     rax,[rbx]
        mov     [rdi], rax

        mov     rax,[r12 + 8]
        sbb     rax,[rbx + 8]
        mov     [rdi + 8], rax

        mov     rax,[r12 + 16]
        sbb     rax,[rbx + 16]
        mov     [rdi + 16], rax

        lea     r12,[r12 + 24]
        lea     rbx,[rbx + 24]
        lea     rdi,[rdi + 24]

        dec     r13d
        jnz     SymCryptFdef369MontgomeryReduceAsmSubLoop

        
        
        sbb     rbp, 0              

align 16

SymCryptFdef369MontgomeryReduceAsmMaskedCopyLoop:
        mov     rax, [r10]
        mov     rcx, [r8]
        xor     rax, rcx
        and     rax, rbp
        xor     rax, rcx
        mov     [r8], rax

        mov     rax, [r10 + 8]
        mov     rcx, [r8 + 8]
        xor     rax, rcx
        and     rax, rbp
        xor     rax, rcx
        mov     [r8 + 8], rax

        mov     rax, [r10 + 16]
        mov     rcx, [r8 + 16]
        xor     rax, rcx
        and     rax, rbp
        xor     rax, rcx
        mov     [r8 + 16], rax

        

        add     r10, 24
        add     r8, 24
        dec     r9d
        jnz     SymCryptFdef369MontgomeryReduceAsmMaskedCopyLoop


BEGIN_EPILOGUE
pop r14
pop r13
pop r12
pop rbx
pop rbp
pop rdi
pop rsi
ret
NESTED_END SymCryptFdef369MontgomeryReduceAsm, _TEXT

























































END
