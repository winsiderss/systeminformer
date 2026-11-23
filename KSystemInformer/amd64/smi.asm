;
; Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
;
; This file is part of System Informer.
;
; Authors:
;
;     jxy-s   2026
;

;
; VOID KphSmi(_In_ USHORT Port, _Inout_updates_(7) PULONG64 Registers)
;
; Loads rax, rbx, rcx, rdx, rsi and rdi from Registers[0..5], puts Port in the
; low 16 bits of rdx, writes al to the port, and stores the six registers and
; the flags back to Registers[0..6].
;

    .code

    align   16
KphSmi PROC FRAME

    push    rbx
    .pushreg rbx
    push    rsi
    .pushreg rsi
    push    rdi
    .pushreg rdi
    push    r12
    .pushreg r12
    sub     rsp, 8
    .allocstack 8
    .endprolog

    mov     r12, rdx
    mov     [rsp], rdx
    movzx   r9d, cx

    mov     rax, [r12]
    mov     rbx, [r12 + 8]
    mov     rcx, [r12 + 16]
    mov     rdx, [r12 + 24]
    mov     rsi, [r12 + 32]
    mov     rdi, [r12 + 40]
    mov     dx, r9w

    out     dx, al

    mov     r12, [rsp]
    mov     [r12], rax
    mov     [r12 + 8], rbx
    mov     [r12 + 16], rcx
    mov     [r12 + 24], rdx
    mov     [r12 + 32], rsi
    mov     [r12 + 40], rdi
    pushfq
    pop     qword ptr [r12 + 48]

    add     rsp, 8
    pop     r12
    pop     rdi
    pop     rsi
    pop     rbx
    ret

KphSmi ENDP

    end
