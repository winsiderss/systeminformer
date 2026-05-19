















































































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















































    



        
        
    




    


        



































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































































;++
;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Module:
;
;   kxarm64.w
;
; Abstract:
;
;   Contains ARM architecture constants and assembly macros.
;
;--

;
; The ARM assembler uses a baroque syntax that is documented as part
; of the online Windows CE documentation.  The syntax derives from
; ARM's own assembler and was chosen to allow the migration of
; specific assembly code bases, namely ARM's floating point runtime.
; While this compatibility is no longer strictly necessary, the
; syntax lives on....
;
; Highlights:
;      * Assembler is white space sensitive.  Symbols are defined by putting
;        them in the first column
;      * The macro definition mechanism is very primitive
;
; To augment the assembler, assembly files are run through CPP (as they are
; on IA64).  This works well for constants but not structural components due
; to the white space sensitivity.
;
; For now, we use a mix of native assembler and CPP macros.
;








;++
;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Module:
;
;   kxarm64unw.w
;
; Abstract:
;
;   Contains ARM64 unwind code helper macros
;
;   This file is not really useful on its own without the support from
;   kxarm64.h.
;
;--

;
; The following macros are defined here:
;
;   PROLOG_STACK_ALLOC <amount>
;   PROLOG_SAVE_REG
;   PROLOG_SAVE_REG_PAIR
;   PROLOG_SAVE_REG_PAIR_NO_FP
;   PROLOG_NOP <operation>
;   PROLOG_SAVE_NEXT_PAIR <operation>
;   PROLOG_PUSH_TRAP_FRAME
;   PROLOG_PUSH_MACHINE_FRAME
;   PROLOG_PUSH_CONTEXT
;   PROLOG_PUSH_EC_CONTEXT
;   PROLOG_SIGN_RETURN_ADDRESS
;
;   EPILOG_STACK_FREE <amount>
;   EPILOG_RECOVER_SP <offset>
;   EPILOG_RESTORE_REG
;   EPILOG_RESTORE_REG_PAIR
;   EPILOG_NOP <operation>
;   EPILOG_RESTORE_NEXT_PAIR <operation>
;   EPILOG_AUTHENTICATE_RETURN_ADDRESS
;   EPILOG_RETURN
;

        ;
        ; Global variables
        ;

        ; result from __ParseRegisterNumber
        GBLA __ParsedRegNumber

        ; result from __ParseOffset
        GBLA __ParsedOffsetAbs
        GBLA __ParsedOffsetShifted3
        GBLA __ParsedOffsetShifted4
        GBLA __ParsedOffsetPreinc
        GBLS __ParsedOffsetRawString
        GBLS __ParsedOffsetString

        ; results from __ComputeCodes[...]
        GBLS __ComputedCodes
        GBLL __RegPairWasFpLr

        ; global state and accumulators
        GBLS __PrologUnwindString
        GBLS __PrologLastLabel
        GBLA __EpilogUnwindCount
        GBLS __Epilog1UnwindString
        GBLS __Epilog2UnwindString
        GBLS __Epilog3UnwindString
        GBLS __Epilog4UnwindString
        GBLL __EpilogStartNotDefined
        GBLA __RunningIndex
        GBLS __RunningLabel
        GBLS __FuncExceptionHandler


        ;
        ; Helper macro: emit an opcode with a generated label
        ;
        ; Output: Name of label is in $__RunningLabel
        ;

        MACRO
        __EmitRunningLabelAndOpcode $O1,$O2,$O3,$O4,$O5,$O6

__RunningLabel SETS "|Temp.$__RunningIndex|"
__RunningIndex SETA __RunningIndex + 1

        IF "$O6" != ""
$__RunningLabel $O1,$O2,$O3,$O4,$O5,$O6
        ELIF "$O5" != ""
$__RunningLabel $O1,$O2,$O3,$O4,$O5
        ELIF "$O4" != ""
$__RunningLabel $O1,$O2,$O3,$O4
        ELIF "$O3" != ""
$__RunningLabel $O1,$O2,$O3
        ELIF "$O2" != ""
$__RunningLabel $O1,$O2
        ELIF "$O1" != ""
$__RunningLabel $O1
        ELSE
$__RunningLabel
        ENDIF

        MEND


        ;
        ; Helper macro: append unwind codes to the prolog string
        ;
        ; Input is in __ComputedCodes
        ;

        MACRO
        __AppendPrologCodes

__PrologUnwindString SETS "$__ComputedCodes,$__PrologUnwindString"

        MEND


        ;
        ; Helper macro: append unwind codes to the epilog string
        ;
        ; Input is in __ComputedCodes
        ;

        MACRO
        __AppendEpilogCodes

        IF __EpilogUnwindCount == 1
__Epilog1UnwindString SETS "$__Epilog1UnwindString,$__ComputedCodes"
        ELIF __EpilogUnwindCount == 2
__Epilog2UnwindString SETS "$__Epilog2UnwindString,$__ComputedCodes"
        ELIF __EpilogUnwindCount == 3
__Epilog3UnwindString SETS "$__Epilog3UnwindString,$__ComputedCodes"
        ELIF __EpilogUnwindCount == 4
__Epilog4UnwindString SETS "$__Epilog4UnwindString,$__ComputedCodes"
        ENDIF

        MEND


        ;
        ; Helper macro: detect prolog end
        ;

        MACRO
        __DeclarePrologEnd

__PrologLastLabel SETS "$__RunningLabel"

        MEND


        ;
        ; Helper macro: detect epilog start
        ;

        MACRO
        __DeclareEpilogStart

        IF __EpilogStartNotDefined
__EpilogStartNotDefined SETL {false}
__EpilogUnwindCount SETA __EpilogUnwindCount + 1
        IF __EpilogUnwindCount == 1
$__FuncEpilog1StartLabel
        ELIF __EpilogUnwindCount == 2
$__FuncEpilog2StartLabel
        ELIF __EpilogUnwindCount == 3
$__FuncEpilog3StartLabel
        ELIF __EpilogUnwindCount == 4
$__FuncEpilog4StartLabel
        ELSE
        INFO    1, "Too many epilogues!"
        ENDIF
        ENDIF

        MEND


        ;
        ; Helper macro: specify epilog end
        ;

        MACRO
        __DeclareEpilogEnd

__EpilogStartNotDefined SETL {true}

        MEND


        ;
        ; Parse a register number
        ;
        ; Calling macro name is in $Name
        ; Input is in $Reg
        ; Output is placed in __ParsedRegNumber
        ;

        MACRO
        __ParseRegisterNumber $Name, $Reg

        LCLS    RString

RString SETS    "$Reg"

        IF (RString:LEFT:1 == "q") || (RString:LEFT:1 == "v")

RString  SETS    RString:RIGHT:(:LEN:RString - 1)
__ParsedRegNumber SETA  64 + $RString

        ELIF RString:LEFT:1 == "d"

RString  SETS    RString:RIGHT:(:LEN:RString - 1)
__ParsedRegNumber SETA  32 + $RString

        ELSE

__ParsedRegNumber SETA  :RCONST:$RString

        ENDIF

        MEND


        ;
        ; Parse a stack offset
        ;
        ; Calling macro name is in $Name
        ; Input is in $Offset
        ; Which is "Prolog" or "Epilog"
        ; Output is placed in __ParsedOffsetAbs, __ParsedOffsetPreinc, __ParsedOffsetString
        ;

        MACRO
        __ParseOffset $Name, $Offset, $Which

        ; copy to local string
        LCLS    OffsStr
OffsStr SETS    "$Offset"

        ; strip opening # if present
        IF OffsStr:LEFT:1 == "#"
OffsStr SETS    OffsStr:RIGHT:(:LEN:OffsStr - 1)
        ENDIF

        ; look for pre/postincrement forms
        IF OffsStr:RIGHT:1 == "!"

        ; prolog must be preincrement with a negative offset
        IF "$Which" == "Prolog"

        IF OffsStr:LEFT:1 != "-"
        INFO    1, "$Name: Preincrement offsets must be negative"
        MEXIT
        ENDIF

OffsStr SETS    OffsStr:LEFT:(:LEN:OffsStr - 1)
__ParsedOffsetAbs SETA $OffsStr
__ParsedOffsetAbs SETA -__ParsedOffsetAbs
__ParsedOffsetPreinc SETA 1
__ParsedOffsetRawString SETS "#":CC:OffsStr
__ParsedOffsetString SETS "[sp, #":CC:OffsStr:CC:"]!"

        ; epilog must be postincrement with a positive offset
        ELSE

        IF OffsStr:LEFT:1 == "-"
        INFO    1, "$Name: Postincrement offsets must not be negative"
        MEXIT
        ENDIF

OffsStr SETS    OffsStr:LEFT:(:LEN:OffsStr - 1)
__ParsedOffsetAbs SETA $OffsStr
__ParsedOffsetPreinc SETA 1
__ParsedOffsetRawString SETS "#":CC:OffsStr
__ParsedOffsetString SETS "[sp], #":CC:OffsStr

        ENDIF

        ; standard form
        ELSE

        IF OffsStr:LEFT:1 == "-"
        INFO    1, "$Name: Stack offsets must not be negative"
        MEXIT
        ENDIF

__ParsedOffsetAbs SETA $OffsStr
__ParsedOffsetPreinc SETA 0
__ParsedOffsetRawString SETS "#":CC:OffsStr
__ParsedOffsetString SETS "[sp, #":CC:OffsStr:CC:"]"

        ENDIF

__ParsedOffsetShifted3 SETA __ParsedOffsetAbs:SHR:3 - __ParsedOffsetPreinc
__ParsedOffsetShifted4 SETA __ParsedOffsetAbs:SHR:4 - __ParsedOffsetPreinc

        IF __ParsedOffsetAbs != ((__ParsedOffsetShifted3 + __ParsedOffsetPreinc):SHL:3) || __ParsedOffsetShifted3 >= 0x40
        INFO    1, "$Name: invalid offset $Offset"
        MEXIT
        ENDIF


        MEND


        ;
        ; Compute unwind codes for a register save operation
        ;
        ; Calling macro name is in $Name
        ; Input is in $Reg1, $Offset
        ; Which specifies "Prolog" or "Epilog"
        ; Output is placed in __ComputedCodes
        ;

        MACRO
        __ComputeSaveRegCodes $Name, $Reg1, $Offset, $Which

        LCLA    ByteVal
        LCLA    ByteVal2
        LCLA    ByteVal3
        LCLA    RegNum

        __ParseRegisterNumber $Name, $Reg1
RegNum  SETA    __ParsedRegNumber

        __ParseOffset $Name, $Offset, $Which

        IF (RegNum >= 19) && (RegNum <= 30)
ByteVal SETA 0xd0:OR:(__ParsedOffsetPreinc:SHL:2):OR:((RegNum - 19):SHR:2)
ByteVal2 SETA (((RegNum - 19):AND:3):SHL:6):OR:__ParsedOffsetShifted3
__ComputedCodes SETS "0x":CC:((:STR:ByteVal):RIGHT:2):CC:",0x":CC:((:STR:ByteVal2):RIGHT:2)

        ELIF (RegNum >= 40) && (RegNum <= 47)
ByteVal SETA 0xdc:OR:(__ParsedOffsetPreinc:SHL:1):OR:((RegNum - 40):SHR:2)
ByteVal2 SETA (((RegNum - 40):AND:3):SHL:6):OR:__ParsedOffsetShifted3
__ComputedCodes SETS "0x":CC:((:STR:ByteVal):RIGHT:2):CC:",0x":CC:((:STR:ByteVal2):RIGHT:2)




















        ELSE
        INFO    1, "$Name: Unsupported register: $Reg1"
        ENDIF

        MEND


        ;
        ; Compute unwind codes for a register pair save operation
        ;
        ; Calling macro name is in $Name
        ; Input is in $Reg1, $Reg2, $Offset
        ; Which specifies "Prolog" or "Epilog"
        ; Output is placed in __ComputedCodes
        ;

        MACRO
        __ComputeSaveRegPairCodes $Name, $Reg1, $Reg2, $Offset, $Which

        LCLA    ByteVal
        LCLA    ByteVal2
        LCLA    ByteVal3
        LCLA    RegNum1
        LCLA    RegNum2

        __ParseRegisterNumber $Name, $Reg1
RegNum1 SETA    __ParsedRegNumber

        __ParseRegisterNumber $Name, $Reg2
RegNum2 SETA    __ParsedRegNumber

        __ParseOffset $Name, $Offset, $Which

__RegPairWasFpLr SETL {false}

        IF (RegNum1 == 29) && (RegNum2 == 30)
ByteVal SETA    (0x40+(__ParsedOffsetPreinc*0x40)):OR:__ParsedOffsetShifted3
__ComputedCodes SETS "0x":CC:((:STR:ByteVal):RIGHT:2)
__RegPairWasFpLr SETL {true}

        ELIF (RegNum1 == 19) && (RegNum2 == 20) && (__ParsedOffsetPreinc != 0)
ByteVal SETA    0x20:OR:(__ParsedOffsetShifted3 + 1)
__ComputedCodes SETS "0x":CC:((:STR:ByteVal):RIGHT:2)

        ELIF (RegNum1 >= 19) && (RegNum1 <= 30) && (((RegNum1 - 19):AND:1) == 0) && (RegNum2 == 30) && (__ParsedOffsetPreinc == 0)
ByteVal SETA 0xd6:OR:((RegNum1 - 19):SHR:3)
ByteVal2 SETA ((((RegNum1 - 19):SHR:1):AND:3):SHL:6):OR:__ParsedOffsetShifted3
__ComputedCodes SETS "0x":CC:((:STR:ByteVal):RIGHT:2):CC:",0x":CC:((:STR:ByteVal2):RIGHT:2)

        ELIF (RegNum1 >= 19) && (RegNum1 <= 30) && (RegNum2 == (RegNum1 + 1))
ByteVal SETA 0xc8:OR:(__ParsedOffsetPreinc:SHL:2):OR:((RegNum1 - 19):SHR:2)
ByteVal2 SETA (((RegNum1 - 19):AND:3):SHL:6):OR:__ParsedOffsetShifted3
__ComputedCodes SETS "0x":CC:((:STR:ByteVal):RIGHT:2):CC:",0x":CC:((:STR:ByteVal2):RIGHT:2)

        ELIF (RegNum1 >= 40) && (RegNum1 <= 47) && (RegNum2 == (RegNum1 + 1))
ByteVal SETA 0xd8:OR:(__ParsedOffsetPreinc:SHL:1):OR:((RegNum1 - 40):SHR:2)
ByteVal2 SETA (((RegNum1 - 40):AND:3):SHL:6):OR:__ParsedOffsetShifted3
__ComputedCodes SETS "0x":CC:((:STR:ByteVal):RIGHT:2):CC:",0x":CC:((:STR:ByteVal2):RIGHT:2)















        ELSE
        INFO    1, "$Name: Unsupported register pair: $Reg1, $Reg2"
        ENDIF

        MEND


        ;
        ; Compute unwind codes for a stack alloc/dealloc operation
        ;
        ; Output is placed in __ComputedCodes
        ;

        MACRO
        __ComputeStackAllocCodes $Name, $Amount

        LCLA    Shifted
        LCLA    Byte1
        LCLA    Byte2
        LCLA    Byte3

Shifted SETA  ($Amount):SHR:4

        IF Shifted < 0x20
__ComputedCodes SETS "0x":CC:((:STR:Shifted):RIGHT:2)

        ELIF Shifted < 0x800
Byte1   SETA  0xC0:OR:((Shifted:SHR:8):AND:0x7)
Byte2   SETA  Shifted:AND:0xFF
__ComputedCodes SETS "0x":CC:((:STR:Byte1):RIGHT:2):CC:",0x":CC:((:STR:Byte2):RIGHT:2)

        ELIF Shifted < 0x1000000
Byte1   SETA  ((Shifted:SHR:16):AND:0xFF)
Byte2   SETA  ((Shifted:SHR:8):AND:0xFF)
Byte3   SETA  (Shifted:AND:0xFF)
__ComputedCodes SETS "0xE0,0x":CC:((:STR:Byte1):RIGHT:2):CC:",0x":CC:((:STR:Byte2):RIGHT:2):CC:",0x":CC:((:STR:Byte3):RIGHT:2)

        ELSE
        INFO    1, "$Name too large for unwind code encoding"
        ENDIF

        MEND


        ;
        ; Macro for allocating space on the stack in the prolog
        ;

        MACRO
        PROLOG_STACK_ALLOC $Amount

        __ComputeStackAllocCodes "PROLOG_STACK_ALLOC", $Amount

        __EmitRunningLabelAndOpcode sub sp, sp, #$Amount
        __DeclarePrologEnd
        __AppendPrologCodes

        MEND


        ;
        ; Macro for a single register save operation in a prologue
        ;

        MACRO
        PROLOG_SAVE_REG $Reg1, $Offset

        IF "$Offset" == ""
        INFO    1, "Must specify offset in PROLOG_SAVE_REG"
        MEXIT
        ENDIF

        __ComputeSaveRegCodes "PROLOG_SAVE_REG", $Reg1, $Offset, "Prolog"

        __EmitRunningLabelAndOpcode str $Reg1, $__ParsedOffsetString
        __DeclarePrologEnd
        __AppendPrologCodes

        MEND


        ;
        ; Macro for an register pair save operation in a prologue
        ;

        MACRO
        PROLOG_SAVE_REG_PAIR $Reg1, $Reg2, $Offset

        IF "$Offset" == ""
        INFO    1, "Must specify offset in PROLOG_SAVE_REG_PAIR"
        MEXIT
        ENDIF

        __ComputeSaveRegPairCodes "PROLOG_SAVE_REG_PAIR", $Reg1, $Reg2, $Offset, "Prolog"

        IF __RegPairWasFpLr







        __EmitRunningLabelAndOpcode stp fp, lr, $__ParsedOffsetString



        IF (__ParsedOffsetAbs != 0) && (__ParsedOffsetPreinc == 0)
        __EmitRunningLabelAndOpcode add fp, sp, $__ParsedOffsetRawString
__ComputedCodes SETS "0xe2,0x":CC:((:STR:__ParsedOffsetShifted3):RIGHT:2):CC:",":CC:__ComputedCodes
        ELSE

        __EmitRunningLabelAndOpcode mov fp, sp
__ComputedCodes SETS "0xe1,":CC:__ComputedCodes
        ENDIF

        ELSE

        __EmitRunningLabelAndOpcode stp $Reg1, $Reg2, $__ParsedOffsetString

        ENDIF

        __DeclarePrologEnd
        __AppendPrologCodes

        MEND


        ;
        ; Same as above but don't treat FP specially
        ;

        MACRO
        PROLOG_SAVE_REG_PAIR_NO_FP $Reg1, $Reg2, $Offset

        IF "$Offset" == ""
        INFO    1, "Must specify offset in PROLOG_SAVE_REG_PAIR"
        MEXIT
        ENDIF

        __ComputeSaveRegPairCodes "PROLOG_SAVE_REG_PAIR", $Reg1, $Reg2, $Offset, "Prolog"

        IF __RegPairWasFpLr







        __EmitRunningLabelAndOpcode stp fp, lr, $__ParsedOffsetString



        ELSE

        __EmitRunningLabelAndOpcode stp $Reg1, $Reg2, $__ParsedOffsetString

        ENDIF

        __DeclarePrologEnd
        __AppendPrologCodes

        MEND


        ;
        ; Macro for including an arbitrary operation in the prolog
        ;

        MACRO
        PROLOG_NOP $O1,$O2,$O3,$O4

__ComputedCodes SETS "0xE3"

        __EmitRunningLabelAndOpcode $O1,$O2,$O3,$O4
        __DeclarePrologEnd
        __AppendPrologCodes

        MEND


        ;
        ; Macro for saving the next pair of registers
        ;

        MACRO
        PROLOG_SAVE_NEXT_PAIR $O1,$O2,$O3,$O4

__ComputedCodes SETS "0xE6"

        __EmitRunningLabelAndOpcode $O1,$O2,$O3,$O4
        __DeclarePrologEnd
        __AppendPrologCodes

        MEND


        ;
        ; Macro for indicating a trap frame lives above us
        ;

        MACRO
        PROLOG_PUSH_TRAP_FRAME
        __DeclarePrologEnd

__ComputedCodes SETS "0xE8"
        __AppendPrologCodes

        MEND


        ;
        ; Macro for indicating a machine frame lives above us
        ;

        MACRO
        PROLOG_PUSH_MACHINE_FRAME
        __DeclarePrologEnd

__ComputedCodes SETS "0xE9"
        __AppendPrologCodes

        MEND


        ;
        ; Macro for indicating a ARM64_NT_CONTEXT lives above us
        ;

        MACRO
        PROLOG_PUSH_CONTEXT
        __DeclarePrologEnd

__ComputedCodes SETS "0xEA"
        __AppendPrologCodes

        MEND


        ;
        ; Macro for indicating a ARM64EC_NT_CONTEXT lives above us
        ;

        MACRO
        PROLOG_PUSH_EC_CONTEXT
        __DeclarePrologEnd

__ComputedCodes SETS "0xEB"
        __AppendPrologCodes

        MEND


        ;
        ; Macro for signing the return address in the prolog
        ;

        MACRO
        PROLOG_SIGN_RETURN_ADDRESS

__ComputedCodes SETS "0xFC"

        __EmitRunningLabelAndOpcode pacibsp
        __DeclarePrologEnd
        __AppendPrologCodes

        MEND


        ;
        ; Macro for restoring the stack pointer.
        ;

        MACRO
        EPILOG_STACK_RESTORE

__ComputedCodes SETS "0xE1"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode mov sp, fp
        __AppendEpilogCodes

        MEND


        ;
        ; Macro for deallocating space on the stack in the prolog
        ;

        MACRO
        EPILOG_STACK_FREE $Amount

        __ComputeStackAllocCodes "EPILOG_STACK_FREE", $Amount

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode add sp, sp, #$Amount
        __AppendEpilogCodes

        MEND


        ;
        ; Macro for a single integer register restore operation in an epilogue
        ;

        MACRO
        EPILOG_RESTORE_REG $Reg1, $Offset

        IF "$Offset" == ""
        INFO    1, "Must specify offset in EPILOG_RESTORE_REG"
        MEXIT
        ENDIF

        __ComputeSaveRegCodes "EPILOG_RESTORE_REG", $Reg1, $Offset, "Epilog"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode ldr $Reg1, $__ParsedOffsetString
        __AppendEpilogCodes

        MEND


        ;
        ; Macro for an integer register pair restore operation in an epilogue
        ;

        MACRO
        EPILOG_RESTORE_REG_PAIR $Reg1, $Reg2, $Offset

        IF "$Offset" == ""
        INFO    1, "Must specify offset in EPILOG_RESTORE_REG_PAIR"
        MEXIT
        ENDIF

        __ComputeSaveRegPairCodes "EPILOG_RESTORE_REG_PAIR", $Reg1, $Reg2, $Offset, "Epilog"

        __DeclareEpilogStart

        IF __RegPairWasFpLr







        __EmitRunningLabelAndOpcode ldp fp, lr, $__ParsedOffsetString




        ELSE

        __EmitRunningLabelAndOpcode ldp $Reg1, $Reg2, $__ParsedOffsetString

        ENDIF

        __AppendEpilogCodes

        MEND


        ;
        ; Macro for including an arbitrary operation in the epilog
        ;

        MACRO
        EPILOG_NOP $O1,$O2,$O3,$O4

__ComputedCodes SETS "0xE3"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode $O1,$O2,$O3,$O4
        __AppendEpilogCodes

        MEND


        ;
        ; Macro for restoring the next pair of registers
        ;

        MACRO
        EPILOG_RESTORE_NEXT_PAIR $O1,$O2,$O3,$O4

__ComputedCodes SETS "0xE6"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode $O1,$O2,$O3,$O4
        __AppendEpilogCodes

        MEND


        ;
        ; Macro for authenticating the return address in the epilog
        ;

        MACRO
        EPILOG_AUTHENTICATE_RETURN_ADDRESS

__ComputedCodes SETS "0xFC"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode autibsp
        __AppendEpilogCodes

        MEND


        ;
        ; Macro for a bx lr-style return in the epilog
        ;

        MACRO
        EPILOG_RETURN

__ComputedCodes SETS "0xE4"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode ret x30
        __AppendEpilogCodes
        __DeclareEpilogEnd

        MEND


        ;
        ; Macro for generic end of epilog such as direct
        ; (b) or indirect (br) tail calls.
        ; 

        MACRO
        EPILOG_END $O1,$O2,$O3,$O4

__ComputedCodes SETS "0xE4"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode $O1,$O2,$O3,$O4
        __AppendEpilogCodes
        __DeclareEpilogEnd

        MEND


        ;
        ; Emit an opcode indicating that the unwind address of this function
        ; will be at the next instruction after a call rather than the call
        ; instruction. An example where this is necessary is the security cookie
        ; pop code, which modifies the caller's frame.
        ;

        MACRO
        EPILOG_RETURN_CLEAR_UNWIND_TO_CALLER

__ComputedCodes SETS "0xEC, 0XE4"

        __DeclareEpilogStart
        __EmitRunningLabelAndOpcode ret x30
        __EmitRunningLabelAndOpcode nop
        __AppendEpilogCodes
        __DeclareEpilogEnd

        MEND

        ;
        ; Macro to reset the internal uwninding states
        ;

        MACRO
        __ResetUnwindState $ExceptHandler
__PrologUnwindString SETS ""
__EpilogUnwindCount SETA 0
__Epilog1UnwindString SETS ""
__Epilog2UnwindString SETS ""
__Epilog3UnwindString SETS ""
__Epilog4UnwindString SETS ""
__EpilogStartNotDefined SETL {true}
__FuncExceptionHandler SETS ""
        IF "$ExceptHandler" != ""
__FuncExceptionHandler SETS "$ExceptHandler"

        ;
        ; Add bars to the exception handler name only when the name does not have bars
        ;

        IF ("$ExceptHandler":LEFT:1 != "|") && ("$ExceptHandler":RIGHT:1 != "|")
__FuncExceptionHandler SETS "|$ExceptHandler|"
        ENDIF

        ENDIF
        MEND


        ;
        ; Macro to emit the xdata for unwinding
        ;

        MACRO
        __EmitUnwindXData

        LCLA    XBit

XBit    SETA    0
        IF "$__FuncExceptionHandler" != ""
XBit    SETA    1:SHL:20
        ENDIF

        ;
        ; Append terminators where necessary
        ;
        IF __EpilogUnwindCount >= 1
__Epilog1UnwindString SETS __Epilog1UnwindString:RIGHT:(:LEN:__Epilog1UnwindString - 1)
        IF (:LEN:__Epilog1UnwindString) >= 5
        IF __Epilog1UnwindString:RIGHT:4 < "0xE4"
__Epilog1UnwindString SETS __Epilog1UnwindString:CC:",0xE4"
        ENDIF
        ENDIF
        ENDIF

        IF __EpilogUnwindCount >= 2
__Epilog2UnwindString SETS __Epilog2UnwindString:RIGHT:(:LEN:__Epilog2UnwindString - 1)
        IF (:LEN:__Epilog2UnwindString) >= 5
        IF __Epilog2UnwindString:RIGHT:4 < "0xE4"
__Epilog2UnwindString SETS __Epilog2UnwindString:CC:",0xE4"
        ENDIF
        ENDIF
        ENDIF

        IF __EpilogUnwindCount >= 3
__Epilog3UnwindString SETS __Epilog3UnwindString:RIGHT:(:LEN:__Epilog3UnwindString - 1)
        IF (:LEN:__Epilog3UnwindString) >= 5
        IF __Epilog3UnwindString:RIGHT:4 < "0xE4"
__Epilog3UnwindString SETS __Epilog3UnwindString:CC:",0xE4"
        ENDIF
        ENDIF
        ENDIF

        IF __EpilogUnwindCount >= 4
__Epilog4UnwindString SETS __Epilog4UnwindString:RIGHT:(:LEN:__Epilog4UnwindString - 1)
        IF (:LEN:__Epilog4UnwindString) >= 5
        IF __Epilog4UnwindString:RIGHT:4 < "0xE4"
__Epilog4UnwindString SETS __Epilog4UnwindString:CC:",0xE4"
        ENDIF
        ENDIF
        ENDIF

        IF "$__PrologUnwindString" != ""
__PrologUnwindString SETS __PrologUnwindString:CC:"0xE4"
        ELSE
__PrologUnwindString SETS "0xE4"
        ENDIF

        ; optimize out the prolog string if it matches
;        IF (:LEN:__Epilog1UnwindString) >= 6
;        IF __Epilog1UnwindString:LEFT:(:LEN:__Epilog1UnwindString - 4) == __PrologUnwindString:LEFT:(:LEN:__PrologUnwindString - 4)
;__PrologUnwindString SETS ""
;        ENDIF
;        ENDIF

        ;
        ; Switch to the .xdata section, aligned to a DWORD
        ;

        IF __FuncComDat != ""
        AREA    $__FuncXDataArea,ALIGN=2,READONLY,ASSOC=$__FuncArea
        ELSE
        AREA    $__FuncXDataArea,ALIGN=2,READONLY
        ENDIF

        ALIGN   4

        ; declare the xdata header with unwind code size, epilog count,
        ; exception bit, and function length
$__FuncXDataLabel
        DCD     ((($__FuncXDataEndLabel - $__FuncXDataPrologLabel)/4):SHL:27) :OR: (__EpilogUnwindCount:SHL:22) :OR: XBit :OR: (($__FuncEndLabel - $__FuncStartLabel)/4)

        ; if we have an epilogue, output a single scope record
        IF __EpilogUnwindCount >= 1
        DCD     (($__FuncXDataEpilog1Label - $__FuncXDataPrologLabel):SHL:22) :OR: (($__FuncEpilog1StartLabel - $__FuncStartLabel)/4)
        ENDIF
        IF __EpilogUnwindCount >= 2
        DCD     (($__FuncXDataEpilog2Label - $__FuncXDataPrologLabel):SHL:22) :OR: (($__FuncEpilog2StartLabel - $__FuncStartLabel)/4)
        ENDIF
        IF __EpilogUnwindCount >= 3
        DCD     (($__FuncXDataEpilog3Label - $__FuncXDataPrologLabel):SHL:22) :OR: (($__FuncEpilog3StartLabel - $__FuncStartLabel)/4)
        ENDIF
        IF __EpilogUnwindCount >= 4
        DCD     (($__FuncXDataEpilog4Label - $__FuncXDataPrologLabel):SHL:22) :OR: (($__FuncEpilog4StartLabel - $__FuncStartLabel)/4)
        ENDIF

        ; output the prolog unwind string
$__FuncXDataPrologLabel
        DCB     $__PrologUnwindString

        ; if we have an epilogue, output the epilog unwind codes
        IF __EpilogUnwindCount >= 1
$__FuncXDataEpilog1Label
        DCB     $__Epilog1UnwindString
        ENDIF
        IF __EpilogUnwindCount >= 2
$__FuncXDataEpilog2Label
        DCB     $__Epilog2UnwindString
        ENDIF
        IF __EpilogUnwindCount >= 3
$__FuncXDataEpilog3Label
        DCB     $__Epilog3UnwindString
        ENDIF
        IF __EpilogUnwindCount >= 4
$__FuncXDataEpilog4Label
        DCB     $__Epilog4UnwindString
        ENDIF

        ALIGN   4
$__FuncXDataEndLabel

        ; output the exception handler information
        IF "$__FuncExceptionHandler" != ""
        DCD     $__FuncExceptionHandler
        RELOC   2                                       ; make this relative to image base
        DCD     0                                       ; append a 0 for the data (keeps Vulcan happy)
        ENDIF

        ; switch back to the original area
        AREA    $__FuncArea,CODE,READONLY

        MEND



;
; For assembly files that are built for both ARM64 and ARM64EC (discouraged
; since the files might not have been ported to use X64 behavior in the ARM64EC
; paths), use this macro to wrap all references to function names (ASM and C).
;
; For ARM64, this does nothing.
;
; For ARM64EC, this changes FuncName to |#FuncName|.
;












        ;
        ; Global variables
        ;

        ; Current function names and labels
        GBLS    __FuncNameNoBars
        GBLS    __FuncStartLabel
        GBLS    __FuncEpilog1StartLabel
        GBLS    __FuncEpilog2StartLabel
        GBLS    __FuncEpilog3StartLabel
        GBLS    __FuncEpilog4StartLabel
        GBLS    __FuncPDataLabel
        GBLS    __FuncXDataLabel
        GBLS    __FuncXDataPrologLabel
        GBLS    __FuncXDataEpilog1Label
        GBLS    __FuncXDataEpilog2Label
        GBLS    __FuncXDataEpilog3Label
        GBLS    __FuncXDataEpilog4Label
        GBLS    __FuncXDataEndLabel
        GBLS    __FuncEndLabel
        GBLS    __FuncEntryThunkLabel
        GBLS    __FuncExitThunkLabel

        ; other globals relating to the current function
        GBLS    __FuncComDat
        GBLS    __FuncArea
        GBLS    __FuncPDataArea
        GBLS    __FuncXDataArea
        GBLA    __FuncAlignment
__FuncAlignment SETA 4

        ;
        ; Helper macro: generate the various labels we will use internally
        ; for a function
        ;
        ; Output is placed in the various __Func*Label globals
        ;

        MACRO
        __DeriveFunctionLabels $FuncName, $AreaName

__FuncNameNoBars        SETS "$FuncName"
        IF ("$FuncName":LEFT:1 == "|") && ("$FuncName":RIGHT:1 == "|")
__FuncNameNoBars        SETS ("$FuncName":LEFT:(:LEN:"$FuncName" - 1):RIGHT:(:LEN:"$FuncName" - 2))
        ENDIF
__FuncStartLabel        SETS "|$__FuncNameNoBars|"
__FuncEndLabel          SETS "|$__FuncNameNoBars._end|"
__FuncEpilog1StartLabel SETS "|$__FuncNameNoBars._epilog1_start|"
__FuncEpilog2StartLabel SETS "|$__FuncNameNoBars._epilog2_start|"
__FuncEpilog3StartLabel SETS "|$__FuncNameNoBars._epilog3_start|"
__FuncEpilog4StartLabel SETS "|$__FuncNameNoBars._epilog4_start|"
__FuncPDataLabel        SETS "|$__FuncNameNoBars._pdata|"
__FuncXDataLabel        SETS "|$__FuncNameNoBars._xdata|"
__FuncXDataPrologLabel  SETS "|$__FuncNameNoBars._xdata_prolog|"
__FuncXDataEpilog1Label SETS "|$__FuncNameNoBars._xdata_epilog1|"
__FuncXDataEpilog2Label SETS "|$__FuncNameNoBars._xdata_epilog2|"
__FuncXDataEpilog3Label SETS "|$__FuncNameNoBars._xdata_epilog3|"
__FuncXDataEpilog4Label SETS "|$__FuncNameNoBars._xdata_epilog4|"
__FuncXDataEndLabel     SETS "|$__FuncNameNoBars._xdata_end|"
__FuncEntryThunkLabel   SETS "|$__FuncNameNoBars._entry_thunk|"
__FuncArea              SETS "|.text|"
__FuncPDataArea         SETS "|.pdata|"
__FuncXDataArea         SETS "|.xdata|"
        IF "$AreaName" != ""
__FuncArea              SETS "$AreaName"
        ENDIF
        IF __FuncComDat != ""
__FuncArea              SETS __FuncArea:CC:"{|$__FuncNameNoBars|}"
__FuncPDataArea         SETS __FuncPDataArea:CC:"{$__FuncPDataLabel}"
__FuncXDataArea         SETS __FuncXDataArea:CC:"{$__FuncXDataLabel}"
        ENDIF

        MEND


        ;
        ; Helper macro: create a global label for the given name,
        ; decorate it, and export it for external consumption.
        ;

        MACRO
        __ExportName $FuncName

        LCLS    Name

        IF ("$FuncName":LEFT:1 == "|") && ("$FuncName":RIGHT:1 == "|")
Name    SETS    "$FuncName"
        ELSE
Name    SETS    "|$FuncName|"
        ENDIF

        ALIGN   4
        EXPORT  $Name
$Name
        MEND

        MACRO
        __ExportProc $FuncName

        LCLS    Name
Name    SETS    "|$FuncName|"
        ALIGN   4
        EXPORT  $Name
$Name   PROC
        MEND


        ;
        ; Helper macro to set the AREA to the correct answer
        ; for the current function, and configure the alignment.
        ;

        MACRO
        __SetFunctionAreaAndAlign $Alignment

        LCLS    AreaAlign
        LCLS    AlignStmt

        ;
        ; "NOALIGN" is supported to just set the area
        ;

        IF "$Alignment" == "NOALIGN"
        AREA    $__FuncArea

        ;
        ; COMDAT functions must set alignment in the AREA
        ; statement
        ;

        ELIF __FuncComDat != ""

AreaAlign SETS "4"
        IF "$Alignment" != ""
        IF $Alignment > 4
AreaAlign SETS "$Alignment"
        ENDIF
        ENDIF

        AREA    $__FuncArea,CODE,READONLY,ALIGN=$AreaAlign

        ELSE

AlignStmt SETS ""
        IF "$Alignment" != ""
AlignStmt SETS "ALIGN 0x":CC: :STR:(1 << $Alignment)
        ENDIF

        AREA    $__FuncArea,CODE,READONLY
        $AlignStmt

        ENDIF

        MEND


        ;
        ; Helper macro to emit the special DWORD prior to the start of a
        ; function which contains an offset to the ARM64EC entry thunk.
        ; The entry thunk must have been previously defined or else this
        ; macro is a no-op.
        ;

        MACRO
        __AddEntryThunkPointer $Alignment

        IF :DEF:$__FuncEntryThunkLabel
        ASSERT(__FuncComDat == "")
        ALIGN   (1 << $Alignment),(1 << $Alignment) - 4
        dcd     ($__FuncEntryThunkLabel - ({PC} + 4)) + 1
        ENDIF

        MEND


        ;
        ; Declare that all following code/data is to be put in the .text segment
        ;
        ; N.B. The ALIGN attribute here specifies an exponent of base 2; not a
        ;      direct byte count. Thus ALIGN=4 specifies a 16 byte alignment.
        ;

        MACRO
        TEXTAREA
        AREA    |.text|,ALIGN=4,CODE,READONLY
        MEND


        ;
        ; Declare that all following code/data is to be put in the .data segment
        ;

        MACRO
        DATAAREA
        AREA    |.data|,DATA
        MEND


        ;
        ; Declare that all following code/data is to be put in the .rdata segment
        ;

        MACRO
        RODATAAREA
        AREA    |.rdata|,DATA,READONLY
        MEND


        ;
        ; Set/reset the alignment for COMDAT functions that follow.
        ;
        MACRO
        SET_COMDAT_ALIGNMENT $Alignment

__FuncAlignment SETA $Alignment
        IF __FuncAlignment < 4
__FuncAlignment SETA 4
        ENDIF

        MEND


        MACRO
        RESET_COMDAT_ALIGNMENT

__FuncAlignment SETA 4

        MEND


        ;
        ; Macro for indicating the start of a nested function. Nested functions
        ; imply a prolog, epilog, and unwind codes. This macro presumes that
        ; __DeriveFunctionLabels and __SetFunctionAreaAndAlign have already been
        ; called as appropriate
        ;

        MACRO
        NESTED_ENTRY $FuncName, $AreaName, $ExceptHandler, $Alignment

__FuncComDat SETS ""
        __DeriveFunctionLabels $FuncName,$AreaName
        __SetFunctionAreaAndAlign $Alignment

        IF ("$Alignment" != "")
__FuncAlignment SETA $Alignment
        ELSE
__FuncAlignment SETA 4
        ENDIF

        __AddEntryThunkPointer $__FuncAlignment
        __ResetUnwindState $ExceptHandler
        __ExportProc $__FuncNameNoBars
        ROUT

        MEND


        MACRO
        NESTED_ENTRY_COMDAT $FuncName, $AreaName, $ExceptHandler, $Alignment

        IF ("$Alignment" != "")
        SET_COMDAT_ALIGNMENT $Alignment
        ENDIF

__FuncComDat SETS "COMDAT"
        __DeriveFunctionLabels $FuncName,$AreaName
        __SetFunctionAreaAndAlign $__FuncAlignment
        ASSERT (!:DEF:$__FuncEntryThunkLabel)
        __ResetUnwindState $ExceptHandler
        __ExportProc $__FuncNameNoBars
        ROUT

        MEND


        ;
        ; Generate an ARM64EC entry thunk for an upcoming function.
        ; Note that only COMDAT functions are supported.
        ;


















































































































































        MACRO
        ARM64EC_ENTRY_THUNK $FuncName, $Parameters, $SaveQCount, $AreaName, $Alignment
        MEND

        MACRO
        ARM64EC_CUSTOM_ENTRY_THUNK $FuncName, $AreaName, $Alignment
        MEND



        ;
        ; Macro for indicating the end of a nested function. We generate the
        ; .pdata and .xdata records here as necessary.
        ;
        ; Note that the $FuncName parameter is vestigial and not consumed.
        ;

        MACRO
        NESTED_END $FuncName

        ; mark the end of the function
$__FuncEndLabel
        LTORG
        ENDP

        ; generate .pdata

        IF __FuncComDat != ""
        AREA    $__FuncPDataArea,ALIGN=2,READONLY,ASSOC=$__FuncArea
        ELSE
        AREA    $__FuncPDataArea,ALIGN=2,READONLY
        ENDIF

        DCD     $__FuncStartLabel
        RELOC   2                                       ; make this relative to image base

        DCD     $__FuncXDataLabel
        RELOC   2                                       ; make this relative to image base

        ; generate .xdata
        __EmitUnwindXData

        ; back to the original area
        __SetFunctionAreaAndAlign NOALIGN

        ; reset the labels
__FuncStartLabel SETS    ""
__FuncEndLabel  SETS    ""

        MEND


        ;
        ; Macro for indicating the start of a leaf function.
        ;

        MACRO
        LEAF_ENTRY $FuncName, $AreaName, $Alignment

        NESTED_ENTRY $FuncName, $AreaName, "", $Alignment

        MEND


        MACRO
        LEAF_ENTRY_COMDAT $FuncName, $AreaName, $Alignment

        NESTED_ENTRY_COMDAT $FuncName, $AreaName, "", $Alignment

        MEND


        ;
        ; Macro for indicating the end of a leaf function.
        ;

        MACRO
        LEAF_END $FuncName

        NESTED_END $FuncName

        MEND










        ;
        ; Macro for indicating the start of a leaf function.
        ;

        MACRO
        LEAF_ENTRY_NO_PDATA $FuncName, $AreaName

        ; compute the function's labels
        __DeriveFunctionLabels $FuncName,$AreaName
        __SetFunctionAreaAndAlign

        ; export the function name
        __ExportProc $__FuncNameNoBars

        ; flush any pending literal pool stuff
        ROUT

        MEND


        ;
        ; Macro for indicating the end of a leaf function.
        ;

        MACRO
        LEAF_END_NO_PDATA $FuncName

        ; mark the end of the function
$__FuncEndLabel
        LTORG
        ENDP

        ; reset the labels
__FuncStartLabel SETS    ""
__FuncEndLabel  SETS    ""

        MEND




        ;
        ; Macro for indicating an alternate entry point into a function.
        ;

        MACRO
        ALTERNATE_ENTRY $FuncName

        ; export the entry point's name
        __ExportName $FuncName

        ; flush any pending literal pool stuff
        ROUT

        MEND


        ;
        ; Macro for getting the address of a data item.
        ;
        
        MACRO
        ADDROF $Reg, $Variable
        
        adrp    $Reg, $Variable                 ; get the page address first
        add     $Reg, $Reg, $Variable           ; add in the low bits
        
        MEND


        ;
        ; Macro for loading a 32-bit constant.
        ;
        
        MACRO
        MOVL32 $Reg, $Variable
        
        IF ((($Variable):SHR:16):AND:0xffff) == 0
        movz    $Reg, #$Variable
        ELIF ((($Variable):SHR:0):AND:0xffff) == 0
        movz    $Reg, #((($Variable):SHR:16):AND:0xffff), lsl #16
        ELSE
        movz    $Reg, #(($Variable):AND:0xffff)
        movk    $Reg, #((($Variable):SHR:16):AND:0xffff), lsl #16
        ENDIF
        
        MEND


















































        MACRO
        CAPSTART $arg1, $arg2
        MEND

        MACRO
        CAPEND $arg1
        MEND




        ;
        ; Macro to align a Control Flow Guard valid call target.
        ; Not necessary to use this before functions anymore as
        ; it is the default for NESTED_ENTRY/LEAF_ENTRY macros.
        ;

        MACRO
        CFG_ALIGN
        ALIGN 16
        MEND


        ;
        ; Macro to perform a Control Flow Guard check on a live call target.
        ;
        ; $TargetReg - Target address register
        ; x16 - Bitmap address
        ; $FailLabel - Label to jump to in the event of a failure
        ; 
        ; N.B. x16-x17 are free, other registers should be treated as live.
        ;
        ; N.B. ValidTarget should only specify a label for the ARM64EC checkers.
        ;      The function has logic below which depends on this.
        ;

        MACRO
        CFG_ICALL_CHECK_BITMAP $TargetReg, $ValidTarget, $FailLabel

        LCLS    ValidLabel

ValidLabel SETS ""

        IF ("$ValidTarget":LEFT:1 != "x" && "$ValidTarget":LEFT:1 != "w" && "$ValidTarget" != "lr")

ValidLabel SETS "$ValidTarget"

        ENDIF

;
; Bitmap is an array of 2-bit values. Each 2-bit value represents 16 bytes, with the low
; bit set if jumps are permitted, and the upper bit set if misaligned jumps are permitted.
;
; The bit index is (address >> 4).
; Each byte holds 4 entries, so the byte index is (address >> 6).
; The shift amount is computed as 2*((address >> 4) & 3), or (address >> 3) & 6
; If address is aligned, (address >> 3) & 7 is equivalent, and ubfx can be used to extract.
;

        lsr     x17, $TargetReg, #6             ; compute bitmap byte index
        tst     $TargetReg, #15                 ; misaligned address?
        ldrb    w17, [x16, x17]                 ; load byte from bitmap
        ubfx    x16, $TargetReg, #3, #3         ; compute bit index*2
        bne     %F2                             ; if misaligned, account for extra bits
        lsr     x17, x17, x16                   ; shift bitmap chunk over to valid align bit
        tbz     x17, #0, %F3                    ; if low bit not set, verify the upper bit to
                                                ; allow an export suppressed target
1 ; Valid

;
; "ret lr" and "br lr" have different encodings and use of "ret lr" is
; preferred to hint that it is returning to the caller.
;

        IF "$ValidLabel" != ""
            b       $ValidLabel                 ; jump to valid target
        ELIF "$ValidTarget" != "lr"
            br      $ValidTarget                ; jump to valid target
        ELSE
            ret     lr                          ; return
        ENDIF

2 ; Misaligned
        ;
        ; Code on ARM64 should always be 16 byte aligned if address taken.
        ;
        ; TODO:  Compress bitmap format to 1 bit per address on ARM64?
        ; TODO:  We are seeing 8-byte aligned code now. (1/22/14)
        ;
        and     x16, x16, #0xfffffffffffffffe   ; force low bit of shift to 0
        lsr     x17, x17, x16                   ; shift bitmap chunk down
        tbz     x17, #0, %F4                    ; invalid if the low bit was clear
3 ; FailOpen
        tbnz    x17, #1, %B1                    ; valid if upper bit was set as well
4 ; Failure
        IF "$ValidLabel" != ""
            ;
            ; Don't need to explicitly set mode for ARM64EC.
            ;
        ELIF "$ValidTarget" != "lr"
            mov     x16, #1                     ; CFG dispatch mode
            IF ("$TargetReg" != "x15")
                mov     x15, $TargetReg         ; move CFG target to x15 for failure function
            ENDIF
        ELSE
            mov     x16, #0                     ; CFG check mode
            IF ("$TargetReg" != "x15")
                mov     x15, $TargetReg         ; move CFG target to x15 for failure function
            ENDIF
        ENDIF

        b       $FailLabel                      ; jump to failure function
        MEND


        ;
        ; Macro to perform a Control Flow Guard check on a live call target with export suppression.
        ;
        ; $TargetReg - Target address register
        ; x16 - Bitmap address
        ; $FailLabel - Label to jump to in the event of a failure
        ; 
        ; N.B. x16-x17 are free, other registers should be treated as live.
        ;
        ; N.B. ValidTarget should only specify a label for the ARM64EC checkers.
        ;      The function has logic below which depends on this.
        ;

        MACRO
        CFG_ICALL_CHECK_BITMAP_ES $TargetReg, $ValidTarget, $FailLabel

        LCLS    ValidLabel

ValidLabel SETS ""

        IF ("$ValidTarget":LEFT:1 != "x" && "$ValidTarget":LEFT:1 != "w" && "$ValidTarget" != "lr")

ValidLabel SETS "$ValidTarget"

        ENDIF

;
; Bitmap is an array of 2-bit values. Each 2-bit value represents 16 bytes, with the low
; bit set if jumps are permitted, and the upper bit set if misaligned jumps are permitted.
;
; The bit index is (address >> 4).
; Each byte holds 4 entries, so the byte index is (address >> 6).
; The shift amount is computed as 2*((address >> 4) & 3), or (address >> 3) & 6
; If address is aligned, (address >> 3) & 7 is equivalent, and ubfx can be used to extract.
;

        lsr     x17, $TargetReg, #6             ; compute bitmap byte index
        tst     $TargetReg, #15                 ; misaligned address?
        ldrb    w17, [x16, x17]                 ; load byte from bitmap
        ubfx    x16, $TargetReg, #3, #3         ; compute bit index*2
        bne     %F2                             ; if misaligned, account for extra bits
        lsr     x17, x17, x16                   ; shift bitmap chunk over to valid align bit
        tbz     x17, #0, %F3                    ; if low bit not set, either invalid or export
                                                ; suppressed target
1 ; Valid

;
; "ret lr" and "br lr" have different encodings and use of "ret lr" is
; preferred to hint that it is returning to the caller.
;

        IF "$ValidLabel" != ""
            b       $ValidLabel                 ; jump to valid target
        ELIF "$ValidTarget" != "lr"
            br      $ValidTarget                ; jump to valid target
        ELSE
            ret     lr                          ; return
        ENDIF

2 ; Misaligned
        ;
        ; Code on ARM64 should always be 16 byte aligned if address taken.
        ;
        ; TODO:  Compress bitmap format to 1 bit per address on ARM64?
        ; TODO:  We are seeing 8-byte aligned code now. (1/22/14)
        ;
        and     x16, x16, #0xfffffffffffffffe   ; force low bit of shift to 0
        lsr     x17, x17, x16                   ; shift bitmap chunk down
        tbz     x17, #0, %F3                    ; invalid if the low bit was clear
        tbnz    x17, #1, %B1                    ; valid if upper bit was set as well

3 ; Failure
        IF "$ValidLabel" != ""
            ;
            ; Don't need to explicitly set mode for ARM64EC.
            ;
        ELIF "$ValidTarget" != "lr"
            mov     x16, #1                     ; CFG dispatch mode
            IF ("$TargetReg" != "x15")
                mov     x15, $TargetReg         ; move CFG target to x15 for failure function
            ENDIF
        ELSE
            mov     x16, #0                     ; CFG check mode
            IF ("$TargetReg" != "x15")
                mov     x15, $TargetReg         ; move CFG target to x15 for failure function
            ENDIF
        ENDIF

        b       $FailLabel                      ; jump to failure function
        MEND


        ;
        ; Macro to acquire a spin lock at address $Reg + $Offset. Clobbers {r0-r2}
        ;

; ARM64_WORKITEM : should we use acquire/release semantics instead of DMB?

;
; TODO: Today this routine is not used. If it is used in the future, consider
;       whether the yield should be switched to enlightened yield.
;

        MACRO
        ACQUIRE_SPIN_LOCK $Reg, $Offset

        mov     x0, #1                                  ; we want to exchange with a 1
        dmb                                             ; memory barrier ahead of the loop
1
        ldxr    x1, [$Reg, $Offset]                     ; load the new value
        stxr    x2, x0, [$Reg, $Offset]                 ; attempt to store the 1
        cbnz    x2, %B1                                 ; did we succeed before someone else did?
        cbz     x1, %F3                                 ; was the lock previously owned? if not, we're done
        yield                                           ; yield execution
        b       %B1                                     ; and try again
3
        dmb

        MEND


        ;
        ; Macro to release a spin lock at address $Reg + $Offset.
        ;

; ARM64_WORKITEM : should we use acquire/release semantics instead of DMB?

        MACRO
        RELEASE_SPIN_LOCK $Reg, $Offset

        dmb
        str     xzr, [$Reg, $Offset]                    ; store 0

        MEND


        ;
        ; Macro to increment a 64-bit statistic.
        ;

        MACRO
        INCREMENT_STAT $AddrReg, $Temp1, $Temp2

1       ldxr    $Temp1, [$AddrReg]                      ; load current value
        add     $Temp1, $Temp1, #1                      ; increment
        stxr    $Temp2, $Temp1, [$AddrReg]              ; attempt to store
        cbnz    $Temp2, %B1                             ; loop until it works?

        MEND


        ;
        ; Macros to enable/disable interrupts.
        ;
        
        MACRO
        ENABLE_INTERRUPTS
        msr     DAIFClr, #2                             ; enable interrupts
        MEND

        MACRO
        DISABLE_INTERRUPTS
        msr     DAIFSet, #2                             ; disable interrupts
        MEND


        ;
        ; Macros to read/write the current IRQL
        ;
        ; N.B. These macros do not do hardware and software IRQL processing.
        ;

        MACRO
        GET_IRQL $Irql
        ldrb    $Irql, [x18, #0x38]            ; read IRQL
        MEND

        MACRO
        RAISE_IRQL $Reg, $NewIrql











        mov     $Reg, #$NewIrql                         ; get new IRQL
        strb    $Reg, [x18, #0x38]             ; update IRQL
        MEND


        ;
        ; Macros to output special undefined opcodes that indicate breakpoints
        ; and debug services.
        ;

        MACRO
        EMIT_BREAKPOINT
        brk     #0xf000
        MEND


        MACRO
        EMIT_DEBUG_SERVICE
        brk     #0xf002
        MEND

        MACRO
        FASTFAIL $FastFailCode
        mov     x0, $FastFailCode
        brk     #0xf003
        MEND


        ;
        ; Macro to generate an exception frame; this is intended to
        ; be used within the prolog of a function.
        ;

        MACRO
        GENERATE_EXCEPTION_FRAME
        PROLOG_SAVE_REG_PAIR x19, x20, #-96!
        PROLOG_SAVE_REG_PAIR x21, x22, #16

        PROLOG_SAVE_REG_PAIR x23, x24, #32



        PROLOG_SAVE_REG_PAIR x25, x26, #48

        PROLOG_SAVE_REG_PAIR x27, x28, #64




        PROLOG_SAVE_REG_PAIR fp, lr, #80
        MEND


        ;
        ; Macro to restore from an exception frame; this is intended to
        ; be used within the epilog of a function.
        ;

        MACRO
        RESTORE_EXCEPTION_STATE
        EPILOG_RESTORE_REG_PAIR fp, lr, #80

        EPILOG_RESTORE_REG_PAIR x27, x28, #64



        EPILOG_RESTORE_REG_PAIR x25, x26, #48

        EPILOG_RESTORE_REG_PAIR x23, x24, #32

        EPILOG_RESTORE_REG_PAIR x21, x22, #16
        EPILOG_RESTORE_REG_PAIR x19, x20, #96!
        MEND


        ;
        ; Macro to ensure that any eret is followed by barriers to
        ; prevent speculation
        ;
        MACRO
        ERET_FIX
        eret
        dsb sy
        isb sy
        DCD 0xD50330FF ; sb
        MEND



        ;
        ; Given an address, obtains a pointer to the EC code bitmap from the
        ; specified global (typically located in the .mrdata section) and
        ; performs a bitmap lookup to determine if the address is EC code.
        ;
        ; Sets the Zero Flag for X64 targets, clears the Zero Flag for EC targets.
        ;
        ; xAddress      - On input, the code address to be tested.
        ;                 This value is preserved.
        ;
        ; T1            - On input, the address of the EC Bitmap.
        ;                 This register is then used as scratch.
        ;
        ; T2            - On input, the max user-land address.
        ;                 This register is then used as scratch.
        ;
        ; xResult       - Returns true(1) if the target is EC code
        ;                 and false(0) otherwise. This can be 'xzr'
        ;                 if a boolen result is not required. It can
        ;                 also overlap with xAddress, T1 or T2.
        ;
        ; SkipBoundsChecking - If set to "SkipBoundsChecking", no EC
        ;                      Bitmap bounds checks are performed (and
        ;                      T2 doesn't need to provide the max user-land
        ;                      address). T2 is still a scratch reg.
        ;
        ; Zero Flag     - Z=0 for EC code and Z=1 otherwise.
        ;

        MACRO
        EC_BITMAP_LOOKUP $xAddress, $T1, $T2, $xResult, $SkipBoundsChecking

        IF "$SkipBoundsChecking" != "SkipBoundsChecking"

        cmp     $xAddress, x$T2             ; Check if the address is above user space range
        bhi     %F1

        cmp     $xAddress, #(0x0000000000010000 / 4096), lsl #12 ; Check if address < 0x0000000000010000 (64KiB)
        blo     %F1                         ; if so, take the fast path

        ENDIF

        lsr     x$T2, $xAddress, #15        ; each byte of bitmap indexes 8*4K = 2^15 byte span
        ldrb    w$T2, [x$T1, x$T2]          ; load the bitmap byte for the 8*4K span
                                            ;
                                            ; * IF THIS INSTRUCTION EVER CHANGES, SO MUST
                                            ; KiOpPreprocessAccessViolation *
                                            ;
        ubfx    x$T1, $xAddress, #12, #3    ; index to the 4K page within the 8*4K span
        lsr     x$T1, x$T2, x$T1

        IF "$SkipBoundsChecking" != "SkipBoundsChecking"

        b       %F2
1
        mov     x$T1, xzr

        ENDIF

2
        ands    $xResult, x$T1, #1          ; test the specific page

        MEND

;

    

    
    
    
    
    
    
    





















































    LEAF_ENTRY SymCryptFdef369RawAddAsm

    ldp     x4, x6, [x0]         
    ldp     x5, x7, [x1]         
    adds    x4, x4, x5
    adcs    x6, x6, x7
    stp     x4, x6, [x2]         

    ldr     x4, [x0, #16]         
    sub     x3, x3, #1            
    ldr     x5, [x1, #16]         
    adcs    x4, x4, x5
    str     x4, [x2, #16]         

    cbz     x3, SymCryptFdef369RawAddAsmEnd

SymCryptFdef369RawAddAsmLoop
    
    
    ldp     x4, x6, [x0, #24]!   
    ldp     x5, x7, [x1, #24]!   
    adcs    x4, x4, x5
    adcs    x6, x6, x7
    stp     x4, x6, [x2, #24]!   

    ldr     x4, [x0, #16]         
    sub     x3, x3, #1            
    ldr     x5, [x1, #16]         
    adcs    x4, x4, x5
    str     x4, [x2, #16]         

    cbnz    x3, SymCryptFdef369RawAddAsmLoop

    align 4
SymCryptFdef369RawAddAsmEnd
    cset    x0, cs                 

    ret
    LEAF_END SymCryptFdef369RawAddAsm









































    LEAF_ENTRY SymCryptFdef369RawSubAsm

    ldp     x4, x6, [x0]         
    ldp     x5, x7, [x1]         
    subs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2]         

    ldr     x4, [x0, #16]         
    sub     x3, x3, #1            
    ldr     x5, [x1, #16]         
    sbcs    x4, x4, x5
    str     x4, [x2, #16]         

    cbz     x3, SymCryptFdef369RawSubAsmEnd

SymCryptFdef369RawSubAsmLoop
    
    
    ldp     x4, x6, [x0, #24]!   
    ldp     x5, x7, [x1, #24]!   
    sbcs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2, #24]!   

    ldr     x4, [x0, #16]         
    sub     x3, x3, #1            
    ldr     x5, [x1, #16]         
    sbcs    x4, x4, x5
    str     x4, [x2, #16]         

    cbnz    x3, SymCryptFdef369RawSubAsmLoop

    align 4
SymCryptFdef369RawSubAsmEnd
    cset    x0, cc                 

    ret
    LEAF_END SymCryptFdef369RawSubAsm







































    LEAF_ENTRY SymCryptFdef369MaskedCopyAsm

    subs    xzr, xzr, x3           

    ldp     x3, x5, [x0]         
    ldp     x4, x6, [x1]         
    csel    x3, x3, x4, cc       
    csel    x5, x5, x6, cc
    stp     x3, x5, [x1]         

    ldr     x3, [x0, #16]         
    sub     x2, x2, #1            
    ldr     x4, [x1, #16]         
    csel    x3, x3, x4, cc
    str     x3, [x1, #16]         

    cbz     x2, SymCryptFdef369MaskedCopyAsmEnd

SymCryptFdef369MaskedCopyAsmLoop
    ldp     x3, x5, [x0, #24]!   
    ldp     x4, x6, [x1, #24]!   
    csel    x3, x3, x4, cc       
    csel    x5, x5, x6, cc
    stp     x3, x5, [x1]         

    ldr     x3, [x0, #16]         
    sub     x2, x2, #1            
    ldr     x4, [x1, #16]         
    csel    x3, x3, x4, cc
    str     x3, [x1, #16]         

    cbnz    x2, SymCryptFdef369MaskedCopyAsmLoop

SymCryptFdef369MaskedCopyAsmEnd
    

    ret
    LEAF_END SymCryptFdef369MaskedCopyAsm











































































    LEAF_ENTRY SymCryptFdef369RawMulAsm

    add     x1, x1, x1, LSL #1       

    sub     x2, x2, #24               
    sub     x4, x4, #24               

    mov     x5, x4                    
    mov     x13, x2                   
    mov     x14, x3                   

    
    
    
    ands    x12, x12, xzr             
    ldr     x6, [x0]                  

SymCryptFdef369RawMulAsmLoopInner1
    sub     x3, x3, #1                

    ldp     x7, x8, [x2, #24]!       

    mul     x11, x6, x7              
    adcs    x11, x11, x12            
    umulh   x12, x6, x7              

    mul     x15, x6, x8              
    adcs    x15, x15, x12            
    umulh   x12, x6, x8              

    stp     x11, x15, [x4, #24]!     
    ldr     x7, [x2, #16]             

    mul     x11, x6, x7              
    adcs    x11, x11, x12            
    umulh   x12, x6, x7              

    str     x11, [x4, #16]            

    cbnz    x3, SymCryptFdef369RawMulAsmLoopInner1

    adc     x12, x12, xzr             
    str     x12, [x4, #24]

    sub     x1, x1, #1                
    add     x0, x0, #8                
    add     x5, x5, #8                

    
    
    
SymCryptFdef369RawMulAsmLoopOuter
    mov     x3, x14                   
    mov     x2, x13                   
    mov     x4, x5                    

    ands    x12, x12, xzr             
    ldr     x6, [x0]                  

SymCryptFdef369RawMulAsmLoopInner
    sub     x3, x3, #1                

    ldp     x7, x8, [x2, #24]!       
    ldp     x9, x10, [x4, #24]!      

    adcs    x9, x9, x12              
    umulh   x11, x6, x7              
    adcs    x10, x11, x10            
    umulh   x12, x6, x8              
    adc     x12, x12, xzr             

    mul     x11, x6, x7              
    adds    x9, x9, x11              
    mul     x11, x6, x8              
    adcs    x10, x10, x11            

    stp     x9, x10, [x4]            

    ldr     x7, [x2, #16]             
    ldr     x9, [x4, #16]             

    adcs    x9, x9, x12              
    umulh   x12, x6, x7              
    adc     x12, x12, xzr             

    mul     x11, x6, x7              
    adds    x9, x9, x11              

    str     x9, [x4, #16]             

    cbnz    x3, SymCryptFdef369RawMulAsmLoopInner

    adc     x12, x12, xzr             
    str     x12, [x4, #24]

    subs    x1, x1, #1                
    add     x0, x0, #8                
    add     x5, x5, #8                

    bne     SymCryptFdef369RawMulAsmLoopOuter

    

    ret
    LEAF_END SymCryptFdef369RawMulAsm































































































    LEAF_ENTRY SymCryptFdef369MontgomeryReduceAsm

    ldr     w3, [x0, #SymCryptModulusNdigitsOffsetArm64]  
    ldr     x5, [x0, #SymCryptModulusInv64OffsetArm64]    
    add     x0, x0, #SymCryptModulusValueOffsetArm64      

    add     x4, x3, x3, LSL #1       

    sub     x0, x0, #24               
    sub     x1, x1, #24               
    sub     x2, x2, #24               

    mov     x15, x3                   
    mov     x16, x0                   
    mov     x17, x1                   

    mov     x7, xzr                    

    
    
    
SymCryptFdef369MontgomeryReduceAsmOuter
    ldr     x8, [x1, #24]             
    mul     x6, x8, x5               

    mov     x12, xzr                   

SymCryptFdef369MontgomeryReduceAsmInner
    ldp     x10, x11, [x0, #24]!     
    ldp     x8, x9, [x1, #24]!       

    mul     x14, x6, x10             
    adds    x14, x14, x8             
    umulh   x13, x6, x10             
    adc     x13, x13, xzr             
    adds    x12, x12, x14            
    adc     x13, x13, xzr             
    
    
    str     x12, [x1]                 
    mov     x12, x13                  

    mul     x14, x6, x11             
    adds    x14, x14, x9             
    umulh   x13, x6, x11             
    adc     x13, x13, xzr             
    adds    x12, x12, x14            
    adc     x13, x13, xzr             
    str     x12, [x1, #8]             
    mov     x12, x13                  

    ldr     x10, [x0, #16]            
    ldr     x8, [x1, #16]             

    mul     x14, x6, x10             
    adds    x14, x14, x8             
    umulh   x13, x6, x10             
    adc     x13, x13, xzr             
    adds    x12, x12, x14            
    adc     x13, x13, xzr             
    str     x12, [x1, #16]            
    mov     x12, x13                  

    subs    x3, x3, #1                
    bne     SymCryptFdef369MontgomeryReduceAsmInner

    ldr     x8, [x1, #24]             
    adds    x12, x12, x8             
    adc     x13, xzr, xzr              

    adds    x12, x12, x7             
    adc     x7, x13, xzr              

    str     x12, [x1, #24]            

    subs    x4, x4, #1                

    add     x17, x17, #8              
    mov     x0, x16                   
    mov     x1, x17                   

    mov     x3, x15                   

    bne     SymCryptFdef369MontgomeryReduceAsmOuter

    
    
    

    mov     x14, x2               

    
    mov     x0, x17               
    mov     x1, x16               

    mov     x10, x7               
    mov     x3, x15               
    subs    x4, x4, x4           

SymCryptFdef369MontgomeryReduceRawSubAsmLoop
    sub     x3, x3, #1            
    

    ldp     x4, x6, [x0, #24]!   
    ldp     x5, x7, [x1, #24]!   
    sbcs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2, #24]!   

    ldr     x4, [x0, #16]         
    ldr     x5, [x1, #16]         
    sbcs    x4, x4, x5
    str     x4, [x2, #16]         

    cbnz    x3, SymCryptFdef369MontgomeryReduceRawSubAsmLoop

    cset    x0, cc                 

    orr     x11, x10, x0         

    
    mov     x0, x17               
    mov     x1, x14               

    mov     x2, x15               
    subs    x4, x10, x11         

SymCryptFdef369MontgomeryReduceMaskedCopyAsmLoop
    sub     x2, x2, #1            

    ldp     x4, x6, [x0, #24]!   
    ldp     x5, x7, [x1, #24]!   
    csel    x4, x4, x5, cc       
    csel    x6, x6, x7, cc
    stp     x4, x6, [x1]         

    ldr     x4, [x0, #16]         
    ldr     x5, [x1, #16]         
    csel    x4, x4, x5, cc       
    str     x4, [x1, #16]         

    cbnz    x2, SymCryptFdef369MontgomeryReduceMaskedCopyAsmLoop

    

    ret
    LEAF_END SymCryptFdef369MontgomeryReduceAsm





































    END
