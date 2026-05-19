

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

    

    
    
    
    
    
    
    





















































    LEAF_ENTRY SymCryptFdefRawAddAsm

    ldp     x4, x6, [x0]         
    ldp     x5, x7, [x1]         
    adds    x4, x4, x5
    adcs    x6, x6, x7
    stp     x4, x6, [x2]         

    ldp     x4, x6, [x0, #16]    
    sub     x3, x3, #1            
    ldp     x5, x7, [x1, #16]    
    adcs    x4, x4, x5
    adcs    x6, x6, x7
    stp     x4, x6, [x2, #16]    

    cbz     x3, SymCryptFdefRawAddAsmEnd

SymCryptFdefRawAddAsmLoop
    
    
    ldp     x4, x6, [x0, #32]!   
    ldp     x5, x7, [x1, #32]!   
    adcs    x4, x4, x5
    adcs    x6, x6, x7
    stp     x4, x6, [x2, #32]!   

    ldp     x4, x6, [x0, #16]    
    sub     x3, x3, #1            
    ldp     x5, x7, [x1, #16]    
    adcs    x4, x4, x5
    adcs    x6, x6, x7
    stp     x4, x6, [x2, #16]    

    cbnz    x3, SymCryptFdefRawAddAsmLoop

    align 4
SymCryptFdefRawAddAsmEnd
    cset    x0, cs                 

    ret
    LEAF_END SymCryptFdefRawAddAsm









































    LEAF_ENTRY SymCryptFdefRawSubAsm

    ldp     x4, x6, [x0]         
    ldp     x5, x7, [x1]         
    subs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2]         

    ldp     x4, x6, [x0, #16]    
    sub     x3, x3, #1            
    ldp     x5, x7, [x1, #16]    
    sbcs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2, #16]    

    cbz     x3, SymCryptFdefRawSubAsmEnd

SymCryptFdefRawSubAsmLoop
    
    
    ldp     x4, x6, [x0, #32]!   
    ldp     x5, x7, [x1, #32]!   
    sbcs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2, #32]!   

    ldp     x4, x6, [x0, #16]    
    sub     x3, x3, #1            
    ldp     x5, x7, [x1, #16]    
    sbcs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2, #16]    

    cbnz    x3, SymCryptFdefRawSubAsmLoop

    align 4
SymCryptFdefRawSubAsmEnd
    cset    x0, cc                 

    ret
    LEAF_END SymCryptFdefRawSubAsm




    LEAF_ENTRY SymCryptFdefMaskedCopyAsm

    dup     v0.4s, w3              

SymCryptFdefMaskedCopyAsmLoop
    ldp     q1, q3, [x0], #32      
    ldp     q2, q4, [x1]           
    bit     v2.16b, v1.16b, v0.16b  
    bit     v4.16b, v3.16b, v0.16b  
    stp     q2, q4, [x1], #32      

    sub     x2, x2, #1            

    cbnz    x2, SymCryptFdefMaskedCopyAsmLoop

    

    ret
    LEAF_END SymCryptFdefMaskedCopyAsm



    LEAF_ENTRY SymCryptFdefRawMulAsm

    lsl     x1, x1, #2                

    sub     x2, x2, #32               
    sub     x4, x4, #32               

    mov     x5, x4                    
    mov     x13, x2                   
    mov     x14, x3                   

    
    
    
    ands    x12, x12, xzr             
    ldr     x6, [x0]                  

SymCryptFdefRawMulAsmLoopInner1
    sub     x3, x3, #1                

    ldp     x7, x8, [x2, #32]!       

    mul     x11, x6, x7              
    adcs    x11, x11, x12            
    umulh   x12, x6, x7              

    mul     x15, x6, x8              
    adcs    x15, x15, x12            
    umulh   x12, x6, x8              

    stp     x11, x15, [x4, #32]!     
    ldp     x7, x8, [x2, #16]        

    mul     x11, x6, x7              
    adcs    x11, x11, x12            
    umulh   x12, x6, x7              

    mul     x15, x6, x8              
    adcs    x15, x15, x12            
    umulh   x12, x6, x8              

    stp     x11, x15, [x4, #16]      

    cbnz    x3, SymCryptFdefRawMulAsmLoopInner1

    adc     x12, x12, xzr             
    str     x12, [x4, #32]

    sub     x1, x1, #1                
    add     x0, x0, #8                
    add     x5, x5, #8                

    
    
    
SymCryptFdefRawMulAsmLoopOuter
    mov     x3, x14                   
    mov     x2, x13                   
    mov     x4, x5                    

    ands    x12, x12, xzr             
    ldr     x6, [x0]                  

SymCryptFdefRawMulAsmLoopInner
    sub     x3, x3, #1                

    ldp     x7, x8, [x2, #32]!       
    ldp     x9, x10, [x4, #32]!      

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

    ldp     x7, x8, [x2, #16]        
    ldp     x9, x10, [x4, #16]       

    adcs    x9, x9, x12              
    umulh   x11, x6, x7              
    adcs    x10, x11, x10            
    umulh   x12, x6, x8              
    adc     x12, x12, xzr             

    mul     x11, x6, x7              
    adds    x9, x9, x11              
    mul     x11, x6, x8              
    adcs    x10, x10, x11            

    stp     x9, x10, [x4, #16]       

    cbnz    x3, SymCryptFdefRawMulAsmLoopInner

    adc     x12, x12, xzr             
    str     x12, [x4, #32]

    subs    x1, x1, #1                
    add     x0, x0, #8                
    add     x5, x5, #8                

    bne     SymCryptFdefRawMulAsmLoopOuter

    

    ret
    LEAF_END SymCryptFdefRawMulAsm



    
    
    
    MACRO
    SQR_SINGLEADD_64 $index, $src_reg, $dst_reg, $mul_word, $src_carry, $dst_carry, $scratch0, $scratch1
    ldr     $scratch0, [$src_reg, #8*$index]   

    mul     $scratch1, $mul_word, $scratch0    
    adds    $scratch1, $scratch1, $src_carry   
    umulh   $dst_carry, $mul_word, $scratch0   
    adc     $dst_carry, $dst_carry, xzr       

    str     $scratch1, [$dst_reg, #8*$index]   

    MEND

    
    
    
    
    
    
    MACRO
    SQR_DOUBLEADD_64 $index, $src_reg, $dst_reg, $mul_word, $src_carry, $dst_carry, $scratch0, $scratch1, $scratch2
    ldr     $scratch0, [$src_reg, #8*$index]   
    ldr     $scratch2, [$dst_reg, #8*$index]   

    mul     $scratch1, $mul_word, $scratch0    
    umulh   $dst_carry, $mul_word, $scratch0   

    adds    $scratch1, $scratch1, $scratch2    
    adc     $dst_carry, $dst_carry, xzr       

    adds    $scratch1, $scratch1, $src_carry   
    adc     $dst_carry, $dst_carry, xzr       

    str     $scratch1, [$dst_reg, #8*$index]   

    MEND

    
    
    
    
    
    
    
    MACRO
    SQR_DIAGONAL_PROP $index, $src_reg, $dst_reg, $squarelo, $squarehi, $scratch0, $scratch1
    ldr     $squarehi, [$src_reg, #8*$index]               
    mul     $squarelo, $squarehi, $squarehi                
    umulh   $squarehi, $squarehi, $squarehi                

    ldp     $scratch0, $scratch1, [$dst_reg, #16*$index]    

    
    adcs    $squarelo, $squarelo, $scratch0                

    
    adcs    $squarehi, $squarehi, $scratch1                

    stp     $squarelo, $squarehi, [$dst_reg, #16*$index]    

    MEND




    LEAF_ENTRY SymCryptFdefRawSquareAsm

    mov     x3, x1                    

    lsl     x1, x1, #2                

    mov     x4, x2                    
    mov     x5, x2                    
    mov     x16, x2                   
    mov     x13, x0                   
    mov     x2, x0                    
    mov     x14, x3                   
    mov     x15, x3                   

    
    
    
    mov     x12, xzr                   
    ldr     x6, [x0]                  
    str     xzr, [x4]                  

    b       SymCryptFdefRawSquareAsmInnerLoopInit_Word1

SymCryptFdefRawSquareAsmInnerLoopInit_Word0
    SQR_SINGLEADD_64 0, x2, x4, x6, x12, x12, x7, x8

SymCryptFdefRawSquareAsmInnerLoopInit_Word1
    SQR_SINGLEADD_64 1, x2, x4, x6, x12, x12, x7, x8

    SQR_SINGLEADD_64 2, x2, x4, x6, x12, x12, x7, x8

    SQR_SINGLEADD_64 3, x2, x4, x6, x12, x12, x7, x8

    sub     x3, x3, #1                
    add     x2, x2, #32
    add     x4, x4, #32

    cbnz    x3, SymCryptFdefRawSquareAsmInnerLoopInit_Word0

    str     x12, [x4]                 

    sub     x1, x1, #2                

    mov     x8, #1                     

    
    
    
SymCryptFdefRawSquareAsmOuterLoop

    add     x5, x5, #8                

    mov     x3, x14                   
    mov     x2, x0                    
    mov     x4, x5                    

    mov     x11, xzr                   
    mov     x12, xzr                   
    ldr     x6, [x0, x8, LSL #3]     

    
    add     x8, x8, #1
    cmp     x8, #1
    beq     SymCryptFdefRawSquareAsmInnerLoop_Word1
    cmp     x8, #2
    beq     SymCryptFdefRawSquareAsmInnerLoop_Word2
    cmp     x8, #3
    beq     SymCryptFdefRawSquareAsmInnerLoop_Word3

    
    mov     x8, xzr                

    add     x0, x0, #32           
    add     x5, x5, #32           

    mov     x2, x0                
    mov     x4, x5                

    sub     x14, x14, #1          
    mov     x3, x14               

SymCryptFdefRawSquareAsmInnerLoop_Word0
    SQR_DOUBLEADD_64 0, x2, x4, x6, x12, x11, x7, x9, x10

SymCryptFdefRawSquareAsmInnerLoop_Word1
    SQR_DOUBLEADD_64 1, x2, x4, x6, x11, x12, x7, x9, x10

SymCryptFdefRawSquareAsmInnerLoop_Word2
    SQR_DOUBLEADD_64 2, x2, x4, x6, x12, x11, x7, x9, x10

SymCryptFdefRawSquareAsmInnerLoop_Word3
    SQR_DOUBLEADD_64 3, x2, x4, x6, x11, x12, x7, x9, x10

    sub     x3, x3, #1                
    add     x2, x2, #32
    add     x4, x4, #32

    cbnz    x3, SymCryptFdefRawSquareAsmInnerLoop_Word0

    str     x12, [x4]                 

    sub     x1, x1, #1                
    cbnz    x1, SymCryptFdefRawSquareAsmOuterLoop

    str     xzr, [x5, #40]             

    
    
    

    mov     x3, x15       
    lsl     x3, x3, #1    
    mov     x4, x16       
    ands    x7, x7, xzr   

SymCryptFdefRawSquareAsmSecondPass

    sub     x3, x3, #1    

    ldp     x7, x8, [x4]
    adcs    x7, x7, x7   
    adcs    x8, x8, x8
    stp     x7, x8, [x4], #16

    ldp     x9, x10, [x4]
    adcs    x9, x9, x9   
    adcs    x10, x10, x10
    stp     x9, x10, [x4], #16

    cbnz    x3, SymCryptFdefRawSquareAsmSecondPass

    
    
    

    ands    x7, x7, xzr   
    mov     x0, x13       
    mov     x4, x16       
    mov     x3, x15       

SymCryptFdefRawSquareAsmThirdPass
    SQR_DIAGONAL_PROP 0, x0, x4, x6, x7, x8, x9
    SQR_DIAGONAL_PROP 1, x0, x4, x6, x7, x8, x9
    SQR_DIAGONAL_PROP 2, x0, x4, x6, x7, x8, x9
    SQR_DIAGONAL_PROP 3, x0, x4, x6, x7, x8, x9

    sub     x3, x3, #1        
    add     x0, x0, #32       
    add     x4, x4, #64       

    cbnz    x3, SymCryptFdefRawSquareAsmThirdPass

    

    ret
    LEAF_END SymCryptFdefRawSquareAsm







    LEAF_ENTRY SymCryptFdefModAdd256Asm

    add     x0, x0, #SymCryptModulusValueOffsetArm64 

    
    ldp     x4, x5, [x1]       
    ldp     x8, x9, [x2]       
    adds    x4, x4, x8
    adcs    x5, x5, x9

    ldp     x6, x7, [x1, #16]  
    ldp     x8, x9, [x2, #16]  
    adcs    x6, x6, x8
    adcs    x7, x7, x9

    cset    x1, cs                 

    

    ldp     x2, x8, [x0]       
    subs    x2, x4, x2
    sbcs    x8, x5, x8

    ldp     x9, x0, [x0, #16]  
    sbcs    x9, x6, x9
    sbcs    x0, x7, x0

    
    
    
    

    
    
    sbcs    x1, x1, XZR

    csel    x4, x4, x2, cc
    csel    x5, x5, x8, cc
    stp     x4, x5, [x3]

    csel    x6, x6, x9, cc
    csel    x7, x7, x0, cc
    stp     x6, x7, [x3, #16]

    ret
    LEAF_END SymCryptFdefModAdd256Asm





    LEAF_ENTRY SymCryptFdefModSub256Asm

    add     x0, x0, #SymCryptModulusValueOffsetArm64 

    
    ldp     x4, x5, [x1]         
    ldp     x8, x9, [x2]         
    subs    x4, x4, x8
    sbcs    x5, x5, x9

    ldp     x6, x7, [x1, #16]    
    ldp     x8, x9, [x2, #16]    
    sbcs    x6, x6, x8
    sbcs    x7, x7, x9

    
    ldp     x1, x2, [x0]         
    csel    x1, x1, XZR, cc       
    csel    x2, x2, XZR, cc

    ldp     x8, x9, [x0, #16]    
    csel    x8, x8, XZR, cc
    csel    x9, x9, XZR, cc

    
    adds    x4, x4, x1
    adcs    x5, x5, x2
    stp     x4, x5, [x3]

    adcs    x6, x6, x8
    adc     x7, x7, x9
    stp     x6, x7, [x3, #16]

    ret
    LEAF_END SymCryptFdefModSub256Asm






















    MACRO
    MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE $T0, $T1, $Ai, $pB, $pM, $K, $Inv64, $R0, $R1, $R2, $R3, $R4, $R5    
    
    
    
    
    
    
    
    

    
    ldr     $T0, [$pB]
    mul     $T1, $Ai, $T0
    adds    $R1, $R1, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R2, $R2, $T0

    ldr     $T0, [$pB, #16]
    mul     $T1, $Ai, $T0
    adcs    $R3, $R3, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R4, $R4, $T0

    adc     $R5, $R5, XZR 

    
    ldr     $T0, [$pB, #8]
    mul     $T1, $Ai, $T0
    adds    $R2, $R2, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R3, $R3, $T0

    ldr     $T0, [$pB, #24]
    mul     $T1, $Ai, $T0
    adcs    $R4, $R4, $T1
    umulh   $Ai, $Ai, $T0
    adc     $Ai, $Ai, XZR 
                        

    
    ldr     $T0, [$pM]
    mul     $T1,  $K, $T0
    adds    $R0, $R0, $T1  
    umulh   $T0,  $K, $T0
    adcs    $R1, $R1, $T0

    ldr     $T0, [$pM, #16]
    mul     $T1,  $K, $T0
    adcs    $R2, $R2, $T1
    umulh   $T0,  $K, $T0
    adcs    $R3, $R3, $T0

    adcs    $R4, $R4, XZR
    adc     $R5, $R5, XZR 

    
    ldr     $T0, [$pM, #8]
    mul     $T1,  $K, $T0
    adds    $R1, $R1, $T1
    umulh   $T0,  $K, $T0
    adcs    $R2, $R2, $T0

    ldr     $T0, [$pM, #24]
    mul     $T1,  $K, $T0
    adcs    $R3, $R3, $T1
    umulh   $T0,  $K, $T0
    adcs    $R4, $R4, $T0

    mul     $K, $R1, $Inv64
    adcs    $R5, $R5, $Ai
    cset    $R0, cs
    MEND


    LEAF_ENTRY SymCryptFdefModMulMontgomery256Asm

    
    ALTERNATE_ENTRY SymCryptFdefModMulMontgomery256AsmInternal

    ldr     x4, [x1]              

    ldp     x10, x11, [x2]       
    mul      x9,  x4, x10        
    umulh   x10,  x4, x10        

    mul      x5,  x4, x11        
    adds    x10, x10,  x5        
    umulh   x11,  x4, x11        

    ldp     x12, x13, [x2, #16]  
    mul      x5,  x4, x12        
    adcs    x11, x11,  x5        
    umulh   x12,  x4, x12        

    mul      x5,  x4, x13        
    adcs    x12, x12,  x5        
    umulh   x13,  x4, x13        
    adc     x13, x13,  XZR

    eor     x14, x14, x14

    ldr     x7, [x0, #SymCryptModulusInv64OffsetArm64]
    mul     x8, x9, x7           

    add     x0, x0, #SymCryptModulusValueOffsetArm64

    ldr     x4, [x1, #8]
    MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE x5, x6, x4, x2, x0, x8, x7, x9, x10, x11, x12, x13, x14

    ldr     x4, [x1, #16]
    MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE x5, x6, x4, x2, x0, x8, x7, x10, x11, x12, x13, x14, x9

    ldr     x4, [x1, #24]
    MUL_AND_MONTGOMERY_REDUCE14_INTERLEAVE x5, x6, x4, x2, x0, x8, x7, x11, x12, x13, x14, x9, x10

    

    ldp      x4,  x5, [x0]   
    mul      x1,  x8, x4
    adds    x12, x12, x1     
    umulh    x2,  x8, x4
    adcs    x13, x13, x2

    ldp      x6,  x7, [x0, #16]
    mul      x1,  x8, x6
    adcs    x14, x14, x1
    umulh    x2,  x8, x6
    adcs     x9,  x9, x2

    adcs    x10, x10, XZR
    adc     x11, x11, XZR

    
    mul      x1,  x8, x5
    adds    x13, x13, x1
    umulh    x2,  x8, x5
    adcs    x14, x14, x2

    mul      x1,  x8, x7
    adcs     x9,  x9, x1
    umulh    x2,  x8, x7
    adcs    x10, x10, x2

    adc     x11, x11, XZR

    
    

    subs    x4, x13, x4
    sbcs    x5, x14, x5
    sbcs    x6,  x9, x6
    sbcs    x7, x10, x7

    
    
    
    

    
    
    sbcs    x11, x11, XZR

    csel    x13, x13, x4, cc
    csel    x14, x14, x5, cc
    stp     x13, x14, [x3]

    csel     x9,  x9, x6, cc
    csel    x10, x10, x7, cc
    stp      x9, x10, [x3, #16]

    ret
    LEAF_END SymCryptFdefModMulMontgomery256Asm






    LEAF_ENTRY SymCryptFdefModSquareMontgomery256Asm

    
    
    

    mov     x3, x2
    mov     x2, x1

    

    b       SymCryptFdefModMulMontgomery256AsmInternal

    ret
    LEAF_END SymCryptFdefModSquareMontgomery256Asm



    LEAF_ENTRY SymCryptFdefModAdd384Asm

    add     x0, x0, #SymCryptModulusValueOffsetArm64 

    
    ldp     x4,  x5,  [x1]       
    ldp     x10, x11, [x2]       
    adds    x4,  x4,  x10
    adcs    x5,  x5,  x11

    ldp     x6,  x7,  [x1, #16]  
    ldp     x10, x11, [x2, #16]  
    adcs    x6,  x6,  x10
    adcs    x7,  x7,  x11

    ldp     x8,  x9,  [x1, #32]  
    ldp     x10, x11, [x2, #32]  
    adcs    x8,  x8,  x10
    adcs    x9,  x9,  x11

    cset    x1, cs                 

    

    ldp     x2, x10,  [x0]       
    subs    x2,  x4,  x2
    sbcs    x10, x5,  x10

    ldp     x11, x12, [x0, #16]  
    sbcs    x11, x6,  x11
    sbcs    x12, x7,  x12

    ldp     x13, x0,  [x0, #32]  
    sbcs    x13, x8,  x13
    sbcs    x0,  x9,  x0

    
    
    
    sbcs    x1, x1, XZR

    csel    x4, x4, x2,  cc
    csel    x5, x5, x10, cc
    stp     x4, x5, [x3]

    csel    x6, x6, x11, cc
    csel    x7, x7, x12, cc
    stp     x6, x7, [x3, #16]

    csel    x8, x8, x13, cc
    csel    x9, x9, x0,  cc
    stp     x8, x9, [x3, #32]

    ret
    LEAF_END SymCryptFdefModAdd384Asm





    LEAF_ENTRY SymCryptFdefModSub384Asm

    add     x0, x0, #SymCryptModulusValueOffsetArm64 

    
    ldp     x4,  x5,  [x1]         
    ldp     x10, x11, [x2]         
    subs    x4,  x4,  x10
    sbcs    x5,  x5,  x11

    ldp     x6,  x7,  [x1, #16]    
    ldp     x10, x11, [x2, #16]    
    sbcs    x6,  x6,  x10
    sbcs    x7,  x7,  x11

    ldp     x8,  x9,  [x1, #32]    
    ldp     x10, x11, [x2, #32]    
    sbcs    x8,  x8,  x10
    sbcs    x9,  x9,  x11

    
    ldp     x1, x2, [x0]         
    csel    x1, x1, XZR, cc       
    csel    x2, x2, XZR, cc

    ldp     x10, x11, [x0, #16]  
    csel    x10, x10, XZR, cc
    csel    x11, x11, XZR, cc

    ldp     x12, x0,  [x0, #32]  
    csel    x12, x12, XZR, cc
    csel    x0,  x0,  XZR, cc

    
    adds    x4, x4, x1
    adcs    x5, x5, x2
    stp     x4, x5, [x3]

    adcs    x6, x6, x10
    adcs    x7, x7, x11
    stp     x6, x7, [x3, #16]

    adcs    x8, x8, x12
    adc     x9, x9, x0
    stp     x8, x9, [x3, #32]

    ret
    LEAF_END SymCryptFdefModSub384Asm




























    MACRO
    MUL_AND_MONTGOMERY_REDUCE16_P384_INTERLEAVE $T0, $T1, $Ai, $pB, $K, $N3, $R0, $R1, $R2, $R3, $R4, $R5, $R6, $R7    
    
    
    
    
    
    
    

    
    ldr     $T0, [$pB]
    mul     $T1, $Ai, $T0
    adds    $R1, $R1, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R2, $R2, $T0

    ldr     $T0, [$pB, #16]
    mul     $T1, $Ai, $T0
    adcs    $R3, $R3, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R4, $R4, $T0

    ldr     $T0, [$pB, #32]
    mul     $T1, $Ai, $T0
    adcs    $R5, $R5, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R6, $R6, $T0

    adc     $R7, $R7, XZR 

    
    ldr     $T0, [$pB, #8]
    mul     $T1, $Ai, $T0
    adds    $R2, $R2, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R3, $R3, $T0

    ldr     $T0, [$pB, #24]
    mul     $T1, $Ai, $T0
    adcs    $R4, $R4, $T1
    umulh   $T0, $Ai, $T0
    adcs    $R5, $R5, $T0

    ldr     $T0, [$pB, #40]
    mul     $T1, $Ai, $T0
    adcs    $R6, $R6, $T1
    umulh   $Ai, $Ai, $T0
    adc     $Ai, $Ai, XZR 
                        
    

    lsl     $T0, $R0, #32     
    adds     $K, $R0, $T0      
    lsr     $T1, $K, #32      
    adcs    $R1, $R1, $T1      
    add     $T1, $T1, $N3      
    adcs    $R2, $R2, XZR
    csetm   $N3, cs          

    subs    $R1, $R1, $T0      
    sbcs    $R2, $R2, $T1
    cinc    $N3, $N3, cc      

    subs    $R2, $R2, $K       
    cinc    $N3, $N3, cc      

    adds    $R6, $R6, $K       
    adcs    $R7, $R7, $Ai
    cset    $R0, cs
    MEND



    LEAF_ENTRY SymCryptFdefModMulMontgomeryP384Asm



    ALTERNATE_ENTRY SymCryptFdefModMulMontgomeryP384AsmInternal

    ldr     x4, [x1]              

    ldp     x10, x11, [x2]       
    mul      x9,  x4, x10        
    umulh   x10,  x4, x10        

    mul      x5,  x4, x11        
    adds    x10, x10,  x5        
    umulh   x11,  x4, x11        

    ldp     x12, x13, [x2, #16]  
    mul      x5,  x4, x12        
    adcs    x11, x11,  x5        
    umulh   x12,  x4, x12        

    mul      x5,  x4, x13        
    adcs    x12, x12,  x5        
    umulh   x13,  x4, x13        

    ldp     x14, x15, [x2, #32]  
    mul      x5,  x4, x14        
    adcs    x13, x13,  x5        
    umulh   x14,  x4, x14        

    mul      x5,  x4, x15        
    adcs    x14, x14,  x5        
    umulh   x15,  x4, x15        
    adc     x15, x15,  XZR

    eor     x16, x16, x16
    eor     x7, x7, x7

    ldr     x4, [x1, #8]
    MUL_AND_MONTGOMERY_REDUCE16_P384_INTERLEAVE x5, x6, x4, x2, x8, x7, x9, x10, x11, x12, x13, x14, x15, x16

    ldr     x4, [x1, #16]
    MUL_AND_MONTGOMERY_REDUCE16_P384_INTERLEAVE x5, x6, x4, x2, x8, x7, x10, x11, x12, x13, x14, x15, x16, x9

    ldr     x4, [x1, #24]
    MUL_AND_MONTGOMERY_REDUCE16_P384_INTERLEAVE x5, x6, x4, x2, x8, x7, x11, x12, x13, x14, x15, x16, x9, x10

    ldr     x4, [x1, #32]
    MUL_AND_MONTGOMERY_REDUCE16_P384_INTERLEAVE x5, x6, x4, x2, x8, x7, x12, x13, x14, x15, x16, x9, x10, x11

    ldr     x4, [x1, #40]
    MUL_AND_MONTGOMERY_REDUCE16_P384_INTERLEAVE x5, x6, x4, x2, x8, x7, x13, x14, x15, x16, x9, x10, x11, x12

    
    lsl     x5, x14, #32  
    adds    x8, x14, x5  
    lsr     x6, x8, #32   
    adcs    x15, x15, x6 
    add     x6, x6, x7   
    adcs    x16, x16, XZR
    csetm   x7, cs         

    subs    x15, x15, x5 
    sbcs    x16, x16, x6
    cinc    x7, x7, cc    

    subs    x16, x16, x8 
    sbcs    x9, x9, x7   
    sbcs    x10, x10, XZR
    sbcs    x11, x11, XZR
    sbcs    x12, x12, XZR
    sbc     x13, x13, XZR

    adds    x12, x12, x8 
    adc     x13, x13, XZR

    
    

    
    mov     x4, 0x00000000ffffffff
    mov     x5, 0xffffffff00000000
    mov     x6, 0xfffffffffffffffe
    mov     x0, 0xffffffffffffffff

    subs     x4, x15,  x4
    sbcs     x5, x16,  x5
    sbcs     x6,  x9,  x6
    sbcs     x7, x10,  x0
    sbcs    x14, x11,  x0
    sbcs     x0, x12,  x0

    
    
    
    

    
    
    sbcs    x13, x13, XZR

    csel    x15, x15,  x4, cc
    csel    x16, x16,  x5, cc
    stp     x15, x16, [x3]

    csel     x9,  x9,  x6, cc
    csel    x10, x10,  x7, cc
    stp      x9, x10, [x3, #16]

    csel    x11, x11, x14, cc
    csel    x12, x12,  x0, cc
    stp     x11, x12, [x3, #32]

    ret
    LEAF_END SymCryptFdefModMulMontgomeryP384Asm




    LEAF_ENTRY SymCryptFdefModSquareMontgomeryP384Asm


    mov     x3, x2
    mov     x2, x1

    

    b       SymCryptFdefModMulMontgomeryP384AsmInternal

    ret
    LEAF_END SymCryptFdefModSquareMontgomeryP384Asm




    NESTED_ENTRY SymCryptFdefMontgomeryReduceAsm
    PROLOG_SAVE_REG_PAIR fp, lr, #-48! 
    PROLOG_SAVE_REG_PAIR x19, x20, #16
    PROLOG_SAVE_REG_PAIR x21, x22, #32

    ldr     w4, [x0, #SymCryptModulusNdigitsOffsetArm64]  
    ldr     x5, [x0, #SymCryptModulusInv64OffsetArm64]    
    add     x0, x0, #SymCryptModulusValueOffsetArm64      

    lsl     x3, x4, #5                
    lsl     x4, x4, #2                

    sub     x3, x3, #32               

    mov     x7, xzr                    
    mov     x22, x3                   

    
    
    
SymCryptFdefMontgomeryReduceAsmOuter
    ldp     x16, x17, [x1]           
    mul     x6, x16, x5              

    ldp     x8,  x9,  [x0]           
    umulh   x8,  x6, x8              
    subs    xzr,  x16, #1              

    mul     x13, x6, x9              
    umulh   x9,  x6, x9              
    adcs    x8,  x8,  x13            

    ldp     x10, x11, [x0, #16]      
    mul     x14, x6, x10             
    umulh   x10, x6, x10             
    adcs    x9,  x9,  x14            

    ldp     x19, x20, [x1, #16]      
    mul     x15, x6, x11             
    umulh   x11, x6, x11             
    adcs    x10, x10, x15            

    adc     x21, x11, xzr             

    
    

    adds    x17, x17, x8             
    adcs    x19, x19, x9             
    adcs    x20, x20, x10            

    stp     xzr,  x17, [x1]           
    stp     x19, x20, [x1, #16]      

    cbz    x3, SymCryptFdefMontgomeryReduceAsmInnerEnd

SymCryptFdefMontgomeryReduceAsmInner
    
    
    

    ldp     x8,  x9,  [x0, #32]!     
    mul     x12, x6, x8              
    umulh   x8,  x6, x8              
    adcs    x12, x21, x12            

    mul     x13, x6, x9              
    umulh   x9,  x6, x9              
    ldp     x16, x17, [x1, #32]!     
    adcs    x8,  x8,  x13            

    ldp     x10, x11, [x0, #16]      
    mul     x14, x6, x10             
    umulh   x10, x6, x10             
    adcs    x9,  x9,  x14            

    ldp     x19, x20, [x1, #16]      
    mul     x15, x6, x11             
    umulh   x11, x6, x11             
    adcs    x10, x10, x15            

    adc     x21, x11, xzr             

    

    adds    x16, x16, x12            
    adcs    x17, x17, x8             
    adcs    x19, x19, x9             
    adcs    x20, x20, x10            

    stp     x16, x17, [x1]           
    stp     x19, x20, [x1, #16]      

    sub     x3, x3, #32               
    cbnz    x3, SymCryptFdefMontgomeryReduceAsmInner

SymCryptFdefMontgomeryReduceAsmInnerEnd

    ldr     x8, [x1, #32]             
    adcs    x21, x21, x7             
    adc     x7, xzr, xzr               

    adds    x21, x21, x8             
    adc     x7, x7, xzr               

    str     x21, [x1, #32]            

    subs    x4, x4, #1                

    sub     x0, x0, x22              
    sub     x1, x1, x22              
    add     x1, x1, #8                

    mov     x3, x22                   

    bne     SymCryptFdefMontgomeryReduceAsmOuter

    
    
    
    add     x3, x3, #32           
    mov     x22, x3               

    sub     x0, x0, #32           
    sub     x1, x1, #32           
    sub     x2, x2, #32           
    mov     x14, x2               

    
    mov     x10, x7               
    subs    x4, x4, x4           

SymCryptFdefMontgomeryReduceRawSubAsmLoop
    sub     x3, x3, #32           
    

    ldp     x4, x6, [x1, #32]!   
    ldp     x5, x7, [x0, #32]!   
    sbcs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2, #32]!   

    ldp     x4, x6, [x1, #16]    
    ldp     x5, x7, [x0, #16]    
    sbcs    x4, x4, x5
    sbcs    x6, x6, x7
    stp     x4, x6, [x2, #16]    

    cbnz    x3, SymCryptFdefMontgomeryReduceRawSubAsmLoop

    cset    x0, cc                 

    orr     x11, x10, x0         

    
    sub     x0, x1, x22          
    mov     x1, x14               

    mov     x2, x22               
    subs    x4, x10, x11         

SymCryptFdefMontgomeryReduceMaskedCopyAsmLoop
    sub     x2, x2, #32           

    ldp     x4, x6, [x0, #32]!   
    ldp     x5, x7, [x1, #32]!   
    csel    x4, x4, x5, cc       
    csel    x6, x6, x7, cc
    stp     x4, x6, [x1]         

    ldp     x4, x6, [x0, #16]
    ldp     x5, x7, [x1, #16]
    csel    x4, x4, x5, cc
    csel    x6, x6, x7, cc
    stp     x4, x6, [x1, #16]

    cbnz    x2, SymCryptFdefMontgomeryReduceMaskedCopyAsmLoop

    

    EPILOG_RESTORE_REG_PAIR x21, x22, #32
    EPILOG_RESTORE_REG_PAIR x19, x20, #16
    EPILOG_RESTORE_REG_PAIR fp, lr, #48! 
    EPILOG_RETURN
    NESTED_END SymCryptFdefMontgomeryReduceAsm



    MACRO
    MULADD_LOADSTORE18 $pS, $pM, $pD, $K, $Tc, $T0, $T1, $T2, $T3, $T4, $T5    
    
    
    
    

    
    ldp     $T2, $T3, [$pS]        
    adds    $T2, $T2, $Tc

    ldr     $T0, [$pM, #8]
    mul     $T1,  $K, $T0
    adcs    $T3, $T3, $T1
    umulh   $T0,  $K, $T0
    ldp     $T4, $T5, [$pS, #16]   
    adcs    $T4, $T4, $T0

    ldr     $T0, [$pM, #24]
    mul     $T1,  $K, $T0
    adcs    $T5, $T5, $T1
    umulh   $Tc,  $K, $T0
    adc     $Tc, $Tc, XZR

    
    ldr     $T0, [$pM]
    mul     $T1,  $K, $T0
    adds    $T2, $T2, $T1
    umulh   $T0,  $K, $T0
    adcs    $T3, $T3, $T0
    stp     $T2, $T3, [$pD]        

    ldr     $T0, [$pM, #16]
    mul     $T1,  $K, $T0
    adcs    $T4, $T4, $T1
    umulh   $T0,  $K, $T0
    adcs    $T5, $T5, $T0
    stp     $T4, $T5, [$pD, #16]   

    adc     $Tc, $Tc, XZR
    MEND

    MACRO
    SHIFTRIGHT2 $pD, $index, $shrVal, $shrMask, $shlVal, $Tcin, $Tcout, $T0, $T1    
    
    
    
    
    
    
    
    
    
    

    ldp     $Tcout, $T0, [$pD, #($index*8)]

    lsr     $T1, $T0, $shrVal
    lsl     $Tcin, $Tcin, $shlVal
    and     $T1, $T1, $shrMask
    orr     $T1, $T1, $Tcin

    lsr     $Tcin, $Tcout, $shrVal
    lsl     $T0, $T0, $shlVal
    and     $Tcin, $Tcin, $shrMask
    orr     $Tcin, $Tcin, $T0

    stp     $Tcin, $T1, [$pD, #($index*8)]
    MEND




    LEAF_ENTRY SymCryptFdefModDivSmallPow2Asm
    ldr     x7, [x1]
    ldr     x5, [x0, #SymCryptModulusInv64OffsetArm64]    

    mul     x7, x7, x5   
                            
                            

    
    mov     x6, #-1
    neg     x5, x2
    lsr     x6, x6, x5   
                            
                            

    and     x7, x7, x6   
                            
                            

    ldr     w4, [x0, #SymCryptModulusNdigitsOffsetArm64]  
    add     x5, x0, #SymCryptModulusValueOffsetArm64      

    eor     x6, x6, x6   

SymCryptFdefModDivSmallPow2AsmMulAddLoop
    


    MULADD_LOADSTORE18 x1, x5, x3, x7, x6, x8, x9, X10, x11, x12, x13

    
    add     x1, x1, #32
    add     x5, x5, #32
    add     x3, x3, #32

    sub     w4, w4, #1
    cbnz    w4, SymCryptFdefModDivSmallPow2AsmMulAddLoop

    ldr     w0, [x0, #SymCryptModulusNdigitsOffsetArm64]  
    mov     x1, #64
    subs    x1, x1, x2   
    csetm   x4, ne         

    sub     x3, x3, #32  

SymCryptFdefModDivSmallPow2AsmShiftRightLoop
    
    


    SHIFTRIGHT2 x3, 2, x2, x4, x1, x6, x7, x8, x9
    SHIFTRIGHT2 x3, 0, x2, x4, x1, x7, x6, x8, x9
        
    sub     x3, x3, #32

    sub     w0, w0, #1
    cbnz    w0, SymCryptFdefModDivSmallPow2AsmShiftRightLoop

    ret
    LEAF_END SymCryptFdefModDivSmallPow2Asm



    END
