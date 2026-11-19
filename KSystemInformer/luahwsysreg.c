/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <kph.h>
#include <script.h>

#include <trace.h>

#if defined(_M_ARM64)

#define KPH_LUA_HW_SYSREGS(X)                                                  \
    X(ARM64_MIDR_EL1)                                                          \
    X(ARM64_MPIDR_EL1)                                                         \
    X(ARM64_REVIDR_EL1)                                                        \
    X(ARM64_ID_AA64PFR0_EL1)                                                   \
    X(ARM64_ID_AA64PFR1_EL1)                                                   \
    X(ARM64_ID_AA64DFR0_EL1)                                                   \
    X(ARM64_CNTFRQ_EL0)                                                        \
    X(ARM64_CNTVCT_EL0)                                                        \
    X(ARM64_PMCCNTR_EL0)                                                       \
    X(ARM64_AMCFGR_EL0)                                                        \
    X(ARM64_AMCGCR_EL0)                                                        \
    X(ARM64_AMCNTENSET0_EL0)                                                   \
    X(ARM64_AMEVCNTR0_EL0(0))                                                  \
    X(ARM64_AMEVCNTR0_EL0(1))                                                  \
    X(ARM64_AMEVCNTR0_EL0(2))                                                  \
    X(ARM64_AMEVCNTR0_EL0(3))                                                  \
    X(ARM64_ID_AA64ISAR0_EL1)                                                  \
    X(ARM64_ID_AA64ISAR1_EL1)                                                  \
    X(ARM64_ID_AA64ISAR2_EL1)                                                  \
    X(ARM64_ID_AA64MMFR0_EL1)                                                  \
    X(ARM64_ID_AA64MMFR1_EL1)                                                  \
    X(ARM64_ID_AA64MMFR2_EL1)                                                  \
    X(ARM64_ID_AA64ZFR0_EL1)                                                   \
    X(ARM64_ID_AA64SMFR0_EL1)                                                  \
    X(ARM64_CNTPCT_EL0)                                                        \
    X(ARM64_AMCG1IDR_EL0)                                                      \
    X(ARM64_AMEVCNTR1_EL0(0))                                                  \
    X(ARM64_AMEVCNTR1_EL0(1))                                                  \
    X(ARM64_AMEVCNTR1_EL0(2))                                                  \
    X(ARM64_AMEVCNTR1_EL0(3))                                                  \
    X(ARM64_AMEVCNTR1_EL0(4))                                                  \
    X(ARM64_AMEVCNTR1_EL0(5))                                                  \
    X(ARM64_AMEVCNTR1_EL0(6))                                                  \
    X(ARM64_AMEVCNTR1_EL0(7))                                                  \
    X(ARM64_AMEVCNTR1_EL0(8))                                                  \
    X(ARM64_AMEVCNTR1_EL0(9))                                                  \
    X(ARM64_AMEVCNTR1_EL0(10))                                                 \
    X(ARM64_AMEVCNTR1_EL0(11))                                                 \
    X(ARM64_AMEVCNTR1_EL0(12))                                                 \
    X(ARM64_AMEVCNTR1_EL0(13))                                                 \
    X(ARM64_AMEVCNTR1_EL0(14))                                                 \
    X(ARM64_AMEVCNTR1_EL0(15))                                                 \
    X(ARM64_AMEVTYPER1_EL0(0))                                                 \
    X(ARM64_AMEVTYPER1_EL0(1))                                                 \
    X(ARM64_AMEVTYPER1_EL0(2))                                                 \
    X(ARM64_AMEVTYPER1_EL0(3))                                                 \
    X(ARM64_AMEVTYPER1_EL0(4))                                                 \
    X(ARM64_AMEVTYPER1_EL0(5))                                                 \
    X(ARM64_AMEVTYPER1_EL0(6))                                                 \
    X(ARM64_AMEVTYPER1_EL0(7))                                                 \
    X(ARM64_AMEVTYPER1_EL0(8))                                                 \
    X(ARM64_AMEVTYPER1_EL0(9))                                                 \
    X(ARM64_AMEVTYPER1_EL0(10))                                                \
    X(ARM64_AMEVTYPER1_EL0(11))                                                \
    X(ARM64_AMEVTYPER1_EL0(12))                                                \
    X(ARM64_AMEVTYPER1_EL0(13))                                                \
    X(ARM64_AMEVTYPER1_EL0(14))                                                \
    X(ARM64_AMEVTYPER1_EL0(15))                                                \
    X(ARM64_AIDR_EL1)                                                          \
    X(ARM64_ID_AA64PFR2_EL1)                                                   \
    X(ARM64_ID_AA64DFR1_EL1)                                                   \
    X(ARM64_ID_AA64AFR0_EL1)                                                   \
    X(ARM64_ID_AA64AFR1_EL1)                                                   \
    X(ARM64_ID_AA64ISAR3_EL1)                                                  \
    X(ARM64_ID_AA64MMFR3_EL1)                                                  \
    X(ARM64_ID_AA64MMFR4_EL1)                                                  \
    X(ARM64_ID_AA64FPFR0_EL1)                                                  \
    X(ARM64_CTR_EL0)                                                           \
    X(ARM64_DCZID_EL0)                                                         \
    X(ARM64_CLIDR_EL1)                                                         \
    X(ARM64_GMID_EL1)                                                          \
    X(ARM64_SMIDR_EL1)                                                         \
    X(ARM64_MPAMIDR_EL1)                                                       \
    X(ARM64_ERRIDR_EL1)                                                        \
    X(ARM64_PMMIR_EL1)                                                         \
    X(ARM64_PMSIDR_EL1)                                                        \
    X(ARM64_PMBIDR_EL1)                                                        \
    X(ARM64_TRBIDR_EL1)                                                        \
    X(ARM64_CNTVCTSS_EL0)                                                      \
    X(ARM64_CNTPCTSS_EL0)                                                      \
    X(ARM64_PMCR_EL0)                                                          \
    X(ARM64_PMCEID0_EL0)                                                       \
    X(ARM64_PMCEID1_EL0)                                                       \
    X(ARM64_PMCNTENSET_EL0)                                                    \
    X(ARM64_PMOVSSET_EL0)                                                      \
    X(ARM64_PMCCFILTR_EL0)                                                     \
    X(ARM64_PMICNTR_EL0)                                                       \
    X(ARM64_PMEVCNTR_EL0(0))                                                   \
    X(ARM64_PMEVCNTR_EL0(1))                                                   \
    X(ARM64_PMEVCNTR_EL0(2))                                                   \
    X(ARM64_PMEVCNTR_EL0(3))                                                   \
    X(ARM64_PMEVCNTR_EL0(4))                                                   \
    X(ARM64_PMEVCNTR_EL0(5))                                                   \
    X(ARM64_PMEVCNTR_EL0(6))                                                   \
    X(ARM64_PMEVCNTR_EL0(7))                                                   \
    X(ARM64_PMEVCNTR_EL0(8))                                                   \
    X(ARM64_PMEVCNTR_EL0(9))                                                   \
    X(ARM64_PMEVCNTR_EL0(10))                                                  \
    X(ARM64_PMEVCNTR_EL0(11))                                                  \
    X(ARM64_PMEVCNTR_EL0(12))                                                  \
    X(ARM64_PMEVCNTR_EL0(13))                                                  \
    X(ARM64_PMEVCNTR_EL0(14))                                                  \
    X(ARM64_PMEVCNTR_EL0(15))                                                  \
    X(ARM64_PMEVCNTR_EL0(16))                                                  \
    X(ARM64_PMEVCNTR_EL0(17))                                                  \
    X(ARM64_PMEVCNTR_EL0(18))                                                  \
    X(ARM64_PMEVCNTR_EL0(19))                                                  \
    X(ARM64_PMEVCNTR_EL0(20))                                                  \
    X(ARM64_PMEVCNTR_EL0(21))                                                  \
    X(ARM64_PMEVCNTR_EL0(22))                                                  \
    X(ARM64_PMEVCNTR_EL0(23))                                                  \
    X(ARM64_PMEVCNTR_EL0(24))                                                  \
    X(ARM64_PMEVCNTR_EL0(25))                                                  \
    X(ARM64_PMEVCNTR_EL0(26))                                                  \
    X(ARM64_PMEVCNTR_EL0(27))                                                  \
    X(ARM64_PMEVCNTR_EL0(28))                                                  \
    X(ARM64_PMEVCNTR_EL0(29))                                                  \
    X(ARM64_PMEVCNTR_EL0(30))                                                  \
    X(ARM64_PMEVTYPER_EL0(0))                                                  \
    X(ARM64_PMEVTYPER_EL0(1))                                                  \
    X(ARM64_PMEVTYPER_EL0(2))                                                  \
    X(ARM64_PMEVTYPER_EL0(3))                                                  \
    X(ARM64_PMEVTYPER_EL0(4))                                                  \
    X(ARM64_PMEVTYPER_EL0(5))                                                  \
    X(ARM64_PMEVTYPER_EL0(6))                                                  \
    X(ARM64_PMEVTYPER_EL0(7))                                                  \
    X(ARM64_PMEVTYPER_EL0(8))                                                  \
    X(ARM64_PMEVTYPER_EL0(9))                                                  \
    X(ARM64_PMEVTYPER_EL0(10))                                                 \
    X(ARM64_PMEVTYPER_EL0(11))                                                 \
    X(ARM64_PMEVTYPER_EL0(12))                                                 \
    X(ARM64_PMEVTYPER_EL0(13))                                                 \
    X(ARM64_PMEVTYPER_EL0(14))                                                 \
    X(ARM64_PMEVTYPER_EL0(15))                                                 \
    X(ARM64_PMEVTYPER_EL0(16))                                                 \
    X(ARM64_PMEVTYPER_EL0(17))                                                 \
    X(ARM64_PMEVTYPER_EL0(18))                                                 \
    X(ARM64_PMEVTYPER_EL0(19))                                                 \
    X(ARM64_PMEVTYPER_EL0(20))                                                 \
    X(ARM64_PMEVTYPER_EL0(21))                                                 \
    X(ARM64_PMEVTYPER_EL0(22))                                                 \
    X(ARM64_PMEVTYPER_EL0(23))                                                 \
    X(ARM64_PMEVTYPER_EL0(24))                                                 \
    X(ARM64_PMEVTYPER_EL0(25))                                                 \
    X(ARM64_PMEVTYPER_EL0(26))                                                 \
    X(ARM64_PMEVTYPER_EL0(27))                                                 \
    X(ARM64_PMEVTYPER_EL0(28))                                                 \
    X(ARM64_PMEVTYPER_EL0(29))                                                 \
    X(ARM64_PMEVTYPER_EL0(30))                                                 \
    X(ARM64_AMCR_EL0)                                                          \
    X(ARM64_AMUSERENR_EL0)                                                     \
    X(ARM64_AMCNTENSET1_EL0)                                                   \
    X(ARM64_AMEVCNTR0_EL0(4))                                                  \
    X(ARM64_AMEVCNTR0_EL0(5))                                                  \
    X(ARM64_AMEVCNTR0_EL0(6))                                                  \
    X(ARM64_AMEVCNTR0_EL0(7))                                                  \
    X(ARM64_AMEVCNTR0_EL0(8))                                                  \
    X(ARM64_AMEVCNTR0_EL0(9))                                                  \
    X(ARM64_AMEVCNTR0_EL0(10))                                                 \
    X(ARM64_AMEVCNTR0_EL0(11))                                                 \
    X(ARM64_AMEVCNTR0_EL0(12))                                                 \
    X(ARM64_AMEVCNTR0_EL0(13))                                                 \
    X(ARM64_AMEVCNTR0_EL0(14))                                                 \
    X(ARM64_AMEVCNTR0_EL0(15))                                                 \
    X(ARM64_AMEVTYPER0_EL0(0))                                                 \
    X(ARM64_AMEVTYPER0_EL0(1))                                                 \
    X(ARM64_AMEVTYPER0_EL0(2))                                                 \
    X(ARM64_AMEVTYPER0_EL0(3))                                                 \
    X(ARM64_AMEVTYPER0_EL0(4))                                                 \
    X(ARM64_AMEVTYPER0_EL0(5))                                                 \
    X(ARM64_AMEVTYPER0_EL0(6))                                                 \
    X(ARM64_AMEVTYPER0_EL0(7))                                                 \
    X(ARM64_AMEVTYPER0_EL0(8))                                                 \
    X(ARM64_AMEVTYPER0_EL0(9))                                                 \
    X(ARM64_AMEVTYPER0_EL0(10))                                                \
    X(ARM64_AMEVTYPER0_EL0(11))                                                \
    X(ARM64_AMEVTYPER0_EL0(12))                                                \
    X(ARM64_AMEVTYPER0_EL0(13))                                                \
    X(ARM64_AMEVTYPER0_EL0(14))                                                \
    X(ARM64_AMEVTYPER0_EL0(15))                                                \
    X(ARM64_ZCR_EL1)                                                           \
    X(ARM64_SMCR_EL1)                                                          \
    X(ARM64_MPAM0_EL1)                                                         \
    X(ARM64_MPAM1_EL1)                                                         \
    X(ARM64_CSSELR_EL1)                                                        \
    X(ARM64_CCSIDR_EL1)                                                        \
    X(ARM64_CCSIDR2_EL1)                                                       \
    X(ARM64_PMBLIMITR_EL1)                                                     \
    X(ARM64_PMBMAR_EL1)                                                        \
    X(ARM64_PMBPTR_EL1)                                                        \
    X(ARM64_PMBSR_EL1)                                                         \
    X(ARM64_PMSCR_EL1)                                                         \
    X(ARM64_PMSDSFR_EL1)                                                       \
    X(ARM64_PMSEVFR_EL1)                                                       \
    X(ARM64_PMSFCR_EL1)                                                        \
    X(ARM64_PMSICR_EL1)                                                        \
    X(ARM64_PMSIRR_EL1)                                                        \
    X(ARM64_PMSLATFR_EL1)                                                      \
    X(ARM64_PMSNEVFR_EL1)                                                      \
    X(ARM64_PMSSCR_EL1)                                                        \
    X(ARM64_TRBBASER_EL1)                                                      \
    X(ARM64_TRBLIMITR_EL1)                                                     \
    X(ARM64_TRBMAR_EL1)                                                        \
    X(ARM64_TRBMPAM_EL1)                                                       \
    X(ARM64_TRBPTR_EL1)                                                        \
    X(ARM64_TRBSR_EL1)                                                         \
    X(ARM64_TRBTRG_EL1)                                                        \
    X(ARM64_ERRSELR_EL1)                                                       \
    X(ARM64_ERXADDR_EL1)                                                       \
    X(ARM64_ERXCTLR_EL1)                                                       \
    X(ARM64_ERXFR_EL1)                                                         \
    X(ARM64_ERXGSR_EL1)                                                        \
    X(ARM64_ERXMISC0_EL1)                                                      \
    X(ARM64_ERXMISC1_EL1)                                                      \
    X(ARM64_ERXMISC2_EL1)                                                      \
    X(ARM64_ERXMISC3_EL1)                                                      \
    X(ARM64_ERXPFGCDN_EL1)                                                     \
    X(ARM64_ERXPFGCTL_EL1)                                                     \
    X(ARM64_ERXPFGF_EL1)                                                       \
    X(ARM64_ERXSTATUS_EL1)                                                     \
    X(ARM64_CLUSTERPMCR_EL1)                                                   \
    X(ARM64_CLUSTERPMCNTENSET_EL1)                                             \
    X(ARM64_CLUSTERPMCNTENCLR_EL1)                                             \
    X(ARM64_CLUSTERPMOVSSET_EL1)                                               \
    X(ARM64_CLUSTERPMOVSCLR_EL1)                                               \
    X(ARM64_CLUSTERPMSELR_EL1)                                                 \
    X(ARM64_CLUSTERPMINTENSET_EL1)                                             \
    X(ARM64_CLUSTERPMINTENCLR_EL1)                                             \
    X(ARM64_CLUSTERPMCCNTR_EL1)                                                \
    X(ARM64_CLUSTERPMXEVTYPER_EL1)                                             \
    X(ARM64_CLUSTERPMXEVCNTR_EL1)                                              \
    X(ARM64_CLUSTERPMMDCR_EL1)                                                 \
    X(ARM64_CLUSTERPMCEID0_EL1)                                                \
    X(ARM64_CLUSTERPMCEID1_EL1)                                                \
    X(ARM64_L2CPUSRSELR_EL1)                                                   \
    X(ARM64_L2CPUSRDR_EL1)                                                     \
    X(ARM64_AMCNTENCLR0_EL0)                                                   \
    X(ARM64_PMCNTENCLR_EL0)                                                    \
    X(ARM64_PMOVSCLR_EL0)                                                      \
    X(ARM64_AMCNTENCLR1_EL0)                                                   \
    X(ARM64_PMSELR_EL0)                                                        \
    X(ARM64_PMXEVCNTR_EL0)                                                     \
    X(ARM64_PMXEVTYPER_EL0)                                                    \
    X(ARM64_PMSWINC_EL0)                                                       \
    X(ARM64_PMUSERENR_EL0)                                                     \
    X(ARM64_PMINTENSET_EL1)                                                    \
    X(ARM64_PMINTENCLR_EL1)

#define KPH_LUA_HW_SYSREG_READ(Register)                                       \
    case Register:                                                             \
    {                                                                          \
        value = (ULONG64)_ReadStatusReg(Register);                             \
        break;                                                                 \
    }

#define KPH_LUA_HW_SYSREG_LISTED(Register)                                     \
    case Register:                                                             \
    {                                                                          \
        return TRUE;                                                           \
    }

#define KPH_LUA_HW_SYSREG_WRITE(Register)                                      \
    case Register:                                                             \
    {                                                                          \
        _WriteStatusReg(Register, (__int64)Value);                             \
        break;                                                                 \
    }

/**
 * \brief Checks whether a register code is in the list.
 *
 * \param[in] Register The register code.
 *
 * \return TRUE when the code is in the list, FALSE otherwise.
 */
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwIsSysregListed(
    _In_ ULONG Register
    )
{
    KPH_NPAGED_CODE_HIGH_MAX();

    switch (Register)
    {
        KPH_LUA_HW_SYSREGS(KPH_LUA_HW_SYSREG_LISTED)
        default:
        {
            return FALSE;
        }
    }
}

/**
 * \brief Reads a system register of the current processor.
 *
 * \param[in] Register The register code.
 * \param[out] Value Receives the value, zero when the read faults.
 * \param[out] Listed Receives FALSE when the code is not in the list.
 *
 * \return TRUE when the read did not fault, FALSE otherwise.
 */
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwReadSysreg(
    _In_ ULONG Register,
    _Out_ PULONG64 Value,
    _Out_ PBOOLEAN Listed
    )
{
    ULONG64 value;
    BOOLEAN listed;

    KPH_NPAGED_CODE_HIGH_MAX();

    *Value = 0;
    *Listed = FALSE;

    value = 0;
    listed = TRUE;

    __try
    {
        __isb(_ARM64_BARRIER_SY);

        switch (Register)
        {
            KPH_LUA_HW_SYSREGS(KPH_LUA_HW_SYSREG_READ)
            default:
            {
                listed = FALSE;
                break;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *Listed = TRUE;
        return FALSE;
    }

    *Value = value;
    *Listed = listed;

    return TRUE;
}

/**
 * \brief Writes a system register of the current processor.
 *
 * \param[in] Register The register code.
 * \param[in] Value The value.
 * \param[out] Listed Receives FALSE when the code is not in the list.
 *
 * \return TRUE when the write did not fault, FALSE otherwise.
 */
_IRQL_requires_max_(HIGH_LEVEL)
BOOLEAN KphpLuaHwWriteSysreg(
    _In_ ULONG Register,
    _In_ ULONG64 Value,
    _Out_ PBOOLEAN Listed
    )
{
    BOOLEAN listed;

    KPH_NPAGED_CODE_HIGH_MAX();

    *Listed = FALSE;

    listed = TRUE;

    __try
    {
        switch (Register)
        {
            KPH_LUA_HW_SYSREGS(KPH_LUA_HW_SYSREG_WRITE)
            default:
            {
                listed = FALSE;
                break;
            }
        }

        __isb(_ARM64_BARRIER_SY);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        *Listed = TRUE;
        return FALSE;
    }

    *Listed = listed;

    return TRUE;
}

#endif

KPH_PAGED_FILE();

/**
 * \brief Initializes the system register group of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareSysreg(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();
}

/**
 * \brief Cleans up the system register group of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareSysreg(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();
}

/**
 * \brief Opens the system register group; it adds no function, the registers
 * are batch items.
 *
 * \param[in] State The Lua state, with the library table on top.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareSysreg(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(State);
}
