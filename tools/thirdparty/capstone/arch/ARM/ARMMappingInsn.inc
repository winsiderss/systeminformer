// This is auto-gen data for Capstone engine (www.capstone-engine.org)
// By Nguyen Anh Quynh <aquynh@gmail.com>

{
	ARM_ADCri, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADCrr, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADCrsi, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADCrsr, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADDri, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADDrr, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADDrsi, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADDrsr, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ADR, ARM_INS_ADR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_AESD, ARM_INS_AESD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_AESE, ARM_INS_AESE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_AESIMC, ARM_INS_AESIMC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_AESMC, ARM_INS_AESMC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_ANDri, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ANDrr, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ANDrsi, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ANDrsr, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_BFC, ARM_INS_BFC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, 0 }, 0, 0
#endif
},
{
	ARM_BFI, ARM_INS_BFI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, 0 }, 0, 0
#endif
},
{
	ARM_BICri, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_BICrr, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_BICrsi, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_BICrsr, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_BKPT, ARM_INS_BKPT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_BL, ARM_INS_BL,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_CALL, ARM_GRP_BRANCH_RELATIVE, ARM_GRP_ARM, 0 }, 1, 0
#endif
},
{
	ARM_BLX, ARM_INS_BLX,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_CALL, ARM_GRP_V5T, 0 }, 0, 1
#endif
},
{
	ARM_BLX_pred, ARM_INS_BLX,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_CALL, ARM_GRP_ARM, ARM_GRP_V5T, 0 }, 0, 1
#endif
},
{
	ARM_BLXi, ARM_INS_BLX,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_CALL, ARM_GRP_BRANCH_RELATIVE, ARM_GRP_ARM, ARM_GRP_V5T, 0 }, 1, 0
#endif
},
{
	ARM_BL_pred, ARM_INS_BL,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_CALL, ARM_GRP_BRANCH_RELATIVE, ARM_GRP_ARM, 0 }, 1, 0
#endif
},
{
	ARM_BX, ARM_INS_BX,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_ARM, ARM_GRP_V4T, 0 }, 0, 1
#endif
},
{
	ARM_BXJ, ARM_INS_BXJ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_ARM, 0 }, 0, 1
#endif
},
{
	ARM_BX_RET, ARM_INS_BX,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_ARM, ARM_GRP_V4T, 0 }, 0, 1
#endif
},
{
	ARM_BX_pred, ARM_INS_BX,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_ARM, ARM_GRP_V4T, 0 }, 0, 1
#endif
},
{
	ARM_Bcc, ARM_INS_B,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_ARM, 0 }, 1, 0
#endif
},
{
	ARM_CDP, ARM_INS_CDP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_CDP2, ARM_INS_CDP2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_CLREX, ARM_INS_CLREX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_CLZ, ARM_INS_CLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5T, 0 }, 0, 0
#endif
},
{
	ARM_CMNri, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CMNzrr, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CMNzrsi, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CMNzrsr, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CMPri, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CMPrr, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CMPrsi, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CMPrsr, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CPS1p, ARM_INS_CPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CPS2p, ARM_INS_CPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CPS3p, ARM_INS_CPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_CRC32B, ARM_INS_CRC32B,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_CRC32CB, ARM_INS_CRC32CB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_CRC32CH, ARM_INS_CRC32CH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_CRC32CW, ARM_INS_CRC32CW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_CRC32H, ARM_INS_CRC32H,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_CRC32W, ARM_INS_CRC32W,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_DBG, ARM_INS_DBG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_DMB, ARM_INS_DMB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_DATABARRIER, 0 }, 0, 0
#endif
},
{
	ARM_DSB, ARM_INS_DSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_DATABARRIER, 0 }, 0, 0
#endif
},
{
	ARM_EORri, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_EORrr, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_EORrsi, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_EORrsr, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ERET, ARM_INS_ERET,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_ARM, ARM_GRP_VIRTUALIZATION, 0 }, 0, 0
#endif
},
{
	ARM_FCONSTD, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP3, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_FCONSTS, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP3, 0 }, 0, 0
#endif
},
{
	ARM_FLDMXDB_UPD, ARM_INS_FLDMDBX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_FLDMXIA, ARM_INS_FLDMIAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_FLDMXIA_UPD, ARM_INS_FLDMIAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_FMSTAT, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR_NZCV, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_FSTMXDB_UPD, ARM_INS_FSTMDBX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_FSTMXIA, ARM_INS_FSTMIAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_FSTMXIA_UPD, ARM_INS_FSTMIAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_HINT, ARM_INS_HINT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_HLT, ARM_INS_HLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_HVC, ARM_INS_HVC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_VIRTUALIZATION, 0 }, 0, 0
#endif
},
{
	ARM_ISB, ARM_INS_ISB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_DATABARRIER, 0 }, 0, 0
#endif
},
{
	ARM_LDA, ARM_INS_LDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_LDAB, ARM_INS_LDAB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_LDAEX, ARM_INS_LDAEX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_LDAEXB, ARM_INS_LDAEXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_LDAEXD, ARM_INS_LDAEXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_LDAEXH, ARM_INS_LDAEXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_LDAH, ARM_INS_LDAH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2L_OFFSET, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2L_OPTION, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2L_POST, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2L_PRE, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2_OFFSET, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2_OPTION, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2_POST, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDC2_PRE, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_LDCL_OFFSET, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDCL_OPTION, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDCL_POST, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDCL_PRE, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDC_OFFSET, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDC_OPTION, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDC_POST, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDC_PRE, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMDA, ARM_INS_LDMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMDA_UPD, ARM_INS_LDMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMDB, ARM_INS_LDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMDB_UPD, ARM_INS_LDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMIA, ARM_INS_LDM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMIA_UPD, ARM_INS_LDM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMIB, ARM_INS_LDMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDMIB_UPD, ARM_INS_LDMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRBT_POST_IMM, ARM_INS_LDRBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRBT_POST_REG, ARM_INS_LDRBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRB_POST_IMM, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRB_POST_REG, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRB_PRE_IMM, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRB_PRE_REG, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRBi12, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRBrs, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRD, ARM_INS_LDRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_LDRD_POST, ARM_INS_LDRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRD_PRE, ARM_INS_LDRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDREX, ARM_INS_LDREX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDREXB, ARM_INS_LDREXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDREXD, ARM_INS_LDREXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDREXH, ARM_INS_LDREXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRH, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRHTi, ARM_INS_LDRHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRHTr, ARM_INS_LDRHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRH_POST, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRH_PRE, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSB, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSBTi, ARM_INS_LDRSBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSBTr, ARM_INS_LDRSBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSB_POST, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSB_PRE, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSH, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSHTi, ARM_INS_LDRSHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSHTr, ARM_INS_LDRSHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSH_POST, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRSH_PRE, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRT_POST_IMM, ARM_INS_LDRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRT_POST_REG, ARM_INS_LDRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDR_POST_IMM, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDR_POST_REG, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDR_PRE_IMM, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDR_PRE_REG, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRcp, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRi12, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_LDRrs, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MCR, ARM_INS_MCR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MCR2, ARM_INS_MCR2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_MCRR, ARM_INS_MCRR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MCRR2, ARM_INS_MCRR2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_MLA, ARM_INS_MLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_MLS, ARM_INS_MLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_MOVPCLR, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MOVTi16, ARM_INS_MOVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, 0 }, 0, 0
#endif
},
{
	ARM_MOVi, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MOVi16, ARM_INS_MOVW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, 0 }, 0, 0
#endif
},
{
	ARM_MOVr, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MOVr_TC, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MOVsi, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MOVsr, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MRC, ARM_INS_MRC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MRC2, ARM_INS_MRC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_MRRC, ARM_INS_MRRC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MRRC2, ARM_INS_MRRC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_MRS, ARM_INS_MRS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MRSbanked, ARM_INS_MRS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_VIRTUALIZATION, 0 }, 0, 0
#endif
},
{
	ARM_MRSsys, ARM_INS_MRS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MSR, ARM_INS_MSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MSRbanked, ARM_INS_MSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_VIRTUALIZATION, 0 }, 0, 0
#endif
},
{
	ARM_MSRi, ARM_INS_MSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MUL, ARM_INS_MUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_MVNi, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MVNr, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MVNsi, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_MVNsr, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ORRri, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ORRrr, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ORRrsi, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_ORRrsr, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_PKHBT, ARM_INS_PKHBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_PKHTB, ARM_INS_PKHTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_PLDWi12, ARM_INS_PLDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V7, ARM_GRP_MULTPRO, 0 }, 0, 0
#endif
},
{
	ARM_PLDWrs, ARM_INS_PLDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V7, ARM_GRP_MULTPRO, 0 }, 0, 0
#endif
},
{
	ARM_PLDi12, ARM_INS_PLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_PLDrs, ARM_INS_PLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_PLIi12, ARM_INS_PLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_PLIrs, ARM_INS_PLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_QADD, ARM_INS_QADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QADD16, ARM_INS_QADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QADD8, ARM_INS_QADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QASX, ARM_INS_QASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QDADD, ARM_INS_QDADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QDSUB, ARM_INS_QDSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QSAX, ARM_INS_QSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QSUB, ARM_INS_QSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QSUB16, ARM_INS_QSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_QSUB8, ARM_INS_QSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RBIT, ARM_INS_RBIT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, 0 }, 0, 0
#endif
},
{
	ARM_REV, ARM_INS_REV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_REV16, ARM_INS_REV16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_REVSH, ARM_INS_REVSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_RFEDA, ARM_INS_RFEDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RFEDA_UPD, ARM_INS_RFEDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RFEDB, ARM_INS_RFEDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RFEDB_UPD, ARM_INS_RFEDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RFEIA, ARM_INS_RFEIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RFEIA_UPD, ARM_INS_RFEIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RFEIB, ARM_INS_RFEIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RFEIB_UPD, ARM_INS_RFEIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSBri, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSBrr, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSBrsi, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSBrsr, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSCri, ARM_INS_RSC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSCrr, ARM_INS_RSC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSCrsi, ARM_INS_RSC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_RSCrsr, ARM_INS_RSC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SADD16, ARM_INS_SADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SADD8, ARM_INS_SADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SASX, ARM_INS_SASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SBCri, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SBCrr, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SBCrsi, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SBCrsr, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SBFX, ARM_INS_SBFX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, 0 }, 0, 0
#endif
},
{
	ARM_SDIV, ARM_INS_SDIV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SEL, ARM_INS_SEL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SETEND, ARM_INS_SETEND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SHA1C, ARM_INS_SHA1C,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA1H, ARM_INS_SHA1H,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA1M, ARM_INS_SHA1M,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA1P, ARM_INS_SHA1P,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA1SU0, ARM_INS_SHA1SU0,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA1SU1, ARM_INS_SHA1SU1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA256H, ARM_INS_SHA256H,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA256H2, ARM_INS_SHA256H2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA256SU0, ARM_INS_SHA256SU0,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHA256SU1, ARM_INS_SHA256SU1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_SHADD16, ARM_INS_SHADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SHADD8, ARM_INS_SHADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SHASX, ARM_INS_SHASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SHSAX, ARM_INS_SHSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SHSUB16, ARM_INS_SHSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SHSUB8, ARM_INS_SHSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SMC, ARM_INS_SMC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_ARM, ARM_GRP_TRUSTZONE, 0 }, 0, 0
#endif
},
{
	ARM_SMLABB, ARM_INS_SMLABB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMLABT, ARM_INS_SMLABT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMLAD, ARM_INS_SMLAD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLADX, ARM_INS_SMLADX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLAL, ARM_INS_SMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLALBB, ARM_INS_SMLALBB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMLALBT, ARM_INS_SMLALBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMLALD, ARM_INS_SMLALD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLALDX, ARM_INS_SMLALDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLALTB, ARM_INS_SMLALTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMLALTT, ARM_INS_SMLALTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMLATB, ARM_INS_SMLATB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMLATT, ARM_INS_SMLATT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMLAWB, ARM_INS_SMLAWB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMLAWT, ARM_INS_SMLAWT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMLSD, ARM_INS_SMLSD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLSDX, ARM_INS_SMLSDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLSLD, ARM_INS_SMLSLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMLSLDX, ARM_INS_SMLSLDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMMLA, ARM_INS_SMMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMMLAR, ARM_INS_SMMLAR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMMLS, ARM_INS_SMMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_SMMLSR, ARM_INS_SMMLSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMMUL, ARM_INS_SMMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMMULR, ARM_INS_SMMULR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMUAD, ARM_INS_SMUAD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMUADX, ARM_INS_SMUADX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMULBB, ARM_INS_SMULBB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMULBT, ARM_INS_SMULBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMULL, ARM_INS_SMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMULTB, ARM_INS_SMULTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMULTT, ARM_INS_SMULTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMULWB, ARM_INS_SMULWB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMULWT, ARM_INS_SMULWT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_SMUSD, ARM_INS_SMUSD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SMUSDX, ARM_INS_SMUSDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SRSDA, ARM_INS_SRSDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SRSDA_UPD, ARM_INS_SRSDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SRSDB, ARM_INS_SRSDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SRSDB_UPD, ARM_INS_SRSDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SRSIA, ARM_INS_SRSIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SRSIA_UPD, ARM_INS_SRSIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SRSIB, ARM_INS_SRSIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SRSIB_UPD, ARM_INS_SRSIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SSAT, ARM_INS_SSAT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SSAT16, ARM_INS_SSAT16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SSAX, ARM_INS_SSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SSUB16, ARM_INS_SSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SSUB8, ARM_INS_SSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STC2L_OFFSET, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STC2L_OPTION, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STC2L_POST, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STC2L_PRE, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STC2_OFFSET, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STC2_OPTION, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STC2_POST, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STC2_PRE, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_STCL_OFFSET, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STCL_OPTION, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STCL_POST, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STCL_PRE, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STC_OFFSET, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STC_OPTION, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STC_POST, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STC_PRE, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STL, ARM_INS_STL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_STLB, ARM_INS_STLB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_STLEX, ARM_INS_STLEX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_STLEXB, ARM_INS_STLEXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_STLEXD, ARM_INS_STLEXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_STLEXH, ARM_INS_STLEXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_STLH, ARM_INS_STLH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_STMDA, ARM_INS_STMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STMDA_UPD, ARM_INS_STMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STMDB, ARM_INS_STMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STMDB_UPD, ARM_INS_STMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STMIA, ARM_INS_STM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STMIA_UPD, ARM_INS_STM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STMIB, ARM_INS_STMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STMIB_UPD, ARM_INS_STMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRBT_POST_IMM, ARM_INS_STRBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRBT_POST_REG, ARM_INS_STRBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRB_POST_IMM, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRB_POST_REG, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRB_PRE_IMM, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRB_PRE_REG, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRBi12, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRBrs, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRD, ARM_INS_STRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V5TE, 0 }, 0, 0
#endif
},
{
	ARM_STRD_POST, ARM_INS_STRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRD_PRE, ARM_INS_STRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STREX, ARM_INS_STREX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STREXB, ARM_INS_STREXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STREXD, ARM_INS_STREXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STREXH, ARM_INS_STREXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRH, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRHTi, ARM_INS_STRHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRHTr, ARM_INS_STRHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRH_POST, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRH_PRE, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRT_POST_IMM, ARM_INS_STRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRT_POST_REG, ARM_INS_STRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STR_POST_IMM, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STR_POST_REG, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STR_PRE_IMM, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STR_PRE_REG, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRi12, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_STRrs, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SUBri, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SUBrr, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SUBrsi, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SUBrsr, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_SVC, ARM_INS_SVC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, 0 }, { ARM_GRP_ARM, ARM_GRP_INT, 0 }, 0, 0
#endif
},
{
	ARM_SWP, ARM_INS_SWP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_SWPB, ARM_INS_SWPB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_SXTAB, ARM_INS_SXTAB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SXTAB16, ARM_INS_SXTAB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SXTAH, ARM_INS_SXTAH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SXTB, ARM_INS_SXTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SXTB16, ARM_INS_SXTB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_SXTH, ARM_INS_SXTH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_TEQri, ARM_INS_TEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TEQrr, ARM_INS_TEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TEQrsi, ARM_INS_TEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TEQrsr, ARM_INS_TEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TRAP, ARM_INS_TRAP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TRAPNaCl, ARM_INS_TRAP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TSTri, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TSTrr, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TSTrsi, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_TSTrsr, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UADD16, ARM_INS_UADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UADD8, ARM_INS_UADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UASX, ARM_INS_UASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UBFX, ARM_INS_UBFX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6T2, 0 }, 0, 0
#endif
},
{
	ARM_UDF, ARM_INS_UDF,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UDIV, ARM_INS_UDIV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UHADD16, ARM_INS_UHADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UHADD8, ARM_INS_UHADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UHASX, ARM_INS_UHASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UHSAX, ARM_INS_UHSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UHSUB16, ARM_INS_UHSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UHSUB8, ARM_INS_UHSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UMAAL, ARM_INS_UMAAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UMLAL, ARM_INS_UMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UMULL, ARM_INS_UMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UQADD16, ARM_INS_UQADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UQADD8, ARM_INS_UQADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UQASX, ARM_INS_UQASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UQSAX, ARM_INS_UQSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UQSUB16, ARM_INS_UQSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UQSUB8, ARM_INS_UQSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_USAD8, ARM_INS_USAD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_USADA8, ARM_INS_USADA8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_USAT, ARM_INS_USAT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_USAT16, ARM_INS_USAT16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_USAX, ARM_INS_USAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_USUB16, ARM_INS_USUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_USUB8, ARM_INS_USUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_UXTAB, ARM_INS_UXTAB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UXTAB16, ARM_INS_UXTAB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UXTAH, ARM_INS_UXTAH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UXTB, ARM_INS_UXTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UXTB16, ARM_INS_UXTB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_UXTH, ARM_INS_UXTH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_VABALsv2i64, ARM_INS_VABAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABALsv4i32, ARM_INS_VABAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABALsv8i16, ARM_INS_VABAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABALuv2i64, ARM_INS_VABAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABALuv4i32, ARM_INS_VABAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABALuv8i16, ARM_INS_VABAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAsv16i8, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAsv2i32, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAsv4i16, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAsv4i32, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAsv8i16, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAsv8i8, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAuv16i8, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAuv2i32, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAuv4i16, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAuv4i32, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAuv8i16, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABAuv8i8, ARM_INS_VABA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDLsv2i64, ARM_INS_VABDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDLsv4i32, ARM_INS_VABDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDLsv8i16, ARM_INS_VABDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDLuv2i64, ARM_INS_VABDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDLuv4i32, ARM_INS_VABDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDLuv8i16, ARM_INS_VABDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDfd, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDfq, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDsv16i8, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDsv2i32, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDsv4i16, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDsv4i32, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDsv8i16, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDsv8i8, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDuv16i8, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDuv2i32, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDuv4i16, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDuv4i32, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDuv8i16, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABDuv8i8, ARM_INS_VABD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSD, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VABSS, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VABSfd, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSfq, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSv16i8, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSv2i32, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSv4i16, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSv4i32, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSv8i16, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VABSv8i8, ARM_INS_VABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VACGEd, ARM_INS_VACGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VACGEq, ARM_INS_VACGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VACGTd, ARM_INS_VACGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VACGTq, ARM_INS_VACGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDD, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VADDHNv2i32, ARM_INS_VADDHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDHNv4i16, ARM_INS_VADDHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDHNv8i8, ARM_INS_VADDHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDLsv2i64, ARM_INS_VADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDLsv4i32, ARM_INS_VADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDLsv8i16, ARM_INS_VADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDLuv2i64, ARM_INS_VADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDLuv4i32, ARM_INS_VADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDLuv8i16, ARM_INS_VADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDS, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VADDWsv2i64, ARM_INS_VADDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDWsv4i32, ARM_INS_VADDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDWsv8i16, ARM_INS_VADDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDWuv2i64, ARM_INS_VADDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDWuv4i32, ARM_INS_VADDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDWuv8i16, ARM_INS_VADDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDfd, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDfq, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv16i8, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv1i64, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv2i32, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv2i64, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv4i16, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv4i32, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv8i16, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VADDv8i8, ARM_INS_VADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VANDd, ARM_INS_VAND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VANDq, ARM_INS_VAND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBICd, ARM_INS_VBIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBICiv2i32, ARM_INS_VBIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBICiv4i16, ARM_INS_VBIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBICiv4i32, ARM_INS_VBIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBICiv8i16, ARM_INS_VBIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBICq, ARM_INS_VBIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBIFd, ARM_INS_VBIF,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBIFq, ARM_INS_VBIF,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBITd, ARM_INS_VBIT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBITq, ARM_INS_VBIT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBSLd, ARM_INS_VBSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VBSLq, ARM_INS_VBSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQfd, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQfq, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQv16i8, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQv2i32, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQv4i16, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQv4i32, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQv8i16, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQv8i8, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv16i8, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv2f32, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv2i32, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv4f32, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv4i16, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv4i32, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv8i16, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCEQzv8i8, ARM_INS_VCEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEfd, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEfq, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEsv16i8, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEsv2i32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEsv4i16, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEsv4i32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEsv8i16, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEsv8i8, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEuv16i8, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEuv2i32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEuv4i16, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEuv4i32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEuv8i16, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEuv8i8, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv16i8, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv2f32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv2i32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv4f32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv4i16, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv4i32, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv8i16, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGEzv8i8, ARM_INS_VCGE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTfd, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTfq, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTsv16i8, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTsv2i32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTsv4i16, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTsv4i32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTsv8i16, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTsv8i8, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTuv16i8, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTuv2i32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTuv4i16, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTuv4i32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTuv8i16, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTuv8i8, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv16i8, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv2f32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv2i32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv4f32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv4i16, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv4i32, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv8i16, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCGTzv8i8, ARM_INS_VCGT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv16i8, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv2f32, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv2i32, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv4f32, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv4i16, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv4i32, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv8i16, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLEzv8i8, ARM_INS_VCLE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLSv16i8, ARM_INS_VCLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLSv2i32, ARM_INS_VCLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLSv4i16, ARM_INS_VCLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLSv4i32, ARM_INS_VCLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLSv8i16, ARM_INS_VCLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLSv8i8, ARM_INS_VCLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv16i8, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv2f32, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv2i32, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv4f32, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv4i16, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv4i32, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv8i16, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLTzv8i8, ARM_INS_VCLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLZv16i8, ARM_INS_VCLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLZv2i32, ARM_INS_VCLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLZv4i16, ARM_INS_VCLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLZv4i32, ARM_INS_VCLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLZv8i16, ARM_INS_VCLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCLZv8i8, ARM_INS_VCLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCMPD, ARM_INS_VCMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCMPED, ARM_INS_VCMPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCMPES, ARM_INS_VCMPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCMPEZD, ARM_INS_VCMPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCMPEZS, ARM_INS_VCMPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCMPS, ARM_INS_VCMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCMPZD, ARM_INS_VCMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCMPZS, ARM_INS_VCMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR_NZCV, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCNTd, ARM_INS_VCNT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCNTq, ARM_INS_VCNT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTANSD, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTANSQ, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTANUD, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTANUQ, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTASD, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTASS, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTAUD, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTAUS, ARM_INS_VCVTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTBDH, ARM_INS_VCVTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTBHD, ARM_INS_VCVTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTBHS, ARM_INS_VCVTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCVTBSH, ARM_INS_VCVTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCVTDS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMNSD, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMNSQ, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMNUD, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMNUQ, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMSD, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMSS, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMUD, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTMUS, ARM_INS_VCVTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNNSD, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNNSQ, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNNUD, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNNUQ, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNSD, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNSS, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNUD, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTNUS, ARM_INS_VCVTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPNSD, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPNSQ, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPNUD, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPNUQ, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPSD, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPSS, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPUD, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTPUS, ARM_INS_VCVTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VCVTSD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTTDH, ARM_INS_VCVTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTTHD, ARM_INS_VCVTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VCVTTHS, ARM_INS_VCVTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCVTTSH, ARM_INS_VCVTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2h, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2sd, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2sq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2ud, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2uq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2xsd, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2xsq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2xud, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTf2xuq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTh2f, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTs2fd, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTs2fq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTu2fd, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTu2fq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTxs2fd, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTxs2fq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTxu2fd, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VCVTxu2fq, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDIVD, ARM_INS_VDIV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VDIVS, ARM_INS_VDIV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VDUP16d, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUP16q, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUP32d, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUP32q, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUP8d, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUP8q, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUPLN16d, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUPLN16q, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUPLN32d, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUPLN32q, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUPLN8d, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VDUPLN8q, ARM_INS_VDUP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEORd, ARM_INS_VEOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEORq, ARM_INS_VEOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEXTd16, ARM_INS_VEXT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEXTd32, ARM_INS_VEXT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEXTd8, ARM_INS_VEXT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEXTq16, ARM_INS_VEXT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEXTq32, ARM_INS_VEXT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEXTq64, ARM_INS_VEXT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VEXTq8, ARM_INS_VEXT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VFMAD, ARM_INS_VFMA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VFMAS, ARM_INS_VFMA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VFMAfd, ARM_INS_VFMA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VFMAfq, ARM_INS_VFMA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VFMSD, ARM_INS_VFMS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VFMSS, ARM_INS_VFMS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VFMSfd, ARM_INS_VFMS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VFMSfq, ARM_INS_VFMS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VFNMAD, ARM_INS_VFNMA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VFNMAS, ARM_INS_VFNMA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VFNMSD, ARM_INS_VFNMS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VFNMSS, ARM_INS_VFNMS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP4, 0 }, 0, 0
#endif
},
{
	ARM_VGETLNi32, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VGETLNs16, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VGETLNs8, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VGETLNu16, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VGETLNu8, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDsv16i8, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDsv2i32, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDsv4i16, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDsv4i32, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDsv8i16, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDsv8i8, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDuv16i8, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDuv2i32, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDuv4i16, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDuv4i32, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDuv8i16, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHADDuv8i8, ARM_INS_VHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBsv16i8, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBsv2i32, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBsv4i16, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBsv4i32, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBsv8i16, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBsv8i8, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBuv16i8, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBuv2i32, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBuv4i16, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBuv4i32, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBuv8i16, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VHSUBuv8i8, ARM_INS_VHSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd16, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd16wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd16wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd32, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd32wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd32wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd8, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd8wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPd8wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq16, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq16wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq16wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq32, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq32wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq32wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq8, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq8wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1DUPq8wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1LNd16, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1LNd16_UPD, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1LNd32, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1LNd32_UPD, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1LNd8, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1LNd8_UPD, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16Q, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16Qwb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16Qwb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16T, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16Twb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16Twb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d16wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32Q, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32Qwb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32Qwb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32T, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32Twb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32Twb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d32wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64Q, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64Qwb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64Qwb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64T, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64Twb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64Twb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d64wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8Q, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8Qwb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8Qwb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8T, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8Twb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8Twb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1d8wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q16, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q16wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q16wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q32, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q32wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q32wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q64, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q64wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q64wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q8, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q8wb_fixed, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD1q8wb_register, ARM_INS_VLD1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd16, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd16wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd16wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd16x2, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd16x2wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd16x2wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd32, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd32wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd32wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd32x2, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd32x2wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd32x2wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd8, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd8wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd8wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd8x2, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd8x2wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2DUPd8x2wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNd16, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNd16_UPD, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNd32, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNd32_UPD, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNd8, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNd8_UPD, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNq16, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNq16_UPD, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNq32, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2LNq32_UPD, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b16, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b16wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b16wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b32, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b32wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b32wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b8, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b8wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2b8wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d16, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d16wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d16wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d32, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d32wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d32wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d8, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d8wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2d8wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q16, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q16wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q16wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q32, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q32wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q32wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q8, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q8wb_fixed, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD2q8wb_register, ARM_INS_VLD2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPd16, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPd16_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPd32, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPd32_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPd8, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPd8_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPq16, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPq16_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPq32, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPq32_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPq8, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3DUPq8_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNd16, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNd16_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNd32, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNd32_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNd8, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNd8_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNq16, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNq16_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNq32, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3LNq32_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3d16, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3d16_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3d32, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3d32_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3d8, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3d8_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3q16, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3q16_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3q32, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3q32_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3q8, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD3q8_UPD, ARM_INS_VLD3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPd16, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPd16_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPd32, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPd32_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPd8, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPd8_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPq16, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPq16_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPq32, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPq32_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPq8, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4DUPq8_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNd16, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNd16_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNd32, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNd32_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNd8, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNd8_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNq16, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNq16_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNq32, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4LNq32_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4d16, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4d16_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4d32, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4d32_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4d8, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4d8_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4q16, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4q16_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4q32, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4q32_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4q8, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLD4q8_UPD, ARM_INS_VLD4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VLDMDDB_UPD, ARM_INS_VLDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VLDMDIA, ARM_INS_VLDMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VLDMDIA_UPD, ARM_INS_VLDMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VLDMSDB_UPD, ARM_INS_VLDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VLDMSIA, ARM_INS_VLDMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VLDMSIA_UPD, ARM_INS_VLDMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VLDRD, ARM_INS_VLDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VLDRS, ARM_INS_VLDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMAXNMD, ARM_INS_VMAXNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VMAXNMND, ARM_INS_VMAXNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXNMNQ, ARM_INS_VMAXNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXNMS, ARM_INS_VMAXNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VMAXfd, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXfq, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXsv16i8, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXsv2i32, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXsv4i16, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXsv4i32, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXsv8i16, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXsv8i8, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXuv16i8, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXuv2i32, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXuv4i16, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXuv4i32, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXuv8i16, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMAXuv8i8, ARM_INS_VMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINNMD, ARM_INS_VMINNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VMINNMND, ARM_INS_VMINNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINNMNQ, ARM_INS_VMINNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINNMS, ARM_INS_VMINNM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VMINfd, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINfq, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINsv16i8, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINsv2i32, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINsv4i16, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINsv4i32, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINsv8i16, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINsv8i8, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINuv16i8, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINuv2i32, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINuv4i16, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINuv4i32, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINuv8i16, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMINuv8i8, ARM_INS_VMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAD, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLALslsv2i32, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALslsv4i16, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALsluv2i32, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALsluv4i16, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALsv2i64, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALsv4i32, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALsv8i16, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALuv2i64, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALuv4i32, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLALuv8i16, ARM_INS_VMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAS, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLAfd, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLAfq, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLAslfd, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLAslfq, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLAslv2i32, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAslv4i16, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAslv4i32, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAslv8i16, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAv16i8, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAv2i32, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAv4i16, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAv4i32, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAv8i16, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLAv8i8, ARM_INS_VMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSD, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLslsv2i32, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLslsv4i16, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLsluv2i32, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLsluv4i16, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLsv2i64, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLsv4i32, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLsv8i16, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLuv2i64, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLuv4i32, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSLuv8i16, ARM_INS_VMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSS, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLSfd, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLSfq, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLSslfd, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLSslfq, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VMLSslv2i32, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSslv4i16, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSslv4i32, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSslv8i16, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSv16i8, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSv2i32, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSv4i16, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSv4i32, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSv8i16, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMLSv8i8, ARM_INS_VMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVD, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VMOVDRR, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMOVLsv2i64, ARM_INS_VMOVL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVLsv4i32, ARM_INS_VMOVL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVLsv8i16, ARM_INS_VMOVL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVLuv2i64, ARM_INS_VMOVL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVLuv4i32, ARM_INS_VMOVL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVLuv8i16, ARM_INS_VMOVL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVNv2i32, ARM_INS_VMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVNv4i16, ARM_INS_VMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVNv8i8, ARM_INS_VMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVRRD, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMOVRRS, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMOVRS, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMOVS, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMOVSR, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMOVSRR, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv16i8, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv1i64, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv2f32, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv2i32, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv2i64, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv4f32, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv4i16, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv4i32, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv8i16, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMOVv8i8, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMRS, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMRS_FPEXC, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMRS_FPINST, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMRS_FPINST2, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMRS_FPSID, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMRS_MVFR0, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMRS_MVFR1, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMRS_MVFR2, ARM_INS_VMRS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VMSR, ARM_INS_VMSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMSR_FPEXC, ARM_INS_VMSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMSR_FPINST, ARM_INS_VMSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMSR_FPINST2, ARM_INS_VMSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMSR_FPSID, ARM_INS_VMSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_FPSCR, 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMULD, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VMULLp64, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_CRYPTO, 0 }, 0, 0
#endif
},
{
	ARM_VMULLp8, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLslsv2i32, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLslsv4i16, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLsluv2i32, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLsluv4i16, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLsv2i64, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLsv4i32, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLsv8i16, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLuv2i64, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLuv4i32, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULLuv8i16, ARM_INS_VMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULS, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VMULfd, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULfq, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULpd, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULpq, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULslfd, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULslfq, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULslv2i32, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULslv4i16, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULslv4i32, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULslv8i16, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULv16i8, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULv2i32, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULv4i16, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULv4i32, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULv8i16, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMULv8i8, ARM_INS_VMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMVNd, ARM_INS_VMVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMVNq, ARM_INS_VMVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMVNv2i32, ARM_INS_VMVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMVNv4i16, ARM_INS_VMVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMVNv4i32, ARM_INS_VMVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VMVNv8i16, ARM_INS_VMVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGD, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VNEGS, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VNEGf32q, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGfd, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGs16d, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGs16q, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGs32d, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGs32q, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGs8d, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNEGs8q, ARM_INS_VNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VNMLAD, ARM_INS_VNMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VNMLAS, ARM_INS_VNMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VNMLSD, ARM_INS_VNMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VNMLSS, ARM_INS_VNMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_FPVMLX, 0 }, 0, 0
#endif
},
{
	ARM_VNMULD, ARM_INS_VNMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VNMULS, ARM_INS_VNMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VORNd, ARM_INS_VORN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VORNq, ARM_INS_VORN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VORRd, ARM_INS_VORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VORRiv2i32, ARM_INS_VORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VORRiv4i16, ARM_INS_VORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VORRiv4i32, ARM_INS_VORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VORRiv8i16, ARM_INS_VORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VORRq, ARM_INS_VORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALsv16i8, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALsv2i32, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALsv4i16, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALsv4i32, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALsv8i16, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALsv8i8, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALuv16i8, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALuv2i32, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALuv4i16, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALuv4i32, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALuv8i16, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADALuv8i8, ARM_INS_VPADAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLsv16i8, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLsv2i32, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLsv4i16, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLsv4i32, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLsv8i16, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLsv8i8, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLuv16i8, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLuv2i32, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLuv4i16, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLuv4i32, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLuv8i16, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDLuv8i8, ARM_INS_VPADDL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDf, ARM_INS_VPADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDi16, ARM_INS_VPADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDi32, ARM_INS_VPADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPADDi8, ARM_INS_VPADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMAXf, ARM_INS_VPMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMAXs16, ARM_INS_VPMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMAXs32, ARM_INS_VPMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMAXs8, ARM_INS_VPMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMAXu16, ARM_INS_VPMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMAXu32, ARM_INS_VPMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMAXu8, ARM_INS_VPMAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMINf, ARM_INS_VPMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMINs16, ARM_INS_VPMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMINs32, ARM_INS_VPMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMINs8, ARM_INS_VPMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMINu16, ARM_INS_VPMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMINu32, ARM_INS_VPMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VPMINu8, ARM_INS_VPMIN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQABSv16i8, ARM_INS_VQABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQABSv2i32, ARM_INS_VQABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQABSv4i16, ARM_INS_VQABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQABSv4i32, ARM_INS_VQABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQABSv8i16, ARM_INS_VQABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQABSv8i8, ARM_INS_VQABS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv16i8, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv1i64, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv2i32, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv2i64, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv4i16, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv4i32, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv8i16, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDsv8i8, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv16i8, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv1i64, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv2i32, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv2i64, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv4i16, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv4i32, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv8i16, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQADDuv8i8, ARM_INS_VQADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLALslv2i32, ARM_INS_VQDMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLALslv4i16, ARM_INS_VQDMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLALv2i64, ARM_INS_VQDMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLALv4i32, ARM_INS_VQDMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLSLslv2i32, ARM_INS_VQDMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLSLslv4i16, ARM_INS_VQDMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLSLv2i64, ARM_INS_VQDMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMLSLv4i32, ARM_INS_VQDMLSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHslv2i32, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHslv4i16, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHslv4i32, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHslv8i16, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHv2i32, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHv4i16, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHv4i32, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULHv8i16, ARM_INS_VQDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULLslv2i32, ARM_INS_VQDMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULLslv4i16, ARM_INS_VQDMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULLv2i64, ARM_INS_VQDMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQDMULLv4i32, ARM_INS_VQDMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNsuv2i32, ARM_INS_VQMOVUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNsuv4i16, ARM_INS_VQMOVUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNsuv8i8, ARM_INS_VQMOVUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNsv2i32, ARM_INS_VQMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNsv4i16, ARM_INS_VQMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNsv8i8, ARM_INS_VQMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNuv2i32, ARM_INS_VQMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNuv4i16, ARM_INS_VQMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQMOVNuv8i8, ARM_INS_VQMOVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQNEGv16i8, ARM_INS_VQNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQNEGv2i32, ARM_INS_VQNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQNEGv4i16, ARM_INS_VQNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQNEGv4i32, ARM_INS_VQNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQNEGv8i16, ARM_INS_VQNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQNEGv8i8, ARM_INS_VQNEG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHslv2i32, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHslv4i16, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHslv4i32, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHslv8i16, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHv2i32, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHv4i16, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHv4i32, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRDMULHv8i16, ARM_INS_VQRDMULH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv16i8, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv1i64, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv2i32, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv2i64, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv4i16, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv4i32, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv8i16, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLsv8i8, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv16i8, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv1i64, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv2i32, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv2i64, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv4i16, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv4i32, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv8i16, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHLuv8i8, ARM_INS_VQRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRNsv2i32, ARM_INS_VQRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRNsv4i16, ARM_INS_VQRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRNsv8i8, ARM_INS_VQRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRNuv2i32, ARM_INS_VQRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRNuv4i16, ARM_INS_VQRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRNuv8i8, ARM_INS_VQRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRUNv2i32, ARM_INS_VQRSHRUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRUNv4i16, ARM_INS_VQRSHRUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQRSHRUNv8i8, ARM_INS_VQRSHRUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv16i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv1i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv2i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv2i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv4i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv4i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv8i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsiv8i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv16i8, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv1i64, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv2i32, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv2i64, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv4i16, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv4i32, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv8i16, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsuv8i8, ARM_INS_VQSHLU,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv16i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv1i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv2i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv2i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv4i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv4i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv8i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLsv8i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv16i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv1i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv2i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv2i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv4i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv4i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv8i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuiv8i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv16i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv1i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv2i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv2i64, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv4i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv4i32, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv8i16, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHLuv8i8, ARM_INS_VQSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRNsv2i32, ARM_INS_VQSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRNsv4i16, ARM_INS_VQSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRNsv8i8, ARM_INS_VQSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRNuv2i32, ARM_INS_VQSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRNuv4i16, ARM_INS_VQSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRNuv8i8, ARM_INS_VQSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRUNv2i32, ARM_INS_VQSHRUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRUNv4i16, ARM_INS_VQSHRUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSHRUNv8i8, ARM_INS_VQSHRUN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv16i8, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv1i64, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv2i32, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv2i64, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv4i16, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv4i32, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv8i16, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBsv8i8, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv16i8, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv1i64, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv2i32, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv2i64, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv4i16, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv4i32, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv8i16, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VQSUBuv8i8, ARM_INS_VQSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRADDHNv2i32, ARM_INS_VRADDHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRADDHNv4i16, ARM_INS_VRADDHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRADDHNv8i8, ARM_INS_VRADDHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRECPEd, ARM_INS_VRECPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRECPEfd, ARM_INS_VRECPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRECPEfq, ARM_INS_VRECPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRECPEq, ARM_INS_VRECPE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRECPSfd, ARM_INS_VRECPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRECPSfq, ARM_INS_VRECPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV16d8, ARM_INS_VREV16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV16q8, ARM_INS_VREV16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV32d16, ARM_INS_VREV32,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV32d8, ARM_INS_VREV32,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV32q16, ARM_INS_VREV32,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV32q8, ARM_INS_VREV32,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV64d16, ARM_INS_VREV64,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV64d32, ARM_INS_VREV64,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV64d8, ARM_INS_VREV64,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV64q16, ARM_INS_VREV64,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV64q32, ARM_INS_VREV64,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VREV64q8, ARM_INS_VREV64,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDsv16i8, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDsv2i32, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDsv4i16, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDsv4i32, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDsv8i16, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDsv8i8, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDuv16i8, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDuv2i32, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDuv4i16, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDuv4i32, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDuv8i16, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRHADDuv8i8, ARM_INS_VRHADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTAD, ARM_INS_VRINTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VRINTAND, ARM_INS_VRINTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTANQ, ARM_INS_VRINTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTAS, ARM_INS_VRINTA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VRINTMD, ARM_INS_VRINTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VRINTMND, ARM_INS_VRINTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTMNQ, ARM_INS_VRINTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTMS, ARM_INS_VRINTM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VRINTND, ARM_INS_VRINTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VRINTNND, ARM_INS_VRINTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTNNQ, ARM_INS_VRINTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTNS, ARM_INS_VRINTN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VRINTPD, ARM_INS_VRINTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VRINTPND, ARM_INS_VRINTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTPNQ, ARM_INS_VRINTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTPS, ARM_INS_VRINTP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VRINTRD, ARM_INS_VRINTR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VRINTRS, ARM_INS_VRINTR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VRINTXD, ARM_INS_VRINTX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VRINTXND, ARM_INS_VRINTX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTXNQ, ARM_INS_VRINTX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTXS, ARM_INS_VRINTX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VRINTZD, ARM_INS_VRINTZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VRINTZND, ARM_INS_VRINTZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTZNQ, ARM_INS_VRINTZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_V8, ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRINTZS, ARM_INS_VRINTZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv16i8, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv1i64, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv2i32, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv2i64, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv4i16, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv4i32, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv8i16, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLsv8i8, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv16i8, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv1i64, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv2i32, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv2i64, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv4i16, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv4i32, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv8i16, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHLuv8i8, ARM_INS_VRSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRNv2i32, ARM_INS_VRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRNv4i16, ARM_INS_VRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRNv8i8, ARM_INS_VRSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv16i8, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv1i64, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv2i32, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv2i64, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv4i16, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv4i32, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv8i16, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRsv8i8, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv16i8, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv1i64, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv2i32, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv2i64, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv4i16, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv4i32, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv8i16, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSHRuv8i8, ARM_INS_VRSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSQRTEd, ARM_INS_VRSQRTE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSQRTEfd, ARM_INS_VRSQRTE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSQRTEfq, ARM_INS_VRSQRTE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSQRTEq, ARM_INS_VRSQRTE,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSQRTSfd, ARM_INS_VRSQRTS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSQRTSfq, ARM_INS_VRSQRTS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv16i8, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv1i64, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv2i32, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv2i64, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv4i16, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv4i32, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv8i16, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAsv8i8, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv16i8, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv1i64, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv2i32, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv2i64, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv4i16, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv4i32, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv8i16, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSRAuv8i8, ARM_INS_VRSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSUBHNv2i32, ARM_INS_VRSUBHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSUBHNv4i16, ARM_INS_VRSUBHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VRSUBHNv8i8, ARM_INS_VRSUBHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSELEQD, ARM_INS_VSELEQ,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSELEQS, ARM_INS_VSELEQ,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VSELGED, ARM_INS_VSELGE,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSELGES, ARM_INS_VSELGE,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VSELGTD, ARM_INS_VSELGT,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSELGTS, ARM_INS_VSELGT,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VSELVSD, ARM_INS_VSELVS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSELVSS, ARM_INS_VSELVS,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_FPARMV8, 0 }, 0, 0
#endif
},
{
	ARM_VSETLNi16, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSETLNi32, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSETLNi8, ARM_INS_VMOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLi16, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLi32, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLi8, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLsv2i64, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLsv4i32, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLsv8i16, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLuv2i64, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLuv4i32, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLLuv8i16, ARM_INS_VSHLL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv16i8, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv1i64, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv2i32, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv2i64, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv4i16, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv4i32, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv8i16, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLiv8i8, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv16i8, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv1i64, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv2i32, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv2i64, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv4i16, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv4i32, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv8i16, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLsv8i8, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv16i8, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv1i64, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv2i32, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv2i64, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv4i16, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv4i32, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv8i16, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHLuv8i8, ARM_INS_VSHL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRNv2i32, ARM_INS_VSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRNv4i16, ARM_INS_VSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRNv8i8, ARM_INS_VSHRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv16i8, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv1i64, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv2i32, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv2i64, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv4i16, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv4i32, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv8i16, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRsv8i8, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv16i8, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv1i64, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv2i32, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv2i64, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv4i16, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv4i32, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv8i16, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHRuv8i8, ARM_INS_VSHR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSHTOD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSHTOS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSITOD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSITOS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv16i8, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv1i64, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv2i32, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv2i64, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv4i16, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv4i32, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv8i16, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLIv8i8, ARM_INS_VSLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSLTOD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSLTOS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSQRTD, ARM_INS_VSQRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSQRTS, ARM_INS_VSQRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv16i8, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv1i64, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv2i32, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv2i64, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv4i16, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv4i32, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv8i16, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAsv8i8, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv16i8, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv1i64, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv2i32, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv2i64, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv4i16, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv4i32, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv8i16, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRAuv8i8, ARM_INS_VSRA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv16i8, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv1i64, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv2i32, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv2i64, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv4i16, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv4i32, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv8i16, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSRIv8i8, ARM_INS_VSRI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1LNd16, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1LNd16_UPD, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1LNd32, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1LNd32_UPD, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1LNd8, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1LNd8_UPD, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16Q, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16Qwb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16Qwb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16T, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16Twb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16Twb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d16wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32Q, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32Qwb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32Qwb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32T, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32Twb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32Twb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d32wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64Q, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64Qwb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64Qwb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64T, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64Twb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64Twb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d64wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8Q, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8Qwb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8Qwb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8T, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8Twb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8Twb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1d8wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q16, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q16wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q16wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q32, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q32wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q32wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q64, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q64wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q64wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q8, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q8wb_fixed, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST1q8wb_register, ARM_INS_VST1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNd16, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNd16_UPD, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNd32, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNd32_UPD, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNd8, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNd8_UPD, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNq16, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNq16_UPD, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNq32, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2LNq32_UPD, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b16, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b16wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b16wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b32, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b32wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b32wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b8, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b8wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2b8wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d16, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d16wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d16wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d32, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d32wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d32wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d8, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d8wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2d8wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q16, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q16wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q16wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q32, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q32wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q32wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q8, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q8wb_fixed, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST2q8wb_register, ARM_INS_VST2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNd16, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNd16_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNd32, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNd32_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNd8, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNd8_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNq16, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNq16_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNq32, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3LNq32_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3d16, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3d16_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3d32, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3d32_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3d8, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3d8_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3q16, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3q16_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3q32, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3q32_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3q8, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST3q8_UPD, ARM_INS_VST3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNd16, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNd16_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNd32, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNd32_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNd8, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNd8_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNq16, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNq16_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNq32, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4LNq32_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4d16, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4d16_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4d32, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4d32_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4d8, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4d8_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4q16, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4q16_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4q32, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4q32_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4q8, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VST4q8_UPD, ARM_INS_VST4,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSTMDDB_UPD, ARM_INS_VSTMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSTMDIA, ARM_INS_VSTMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSTMDIA_UPD, ARM_INS_VSTMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSTMSDB_UPD, ARM_INS_VSTMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSTMSIA, ARM_INS_VSTMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSTMSIA_UPD, ARM_INS_VSTMIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSTRD, ARM_INS_VSTR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSTRS, ARM_INS_VSTR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSUBD, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VSUBHNv2i32, ARM_INS_VSUBHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBHNv4i16, ARM_INS_VSUBHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBHNv8i8, ARM_INS_VSUBHN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBLsv2i64, ARM_INS_VSUBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBLsv4i32, ARM_INS_VSUBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBLsv8i16, ARM_INS_VSUBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBLuv2i64, ARM_INS_VSUBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBLuv4i32, ARM_INS_VSUBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBLuv8i16, ARM_INS_VSUBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBS, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VSUBWsv2i64, ARM_INS_VSUBW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBWsv4i32, ARM_INS_VSUBW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBWsv8i16, ARM_INS_VSUBW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBWuv2i64, ARM_INS_VSUBW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBWuv4i32, ARM_INS_VSUBW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBWuv8i16, ARM_INS_VSUBW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBfd, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBfq, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv16i8, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv1i64, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv2i32, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv2i64, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv4i16, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv4i32, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv8i16, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSUBv8i8, ARM_INS_VSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSWPd, ARM_INS_VSWP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VSWPq, ARM_INS_VSWP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBL1, ARM_INS_VTBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBL2, ARM_INS_VTBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBL3, ARM_INS_VTBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBL4, ARM_INS_VTBL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBX1, ARM_INS_VTBX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBX2, ARM_INS_VTBX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBX3, ARM_INS_VTBX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTBX4, ARM_INS_VTBX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTOSHD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOSHS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTOSIRD, ARM_INS_VCVTR,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOSIRS, ARM_INS_VCVTR,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTOSIZD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOSIZS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTOSLD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOSLS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTOUHD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOUHS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTOUIRD, ARM_INS_VCVTR,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOUIRS, ARM_INS_VCVTR,
#ifndef CAPSTONE_DIET
	{ ARM_REG_FPSCR, 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTOUIZD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOUIZS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTOULD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VTOULS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VTRNd16, ARM_INS_VTRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTRNd32, ARM_INS_VTRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTRNd8, ARM_INS_VTRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTRNq16, ARM_INS_VTRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTRNq32, ARM_INS_VTRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTRNq8, ARM_INS_VTRN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTSTv16i8, ARM_INS_VTST,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTSTv2i32, ARM_INS_VTST,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTSTv4i16, ARM_INS_VTST,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTSTv4i32, ARM_INS_VTST,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTSTv8i16, ARM_INS_VTST,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VTSTv8i8, ARM_INS_VTST,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VUHTOD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VUHTOS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VUITOD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VUITOS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VULTOD, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, ARM_GRP_DPVFP, 0 }, 0, 0
#endif
},
{
	ARM_VULTOS, ARM_INS_VCVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_VFP2, 0 }, 0, 0
#endif
},
{
	ARM_VUZPd16, ARM_INS_VUZP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VUZPd8, ARM_INS_VUZP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VUZPq16, ARM_INS_VUZP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VUZPq32, ARM_INS_VUZP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VUZPq8, ARM_INS_VUZP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VZIPd16, ARM_INS_VZIP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VZIPd8, ARM_INS_VZIP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VZIPq16, ARM_INS_VZIP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VZIPq32, ARM_INS_VZIP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_VZIPq8, ARM_INS_VZIP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_NEON, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMDA, ARM_INS_LDMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMDA_UPD, ARM_INS_LDMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMDB, ARM_INS_LDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMDB_UPD, ARM_INS_LDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMIA, ARM_INS_LDM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMIA_UPD, ARM_INS_LDM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMIB, ARM_INS_LDMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysLDMIB_UPD, ARM_INS_LDMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMDA, ARM_INS_STMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMDA_UPD, ARM_INS_STMDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMDB, ARM_INS_STMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMDB_UPD, ARM_INS_STMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMIA, ARM_INS_STM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMIA_UPD, ARM_INS_STM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMIB, ARM_INS_STMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_sysSTMIB_UPD, ARM_INS_STMIB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_ARM, 0 }, 0, 0
#endif
},
{
	ARM_t2ADCri, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ADCrr, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ADCrs, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ADDri, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ADDri12, ARM_INS_ADDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ADDrr, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ADDrs, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ADR, ARM_INS_ADR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ANDri, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ANDrr, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ANDrs, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ASRri, ARM_INS_ASR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ASRrr, ARM_INS_ASR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2B, ARM_INS_B,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB2, 0 }, 1, 0
#endif
},
{
	ARM_t2BFC, ARM_INS_BFC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2BFI, ARM_INS_BFI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2BICri, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2BICrr, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2BICrs, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2BXJ, ARM_INS_BXJ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, ARM_GRP_PREV8, 0 }, 0, 1
#endif
},
{
	ARM_t2Bcc, ARM_INS_B,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB2, 0 }, 1, 0
#endif
},
{
	ARM_t2CDP, ARM_INS_CDP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_t2CDP2, ARM_INS_CDP2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_t2CLREX, ARM_INS_CLREX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_t2CLZ, ARM_INS_CLZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2CMNri, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2CMNzrr, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2CMNzrs, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2CMPri, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2CMPrr, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2CMPrs, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2CPS1p, ARM_INS_CPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2CPS2p, ARM_INS_CPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2CPS3p, ARM_INS_CPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2CRC32B, ARM_INS_CRC32B,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_t2CRC32CB, ARM_INS_CRC32CB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_t2CRC32CH, ARM_INS_CRC32CH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_t2CRC32CW, ARM_INS_CRC32CW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_t2CRC32H, ARM_INS_CRC32H,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_t2CRC32W, ARM_INS_CRC32W,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, ARM_GRP_CRC, 0 }, 0, 0
#endif
},
{
	ARM_t2DBG, ARM_INS_DBG,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2DCPS1, ARM_INS_DCPS1,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2DCPS2, ARM_INS_DCPS2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2DCPS3, ARM_INS_DCPS3,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2DMB, ARM_INS_DMB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_DATABARRIER, 0 }, 0, 0
#endif
},
{
	ARM_t2DSB, ARM_INS_DSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_DATABARRIER, 0 }, 0, 0
#endif
},
{
	ARM_t2EORri, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2EORrr, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2EORrs, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2HINT, ARM_INS_HINT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2HVC, ARM_INS_HVC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_VIRTUALIZATION, 0 }, 0, 0
#endif
},
{
	ARM_t2ISB, ARM_INS_ISB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_DATABARRIER, 0 }, 0, 0
#endif
},
{
	ARM_t2IT, ARM_INS_IT,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_ITSTATE, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDA, ARM_INS_LDA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2LDAB, ARM_INS_LDAB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2LDAEX, ARM_INS_LDAEX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2LDAEXB, ARM_INS_LDAEXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2LDAEXD, ARM_INS_LDAEXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2LDAEXH, ARM_INS_LDAEXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2LDAH, ARM_INS_LDAH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2L_OFFSET, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2L_OPTION, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2L_POST, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2L_PRE, ARM_INS_LDC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2_OFFSET, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2_OPTION, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2_POST, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC2_PRE, ARM_INS_LDC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDCL_OFFSET, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDCL_OPTION, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDCL_POST, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDCL_PRE, ARM_INS_LDCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC_OFFSET, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC_OPTION, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC_POST, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDC_PRE, ARM_INS_LDC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDMDB, ARM_INS_LDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDMDB_UPD, ARM_INS_LDMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDMIA, ARM_INS_LDM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDMIA_UPD, ARM_INS_LDM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRBT, ARM_INS_LDRBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRB_POST, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRB_PRE, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRBi12, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRBi8, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRBpci, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRBs, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRD_POST, ARM_INS_LDRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRD_PRE, ARM_INS_LDRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRDi8, ARM_INS_LDRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDREX, ARM_INS_LDREX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDREXB, ARM_INS_LDREXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDREXD, ARM_INS_LDREXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2LDREXH, ARM_INS_LDREXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRHT, ARM_INS_LDRHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRH_POST, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRH_PRE, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRHi12, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRHi8, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRHpci, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRHs, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSBT, ARM_INS_LDRSBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSB_POST, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSB_PRE, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSBi12, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSBi8, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSBpci, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSBs, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSHT, ARM_INS_LDRSHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSH_POST, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSH_PRE, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSHi12, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSHi8, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSHpci, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRSHs, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRT, ARM_INS_LDRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDR_POST, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDR_PRE, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRi12, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRi8, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRpci, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LDRs, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LSLri, ARM_INS_LSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LSLrr, ARM_INS_LSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LSRri, ARM_INS_LSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2LSRrr, ARM_INS_LSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MCR, ARM_INS_MCR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MCR2, ARM_INS_MCR2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_t2MCRR, ARM_INS_MCRR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MCRR2, ARM_INS_MCRR2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_t2MLA, ARM_INS_MLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2MLS, ARM_INS_MLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2MOVTi16, ARM_INS_MOVT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MOVi, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MOVi16, ARM_INS_MOVW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MOVr, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MOVsra_flag, ARM_INS_ASR,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MOVsrl_flag, ARM_INS_LSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MRC, ARM_INS_MRC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MRC2, ARM_INS_MRC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_t2MRRC, ARM_INS_MRRC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MRRC2, ARM_INS_MRRC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_PREV8, 0 }, 0, 0
#endif
},
{
	ARM_t2MRS_AR, ARM_INS_MRS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2MRS_M, ARM_INS_MRS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_MCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2MRSbanked, ARM_INS_MRS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_VIRTUALIZATION, 0 }, 0, 0
#endif
},
{
	ARM_t2MRSsys_AR, ARM_INS_MRS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2MSR_AR, ARM_INS_MSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2MSR_M, ARM_INS_MSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_MCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2MSRbanked, ARM_INS_MSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_VIRTUALIZATION, 0 }, 0, 0
#endif
},
{
	ARM_t2MUL, ARM_INS_MUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MVNi, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MVNr, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2MVNs, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ORNri, ARM_INS_ORN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ORNrr, ARM_INS_ORN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ORNrs, ARM_INS_ORN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ORRri, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ORRrr, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2ORRrs, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2PKHBT, ARM_INS_PKHBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2PKHTB, ARM_INS_PKHTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2PLDWi12, ARM_INS_PLDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, ARM_GRP_MULTPRO, 0 }, 0, 0
#endif
},
{
	ARM_t2PLDWi8, ARM_INS_PLDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, ARM_GRP_MULTPRO, 0 }, 0, 0
#endif
},
{
	ARM_t2PLDWs, ARM_INS_PLDW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, ARM_GRP_MULTPRO, 0 }, 0, 0
#endif
},
{
	ARM_t2PLDi12, ARM_INS_PLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2PLDi8, ARM_INS_PLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2PLDpci, ARM_INS_PLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2PLDs, ARM_INS_PLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2PLIi12, ARM_INS_PLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_t2PLIi8, ARM_INS_PLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_t2PLIpci, ARM_INS_PLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_t2PLIs, ARM_INS_PLI,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_V7, 0 }, 0, 0
#endif
},
{
	ARM_t2QADD, ARM_INS_QADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QADD16, ARM_INS_QADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QADD8, ARM_INS_QADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QASX, ARM_INS_QASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QDADD, ARM_INS_QDADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QDSUB, ARM_INS_QDSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QSAX, ARM_INS_QSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QSUB, ARM_INS_QSUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QSUB16, ARM_INS_QSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2QSUB8, ARM_INS_QSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2RBIT, ARM_INS_RBIT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2REV, ARM_INS_REV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2REV16, ARM_INS_REV16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2REVSH, ARM_INS_REVSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2RFEDB, ARM_INS_RFEDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2RFEDBW, ARM_INS_RFEDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2RFEIA, ARM_INS_RFEIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2RFEIAW, ARM_INS_RFEIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2RORri, ARM_INS_ROR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2RORrr, ARM_INS_ROR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2RRX, ARM_INS_RRX,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2RSBri, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2RSBrr, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2RSBrs, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SADD16, ARM_INS_SADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SADD8, ARM_INS_SADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SASX, ARM_INS_SASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SBCri, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SBCrr, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SBCrs, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SBFX, ARM_INS_SBFX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SDIV, ARM_INS_SDIV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_DIVIDE, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SEL, ARM_INS_SEL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SHADD16, ARM_INS_SHADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SHADD8, ARM_INS_SHADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SHASX, ARM_INS_SHASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SHSAX, ARM_INS_SHSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SHSUB16, ARM_INS_SHSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SHSUB8, ARM_INS_SHSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMC, ARM_INS_SMC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PRIVILEGE, ARM_GRP_THUMB2, ARM_GRP_TRUSTZONE, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLABB, ARM_INS_SMLABB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLABT, ARM_INS_SMLABT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLAD, ARM_INS_SMLAD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLADX, ARM_INS_SMLADX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLAL, ARM_INS_SMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLALBB, ARM_INS_SMLALBB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLALBT, ARM_INS_SMLALBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLALD, ARM_INS_SMLALD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLALDX, ARM_INS_SMLALDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLALTB, ARM_INS_SMLALTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLALTT, ARM_INS_SMLALTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLATB, ARM_INS_SMLATB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLATT, ARM_INS_SMLATT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLAWB, ARM_INS_SMLAWB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLAWT, ARM_INS_SMLAWT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLSD, ARM_INS_SMLSD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLSDX, ARM_INS_SMLSDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLSLD, ARM_INS_SMLSLD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMLSLDX, ARM_INS_SMLSLDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMMLA, ARM_INS_SMMLA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMMLAR, ARM_INS_SMMLAR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMMLS, ARM_INS_SMMLS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, ARM_GRP_MULOPS, 0 }, 0, 0
#endif
},
{
	ARM_t2SMMLSR, ARM_INS_SMMLSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMMUL, ARM_INS_SMMUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMMULR, ARM_INS_SMMULR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMUAD, ARM_INS_SMUAD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMUADX, ARM_INS_SMUADX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMULBB, ARM_INS_SMULBB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMULBT, ARM_INS_SMULBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMULL, ARM_INS_SMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SMULTB, ARM_INS_SMULTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMULTT, ARM_INS_SMULTT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMULWB, ARM_INS_SMULWB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMULWT, ARM_INS_SMULWT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMUSD, ARM_INS_SMUSD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SMUSDX, ARM_INS_SMUSDX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SRSDB, ARM_INS_SRSDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2SRSDB_UPD, ARM_INS_SRSDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2SRSIA, ARM_INS_SRSIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2SRSIA_UPD, ARM_INS_SRSIA,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2SSAT, ARM_INS_SSAT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SSAT16, ARM_INS_SSAT16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SSAX, ARM_INS_SSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SSUB16, ARM_INS_SSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2SSUB8, ARM_INS_SSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2L_OFFSET, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2L_OPTION, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2L_POST, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2L_PRE, ARM_INS_STC2L,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2_OFFSET, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2_OPTION, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2_POST, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC2_PRE, ARM_INS_STC2,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_PREV8, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STCL_OFFSET, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STCL_OPTION, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STCL_POST, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STCL_PRE, ARM_INS_STCL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC_OFFSET, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC_OPTION, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC_POST, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STC_PRE, ARM_INS_STC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STL, ARM_INS_STL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2STLB, ARM_INS_STLB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2STLEX, ARM_INS_STLEX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2STLEXB, ARM_INS_STLEXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2STLEXD, ARM_INS_STLEXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2STLEXH, ARM_INS_STLEXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2STLH, ARM_INS_STLH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_t2STMDB, ARM_INS_STMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STMDB_UPD, ARM_INS_STMDB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STMIA, ARM_INS_STM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STMIA_UPD, ARM_INS_STM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRBT, ARM_INS_STRBT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRB_POST, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRB_PRE, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRBi12, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRBi8, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRBs, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRD_POST, ARM_INS_STRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRD_PRE, ARM_INS_STRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRDi8, ARM_INS_STRD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STREX, ARM_INS_STREX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STREXB, ARM_INS_STREXB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STREXD, ARM_INS_STREXD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2STREXH, ARM_INS_STREXH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRHT, ARM_INS_STRHT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRH_POST, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRH_PRE, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRHi12, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRHi8, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRHs, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRT, ARM_INS_STRT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STR_POST, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STR_PRE, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRi12, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRi8, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2STRs, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SUBS_PC_LR, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ ARM_REG_SPSR, ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_REG_CPSR, ARM_REG_PC, 0 }, { ARM_GRP_THUMB2, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_t2SUBri, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SUBri12, ARM_INS_SUBW,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SUBrr, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SUBrs, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SXTAB, ARM_INS_SXTAB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SXTAB16, ARM_INS_SXTAB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SXTAH, ARM_INS_SXTAH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SXTB, ARM_INS_SXTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2SXTB16, ARM_INS_SXTB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_T2EXTRACTPACK, 0 }, 0, 0
#endif
},
{
	ARM_t2SXTH, ARM_INS_SXTH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2TBB, ARM_INS_TBB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 1
#endif
},
{
	ARM_t2TBH, ARM_INS_TBH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 1
#endif
},
{
	ARM_t2TEQri, ARM_INS_TEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2TEQrr, ARM_INS_TEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2TEQrs, ARM_INS_TEQ,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2TSTri, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2TSTrr, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2TSTrs, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UADD16, ARM_INS_UADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UADD8, ARM_INS_UADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UASX, ARM_INS_UASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UBFX, ARM_INS_UBFX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UDF, ARM_INS_UDF,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UDIV, ARM_INS_UDIV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_DIVIDE, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UHADD16, ARM_INS_UHADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UHADD8, ARM_INS_UHADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UHASX, ARM_INS_UHASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UHSAX, ARM_INS_UHSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UHSUB16, ARM_INS_UHSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UHSUB8, ARM_INS_UHSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UMAAL, ARM_INS_UMAAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UMLAL, ARM_INS_UMLAL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UMULL, ARM_INS_UMULL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UQADD16, ARM_INS_UQADD16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UQADD8, ARM_INS_UQADD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UQASX, ARM_INS_UQASX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UQSAX, ARM_INS_UQSAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UQSUB16, ARM_INS_UQSUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UQSUB8, ARM_INS_UQSUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2USAD8, ARM_INS_USAD8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2USADA8, ARM_INS_USADA8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2USAT, ARM_INS_USAT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2USAT16, ARM_INS_USAT16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2USAX, ARM_INS_USAX,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2USUB16, ARM_INS_USUB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2USUB8, ARM_INS_USUB8,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, ARM_GRP_THUMB2DSP, 0 }, 0, 0
#endif
},
{
	ARM_t2UXTAB, ARM_INS_UXTAB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UXTAB16, ARM_INS_UXTAB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UXTAH, ARM_INS_UXTAH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UXTB, ARM_INS_UXTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UXTB16, ARM_INS_UXTB16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_T2EXTRACTPACK, ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_t2UXTH, ARM_INS_UXTH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB2, 0 }, 0, 0
#endif
},
{
	ARM_tADC, ARM_INS_ADC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDhirr, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDi3, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDi8, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDrSP, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDrSPi, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDrr, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDspi, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADDspr, ARM_INS_ADD,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tADR, ARM_INS_ADR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tAND, ARM_INS_AND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tASRri, ARM_INS_ASR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tASRrr, ARM_INS_ASR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tB, ARM_INS_B,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 1, 0
#endif
},
{
	ARM_tBIC, ARM_INS_BIC,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tBKPT, ARM_INS_BKPT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tBL, ARM_INS_BL,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB, ARM_GRP_CALL, 0 }, 1, 0
#endif
},
{
	ARM_tBLXi, ARM_INS_BLX,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB, ARM_GRP_V5T, ARM_GRP_NOTMCLASS, ARM_GRP_CALL, 0 }, 1, 0
#endif
},
{
	ARM_tBLXr, ARM_INS_BLX,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, ARM_REG_PC, 0 }, { ARM_GRP_THUMB, ARM_GRP_V5T, ARM_GRP_CALL, 0 }, 0, 1
#endif
},
{
	ARM_tBX, ARM_INS_BX,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_THUMB, 0 }, 0, 1
#endif
},
{
	ARM_tBcc, ARM_INS_B,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_PC, 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 1, 0
#endif
},
{
	ARM_tCBNZ, ARM_INS_CBNZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB2, 0 }, 1, 0
#endif
},
{
	ARM_tCBZ, ARM_INS_CBZ,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_BRANCH_RELATIVE, ARM_GRP_THUMB2, 0 }, 1, 0
#endif
},
{
	ARM_tCMNz, ARM_INS_CMN,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tCMPhir, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tCMPi8, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tCMPr, ARM_INS_CMP,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tCPS, ARM_INS_CPS,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tEOR, ARM_INS_EOR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tHINT, ARM_INS_HINT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V6M, 0 }, 0, 0
#endif
},
{
	ARM_tHLT, ARM_INS_HLT,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V8, 0 }, 0, 0
#endif
},
{
	ARM_tLDMIA, ARM_INS_LDM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRBi, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRBr, ARM_INS_LDRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRHi, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRHr, ARM_INS_LDRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRSB, ARM_INS_LDRSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRSH, ARM_INS_LDRSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRi, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRpci, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRr, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLDRspi, ARM_INS_LDR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLSLri, ARM_INS_LSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLSLrr, ARM_INS_LSL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLSRri, ARM_INS_LSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tLSRrr, ARM_INS_LSR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tMOVSr, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tMOVi8, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tMOVr, ARM_INS_MOV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tMUL, ARM_INS_MUL,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tMVN, ARM_INS_MVN,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tORR, ARM_INS_ORR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tPOP, ARM_INS_POP,
#ifndef CAPSTONE_DIET
	{ ARM_REG_SP, 0 }, { ARM_REG_SP, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tPUSH, ARM_INS_PUSH,
#ifndef CAPSTONE_DIET
	{ ARM_REG_SP, 0 }, { ARM_REG_SP, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tREV, ARM_INS_REV,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_tREV16, ARM_INS_REV16,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_tREVSH, ARM_INS_REVSH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_tROR, ARM_INS_ROR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tRSB, ARM_INS_RSB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSBC, ARM_INS_SBC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_CPSR, 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSETEND, ARM_INS_SETEND,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_V6, ARM_GRP_NOTMCLASS, 0 }, 0, 0
#endif
},
{
	ARM_tSTMIA_UPD, ARM_INS_STM,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSTRBi, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSTRBr, ARM_INS_STRB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSTRHi, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSTRHr, ARM_INS_STRH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSTRi, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSTRr, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSTRspi, ARM_INS_STR,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSUBi3, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSUBi8, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSUBrr, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSUBspi, ARM_INS_SUB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tSVC, ARM_INS_SVC,
#ifndef CAPSTONE_DIET
	{ ARM_REG_PC, 0 }, { ARM_REG_LR, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_INT, 0 }, 0, 0
#endif
},
{
	ARM_tSXTB, ARM_INS_SXTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_tSXTH, ARM_INS_SXTH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_tTRAP, ARM_INS_TRAP,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, 0 }, 0, 0
#endif
},
{
	ARM_tTST, ARM_INS_TST,
#ifndef CAPSTONE_DIET
	{ 0 }, { ARM_REG_CPSR, 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, 0 }, 0, 0
#endif
},
{
	ARM_tUDF, ARM_INS_UDF,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, 0 }, 0, 0
#endif
},
{
	ARM_tUXTB, ARM_INS_UXTB,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_V6, 0 }, 0, 0
#endif
},
{
	ARM_tUXTH, ARM_INS_UXTH,
#ifndef CAPSTONE_DIET
	{ 0 }, { 0 }, { ARM_GRP_THUMB, ARM_GRP_THUMB1ONLY, ARM_GRP_V6, 0 }, 0, 0
#endif
},
