









































































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
    



    
    
    
    
    
    
    
















































LEAF_ENTRY SymCryptWipeAsm, _TEXT


        
        

        
        
        
        
        
        
        

        xorps   xmm0,xmm0               
        cmp     rdx,16
        jb      SymCryptWipeAsmSmall    

        test    rcx,15
        jnz     SymCryptWipeAsmUnaligned 
                                        
                                        

SymCryptWipeAsmAligned:
        
        
        
        
        
        
        
        test    rdx,16
        movaps  [rcx],xmm0               
        lea     r8,[rcx+16]
        cmovnz  rcx,r8                   

        sub     rdx,32                   
        jc      SymCryptWipeAsmTailOptional 

align 16

SymCryptWipeAsmLoop:
        movaps  [rcx],xmm0
        movaps  [rcx+16],xmm0            
        add     rcx,32
        sub     rdx,32
        jnc     SymCryptWipeAsmLoop

SymCryptWipeAsmTailOptional:
        
        

        and     edx,15
        jnz     SymCryptWipeAsmTail
        ret

SymCryptWipeAsmTail:
        
        
        xor     eax,eax
        mov     [rcx+rdx-16],rax
        mov     [rcx+rdx-8],rax
        ret

align 4

SymCryptWipeAsmUnaligned:

        
        
        
        
        xor     eax,eax
        mov     [rcx],rax
        mov     [rcx+8],rax

        mov     eax,ecx
        neg     eax                      
        and     eax,15
        add     rcx,rax
        sub     rdx,rax

        
        
        
        cmp     rdx,16
        jae     SymCryptWipeAsmAligned  

        
        
        
        
        xor     eax,eax
        mov     [rcx+rdx-16],rax
        mov     [rcx+rdx-8],rax
        ret

align 8

SymCryptWipeAsmSmall:
        
        
        
        
        
        
        
        
        
        

        xor     eax,eax

        cmp     edx, 8
        jb      SymCryptWipeAsmSmallLessThan8

        
        mov     [rcx],rax
        mov     [rcx+rdx-8],rax
        ret

SymCryptWipeAsmSmallLessThan8:
        cmp     edx, 4
        jb      SymCryptWipeAsmSmallLessThan4

        
        mov     [rcx],eax
        mov     [rcx+rdx-4],eax
        ret

SymCryptWipeAsmSmallLessThan4:
        cmp     edx, 2
        jb      SymCryptWipeAsmSmallLessThan2

        
        mov     [rcx],ax
        mov     [rcx+rdx-2],ax
        ret

SymCryptWipeAsmSmallLessThan2:
        or      edx,edx
        jz      SymCryptWipeAsmSmallDone

        
        mov     [rcx],al

SymCryptWipeAsmSmallDone:


BEGIN_EPILOGUE
ret
LEAF_END SymCryptWipeAsm, _TEXT

















END
