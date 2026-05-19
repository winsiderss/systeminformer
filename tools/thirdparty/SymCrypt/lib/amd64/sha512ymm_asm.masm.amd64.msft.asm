












































































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
    



    
    
    
    
    
    
    



























EXTERN SymCryptSha512K:QWORD
EXTERN BYTE_REVERSE_64X2:QWORD
EXTERN BYTE_ROTATE_64:QWORD


SHA2_INPUT_BLOCK_BYTES_LOG2 EQU 7
SHA2_INPUT_BLOCK_BYTES EQU 128
SHA2_ROUNDS EQU 80
SHA2_BYTES_PER_WORD EQU 8
SHA2_SIMD_REG_SIZE EQU 32
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












ROR64_YMM MACRO  x, c, res, t1

    vpsrlq  res, x, c
    vpsllq  t1, x, 64 - c
    vpxor   res, res, t1        
    
ENDM










LSIGMA_YMM MACRO  x, c1, c2, c3, res, t1, t2

        ROR64_YMM   x, c1, res, t1
        ROR64_YMM   x, c2, t2, t1
        vpsrlq      t1, x, c3
        vpxor       res, res, t2
        vpxor       res, res, t1

ENDM













LSIGMA0_YMM MACRO  x, t1, t2, t3, krot8

        ROR64_YMM   x, 1, t1, t2
        vpsrlq      t3, x, 7
        vpshufb     t2, x, krot8
        vpxor       t1, t1, t2
        vpxor       t1, t1, t3

ENDM

















SHA512_MSG_EXPAND_4BLOCKS MACRO  y0, y1, y9, y14, rnd, t1, t2, t3, t4, t5, t6, krot8, Wx, k512

        vpbroadcastq t6, QWORD ptr [k512 + 8 * (rnd - 16)]      
        vpaddq      t6, t6, y0                                  
        vmovdqu     YMMWORD ptr [Wx + (rnd - 16) * 32], t6      
        
        LSIGMA_YMM  y14, 19, 61, 6, t4, t5, t3                  
        LSIGMA0_YMM y1, t2, t1, t6, krot8                       
        vpaddq      t5, y9, y0                                  
        vpaddq      t3, t2, t4                                  
        vpaddq      t1, t3, t5                                  
        vmovdqu     y0, YMMWORD ptr [Wx + (rnd - 14) * 32]      
        vmovdqu     YMMWORD ptr [Wx + rnd * 32], t1             

ENDM










SHA512_MSG_ADD_CONST MACRO  rnd, t1, t2, Wx, k512

        vpbroadcastq t2, QWORD ptr [k512 + 8 * (rnd)]
        vmovdqu t1, YMMWORD ptr [Wx + 32 * (rnd)]
        vpaddq  t1, t1, t2
        vmovdqu YMMWORD ptr [Wx + 32 * (rnd)], t1

ENDM








SHA512_MSG_ADD_CONST_8X MACRO  rnd, t1, t2, Wx, k512

        SHA512_MSG_ADD_CONST (rnd + 0), t1, t2, Wx, k512
        SHA512_MSG_ADD_CONST (rnd + 1), t1, t2, Wx, k512
        SHA512_MSG_ADD_CONST (rnd + 2), t1, t2, Wx, k512
        SHA512_MSG_ADD_CONST (rnd + 3), t1, t2, Wx, k512
        SHA512_MSG_ADD_CONST (rnd + 4), t1, t2, Wx, k512
        SHA512_MSG_ADD_CONST (rnd + 5), t1, t2, Wx, k512
        SHA512_MSG_ADD_CONST (rnd + 6), t1, t2, Wx, k512
        SHA512_MSG_ADD_CONST (rnd + 7), t1, t2, Wx, k512

ENDM

























SHA512_MSG_EXPAND_1BLOCK_0 MACRO  y0, y1, y2, y3, t1, t2, t3, t4, t5, t6, krot8, karr, ind
        
        vpblendd    t1, y1, y0, 0fch                
        vpblendd    t5, y3, y2, 0fch                
        LSIGMA0_YMM t1, t2, t3, t6, krot8               

ENDM
SHA512_MSG_EXPAND_1BLOCK_1 MACRO  y0, y1, y2, y3, t1, t2, t3, t4, t5, t6, krot8, karr, ind

        vpaddq      t2, t2, t5                          
        LSIGMA_YMM  y3, 19, 61, 6, t4, t1, t3           
        vpermq      t2, t2, 39h                     

ENDM
SHA512_MSG_EXPAND_1BLOCK_2 MACRO  y0, y1, y2, y3, t1, t2, t3, t4, t5, t6, krot8, karr, ind

        vperm2i128  t3, t4, t4, 81h                 
        vpaddq      t2, y0, t2                          
        vpaddq      t2, t2, t3                          
                                                        
        LSIGMA_YMM  t2, 19, 61, 6, t4, t5, t3           

ENDM
SHA512_MSG_EXPAND_1BLOCK_3 MACRO  y0, y1, y2, y3, t1, t2, t3, t4, t5, t6, krot8, karr, ind
        
        vperm2i128  t4, t4, t4, 08h                 
        vmovdqa     t6, YMMWORD ptr [karr + 32 * ind]   
        vpaddq      y0, t2, t4                          
        vpaddq      t6, t6, y0                          
        vmovdqu     YMMWORD ptr [rsp + 32 * ind], t6

ENDM








































































NESTED_ENTRY SymCryptSha512AppendBlocks_ymm_avx2_asm, _TEXT

rex_push_reg rsi
push_reg rdi
push_reg rbp
push_reg rbx
push_reg r12
push_reg r13
push_reg r14
push_reg r15
alloc_stack 2744
save_xmm128 xmm6, 2576
save_xmm128 xmm7, 2592
save_xmm128 xmm8, 2608
save_xmm128 xmm9, 2624
save_xmm128 xmm10, 2640
save_xmm128 xmm11, 2656
save_xmm128 xmm12, 2672
save_xmm128 xmm13, 2688
save_xmm128 xmm14, 2704
save_xmm128 xmm15, 2720

END_PROLOGUE


        
        
        
        

        vzeroupper

        mov     [rsp + 2816 ], rcx
        mov     [rsp + 2824 ], rdx
        mov     [rsp + 2832 ], r8
        mov     [rsp + 2840 ], r9


        
        
        
        
        
        
        
        
        
        
        
        
        mov     edi, 16 * SHA2_BYTES_PER_WORD
        mov     ebp, SHA2_EXPANDED_MESSAGE_SIZE
        cmp     r8, SHA2_SINGLE_BLOCK_THRESHOLD 
        cmovae  edi, ebp      
        mov     [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 1 * 8)], edi

        mov     rbx, rcx
        mov     rax, [rbx +  0]
        mov     rcx, [rbx +  8]
        mov     rdx, [rbx + 16]
        mov     r8, [rbx + 24]
        mov     r9, [rbx + 32]
        mov     r10, [rbx + 40]
        mov     r11, [rbx + 48]
        mov     rsi, [rbx + 56]

        
        
        mov     rdi, [rsp + 2832 ]
        cmp     rdi, SHA2_SINGLE_BLOCK_THRESHOLD
        jb      single_block_entry

        align 16
process_blocks:
        
        GET_SIMD_BLOCK_COUNT rdi, rbp     
        mov     [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)], rdi

        
        
        
        
        
        
        
        
        
        mov r13, [rsp + 2824 ]
        vmovdqa ymm15, YMMWORD ptr [BYTE_REVERSE_64X2]
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 0, ymm15,  ymm0,  ymm1, ymm2, ymm3,  ymm9, ymm10, ymm11, ymm12 
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 1, ymm15,  ymm2,  ymm3, ymm4, ymm5,  ymm9, ymm10, ymm11, ymm12
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 2, ymm15,  ymm13, ymm2, ymm3, ymm4,  ymm9, ymm10, ymm11, ymm12 
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 3, ymm15,  ymm5,  ymm6, ymm7, ymm8,  ymm9, ymm10, ymm11, ymm12 

        lea     r14, [rsp]
        lea     r15, [SymCryptSha512K]
        vmovdqa ymm15, YMMWORD ptr [BYTE_ROTATE_64]

expand_process_first_block:

        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm2, ymm7, (16 + 0), ymm9, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    rax, rcx, rdx, r8, r9, r10, r11, rsi, 0, rdi, rbp, rbx, r12, r13, r14, 32
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm3, ymm8, (16 + 1), ymm2, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    rsi, rax, rcx, rdx, r8, r9, r10, r11, 1, rdi, rbp, rbx, r12, r13, r14, 32  
        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm4, ymm9, (16 + 2), ymm3, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    r11, rsi, rax, rcx, rdx, r8, r9, r10, 2, rdi, rbp, rbx, r12, r13, r14, 32
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm5, ymm2, (16 + 3), ymm4, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    r10, r11, rsi, rax, rcx, rdx, r8, r9, 3, rdi, rbp, rbx, r12, r13, r14, 32      
        
        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm6, ymm3, (16 + 4), ymm5, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    r9, r10, r11, rsi, rax, rcx, rdx, r8, 4, rdi, rbp, rbx, r12, r13, r14, 32
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm7, ymm4, (16 + 5), ymm6, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    r8, r9, r10, r11, rsi, rax, rcx, rdx, 5, rdi, rbp, rbx, r12, r13, r14, 32
        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm8, ymm5, (16 + 6), ymm7, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    rdx, r8, r9, r10, r11, rsi, rax, rcx, 6, rdi, rbp, rbx, r12, r13, r14, 32
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm9, ymm6, (16 + 7), ymm8, ymm10, ymm11, ymm12, ymm13, ymm14, ymm15, r14, r15
        ROUND_512    rcx, rdx, r8, r9, r10, r11, rsi, rax, 7, rdi, rbp, rbx, r12, r13, r14, 32

        lea     rdi, [SymCryptSha512K + 64 * 8]
        add     r14, 8 * 32 
        add     r15, 8 * 8  
        cmp     r15, rdi
        jb      expand_process_first_block

        
final_rounds:
        SHA512_MSG_ADD_CONST_8X 0, ymm0, ymm1, r14, r15
        ROUND_512    rax, rcx, rdx, r8, r9, r10, r11, rsi,  0, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rsi, rax, rcx, rdx, r8, r9, r10, r11,  1, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r11, rsi, rax, rcx, rdx, r8, r9, r10,  2, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r10, r11, rsi, rax, rcx, rdx, r8, r9,  3, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r9, r10, r11, rsi, rax, rcx, rdx, r8,  4, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r8, r9, r10, r11, rsi, rax, rcx, rdx,  5, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rdx, r8, r9, r10, r11, rsi, rax, rcx,  6, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rcx, rdx, r8, r9, r10, r11, rsi, rax,  7, rdi, rbp, rbx, r12, r13, r14, 32
            
        lea     rdi, [SymCryptSha512K + 80 * 8]
        add     r14, 8 * 32 
        add     r15, 8 * 8  
        cmp     r15, rdi
        jb      final_rounds
            
        mov rdi, [rsp + 2816 ]
        SHA2_UPDATE_CV_HELPER rdi, rax, rcx, rdx, r8, r9, r10, r11, rsi

        
        
        dec qword ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]

        lea     r14, [rsp + 8]    
        
block_begin:

        mov     r15d, 80 / 8

        align 16
inner_loop:
        
        ROUND_512    rax, rcx, rdx, r8, r9, r10, r11, rsi,  0, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rsi, rax, rcx, rdx, r8, r9, r10, r11,  1, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r11, rsi, rax, rcx, rdx, r8, r9, r10,  2, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r10, r11, rsi, rax, rcx, rdx, r8, r9,  3, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r9, r10, r11, rsi, rax, rcx, rdx, r8,  4, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r8, r9, r10, r11, rsi, rax, rcx, rdx,  5, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rdx, r8, r9, r10, r11, rsi, rax, rcx,  6, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rcx, rdx, r8, r9, r10, r11, rsi, rax,  7, rdi, rbp, rbx, r12, r13, r14, 32

        add     r14, 8 * 32         
        sub     r15d, 1
        jnz     inner_loop

        add     r14, (8 - 80 * 32)  
        
        mov rdi, [rsp + 2816 ]
        SHA2_UPDATE_CV_HELPER rdi, rax, rcx, rdx, r8, r9, r10, r11, rsi
        
        dec     QWORD ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]
        jnz     block_begin

        
        mov     rdi, [rsp + 2832 ]
        GET_PROCESSED_BYTES rdi, rbp, rbx     
        sub     rdi, rbp
        add     QWORD ptr [rsp + 2824 ], rbp
        mov     QWORD ptr [rsp + 2832 ], rdi
        cmp     rdi, SHA2_SINGLE_BLOCK_THRESHOLD
        jae     process_blocks

        align 16
single_block_entry:

        cmp     rdi, SHA2_INPUT_BLOCK_BYTES      
        jb      done

        
        
        vmovdqa ymm14, YMMWORD ptr [BYTE_REVERSE_64X2]
        vmovdqa ymm15, YMMWORD ptr [BYTE_ROTATE_64]

single_block_start:

        mov r14, [rsp + 2824 ]
        lea r15, [SymCryptSha512K]

        
        
        
        
        vmovdqu ymm0, YMMWORD ptr [r14 + 0 * 32]
        vmovdqu ymm1, YMMWORD ptr [r14 + 1 * 32]
        vmovdqu ymm2, YMMWORD ptr [r14 + 2 * 32]
        vmovdqu ymm3, YMMWORD ptr [r14 + 3 * 32]
        vpshufb ymm0, ymm0, ymm14
        vpshufb ymm1, ymm1, ymm14
        vpshufb ymm2, ymm2, ymm14
        vpshufb ymm3, ymm3, ymm14
        vmovdqu ymm4, YMMWORD ptr [r15 + 0 * 32]
        vmovdqu ymm5, YMMWORD ptr [r15 + 1 * 32]
        vmovdqu ymm6, YMMWORD ptr [r15 + 2 * 32]
        vmovdqu ymm7, YMMWORD ptr [r15 + 3 * 32]
        vpaddq  ymm4, ymm4, ymm0
        vpaddq  ymm5, ymm5, ymm1
        vpaddq  ymm6, ymm6, ymm2
        vpaddq  ymm7, ymm7, ymm3
        vmovdqu YMMWORD ptr [rsp + 0 * 32], ymm4
        vmovdqu YMMWORD ptr [rsp + 1 * 32], ymm5
        vmovdqu YMMWORD ptr [rsp + 2 * 32], ymm6
        vmovdqu YMMWORD ptr [rsp + 3 * 32], ymm7

inner_loop_single:

        add     r15, 16 * 8
        
        ROUND_512 rax, rcx, rdx, r8, r9, r10, r11, rsi,  0, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_0 ymm0, ymm1, ymm2, ymm3,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 0
        ROUND_512 rsi, rax, rcx, rdx, r8, r9, r10, r11,  1, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_1 ymm0, ymm1, ymm2, ymm3,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 0
        ROUND_512 r11, rsi, rax, rcx, rdx, r8, r9, r10,  2, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_2 ymm0, ymm1, ymm2, ymm3,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 0
        ROUND_512 r10, r11, rsi, rax, rcx, rdx, r8, r9,  3, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_3 ymm0, ymm1, ymm2, ymm3,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 0

        ROUND_512 r9, r10, r11, rsi, rax, rcx, rdx, r8,  4, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_0 ymm1, ymm2, ymm3, ymm0,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 1
        ROUND_512 r8, r9, r10, r11, rsi, rax, rcx, rdx,  5, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_1 ymm1, ymm2, ymm3, ymm0,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 1
        ROUND_512 rdx, r8, r9, r10, r11, rsi, rax, rcx,  6, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_2 ymm1, ymm2, ymm3, ymm0,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 1
        ROUND_512 rcx, rdx, r8, r9, r10, r11, rsi, rax,  7, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_3 ymm1, ymm2, ymm3, ymm0,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 1
        
        ROUND_512 rax, rcx, rdx, r8, r9, r10, r11, rsi,  8, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_0 ymm2, ymm3, ymm0, ymm1,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 2
        ROUND_512 rsi, rax, rcx, rdx, r8, r9, r10, r11,  9, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_1 ymm2, ymm3, ymm0, ymm1,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 2
        ROUND_512 r11, rsi, rax, rcx, rdx, r8, r9, r10, 10, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_2 ymm2, ymm3, ymm0, ymm1,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 2
        ROUND_512 r10, r11, rsi, rax, rcx, rdx, r8, r9, 11, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_3 ymm2, ymm3, ymm0, ymm1,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 2

        ROUND_512 r9, r10, r11, rsi, rax, rcx, rdx, r8, 12, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_0 ymm3, ymm0, ymm1, ymm2,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 3
        ROUND_512 r8, r9, r10, r11, rsi, rax, rcx, rdx, 13, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_1 ymm3, ymm0, ymm1, ymm2,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 3
        ROUND_512 rdx, r8, r9, r10, r11, rsi, rax, rcx, 14, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_2 ymm3, ymm0, ymm1, ymm2,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 3
        ROUND_512 rcx, rdx, r8, r9, r10, r11, rsi, rax, 15, rdi, rbp, rbx, r12, r13, rsp, 8
        SHA512_MSG_EXPAND_1BLOCK_3 ymm3, ymm0, ymm1, ymm2,  ymm4, ymm5, ymm6, ymm7, ymm8, ymm9, ymm15, r15, 3
        
        lea     rdi, [SymCryptSha512K + 64 * 8]
        cmp     r15, rdi
        jb      inner_loop_single

        lea r14, [rsp]
        lea r15, [rsp + 16 * 8]

single_block_final_rounds:

        ROUND_512    rax, rcx, rdx, r8, r9, r10, r11, rsi,  0, rdi, rbp, rbx, r12, r13, r14, 8
        ROUND_512    rsi, rax, rcx, rdx, r8, r9, r10, r11,  1, rdi, rbp, rbx, r12, r13, r14, 8
        ROUND_512    r11, rsi, rax, rcx, rdx, r8, r9, r10,  2, rdi, rbp, rbx, r12, r13, r14, 8
        ROUND_512    r10, r11, rsi, rax, rcx, rdx, r8, r9,  3, rdi, rbp, rbx, r12, r13, r14, 8
        ROUND_512    r9, r10, r11, rsi, rax, rcx, rdx, r8,  4, rdi, rbp, rbx, r12, r13, r14, 8
        ROUND_512    r8, r9, r10, r11, rsi, rax, rcx, rdx,  5, rdi, rbp, rbx, r12, r13, r14, 8
        ROUND_512    rdx, r8, r9, r10, r11, rsi, rax, rcx,  6, rdi, rbp, rbx, r12, r13, r14, 8
        ROUND_512    rcx, rdx, r8, r9, r10, r11, rsi, rax,  7, rdi, rbp, rbx, r12, r13, r14, 8
        
        add r14, 8 * 8
        cmp r14, r15
        jb single_block_final_rounds

        mov rdi, [rsp + 2816 ]
        SHA2_UPDATE_CV_HELPER rdi, rax, rcx, rdx, r8, r9, r10, r11, rsi

        
        mov     rdi, [rsp + 2832 ]
        sub     rdi, SHA2_INPUT_BLOCK_BYTES
        add     QWORD ptr [rsp + 2824 ], SHA2_INPUT_BLOCK_BYTES
        mov     QWORD ptr [rsp + 2832 ], rdi
        cmp     rdi, SHA2_INPUT_BLOCK_BYTES
        jae     single_block_start

done:

        
        mov     rbp, [rsp + 2840 ]
        mov     QWORD ptr [rbp], rdi

        vzeroupper

        
        mov     rdi, rsp
        xor     rax, rax
        mov     ecx, [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 1 * 8)]
        
        
        pxor    xmm0, xmm0
        movaps  [rdi + 0 * 16], xmm0
        movaps  [rdi + 1 * 16], xmm0
        movaps  [rdi + 2 * 16], xmm0
        movaps  [rdi + 3 * 16], xmm0
        movaps  [rdi + 4 * 16], xmm0
        movaps  [rdi + 5 * 16], xmm0
        movaps  [rdi + 6 * 16], xmm0
        movaps  [rdi + 7 * 16], xmm0
        add     rdi, 8 * 16

        
        sub     ecx, 8 * 16 
        jz      nowipe
        rep     stosb

nowipe:



movdqa xmm6, xmmword ptr [rsp + 2576]
movdqa xmm7, xmmword ptr [rsp + 2592]
movdqa xmm8, xmmword ptr [rsp + 2608]
movdqa xmm9, xmmword ptr [rsp + 2624]
movdqa xmm10, xmmword ptr [rsp + 2640]
movdqa xmm11, xmmword ptr [rsp + 2656]
movdqa xmm12, xmmword ptr [rsp + 2672]
movdqa xmm13, xmmword ptr [rsp + 2688]
movdqa xmm14, xmmword ptr [rsp + 2704]
movdqa xmm15, xmmword ptr [rsp + 2720]
add rsp, 2744
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
NESTED_END SymCryptSha512AppendBlocks_ymm_avx2_asm, _TEXT





























































END
