;
; Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
;
; This file is part of System Informer.
;
; Authors:
;
;     jxy-s   2026
;

#include "kxarm64.h"

;
; VOID KphSmc(_Inout_updates_(18) PULONG64 Registers)
; VOID KphHvc(_Inout_updates_(18) PULONG64 Registers)
;
; Loads x0 to x17 from Registers[0..17], issues SMC #0 or HVC #0, and stores
; x0 to x17 back to Registers[0..17].
;

        MACRO
        KPH_LOAD_CALL_REGISTERS
        ldp     x0, x1, [x19]
        ldp     x2, x3, [x19, #0x10]
        ldp     x4, x5, [x19, #0x20]
        ldp     x6, x7, [x19, #0x30]
        ldp     x8, x9, [x19, #0x40]
        ldp     x10, x11, [x19, #0x50]
        ldp     x12, x13, [x19, #0x60]
        ldp     x14, x15, [x19, #0x70]
        ldp     x16, x17, [x19, #0x80]
        MEND

        MACRO
        KPH_STORE_CALL_REGISTERS
        stp     x0, x1, [x19]
        stp     x2, x3, [x19, #0x10]
        stp     x4, x5, [x19, #0x20]
        stp     x6, x7, [x19, #0x30]
        stp     x8, x9, [x19, #0x40]
        stp     x10, x11, [x19, #0x50]
        stp     x12, x13, [x19, #0x60]
        stp     x14, x15, [x19, #0x70]
        stp     x16, x17, [x19, #0x80]
        MEND

        TEXTAREA

        NESTED_ENTRY KphSmc, , , 4

        PROLOG_SAVE_REG_PAIR fp, lr, #-32!
        PROLOG_SAVE_REG x19, #16

        mov     x19, x0
        KPH_LOAD_CALL_REGISTERS
        smc     #0
        KPH_STORE_CALL_REGISTERS

        EPILOG_RESTORE_REG x19, #16
        EPILOG_RESTORE_REG_PAIR fp, lr, #32!
        EPILOG_RETURN

        NESTED_END KphSmc

        NESTED_ENTRY KphHvc, , , 4

        PROLOG_SAVE_REG_PAIR fp, lr, #-32!
        PROLOG_SAVE_REG x19, #16

        mov     x19, x0
        KPH_LOAD_CALL_REGISTERS
        hvc     #0
        KPH_STORE_CALL_REGISTERS

        EPILOG_RESTORE_REG x19, #16
        EPILOG_RESTORE_REG_PAIR fp, lr, #32!
        EPILOG_RETURN

        NESTED_END KphHvc

        END
