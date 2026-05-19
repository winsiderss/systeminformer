











































































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














ROUND_AVX512 MACRO  a, b, c, d, e, f, g, h, xt1, xt2, xt3, xt4, rnd, Wk, scale



                                                            
                                                                vprorq      xt4, e, 14
    vmovq       xt2, QWORD ptr [Wk + rnd * scale]
                                                                vprorq      xt1, e, 18
                                                                vprorq      xt3, e, 41
                                                                vpternlogq  xt3, xt4, xt1, 96h
                            vmovdqa     xt1, e
                            vpternlogq  xt1, f, g, 0cah
                                                                                vmovdqa     xt4, a
    vpaddq      h, h, xt2
    vpaddq      h, h, xt1
                                                                                vpternlogq  xt4, b, c, 0e8h
    vpaddq      h, h, xt3
    vpaddq      d, d, h
                                                vprorq      xt2, a, 28
    vpaddq      h, h, xt4
                                                vprorq      xt1, a, 34
                                                vprorq      xt3, a, 39
                                                vpternlogq  xt1, xt3, xt2, 96h
    vpaddq      h, h, xt1

ENDM

















SHA512_MSG_EXPAND_4BLOCKS MACRO  y0, y1, y9, y14, rnd, t1, t2, t3, Wx, k512

        vpbroadcastq t1, QWORD ptr [k512 + 8 * (rnd - 16)]      
        vpaddq      t1, t1, y0                                  
        vmovdqu     YMMWORD ptr [Wx + (rnd - 16) * 32], t1      
        
        vprorq      t1, y14, 19
        vprorq      t2, y14, 61
        vpsrlq      t3, y14, 6
        vpternlogq  t1, t2, t3, 96h                         

        vpaddq      y0, y0, y9                                  
        vpaddq      y0, y0, t1                                  

        vprorq      t2, y1, 1
        vprorq      t3, y1, 8
        vpsrlq      t1, y1, 7
        vpternlogq  t2, t3, t1, 96h                         
        
        vpaddq      t1, t2, y0                                  

        vmovdqu     y0, YMMWORD ptr [Wx + (rnd - 14) * 32]      
        vmovdqu     YMMWORD ptr [Wx + rnd * 32], t1             

ENDM











SHA512_MSG_EXPAND_1BLOCK MACRO  y0, y1, y2, y3, t1, t2, t3, t4, karr, ind

        
        
        
        
        

        
        
        
        
        

        valignq     t1, y1, y0, 1                       
        vprorq      t2, t1, 1
        vprorq      t3, t1, 8
        vpsrlq      t1, t1, 7
        vpternlogq  t1, t2, t3, 96h                 
    
        valignq     t4, y3, y2, 1                       
        vpaddq      y0, y0, t1                          
        vpaddq      y0, y0, t4                          
    
        vprorq      t2, y3, 19
        vprorq      t3, y3, 61
        vpsrlq      t1, y3, 6
        vpternlogq  t1, t2, t3, 96h                 
        vperm2i128  t1, t1, t1, 81h                 

        vpaddq      t1, y0, t1                          
                                                        
        vprorq      t2, t1, 19
        vprorq      t3, t1, 61
        vpsrlq      t4, t1, 6
        vpternlogq  t2, t3, t4, 96h                 
        vperm2i128  t3, t2, t2, 28h                 

        vmovdqa     t4, YMMWORD ptr [karr + ind * 32]   
        vpaddq      y0, t1, t3                          
        vpaddq      t4, t4, y0                          
        vmovdqu     YMMWORD ptr [rsp + ind * 32], t4

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














SHA512_COPY_STATE_R64_TO_XMM MACRO  a, b, c, d, e, f, g, h

        vmovq   xmm0, a
        vmovq   xmm1, b
        vmovq   xmm2, c
        vmovq   xmm3, d
        vmovq   xmm4, e
        vmovq   xmm5, f
        vmovq   xmm6, g
        vmovq   xmm7, h

ENDM









SHA512_COPY_STATE_XMM_TO_R64 MACRO  a, b, c, d, e, f, g, h

        vmovq   a, xmm0
        vmovq   b, xmm1
        vmovq   c, xmm2
        vmovq   d, xmm3
        vmovq   e, xmm4
        vmovq   f, xmm5
        vmovq   g, xmm6
        vmovq   h, xmm7

ENDM











SHA512_UPDATE_CV_XMM MACRO  Xba, Xdc, Xfe, Xhg, xt

        
        
        
        vpshufd     xt, Xba, 14         
        vpaddq      xmm0, xmm0, Xba     
        vpaddq      xmm1, xmm1, xt      
        vpshufd     xt, Xdc, 14         
        vpaddq      xmm2, xmm2, Xdc     
        vpaddq      xmm3, xmm3, xt      
        vpshufd     xt, Xfe, 14         
        vpaddq      xmm4, xmm4, Xfe     
        vpaddq      xmm5, xmm5, xt      
        vpshufd     xt, Xhg, 14         
        vpaddq      xmm6, xmm6, Xhg     
        vpaddq      xmm7, xmm7, xt      
        vpunpcklqdq Xba, xmm0, xmm1     
        vpunpcklqdq Xdc, xmm2, xmm3     
        vpunpcklqdq Xfe, xmm4, xmm5     
        vpunpcklqdq Xhg, xmm6, xmm7     

ENDM






































































NESTED_ENTRY SymCryptSha512AppendBlocks_ymm_avx512vl_asm, _TEXT

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

        
        
        vmovdqu     ymm14, YMMWORD ptr [rcx + 0 * 32]        
        vmovdqu     ymm15, YMMWORD ptr [rcx + 1 * 32]        

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
        vmovdqa ymm13, YMMWORD ptr [BYTE_REVERSE_64X2]
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 1, ymm13,  ymm2,  ymm3, ymm4, ymm5,  ymm9, ymm10, ymm11, ymm12
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 2, ymm13,  ymm5,  ymm2, ymm3, ymm4,  ymm9, ymm10, ymm11, ymm12 
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 0, ymm13,  ymm0,  ymm1, ymm5, ymm6,  ymm9, ymm10, ymm11, ymm12 
        SHA512_MSG_LOAD_TRANSPOSE_YMM r13, rdi, rbp, rbx, 3, ymm13,  ymm5,  ymm6, ymm7, ymm8,  ymm9, ymm10, ymm11, ymm12 

        lea     r14, [rsp]
        lea     r15, [SymCryptSha512K]

        
        

expand_process_first_block:

        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm2, ymm7, (16 + 0), ymm9, ymm10, ymm11, r14, r15
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm3, ymm8, (16 + 1), ymm2, ymm10, ymm11, r14, r15
        ROUND_512    rax, rcx, rdx, r8, r9, r10, r11, rsi, 0, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rsi, rax, rcx, rdx, r8, r9, r10, r11, 1, rdi, rbp, rbx, r12, r13, r14, 32  
        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm4, ymm9, (16 + 2), ymm3, ymm10, ymm11, r14, r15
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm5, ymm2, (16 + 3), ymm4, ymm10, ymm11, r14, r15
        ROUND_512    r11, rsi, rax, rcx, rdx, r8, r9, r10, 2, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r10, r11, rsi, rax, rcx, rdx, r8, r9, 3, rdi, rbp, rbx, r12, r13, r14, 32      
        
        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm6, ymm3, (16 + 4), ymm5, ymm10, ymm11, r14, r15
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm7, ymm4, (16 + 5), ymm6, ymm10, ymm11, r14, r15
        ROUND_512    r9, r10, r11, rsi, rax, rcx, rdx, r8, 4, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    r8, r9, r10, r11, rsi, rax, rcx, rdx, 5, rdi, rbp, rbx, r12, r13, r14, 32
        SHA512_MSG_EXPAND_4BLOCKS   ymm0, ymm1, ymm8, ymm5, (16 + 6), ymm7, ymm10, ymm11, r14, r15
        SHA512_MSG_EXPAND_4BLOCKS   ymm1, ymm0, ymm9, ymm6, (16 + 7), ymm8, ymm10, ymm11, r14, r15
        ROUND_512    rdx, r8, r9, r10, r11, rsi, rax, rcx, 6, rdi, rbp, rbx, r12, r13, r14, 32
        ROUND_512    rcx, rdx, r8, r9, r10, r11, rsi, rax, 7, rdi, rbp, rbx, r12, r13, r14, 32

        lea     rdi, [SymCryptSha512K + 64 * 8]
        add     r14, 8 * 32 
        add     r15, 8 * 8  
        cmp     r15, rdi
        jb      expand_process_first_block

        
        
        
        
        
        
        
        
        
        
        vpermq      ymm12, ymm14, 0eh       
        vpermq      ymm13, ymm15, 0eh       

        
        
        SHA512_COPY_STATE_R64_TO_XMM rax, rcx, rdx, r8, r9, r10, r11, rsi

        
final_rounds:
        SHA512_MSG_ADD_CONST_8X 0, ymm8, ymm9, r14, r15
        ROUND_AVX512    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,  xmm8, xmm9, xmm10, xmm11, 0, r14, 32       
        ROUND_AVX512    xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6,  xmm8, xmm9, xmm10, xmm11, 1, r14, 32
        ROUND_AVX512    xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5,  xmm8, xmm9, xmm10, xmm11, 2, r14, 32
        ROUND_AVX512    xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4,  xmm8, xmm9, xmm10, xmm11, 3, r14, 32
        ROUND_AVX512    xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,  xmm8, xmm9, xmm10, xmm11, 4, r14, 32
        ROUND_AVX512    xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,  xmm8, xmm9, xmm10, xmm11, 5, r14, 32
        ROUND_AVX512    xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1,  xmm8, xmm9, xmm10, xmm11, 6, r14, 32
        ROUND_AVX512    xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,  xmm8, xmm9, xmm10, xmm11, 7, r14, 32   
        
        lea     rdi, [SymCryptSha512K + 80 * 8]  
        add     r14, 8 * 32 
        add     r15, 8 * 8
        cmp     r15, rdi
        jb      final_rounds

        
        
        SHA512_UPDATE_CV_XMM xmm14, xmm12, xmm15, xmm13,  xmm9

        
        
        dec qword ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]

        lea     r14, [rsp + 8]    
        
block_begin:

        mov     r15d, 80 / 8

        align 16
inner_loop:
        
        ROUND_AVX512    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,  xmm8, xmm9, xmm10, xmm11, 0, r14, 32       
        ROUND_AVX512    xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6,  xmm8, xmm9, xmm10, xmm11, 1, r14, 32
        ROUND_AVX512    xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5,  xmm8, xmm9, xmm10, xmm11, 2, r14, 32
        ROUND_AVX512    xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4,  xmm8, xmm9, xmm10, xmm11, 3, r14, 32
        ROUND_AVX512    xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,  xmm8, xmm9, xmm10, xmm11, 4, r14, 32
        ROUND_AVX512    xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,  xmm8, xmm9, xmm10, xmm11, 5, r14, 32
        ROUND_AVX512    xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1,  xmm8, xmm9, xmm10, xmm11, 6, r14, 32
        ROUND_AVX512    xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,  xmm8, xmm9, xmm10, xmm11, 7, r14, 32   

        add     r14, 8 * 32         
        sub     r15d, 1
        jnz     inner_loop

        add     r14, (8 - 80 * 32)  
                
        
        
        SHA512_UPDATE_CV_XMM xmm14, xmm12, xmm15, xmm13,  xmm9

        dec     QWORD ptr [((rsp + SHA2_EXPANDED_MESSAGE_SIZE) + 0 * 8)]
        jnz     block_begin

        
        
        SHA512_COPY_STATE_XMM_TO_R64 rax, rcx, rdx, r8, r9, r10, r11, rsi

        vperm2i128  ymm14, ymm14, ymm12, 20h
        vperm2i128  ymm15, ymm15, ymm13, 20h

        
        mov     rdi, [rsp + 2832 ]
        GET_PROCESSED_BYTES rdi, rbp, rbx     
        sub     rdi, rbp
        add     QWORD ptr [rsp + 2824 ], rbp
        mov     QWORD ptr [rsp + 2832 ], rdi
        cmp     rdi, SHA2_SINGLE_BLOCK_THRESHOLD
        jae     process_blocks

        
        mov         rbp, [rsp + 2816 ]
        vmovdqu     YMMWORD ptr [rbp + 0 * 32], ymm14
        vmovdqu     YMMWORD ptr [rbp + 1 * 32], ymm15


        align 16
single_block_entry:

        cmp     rdi, SHA2_INPUT_BLOCK_BYTES      
        jb      done

single_block_start:

        mov r14, [rsp + 2824 ]
        lea r15, [SymCryptSha512K]

        SHA512_COPY_STATE_R64_TO_XMM rax, rcx, rdx, r8, r9, r10, r11, rsi

        
        
        
        
        vmovdqa     ymm8, YMMWORD ptr [BYTE_REVERSE_64X2]
        vmovdqu     ymm12, YMMWORD ptr [r14 + 0 * 32]
        vmovdqu     ymm13, YMMWORD ptr [r14 + 1 * 32]
        vmovdqu     ymm14, YMMWORD ptr [r14 + 2 * 32]
        vmovdqu     ymm15, YMMWORD ptr [r14 + 3 * 32]
        vpshufb     ymm12, ymm12, ymm8
        vpshufb     ymm13, ymm13, ymm8
        vpshufb     ymm14, ymm14, ymm8
        vpshufb     ymm15, ymm15, ymm8
        vmovdqa     ymm8,  YMMWORD ptr [r15 + 0 * 32]
        vmovdqa     ymm9,  YMMWORD ptr [r15 + 1 * 32]
        vmovdqa     ymm10, YMMWORD ptr [r15 + 2 * 32]
        vmovdqa     ymm11, YMMWORD ptr [r15 + 3 * 32]
        vpaddq      ymm8, ymm12, ymm8
        vpaddq      ymm9, ymm13, ymm9
        vpaddq      ymm10, ymm14, ymm10
        vpaddq      ymm11, ymm15, ymm11
        vmovdqu     YMMWORD ptr [rsp + 0 * 32], ymm8
        vmovdqu     YMMWORD ptr [rsp + 1 * 32], ymm9
        vmovdqu     YMMWORD ptr [rsp + 2 * 32], ymm10
        vmovdqu     YMMWORD ptr [rsp + 3 * 32], ymm11
        
inner_loop_single:

        add     r15, 16 * 8

        ROUND_AVX512    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,  xmm8, xmm9, xmm10, xmm11,  0, rsp, 8     
        ROUND_AVX512    xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6,  xmm8, xmm9, xmm10, xmm11,  1, rsp, 8
        ROUND_AVX512    xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5,  xmm8, xmm9, xmm10, xmm11,  2, rsp, 8
        ROUND_AVX512    xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4,  xmm8, xmm9, xmm10, xmm11,  3, rsp, 8                                                                                                
        SHA512_MSG_EXPAND_1BLOCK ymm12, ymm13, ymm14, ymm15,  ymm8, ymm9, ymm10, ymm11, r15, 0

        ROUND_AVX512    xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,  xmm8, xmm9, xmm10, xmm11,  4, rsp, 8
        ROUND_AVX512    xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,  xmm8, xmm9, xmm10, xmm11,  5, rsp, 8
        ROUND_AVX512    xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1,  xmm8, xmm9, xmm10, xmm11,  6, rsp, 8
        ROUND_AVX512    xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,  xmm8, xmm9, xmm10, xmm11,  7, rsp, 8     
        SHA512_MSG_EXPAND_1BLOCK ymm13, ymm14, ymm15, ymm12,  ymm8, ymm9, ymm10, ymm11, r15, 1

        ROUND_AVX512    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,  xmm8, xmm9, xmm10, xmm11,  8, rsp, 8     
        ROUND_AVX512    xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6,  xmm8, xmm9, xmm10, xmm11,  9, rsp, 8
        ROUND_AVX512    xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5,  xmm8, xmm9, xmm10, xmm11, 10, rsp, 8
        ROUND_AVX512    xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4,  xmm8, xmm9, xmm10, xmm11, 11, rsp, 8     
        SHA512_MSG_EXPAND_1BLOCK ymm14, ymm15, ymm12, ymm13,  ymm8, ymm9, ymm10, ymm11, r15, 2

        ROUND_AVX512    xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,  xmm8, xmm9, xmm10, xmm11, 12, rsp, 8
        ROUND_AVX512    xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,  xmm8, xmm9, xmm10, xmm11, 13, rsp, 8
        ROUND_AVX512    xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1,  xmm8, xmm9, xmm10, xmm11, 14, rsp, 8     
        ROUND_AVX512    xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,  xmm8, xmm9, xmm10, xmm11, 15, rsp, 8 
        SHA512_MSG_EXPAND_1BLOCK ymm15, ymm12, ymm13, ymm14,  ymm8, ymm9, ymm10, ymm11, r15, 3
        
        lea     rdi, [SymCryptSha512K + 64 * 8]
        cmp     r15, rdi
        jb      inner_loop_single


        lea r14, [rsp]
        lea r15, [rsp + 16 * 8]

single_block_final_rounds:

        ROUND_AVX512    xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,  xmm8, xmm9, xmm10, xmm11,  0, r14, 8       
        ROUND_AVX512    xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6,  xmm8, xmm9, xmm10, xmm11,  1, r14, 8
        ROUND_AVX512    xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5,  xmm8, xmm9, xmm10, xmm11,  2, r14, 8
        ROUND_AVX512    xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4,  xmm8, xmm9, xmm10, xmm11,  3, r14, 8                                                                                                  
        ROUND_AVX512    xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,  xmm8, xmm9, xmm10, xmm11,  4, r14, 8
        ROUND_AVX512    xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,  xmm8, xmm9, xmm10, xmm11,  5, r14, 8
        ROUND_AVX512    xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1,  xmm8, xmm9, xmm10, xmm11,  6, r14, 8
        ROUND_AVX512    xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,  xmm8, xmm9, xmm10, xmm11,  7, r14, 8       
        
        add r14, 8 * 8
        cmp r14, r15
        jb single_block_final_rounds
                
        SHA512_COPY_STATE_XMM_TO_R64 rax, rcx, rdx, r8, r9, r10, r11, rsi

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
NESTED_END SymCryptSha512AppendBlocks_ymm_avx512vl_asm, _TEXT





























































END
