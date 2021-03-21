// This is auto-gen data for Capstone disassembly engine (www.capstone-engine.org)
// By Nguyen Anh Quynh <aquynh@gmail.com>

{	/* ARM_ADCri, ARM_INS_ADC: adc${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ADCrr, ARM_INS_ADC: adc${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_ADCrsi, ARM_INS_ADC: adc${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ADCrsr, ARM_INS_ADC: adc${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_ADDri, ARM_INS_ADD: add${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ADDrr, ARM_INS_ADD: add${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_ADDrsi, ARM_INS_ADD: add${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ADDrsr, ARM_INS_ADD: add${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_ADR, ARM_INS_ADR: adr${p}	$rd, $label */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_AESD, ARM_INS_AESD: aesd.8	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_AESE, ARM_INS_AESE: aese.8	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_AESIMC, ARM_INS_AESIMC: aesimc.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_AESMC, ARM_INS_AESMC: aesmc.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ANDri, ARM_INS_AND: and${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ANDrr, ARM_INS_AND: and${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_ANDrsi, ARM_INS_AND: and${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ANDrsr, ARM_INS_AND: and${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_BFC, ARM_INS_BFC: bfc${p}	$rd, $imm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_BFI, ARM_INS_BFI: bfi${p}	$rd, $rn, $imm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_BICri, ARM_INS_BIC: bic${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_BICrr, ARM_INS_BIC: bic${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_BICrsi, ARM_INS_BIC: bic${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_BICrsr, ARM_INS_BIC: bic${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_BKPT, ARM_INS_BKPT: bkpt	$val */
	{ 0 }
},
{	/* ARM_BL, ARM_INS_BL: bl	$func */
	{ 0 }
},
{	/* ARM_BLX, ARM_INS_BLX: blx	$func */
	{ CS_AC_READ, 0 }
},
{	/* ARM_BLX_pred, ARM_INS_BLX: blx${p}	$func */
	{ CS_AC_READ, 0 }
},
{	/* ARM_BLXi, ARM_INS_BLX: blx	$target */
	{ 0 }
},
{	/* ARM_BL_pred, ARM_INS_BL: bl${p}	$func */
	{ 0 }
},
{	/* ARM_BX, ARM_INS_BX: bx	$dst */
	{ CS_AC_READ, 0 }
},
{	/* ARM_BXJ, ARM_INS_BXJ: bxj${p}	$func */
	{ CS_AC_READ, 0 }
},
{	/* ARM_BX_RET, ARM_INS_BX: bx${p}	lr */
	{ 0 }
},
{	/* ARM_BX_pred, ARM_INS_BX: bx${p}	$dst */
	{ CS_AC_READ, 0 }
},
{	/* ARM_Bcc, ARM_INS_B: b${p}	$target */
	{ 0 }
},
{	/* ARM_CDP, ARM_INS_CDP: cdp${p}	$cop, $opc1, $crd, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_CDP2, ARM_INS_CDP2: cdp2	$cop, $opc1, $crd, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_CLREX, ARM_INS_CLREX: clrex */
	{ 0 }
},
{	/* ARM_CLZ, ARM_INS_CLZ: clz${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_CMNri, ARM_INS_CMN: cmn${p}	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_CMNzrr, ARM_INS_CMN: cmn${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CMNzrsi, ARM_INS_CMN: cmn${p}	$rn, $shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_CMNzrsr, ARM_INS_CMN: cmn${p}	$rn, $shift */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CMPri, ARM_INS_CMP: cmp${p}	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_CMPrr, ARM_INS_CMP: cmp${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CMPrsi, ARM_INS_CMP: cmp${p}	$rn, $shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_CMPrsr, ARM_INS_CMP: cmp${p}	$rn, $shift */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CPS1p, ARM_INS_CPS: cps	$mode */
	{ 0 }
},
{	/* ARM_CPS2p, ARM_INS_CPS: cps$imod	$iflags */
	{ 0 }
},
{	/* ARM_CPS3p, ARM_INS_CPS: cps$imod	$iflags, $mode */
	{ 0 }
},
{	/* ARM_CRC32B, ARM_INS_CRC32B: crc32b	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CRC32CB, ARM_INS_CRC32CB: crc32cb	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CRC32CH, ARM_INS_CRC32CH: crc32ch	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CRC32CW, ARM_INS_CRC32CW: crc32cw	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CRC32H, ARM_INS_CRC32H: crc32h	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_CRC32W, ARM_INS_CRC32W: crc32w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_DBG, ARM_INS_DBG: dbg${p}	$opt */
	{ 0 }
},
{	/* ARM_DMB, ARM_INS_DMB: dmb	$opt */
	{ 0 }
},
{	/* ARM_DSB, ARM_INS_DSB: dsb	$opt */
	{ 0 }
},
{	/* ARM_EORri, ARM_INS_EOR: eor${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_EORrr, ARM_INS_EOR: eor${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_EORrsi, ARM_INS_EOR: eor${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_EORrsr, ARM_INS_EOR: eor${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_ERET, ARM_INS_ERET: eret${p} */
	{ 0 }
},
{	/* ARM_FCONSTD, ARM_INS_VMOV: vmov${p}.f64	$dd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_FCONSTS, ARM_INS_VMOV: vmov${p}.f32	$sd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_FLDMXDB_UPD, ARM_INS_FLDMDBX: fldmdbx${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_FLDMXIA, ARM_INS_FLDMIAX: fldmiax${p}	$rn, $regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_FLDMXIA_UPD, ARM_INS_FLDMIAX: fldmiax${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_FMSTAT, ARM_INS_VMRS: vmrs${p}	apsr_nzcv, fpscr */
	{ 0 }
},
{	/* ARM_FSTMXDB_UPD, ARM_INS_FSTMDBX: fstmdbx${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_FSTMXIA, ARM_INS_FSTMIAX: fstmiax${p}	$rn, $regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_FSTMXIA_UPD, ARM_INS_FSTMIAX: fstmiax${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_HINT, ARM_INS_HINT: hint${p}	$imm */
	{ 0 }
},
{	/* ARM_HLT, ARM_INS_HLT: hlt	$val */
	{ 0 }
},
{	/* ARM_HVC, ARM_INS_HVC: hvc	$imm */
	{ 0 }
},
{	/* ARM_ISB, ARM_INS_ISB: isb	$opt */
	{ 0 }
},
{	/* ARM_LDA, ARM_INS_LDA: lda${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDAB, ARM_INS_LDAB: ldab${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDAEX, ARM_INS_LDAEX: ldaex${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDAEXB, ARM_INS_LDAEXB: ldaexb${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDAEXD, ARM_INS_LDAEXD: ldaexd${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDAEXH, ARM_INS_LDAEXH: ldaexh${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDAH, ARM_INS_LDAH: ldah${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDC2L_OFFSET, ARM_INS_LDC2L: ldc2l	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC2L_OPTION, ARM_INS_LDC2L: ldc2l	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC2L_POST, ARM_INS_LDC2L: ldc2l	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC2L_PRE, ARM_INS_LDC2L: ldc2l	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC2_OFFSET, ARM_INS_LDC2: ldc2	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC2_OPTION, ARM_INS_LDC2: ldc2	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC2_POST, ARM_INS_LDC2: ldc2	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC2_PRE, ARM_INS_LDC2: ldc2	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDCL_OFFSET, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDCL_OPTION, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDCL_POST, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDCL_PRE, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC_OFFSET, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC_OPTION, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC_POST, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDC_PRE, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDMDA, ARM_INS_LDMDA: ldmda${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_LDMDA_UPD, ARM_INS_LDMDA: ldmda${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_LDMDB, ARM_INS_LDMDB: ldmdb${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_LDMDB_UPD, ARM_INS_LDMDB: ldmdb${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_LDMIA, ARM_INS_LDM: ldm${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_LDMIA_UPD, ARM_INS_LDM: ldm${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_LDMIB, ARM_INS_LDMIB: ldmib${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_LDMIB_UPD, ARM_INS_LDMIB: ldmib${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_LDRBT_POST_IMM, ARM_INS_LDRBT: ldrbt${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRBT_POST_REG, ARM_INS_LDRBT: ldrbt${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRB_POST_IMM, ARM_INS_LDRB: ldrb${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRB_POST_REG, ARM_INS_LDRB: ldrb${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRB_PRE_IMM, ARM_INS_LDRB: ldrb${p}	$rt, $addr! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRB_PRE_REG, ARM_INS_LDRB: ldrb${p}	$rt, $addr! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRBi12, ARM_INS_LDRB: ldrb${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRBrs, ARM_INS_LDRB: ldrb${p}	$rt, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRD, ARM_INS_LDRD: ldrd${p}	$rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_LDRD_POST, ARM_INS_LDRD: ldrd${p}	$rt, $rt2, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRD_PRE, ARM_INS_LDRD: ldrd${p}	$rt, $rt2, $addr! */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_LDREX, ARM_INS_LDREX: ldrex${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDREXB, ARM_INS_LDREXB: ldrexb${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDREXD, ARM_INS_LDREXD: ldrexd${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDREXH, ARM_INS_LDREXH: ldrexh${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRH, ARM_INS_LDRH: ldrh${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_LDRHTi, ARM_INS_LDRHT: ldrht${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRHTr, ARM_INS_LDRHT: ldrht${p}	$rt, $addr, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDRH_POST, ARM_INS_LDRH: ldrh${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRH_PRE, ARM_INS_LDRH: ldrh${p}	$rt, $addr! */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_LDRSB, ARM_INS_LDRSB: ldrsb${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_LDRSBTi, ARM_INS_LDRSBT: ldrsbt${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRSBTr, ARM_INS_LDRSBT: ldrsbt${p}	$rt, $addr, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDRSB_POST, ARM_INS_LDRSB: ldrsb${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRSB_PRE, ARM_INS_LDRSB: ldrsb${p}	$rt, $addr! */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_LDRSH, ARM_INS_LDRSH: ldrsh${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_LDRSHTi, ARM_INS_LDRSHT: ldrsht${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRSHTr, ARM_INS_LDRSHT: ldrsht${p}	$rt, $addr, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_LDRSH_POST, ARM_INS_LDRSH: ldrsh${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRSH_PRE, ARM_INS_LDRSH: ldrsh${p}	$rt, $addr! */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_LDRT_POST_IMM, ARM_INS_LDRT: ldrt${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRT_POST_REG, ARM_INS_LDRT: ldrt${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDR_POST_IMM, ARM_INS_LDR: ldr${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDR_POST_REG, ARM_INS_LDR: ldr${p}	$rt, $addr, $offset */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDR_PRE_IMM, ARM_INS_LDR: ldr${p}	$rt, $addr! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDR_PRE_REG, ARM_INS_LDR: ldr${p}	$rt, $addr! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRcp, ARM_INS_LDR: ldr${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRi12, ARM_INS_LDR: ldr${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_LDRrs, ARM_INS_LDR: ldr${p}	$rt, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_MCR, ARM_INS_MCR: mcr${p}	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_MCR2, ARM_INS_MCR2: mcr2	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_MCRR, ARM_INS_MCRR: mcrr${p}	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_MCRR2, ARM_INS_MCRR2: mcrr2	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_MLA, ARM_INS_MLA: mla${s}${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_MLS, ARM_INS_MLS: mls${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_MOVPCLR, ARM_INS_MOV: mov${p}	pc, lr */
	{ 0 }
},
{	/* ARM_MOVTi16, ARM_INS_MOVT: movt${p}	$rd, $imm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_MOVi, ARM_INS_MOV: mov${s}${p}	$rd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MOVi16, ARM_INS_MOVW: movw${p}	$rd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MOVr, ARM_INS_MOV: mov${s}${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_MOVr_TC, ARM_INS_MOV: mov${s}${p}	$rd, $rm */
	{ 0 }
},
{	/* ARM_MOVsi, ARM_INS_MOV: mov${s}${p}	$rd, $src */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MOVsr, ARM_INS_MOV: mov${s}${p}	$rd, $src */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MRC, ARM_INS_MRC: mrc${p}	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_MRC2, ARM_INS_MRC2: mrc2	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_MRRC, ARM_INS_MRRC: mrrc${p}	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_MRRC2, ARM_INS_MRRC2: mrrc2	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_MRS, ARM_INS_MRS: mrs${p}	$rd, apsr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MRSbanked, ARM_INS_MRS: mrs${p}	$rd, $banked */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MRSsys, ARM_INS_MRS: mrs${p}	$rd, spsr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MSR, ARM_INS_MSR: msr${p}	$mask, $rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_MSRbanked, ARM_INS_MSR: msr${p}	$banked, $rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_MSRi, ARM_INS_MSR: msr${p}	$mask, $imm */
	{ 0 }
},
{	/* ARM_MUL, ARM_INS_MUL: mul${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_MVNi, ARM_INS_MVN: mvn${s}${p}	$rd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MVNr, ARM_INS_MVN: mvn${s}${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_MVNsi, ARM_INS_MVN: mvn${s}${p}	$rd, $shift */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_MVNsr, ARM_INS_MVN: mvn${s}${p}	$rd, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ORRri, ARM_INS_ORR: orr${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ORRrr, ARM_INS_ORR: orr${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_ORRrsi, ARM_INS_ORR: orr${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_ORRrsr, ARM_INS_ORR: orr${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_PKHBT, ARM_INS_PKHBT: pkhbt${p}	$rd, $rn, $rm$sh */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_PKHTB, ARM_INS_PKHTB: pkhtb${p}	$rd, $rn, $rm$sh */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_PLDWi12, ARM_INS_PLDW: pldw	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_PLDWrs, ARM_INS_PLDW: pldw	$shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_PLDi12, ARM_INS_PLD: pld	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_PLDrs, ARM_INS_PLD: pld	$shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_PLIi12, ARM_INS_PLI: pli	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_PLIrs, ARM_INS_PLI: pli	$shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_QADD, ARM_INS_QADD: qadd${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QADD16, ARM_INS_QADD16: qadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QADD8, ARM_INS_QADD8: qadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QASX, ARM_INS_QASX: qasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QDADD, ARM_INS_QDADD: qdadd${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QDSUB, ARM_INS_QDSUB: qdsub${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QSAX, ARM_INS_QSAX: qsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QSUB, ARM_INS_QSUB: qsub${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QSUB16, ARM_INS_QSUB16: qsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_QSUB8, ARM_INS_QSUB8: qsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_RBIT, ARM_INS_RBIT: rbit${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_REV, ARM_INS_REV: rev${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_REV16, ARM_INS_REV16: rev16${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_REVSH, ARM_INS_REVSH: revsh${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_RFEDA, ARM_INS_RFEDA: rfeda	$rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RFEDA_UPD, ARM_INS_RFEDA: rfeda	$rn! */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RFEDB, ARM_INS_RFEDB: rfedb	$rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RFEDB_UPD, ARM_INS_RFEDB: rfedb	$rn! */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RFEIA, ARM_INS_RFEIA: rfeia	$rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RFEIA_UPD, ARM_INS_RFEIA: rfeia	$rn! */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RFEIB, ARM_INS_RFEIB: rfeib	$rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RFEIB_UPD, ARM_INS_RFEIB: rfeib	$rn! */
	{ CS_AC_READ, 0 }
},
{	/* ARM_RSBri, ARM_INS_RSB: rsb${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_RSBrr, ARM_INS_RSB: rsb${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_RSBrsi, ARM_INS_RSB: rsb${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_RSBrsr, ARM_INS_RSB: rsb${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_RSCri, ARM_INS_RSC: rsc${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_RSCrr, ARM_INS_RSC: rsc${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_RSCrsi, ARM_INS_RSC: rsc${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_RSCrsr, ARM_INS_RSC: rsc${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SADD16, ARM_INS_SADD16: sadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SADD8, ARM_INS_SADD8: sadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SASX, ARM_INS_SASX: sasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SBCri, ARM_INS_SBC: sbc${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SBCrr, ARM_INS_SBC: sbc${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SBCrsi, ARM_INS_SBC: sbc${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SBCrsr, ARM_INS_SBC: sbc${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SBFX, ARM_INS_SBFX: sbfx${p}	$rd, $rn, $lsb, $width */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SDIV, ARM_INS_SDIV: sdiv${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SEL, ARM_INS_SEL: sel${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SETEND, ARM_INS_SETEND: setend	$end */
	{ 0 }
},
{	/* ARM_SHA1C, ARM_INS_SHA1C: sha1c.32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHA1H, ARM_INS_SHA1H: sha1h.32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SHA1M, ARM_INS_SHA1M: sha1m.32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHA1P, ARM_INS_SHA1P: sha1p.32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHA1SU0, ARM_INS_SHA1SU0: sha1su0.32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHA1SU1, ARM_INS_SHA1SU1: sha1su1.32	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SHA256H, ARM_INS_SHA256H: sha256h.32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHA256H2, ARM_INS_SHA256H2: sha256h2.32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHA256SU0, ARM_INS_SHA256SU0: sha256su0.32	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SHA256SU1, ARM_INS_SHA256SU1: sha256su1.32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHADD16, ARM_INS_SHADD16: shadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHADD8, ARM_INS_SHADD8: shadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHASX, ARM_INS_SHASX: shasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHSAX, ARM_INS_SHSAX: shsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHSUB16, ARM_INS_SHSUB16: shsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SHSUB8, ARM_INS_SHSUB8: shsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMC, ARM_INS_SMC: smc${p}	$opt */
	{ 0 }
},
{	/* ARM_SMLABB, ARM_INS_SMLABB: smlabb${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLABT, ARM_INS_SMLABT: smlabt${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLAD, ARM_INS_SMLAD: smlad${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLADX, ARM_INS_SMLADX: smladx${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLAL, ARM_INS_SMLAL: smlal${s}${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLALBB, ARM_INS_SMLALBB: smlalbb${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLALBT, ARM_INS_SMLALBT: smlalbt${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLALD, ARM_INS_SMLALD: smlald${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLALDX, ARM_INS_SMLALDX: smlaldx${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLALTB, ARM_INS_SMLALTB: smlaltb${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLALTT, ARM_INS_SMLALTT: smlaltt${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLATB, ARM_INS_SMLATB: smlatb${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLATT, ARM_INS_SMLATT: smlatt${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLAWB, ARM_INS_SMLAWB: smlawb${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLAWT, ARM_INS_SMLAWT: smlawt${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLSD, ARM_INS_SMLSD: smlsd${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLSDX, ARM_INS_SMLSDX: smlsdx${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLSLD, ARM_INS_SMLSLD: smlsld${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMLSLDX, ARM_INS_SMLSLDX: smlsldx${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMMLA, ARM_INS_SMMLA: smmla${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMMLAR, ARM_INS_SMMLAR: smmlar${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMMLS, ARM_INS_SMMLS: smmls${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMMLSR, ARM_INS_SMMLSR: smmlsr${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMMUL, ARM_INS_SMMUL: smmul${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMMULR, ARM_INS_SMMULR: smmulr${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMUAD, ARM_INS_SMUAD: smuad${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMUADX, ARM_INS_SMUADX: smuadx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMULBB, ARM_INS_SMULBB: smulbb${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMULBT, ARM_INS_SMULBT: smulbt${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMULL, ARM_INS_SMULL: smull${s}${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMULTB, ARM_INS_SMULTB: smultb${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMULTT, ARM_INS_SMULTT: smultt${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMULWB, ARM_INS_SMULWB: smulwb${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMULWT, ARM_INS_SMULWT: smulwt${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMUSD, ARM_INS_SMUSD: smusd${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SMUSDX, ARM_INS_SMUSDX: smusdx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SRSDA, ARM_INS_SRSDA: srsda	sp, $mode */
	{ 0 }
},
{	/* ARM_SRSDA_UPD, ARM_INS_SRSDA: srsda	sp!, $mode */
	{ 0 }
},
{	/* ARM_SRSDB, ARM_INS_SRSDB: srsdb	sp, $mode */
	{ 0 }
},
{	/* ARM_SRSDB_UPD, ARM_INS_SRSDB: srsdb	sp!, $mode */
	{ 0 }
},
{	/* ARM_SRSIA, ARM_INS_SRSIA: srsia	sp, $mode */
	{ 0 }
},
{	/* ARM_SRSIA_UPD, ARM_INS_SRSIA: srsia	sp!, $mode */
	{ 0 }
},
{	/* ARM_SRSIB, ARM_INS_SRSIB: srsib	sp, $mode */
	{ 0 }
},
{	/* ARM_SRSIB_UPD, ARM_INS_SRSIB: srsib	sp!, $mode */
	{ 0 }
},
{	/* ARM_SSAT, ARM_INS_SSAT: ssat${p}	$rd, $sat_imm, $rn$sh */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_SSAT16, ARM_INS_SSAT16: ssat16${p}	$rd, $sat_imm, $rn */
	{ CS_AC_WRITE, CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_SSAX, ARM_INS_SSAX: ssax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SSUB16, ARM_INS_SSUB16: ssub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SSUB8, ARM_INS_SSUB8: ssub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2L_OFFSET, ARM_INS_STC2L: stc2l	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2L_OPTION, ARM_INS_STC2L: stc2l	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2L_POST, ARM_INS_STC2L: stc2l	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2L_PRE, ARM_INS_STC2L: stc2l	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2_OFFSET, ARM_INS_STC2: stc2	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2_OPTION, ARM_INS_STC2: stc2	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2_POST, ARM_INS_STC2: stc2	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC2_PRE, ARM_INS_STC2: stc2	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STCL_OFFSET, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STCL_OPTION, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STCL_POST, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STCL_PRE, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC_OFFSET, ARM_INS_STC: stc${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC_OPTION, ARM_INS_STC: stc${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC_POST, ARM_INS_STC: stc${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STC_PRE, ARM_INS_STC: stc${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STL, ARM_INS_STL: stl${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STLB, ARM_INS_STLB: stlb${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STLEX, ARM_INS_STLEX: stlex${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STLEXB, ARM_INS_STLEXB: stlexb${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STLEXD, ARM_INS_STLEXD: stlexd${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STLEXH, ARM_INS_STLEXH: stlexh${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STLH, ARM_INS_STLH: stlh${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STMDA, ARM_INS_STMDA: stmda${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STMDA_UPD, ARM_INS_STMDA: stmda${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_STMDB, ARM_INS_STMDB: stmdb${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STMDB_UPD, ARM_INS_STMDB: stmdb${p}	$rn!, $regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_STMIA, ARM_INS_STM: stm${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STMIA_UPD, ARM_INS_STM: stm${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_STMIB, ARM_INS_STMIB: stmib${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STMIB_UPD, ARM_INS_STMIB: stmib${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_STRBT_POST_IMM, ARM_INS_STRBT: strbt${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRBT_POST_REG, ARM_INS_STRBT: strbt${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRB_POST_IMM, ARM_INS_STRB: strb${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRB_POST_REG, ARM_INS_STRB: strb${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRB_PRE_IMM, ARM_INS_STRB: strb${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRB_PRE_REG, ARM_INS_STRB: strb${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRBi12, ARM_INS_STRB: strb${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRBrs, ARM_INS_STRB: strb${p}	$rt, $shift */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRD, ARM_INS_STRD: strd${p}	$rt, $rt2, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRD_POST, ARM_INS_STRD: strd${p}	$rt, $rt2, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRD_PRE, ARM_INS_STRD: strd${p}	$rt, $rt2, $addr! */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STREX, ARM_INS_STREX: strex${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STREXB, ARM_INS_STREXB: strexb${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STREXD, ARM_INS_STREXD: strexd${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STREXH, ARM_INS_STREXH: strexh${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRH, ARM_INS_STRH: strh${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRHTi, ARM_INS_STRHT: strht${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRHTr, ARM_INS_STRHT: strht${p}	$rt, $addr, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRH_POST, ARM_INS_STRH: strh${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_STRH_PRE, ARM_INS_STRH: strh${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRT_POST_IMM, ARM_INS_STRT: strt${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRT_POST_REG, ARM_INS_STRT: strt${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STR_POST_IMM, ARM_INS_STR: str${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STR_POST_REG, ARM_INS_STR: str${p}	$rt, $addr, $offset */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STR_PRE_IMM, ARM_INS_STR: str${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STR_PRE_REG, ARM_INS_STR: str${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRi12, ARM_INS_STR: str${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_STRrs, ARM_INS_STR: str${p}	$rt, $shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_SUBri, ARM_INS_SUB: sub${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SUBrr, ARM_INS_SUB: sub${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SUBrsi, ARM_INS_SUB: sub${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SUBrsr, ARM_INS_SUB: sub${s}${p}	$rd, $rn, $shift */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SVC, ARM_INS_SVC: svc${p}	$svc */
	{ 0 }
},
{	/* ARM_SWP, ARM_INS_SWP: swp${p}	$rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SWPB, ARM_INS_SWPB: swpb${p}	$rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_SXTAB, ARM_INS_SXTAB: sxtab${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SXTAB16, ARM_INS_SXTAB16: sxtab16${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SXTAH, ARM_INS_SXTAH: sxtah${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_SXTB, ARM_INS_SXTB: sxtb${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_SXTB16, ARM_INS_SXTB16: sxtb16${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_SXTH, ARM_INS_SXTH: sxth${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_TEQri, ARM_INS_TEQ: teq${p}	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_TEQrr, ARM_INS_TEQ: teq${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_TEQrsi, ARM_INS_TEQ: teq${p}	$rn, $shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_TEQrsr, ARM_INS_TEQ: teq${p}	$rn, $shift */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_TRAP, ARM_INS_TRAP: trap */
	{ 0 }
},
{	/* ARM_TRAPNaCl, ARM_INS_TRAP: trap */
	{ 0 }
},
{	/* ARM_TSTri, ARM_INS_TST: tst${p}	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_TSTrr, ARM_INS_TST: tst${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_TSTrsi, ARM_INS_TST: tst${p}	$rn, $shift */
	{ CS_AC_READ, 0 }
},
{	/* ARM_TSTrsr, ARM_INS_TST: tst${p}	$rn, $shift */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UADD16, ARM_INS_UADD16: uadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UADD8, ARM_INS_UADD8: uadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UASX, ARM_INS_UASX: uasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UBFX, ARM_INS_UBFX: ubfx${p}	$rd, $rn, $lsb, $width */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_UDF, ARM_INS_UDF: udf	$imm16 */
	{ 0 }
},
{	/* ARM_UDIV, ARM_INS_UDIV: udiv${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UHADD16, ARM_INS_UHADD16: uhadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UHADD8, ARM_INS_UHADD8: uhadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UHASX, ARM_INS_UHASX: uhasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UHSAX, ARM_INS_UHSAX: uhsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UHSUB16, ARM_INS_UHSUB16: uhsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UHSUB8, ARM_INS_UHSUB8: uhsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UMAAL, ARM_INS_UMAAL: umaal${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UMLAL, ARM_INS_UMLAL: umlal${s}${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UMULL, ARM_INS_UMULL: umull${s}${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UQADD16, ARM_INS_UQADD16: uqadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UQADD8, ARM_INS_UQADD8: uqadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UQASX, ARM_INS_UQASX: uqasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UQSAX, ARM_INS_UQSAX: uqsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UQSUB16, ARM_INS_UQSUB16: uqsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UQSUB8, ARM_INS_UQSUB8: uqsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_USAD8, ARM_INS_USAD8: usad8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_USADA8, ARM_INS_USADA8: usada8${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_USAT, ARM_INS_USAT: usat${p}	$rd, $sat_imm, $rn$sh */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_USAT16, ARM_INS_USAT16: usat16${p}	$rd, $sat_imm, $rn */
	{ CS_AC_WRITE, CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_USAX, ARM_INS_USAX: usax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_USUB16, ARM_INS_USUB16: usub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_USUB8, ARM_INS_USUB8: usub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_UXTAB, ARM_INS_UXTAB: uxtab${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_UXTAB16, ARM_INS_UXTAB16: uxtab16${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_UXTAH, ARM_INS_UXTAH: uxtah${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_UXTB, ARM_INS_UXTB: uxtb${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_UXTB16, ARM_INS_UXTB16: uxtb16${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_UXTH, ARM_INS_UXTH: uxth${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VABALsv2i64, ARM_INS_VABAL: vabal${p}.s32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABALsv4i32, ARM_INS_VABAL: vabal${p}.s16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABALsv8i16, ARM_INS_VABAL: vabal${p}.s8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABALuv2i64, ARM_INS_VABAL: vabal${p}.u32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABALuv4i32, ARM_INS_VABAL: vabal${p}.u16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABALuv8i16, ARM_INS_VABAL: vabal${p}.u8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAsv16i8, ARM_INS_VABA: vaba${p}.s8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAsv2i32, ARM_INS_VABA: vaba${p}.s32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAsv4i16, ARM_INS_VABA: vaba${p}.s16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAsv4i32, ARM_INS_VABA: vaba${p}.s32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAsv8i16, ARM_INS_VABA: vaba${p}.s16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAsv8i8, ARM_INS_VABA: vaba${p}.s8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAuv16i8, ARM_INS_VABA: vaba${p}.u8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAuv2i32, ARM_INS_VABA: vaba${p}.u32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAuv4i16, ARM_INS_VABA: vaba${p}.u16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAuv4i32, ARM_INS_VABA: vaba${p}.u32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAuv8i16, ARM_INS_VABA: vaba${p}.u16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABAuv8i8, ARM_INS_VABA: vaba${p}.u8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDLsv2i64, ARM_INS_VABDL: vabdl${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDLsv4i32, ARM_INS_VABDL: vabdl${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDLsv8i16, ARM_INS_VABDL: vabdl${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDLuv2i64, ARM_INS_VABDL: vabdl${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDLuv4i32, ARM_INS_VABDL: vabdl${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDLuv8i16, ARM_INS_VABDL: vabdl${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDfd, ARM_INS_VABD: vabd${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDfq, ARM_INS_VABD: vabd${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDsv16i8, ARM_INS_VABD: vabd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDsv2i32, ARM_INS_VABD: vabd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDsv4i16, ARM_INS_VABD: vabd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDsv4i32, ARM_INS_VABD: vabd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDsv8i16, ARM_INS_VABD: vabd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDsv8i8, ARM_INS_VABD: vabd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDuv16i8, ARM_INS_VABD: vabd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDuv2i32, ARM_INS_VABD: vabd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDuv4i16, ARM_INS_VABD: vabd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDuv4i32, ARM_INS_VABD: vabd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDuv8i16, ARM_INS_VABD: vabd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABDuv8i8, ARM_INS_VABD: vabd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VABSD, ARM_INS_VABS: vabs${p}.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSS, ARM_INS_VABS: vabs${p}.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSfd, ARM_INS_VABS: vabs${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSfq, ARM_INS_VABS: vabs${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSv16i8, ARM_INS_VABS: vabs${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSv2i32, ARM_INS_VABS: vabs${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSv4i16, ARM_INS_VABS: vabs${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSv4i32, ARM_INS_VABS: vabs${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSv8i16, ARM_INS_VABS: vabs${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VABSv8i8, ARM_INS_VABS: vabs${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VACGEd, ARM_INS_VACGE: vacge${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VACGEq, ARM_INS_VACGE: vacge${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VACGTd, ARM_INS_VACGT: vacgt${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VACGTq, ARM_INS_VACGT: vacgt${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDD, ARM_INS_VADD: vadd${p}.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDHNv2i32, ARM_INS_VADDHN: vaddhn${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDHNv4i16, ARM_INS_VADDHN: vaddhn${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDHNv8i8, ARM_INS_VADDHN: vaddhn${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDLsv2i64, ARM_INS_VADDL: vaddl${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDLsv4i32, ARM_INS_VADDL: vaddl${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDLsv8i16, ARM_INS_VADDL: vaddl${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDLuv2i64, ARM_INS_VADDL: vaddl${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDLuv4i32, ARM_INS_VADDL: vaddl${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDLuv8i16, ARM_INS_VADDL: vaddl${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDS, ARM_INS_VADD: vadd${p}.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDWsv2i64, ARM_INS_VADDW: vaddw${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDWsv4i32, ARM_INS_VADDW: vaddw${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDWsv8i16, ARM_INS_VADDW: vaddw${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDWuv2i64, ARM_INS_VADDW: vaddw${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDWuv4i32, ARM_INS_VADDW: vaddw${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDWuv8i16, ARM_INS_VADDW: vaddw${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDfd, ARM_INS_VADD: vadd${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDfq, ARM_INS_VADD: vadd${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv16i8, ARM_INS_VADD: vadd${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv1i64, ARM_INS_VADD: vadd${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv2i32, ARM_INS_VADD: vadd${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv2i64, ARM_INS_VADD: vadd${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv4i16, ARM_INS_VADD: vadd${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv4i32, ARM_INS_VADD: vadd${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv8i16, ARM_INS_VADD: vadd${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VADDv8i8, ARM_INS_VADD: vadd${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VANDd, ARM_INS_VAND: vand${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VANDq, ARM_INS_VAND: vand${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBICd, ARM_INS_VBIC: vbic${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBICiv2i32, ARM_INS_VBIC: vbic${p}.i32	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VBICiv4i16, ARM_INS_VBIC: vbic${p}.i16	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VBICiv4i32, ARM_INS_VBIC: vbic${p}.i32	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VBICiv8i16, ARM_INS_VBIC: vbic${p}.i16	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VBICq, ARM_INS_VBIC: vbic${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBIFd, ARM_INS_VBIF: vbif${p}	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBIFq, ARM_INS_VBIF: vbif${p}	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBITd, ARM_INS_VBIT: vbit${p}	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBITq, ARM_INS_VBIT: vbit${p}	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBSLd, ARM_INS_VBSL: vbsl${p}	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VBSLq, ARM_INS_VBSL: vbsl${p}	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQfd, ARM_INS_VCEQ: vceq${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQfq, ARM_INS_VCEQ: vceq${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQv16i8, ARM_INS_VCEQ: vceq${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQv2i32, ARM_INS_VCEQ: vceq${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQv4i16, ARM_INS_VCEQ: vceq${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQv4i32, ARM_INS_VCEQ: vceq${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQv8i16, ARM_INS_VCEQ: vceq${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQv8i8, ARM_INS_VCEQ: vceq${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv16i8, ARM_INS_VCEQ: vceq${p}.i8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv2f32, ARM_INS_VCEQ: vceq${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv2i32, ARM_INS_VCEQ: vceq${p}.i32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv4f32, ARM_INS_VCEQ: vceq${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv4i16, ARM_INS_VCEQ: vceq${p}.i16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv4i32, ARM_INS_VCEQ: vceq${p}.i32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv8i16, ARM_INS_VCEQ: vceq${p}.i16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCEQzv8i8, ARM_INS_VCEQ: vceq${p}.i8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEfd, ARM_INS_VCGE: vcge${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEfq, ARM_INS_VCGE: vcge${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEsv16i8, ARM_INS_VCGE: vcge${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEsv2i32, ARM_INS_VCGE: vcge${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEsv4i16, ARM_INS_VCGE: vcge${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEsv4i32, ARM_INS_VCGE: vcge${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEsv8i16, ARM_INS_VCGE: vcge${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEsv8i8, ARM_INS_VCGE: vcge${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEuv16i8, ARM_INS_VCGE: vcge${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEuv2i32, ARM_INS_VCGE: vcge${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEuv4i16, ARM_INS_VCGE: vcge${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEuv4i32, ARM_INS_VCGE: vcge${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEuv8i16, ARM_INS_VCGE: vcge${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEuv8i8, ARM_INS_VCGE: vcge${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv16i8, ARM_INS_VCGE: vcge${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv2f32, ARM_INS_VCGE: vcge${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv2i32, ARM_INS_VCGE: vcge${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv4f32, ARM_INS_VCGE: vcge${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv4i16, ARM_INS_VCGE: vcge${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv4i32, ARM_INS_VCGE: vcge${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv8i16, ARM_INS_VCGE: vcge${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGEzv8i8, ARM_INS_VCGE: vcge${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTfd, ARM_INS_VCGT: vcgt${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTfq, ARM_INS_VCGT: vcgt${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTsv16i8, ARM_INS_VCGT: vcgt${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTsv2i32, ARM_INS_VCGT: vcgt${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTsv4i16, ARM_INS_VCGT: vcgt${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTsv4i32, ARM_INS_VCGT: vcgt${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTsv8i16, ARM_INS_VCGT: vcgt${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTsv8i8, ARM_INS_VCGT: vcgt${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTuv16i8, ARM_INS_VCGT: vcgt${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTuv2i32, ARM_INS_VCGT: vcgt${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTuv4i16, ARM_INS_VCGT: vcgt${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTuv4i32, ARM_INS_VCGT: vcgt${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTuv8i16, ARM_INS_VCGT: vcgt${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTuv8i8, ARM_INS_VCGT: vcgt${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv16i8, ARM_INS_VCGT: vcgt${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv2f32, ARM_INS_VCGT: vcgt${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv2i32, ARM_INS_VCGT: vcgt${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv4f32, ARM_INS_VCGT: vcgt${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv4i16, ARM_INS_VCGT: vcgt${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv4i32, ARM_INS_VCGT: vcgt${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv8i16, ARM_INS_VCGT: vcgt${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCGTzv8i8, ARM_INS_VCGT: vcgt${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv16i8, ARM_INS_VCLE: vcle${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv2f32, ARM_INS_VCLE: vcle${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv2i32, ARM_INS_VCLE: vcle${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv4f32, ARM_INS_VCLE: vcle${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv4i16, ARM_INS_VCLE: vcle${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv4i32, ARM_INS_VCLE: vcle${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv8i16, ARM_INS_VCLE: vcle${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLEzv8i8, ARM_INS_VCLE: vcle${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLSv16i8, ARM_INS_VCLS: vcls${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLSv2i32, ARM_INS_VCLS: vcls${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLSv4i16, ARM_INS_VCLS: vcls${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLSv4i32, ARM_INS_VCLS: vcls${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLSv8i16, ARM_INS_VCLS: vcls${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLSv8i8, ARM_INS_VCLS: vcls${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv16i8, ARM_INS_VCLT: vclt${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv2f32, ARM_INS_VCLT: vclt${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv2i32, ARM_INS_VCLT: vclt${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv4f32, ARM_INS_VCLT: vclt${p}.f32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv4i16, ARM_INS_VCLT: vclt${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv4i32, ARM_INS_VCLT: vclt${p}.s32	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv8i16, ARM_INS_VCLT: vclt${p}.s16	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLTzv8i8, ARM_INS_VCLT: vclt${p}.s8	$vd, $vm, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLZv16i8, ARM_INS_VCLZ: vclz${p}.i8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLZv2i32, ARM_INS_VCLZ: vclz${p}.i32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLZv4i16, ARM_INS_VCLZ: vclz${p}.i16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLZv4i32, ARM_INS_VCLZ: vclz${p}.i32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLZv8i16, ARM_INS_VCLZ: vclz${p}.i16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCLZv8i8, ARM_INS_VCLZ: vclz${p}.i8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCMPD, ARM_INS_VCMP: vcmp${p}.f64	$dd, $dm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCMPED, ARM_INS_VCMPE: vcmpe${p}.f64	$dd, $dm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCMPES, ARM_INS_VCMPE: vcmpe${p}.f32	$sd, $sm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCMPEZD, ARM_INS_VCMPE: vcmpe${p}.f64	$dd, #0 */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VCMPEZS, ARM_INS_VCMPE: vcmpe${p}.f32	$sd, #0 */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VCMPS, ARM_INS_VCMP: vcmp${p}.f32	$sd, $sm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VCMPZD, ARM_INS_VCMP: vcmp${p}.f64	$dd, #0 */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VCMPZS, ARM_INS_VCMP: vcmp${p}.f32	$sd, #0 */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VCNTd, ARM_INS_VCNT: vcnt${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCNTq, ARM_INS_VCNT: vcnt${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTANSD, ARM_INS_VCVTA: vcvta.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTANSQ, ARM_INS_VCVTA: vcvta.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTANUD, ARM_INS_VCVTA: vcvta.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTANUQ, ARM_INS_VCVTA: vcvta.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTASD, ARM_INS_VCVTA: vcvta.s32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTASS, ARM_INS_VCVTA: vcvta.s32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTAUD, ARM_INS_VCVTA: vcvta.u32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTAUS, ARM_INS_VCVTA: vcvta.u32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTBDH, ARM_INS_VCVTB: vcvtb${p}.f16.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTBHD, ARM_INS_VCVTB: vcvtb${p}.f64.f16	$dd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTBHS, ARM_INS_VCVTB: vcvtb${p}.f32.f16	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTBSH, ARM_INS_VCVTB: vcvtb${p}.f16.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTDS, ARM_INS_VCVT: vcvt${p}.f64.f32	$dd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMNSD, ARM_INS_VCVTM: vcvtm.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMNSQ, ARM_INS_VCVTM: vcvtm.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMNUD, ARM_INS_VCVTM: vcvtm.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMNUQ, ARM_INS_VCVTM: vcvtm.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMSD, ARM_INS_VCVTM: vcvtm.s32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMSS, ARM_INS_VCVTM: vcvtm.s32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMUD, ARM_INS_VCVTM: vcvtm.u32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTMUS, ARM_INS_VCVTM: vcvtm.u32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNNSD, ARM_INS_VCVTN: vcvtn.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNNSQ, ARM_INS_VCVTN: vcvtn.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNNUD, ARM_INS_VCVTN: vcvtn.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNNUQ, ARM_INS_VCVTN: vcvtn.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNSD, ARM_INS_VCVTN: vcvtn.s32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNSS, ARM_INS_VCVTN: vcvtn.s32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNUD, ARM_INS_VCVTN: vcvtn.u32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTNUS, ARM_INS_VCVTN: vcvtn.u32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPNSD, ARM_INS_VCVTP: vcvtp.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPNSQ, ARM_INS_VCVTP: vcvtp.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPNUD, ARM_INS_VCVTP: vcvtp.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPNUQ, ARM_INS_VCVTP: vcvtp.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPSD, ARM_INS_VCVTP: vcvtp.s32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPSS, ARM_INS_VCVTP: vcvtp.s32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPUD, ARM_INS_VCVTP: vcvtp.u32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTPUS, ARM_INS_VCVTP: vcvtp.u32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTSD, ARM_INS_VCVT: vcvt${p}.f32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTTDH, ARM_INS_VCVTT: vcvtt${p}.f16.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTTHD, ARM_INS_VCVTT: vcvtt${p}.f64.f16	$dd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTTHS, ARM_INS_VCVTT: vcvtt${p}.f32.f16	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTTSH, ARM_INS_VCVTT: vcvtt${p}.f16.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2h, ARM_INS_VCVT: vcvt${p}.f16.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2sd, ARM_INS_VCVT: vcvt${p}.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2sq, ARM_INS_VCVT: vcvt${p}.s32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2ud, ARM_INS_VCVT: vcvt${p}.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2uq, ARM_INS_VCVT: vcvt${p}.u32.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2xsd, ARM_INS_VCVT: vcvt${p}.s32.f32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2xsq, ARM_INS_VCVT: vcvt${p}.s32.f32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2xud, ARM_INS_VCVT: vcvt${p}.u32.f32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTf2xuq, ARM_INS_VCVT: vcvt${p}.u32.f32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTh2f, ARM_INS_VCVT: vcvt${p}.f32.f16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTs2fd, ARM_INS_VCVT: vcvt${p}.f32.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTs2fq, ARM_INS_VCVT: vcvt${p}.f32.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTu2fd, ARM_INS_VCVT: vcvt${p}.f32.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTu2fq, ARM_INS_VCVT: vcvt${p}.f32.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTxs2fd, ARM_INS_VCVT: vcvt${p}.f32.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTxs2fq, ARM_INS_VCVT: vcvt${p}.f32.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTxu2fd, ARM_INS_VCVT: vcvt${p}.f32.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VCVTxu2fq, ARM_INS_VCVT: vcvt${p}.f32.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDIVD, ARM_INS_VDIV: vdiv${p}.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VDIVS, ARM_INS_VDIV: vdiv${p}.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VDUP16d, ARM_INS_VDUP: vdup${p}.16	$v, $r */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUP16q, ARM_INS_VDUP: vdup${p}.16	$v, $r */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUP32d, ARM_INS_VDUP: vdup${p}.32	$v, $r */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUP32q, ARM_INS_VDUP: vdup${p}.32	$v, $r */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUP8d, ARM_INS_VDUP: vdup${p}.8	$v, $r */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUP8q, ARM_INS_VDUP: vdup${p}.8	$v, $r */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUPLN16d, ARM_INS_VDUP: vdup${p}.16	$vd, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUPLN16q, ARM_INS_VDUP: vdup${p}.16	$vd, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUPLN32d, ARM_INS_VDUP: vdup${p}.32	$vd, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUPLN32q, ARM_INS_VDUP: vdup${p}.32	$vd, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUPLN8d, ARM_INS_VDUP: vdup${p}.8	$vd, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VDUPLN8q, ARM_INS_VDUP: vdup${p}.8	$vd, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VEORd, ARM_INS_VEOR: veor${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEORq, ARM_INS_VEOR: veor${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEXTd16, ARM_INS_VEXT: vext${p}.16	$vd, $vn, $vm, $index */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEXTd32, ARM_INS_VEXT: vext${p}.32	$vd, $vn, $vm, $index */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEXTd8, ARM_INS_VEXT: vext${p}.8	$vd, $vn, $vm, $index */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEXTq16, ARM_INS_VEXT: vext${p}.16	$vd, $vn, $vm, $index */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEXTq32, ARM_INS_VEXT: vext${p}.32	$vd, $vn, $vm, $index */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEXTq64, ARM_INS_VEXT: vext${p}.64	$vd, $vn, $vm, $index */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VEXTq8, ARM_INS_VEXT: vext${p}.8	$vd, $vn, $vm, $index */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMAD, ARM_INS_VFMA: vfma${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMAS, ARM_INS_VFMA: vfma${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMAfd, ARM_INS_VFMA: vfma${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMAfq, ARM_INS_VFMA: vfma${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMSD, ARM_INS_VFMS: vfms${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMSS, ARM_INS_VFMS: vfms${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMSfd, ARM_INS_VFMS: vfms${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFMSfq, ARM_INS_VFMS: vfms${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFNMAD, ARM_INS_VFNMA: vfnma${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFNMAS, ARM_INS_VFNMA: vfnma${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFNMSD, ARM_INS_VFNMS: vfnms${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VFNMSS, ARM_INS_VFNMS: vfnms${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VGETLNi32, ARM_INS_VMOV: vmov${p}.32	$r, $v$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VGETLNs16, ARM_INS_VMOV: vmov${p}.s16	$r, $v$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VGETLNs8, ARM_INS_VMOV: vmov${p}.s8	$r, $v$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VGETLNu16, ARM_INS_VMOV: vmov${p}.u16	$r, $v$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VGETLNu8, ARM_INS_VMOV: vmov${p}.u8	$r, $v$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VHADDsv16i8, ARM_INS_VHADD: vhadd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDsv2i32, ARM_INS_VHADD: vhadd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDsv4i16, ARM_INS_VHADD: vhadd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDsv4i32, ARM_INS_VHADD: vhadd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDsv8i16, ARM_INS_VHADD: vhadd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDsv8i8, ARM_INS_VHADD: vhadd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDuv16i8, ARM_INS_VHADD: vhadd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDuv2i32, ARM_INS_VHADD: vhadd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDuv4i16, ARM_INS_VHADD: vhadd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDuv4i32, ARM_INS_VHADD: vhadd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDuv8i16, ARM_INS_VHADD: vhadd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHADDuv8i8, ARM_INS_VHADD: vhadd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBsv16i8, ARM_INS_VHSUB: vhsub${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBsv2i32, ARM_INS_VHSUB: vhsub${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBsv4i16, ARM_INS_VHSUB: vhsub${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBsv4i32, ARM_INS_VHSUB: vhsub${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBsv8i16, ARM_INS_VHSUB: vhsub${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBsv8i8, ARM_INS_VHSUB: vhsub${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBuv16i8, ARM_INS_VHSUB: vhsub${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBuv2i32, ARM_INS_VHSUB: vhsub${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBuv4i16, ARM_INS_VHSUB: vhsub${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBuv4i32, ARM_INS_VHSUB: vhsub${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBuv8i16, ARM_INS_VHSUB: vhsub${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VHSUBuv8i8, ARM_INS_VHSUB: vhsub${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd16, ARM_INS_VLD1: vld1${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd16wb_fixed, ARM_INS_VLD1: vld1${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd16wb_register, ARM_INS_VLD1: vld1${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd32, ARM_INS_VLD1: vld1${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd32wb_fixed, ARM_INS_VLD1: vld1${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd32wb_register, ARM_INS_VLD1: vld1${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd8, ARM_INS_VLD1: vld1${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd8wb_fixed, ARM_INS_VLD1: vld1${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPd8wb_register, ARM_INS_VLD1: vld1${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq16, ARM_INS_VLD1: vld1${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq16wb_fixed, ARM_INS_VLD1: vld1${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq16wb_register, ARM_INS_VLD1: vld1${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq32, ARM_INS_VLD1: vld1${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq32wb_fixed, ARM_INS_VLD1: vld1${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq32wb_register, ARM_INS_VLD1: vld1${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq8, ARM_INS_VLD1: vld1${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq8wb_fixed, ARM_INS_VLD1: vld1${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1DUPq8wb_register, ARM_INS_VLD1: vld1${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1LNd16, ARM_INS_VLD1: vld1${p}.16	\{$vd[$lane]\}, $rn */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1LNd16_UPD, ARM_INS_VLD1: vld1${p}.16	\{$vd[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VLD1LNd32, ARM_INS_VLD1: vld1${p}.32	\{$vd[$lane]\}, $rn */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VLD1LNd32_UPD, ARM_INS_VLD1: vld1${p}.32	\{$vd[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VLD1LNd8, ARM_INS_VLD1: vld1${p}.8	\{$vd[$lane]\}, $rn */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1LNd8_UPD, ARM_INS_VLD1: vld1${p}.8	\{$vd[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VLD1d16, ARM_INS_VLD1: vld1${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16Q, ARM_INS_VLD1: vld1${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16Qwb_fixed, ARM_INS_VLD1: vld1${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16Qwb_register, ARM_INS_VLD1: vld1${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16T, ARM_INS_VLD1: vld1${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16Twb_fixed, ARM_INS_VLD1: vld1${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16Twb_register, ARM_INS_VLD1: vld1${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16wb_fixed, ARM_INS_VLD1: vld1${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d16wb_register, ARM_INS_VLD1: vld1${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32, ARM_INS_VLD1: vld1${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32Q, ARM_INS_VLD1: vld1${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32Qwb_fixed, ARM_INS_VLD1: vld1${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32Qwb_register, ARM_INS_VLD1: vld1${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32T, ARM_INS_VLD1: vld1${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32Twb_fixed, ARM_INS_VLD1: vld1${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32Twb_register, ARM_INS_VLD1: vld1${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32wb_fixed, ARM_INS_VLD1: vld1${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d32wb_register, ARM_INS_VLD1: vld1${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64, ARM_INS_VLD1: vld1${p}.64	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64Q, ARM_INS_VLD1: vld1${p}.64	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64Qwb_fixed, ARM_INS_VLD1: vld1${p}.64	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64Qwb_register, ARM_INS_VLD1: vld1${p}.64	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64T, ARM_INS_VLD1: vld1${p}.64	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64Twb_fixed, ARM_INS_VLD1: vld1${p}.64	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64Twb_register, ARM_INS_VLD1: vld1${p}.64	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64wb_fixed, ARM_INS_VLD1: vld1${p}.64	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d64wb_register, ARM_INS_VLD1: vld1${p}.64	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8, ARM_INS_VLD1: vld1${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8Q, ARM_INS_VLD1: vld1${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8Qwb_fixed, ARM_INS_VLD1: vld1${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8Qwb_register, ARM_INS_VLD1: vld1${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8T, ARM_INS_VLD1: vld1${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8Twb_fixed, ARM_INS_VLD1: vld1${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8Twb_register, ARM_INS_VLD1: vld1${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8wb_fixed, ARM_INS_VLD1: vld1${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1d8wb_register, ARM_INS_VLD1: vld1${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q16, ARM_INS_VLD1: vld1${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q16wb_fixed, ARM_INS_VLD1: vld1${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q16wb_register, ARM_INS_VLD1: vld1${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q32, ARM_INS_VLD1: vld1${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q32wb_fixed, ARM_INS_VLD1: vld1${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q32wb_register, ARM_INS_VLD1: vld1${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q64, ARM_INS_VLD1: vld1${p}.64	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q64wb_fixed, ARM_INS_VLD1: vld1${p}.64	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q64wb_register, ARM_INS_VLD1: vld1${p}.64	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q8, ARM_INS_VLD1: vld1${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q8wb_fixed, ARM_INS_VLD1: vld1${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD1q8wb_register, ARM_INS_VLD1: vld1${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd16, ARM_INS_VLD2: vld2${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd16wb_fixed, ARM_INS_VLD2: vld2${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd16wb_register, ARM_INS_VLD2: vld2${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd16x2, ARM_INS_VLD2: vld2${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd16x2wb_fixed, ARM_INS_VLD2: vld2${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd16x2wb_register, ARM_INS_VLD2: vld2${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd32, ARM_INS_VLD2: vld2${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd32wb_fixed, ARM_INS_VLD2: vld2${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd32wb_register, ARM_INS_VLD2: vld2${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd32x2, ARM_INS_VLD2: vld2${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd32x2wb_fixed, ARM_INS_VLD2: vld2${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd32x2wb_register, ARM_INS_VLD2: vld2${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd8, ARM_INS_VLD2: vld2${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd8wb_fixed, ARM_INS_VLD2: vld2${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd8wb_register, ARM_INS_VLD2: vld2${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd8x2, ARM_INS_VLD2: vld2${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd8x2wb_fixed, ARM_INS_VLD2: vld2${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2DUPd8x2wb_register, ARM_INS_VLD2: vld2${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2LNd16, ARM_INS_VLD2: vld2${p}.16	\{$vd[$lane], $dst2[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2LNd16_UPD, ARM_INS_VLD2: vld2${p}.16	\{$vd[$lane], $dst2[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD2LNd32, ARM_INS_VLD2: vld2${p}.32	\{$vd[$lane], $dst2[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2LNd32_UPD, ARM_INS_VLD2: vld2${p}.32	\{$vd[$lane], $dst2[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD2LNd8, ARM_INS_VLD2: vld2${p}.8	\{$vd[$lane], $dst2[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2LNd8_UPD, ARM_INS_VLD2: vld2${p}.8	\{$vd[$lane], $dst2[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD2LNq16, ARM_INS_VLD2: vld2${p}.16	\{$vd[$lane], $dst2[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2LNq16_UPD, ARM_INS_VLD2: vld2${p}.16	\{$vd[$lane], $dst2[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD2LNq32, ARM_INS_VLD2: vld2${p}.32	\{$vd[$lane], $dst2[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2LNq32_UPD, ARM_INS_VLD2: vld2${p}.32	\{$vd[$lane], $dst2[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD2b16, ARM_INS_VLD2: vld2${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b16wb_fixed, ARM_INS_VLD2: vld2${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b16wb_register, ARM_INS_VLD2: vld2${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b32, ARM_INS_VLD2: vld2${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b32wb_fixed, ARM_INS_VLD2: vld2${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b32wb_register, ARM_INS_VLD2: vld2${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b8, ARM_INS_VLD2: vld2${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b8wb_fixed, ARM_INS_VLD2: vld2${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2b8wb_register, ARM_INS_VLD2: vld2${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d16, ARM_INS_VLD2: vld2${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d16wb_fixed, ARM_INS_VLD2: vld2${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d16wb_register, ARM_INS_VLD2: vld2${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d32, ARM_INS_VLD2: vld2${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d32wb_fixed, ARM_INS_VLD2: vld2${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d32wb_register, ARM_INS_VLD2: vld2${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d8, ARM_INS_VLD2: vld2${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d8wb_fixed, ARM_INS_VLD2: vld2${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2d8wb_register, ARM_INS_VLD2: vld2${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q16, ARM_INS_VLD2: vld2${p}.16	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q16wb_fixed, ARM_INS_VLD2: vld2${p}.16	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q16wb_register, ARM_INS_VLD2: vld2${p}.16	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q32, ARM_INS_VLD2: vld2${p}.32	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q32wb_fixed, ARM_INS_VLD2: vld2${p}.32	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q32wb_register, ARM_INS_VLD2: vld2${p}.32	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q8, ARM_INS_VLD2: vld2${p}.8	$vd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q8wb_fixed, ARM_INS_VLD2: vld2${p}.8	$vd, $rn! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD2q8wb_register, ARM_INS_VLD2: vld2${p}.8	$vd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPd16, ARM_INS_VLD3: vld3${p}.16	\{$vd[], $dst2[], $dst3[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPd16_UPD, ARM_INS_VLD3: vld3${p}.16	\{$vd[], $dst2[], $dst3[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPd32, ARM_INS_VLD3: vld3${p}.32	\{$vd[], $dst2[], $dst3[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPd32_UPD, ARM_INS_VLD3: vld3${p}.32	\{$vd[], $dst2[], $dst3[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPd8, ARM_INS_VLD3: vld3${p}.8	\{$vd[], $dst2[], $dst3[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPd8_UPD, ARM_INS_VLD3: vld3${p}.8	\{$vd[], $dst2[], $dst3[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPq16, ARM_INS_VLD3: vld3${p}.16	\{$vd[], $dst2[], $dst3[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPq16_UPD, ARM_INS_VLD3: vld3${p}.16	\{$vd[], $dst2[], $dst3[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPq32, ARM_INS_VLD3: vld3${p}.32	\{$vd[], $dst2[], $dst3[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPq32_UPD, ARM_INS_VLD3: vld3${p}.32	\{$vd[], $dst2[], $dst3[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPq8, ARM_INS_VLD3: vld3${p}.8	\{$vd[], $dst2[], $dst3[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3DUPq8_UPD, ARM_INS_VLD3: vld3${p}.8	\{$vd[], $dst2[], $dst3[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3LNd16, ARM_INS_VLD3: vld3${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3LNd16_UPD, ARM_INS_VLD3: vld3${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3LNd32, ARM_INS_VLD3: vld3${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3LNd32_UPD, ARM_INS_VLD3: vld3${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3LNd8, ARM_INS_VLD3: vld3${p}.8	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3LNd8_UPD, ARM_INS_VLD3: vld3${p}.8	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3LNq16, ARM_INS_VLD3: vld3${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3LNq16_UPD, ARM_INS_VLD3: vld3${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3LNq32, ARM_INS_VLD3: vld3${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3LNq32_UPD, ARM_INS_VLD3: vld3${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3d16, ARM_INS_VLD3: vld3${p}.16	\{$vd, $dst2, $dst3\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3d16_UPD, ARM_INS_VLD3: vld3${p}.16	\{$vd, $dst2, $dst3\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3d32, ARM_INS_VLD3: vld3${p}.32	\{$vd, $dst2, $dst3\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3d32_UPD, ARM_INS_VLD3: vld3${p}.32	\{$vd, $dst2, $dst3\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3d8, ARM_INS_VLD3: vld3${p}.8	\{$vd, $dst2, $dst3\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3d8_UPD, ARM_INS_VLD3: vld3${p}.8	\{$vd, $dst2, $dst3\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3q16, ARM_INS_VLD3: vld3${p}.16	\{$vd, $dst2, $dst3\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3q16_UPD, ARM_INS_VLD3: vld3${p}.16	\{$vd, $dst2, $dst3\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3q32, ARM_INS_VLD3: vld3${p}.32	\{$vd, $dst2, $dst3\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3q32_UPD, ARM_INS_VLD3: vld3${p}.32	\{$vd, $dst2, $dst3\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD3q8, ARM_INS_VLD3: vld3${p}.8	\{$vd, $dst2, $dst3\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD3q8_UPD, ARM_INS_VLD3: vld3${p}.8	\{$vd, $dst2, $dst3\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4DUPd16, ARM_INS_VLD4: vld4${p}.16	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPd16_UPD, ARM_INS_VLD4: vld4${p}.16	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPd32, ARM_INS_VLD4: vld4${p}.32	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPd32_UPD, ARM_INS_VLD4: vld4${p}.32	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPd8, ARM_INS_VLD4: vld4${p}.8	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPd8_UPD, ARM_INS_VLD4: vld4${p}.8	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPq16, ARM_INS_VLD4: vld4${p}.16	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPq16_UPD, ARM_INS_VLD4: vld4${p}.16	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPq32, ARM_INS_VLD4: vld4${p}.32	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPq32_UPD, ARM_INS_VLD4: vld4${p}.32	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPq8, ARM_INS_VLD4: vld4${p}.8	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4DUPq8_UPD, ARM_INS_VLD4: vld4${p}.8	\{$vd[], $dst2[], $dst3[], $dst4[]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4LNd16, ARM_INS_VLD4: vld4${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4LNd16_UPD, ARM_INS_VLD4: vld4${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4LNd32, ARM_INS_VLD4: vld4${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4LNd32_UPD, ARM_INS_VLD4: vld4${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4LNd8, ARM_INS_VLD4: vld4${p}.8	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4LNd8_UPD, ARM_INS_VLD4: vld4${p}.8	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4LNq16, ARM_INS_VLD4: vld4${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4LNq16_UPD, ARM_INS_VLD4: vld4${p}.16	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4LNq32, ARM_INS_VLD4: vld4${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4LNq32_UPD, ARM_INS_VLD4: vld4${p}.32	\{$vd[$lane], $dst2[$lane], $dst3[$lane], $dst4[$lane]\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4d16, ARM_INS_VLD4: vld4${p}.16	\{$vd, $dst2, $dst3, $dst4\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4d16_UPD, ARM_INS_VLD4: vld4${p}.16	\{$vd, $dst2, $dst3, $dst4\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4d32, ARM_INS_VLD4: vld4${p}.32	\{$vd, $dst2, $dst3, $dst4\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4d32_UPD, ARM_INS_VLD4: vld4${p}.32	\{$vd, $dst2, $dst3, $dst4\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4d8, ARM_INS_VLD4: vld4${p}.8	\{$vd, $dst2, $dst3, $dst4\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4d8_UPD, ARM_INS_VLD4: vld4${p}.8	\{$vd, $dst2, $dst3, $dst4\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4q16, ARM_INS_VLD4: vld4${p}.16	\{$vd, $dst2, $dst3, $dst4\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4q16_UPD, ARM_INS_VLD4: vld4${p}.16	\{$vd, $dst2, $dst3, $dst4\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4q32, ARM_INS_VLD4: vld4${p}.32	\{$vd, $dst2, $dst3, $dst4\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4q32_UPD, ARM_INS_VLD4: vld4${p}.32	\{$vd, $dst2, $dst3, $dst4\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLD4q8, ARM_INS_VLD4: vld4${p}.8	\{$vd, $dst2, $dst3, $dst4\}, $rn */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VLD4q8_UPD, ARM_INS_VLD4: vld4${p}.8	\{$vd, $dst2, $dst3, $dst4\}, $rn$rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VLDMDDB_UPD, ARM_INS_VLDMDB: vldmdb${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VLDMDIA, ARM_INS_VLDMIA: vldmia${p}	$rn, $regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VLDMDIA_UPD, ARM_INS_VLDMIA: vldmia${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VLDMSDB_UPD, ARM_INS_VLDMDB: vldmdb${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VLDMSIA, ARM_INS_VLDMIA: vldmia${p}	$rn, $regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VLDMSIA_UPD, ARM_INS_VLDMIA: vldmia${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VLDRD, ARM_INS_VLDR: vldr${p}	$dd, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VLDRS, ARM_INS_VLDR: vldr${p}	$sd, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMAXNMD, ARM_INS_VMAXNM: vmaxnm.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXNMND, ARM_INS_VMAXNM: vmaxnm.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXNMNQ, ARM_INS_VMAXNM: vmaxnm.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXNMS, ARM_INS_VMAXNM: vmaxnm.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXfd, ARM_INS_VMAX: vmax${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXfq, ARM_INS_VMAX: vmax${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXsv16i8, ARM_INS_VMAX: vmax${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXsv2i32, ARM_INS_VMAX: vmax${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXsv4i16, ARM_INS_VMAX: vmax${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXsv4i32, ARM_INS_VMAX: vmax${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXsv8i16, ARM_INS_VMAX: vmax${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXsv8i8, ARM_INS_VMAX: vmax${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXuv16i8, ARM_INS_VMAX: vmax${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXuv2i32, ARM_INS_VMAX: vmax${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXuv4i16, ARM_INS_VMAX: vmax${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXuv4i32, ARM_INS_VMAX: vmax${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXuv8i16, ARM_INS_VMAX: vmax${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMAXuv8i8, ARM_INS_VMAX: vmax${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINNMD, ARM_INS_VMINNM: vminnm.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINNMND, ARM_INS_VMINNM: vminnm.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINNMNQ, ARM_INS_VMINNM: vminnm.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINNMS, ARM_INS_VMINNM: vminnm.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINfd, ARM_INS_VMIN: vmin${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINfq, ARM_INS_VMIN: vmin${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINsv16i8, ARM_INS_VMIN: vmin${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINsv2i32, ARM_INS_VMIN: vmin${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINsv4i16, ARM_INS_VMIN: vmin${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINsv4i32, ARM_INS_VMIN: vmin${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINsv8i16, ARM_INS_VMIN: vmin${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINsv8i8, ARM_INS_VMIN: vmin${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINuv16i8, ARM_INS_VMIN: vmin${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINuv2i32, ARM_INS_VMIN: vmin${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINuv4i16, ARM_INS_VMIN: vmin${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINuv4i32, ARM_INS_VMIN: vmin${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINuv8i16, ARM_INS_VMIN: vmin${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMINuv8i8, ARM_INS_VMIN: vmin${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAD, ARM_INS_VMLA: vmla${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLALslsv2i32, ARM_INS_VMLAL: vmlal${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLALslsv4i16, ARM_INS_VMLAL: vmlal${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLALsluv2i32, ARM_INS_VMLAL: vmlal${p}.u32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLALsluv4i16, ARM_INS_VMLAL: vmlal${p}.u16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLALsv2i64, ARM_INS_VMLAL: vmlal${p}.s32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLALsv4i32, ARM_INS_VMLAL: vmlal${p}.s16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLALsv8i16, ARM_INS_VMLAL: vmlal${p}.s8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLALuv2i64, ARM_INS_VMLAL: vmlal${p}.u32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLALuv4i32, ARM_INS_VMLAL: vmlal${p}.u16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLALuv8i16, ARM_INS_VMLAL: vmlal${p}.u8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAS, ARM_INS_VMLA: vmla${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAfd, ARM_INS_VMLA: vmla${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAfq, ARM_INS_VMLA: vmla${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAslfd, ARM_INS_VMLA: vmla${p}.f32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLAslfq, ARM_INS_VMLA: vmla${p}.f32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLAslv2i32, ARM_INS_VMLA: vmla${p}.i32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLAslv4i16, ARM_INS_VMLA: vmla${p}.i16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLAslv4i32, ARM_INS_VMLA: vmla${p}.i32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLAslv8i16, ARM_INS_VMLA: vmla${p}.i16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLAv16i8, ARM_INS_VMLA: vmla${p}.i8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAv2i32, ARM_INS_VMLA: vmla${p}.i32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAv4i16, ARM_INS_VMLA: vmla${p}.i16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAv4i32, ARM_INS_VMLA: vmla${p}.i32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAv8i16, ARM_INS_VMLA: vmla${p}.i16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLAv8i8, ARM_INS_VMLA: vmla${p}.i8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSD, ARM_INS_VMLS: vmls${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLslsv2i32, ARM_INS_VMLSL: vmlsl${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLslsv4i16, ARM_INS_VMLSL: vmlsl${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLsluv2i32, ARM_INS_VMLSL: vmlsl${p}.u32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLsluv4i16, ARM_INS_VMLSL: vmlsl${p}.u16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLsv2i64, ARM_INS_VMLSL: vmlsl${p}.s32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLsv4i32, ARM_INS_VMLSL: vmlsl${p}.s16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLsv8i16, ARM_INS_VMLSL: vmlsl${p}.s8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLuv2i64, ARM_INS_VMLSL: vmlsl${p}.u32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLuv4i32, ARM_INS_VMLSL: vmlsl${p}.u16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSLuv8i16, ARM_INS_VMLSL: vmlsl${p}.u8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSS, ARM_INS_VMLS: vmls${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSfd, ARM_INS_VMLS: vmls${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSfq, ARM_INS_VMLS: vmls${p}.f32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSslfd, ARM_INS_VMLS: vmls${p}.f32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSslfq, ARM_INS_VMLS: vmls${p}.f32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSslv2i32, ARM_INS_VMLS: vmls${p}.i32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSslv4i16, ARM_INS_VMLS: vmls${p}.i16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSslv4i32, ARM_INS_VMLS: vmls${p}.i32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSslv8i16, ARM_INS_VMLS: vmls${p}.i16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMLSv16i8, ARM_INS_VMLS: vmls${p}.i8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSv2i32, ARM_INS_VMLS: vmls${p}.i32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSv4i16, ARM_INS_VMLS: vmls${p}.i16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSv4i32, ARM_INS_VMLS: vmls${p}.i32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSv8i16, ARM_INS_VMLS: vmls${p}.i16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMLSv8i8, ARM_INS_VMLS: vmls${p}.i8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMOVD, ARM_INS_VMOV: vmov${p}.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVDRR, ARM_INS_VMOV: vmov${p}	$dm, $rt, $rt2 */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMOVLsv2i64, ARM_INS_VMOVL: vmovl${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVLsv4i32, ARM_INS_VMOVL: vmovl${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVLsv8i16, ARM_INS_VMOVL: vmovl${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVLuv2i64, ARM_INS_VMOVL: vmovl${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVLuv4i32, ARM_INS_VMOVL: vmovl${p}.u16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVLuv8i16, ARM_INS_VMOVL: vmovl${p}.u8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVNv2i32, ARM_INS_VMOVN: vmovn${p}.i64	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVNv4i16, ARM_INS_VMOVN: vmovn${p}.i32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVNv8i8, ARM_INS_VMOVN: vmovn${p}.i16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVRRD, ARM_INS_VMOV: vmov${p}	$rt, $rt2, $dm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVRRS, ARM_INS_VMOV: vmov${p}	$rt, $rt2, $src1, $src2 */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMOVRS, ARM_INS_VMOV: vmov${p}	$rt, $sn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVS, ARM_INS_VMOV: vmov${p}.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVSR, ARM_INS_VMOV: vmov${p}	$sn, $rt */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMOVSRR, ARM_INS_VMOV: vmov${p}	$dst1, $dst2, $src1, $src2 */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMOVv16i8, ARM_INS_VMOV: vmov${p}.i8	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv1i64, ARM_INS_VMOV: vmov${p}.i64	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv2f32, ARM_INS_VMOV: vmov${p}.f32	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv2i32, ARM_INS_VMOV: vmov${p}.i32	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv2i64, ARM_INS_VMOV: vmov${p}.i64	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv4f32, ARM_INS_VMOV: vmov${p}.f32	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv4i16, ARM_INS_VMOV: vmov${p}.i16	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv4i32, ARM_INS_VMOV: vmov${p}.i32	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv8i16, ARM_INS_VMOV: vmov${p}.i16	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMOVv8i8, ARM_INS_VMOV: vmov${p}.i8	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS, ARM_INS_VMRS: vmrs${p}	$rt, fpscr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS_FPEXC, ARM_INS_VMRS: vmrs${p}	$rt, fpexc */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS_FPINST, ARM_INS_VMRS: vmrs${p}	$rt, fpinst */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS_FPINST2, ARM_INS_VMRS: vmrs${p}	$rt, fpinst2 */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS_FPSID, ARM_INS_VMRS: vmrs${p}	$rt, fpsid */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS_MVFR0, ARM_INS_VMRS: vmrs${p}	$rt, mvfr0 */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS_MVFR1, ARM_INS_VMRS: vmrs${p}	$rt, mvfr1 */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMRS_MVFR2, ARM_INS_VMRS: vmrs${p}	$rt, mvfr2 */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMSR, ARM_INS_VMSR: vmsr${p}	fpscr, $src */
	{ CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_VMSR_FPEXC, ARM_INS_VMSR: vmsr${p}	fpexc, $src */
	{ CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_VMSR_FPINST, ARM_INS_VMSR: vmsr${p}	fpinst, $src */
	{ CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_VMSR_FPINST2, ARM_INS_VMSR: vmsr${p}	fpinst2, $src */
	{ CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_VMSR_FPSID, ARM_INS_VMSR: vmsr${p}	fpsid, $src */
	{ CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_VMULD, ARM_INS_VMUL: vmul${p}.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLp64, ARM_INS_VMULL: vmull.p64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLp8, ARM_INS_VMULL: vmull${p}.p8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLslsv2i32, ARM_INS_VMULL: vmull${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULLslsv4i16, ARM_INS_VMULL: vmull${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULLsluv2i32, ARM_INS_VMULL: vmull${p}.u32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULLsluv4i16, ARM_INS_VMULL: vmull${p}.u16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULLsv2i64, ARM_INS_VMULL: vmull${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLsv4i32, ARM_INS_VMULL: vmull${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLsv8i16, ARM_INS_VMULL: vmull${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLuv2i64, ARM_INS_VMULL: vmull${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLuv4i32, ARM_INS_VMULL: vmull${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULLuv8i16, ARM_INS_VMULL: vmull${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULS, ARM_INS_VMUL: vmul${p}.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULfd, ARM_INS_VMUL: vmul${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULfq, ARM_INS_VMUL: vmul${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULpd, ARM_INS_VMUL: vmul${p}.p8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULpq, ARM_INS_VMUL: vmul${p}.p8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULslfd, ARM_INS_VMUL: vmul${p}.f32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULslfq, ARM_INS_VMUL: vmul${p}.f32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULslv2i32, ARM_INS_VMUL: vmul${p}.i32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULslv4i16, ARM_INS_VMUL: vmul${p}.i16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULslv4i32, ARM_INS_VMUL: vmul${p}.i32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULslv8i16, ARM_INS_VMUL: vmul${p}.i16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMULv16i8, ARM_INS_VMUL: vmul${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULv2i32, ARM_INS_VMUL: vmul${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULv4i16, ARM_INS_VMUL: vmul${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULv4i32, ARM_INS_VMUL: vmul${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULv8i16, ARM_INS_VMUL: vmul${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMULv8i8, ARM_INS_VMUL: vmul${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VMVNd, ARM_INS_VMVN: vmvn${p}	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMVNq, ARM_INS_VMVN: vmvn${p}	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VMVNv2i32, ARM_INS_VMVN: vmvn${p}.i32	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMVNv4i16, ARM_INS_VMVN: vmvn${p}.i16	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMVNv4i32, ARM_INS_VMVN: vmvn${p}.i32	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VMVNv8i16, ARM_INS_VMVN: vmvn${p}.i16	$vd, $simm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_VNEGD, ARM_INS_VNEG: vneg${p}.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGS, ARM_INS_VNEG: vneg${p}.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGf32q, ARM_INS_VNEG: vneg${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGfd, ARM_INS_VNEG: vneg${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGs16d, ARM_INS_VNEG: vneg${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGs16q, ARM_INS_VNEG: vneg${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGs32d, ARM_INS_VNEG: vneg${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGs32q, ARM_INS_VNEG: vneg${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGs8d, ARM_INS_VNEG: vneg${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNEGs8q, ARM_INS_VNEG: vneg${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VNMLAD, ARM_INS_VNMLA: vnmla${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VNMLAS, ARM_INS_VNMLA: vnmla${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VNMLSD, ARM_INS_VNMLS: vnmls${p}.f64	$dd, $dn, $dm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VNMLSS, ARM_INS_VNMLS: vnmls${p}.f32	$sd, $sn, $sm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VNMULD, ARM_INS_VNMUL: vnmul${p}.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VNMULS, ARM_INS_VNMUL: vnmul${p}.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VORNd, ARM_INS_VORN: vorn${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VORNq, ARM_INS_VORN: vorn${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VORRd, ARM_INS_VORR: vorr${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VORRiv2i32, ARM_INS_VORR: vorr${p}.i32	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VORRiv4i16, ARM_INS_VORR: vorr${p}.i16	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VORRiv4i32, ARM_INS_VORR: vorr${p}.i32	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VORRiv8i16, ARM_INS_VORR: vorr${p}.i16	$vd, $simm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VORRq, ARM_INS_VORR: vorr${p}	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPADALsv16i8, ARM_INS_VPADAL: vpadal${p}.s8	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALsv2i32, ARM_INS_VPADAL: vpadal${p}.s32	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALsv4i16, ARM_INS_VPADAL: vpadal${p}.s16	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALsv4i32, ARM_INS_VPADAL: vpadal${p}.s32	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALsv8i16, ARM_INS_VPADAL: vpadal${p}.s16	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALsv8i8, ARM_INS_VPADAL: vpadal${p}.s8	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALuv16i8, ARM_INS_VPADAL: vpadal${p}.u8	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALuv2i32, ARM_INS_VPADAL: vpadal${p}.u32	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALuv4i16, ARM_INS_VPADAL: vpadal${p}.u16	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALuv4i32, ARM_INS_VPADAL: vpadal${p}.u32	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALuv8i16, ARM_INS_VPADAL: vpadal${p}.u16	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADALuv8i8, ARM_INS_VPADAL: vpadal${p}.u8	$vd, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLsv16i8, ARM_INS_VPADDL: vpaddl${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLsv2i32, ARM_INS_VPADDL: vpaddl${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLsv4i16, ARM_INS_VPADDL: vpaddl${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLsv4i32, ARM_INS_VPADDL: vpaddl${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLsv8i16, ARM_INS_VPADDL: vpaddl${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLsv8i8, ARM_INS_VPADDL: vpaddl${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLuv16i8, ARM_INS_VPADDL: vpaddl${p}.u8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLuv2i32, ARM_INS_VPADDL: vpaddl${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLuv4i16, ARM_INS_VPADDL: vpaddl${p}.u16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLuv4i32, ARM_INS_VPADDL: vpaddl${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLuv8i16, ARM_INS_VPADDL: vpaddl${p}.u16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDLuv8i8, ARM_INS_VPADDL: vpaddl${p}.u8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VPADDf, ARM_INS_VPADD: vpadd${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPADDi16, ARM_INS_VPADD: vpadd${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPADDi32, ARM_INS_VPADD: vpadd${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPADDi8, ARM_INS_VPADD: vpadd${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMAXf, ARM_INS_VPMAX: vpmax${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMAXs16, ARM_INS_VPMAX: vpmax${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMAXs32, ARM_INS_VPMAX: vpmax${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMAXs8, ARM_INS_VPMAX: vpmax${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMAXu16, ARM_INS_VPMAX: vpmax${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMAXu32, ARM_INS_VPMAX: vpmax${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMAXu8, ARM_INS_VPMAX: vpmax${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMINf, ARM_INS_VPMIN: vpmin${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMINs16, ARM_INS_VPMIN: vpmin${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMINs32, ARM_INS_VPMIN: vpmin${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMINs8, ARM_INS_VPMIN: vpmin${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMINu16, ARM_INS_VPMIN: vpmin${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMINu32, ARM_INS_VPMIN: vpmin${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VPMINu8, ARM_INS_VPMIN: vpmin${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQABSv16i8, ARM_INS_VQABS: vqabs${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQABSv2i32, ARM_INS_VQABS: vqabs${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQABSv4i16, ARM_INS_VQABS: vqabs${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQABSv4i32, ARM_INS_VQABS: vqabs${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQABSv8i16, ARM_INS_VQABS: vqabs${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQABSv8i8, ARM_INS_VQABS: vqabs${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv16i8, ARM_INS_VQADD: vqadd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv1i64, ARM_INS_VQADD: vqadd${p}.s64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv2i32, ARM_INS_VQADD: vqadd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv2i64, ARM_INS_VQADD: vqadd${p}.s64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv4i16, ARM_INS_VQADD: vqadd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv4i32, ARM_INS_VQADD: vqadd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv8i16, ARM_INS_VQADD: vqadd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDsv8i8, ARM_INS_VQADD: vqadd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv16i8, ARM_INS_VQADD: vqadd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv1i64, ARM_INS_VQADD: vqadd${p}.u64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv2i32, ARM_INS_VQADD: vqadd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv2i64, ARM_INS_VQADD: vqadd${p}.u64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv4i16, ARM_INS_VQADD: vqadd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv4i32, ARM_INS_VQADD: vqadd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv8i16, ARM_INS_VQADD: vqadd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQADDuv8i8, ARM_INS_VQADD: vqadd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLALslv2i32, ARM_INS_VQDMLAL: vqdmlal${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLALslv4i16, ARM_INS_VQDMLAL: vqdmlal${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLALv2i64, ARM_INS_VQDMLAL: vqdmlal${p}.s32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLALv4i32, ARM_INS_VQDMLAL: vqdmlal${p}.s16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLSLslv2i32, ARM_INS_VQDMLSL: vqdmlsl${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLSLslv4i16, ARM_INS_VQDMLSL: vqdmlsl${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLSLv2i64, ARM_INS_VQDMLSL: vqdmlsl${p}.s32	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMLSLv4i32, ARM_INS_VQDMLSL: vqdmlsl${p}.s16	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHslv2i32, ARM_INS_VQDMULH: vqdmulh${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHslv4i16, ARM_INS_VQDMULH: vqdmulh${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHslv4i32, ARM_INS_VQDMULH: vqdmulh${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHslv8i16, ARM_INS_VQDMULH: vqdmulh${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHv2i32, ARM_INS_VQDMULH: vqdmulh${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHv4i16, ARM_INS_VQDMULH: vqdmulh${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHv4i32, ARM_INS_VQDMULH: vqdmulh${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULHv8i16, ARM_INS_VQDMULH: vqdmulh${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULLslv2i32, ARM_INS_VQDMULL: vqdmull${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULLslv4i16, ARM_INS_VQDMULL: vqdmull${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULLv2i64, ARM_INS_VQDMULL: vqdmull${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQDMULLv4i32, ARM_INS_VQDMULL: vqdmull${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNsuv2i32, ARM_INS_VQMOVUN: vqmovun${p}.s64	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNsuv4i16, ARM_INS_VQMOVUN: vqmovun${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNsuv8i8, ARM_INS_VQMOVUN: vqmovun${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNsv2i32, ARM_INS_VQMOVN: vqmovn${p}.s64	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNsv4i16, ARM_INS_VQMOVN: vqmovn${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNsv8i8, ARM_INS_VQMOVN: vqmovn${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNuv2i32, ARM_INS_VQMOVN: vqmovn${p}.u64	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNuv4i16, ARM_INS_VQMOVN: vqmovn${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQMOVNuv8i8, ARM_INS_VQMOVN: vqmovn${p}.u16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQNEGv16i8, ARM_INS_VQNEG: vqneg${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQNEGv2i32, ARM_INS_VQNEG: vqneg${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQNEGv4i16, ARM_INS_VQNEG: vqneg${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQNEGv4i32, ARM_INS_VQNEG: vqneg${p}.s32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQNEGv8i16, ARM_INS_VQNEG: vqneg${p}.s16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQNEGv8i8, ARM_INS_VQNEG: vqneg${p}.s8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHslv2i32, ARM_INS_VQRDMULH: vqrdmulh${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHslv4i16, ARM_INS_VQRDMULH: vqrdmulh${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHslv4i32, ARM_INS_VQRDMULH: vqrdmulh${p}.s32	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHslv8i16, ARM_INS_VQRDMULH: vqrdmulh${p}.s16	$vd, $vn, $vm$lane */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHv2i32, ARM_INS_VQRDMULH: vqrdmulh${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHv4i16, ARM_INS_VQRDMULH: vqrdmulh${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHv4i32, ARM_INS_VQRDMULH: vqrdmulh${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRDMULHv8i16, ARM_INS_VQRDMULH: vqrdmulh${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv16i8, ARM_INS_VQRSHL: vqrshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv1i64, ARM_INS_VQRSHL: vqrshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv2i32, ARM_INS_VQRSHL: vqrshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv2i64, ARM_INS_VQRSHL: vqrshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv4i16, ARM_INS_VQRSHL: vqrshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv4i32, ARM_INS_VQRSHL: vqrshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv8i16, ARM_INS_VQRSHL: vqrshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLsv8i8, ARM_INS_VQRSHL: vqrshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv16i8, ARM_INS_VQRSHL: vqrshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv1i64, ARM_INS_VQRSHL: vqrshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv2i32, ARM_INS_VQRSHL: vqrshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv2i64, ARM_INS_VQRSHL: vqrshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv4i16, ARM_INS_VQRSHL: vqrshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv4i32, ARM_INS_VQRSHL: vqrshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv8i16, ARM_INS_VQRSHL: vqrshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHLuv8i8, ARM_INS_VQRSHL: vqrshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRNsv2i32, ARM_INS_VQRSHRN: vqrshrn${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRNsv4i16, ARM_INS_VQRSHRN: vqrshrn${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRNsv8i8, ARM_INS_VQRSHRN: vqrshrn${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRNuv2i32, ARM_INS_VQRSHRN: vqrshrn${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRNuv4i16, ARM_INS_VQRSHRN: vqrshrn${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRNuv8i8, ARM_INS_VQRSHRN: vqrshrn${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRUNv2i32, ARM_INS_VQRSHRUN: vqrshrun${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRUNv4i16, ARM_INS_VQRSHRUN: vqrshrun${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQRSHRUNv8i8, ARM_INS_VQRSHRUN: vqrshrun${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv16i8, ARM_INS_VQSHL: vqshl${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv1i64, ARM_INS_VQSHL: vqshl${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv2i32, ARM_INS_VQSHL: vqshl${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv2i64, ARM_INS_VQSHL: vqshl${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv4i16, ARM_INS_VQSHL: vqshl${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv4i32, ARM_INS_VQSHL: vqshl${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv8i16, ARM_INS_VQSHL: vqshl${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsiv8i8, ARM_INS_VQSHL: vqshl${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv16i8, ARM_INS_VQSHLU: vqshlu${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv1i64, ARM_INS_VQSHLU: vqshlu${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv2i32, ARM_INS_VQSHLU: vqshlu${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv2i64, ARM_INS_VQSHLU: vqshlu${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv4i16, ARM_INS_VQSHLU: vqshlu${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv4i32, ARM_INS_VQSHLU: vqshlu${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv8i16, ARM_INS_VQSHLU: vqshlu${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsuv8i8, ARM_INS_VQSHLU: vqshlu${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv16i8, ARM_INS_VQSHL: vqshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv1i64, ARM_INS_VQSHL: vqshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv2i32, ARM_INS_VQSHL: vqshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv2i64, ARM_INS_VQSHL: vqshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv4i16, ARM_INS_VQSHL: vqshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv4i32, ARM_INS_VQSHL: vqshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv8i16, ARM_INS_VQSHL: vqshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLsv8i8, ARM_INS_VQSHL: vqshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv16i8, ARM_INS_VQSHL: vqshl${p}.u8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv1i64, ARM_INS_VQSHL: vqshl${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv2i32, ARM_INS_VQSHL: vqshl${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv2i64, ARM_INS_VQSHL: vqshl${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv4i16, ARM_INS_VQSHL: vqshl${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv4i32, ARM_INS_VQSHL: vqshl${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv8i16, ARM_INS_VQSHL: vqshl${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuiv8i8, ARM_INS_VQSHL: vqshl${p}.u8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv16i8, ARM_INS_VQSHL: vqshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv1i64, ARM_INS_VQSHL: vqshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv2i32, ARM_INS_VQSHL: vqshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv2i64, ARM_INS_VQSHL: vqshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv4i16, ARM_INS_VQSHL: vqshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv4i32, ARM_INS_VQSHL: vqshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv8i16, ARM_INS_VQSHL: vqshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHLuv8i8, ARM_INS_VQSHL: vqshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRNsv2i32, ARM_INS_VQSHRN: vqshrn${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRNsv4i16, ARM_INS_VQSHRN: vqshrn${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRNsv8i8, ARM_INS_VQSHRN: vqshrn${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRNuv2i32, ARM_INS_VQSHRN: vqshrn${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRNuv4i16, ARM_INS_VQSHRN: vqshrn${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRNuv8i8, ARM_INS_VQSHRN: vqshrn${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRUNv2i32, ARM_INS_VQSHRUN: vqshrun${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRUNv4i16, ARM_INS_VQSHRUN: vqshrun${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSHRUNv8i8, ARM_INS_VQSHRUN: vqshrun${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv16i8, ARM_INS_VQSUB: vqsub${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv1i64, ARM_INS_VQSUB: vqsub${p}.s64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv2i32, ARM_INS_VQSUB: vqsub${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv2i64, ARM_INS_VQSUB: vqsub${p}.s64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv4i16, ARM_INS_VQSUB: vqsub${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv4i32, ARM_INS_VQSUB: vqsub${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv8i16, ARM_INS_VQSUB: vqsub${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBsv8i8, ARM_INS_VQSUB: vqsub${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv16i8, ARM_INS_VQSUB: vqsub${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv1i64, ARM_INS_VQSUB: vqsub${p}.u64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv2i32, ARM_INS_VQSUB: vqsub${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv2i64, ARM_INS_VQSUB: vqsub${p}.u64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv4i16, ARM_INS_VQSUB: vqsub${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv4i32, ARM_INS_VQSUB: vqsub${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv8i16, ARM_INS_VQSUB: vqsub${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VQSUBuv8i8, ARM_INS_VQSUB: vqsub${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRADDHNv2i32, ARM_INS_VRADDHN: vraddhn${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRADDHNv4i16, ARM_INS_VRADDHN: vraddhn${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRADDHNv8i8, ARM_INS_VRADDHN: vraddhn${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRECPEd, ARM_INS_VRECPE: vrecpe${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRECPEfd, ARM_INS_VRECPE: vrecpe${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRECPEfq, ARM_INS_VRECPE: vrecpe${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRECPEq, ARM_INS_VRECPE: vrecpe${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRECPSfd, ARM_INS_VRECPS: vrecps${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRECPSfq, ARM_INS_VRECPS: vrecps${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VREV16d8, ARM_INS_VREV16: vrev16${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV16q8, ARM_INS_VREV16: vrev16${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV32d16, ARM_INS_VREV32: vrev32${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV32d8, ARM_INS_VREV32: vrev32${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV32q16, ARM_INS_VREV32: vrev32${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV32q8, ARM_INS_VREV32: vrev32${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV64d16, ARM_INS_VREV64: vrev64${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV64d32, ARM_INS_VREV64: vrev64${p}.32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV64d8, ARM_INS_VREV64: vrev64${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV64q16, ARM_INS_VREV64: vrev64${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV64q32, ARM_INS_VREV64: vrev64${p}.32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VREV64q8, ARM_INS_VREV64: vrev64${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDsv16i8, ARM_INS_VRHADD: vrhadd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDsv2i32, ARM_INS_VRHADD: vrhadd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDsv4i16, ARM_INS_VRHADD: vrhadd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDsv4i32, ARM_INS_VRHADD: vrhadd${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDsv8i16, ARM_INS_VRHADD: vrhadd${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDsv8i8, ARM_INS_VRHADD: vrhadd${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDuv16i8, ARM_INS_VRHADD: vrhadd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDuv2i32, ARM_INS_VRHADD: vrhadd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDuv4i16, ARM_INS_VRHADD: vrhadd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDuv4i32, ARM_INS_VRHADD: vrhadd${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDuv8i16, ARM_INS_VRHADD: vrhadd${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRHADDuv8i8, ARM_INS_VRHADD: vrhadd${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRINTAD, ARM_INS_VRINTA: vrinta.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTAND, ARM_INS_VRINTA: vrinta.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTANQ, ARM_INS_VRINTA: vrinta.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTAS, ARM_INS_VRINTA: vrinta.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTMD, ARM_INS_VRINTM: vrintm.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTMND, ARM_INS_VRINTM: vrintm.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTMNQ, ARM_INS_VRINTM: vrintm.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTMS, ARM_INS_VRINTM: vrintm.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTND, ARM_INS_VRINTN: vrintn.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTNND, ARM_INS_VRINTN: vrintn.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTNNQ, ARM_INS_VRINTN: vrintn.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTNS, ARM_INS_VRINTN: vrintn.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTPD, ARM_INS_VRINTP: vrintp.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTPND, ARM_INS_VRINTP: vrintp.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTPNQ, ARM_INS_VRINTP: vrintp.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTPS, ARM_INS_VRINTP: vrintp.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTRD, ARM_INS_VRINTR: vrintr${p}.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTRS, ARM_INS_VRINTR: vrintr${p}.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTXD, ARM_INS_VRINTX: vrintx${p}.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTXND, ARM_INS_VRINTX: vrintx.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTXNQ, ARM_INS_VRINTX: vrintx.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTXS, ARM_INS_VRINTX: vrintx${p}.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTZD, ARM_INS_VRINTZ: vrintz${p}.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTZND, ARM_INS_VRINTZ: vrintz.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTZNQ, ARM_INS_VRINTZ: vrintz.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRINTZS, ARM_INS_VRINTZ: vrintz${p}.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv16i8, ARM_INS_VRSHL: vrshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv1i64, ARM_INS_VRSHL: vrshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv2i32, ARM_INS_VRSHL: vrshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv2i64, ARM_INS_VRSHL: vrshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv4i16, ARM_INS_VRSHL: vrshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv4i32, ARM_INS_VRSHL: vrshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv8i16, ARM_INS_VRSHL: vrshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLsv8i8, ARM_INS_VRSHL: vrshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv16i8, ARM_INS_VRSHL: vrshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv1i64, ARM_INS_VRSHL: vrshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv2i32, ARM_INS_VRSHL: vrshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv2i64, ARM_INS_VRSHL: vrshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv4i16, ARM_INS_VRSHL: vrshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv4i32, ARM_INS_VRSHL: vrshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv8i16, ARM_INS_VRSHL: vrshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHLuv8i8, ARM_INS_VRSHL: vrshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRNv2i32, ARM_INS_VRSHRN: vrshrn${p}.i64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRNv4i16, ARM_INS_VRSHRN: vrshrn${p}.i32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRNv8i8, ARM_INS_VRSHRN: vrshrn${p}.i16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv16i8, ARM_INS_VRSHR: vrshr${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv1i64, ARM_INS_VRSHR: vrshr${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv2i32, ARM_INS_VRSHR: vrshr${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv2i64, ARM_INS_VRSHR: vrshr${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv4i16, ARM_INS_VRSHR: vrshr${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv4i32, ARM_INS_VRSHR: vrshr${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv8i16, ARM_INS_VRSHR: vrshr${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRsv8i8, ARM_INS_VRSHR: vrshr${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv16i8, ARM_INS_VRSHR: vrshr${p}.u8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv1i64, ARM_INS_VRSHR: vrshr${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv2i32, ARM_INS_VRSHR: vrshr${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv2i64, ARM_INS_VRSHR: vrshr${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv4i16, ARM_INS_VRSHR: vrshr${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv4i32, ARM_INS_VRSHR: vrshr${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv8i16, ARM_INS_VRSHR: vrshr${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSHRuv8i8, ARM_INS_VRSHR: vrshr${p}.u8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSQRTEd, ARM_INS_VRSQRTE: vrsqrte${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSQRTEfd, ARM_INS_VRSQRTE: vrsqrte${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSQRTEfq, ARM_INS_VRSQRTE: vrsqrte${p}.f32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSQRTEq, ARM_INS_VRSQRTE: vrsqrte${p}.u32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSQRTSfd, ARM_INS_VRSQRTS: vrsqrts${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSQRTSfq, ARM_INS_VRSQRTS: vrsqrts${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv16i8, ARM_INS_VRSRA: vrsra${p}.s8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv1i64, ARM_INS_VRSRA: vrsra${p}.s64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv2i32, ARM_INS_VRSRA: vrsra${p}.s32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv2i64, ARM_INS_VRSRA: vrsra${p}.s64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv4i16, ARM_INS_VRSRA: vrsra${p}.s16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv4i32, ARM_INS_VRSRA: vrsra${p}.s32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv8i16, ARM_INS_VRSRA: vrsra${p}.s16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAsv8i8, ARM_INS_VRSRA: vrsra${p}.s8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv16i8, ARM_INS_VRSRA: vrsra${p}.u8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv1i64, ARM_INS_VRSRA: vrsra${p}.u64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv2i32, ARM_INS_VRSRA: vrsra${p}.u32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv2i64, ARM_INS_VRSRA: vrsra${p}.u64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv4i16, ARM_INS_VRSRA: vrsra${p}.u16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv4i32, ARM_INS_VRSRA: vrsra${p}.u32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv8i16, ARM_INS_VRSRA: vrsra${p}.u16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSRAuv8i8, ARM_INS_VRSRA: vrsra${p}.u8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VRSUBHNv2i32, ARM_INS_VRSUBHN: vrsubhn${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSUBHNv4i16, ARM_INS_VRSUBHN: vrsubhn${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VRSUBHNv8i8, ARM_INS_VRSUBHN: vrsubhn${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELEQD, ARM_INS_VSELEQ: vseleq.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELEQS, ARM_INS_VSELEQ: vseleq.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELGED, ARM_INS_VSELGE: vselge.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELGES, ARM_INS_VSELGE: vselge.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELGTD, ARM_INS_VSELGT: vselgt.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELGTS, ARM_INS_VSELGT: vselgt.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELVSD, ARM_INS_VSELVS: vselvs.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSELVSS, ARM_INS_VSELVS: vselvs.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSETLNi16, ARM_INS_VMOV: vmov${p}.16	$v$lane, $r */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSETLNi32, ARM_INS_VMOV: vmov${p}.32	$v$lane, $r */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSETLNi8, ARM_INS_VMOV: vmov${p}.8	$v$lane, $r */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLi16, ARM_INS_VSHLL: vshll${p}.i16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLi32, ARM_INS_VSHLL: vshll${p}.i32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLi8, ARM_INS_VSHLL: vshll${p}.i8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLsv2i64, ARM_INS_VSHLL: vshll${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLsv4i32, ARM_INS_VSHLL: vshll${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLsv8i16, ARM_INS_VSHLL: vshll${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLuv2i64, ARM_INS_VSHLL: vshll${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLuv4i32, ARM_INS_VSHLL: vshll${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLLuv8i16, ARM_INS_VSHLL: vshll${p}.u8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv16i8, ARM_INS_VSHL: vshl${p}.i8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv1i64, ARM_INS_VSHL: vshl${p}.i64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv2i32, ARM_INS_VSHL: vshl${p}.i32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv2i64, ARM_INS_VSHL: vshl${p}.i64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv4i16, ARM_INS_VSHL: vshl${p}.i16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv4i32, ARM_INS_VSHL: vshl${p}.i32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv8i16, ARM_INS_VSHL: vshl${p}.i16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLiv8i8, ARM_INS_VSHL: vshl${p}.i8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv16i8, ARM_INS_VSHL: vshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv1i64, ARM_INS_VSHL: vshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv2i32, ARM_INS_VSHL: vshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv2i64, ARM_INS_VSHL: vshl${p}.s64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv4i16, ARM_INS_VSHL: vshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv4i32, ARM_INS_VSHL: vshl${p}.s32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv8i16, ARM_INS_VSHL: vshl${p}.s16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLsv8i8, ARM_INS_VSHL: vshl${p}.s8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv16i8, ARM_INS_VSHL: vshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv1i64, ARM_INS_VSHL: vshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv2i32, ARM_INS_VSHL: vshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv2i64, ARM_INS_VSHL: vshl${p}.u64	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv4i16, ARM_INS_VSHL: vshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv4i32, ARM_INS_VSHL: vshl${p}.u32	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv8i16, ARM_INS_VSHL: vshl${p}.u16	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHLuv8i8, ARM_INS_VSHL: vshl${p}.u8	$vd, $vm, $vn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSHRNv2i32, ARM_INS_VSHRN: vshrn${p}.i64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRNv4i16, ARM_INS_VSHRN: vshrn${p}.i32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRNv8i8, ARM_INS_VSHRN: vshrn${p}.i16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv16i8, ARM_INS_VSHR: vshr${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv1i64, ARM_INS_VSHR: vshr${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv2i32, ARM_INS_VSHR: vshr${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv2i64, ARM_INS_VSHR: vshr${p}.s64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv4i16, ARM_INS_VSHR: vshr${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv4i32, ARM_INS_VSHR: vshr${p}.s32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv8i16, ARM_INS_VSHR: vshr${p}.s16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRsv8i8, ARM_INS_VSHR: vshr${p}.s8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv16i8, ARM_INS_VSHR: vshr${p}.u8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv1i64, ARM_INS_VSHR: vshr${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv2i32, ARM_INS_VSHR: vshr${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv2i64, ARM_INS_VSHR: vshr${p}.u64	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv4i16, ARM_INS_VSHR: vshr${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv4i32, ARM_INS_VSHR: vshr${p}.u32	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv8i16, ARM_INS_VSHR: vshr${p}.u16	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHRuv8i8, ARM_INS_VSHR: vshr${p}.u8	$vd, $vm, $simm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSHTOD, ARM_INS_VCVT: vcvt${p}.f64.s16	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSHTOS, ARM_INS_VCVT: vcvt${p}.f32.s16	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSITOD, ARM_INS_VCVT: vcvt${p}.f64.s32	$dd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSITOS, ARM_INS_VCVT: vcvt${p}.f32.s32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv16i8, ARM_INS_VSLI: vsli${p}.8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv1i64, ARM_INS_VSLI: vsli${p}.64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv2i32, ARM_INS_VSLI: vsli${p}.32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv2i64, ARM_INS_VSLI: vsli${p}.64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv4i16, ARM_INS_VSLI: vsli${p}.16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv4i32, ARM_INS_VSLI: vsli${p}.32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv8i16, ARM_INS_VSLI: vsli${p}.16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLIv8i8, ARM_INS_VSLI: vsli${p}.8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSLTOD, ARM_INS_VCVT: vcvt${p}.f64.s32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSLTOS, ARM_INS_VCVT: vcvt${p}.f32.s32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSQRTD, ARM_INS_VSQRT: vsqrt${p}.f64	$dd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSQRTS, ARM_INS_VSQRT: vsqrt${p}.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv16i8, ARM_INS_VSRA: vsra${p}.s8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv1i64, ARM_INS_VSRA: vsra${p}.s64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv2i32, ARM_INS_VSRA: vsra${p}.s32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv2i64, ARM_INS_VSRA: vsra${p}.s64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv4i16, ARM_INS_VSRA: vsra${p}.s16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv4i32, ARM_INS_VSRA: vsra${p}.s32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv8i16, ARM_INS_VSRA: vsra${p}.s16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAsv8i8, ARM_INS_VSRA: vsra${p}.s8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv16i8, ARM_INS_VSRA: vsra${p}.u8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv1i64, ARM_INS_VSRA: vsra${p}.u64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv2i32, ARM_INS_VSRA: vsra${p}.u32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv2i64, ARM_INS_VSRA: vsra${p}.u64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv4i16, ARM_INS_VSRA: vsra${p}.u16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv4i32, ARM_INS_VSRA: vsra${p}.u32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv8i16, ARM_INS_VSRA: vsra${p}.u16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRAuv8i8, ARM_INS_VSRA: vsra${p}.u8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv16i8, ARM_INS_VSRI: vsri${p}.8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv1i64, ARM_INS_VSRI: vsri${p}.64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv2i32, ARM_INS_VSRI: vsri${p}.32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv2i64, ARM_INS_VSRI: vsri${p}.64	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv4i16, ARM_INS_VSRI: vsri${p}.16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv4i32, ARM_INS_VSRI: vsri${p}.32	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv8i16, ARM_INS_VSRI: vsri${p}.16	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VSRIv8i8, ARM_INS_VSRI: vsri${p}.8	$vd, $vm, $simm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VST1LNd16, ARM_INS_VST1: vst1${p}.16	\{$vd[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1LNd16_UPD, ARM_INS_VST1: vst1${p}.16	\{$vd[$lane]\}, $rn$rm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VST1LNd32, ARM_INS_VST1: vst1${p}.32	\{$vd[$lane]\}, $rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VST1LNd32_UPD, ARM_INS_VST1: vst1${p}.32	\{$vd[$lane]\}, $rn$rm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VST1LNd8, ARM_INS_VST1: vst1${p}.8	\{$vd[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1LNd8_UPD, ARM_INS_VST1: vst1${p}.8	\{$vd[$lane]\}, $rn$rm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VST1d16, ARM_INS_VST1: vst1${p}.16	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16Q, ARM_INS_VST1: vst1${p}.16	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16Qwb_fixed, ARM_INS_VST1: vst1${p}.16	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16Qwb_register, ARM_INS_VST1: vst1${p}.16	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16T, ARM_INS_VST1: vst1${p}.16	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16Twb_fixed, ARM_INS_VST1: vst1${p}.16	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16Twb_register, ARM_INS_VST1: vst1${p}.16	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16wb_fixed, ARM_INS_VST1: vst1${p}.16	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d16wb_register, ARM_INS_VST1: vst1${p}.16	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32, ARM_INS_VST1: vst1${p}.32	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32Q, ARM_INS_VST1: vst1${p}.32	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32Qwb_fixed, ARM_INS_VST1: vst1${p}.32	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32Qwb_register, ARM_INS_VST1: vst1${p}.32	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32T, ARM_INS_VST1: vst1${p}.32	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32Twb_fixed, ARM_INS_VST1: vst1${p}.32	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32Twb_register, ARM_INS_VST1: vst1${p}.32	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32wb_fixed, ARM_INS_VST1: vst1${p}.32	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d32wb_register, ARM_INS_VST1: vst1${p}.32	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64, ARM_INS_VST1: vst1${p}.64	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64Q, ARM_INS_VST1: vst1${p}.64	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64Qwb_fixed, ARM_INS_VST1: vst1${p}.64	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64Qwb_register, ARM_INS_VST1: vst1${p}.64	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64T, ARM_INS_VST1: vst1${p}.64	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64Twb_fixed, ARM_INS_VST1: vst1${p}.64	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64Twb_register, ARM_INS_VST1: vst1${p}.64	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64wb_fixed, ARM_INS_VST1: vst1${p}.64	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d64wb_register, ARM_INS_VST1: vst1${p}.64	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8, ARM_INS_VST1: vst1${p}.8	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8Q, ARM_INS_VST1: vst1${p}.8	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8Qwb_fixed, ARM_INS_VST1: vst1${p}.8	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8Qwb_register, ARM_INS_VST1: vst1${p}.8	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8T, ARM_INS_VST1: vst1${p}.8	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8Twb_fixed, ARM_INS_VST1: vst1${p}.8	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8Twb_register, ARM_INS_VST1: vst1${p}.8	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8wb_fixed, ARM_INS_VST1: vst1${p}.8	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1d8wb_register, ARM_INS_VST1: vst1${p}.8	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q16, ARM_INS_VST1: vst1${p}.16	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q16wb_fixed, ARM_INS_VST1: vst1${p}.16	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q16wb_register, ARM_INS_VST1: vst1${p}.16	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q32, ARM_INS_VST1: vst1${p}.32	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q32wb_fixed, ARM_INS_VST1: vst1${p}.32	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q32wb_register, ARM_INS_VST1: vst1${p}.32	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q64, ARM_INS_VST1: vst1${p}.64	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q64wb_fixed, ARM_INS_VST1: vst1${p}.64	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q64wb_register, ARM_INS_VST1: vst1${p}.64	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q8, ARM_INS_VST1: vst1${p}.8	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q8wb_fixed, ARM_INS_VST1: vst1${p}.8	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST1q8wb_register, ARM_INS_VST1: vst1${p}.8	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNd16, ARM_INS_VST2: vst2${p}.16	\{$vd[$lane], $src2[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNd16_UPD, ARM_INS_VST2: vst2${p}.16	\{$vd[$lane], $src2[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNd32, ARM_INS_VST2: vst2${p}.32	\{$vd[$lane], $src2[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNd32_UPD, ARM_INS_VST2: vst2${p}.32	\{$vd[$lane], $src2[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNd8, ARM_INS_VST2: vst2${p}.8	\{$vd[$lane], $src2[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNd8_UPD, ARM_INS_VST2: vst2${p}.8	\{$vd[$lane], $src2[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNq16, ARM_INS_VST2: vst2${p}.16	\{$vd[$lane], $src2[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNq16_UPD, ARM_INS_VST2: vst2${p}.16	\{$vd[$lane], $src2[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNq32, ARM_INS_VST2: vst2${p}.32	\{$vd[$lane], $src2[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2LNq32_UPD, ARM_INS_VST2: vst2${p}.32	\{$vd[$lane], $src2[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b16, ARM_INS_VST2: vst2${p}.16	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b16wb_fixed, ARM_INS_VST2: vst2${p}.16	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b16wb_register, ARM_INS_VST2: vst2${p}.16	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b32, ARM_INS_VST2: vst2${p}.32	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b32wb_fixed, ARM_INS_VST2: vst2${p}.32	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b32wb_register, ARM_INS_VST2: vst2${p}.32	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b8, ARM_INS_VST2: vst2${p}.8	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b8wb_fixed, ARM_INS_VST2: vst2${p}.8	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2b8wb_register, ARM_INS_VST2: vst2${p}.8	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d16, ARM_INS_VST2: vst2${p}.16	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d16wb_fixed, ARM_INS_VST2: vst2${p}.16	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d16wb_register, ARM_INS_VST2: vst2${p}.16	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d32, ARM_INS_VST2: vst2${p}.32	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d32wb_fixed, ARM_INS_VST2: vst2${p}.32	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d32wb_register, ARM_INS_VST2: vst2${p}.32	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d8, ARM_INS_VST2: vst2${p}.8	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d8wb_fixed, ARM_INS_VST2: vst2${p}.8	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2d8wb_register, ARM_INS_VST2: vst2${p}.8	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q16, ARM_INS_VST2: vst2${p}.16	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q16wb_fixed, ARM_INS_VST2: vst2${p}.16	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q16wb_register, ARM_INS_VST2: vst2${p}.16	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q32, ARM_INS_VST2: vst2${p}.32	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q32wb_fixed, ARM_INS_VST2: vst2${p}.32	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q32wb_register, ARM_INS_VST2: vst2${p}.32	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q8, ARM_INS_VST2: vst2${p}.8	$vd, $rn */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q8wb_fixed, ARM_INS_VST2: vst2${p}.8	$vd, $rn! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST2q8wb_register, ARM_INS_VST2: vst2${p}.8	$vd, $rn, $rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNd16, ARM_INS_VST3: vst3${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNd16_UPD, ARM_INS_VST3: vst3${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNd32, ARM_INS_VST3: vst3${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNd32_UPD, ARM_INS_VST3: vst3${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNd8, ARM_INS_VST3: vst3${p}.8	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNd8_UPD, ARM_INS_VST3: vst3${p}.8	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNq16, ARM_INS_VST3: vst3${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNq16_UPD, ARM_INS_VST3: vst3${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNq32, ARM_INS_VST3: vst3${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3LNq32_UPD, ARM_INS_VST3: vst3${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3d16, ARM_INS_VST3: vst3${p}.16	\{$vd, $src2, $src3\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3d16_UPD, ARM_INS_VST3: vst3${p}.16	\{$vd, $src2, $src3\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3d32, ARM_INS_VST3: vst3${p}.32	\{$vd, $src2, $src3\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3d32_UPD, ARM_INS_VST3: vst3${p}.32	\{$vd, $src2, $src3\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3d8, ARM_INS_VST3: vst3${p}.8	\{$vd, $src2, $src3\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3d8_UPD, ARM_INS_VST3: vst3${p}.8	\{$vd, $src2, $src3\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3q16, ARM_INS_VST3: vst3${p}.16	\{$vd, $src2, $src3\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3q16_UPD, ARM_INS_VST3: vst3${p}.16	\{$vd, $src2, $src3\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3q32, ARM_INS_VST3: vst3${p}.32	\{$vd, $src2, $src3\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3q32_UPD, ARM_INS_VST3: vst3${p}.32	\{$vd, $src2, $src3\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3q8, ARM_INS_VST3: vst3${p}.8	\{$vd, $src2, $src3\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST3q8_UPD, ARM_INS_VST3: vst3${p}.8	\{$vd, $src2, $src3\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNd16, ARM_INS_VST4: vst4${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNd16_UPD, ARM_INS_VST4: vst4${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNd32, ARM_INS_VST4: vst4${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNd32_UPD, ARM_INS_VST4: vst4${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNd8, ARM_INS_VST4: vst4${p}.8	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNd8_UPD, ARM_INS_VST4: vst4${p}.8	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNq16, ARM_INS_VST4: vst4${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNq16_UPD, ARM_INS_VST4: vst4${p}.16	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNq32, ARM_INS_VST4: vst4${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4LNq32_UPD, ARM_INS_VST4: vst4${p}.32	\{$vd[$lane], $src2[$lane], $src3[$lane], $src4[$lane]\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4d16, ARM_INS_VST4: vst4${p}.16	\{$vd, $src2, $src3, $src4\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4d16_UPD, ARM_INS_VST4: vst4${p}.16	\{$vd, $src2, $src3, $src4\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4d32, ARM_INS_VST4: vst4${p}.32	\{$vd, $src2, $src3, $src4\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4d32_UPD, ARM_INS_VST4: vst4${p}.32	\{$vd, $src2, $src3, $src4\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4d8, ARM_INS_VST4: vst4${p}.8	\{$vd, $src2, $src3, $src4\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4d8_UPD, ARM_INS_VST4: vst4${p}.8	\{$vd, $src2, $src3, $src4\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4q16, ARM_INS_VST4: vst4${p}.16	\{$vd, $src2, $src3, $src4\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4q16_UPD, ARM_INS_VST4: vst4${p}.16	\{$vd, $src2, $src3, $src4\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4q32, ARM_INS_VST4: vst4${p}.32	\{$vd, $src2, $src3, $src4\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4q32_UPD, ARM_INS_VST4: vst4${p}.32	\{$vd, $src2, $src3, $src4\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4q8, ARM_INS_VST4: vst4${p}.8	\{$vd, $src2, $src3, $src4\}, $rn */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VST4q8_UPD, ARM_INS_VST4: vst4${p}.8	\{$vd, $src2, $src3, $src4\}, $rn$rm */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSTMDDB_UPD, ARM_INS_VSTMDB: vstmdb${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSTMDIA, ARM_INS_VSTMIA: vstmia${p}	$rn, $regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VSTMDIA_UPD, ARM_INS_VSTMIA: vstmia${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSTMSDB_UPD, ARM_INS_VSTMDB: vstmdb${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSTMSIA, ARM_INS_VSTMIA: vstmia${p}	$rn, $regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VSTMSIA_UPD, ARM_INS_VSTMIA: vstmia${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VSTRD, ARM_INS_VSTR: vstr${p}	$dd, $addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VSTRS, ARM_INS_VSTR: vstr${p}	$sd, $addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_VSUBD, ARM_INS_VSUB: vsub${p}.f64	$dd, $dn, $dm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBHNv2i32, ARM_INS_VSUBHN: vsubhn${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBHNv4i16, ARM_INS_VSUBHN: vsubhn${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBHNv8i8, ARM_INS_VSUBHN: vsubhn${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBLsv2i64, ARM_INS_VSUBL: vsubl${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBLsv4i32, ARM_INS_VSUBL: vsubl${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBLsv8i16, ARM_INS_VSUBL: vsubl${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBLuv2i64, ARM_INS_VSUBL: vsubl${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBLuv4i32, ARM_INS_VSUBL: vsubl${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBLuv8i16, ARM_INS_VSUBL: vsubl${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBS, ARM_INS_VSUB: vsub${p}.f32	$sd, $sn, $sm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBWsv2i64, ARM_INS_VSUBW: vsubw${p}.s32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBWsv4i32, ARM_INS_VSUBW: vsubw${p}.s16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBWsv8i16, ARM_INS_VSUBW: vsubw${p}.s8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBWuv2i64, ARM_INS_VSUBW: vsubw${p}.u32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBWuv4i32, ARM_INS_VSUBW: vsubw${p}.u16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBWuv8i16, ARM_INS_VSUBW: vsubw${p}.u8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBfd, ARM_INS_VSUB: vsub${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBfq, ARM_INS_VSUB: vsub${p}.f32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv16i8, ARM_INS_VSUB: vsub${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv1i64, ARM_INS_VSUB: vsub${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv2i32, ARM_INS_VSUB: vsub${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv2i64, ARM_INS_VSUB: vsub${p}.i64	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv4i16, ARM_INS_VSUB: vsub${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv4i32, ARM_INS_VSUB: vsub${p}.i32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv8i16, ARM_INS_VSUB: vsub${p}.i16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSUBv8i8, ARM_INS_VSUB: vsub${p}.i8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VSWPd, ARM_INS_VSWP: vswp${p}	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VSWPq, ARM_INS_VSWP: vswp${p}	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VTBL1, ARM_INS_VTBL: vtbl${p}.8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTBL2, ARM_INS_VTBL: vtbl${p}.8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTBL3, ARM_INS_VTBL: vtbl${p}.8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTBL4, ARM_INS_VTBL: vtbl${p}.8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTBX1, ARM_INS_VTBX: vtbx${p}.8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTBX2, ARM_INS_VTBX: vtbx${p}.8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTBX3, ARM_INS_VTBX: vtbx${p}.8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTBX4, ARM_INS_VTBX: vtbx${p}.8	$vd, $vn, $vm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTOSHD, ARM_INS_VCVT: vcvt${p}.s16.f64	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTOSHS, ARM_INS_VCVT: vcvt${p}.s16.f32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTOSIRD, ARM_INS_VCVTR: vcvtr${p}.s32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOSIRS, ARM_INS_VCVTR: vcvtr${p}.s32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOSIZD, ARM_INS_VCVT: vcvt${p}.s32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOSIZS, ARM_INS_VCVT: vcvt${p}.s32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOSLD, ARM_INS_VCVT: vcvt${p}.s32.f64	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTOSLS, ARM_INS_VCVT: vcvt${p}.s32.f32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTOUHD, ARM_INS_VCVT: vcvt${p}.u16.f64	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTOUHS, ARM_INS_VCVT: vcvt${p}.u16.f32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTOUIRD, ARM_INS_VCVTR: vcvtr${p}.u32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOUIRS, ARM_INS_VCVTR: vcvtr${p}.u32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOUIZD, ARM_INS_VCVT: vcvt${p}.u32.f64	$sd, $dm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOUIZS, ARM_INS_VCVT: vcvt${p}.u32.f32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VTOULD, ARM_INS_VCVT: vcvt${p}.u32.f64	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTOULS, ARM_INS_VCVT: vcvt${p}.u32.f32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VTRNd16, ARM_INS_VTRN: vtrn${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VTRNd32, ARM_INS_VTRN: vtrn${p}.32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VTRNd8, ARM_INS_VTRN: vtrn${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VTRNq16, ARM_INS_VTRN: vtrn${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VTRNq32, ARM_INS_VTRN: vtrn${p}.32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VTRNq8, ARM_INS_VTRN: vtrn${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VTSTv16i8, ARM_INS_VTST: vtst${p}.8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTSTv2i32, ARM_INS_VTST: vtst${p}.32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTSTv4i16, ARM_INS_VTST: vtst${p}.16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTSTv4i32, ARM_INS_VTST: vtst${p}.32	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTSTv8i16, ARM_INS_VTST: vtst${p}.16	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VTSTv8i8, ARM_INS_VTST: vtst${p}.8	$vd, $vn, $vm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_VUHTOD, ARM_INS_VCVT: vcvt${p}.f64.u16	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VUHTOS, ARM_INS_VCVT: vcvt${p}.f32.u16	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VUITOD, ARM_INS_VCVT: vcvt${p}.f64.u32	$dd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VUITOS, ARM_INS_VCVT: vcvt${p}.f32.u32	$sd, $sm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_VULTOD, ARM_INS_VCVT: vcvt${p}.f64.u32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VULTOS, ARM_INS_VCVT: vcvt${p}.f32.u32	$dst, $a, $fbits */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_VUZPd16, ARM_INS_VUZP: vuzp${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VUZPd8, ARM_INS_VUZP: vuzp${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VUZPq16, ARM_INS_VUZP: vuzp${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VUZPq32, ARM_INS_VUZP: vuzp${p}.32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VUZPq8, ARM_INS_VUZP: vuzp${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VZIPd16, ARM_INS_VZIP: vzip${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VZIPd8, ARM_INS_VZIP: vzip${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VZIPq16, ARM_INS_VZIP: vzip${p}.16	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VZIPq32, ARM_INS_VZIP: vzip${p}.32	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_VZIPq8, ARM_INS_VZIP: vzip${p}.8	$vd, $vm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMDA, ARM_INS_LDMDA: ldmda${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMDA_UPD, ARM_INS_LDMDA: ldmda${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMDB, ARM_INS_LDMDB: ldmdb${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMDB_UPD, ARM_INS_LDMDB: ldmdb${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMIA, ARM_INS_LDM: ldm${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMIA_UPD, ARM_INS_LDM: ldm${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMIB, ARM_INS_LDMIB: ldmib${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_sysLDMIB_UPD, ARM_INS_LDMIB: ldmib${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_sysSTMDA, ARM_INS_STMDA: stmda${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_sysSTMDA_UPD, ARM_INS_STMDA: stmda${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_sysSTMDB, ARM_INS_STMDB: stmdb${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_sysSTMDB_UPD, ARM_INS_STMDB: stmdb${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_sysSTMIA, ARM_INS_STM: stm${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_sysSTMIA_UPD, ARM_INS_STM: stm${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_sysSTMIB, ARM_INS_STMIB: stmib${p}	$rn, $regs ^ */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_sysSTMIB_UPD, ARM_INS_STMIB: stmib${p}	$rn!, $regs ^ */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ADCri, ARM_INS_ADC: adc${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ADCrr, ARM_INS_ADC: adc${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2ADCrs, ARM_INS_ADC: adc${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ADDri, ARM_INS_ADD: add${s}${p}.w	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ADDri12, ARM_INS_ADDW: addw${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ADDrr, ARM_INS_ADD: add${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2ADDrs, ARM_INS_ADD: add${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ADR, ARM_INS_ADR: adr{$p}.w	$rd, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2ANDri, ARM_INS_AND: and${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ANDrr, ARM_INS_AND: and${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2ANDrs, ARM_INS_AND: and${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ASRri, ARM_INS_ASR: asr${s}${p}.w	$rd, $rm, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ASRrr, ARM_INS_ASR: asr${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2B, ARM_INS_B: b${p}.w	$target */
	{ 0 }
},
{	/* ARM_t2BFC, ARM_INS_BFC: bfc${p}	$rd, $imm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_t2BFI, ARM_INS_BFI: bfi${p}	$rd, $rn, $imm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2BICri, ARM_INS_BIC: bic${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2BICrr, ARM_INS_BIC: bic${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2BICrs, ARM_INS_BIC: bic${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2BXJ, ARM_INS_BXJ: bxj${p}	$func */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2Bcc, ARM_INS_B: b${p}.w	$target */
	{ 0 }
},
{	/* ARM_t2CDP, ARM_INS_CDP: cdp${p}	$cop, $opc1, $crd, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_t2CDP2, ARM_INS_CDP2: cdp2${p}	$cop, $opc1, $crd, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_t2CLREX, ARM_INS_CLREX: clrex${p} */
	{ 0 }
},
{	/* ARM_t2CLZ, ARM_INS_CLZ: clz${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2CMNri, ARM_INS_CMN: cmn${p}.w	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2CMNzrr, ARM_INS_CMN: cmn${p}.w	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2CMNzrs, ARM_INS_CMN: cmn${p}.w	$rn, $shiftedrm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2CMPri, ARM_INS_CMP: cmp${p}.w	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2CMPrr, ARM_INS_CMP: cmp${p}.w	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2CMPrs, ARM_INS_CMP: cmp${p}.w	$rn, $shiftedrm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2CPS1p, ARM_INS_CPS: cps	$mode */
	{ 0 }
},
{	/* ARM_t2CPS2p, ARM_INS_CPS: cps$imod.w	$iflags */
	{ 0 }
},
{	/* ARM_t2CPS3p, ARM_INS_CPS: cps$imod	$iflags, $mode */
	{ 0 }
},
{	/* ARM_t2CRC32B, ARM_INS_CRC32B: crc32b	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2CRC32CB, ARM_INS_CRC32CB: crc32cb	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2CRC32CH, ARM_INS_CRC32CH: crc32ch	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2CRC32CW, ARM_INS_CRC32CW: crc32cw	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2CRC32H, ARM_INS_CRC32H: crc32h	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2CRC32W, ARM_INS_CRC32W: crc32w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2DBG, ARM_INS_DBG: dbg${p}	$opt */
	{ 0 }
},
{	/* ARM_t2DCPS1, ARM_INS_DCPS1: dcps1${p} */
	{ 0 }
},
{	/* ARM_t2DCPS2, ARM_INS_DCPS2: dcps2${p} */
	{ 0 }
},
{	/* ARM_t2DCPS3, ARM_INS_DCPS3: dcps3${p} */
	{ 0 }
},
{	/* ARM_t2DMB, ARM_INS_DMB: dmb${p}	$opt */
	{ 0 }
},
{	/* ARM_t2DSB, ARM_INS_DSB: dsb${p}	$opt */
	{ 0 }
},
{	/* ARM_t2EORri, ARM_INS_EOR: eor${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2EORrr, ARM_INS_EOR: eor${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2EORrs, ARM_INS_EOR: eor${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2HINT, ARM_INS_HINT: hint${p}.w	$imm */
	{ 0 }
},
{	/* ARM_t2HVC, ARM_INS_HVC: hvc.w	$imm16 */
	{ 0 }
},
{	/* ARM_t2ISB, ARM_INS_ISB: isb${p}	$opt */
	{ 0 }
},
{	/* ARM_t2IT, ARM_INS_IT: it$mask	$cc */
	{ 0 }
},
{	/* ARM_t2LDA, ARM_INS_LDA: lda${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDAB, ARM_INS_LDAB: ldab${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDAEX, ARM_INS_LDAEX: ldaex${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDAEXB, ARM_INS_LDAEXB: ldaexb${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDAEXD, ARM_INS_LDAEXD: ldaexd${p}	$rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDAEXH, ARM_INS_LDAEXH: ldaexh${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDAH, ARM_INS_LDAH: ldah${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2L_OFFSET, ARM_INS_LDC2L: ldc2l${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2L_OPTION, ARM_INS_LDC2L: ldc2l${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2L_POST, ARM_INS_LDC2L: ldc2l${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2L_PRE, ARM_INS_LDC2L: ldc2l${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2_OFFSET, ARM_INS_LDC2: ldc2${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2_OPTION, ARM_INS_LDC2: ldc2${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2_POST, ARM_INS_LDC2: ldc2${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC2_PRE, ARM_INS_LDC2: ldc2${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDCL_OFFSET, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDCL_OPTION, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDCL_POST, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDCL_PRE, ARM_INS_LDCL: ldcl${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC_OFFSET, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC_OPTION, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC_POST, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDC_PRE, ARM_INS_LDC: ldc${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LDMDB, ARM_INS_LDMDB: ldmdb${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDMDB_UPD, ARM_INS_LDMDB: ldmdb${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDMIA, ARM_INS_LDM: ldm${p}.w	$rn, $regs */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDMIA_UPD, ARM_INS_LDM: ldm${p}.w	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRBT, ARM_INS_LDRBT: ldrbt${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRB_POST, ARM_INS_LDRB: ldrb${p}	$rt, $rn$offset */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRB_PRE, ARM_INS_LDRB: ldrb${p}	$rt, $addr! */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRBi12, ARM_INS_LDRB: ldrb${p}.w	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRBi8, ARM_INS_LDRB: ldrb${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRBpci, ARM_INS_LDRB: ldrb${p}.w	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRBs, ARM_INS_LDRB: ldrb${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRD_POST, ARM_INS_LDRD: ldrd${p}	$rt, $rt2, $addr$imm */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRD_PRE, ARM_INS_LDRD: ldrd${p}	$rt, $rt2, $addr! */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRDi8, ARM_INS_LDRD: ldrd${p}	$rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDREX, ARM_INS_LDREX: ldrex${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDREXB, ARM_INS_LDREXB: ldrexb${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDREXD, ARM_INS_LDREXD: ldrexd${p}	$rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDREXH, ARM_INS_LDREXH: ldrexh${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRHT, ARM_INS_LDRHT: ldrht${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRH_POST, ARM_INS_LDRH: ldrh${p}	$rt, $rn$offset */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRH_PRE, ARM_INS_LDRH: ldrh${p}	$rt, $addr! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRHi12, ARM_INS_LDRH: ldrh${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRHi8, ARM_INS_LDRH: ldrh${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRHpci, ARM_INS_LDRH: ldrh${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRHs, ARM_INS_LDRH: ldrh${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSBT, ARM_INS_LDRSBT: ldrsbt${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSB_POST, ARM_INS_LDRSB: ldrsb${p}	$rt, $rn$offset */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRSB_PRE, ARM_INS_LDRSB: ldrsb${p}	$rt, $addr! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSBi12, ARM_INS_LDRSB: ldrsb${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSBi8, ARM_INS_LDRSB: ldrsb${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSBpci, ARM_INS_LDRSB: ldrsb${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSBs, ARM_INS_LDRSB: ldrsb${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSHT, ARM_INS_LDRSHT: ldrsht${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSH_POST, ARM_INS_LDRSH: ldrsh${p}	$rt, $rn$offset */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRSH_PRE, ARM_INS_LDRSH: ldrsh${p}	$rt, $addr! */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSHi12, ARM_INS_LDRSH: ldrsh${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSHi8, ARM_INS_LDRSH: ldrsh${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSHpci, ARM_INS_LDRSH: ldrsh${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRSHs, ARM_INS_LDRSH: ldrsh${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LDRT, ARM_INS_LDRT: ldrt${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDR_POST, ARM_INS_LDR: ldr${p}	$rt, $rn$offset */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDR_PRE, ARM_INS_LDR: ldr${p}	$rt, $addr! */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRi12, ARM_INS_LDR: ldr${p}.w	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRi8, ARM_INS_LDR: ldr${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRpci, ARM_INS_LDR: ldr${p}.w	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2LDRs, ARM_INS_LDR: ldr${p}.w	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LSLri, ARM_INS_LSL: lsl${s}${p}.w	$rd, $rm, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LSLrr, ARM_INS_LSL: lsl${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2LSRri, ARM_INS_LSR: lsr${s}${p}.w	$rd, $rm, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2LSRrr, ARM_INS_LSR: lsr${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MCR, ARM_INS_MCR: mcr${p}	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_t2MCR2, ARM_INS_MCR2: mcr2${p}	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_t2MCRR, ARM_INS_MCRR: mcrr${p}	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MCRR2, ARM_INS_MCRR2: mcrr2${p}	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MLA, ARM_INS_MLA: mla${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MLS, ARM_INS_MLS: mls${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MOVTi16, ARM_INS_MOVT: movt${p}	$rd, $imm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_t2MOVi, ARM_INS_MOV: mov${s}${p}.w	$rd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2MOVi16, ARM_INS_MOVW: movw${p}	$rd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2MOVr, ARM_INS_MOV: mov${s}${p}.w	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2MOVsra_flag, ARM_INS_ASR: asrs${p}.w	$rd, $rm, #1 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2MOVsrl_flag, ARM_INS_LSR: lsrs${p}.w	$rd, $rm, #1 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2MRC, ARM_INS_MRC: mrc${p}	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_t2MRC2, ARM_INS_MRC2: mrc2${p}	$cop, $opc1, $rt, $crn, $crm, $opc2 */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_IGNORE, 0 }
},
{	/* ARM_t2MRRC, ARM_INS_MRRC: mrrc${p}	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MRRC2, ARM_INS_MRRC2: mrrc2${p}	$cop, $opc1, $rt, $rt2, $crm */
	{ CS_AC_READ, CS_AC_IGNORE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MRS_AR, ARM_INS_MRS: mrs${p}	$rd, apsr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2MRS_M, ARM_INS_MRS: mrs${p}	$rd, $sysm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2MRSbanked, ARM_INS_MRS: mrs${p}	$rd, $banked */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2MRSsys_AR, ARM_INS_MRS: mrs${p}	$rd, spsr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2MSR_AR, ARM_INS_MSR: msr${p}	$mask, $rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2MSR_M, ARM_INS_MSR: msr${p}	$sysm, $rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2MSRbanked, ARM_INS_MSR: msr${p}	$banked, $rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2MUL, ARM_INS_MUL: mul${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2MVNi, ARM_INS_MVN: mvn${s}${p}	$rd, $imm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2MVNr, ARM_INS_MVN: mvn${s}${p}.w	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2MVNs, ARM_INS_MVN: mvn${s}${p}.w	$rd, $shiftedrm */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2ORNri, ARM_INS_ORN: orn${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ORNrr, ARM_INS_ORN: orn${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2ORNrs, ARM_INS_ORN: orn${s}${p}	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ORRri, ARM_INS_ORR: orr${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2ORRrr, ARM_INS_ORR: orr${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2ORRrs, ARM_INS_ORR: orr${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2PKHBT, ARM_INS_PKHBT: pkhbt${p}	$rd, $rn, $rm$sh */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2PKHTB, ARM_INS_PKHTB: pkhtb${p}	$rd, $rn, $rm$sh */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2PLDWi12, ARM_INS_PLDW: pldw${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLDWi8, ARM_INS_PLDW: pldw${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLDWs, ARM_INS_PLDW: pldw${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLDi12, ARM_INS_PLD: pld${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLDi8, ARM_INS_PLD: pld${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLDpci, ARM_INS_PLD: pld${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLDs, ARM_INS_PLD: pld${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLIi12, ARM_INS_PLI: pli${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLIi8, ARM_INS_PLI: pli${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLIpci, ARM_INS_PLI: pli${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2PLIs, ARM_INS_PLI: pli${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2QADD, ARM_INS_QADD: qadd${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QADD16, ARM_INS_QADD16: qadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QADD8, ARM_INS_QADD8: qadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QASX, ARM_INS_QASX: qasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QDADD, ARM_INS_QDADD: qdadd${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QDSUB, ARM_INS_QDSUB: qdsub${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QSAX, ARM_INS_QSAX: qsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QSUB, ARM_INS_QSUB: qsub${p}	$rd, $rm, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QSUB16, ARM_INS_QSUB16: qsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2QSUB8, ARM_INS_QSUB8: qsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2RBIT, ARM_INS_RBIT: rbit${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2REV, ARM_INS_REV: rev${p}.w	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2REV16, ARM_INS_REV16: rev16${p}.w	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2REVSH, ARM_INS_REVSH: revsh${p}.w	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2RFEDB, ARM_INS_RFEDB: rfedb${p}	$rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2RFEDBW, ARM_INS_RFEDB: rfedb${p}	$rn! */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2RFEIA, ARM_INS_RFEIA: rfeia${p}	$rn */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2RFEIAW, ARM_INS_RFEIA: rfeia${p}	$rn! */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2RORri, ARM_INS_ROR: ror${s}${p}.w	$rd, $rm, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2RORrr, ARM_INS_ROR: ror${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2RRX, ARM_INS_RRX: rrx${s}${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2RSBri, ARM_INS_RSB: rsb${s}${p}.w	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2RSBrr, ARM_INS_RSB: rsb${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2RSBrs, ARM_INS_RSB: rsb${s}${p}	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SADD16, ARM_INS_SADD16: sadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SADD8, ARM_INS_SADD8: sadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SASX, ARM_INS_SASX: sasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SBCri, ARM_INS_SBC: sbc${s}${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SBCrr, ARM_INS_SBC: sbc${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SBCrs, ARM_INS_SBC: sbc${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SBFX, ARM_INS_SBFX: sbfx${p}	$rd, $rn, $lsb, $msb */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SDIV, ARM_INS_SDIV: sdiv${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SEL, ARM_INS_SEL: sel${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SHADD16, ARM_INS_SHADD16: shadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SHADD8, ARM_INS_SHADD8: shadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SHASX, ARM_INS_SHASX: shasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SHSAX, ARM_INS_SHSAX: shsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SHSUB16, ARM_INS_SHSUB16: shsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SHSUB8, ARM_INS_SHSUB8: shsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMC, ARM_INS_SMC: smc${p}	$opt */
	{ 0 }
},
{	/* ARM_t2SMLABB, ARM_INS_SMLABB: smlabb${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLABT, ARM_INS_SMLABT: smlabt${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLAD, ARM_INS_SMLAD: smlad${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLADX, ARM_INS_SMLADX: smladx${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLAL, ARM_INS_SMLAL: smlal${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLALBB, ARM_INS_SMLALBB: smlalbb${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLALBT, ARM_INS_SMLALBT: smlalbt${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLALD, ARM_INS_SMLALD: smlald${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLALDX, ARM_INS_SMLALDX: smlaldx${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLALTB, ARM_INS_SMLALTB: smlaltb${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLALTT, ARM_INS_SMLALTT: smlaltt${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLATB, ARM_INS_SMLATB: smlatb${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLATT, ARM_INS_SMLATT: smlatt${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLAWB, ARM_INS_SMLAWB: smlawb${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLAWT, ARM_INS_SMLAWT: smlawt${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLSD, ARM_INS_SMLSD: smlsd${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLSDX, ARM_INS_SMLSDX: smlsdx${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLSLD, ARM_INS_SMLSLD: smlsld${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMLSLDX, ARM_INS_SMLSLDX: smlsldx${p}	$ra, $rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMMLA, ARM_INS_SMMLA: smmla${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMMLAR, ARM_INS_SMMLAR: smmlar${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMMLS, ARM_INS_SMMLS: smmls${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMMLSR, ARM_INS_SMMLSR: smmlsr${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMMUL, ARM_INS_SMMUL: smmul${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMMULR, ARM_INS_SMMULR: smmulr${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMUAD, ARM_INS_SMUAD: smuad${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMUADX, ARM_INS_SMUADX: smuadx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMULBB, ARM_INS_SMULBB: smulbb${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMULBT, ARM_INS_SMULBT: smulbt${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMULL, ARM_INS_SMULL: smull${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMULTB, ARM_INS_SMULTB: smultb${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMULTT, ARM_INS_SMULTT: smultt${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMULWB, ARM_INS_SMULWB: smulwb${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMULWT, ARM_INS_SMULWT: smulwt${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMUSD, ARM_INS_SMUSD: smusd${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SMUSDX, ARM_INS_SMUSDX: smusdx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SRSDB, ARM_INS_SRSDB: srsdb${p}	sp, $mode */
	{ 0 }
},
{	/* ARM_t2SRSDB_UPD, ARM_INS_SRSDB: srsdb${p}	sp!, $mode */
	{ 0 }
},
{	/* ARM_t2SRSIA, ARM_INS_SRSIA: srsia${p}	sp, $mode */
	{ 0 }
},
{	/* ARM_t2SRSIA_UPD, ARM_INS_SRSIA: srsia${p}	sp!, $mode */
	{ 0 }
},
{	/* ARM_t2SSAT, ARM_INS_SSAT: ssat${p}	$rd, $sat_imm, $rn$sh */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2SSAT16, ARM_INS_SSAT16: ssat16${p}	$rd, $sat_imm, $rn */
	{ CS_AC_WRITE, CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_t2SSAX, ARM_INS_SSAX: ssax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SSUB16, ARM_INS_SSUB16: ssub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SSUB8, ARM_INS_SSUB8: ssub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2L_OFFSET, ARM_INS_STC2L: stc2l${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2L_OPTION, ARM_INS_STC2L: stc2l${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2L_POST, ARM_INS_STC2L: stc2l${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2L_PRE, ARM_INS_STC2L: stc2l${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2_OFFSET, ARM_INS_STC2: stc2${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2_OPTION, ARM_INS_STC2: stc2${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2_POST, ARM_INS_STC2: stc2${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC2_PRE, ARM_INS_STC2: stc2${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STCL_OFFSET, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STCL_OPTION, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STCL_POST, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STCL_PRE, ARM_INS_STCL: stcl${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC_OFFSET, ARM_INS_STC: stc${p}	$cop, $crd, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC_OPTION, ARM_INS_STC: stc${p}	$cop, $crd, $addr, $option */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC_POST, ARM_INS_STC: stc${p}	$cop, $crd, $addr, $offset */
	{ CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STC_PRE, ARM_INS_STC: stc${p}	$cop, $crd, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STL, ARM_INS_STL: stl${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STLB, ARM_INS_STLB: stlb${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STLEX, ARM_INS_STLEX: stlex${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STLEXB, ARM_INS_STLEXB: stlexb${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STLEXD, ARM_INS_STLEXD: stlexd${p}	$rd, $rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STLEXH, ARM_INS_STLEXH: stlexh${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STLH, ARM_INS_STLH: stlh${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STMDB, ARM_INS_STMDB: stmdb${p}	$rn, $regs */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STMDB_UPD, ARM_INS_STMDB: stmdb${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2STMIA, ARM_INS_STM: stm${p}.w	$rn, $regs */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STMIA_UPD, ARM_INS_STM: stm${p}.w	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2STRBT, ARM_INS_STRBT: strbt${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRB_POST, ARM_INS_STRB: strb${p}	$rt, $rn$offset */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2STRB_PRE, ARM_INS_STRB: strb${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRBi12, ARM_INS_STRB: strb${p}.w	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRBi8, ARM_INS_STRB: strb${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRBs, ARM_INS_STRB: strb${p}.w	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRD_POST, ARM_INS_STRD: strd${p}	$rt, $rt2, $addr$imm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STRD_PRE, ARM_INS_STRD: strd${p}	$rt, $rt2, $addr! */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STRDi8, ARM_INS_STRD: strd${p}	$rt, $rt2, $addr */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STREX, ARM_INS_STREX: strex${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2STREXB, ARM_INS_STREXB: strexb${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STREXD, ARM_INS_STREXD: strexd${p}	$rd, $rt, $rt2, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STREXH, ARM_INS_STREXH: strexh${p}	$rd, $rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2STRHT, ARM_INS_STRHT: strht${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRH_POST, ARM_INS_STRH: strh${p}	$rt, $rn$offset */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRH_PRE, ARM_INS_STRH: strh${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRHi12, ARM_INS_STRH: strh${p}.w	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRHi8, ARM_INS_STRH: strh${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRHs, ARM_INS_STRH: strh${p}.w	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRT, ARM_INS_STRT: strt${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STR_POST, ARM_INS_STR: str${p}	$rt, $rn$offset */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2STR_PRE, ARM_INS_STR: str${p}	$rt, $addr! */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRi12, ARM_INS_STR: str${p}.w	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRi8, ARM_INS_STR: str${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2STRs, ARM_INS_STR: str${p}.w	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_t2SUBS_PC_LR, ARM_INS_SUB: subs${p}	pc, lr, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SUBri, ARM_INS_SUB: sub${s}${p}.w	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SUBri12, ARM_INS_SUBW: subw${p}	$rd, $rn, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SUBrr, ARM_INS_SUB: sub${s}${p}.w	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2SUBrs, ARM_INS_SUB: sub${s}${p}.w	$rd, $rn, $shiftedrm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SXTAB, ARM_INS_SXTAB: sxtab${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SXTAB16, ARM_INS_SXTAB16: sxtab16${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SXTAH, ARM_INS_SXTAH: sxtah${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SXTB, ARM_INS_SXTB: sxtb${p}.w	$rd, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SXTB16, ARM_INS_SXTB16: sxtb16${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2SXTH, ARM_INS_SXTH: sxth${p}.w	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2TBB, ARM_INS_TBB: tbb${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2TBH, ARM_INS_TBH: tbh${p}	$addr */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2TEQri, ARM_INS_TEQ: teq${p}.w	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2TEQrr, ARM_INS_TEQ: teq${p}.w	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2TEQrs, ARM_INS_TEQ: teq${p}.w	$rn, $shiftedrm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2TSTri, ARM_INS_TST: tst${p}.w	$rn, $imm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2TSTrr, ARM_INS_TST: tst${p}.w	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2TSTrs, ARM_INS_TST: tst${p}.w	$rn, $shiftedrm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_t2UADD16, ARM_INS_UADD16: uadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UADD8, ARM_INS_UADD8: uadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UASX, ARM_INS_UASX: uasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UBFX, ARM_INS_UBFX: ubfx${p}	$rd, $rn, $lsb, $msb */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2UDF, ARM_INS_UDF: udf.w	$imm16 */
	{ 0 }
},
{	/* ARM_t2UDIV, ARM_INS_UDIV: udiv${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UHADD16, ARM_INS_UHADD16: uhadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UHADD8, ARM_INS_UHADD8: uhadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UHASX, ARM_INS_UHASX: uhasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UHSAX, ARM_INS_UHSAX: uhsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UHSUB16, ARM_INS_UHSUB16: uhsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UHSUB8, ARM_INS_UHSUB8: uhsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UMAAL, ARM_INS_UMAAL: umaal${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UMLAL, ARM_INS_UMLAL: umlal${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UMULL, ARM_INS_UMULL: umull${p}	$rdlo, $rdhi, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UQADD16, ARM_INS_UQADD16: uqadd16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UQADD8, ARM_INS_UQADD8: uqadd8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UQASX, ARM_INS_UQASX: uqasx${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UQSAX, ARM_INS_UQSAX: uqsax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UQSUB16, ARM_INS_UQSUB16: uqsub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UQSUB8, ARM_INS_UQSUB8: uqsub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2USAD8, ARM_INS_USAD8: usad8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2USADA8, ARM_INS_USADA8: usada8${p}	$rd, $rn, $rm, $ra */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2USAT, ARM_INS_USAT: usat${p}	$rd, $sat_imm, $rn$sh */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2USAT16, ARM_INS_USAT16: usat16${p}	$rd, $sat_imm, $rn */
	{ CS_AC_WRITE, CS_AC_IGNORE, CS_AC_READ, 0 }
},
{	/* ARM_t2USAX, ARM_INS_USAX: usax${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2USUB16, ARM_INS_USUB16: usub16${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2USUB8, ARM_INS_USUB8: usub8${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_t2UXTAB, ARM_INS_UXTAB: uxtab${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2UXTAB16, ARM_INS_UXTAB16: uxtab16${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2UXTAH, ARM_INS_UXTAH: uxtah${p}	$rd, $rn, $rm$rot */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_t2UXTB, ARM_INS_UXTB: uxtb${p}.w	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2UXTB16, ARM_INS_UXTB16: uxtb16${p}	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_t2UXTH, ARM_INS_UXTH: uxth${p}.w	$rd, $rm$rot */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tADC, ARM_INS_ADC: adc${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tADDhirr, ARM_INS_ADD: add${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tADDi3, ARM_INS_ADD: add${s}${p}	$rd, $rm, $imm3 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tADDi8, ARM_INS_ADD: add${s}${p}	$rdn, $imm8 */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_tADDrSP, ARM_INS_ADD: add${p}	$rdn, $sp, $rn */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_tADDrSPi, ARM_INS_ADD: add${p}	$dst, $sp, $imm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tADDrr, ARM_INS_ADD: add${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_tADDspi, ARM_INS_ADD: add${p}	$rdn, $imm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_tADDspr, ARM_INS_ADD: add${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tADR, ARM_INS_ADR: adr{$p}	$rd, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tAND, ARM_INS_AND: and${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tASRri, ARM_INS_ASR: asr${s}${p}	$rd, $rm, $imm5 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tASRrr, ARM_INS_ASR: asr${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tB, ARM_INS_B: b${p}	$target */
	{ 0 }
},
{	/* ARM_tBIC, ARM_INS_BIC: bic${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tBKPT, ARM_INS_BKPT: bkpt	$val */
	{ 0 }
},
{	/* ARM_tBL, ARM_INS_BL: bl${p}	$func */
	{ 0 }
},
{	/* ARM_tBLXi, ARM_INS_BLX: blx${p}	$func */
	{ 0 }
},
{	/* ARM_tBLXr, ARM_INS_BLX: blx${p}	$func */
	{ CS_AC_READ, 0 }
},
{	/* ARM_tBX, ARM_INS_BX: bx${p}	$rm */
	{ CS_AC_READ, 0 }
},
{	/* ARM_tBcc, ARM_INS_B: b${p}	$target */
	{ 0 }
},
{	/* ARM_tCBNZ, ARM_INS_CBNZ: cbnz	$rn, $target */
	{ CS_AC_READ, 0 }
},
{	/* ARM_tCBZ, ARM_INS_CBZ: cbz	$rn, $target */
	{ CS_AC_READ, 0 }
},
{	/* ARM_tCMNz, ARM_INS_CMN: cmn${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_tCMPhir, ARM_INS_CMP: cmp${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_tCMPi8, ARM_INS_CMP: cmp${p}	$rn, $imm8 */
	{ CS_AC_READ, 0 }
},
{	/* ARM_tCMPr, ARM_INS_CMP: cmp${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_tCPS, ARM_INS_CPS: cps$imod $iflags */
	{ 0 }
},
{	/* ARM_tEOR, ARM_INS_EOR: eor${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tHINT, ARM_INS_HINT: hint${p}	$imm */
	{ 0 }
},
{	/* ARM_tHLT, ARM_INS_HLT: hlt	$val */
	{ 0 }
},
{	/* ARM_tLDMIA, ARM_INS_LDM: ldm${p}	$rn, $regs */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tLDRBi, ARM_INS_LDRB: ldrb${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tLDRBr, ARM_INS_LDRB: ldrb${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tLDRHi, ARM_INS_LDRH: ldrh${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tLDRHr, ARM_INS_LDRH: ldrh${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tLDRSB, ARM_INS_LDRSB: ldrsb${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tLDRSH, ARM_INS_LDRSH: ldrsh${p}	$rt, $addr */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tLDRi, ARM_INS_LDR: ldr${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tLDRpci, ARM_INS_LDR: ldr${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tLDRr, ARM_INS_LDR: ldr${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tLDRspi, ARM_INS_LDR: ldr${p}	$rt, $addr */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tLSLri, ARM_INS_LSL: lsl${s}${p}	$rd, $rm, $imm5 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tLSLrr, ARM_INS_LSL: lsl${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tLSRri, ARM_INS_LSR: lsr${s}${p}	$rd, $rm, $imm5 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tLSRrr, ARM_INS_LSR: lsr${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tMOVSr, ARM_INS_MOV: movs	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tMOVi8, ARM_INS_MOV: mov${s}${p}	$rd, $imm8 */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tMOVr, ARM_INS_MOV: mov${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tMUL, ARM_INS_MUL: mul${s}${p}	$rd, $rn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_tMVN, ARM_INS_MVN: mvn${s}${p}	$rd, $rn */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tORR, ARM_INS_ORR: orr${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tPOP, ARM_INS_POP: pop${p}	$regs */
	{ CS_AC_WRITE, 0 }
},
{	/* ARM_tPUSH, ARM_INS_PUSH: push${p}	$regs */
	{ CS_AC_READ, 0 }
},
{	/* ARM_tREV, ARM_INS_REV: rev${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tREV16, ARM_INS_REV16: rev16${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tREVSH, ARM_INS_REVSH: revsh${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tROR, ARM_INS_ROR: ror${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tRSB, ARM_INS_RSB: rsb${s}${p}	$rd, $rn, #0 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tSBC, ARM_INS_SBC: sbc${s}${p}	$rdn, $rm */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tSETEND, ARM_INS_SETEND: setend	$end */
	{ 0 }
},
{	/* ARM_tSTMIA_UPD, ARM_INS_STM: stm${p}	$rn!, $regs */
	{ CS_AC_READ | CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tSTRBi, ARM_INS_STRB: strb${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_tSTRBr, ARM_INS_STRB: strb${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_tSTRHi, ARM_INS_STRH: strh${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_tSTRHr, ARM_INS_STRH: strh${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_tSTRi, ARM_INS_STR: str${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_tSTRr, ARM_INS_STR: str${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_tSTRspi, ARM_INS_STR: str${p}	$rt, $addr */
	{ CS_AC_READ, CS_AC_WRITE, 0 }
},
{	/* ARM_tSUBi3, ARM_INS_SUB: sub${s}${p}	$rd, $rm, $imm3 */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tSUBi8, ARM_INS_SUB: sub${s}${p}	$rdn, $imm8 */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_tSUBrr, ARM_INS_SUB: sub${s}${p}	$rd, $rn, $rm */
	{ CS_AC_WRITE, CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_tSUBspi, ARM_INS_SUB: sub${p}	$rdn, $imm */
	{ CS_AC_READ | CS_AC_WRITE, 0 }
},
{	/* ARM_tSVC, ARM_INS_SVC: svc${p}	$imm */
	{ 0 }
},
{	/* ARM_tSXTB, ARM_INS_SXTB: sxtb${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tSXTH, ARM_INS_SXTH: sxth${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tTRAP, ARM_INS_TRAP: trap */
	{ 0 }
},
{	/* ARM_tTST, ARM_INS_TST: tst${p}	$rn, $rm */
	{ CS_AC_READ, CS_AC_READ, 0 }
},
{	/* ARM_tUDF, ARM_INS_UDF: udf	$imm8 */
	{ 0 }
},
{	/* ARM_tUXTB, ARM_INS_UXTB: uxtb${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
{	/* ARM_tUXTH, ARM_INS_UXTH: uxth${p}	$rd, $rm */
	{ CS_AC_WRITE, CS_AC_READ, 0 }
},
