












































































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
EXTERN BYTE_REVERSE_32X2:DWORD
EXTERN XMM_PACKLOW:DWORD
EXTERN XMM_PACKHIGH:DWORD


SHA2_INPUT_BLOCK_BYTES_LOG2 EQU 6
SHA2_INPUT_BLOCK_BYTES EQU 64
SHA2_ROUNDS EQU 64
SHA2_BYTES_PER_WORD EQU 4
SHA2_SIMD_REG_SIZE EQU 32
SHA2_SINGLE_BLOCK_THRESHOLD EQU (5 * SHA2_INPUT_BLOCK_BYTES)   












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













SHA256_MSG_LOAD_TRANSPOSE_YMM MACRO  P, N, t1, t2, t3, t4, Wbase
        
        vmovdqa ymm15, YMMWORD ptr [BYTE_REVERSE_32X2]

        
        
        
        vmovdqu ymm13, YMMWORD ptr [P + 0 * 64]
        vpshufb ymm13, ymm13, ymm15
        vmovdqu ymm7,  YMMWORD ptr [P + 1 * 64]
        vpshufb ymm7,  ymm7, ymm15
        vmovdqu ymm10, YMMWORD ptr [P + 2 * 64]
        vpshufb ymm10,  ymm10, ymm15
        vmovdqu ymm0,  YMMWORD ptr [P + 3 * 64]
        vpshufb ymm0,  ymm0, ymm15
        vmovdqu ymm14, YMMWORD ptr [P + 4 * 64]
        vpshufb ymm14,  ymm14, ymm15
        
        lea t1, [P + 4 * 64]
        lea t2, [P + 5 * 64]
        lea t3, [P + 6 * 64]
        lea t4, [P + 7 * 64]

        cmp     N, 6
        cmovb   t2, t1 
        cmovbe  t3, t1 
        cmp     N, 8
        cmovb   t4, t1 

        vmovdqu ymm8, YMMWORD ptr [t2]
        vpshufb ymm8,  ymm8, ymm15

        vmovdqu ymm11, YMMWORD ptr [t3]
        vpshufb ymm11,  ymm11, ymm15

        vmovdqu ymm9, YMMWORD ptr [t4]
        vpshufb ymm9,  ymm9, ymm15

        SHA256_MSG_TRANSPOSE_YMM Wbase

ENDM
















SHA256_MSG_TRANSPOSE_YMM MACRO  Wbase

        
        
        
        
        
        
        
        

        vpunpckldq ymm1,  ymm13, ymm7
        vpunpckldq ymm5,  ymm10, ymm0
        vpunpckldq ymm2,  ymm14, ymm8
        vpunpckldq ymm6,  ymm11, ymm9
        vpunpckhdq ymm12, ymm13, ymm7
        vpunpckhdq ymm3,  ymm10, ymm0
        vpunpckhdq ymm4,  ymm14, ymm8
        vpunpckhdq ymm15, ymm11, ymm9

        vpunpcklqdq ymm13, ymm1,  ymm5
        vpunpcklqdq ymm7,  ymm2,  ymm6
        vpunpckhqdq ymm14, ymm1,  ymm5
        vpunpckhqdq ymm8,  ymm2,  ymm6
        vpunpcklqdq ymm10, ymm12, ymm3
        vpunpcklqdq ymm0,  ymm4,  ymm15
        vpunpckhqdq ymm11, ymm12, ymm3
        vpunpckhqdq ymm9,  ymm4,  ymm15

        vperm2i128  ymm1, ymm13, ymm7, 20h
        vperm2i128  ymm2, ymm14, ymm8, 20h
        vperm2i128  ymm3, ymm10, ymm0, 20h
        vperm2i128  ymm4, ymm11, ymm9, 20h
        vperm2i128  ymm5, ymm13, ymm7, 31h
        vperm2i128  ymm6, ymm14, ymm8, 31h          
        vperm2i128  ymm7, ymm10, ymm0, 31h
        vperm2i128  ymm8, ymm11, ymm9, 31h

        vmovdqu YMMWORD ptr [Wbase + 0 * 32], ymm1
        vmovdqu YMMWORD ptr [Wbase + 1 * 32], ymm2  
        vmovdqu YMMWORD ptr [Wbase + 2 * 32], ymm3
        vmovdqu YMMWORD ptr [Wbase + 3 * 32], ymm4
        vmovdqu YMMWORD ptr [Wbase + 4 * 32], ymm5
        vmovdqu YMMWORD ptr [Wbase + 5 * 32], ymm6
        vmovdqu YMMWORD ptr [Wbase + 6 * 32], ymm7
        vmovdqu YMMWORD ptr [Wbase + 7 * 32], ymm8

ENDM











ROR32_YMM MACRO  x, c, t1, t2

    vpsrld  t1, x, c
    vpslld  t2, x, 32 - c
    vpxor   t1, t1, t2  
    
ENDM










LSIGMA_YMM MACRO  x, c1, c2, c3, t1, t2, t3, t4

        ROR32_YMM   x, c1, t1, t2
        ROR32_YMM   x, c2, t3, t4
        vpsrld      t2, x, c3
        vpxor       t1, t1, t3
        vpxor       t1, t1, t2

ENDM


















SHA256_MSG_EXPAND_8BLOCKS MACRO  y0, y1, y9, y14, rnd, t1, t2, t3, t4, t5, t6, krot8, Wx, k256

        vpbroadcastd t6, DWORD ptr [k256 + 4 * (rnd - 16)]      
        vpaddd      t6, t6, y0                                  
        vmovdqu     YMMWORD ptr [Wx + (rnd - 16) * 32], t6      
        
        LSIGMA_YMM  y14, 17, 19, 10, t4, t5, t3, t1             
        LSIGMA_YMM  y1,   7, 18,  3, t2, t1, t6, t3             
        vpaddd      t5, y9, y0                                  
        vpaddd      t3, t2, t4                                  
        vpaddd      t1, t3, t5                                  
        vmovdqu     y0, YMMWORD ptr [Wx + (rnd - 14) * 32]      
        vmovdqu     YMMWORD ptr [Wx + rnd * 32], t1             

ENDM



























SHA256_MSG_EXPAND_1BLOCK_0 MACRO  x0, x1, x2, x3, t1, t2, t3, t4, t5, t6, karr, ind, packlow, packhigh

        vpalignr    t2, x1, x0, 4                       
        vpshufd     t1, x3, 0fah                    
        vpsrlq      t5, t1, 17
        vpsrlq      t3, t1, 19
        vpxor       t5, t5, t3
        vpsrld      t1, t1, 10
        vpxor       t5, t5, t1
        vpshufb     t5, t5, packlow                     
        LSIGMA_YMM  t2, 7, 18, 3, t3, t1, t6, t4        

ENDM
SHA256_MSG_EXPAND_1BLOCK_1 MACRO  x0, x1, x2, x3, t1, t2, t3, t4, t5, t6, karr, ind, packlow, packhigh

        vpalignr    t4, x3, x2, 4                       
        vpaddd      x0, x0, t3                          
        vpaddd      t5, t5, t4                          
        vpaddd      x0, x0, t5                          

ENDM
SHA256_MSG_EXPAND_1BLOCK_2 MACRO  x0, x1, x2, x3, t1, t2, t3, t4, t5, t6, karr, ind, packlow, packhigh

        vpshufd     t1, x0, 50h                     
        vpsrlq      t2, t1, 17
        vpsrlq      t3, t1, 19
        vpxor       t2, t2, t3
        vpsrld      t1, t1, 10
        vpxor       t2, t2, t1

ENDM
SHA256_MSG_EXPAND_1BLOCK_3 MACRO  x0, x1, x2, x3, t1, t2, t3, t4, t5, t6, karr, ind, packlow, packhigh

        vpshufb     t2, t2, packhigh                    
        vmovdqa     t6, XMMWORD ptr [karr + ind * 16]   
        vpaddd      x0, x0, t2                          
        vpaddd      t6, t6, x0                          
        vmovdqa     XMMWORD ptr [rsp + ind * 16], t6

ENDM










SHA256_MSG_ADD_CONST MACRO  rnd, t1, t2, Wx, k256

        vpbroadcastd t2, DWORD ptr [k256 + 4 * (rnd)]
        vmovdqu t1, YMMWORD ptr [Wx + 32 * (rnd)]
        vpaddd  t1, t1, t2
        vmovdqu YMMWORD ptr [Wx + 32 * (rnd)], t1

ENDM








































































NESTED_ENTRY SymCryptSha256AppendBlocks_ymm_avx2_asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 2232
save_xmm128 xmm6, 2064
save_xmm128 xmm7, 2080
save_xmm128 xmm8, 2096
save_xmm128 xmm9, 2112
save_xmm128 xmm10, 2128
save_xmm128 xmm11, 2144
save_xmm128 xmm12, 2160
save_xmm128 xmm13, 2176
save_xmm128 xmm14, 2192
save_xmm128 xmm15, 2208

END_PROLOGUE


        
        
        
        

        vzeroupper

        mov     [rsp + 2304 ], rcx
        mov     [rsp + 2312 ], rdx
        mov     [rsp + 2320 ], r8
        mov     [rsp + 2328 ], r9

        
        
        
        
        
        
        
        
        
        
        
        
        mov     edi, 16 * SHA2_BYTES_PER_WORD
        mov     ebp, SHA2_EXPANDED_MESSAGE_SIZE
        cmp     r8, SHA2_SINGLE_BLOCK_THRESHOLD 
        cmovae  edi, ebp      
        mov     [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 1 * 8)], edi

        mov     rbx, rcx
        mov     eax, [rbx +  0]
        mov     ecx, [rbx +  4]
        mov     edx, [rbx +  8]
        mov     r8d, [rbx + 12]
        mov     r9d, [rbx + 16]
        mov     r10d, [rbx + 20]
        mov     r11d, [rbx + 24]
        mov     esi, [rbx + 28]


        
        
        mov     rdi, [rsp + 2320 ]
        cmp     rdi, SHA2_SINGLE_BLOCK_THRESHOLD
        jb      single_block_entry

        align 16
process_blocks:
        
        GET_SIMD_BLOCK_COUNT rdi, rbp     
        mov     [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)], rdi

        
        
        
        mov     rbp, [rsp + 2312 ]
        lea     rbx, [rsp]
msg_transpose:      
        
        SHA256_MSG_LOAD_TRANSPOSE_YMM   rbp, rdi, r12, r13, r14, r15, rbx
        add      rbp, 32
        add     rbx, 256    
        lea     r12, [rsp + 256]
        cmp     rbx, r12
        jbe     msg_transpose


        lea     r14, [rsp]
        lea     r15, [SymCryptSha256K]

        vmovdqu ymm0, YMMWORD ptr [rsp + 32 *  0]
        vmovdqu ymm1, YMMWORD ptr [rsp + 32 *  1]

        
        
        
        
        
        
        
        

        align 16
expand_process_first_block:

        SHA256_MSG_EXPAND_8BLOCKS   ymm0, ymm1, ymm2, ymm7, (16 + 0), ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi, 0, edi, ebp, ebx, r12d, r13d, r14, 32
        SHA256_MSG_EXPAND_8BLOCKS   ymm1, ymm0, ymm3, ymm8, (16 + 1), ymm2, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d, 1, edi, ebp, ebx, r12d, r13d, r14, 32  
        SHA256_MSG_EXPAND_8BLOCKS   ymm0, ymm1, ymm4, ymm9, (16 + 2), ymm3, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d, 2, edi, ebp, ebx, r12d, r13d, r14, 32
        SHA256_MSG_EXPAND_8BLOCKS   ymm1, ymm0, ymm5, ymm2, (16 + 3), ymm4, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d, 3, edi, ebp, ebx, r12d, r13d, r14, 32      
        
        SHA256_MSG_EXPAND_8BLOCKS   ymm0, ymm1, ymm6, ymm3, (16 + 4), ymm5, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d, 4, edi, ebp, ebx, r12d, r13d, r14, 32
        SHA256_MSG_EXPAND_8BLOCKS   ymm1, ymm0, ymm7, ymm4, (16 + 5), ymm6, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx, 5, edi, ebp, ebx, r12d, r13d, r14, 32
        SHA256_MSG_EXPAND_8BLOCKS   ymm0, ymm1, ymm8, ymm5, (16 + 6), ymm7, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx, 6, edi, ebp, ebx, r12d, r13d, r14, 32
        SHA256_MSG_EXPAND_8BLOCKS   ymm1, ymm0, ymm9, ymm6, (16 + 7), ymm8, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax, 7, edi, ebp, ebx, r12d, r13d, r14, 32

        lea     rdi, [SymCryptSha256K + 48 * 4]
        add     r14, 8 * 32 
        add     r15, 8 * 4  
        cmp     r15, rdi
        jb      expand_process_first_block

        
final_rounds:
        SHA256_MSG_ADD_CONST 0, ymm1, ymm2, r14, r15
        SHA256_MSG_ADD_CONST 1, ymm1, ymm2, r14, r15
        SHA256_MSG_ADD_CONST 2, ymm1, ymm2, r14, r15
        SHA256_MSG_ADD_CONST 3, ymm1, ymm2, r14, r15
        SHA256_MSG_ADD_CONST 4, ymm1, ymm2, r14, r15
        SHA256_MSG_ADD_CONST 5, ymm1, ymm2, r14, r15
        SHA256_MSG_ADD_CONST 6, ymm1, ymm2, r14, r15
        SHA256_MSG_ADD_CONST 7, ymm1, ymm2, r14, r15
        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi, 0, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d, 1, edi, ebp, ebx, r12d, r13d, r14, 32  
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d, 2, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d, 3, edi, ebp, ebx, r12d, r13d, r14, 32      
        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d, 4, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx, 5, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx, 6, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax, 7, edi, ebp, ebx, r12d, r13d, r14, 32
            
        lea     rdi, [SymCryptSha256K + 64 * 4]
        add     r14, 8 * 32 
        add     r15, 8 * 4  
        cmp     r15, rdi
        jb      final_rounds

        mov rdi, [rsp + 2304 ]
        SHA2_UPDATE_CV_HELPER rdi, eax, ecx, edx, r8d, r9d, r10d, r11d, esi

        
        
        dec qword ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]

        lea     r14, [rsp + 4]    

block_begin:
        
        mov     r15d, 64 / 8

        align 16
inner_loop:
        
        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi,  0, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d,  1, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d,  2, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d,  3, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d,  4, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx,  5, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx,  6, edi, ebp, ebx, r12d, r13d, r14, 32
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax,  7, edi, ebp, ebx, r12d, r13d, r14, 32

        add     r14, 8 * 32 
        sub     r15d, 1
        jnz     inner_loop

        add     r14, (4 - 64 * 32)  
                
        mov rdi, [rsp + 2304 ]
        SHA2_UPDATE_CV_HELPER rdi, eax, ecx, edx, r8d, r9d, r10d, r11d, esi
        
        dec     QWORD ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]
        jnz     block_begin

        
        mov     rdi, [rsp + 2320 ]
        GET_PROCESSED_BYTES rdi, rbp, rbx     
        sub     rdi, rbp
        add     QWORD ptr [rsp + 2312 ], rbp
        mov     QWORD ptr [rsp + 2320 ], rdi
        cmp     rdi, SHA2_SINGLE_BLOCK_THRESHOLD
        jae     process_blocks


        align 16
single_block_entry:

        cmp     rdi, SHA2_INPUT_BLOCK_BYTES      
        jb      done

        
        
        vmovdqa xmm13, XMMWORD ptr [BYTE_REVERSE_32X2]
        vmovdqa xmm14, XMMWORD ptr [XMM_PACKLOW]
        vmovdqa xmm15, XMMWORD ptr [XMM_PACKHIGH]

single_block_start:

        mov     r14, [rsp + 2312 ]
        lea     r15, [SymCryptSha256K]              

        
        
        
        
        vmovdqu xmm0, XMMWORD ptr [r14 + 0 * 16]
        vmovdqu xmm1, XMMWORD ptr [r14 + 1 * 16]
        vmovdqu xmm2, XMMWORD ptr [r14 + 2 * 16]
        vmovdqu xmm3, XMMWORD ptr [r14 + 3 * 16]
        vpshufb xmm0, xmm0, xmm13
        vpshufb xmm1, xmm1, xmm13
        vpshufb xmm2, xmm2, xmm13
        vpshufb xmm3, xmm3, xmm13       
        vmovdqa xmm4, XMMWORD ptr [r15 + 0 * 16]
        vmovdqa xmm5, XMMWORD ptr [r15 + 1 * 16]
        vmovdqa xmm6, XMMWORD ptr [r15 + 2 * 16]
        vmovdqa xmm7, XMMWORD ptr [r15 + 3 * 16]
        vpaddd  xmm4, xmm4, xmm0
        vpaddd  xmm5, xmm5, xmm1
        vpaddd  xmm6, xmm6, xmm2
        vpaddd  xmm7, xmm7, xmm3
        vmovdqa XMMWORD ptr [rsp + 0 * 16], xmm4
        vmovdqa XMMWORD ptr [rsp + 1 * 16], xmm5
        vmovdqa XMMWORD ptr [rsp + 2 * 16], xmm6
        vmovdqa XMMWORD ptr [rsp + 3 * 16], xmm7

inner_loop_single:

        add r15, 16 * 4

        
        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi,  0, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_0 xmm0, xmm1, xmm2, xmm3,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 0, xmm14, xmm15
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d,  1, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_1 xmm0, xmm1, xmm2, xmm3,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 0, xmm14, xmm15
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d,  2, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_2 xmm0, xmm1, xmm2, xmm3,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 0, xmm14, xmm15
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d,  3, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_3 xmm0, xmm1, xmm2, xmm3,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 0, xmm14, xmm15

        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d,  4, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_0 xmm1, xmm2, xmm3, xmm0,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 1, xmm14, xmm15
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx,  5, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_1 xmm1, xmm2, xmm3, xmm0,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 1, xmm14, xmm15
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx,  6, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_2 xmm1, xmm2, xmm3, xmm0,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 1, xmm14, xmm15
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax,  7, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_3 xmm1, xmm2, xmm3, xmm0,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 1, xmm14, xmm15

        ROUND_256    eax, ecx, edx, r8d, r9d, r10d, r11d, esi,  8, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_0 xmm2, xmm3, xmm0, xmm1,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 2, xmm14, xmm15
        ROUND_256    esi, eax, ecx, edx, r8d, r9d, r10d, r11d,  9, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_1 xmm2, xmm3, xmm0, xmm1,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 2, xmm14, xmm15
        ROUND_256    r11d, esi, eax, ecx, edx, r8d, r9d, r10d, 10, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_2 xmm2, xmm3, xmm0, xmm1,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 2, xmm14, xmm15
        ROUND_256    r10d, r11d, esi, eax, ecx, edx, r8d, r9d, 11, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_3 xmm2, xmm3, xmm0, xmm1,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 2, xmm14, xmm15

        ROUND_256    r9d, r10d, r11d, esi, eax, ecx, edx, r8d, 12, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_0 xmm3, xmm0, xmm1, xmm2,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 3, xmm14, xmm15
        ROUND_256    r8d, r9d, r10d, r11d, esi, eax, ecx, edx, 13, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_1 xmm3, xmm0, xmm1, xmm2,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 3, xmm14, xmm15
        ROUND_256    edx, r8d, r9d, r10d, r11d, esi, eax, ecx, 14, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_2 xmm3, xmm0, xmm1, xmm2,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 3, xmm14, xmm15
        ROUND_256    ecx, edx, r8d, r9d, r10d, r11d, esi, eax, 15, edi, ebp, ebx, r12d, r13d, rsp, 4
        SHA256_MSG_EXPAND_1BLOCK_3 xmm3, xmm0, xmm1, xmm2,  xmm4, xmm5, xmm6, xmm7, xmm8, xmm9,  r15, 3, xmm14, xmm15

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
                
        mov     rdi, [rsp + 2304 ]
        SHA2_UPDATE_CV_HELPER rdi, eax, ecx, edx, r8d, r9d, r10d, r11d, esi

        
        mov     rdi, [rsp + 2320 ]
        sub     rdi, SHA2_INPUT_BLOCK_BYTES
        add     QWORD ptr [rsp + 2312 ], SHA2_INPUT_BLOCK_BYTES
        mov     QWORD ptr [rsp + 2320 ], rdi
        cmp     rdi, SHA2_INPUT_BLOCK_BYTES
        jae     single_block_start

done:

        
        mov     rbp, [rsp + 2328 ]
        mov     QWORD ptr [rbp], rdi

        vzeroupper

        
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



movdqa xmm6, xmmword ptr [rsp + 2064]
movdqa xmm7, xmmword ptr [rsp + 2080]
movdqa xmm8, xmmword ptr [rsp + 2096]
movdqa xmm9, xmmword ptr [rsp + 2112]
movdqa xmm10, xmmword ptr [rsp + 2128]
movdqa xmm11, xmmword ptr [rsp + 2144]
movdqa xmm12, xmmword ptr [rsp + 2160]
movdqa xmm13, xmmword ptr [rsp + 2176]
movdqa xmm14, xmmword ptr [rsp + 2192]
movdqa xmm15, xmmword ptr [rsp + 2208]
add rsp, 2232
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
NESTED_END SymCryptSha256AppendBlocks_ymm_avx2_asm, _TEXT





























































END
