












































































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
    



    
    
    
    
    
    
    



























EXTERN SymCryptSha256K:DWORD
EXTERN BYTE_REVERSE_32:DWORD
EXTERN XMM_PACKLOW:DWORD
EXTERN XMM_PACKHIGH:DWORD


SHA2_INPUT_BLOCK_BYTES_LOG2 EQU 6
SHA2_INPUT_BLOCK_BYTES EQU 64
SHA2_ROUNDS EQU 64
SHA2_BYTES_PER_WORD EQU 4
SHA2_SIMD_REG_SIZE EQU 16
SHA2_SINGLE_BLOCK_THRESHOLD EQU (3 * SHA2_INPUT_BLOCK_BYTES)   












SHA2_SIMD_LANES EQU ((SHA2_SIMD_REG_SIZE) / (SHA2_BYTES_PER_WORD))
SHA2_EXPANDED_MESSAGE_SIZE EQU ((SHA2_ROUNDS) * (SHA2_SIMD_REG_SIZE))

















LOAD_MSG_WORD MACRO  ptr, res, ind

        mov     res, [ptr + (ind) * SHA2_BYTES_PER_WORD]
        bswap   res
        mov     [rsp + (ind) * SHA2_BYTES_PER_WORD], res
    
ENDM








GET_SIMD_BLOCK_COUNT MACRO  cbMsg, t

        shr     cbMsg, SHA2_INPUT_BLOCK_BYTES_LOG2              
        mov     t, SHA2_SIMD_LANES
        cmp     cbMsg, t
        cmova   cbMsg, t

ENDM












GET_PROCESSED_BYTES MACRO  cbMsg, cbProcessed, t

        mov     cbProcessed, cbMsg
        mov     t, SHA2_SIMD_LANES * SHA2_INPUT_BLOCK_BYTES
        cmp     cbProcessed, t
        cmova   cbProcessed, t
        and     cbProcessed, -SHA2_INPUT_BLOCK_BYTES

ENDM














ROUND_T5_BMI2_V1 MACRO  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale, c0_r1, c0_r2, c0_r3, c1_r1, c1_r2, c1_r3
                    
    
    
                                                         rorx t5, e, c1_r1                                                                           
                                                         rorx t4, e, c1_r2                                                  
                        mov  t1, f
                        andn t2, e, g               
                                                         rorx t3, e, c1_r3
        add h, [Wk + (rnd) * (scale)]
                        and  t1, e
                                                         xor  t5, t4
                        xor  t1, t2                                     
                                                         xor  t5, t3
        add h, t1                       
                                         rorx t2, a, c0_r1                                                          
                                                                        mov t3, b
                                                                        mov t4, b
                                         rorx t1, a, c0_r2                                                                  
        add h, t5
                                                                        or  t3, c   
                                                                        and t4, c
                                                                        and t3, a
                                                                        or  t4, t3
        add d, h
                                         xor  t2, t1
                                         rorx t5, a, c0_r3       
                                         xor  t2, t5                                                                        
        add h, t4                                                                                                                                       
        add h, t2
                                                                                                                                                                                                        
ENDM


ROUND_T5_BMI2_V2 MACRO  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale, c0_r1, c0_r2, c0_r3, c1_r1, c1_r2, c1_r3

    
    
                        mov  t1, f
                        andn t2, e, g               
                                                         rorx t5, e, c1_r1                                                                           
                        and  t1, e
                                                         rorx t4, e, c1_r2                                                  
                                                         rorx t3, e, c1_r3  
        add h, [Wk + (rnd) * (scale)]
                                                         xor t5, t4
                        xor  t1, t2                                     
                                                         xor  t5, t3
        add h, t1                       
        add h, t5
                                                                        mov t3, b
                                         rorx t1, a, c0_r2                                                                  
                                         rorx t2, a, c0_r1                                                          
                                                                        mov t4, b
                                                                        or  t3, c   
                                                                        and t4, c
        add d, h
                                                                        and t3, a
                                                                        or  t4, t3
                                         xor  t2, t1
                                         rorx t5, a, c0_r3       
                                         xor  t2, t5                                                                        
        add h, t4                                                                                                                                       
        add h, t2
                                                                                                                                                                                                        
ENDM



ROUND_256 MACRO  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale

    ROUND_T5_BMI2_V1 a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale, 2, 13, 22, 6, 11, 25

ENDM


ROUND_512 MACRO  a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale

    ROUND_T5_BMI2_V1 a, b, c, d, e, f, g, h, rnd, t1, t2, t3, t4, t5, Wk, scale, 28, 34, 39, 14, 18, 41

ENDM



SHA2_UPDATE_CV_HELPER MACRO  rcv, r0, r1, r2, r3, r4, r5, r6, r7

        add r0, [rcv + 0 * SHA2_BYTES_PER_WORD]
        mov [rcv + 0 * SHA2_BYTES_PER_WORD], r0
        add r1, [rcv + 1 * SHA2_BYTES_PER_WORD]
        mov [rcv + 1 * SHA2_BYTES_PER_WORD], r1
        add r2, [rcv + 2 * SHA2_BYTES_PER_WORD]
        mov [rcv + 2 * SHA2_BYTES_PER_WORD], r2
        add r3, [rcv + 3 * SHA2_BYTES_PER_WORD]
        mov [rcv + 3 * SHA2_BYTES_PER_WORD], r3
        add r4, [rcv + 4 * SHA2_BYTES_PER_WORD]
        mov [rcv + 4 * SHA2_BYTES_PER_WORD], r4
        add r5, [rcv + 5 * SHA2_BYTES_PER_WORD]
        mov [rcv + 5 * SHA2_BYTES_PER_WORD], r5
        add r6, [rcv + 6 * SHA2_BYTES_PER_WORD]
        mov [rcv + 6 * SHA2_BYTES_PER_WORD], r6
        add r7, [rcv + 7 * SHA2_BYTES_PER_WORD]     
        mov [rcv + 7 * SHA2_BYTES_PER_WORD], r7

ENDM
















SHA512_MSG_LOAD_TRANSPOSE_YMM MACRO  P, N, t, v, ind, kreverse, y1, y2, y3, y4, t1, t2, t3, t4

        
        
        mov t, 2 * SHA2_INPUT_BLOCK_BYTES + (ind) * 32
        mov v, 3 * SHA2_INPUT_BLOCK_BYTES + (ind) * 32
        cmp N, 4
        cmove t, v

        
        vmovdqu y1, YMMWORD ptr [P + 0 * SHA2_INPUT_BLOCK_BYTES + (ind) * 32]
        vpshufb y1, y1, kreverse
        vmovdqu y2, YMMWORD ptr [P + 1 * SHA2_INPUT_BLOCK_BYTES + (ind) * 32]
        vpshufb y2, y2, kreverse
        vmovdqu y3, YMMWORD ptr [P + 2 * SHA2_INPUT_BLOCK_BYTES + (ind) * 32]
        vpshufb y3, y3, kreverse
        
        
        
        
        
        vmovdqu y4, YMMWORD ptr [P + t]
        vpshufb y4, y4, kreverse
        
        SHA512_MSG_TRANSPOSE_YMM ind, y1, y2, y3, y4, t1, t2, t3, t4

ENDM














SHA512_MSG_TRANSPOSE_YMM MACRO  ind, y1, y2, y3, y4, t1, t2, t3, t4

        vpunpcklqdq t1, y1, y2
        vpunpcklqdq t2, y3, y4
        vpunpckhqdq t3, y1, y2
        vpunpckhqdq t4, y3, y4
        vperm2i128  y1, t1, t2, 20h
        vperm2i128  y2, t3, t4, 20h
        vperm2i128  y3, t1, t2, 31h
        vperm2i128  y4, t3, t4, 31h
        vmovdqu     YMMWORD ptr [rsp + (ind) * 128 + 0 * 32], y1
        vmovdqu     YMMWORD ptr [rsp + (ind) * 128 + 1 * 32], y2
        vmovdqu     YMMWORD ptr [rsp + (ind) * 128 + 2 * 32], y3
        vmovdqu     YMMWORD ptr [rsp + (ind) * 128 + 3 * 32], y4

ENDM
















SHA256_MSG_LOAD_TRANSPOSE_XMM MACRO  P, N, t1, t2, ind, kreverse, x0, x1, x2, x3, xt0, xt1, xt2, xt3

        
        
        mov     t2, 2 * SHA2_INPUT_BLOCK_BYTES + (ind) * 16
        mov     t1, 3 * SHA2_INPUT_BLOCK_BYTES + (ind) * 16 
        cmp     N, 4
        cmove   t2, t1

        
        movdqu  x0, XMMWORD ptr [P + 0 * SHA2_INPUT_BLOCK_BYTES + (ind) * 16]
        pshufb  x0, kreverse
        movdqu  x1, XMMWORD ptr [P + 1 * SHA2_INPUT_BLOCK_BYTES + (ind) * 16]
        pshufb  x1, kreverse
        movdqu  x2, XMMWORD ptr [P + 2 * SHA2_INPUT_BLOCK_BYTES + (ind) * 16]
        pshufb  x2, kreverse
        
        
        
        
        
        movdqu  x3, XMMWORD ptr [P + t2]
        pshufb  x3, kreverse

        SHA256_MSG_TRANSPOSE_XMM ind, x0, x1, x2, x3, xt0, xt1, xt2, xt3

ENDM














SHA256_MSG_TRANSPOSE_XMM MACRO  ind, x0, x1, x2, x3, t0, t1, t2, t3

        
        
        
        
        movdqa      t0, x0
        punpckhdq   t0, x1  
        punpckldq   x0, x1  
        movdqa      t1, x2
        punpckhdq   t1, x3  
        punpckldq   x2, x3  

        movdqa      x1, x0
        punpckhqdq  x1, x2  
        punpcklqdq  x0, x2  
        movdqa      XMMWORD ptr [rsp + 64 * (ind) + 0 * 16], x0
        movdqa      XMMWORD ptr [rsp + 64 * (ind) + 1 * 16], x1

        movdqa      x3, t0
        punpckhqdq  x3, t1  
        punpcklqdq  t0, t1  
        movdqa      XMMWORD ptr [rsp + 64 * (ind) + 2 * 16], t0
        movdqa      XMMWORD ptr [rsp + 64 * (ind) + 3 * 16], x3

ENDM










ROR32_XMM MACRO  x, c, res, t1

    movdqa  res, x
    movdqa  t1, x
    psrld   res, c
    pslld   t1, 32 - c
    pxor    res, t1 
    
ENDM










LSIGMA_XMM MACRO  x, c1, c2, c3, res, t1, t2

        ROR32_XMM   x, c1, res, t1
        ROR32_XMM   x, c2, t2, t1
        movdqa      t1, x
        psrld       t1, c3
        pxor        res, t2
        pxor        res, t1

ENDM

















SHA256_MSG_EXPAND_4BLOCKS MACRO  y0, y1, y9, y14, rnd, t1, t2, t3, t4, t5, t6, Wx, k256

        movd        t1, DWORD ptr [k256 + 4 * (rnd - 16)]
        pshufd      t1, t1, 0
        paddd       t1, y0                                      
        movdqa      XMMWORD ptr [Wx + (rnd - 16) * 16], t1      
        
        LSIGMA_XMM  y14, 17, 19, 10, t4, t5, t3                 
        LSIGMA_XMM  y1,   7, 18,  3, t1, t5, t3                 
        paddd       y0, y9                                      
        paddd       t1, y0                                      
        paddd       t1, t4                                      
        movdqa      y0, XMMWORD ptr [Wx + (rnd - 14) * 16]      
        movdqa      XMMWORD ptr [Wx + (rnd) * 16], t1           

ENDM












SHA256_MSG_EXPAND_1BLOCK MACRO  x0, x1, x2, x3, t1, t2, t3, t4, t5, t6, karr, ind, packlow, packhigh

        
        
        
        
        

        
        
        
        
        

        movdqa      t2, x1
        palignr     t2, x0, 4                           

        
        pshufd      t1, x3, 0fah                    
        movdqa      t5, t1
        movdqa      t3, t1
        psrlq       t5, 17
        psrlq       t3, 19
        pxor        t5, t3
        psrld       t1, 10
        pxor        t5, t1
        pshufb      t5, packlow                         

        LSIGMA_XMM  t2, 7, 18, 3, t3, t1, t6            

        paddd       x0, t5                              

        movdqa      t4, x3
        palignr     t4, x2, 4                           

        paddd       t4, t3                              
        paddd       x0, t4                              

        
        pshufd      t1, x0, 50h                     
        movdqa      t2, t1
        movdqa      t3, t1
        psrlq       t2, 17
        psrlq       t3, 19
        pxor        t2, t3
        psrld       t1, 10
        pxor        t2, t1
        pshufb      t2, packhigh                        

        movdqa      t6, XMMWORD ptr [karr + ind * 16]   
        paddd       x0, t2                              
        paddd       t6, x0                              
        movdqa      XMMWORD ptr [rsp + ind * 16], t6

ENDM










SHA256_MSG_ADD_CONST MACRO  rnd, t1, t2, Wx, k256

        movd    t1, DWORD ptr [k256 + 4 * (rnd)]
        pshufd  t1, t1, 0
        movdqa  t2, XMMWORD ptr [Wx + 16 * (rnd)]
        paddd   t1, t2
        movdqa XMMWORD ptr [Wx + (rnd) * 16], t1

ENDM











































































NESTED_ENTRY SymCryptSha256AppendBlocks_xmm_ssse3_asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 1208
save_xmm128 xmm6, 1040
save_xmm128 xmm7, 1056
save_xmm128 xmm8, 1072
save_xmm128 xmm9, 1088
save_xmm128 xmm10, 1104
save_xmm128 xmm11, 1120
save_xmm128 xmm12, 1136
save_xmm128 xmm13, 1152
save_xmm128 xmm14, 1168
save_xmm128 xmm15, 1184

END_PROLOGUE


        
        
        
        

        mov     [rsp + 1280 ], rcx
        mov     [rsp + 1288 ], rdx
        mov     [rsp + 1296 ], r8
        mov     [rsp + 1304 ], r9


        
        
        
        
        
        
        
        
        
        
        
        
        mov     edi, 16 * SHA2_BYTES_PER_WORD
        mov     ebp, SHA2_EXPANDED_MESSAGE_SIZE
        cmp     r8, SHA2_SINGLE_BLOCK_THRESHOLD 
        cmovae  edi, ebp      
        mov     [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 1 * 8)], edi

        mov     rbx, rcx
        mov     eax, DWORD ptr [rbx +  0]
        mov     ecx, DWORD ptr [rbx +  4]
        mov     edx, DWORD ptr [rbx +  8]
        mov     r8d, DWORD ptr [rbx + 12]
        mov     r9d, DWORD ptr [rbx + 16]
        mov     r10d, DWORD ptr [rbx + 20]
        mov     r11d, DWORD ptr [rbx + 24]
        mov     esi, DWORD ptr [rbx + 28]

        
        
        mov     rdi, [rsp + 1296 ]
        cmp     rdi, SHA2_SINGLE_BLOCK_THRESHOLD
        jb      single_block_entry

        align 16
process_blocks:
        
        GET_SIMD_BLOCK_COUNT rdi, rbp     
        mov     [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)], rdi

        
        
        
        mov rbp, [rsp + 1288 ]
        movdqa xmm8, XMMWORD ptr [BYTE_REVERSE_32]
        SHA256_MSG_LOAD_TRANSPOSE_XMM rbp, rdi, rbx, r12, 0, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7
        SHA256_MSG_LOAD_TRANSPOSE_XMM rbp, rdi, rbx, r12, 1, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7
        SHA256_MSG_LOAD_TRANSPOSE_XMM rbp, rdi, rbx, r12, 2, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7
        SHA256_MSG_LOAD_TRANSPOSE_XMM rbp, rdi, rbx, r12, 3, xmm8, xmm0, xmm1, xmm2, xmm3,    xmm4, xmm5, xmm6, xmm7

        
        
        movdqa xmm0, XMMWORD ptr [rsp + 16 *  0]
        movdqa xmm1, XMMWORD ptr [rsp + 16 *  1]
        movdqa xmm2, XMMWORD ptr [rsp + 16 *  9]
        movdqa xmm3, XMMWORD ptr [rsp + 16 * 10]
        movdqa xmm4, XMMWORD ptr [rsp + 16 * 11]
        movdqa xmm5, XMMWORD ptr [rsp + 16 * 12]
        movdqa xmm6, XMMWORD ptr [rsp + 16 * 13]
        movdqa xmm7, XMMWORD ptr [rsp + 16 * 14]
        movdqa xmm8, XMMWORD ptr [rsp + 16 * 15]

        lea     r14, [rsp]                                    
        lea     r15, [SymCryptSha256K]  

expand_process_first_block:

        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm2, xmm7, (16 + 0), xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm3, xmm8, (16 + 1), xmm2, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15
        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm4, xmm9, (16 + 2), xmm3, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm5, xmm2, (16 + 3), xmm4, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15     
        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm6, xmm3, (16 + 4), xmm5, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm7, xmm4, (16 + 5), xmm6, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15
        SHA256_MSG_EXPAND_4BLOCKS   xmm0, xmm1, xmm8, xmm5, (16 + 6), xmm7, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15
        SHA256_MSG_EXPAND_4BLOCKS   xmm1, xmm0, xmm9, xmm6, (16 + 7), xmm8, xmm10, xmm11, xmm12, xmm13, xmm14, r14, r15

        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi, 0, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d, 1, edi, ebp, ebx, r12d, r13d, r14, 16  
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d, 2, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d, 3, edi, ebp, ebx, r12d, r13d, r14, 16      
        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d, 4, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx, 5, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx, 6, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax, 7, edi, ebp, ebx, r12d, r13d, r14, 16

        lea     rdi, [SymCryptSha256K + 48 * 4]
        add     r14, 8 * 16 
        add     r15, 8 * 4  
        cmp     r15, rdi
        jb      expand_process_first_block

        
final_rounds:
        SHA256_MSG_ADD_CONST 0, xmm1, xmm2, r14, r15
        SHA256_MSG_ADD_CONST 1, xmm1, xmm2, r14, r15
        SHA256_MSG_ADD_CONST 2, xmm1, xmm2, r14, r15
        SHA256_MSG_ADD_CONST 3, xmm1, xmm2, r14, r15
        SHA256_MSG_ADD_CONST 4, xmm1, xmm2, r14, r15
        SHA256_MSG_ADD_CONST 5, xmm1, xmm2, r14, r15
        SHA256_MSG_ADD_CONST 6, xmm1, xmm2, r14, r15
        SHA256_MSG_ADD_CONST 7, xmm1, xmm2, r14, r15
        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi, 0, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d, 1, edi, ebp, ebx, r12d, r13d, r14, 16  
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d, 2, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d, 3, edi, ebp, ebx, r12d, r13d, r14, 16      
        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d, 4, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx, 5, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx, 6, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax, 7, edi, ebp, ebx, r12d, r13d, r14, 16
        
        lea     rdi, [SymCryptSha256K + 64 * 4]
        add     r14, 8 * 16 
        add     r15, 8 * 4  
        cmp     r15, rdi
        jb      final_rounds

        mov     rdi, [rsp + 1280 ]
        SHA2_UPDATE_CV_HELPER rdi, eax, ecx, edx, r8d, r9d, r10d, r11d, esi

        
        
        dec qword ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]

        lea     r14, [rsp + 4]    

block_begin:
        
        mov     r15d, 64 / 8

        align 16
inner_loop:
        
        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi,  0, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d,  1, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d,  2, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d,  3, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d,  4, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx,  5, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx,  6, edi, ebp, ebx, r12d, r13d, r14, 16
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax,  7, edi, ebp, ebx, r12d, r13d, r14, 16

        add     r14, 8 * 16 
        sub     r15d, 1
        jnz     inner_loop

        add     r14, (4 - 64 * 16)  
    
        mov     rdi, [rsp + 1280 ]
        SHA2_UPDATE_CV_HELPER rdi, eax, ecx, edx, r8d, r9d, r10d, r11d, esi
        
        dec     QWORD ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]
        jnz     block_begin

        
        mov     rdi, [rsp + 1296 ]
        GET_PROCESSED_BYTES rdi, rbp, rbx     
        sub     rdi, rbp
        add     QWORD ptr [rsp + 1288 ], rbp
        mov     QWORD ptr [rsp + 1296 ], rdi
        cmp     rdi, SHA2_SINGLE_BLOCK_THRESHOLD
        jae     process_blocks


        align 16
single_block_entry:

        cmp     rdi, SHA2_INPUT_BLOCK_BYTES      
        jb      done

        
        
        movdqa  xmm13, XMMWORD ptr [BYTE_REVERSE_32]
        movdqa  xmm14, XMMWORD ptr [XMM_PACKLOW]
        movdqa  xmm15, XMMWORD ptr [XMM_PACKHIGH]

single_block_start:

        mov     r14, [rsp + 1288 ]
        lea     r15, [SymCryptSha256K]              
        
        
        
        
        
        movdqu  xmm0, XMMWORD ptr [r14 + 0 * 16]
        movdqu  xmm1, XMMWORD ptr [r14 + 1 * 16]
        movdqu  xmm2, XMMWORD ptr [r14 + 2 * 16]
        movdqu  xmm3, XMMWORD ptr [r14 + 3 * 16]
        pshufb  xmm0, xmm13
        pshufb  xmm1, xmm13
        pshufb  xmm2, xmm13
        pshufb  xmm3, xmm13     
        movdqa  xmm8,  XMMWORD ptr [r15 + 0 * 16]
        movdqa  xmm9,  XMMWORD ptr [r15 + 1 * 16]
        movdqa  xmm10, XMMWORD ptr [r15 + 2 * 16]
        movdqa  xmm11, XMMWORD ptr [r15 + 3 * 16]
        paddd   xmm8,  xmm0
        paddd   xmm9,  xmm1
        paddd   xmm10, xmm2
        paddd   xmm11, xmm3
        movdqa  XMMWORD ptr [rsp + 0 * 16], xmm8
        movdqa  XMMWORD ptr [rsp + 1 * 16], xmm9
        movdqa  XMMWORD ptr [rsp + 2 * 16], xmm10
        movdqa  XMMWORD ptr [rsp + 3 * 16], xmm11

inner_loop_single:

        add     r15, 16 * 4

        
        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi,  0, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d,  1, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d,  2, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d,  3, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK xmm0, xmm1, xmm2, xmm3,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 0, xmm14, xmm15

        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d,  4, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx,  5, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx,  6, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax,  7, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK xmm1, xmm2, xmm3, xmm0,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 1, xmm14, xmm15

        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi,  8, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d,  9, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d, 10, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d, 11, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK xmm2, xmm3, xmm0, xmm1,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 2, xmm14, xmm15

        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d, 12, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx, 13, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx, 14, edi, ebp, ebx, r12d, r13d, rsp, 4
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax, 15, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK xmm3, xmm0, xmm1, xmm2,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 3, xmm14, xmm15

        lea     rdi, [SymCryptSha256K + 48 * 4]
        cmp     r15, rdi
        jb      inner_loop_single

        lea r14, [rsp]
        lea r15, [rsp + 16 * 4]

single_block_final_rounds:

        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi,  0, edi, ebp, ebx, r12d, r13d, r14, 4
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d,  1, edi, ebp, ebx, r12d, r13d, r14, 4
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d,  2, edi, ebp, ebx, r12d, r13d, r14, 4
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d,  3, edi, ebp, ebx, r12d, r13d, r14, 4
        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d,  4, edi, ebp, ebx, r12d, r13d, r14, 4
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx,  5, edi, ebp, ebx, r12d, r13d, r14, 4
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx,  6, edi, ebp, ebx, r12d, r13d, r14, 4
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax,  7, edi, ebp, ebx, r12d, r13d, r14, 4
    
        add r14, 8 * 4
        cmp r14, r15
        jb single_block_final_rounds
    
        mov     rdi, [rsp + 1280 ]
        SHA2_UPDATE_CV_HELPER rdi, eax, ecx, edx, r8d, r9d, r10d, r11d, esi

        
        mov     rdi, [rsp + 1296 ]
        sub     rdi, SHA2_INPUT_BLOCK_BYTES
        add     QWORD ptr [rsp + 1288 ], SHA2_INPUT_BLOCK_BYTES
        mov     QWORD ptr [rsp + 1296 ], rdi
        cmp     rdi, SHA2_INPUT_BLOCK_BYTES
        jae     single_block_start

done:

        
        mov     rbp, [rsp + 1304 ]
        mov     QWORD ptr [rbp], rdi

        
        xor     rax, rax
        mov     rdi, rsp
        mov     ecx, [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 1 * 8)]

        
        pxor    xmm0, xmm0
        movaps  [rdi + 0 * 16], xmm0
        movaps  [rdi + 1 * 16], xmm0
        movaps  [rdi + 2 * 16], xmm0
        movaps  [rdi + 3 * 16], xmm0
        add     rdi, 4 * 16

        
        sub     ecx, 4 * 16 
        jz      nowipe
        rep     stosb

nowipe:



movdqa xmm6, xmmword ptr [rsp + 1040]
movdqa xmm7, xmmword ptr [rsp + 1056]
movdqa xmm8, xmmword ptr [rsp + 1072]
movdqa xmm9, xmmword ptr [rsp + 1088]
movdqa xmm10, xmmword ptr [rsp + 1104]
movdqa xmm11, xmmword ptr [rsp + 1120]
movdqa xmm12, xmmword ptr [rsp + 1136]
movdqa xmm13, xmmword ptr [rsp + 1152]
movdqa xmm14, xmmword ptr [rsp + 1168]
movdqa xmm15, xmmword ptr [rsp + 1184]
add rsp, 1208
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
NESTED_END SymCryptSha256AppendBlocks_xmm_ssse3_asm, _TEXT





























































END
