/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s
 *
 */

using System.Xml.Linq;

namespace CustomBuildTool
{
    /// <summary>
    /// Hardware section constants and layouts; the layouts mirror the pack(1) KPH_DYN_HW_* types in kphdyn.h.
    /// </summary>
    public static class DynDataHw
    {
        /// <summary>
        /// Bytes "KPHW".
        /// </summary>
        public const UInt32 SectionMagic = 0x5748504B;
        public const UInt32 MaxSectionLength = 0x100000;
        public const UInt32 MaxRows = 16384;
        public const UInt32 MaxOps = 1024;
        /// <summary>
        /// The maximum ranges, and values, per op.
        /// </summary>
        public const UInt32 MaxRanges = 128;
        /// <summary>
        /// Generator budget for the ops walked at activation (each row's table ops, summed); warned about at half.
        /// </summary>
        public const UInt32 RowOpsBudget = 0x40000;
        public const UInt32 PciConfigLength = 0x1000;
        /// <summary>
        /// Indirect index/data registers lie above the standard header.
        /// </summary>
        public const UInt32 PciConfigHeaderLength = 0x40;
        /// <summary>
        /// Bit 7 (multi-function) is not part of an identity.
        /// </summary>
        public const byte PciHeaderLayoutMask = 0x7F;

        public const UInt16 KindMsrRead = 1;
        public const UInt16 KindPciRead = 2;
        public const UInt16 KindPciIndirectRead = 3;
        public const UInt16 KindWindowRead = 4;
        public const UInt16 KindSuperIoRead = 5;
        public const UInt16 KindSmbusRead = 6;
        public const UInt16 KindVsecRead = 7;
        /// <summary>
        /// Read-type (GET) commands only.
        /// </summary>
        public const UInt16 KindMailboxTransact = 8;
        /// <summary>
        /// SMU-owned DRAM table copy-out through GET commands of a mailbox op.
        /// </summary>
        public const UInt16 KindSmuTableRead = 9;
        /// <summary>
        /// Read-only methods with integer arguments only.
        /// </summary>
        public const UInt16 KindAcpiEval = 10;
        /// <summary>
        /// Gates the acpi_eval ops after it, up to the next acpi_require: one of the IDs names a present ACPI device.
        /// </summary>
        public const UInt16 KindAcpiRequire = 15;

        public const byte AcpiMaxArgs = 7;
        /// <summary>
        /// The minimum is sizeof(ACPI_EVAL_OUTPUT_BUFFER); the ACPI driver refuses a smaller buffer.
        /// </summary>
        public const UInt16 AcpiMinOutput = 20;
        public const UInt16 AcpiMaxOutput = 4096;
        /// <summary>
        /// Bit n = ACPI_METHOD_ARGUMENT type n; PACKAGE_EX (4) is never accepted.
        /// </summary>
        public const byte AcpiResultInteger = 0x01;
        public const byte AcpiResultString = 0x02;
        public const byte AcpiResultBuffer = 0x04;
        public const byte AcpiResultPackage = 0x08;
        public const byte AcpiResultMask = AcpiResultInteger | AcpiResultString | AcpiResultBuffer | AcpiResultPackage;
        public const int AcpiIdSize = 16;
        /// <summary>
        /// Read-only spec (underscore) methods an acpi_eval op may evaluate (ACPI 6.5); the generator is the only gate.
        /// </summary>
        public static readonly string[] AcpiReadMethods =
        [
            "_STA", "_UID", "_HID", "_CID", "_ADR", "_SUN", "_STR",
            "_TMP", "_CRT", "_HOT", "_PSV",
            "_AC0", "_AC1", "_AC2", "_AC3", "_AC4", "_AC5", "_AC6", "_AC7", "_AC8", "_AC9",
            "_TC1", "_TC2", "_TSP", "_TZP",
            "_FST", "_FIF", "_FPS",
            "_BST", "_BIF", "_BIX",
            "_PMM", "_PMC", "_GAI"
        ];

        /// <summary>
        /// Intel DPTF generic participant IDs, Broadwell through Nova Lake.
        /// </summary>
        public static readonly string[] DptfParticipantIds =
        [
            "INT3403", "INTC1043", "INTC1046", "INTC1062", "INTC1069", "INTC10A1", "INTC10D5", "INTC10FD", "INTC10F4"
        ];

        /// <summary>
        /// Intel DPTF platform power participant IDs.
        /// </summary>
        public static readonly string[] DptfPowerIds =
        [
            "INT3407", "INT3532", "INTC1047", "INTC1050", "INTC1060", "INTC1061", "INTC1065", "INTC1066", "INTC106C",
            "INTC106D", "INTC10A4", "INTC10A5", "INTC10D8", "INTC10D9", "INTC1100", "INTC1101", "INTC10F7", "INTC10F8"
        ];

        /// <summary>
        /// Vendor (non-underscore) methods an acpi_eval op may evaluate, by ACPI ID; each is reviewed as a read before it is listed.
        /// </summary>
        public static readonly (string Id, string Method)[] AcpiVendorMethods =
        [
            // Intel DPTF generic participant: integer getters.
            .. DptfParticipantIds.SelectMany(id => new[] { (id, "PTYP"), (id, "GTSH") }),
            // Intel DPTF platform power participant: integer getters; PBOK is a write and stays out.
            .. DptfPowerIds.SelectMany(id => new[] { "PTYP", "PSRC", "PMAX", "ARTG", "PBSS", "PROP", "CTYP", "RBHF", "VBNL", "CMPP" }.Select(m => (id, m))),
            // ASUS ATK0110 legacy interface; reader arguments in AcpiMethodArgs.
            .. new[] { "TSIF", "VSIF", "FSIF", "RTMP", "RVLT", "RFAN" }.Select(m => ("ATK0110", m)),
            // Lenovo ThinkPad HKEY DYTC; read commands in AcpiMethodArgs.
            ("LEN0068", "DYTC"),
            ("LEN0268", "DYTC"),
            // ThinkPad EC thermal sensors, degrees C; gated in AcpiMethodRequires.
            .. Enumerable.Range(0, 8).Select(i => ("PNP0C09", $"TMP{i}"))
        ];

        /// <summary>
        /// ThinkPad hotkey device IDs.
        /// </summary>
        public static readonly string[] ThinkPadHotkeyIds = ["IBM0068", "LEN0068", "LEN0268"];

        /// <summary>
        /// Vendor methods allowed only inside a require group, by ACPI ID: every ID the group requires is one of these.
        /// </summary>
        public static readonly Dictionary<(string Id, string Method), string[]> AcpiMethodRequires =
            Enumerable.Range(0, 8).ToDictionary(i => ("PNP0C09", $"TMP{i}"), i => ThinkPadHotkeyIds);

        /// <summary>
        /// Returns why the require group of a method breaks AcpiMethodRequires, or null when it is allowed.
        /// </summary>
        public static string AcpiMethodRequireError(string Id, string Method, IReadOnlyCollection<string> Required)
        {
            if (!AcpiMethodRequires.TryGetValue((Id, Method), out var allowed))
                return null;
            if (Required == null)
                return $"method \"{Method}\" of {Id} needs a <require> of {string.Join(",", allowed)} (DynDataHw.AcpiMethodRequires)";

            string other = Required.FirstOrDefault(r => !allowed.Contains(r));

            if (other != null)
                return $"method \"{Method}\" of {Id} requires {other}, not one of {string.Join(",", allowed)} (DynDataHw.AcpiMethodRequires)";

            return null;
        }

        /// <summary>
        /// The arguments a method of a device may take, by position: the fixed values allowed and the widest mask. A
        /// method not listed, spec or vendor, takes no arguments.
        /// </summary>
        public static readonly Dictionary<(string Id, string Method), (UInt32[] Fixed, UInt32 Mask)[]> AcpiMethodArgs = new()
        {
            // ATK0110 readers: class 06h hwmon and the reader's type are fixed, the 16-bit element id varies.
            [("ATK0110", "RTMP")] = [([0x06030000], 0xFFFF)],
            [("ATK0110", "RVLT")] = [([0x06020000], 0xFFFF)],
            [("ATK0110", "RFAN")] = [([0x06040000], 0xFFFF)],
            // DYTC QUERY 0, GET 2, FUNC_CAP 3, MMC_GET 8; SET 1 and RESET 1FFh change the platform profile.
            [("LEN0068", "DYTC")] = [([0x0, 0x2, 0x3, 0x8], 0)],
            [("LEN0268", "DYTC")] = [([0x0, 0x2, 0x3, 0x8], 0)],
        };

        /// <summary>
        /// Returns why the arguments of a method of a device break AcpiMethodArgs, or null when they are allowed.
        /// </summary>
        public static string AcpiMethodArgsError(string Id, string Method, IReadOnlyList<HwAcpiArg> Args)
        {
            var allowed = AcpiMethodArgs.TryGetValue((Id, Method), out var list) ? list : [];

            if (Args.Count > allowed.Length)
                return $"method \"{Method}\" of {Id} takes at most {allowed.Length} arguments (DynDataHw.AcpiMethodArgs), not {Args.Count}";

            for (int i = 0; i < Args.Count; i++)
            {
                if ((Args[i].Mask & ~allowed[i].Mask) != 0)
                    return $"method \"{Method}\" of {Id} arg {i} mask 0x{Args[i].Mask:x} is wider than 0x{allowed[i].Mask:x} (DynDataHw.AcpiMethodArgs)";
                if (!allowed[i].Fixed.Contains(Args[i].Fixed))
                    return $"method \"{Method}\" of {Id} arg {i} fixed 0x{Args[i].Fixed:x} is not an allowed value (DynDataHw.AcpiMethodArgs)";
            }

            return null;
        }

        /// <summary>
        /// Returns whether a name is an ACPI NameSeg: [A-Z_][A-Z0-9_]{3}.
        /// </summary>
        public static bool IsAcpiNameSeg(string Name)
        {
            if (Name.Length != 4)
                return false;
            if (!(Name[0] == '_' || (Name[0] >= 'A' && Name[0] <= 'Z')))
                return false;
            return Name.Skip(1).All(c => c == '_' || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
        }

        /// <summary>
        /// Returns whether an ACPI method name passes the driver's check: a vendor NameSeg, or a read-only spec name.
        /// </summary>
        public static bool AcpiMethodAllowed(string Name)
        {
            return IsAcpiNameSeg(Name) && (Name[0] != '_' || AcpiReadMethods.Contains(Name));
        }

        /// <summary>
        /// Returns whether an ACPI method may be evaluated on a device: a read-only spec name, or a listed vendor method of that ID.
        /// </summary>
        public static bool AcpiMethodAllowed(string Id, string Name)
        {
            return AcpiMethodAllowed(Name) && (Name[0] == '_' || AcpiVendorMethods.Contains((Id, Name)));
        }

        /// <summary>
        /// Returns whether an ACPI_HID identity is well formed: 7 or 8 characters of [A-Z0-9_].
        /// </summary>
        public static bool IsAcpiId(string Id)
        {
            return (Id.Length == 7 || Id.Length == 8) && Id.All(c => c == '_' || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
        }

        /// <summary>
        /// Reads from a fixed register enumeration compiled into the driver.
        /// </summary>
        public const UInt16 KindSysregRead = 11;

        /// <summary>
        /// The row names an index/data port pair of <see cref="IoPortPairs"/>.
        /// </summary>
        public const UInt16 KindPortIndirectRead = 12;
        /// <summary>
        /// The index/data I/O port pairs a portindirect or smbus-host may name: AMD FCH PM, index CD6h and data CD7h.
        /// </summary>
        public static readonly (UInt16 Index, UInt16 Data)[] IoPortPairs = [(0xCD6, 0xCD7)];
        /// <summary>
        /// Exclusive; an undecoded pair reads FFh.
        /// </summary>
        public const UInt32 PortIndexEnd = 0xFF;

        /// <summary>
        /// How a PIIX4 host reaches its PM registers in one revision range of its PCI function; the smbus_read ops of the
        /// function use the one whose PCI_CONFIG gate passes, and a revision no smbus_host covers is refused.
        /// </summary>
        public const UInt16 KindSmbusHost = 13;
        /// <summary>
        /// The layout of an AMD Data Fabric DRAM address map, read through the ECAM page of the fabric function; the
        /// smu_table_read op that names it bounds the table base to the DRAM it describes.
        /// </summary>
        public const UInt16 KindDfMap = 14;

        /// <summary>
        /// The registers the driver reads, a compiled case each; <see cref="SysregIdRegisters"/> are the ones a
        /// SYSREG_FIELD gate may read.
        /// </summary>
        public const byte SysregMidr = 1;
        public const byte SysregMpidr = 2;
        public const byte SysregRevidr = 3;
        public const byte SysregIdAa64Pfr0 = 4;
        public const byte SysregIdAa64Pfr1 = 5;
        public const byte SysregIdAa64Dfr0 = 6;
        public const byte SysregCntfrq = 7;
        public const byte SysregCntvct = 8;
        public const byte SysregPmccntr = 9;
        public const byte SysregAmcfgr = 10;
        public const byte SysregAmcgcr = 11;
        public const byte SysregAmcntenset0 = 12;
        public const byte SysregAmevcntr00 = 13;
        public const byte SysregAmevcntr01 = 14;
        public const byte SysregAmevcntr02 = 15;
        public const byte SysregAmevcntr03 = 16;
        public const byte SysregIdAa64Isar0 = 17;
        public const byte SysregIdAa64Isar1 = 18;
        public const byte SysregIdAa64Isar2 = 19;
        public const byte SysregIdAa64Mmfr0 = 20;
        public const byte SysregIdAa64Mmfr1 = 21;
        public const byte SysregIdAa64Mmfr2 = 22;
        public const byte SysregIdAa64Zfr0 = 23;
        public const byte SysregIdAa64Smfr0 = 24;
        public const byte SysregCntpct = 25;
        public const byte SysregAmcg1idr = 26;
        /// <summary>
        /// AMEVCNTR1&lt;n&gt;_EL0, n = 0 to 15, at this value + n.
        /// </summary>
        public const byte SysregAmevcntr1First = 27;
        /// <summary>
        /// AMEVTYPER1&lt;n&gt;_EL0, n = 0 to 15, at this value + n.
        /// </summary>
        public const byte SysregAmevtyper1First = 43;
        public const byte SysregAmuGroup1Counters = 16;
        public const byte SysregMax = SysregAmevtyper1First + SysregAmuGroup1Counters - 1;

        public static readonly byte[] SysregIdRegisters =
        [
            SysregMidr, SysregMpidr, SysregRevidr, SysregIdAa64Pfr0, SysregIdAa64Pfr1, SysregIdAa64Dfr0,
            SysregIdAa64Isar0, SysregIdAa64Isar1, SysregIdAa64Isar2, SysregIdAa64Mmfr0, SysregIdAa64Mmfr1, SysregIdAa64Mmfr2,
            SysregIdAa64Zfr0, SysregIdAa64Smfr0
        ];

        public static readonly FrozenDictionary<string, byte> SysregNames = new Dictionary<string, byte>
        {
            ["MIDR_EL1"] = SysregMidr,
            ["MPIDR_EL1"] = SysregMpidr,
            ["REVIDR_EL1"] = SysregRevidr,
            ["ID_AA64PFR0_EL1"] = SysregIdAa64Pfr0,
            ["ID_AA64PFR1_EL1"] = SysregIdAa64Pfr1,
            ["ID_AA64DFR0_EL1"] = SysregIdAa64Dfr0,
            ["CNTFRQ_EL0"] = SysregCntfrq,
            ["CNTVCT_EL0"] = SysregCntvct,
            ["PMCCNTR_EL0"] = SysregPmccntr,
            ["AMCFGR_EL0"] = SysregAmcfgr,
            ["AMCGCR_EL0"] = SysregAmcgcr,
            ["AMCNTENSET0_EL0"] = SysregAmcntenset0,
            ["AMEVCNTR00_EL0"] = SysregAmevcntr00,
            ["AMEVCNTR01_EL0"] = SysregAmevcntr01,
            ["AMEVCNTR02_EL0"] = SysregAmevcntr02,
            ["AMEVCNTR03_EL0"] = SysregAmevcntr03,
            ["ID_AA64ISAR0_EL1"] = SysregIdAa64Isar0,
            ["ID_AA64ISAR1_EL1"] = SysregIdAa64Isar1,
            ["ID_AA64ISAR2_EL1"] = SysregIdAa64Isar2,
            ["ID_AA64MMFR0_EL1"] = SysregIdAa64Mmfr0,
            ["ID_AA64MMFR1_EL1"] = SysregIdAa64Mmfr1,
            ["ID_AA64MMFR2_EL1"] = SysregIdAa64Mmfr2,
            ["ID_AA64ZFR0_EL1"] = SysregIdAa64Zfr0,
            ["ID_AA64SMFR0_EL1"] = SysregIdAa64Smfr0,
            ["CNTPCT_EL0"] = SysregCntpct,
            ["AMCG1IDR_EL0"] = SysregAmcg1idr,
        }.Concat(Enumerable.Range(0, SysregAmuGroup1Counters).SelectMany(n => new[]
        {
            KeyValuePair.Create($"AMEVCNTR1{n}_EL0", (byte)(SysregAmevcntr1First + n)),
            KeyValuePair.Create($"AMEVTYPER1{n}_EL0", (byte)(SysregAmevtyper1First + n)),
        })).ToFrozenDictionary();

        public static bool SysregIsIdClass(byte Register) => SysregIdRegisters.Contains(Register);

        /// <summary>
        /// The #define lines of the AMU group 1 registers for kphdyn.h.
        /// </summary>
        public static string SysregGroup1Defines(string NewLine) => string.Join(NewLine,
            Enumerable.Range(0, SysregAmuGroup1Counters).Select(n => $"#define {$"KPH_DYN_HW_SYSREG_AMEVCNTR1{n}_EL0",-40}((UCHAR){SysregAmevcntr1First + n})").Concat(
            Enumerable.Range(0, SysregAmuGroup1Counters).Select(n => $"#define {$"KPH_DYN_HW_SYSREG_AMEVTYPER1{n}_EL0",-40}((UCHAR){SysregAmevtyper1First + n})")));

        /// <summary>
        /// MIDR_EL1 variant [23:20], part number [15:4], revision [3:0].
        /// </summary>
        public const byte ArmMaxVariant = 0xF;
        public const UInt16 ArmMaxPartNum = 0xFFF;
        public const byte ArmMaxRevision = 0xF;
        /// <summary>
        /// The most ARM_CPU rows one arm element may expand to.
        /// </summary>
        public const int ArmMaxBucketIdentities = 4096;

        /// <summary>
        /// Intel DVSEC/VSEC ID 2.
        /// </summary>
        public const byte VsecDiscoveryPmtTelemetry = 2;
        /// <summary>
        /// Intel DVSEC/VSEC ID 66.
        /// </summary>
        public const byte VsecDiscoveryTpmi = 66;
        /// <summary>
        /// Discovery table offset >> 3 (TGL/ADL/RPL).
        /// </summary>
        public const byte VsecQuirkTableShift = 0x01;
        /// <summary>
        /// PMT local region at the discovery BAR plus its offset; fixed PUNIT block hidden (TGL/ADL/RPL).
        /// </summary>
        public const byte VsecQuirkEarlyHw = 0x02;
        public const byte VsecQuirkMask = VsecQuirkTableShift | VsecQuirkEarlyHw;
        /// <summary>
        /// TPMI_CONTROL, a mailbox; never a vsec_read feature.
        /// </summary>
        public const byte VsecTpmiControl = 0x80;
        public const UInt32 VsecGuidAny = 0;
        public const byte VsecRevisionDefault = 1;
        public const byte VsecRevisionMax = 0xF;
        public const UInt16 VsecDvsecVendorDefault = 0x8086;
        public const byte VsecMaxRegions = 64;
        public const UInt32 VsecMaxRegionSize = 0x40000;
        /// <summary>
        /// Regions the vsec_read ops of one table expose together.
        /// </summary>
        public const UInt32 VsecMaxTableRegions = 512;

        /// <summary>
        /// MSG/RSP/ARG are SMN addresses reached through a root complex index/data pair.
        /// </summary>
        public const byte MailboxTransportSmnPair = 1;
        /// <summary>
        /// MSG/RSP/ARG are offsets in an ACPI _CRS memory descriptor (HSMP on AMDI0097); ACPI_HID rows only.
        /// </summary>
        public const byte MailboxTransportMmioWindow = 2;
        /// <summary>
        /// A non-zero RSP is idle; a zero RSP is another agent's command in flight and is never written over.
        /// </summary>
        public const byte MailboxProtocolRsmu = 1;
        /// <summary>
        /// AMD HSMP: software writes zero to RSP before every command, so a zero RSP that stays zero is stale.
        /// </summary>
        public const byte MailboxProtocolHsmp = 2;
        /// <summary>
        /// MSG, RSP and the argument registers lie in one page of the window.
        /// </summary>
        public const UInt32 MailboxWindowPage = 0x1000;
        /// <summary>
        /// HSMP 8, RSMU/MP1 6.
        /// </summary>
        public const byte MailboxMaxArgs = 8;
        public const byte MailboxMaxCommands = 64;
        /// <summary>
        /// The MSG register takes a message ID byte.
        /// </summary>
        public const UInt32 MailboxMaxOpcode = 0xFF;
        /// <summary>
        /// SET and SET_GET commands are never accepted.
        /// </summary>
        public const byte MailboxCommandGet = 1;
        /// <summary>
        /// Issued by smu_table_read only, never on behalf of a client.
        /// </summary>
        public const byte MailboxCommandInternal = 0x01;
        /// <summary>
        /// The table base must lie in DRAM the CPU decodes (AMD).
        /// </summary>
        public const byte SmuTableBoundAmdDram = 1;
        /// <summary>
        /// No reply word: the base has no high half, or no size is reported.
        /// </summary>
        public const byte SmuTableWordNone = 0xFF;
        /// <summary>
        /// The response words the base opcode must read back: one more than the highest word index the row names.
        /// </summary>
        public static byte SmuTableBaseWords(byte BaseLowWord, byte BaseHighWord, byte SizeWord)
        {
            byte highest = BaseLowWord;

            if (BaseHighWord != SmuTableWordNone)
                highest = Math.Max(highest, BaseHighWord);
            if (SizeWord != SmuTableWordNone)
                highest = Math.Max(highest, SizeWord);

            return (byte)(highest + 1);
        }
        /// <summary>
        /// The table names no df_map op.
        /// </summary>
        public const byte SmuTableDfMapNone = 0xFF;
        /// <summary>
        /// The Data Fabric function whose data register performs an indirect fabric access; never read.
        /// </summary>
        public const byte DfMapIndirectFunction = 4;
        /// <summary>
        /// The layout rules of a df_map body the loader and the section check share; null when it holds.
        /// </summary>
        public static unsafe string DfMapError(HwDfMap Map)
        {
            UInt32 mapsEnd = (UInt32)Map.MapOffset + (UInt32)(Map.MapCount - 1) * Map.MapStride + Math.Max(Map.LimitOffset, Map.ControlOffset) + 4u;

            if (Map.Device > 31)
                return $"device {Map.Device} is not a PCI device number (0-31)";
            if (Map.Function > 7 || Map.Function == DfMapIndirectFunction)
                return $"function {Map.Function} is not a Data Fabric function KSI reads (0-7, never {DfMapIndirectFunction})";
            if (Map.MapCount == 0 || Map.MapStride < 4 || (Map.MapStride % 4) != 0 || (Map.MapOffset % 4) != 0)
                return "maps need a count, and a stride and offset that are dword multiples";
            if ((Map.LimitOffset % 4) != 0 || (Map.ControlOffset % 4) != 0 || Map.LimitOffset == 0 || Map.ControlOffset == 0 || Map.LimitOffset == Map.ControlOffset ||
                Map.LimitOffset >= Map.MapStride || Map.ControlOffset >= Map.MapStride)
                return "limit and control are distinct dword registers of a map, after the base";
            if (mapsEnd > PciConfigLength)
                return $"the maps end at 0x{mapsEnd:x}, past the configuration space";
            if (Map.AddressMask == 0 || Map.AddressShift < 12 || Map.AddressShift > 40 || (Map.AddressMask & (Map.AddressMask + 1)) != 0)
                return "address field must be one low run of bits shifted by 12 to 40";
            if (Map.ValidBit > 31 || Map.HoleRemapBit > 31 || Map.ValidBit == Map.HoleRemapBit)
                return "valid and hole-remap bits must be distinct bits of the control register";
            if (Map.HoleOffset == 0 ? (Map.HoleBaseMask != 0 || Map.HoleValidBit != 0) :
                ((Map.HoleOffset % 4) != 0 || Map.HoleOffset > PciConfigLength - 4 || Map.HoleBaseMask == 0 || Map.HoleValidBit > 31 ||
                 (Map.HoleBaseMask & (1u << Map.HoleValidBit)) != 0))
                return "a hole register needs a dword offset, a base mask and a valid bit outside it; no hole register names none";
            if (Map.Reserved[0] != 0 || Map.Reserved[1] != 0)
                return "reserved bytes are non-zero";

            return null;
        }
        public const byte SmuTableMaxVersions = 64;
        public const UInt32 SmuTableMaxSize = 0x10000;

        /// <summary>
        /// A mailbox register set: the SMN index/data pair (zero for MMIO), MSG, RSP, the first argument register and the argument count.
        /// </summary>
        public readonly record struct MailboxRegisters(UInt16 IndexOffset, UInt16 DataOffset, UInt32 Msg, UInt32 Rsp, UInt32 Arg0, byte ArgCount);

        /// <summary>
        /// A mailbox protocol: its handshake kind, its register sets, its read-only (GET) opcodes and, where the opcode map differs by part,
        /// the (family, first model, last model) blocks that use it (null: any CPU whose root complex may hold a mailbox).
        /// </summary>
        public sealed record MailboxProtocol(string Name, byte Kind, MailboxRegisters[] Smn, MailboxRegisters[] Mmio, byte[] GetOpcodes, (UInt16 Family, byte ModelMin, byte ModelMax)[] Cpus);

        /// <summary>
        /// The protocols a mailbox element names; an opcode or register set enters here as reviewed code, never as XML.
        /// </summary>
        public static readonly MailboxProtocol[] MailboxProtocols =
        [
            // AMD HSMP specification (EPYC PPR, Host System Management Port) GET messages; 02h GET_SMU_VER is the version query.
            new("hsmp",
                Kind: MailboxProtocolHsmp,
                Smn: [new(0xC4, 0xC8, 0x3B10534, 0x3B10980, 0x3B109E0, 8), new(0xC4, 0xC8, 0x3B10934, 0x3B10980, 0x3B109E0, 8)],
                Mmio: [],
                GetOpcodes: [0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x0A, 0x0B, 0x0F, 0x10, 0x11, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                             0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x23, 0x30, 0x31, 0x32, 0x34, 0x35, 0x36, 0x37],
                Cpus: [(0x19, 0x00, 0x0F), (0x19, 0x10, 0x1F), (0x19, 0x30, 0x3F), (0x19, 0x90, 0x9F), (0x19, 0xA0, 0xAF), (0x1A, 0x00, 0x1F)]),
            // RSMU, APU map: 02h GetSmuVersion, 06h GetTableVersion, 65h TransferTableToDram, 66h GetDramBaseAddress.
            new("rsmu-renoir",
                Kind: MailboxProtocolRsmu,
                Smn: [new(0x60, 0x64, 0x3B10A20, 0x3B10A80, 0x3B10A88, 6)],
                Mmio: [],
                GetOpcodes: [0x02, 0x06, 0x65, 0x66],
                Cpus: [(0x17, 0x60, 0x6F), (0x19, 0x40, 0x4F), (0x19, 0x50, 0x5F), (0x19, 0x70, 0x7F), (0x1A, 0x20, 0x2F), (0x1A, 0x60, 0x6F)]),
            // RSMU, Raven map: 02h GetSmuVersion, 0Ah/0Bh the two-step GetDramBaseAddress, 0Ch GetTableVersion,
            // 3Dh TransferTableToDram.
            new("rsmu-raven",
                Kind: MailboxProtocolRsmu,
                Smn: [new(0x60, 0x64, 0x3B10A20, 0x3B10A80, 0x3B10A88, 6)],
                Mmio: [],
                GetOpcodes: [0x02, 0x0A, 0x0B, 0x0C, 0x3D],
                Cpus: [(0x17, 0x10, 0x2F), (0x17, 0x50, 0x5F)]),
            // RSMU: 02h GetSmuVersion, 05h TransferTableToDram, 06h GetDramBaseAddress, 08h GetTableVersion.
            new("rsmu-matisse",
                Kind: MailboxProtocolRsmu,
                Smn: [new(0x60, 0x64, 0x3B10524, 0x3B10570, 0x3B10A40, 6)],
                Mmio: [],
                GetOpcodes: [0x02, 0x05, 0x06, 0x08],
                Cpus: [(0x17, 0x30, 0x3F), (0x17, 0x70, 0x7F), (0x19, 0x00, 0x0F), (0x19, 0x20, 0x2F)]),
            // RSMU: 02h GetSmuVersion, 03h TransferTableToDram, 04h GetDramBaseAddress, 05h GetTableVersion.
            new("rsmu-raphael",
                Kind: MailboxProtocolRsmu,
                Smn: [new(0x60, 0x64, 0x3B10524, 0x3B10570, 0x3B10A40, 6)],
                Mmio: [],
                GetOpcodes: [0x02, 0x03, 0x04, 0x05],
                Cpus: [(0x19, 0x18, 0x18), (0x19, 0x60, 0x6F), (0x1A, 0x40, 0x4F)]),
            // The Raphael map; Shimada Peak (1Ah M08h) moves MSG and RSP by 400h.
            new("rsmu-shimada",
                Kind: MailboxProtocolRsmu,
                Smn: [new(0x60, 0x64, 0x3B10924, 0x3B10970, 0x3B10A40, 6)],
                Mmio: [],
                GetOpcodes: [0x02, 0x03, 0x04, 0x05],
                Cpus: [(0x1A, 0x08, 0x08)]),
        ];

        /// <summary>
        /// ACPI IDs whose _CRS may hold an MMIO mailbox (AMD HSMP).
        /// </summary>
        public static readonly string[] MailboxAcpiIds = ["AMDI0097"];

        /// <summary>
        /// Returns whether a protocol is used by every model in [ModelMin, ModelMax] of a family.
        /// </summary>
        public static bool MailboxProtocolCovers(MailboxProtocol Protocol, UInt16 Family, UInt32 ModelMin, UInt32 ModelMax)
        {
            return Protocol.Cpus == null || Protocol.Cpus.Any(c => c.Family == Family && c.ModelMin <= ModelMin && ModelMax <= c.ModelMax);
        }

        /// <summary>
        /// The register set of a mailbox_transact body; Msg, Rsp and Arg0 sit at the same offsets in the SMN and window members.
        /// </summary>
        public static MailboxRegisters MailboxRegistersOf(ReadOnlySpan<byte> Body)
        {
            var mailbox = MemoryMarshal.Read<HwMailboxTransact>(Body);
            bool smn = mailbox.Transport == MailboxTransportSmnPair;

            return new MailboxRegisters(
                smn ? mailbox.Address.IndexOffset : (UInt16)0,
                smn ? mailbox.Address.DataOffset : (UInt16)0,
                mailbox.Address.Msg,
                mailbox.Address.Rsp,
                mailbox.Address.Arg0,
                mailbox.ArgCount);
        }

        /// <summary>
        /// Returns why the driver's version query (VersionOpcode, arg0 = VersionArg0, the rest zero, before the version
        /// is known) is not a command of a mailbox_transact body, or null when one command allows it.
        /// </summary>
        public static string MailboxVersionProblem(ReadOnlySpan<byte> Body)
        {
            var mailbox = MemoryMarshal.Read<HwMailboxTransact>(Body);
            int found = 0;

            for (int i = 0; i < mailbox.CommandCount; i++)
            {
                var command = Body.Slice(MailboxTransactSize + i * MailboxCommandSize, MailboxCommandSize);

                if (BinaryPrimitives.ReadUInt32LittleEndian(command) != mailbox.VersionOpcode)
                    continue;

                found++;

                UInt32 mask = BinaryPrimitives.ReadUInt32LittleEndian(command[8..]);
                UInt32 max = BinaryPrimitives.ReadUInt32LittleEndian(command[40..]);
                UInt32 fixedValue = BinaryPrimitives.ReadUInt32LittleEndian(command[72..]);

                if (command[4] != MailboxCommandGet)
                    return $"version opcode 0x{mailbox.VersionOpcode:x2} is not a GET command";
                if (command[5] < 1)
                    return $"version opcode 0x{mailbox.VersionOpcode:x2} command reads back no word";
                if ((mailbox.VersionArg0 & ~mask) != fixedValue || (mailbox.VersionArg0 & mask) > max)
                    return $"version-arg 0x{mailbox.VersionArg0:x} is not an argument 0 the version opcode 0x{mailbox.VersionOpcode:x2} command allows";

                for (int a = 1; a < MailboxMaxArgs; a++)
                {
                    if (BinaryPrimitives.ReadUInt32LittleEndian(command[(72 + a * 4)..]) != 0)
                        return $"version opcode 0x{mailbox.VersionOpcode:x2} command fixes argument {a} (the query writes zero)";
                }
            }

            if (found == 0)
                return $"version opcode 0x{mailbox.VersionOpcode:x2} is not one of the mailbox's commands";

            return null;
        }

        /// <summary>
        /// CPU vendors, and root complex PCI vendors, whose host bridge carries the SMN index/data pairs.
        /// </summary>
        public static readonly string[] SmnCpuVendors = ["AuthenticAMD", "HygonGenuine"];
        public static readonly UInt16[] SmnRootVendors = [0x1022, 0x1D94];
        /// <summary>
        /// PCI vendors of a PIIX4-style SMBus host (AMD FCH, Hygon, ATI SB); i801 is Intel only.
        /// </summary>
        public static readonly UInt16[] SmbusPiix4Vendors = [0x1022, 0x1D94, 0x1002];
        /// <summary>
        /// The (address, size) of every PM MMIO block an smbus-host may name, reviewed before it is listed.
        /// </summary>
        public static readonly (UInt64 Base, UInt32 Size)[] SmbusPmMmioBlocks =
        [
            (0xFED80300, 0x100), // AMD FCH AcpiMmio PM registers (PMx00-PMxFF)
        ];
        /// <summary>
        /// Host bridge class code, the class of a pciclass root complex.
        /// </summary>
        public const UInt32 HostBridgeClass = 0x060000;
        /// <summary>
        /// The index/data pairs an indirect read may use: SMN at 60h/64h, and D0F0xB8/xBC (family 15h).
        /// </summary>
        public static readonly (UInt16 Index, UInt16 Data)[] IndirectPairs = [(0x60, 0x64), (0xB8, 0xBC)];

        /// <summary>
        /// Intel PCH SMBus (i801): I/O BAR4, HOSTC 40h, INUSE_STS semaphore.
        /// </summary>
        public const byte SmbusHostI801 = 1;
        /// <summary>
        /// PIIX4-style SMBus with an SLVCNT semaphore, its base decoded and its port selected through PM registers the
        /// function's smbus_host ops describe.
        /// </summary>
        public const byte SmbusHostPiix4 = 2;
        public const byte SmbusProtocolByteData = 0x01;
        public const byte SmbusProtocolWordData = 0x02;
        /// <summary>
        /// Every protocol the driver implements is a read.
        /// </summary>
        public const byte SmbusProtocolMask = SmbusProtocolByteData | SmbusProtocolWordData;
        /// <summary>
        /// FCH PM 02h bits 4:3.
        /// </summary>
        public const byte SmbusPorts = 4;
        /// <summary>
        /// The PM registers of an smbus_host through the index/data port pair the row names; optionally a mapped view
        /// behind a gate read through the pair.
        /// </summary>
        public const byte SmbusPmPortPair = 1;
        /// <summary>
        /// The PM registers of an smbus_host in a fixed MMIO block, mapped for the request.
        /// </summary>
        public const byte SmbusPmMmio = 2;
        /// <summary>
        /// The host's own configuration space holds the SMBus base (decode, a word) and the host enable (enable); one
        /// port, no PM registers and no port select.
        /// </summary>
        public const byte SmbusPmConfig = 3;
        /// <summary>
        /// The port select register must not be written.
        /// </summary>
        public const byte SmbusHostFlagSelectReadOnly = 0x01;
        /// <summary>
        /// The host has no host semaphore in its slave control register (hosts before SB800).
        /// </summary>
        public const byte SmbusHostFlagNoSemaphore = 0x02;
        public const byte SmbusHostFlagMask = SmbusHostFlagSelectReadOnly | SmbusHostFlagNoSemaphore;
        /// <summary>
        /// The PCI revision ID, byte 08h of the configuration header.
        /// </summary>
        public const byte PciRevisionOffset = 0x08;
        /// <summary>
        /// Bit n: 7-bit address n.
        /// </summary>
        public const int SmbusSlaveMapSize = 16;
        /// <summary>
        /// General call/reserved (00h-07h) and 10-bit/reserved (78h-7Fh); generator-only, the driver serves any allow-listed slave.
        /// </summary>
        public static bool SmbusAddressReserved(int Address) => Address < 0x08 || Address >= 0x78;
        /// <summary>
        /// Host (08h), alert response (0Ch), EE1004 pseudo-addresses (30h-37h) and ARP default (61h); generator-only, the driver serves any allow-listed slave.
        /// </summary>
        public static bool SmbusAddressNotDevice(int Address) => Address == 0x08 || Address == 0x0C || (Address >= 0x30 && Address <= 0x37) || Address == 0x61;
        /// <summary>
        /// A byte-addressed device (byte and word allowed, or an SPD EEPROM slave 50h-57h) returns command + 1 on a word
        /// read; the driver checks only the command.
        /// </summary>
        public static bool SmbusWordReadsPastRange(byte Protocols, bool SpdSlave, IEnumerable<HwRange> Ranges) =>
            (Protocols & SmbusProtocolWordData) != 0 && ((Protocols & SmbusProtocolByteData) != 0 || SpdSlave) &&
            Ranges.Any(range => range.Offset != 0 || range.Length != 0x100);

        /// <summary>
        /// Returns whether a slave is an SPD EEPROM address (50h-57h).
        /// </summary>
        public static bool SmbusSpdSlave(int Address) => Address >= 0x50 && Address <= 0x57;

        /// <summary>
        /// The layout rules of an smbus_host body the loader and the section check share; null when it holds.
        /// </summary>
        public static unsafe string SmbusHostError(HwSmbusHost Host)
        {
            UInt32 registerEnd = Host.PmLength != 0 ? Host.PmLength : PortIndexEnd;
            int portMask = (SmbusPorts - 1) << Host.SelectShift;
            UInt64 pmBase = Host.PmBase;
            UInt64 pmEnd = Host.PmBase + Host.PmLength;
            bool view = Host.PmBase != 0 || Host.PmLength != 0 || Host.GateMask != 0;

            if (Host.PmAccess == SmbusPmConfig)
            {
                if (Host.IndexPort != 0 || Host.DataPort != 0 || Host.PmBase != 0 || Host.PmLength != 0)
                    return "a configuration layout has no ports and no PM view";
                if (Host.DecodeLow < PciConfigHeaderLength || (Host.DecodeLow & 1) != 0 || Host.DecodeHigh != Host.DecodeLow + 1)
                    return "decode must be an aligned configuration word above the header";
                if (Host.GateRegister < PciConfigHeaderLength || Host.GateMask == 0)
                    return "a configuration layout needs an enable register above the header";
                if (Host.BaseMask == 0)
                    return "base-mask must be non-zero";
                if ((Host.Flags & SmbusHostFlagSelectReadOnly) != 0 || Host.EnableMask != 0 || Host.Select != 0 || Host.SelectMask != 0 || Host.SelectShift != 0 ||
                    Host.SelectAlternate != 0 || Host.SelectChoose != 0 || Host.SelectChooseMask != 0)
                    return "a configuration layout has no enable-mask or port select";
                if (Host.Reserved[0] != 0 || Host.Reserved[1] != 0 || Host.Reserved[2] != 0 || Host.Reserved[3] != 0)
                    return "reserved bytes are non-zero";

                return null;
            }

            if (Host.PmAccess == SmbusPmPortPair)
            {
                if (!IoPortPairs.Contains((Host.IndexPort, Host.DataPort)))
                    return $"index port 0x{Host.IndexPort:x} / data port 0x{Host.DataPort:x} is not a pair of DynDataHw.IoPortPairs";
                if (view && (Host.PmBase == 0 || Host.PmLength == 0 || Host.GateMask == 0))
                    return "a mapped view needs base, length and enable together";
                if (!view && Host.GateRegister != 0)
                    return "an enable register without a mapped view";
            }
            else if (Host.PmAccess == SmbusPmMmio)
            {
                if (Host.IndexPort != 0 || Host.DataPort != 0 || Host.GateRegister != 0 || Host.GateMask != 0)
                    return "a mapped layout has no ports and no enable gate";
                if (Host.PmLength == 0 || Host.PmBase == 0)
                    return "a mapped layout needs base and length";
            }
            else
            {
                return $"PM access {Host.PmAccess} is unknown";
            }

            if ((Host.Flags & ~SmbusHostFlagMask) != 0)
                return $"flags 0x{Host.Flags:x2} unknown";
            if (Host.PmBase != 0 && !SmbusPmMmioBlocks.Any(block => pmBase >= block.Base && pmEnd <= block.Base + block.Size))
                return $"PM MMIO 0x{Host.PmBase:x} length 0x{Host.PmLength:x} is not inside a DynDataHw.SmbusPmMmioBlocks block";
            if (Host.PmBase != 0 && (pmBase & ~0xFFFul) != ((pmEnd - 1) & ~0xFFFul))
                return $"PM MMIO 0x{Host.PmBase:x} length 0x{Host.PmLength:x} crosses a page";
            if (Host.DecodeLow >= registerEnd || Host.DecodeHigh >= registerEnd || Host.Select >= registerEnd ||
                Host.SelectAlternate >= registerEnd || Host.SelectChoose >= registerEnd || Host.GateRegister >= registerEnd)
                return "a PM register is outside the PM registers";
            if (Host.DecodeLow == Host.DecodeHigh)
                return "decode names one register twice";
            if (Host.BaseMask == 0 || Host.EnableMask == 0)
                return "base-mask and enable-mask must be non-zero";
            if (Host.SelectMask == 0 || ((Host.SelectMask >> Host.SelectShift) & ((Host.SelectMask >> Host.SelectShift) + 1)) != 0)
                return $"select-mask 0x{Host.SelectMask:x2} is not one run of bits";
            if (Host.SelectShift != BitOperations.TrailingZeroCount(Host.SelectMask) || Host.SelectMask != portMask)
                return $"select-mask 0x{Host.SelectMask:x2} is not exactly the {SmbusPorts} ports";
            if (Host.SelectChooseMask == 0 ? (Host.SelectChoose != 0 || Host.SelectAlternate != 0) : Host.SelectAlternate == Host.Select)
                return "select-choose has no alternate, or an alternate without select-choose";
            if (Host.Reserved[0] != 0 || Host.Reserved[1] != 0 || Host.Reserved[2] != 0 || Host.Reserved[3] != 0)
                return "reserved bytes are non-zero";

            return null;
        }

        /// <summary>
        /// SMSC chips whose HWM device ID at 3Dh is known (SCH311x 8Ch, SCH5127 8Eh); their rows name it in an id.
        /// </summary>
        public static readonly UInt16[] SuperIoSmscChips = [0x7C, 0x7D, 0x7F, 0x86];

        /// <summary>
        /// Read-to-clear registers a family's rows must not cover (SMSC: interrupt status 41h, 42h, 83h).
        /// </summary>
        public static UInt16[] SuperIoReadToClear(byte Family) => Family == SuperIoFamilySmsc ? [0x41, 0x42, 0x83] : [];

        /// <summary>
        /// The default base= range of a superio element: ISA I/O above the legacy block, with the family's whole
        /// block below the FCH PM index/data ports at 0x0CD6 (0x0100-0x0CC8, or 0x0100-0x0C50 for SMSC's 128-byte block).
        /// </summary>
        public const UInt16 SuperIoBaseMinDefault = 0x0100;
        public static UInt16 SuperIoBaseMaxDefault(byte Family) => (UInt16)(0x0CD0 - SuperIoBaseExtent(Family));
        /// <summary>
        /// Bounds of any base= range.
        /// </summary>
        public const UInt16 SuperIoBaseFloor = 0x0100;
        public const UInt16 SuperIoBaseCeiling = 0xFFF0;
        /// <summary>
        /// Ports the HWM block of a superio element may never cover.
        /// </summary>
        public static readonly (UInt16 First, UInt16 Last, string Name)[] SuperIoBaseForbidden =
        [
            (0x0CF8, 0x0CFF, "PCI CF8/CFC"),
            (0x0CD6, 0x0CD7, "FCH PM"),
            (0x002E, 0x002F, "SuperIO config"),
            (0x004E, 0x004F, "SuperIO config")
        ];

        /// <summary>
        /// The ports the driver touches from the HWM base: SMSC's 128-byte runtime block, the 8-byte index/data block elsewhere.
        /// </summary>
        public static UInt16 SuperIoBaseExtent(byte Family) => Family == SuperIoFamilySmsc ? (UInt16)0x80 : (UInt16)8;

        /// <summary>
        /// Checks a superio_read base range, returning the complaint or null.
        /// </summary>
        public static string SuperIoBaseRangeError(UInt16 BaseMin, UInt16 BaseMax, UInt16 Extent)
        {
            if (BaseMin < SuperIoBaseFloor || BaseMin > BaseMax || BaseMax > SuperIoBaseCeiling)
                return $"base range 0x{BaseMin:x4}-0x{BaseMax:x4} is reversed or outside 0x{SuperIoBaseFloor:x4}-0x{SuperIoBaseCeiling:x4}";
            if ((BaseMin & 7) != 0)
                return $"base range minimum 0x{BaseMin:x4} is not 8-byte aligned";

            UInt32 last = (UInt32)BaseMax + Extent - 1;

            foreach (var (first, lastForbidden, name) in SuperIoBaseForbidden)
            {
                if (BaseMin <= lastForbidden && last >= first)
                    return $"base range 0x{BaseMin:x4}-0x{BaseMax:x4} (+0x{Extent:x}) overlaps {name} 0x{first:x4}-0x{lastForbidden:x4}";
            }

            return null;
        }

        public const byte FlagRequireD0 = 0x01;
        /// <summary>
        /// A PCI window or VSEC op ARM64 builds may read; x64 builds ignore it.
        /// </summary>
        public const byte FlagArm64 = 0x02;

        public const byte WindowBaseStandardBar = 1;
        public const byte WindowBaseConfigReg = 2;
        /// <summary>
        /// An LPIT native C-state residency counter, by ordinal among the usable entries; CPU rows only.
        /// </summary>
        public const byte WindowBaseAcpiLpit = 3;
        /// <summary>
        /// A _CRS memory descriptor of an ACPI device, by ordinal; ACPI_HID rows only.
        /// </summary>
        public const byte WindowBaseAcpiCrs = 4;
        /// <summary>
        /// AMD PMC aperture ((SMN[hi] &lt;&lt; 32) | SMN[lo]) &amp; ConfigMask while bit EnableBit is set; FixedBase is the window offset, Aux the SMN addresses.
        /// </summary>
        public const byte WindowBaseSmnDerived = 5;
        /// <summary>
        /// A fixed physical address below 4 GB (CPU rows behind a presence device in Aux[0], and BOARD rows).
        /// </summary>
        public const byte WindowBaseFixedMmio = 6;
        /// <summary>
        /// A fixed port range behind a signature in Aux; BOARD rows, x64, byte reads only.
        /// </summary>
        public const byte WindowBaseFixedIo = 7;
        /// <summary>
        /// The block holding an LPIT counter: counter address - FixedBase, aligned to WindowSize; Intel CPU rows only.
        /// </summary>
        public const byte WindowBaseAcpiLpitRelative = 8;
        /// <summary>
        /// The largest ACPI_LPIT_RELATIVE block.
        /// </summary>
        public const UInt32 AcpiLpitRelativeMaxSize = 0x10000;
        /// <summary>
        /// Every read of an ACPI_LPIT_RELATIVE window lies in the 4 KiB page that holds the counter.
        /// </summary>
        public const UInt32 AcpiLpitCounterPage = 0x1000;

        /// <summary>
        /// Whether a read range of an ACPI_LPIT_RELATIVE window lies in the page of the counter at offset Counter.
        /// </summary>
        public static bool InLpitCounterPage(UInt64 Counter, UInt64 Offset, UInt64 Length)
        {
            UInt64 page = Counter / AcpiLpitCounterPage;

            return Length != 0 && Offset / AcpiLpitCounterPage == page && (Offset + Length - 1) / AcpiLpitCounterPage == page;
        }
        /// <summary>
        /// Intel PMC PWRM registers that hold lock bits (CTEN, ECRPTEN, TL, TLEN, PHLC); no ACPI_LPIT_RELATIVE range covers them.
        /// </summary>
        public static readonly UInt32[] PwrmLockRegisters = [0x150C, 0x1510, 0x1520, 0x1528, 0x1540];
        /// <summary>
        /// The base is 1 MB aligned; FixedBase + WindowSize end inside it.
        /// </summary>
        public const UInt32 SmnApertureSize = 0x100000;
        /// <summary>
        /// The legacy ports below are never a window.
        /// </summary>
        public const UInt32 FixedIoMinBase = 0x100;
        public const UInt32 FixedIoMaxSize = 0x100;
        public const UInt32 FixedIoMaxSignature = 4;
        /// <summary>
        /// The (address, size) of every FIXED_MMIO window, and (port, size) of every FIXED_IO window, reviewed before it is listed.
        /// </summary>
        public static readonly (UInt64 Base, UInt32 Size)[] FixedMmioWindows = [];
        public static readonly (UInt64 Base, UInt32 Size)[] FixedIoWindows =
        [
            (0xE00, 0x100), // ChromeOS EC memory map at the Framework NPCX base
        ];
        public const byte AcpiLpitMaxEntries = 8;
        public const byte WindowSpaceMmio = 0;
        /// <summary>
        /// FIXED_IO only.
        /// </summary>
        public const byte WindowSpaceIo = 1;
        public const byte WindowEnableNone = 0xFF;
        public const UInt32 MaxWindowSize = 0x100000;
        public const byte PciBarCount = 6;

        public const byte IdentityCpu = 1;
        public const byte IdentityPci = 2;
        public const byte IdentityCpuAndPci = 3;
        /// <summary>
        /// CPU and PCI vendor, class code and device/function.
        /// </summary>
        public const byte IdentityCpuAndPciClass = 4;
        public const byte IdentitySuperIo = 5;
        /// <summary>
        /// Hashed by <see cref="BoardHash"/>, which must match KphpHwHashBoard.
        /// </summary>
        public const byte IdentityBoard = 6;
        /// <summary>
        /// An ACPI device by a _HID or _CID read from its physical device object.
        /// </summary>
        public const byte IdentityAcpiHid = 9;
        /// <summary>
        /// An ARM64 core by MIDR_EL1, read on the processor of each entry.
        /// </summary>
        public const byte IdentityArmCpu = 11;

        /// <summary>
        /// 0x2E/0x2F, 0x4E/0x4F.
        /// </summary>
        public const byte SuperIoSlots = 2;
        /// <summary>
        /// Nuvoton NCT67xx: enter 87h 87h, exit AAh; HWM at base+5/+6, bank select 4Eh; registers (bank &lt;&lt; 8) | index.
        /// </summary>
        public const byte SuperIoFamilyNuvoton = 1;
        /// <summary>
        /// Nuvoton NCT668x: as Nuvoton, HWM LDN 0Bh; EC page/index/data at base+4/+5/+6; customer ID at EC 602h/603h.
        /// </summary>
        public const byte SuperIoFamilyNuvotonEc = 2;
        /// <summary>
        /// ITE IT86xx/IT87xx: enter 87h 01h 55h 55h (AAh at 4Eh), exit config 02h &lt;- 02h or none; flat HWM at base+5/+6.
        /// </summary>
        public const byte SuperIoFamilyIte = 3;
        /// <summary>
        /// Fintek F718xx/F818xx: enter 87h 87h, exit AAh, manufacturer ID 1934h at config 23h/24h; flat HWM at base+5/+6.
        /// </summary>
        public const byte SuperIoFamilyFintek = 4;
        /// <summary>
        /// SMSC SCH311x/SCH5127: enter 55h, exit AAh, 8-bit ID at config 20h, runtime LDN 0Ah; flat HWM at base+70h/+71h,
        /// company ID 5Ch at 3Eh.
        /// </summary>
        public const byte SuperIoFamilySmsc = 5;
        /// <summary>
        /// The chip stays in configuration mode.
        /// </summary>
        public const byte SuperIoExitNone = 0;
        /// <summary>
        /// AAh to the index port.
        /// </summary>
        public const byte SuperIoExitKeyAa = 1;
        /// <summary>
        /// Configuration register 02h &lt;- 02h.
        /// </summary>
        public const byte SuperIoExitConfig02 = 2;
        /// <summary>
        /// The NCT668x HWM logical device, selected before the customer ID is known; an authoring rule.
        /// </summary>
        public const byte SuperIoEcLdn = 0x0B;
        /// <summary>
        /// The slot's logical device need not be activated (the identity bytes at the base are the check).
        /// </summary>
        public const byte SuperIoFlagActiveOptional = 0x01;
        public const byte SuperIoFlagMask = SuperIoFlagActiveOptional;
        /// <summary>
        /// The Nuvoton bank register bit that selects the high byte of a 16-bit register (HBACS).
        /// </summary>
        public const byte SuperIoNuvotonHbacs = 0x80;
        public const byte SuperIoMaxIds = 2;
        /// <summary>
        /// The configuration index port of a slot when the row names none (2Eh, 4Eh); the data port is the next one.
        /// </summary>
        public static UInt16 SuperIoPortDefault(byte Slot) => Slot == 0 ? (UInt16)0x2E : (UInt16)0x4E;
        /// <summary>
        /// ITE chips that must never leave configuration mode.
        /// </summary>
        public static readonly UInt16[] SuperIoNoConfigChips = [0x8790, 0x8733, 0x8695];

        public const byte LockNone = 0;
        /// <summary>
        /// Global\Access_PCI.
        /// </summary>
        public const byte LockPci = 1;
        /// <summary>
        /// Global\Access_ISABUS.HTP.Method.
        /// </summary>
        public const byte LockIsaBus = 2;
        /// <summary>
        /// Global\Access_SMBUS.HTP.Method.
        /// </summary>
        public const byte LockSmbus = 3;

        public const byte GateNone = 0;
        public const byte GateCpuid = 1;
        /// <summary>
        /// SMBIOS chassis type (type 3 offset 05h bits 6:0) bit set in Value | ValueMax &lt;&lt; 32; superio_read only.
        /// </summary>
        public const byte GateChassis = 2;
        /// <summary>
        /// An ID register field in [Value, ValueMax] on the processor of the entry; sysreg_read only.
        /// </summary>
        public const byte GateSysregField = 3;
        /// <summary>
        /// A field of a 4-aligned configuration header register of the op's PCI function in [Value, ValueMax]; smbus_host only.
        /// </summary>
        public const byte GatePciConfig = 4;

        public const byte CpuidEax = 0;
        public const byte CpuidEbx = 1;
        public const byte CpuidEcx = 2;
        public const byte CpuidEdx = 3;

        /// <summary>
        /// Fixed layout sizes, asserted in kphdyn.h and against the mirrors below.
        /// </summary>
        public const int CpuIdentitySize = 16;
        public const int PciIdentitySize = 4;
        public const int CpuAndPciIdentitySize = 20;
        public const int CpuAndPciClassIdentitySize = 24;
        public const int SuperIoIdentitySize = 6;
        public const int BoardIdentitySize = 16;
        public const int AcpiHidIdentitySize = AcpiIdSize;
        public const int ArmCpuIdentitySize = 6;
        public const int IdentitySize = 24;
        public const int IndexSize = 40;
        public const int IndexIdentityOffset = 8;
        public const int GateSize = 20;
        public const int OpHeaderSize = 32;
        public const int MsrReadSize = 16;
        public const int RangeSize = 4;
        public const int PciReadHeaderSize = 2;
        public const int TargetRangeSize = 16;
        public const int PciIndirectReadSize = 8;
        public const int WindowRangeSize = 12;
        public const int WindowReadSize = 32;
        public const int SuperIoReadSize = 26;
        public const int SuperIoIdSize = 4;
        public const int SuperIoIdsOffset = 18;
        public static HwSuperIoId SuperIoId(ReadOnlySpan<byte> Body, int Index) =>
            MemoryMarshal.Read<HwSuperIoId>(Body[(SuperIoIdsOffset + Index * SuperIoIdSize)..]);
        public const int SmbusReadSize = 20;
        public const int SmbusSlavesOffset = 4;
        public const int VsecReadSize = 20;
        public const int MailboxAddressSize = 20;
        public const int MailboxAddressOffset = 12;
        public const int MailboxCommandSize = 104;
        public const int MailboxTransactSize = 32;
        public const int SmuTableVersionSize = 8;
        public const int SmuTableReadSize = 20;
        public const int AcpiArgSize = 12;
        public const int AcpiEvalSize = 8;
        public const int AcpiRequireSize = 1;
        public const int SysregReadSize = 16;
        public const int PortIndirectReadSize = 8;
        public const int SmbusHostSize = 32;
        public const int DfMapSize = 24;
        public const int SectionSize = 16;
        public const int TableSize = 8;
        /// <summary>
        /// Every op length is a multiple of this, so each op, and the 8-byte fields of its body, stay naturally aligned.
        /// </summary>
        public const int OpAlignment = 8;

        /// <summary>
        /// Rounds an op length up to OpAlignment.
        /// </summary>
        public static int AlignOp(int Length) => (Length + OpAlignment - 1) & ~(OpAlignment - 1);

        /// <summary>
        /// The kinds the driver implements; a build may only emit these.
        /// </summary>
        public static readonly HwKindSet DriverKinds = new(
            OpKinds: [KindMsrRead, KindPciRead, KindPciIndirectRead, KindWindowRead, KindSuperIoRead, KindSmbusRead, KindVsecRead, KindMailboxTransact, KindSmuTableRead, KindAcpiEval, KindSysregRead, KindPortIndirectRead, KindSmbusHost, KindDfMap, KindAcpiRequire],
            IdentityKinds: [IdentityCpu, IdentityPci, IdentityCpuAndPci, IdentityCpuAndPciClass, IdentitySuperIo, IdentityBoard, IdentityAcpiHid, IdentityArmCpu],
            GateKinds: [GateNone, GateCpuid, GateChassis, GateSysregField, GatePciConfig],
            WindowBaseKinds: [WindowBaseStandardBar, WindowBaseConfigReg, WindowBaseAcpiLpit, WindowBaseAcpiCrs, WindowBaseSmnDerived, WindowBaseFixedMmio, WindowBaseFixedIo, WindowBaseAcpiLpitRelative],
            SuperIoFamilies: [SuperIoFamilyNuvoton, SuperIoFamilyNuvotonEc, SuperIoFamilyIte, SuperIoFamilyFintek, SuperIoFamilySmsc],
            SmbusHosts: [SmbusHostI801, SmbusHostPiix4],
            VsecDiscoveries: [VsecDiscoveryPmtTelemetry, VsecDiscoveryTpmi],
            MailboxTransports: [MailboxTransportSmnPair, MailboxTransportMmioWindow],
            MailboxProtocolKinds: [MailboxProtocolRsmu, MailboxProtocolHsmp],
            SmuTableBounds: [SmuTableBoundAmdDram],
            SysregRegisters: [.. Enumerable.Range(SysregMidr, SysregMax).Select(r => (byte)r)]
        );

        private static readonly FrozenDictionary<byte, string> IdentityKindNames = new Dictionary<byte, string>
        {
            [IdentityCpu] = "KPH_DYN_HW_IDENTITY_CPU",
            [IdentityPci] = "KPH_DYN_HW_IDENTITY_PCI",
            [IdentityCpuAndPci] = "KPH_DYN_HW_IDENTITY_CPU_AND_PCI",
            [IdentityCpuAndPciClass] = "KPH_DYN_HW_IDENTITY_CPU_AND_PCI_CLASS",
            [IdentitySuperIo] = "KPH_DYN_HW_IDENTITY_SUPERIO",
            [IdentityBoard] = "KPH_DYN_HW_IDENTITY_BOARD",
            [IdentityAcpiHid] = "KPH_DYN_HW_IDENTITY_ACPI_HID",
            [IdentityArmCpu] = "KPH_DYN_HW_IDENTITY_ARM_CPU",
        }.ToFrozenDictionary();

        /// <summary>
        /// Builds KPH_DYN_HW_IMPLEMENTED_IDENTITY_KINDS for kphdyn.h.
        /// </summary>
        /// <returns>The #define, without a trailing newline.</returns>
        public static string ImplementedIdentityKindsDefinition()
        {
            var bits = DriverKinds.IdentityKinds.Order().Select(kind => IdentityKindNames.TryGetValue(kind, out string name) ?
                $"(1ull << {name})" :
                throw new InvalidOperationException($"identity kind {kind} has no kphdyn.h name"));

            return "#define KPH_DYN_HW_IMPLEMENTED_IDENTITY_KINDS \\\r\n    (" + string.Join(" | \\\r\n     ", bits) + ")";
        }

        /// <summary>
        /// Field offsets (offset 0 omitted), asserted in kphdyn.h and checked against the mirrors.
        /// </summary>
        private static readonly (string Name, string Mirror, string Field, int Offset, Func<int> Actual)[] FieldOffsets =
        [
            FieldOffset<HwCpuIdentity>("KPH_DYN_HW_CPU_IDENTITY", "DisplayFamily", 12),
            FieldOffset<HwCpuIdentity>("KPH_DYN_HW_CPU_IDENTITY", "DisplayModel", 14),
            FieldOffset<HwCpuIdentity>("KPH_DYN_HW_CPU_IDENTITY", "Reserved", 15),
            FieldOffset<HwPciIdentity>("KPH_DYN_HW_PCI_IDENTITY", "DeviceId", 2),
            FieldOffset<HwCpuAndPciIdentity>("KPH_DYN_HW_CPU_AND_PCI_IDENTITY", "Pci", 16),
            FieldOffset<HwCpuAndPciClassIdentity>("KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY", "VendorId", 16),
            FieldOffset<HwCpuAndPciClassIdentity>("KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY", "BaseClass", 18),
            FieldOffset<HwCpuAndPciClassIdentity>("KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY", "SubClass", 19),
            FieldOffset<HwCpuAndPciClassIdentity>("KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY", "ProgIf", 20),
            FieldOffset<HwCpuAndPciClassIdentity>("KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY", "HeaderType", 21),
            FieldOffset<HwCpuAndPciClassIdentity>("KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY", "DevFn", 22),
            FieldOffset<HwCpuAndPciClassIdentity>("KPH_DYN_HW_CPU_AND_PCI_CLASS_IDENTITY", "Reserved", 23),
            FieldOffset<HwSuperIoIdentity>("KPH_DYN_HW_SUPERIO_IDENTITY", "VendorFamily", 1),
            FieldOffset<HwSuperIoIdentity>("KPH_DYN_HW_SUPERIO_IDENTITY", "ChipId", 2),
            FieldOffset<HwSuperIoIdentity>("KPH_DYN_HW_SUPERIO_IDENTITY", "CustomerId", 4),
            FieldOffset<HwBoardIdentity>("KPH_DYN_HW_BOARD_IDENTITY", "SystemHash", 8),
            FieldOffset<HwArmCpuIdentity>("KPH_DYN_HW_ARM_CPU_IDENTITY", "Variant", 1),
            FieldOffset<HwArmCpuIdentity>("KPH_DYN_HW_ARM_CPU_IDENTITY", "PartNum", 2),
            FieldOffset<HwArmCpuIdentity>("KPH_DYN_HW_ARM_CPU_IDENTITY", "Revision", 4),
            FieldOffset<HwArmCpuIdentity>("KPH_DYN_HW_ARM_CPU_IDENTITY", "Reserved", 5),
            FieldOffset<HwIndex>("KPH_DYN_HW_INDEX", "Reserved", 1),
            FieldOffset<HwIndex>("KPH_DYN_HW_INDEX", "Identity", 8),
            FieldOffset<HwIndex>("KPH_DYN_HW_INDEX", "Offset", 32),
            FieldOffset<HwIndex>("KPH_DYN_HW_INDEX", "Length", 36),
            FieldOffset<HwGate>("KPH_DYN_HW_GATE", "Register", 1),
            FieldOffset<HwGate>("KPH_DYN_HW_GATE", "Shift", 2),
            FieldOffset<HwGate>("KPH_DYN_HW_GATE", "Width", 3),
            FieldOffset<HwGate>("KPH_DYN_HW_GATE", "Leaf", 4),
            FieldOffset<HwGate>("KPH_DYN_HW_GATE", "SubLeaf", 8),
            FieldOffset<HwGate>("KPH_DYN_HW_GATE", "Value", 12),
            FieldOffset<HwGate>("KPH_DYN_HW_GATE", "ValueMax", 16),
            FieldOffset<HwOpHeader>("KPH_DYN_HW_OP", "Length", 2),
            FieldOffset<HwOpHeader>("KPH_DYN_HW_OP", "Reserved0", 4),
            FieldOffset<HwOpHeader>("KPH_DYN_HW_OP", "LockClass", 5),
            FieldOffset<HwOpHeader>("KPH_DYN_HW_OP", "Flags", 6),
            FieldOffset<HwOpHeader>("KPH_DYN_HW_OP", "Reserved", 7),
            FieldOffset<HwOpHeader>("KPH_DYN_HW_OP", "Reserved2", 8),
            FieldOffset<HwOpHeader>("KPH_DYN_HW_OP", "Gate", 12),
            FieldOffset<HwMsrRead>("KPH_DYN_HW_MSR_READ", "Reserved", 4),
            FieldOffset<HwMsrRead>("KPH_DYN_HW_MSR_READ", "ReadMask", 8),
            FieldOffset<HwRange>("KPH_DYN_HW_RANGE", "Length", 2),
            FieldOffset<HwPciReadHeader>("KPH_DYN_HW_PCI_READ", "Reserved", 1),
            FieldOffset<HwTargetRange>("KPH_DYN_HW_TARGET_RANGE", "End", 4),
            FieldOffset<HwTargetRange>("KPH_DYN_HW_TARGET_RANGE", "Stride", 8),
            FieldOffset<HwTargetRange>("KPH_DYN_HW_TARGET_RANGE", "StrideCount", 12),
            FieldOffset<HwTargetRange>("KPH_DYN_HW_TARGET_RANGE", "Reserved", 13),
            FieldOffset<HwPciIndirectRead>("KPH_DYN_HW_PCI_INDIRECT_READ", "DataOffset", 2),
            FieldOffset<HwPciIndirectRead>("KPH_DYN_HW_PCI_INDIRECT_READ", "PinDevice", 4),
            FieldOffset<HwPciIndirectRead>("KPH_DYN_HW_PCI_INDIRECT_READ", "PinFunction", 5),
            FieldOffset<HwPciIndirectRead>("KPH_DYN_HW_PCI_INDIRECT_READ", "RangeCount", 6),
            FieldOffset<HwPciIndirectRead>("KPH_DYN_HW_PCI_INDIRECT_READ", "ValueCount", 7),
            FieldOffset<HwWindowRange>("KPH_DYN_HW_WINDOW_RANGE", "Length", 4),
            FieldOffset<HwWindowRange>("KPH_DYN_HW_WINDOW_RANGE", "Width", 8),
            FieldOffset<HwWindowRange>("KPH_DYN_HW_WINDOW_RANGE", "Reserved", 9),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "BarIndex", 1),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "SpaceKind", 2),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "EnableBit", 3),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "ConfigOffset", 4),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "Is64", 6),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "Width", 7),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "ConfigMask", 8),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "FixedBase", 16),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "WindowSize", 24),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "RangeCount", 28),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "AuxCount", 29),
            FieldOffset<HwWindowRead>("KPH_DYN_HW_WINDOW_READ", "Reserved", 30),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "Exit", 1),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "RangeCount", 2),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "IdCount", 3),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "BaseMin", 4),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "BaseMax", 6),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "ConfigPort", 8),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "Flags", 10),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "BankMask", 11),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "ConfigIdIndex", 12),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "Reserved", 13),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "ConfigIdValue", 14),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "CustomerRegister", 16),
            FieldOffset<HwSuperIoRead>("KPH_DYN_HW_SUPERIO_READ", "Ids", 18),
            FieldOffset<HwSuperIoId>("KPH_DYN_HW_SUPERIO_ID", "Value", 2),
            FieldOffset<HwSuperIoId>("KPH_DYN_HW_SUPERIO_ID", "Alternate", 3),
            FieldOffset<HwSmbusRead>("KPH_DYN_HW_SMBUS_READ", "Port", 1),
            FieldOffset<HwSmbusRead>("KPH_DYN_HW_SMBUS_READ", "Protocols", 2),
            FieldOffset<HwSmbusRead>("KPH_DYN_HW_SMBUS_READ", "RangeCount", 3),
            FieldOffset<HwSmbusRead>("KPH_DYN_HW_SMBUS_READ", "Slaves", 4),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "Quirks", 1),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "MaxRegions", 2),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "FeatureId", 3),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "Guid", 4),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "MaxRegionSize", 8),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "FeatureCapsMask", 12),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "RangeCount", 16),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "Revision", 17),
            FieldOffset<HwVsecRead>("KPH_DYN_HW_VSEC_READ", "DvsecVendor", 18),
            FieldOffset<HwMailboxSmn>("KPH_DYN_HW_MAILBOX_SMN", "DataOffset", 2),
            FieldOffset<HwMailboxSmn>("KPH_DYN_HW_MAILBOX_SMN", "PinDevice", 4),
            FieldOffset<HwMailboxSmn>("KPH_DYN_HW_MAILBOX_SMN", "PinFunction", 5),
            FieldOffset<HwMailboxSmn>("KPH_DYN_HW_MAILBOX_SMN", "Reserved", 6),
            FieldOffset<HwMailboxSmn>("KPH_DYN_HW_MAILBOX_SMN", "Msg", 8),
            FieldOffset<HwMailboxSmn>("KPH_DYN_HW_MAILBOX_SMN", "Rsp", 12),
            FieldOffset<HwMailboxSmn>("KPH_DYN_HW_MAILBOX_SMN", "Arg0", 16),
            FieldOffset<HwMailboxWindow>("KPH_DYN_HW_MAILBOX_WINDOW", "BarIndex", 1),
            FieldOffset<HwMailboxWindow>("KPH_DYN_HW_MAILBOX_WINDOW", "Reserved", 2),
            FieldOffset<HwMailboxWindow>("KPH_DYN_HW_MAILBOX_WINDOW", "WindowSize", 4),
            FieldOffset<HwMailboxWindow>("KPH_DYN_HW_MAILBOX_WINDOW", "Msg", 8),
            FieldOffset<HwMailboxWindow>("KPH_DYN_HW_MAILBOX_WINDOW", "Rsp", 12),
            FieldOffset<HwMailboxWindow>("KPH_DYN_HW_MAILBOX_WINDOW", "Arg0", 16),
            FieldOffset<HwMailboxCommand>("KPH_DYN_HW_MAILBOX_COMMAND", "IsGet", 4),
            FieldOffset<HwMailboxCommand>("KPH_DYN_HW_MAILBOX_COMMAND", "ResponseWords", 5),
            FieldOffset<HwMailboxCommand>("KPH_DYN_HW_MAILBOX_COMMAND", "Flags", 6),
            FieldOffset<HwMailboxCommand>("KPH_DYN_HW_MAILBOX_COMMAND", "Reserved", 7),
            FieldOffset<HwMailboxCommand>("KPH_DYN_HW_MAILBOX_COMMAND", "ArgMask", 8),
            FieldOffset<HwMailboxCommand>("KPH_DYN_HW_MAILBOX_COMMAND", "ArgMax", 40),
            FieldOffset<HwMailboxCommand>("KPH_DYN_HW_MAILBOX_COMMAND", "ArgFixed", 72),
            FieldOffset<HwMailboxTransact>("KPH_DYN_HW_MAILBOX_TRANSACT", "ArgCount", 1),
            FieldOffset<HwMailboxTransact>("KPH_DYN_HW_MAILBOX_TRANSACT", "CommandCount", 2),
            FieldOffset<HwMailboxTransact>("KPH_DYN_HW_MAILBOX_TRANSACT", "Protocol", 3),
            FieldOffset<HwMailboxTransact>("KPH_DYN_HW_MAILBOX_TRANSACT", "VersionOpcode", 4),
            FieldOffset<HwMailboxTransact>("KPH_DYN_HW_MAILBOX_TRANSACT", "VersionArg0", 8),
            FieldOffset<HwMailboxTransact>("KPH_DYN_HW_MAILBOX_TRANSACT", "Address", 12),
            FieldOffset<HwSmuTableVersion>("KPH_DYN_HW_SMU_TABLE_VERSION", "Size", 4),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "Bound", 1),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "VersionCount", 2),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "SetupOpcode", 3),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "VersionOpcode", 4),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "TransferOpcode", 8),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "BaseOpcode", 12),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "BaseLowWord", 16),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "BaseHighWord", 17),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "SizeWord", 18),
            FieldOffset<HwSmuTableRead>("KPH_DYN_HW_SMU_TABLE_READ", "DfMapOp", 19),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "MapCount", 1),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "MapStride", 2),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "AddressShift", 3),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "MapOffset", 4),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "HoleOffset", 6),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "AddressMask", 8),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "HoleBaseMask", 12),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "LimitOffset", 16),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "ControlOffset", 17),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "ValidBit", 18),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "HoleRemapBit", 19),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "HoleValidBit", 20),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "Device", 21),
            FieldOffset<HwDfMap>("KPH_DYN_HW_DF_MAP", "Reserved", 22),
            FieldOffset<HwAcpiArg>("KPH_DYN_HW_ACPI_ARG", "Max", 4),
            FieldOffset<HwAcpiArg>("KPH_DYN_HW_ACPI_ARG", "Fixed", 8),
            FieldOffset<HwAcpiEval>("KPH_DYN_HW_ACPI_EVAL", "ArgCount", 4),
            FieldOffset<HwAcpiEval>("KPH_DYN_HW_ACPI_EVAL", "ResultTypes", 5),
            FieldOffset<HwAcpiEval>("KPH_DYN_HW_ACPI_EVAL", "MaxOutput", 6),
            FieldOffset<HwSysregRead>("KPH_DYN_HW_SYSREG_READ", "Reserved", 1),
            FieldOffset<HwSysregRead>("KPH_DYN_HW_SYSREG_READ", "ReadMask", 8),
            FieldOffset<HwPortIndirectRead>("KPH_DYN_HW_PORT_INDIRECT_READ", "PinDevice", 1),
            FieldOffset<HwPortIndirectRead>("KPH_DYN_HW_PORT_INDIRECT_READ", "PinFunction", 2),
            FieldOffset<HwPortIndirectRead>("KPH_DYN_HW_PORT_INDIRECT_READ", "RangeCount", 3),
            FieldOffset<HwPortIndirectRead>("KPH_DYN_HW_PORT_INDIRECT_READ", "IndexPort", 4),
            FieldOffset<HwPortIndirectRead>("KPH_DYN_HW_PORT_INDIRECT_READ", "DataPort", 6),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "Flags", 1),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "PmLength", 2),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "DecodeLow", 3),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "DecodeHigh", 4),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "EnableMask", 5),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "BaseMask", 6),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "Select", 8),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "SelectAlternate", 9),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "SelectChoose", 10),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "SelectChooseMask", 11),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "SelectMask", 12),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "SelectShift", 13),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "GateRegister", 14),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "GateMask", 15),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "PmBase", 16),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "IndexPort", 24),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "DataPort", 26),
            FieldOffset<HwSmbusHost>("KPH_DYN_HW_SMBUS_HOST", "Reserved", 28),
            FieldOffset<HwSectionHeader>("KPH_DYN_HW_SECTION", "Length", 4),
            FieldOffset<HwSectionHeader>("KPH_DYN_HW_SECTION", "Count", 8),
            FieldOffset<HwSectionHeader>("KPH_DYN_HW_SECTION", "Reserved", 12),
            FieldOffset<HwTableHeader>("KPH_DYN_HW_TABLE", "Reserved", 4),
        ];

        /// <summary>
        /// Marshal.OffsetOf takes the mirror as a type argument so the AOT compiler keeps its layout data.
        /// </summary>
        private static (string Name, string Mirror, string Field, int Offset, Func<int> Actual) FieldOffset<T>(string Name, string Field, int Offset) where T : struct
        {
            return (Name, typeof(T).Name, Field, Offset, () => Marshal.OffsetOf<T>(Field).ToInt32());
        }

        /// <summary>
        /// Builds the FIELD_OFFSET assertions for kphdyn.h.
        /// </summary>
        /// <returns>One C_ASSERT per line, without a trailing newline.</returns>
        public static string FieldOffsetAsserts()
        {
            return string.Join("\r\n", FieldOffsets.Select(entry => $"C_ASSERT(FIELD_OFFSET({entry.Name}, {entry.Field}) == {entry.Offset});"));
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwCpuIdentity
        {
            public fixed UInt32 Vendor[3];
            public UInt16 DisplayFamily;
            public byte DisplayModel;
            public byte Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwPciIdentity
        {
            public UInt16 VendorId;
            public UInt16 DeviceId;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwCpuAndPciIdentity
        {
            public HwCpuIdentity Cpu;
            public HwPciIdentity Pci;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwCpuAndPciClassIdentity
        {
            public HwCpuIdentity Cpu;
            public UInt16 VendorId;
            public byte BaseClass;
            public byte SubClass;
            public byte ProgIf;
            public byte HeaderType;
            public byte DevFn;
            public byte Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwSuperIoIdentity
        {
            public byte Slot;
            public byte VendorFamily;
            public UInt16 ChipId;
            public UInt16 CustomerId;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwBoardIdentity
        {
            public UInt64 BaseboardHash;
            public UInt64 SystemHash;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwAcpiHidIdentity
        {
            public fixed byte Id[AcpiIdSize];
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwArmCpuIdentity
        {
            public byte Implementer;
            public byte Variant;
            public UInt16 PartNum;
            public byte Revision;
            public byte Reserved;
        }

        [StructLayout(LayoutKind.Explicit, Size = IdentitySize)]
        public struct HwIdentity
        {
            [FieldOffset(0)] public HwCpuIdentity Cpu;
            [FieldOffset(0)] public HwPciIdentity Pci;
            [FieldOffset(0)] public HwCpuAndPciIdentity CpuAndPci;
            [FieldOffset(0)] public HwCpuAndPciClassIdentity CpuAndPciClass;
            [FieldOffset(0)] public HwSuperIoIdentity SuperIo;
            [FieldOffset(0)] public HwBoardIdentity Board;
            [FieldOffset(0)] public HwAcpiHidIdentity AcpiHid;
            [FieldOffset(0)] public HwArmCpuIdentity ArmCpu;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwIndex
        {
            public byte IdentityKind;
            public fixed byte Reserved[7];
            public HwIdentity Identity;
            public UInt32 Offset;
            public UInt32 Length;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwGate
        {
            public byte Kind;
            public byte Register;
            public byte Shift;
            public byte Width;
            public UInt32 Leaf;
            public UInt32 SubLeaf;
            public UInt32 Value;
            public UInt32 ValueMax;
        }

        /// <summary>
        /// The KPH_DYN_HW_OP header, excluding Body.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwOpHeader
        {
            public UInt16 Kind;
            public UInt16 Length;
            public byte Reserved0;
            public byte LockClass;
            public byte Flags;
            public byte Reserved;
            public UInt32 Reserved2;
            public HwGate Gate;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwMsrRead
        {
            public UInt32 Register;
            public UInt32 Reserved;
            public UInt64 ReadMask;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwRange
        {
            public UInt16 Offset;
            public UInt16 Length;
        }

        /// <summary>
        /// The KPH_DYN_HW_PCI_READ header; ranges follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwPciReadHeader
        {
            public byte RangeCount;
            public byte Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwTargetRange
        {
            public UInt32 Begin;
            public UInt32 End;
            public UInt32 Stride;
            public byte StrideCount;
            public fixed byte Reserved[3];
        }

        /// <summary>
        /// The KPH_DYN_HW_PCI_INDIRECT_READ header; ranges and values follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwPciIndirectRead
        {
            public UInt16 IndexOffset;
            public UInt16 DataOffset;
            public byte PinDevice;
            public byte PinFunction;
            public byte RangeCount;
            public byte ValueCount;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwWindowRange
        {
            public UInt32 Offset;
            public UInt32 Length;
            public byte Width;
            public fixed byte Reserved[3];
        }

        /// <summary>
        /// The KPH_DYN_HW_WINDOW_READ header; ranges and aux follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwWindowRead
        {
            public byte BaseKind;
            public byte BarIndex;
            public byte SpaceKind;
            public byte EnableBit;
            public UInt16 ConfigOffset;
            public byte Is64;
            public byte Width;
            public UInt64 ConfigMask;
            public UInt64 FixedBase;
            public UInt32 WindowSize;
            public byte RangeCount;
            public byte AuxCount;
            public fixed byte Reserved[2];
        }

        /// <summary>
        /// The KPH_DYN_HW_SUPERIO_READ header; ranges follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwSuperIoRead
        {
            public byte Ldn;
            public byte Exit;
            public byte RangeCount;
            public byte IdCount;
            public UInt16 BaseMin;
            public UInt16 BaseMax;
            public UInt16 ConfigPort;
            public byte Flags;
            public byte BankMask;
            public byte ConfigIdIndex;
            public byte Reserved;
            public UInt16 ConfigIdValue;
            public UInt16 CustomerRegister;
            public fixed byte Ids[SuperIoMaxIds * SuperIoIdSize];
        }

        /// <summary>
        /// The KPH_DYN_HW_SUPERIO_ID body: a monitor register (bank or page in the high byte) and its accepted values.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwSuperIoId
        {
            public UInt16 Register;
            public byte Value;
            public byte Alternate;
        }

        /// <summary>
        /// The KPH_DYN_HW_SMBUS_READ header; ranges follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwSmbusRead
        {
            public byte HostKind;
            public byte Port;
            public byte Protocols;
            public byte RangeCount;
            public fixed byte Slaves[SmbusSlaveMapSize];
        }

        /// <summary>
        /// The KPH_DYN_HW_VSEC_READ header; ranges follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwVsecRead
        {
            public byte Discovery;
            public byte Quirks;
            public byte MaxRegions;
            public byte FeatureId;
            public UInt32 Guid;
            public UInt32 MaxRegionSize;
            public UInt32 FeatureCapsMask;
            public byte RangeCount;
            public byte Revision;
            public UInt16 DvsecVendor;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwMailboxSmn
        {
            public UInt16 IndexOffset;
            public UInt16 DataOffset;
            public byte PinDevice;
            public byte PinFunction;
            public fixed byte Reserved[2];
            public UInt32 Msg;
            public UInt32 Rsp;
            public UInt32 Arg0;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwMailboxWindow
        {
            public byte BaseKind;
            public byte BarIndex;
            public fixed byte Reserved[2];
            public UInt32 WindowSize;
            public UInt32 Msg;
            public UInt32 Rsp;
            public UInt32 Arg0;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwMailboxCommand
        {
            public UInt32 Opcode;
            public byte IsGet;
            public byte ResponseWords;
            public byte Flags;
            public byte Reserved;
            public fixed UInt32 ArgMask[MailboxMaxArgs];
            public fixed UInt32 ArgMax[MailboxMaxArgs];
            public fixed UInt32 ArgFixed[MailboxMaxArgs];
        }

        /// <summary>
        /// The KPH_DYN_HW_MAILBOX_TRANSACT header; commands follow. Address is the SMN member.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwMailboxTransact
        {
            public byte Transport;
            public byte ArgCount;
            public byte CommandCount;
            public byte Protocol;
            public UInt32 VersionOpcode;
            public UInt32 VersionArg0;
            public HwMailboxSmn Address;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwSmuTableVersion
        {
            public UInt32 Version;
            public UInt32 Size;
        }

        /// <summary>
        /// The KPH_DYN_HW_SMU_TABLE_READ header; versions follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwSmuTableRead
        {
            public byte MailboxOp;
            public byte Bound;
            public byte VersionCount;
            public byte SetupOpcode;
            public UInt32 VersionOpcode;
            public UInt32 TransferOpcode;
            public UInt32 BaseOpcode;
            public byte BaseLowWord;
            public byte BaseHighWord;
            public byte SizeWord;
            public byte DfMapOp;
        }

        /// <summary>
        /// The KPH_DYN_HW_DF_MAP body: where the DRAM address maps of an AMD Data Fabric function lie and how their
        /// fields decode.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwDfMap
        {
            public byte Function;
            public byte MapCount;
            public byte MapStride;
            public byte AddressShift;
            public UInt16 MapOffset;
            public UInt16 HoleOffset;
            public UInt32 AddressMask;
            public UInt32 HoleBaseMask;
            public byte LimitOffset;
            public byte ControlOffset;
            public byte ValidBit;
            public byte HoleRemapBit;
            public byte HoleValidBit;
            public byte Device;
            public fixed byte Reserved[2];
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwAcpiArg
        {
            public UInt32 Mask;
            public UInt32 Max;
            public UInt32 Fixed;
        }

        /// <summary>
        /// The KPH_DYN_HW_ACPI_EVAL header; arguments follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwAcpiEval
        {
            public UInt32 Method;
            public byte ArgCount;
            public byte ResultTypes;
            public UInt16 MaxOutput;
        }

        /// <summary>
        /// The KPH_DYN_HW_ACPI_REQUIRE header; IdCount IDs of AcpiIdSize bytes follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwAcpiRequire
        {
            public byte IdCount;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwSysregRead
        {
            public byte SysregEnum;
            public fixed byte Reserved[7];
            public UInt64 ReadMask;
        }

        /// <summary>
        /// The KPH_DYN_HW_PORT_INDIRECT_READ header; ranges follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwPortIndirectRead
        {
            public byte Reserved;
            public byte PinDevice;
            public byte PinFunction;
            public byte RangeCount;
            public UInt16 IndexPort;
            public UInt16 DataPort;
        }

        /// <summary>
        /// The KPH_DYN_HW_SMBUS_HOST body.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public unsafe struct HwSmbusHost
        {
            public byte PmAccess;
            public byte Flags;
            public byte PmLength;
            public byte DecodeLow;
            public byte DecodeHigh;
            public byte EnableMask;
            public UInt16 BaseMask;
            public byte Select;
            public byte SelectAlternate;
            public byte SelectChoose;
            public byte SelectChooseMask;
            public byte SelectMask;
            public byte SelectShift;
            public byte GateRegister;
            public byte GateMask;
            public UInt64 PmBase;
            public UInt16 IndexPort;
            public UInt16 DataPort;
            public fixed byte Reserved[4];
        }

        /// <summary>
        /// The KPH_DYN_HW_SECTION header; index and body area follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwSectionHeader
        {
            public UInt32 Magic;
            public UInt32 Length;
            public UInt32 Count;
            public UInt32 Reserved;
        }

        /// <summary>
        /// The KPH_DYN_HW_TABLE header; ops follow.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct HwTableHeader
        {
            public UInt32 Count;
            public UInt32 Reserved;
        }

        /// <summary>
        /// The hash of a BOARD row; must match KphpHwHashBoard and the KPH_DYN_HW_BOARD_IDENTITY rule in kphdyn.h.
        /// </summary>
        public static UInt64 BoardHash(string Manufacturer, string Product)
        {
            UInt64 hash = 0xcbf29ce484222325;

            void Mix(string Text)
            {
                foreach (char c in Text.Trim(' ', '\t'))
                {
                    byte b = (byte)c;

                    if (b >= (byte)'a' && b <= (byte)'z')
                        b -= 'a' - 'A';

                    hash = (hash ^ b) * 0x100000001b3;
                }
            }

            Mix(Manufacturer);
            hash = (hash ^ 0) * 0x100000001b3;
            Mix(Product);

            return hash;
        }

        /// <summary>
        /// Verifies the mirror layouts against the sizes asserted in kphdyn.h, and the board hash against the driver's.
        /// </summary>
        public static void VerifyLayouts()
        {
            // ToBytes and MemoryMarshal.Read use host order; the driver reads little endian.
            if (!BitConverter.IsLittleEndian)
                throw new InvalidOperationException("the hardware section is built in host byte order, which must be little endian");

            // Hyper-V Gen2 guest ("Microsoft Corporation" / "Virtual Machine"), as traced by the driver.
            if (BoardHash("Microsoft Corporation", "Virtual Machine") != 0xf2bf412e0c65e2e3)
                throw new InvalidOperationException("BoardHash differs from the driver's (KphpHwHashBoard)");

            CheckSize<HwCpuIdentity>(CpuIdentitySize);
            CheckSize<HwPciIdentity>(PciIdentitySize);
            CheckSize<HwCpuAndPciIdentity>(CpuAndPciIdentitySize);
            CheckSize<HwCpuAndPciClassIdentity>(CpuAndPciClassIdentitySize);
            CheckSize<HwSuperIoIdentity>(SuperIoIdentitySize);
            CheckSize<HwBoardIdentity>(BoardIdentitySize);
            CheckSize<HwAcpiHidIdentity>(AcpiHidIdentitySize);
            CheckSize<HwArmCpuIdentity>(ArmCpuIdentitySize);
            CheckSize<HwIdentity>(IdentitySize);
            CheckSize<HwIndex>(IndexSize);
            CheckSize<HwGate>(GateSize);
            CheckSize<HwOpHeader>(OpHeaderSize);
            CheckSize<HwMsrRead>(MsrReadSize);
            CheckSize<HwRange>(RangeSize);
            CheckSize<HwPciReadHeader>(PciReadHeaderSize);
            CheckSize<HwTargetRange>(TargetRangeSize);
            CheckSize<HwPciIndirectRead>(PciIndirectReadSize);
            CheckSize<HwWindowRange>(WindowRangeSize);
            CheckSize<HwWindowRead>(WindowReadSize);
            CheckSize<HwSuperIoRead>(SuperIoReadSize);
            CheckSize<HwSuperIoId>(SuperIoIdSize);

            if (Marshal.OffsetOf<HwSuperIoRead>("Ids").ToInt32() != SuperIoIdsOffset)
                throw new InvalidOperationException($"HwSuperIoRead.Ids is not at {SuperIoIdsOffset}");
            CheckSize<HwSmbusRead>(SmbusReadSize);
            CheckSize<HwVsecRead>(VsecReadSize);
            CheckSize<HwMailboxSmn>(MailboxAddressSize);
            CheckSize<HwMailboxWindow>(MailboxAddressSize);
            CheckSize<HwMailboxCommand>(MailboxCommandSize);
            CheckSize<HwMailboxTransact>(MailboxTransactSize);
            CheckSize<HwSmuTableVersion>(SmuTableVersionSize);
            CheckSize<HwSmuTableRead>(SmuTableReadSize);
            CheckSize<HwAcpiArg>(AcpiArgSize);
            CheckSize<HwAcpiEval>(AcpiEvalSize);
            CheckSize<HwAcpiRequire>(AcpiRequireSize);
            CheckSize<HwSysregRead>(SysregReadSize);
            CheckSize<HwPortIndirectRead>(PortIndirectReadSize);
            CheckSize<HwSmbusHost>(SmbusHostSize);
            CheckSize<HwDfMap>(DfMapSize);
            CheckSize<HwSectionHeader>(SectionSize);
            CheckSize<HwTableHeader>(TableSize);

            foreach (var (name, mirror, field, offset, actual) in FieldOffsets)
            {
                int value = actual();

                if (value != offset)
                    throw new InvalidOperationException($"{mirror}.{field} offset {value} != {offset} ({name})");
            }
        }

        private static void CheckSize<T>(int Expected) where T : struct
        {
            int size = Unsafe.SizeOf<T>();

            if (size != Expected)
                throw new InvalidOperationException($"{typeof(T).Name} size {size} != {Expected}");
        }

        public static byte[] ToBytes<T>(T Value) where T : unmanaged
        {
            return MemoryMarshal.AsBytes(MemoryMarshal.CreateReadOnlySpan(ref Value, 1)).ToArray();
        }
    }

    /// <summary>
    /// The kinds, families, transports, shapes, registers and port pairs a driver implements.
    /// </summary>
    public sealed record HwKindSet(UInt16[] OpKinds, byte[] IdentityKinds, byte[] GateKinds, byte[] WindowBaseKinds, byte[] SuperIoFamilies, byte[] SmbusHosts, byte[] VsecDiscoveries, byte[] MailboxTransports, byte[] MailboxProtocolKinds, byte[] SmuTableBounds, byte[] SysregRegisters);

    /// <summary>
    /// An index row: an exact identity and its op table.
    /// </summary>
    public sealed class HwRow
    {
        public byte IdentityKind;
        public byte[] Identity;
        /// <summary>
        /// Count followed by the ops.
        /// </summary>
        public byte[] Table;
    }

    /// <summary>
    /// Parses and checks kphdynhw.xml into index rows; the checks catch authoring mistakes, they are not policy.
    /// </summary>
    public static class HwManifest
    {
        private static readonly string[] CpuAttributes = ["vendor", "family", "models", "name", "desc"];
        private static readonly string[] MsrAttributes = ["reg", "gate", "readmask", "desc"];
        private static readonly string[] PciAttributes = ["vendor", "device", "pin", "name", "desc"];
        private static readonly string[] PciClassAttributes = ["vendor", "class", "header", "pin", "source", "name", "desc"];
        private static readonly string[] ReadAttributes = ["offset", "length", "desc"];
        private static readonly string[] IndirectAttributes = ["index", "data", "desc"];
        private static readonly string[] TargetAttributes = ["begin", "end", "stride", "count", "desc"];
        private static readonly string[] WindowAttributes = ["base", "bar", "offset", "width", "mask", "enable", "size", "require-d0", "arm64", "resource", "entry", "lo", "hi", "address", "port", "present", "signature", "desc"];
        private static readonly string[] WindowReadAttributes = ["offset", "length", "width", "desc"];
        private static readonly string[] SuperIoAttributes = ["slots", "family", "chips", "mask", "customers", "ldn", "exit", "base", "chassis", "port", "bank-mask",
                                                              "config-id", "customer-reg", "require-active", "name", "desc"];
        private static readonly string[] SuperIoIdAttributes = ["reg", "value", "alt", "desc"];
        private static readonly string[] SuperIoReadAttributes = ["reg", "length", "desc"];
        private static readonly string[] SmbusAttributes = ["host", "port", "protocols", "slaves", "name", "desc"];
        private static readonly string[] SmbusReadAttributes = ["command", "length", "desc"];
        private static readonly string[] VsecAttributes = ["discovery", "quirks", "guid", "feature", "max-regions", "max-size", "revision", "dvsec-vendor", "require-d0", "arm64", "name", "desc"];
        private static readonly string[] MailboxAttributes = ["transport", "protocol", "index", "data", "msg", "rsp", "arg", "args", "version", "version-arg", "name", "desc"];
        private static readonly string[] MmioMailboxAttributes = ["transport", "protocol", "resource", "size", "msg", "rsp", "arg", "args", "version", "version-arg", "cpu-family", "cpu-models", "name", "desc"];
        private static readonly string[] CommandAttributes = ["opcode", "type", "response", "internal", "name", "desc"];
        private static readonly string[] ArgAttributes = ["index", "mask", "max", "fixed", "desc"];
        private static readonly string[] SmuTableAttributes = ["mailbox", "bound", "dfmap", "version", "transfer", "setup", "base", "base-low", "base-high", "size-word", "name", "desc"];
        private static readonly string[] DfMapAttributes = ["name", "device", "function", "offset", "count", "stride", "limit", "control", "shift", "mask", "valid", "remap",
                                                            "hole", "hole-mask", "hole-valid", "desc"];
        private static readonly string[] SmuTableVersionAttributes = ["id", "size", "desc"];
        private static readonly string[] AcpiAttributes = ["id", "name", "desc"];
        private static readonly string[] EvalAttributes = ["method", "results", "max-output", "desc"];
        private static readonly string[] RequireAttributes = ["id", "desc"];
        private static readonly string[] ArmAttributes = ["implementer", "parts", "variants", "revisions", "name", "desc"];
        private static readonly string[] SysregAttributes = ["reg", "readmask", "gate", "desc"];
        private static readonly string[] BoardAttributes = ["smbios", "manufacturer", "product", "name", "desc"];
        private static readonly string[] PortIndirectAttributes = ["index-port", "data-port", "name", "desc"];
        private static readonly string[] SmbusHostAttributes = ["revisions", "pm", "index-port", "data-port", "base", "length", "enable", "decode", "base-mask",
                                                                "enable-mask", "select", "select-mask", "select-alternate", "select-choose", "select-write",
                                                                "semaphore", "name", "desc"];
        private static readonly string[] PortIndirectReadAttributes = ["index", "length", "desc"];

        private enum WindowPlacement
        {
            Pci,
            RootPci,
            Acpi,
            Cpu,
            Board
        }

        /// <summary>
        /// Loads the hardware manifests as one.
        /// </summary>
        /// <param name="ManifestFiles">The kphdynhw.xml paths.</param>
        /// <returns>The index rows, in document order.</returns>
        public static List<HwRow> Load(params string[] ManifestFiles)
        {
            var context = new LoadContext();

            foreach (string manifestFile in ManifestFiles)
            {
                XDocument xml;
                var xmlSettings = new XmlReaderSettings
                {
                    DtdProcessing = DtdProcessing.Prohibit,
                    IgnoreWhitespace = true
                };
                using (var xmlReader = XmlReader.Create(manifestFile, xmlSettings))
                {
                    xml = XDocument.Load(xmlReader, LoadOptions.SetLineInfo);
                }

                context.FileName = Path.GetFileName(manifestFile);

                var root = xml.Root;

                if (root.Name != "hw")
                    throw context.Error(root, "root element must be <hw>");

                CheckAttributes(context, root, ["desc"]);

                foreach (var element in root.Elements())
                {
                    switch (element.Name.LocalName)
                    {
                    case "cpu":
                        LoadCpu(context, element);
                        break;
                    case "pci":
                        {
                            var pci = LoadPci(context, element, null, false);

                            foreach (UInt16 deviceId in pci.DeviceIds)
                            {
                                context.AddRow(element, DynDataHw.IdentityPci, PciIdentity(pci.VendorId, deviceId), pci.Table);
                                context.AddTopLevelPci(element, pci.VendorId, deviceId);
                            }
                        }
                        break;
                    case "pciclass":
                        throw context.Error(element, "<pciclass> is only allowed nested in a <cpu>");
                    case "superio":
                        LoadSuperIo(context, element);
                        break;
                    case "acpi":
                        LoadAcpi(context, element);
                        break;
                    case "arm":
                        LoadArm(context, element);
                        break;
                    case "board":
                        LoadBoard(context, element);
                        break;
                    default:
                        throw context.Error(element, "unknown element");
                    }
                }
            }

            context.CheckSources();
            context.CheckShadowing();

            if (context.Rows.Count > DynDataHw.MaxRows)
                throw new InvalidDataException($"{string.Join(", ", ManifestFiles.Select(Path.GetFileName))}: {context.Rows.Count} rows exceeds the cap of {DynDataHw.MaxRows}");

            return context.Rows;
        }

        /// <summary>
        /// Loads a cpu bucket: one CPU row per model, one CPU_AND_PCI row per model and nested pci, one CPU_AND_PCI_CLASS row per model and nested pciclass.
        /// </summary>
        /// <remarks>A pciclass table must be a subset of every pinned pci table on its pin in the bucket, or with source= equal those of the named bucket.</remarks>
        private static void LoadCpu(LoadContext Context, XElement Cpu)
        {
            CheckAttributes(Context, Cpu, CpuAttributes);

            string vendor = RequiredAttribute(Context, Cpu, "vendor");

            if (vendor.Length != 12 || !vendor.All(c => c >= 0x20 && c <= 0x7E))
                throw Context.Error(Cpu, $"vendor \"{vendor}\" is not 12 printable ASCII characters");

            UInt16 family = (UInt16)ParseHex(Context, Cpu, "family", RequiredAttribute(Context, Cpu, "family"), UInt16.MaxValue);
            List<byte> models = ParseModels(Context, Cpu);
            var scope = new CpuScope(vendor, family, models);
            var msrOps = new List<byte[]>();
            var msrRegisters = new HashSet<UInt32>();
            var pcis = new List<(XElement Element, LoadedPci Pci)>();

            foreach (var element in Cpu.Elements())
            {
                switch (element.Name.LocalName)
                {
                case "msr":
                    {
                        CheckAttributes(Context, element, MsrAttributes);
                        CheckNoChildren(Context, element);

                        var msr = new DynDataHw.HwMsrRead
                        {
                            Register = (UInt32)ParseHex(Context, element, "reg", RequiredAttribute(Context, element, "reg"), UInt32.MaxValue),
                            ReadMask = UInt64.MaxValue
                        };

                        if (!msrRegisters.Add(msr.Register))
                            throw Context.Error(element, $"duplicate msr reg 0x{msr.Register:x} in this table");

                        string readMask = OptionalAttribute(element, "readmask");

                        if (readMask != null)
                        {
                            msr.ReadMask = ParseHex(Context, element, "readmask", readMask, UInt64.MaxValue);

                            if (msr.ReadMask == 0)
                                throw Context.Error(element, "readmask 0x0 reads nothing");
                        }

                        msrOps.Add(BuildOp(
                            Context,
                            element,
                            DynDataHw.KindMsrRead,
                            DynDataHw.LockNone,
                            ParseGate(Context, element, false),
                            DynDataHw.ToBytes(msr)
                            ));
                    }
                    break;
                case "pci":
                    {
                        var pci = LoadPci(Context, element, scope, false);

                        var duplicate = pcis.Where(p => !p.Pci.Class && p.Pci.VendorId == pci.VendorId).SelectMany(p => p.Pci.DeviceIds).Intersect(pci.DeviceIds).ToList();

                        if (duplicate.Count != 0)
                            throw Context.Error(element, $"duplicate pci 0x{pci.VendorId:x4}:0x{duplicate[0]:x4} in this <cpu>");

                        if (pci.PciRowOps)
                        {
                            foreach (UInt16 deviceId in pci.DeviceIds)
                                Context.AddNestedPci(element, pci.VendorId, deviceId);
                        }

                        pcis.Add((element, pci));
                    }
                    break;
                case "window":
                    {
                        //
                        // Intel PMC windows come from the LPIT or a board row only: PWRMBASE is firmware-relocatable.
                        //
                        if (OptionalAttribute(element, "base") == "fixed" && vendor != "AuthenticAMD")
                            throw Context.Error(element, "base=\"fixed\" in a <cpu> is for AuthenticAMD (FCH) only; other platform windows belong to a <board>");
                        if (OptionalAttribute(element, "base") == "lpit-relative" && vendor != "GenuineIntel")
                            throw Context.Error(element, "base=\"lpit-relative\" is for GenuineIntel (the PMC PWRM block) only");

                        msrOps.Add(BuildWindowOp(Context, element, WindowPlacement.Cpu));
                    }
                    break;
                case "board":
                    throw Context.Error(element, "<board> is only allowed at the top level");
                case "sysreg":
                    throw Context.Error(element, "<sysreg> is only allowed in an <arm>");
                case "pciclass":
                    {
                        var pci = LoadPci(Context, element, scope, true);

                        if (pcis.Any(p => p.Pci.Class && p.Pci.VendorId == pci.VendorId && p.Pci.ClassCode == pci.ClassCode && p.Pci.HeaderType == pci.HeaderType && p.Pci.DevFn == pci.DevFn))
                            throw Context.Error(element, $"duplicate pciclass 0x{pci.VendorId:x4} class 0x{pci.ClassCode:x6} in this <cpu>");

                        pcis.Add((element, pci));
                    }
                    break;
                default:
                    throw Context.Error(element, "unknown element");
                }
            }

            if (msrOps.Count == 0 && pcis.Count == 0)
                throw Context.Error(Cpu, "<cpu> has no <msr>, no <window>, no <pci> and no <pciclass>");

            Context.AddBucket(Cpu, pcis.Where(p => !p.Pci.Class && p.Pci.Pinned).Select(p => (p.Pci.DevFn, p.Pci.CompareTable)));

            foreach (var (element, pci) in pcis.Where(p => p.Pci.Class))
            {
                var exact = pcis.Where(p => !p.Pci.Class && p.Pci.Pinned && p.Pci.DevFn == pci.DevFn).ToList();

                if (exact.Count == 0)
                {
                    if (pci.Source == null)
                        throw Context.Error(element, "a <pciclass> without a pinned <pci> on its pin in this <cpu> needs source=\"<bucket>\"");

                    Context.AddSource(element, pci.Source, pci.DevFn, pci.CompareTable);
                    continue;
                }

                if (pci.Source != null)
                    throw Context.Error(element, "source= is only for a <pciclass> without a pinned <pci> on its pin in this <cpu>");

                foreach (var (exactElement, exactPci) in exact)
                {
                    if (!HwTableSubset.IsSubset(pci.CompareTable, exactPci.CompareTable))
                        throw Context.Error(element, $"table is not equal to or a subset of the table of <pci> 0x{exactPci.VendorId:x4}:0x{exactPci.DeviceIds[0]:x4} (line {((IXmlLineInfo)exactElement).LineNumber})");
                }
            }

            //
            // Across the tables of one CPU identity, no table may expose an internal opcode or let an
            // indirect target land on mailbox registers.
            //
            foreach (var (element, pci) in pcis)
            {
                var ops = SplitTable(pci.Table);

                if (!ops.Any(op => BinaryPrimitives.ReadUInt16LittleEndian(op) == DynDataHw.KindMailboxTransact))
                    continue;

                var combined = new List<byte[]>();

                foreach (var (_, other) in pcis)
                {
                    foreach (var op in SplitTable(other.Table))
                    {
                        if ((other.Pinned && other.DevFn == pci.DevFn) ||
                            BinaryPrimitives.ReadUInt16LittleEndian(op) == DynDataHw.KindPciIndirectRead)
                        {
                            combined.Add(op);
                        }
                    }
                }

                try
                {
                    HwSection.CheckMailboxRefs(BuildTable(Context, element, combined), -1);
                }
                catch (InvalidDataException e)
                {
                    throw Context.Error(element, $"across the tables of this <cpu>: {e.Message}");
                }
            }

            //
            // Across the tables of one CPU identity, no indirect target may read an SMN aperture base register.
            //
            foreach (var (element, pci) in pcis)
            {
                var ops = SplitTable(pci.Table);

                if (!ops.Any(op => BinaryPrimitives.ReadUInt16LittleEndian(op) == DynDataHw.KindWindowRead && op[DynDataHw.OpHeaderSize] == DynDataHw.WindowBaseSmnDerived))
                    continue;

                var combined = new List<byte[]>(ops);

                foreach (var (_, other) in pcis)
                {
                    if (other == pci)
                        continue;

                    foreach (var op in SplitTable(other.Table))
                    {
                        if (BinaryPrimitives.ReadUInt16LittleEndian(op) == DynDataHw.KindPciIndirectRead)
                            combined.Add(op);
                    }
                }

                try
                {
                    HwSection.CheckWindowRefs(BuildTable(Context, element, combined), -1);
                }
                catch (InvalidDataException e)
                {
                    throw Context.Error(element, $"across the tables of this <cpu>: {e.Message}");
                }
            }

            byte[] cpuTable = msrOps.Count != 0 ? BuildTable(Context, Cpu, msrOps) : null;

            foreach (byte model in models)
            {
                Context.AddModel(Cpu, vendor, family, model);

                byte[] cpuIdentity = CpuIdentity(vendor, family, model);

                if (cpuTable != null)
                    Context.AddRow(Cpu, DynDataHw.IdentityCpu, cpuIdentity, cpuTable);

                foreach (var (element, pci) in pcis)
                {
                    byte[] identity;

                    if (pci.Class)
                    {
                        identity = PciClassIdentity(pci);
                        cpuIdentity.AsSpan(0, DynDataHw.CpuIdentitySize).CopyTo(identity);

                        Context.AddRow(element, DynDataHw.IdentityCpuAndPciClass, identity, pci.Table);
                    }
                    else
                    {
                        foreach (UInt16 deviceId in pci.DeviceIds)
                        {
                            identity = new byte[DynDataHw.IdentitySize];
                            cpuIdentity.AsSpan(0, DynDataHw.CpuIdentitySize).CopyTo(identity);
                            PciIdentity(pci.VendorId, deviceId).AsSpan(0, DynDataHw.PciIdentitySize).CopyTo(identity.AsSpan(DynDataHw.CpuIdentitySize));

                            Context.AddRow(element, DynDataHw.IdentityCpuAndPci, identity, pci.Table);
                        }
                    }
                }
            }
        }

        /// <summary>
        /// The cpu element a pci or pciclass is nested in.
        /// </summary>
        private sealed record CpuScope(string Vendor, UInt16 Family, List<byte> Models);

        private sealed class LoadedPci
        {
            public bool Class;
            public UInt16 VendorId;
            public List<UInt16> DeviceIds = [];
            /// <summary>
            /// (base &lt;&lt; 16) | (subclass &lt;&lt; 8) | programming interface.
            /// </summary>
            public UInt32 ClassCode;
            public byte HeaderType;
            public byte DevFn;
            public bool Pinned;
            /// <summary>
            /// The bucket a pciclass copies its table from, or null.
            /// </summary>
            public string Source;
            public byte[] Table;
            /// <summary>
            /// The table without its per-codename ops (mailbox, smu_table_read, port_indirect_read, SMN aperture windows).
            /// </summary>
            public byte[] CompareTable;
            /// <summary>
            /// The table holds ops the driver looks up with the PCI row first (window_read, smbus_read, vsec_read).
            /// </summary>
            public bool PciRowOps;
        }

        /// <summary>
        /// Loads a pci or pciclass element: all reads form one pci_read op, an indirect (nested only) one pci_indirect_read op.
        /// </summary>
        /// <param name="Cpu">The enclosing cpu, null at the top level.</param>
        private static LoadedPci LoadPci(LoadContext Context, XElement Pci, CpuScope Cpu, bool Class)
        {
            CheckAttributes(Context, Pci, Class ? PciClassAttributes : PciAttributes);

            bool nested = Cpu != null;

            var result = new LoadedPci
            {
                Class = Class,
                VendorId = (UInt16)ParseHex(Context, Pci, "vendor", RequiredAttribute(Context, Pci, "vendor"), UInt16.MaxValue)
            };

            if (Class)
            {
                var (device, function) = ParsePin(Context, Pci, RequiredAttribute(Context, Pci, "pin"));

                result.ClassCode = (UInt32)ParseHex(Context, Pci, "class", RequiredAttribute(Context, Pci, "class"), 0xFFFFFF);
                result.HeaderType = (byte)ParseHex(Context, Pci, "header", RequiredAttribute(Context, Pci, "header"), DynDataHw.PciHeaderLayoutMask);
                result.DevFn = (byte)((device << 3) | function);
                result.Pinned = true;
                result.Source = OptionalAttribute(Pci, "source");
            }
            else
            {
                string pin = OptionalAttribute(Pci, "pin");

                foreach (string item in RequiredAttribute(Context, Pci, "device").Split(','))
                {
                    UInt16 deviceId = (UInt16)ParseHex(Context, Pci, "device", item.Trim(), UInt16.MaxValue);

                    if (result.DeviceIds.Contains(deviceId))
                        throw Context.Error(Pci, $"duplicate device 0x{deviceId:x4} in device");

                    result.DeviceIds.Add(deviceId);
                }

                if (pin != null)
                {
                    var (device, function) = ParsePin(Context, Pci, pin);

                    result.DevFn = (byte)((device << 3) | function);
                    result.Pinned = true;
                }
            }

            var ranges = new List<DynDataHw.HwRange>();
            var windows = new List<XElement>();
            var smnWindows = new List<XElement>();
            var smbuses = new List<XElement>();
            var smbusHosts = new List<XElement>();
            var vsecs = new List<XElement>();
            var mailboxes = new List<XElement>();
            var smuTables = new List<XElement>();
            var dfMaps = new List<XElement>();
            var portIndirects = new List<XElement>();
            XElement firstRead = null;
            XElement indirect = null;

            foreach (var element in Pci.Elements())
            {
                switch (element.Name.LocalName)
                {
                case "read":
                    {
                        CheckAttributes(Context, element, ReadAttributes);
                        CheckNoChildren(Context, element);

                        UInt64 offset = ParseHex(Context, element, "offset", RequiredAttribute(Context, element, "offset"), 0xFFF);
                        UInt64 length = ParseHex(Context, element, "length", RequiredAttribute(Context, element, "length"), DynDataHw.PciConfigLength);

                        if (length == 0)
                            throw Context.Error(element, "length is zero");
                        if (offset + length > DynDataHw.PciConfigLength)
                            throw Context.Error(element, $"offset 0x{offset:x} + length 0x{length:x} exceeds 0x{DynDataHw.PciConfigLength:x}");

                        firstRead ??= element;
                        ranges.Add(new DynDataHw.HwRange { Offset = (UInt16)offset, Length = (UInt16)length });
                    }
                    break;
                case "indirect":
                    {
                        if (!nested)
                            throw Context.Error(element, "<indirect> is only allowed in a <pci> or <pciclass> nested in a <cpu>");
                        if (indirect != null)
                            throw Context.Error(element, "more than one <indirect> in this <pci>");

                        indirect = element;
                    }
                    break;
                case "window":
                    if (OptionalAttribute(element, "base") == "smn")
                        smnWindows.Add(element);
                    else
                        windows.Add(element);
                    break;
                case "portindirect":
                    portIndirects.Add(element);
                    break;
                case "smbus":
                    smbuses.Add(element);
                    break;
                case "smbus-host":
                    smbusHosts.Add(element);
                    break;
                case "vsec":
                    vsecs.Add(element);
                    break;
                case "mailbox":
                    mailboxes.Add(element);
                    break;
                case "smutable":
                    smuTables.Add(element);
                    break;
                case "dfmap":
                    dfMaps.Add(element);
                    break;
                default:
                    throw Context.Error(element, "unknown element");
                }
            }

            if (firstRead == null && indirect == null && windows.Count == 0 && smnWindows.Count == 0 && smbuses.Count == 0 && vsecs.Count == 0 &&
                mailboxes.Count == 0 && portIndirects.Count == 0)
            {
                throw Context.Error(Pci, $"<{Pci.Name}> has no <read>, no <indirect>, no <window>, no <smbus>, no <vsec>, no <mailbox> and no <portindirect>");
            }

            //
            // An SMN aperture base is read through the element's own index/data pair.
            //
            if (smnWindows.Count != 0 && (!nested || indirect == null))
                throw Context.Error(smnWindows[0], "base=\"smn\" is only allowed in a <pci> or <pciclass> nested in a <cpu> that has an <indirect>");

            result.PciRowOps = windows.Count != 0 || smnWindows.Count != 0 || smbuses.Count != 0 || smbusHosts.Count != 0 || vsecs.Count != 0;

            byte[] readOp = null;
            byte[] indirectOp = null;

            if (firstRead != null)
            {
                if (ranges.Count > DynDataHw.MaxRanges)
                    throw Context.Error(Pci, $"{ranges.Count} <read> ranges exceeds the cap of {DynDataHw.MaxRanges}");

                var body = new List<byte>();

                body.AddRange(DynDataHw.ToBytes(new DynDataHw.HwPciReadHeader { RangeCount = (byte)ranges.Count }));
                foreach (var range in ranges)
                    body.AddRange(DynDataHw.ToBytes(range));

                readOp = BuildOp(Context, firstRead, DynDataHw.KindPciRead, DynDataHw.LockNone, default, body.ToArray());
            }

            if (indirect != null)
            {
                CheckSmnRoot(Context, indirect, result, Cpu);

                indirectOp = BuildIndirectOp(Context, Pci, indirect);
            }

            var anchored = new List<(XElement Anchor, byte[] Op)>();

            if (readOp != null)
                anchored.Add((firstRead, readOp));
            if (indirectOp != null)
                anchored.Add((indirect, indirectOp));
            foreach (var window in windows)
                anchored.Add((window, BuildWindowOp(Context, window, nested ? WindowPlacement.RootPci : WindowPlacement.Pci)));
            foreach (var smbus in smbuses)
                anchored.Add((smbus, BuildSmbusOp(Context, result, nested, smbus)));

            //
            // A PIIX4 host is used only in the revision ranges its smbus-host elements describe.
            //
            bool piix4 = smbuses.Any(smbus => OptionalAttribute(smbus, "host") == "piix4");

            if (piix4 != (smbusHosts.Count != 0))
                throw Context.Error(Pci, piix4 ? "host=\"piix4\" needs an <smbus-host> (the PM layout of a revision range)" : "<smbus-host> is only for a <pci> with host=\"piix4\" <smbus>");

            var hostRevisions = new List<(UInt32 Low, UInt32 High)>();

            foreach (var smbusHost in smbusHosts)
            {
                byte[] op = BuildSmbusHostOp(Context, result, nested, smbusHost, out UInt32 low, out UInt32 high);

                foreach (var (otherLow, otherHigh) in hostRevisions)
                {
                    if (low <= otherHigh && otherLow <= high)
                        throw Context.Error(smbusHost, $"revisions 0x{low:x2}-0x{high:x2} overlap 0x{otherLow:x2}-0x{otherHigh:x2} of another <smbus-host>");
                }

                hostRevisions.Add((low, high));
                anchored.Add((smbusHost, op));
            }

            if (smbusHosts.Any(host => OptionalAttribute(host, "pm") == "config"))
            {
                foreach (var smbus in smbuses)
                {
                    string port = OptionalAttribute(smbus, "port");

                    if (port != null && ParseDecimal(Context, smbus, "port", port, 0, DynDataHw.SmbusPorts - 1u) != 0)
                        throw Context.Error(smbus, "a <pci> with a pm=\"config\" <smbus-host> has one port; port must be 0");
                }
            }
            foreach (var vsec in vsecs)
                anchored.Add((vsec, BuildVsecOp(Context, result, vsec)));

            if (vsecs.Count != 0)
            {
                UInt64 vsecRegions = 0;

                foreach (var vsec in vsecs)
                    vsecRegions += ParseDecimal(Context, vsec, "max-regions", RequiredAttribute(Context, vsec, "max-regions"), 1, DynDataHw.VsecMaxRegions);

                if (vsecRegions > DynDataHw.VsecMaxTableRegions)
                    throw Context.Error(Pci, $"<vsec> max-regions total {vsecRegions} exceeds the cap of {DynDataHw.VsecMaxTableRegions} per table");
            }

            var compared = anchored.ToList();
            var loadedMailboxes = new Dictionary<string, LoadedMailbox>();

            foreach (var window in smnWindows)
                anchored.Add((window, BuildWindowOp(Context, window, WindowPlacement.RootPci)));
            foreach (var portIndirect in portIndirects)
                anchored.Add((portIndirect, BuildPortIndirectOp(Context, result, nested, portIndirect)));

            for (int i = 0; i < mailboxes.Count; i++)
            {
                var mailbox = LoadMailbox(Context, result, Cpu, mailboxes[i], i);

                if (mailbox.Name != null && !loadedMailboxes.TryAdd(mailbox.Name, mailbox))
                    throw Context.Error(mailboxes[i], $"duplicate <mailbox> name \"{mailbox.Name}\"");

                anchored.Add((mailboxes[i], mailbox.Op));
            }

            if (mailboxes.Count > byte.MaxValue + 1)
                throw Context.Error(Pci, $"{mailboxes.Count} <mailbox> exceeds 256");

            var loadedDfMaps = new Dictionary<string, int>();

            for (int i = 0; i < dfMaps.Count; i++)
            {
                string name = RequiredAttribute(Context, dfMaps[i], "name");

                if (!loadedDfMaps.TryAdd(name, i))
                    throw Context.Error(dfMaps[i], $"duplicate <dfmap> name \"{name}\"");

                anchored.Add((dfMaps[i], BuildDfMapOp(Context, result, nested, dfMaps[i])));
            }

            if (dfMaps.Count > DynDataHw.SmuTableDfMapNone)
                throw Context.Error(Pci, $"{dfMaps.Count} <dfmap> exceeds {DynDataHw.SmuTableDfMapNone}");

            foreach (var smuTable in smuTables)
                anchored.Add((smuTable, BuildSmuTableOp(Context, loadedMailboxes, loadedDfMaps, smuTable)));

            //
            // An op's index among the ops of its kind in the table is the document order of its element.
            //
            anchored.Sort((a, b) => XNode.DocumentOrderComparer.Compare(a.Anchor, b.Anchor));
            compared.Sort((a, b) => XNode.DocumentOrderComparer.Compare(a.Anchor, b.Anchor));

            result.Table = BuildTable(Context, Pci, anchored.Select(a => a.Op).ToList());
            result.CompareTable = BuildTable(Context, Pci, compared.Select(a => a.Op).ToList());

            return result;
        }

        private sealed class LoadedMailbox
        {
            public string Name;
            public int Index;
            public byte ArgCount;
            public List<(UInt32 Opcode, byte ResponseWords, bool Internal)> Commands = [];
            public byte[] Op;
        }

        /// <summary>
        /// An SMN pair is written only on an AMD or Hygon host bridge: a root complex vendor, function 0.0 and, for a pciclass, the host bridge class.
        /// </summary>
        private static void CheckSmnRoot(LoadContext Context, XElement Element, LoadedPci Loaded, CpuScope Cpu)
        {
            if (!DynDataHw.SmnCpuVendors.Contains(Cpu.Vendor))
                throw Context.Error(Element, $"<{Element.Name}> is only allowed in an AuthenticAMD or HygonGenuine <cpu>, not {Cpu.Vendor}");
            if (!DynDataHw.SmnRootVendors.Contains(Loaded.VendorId))
                throw Context.Error(Element, $"<{Element.Name}> is only allowed on a root complex of vendor 0x1022 or 0x1d94, not 0x{Loaded.VendorId:x4}");
            if (!Loaded.Pinned || Loaded.DevFn != 0)
                throw Context.Error(Element, $"<{Element.Name}> is only allowed on the host bridge, pin=\"0.0\"");
            if (Loaded.Class && Loaded.ClassCode != DynDataHw.HostBridgeClass)
                throw Context.Error(Element, $"<{Element.Name}> in a <pciclass> needs class=\"0x{DynDataHw.HostBridgeClass:x6}\" (host bridge), not 0x{Loaded.ClassCode:x6}");
        }

        private static DynDataHw.MailboxProtocol ParseMailboxProtocol(LoadContext Context, XElement Mailbox)
        {
            string name = RequiredAttribute(Context, Mailbox, "protocol");

            return DynDataHw.MailboxProtocols.FirstOrDefault(p => p.Name == name) ??
                throw Context.Error(Mailbox, $"protocol \"{name}\" is not one of {string.Join(", ", DynDataHw.MailboxProtocols.Select(p => $"\"{p.Name}\""))} (DynDataHw.MailboxProtocols)");
        }

        /// <summary>
        /// Builds the mailbox_transact op of an SMN mailbox: every command is a GET of its protocol.
        /// </summary>
        private static LoadedMailbox LoadMailbox(LoadContext Context, LoadedPci Loaded, CpuScope Cpu, XElement Mailbox, int Index)
        {
            CheckAttributes(Context, Mailbox, MailboxAttributes);

            if (Cpu == null || !Loaded.Pinned)
                throw Context.Error(Mailbox, "<mailbox> is only allowed in a pinned <pci> or <pciclass> nested in a <cpu> (the root complex)");
            if (RequiredAttribute(Context, Mailbox, "transport") != "smn")
                throw Context.Error(Mailbox, "transport must be \"smn\" in a <pci> (\"mmio\" belongs to an <acpi>; the other transports are not implemented)");

            CheckSmnRoot(Context, Mailbox, Loaded, Cpu);

            var protocol = ParseMailboxProtocol(Context, Mailbox);

            var result = new LoadedMailbox { Name = OptionalAttribute(Mailbox, "name"), Index = Index };
            UInt16 indexOffset = (UInt16)ParseHex(Context, Mailbox, "index", RequiredAttribute(Context, Mailbox, "index"), DynDataHw.PciConfigLength - 1);
            UInt16 dataOffset = (UInt16)ParseHex(Context, Mailbox, "data", RequiredAttribute(Context, Mailbox, "data"), DynDataHw.PciConfigLength - 1);
            UInt32 msg = (UInt32)ParseHex(Context, Mailbox, "msg", RequiredAttribute(Context, Mailbox, "msg"), UInt32.MaxValue);
            UInt32 rsp = (UInt32)ParseHex(Context, Mailbox, "rsp", RequiredAttribute(Context, Mailbox, "rsp"), UInt32.MaxValue);
            UInt32 arg = (UInt32)ParseHex(Context, Mailbox, "arg", RequiredAttribute(Context, Mailbox, "arg"), UInt32.MaxValue);
            byte argCount = (byte)ParseDecimal(Context, Mailbox, "args", RequiredAttribute(Context, Mailbox, "args"), 1, DynDataHw.MailboxMaxArgs);
            UInt32 versionOpcode = (UInt32)ParseHex(Context, Mailbox, "version", RequiredAttribute(Context, Mailbox, "version"), DynDataHw.MailboxMaxOpcode);
            string versionArg = OptionalAttribute(Mailbox, "version-arg");
            UInt64 argEnd = (UInt64)arg + 4ul * argCount;

            if ((indexOffset % 4) != 0 || (dataOffset % 4) != 0)
                throw Context.Error(Mailbox, $"index 0x{indexOffset:x} / data 0x{dataOffset:x} not 4-aligned");
            if (indexOffset < DynDataHw.PciConfigHeaderLength || dataOffset < DynDataHw.PciConfigHeaderLength)
                throw Context.Error(Mailbox, $"index 0x{indexOffset:x} / data 0x{dataOffset:x} inside the 0x{DynDataHw.PciConfigHeaderLength:x}-byte config header");
            if (indexOffset == dataOffset)
                throw Context.Error(Mailbox, $"index and data are both 0x{indexOffset:x}");
            if ((msg % 4) != 0 || (rsp % 4) != 0 || (arg % 4) != 0)
                throw Context.Error(Mailbox, "msg, rsp and arg must be 4-aligned");
            if (argEnd > UInt32.MaxValue + 1ul)
                throw Context.Error(Mailbox, $"arg 0x{arg:x} + {argCount} registers overflows 32 bits");
            if (msg == rsp || (msg >= arg && msg < argEnd) || (rsp >= arg && rsp < argEnd))
                throw Context.Error(Mailbox, "msg, rsp and the argument registers overlap");
            if (versionOpcode == 0)
                throw Context.Error(Mailbox, "version opcode 0x0");
            if (!protocol.Smn.Contains(new DynDataHw.MailboxRegisters(indexOffset, dataOffset, msg, rsp, arg, argCount)))
                throw Context.Error(Mailbox, $"index 0x{indexOffset:x} data 0x{dataOffset:x} msg 0x{msg:x} rsp 0x{rsp:x} arg 0x{arg:x} args {argCount} is not a register set of protocol \"{protocol.Name}\"");

            foreach (byte model in Cpu.Models)
            {
                if (!DynDataHw.MailboxProtocolCovers(protocol, Cpu.Family, model, model))
                    throw Context.Error(Mailbox, $"protocol \"{protocol.Name}\" is not the opcode map of family 0x{Cpu.Family:x} model 0x{model:x2}");
            }

            var body = new DynDataHw.HwMailboxTransact
            {
                Transport = DynDataHw.MailboxTransportSmnPair,
                ArgCount = argCount,
                Protocol = protocol.Kind,
                VersionOpcode = versionOpcode,
                VersionArg0 = versionArg != null ? (UInt32)ParseHex(Context, Mailbox, "version-arg", versionArg, UInt32.MaxValue) : 0,
                Address = new DynDataHw.HwMailboxSmn
                {
                    IndexOffset = indexOffset,
                    DataOffset = dataOffset,
                    PinDevice = (byte)(Loaded.DevFn >> 3),
                    PinFunction = (byte)(Loaded.DevFn & 7),
                    Msg = msg,
                    Rsp = rsp,
                    Arg0 = arg
                }
            };

            byte[] commands = LoadMailboxCommands(Context, Mailbox, protocol, argCount, true, result);

            body.CommandCount = (byte)result.Commands.Count;
            result.ArgCount = argCount;
            result.Op = BuildOp(Context, Mailbox, DynDataHw.KindMailboxTransact, DynDataHw.LockPci, default, [.. DynDataHw.ToBytes(body), .. commands]);

            string problem = DynDataHw.MailboxVersionProblem(result.Op.AsSpan(DynDataHw.OpHeaderSize));

            if (problem != null)
                throw Context.Error(Mailbox, problem);

            return result;
        }

        /// <summary>
        /// Builds the mailbox_transact op of an MMIO mailbox in an acpi; cpu-family and cpu-models become a CPUID 1 EAX gate.
        /// </summary>
        private static LoadedMailbox LoadMmioMailbox(LoadContext Context, string AcpiId, XElement Mailbox, int Index)
        {
            CheckAttributes(Context, Mailbox, MmioMailboxAttributes);

            if (RequiredAttribute(Context, Mailbox, "transport") != "mmio")
                throw Context.Error(Mailbox, "transport must be \"mmio\" in an <acpi> (the SMN pair belongs to a root complex <pci>)");
            if (!DynDataHw.MailboxAcpiIds.Contains(AcpiId))
                throw Context.Error(Mailbox, $"an MMIO <mailbox> is only allowed in an <acpi> of {string.Join(", ", DynDataHw.MailboxAcpiIds)} (DynDataHw.MailboxAcpiIds), not {AcpiId}");

            var protocol = ParseMailboxProtocol(Context, Mailbox);

            var result = new LoadedMailbox { Name = OptionalAttribute(Mailbox, "name"), Index = Index };
            byte resource = (byte)ParseDecimal(Context, Mailbox, "resource", RequiredAttribute(Context, Mailbox, "resource"), 0, byte.MaxValue);
            UInt32 size = (UInt32)ParseHex(Context, Mailbox, "size", RequiredAttribute(Context, Mailbox, "size"), DynDataHw.MaxWindowSize);
            UInt32 msg = (UInt32)ParseHex(Context, Mailbox, "msg", RequiredAttribute(Context, Mailbox, "msg"), UInt32.MaxValue);
            UInt32 rsp = (UInt32)ParseHex(Context, Mailbox, "rsp", RequiredAttribute(Context, Mailbox, "rsp"), UInt32.MaxValue);
            UInt32 arg = (UInt32)ParseHex(Context, Mailbox, "arg", RequiredAttribute(Context, Mailbox, "arg"), UInt32.MaxValue);
            byte argCount = (byte)ParseDecimal(Context, Mailbox, "args", RequiredAttribute(Context, Mailbox, "args"), 1, DynDataHw.MailboxMaxArgs);
            UInt32 versionOpcode = (UInt32)ParseHex(Context, Mailbox, "version", RequiredAttribute(Context, Mailbox, "version"), DynDataHw.MailboxMaxOpcode);
            string versionArg = OptionalAttribute(Mailbox, "version-arg");
            UInt32 family = (UInt32)ParseHex(Context, Mailbox, "cpu-family", RequiredAttribute(Context, Mailbox, "cpu-family"), 0xFF + 0xF);
            string modelsText = RequiredAttribute(Context, Mailbox, "cpu-models");
            string[] models = modelsText.Split('-');
            UInt64 argEnd = (UInt64)arg + 4ul * argCount;

            if (size == 0 || (size & (size - 1)) != 0)
                throw Context.Error(Mailbox, $"size 0x{size:x} is not a power of two");
            if ((msg % 4) != 0 || (rsp % 4) != 0 || (arg % 4) != 0)
                throw Context.Error(Mailbox, "msg, rsp and arg must be 4-aligned");
            if ((UInt64)msg + 4 > size || (UInt64)rsp + 4 > size || argEnd > size)
                throw Context.Error(Mailbox, $"msg, rsp or the argument registers end past size 0x{size:x}");
            if (msg == rsp || (msg >= arg && msg < argEnd) || (rsp >= arg && rsp < argEnd))
                throw Context.Error(Mailbox, "msg, rsp and the argument registers overlap");
            if (msg / DynDataHw.MailboxWindowPage != rsp / DynDataHw.MailboxWindowPage ||
                msg / DynDataHw.MailboxWindowPage != arg / DynDataHw.MailboxWindowPage ||
                msg / DynDataHw.MailboxWindowPage != (argEnd - 4) / DynDataHw.MailboxWindowPage)
            {
                throw Context.Error(Mailbox, $"msg, rsp and the argument registers are not in one 0x{DynDataHw.MailboxWindowPage:x}-byte page of the window");
            }
            if (versionOpcode == 0)
                throw Context.Error(Mailbox, "version opcode 0x0");

            //
            // CPUID 1 EAX: extended family [27:20], extended model [19:16], family [11:8], model [7:4].
            //
            if (family < 0xF)
                throw Context.Error(Mailbox, $"cpu-family 0x{family:x} is below 0xf (the gate is built from the extended family)");
            if (models.Length != 2)
                throw Context.Error(Mailbox, $"malformed cpu-models \"{modelsText}\" (expected 0xMIN-0xMAX)");

            UInt32 modelMin = (UInt32)ParseHex(Context, Mailbox, "cpu-models", models[0].Trim(), 0xFF);
            UInt32 modelMax = (UInt32)ParseHex(Context, Mailbox, "cpu-models", models[1].Trim(), 0xFF);

            if (modelMin > modelMax || (modelMin >> 4) != (modelMax >> 4))
                throw Context.Error(Mailbox, $"cpu-models \"{modelsText}\" must be ascending within one block of 16 (one extended model)");
            if (!protocol.Mmio.Contains(new DynDataHw.MailboxRegisters(0, 0, msg, rsp, arg, argCount)))
                throw Context.Error(Mailbox, $"msg 0x{msg:x} rsp 0x{rsp:x} arg 0x{arg:x} args {argCount} is not an MMIO register set of protocol \"{protocol.Name}\"");
            if (!DynDataHw.MailboxProtocolCovers(protocol, (UInt16)family, modelMin, modelMax))
                throw Context.Error(Mailbox, $"protocol \"{protocol.Name}\" is not the opcode map of family 0x{family:x} models \"{modelsText}\"");

            UInt32 signature = ((family - 0xF) << 16) | ((modelMin >> 4) << 12) | (0xFu << 4);
            var gate = new DynDataHw.HwGate
            {
                Kind = DynDataHw.GateCpuid,
                Register = DynDataHw.CpuidEax,
                Shift = 4,
                Width = 24,
                Leaf = 1,
                SubLeaf = 0,
                Value = signature | (modelMin & 0xF),
                ValueMax = signature | (modelMax & 0xF)
            };

            var body = new DynDataHw.HwMailboxTransact
            {
                Transport = DynDataHw.MailboxTransportMmioWindow,
                ArgCount = argCount,
                Protocol = protocol.Kind,
                VersionOpcode = versionOpcode,
                VersionArg0 = versionArg != null ? (UInt32)ParseHex(Context, Mailbox, "version-arg", versionArg, UInt32.MaxValue) : 0
            };
            var window = new DynDataHw.HwMailboxWindow
            {
                BaseKind = DynDataHw.WindowBaseAcpiCrs,
                BarIndex = resource,
                WindowSize = size,
                Msg = msg,
                Rsp = rsp,
                Arg0 = arg
            };
            byte[] header = DynDataHw.ToBytes(body);

            DynDataHw.ToBytes(window).CopyTo(header.AsSpan(DynDataHw.MailboxAddressOffset, DynDataHw.MailboxAddressSize));

            byte[] commands = LoadMailboxCommands(Context, Mailbox, protocol, argCount, false, result);

            header[2] = (byte)result.Commands.Count;
            result.ArgCount = argCount;
            result.Op = BuildOp(Context, Mailbox, DynDataHw.KindMailboxTransact, DynDataHw.LockPci, gate, [.. header, .. commands]);

            string problem = DynDataHw.MailboxVersionProblem(result.Op.AsSpan(DynDataHw.OpHeaderSize));

            if (problem != null)
                throw Context.Error(Mailbox, problem);

            return result;
        }

        /// <summary>
        /// Builds the command rows of a mailbox: GET opcodes of its protocol only, each opcode once.
        /// </summary>
        private static byte[] LoadMailboxCommands(LoadContext Context, XElement Mailbox, DynDataHw.MailboxProtocol Protocol, byte argCount, bool AllowInternal, LoadedMailbox Result)
        {
            var commands = new List<byte>();
            var opcodes = new Dictionary<UInt32, XElement>();
            int commandCount = 0;

            foreach (var element in Mailbox.Elements())
            {
                if (element.Name.LocalName != "command")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, CommandAttributes);

                UInt32 opcode = (UInt32)ParseHex(Context, element, "opcode", RequiredAttribute(Context, element, "opcode"), DynDataHw.MailboxMaxOpcode);
                string type = RequiredAttribute(Context, element, "type");
                byte response = (byte)ParseDecimal(Context, element, "response", RequiredAttribute(Context, element, "response"), 0, argCount);
                string internalText = OptionalAttribute(element, "internal");
                bool isInternal = false;

                if (type != "get")
                    throw Context.Error(element, $"type \"{type}\" is not allowed: only read-type (\"get\") commands exist");
                if (opcode == 0)
                    throw Context.Error(element, "opcode 0x0");
                if (!Protocol.GetOpcodes.Contains((byte)opcode))
                    throw Context.Error(element, $"opcode 0x{opcode:x2} is not a GET of protocol \"{Protocol.Name}\" (DynDataHw.MailboxProtocols)");
                if (opcodes.TryGetValue(opcode, out var other))
                    throw Context.Error(element, $"opcode 0x{opcode:x2} repeats the <command> on line {((IXmlLineInfo)other).LineNumber}");

                opcodes.Add(opcode, element);

                if (internalText != null)
                {
                    if (internalText == "true")
                        isInternal = true;
                    else if (internalText != "false")
                        throw Context.Error(element, "internal must be \"true\" or \"false\"");
                }

                if (isInternal && !AllowInternal)
                    throw Context.Error(element, "an MMIO mailbox has no internal command (no <smutable> reaches it)");

                var mask = new UInt32[DynDataHw.MailboxMaxArgs];
                var max = new UInt32[DynDataHw.MailboxMaxArgs];
                var fixedValue = new UInt32[DynDataHw.MailboxMaxArgs];
                var seen = new bool[DynDataHw.MailboxMaxArgs];

                foreach (var argElement in element.Elements())
                {
                    if (argElement.Name.LocalName != "arg")
                        throw Context.Error(argElement, "unknown element");

                    CheckAttributes(Context, argElement, ArgAttributes);
                    CheckNoChildren(Context, argElement);

                    int index = (int)ParseDecimal(Context, argElement, "index", RequiredAttribute(Context, argElement, "index"), 0, (UInt64)argCount - 1);
                    string maskText = OptionalAttribute(argElement, "mask");
                    string maxText = OptionalAttribute(argElement, "max");
                    string fixedText = OptionalAttribute(argElement, "fixed");

                    if (seen[index])
                        throw Context.Error(argElement, $"duplicate arg {index}");

                    seen[index] = true;
                    mask[index] = maskText != null ? (UInt32)ParseHex(Context, argElement, "mask", maskText, UInt32.MaxValue) : 0;
                    max[index] = maxText != null ? (UInt32)ParseHex(Context, argElement, "max", maxText, UInt32.MaxValue) : mask[index];
                    fixedValue[index] = fixedText != null ? (UInt32)ParseHex(Context, argElement, "fixed", fixedText, UInt32.MaxValue) : 0;

                    if (maskText == null && fixedText == null)
                        throw Context.Error(argElement, "an <arg> needs mask or fixed");
                    if ((max[index] & ~mask[index]) != 0)
                        throw Context.Error(argElement, $"max 0x{max[index]:x} has bits outside mask 0x{mask[index]:x}");
                    if ((fixedValue[index] & mask[index]) != 0)
                        throw Context.Error(argElement, $"fixed 0x{fixedValue[index]:x} has bits inside mask 0x{mask[index]:x}");
                }

                var command = new byte[DynDataHw.MailboxCommandSize];
                var span = command.AsSpan();

                BinaryPrimitives.WriteUInt32LittleEndian(span, opcode);
                span[4] = DynDataHw.MailboxCommandGet;
                span[5] = response;
                span[6] = isInternal ? DynDataHw.MailboxCommandInternal : (byte)0;

                for (int i = 0; i < DynDataHw.MailboxMaxArgs; i++)
                {
                    BinaryPrimitives.WriteUInt32LittleEndian(span[(8 + i * 4)..], mask[i]);
                    BinaryPrimitives.WriteUInt32LittleEndian(span[(40 + i * 4)..], max[i]);
                    BinaryPrimitives.WriteUInt32LittleEndian(span[(72 + i * 4)..], fixedValue[i]);
                }

                commands.AddRange(command);
                Result.Commands.Add((opcode, response, isInternal));
                commandCount++;
            }

            if (commandCount == 0)
                throw Context.Error(Mailbox, "<mailbox> has no <command>");
            if (commandCount > DynDataHw.MailboxMaxCommands)
                throw Context.Error(Mailbox, $"{commandCount} <command> exceeds the cap of {DynDataHw.MailboxMaxCommands}");

            return commands.ToArray();
        }

        /// <summary>
        /// Builds the smu_table_read op of a smutable: the version, optional transfer and base commands of a named mailbox.
        /// </summary>
        /// <remarks>The base and transfer commands must be internal; every version the firmware may report is listed.</remarks>
        private static byte[] BuildSmuTableOp(LoadContext Context, Dictionary<string, LoadedMailbox> Mailboxes, Dictionary<string, int> DfMaps, XElement SmuTable)
        {
            CheckAttributes(Context, SmuTable, SmuTableAttributes);

            string mailboxName = RequiredAttribute(Context, SmuTable, "mailbox");

            if (!Mailboxes.TryGetValue(mailboxName, out var mailbox))
                throw Context.Error(SmuTable, $"no <mailbox name=\"{mailboxName}\"> in this element");

            var body = new DynDataHw.HwSmuTableRead
            {
                MailboxOp = (byte)mailbox.Index,
                Bound = RequiredAttribute(Context, SmuTable, "bound") switch
                {
                    "amd-dram" => DynDataHw.SmuTableBoundAmdDram,
                    _ => throw Context.Error(SmuTable, "bound must be \"amd-dram\"")
                },
                VersionOpcode = (UInt32)ParseHex(Context, SmuTable, "version", RequiredAttribute(Context, SmuTable, "version"), DynDataHw.MailboxMaxOpcode),
                BaseOpcode = (UInt32)ParseHex(Context, SmuTable, "base", RequiredAttribute(Context, SmuTable, "base"), DynDataHw.MailboxMaxOpcode),
                BaseLowWord = (byte)ParseDecimal(Context, SmuTable, "base-low", RequiredAttribute(Context, SmuTable, "base-low"), 0, DynDataHw.MailboxMaxArgs - 1),
                BaseHighWord = DynDataHw.SmuTableWordNone,
                SizeWord = DynDataHw.SmuTableWordNone,
                DfMapOp = DynDataHw.SmuTableDfMapNone
            };
            string dfMap = OptionalAttribute(SmuTable, "dfmap");

            if (dfMap != null)
            {
                if (!DfMaps.TryGetValue(dfMap, out int dfMapIndex))
                    throw Context.Error(SmuTable, $"no <dfmap name=\"{dfMap}\"> in this element");

                body.DfMapOp = (byte)dfMapIndex;
            }
            string baseHigh = OptionalAttribute(SmuTable, "base-high");
            string sizeWord = OptionalAttribute(SmuTable, "size-word");

            if (baseHigh != null)
                body.BaseHighWord = (byte)ParseDecimal(Context, SmuTable, "base-high", baseHigh, 0, DynDataHw.MailboxMaxArgs - 1);
            if (sizeWord != null)
                body.SizeWord = (byte)ParseDecimal(Context, SmuTable, "size-word", sizeWord, 0, DynDataHw.MailboxMaxArgs - 1);
            if ((baseHigh != null && body.BaseHighWord == body.BaseLowWord) ||
                (sizeWord != null && (body.SizeWord == body.BaseLowWord || body.SizeWord == body.BaseHighWord)))
                throw Context.Error(SmuTable, "base-low, base-high and size-word name the same reply word");

            byte baseWords = DynDataHw.SmuTableBaseWords(body.BaseLowWord, body.BaseHighWord, body.SizeWord);

            if (baseWords > mailbox.ArgCount)
                throw Context.Error(SmuTable, $"reply word {baseWords - 1} is not below the {mailbox.ArgCount} arguments of mailbox \"{mailboxName}\"");

            string transfer = OptionalAttribute(SmuTable, "transfer");

            if (transfer != null)
            {
                body.TransferOpcode = (UInt32)ParseHex(Context, SmuTable, "transfer", transfer, DynDataHw.MailboxMaxOpcode);

                if (body.TransferOpcode == 0)
                    throw Context.Error(SmuTable, "transfer opcode 0x0 (omit transfer for none)");
            }

            string setup = OptionalAttribute(SmuTable, "setup");

            if (setup != null)
            {
                body.SetupOpcode = (byte)ParseHex(Context, SmuTable, "setup", setup, DynDataHw.MailboxMaxOpcode);

                if (body.SetupOpcode == 0 || body.SetupOpcode == body.BaseOpcode || body.SetupOpcode == body.VersionOpcode || body.SetupOpcode == body.TransferOpcode)
                    throw Context.Error(SmuTable, $"setup opcode 0x{body.SetupOpcode:x2} is zero or another opcode of the table");
            }

            if (body.VersionOpcode == body.BaseOpcode || body.VersionOpcode == body.TransferOpcode)
                throw Context.Error(SmuTable, $"version opcode 0x{body.VersionOpcode:x2} is also the base or transfer opcode");

            void Require(string Name, UInt32 Opcode, byte ResponseWords, bool Internal)
            {
                var rows = mailbox.Commands.Where(c => c.Opcode == Opcode).ToList();

                if (rows.Count == 0)
                    throw Context.Error(SmuTable, $"{Name} opcode 0x{Opcode:x2} is not a <command> of mailbox \"{mailboxName}\"");
                if (rows.Any(c => c.ResponseWords < ResponseWords))
                    throw Context.Error(SmuTable, $"{Name} opcode 0x{Opcode:x2} reads back fewer than {ResponseWords} words");
                if (Internal && rows.Any(c => !c.Internal))
                    throw Context.Error(SmuTable, $"{Name} opcode 0x{Opcode:x2} must be internal=\"true\" (never issued for a client)");
            }

            Require("version", body.VersionOpcode, 1, false);
            Require("base", body.BaseOpcode, baseWords, true);

            if (body.TransferOpcode != 0)
                Require("transfer", body.TransferOpcode, 0, true);
            if (body.SetupOpcode != 0)
                Require("setup", body.SetupOpcode, 0, true);

            var versions = new List<DynDataHw.HwSmuTableVersion>();

            foreach (var element in SmuTable.Elements())
            {
                if (element.Name.LocalName != "version")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, SmuTableVersionAttributes);
                CheckNoChildren(Context, element);

                UInt32 id = (UInt32)ParseHex(Context, element, "id", RequiredAttribute(Context, element, "id"), UInt32.MaxValue);
                UInt32 size = (UInt32)ParseHex(Context, element, "size", RequiredAttribute(Context, element, "size"), DynDataHw.SmuTableMaxSize);

                if (size < 4 || (size % 4) != 0)
                    throw Context.Error(element, $"size 0x{size:x} is not a non-zero multiple of 4");
                if (versions.Any(v => v.Version == id))
                    throw Context.Error(element, $"duplicate version 0x{id:x}");

                versions.Add(new DynDataHw.HwSmuTableVersion { Version = id, Size = size });
            }

            if (versions.Count == 0)
                throw Context.Error(SmuTable, "<smutable> has no <version>");
            if (versions.Count > DynDataHw.SmuTableMaxVersions)
                throw Context.Error(SmuTable, $"{versions.Count} <version> exceeds the cap of {DynDataHw.SmuTableMaxVersions}");

            body.VersionCount = (byte)versions.Count;

            var bytes = new List<byte>();

            bytes.AddRange(DynDataHw.ToBytes(body));
            foreach (var version in versions)
                bytes.AddRange(DynDataHw.ToBytes(version));

            return BuildOp(Context, SmuTable, DynDataHw.KindSmuTableRead, DynDataHw.LockPci, default, bytes.ToArray());
        }

        /// <summary>
        /// Builds the df_map op of a dfmap element: the DRAM address map layout of the AMD Data Fabric of this CPU,
        /// read by the driver through the ECAM page of device.function on the root's segment and bus.
        /// </summary>
        private static byte[] BuildDfMapOp(LoadContext Context, LoadedPci Loaded, bool Nested, XElement Map)
        {
            CheckAttributes(Context, Map, DfMapAttributes);
            CheckNoChildren(Context, Map);

            if (!Nested || Loaded.Class || !Loaded.Pinned || Loaded.VendorId != 0x1022)
                throw Context.Error(Map, "<dfmap> is only allowed in the pinned AMD root complex <pci> of a <cpu>");

            var body = new DynDataHw.HwDfMap
            {
                Device = (byte)ParseHex(Context, Map, "device", RequiredAttribute(Context, Map, "device"), 31),
                Function = (byte)ParseDecimal(Context, Map, "function", RequiredAttribute(Context, Map, "function"), 0, 7),
                MapOffset = (UInt16)ParseHex(Context, Map, "offset", RequiredAttribute(Context, Map, "offset"), DynDataHw.PciConfigLength - 4),
                MapCount = (byte)ParseDecimal(Context, Map, "count", RequiredAttribute(Context, Map, "count"), 1, byte.MaxValue),
                MapStride = (byte)ParseHex(Context, Map, "stride", RequiredAttribute(Context, Map, "stride"), byte.MaxValue),
                LimitOffset = (byte)ParseHex(Context, Map, "limit", RequiredAttribute(Context, Map, "limit"), byte.MaxValue),
                ControlOffset = (byte)ParseHex(Context, Map, "control", RequiredAttribute(Context, Map, "control"), byte.MaxValue),
                AddressShift = (byte)ParseDecimal(Context, Map, "shift", RequiredAttribute(Context, Map, "shift"), 12, 40),
                AddressMask = (UInt32)ParseHex(Context, Map, "mask", RequiredAttribute(Context, Map, "mask"), UInt32.MaxValue),
                ValidBit = (byte)ParseDecimal(Context, Map, "valid", RequiredAttribute(Context, Map, "valid"), 0, 31),
                HoleRemapBit = (byte)ParseDecimal(Context, Map, "remap", RequiredAttribute(Context, Map, "remap"), 0, 31)
            };
            string hole = OptionalAttribute(Map, "hole");
            string holeMask = OptionalAttribute(Map, "hole-mask");
            string holeValid = OptionalAttribute(Map, "hole-valid");

            if ((hole != null) != (holeMask != null) || (hole != null) != (holeValid != null))
                throw Context.Error(Map, "hole, hole-mask and hole-valid go together");
            if (hole != null)
            {
                body.HoleOffset = (UInt16)ParseHex(Context, Map, "hole", hole, DynDataHw.PciConfigLength - 4);
                body.HoleBaseMask = (UInt32)ParseHex(Context, Map, "hole-mask", holeMask, UInt32.MaxValue);
                body.HoleValidBit = (byte)ParseDecimal(Context, Map, "hole-valid", holeValid, 0, 31);

                if (body.HoleOffset == 0)
                    throw Context.Error(Map, "hole offset is zero");
            }

            string error = DynDataHw.DfMapError(body);

            if (error != null)
                throw Context.Error(Map, error);

            return BuildOp(Context, Map, DynDataHw.KindDfMap, DynDataHw.LockNone, default, DynDataHw.ToBytes(body));
        }

        private static byte[] BuildIndirectOp(LoadContext Context, XElement Pci, XElement Indirect)
        {
            CheckAttributes(Context, Indirect, IndirectAttributes);

            string pin = OptionalAttribute(Pci, "pin");

            if (pin == null)
                throw Context.Error(Pci, "pin is required when <indirect> is present");

            var (pinDevice, pinFunction) = ParsePin(Context, Pci, pin);
            var targets = new List<DynDataHw.HwTargetRange>();
            var values = new List<UInt32>();

            foreach (var element in Indirect.Elements())
            {
                switch (element.Name.LocalName)
                {
                case "target":
                    {
                        CheckAttributes(Context, element, TargetAttributes);
                        CheckNoChildren(Context, element);

                        UInt32 begin = (UInt32)ParseHex(Context, element, "begin", RequiredAttribute(Context, element, "begin"), UInt32.MaxValue);
                        UInt32 end = (UInt32)ParseHex(Context, element, "end", RequiredAttribute(Context, element, "end"), UInt32.MaxValue);
                        string strideText = OptionalAttribute(element, "stride");
                        string countText = OptionalAttribute(element, "count");
                        UInt32 stride = 0;
                        byte count = 0;

                        if ((begin % 4) != 0 || (end % 4) != 0)
                            throw Context.Error(element, $"begin 0x{begin:x} / end 0x{end:x} not 4-aligned");
                        if (begin >= end)
                            throw Context.Error(element, $"begin 0x{begin:x} >= end 0x{end:x}");
                        if ((strideText == null) != (countText == null))
                            throw Context.Error(element, "stride and count must be given together");

                        if (strideText != null)
                        {
                            stride = (UInt32)ParseHex(Context, element, "stride", strideText, UInt32.MaxValue);
                            count = (byte)ParseDecimal(Context, element, "count", countText, 1, byte.MaxValue);

                            if ((stride % 4) != 0)
                                throw Context.Error(element, $"stride 0x{stride:x} not 4-aligned");

                            UInt64 span = (UInt64)stride * (UInt64)(count - 1);

                            if (begin + span > UInt32.MaxValue || end + span > UInt32.MaxValue)
                                throw Context.Error(element, $"stride 0x{stride:x} x count {count} overflows 32 bits");
                        }

                        targets.Add(new DynDataHw.HwTargetRange { Begin = begin, End = end, Stride = stride, StrideCount = count });
                    }
                    break;
                case "value":
                    {
                        CheckAttributes(Context, element, ["desc"]);
                        CheckNoChildren(Context, element);

                        UInt32 value = (UInt32)ParseHex(Context, element, "value", element.Value.Trim(), UInt32.MaxValue);

                        if ((value % 4) != 0)
                            throw Context.Error(element, $"value 0x{value:x} not 4-aligned");

                        values.Add(value);
                    }
                    break;
                default:
                    throw Context.Error(element, "unknown element");
                }
            }

            if (targets.Count == 0 && values.Count == 0)
                throw Context.Error(Indirect, "<indirect> has no <target> and no <value>");
            if (targets.Count > DynDataHw.MaxRanges)
                throw Context.Error(Indirect, $"{targets.Count} <target> exceeds the cap of {DynDataHw.MaxRanges}");
            if (values.Count > DynDataHw.MaxRanges)
                throw Context.Error(Indirect, $"{values.Count} <value> exceeds the cap of {DynDataHw.MaxRanges}");

            UInt16 indexOffset = (UInt16)ParseHex(Context, Indirect, "index", RequiredAttribute(Context, Indirect, "index"), DynDataHw.PciConfigLength - 1);
            UInt16 dataOffset = (UInt16)ParseHex(Context, Indirect, "data", RequiredAttribute(Context, Indirect, "data"), DynDataHw.PciConfigLength - 1);

            if ((indexOffset % 4) != 0 || (dataOffset % 4) != 0)
                throw Context.Error(Indirect, $"index 0x{indexOffset:x} / data 0x{dataOffset:x} not 4-aligned");
            if (indexOffset < DynDataHw.PciConfigHeaderLength || dataOffset < DynDataHw.PciConfigHeaderLength)
                throw Context.Error(Indirect, $"index 0x{indexOffset:x} / data 0x{dataOffset:x} inside the 0x{DynDataHw.PciConfigHeaderLength:x}-byte config header");
            if (indexOffset == dataOffset)
                throw Context.Error(Indirect, $"index and data are both 0x{indexOffset:x}");
            if (!DynDataHw.IndirectPairs.Contains((indexOffset, dataOffset)))
                throw Context.Error(Indirect, $"index 0x{indexOffset:x} / data 0x{dataOffset:x} is not a pair of DynDataHw.IndirectPairs");

            var body = new List<byte>();

            body.AddRange(DynDataHw.ToBytes(new DynDataHw.HwPciIndirectRead
            {
                IndexOffset = indexOffset,
                DataOffset = dataOffset,
                PinDevice = pinDevice,
                PinFunction = pinFunction,
                RangeCount = (byte)targets.Count,
                ValueCount = (byte)values.Count
            }));
            foreach (var target in targets)
                body.AddRange(DynDataHw.ToBytes(target));
            foreach (var value in values)
                body.AddRange(DynDataHw.ToBytes(value));

            return BuildOp(Context, Indirect, DynDataHw.KindPciIndirectRead, DynDataHw.LockPci, default, body.ToArray());
        }

        /// <summary>
        /// Builds the window_read op of a window element; the base is derived by the driver, the size is signed data.
        /// </summary>
        private static byte[] BuildWindowOp(LoadContext Context, XElement Window, WindowPlacement Placement)
        {
            CheckAttributes(Context, Window, WindowAttributes);

            var body = new DynDataHw.HwWindowRead
            {
                SpaceKind = DynDataHw.WindowSpaceMmio,
                EnableBit = DynDataHw.WindowEnableNone
            };
            var aux = new List<UInt32>();
            UInt64 size = ParseHex(Context, Window, "size", RequiredAttribute(Context, Window, "size"), DynDataHw.MaxWindowSize);
            string requireD0 = OptionalAttribute(Window, "require-d0");
            string baseKind = RequiredAttribute(Context, Window, "base");
            byte lockClass = DynDataHw.LockNone;
            byte flags = 0;

            if (size == 0 || (size & (size - 1)) != 0)
                throw Context.Error(Window, $"size 0x{size:x} is not a power of two");

            body.WindowSize = (UInt32)size;

            void Disallow(params string[] Names)
            {
                foreach (string name in Names)
                {
                    if (OptionalAttribute(Window, name) != null)
                        throw Context.Error(Window, $"{name} is not an attribute of base=\"{baseKind}\"");
                }
            }

            switch (baseKind)
            {
            case "bar":
            case "config":
                if (Placement != WindowPlacement.Pci && Placement != WindowPlacement.RootPci)
                    throw Context.Error(Window, $"base=\"{baseKind}\" is only allowed in a <pci> or <pciclass>");
                Disallow("lo", "hi", "address", "port", "present", "signature");
                break;
            case "smn":
                if (Placement != WindowPlacement.RootPci)
                    throw Context.Error(Window, "base=\"smn\" is only allowed in a <pci> or <pciclass> nested in a <cpu>");
                break;
            case "crs":
                if (Placement != WindowPlacement.Acpi)
                    throw Context.Error(Window, "base=\"crs\" is only allowed in an <acpi>");
                Disallow("lo", "hi", "address", "port", "present", "signature");
                break;
            case "lpit":
            case "lpit-relative":
                if (Placement != WindowPlacement.Cpu)
                    throw Context.Error(Window, $"base=\"{baseKind}\" is only allowed in a <cpu>");
                Disallow("lo", "hi", "address", "port", "present", "signature");
                break;
            case "fixed":
                if (Placement != WindowPlacement.Cpu && Placement != WindowPlacement.Board)
                    throw Context.Error(Window, "base=\"fixed\" is only allowed in a <cpu> or a <board>");
                break;
            case "fixed-io":
                if (Placement != WindowPlacement.Board)
                    throw Context.Error(Window, "base=\"fixed-io\" is only allowed in a <board>");
                break;
            default:
                throw Context.Error(Window, "base must be \"bar\", \"config\", \"smn\", \"crs\", \"lpit\", \"lpit-relative\", \"fixed\" or \"fixed-io\"");
            }

            switch (baseKind)
            {
            case "bar":
                {
                    Disallow("offset", "width", "mask", "enable", "resource", "entry");

                    body.BaseKind = DynDataHw.WindowBaseStandardBar;
                    body.BarIndex = (byte)ParseDecimal(Context, Window, "bar", RequiredAttribute(Context, Window, "bar"), 0, DynDataHw.PciBarCount - 1u);
                }
                break;
            case "config":
                {
                    Disallow("bar", "resource", "entry");

                    UInt64 offset = ParseHex(Context, Window, "offset", RequiredAttribute(Context, Window, "offset"), DynDataHw.PciConfigLength - 1);
                    UInt64 width = ParseDecimal(Context, Window, "width", RequiredAttribute(Context, Window, "width"), 4, 8);
                    string enable = OptionalAttribute(Window, "enable");

                    if (width != 4 && width != 8)
                        throw Context.Error(Window, $"width {width} is not 4 or 8");
                    if ((offset % 4) != 0 || offset < DynDataHw.PciConfigHeaderLength || offset + width > DynDataHw.PciConfigLength)
                        throw Context.Error(Window, $"offset 0x{offset:x} must be 4-aligned, above the config header and end within config space");

                    UInt64 mask = ParseHex(Context, Window, "mask", RequiredAttribute(Context, Window, "mask"), width == 4 ? UInt32.MaxValue : UInt64.MaxValue);

                    if (mask == 0 || (mask & (size - 1)) != 0)
                        throw Context.Error(Window, $"mask 0x{mask:x} is zero or has bits below the window size 0x{size:x}");

                    body.BaseKind = DynDataHw.WindowBaseConfigReg;
                    body.ConfigOffset = (UInt16)offset;
                    body.Is64 = (byte)(width == 8 ? 1 : 0);
                    body.ConfigMask = mask;

                    if (enable != null)
                    {
                        body.EnableBit = (byte)ParseDecimal(Context, Window, "enable", enable, 0, width * 8 - 1);

                        if ((mask & (1ul << body.EnableBit)) != 0)
                            throw Context.Error(Window, $"enable bit {body.EnableBit} is inside the mask");
                    }
                }
                break;
            case "smn":
                {
                    Disallow("bar", "width", "resource", "entry", "address", "port", "present", "signature", "require-d0");

                    //
                    // base = ((SMN[hi] << 32) | SMN[lo]) & mask, while bit enable of that value is set.
                    //
                    UInt32 lo = (UInt32)ParseHex(Context, Window, "lo", RequiredAttribute(Context, Window, "lo"), UInt32.MaxValue);
                    UInt32 hi = (UInt32)ParseHex(Context, Window, "hi", RequiredAttribute(Context, Window, "hi"), UInt32.MaxValue);
                    UInt64 offset = ParseHex(Context, Window, "offset", RequiredAttribute(Context, Window, "offset"), DynDataHw.SmnApertureSize);
                    UInt64 mask = ParseHex(Context, Window, "mask", RequiredAttribute(Context, Window, "mask"), UInt64.MaxValue);
                    byte enable = (byte)ParseDecimal(Context, Window, "enable", RequiredAttribute(Context, Window, "enable"), 0, 63);

                    if ((lo % 4) != 0 || (hi % 4) != 0 || lo == hi)
                        throw Context.Error(Window, $"lo 0x{lo:x} and hi 0x{hi:x} must be distinct 4-aligned SMN addresses");
                    if ((offset % size) != 0 || offset + size > DynDataHw.SmnApertureSize)
                        throw Context.Error(Window, $"offset 0x{offset:x} must be aligned to the size 0x{size:x} and the window end inside the 0x{DynDataHw.SmnApertureSize:x}-byte aperture");
                    if (mask == 0 || (mask & (DynDataHw.SmnApertureSize - 1)) != 0)
                        throw Context.Error(Window, $"mask 0x{mask:x} is zero or has bits below the 0x{DynDataHw.SmnApertureSize:x}-byte aperture");
                    if ((mask & (1ul << enable)) != 0)
                        throw Context.Error(Window, $"enable bit {enable} is inside the mask");

                    body.BaseKind = DynDataHw.WindowBaseSmnDerived;
                    body.FixedBase = offset;
                    body.ConfigMask = mask;
                    body.EnableBit = enable;
                    aux.Add(lo);
                    aux.Add(hi);
                    lockClass = DynDataHw.LockPci;
                }
                break;
            case "crs":
                {
                    Disallow("bar", "offset", "width", "mask", "enable", "entry", "require-d0");

                    body.BaseKind = DynDataHw.WindowBaseAcpiCrs;
                    body.BarIndex = (byte)ParseDecimal(Context, Window, "resource", RequiredAttribute(Context, Window, "resource"), 0, byte.MaxValue);
                }
                break;
            case "lpit":
                {
                    Disallow("bar", "offset", "width", "mask", "enable", "resource", "require-d0");

                    if (size != 4 && size != 8)
                        throw Context.Error(Window, $"size 0x{size:x} is not 0x4 or 0x8 (a 32- or 64-bit residency counter)");

                    body.BaseKind = DynDataHw.WindowBaseAcpiLpit;
                    body.BarIndex = (byte)ParseDecimal(Context, Window, "entry", RequiredAttribute(Context, Window, "entry"), 0, DynDataHw.AcpiLpitMaxEntries - 1u);
                }
                break;
            case "lpit-relative":
                {
                    Disallow("bar", "width", "mask", "enable", "resource", "require-d0");

                    //
                    // base = LPIT counter address - offset, aligned to the size: the counter is at offset in its block.
                    //
                    UInt64 offset = ParseHex(Context, Window, "offset", RequiredAttribute(Context, Window, "offset"), DynDataHw.AcpiLpitRelativeMaxSize);

                    if (size > DynDataHw.AcpiLpitRelativeMaxSize)
                        throw Context.Error(Window, $"size 0x{size:x} exceeds 0x{DynDataHw.AcpiLpitRelativeMaxSize:x}");
                    if (offset == 0 || (offset % 4) != 0 || offset >= size)
                        throw Context.Error(Window, $"offset 0x{offset:x} must be non-zero, 4-aligned and inside the size 0x{size:x}");

                    body.BaseKind = DynDataHw.WindowBaseAcpiLpitRelative;
                    body.BarIndex = (byte)ParseDecimal(Context, Window, "entry", RequiredAttribute(Context, Window, "entry"), 0, DynDataHw.AcpiLpitMaxEntries - 1u);
                    body.FixedBase = offset;
                }
                break;
            case "fixed":
                {
                    Disallow("bar", "offset", "width", "mask", "enable", "resource", "entry", "lo", "hi", "port", "signature", "require-d0");

                    UInt64 address = ParseHex(Context, Window, "address", RequiredAttribute(Context, Window, "address"), UInt32.MaxValue);
                    string present = OptionalAttribute(Window, "present");

                    if (address == 0 || (address % size) != 0 || address + size > 0x100000000ul)
                        throw Context.Error(Window, $"address 0x{address:x} must be non-zero, aligned to the size 0x{size:x} and the window end at or below 4 GB");
                    if (!DynDataHw.FixedMmioWindows.Contains((address, (UInt32)size)))
                        throw Context.Error(Window, $"address 0x{address:x} size 0x{size:x} is not in DynDataHw.FixedMmioWindows");

                    body.BaseKind = DynDataHw.WindowBaseFixedMmio;
                    body.FixedBase = address;

                    if (present == null)
                    {
                        if (Placement == WindowPlacement.Cpu)
                            throw Context.Error(Window, "base=\"fixed\" in a <cpu> needs present=\"0xVVVV:0xDDDD\" (the PCI device whose presence the driver checks)");
                    }
                    else
                    {
                        string[] ids = present.Split(':');

                        if (ids.Length != 2)
                            throw Context.Error(Window, $"malformed present \"{present}\" (expected 0xVVVV:0xDDDD)");

                        UInt32 vendorId = (UInt32)ParseHex(Context, Window, "present vendor", ids[0].Trim(), UInt16.MaxValue);
                        UInt32 deviceId = (UInt32)ParseHex(Context, Window, "present device", ids[1].Trim(), UInt16.MaxValue);

                        if (vendorId == 0 || vendorId == UInt16.MaxValue || deviceId == 0 || deviceId == UInt16.MaxValue)
                            throw Context.Error(Window, $"present \"{present}\" is not an ID a PCI function can have");

                        aux.Add(vendorId | (deviceId << 16));
                    }
                }
                break;
            case "fixed-io":
                {
                    Disallow("bar", "offset", "width", "mask", "enable", "resource", "entry", "lo", "hi", "address", "present", "require-d0");

                    UInt64 port = ParseHex(Context, Window, "port", RequiredAttribute(Context, Window, "port"), UInt16.MaxValue);
                    string signature = RequiredAttribute(Context, Window, "signature");
                    string[] parts = signature.Split('=');

                    if (size > DynDataHw.FixedIoMaxSize)
                        throw Context.Error(Window, $"size 0x{size:x} exceeds 0x{DynDataHw.FixedIoMaxSize:x}");
                    if (port < DynDataHw.FixedIoMinBase || (port % size) != 0 || port + size > 0x10000)
                        throw Context.Error(Window, $"port 0x{port:x} must be at least 0x{DynDataHw.FixedIoMinBase:x}, aligned to the size 0x{size:x} and the window end inside port space");
                    if (!DynDataHw.FixedIoWindows.Contains((port, (UInt32)size)))
                        throw Context.Error(Window, $"port 0x{port:x} size 0x{size:x} is not in DynDataHw.FixedIoWindows");
                    if (parts.Length != 2)
                        throw Context.Error(Window, $"malformed signature \"{signature}\" (expected 0xOFF=0xB0[,0xB1...])");

                    UInt64 signatureOffset = ParseHex(Context, Window, "signature offset", parts[0].Trim(), size - 1);
                    string[] signatureBytes = parts[1].Split(',');
                    UInt32 value = 0;

                    if (signatureBytes.Length > DynDataHw.FixedIoMaxSignature)
                        throw Context.Error(Window, $"signature \"{signature}\" has more than {DynDataHw.FixedIoMaxSignature} bytes");
                    if (signatureOffset + (UInt64)signatureBytes.Length > size)
                        throw Context.Error(Window, $"signature \"{signature}\" ends past the window size 0x{size:x}");

                    for (int i = 0; i < signatureBytes.Length; i++)
                        value |= (UInt32)ParseHex(Context, Window, "signature byte", signatureBytes[i].Trim(), byte.MaxValue) << (8 * i);

                    if (value == UInt32.MaxValue >> (8 * (4 - signatureBytes.Length)))
                        throw Context.Error(Window, $"signature \"{signature}\" is all FFh (what ports nothing decodes read)");

                    body.BaseKind = DynDataHw.WindowBaseFixedIo;
                    body.SpaceKind = DynDataHw.WindowSpaceIo;
                    body.FixedBase = port;
                    aux.Add((UInt32)signatureOffset | ((UInt32)signatureBytes.Length << 16));
                    aux.Add(value);
                }
                break;
            }

            //
            // A BAR window requires D0 unless opted out; a config register window only when asked.
            //
            if (body.BaseKind == DynDataHw.WindowBaseStandardBar)
                flags |= DynDataHw.FlagRequireD0;

            if (requireD0 != null)
            {
                if (requireD0 == "true")
                    flags |= DynDataHw.FlagRequireD0;
                else if (requireD0 == "false")
                    flags &= unchecked((byte)~DynDataHw.FlagRequireD0);
                else
                    throw Context.Error(Window, "require-d0 must be \"true\" or \"false\"");
            }

            flags |= ParseArm64(Context, Window, body.BaseKind == DynDataHw.WindowBaseStandardBar || body.BaseKind == DynDataHw.WindowBaseConfigReg);

            var ranges = new List<DynDataHw.HwWindowRange>();

            if (body.BaseKind == DynDataHw.WindowBaseAcpiLpit)
            {
                CheckNoChildren(Context, Window);

                ranges.Add(new DynDataHw.HwWindowRange { Offset = 0, Length = (UInt32)size, Width = (byte)size });
            }

            foreach (var element in Window.Elements())
            {
                if (element.Name.LocalName != "read")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, WindowReadAttributes);
                CheckNoChildren(Context, element);

                UInt64 offset = ParseHex(Context, element, "offset", RequiredAttribute(Context, element, "offset"), DynDataHw.MaxWindowSize);
                UInt64 length = ParseHex(Context, element, "length", RequiredAttribute(Context, element, "length"), DynDataHw.MaxWindowSize);
                UInt64 width = ParseDecimal(Context, element, "width", RequiredAttribute(Context, element, "width"), 1, 8);

                if (width != 1 && width != 2 && width != 4 && width != 8)
                    throw Context.Error(element, $"width {width} is not 1, 2, 4 or 8");
                if (body.BaseKind == DynDataHw.WindowBaseFixedIo && width != 1)
                    throw Context.Error(element, $"width {width}: a base=\"fixed-io\" window is read a byte at a time");
                if ((offset % width) != 0 || length < width || (length % width) != 0)
                    throw Context.Error(element, $"offset 0x{offset:x} / length 0x{length:x} not aligned to width {width}");
                if (offset + length > size)
                    throw Context.Error(element, $"offset 0x{offset:x} + length 0x{length:x} exceeds the window size 0x{size:x}");
                if (body.BaseKind == DynDataHw.WindowBaseAcpiLpitRelative && !DynDataHw.InLpitCounterPage(body.FixedBase, offset, length))
                    throw Context.Error(element, $"offset 0x{offset:x} + length 0x{length:x} leaves the 4 KiB page of the LPIT counter at 0x{body.FixedBase:x}");

                ranges.Add(new DynDataHw.HwWindowRange { Offset = (UInt32)offset, Length = (UInt32)length, Width = (byte)width });
            }

            if (ranges.Count == 0)
                throw Context.Error(Window, "<window> has no <read>");
            if (ranges.Count > DynDataHw.MaxRanges)
                throw Context.Error(Window, $"{ranges.Count} <read> ranges exceeds the cap of {DynDataHw.MaxRanges}");

            body.RangeCount = (byte)ranges.Count;
            body.AuxCount = (byte)aux.Count;

            var bytes = new List<byte>();

            bytes.AddRange(DynDataHw.ToBytes(body));
            foreach (var range in ranges)
                bytes.AddRange(DynDataHw.ToBytes(range));
            foreach (UInt32 word in aux)
                bytes.AddRange(DynDataHw.ToBytes(word));

            return BuildOp(Context, Window, DynDataHw.KindWindowRead, lockClass, default, bytes.ToArray(), flags);
        }

        /// <summary>
        /// Builds the port_indirect_read op of a portindirect element, proved present by the pinned function it is nested in.
        /// </summary>
        /// <remarks>Every index listed must be harmless to a stray index write.</remarks>
        private static byte[] BuildPortIndirectOp(LoadContext Context, LoadedPci Loaded, bool Nested, XElement Port)
        {
            CheckAttributes(Context, Port, PortIndirectAttributes);

            if (!Nested || Loaded.Class || !Loaded.Pinned)
                throw Context.Error(Port, "<portindirect> is only allowed in a pinned <pci> nested in a <cpu> (CPU_AND_PCI: the device ID proves the pair)");

            var body = new DynDataHw.HwPortIndirectRead
            {
                PinDevice = (byte)(Loaded.DevFn >> 3),
                PinFunction = (byte)(Loaded.DevFn & 7)
            };

            if (Loaded.VendorId != 0x1022)
                throw Context.Error(Port, "<portindirect> is an AMD FCH pair (vendor 0x1022)");

            (body.IndexPort, body.DataPort) = ParsePortPair(Context, Port);

            var ranges = new List<DynDataHw.HwRange>();

            foreach (var element in Port.Elements())
            {
                if (element.Name.LocalName != "read")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, PortIndirectReadAttributes);
                CheckNoChildren(Context, element);

                UInt64 index = ParseHex(Context, element, "index", RequiredAttribute(Context, element, "index"), DynDataHw.PortIndexEnd - 1);
                UInt64 length = ParseHex(Context, element, "length", RequiredAttribute(Context, element, "length"), DynDataHw.PortIndexEnd);

                if (length == 0)
                    throw Context.Error(element, "length is zero");
                if (index + length > DynDataHw.PortIndexEnd)
                    throw Context.Error(element, $"index 0x{index:x2} + length 0x{length:x} reaches past 0x{DynDataHw.PortIndexEnd - 1:x2} (index FFh is what a pair that does not decode reads)");

                ranges.Add(new DynDataHw.HwRange { Offset = (UInt16)index, Length = (UInt16)length });
            }

            if (ranges.Count == 0)
                throw Context.Error(Port, "<portindirect> has no <read>");
            if (ranges.Count > DynDataHw.MaxRanges)
                throw Context.Error(Port, $"{ranges.Count} <read> ranges exceeds the cap of {DynDataHw.MaxRanges}");

            body.RangeCount = (byte)ranges.Count;

            var bytes = new List<byte>();

            bytes.AddRange(DynDataHw.ToBytes(body));
            foreach (var range in ranges)
                bytes.AddRange(DynDataHw.ToBytes(range));

            return BuildOp(Context, Port, DynDataHw.KindPortIndirectRead, DynDataHw.LockIsaBus, default, bytes.ToArray());
        }

        /// <summary>
        /// Builds the smbus_read op of an smbus element.
        /// </summary>
        private static byte[] BuildSmbusOp(LoadContext Context, LoadedPci Loaded, bool Nested, XElement Smbus)
        {
            CheckAttributes(Context, Smbus, SmbusAttributes);

            if (Nested || Loaded.Class)
                throw Context.Error(Smbus, "<smbus> is only allowed in a top-level <pci> (PCI identity of the host controller)");

            var body = new DynDataHw.HwSmbusRead();
            string port = OptionalAttribute(Smbus, "port");

            switch (RequiredAttribute(Context, Smbus, "host"))
            {
            case "i801":
                {
                    if (Loaded.VendorId != 0x8086)
                        throw Context.Error(Smbus, "host=\"i801\" is an Intel controller (vendor 0x8086)");
                    if (port != null)
                        throw Context.Error(Smbus, "port is only for host=\"piix4\"");

                    body.HostKind = DynDataHw.SmbusHostI801;
                }
                break;
            case "piix4":
                {
                    if (!DynDataHw.SmbusPiix4Vendors.Contains(Loaded.VendorId))
                        throw Context.Error(Smbus, $"host=\"piix4\" is on vendor 0x{Loaded.VendorId:x4}, not in DynDataHw.SmbusPiix4Vendors");
                    if (port == null)
                        throw Context.Error(Smbus, "host=\"piix4\" needs port (the PM port select)");

                    body.HostKind = DynDataHw.SmbusHostPiix4;
                    body.Port = (byte)ParseDecimal(Context, Smbus, "port", port, 0, DynDataHw.SmbusPorts - 1u);
                }
                break;
            default:
                throw Context.Error(Smbus, "host must be \"i801\" or \"piix4\"");
            }

            foreach (string item in RequiredAttribute(Context, Smbus, "protocols").Split(','))
            {
                byte protocol = item.Trim() switch
                {
                    "byte" => DynDataHw.SmbusProtocolByteData,
                    "word" => DynDataHw.SmbusProtocolWordData,
                    _ => throw Context.Error(Smbus, $"protocol \"{item.Trim()}\" must be \"byte\" (read byte data) or \"word\" (read word data)")
                };

                if ((body.Protocols & protocol) != 0)
                    throw Context.Error(Smbus, $"duplicate protocol \"{item.Trim()}\"");

                body.Protocols |= protocol;
            }

            var slaves = new List<int>();

            foreach (string item in RequiredAttribute(Context, Smbus, "slaves").Split(','))
            {
                string[] bounds = item.Trim().Split('-');

                if (bounds.Length > 2)
                    throw Context.Error(Smbus, $"malformed slaves item \"{item}\"");

                UInt64 low = ParseHex(Context, Smbus, "slaves", bounds[0].Trim(), 0x7F);
                UInt64 high = bounds.Length == 2 ? ParseHex(Context, Smbus, "slaves", bounds[1].Trim(), 0x7F) : low;

                if (low > high)
                    throw Context.Error(Smbus, $"slaves range \"{item}\" is reversed");

                for (int address = (int)low; address <= (int)high; address++)
                {
                    if (DynDataHw.SmbusAddressReserved(address))
                        throw Context.Error(Smbus, $"slave 0x{address:x2} is a reserved address (00h-07h, 78h-7Fh)");
                    if (DynDataHw.SmbusAddressNotDevice(address))
                        throw Context.Error(Smbus, $"slave 0x{address:x2} is the host, alert response, EE1004 page/protection select or ARP address");
                    if (slaves.Contains(address))
                        throw Context.Error(Smbus, $"duplicate slave 0x{address:x2}");

                    slaves.Add(address);
                }
            }

            unsafe
            {
                foreach (int address in slaves)
                    body.Slaves[address >> 3] |= (byte)(1 << (address & 7));
            }

            var ranges = new List<DynDataHw.HwRange>();

            foreach (var element in Smbus.Elements())
            {
                if (element.Name.LocalName != "read")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, SmbusReadAttributes);
                CheckNoChildren(Context, element);

                UInt64 command = ParseHex(Context, element, "command", RequiredAttribute(Context, element, "command"), byte.MaxValue);
                UInt64 length = ParseHex(Context, element, "length", RequiredAttribute(Context, element, "length"), 0x100);

                if (length == 0)
                    throw Context.Error(element, "length is zero");
                if (command + length > 0x100)
                    throw Context.Error(element, $"command 0x{command:x2} + length 0x{length:x} exceeds the command space 0x100");

                ranges.Add(new DynDataHw.HwRange { Offset = (UInt16)command, Length = (UInt16)length });
            }

            if (ranges.Count == 0)
                throw Context.Error(Smbus, "<smbus> has no <read>");
            if (ranges.Count > DynDataHw.MaxRanges)
                throw Context.Error(Smbus, $"{ranges.Count} <read> ranges exceeds the cap of {DynDataHw.MaxRanges}");
            if (DynDataHw.SmbusWordReadsPastRange(body.Protocols, slaves.Any(DynDataHw.SmbusSpdSlave), ranges))
                throw Context.Error(Smbus, "a word read on a byte-addressed device (byte and word, or an SPD slave 50h-57h) also returns command + 1; its <read> must be command=\"0x00\" length=\"0x100\"");

            body.RangeCount = (byte)ranges.Count;

            var bytes = new List<byte>();

            bytes.AddRange(DynDataHw.ToBytes(body));
            foreach (var range in ranges)
                bytes.AddRange(DynDataHw.ToBytes(range));

            return BuildOp(Context, Smbus, DynDataHw.KindSmbusRead, DynDataHw.LockSmbus, default, bytes.ToArray());
        }

        /// <summary>
        /// Builds the smbus_host op of an smbus-host element: the PM layout of a PIIX4 host in a range of its PCI revision.
        /// </summary>
        private static byte[] BuildSmbusHostOp(LoadContext Context, LoadedPci Loaded, bool Nested, XElement Host, out UInt32 RevisionLow, out UInt32 RevisionHigh)
        {
            CheckAttributes(Context, Host, SmbusHostAttributes);
            CheckNoChildren(Context, Host);

            if (Nested || Loaded.Class)
                throw Context.Error(Host, "<smbus-host> is only allowed in a top-level <pci> (PCI identity of the host controller)");
            if (!DynDataHw.SmbusPiix4Vendors.Contains(Loaded.VendorId))
                throw Context.Error(Host, $"<smbus-host> is on vendor 0x{Loaded.VendorId:x4}, not in DynDataHw.SmbusPiix4Vendors");

            string[] revisions = RequiredAttribute(Context, Host, "revisions").Split('-');

            if (revisions.Length != 2)
                throw Context.Error(Host, "revisions must be a range 0xLOW-0xHIGH");

            RevisionLow = (UInt32)ParseHex(Context, Host, "revisions", revisions[0].Trim(), byte.MaxValue);
            RevisionHigh = (UInt32)ParseHex(Context, Host, "revisions", revisions[1].Trim(), byte.MaxValue);

            if (RevisionLow > RevisionHigh)
                throw Context.Error(Host, "revisions range is reversed");

            var gate = new DynDataHw.HwGate
            {
                Kind = DynDataHw.GatePciConfig,
                Register = DynDataHw.PciRevisionOffset,
                Shift = 0,
                Width = 8,
                Value = RevisionLow,
                ValueMax = RevisionHigh
            };

            var body = new DynDataHw.HwSmbusHost();
            UInt32 registerEnd = DynDataHw.PortIndexEnd;
            string viewBase = OptionalAttribute(Host, "base");
            string viewLength = OptionalAttribute(Host, "length");
            string enable = OptionalAttribute(Host, "enable");

            void ParseView()
            {
                UInt64 pmBase = ParseHex(Context, Host, "base", RequiredAttribute(Context, Host, "base"), UInt32.MaxValue);
                UInt64 pmLength = ParseHex(Context, Host, "length", RequiredAttribute(Context, Host, "length"), byte.MaxValue);

                if (pmBase == 0 || pmLength == 0)
                    throw Context.Error(Host, "base and length must be non-zero");

                body.PmLength = (byte)pmLength;
                body.PmBase = pmBase;
                registerEnd = (UInt32)pmLength;
            }

            switch (RequiredAttribute(Context, Host, "pm"))
            {
            case "pair":
                body.PmAccess = DynDataHw.SmbusPmPortPair;
                (body.IndexPort, body.DataPort) = ParsePortPair(Context, Host);

                if ((viewBase != null) != (viewLength != null) || (viewBase != null) != (enable != null))
                    throw Context.Error(Host, "a mapped view for pm=\"pair\" needs base, length and enable together");
                if (viewBase != null)
                    ParseView();
                break;
            case "mmio":
                if (OptionalAttribute(Host, "index-port") != null || OptionalAttribute(Host, "data-port") != null || enable != null)
                    throw Context.Error(Host, "index-port, data-port and enable are only for pm=\"pair\"");

                body.PmAccess = DynDataHw.SmbusPmMmio;
                ParseView();
                break;
            case "config":
                foreach (string name in new[] { "index-port", "data-port", "base", "length", "enable-mask", "select", "select-mask", "select-alternate", "select-choose", "select-write" })
                {
                    if (OptionalAttribute(Host, name) != null)
                        throw Context.Error(Host, $"{name} is not for pm=\"config\" (configuration registers, one port, no port select)");
                }

                if (enable == null)
                    throw Context.Error(Host, "pm=\"config\" needs enable=\"0xREGISTER:0xMASK\", the host enable in configuration space");

                body.PmAccess = DynDataHw.SmbusPmConfig;
                registerEnd = DynDataHw.PciConfigLength;
                break;
            default:
                throw Context.Error(Host, "pm must be \"pair\", \"mmio\" or \"config\"");
            }

            bool config = body.PmAccess == DynDataHw.SmbusPmConfig;

            switch (config ? "true" : OptionalAttribute(Host, "select-write") ?? "true")
            {
            case "true":
                break;
            case "false":
                body.Flags |= DynDataHw.SmbusHostFlagSelectReadOnly;
                break;
            default:
                throw Context.Error(Host, "select-write must be \"true\" or \"false\"");
            }

            switch (OptionalAttribute(Host, "semaphore") ?? "true")
            {
            case "true":
                break;
            case "false":
                body.Flags |= DynDataHw.SmbusHostFlagNoSemaphore;
                break;
            default:
                throw Context.Error(Host, "semaphore must be \"true\" or \"false\"");
            }

            byte Register(string Name, string Text)
            {
                UInt64 value = ParseHex(Context, Host, Name, Text.Trim(), byte.MaxValue);

                if (value >= registerEnd)
                    throw Context.Error(Host, $"{Name} 0x{value:x2} is outside the PM registers (end 0x{registerEnd:x})");

                return (byte)value;
            }

            string[] decode = RequiredAttribute(Context, Host, "decode").Split(',');

            if (decode.Length != 2)
                throw Context.Error(Host, "decode must be the low and high PM registers, \"0xLOW,0xHIGH\"");

            body.DecodeLow = Register("decode", decode[0]);
            body.DecodeHigh = Register("decode", decode[1]);
            body.BaseMask = (UInt16)ParseHex(Context, Host, "base-mask", RequiredAttribute(Context, Host, "base-mask"), UInt16.MaxValue);

            if (!config)
            {
                body.EnableMask = (byte)ParseHex(Context, Host, "enable-mask", RequiredAttribute(Context, Host, "enable-mask"), byte.MaxValue);
                body.Select = Register("select", RequiredAttribute(Context, Host, "select"));
                body.SelectMask = (byte)ParseHex(Context, Host, "select-mask", RequiredAttribute(Context, Host, "select-mask"), byte.MaxValue);

                if (body.SelectMask != 0)
                    body.SelectShift = (byte)BitOperations.TrailingZeroCount(body.SelectMask);
            }

            string alternate = OptionalAttribute(Host, "select-alternate");
            string choose = OptionalAttribute(Host, "select-choose");

            if ((alternate == null) != (choose == null))
                throw Context.Error(Host, "select-alternate and select-choose go together");

            if (choose != null)
            {
                string[] parts = choose.Split(':');

                if (parts.Length != 2)
                    throw Context.Error(Host, "select-choose must be \"0xREGISTER:0xMASK\"");

                body.SelectAlternate = Register("select-alternate", alternate);
                body.SelectChoose = Register("select-choose", parts[0]);
                body.SelectChooseMask = (byte)ParseHex(Context, Host, "select-choose", parts[1].Trim(), byte.MaxValue);
            }

            if (enable != null)
            {
                string[] parts = enable.Split(':');

                if (parts.Length != 2)
                    throw Context.Error(Host, "enable must be \"0xREGISTER:0xMASK\"");

                body.GateRegister = Register("enable", parts[0]);
                body.GateMask = (byte)ParseHex(Context, Host, "enable", parts[1].Trim(), byte.MaxValue);

                if (body.GateMask == 0)
                    throw Context.Error(Host, "enable mask is zero");
            }

            string error = DynDataHw.SmbusHostError(body);

            if (error != null)
                throw Context.Error(Host, error);

            return BuildOp(Context, Host, DynDataHw.KindSmbusHost, DynDataHw.LockNone, gate, DynDataHw.ToBytes(body));
        }

        /// <summary>
        /// Builds the vsec_read op of a vsec element; the driver parses the discovery tables itself.
        /// </summary>
        private static byte[] BuildVsecOp(LoadContext Context, LoadedPci Loaded, XElement Vsec)
        {
            CheckAttributes(Context, Vsec, VsecAttributes);

            if (Loaded.Class)
                throw Context.Error(Vsec, "<vsec> is only allowed in a <pci> (PCI identity of the VSEC device)");
            if (Loaded.VendorId != 0x8086)
                throw Context.Error(Vsec, "<vsec> is an Intel VSEC/DVSEC device (vendor 0x8086)");

            var body = new DynDataHw.HwVsecRead();
            string quirks = OptionalAttribute(Vsec, "quirks");
            string guid = OptionalAttribute(Vsec, "guid");
            string feature = OptionalAttribute(Vsec, "feature");
            string requireD0 = OptionalAttribute(Vsec, "require-d0");
            byte flags = DynDataHw.FlagRequireD0;

            switch (RequiredAttribute(Context, Vsec, "discovery"))
            {
            case "pmt-telemetry":
                {
                    if (feature != null)
                        throw Context.Error(Vsec, "feature is only for discovery=\"tpmi\"");
                    if (guid == null)
                        throw Context.Error(Vsec, "discovery=\"pmt-telemetry\" needs guid (0x-prefixed hex, or \"any\")");

                    body.Discovery = DynDataHw.VsecDiscoveryPmtTelemetry;

                    if (guid != "any")
                    {
                        body.Guid = (UInt32)ParseHex(Context, Vsec, "guid", guid, UInt32.MaxValue);

                        if (body.Guid == DynDataHw.VsecGuidAny)
                            throw Context.Error(Vsec, "guid 0x0 is spelled \"any\"");
                    }
                }
                break;
            case "tpmi":
                {
                    if (guid != null)
                        throw Context.Error(Vsec, "guid is only for discovery=\"pmt-telemetry\"");

                    body.Discovery = DynDataHw.VsecDiscoveryTpmi;
                    body.FeatureId = (byte)ParseHex(Context, Vsec, "feature", RequiredAttribute(Context, Vsec, "feature"), byte.MaxValue);

                    //
                    // TPMI_CONTROL is the feature-state mailbox, a write channel.
                    //
                    if (body.FeatureId == DynDataHw.VsecTpmiControl)
                        throw Context.Error(Vsec, "feature 0x80 (TPMI_CONTROL) is a mailbox and is never allowed");
                }
                break;
            default:
                throw Context.Error(Vsec, "discovery must be \"pmt-telemetry\" or \"tpmi\"");
            }

            if (quirks != null)
            {
                foreach (string item in quirks.Split(','))
                {
                    byte quirk = item.Trim() switch
                    {
                        "table-shift" => DynDataHw.VsecQuirkTableShift,
                        "early-hw" => DynDataHw.VsecQuirkEarlyHw,
                        _ => throw Context.Error(Vsec, $"quirk \"{item.Trim()}\" must be \"table-shift\" or \"early-hw\"")
                    };

                    if ((body.Quirks & quirk) != 0)
                        throw Context.Error(Vsec, $"duplicate quirk \"{item.Trim()}\"");

                    body.Quirks |= quirk;
                }

                if ((body.Quirks & DynDataHw.VsecQuirkEarlyHw) != 0 && body.Discovery != DynDataHw.VsecDiscoveryPmtTelemetry)
                    throw Context.Error(Vsec, "quirk \"early-hw\" is only for discovery=\"pmt-telemetry\"");
            }

            body.MaxRegions = (byte)ParseDecimal(Context, Vsec, "max-regions", RequiredAttribute(Context, Vsec, "max-regions"), 1, DynDataHw.VsecMaxRegions);

            UInt64 maxSize = ParseHex(Context, Vsec, "max-size", RequiredAttribute(Context, Vsec, "max-size"), DynDataHw.VsecMaxRegionSize);

            if (maxSize < 4 || (maxSize % 4) != 0)
                throw Context.Error(Vsec, $"max-size 0x{maxSize:x} is not a non-zero multiple of 4");

            body.MaxRegionSize = (UInt32)maxSize;

            string revision = OptionalAttribute(Vsec, "revision");
            string dvsecVendor = OptionalAttribute(Vsec, "dvsec-vendor");

            body.Revision = revision != null ? (byte)ParseDecimal(Context, Vsec, "revision", revision, 0, DynDataHw.VsecRevisionMax) : DynDataHw.VsecRevisionDefault;
            body.DvsecVendor = dvsecVendor != null ? (UInt16)ParseHex(Context, Vsec, "dvsec-vendor", dvsecVendor, UInt16.MaxValue) : DynDataHw.VsecDvsecVendorDefault;

            if (requireD0 != null)
            {
                if (requireD0 == "true")
                    flags |= DynDataHw.FlagRequireD0;
                else if (requireD0 == "false")
                    flags &= unchecked((byte)~DynDataHw.FlagRequireD0);
                else
                    throw Context.Error(Vsec, "require-d0 must be \"true\" or \"false\"");
            }

            flags |= ParseArm64(Context, Vsec, true);

            var ranges = new List<DynDataHw.HwWindowRange>();

            foreach (var element in Vsec.Elements())
            {
                if (element.Name.LocalName != "read")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, WindowReadAttributes);
                CheckNoChildren(Context, element);

                UInt64 offset = ParseHex(Context, element, "offset", RequiredAttribute(Context, element, "offset"), DynDataHw.VsecMaxRegionSize);
                UInt64 length = ParseHex(Context, element, "length", RequiredAttribute(Context, element, "length"), DynDataHw.VsecMaxRegionSize);
                UInt64 width = ParseDecimal(Context, element, "width", RequiredAttribute(Context, element, "width"), 4, 8);

                if (width != 4 && width != 8)
                    throw Context.Error(element, $"width {width} is not 4 or 8");
                if ((offset % width) != 0 || length < width || (length % width) != 0)
                    throw Context.Error(element, $"offset 0x{offset:x} / length 0x{length:x} not aligned to width {width}");
                if (offset + length > maxSize)
                    throw Context.Error(element, $"offset 0x{offset:x} + length 0x{length:x} exceeds max-size 0x{maxSize:x}");

                ranges.Add(new DynDataHw.HwWindowRange { Offset = (UInt32)offset, Length = (UInt32)length, Width = (byte)width });
            }

            if (ranges.Count == 0)
                throw Context.Error(Vsec, "<vsec> has no <read>");
            if (ranges.Count > DynDataHw.MaxRanges)
                throw Context.Error(Vsec, $"{ranges.Count} <read> ranges exceeds the cap of {DynDataHw.MaxRanges}");

            body.RangeCount = (byte)ranges.Count;

            var bytes = new List<byte>();

            bytes.AddRange(DynDataHw.ToBytes(body));
            foreach (var range in ranges)
                bytes.AddRange(DynDataHw.ToBytes(range));

            return BuildOp(Context, Vsec, DynDataHw.KindVsecRead, DynDataHw.LockNone, default, bytes.ToArray(), flags);
        }

        /// <summary>
        /// Loads a superio element: one SUPERIO row per slot, chip ID and customer ID, sharing one superio_read table.
        /// </summary>
        private static void LoadSuperIo(LoadContext Context, XElement SuperIo)
        {
            CheckAttributes(Context, SuperIo, SuperIoAttributes);

            byte family = RequiredAttribute(Context, SuperIo, "family") switch
            {
                "nuvoton" => DynDataHw.SuperIoFamilyNuvoton,
                "nuvoton-ec" => DynDataHw.SuperIoFamilyNuvotonEc,
                "ite" => DynDataHw.SuperIoFamilyIte,
                "fintek" => DynDataHw.SuperIoFamilyFintek,
                "smsc" => DynDataHw.SuperIoFamilySmsc,
                _ => throw Context.Error(SuperIo, "family must be \"nuvoton\", \"nuvoton-ec\", \"ite\", \"fintek\" or \"smsc\"")
            };
            bool flat = family == DynDataHw.SuperIoFamilyIte || family == DynDataHw.SuperIoFamilyFintek || family == DynDataHw.SuperIoFamilySmsc;
            var slots = new List<byte>();

            foreach (string item in RequiredAttribute(Context, SuperIo, "slots").Split(','))
            {
                byte slot = (byte)ParseDecimal(Context, SuperIo, "slots", item.Trim(), 0, DynDataHw.SuperIoSlots - 1u);

                if (slots.Contains(slot))
                    throw Context.Error(SuperIo, $"duplicate slot {slot} in slots");

                slots.Add(slot);
            }

            if (family == DynDataHw.SuperIoFamilySmsc && OptionalAttribute(SuperIo, "mask") != null)
                throw Context.Error(SuperIo, "mask is not for family=\"smsc\" (8-bit IDs, no revision)");

            UInt16 mask = (UInt16)ParseHex(Context, SuperIo, "mask", OptionalAttribute(SuperIo, "mask") ?? "0xffff", UInt16.MaxValue);
            UInt16 revisions = (UInt16)~mask;

            if (revisions > 0xF || (revisions & (revisions + 1)) != 0)
                throw Context.Error(SuperIo, $"mask 0x{mask:x4} must clear only low bits, at most four");

            var chips = new List<UInt16>();

            foreach (string item in RequiredAttribute(Context, SuperIo, "chips").Split(','))
            {
                UInt16 chip = (UInt16)ParseHex(Context, SuperIo, "chips", item.Trim(), family == DynDataHw.SuperIoFamilySmsc ? byte.MaxValue : UInt16.MaxValue);

                if ((chip & revisions) != 0)
                    throw Context.Error(SuperIo, $"chip 0x{chip:x4} has bits outside the mask 0x{mask:x4}");

                for (UInt32 low = 0; low <= revisions; low++)
                {
                    UInt16 id = (UInt16)(chip | low);

                    if (id == 0 || id == UInt16.MaxValue || (family == DynDataHw.SuperIoFamilySmsc && id == byte.MaxValue))
                        throw Context.Error(SuperIo, $"chip ID 0x{id:x4} is what an absent chip reads");
                    if (family == DynDataHw.SuperIoFamilySmsc && !DynDataHw.SuperIoSmscChips.Contains(id))
                        throw Context.Error(SuperIo, $"SMSC chip 0x{id:x2} is not in DynDataHw.SuperIoSmscChips (the driver maps the chip to its HWM device ID)");
                    if (chips.Contains(id))
                        throw Context.Error(SuperIo, $"duplicate chip ID 0x{id:x4} in chips");

                    chips.Add(id);
                }
            }

            var customers = new List<UInt16>();
            string customersText = OptionalAttribute(SuperIo, "customers");

            if (family == DynDataHw.SuperIoFamilyNuvotonEc)
            {
                if (customersText == null)
                    throw Context.Error(SuperIo, "customers is required for family=\"nuvoton-ec\"");

                foreach (string item in customersText.Split(','))
                {
                    UInt16 customer = (UInt16)ParseHex(Context, SuperIo, "customers", item.Trim(), UInt16.MaxValue);

                    if (customers.Contains(customer))
                        throw Context.Error(SuperIo, $"duplicate customer 0x{customer:x4} in customers");

                    customers.Add(customer);
                }
            }
            else
            {
                if (customersText != null)
                    throw Context.Error(SuperIo, "customers is only for family=\"nuvoton-ec\"");

                customers.Add(0);
            }

            var body = new DynDataHw.HwSuperIoRead
            {
                Ldn = (byte)ParseHex(Context, SuperIo, "ldn", RequiredAttribute(Context, SuperIo, "ldn"), byte.MaxValue),
                Exit = family == DynDataHw.SuperIoFamilyIte ? DynDataHw.SuperIoExitConfig02 : DynDataHw.SuperIoExitKeyAa
            };

            if (family == DynDataHw.SuperIoFamilyNuvotonEc && body.Ldn != DynDataHw.SuperIoEcLdn)
                throw Context.Error(SuperIo, $"family=\"nuvoton-ec\" needs ldn=\"0x{DynDataHw.SuperIoEcLdn:x2}\"");

            //
            // Only the hardware monitor device; another device's base would be used as the index/data pair.
            //
            byte[] ldns = family switch
            {
                DynDataHw.SuperIoFamilyNuvoton or DynDataHw.SuperIoFamilyNuvotonEc => [0x0B],
                DynDataHw.SuperIoFamilyIte => [0x04],
                DynDataHw.SuperIoFamilySmsc => [0x0A],
                _ => [0x04, 0x02]
            };

            if (!ldns.Contains(body.Ldn))
                throw Context.Error(SuperIo, $"ldn 0x{body.Ldn:x2} is not the hardware monitor device of the family ({string.Join(", ", ldns.Select(l => $"0x{l:x2}"))})");

            //
            // Some ITE chips are left in configuration mode by firmware and must never be exited.
            //
            string exit = OptionalAttribute(SuperIo, "exit");
            bool noConfig = family == DynDataHw.SuperIoFamilyIte && chips.Any(c => DynDataHw.SuperIoNoConfigChips.Contains(c));

            if (noConfig && !chips.All(c => DynDataHw.SuperIoNoConfigChips.Contains(c)))
                throw Context.Error(SuperIo, "no-exit chips (0x8790, 0x8733, 0x8695) need their own <superio>");

            if (exit != null)
            {
                if (exit != "none")
                    throw Context.Error(SuperIo, "exit may only be \"none\"");
                if (!noConfig)
                    throw Context.Error(SuperIo, "exit=\"none\" is only for the ITE chips 0x8790, 0x8733, 0x8695");

                body.Exit = DynDataHw.SuperIoExitNone;
            }
            else if (noConfig)
            {
                throw Context.Error(SuperIo, "no-exit chips (0x8790, 0x8733, 0x8695) need exit=\"none\"");
            }

            string bankMask = OptionalAttribute(SuperIo, "bank-mask");

            if ((bankMask != null) != (family == DynDataHw.SuperIoFamilyNuvoton))
                throw Context.Error(SuperIo, "bank-mask is required for family=\"nuvoton\" and only for it");
            if (bankMask != null)
            {
                body.BankMask = (byte)ParseHex(Context, SuperIo, "bank-mask", bankMask, byte.MaxValue);

                if ((body.BankMask & DynDataHw.SuperIoNuvotonHbacs) == 0)
                    throw Context.Error(SuperIo, $"bank-mask 0x{body.BankMask:x2} does not include bit 7 (HBACS)");
            }

            string configId = OptionalAttribute(SuperIo, "config-id");

            if (configId != null)
            {
                string[] parts = configId.Split(':');

                if (family != DynDataHw.SuperIoFamilyFintek)
                    throw Context.Error(SuperIo, "config-id is only for family=\"fintek\"");
                if (parts.Length != 2)
                    throw Context.Error(SuperIo, "config-id must be \"0xINDEX:0xVALUE\"");

                body.ConfigIdIndex = (byte)ParseHex(Context, SuperIo, "config-id", parts[0].Trim(), byte.MaxValue - 1);
                body.ConfigIdValue = (UInt16)ParseHex(Context, SuperIo, "config-id", parts[1].Trim(), UInt16.MaxValue);

                if (body.ConfigIdIndex == 0)
                    throw Context.Error(SuperIo, "config-id index is zero");
            }

            string customerReg = OptionalAttribute(SuperIo, "customer-reg");

            if ((customerReg != null) != (family == DynDataHw.SuperIoFamilyNuvotonEc))
                throw Context.Error(SuperIo, "customer-reg is required for family=\"nuvoton-ec\" and only for it");
            if (customerReg != null)
            {
                body.CustomerRegister = (UInt16)ParseHex(Context, SuperIo, "customer-reg", customerReg, UInt16.MaxValue);

                if ((body.CustomerRegister & 0xFF) == 0xFF || (body.CustomerRegister >> 8) == 0xFF)
                    throw Context.Error(SuperIo, $"customer-reg 0x{body.CustomerRegister:x4} must be a register pair in a page other than FFh");
            }

            switch (OptionalAttribute(SuperIo, "require-active") ?? "true")
            {
            case "true":
                break;
            case "false":
                if (family != DynDataHw.SuperIoFamilySmsc)
                    throw Context.Error(SuperIo, "require-active=\"false\" is only for family=\"smsc\"");

                body.Flags |= DynDataHw.SuperIoFlagActiveOptional;
                break;
            default:
                throw Context.Error(SuperIo, "require-active must be \"true\" or \"false\"");
            }

            string port = OptionalAttribute(SuperIo, "port");

            if (port != null && slots.Count != 1)
                throw Context.Error(SuperIo, "port is only for a <superio> of one slot (a row that lists both slots uses the defaults)");

            UInt16 portValue = port != null ? (UInt16)ParseHex(Context, SuperIo, "port", port, UInt16.MaxValue - 1) : (UInt16)0;

            if (port != null && portValue == 0)
                throw Context.Error(SuperIo, "port is zero");

            //
            // The HWM base read from the chip must start in this range; the driver refuses any other block.
            //
            string baseText = OptionalAttribute(SuperIo, "base");

            if (baseText != null)
            {
                string[] bounds = baseText.Split('-');

                if (bounds.Length != 2)
                    throw Context.Error(SuperIo, $"malformed base \"{baseText}\" (0xMIN-0xMAX)");

                body.BaseMin = (UInt16)ParseHex(Context, SuperIo, "base", bounds[0].Trim(), UInt16.MaxValue);
                body.BaseMax = (UInt16)ParseHex(Context, SuperIo, "base", bounds[1].Trim(), UInt16.MaxValue);
            }
            else
            {
                body.BaseMin = DynDataHw.SuperIoBaseMinDefault;
                body.BaseMax = DynDataHw.SuperIoBaseMaxDefault(family);
            }

            string baseError = DynDataHw.SuperIoBaseRangeError(body.BaseMin, body.BaseMax, DynDataHw.SuperIoBaseExtent(family));

            if (baseError != null)
                throw Context.Error(SuperIo, baseError);

            var ranges = new List<DynDataHw.HwRange>();
            var ids = new List<DynDataHw.HwSuperIoId>();

            UInt64 Register(XElement Element, UInt64 Length)
            {
                UInt64 reg = ParseHex(Context, Element, "reg", RequiredAttribute(Context, Element, "reg"), flat ? byte.MaxValue : UInt16.MaxValue);

                if ((reg & 0xFF) + Length > 0x100)
                    throw Context.Error(Element, $"reg 0x{reg:x} + length 0x{Length:x} crosses a {(flat ? "register space" : "bank")} boundary");
                if (family == DynDataHw.SuperIoFamilyNuvotonEc && (reg >> 8) == 0xFF)
                    throw Context.Error(Element, $"reg 0x{reg:x} is in page FFh, the NCT668x page register's free value");
                if (family == DynDataHw.SuperIoFamilyNuvoton && ((reg >> 8) & ~(UInt64)body.BankMask) != 0)
                    throw Context.Error(Element, $"reg 0x{reg:x} is in bank 0x{reg >> 8:x2}, outside bank-mask 0x{body.BankMask:x2}");
                foreach (UInt16 clear in DynDataHw.SuperIoReadToClear(family))
                {
                    if (clear >= reg && clear < reg + Length)
                        throw Context.Error(Element, $"reg 0x{reg:x} + length 0x{Length:x} covers the read-to-clear register 0x{clear:x2}");
                }

                return reg;
            }

            foreach (var element in SuperIo.Elements())
            {
                if (element.Name.LocalName == "id")
                {
                    CheckAttributes(Context, element, SuperIoIdAttributes);
                    CheckNoChildren(Context, element);

                    var id = new DynDataHw.HwSuperIoId
                    {
                        Register = (UInt16)Register(element, 1),
                        Value = (byte)ParseHex(Context, element, "value", RequiredAttribute(Context, element, "value"), byte.MaxValue)
                    };
                    string alternate = OptionalAttribute(element, "alt");

                    id.Alternate = alternate != null ? (byte)ParseHex(Context, element, "alt", alternate, byte.MaxValue) : id.Value;

                    if (ids.Any(other => other.Register == id.Register))
                        throw Context.Error(element, $"duplicate id reg 0x{id.Register:x}");
                    if (ids.Count == DynDataHw.SuperIoMaxIds)
                        throw Context.Error(element, $"more than {DynDataHw.SuperIoMaxIds} <id>");

                    ids.Add(id);
                    continue;
                }

                if (element.Name.LocalName != "read")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, SuperIoReadAttributes);
                CheckNoChildren(Context, element);

                UInt64 length = ParseHex(Context, element, "length", RequiredAttribute(Context, element, "length"), 0x100);

                if (length == 0)
                    throw Context.Error(element, "length is zero");

                UInt64 reg = Register(element, length);

                ranges.Add(new DynDataHw.HwRange { Offset = (UInt16)reg, Length = (UInt16)length });
            }

            if (ranges.Count == 0)
                throw Context.Error(SuperIo, "<superio> has no <read>");
            if (ranges.Count > DynDataHw.MaxRanges)
                throw Context.Error(SuperIo, $"{ranges.Count} <read> ranges exceeds the cap of {DynDataHw.MaxRanges}");
            if ((body.Flags & DynDataHw.SuperIoFlagActiveOptional) != 0 && ids.Count == 0)
                throw Context.Error(SuperIo, "require-active=\"false\" needs an <id> (the identity bytes at the base are the check)");

            body.RangeCount = (byte)ranges.Count;
            body.IdCount = (byte)ids.Count;

            //
            // The driver never probes a slot outside the listed chassis types.
            //
            DynDataHw.HwGate gate = default;
            string chassis = OptionalAttribute(SuperIo, "chassis");

            if (chassis != null)
            {
                UInt64 types = 0;

                foreach (string item in chassis.Split(','))
                {
                    string[] bounds = item.Trim().Split('-');

                    if (bounds.Length > 2)
                        throw Context.Error(SuperIo, $"malformed chassis item \"{item}\"");

                    UInt64 low = ParseHex(Context, SuperIo, "chassis", bounds[0].Trim(), 63);
                    UInt64 high = bounds.Length == 2 ? ParseHex(Context, SuperIo, "chassis", bounds[1].Trim(), 63) : low;

                    if (low == 0 || low > high)
                        throw Context.Error(SuperIo, $"chassis item \"{item}\" is zero or reversed");

                    for (UInt64 type = low; type <= high; type++)
                        types |= 1ul << (int)type;
                }

                gate.Kind = DynDataHw.GateChassis;
                gate.Value = (UInt32)types;
                gate.ValueMax = (UInt32)(types >> 32);
            }

            foreach (byte slot in slots)
            {
                body.ConfigPort = port != null ? portValue : DynDataHw.SuperIoPortDefault(slot);

                byte[] fixedBytes = DynDataHw.ToBytes(body);

                for (int i = 0; i < ids.Count; i++)
                    DynDataHw.ToBytes(ids[i]).CopyTo(fixedBytes, DynDataHw.SuperIoReadSize - DynDataHw.SuperIoMaxIds * DynDataHw.SuperIoIdSize + i * DynDataHw.SuperIoIdSize);

                var bytes = new List<byte>(fixedBytes);

                foreach (var range in ranges)
                    bytes.AddRange(DynDataHw.ToBytes(range));

                byte[] table = BuildTable(Context, SuperIo, [BuildOp(Context, SuperIo, DynDataHw.KindSuperIoRead, DynDataHw.LockIsaBus, gate, bytes.ToArray())]);

                Context.AddSuperIoPort(SuperIo, slot, body.ConfigPort);

                foreach (UInt16 chip in chips)
                {
                    Context.AddSuperIoChip(SuperIo, slot, family, chip, body);

                    foreach (UInt16 customer in customers)
                    {
                        var identity = new DynDataHw.HwIdentity();

                        identity.SuperIo.Slot = slot;
                        identity.SuperIo.VendorFamily = family;
                        identity.SuperIo.ChipId = chip;
                        identity.SuperIo.CustomerId = customer;

                        Context.AddRow(SuperIo, DynDataHw.IdentitySuperIo, DynDataHw.ToBytes(identity), table);
                    }
                }
            }
        }

        /// <summary>
        /// Loads an acpi element: one ACPI_HID row per id= list item, each holding the eval ops and windows in
        /// document order. Generator-only: the list expands to identical rows that share one table.
        /// </summary>
        private static void LoadAcpi(LoadContext Context, XElement Acpi)
        {
            CheckAttributes(Context, Acpi, AcpiAttributes);

            string[] ids = RequiredAttribute(Context, Acpi, "id").Split(',').Select(s => s.Trim()).ToArray();

            if (ids.Distinct().Count() != ids.Length)
                throw Context.Error(Acpi, "duplicate id in the id list");

            foreach (string id in ids)
                LoadAcpiId(Context, Acpi, id);
        }

        /// <summary>
        /// Loads the ACPI_HID row of one id of an acpi element.
        /// </summary>
        private static void LoadAcpiId(LoadContext Context, XElement Acpi, string id)
        {
            if (!DynDataHw.IsAcpiId(id))
                throw Context.Error(Acpi, $"id \"{id}\" is not a 7-character EISA or 8-character ACPI ID of [A-Z0-9_]");

            var ops = new List<byte[]>();
            var evals = new Dictionary<string, XElement>();
            var mailboxes = new List<XElement>();
            bool gated = false;

            void AddEval(XElement Element, string[] Required)
            {
                byte[] op = BuildAcpiEvalOp(Context, id, Element, Required);
                string key = Convert.ToHexString(op.AsSpan(DynDataHw.OpHeaderSize + 4, 1)) +
                             Convert.ToHexString(op.AsSpan(DynDataHw.OpHeaderSize)[..4]) +
                             Convert.ToHexString(op.AsSpan(DynDataHw.OpHeaderSize + DynDataHw.AcpiEvalSize));

                if (evals.TryGetValue(key, out var other))
                    throw Context.Error(Element, $"duplicate <eval> method and arguments, also on line {((IXmlLineInfo)other).LineNumber}");

                evals.Add(key, Element);
                ops.Add(op);
            }

            foreach (var element in Acpi.Elements())
            {
                switch (element.Name.LocalName)
                {
                case "require":
                    {
                        CheckAttributes(Context, element, RequireAttributes);

                        string[] required = RequiredAttribute(Context, element, "id").Split(',').Select(s => s.Trim()).ToArray();

                        if (required.Distinct().Count() != required.Length)
                            throw Context.Error(element, "duplicate id in the id list");
                        if (required.Length > byte.MaxValue)
                            throw Context.Error(element, $"more than {byte.MaxValue} ids");

                        foreach (string item in required)
                        {
                            if (!DynDataHw.IsAcpiId(item))
                                throw Context.Error(element, $"id \"{item}\" is not a 7-character EISA or 8-character ACPI ID of [A-Z0-9_]");
                        }

                        if (!element.Elements().Any())
                            throw Context.Error(element, "<require> has no <eval>");

                        ops.Add(BuildAcpiRequireOp(Context, element, required));

                        foreach (var child in element.Elements())
                        {
                            if (child.Name.LocalName != "eval")
                                throw Context.Error(child, "unknown element (a <require> holds <eval> only)");

                            AddEval(child, required);
                        }

                        gated = true;
                    }
                    break;
                case "mailbox":
                    {
                        if (mailboxes.Count > byte.MaxValue)
                            throw Context.Error(element, "more than 256 <mailbox>");

                        var mailbox = LoadMmioMailbox(Context, id, element, mailboxes.Count);

                        if (mailbox.Name != null && mailboxes.Any(m => OptionalAttribute(m, "name") == mailbox.Name))
                            throw Context.Error(element, $"duplicate <mailbox> name \"{mailbox.Name}\"");

                        mailboxes.Add(element);
                        ops.Add(mailbox.Op);
                    }
                    break;
                case "eval":
                    if (gated)
                        throw Context.Error(element, "an <eval> after a <require> is gated by it; ungated <eval> go before the first <require>");

                    AddEval(element, null);
                    break;
                case "window":
                    ops.Add(BuildWindowOp(Context, element, WindowPlacement.Acpi));
                    break;
                default:
                    throw Context.Error(element, "unknown element");
                }
            }

            if (ops.Count == 0)
                throw Context.Error(Acpi, "<acpi> has no <eval>, no <window> and no <mailbox>");

            //
            // A device with an MMIO mailbox evaluates nothing; what AML touches is unknown.
            //
            if (mailboxes.Count != 0 && evals.Count != 0)
                throw Context.Error(Acpi, "an <acpi> with a <mailbox> has no <eval>");

            var identity = new byte[DynDataHw.IdentitySize];
            byte[] table = BuildTable(Context, Acpi, ops);

            Encoding.ASCII.GetBytes(id, identity.AsSpan(0, DynDataHw.AcpiIdSize));

            if (mailboxes.Count != 0)
            {
                try
                {
                    HwSection.CheckMailboxRefs(table, -1);
                }
                catch (InvalidDataException ex)
                {
                    throw Context.Error(Acpi, ex.Message);
                }
            }

            Context.AddRow(Acpi, DynDataHw.IdentityAcpiHid, identity, table);
        }

        /// <summary>
        /// Builds the acpi_eval op of an eval element.
        /// </summary>
        private static byte[] BuildAcpiEvalOp(LoadContext Context, string AcpiId, XElement Eval, string[] Required)
        {
            CheckAttributes(Context, Eval, EvalAttributes);

            string method = RequiredAttribute(Context, Eval, "method");
            UInt64 maxOutput = ParseHex(Context, Eval, "max-output", RequiredAttribute(Context, Eval, "max-output"), DynDataHw.AcpiMaxOutput);
            byte results = 0;

            if (!DynDataHw.IsAcpiNameSeg(method))
                throw Context.Error(Eval, $"method \"{method}\" is not an ACPI NameSeg ([A-Z_][A-Z0-9_]{{3}})");
            if (!DynDataHw.AcpiMethodAllowed(AcpiId, method))
                throw Context.Error(Eval, $"method \"{method}\" is not a read-only spec method nor a DynDataHw.AcpiVendorMethods entry of {AcpiId}");
            if (maxOutput < DynDataHw.AcpiMinOutput)
                throw Context.Error(Eval, $"max-output 0x{maxOutput:x} is below 0x{DynDataHw.AcpiMinOutput:x}");

            string requireError = DynDataHw.AcpiMethodRequireError(AcpiId, method, Required);

            if (requireError != null)
                throw Context.Error(Eval, requireError);

            foreach (string item in RequiredAttribute(Context, Eval, "results").Split('|'))
            {
                byte type = item.Trim() switch
                {
                    "integer" => DynDataHw.AcpiResultInteger,
                    "string" => DynDataHw.AcpiResultString,
                    "buffer" => DynDataHw.AcpiResultBuffer,
                    "package" => DynDataHw.AcpiResultPackage,
                    _ => throw Context.Error(Eval, $"results item \"{item}\" is not integer, string, buffer or package")
                };

                if ((results & type) != 0)
                    throw Context.Error(Eval, $"duplicate results item \"{item}\"");

                results |= type;
            }

            var args = new DynDataHw.HwAcpiArg?[DynDataHw.AcpiMaxArgs];

            foreach (var element in Eval.Elements())
            {
                if (element.Name.LocalName != "arg")
                    throw Context.Error(element, "unknown element");

                CheckAttributes(Context, element, ArgAttributes);
                CheckNoChildren(Context, element);

                int index = (int)ParseDecimal(Context, element, "index", RequiredAttribute(Context, element, "index"), 0, DynDataHw.AcpiMaxArgs - 1u);
                string maskText = OptionalAttribute(element, "mask");
                string maxText = OptionalAttribute(element, "max");
                string fixedText = OptionalAttribute(element, "fixed");

                if (args[index] != null)
                    throw Context.Error(element, $"duplicate arg {index}");
                if (maskText == null && fixedText == null)
                    throw Context.Error(element, "an <arg> needs mask or fixed");
                if (maskText == null && maxText != null)
                    throw Context.Error(element, "max needs mask");

                var arg = new DynDataHw.HwAcpiArg
                {
                    Mask = maskText != null ? (UInt32)ParseHex(Context, element, "mask", maskText, UInt32.MaxValue) : 0,
                    Fixed = fixedText != null ? (UInt32)ParseHex(Context, element, "fixed", fixedText, UInt32.MaxValue) : 0
                };

                arg.Max = maxText != null ? (UInt32)ParseHex(Context, element, "max", maxText, UInt32.MaxValue) : arg.Mask;

                if ((arg.Max & ~arg.Mask) != 0)
                    throw Context.Error(element, $"max 0x{arg.Max:x} has bits outside mask 0x{arg.Mask:x}");
                if ((arg.Fixed & arg.Mask) != 0)
                    throw Context.Error(element, $"fixed 0x{arg.Fixed:x} has bits inside mask 0x{arg.Mask:x}");

                args[index] = arg;
            }

            int argCount = Array.FindLastIndex(args, a => a != null) + 1;

            for (int i = 0; i < argCount; i++)
            {
                if (args[i] == null)
                    throw Context.Error(Eval, $"arg {i} is missing (arguments are positional from 0)");
            }

            string argsError = DynDataHw.AcpiMethodArgsError(AcpiId, method, args.Take(argCount).Select(a => a.Value).ToArray());

            if (argsError != null)
                throw Context.Error(Eval, argsError);

            var body = new DynDataHw.HwAcpiEval
            {
                Method = BinaryPrimitives.ReadUInt32LittleEndian(Encoding.ASCII.GetBytes(method)),
                ArgCount = (byte)argCount,
                ResultTypes = results,
                MaxOutput = (UInt16)maxOutput
            };

            var bytes = new List<byte>();

            bytes.AddRange(DynDataHw.ToBytes(body));
            for (int i = 0; i < argCount; i++)
                bytes.AddRange(DynDataHw.ToBytes(args[i].Value));

            return BuildOp(Context, Eval, DynDataHw.KindAcpiEval, DynDataHw.LockNone, default, bytes.ToArray());
        }

        /// <summary>
        /// Builds the acpi_require op of a require element.
        /// </summary>
        private static byte[] BuildAcpiRequireOp(LoadContext Context, XElement Require, string[] Ids)
        {
            var bytes = new List<byte>();

            bytes.AddRange(DynDataHw.ToBytes(new DynDataHw.HwAcpiRequire { IdCount = (byte)Ids.Length }));

            foreach (string id in Ids)
            {
                var buffer = new byte[DynDataHw.AcpiIdSize];

                Encoding.ASCII.GetBytes(id, buffer);
                bytes.AddRange(buffer);
            }

            return BuildOp(Context, Require, DynDataHw.KindAcpiRequire, DynDataHw.LockNone, default, bytes.ToArray());
        }

        /// <summary>
        /// Loads a board element: one BOARD row keyed by <see cref="DynDataHw.BoardHash"/>, holding only fixed windows.
        /// </summary>
        private static void LoadBoard(LoadContext Context, XElement Board)
        {
            CheckAttributes(Context, Board, BoardAttributes);

            string smbios = RequiredAttribute(Context, Board, "smbios");
            string manufacturer = RequiredAttribute(Context, Board, "manufacturer");
            string product = RequiredAttribute(Context, Board, "product");
            var ops = new List<byte[]>();

            if (smbios != "baseboard" && smbios != "system")
                throw Context.Error(Board, "smbios must be \"baseboard\" (type 2) or \"system\" (type 1)");

            foreach (var (name, text) in new[] { ("manufacturer", manufacturer), ("product", product) })
            {
                if (!text.All(c => c >= 0x20 && c <= 0x7E))
                    throw Context.Error(Board, $"{name} \"{text}\" is not printable ASCII (the strings as SMBIOS stores them)");
                if (text.Trim(' ', '\t').Length == 0)
                    throw Context.Error(Board, $"{name} is empty after trimming");
            }

            UInt64 hash = DynDataHw.BoardHash(manufacturer, product);

            Context.AddBoardHash(Board, hash, manufacturer, product);

            foreach (var element in Board.Elements())
            {
                switch (element.Name.LocalName)
                {
                case "window":
                    ops.Add(BuildWindowOp(Context, element, WindowPlacement.Board));
                    break;
                case "msr":
                    throw Context.Error(element, "<msr> is only allowed in a <cpu>");
                default:
                    throw Context.Error(element, "unknown element");
                }
            }

            if (ops.Count == 0)
                throw Context.Error(Board, "<board> has no <window>");

            var identity = new DynDataHw.HwIdentity();

            if (smbios == "baseboard")
                identity.Board.BaseboardHash = hash;
            else
                identity.Board.SystemHash = hash;

            Context.AddRow(Board, DynDataHw.IdentityBoard, DynDataHw.ToBytes(identity), BuildTable(Context, Board, ops));
        }

        /// <summary>
        /// Loads an arm bucket: one ARM_CPU row per implementer, part, variant and revision, sharing one sysreg table.
        /// </summary>
        /// <remarks>MIDR_EL1 is read per processor, so a part whose cores differ by variant needs every variant.</remarks>
        private static void LoadArm(LoadContext Context, XElement Arm)
        {
            CheckAttributes(Context, Arm, ArmAttributes);

            byte implementer = (byte)ParseHex(Context, Arm, "implementer", RequiredAttribute(Context, Arm, "implementer"), byte.MaxValue);
            List<UInt64> parts = ParseHexList(Context, Arm, "parts", DynDataHw.ArmMaxPartNum);
            List<UInt64> variants = ParseHexList(Context, Arm, "variants", DynDataHw.ArmMaxVariant);
            List<UInt64> revisions = ParseHexList(Context, Arm, "revisions", DynDataHw.ArmMaxRevision);
            long identities = (long)parts.Count * variants.Count * revisions.Count;
            var ops = new List<byte[]>();

            if (identities > DynDataHw.ArmMaxBucketIdentities)
                throw Context.Error(Arm, $"parts x variants x revisions expands to {identities} rows, more than {DynDataHw.ArmMaxBucketIdentities}");
            var registers = new HashSet<byte>();

            foreach (var element in Arm.Elements())
            {
                switch (element.Name.LocalName)
                {
                case "sysreg":
                    {
                        CheckAttributes(Context, element, SysregAttributes);
                        CheckNoChildren(Context, element);

                        string name = RequiredAttribute(Context, element, "reg");

                        if (!DynDataHw.SysregNames.TryGetValue(name, out byte register))
                            throw Context.Error(element, $"unknown reg \"{name}\" (expected one of {string.Join(", ", DynDataHw.SysregNames.OrderBy(n => n.Value).Select(n => n.Key))})");
                        if (!registers.Add(register))
                            throw Context.Error(element, $"duplicate sysreg reg \"{name}\" in this table");

                        var sysreg = new DynDataHw.HwSysregRead
                        {
                            SysregEnum = register,
                            ReadMask = UInt64.MaxValue
                        };

                        string readMask = OptionalAttribute(element, "readmask");

                        if (readMask != null)
                        {
                            sysreg.ReadMask = ParseHex(Context, element, "readmask", readMask, UInt64.MaxValue);

                            if (sysreg.ReadMask == 0)
                                throw Context.Error(element, "readmask 0x0 reads nothing");
                        }

                        ops.Add(BuildOp(Context, element, DynDataHw.KindSysregRead, DynDataHw.LockNone, ParseGate(Context, element, true), DynDataHw.ToBytes(sysreg)));
                    }
                    break;
                case "msr":
                    throw Context.Error(element, "<msr> is only allowed in a <cpu>");
                default:
                    throw Context.Error(element, "unknown element");
                }
            }

            if (ops.Count == 0)
                throw Context.Error(Arm, "<arm> has no <sysreg>");

            byte[] table = BuildTable(Context, Arm, ops);

            foreach (UInt64 part in parts)
            {
                foreach (UInt64 variant in variants)
                {
                    foreach (UInt64 revision in revisions)
                    {
                        var identity = new DynDataHw.HwIdentity();

                        identity.ArmCpu.Implementer = implementer;
                        identity.ArmCpu.Variant = (byte)variant;
                        identity.ArmCpu.PartNum = (UInt16)part;
                        identity.ArmCpu.Revision = (byte)revision;

                        Context.AddRow(Arm, DynDataHw.IdentityArmCpu, DynDataHw.ToBytes(identity), table);
                    }
                }
            }
        }

        /// <summary>
        /// Builds an op: the 32-byte header followed by the body.
        /// </summary>
        private static byte[] BuildOp(LoadContext Context, XElement Element, UInt16 Kind, byte LockClass, DynDataHw.HwGate Gate, byte[] Body, byte Flags = 0)
        {
            // The body is zero-padded so the next op stays aligned.
            int length = DynDataHw.AlignOp(DynDataHw.OpHeaderSize + Body.Length);

            if (length > UInt16.MaxValue)
                throw Context.Error(Element, $"op length {length} exceeds 0xFFFF");

            var header = new DynDataHw.HwOpHeader
            {
                Kind = Kind,
                Length = (UInt16)length,
                LockClass = LockClass,
                Flags = Flags,
                Gate = Gate
            };

            return [.. DynDataHw.ToBytes(header), .. Body, .. new byte[length - DynDataHw.OpHeaderSize - Body.Length]];
        }

        /// <summary>
        /// Parses arm64="true|false" (default false): whether ARM64 builds may run a PCI window or VSEC op.
        /// </summary>
        private static byte ParseArm64(LoadContext Context, XElement Element, bool Allowed)
        {
            string arm64 = OptionalAttribute(Element, "arm64");

            if (arm64 == null || arm64 == "false")
                return 0;
            if (arm64 != "true")
                throw Context.Error(Element, "arm64 must be \"true\" or \"false\"");
            if (!Allowed)
                throw Context.Error(Element, "arm64 is only an attribute of a base=\"bar\" or base=\"config\" window and of a <vsec>");

            return DynDataHw.FlagArm64;
        }

        private static List<byte[]> SplitTable(byte[] Table)
        {
            var ops = new List<byte[]>();
            UInt32 count = BinaryPrimitives.ReadUInt32LittleEndian(Table);
            int offset = DynDataHw.TableSize;

            for (UInt32 i = 0; i < count; i++)
            {
                int length = BinaryPrimitives.ReadUInt16LittleEndian(Table.AsSpan(offset + 2));

                ops.Add(Table.AsSpan(offset, length).ToArray());
                offset += length;
            }

            return ops;
        }

        /// <summary>
        /// Builds an op table: Count followed by the ops.
        /// </summary>
        private static byte[] BuildTable(LoadContext Context, XElement Element, List<byte[]> Ops)
        {
            if (Ops.Count > DynDataHw.MaxOps)
                throw Context.Error(Element, $"{Ops.Count} ops exceeds the cap of {DynDataHw.MaxOps}");

            var table = new List<byte>();

            table.AddRange(DynDataHw.ToBytes(new DynDataHw.HwTableHeader { Count = (UInt32)Ops.Count }));
            foreach (var op in Ops)
                table.AddRange(op);

            return table.ToArray();
        }

        /// <summary>
        /// The vendor is written verbatim, which is string order {EBX, EDX, ECX}.
        /// </summary>
        private static byte[] CpuIdentity(string Vendor, UInt16 Family, byte Model)
        {
            var identity = new DynDataHw.HwIdentity();

            identity.Cpu.DisplayFamily = Family;
            identity.Cpu.DisplayModel = Model;

            byte[] bytes = DynDataHw.ToBytes(identity);
            Encoding.ASCII.GetBytes(Vendor, bytes.AsSpan(0, 12));

            return bytes;
        }

        private static byte[] PciIdentity(UInt16 VendorId, UInt16 DeviceId)
        {
            var identity = new DynDataHw.HwIdentity();

            identity.Pci.VendorId = VendorId;
            identity.Pci.DeviceId = DeviceId;

            return DynDataHw.ToBytes(identity);
        }

        /// <summary>
        /// The caller fills the CPU part.
        /// </summary>
        private static byte[] PciClassIdentity(LoadedPci Pci)
        {
            var identity = new DynDataHw.HwIdentity();

            identity.CpuAndPciClass.VendorId = Pci.VendorId;
            identity.CpuAndPciClass.BaseClass = (byte)(Pci.ClassCode >> 16);
            identity.CpuAndPciClass.SubClass = (byte)(Pci.ClassCode >> 8);
            identity.CpuAndPciClass.ProgIf = (byte)Pci.ClassCode;
            identity.CpuAndPciClass.HeaderType = Pci.HeaderType;
            identity.CpuAndPciClass.DevFn = Pci.DevFn;

            return DynDataHw.ToBytes(identity);
        }

        /// <summary>
        /// Parses a models list: comma-separated hex values and inclusive "lo-hi" ranges.
        /// </summary>
        private static List<byte> ParseModels(LoadContext Context, XElement Cpu)
        {
            string text = RequiredAttribute(Context, Cpu, "models");
            var models = new List<byte>();

            foreach (string item in text.Split(','))
            {
                string[] bounds = item.Trim().Split('-');

                if (bounds.Length > 2)
                    throw Context.Error(Cpu, $"malformed models item \"{item}\"");

                UInt64 low = ParseHex(Context, Cpu, "models", bounds[0].Trim(), byte.MaxValue);
                UInt64 high = bounds.Length == 2 ? ParseHex(Context, Cpu, "models", bounds[1].Trim(), byte.MaxValue) : low;

                if (low > high)
                    throw Context.Error(Cpu, $"models range \"{item}\" is reversed");

                for (UInt64 model = low; model <= high; model++)
                {
                    if (models.Contains((byte)model))
                        throw Context.Error(Cpu, $"duplicate model 0x{model:x2} in models");

                    models.Add((byte)model);
                }
            }

            return models;
        }

        /// <summary>
        /// Parses a required list of hex values and inclusive "lo-hi" ranges, each at most Maximum and listed once.
        /// </summary>
        private static List<UInt64> ParseHexList(LoadContext Context, XElement Element, string Name, UInt64 Maximum)
        {
            string text = RequiredAttribute(Context, Element, Name);
            var values = new List<UInt64>();

            foreach (string item in text.Split(','))
            {
                string[] bounds = item.Trim().Split('-');

                if (bounds.Length > 2)
                    throw Context.Error(Element, $"malformed {Name} item \"{item}\"");

                UInt64 low = ParseHex(Context, Element, Name, bounds[0].Trim(), Maximum);
                UInt64 high = bounds.Length == 2 ? ParseHex(Context, Element, Name, bounds[1].Trim(), Maximum) : low;

                if (low > high)
                    throw Context.Error(Element, $"{Name} range \"{item}\" is reversed");

                for (UInt64 value = low; value <= high; value++)
                {
                    if (values.Contains(value))
                        throw Context.Error(Element, $"duplicate 0x{value:x} in {Name}");

                    values.Add(value);
                }
            }

            return values;
        }

        /// <summary>
        /// Parses an optional gate: CPUID "LEAFh:reg:bit" or "LEAFh:reg:shift:width=0xV[-0xM]" on an msr; "name:bit" or "name:shift:width=0xV[-0xM]" on a sysreg.
        /// </summary>
        private static DynDataHw.HwGate ParseGate(LoadContext Context, XElement Element, bool Sysreg)
        {
            string text = OptionalAttribute(Element, "gate");

            if (text == null)
                return default;

            string[] parts = text.Split(':');
            bool cpuid = parts[0].EndsWith('h');

            if (cpuid && Sysreg)
                throw Context.Error(Element, $"gate \"{text}\" is a CPUID gate, which only an <msr> may have");
            if (!cpuid && !Sysreg)
                throw Context.Error(Element, $"gate \"{text}\" is a system register gate, which only a <sysreg> may have");

            if (Sysreg)
            {
                if (parts.Length != 2 && parts.Length != 3)
                    throw Context.Error(Element, $"malformed gate \"{text}\" (expected name:bit or name:shift:width=0xV[-0xM])");
                if (!DynDataHw.SysregNames.TryGetValue(parts[0], out byte gateRegister))
                    throw Context.Error(Element, $"malformed gate \"{text}\" (unknown register \"{parts[0]}\")");
                if (!DynDataHw.SysregIsIdClass(gateRegister))
                    throw Context.Error(Element, $"gate register \"{parts[0]}\" is not an ID register");

                var sysregGate = new DynDataHw.HwGate
                {
                    Kind = DynDataHw.GateSysregField,
                    Register = gateRegister,
                    Shift = (byte)ParseDecimal(Context, Element, "gate shift", parts[1], 0, 63)
                };

                if (parts.Length == 2)
                {
                    sysregGate.Width = 1;
                    sysregGate.Value = 1;
                    sysregGate.ValueMax = 1;
                    return sysregGate;
                }

                ParseGateField(Context, Element, text, parts[2], Math.Min(32u, 64u - sysregGate.Shift), ref sysregGate);

                return sysregGate;
            }

            if ((parts.Length != 3 && parts.Length != 4) ||
                parts[0].Length < 2 || parts[0].Length > 9 || parts[0][^1] != 'h' ||
                !UInt32.TryParse(parts[0].AsSpan(0, parts[0].Length - 1), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out UInt32 leaf))
            {
                throw Context.Error(Element, $"malformed gate \"{text}\" (expected LEAFh:reg:bit or LEAFh:reg:shift:width=0xV[-0xM])");
            }

            byte register = parts[1] switch
            {
                "eax" => DynDataHw.CpuidEax,
                "ebx" => DynDataHw.CpuidEbx,
                "ecx" => DynDataHw.CpuidEcx,
                "edx" => DynDataHw.CpuidEdx,
                _ => throw Context.Error(Element, $"malformed gate \"{text}\" (register must be eax, ebx, ecx or edx)")
            };

            var gate = new DynDataHw.HwGate
            {
                Kind = DynDataHw.GateCpuid,
                Register = register,
                Shift = (byte)ParseDecimal(Context, Element, "gate shift", parts[2], 0, 31),
                Leaf = leaf,
                SubLeaf = 0
            };

            if (parts.Length == 3)
            {
                gate.Width = 1;
                gate.Value = 1;
                gate.ValueMax = 1;
                return gate;
            }

            ParseGateField(Context, Element, text, parts[3], 32u - gate.Shift, ref gate);

            return gate;
        }

        /// <summary>
        /// Parses the "width=0xV[-0xM]" part of a field gate.
        /// </summary>
        private static void ParseGateField(LoadContext Context, XElement Element, string Text, string Field, UInt64 MaxWidth, ref DynDataHw.HwGate Gate)
        {
            string[] field = Field.Split('=');

            if (field.Length != 2)
                throw Context.Error(Element, $"malformed gate \"{Text}\" (expected width=0xV[-0xM])");

            Gate.Width = (byte)ParseDecimal(Context, Element, "gate width", field[0], 1, MaxWidth);

            UInt64 fieldMax = (1ul << Gate.Width) - 1;
            string[] bounds = field[1].Split('-');

            if (bounds.Length > 2)
                throw Context.Error(Element, $"malformed gate \"{Text}\" (expected width=0xV[-0xM])");

            Gate.Value = (UInt32)ParseHex(Context, Element, "gate value", bounds[0], fieldMax);
            Gate.ValueMax = bounds.Length == 2 ? (UInt32)ParseHex(Context, Element, "gate value", bounds[1], fieldMax) : Gate.Value;

            if (Gate.Value > Gate.ValueMax)
                throw Context.Error(Element, $"gate \"{Text}\" range is reversed");
        }

        /// <summary>
        /// Parses a pin "device.function".
        /// </summary>
        private static (byte Device, byte Function) ParsePin(LoadContext Context, XElement Element, string Text)
        {
            string[] parts = Text.Split('.');

            if (parts.Length != 2)
                throw Context.Error(Element, $"malformed pin \"{Text}\" (expected device.function)");

            return ((byte)ParseDecimal(Context, Element, "pin device", parts[0], 0, 31),
                    (byte)ParseDecimal(Context, Element, "pin function", parts[1], 0, 7));
        }

        /// <summary>
        /// Parses the required index-port and data-port of an element; the two must be a pair of DynDataHw.IoPortPairs.
        /// </summary>
        private static (UInt16 Index, UInt16 Data) ParsePortPair(LoadContext Context, XElement Element)
        {
            UInt16 index = (UInt16)ParseHex(Context, Element, "index-port", RequiredAttribute(Context, Element, "index-port"), UInt16.MaxValue);
            UInt16 data = (UInt16)ParseHex(Context, Element, "data-port", RequiredAttribute(Context, Element, "data-port"), UInt16.MaxValue);

            if (!DynDataHw.IoPortPairs.Contains((index, data)))
                throw Context.Error(Element, $"index-port 0x{index:x} / data-port 0x{data:x} is not a pair of DynDataHw.IoPortPairs");

            return (index, data);
        }

        /// <summary>
        /// Parses a "0x"-prefixed hex number no greater than Maximum.
        /// </summary>
        private static UInt64 ParseHex(LoadContext Context, XElement Element, string Name, string Text, UInt64 Maximum)
        {
            if (Text.Length < 3 || Text.Length > 18 ||
                !Text.StartsWith("0x", StringComparison.Ordinal) ||
                !UInt64.TryParse(Text.AsSpan(2), NumberStyles.AllowHexSpecifier, CultureInfo.InvariantCulture, out UInt64 value))
            {
                throw Context.Error(Element, $"malformed {Name} \"{Text}\" (expected 0x-prefixed hex)");
            }

            if (value > Maximum)
                throw Context.Error(Element, $"{Name} {Text} exceeds 0x{Maximum:x}");

            return value;
        }

        /// <summary>
        /// Parses a decimal number in [Minimum, Maximum].
        /// </summary>
        private static UInt64 ParseDecimal(LoadContext Context, XElement Element, string Name, string Text, UInt64 Minimum, UInt64 Maximum)
        {
            if (Text.Length == 0 || Text.Length > 4 || !Text.All(char.IsAsciiDigit))
                throw Context.Error(Element, $"malformed {Name} \"{Text}\" (expected decimal)");

            UInt64 value = UInt64.Parse(Text, CultureInfo.InvariantCulture);

            if (value < Minimum || value > Maximum)
                throw Context.Error(Element, $"{Name} {Text} outside [{Minimum}, {Maximum}]");

            return value;
        }

        private static string RequiredAttribute(LoadContext Context, XElement Element, string Name)
        {
            return Element.Attribute(Name)?.Value ?? throw Context.Error(Element, $"missing attribute {Name}");
        }

        private static string OptionalAttribute(XElement Element, string Name)
        {
            return Element.Attribute(Name)?.Value;
        }

        private static void CheckAttributes(LoadContext Context, XElement Element, string[] Allowed)
        {
            foreach (var attribute in Element.Attributes())
            {
                if (attribute.IsNamespaceDeclaration || !Allowed.Contains(attribute.Name.ToString()))
                    throw Context.Error(Element, $"unknown attribute {attribute.Name}");
            }
        }

        private static void CheckNoChildren(LoadContext Context, XElement Element)
        {
            var child = Element.Elements().FirstOrDefault();

            if (child != null)
                throw Context.Error(child, "unknown element");
        }

        private sealed class LoadContext
        {
            public string FileName;
            public readonly List<HwRow> Rows = [];
            private readonly Dictionary<string, XElement> Identities = [];
            private readonly Dictionary<(string Vendor, UInt16 Family, byte Model), XElement> Models = [];
            private readonly Dictionary<string, List<XElement>> Buckets = [];
            private readonly Dictionary<XElement, List<(byte DevFn, byte[] Table)>> BucketTables = [];
            private readonly List<(string FileName, XElement Element, string Source, byte DevFn, byte[] Table)> Sources = [];

            /// <summary>
            /// Records the pinned pci tables of a named bucket, for source= references.
            /// </summary>
            public void AddBucket(XElement Cpu, IEnumerable<(byte DevFn, byte[] Table)> Tables)
            {
                string name = Cpu.Attribute("name")?.Value;

                BucketTables.Add(Cpu, Tables.ToList());

                if (name == null)
                    return;

                if (!Buckets.TryGetValue(name, out var list))
                    Buckets.Add(name, list = []);

                list.Add(Cpu);
            }

            public void AddSource(XElement Element, string Source, byte DevFn, byte[] Table)
            {
                Sources.Add((FileName, Element, Source, DevFn, Table));
            }

            /// <summary>
            /// Checks that each pciclass with source= equals every pinned pci table on its pin in the named bucket.
            /// </summary>
            public void CheckSources()
            {
                string fileName = FileName;

                foreach (var (sourceFile, element, source, devFn, table) in Sources)
                {
                    FileName = sourceFile;

                    if (!Buckets.TryGetValue(source, out var buckets))
                        throw Error(element, $"source bucket \"{source}\" not found");
                    if (buckets.Count != 1)
                        throw Error(element, $"source bucket name \"{source}\" is not unique");

                    var tables = BucketTables[buckets[0]].Where(t => t.DevFn == devFn).ToList();

                    if (tables.Count == 0)
                        throw Error(element, $"source bucket \"{source}\" has no pinned <pci> on device {devFn >> 3} function {devFn & 7}");

                    if (tables.Any(t => !t.Table.AsSpan().SequenceEqual(table)))
                        throw Error(element, $"table differs from the pinned <pci> table of source bucket \"{source}\"");
                }

                FileName = fileName;
            }

            private readonly Dictionary<(UInt16 Vendor, UInt16 Device), XElement> TopLevelPcis = [];
            private readonly List<(string FileName, XElement Element, UInt16 Vendor, UInt16 Device)> NestedPcis = [];

            public void AddTopLevelPci(XElement Element, UInt16 Vendor, UInt16 Device)
            {
                TopLevelPcis.TryAdd((Vendor, Device), Element);
            }

            public void AddNestedPci(XElement Element, UInt16 Vendor, UInt16 Device)
            {
                NestedPcis.Add((FileName, Element, Vendor, Device));
            }

            /// <summary>
            /// A top-level pci with the IDs of a nested pci holding PCI-row-first ops leaves those ops unreachable.
            /// </summary>
            public void CheckShadowing()
            {
                string fileName = FileName;

                foreach (var (nestedFile, element, vendor, device) in NestedPcis)
                {
                    if (!TopLevelPcis.TryGetValue((vendor, device), out var top))
                        continue;

                    FileName = nestedFile;

                    throw Error(element, $"pci 0x{vendor:x4}:0x{device:x4} holds window/smbus/vsec ops that the top-level {Describe(top)} shadows (the driver tries the PCI row first)");
                }

                FileName = fileName;
            }

            /// <summary>
            /// A model may appear in only one bucket.
            /// </summary>
            public void AddModel(XElement Cpu, string Vendor, UInt16 Family, byte Model)
            {
                if (Models.TryGetValue((Vendor, Family, Model), out var other))
                    throw Error(Cpu, $"model 0x{Model:x2} of {Vendor} family 0x{Family:x} is also in {Describe(other)}");

                Models.Add((Vendor, Family, Model), Cpu);
            }

            /// <summary>
            /// The driver takes the first family whose probe matches, so a chip ID belongs to one family only, and
            /// the base range and the chip-level fields are read from the first passing row of (slot, family, chip),
            /// so every row of the chip must carry the same ones.
            /// </summary>
            public void AddSuperIoChip(XElement Element, byte Slot, byte Family, UInt16 Chip, DynDataHw.HwSuperIoRead Body)
            {
                if (SuperIoChips.TryGetValue((Slot, Chip), out var other))
                {
                    var o = other.Body;

                    if (other.Family != Family)
                        throw Error(Element, $"chip 0x{Chip:x4} at slot {Slot} is also family {other.Family} in {Describe(other.Element)}");
                    if (o.BaseMin != Body.BaseMin || o.BaseMax != Body.BaseMax)
                        throw Error(Element, $"chip 0x{Chip:x4} at slot {Slot} has base range 0x{o.BaseMin:x4}-0x{o.BaseMax:x4} in {Describe(other.Element)}");
                    if (o.Ldn != Body.Ldn || o.Exit != Body.Exit || o.ConfigIdIndex != Body.ConfigIdIndex || o.ConfigIdValue != Body.ConfigIdValue ||
                        o.CustomerRegister != Body.CustomerRegister || o.Flags != Body.Flags)
                        throw Error(Element, $"chip 0x{Chip:x4} at slot {Slot} has other ldn, exit, config-id, customer-reg or require-active in {Describe(other.Element)}");
                }

                SuperIoChips[(Slot, Chip)] = (Family, Body, Element);
            }

            private readonly Dictionary<(byte Slot, UInt16 Chip), (byte Family, DynDataHw.HwSuperIoRead Body, XElement Element)> SuperIoChips = [];

            /// <summary>
            /// The driver probes a slot at the port of its first passing row, so all rows of a slot name one port, and
            /// two slots never share one.
            /// </summary>
            public void AddSuperIoPort(XElement Element, byte Slot, UInt16 Port)
            {
                if (SuperIoPorts.TryGetValue(Slot, out var other) && other.Port != Port)
                    throw Error(Element, $"slot {Slot} port 0x{Port:x2} differs from port 0x{other.Port:x2} in {Describe(other.Element)}");

                foreach (var (slot, (otherPort, otherElement)) in SuperIoPorts)
                {
                    if (slot != Slot && (Math.Abs(otherPort - Port) <= 1))
                        throw Error(Element, $"slot {Slot} port 0x{Port:x2} overlaps slot {slot} port 0x{otherPort:x2} in {Describe(otherElement)}");
                }

                SuperIoPorts[Slot] = (Port, Element);
            }

            private readonly Dictionary<byte, (UInt16 Port, XElement Element)> SuperIoPorts = [];

            /// <summary>
            /// One board row per hash: a second (manufacturer, product) with the same FNV-1a hash is a collision.
            /// </summary>
            public void AddBoardHash(XElement Board, UInt64 Hash, string Manufacturer, string Product)
            {
                // The strings as the hash sees them: trimmed, ASCII upper case.
                string key = $"{Manufacturer.Trim(' ', '\t').ToUpperInvariant()}\0{Product.Trim(' ', '\t').ToUpperInvariant()}";

                if (BoardHashes.TryGetValue(Hash, out var other))
                {
                    if (other.Key != key)
                        throw Error(Board, $"manufacturer and product hash to 0x{Hash:x16}, as do the different strings of {Describe(other.Element)} (an FNV-1a collision)");

                    throw Error(Board, $"hash 0x{Hash:x16} is also the row of {Describe(other.Element)} (one board row per hash)");
                }

                BoardHashes.Add(Hash, (key, Board));
            }

            private readonly Dictionary<UInt64, (string Key, XElement Element)> BoardHashes = [];

            /// <summary>
            /// An identity may appear only once.
            /// </summary>
            public void AddRow(XElement Element, byte IdentityKind, byte[] Identity, byte[] Table)
            {
                string key = $"{IdentityKind}:{Convert.ToHexString(Identity)}";

                if (BinaryPrimitives.ReadUInt32LittleEndian(Table) == 0)
                    throw Error(Element, "the element holds no op (an empty table is rejected)");

                if (Identities.TryGetValue(key, out var other))
                    throw Error(Element, $"duplicate identity row, also from {Describe(other)}");

                Identities.Add(key, Element);
                Rows.Add(new HwRow { IdentityKind = IdentityKind, Identity = Identity, Table = Table });
            }

            public InvalidDataException Error(XElement Element, string Message)
            {
                var top = Element.AncestorsAndSelf().FirstOrDefault(e => e.Parent != null && e.Parent.Parent == null);
                string what;

                if (top == null)
                    what = $"<{Element.Name}>";
                else if (top == Element)
                    what = Describe(Element);
                else
                    what = $"<{Element.Name}> in {Describe(top)}";

                return new InvalidDataException($"{FileName}({Line(Element)}): {what}: {Message}");
            }

            private static string Describe(XElement Element)
            {
                string name = Element.Attribute("name")?.Value;
                string what = name != null ? $"\"{name}\"" : string.Join(" ", Element.Attributes().Where(a => a.Name != "desc").Select(a => $"{a.Name}=\"{a.Value}\""));

                return what.Length != 0 ? $"<{Element.Name}> {what} (line {Line(Element)})" : $"<{Element.Name}> (line {Line(Element)})";
            }

            private static int Line(XElement Element)
            {
                return ((IXmlLineInfo)Element).LineNumber;
            }
        }
    }

    /// <summary>
    /// Compares tables of pci_read and pci_indirect_read ops by what they allow.
    /// </summary>
    public static class HwTableSubset
    {
        private sealed class Decoded
        {
            public byte[] ReadHeader;
            public List<(UInt32 Begin, UInt32 End)> Reads = [];
            public byte[] IndirectHeader;
            public (UInt16 Index, UInt16 Data, byte PinDevice, byte PinFunction) Indirect;
            public List<DynDataHw.HwTargetRange> Targets = [];
            public List<UInt32> Values = [];
        }

        /// <summary>
        /// Returns whether Superset allows every target Subset allows, with identical op headers (Length aside), pair and pin.
        /// </summary>
        public static bool IsSubset(byte[] Subset, byte[] Superset)
        {
            if (Subset.AsSpan().SequenceEqual(Superset))
                return true;

            var sub = Decode(Subset);
            var super = Decode(Superset);

            if (sub == null || super == null)
                return false;

            if (sub.ReadHeader != null)
            {
                if (super.ReadHeader == null || !sub.ReadHeader.AsSpan().SequenceEqual(super.ReadHeader))
                    return false;
                if (!sub.Reads.All(r => super.Reads.Any(s => s.Begin <= r.Begin && r.End <= s.End)))
                    return false;
            }

            if (sub.IndirectHeader != null)
            {
                if (super.IndirectHeader == null || !sub.IndirectHeader.AsSpan().SequenceEqual(super.IndirectHeader))
                    return false;
                if (sub.Indirect != super.Indirect)
                    return false;
                if (!sub.Targets.All(t => super.Targets.Any(s => s.Stride == t.Stride && s.StrideCount == t.StrideCount && s.Begin <= t.Begin && t.End <= s.End)))
                    return false;
                if (!sub.Values.All(v => super.Values.Contains(v) || super.Targets.Any(s => Covers(s, v))))
                    return false;
            }

            return true;
        }

        private static bool Covers(DynDataHw.HwTargetRange Range, UInt32 Value)
        {
            int count = Math.Max(1, (int)Range.StrideCount);

            for (int k = 0; k < count; k++)
            {
                UInt64 begin = Range.Begin + (UInt64)Range.Stride * (UInt64)k;
                UInt64 end = Range.End + (UInt64)Range.Stride * (UInt64)k;

                if (begin <= Value && Value < end)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Decodes a table of at most one pci_read and one pci_indirect_read op; null otherwise.
        /// </summary>
        private static Decoded Decode(byte[] Table)
        {
            var result = new Decoded();
            var span = Table.AsSpan();
            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(span).Count;
            int offset = DynDataHw.TableSize;

            for (UInt32 i = 0; i < count; i++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(span[offset..]);
                var body = span.Slice(offset + DynDataHw.OpHeaderSize, op.Length - DynDataHw.OpHeaderSize);
                byte[] header = span.Slice(offset + 4, DynDataHw.OpHeaderSize - 4).ToArray();

                if (op.Kind == DynDataHw.KindPciRead && result.ReadHeader == null)
                {
                    result.ReadHeader = header;

                    for (int r = 0; r < body[0]; r++)
                    {
                        var range = MemoryMarshal.Read<DynDataHw.HwRange>(body[(DynDataHw.PciReadHeaderSize + r * DynDataHw.RangeSize)..]);

                        result.Reads.Add((range.Offset, (UInt32)range.Offset + range.Length));
                    }
                }
                else if (op.Kind == DynDataHw.KindPciIndirectRead && result.IndirectHeader == null)
                {
                    var indirect = MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(body);
                    int valuesOffset = DynDataHw.PciIndirectReadSize + indirect.RangeCount * DynDataHw.TargetRangeSize;

                    result.IndirectHeader = header;
                    result.Indirect = (indirect.IndexOffset, indirect.DataOffset, indirect.PinDevice, indirect.PinFunction);

                    for (int r = 0; r < indirect.RangeCount; r++)
                        result.Targets.Add(MemoryMarshal.Read<DynDataHw.HwTargetRange>(body[(DynDataHw.PciIndirectReadSize + r * DynDataHw.TargetRangeSize)..]));
                    for (int v = 0; v < indirect.ValueCount; v++)
                        result.Values.Add(BinaryPrimitives.ReadUInt32LittleEndian(body[(valuesOffset + v * sizeof(UInt32))..]));
                }
                else
                {
                    return null;
                }

                offset += op.Length;
            }

            return result;
        }
    }

    public static class HwSection
    {
        /// <summary>
        /// Serializes the section: header, index rows sorted by (IdentityKind, Identity), then each distinct op table once.
        /// </summary>
        /// <param name="Rows">The index rows.</param>
        /// <param name="TableCount">The number of distinct op tables emitted.</param>
        /// <returns>The section bytes.</returns>
        public static byte[] Serialize(List<HwRow> Rows, out int TableCount)
        {
            if (Rows.Count > DynDataHw.MaxRows)
                throw new InvalidDataException($"hardware section: {Rows.Count} rows exceeds the cap of {DynDataHw.MaxRows}");

            var rows = Rows.ToList();

            rows.Sort((a, b) =>
            {
                int result = a.IdentityKind.CompareTo(b.IdentityKind);
                return result != 0 ? result : a.Identity.AsSpan().SequenceCompareTo(b.Identity);
            });

            var body = new List<byte>();
            var tableOffsets = new Dictionary<string, UInt32>();
            var index = new List<byte>(rows.Count * DynDataHw.IndexSize);

            foreach (var row in rows)
            {
                if (row.Identity.Length != DynDataHw.IdentitySize)
                    throw new InvalidDataException($"hardware section: identity kind {row.IdentityKind} identity is {row.Identity.Length} bytes, not {DynDataHw.IdentitySize}");

                string key = Convert.ToHexString(row.Table);

                if (!tableOffsets.TryGetValue(key, out UInt32 offset))
                {
                    offset = (UInt32)body.Count;
                    tableOffsets.Add(key, offset);
                    body.AddRange(row.Table);
                }

                var entry = new DynDataHw.HwIndex
                {
                    IdentityKind = row.IdentityKind,
                    Offset = offset,
                    Length = (UInt32)row.Table.Length
                };
                byte[] entryBytes = DynDataHw.ToBytes(entry);

                row.Identity.CopyTo(entryBytes.AsSpan(DynDataHw.IndexIdentityOffset, DynDataHw.IdentitySize));
                index.AddRange(entryBytes);
            }

            long length = DynDataHw.SectionSize + index.Count + body.Count;

            if (length > DynDataHw.MaxSectionLength)
                throw new InvalidDataException($"hardware section: {length} bytes exceeds the cap of 0x{DynDataHw.MaxSectionLength:x}");

            var header = new DynDataHw.HwSectionHeader
            {
                Magic = DynDataHw.SectionMagic,
                Length = (UInt32)length,
                Count = (UInt32)rows.Count
            };

            byte[] section = [.. DynDataHw.ToBytes(header), .. index, .. body];

            CheckSection(section);

            TableCount = tableOffsets.Count;

            return section;
        }

        /// <summary>
        /// Validates the section with the rules of KphDynDataLookupHardware, so a build cannot emit a section the driver rejects.
        /// </summary>
        /// <param name="Section">The section bytes.</param>
        private static void CheckSection(ReadOnlySpan<byte> Section)
        {
            if (Section.Length < DynDataHw.SectionSize || Section.Length > DynDataHw.MaxSectionLength)
                throw Fail($"length {Section.Length} outside [{DynDataHw.SectionSize}, {DynDataHw.MaxSectionLength}]");

            var header = MemoryMarshal.Read<DynDataHw.HwSectionHeader>(Section);

            if (header.Magic != DynDataHw.SectionMagic)
                throw Fail("bad magic");
            if (header.Length != Section.Length || header.Count > DynDataHw.MaxRows || header.Reserved != 0)
                throw Fail($"length {header.Length}, row count {header.Count} or reserved invalid");

            long bodyOffset = DynDataHw.SectionSize + (long)header.Count * DynDataHw.IndexSize;

            if (bodyOffset > Section.Length)
                throw Fail("index overruns the section");

            var body = Section[(int)bodyOffset..];
            var tables = new HashSet<(UInt32, UInt32)>();
            UInt32 maxOps = 0;
            UInt32 maxRanges = 0;
            UInt32 maxCommands = 0;
            UInt32 maxVersions = 0;
            UInt32 maxVsecRegions = 0;
            UInt64 rowOps = 0;
            var superIoPorts = new Dictionary<byte, UInt16>();
            var superIoChips = new Dictionary<(byte Slot, byte Family, UInt16 Chip), byte[]>();

            for (int i = 0; i < header.Count; i++)
            {
                var rowBytes = Section.Slice(DynDataHw.SectionSize + i * DynDataHw.IndexSize, DynDataHw.IndexSize);
                var row = MemoryMarshal.Read<DynDataHw.HwIndex>(rowBytes);

                CheckIdentity(rowBytes);

                // The driver takes the first row of an identity; strictly ascending rows are unique.
                if (i != 0 && Section.Slice(DynDataHw.SectionSize + (i - 1) * DynDataHw.IndexSize, DynDataHw.IndexIdentityOffset + DynDataHw.IdentitySize).SequenceCompareTo(rowBytes[..(DynDataHw.IndexIdentityOffset + DynDataHw.IdentitySize)]) >= 0)
                    throw Fail($"row {i} is not above row {i - 1} in (identity kind, identity) order");

                if (row.Offset % DynDataHw.OpAlignment != 0)
                    throw Fail($"row {i} table offset 0x{row.Offset:x} is not {DynDataHw.OpAlignment}-byte aligned");
                if ((UInt64)row.Offset + row.Length > (UInt64)body.Length)
                    throw Fail($"row {i} table [0x{row.Offset:x}, +0x{row.Length:x}) overruns the body");

                var table = body.Slice((int)row.Offset, (int)row.Length);

                if (tables.Add((row.Offset, row.Length)))
                {
                    if (table.Length < DynDataHw.TableSize)
                        throw Fail($"row {i} table shorter than its count");

                    var tableHeader = MemoryMarshal.Read<DynDataHw.HwTableHeader>(table);
                    UInt32 count = tableHeader.Count;

                    if (tableHeader.Reserved != 0)
                        throw Fail($"row {i} table reserved is non-zero");

                    if (count > DynDataHw.MaxOps)
                        throw Fail($"row {i} table has {count} ops, cap {DynDataHw.MaxOps}");
                    if (count == 0)
                        throw Fail($"row {i} table is empty");

                    maxOps = Math.Max(maxOps, count);

                    var remaining = table[DynDataHw.TableSize..];
                    UInt32 vsecRegions = 0;

                    for (UInt32 j = 0; j < count; j++)
                    {
                        int length = CheckOp(remaining, ref maxRanges);
                        UInt16 kind = MemoryMarshal.Read<DynDataHw.HwOpHeader>(remaining).Kind;

                        if (kind == DynDataHw.KindVsecRead)
                        {
                            vsecRegions += remaining[DynDataHw.OpHeaderSize + 2];

                            if (vsecRegions > DynDataHw.VsecMaxTableRegions)
                                throw Fail($"row {i} table vsec_read ops expose {vsecRegions} regions, cap {DynDataHw.VsecMaxTableRegions}");
                        }
                        else if (kind == DynDataHw.KindMailboxTransact)
                        {
                            maxCommands = Math.Max(maxCommands, remaining[DynDataHw.OpHeaderSize + 2]);
                        }
                        else if (kind == DynDataHw.KindSmuTableRead)
                        {
                            maxVersions = Math.Max(maxVersions, remaining[DynDataHw.OpHeaderSize + 2]);
                        }

                        remaining = remaining[length..];
                    }

                    if (remaining.Length != 0)
                        throw Fail($"row {i} table has {remaining.Length} bytes past its ops");

                    maxVsecRegions = Math.Max(maxVsecRegions, vsecRegions);

                    CheckSmuTableRefs(table, i);
                    CheckMailboxRefs(table, i);
                    CheckWindowRefs(table, i);
                    CheckPairReads(table, i);
                }

                CheckRowTable(rowBytes, table, i);
                CheckRowPolicy(rowBytes, table, i);
                CheckSuperIoRow(rowBytes, table, i, superIoPorts, superIoChips);

                rowOps += MemoryMarshal.Read<DynDataHw.HwTableHeader>(table).Count;
            }

            // Each distinct table is emitted once, back to back, so the tables tile the body.
            UInt64 tableEnd = 0;

            foreach (var (offset, length) in tables.OrderBy(t => t.Item1))
            {
                if (offset != tableEnd)
                    throw Fail($"table [0x{offset:x}, +0x{length:x}) does not start where the previous table ends (0x{tableEnd:x})");

                tableEnd = (UInt64)offset + length;
            }

            if (tableEnd != (UInt64)body.Length)
                throw Fail($"tables end at 0x{tableEnd:x}, the body at 0x{body.Length:x}");

            CheckPciRows(Section, header.Count, body);
            CheckCpuRows(Section, header.Count, body);
            CheckMmioMailboxes(Section, header.Count, body);
            CheckSmnApertures(Section, header.Count, body);
            CheckBoardRows(Section, header.Count);

            WarnCap("section length", (UInt64)Section.Length, DynDataHw.MaxSectionLength);
            WarnCap("rows", header.Count, DynDataHw.MaxRows);
            WarnCap("ops in one table", maxOps, DynDataHw.MaxOps);
            WarnCap("ranges or values in one op", maxRanges, DynDataHw.MaxRanges);
            WarnCap("commands in one mailbox_transact op", maxCommands, DynDataHw.MailboxMaxCommands);
            WarnCap("versions in one smu_table_read op", maxVersions, DynDataHw.SmuTableMaxVersions);
            WarnCap("vsec_read regions in one table", maxVsecRegions, DynDataHw.VsecMaxTableRegions);
            WarnCap("ops summed over the rows", rowOps, DynDataHw.RowOpsBudget);
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckPciRows(ReadOnlySpan<byte> Section, UInt32 Count, ReadOnlySpan<byte> Body)
        {
            var rows = new List<DynDataHw.HwIndex>();

            for (int i = 0; i < Count; i++)
                rows.Add(MemoryMarshal.Read<DynDataHw.HwIndex>(Section.Slice(DynDataHw.SectionSize + i * DynDataHw.IndexSize, DynDataHw.IndexSize)));

            foreach (var pci in rows.Where(r => r.IdentityKind == DynDataHw.IdentityPci))
            {
                var pciTable = Body.Slice((int)pci.Offset, (int)pci.Length);
                var reads = new List<(UInt32 Begin, UInt32 End)>();
                UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(pciTable).Count;
                int offset = DynDataHw.TableSize;

                for (UInt32 j = 0; j < count; j++)
                {
                    var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(pciTable[offset..]);

                    if (op.Kind == DynDataHw.KindPciRead)
                    {
                        var body = pciTable[(offset + DynDataHw.OpHeaderSize)..];

                        for (int r = 0; r < body[0]; r++)
                        {
                            var range = MemoryMarshal.Read<DynDataHw.HwRange>(body[(DynDataHw.PciReadHeaderSize + r * DynDataHw.RangeSize)..]);
                            reads.Add((range.Offset, (UInt32)range.Offset + range.Length));
                        }
                    }

                    offset += op.Length;
                }

                if (reads.Count == 0)
                    continue;

                foreach (var root in rows)
                {
                    if (root.IdentityKind == DynDataHw.IdentityCpuAndPci)
                    {
                        if (root.Identity.CpuAndPci.Pci.VendorId != pci.Identity.Pci.VendorId || root.Identity.CpuAndPci.Pci.DeviceId != pci.Identity.Pci.DeviceId)
                            continue;
                    }
                    else if (root.IdentityKind == DynDataHw.IdentityCpuAndPciClass)
                    {
                        if (root.Identity.CpuAndPciClass.VendorId != pci.Identity.Pci.VendorId)
                            continue;
                    }
                    else
                    {
                        continue;
                    }

                    var rootTable = Body.Slice((int)root.Offset, (int)root.Length);
                    UInt32 rootCount = MemoryMarshal.Read<DynDataHw.HwTableHeader>(rootTable).Count;
                    int rootOffset = DynDataHw.TableSize;
                    var pairs = new List<DynDataHw.HwPciIndirectRead>();
                    bool smnWindow = false;

                    for (UInt32 j = 0; j < rootCount; j++)
                    {
                        var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(rootTable[rootOffset..]);

                        if (op.Kind == DynDataHw.KindPciIndirectRead)
                            pairs.Add(MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(rootTable[(rootOffset + DynDataHw.OpHeaderSize)..]));
                        else if (op.Kind == DynDataHw.KindWindowRead && rootTable[rootOffset + DynDataHw.OpHeaderSize] == DynDataHw.WindowBaseSmnDerived)
                            smnWindow = true;

                        rootOffset += op.Length;
                    }

                    // Stricter than KphpDynHwValidatePciRows for an exact root, the same device: every pair, not only the one an SMN_DERIVED window uses.
                    for (int p = 0; p < pairs.Count; p++)
                    {
                        var pair = pairs[p];

                        if (root.IdentityKind != DynDataHw.IdentityCpuAndPci && !(smnWindow && p == 0))
                            continue;

                        foreach (var (begin, end) in reads)
                        {
                            if ((begin < pair.IndexOffset + 4u && pair.IndexOffset < end) || (begin < pair.DataOffset + 4u && pair.DataOffset < end))
                                throw Fail($"PCI row {pci.Identity.Pci.VendorId:x4}:{pci.Identity.Pci.DeviceId:x4} pci_read covers the SMN pair 0x{pair.IndexOffset:x}/0x{pair.DataOffset:x} of a root it may be");
                        }
                    }

                    rootOffset = DynDataHw.TableSize;

                    for (UInt32 j = 0; j < rootCount; j++)
                    {
                        var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(rootTable[rootOffset..]);

                        if (op.Kind == DynDataHw.KindMailboxTransact)
                        {
                            var smn = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(rootTable[(rootOffset + DynDataHw.OpHeaderSize)..]).Address;

                            foreach (var (begin, end) in reads)
                            {
                                if ((begin < smn.IndexOffset + 4u && smn.IndexOffset < end) || (begin < smn.DataOffset + 4u && smn.DataOffset < end))
                                    throw Fail($"PCI row {pci.Identity.Pci.VendorId:x4}:{pci.Identity.Pci.DeviceId:x4} pci_read covers the SMN pair 0x{smn.IndexOffset:x}/0x{smn.DataOffset:x} of a mailbox root it may be");
                            }
                        }

                        rootOffset += op.Length;
                    }
                }
            }
        }

        /// <summary>
        /// Generator-only: no pci_read of a table covers the index or data register of its pci_indirect_read ops.
        /// </summary>
        private static void CheckPairReads(ReadOnlySpan<byte> Table, int Index)
        {
            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(Table).Count;
            var pairs = new List<DynDataHw.HwPciIndirectRead>();
            var reads = new List<(UInt32 Begin, UInt32 End)>();
            int offset = DynDataHw.TableSize;

            for (UInt32 j = 0; j < count; j++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(Table[offset..]);

                if (op.Kind == DynDataHw.KindPciIndirectRead)
                    pairs.Add(MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(Table[(offset + DynDataHw.OpHeaderSize)..]));
                else if (op.Kind == DynDataHw.KindPciRead)
                    reads.AddRange(PciReadRanges(Table[(offset + DynDataHw.OpHeaderSize)..]));

                offset += op.Length;
            }

            foreach (var pair in pairs)
            {
                foreach (var (begin, end) in reads)
                {
                    if ((begin < pair.IndexOffset + 4u && pair.IndexOffset < end) || (begin < pair.DataOffset + 4u && pair.DataOffset < end))
                        throw Fail($"row {Index} pci_read covers the SMN pair 0x{pair.IndexOffset:x}/0x{pair.DataOffset:x} of its pci_indirect_read (the data register reads whatever target is selected)");
                }
            }
        }

        /// <summary>
        /// Generator-only: <see cref="CheckMailboxRefs"/> and <see cref="CheckWindowRefs"/> over the SMN ops of every table one CPU identity can activate.
        /// </summary>
        private static void CheckCpuRows(ReadOnlySpan<byte> Section, UInt32 Count, ReadOnlySpan<byte> Body)
        {
            var groups = new Dictionary<string, (int Row, SortedSet<UInt32> Tables)>();

            for (int i = 0; i < Count; i++)
            {
                var rowBytes = Section.Slice(DynDataHw.SectionSize + i * DynDataHw.IndexSize, DynDataHw.IndexSize);
                var row = MemoryMarshal.Read<DynDataHw.HwIndex>(rowBytes);

                if (row.IdentityKind != DynDataHw.IdentityCpuAndPci && row.IdentityKind != DynDataHw.IdentityCpuAndPciClass)
                    continue;

                string key = Convert.ToHexString(rowBytes.Slice(DynDataHw.IndexIdentityOffset, DynDataHw.CpuIdentitySize));

                if (!groups.TryGetValue(key, out var group))
                    groups.Add(key, group = (i, []));

                group.Tables.Add(row.Offset);
            }

            foreach (var (_, (rowIndex, offsets)) in groups)
            {
                var ops = new List<byte[]>();

                foreach (UInt32 tableOffset in offsets)
                {
                    var table = Body[(int)tableOffset..];
                    UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(table).Count;
                    int offset = DynDataHw.TableSize;

                    for (UInt32 j = 0; j < count; j++)
                    {
                        var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(table[offset..]);
                        byte first = table[offset + DynDataHw.OpHeaderSize];

                        if (op.Kind == DynDataHw.KindPciIndirectRead ||
                            (op.Kind == DynDataHw.KindMailboxTransact && first == DynDataHw.MailboxTransportSmnPair) ||
                            (op.Kind == DynDataHw.KindWindowRead && first == DynDataHw.WindowBaseSmnDerived))
                        {
                            byte[] bytes = table.Slice(offset, op.Length).ToArray();

                            if (!ops.Any(o => o.AsSpan().SequenceEqual(bytes)))
                                ops.Add(bytes);
                        }

                        offset += op.Length;
                    }
                }

                var combined = new List<byte>(DynDataHw.ToBytes(new DynDataHw.HwTableHeader { Count = (UInt32)ops.Count }));

                foreach (var op in ops)
                    combined.AddRange(op);

                try
                {
                    CheckMailboxRefs(combined.ToArray(), rowIndex);
                    CheckWindowRefs(combined.ToArray(), rowIndex);
                }
                catch (InvalidDataException e)
                {
                    throw Fail($"across the tables of the CPU identity of row {rowIndex}: {e.Message.Replace("hardware section: ", "")}");
                }
            }
        }

        /// <summary>
        /// Generator-only: no opcode of an MMIO mailbox may be internal on an SMN mailbox sharing its MSG or RSP modulo the window.
        /// </summary>
        private static void CheckMmioMailboxes(ReadOnlySpan<byte> Section, UInt32 Count, ReadOnlySpan<byte> Body)
        {
            var mmio = new List<(DynDataHw.HwMailboxTransact Header, DynDataHw.HwMailboxWindow Window, List<UInt32> Opcodes)>();
            var smn = new List<(DynDataHw.HwMailboxTransact Header, HashSet<UInt32> Internal)>();

            for (int i = 0; i < Count; i++)
            {
                var row = MemoryMarshal.Read<DynDataHw.HwIndex>(Section.Slice(DynDataHw.SectionSize + i * DynDataHw.IndexSize, DynDataHw.IndexSize));
                var table = Body.Slice((int)row.Offset, (int)row.Length);
                UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(table).Count;
                int offset = DynDataHw.TableSize;

                for (UInt32 j = 0; j < count; j++)
                {
                    var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(table[offset..]);

                    if (op.Kind == DynDataHw.KindMailboxTransact)
                    {
                        var opBody = table[(offset + DynDataHw.OpHeaderSize)..];
                        var mailbox = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(opBody);
                        var opcodes = new List<UInt32>();
                        var internalOpcodes = new HashSet<UInt32>();

                        for (int c = 0; c < mailbox.CommandCount; c++)
                        {
                            var command = opBody.Slice(DynDataHw.MailboxTransactSize + c * DynDataHw.MailboxCommandSize, DynDataHw.MailboxCommandSize);

                            opcodes.Add(BinaryPrimitives.ReadUInt32LittleEndian(command));

                            if ((command[6] & DynDataHw.MailboxCommandInternal) != 0)
                                internalOpcodes.Add(BinaryPrimitives.ReadUInt32LittleEndian(command));
                        }

                        if (mailbox.Transport == DynDataHw.MailboxTransportMmioWindow)
                        {
                            opcodes.Add(mailbox.VersionOpcode);
                            mmio.Add((mailbox, MemoryMarshal.Read<DynDataHw.HwMailboxWindow>(opBody.Slice(DynDataHw.MailboxAddressOffset, DynDataHw.MailboxAddressSize)), opcodes));
                        }
                        else if (internalOpcodes.Count != 0)
                        {
                            smn.Add((mailbox, internalOpcodes));
                        }
                    }

                    offset += op.Length;
                }
            }

            foreach (var (_, window, opcodes) in mmio)
            {
                foreach (var (header, internalOpcodes) in smn)
                {
                    UInt32 msg = header.Address.Msg % window.WindowSize;
                    UInt32 rsp = header.Address.Rsp % window.WindowSize;

                    if (msg != window.Msg && msg != window.Rsp && rsp != window.Msg && rsp != window.Rsp)
                        continue;

                    foreach (UInt32 opcode in opcodes.Where(internalOpcodes.Contains))
                        throw Fail($"MMIO mailbox 0x{window.Msg:x} opcode 0x{opcode:x2} is internal on SMN mailbox 0x{header.Address.Msg:x} (the same mailbox through its aperture)");
                }
            }
        }

        /// <summary>
        /// Generator-only: no window range or aperture base register may alias an SMN mailbox's registers modulo the 1 MB aperture.
        /// </summary>
        private static void CheckSmnApertures(ReadOnlySpan<byte> Section, UInt32 Count, ReadOnlySpan<byte> Body)
        {
            var windows = new List<(UInt64 Begin, UInt64 End)>();
            var registers = new List<(UInt64 Begin, UInt64 End, UInt32 Msg)>();
            var mailboxRegisters = new List<(UInt64 Begin, UInt64 End, UInt32 Msg)>();
            var baseRegisters = new List<UInt32>();

            for (int i = 0; i < Count; i++)
            {
                var row = MemoryMarshal.Read<DynDataHw.HwIndex>(Section.Slice(DynDataHw.SectionSize + i * DynDataHw.IndexSize, DynDataHw.IndexSize));
                var table = Body.Slice((int)row.Offset, (int)row.Length);
                UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(table).Count;
                int offset = DynDataHw.TableSize;

                for (UInt32 j = 0; j < count; j++)
                {
                    var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(table[offset..]);
                    var opBody = table[(offset + DynDataHw.OpHeaderSize)..];

                    if (op.Kind == DynDataHw.KindWindowRead && opBody[0] == DynDataHw.WindowBaseSmnDerived)
                    {
                        var window = MemoryMarshal.Read<DynDataHw.HwWindowRead>(opBody);

                        for (int r = 0; r < window.RangeCount; r++)
                        {
                            var range = MemoryMarshal.Read<DynDataHw.HwWindowRange>(opBody[(DynDataHw.WindowReadSize + r * DynDataHw.WindowRangeSize)..]);

                            windows.Add((window.FixedBase + range.Offset, window.FixedBase + range.Offset + range.Length));
                        }

                        var aux = opBody[(DynDataHw.WindowReadSize + window.RangeCount * DynDataHw.WindowRangeSize)..];

                        baseRegisters.Add(MemoryMarshal.Read<UInt32>(aux));
                        baseRegisters.Add(MemoryMarshal.Read<UInt32>(aux[4..]));
                    }
                    else if (op.Kind == DynDataHw.KindMailboxTransact && opBody[0] == DynDataHw.MailboxTransportSmnPair)
                    {
                        var mailbox = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(opBody);
                        var smn = mailbox.Address;

                        registers.Add((smn.Msg % DynDataHw.SmnApertureSize, smn.Msg % DynDataHw.SmnApertureSize + 4, smn.Msg));
                        registers.Add((smn.Rsp % DynDataHw.SmnApertureSize, smn.Rsp % DynDataHw.SmnApertureSize + 4, smn.Msg));

                        // One register at a time: the argument block may cross a 1 MB boundary and wrap.
                        for (UInt32 a = 0; a < mailbox.ArgCount; a++)
                        {
                            UInt32 register = (smn.Arg0 + 4 * a) % DynDataHw.SmnApertureSize;

                            registers.Add((register, register + 4ul, smn.Msg));
                        }
                        mailboxRegisters.Add((smn.Msg, smn.Msg + 4ul, smn.Msg));
                        mailboxRegisters.Add((smn.Rsp, smn.Rsp + 4ul, smn.Msg));
                        mailboxRegisters.Add((smn.Arg0, smn.Arg0 + 4ul * mailbox.ArgCount, smn.Msg));
                    }

                    offset += op.Length;
                }
            }

            foreach (var (begin, end) in windows)
            {
                foreach (var (registerBegin, registerEnd, msg) in registers)
                {
                    if (begin < registerEnd && registerBegin < end)
                        throw Fail($"SMN_DERIVED window range [0x{begin:x}, 0x{end:x}) of the aperture covers a register of SMN mailbox 0x{msg:x} (modulo 0x{DynDataHw.SmnApertureSize:x})");
                }
            }

            foreach (UInt32 baseRegister in baseRegisters)
            {
                foreach (var (registerBegin, registerEnd, msg) in mailboxRegisters)
                {
                    if (registerBegin <= baseRegister && baseRegister < registerEnd)
                        throw Fail($"SMN_DERIVED base register 0x{baseRegister:x} is a register of SMN mailbox 0x{msg:x} (the mailbox would return the aperture base)");
                }
            }
        }

        /// <summary>
        /// Generator-only: the driver probes a slot at the port of its first passing row and reads the chip-level
        /// fields from the first passing row of (slot, family, chip), so those agree across rows; two slots never
        /// share a port.
        /// </summary>
        private static void CheckSuperIoRow(ReadOnlySpan<byte> Row, ReadOnlySpan<byte> Table, int Index,
                                            Dictionary<byte, UInt16> Ports, Dictionary<(byte Slot, byte Family, UInt16 Chip), byte[]> Chips)
        {
            if (Row[0] != DynDataHw.IdentitySuperIo)
                return;

            byte slot = Row[DynDataHw.IndexIdentityOffset];
            byte family = Row[DynDataHw.IndexIdentityOffset + 1];
            UInt16 chip = BinaryPrimitives.ReadUInt16LittleEndian(Row[(DynDataHw.IndexIdentityOffset + 2)..]);
            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(Table).Count;
            var remaining = Table[DynDataHw.TableSize..];

            for (UInt32 j = 0; j < count; j++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(remaining);
                var superIo = MemoryMarshal.Read<DynDataHw.HwSuperIoRead>(remaining[DynDataHw.OpHeaderSize..]);
                byte[] fields =
                [
                    superIo.Ldn, superIo.Exit, superIo.Flags, superIo.ConfigIdIndex,
                    .. BitConverter.GetBytes(superIo.ConfigIdValue), .. BitConverter.GetBytes(superIo.CustomerRegister)
                ];

                if (Ports.TryGetValue(slot, out UInt16 port) && port != superIo.ConfigPort)
                    throw Fail($"row {Index} superio_read slot {slot} port 0x{superIo.ConfigPort:x} differs from 0x{port:x}");
                if (Ports.Any(p => p.Key != slot && Math.Abs(p.Value - superIo.ConfigPort) <= 1))
                    throw Fail($"row {Index} superio_read slot {slot} port 0x{superIo.ConfigPort:x} overlaps another slot's");
                if (Chips.TryGetValue((slot, family, chip), out var other) && !other.AsSpan().SequenceEqual(fields))
                    throw Fail($"row {Index} superio_read chip 0x{chip:x4} at slot {slot} disagrees with another row on a chip-level field");

                Ports[slot] = superIo.ConfigPort;
                Chips[(slot, family, chip)] = fields;
                remaining = remaining[op.Length..];
            }
        }

        /// <summary>
        /// Generator-only; the table already passed <see cref="CheckOp"/>.
        /// </summary>
        private static void CheckRowTable(ReadOnlySpan<byte> Row, ReadOnlySpan<byte> Table, int Index)
        {
            byte kind = Row[0];
            byte family = Row[DynDataHw.IndexIdentityOffset + 1];
            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(Table).Count;
            var remaining = Table[DynDataHw.TableSize..];

            for (UInt32 j = 0; j < count; j++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(remaining);

                if ((kind == DynDataHw.IdentitySuperIo) != (op.Kind == DynDataHw.KindSuperIoRead))
                    throw Fail($"row {Index} identity kind {kind} holds op kind {op.Kind} (superio_read belongs to SUPERIO rows only, and they hold nothing else)");
                bool mmioMailbox = op.Kind == DynDataHw.KindMailboxTransact && remaining[DynDataHw.OpHeaderSize] == DynDataHw.MailboxTransportMmioWindow;

                if (((op.Kind == DynDataHw.KindMailboxTransact && !mmioMailbox) || op.Kind == DynDataHw.KindSmuTableRead) &&
                    kind != DynDataHw.IdentityCpuAndPci && kind != DynDataHw.IdentityCpuAndPciClass)
                {
                    throw Fail($"row {Index} identity kind {kind} holds op kind {op.Kind} (SMN mailboxes are reached through a CPU-keyed root complex)");
                }

                byte windowBase = op.Kind == DynDataHw.KindWindowRead ? remaining[DynDataHw.OpHeaderSize] : (byte)0;
                bool acpiOp = op.Kind == DynDataHw.KindAcpiEval || op.Kind == DynDataHw.KindAcpiRequire || windowBase == DynDataHw.WindowBaseAcpiCrs || mmioMailbox;

                if ((kind == DynDataHw.IdentityAcpiHid) != acpiOp)
                    throw Fail($"row {Index} identity kind {kind} holds op kind {op.Kind} base {windowBase} (acpi_eval, acpi_require, ACPI_CRS windows and MMIO mailboxes belong to ACPI_HID rows only, and they hold nothing else)");
                bool lpitWindow = windowBase == DynDataHw.WindowBaseAcpiLpit || windowBase == DynDataHw.WindowBaseAcpiLpitRelative;

                if (kind == DynDataHw.IdentityCpu ?
                    (op.Kind != DynDataHw.KindMsrRead && !lpitWindow && windowBase != DynDataHw.WindowBaseFixedMmio) :
                    lpitWindow)
                {
                    throw Fail($"row {Index} identity kind {kind} holds op kind {op.Kind} base {windowBase} (ACPI_LPIT and ACPI_LPIT_RELATIVE windows belong to CPU rows only, which hold only them, FIXED_MMIO windows and msr_read)");
                }
                if (kind == DynDataHw.IdentityBoard ?
                    (windowBase != DynDataHw.WindowBaseFixedMmio && windowBase != DynDataHw.WindowBaseFixedIo) :
                    windowBase == DynDataHw.WindowBaseFixedIo)
                {
                    throw Fail($"row {Index} identity kind {kind} holds op kind {op.Kind} base {windowBase} (BOARD rows hold only FIXED_MMIO and FIXED_IO windows, and FIXED_IO windows belong to them only)");
                }
                if (windowBase == DynDataHw.WindowBaseFixedMmio && kind != DynDataHw.IdentityBoard &&
                    (kind != DynDataHw.IdentityCpu || remaining[DynDataHw.OpHeaderSize + 29] != 1))
                {
                    throw Fail($"row {Index} identity kind {kind} holds a FIXED_MMIO window (CPU rows with a presence device and BOARD rows only)");
                }
                if (windowBase == DynDataHw.WindowBaseSmnDerived && kind != DynDataHw.IdentityCpuAndPci && kind != DynDataHw.IdentityCpuAndPciClass)
                    throw Fail($"row {Index} identity kind {kind} holds an SMN_DERIVED window (the aperture is read through a CPU-keyed root)");
                if (op.Kind == DynDataHw.KindPciIndirectRead && kind != DynDataHw.IdentityCpuAndPci && kind != DynDataHw.IdentityCpuAndPciClass)
                    throw Fail($"row {Index} identity kind {kind} holds a pci_indirect_read op (an SMN pair is reached through a CPU-keyed root)");
                if (op.Kind == DynDataHw.KindMsrRead && kind != DynDataHw.IdentityCpu)
                    throw Fail($"row {Index} identity kind {kind} holds an msr_read op (CPU rows only)");
                if (op.Kind == DynDataHw.KindPortIndirectRead && kind != DynDataHw.IdentityCpuAndPci)
                    throw Fail($"row {Index} identity kind {kind} holds a port_indirect_read op (CPU_AND_PCI rows only)");
                if (op.Kind == DynDataHw.KindDfMap && kind != DynDataHw.IdentityCpuAndPci)
                    throw Fail($"row {Index} identity kind {kind} holds a df_map op (CPU_AND_PCI rows only)");
                if ((kind == DynDataHw.IdentityArmCpu) != (op.Kind == DynDataHw.KindSysregRead))
                    throw Fail($"row {Index} identity kind {kind} holds op kind {op.Kind} (sysreg_read belongs to ARM_CPU rows only, and they hold nothing else)");

                if (op.Kind == DynDataHw.KindSuperIoRead)
                {
                    var body = remaining.Slice(DynDataHw.OpHeaderSize, op.Length - DynDataHw.OpHeaderSize);
                    var superIo = MemoryMarshal.Read<DynDataHw.HwSuperIoRead>(body);
                    bool flat = family == DynDataHw.SuperIoFamilyIte || family == DynDataHw.SuperIoFamilyFintek || family == DynDataHw.SuperIoFamilySmsc;

                    if (family == DynDataHw.SuperIoFamilyIte ?
                        (superIo.Exit != DynDataHw.SuperIoExitNone && superIo.Exit != DynDataHw.SuperIoExitConfig02) :
                        (superIo.Exit != DynDataHw.SuperIoExitKeyAa))
                    {
                        throw Fail($"row {Index} superio_read exit {superIo.Exit} is not one of family {family}");
                    }

                    if (family == DynDataHw.SuperIoFamilyNuvotonEc && superIo.Ldn != DynDataHw.SuperIoEcLdn)
                        throw Fail($"row {Index} NCT668x superio_read ldn 0x{superIo.Ldn:x2}");
                    if ((superIo.BankMask != 0) != (family == DynDataHw.SuperIoFamilyNuvoton) ||
                        (superIo.BankMask != 0 && (superIo.BankMask & DynDataHw.SuperIoNuvotonHbacs) == 0))
                        throw Fail($"row {Index} superio_read bank mask 0x{superIo.BankMask:x2} on family {family}");
                    if ((superIo.Flags & DynDataHw.SuperIoFlagActiveOptional) != 0 && (family != DynDataHw.SuperIoFamilySmsc || superIo.IdCount == 0))
                        throw Fail($"row {Index} superio_read activation is optional on family {family} or without an id");
                    if (superIo.ConfigIdIndex != 0 && family != DynDataHw.SuperIoFamilyFintek)
                        throw Fail($"row {Index} superio_read config-id on family {family}");
                    if ((superIo.CustomerRegister != 0) != (family == DynDataHw.SuperIoFamilyNuvotonEc))
                        throw Fail($"row {Index} superio_read customer register 0x{superIo.CustomerRegister:x4} on family {family}");

                    bool RegisterBad(UInt16 Register) =>
                        (flat && Register > 0xFF) ||
                        (family == DynDataHw.SuperIoFamilyNuvotonEc && (Register >> 8) == 0xFF) ||
                        (family == DynDataHw.SuperIoFamilyNuvoton && ((Register >> 8) & ~superIo.BankMask) != 0);

                    for (int d = 0; d < superIo.IdCount; d++)
                    {
                        var id = DynDataHw.SuperIoId(body, d);

                        if (RegisterBad(id.Register))
                            throw Fail($"row {Index} superio_read id 0x{id.Register:x} is banked on a flat family, in page FFh or outside the bank mask");
                    }

                    string baseError = DynDataHw.SuperIoBaseRangeError(superIo.BaseMin, superIo.BaseMax, DynDataHw.SuperIoBaseExtent(family));

                    if (baseError != null)
                        throw Fail($"row {Index} superio_read {baseError}");

                    for (int r = 0; r < superIo.RangeCount; r++)
                    {
                        var range = MemoryMarshal.Read<DynDataHw.HwRange>(body[(DynDataHw.SuperIoReadSize + r * DynDataHw.RangeSize)..]);

                        if (RegisterBad(range.Offset))
                            throw Fail($"row {Index} superio_read range 0x{range.Offset:x} is banked on a flat family, in page FFh or outside the bank mask");
                        if (DynDataHw.SuperIoReadToClear(family).Any(clear => clear >= range.Offset && clear < (UInt32)range.Offset + range.Length))
                            throw Fail($"row {Index} superio_read range 0x{range.Offset:x}+0x{range.Length:x} covers a read-to-clear register of family {family}");
                    }
                }

                remaining = remaining[op.Length..];
            }
        }

        /// <summary>
        /// Generator-only: the loader's semantic rules, re-checked where the row identity and its table carry them.
        /// </summary>
        private static void CheckRowPolicy(ReadOnlySpan<byte> Row, ReadOnlySpan<byte> Table, int Index)
        {
            byte kind = Row[0];
            var identity = MemoryMarshal.Read<DynDataHw.HwIdentity>(Row[DynDataHw.IndexIdentityOffset..]);
            bool cpuKeyed = kind == DynDataHw.IdentityCpu || kind == DynDataHw.IdentityCpuAndPci || kind == DynDataHw.IdentityCpuAndPciClass;
            bool classRow = kind == DynDataHw.IdentityCpuAndPciClass;
            string cpuVendor = cpuKeyed ? Encoding.ASCII.GetString(Row.Slice(DynDataHw.IndexIdentityOffset, 12)) : null;
            UInt16 family = identity.Cpu.DisplayFamily;
            byte model = identity.Cpu.DisplayModel;
            UInt16 pciVendor = kind switch
            {
                DynDataHw.IdentityPci => identity.Pci.VendorId,
                DynDataHw.IdentityCpuAndPci => identity.CpuAndPci.Pci.VendorId,
                DynDataHw.IdentityCpuAndPciClass => identity.CpuAndPciClass.VendorId,
                _ => 0
            };
            UInt16 pciDevice = kind switch
            {
                DynDataHw.IdentityPci => identity.Pci.DeviceId,
                DynDataHw.IdentityCpuAndPci => identity.CpuAndPci.Pci.DeviceId,
                _ => 0
            };
            UInt32 classCode = ((UInt32)identity.CpuAndPciClass.BaseClass << 16) | ((UInt32)identity.CpuAndPciClass.SubClass << 8) | identity.CpuAndPciClass.ProgIf;
            string acpiId = kind == DynDataHw.IdentityAcpiHid ? Encoding.ASCII.GetString(Row.Slice(DynDataHw.IndexIdentityOffset, DynDataHw.AcpiIdSize)).TrimEnd('\0') : null;
            int tablePin = -1;

            void CheckPin(string What, byte Device, byte Function)
            {
                int devFn = (Device << 3) | Function;

                if (classRow && devFn != identity.CpuAndPciClass.DevFn)
                    throw Fail($"row {Index} {What} pin {Device}.{Function} is not the pin {identity.CpuAndPciClass.DevFn >> 3}.{identity.CpuAndPciClass.DevFn & 7} of its CPU_AND_PCI_CLASS row");
                if (tablePin >= 0 && devFn != tablePin)
                    throw Fail($"row {Index} {What} pin {Device}.{Function} differs from pin {tablePin >> 3}.{tablePin & 7} of another op in its table");

                tablePin = devFn;
            }

            void CheckSmnRoot(string What, byte Device, byte Function)
            {
                if (!DynDataHw.SmnCpuVendors.Contains(cpuVendor) || !DynDataHw.SmnRootVendors.Contains(pciVendor) || Device != 0 || Function != 0 ||
                    (classRow && classCode != DynDataHw.HostBridgeClass))
                {
                    throw Fail($"row {Index} {What} is not on an AMD or Hygon host bridge (CPU {cpuVendor}, PCI vendor 0x{pciVendor:x4}, pin {Device}.{Function}" +
                               (classRow ? $", class 0x{classCode:x6})" : ")"));
                }
            }

            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(Table).Count;
            var remaining = Table[DynDataHw.TableSize..];
            int piix4Reads = 0;
            var hostRevisions = new List<(UInt32 Low, UInt32 High)>();
            List<string> required = null;
            int requiredEvals = 0;

            for (UInt32 j = 0; j < count; j++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(remaining);
                var body = remaining.Slice(DynDataHw.OpHeaderSize, op.Length - DynDataHw.OpHeaderSize);

                if (op.Kind == DynDataHw.KindPciIndirectRead)
                {
                    var indirect = MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(body);

                    CheckSmnRoot("pci_indirect_read", indirect.PinDevice, indirect.PinFunction);
                    CheckPin("pci_indirect_read", indirect.PinDevice, indirect.PinFunction);

                    if (!DynDataHw.IndirectPairs.Contains((indirect.IndexOffset, indirect.DataOffset)))
                        throw Fail($"row {Index} pci_indirect_read index 0x{indirect.IndexOffset:x} / data 0x{indirect.DataOffset:x} is not a pair of DynDataHw.IndirectPairs");
                }
                else if (op.Kind == DynDataHw.KindMailboxTransact)
                {
                    var mailbox = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(body);
                    var registers = DynDataHw.MailboxRegistersOf(body);
                    string what = $"mailbox_transact 0x{registers.Msg:x}";
                    string problem = DynDataHw.MailboxVersionProblem(body);
                    List<DynDataHw.MailboxProtocol> candidates;

                    if (problem != null)
                        throw Fail($"row {Index} {what}: {problem}");

                    if (mailbox.Transport == DynDataHw.MailboxTransportSmnPair)
                    {
                        CheckSmnRoot(what, mailbox.Address.PinDevice, mailbox.Address.PinFunction);
                        CheckPin(what, mailbox.Address.PinDevice, mailbox.Address.PinFunction);

                        candidates = DynDataHw.MailboxProtocols.Where(p => p.Smn.Contains(registers) && DynDataHw.MailboxProtocolCovers(p, family, model, model)).ToList();
                    }
                    else
                    {
                        // CPUID 1 EAX [27:4]: extended family [23:16], extended model [15:12], family [7:4] (Fh), model [3:0].
                        UInt16 gateFamily = (UInt16)(((op.Gate.Value >> 16) & 0xFF) + 0xF);
                        UInt32 modelMin = (((op.Gate.Value >> 12) & 0xF) << 4) | (op.Gate.Value & 0xF);
                        UInt32 modelMax = (((op.Gate.ValueMax >> 12) & 0xF) << 4) | (op.Gate.ValueMax & 0xF);

                        if (!DynDataHw.MailboxAcpiIds.Contains(acpiId))
                            throw Fail($"row {Index} MMIO {what} is on ACPI ID {acpiId}, not one of DynDataHw.MailboxAcpiIds");

                        candidates = DynDataHw.MailboxProtocols.Where(p => p.Mmio.Contains(registers) && DynDataHw.MailboxProtocolCovers(p, gateFamily, modelMin, modelMax)).ToList();
                    }

                    if (candidates.Count == 0)
                        throw Fail($"row {Index} {what} rsp 0x{registers.Rsp:x} arg 0x{registers.Arg0:x} args {registers.ArgCount} is not a register set of a protocol for its CPU");

                    var opcodes = new List<byte>();

                    for (int c = 0; c < mailbox.CommandCount; c++)
                        opcodes.Add((byte)BinaryPrimitives.ReadUInt32LittleEndian(body[(DynDataHw.MailboxTransactSize + c * DynDataHw.MailboxCommandSize)..]));

                    if (!candidates.Any(p => opcodes.All(p.GetOpcodes.Contains)))
                    {
                        byte opcode = opcodes.First(o => !candidates[0].GetOpcodes.Contains(o));

                        throw Fail($"row {Index} {what} opcode 0x{opcode:x2} is not a GET of protocol {string.Join(" or ", candidates.Select(p => $"\"{p.Name}\""))}");
                    }
                }
                else if (op.Kind == DynDataHw.KindPortIndirectRead)
                {
                    var port = MemoryMarshal.Read<DynDataHw.HwPortIndirectRead>(body);

                    if (pciVendor != 0x1022)
                        throw Fail($"row {Index} port_indirect_read 0x{port.IndexPort:x}/0x{port.DataPort:x} is on PCI vendor 0x{pciVendor:x4}, not an AMD FCH (0x1022)");

                    CheckPin("port_indirect_read", port.PinDevice, port.PinFunction);
                }
                else if (op.Kind == DynDataHw.KindSmbusRead)
                {
                    byte host = body[0];

                    if (kind != DynDataHw.IdentityPci)
                        throw Fail($"row {Index} identity kind {kind} holds an smbus_read op (PCI rows of the host controller only)");
                    if (host == DynDataHw.SmbusHostI801 && pciVendor != 0x8086)
                        throw Fail($"row {Index} i801 smbus_read is on PCI vendor 0x{pciVendor:x4}, not Intel (0x8086)");
                    if (host == DynDataHw.SmbusHostPiix4 && !DynDataHw.SmbusPiix4Vendors.Contains(pciVendor))
                        throw Fail($"row {Index} piix4 smbus_read is on PCI vendor 0x{pciVendor:x4}, not in DynDataHw.SmbusPiix4Vendors");
                    if (host == DynDataHw.SmbusHostPiix4)
                        piix4Reads++;
                }
                else if (op.Kind == DynDataHw.KindSmbusHost)
                {
                    if (kind != DynDataHw.IdentityPci)
                        throw Fail($"row {Index} identity kind {kind} holds an smbus_host op (PCI rows of the host controller only)");
                    if (!DynDataHw.SmbusPiix4Vendors.Contains(pciVendor))
                        throw Fail($"row {Index} smbus_host is on PCI vendor 0x{pciVendor:x4}, not in DynDataHw.SmbusPiix4Vendors");

                    foreach (var (low, high) in hostRevisions)
                    {
                        if (op.Gate.Value <= high && low <= op.Gate.ValueMax)
                            throw Fail($"row {Index} smbus_host revisions 0x{op.Gate.Value:x2}-0x{op.Gate.ValueMax:x2} overlap 0x{low:x2}-0x{high:x2}");
                    }

                    hostRevisions.Add((op.Gate.Value, op.Gate.ValueMax));
                }
                else if (op.Kind == DynDataHw.KindVsecRead)
                {
                    if ((kind != DynDataHw.IdentityPci && kind != DynDataHw.IdentityCpuAndPci) || pciVendor != 0x8086)
                        throw Fail($"row {Index} vsec_read is on identity kind {kind} PCI vendor 0x{pciVendor:x4}, not an Intel (0x8086) PCI identity");
                }
                else if (op.Kind == DynDataHw.KindWindowRead)
                {
                    var window = MemoryMarshal.Read<DynDataHw.HwWindowRead>(body);

                    if (window.BaseKind == DynDataHw.WindowBaseFixedMmio)
                    {
                        if (!DynDataHw.FixedMmioWindows.Contains((window.FixedBase, window.WindowSize)))
                            throw Fail($"row {Index} FIXED_MMIO window 0x{window.FixedBase:x} size 0x{window.WindowSize:x} is not in DynDataHw.FixedMmioWindows");
                        if (kind == DynDataHw.IdentityCpu && cpuVendor != "AuthenticAMD")
                            throw Fail($"row {Index} FIXED_MMIO window 0x{window.FixedBase:x} is in a CPU row of {cpuVendor} (AuthenticAMD only)");
                    }
                    else if (window.BaseKind == DynDataHw.WindowBaseFixedIo)
                    {
                        if (!DynDataHw.FixedIoWindows.Contains((window.FixedBase, window.WindowSize)))
                            throw Fail($"row {Index} FIXED_IO window 0x{window.FixedBase:x} size 0x{window.WindowSize:x} is not in DynDataHw.FixedIoWindows");
                    }
                }
                else if (op.Kind == DynDataHw.KindAcpiEval)
                {
                    var eval = MemoryMarshal.Read<DynDataHw.HwAcpiEval>(body);
                    string method = Encoding.ASCII.GetString(body[..4]);
                    var args = new DynDataHw.HwAcpiArg[eval.ArgCount];

                    if (!DynDataHw.AcpiMethodAllowed(acpiId, method))
                        throw Fail($"row {Index} acpi_eval method \"{method}\" of {acpiId} is not a read-only spec method nor a DynDataHw.AcpiVendorMethods entry");

                    for (int a = 0; a < args.Length; a++)
                        args[a] = MemoryMarshal.Read<DynDataHw.HwAcpiArg>(body[(DynDataHw.AcpiEvalSize + a * DynDataHw.AcpiArgSize)..]);

                    string argsError = DynDataHw.AcpiMethodArgsError(acpiId, method, args);

                    if (argsError != null)
                        throw Fail($"row {Index} acpi_eval: {argsError}");

                    string requireError = DynDataHw.AcpiMethodRequireError(acpiId, method, required);

                    if (requireError != null)
                        throw Fail($"row {Index} acpi_eval: {requireError}");

                    requiredEvals++;
                }
                else if (op.Kind == DynDataHw.KindAcpiRequire)
                {
                    if (required != null && requiredEvals == 0)
                        throw Fail($"row {Index} acpi_require gates no acpi_eval");

                    var require = MemoryMarshal.Read<DynDataHw.HwAcpiRequire>(body);

                    required = [];
                    requiredEvals = 0;

                    for (int r = 0; r < require.IdCount; r++)
                        required.Add(Encoding.ASCII.GetString(body.Slice(DynDataHw.AcpiRequireSize + r * DynDataHw.AcpiIdSize, DynDataHw.AcpiIdSize)).TrimEnd('\0'));
                }

                remaining = remaining[op.Length..];
            }

            if (required != null && requiredEvals == 0)
                throw Fail($"row {Index} acpi_require gates no acpi_eval");

            if ((piix4Reads != 0) != (hostRevisions.Count != 0))
                throw Fail($"row {Index} holds {piix4Reads} PIIX4 smbus_read and {hostRevisions.Count} smbus_host ops (a PIIX4 host needs its PM layout, and only it has one)");
        }

        /// <summary>
        /// Generator-only: no two BOARD rows share a hash, in either SMBIOS structure.
        /// </summary>
        private static void CheckBoardRows(ReadOnlySpan<byte> Section, UInt32 Count)
        {
            var hashes = new Dictionary<UInt64, int>();

            for (int i = 0; i < Count; i++)
            {
                var row = MemoryMarshal.Read<DynDataHw.HwIndex>(Section.Slice(DynDataHw.SectionSize + i * DynDataHw.IndexSize, DynDataHw.IndexSize));

                if (row.IdentityKind != DynDataHw.IdentityBoard)
                    continue;

                UInt64 hash = row.Identity.Board.BaseboardHash | row.Identity.Board.SystemHash;

                if (!hashes.TryAdd(hash, i))
                    throw Fail($"BOARD rows {hashes[hash]} and {i} share the hash 0x{hash:x16}");
            }
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckIdentity(ReadOnlySpan<byte> Row)
        {
            byte kind = Row[0];
            var identity = Row.Slice(DynDataHw.IndexIdentityOffset, DynDataHw.IdentitySize);
            int length = kind switch
            {
                DynDataHw.IdentityCpu => DynDataHw.CpuIdentitySize,
                DynDataHw.IdentityPci => DynDataHw.PciIdentitySize,
                DynDataHw.IdentityCpuAndPci => DynDataHw.CpuAndPciIdentitySize,
                DynDataHw.IdentityCpuAndPciClass => DynDataHw.CpuAndPciClassIdentitySize,
                DynDataHw.IdentitySuperIo => DynDataHw.SuperIoIdentitySize,
                DynDataHw.IdentityBoard => DynDataHw.BoardIdentitySize,
                DynDataHw.IdentityAcpiHid => DynDataHw.AcpiHidIdentitySize,
                DynDataHw.IdentityArmCpu => DynDataHw.ArmCpuIdentitySize,
                _ => 0
            };

            if (length == 0 || !DynDataHw.DriverKinds.IdentityKinds.Contains(kind))
                throw Fail($"identity kind {kind} is not implemented by the driver");
            if (Row[1..DynDataHw.IndexIdentityOffset].ContainsAnyExcept((byte)0))
                throw Fail("index row reserved bytes are not zero");

            switch (kind)
            {
            case DynDataHw.IdentityCpu:
            case DynDataHw.IdentityCpuAndPci:
                if (identity[15] != 0)
                    throw Fail("CPU identity reserved byte is not zero");
                break;
            case DynDataHw.IdentityCpuAndPciClass:
                if (identity[15] != 0 || identity[23] != 0)
                    throw Fail("CPU_AND_PCI_CLASS identity reserved bytes are not zero");
                if ((identity[21] & ~DynDataHw.PciHeaderLayoutMask) != 0)
                    throw Fail($"CPU_AND_PCI_CLASS header type 0x{identity[21]:x2} has bits outside the layout");
                break;
            case DynDataHw.IdentitySuperIo:
                {
                    UInt16 chip = BinaryPrimitives.ReadUInt16LittleEndian(identity[2..]);
                    UInt16 customer = BinaryPrimitives.ReadUInt16LittleEndian(identity[4..]);

                    if (identity[0] >= DynDataHw.SuperIoSlots)
                        throw Fail($"SUPERIO slot {identity[0]}");
                    if (!DynDataHw.DriverKinds.SuperIoFamilies.Contains(identity[1]))
                        throw Fail($"SUPERIO family {identity[1]} is not implemented by the driver");
                    if (chip == 0 || chip == UInt16.MaxValue)
                        throw Fail($"SUPERIO chip ID 0x{chip:x4} is what an absent chip reads");
                    if (identity[1] == DynDataHw.SuperIoFamilySmsc && !DynDataHw.SuperIoSmscChips.Contains(chip))
                        throw Fail($"SUPERIO SMSC chip ID 0x{chip:x4} is not in DynDataHw.SuperIoSmscChips");
                    if (customer != 0 && identity[1] != DynDataHw.SuperIoFamilyNuvotonEc)
                        throw Fail($"SUPERIO customer ID 0x{customer:x4} on family {identity[1]}");
                }
                break;
            case DynDataHw.IdentityBoard:
                if ((BinaryPrimitives.ReadUInt64LittleEndian(identity) == 0) == (BinaryPrimitives.ReadUInt64LittleEndian(identity[8..]) == 0))
                    throw Fail("BOARD row needs exactly one non-zero hash");
                break;
            case DynDataHw.IdentityAcpiHid:
                {
                    int idLength = identity[..DynDataHw.AcpiIdSize].IndexOf((byte)0);

                    if (idLength < 0)
                        idLength = DynDataHw.AcpiIdSize;
                    if ((idLength != 7 && idLength != 8) ||
                        identity[..idLength].ContainsAnyExcept("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"u8) ||
                        identity[idLength..DynDataHw.AcpiIdSize].ContainsAnyExcept((byte)0))
                    {
                        throw Fail($"ACPI_HID identity \"{Encoding.ASCII.GetString(identity[..DynDataHw.AcpiIdSize]).TrimEnd('\0')}\" is not 7 or 8 characters of [A-Z0-9_]");
                    }
                }
                break;
            case DynDataHw.IdentityArmCpu:
                {
                    var arm = MemoryMarshal.Read<DynDataHw.HwArmCpuIdentity>(identity);

                    if (arm.Variant > DynDataHw.ArmMaxVariant || arm.PartNum > DynDataHw.ArmMaxPartNum || arm.Revision > DynDataHw.ArmMaxRevision ||
                        identity[5..DynDataHw.ArmCpuIdentitySize].ContainsAnyExcept((byte)0))
                    {
                        throw Fail($"ARM_CPU identity variant 0x{arm.Variant:x}, part 0x{arm.PartNum:x}, revision 0x{arm.Revision:x} or reserved invalid");
                    }
                }
                break;
            }

            if (identity[length..].ContainsAnyExcept((byte)0))
                throw Fail($"identity kind {kind} has non-zero bytes past its {length} bytes");
        }

        /// <summary>
        /// Returns the op length. KphpDynHwValidateOp checks only the layout; the rest is generator-only.
        /// </summary>
        private static int CheckOp(ReadOnlySpan<byte> Remaining, ref UInt32 MaxRanges)
        {
            if (Remaining.Length < DynDataHw.OpHeaderSize)
                throw Fail("op header overruns its table");

            var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(Remaining);
            int minimum = op.Kind switch
            {
                DynDataHw.KindMsrRead => DynDataHw.MsrReadSize,
                DynDataHw.KindPciRead => DynDataHw.PciReadHeaderSize,
                DynDataHw.KindPciIndirectRead => DynDataHw.PciIndirectReadSize,
                DynDataHw.KindWindowRead => DynDataHw.WindowReadSize,
                DynDataHw.KindSuperIoRead => DynDataHw.SuperIoReadSize,
                DynDataHw.KindSmbusRead => DynDataHw.SmbusReadSize,
                DynDataHw.KindVsecRead => DynDataHw.VsecReadSize,
                DynDataHw.KindMailboxTransact => DynDataHw.MailboxTransactSize,
                DynDataHw.KindSmuTableRead => DynDataHw.SmuTableReadSize,
                DynDataHw.KindAcpiEval => DynDataHw.AcpiEvalSize,
                DynDataHw.KindAcpiRequire => DynDataHw.AcpiRequireSize,
                DynDataHw.KindSysregRead => DynDataHw.SysregReadSize,
                DynDataHw.KindPortIndirectRead => DynDataHw.PortIndirectReadSize,
                DynDataHw.KindSmbusHost => DynDataHw.SmbusHostSize,
                DynDataHw.KindDfMap => DynDataHw.DfMapSize,
                _ => -1
            };

            if (minimum < 0 || !DynDataHw.DriverKinds.OpKinds.Contains(op.Kind))
                throw Fail($"op kind {op.Kind} is not implemented by the driver");
            if (op.Length < DynDataHw.OpHeaderSize + minimum || op.Length > Remaining.Length || op.Length % DynDataHw.OpAlignment != 0)
                throw Fail($"op kind {op.Kind} length {op.Length} invalid");
            byte allowedFlags = (op.Kind == DynDataHw.KindWindowRead || op.Kind == DynDataHw.KindVsecRead) ? (byte)(DynDataHw.FlagRequireD0 | DynDataHw.FlagArm64) : (byte)0;

            if ((op.Flags & ~allowedFlags) != 0 || op.Reserved0 != 0 || op.Reserved != 0 || op.Reserved2 != 0 || op.LockClass > DynDataHw.LockSmbus)
                throw Fail($"op kind {op.Kind} sets a field no implemented kind honours");
            if (op.Kind == DynDataHw.KindWindowRead &&
                op.LockClass != (Remaining[DynDataHw.OpHeaderSize] == DynDataHw.WindowBaseSmnDerived ? DynDataHw.LockPci : DynDataHw.LockNone))
            {
                throw Fail("window_read op lock class is not PCI for an SMN_DERIVED base and none otherwise");
            }
            if ((op.Kind == DynDataHw.KindMsrRead || op.Kind == DynDataHw.KindPciRead) && op.LockClass != DynDataHw.LockNone)
                throw Fail($"op kind {op.Kind} has a lock class");
            if (op.Kind == DynDataHw.KindPciIndirectRead && op.LockClass != DynDataHw.LockPci)
                throw Fail("pci_indirect_read op lock class is not PCI");
            if (op.Kind == DynDataHw.KindVsecRead && op.LockClass != DynDataHw.LockNone)
                throw Fail("vsec_read op has a lock class");
            if (op.Kind == DynDataHw.KindAcpiEval && op.LockClass != DynDataHw.LockNone)
                throw Fail("acpi_eval op has a lock class");
            if (op.Kind == DynDataHw.KindAcpiRequire && (op.LockClass != DynDataHw.LockNone || op.Gate.Kind != DynDataHw.GateNone))
                throw Fail("acpi_require op has a lock class or a gate");
            if (op.Kind == DynDataHw.KindSysregRead && op.LockClass != DynDataHw.LockNone)
                throw Fail("sysreg_read op has a lock class");
            if (op.Kind == DynDataHw.KindSmbusHost && op.LockClass != DynDataHw.LockNone)
                throw Fail("smbus_host op has a lock class");
            if (op.Kind == DynDataHw.KindDfMap && (op.LockClass != DynDataHw.LockNone || op.Gate.Kind != DynDataHw.GateNone))
                throw Fail("df_map op has a lock class or a gate");
            if ((op.Kind == DynDataHw.KindSmbusHost) != (op.Gate.Kind == DynDataHw.GatePciConfig))
                throw Fail($"op kind {op.Kind} gate kind {op.Gate.Kind} (an smbus_host has a PCI_CONFIG gate, and nothing else does)");
            if ((op.Kind == DynDataHw.KindSuperIoRead || op.Kind == DynDataHw.KindPortIndirectRead) && op.LockClass != DynDataHw.LockIsaBus)
                throw Fail($"op kind {op.Kind} lock class is not ISA bus");
            if (op.Kind == DynDataHw.KindSmbusRead && op.LockClass != DynDataHw.LockSmbus)
                throw Fail("smbus_read op lock class is not SMBus");
            if ((op.Kind == DynDataHw.KindMailboxTransact || op.Kind == DynDataHw.KindSmuTableRead) && op.LockClass != DynDataHw.LockPci)
                throw Fail($"op kind {op.Kind} lock class is not PCI");

            CheckGate(op.Gate);

            bool mmioMailbox = op.Kind == DynDataHw.KindMailboxTransact && Remaining[DynDataHw.OpHeaderSize] == DynDataHw.MailboxTransportMmioWindow;

            if (mmioMailbox && (op.Gate.Kind != DynDataHw.GateCpuid || op.Gate.Leaf != 1 || op.Gate.SubLeaf != 0 || op.Gate.Register != DynDataHw.CpuidEax ||
                                op.Gate.Shift != 4 || op.Gate.Width != 24 || ((op.Gate.Value ^ op.Gate.ValueMax) >> 4) != 0))
                throw Fail("MMIO mailbox_transact is not gated on CPUID 1 EAX [27:4] for one block of 16 models (its ACPI_HID key names no CPU)");

            if ((op.Gate.Kind == DynDataHw.GateCpuid && op.Kind != DynDataHw.KindMsrRead && !mmioMailbox) ||
                (op.Gate.Kind == DynDataHw.GateChassis && op.Kind != DynDataHw.KindSuperIoRead) ||
                (op.Gate.Kind == DynDataHw.GateSysregField && op.Kind != DynDataHw.KindSysregRead))
            {
                throw Fail($"op kind {op.Kind} has gate kind {op.Gate.Kind}, which it does not evaluate");
            }

            var body = Remaining.Slice(DynDataHw.OpHeaderSize, op.Length - DynDataHw.OpHeaderSize);

            if (op.Kind == DynDataHw.KindMsrRead)
            {
                var msr = MemoryMarshal.Read<DynDataHw.HwMsrRead>(body);

                if (msr.ReadMask == 0 || msr.Reserved != 0)
                    throw Fail($"msr_read 0x{msr.Register:x} has a zero readmask or a non-zero reserved");
            }
            else if (op.Kind == DynDataHw.KindPciRead)
            {
                byte rangeCount = body[0];

                if (rangeCount > DynDataHw.MaxRanges)
                    throw Fail($"pci_read has {rangeCount} ranges, cap {DynDataHw.MaxRanges}");
                if (rangeCount < 1 || body[1] != 0)
                    throw Fail("pci_read has no range or non-zero reserved bytes");
                if (DynDataHw.PciReadHeaderSize + rangeCount * DynDataHw.RangeSize > body.Length)
                    throw Fail("pci_read ranges overrun the op");

                for (int i = 0; i < rangeCount; i++)
                {
                    var range = MemoryMarshal.Read<DynDataHw.HwRange>(body[(DynDataHw.PciReadHeaderSize + i * DynDataHw.RangeSize)..]);

                    if (range.Length < 1 || (UInt32)range.Offset + range.Length > DynDataHw.PciConfigLength)
                        throw Fail($"pci_read range 0x{range.Offset:x}+0x{range.Length:x} invalid");
                }

                MaxRanges = Math.Max(MaxRanges, rangeCount);
            }
            else if (op.Kind == DynDataHw.KindPciIndirectRead)
            {
                var indirect = MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(body);

                if (indirect.RangeCount > DynDataHw.MaxRanges || indirect.ValueCount > DynDataHw.MaxRanges)
                    throw Fail($"pci_indirect_read has {indirect.RangeCount} ranges / {indirect.ValueCount} values, cap {DynDataHw.MaxRanges}");
                if (indirect.RangeCount + indirect.ValueCount < 1)
                    throw Fail("pci_indirect_read has no range and no value");
                if ((indirect.IndexOffset % 4) != 0 || (indirect.DataOffset % 4) != 0 ||
                    indirect.IndexOffset >= DynDataHw.PciConfigLength || indirect.DataOffset >= DynDataHw.PciConfigLength ||
                    indirect.IndexOffset == indirect.DataOffset)
                {
                    throw Fail($"pci_indirect_read index 0x{indirect.IndexOffset:x} / data 0x{indirect.DataOffset:x} invalid");
                }

                int rangesOffset = DynDataHw.PciIndirectReadSize;
                int valuesOffset = rangesOffset + indirect.RangeCount * DynDataHw.TargetRangeSize;

                if (valuesOffset + indirect.ValueCount * sizeof(UInt32) > body.Length)
                    throw Fail("pci_indirect_read ranges and values overrun the op");

                for (int i = 0; i < indirect.RangeCount; i++)
                {
                    var range = MemoryMarshal.Read<DynDataHw.HwTargetRange>(body[(rangesOffset + i * DynDataHw.TargetRangeSize)..]);
                    var rangeBytes = body.Slice(rangesOffset + i * DynDataHw.TargetRangeSize, DynDataHw.TargetRangeSize);

                    if (range.Begin >= range.End || (range.Begin & 3) != 0 || (range.End & 3) != 0 || (range.Stride & 3) != 0 ||
                        rangeBytes[13..].ContainsAnyExcept((byte)0))
                    {
                        throw Fail($"pci_indirect_read target [0x{range.Begin:x}, 0x{range.End:x}) stride 0x{range.Stride:x} invalid");
                    }

                    if (range.StrideCount > 1 && (UInt64)range.End + (UInt64)range.Stride * (UInt64)(range.StrideCount - 1) > UInt32.MaxValue)
                        throw Fail($"pci_indirect_read target 0x{range.Begin:x} stride overflows 32 bits");
                }

                for (int i = 0; i < indirect.ValueCount; i++)
                {
                    if ((BinaryPrimitives.ReadUInt32LittleEndian(body[(valuesOffset + i * sizeof(UInt32))..]) & 3) != 0)
                        throw Fail("pci_indirect_read value not 4-aligned");
                }

                MaxRanges = Math.Max(MaxRanges, Math.Max(indirect.RangeCount, indirect.ValueCount));
            }
            else if (op.Kind == DynDataHw.KindWindowRead)
            {
                CheckWindowOp(body);

                if (body[0] != DynDataHw.WindowBaseStandardBar && body[0] != DynDataHw.WindowBaseConfigReg && op.Flags != 0)
                    throw Fail($"window_read base kind {body[0]} has a flag (no PCI power state or ARM64 read)");

                MaxRanges = Math.Max(MaxRanges, body[28]);
            }
            else if (op.Kind == DynDataHw.KindAcpiEval)
            {
                CheckAcpiEvalOp(body);
            }
            else if (op.Kind == DynDataHw.KindAcpiRequire)
            {
                CheckAcpiRequireOp(body);
            }
            else if (op.Kind == DynDataHw.KindSmbusHost)
            {
                var host = MemoryMarshal.Read<DynDataHw.HwSmbusHost>(body);

                if (op.Length != DynDataHw.AlignOp(DynDataHw.OpHeaderSize + DynDataHw.SmbusHostSize))
                    throw Fail("smbus_host op length is not its body");
                string error = DynDataHw.SmbusHostError(host);

                if (error != null)
                    throw Fail($"smbus_host: {error}");
            }
            else if (op.Kind == DynDataHw.KindDfMap)
            {
                if (op.Length != DynDataHw.AlignOp(DynDataHw.OpHeaderSize + DynDataHw.DfMapSize))
                    throw Fail("df_map op length is not its body");

                string error = DynDataHw.DfMapError(MemoryMarshal.Read<DynDataHw.HwDfMap>(body));

                if (error != null)
                    throw Fail($"df_map: {error}");
            }
            else if (op.Kind == DynDataHw.KindPortIndirectRead)
            {
                CheckPortIndirectOp(body);

                MaxRanges = Math.Max(MaxRanges, body[3]);
            }
            else if (op.Kind == DynDataHw.KindSysregRead)
            {
                var sysreg = MemoryMarshal.Read<DynDataHw.HwSysregRead>(body);

                if (!DynDataHw.DriverKinds.SysregRegisters.Contains(sysreg.SysregEnum))
                    throw Fail($"sysreg_read register {sysreg.SysregEnum} is not implemented by the driver");
                if (body[1..8].ContainsAnyExcept((byte)0))
                    throw Fail($"sysreg_read register {sysreg.SysregEnum} has non-zero reserved bytes");
                if (sysreg.ReadMask == 0)
                    throw Fail($"sysreg_read register {sysreg.SysregEnum} has a zero readmask");
            }
            else if (op.Kind == DynDataHw.KindSuperIoRead)
            {
                var superIo = MemoryMarshal.Read<DynDataHw.HwSuperIoRead>(body);

                if (superIo.RangeCount > DynDataHw.MaxRanges)
                    throw Fail($"superio_read has {superIo.RangeCount} ranges, cap {DynDataHw.MaxRanges}");
                if (superIo.RangeCount < 1 || superIo.Exit > DynDataHw.SuperIoExitConfig02)
                    throw Fail("superio_read has no range or an unknown exit");
                if (superIo.IdCount > DynDataHw.SuperIoMaxIds || (superIo.Flags & ~DynDataHw.SuperIoFlagMask) != 0)
                    throw Fail($"superio_read has {superIo.IdCount} ids (cap {DynDataHw.SuperIoMaxIds}) or flags 0x{superIo.Flags:x2}");
                if (superIo.ConfigPort == 0 || superIo.ConfigPort == UInt16.MaxValue)
                    throw Fail($"superio_read configuration port 0x{superIo.ConfigPort:x}");
                if (superIo.ConfigIdIndex == 0 ? superIo.ConfigIdValue != 0 : superIo.ConfigIdIndex == byte.MaxValue)
                    throw Fail($"superio_read config-id 0x{superIo.ConfigIdIndex:x2}:0x{superIo.ConfigIdValue:x4}");
                if (superIo.Reserved != 0 ||
                    body[(DynDataHw.SuperIoIdsOffset + superIo.IdCount * DynDataHw.SuperIoIdSize)..DynDataHw.SuperIoReadSize].ContainsAnyExcept((byte)0))
                    throw Fail("superio_read reserved bytes or unused ids are non-zero");

                // The family is on the row; the 8-port extent is the floor every family shares.
                string baseError = DynDataHw.SuperIoBaseRangeError(superIo.BaseMin, superIo.BaseMax, DynDataHw.SuperIoBaseExtent(0));

                if (baseError != null)
                    throw Fail($"superio_read {baseError}");
                if (DynDataHw.SuperIoReadSize + superIo.RangeCount * DynDataHw.RangeSize > body.Length)
                    throw Fail("superio_read ranges overrun the op");

                for (int i = 0; i < superIo.RangeCount; i++)
                {
                    var range = MemoryMarshal.Read<DynDataHw.HwRange>(body[(DynDataHw.SuperIoReadSize + i * DynDataHw.RangeSize)..]);

                    if (range.Length < 1 || (range.Offset & 0xFF) + range.Length > 0x100)
                        throw Fail($"superio_read range 0x{range.Offset:x}+0x{range.Length:x} invalid");
                }

                MaxRanges = Math.Max(MaxRanges, superIo.RangeCount);
            }
            else if (op.Kind == DynDataHw.KindSmbusRead)
            {
                CheckSmbusOp(body);

                MaxRanges = Math.Max(MaxRanges, body[3]);
            }
            else if (op.Kind == DynDataHw.KindVsecRead)
            {
                CheckVsecOp(body);

                MaxRanges = Math.Max(MaxRanges, body[16]);
            }
            else if (op.Kind == DynDataHw.KindMailboxTransact)
            {
                CheckMailboxOp(body);
            }
            else if (op.Kind == DynDataHw.KindSmuTableRead)
            {
                CheckSmuTableOp(body);
            }

            return op.Length;
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckMailboxOp(ReadOnlySpan<byte> Body)
        {
            var mailbox = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(Body);
            var smn = mailbox.Address;
            var address = Body.Slice(DynDataHw.MailboxAddressOffset, DynDataHw.MailboxAddressSize);
            var window = MemoryMarshal.Read<DynDataHw.HwMailboxWindow>(address);
            bool mmio = mailbox.Transport == DynDataHw.MailboxTransportMmioWindow;

            if (!DynDataHw.DriverKinds.MailboxTransports.Contains(mailbox.Transport))
                throw Fail($"mailbox_transact transport {mailbox.Transport} is not implemented by the driver");
            if (!DynDataHw.DriverKinds.MailboxProtocolKinds.Contains(mailbox.Protocol))
                throw Fail($"mailbox_transact protocol {mailbox.Protocol} is not implemented by the driver");
            if (mailbox.CommandCount > DynDataHw.MailboxMaxCommands)
                throw Fail($"mailbox_transact has {mailbox.CommandCount} commands, cap {DynDataHw.MailboxMaxCommands}");
            if (DynDataHw.MailboxTransactSize + mailbox.CommandCount * DynDataHw.MailboxCommandSize > Body.Length)
                throw Fail("mailbox_transact commands overrun the op");
            if (mailbox.CommandCount < 1 || mailbox.ArgCount < 1 || mailbox.ArgCount > DynDataHw.MailboxMaxArgs ||
                mailbox.VersionOpcode < 1 || mailbox.VersionOpcode > DynDataHw.MailboxMaxOpcode)
            {
                throw Fail("mailbox_transact has no command or a bad argument count or version opcode");
            }
            if (mmio)
            {
                if (window.BaseKind != DynDataHw.WindowBaseAcpiCrs || address[2..4].ContainsAnyExcept((byte)0) ||
                    window.WindowSize == 0 || window.WindowSize > DynDataHw.MaxWindowSize || (window.WindowSize & (window.WindowSize - 1)) != 0)
                {
                    throw Fail($"MMIO mailbox_transact base kind {window.BaseKind} / size 0x{window.WindowSize:x} / reserved bytes invalid");
                }
            }
            else if (mailbox.Transport != DynDataHw.MailboxTransportSmnPair)
            {
                throw Fail($"mailbox_transact transport {mailbox.Transport} has no check");
            }
            else if ((smn.IndexOffset % 4) != 0 || (smn.DataOffset % 4) != 0 ||
                smn.IndexOffset < DynDataHw.PciConfigHeaderLength || smn.DataOffset < DynDataHw.PciConfigHeaderLength ||
                smn.IndexOffset >= DynDataHw.PciConfigLength || smn.DataOffset >= DynDataHw.PciConfigLength ||
                smn.IndexOffset == smn.DataOffset || smn.PinDevice > 31 || smn.PinFunction > 7 ||
                address[6..8].ContainsAnyExcept((byte)0))
            {
                throw Fail($"mailbox_transact index 0x{smn.IndexOffset:x} / data 0x{smn.DataOffset:x} / pin invalid");
            }

            // Msg, Rsp and Arg0 sit at the same offsets in the SMN and window members.
            UInt64 argEnd = (UInt64)smn.Arg0 + 4ul * mailbox.ArgCount;

            if ((smn.Msg % 4) != 0 || (smn.Rsp % 4) != 0 || (smn.Arg0 % 4) != 0 || argEnd > UInt32.MaxValue + 1ul ||
                smn.Msg == smn.Rsp || (smn.Msg >= smn.Arg0 && smn.Msg < argEnd) || (smn.Rsp >= smn.Arg0 && smn.Rsp < argEnd))
            {
                throw Fail($"mailbox_transact msg 0x{smn.Msg:x} rsp 0x{smn.Rsp:x} arg 0x{smn.Arg0:x} invalid");
            }

            if (mmio)
            {
                UInt32 page = window.Msg / DynDataHw.MailboxWindowPage;

                if ((UInt64)window.Msg + 4 > window.WindowSize || (UInt64)window.Rsp + 4 > window.WindowSize || argEnd > window.WindowSize ||
                    window.Rsp / DynDataHw.MailboxWindowPage != page || window.Arg0 / DynDataHw.MailboxWindowPage != page ||
                    (argEnd - 4) / DynDataHw.MailboxWindowPage != page)
                {
                    throw Fail($"MMIO mailbox_transact msg 0x{window.Msg:x} rsp 0x{window.Rsp:x} arg 0x{window.Arg0:x} not inside one page of the 0x{window.WindowSize:x} window");
                }
            }

            for (int i = 0; i < mailbox.CommandCount; i++)
            {
                var command = Body.Slice(DynDataHw.MailboxTransactSize + i * DynDataHw.MailboxCommandSize, DynDataHw.MailboxCommandSize);
                UInt32 opcode = BinaryPrimitives.ReadUInt32LittleEndian(command);

                if (opcode < 1 || opcode > DynDataHw.MailboxMaxOpcode)
                    throw Fail($"mailbox_transact opcode 0x{opcode:x} invalid");
                if (command[4] != DynDataHw.MailboxCommandGet)
                    throw Fail($"mailbox_transact opcode 0x{opcode:x2} is not a GET");
                if (command[5] > mailbox.ArgCount || (command[6] & ~DynDataHw.MailboxCommandInternal) != 0 || command[7] != 0 || (mmio && command[6] != 0))
                    throw Fail($"mailbox_transact opcode 0x{opcode:x2} response words, flags or reserved invalid");
                for (int j = 0; j < i; j++)
                {
                    var earlier = Body.Slice(DynDataHw.MailboxTransactSize + j * DynDataHw.MailboxCommandSize, DynDataHw.MailboxCommandSize);

                    if (BinaryPrimitives.ReadUInt32LittleEndian(earlier) == opcode)
                        throw Fail($"mailbox_transact opcode 0x{opcode:x2} repeats");
                }

                for (int a = 0; a < DynDataHw.MailboxMaxArgs; a++)
                {
                    UInt32 mask = BinaryPrimitives.ReadUInt32LittleEndian(command[(8 + a * 4)..]);
                    UInt32 max = BinaryPrimitives.ReadUInt32LittleEndian(command[(40 + a * 4)..]);
                    UInt32 fixedValue = BinaryPrimitives.ReadUInt32LittleEndian(command[(72 + a * 4)..]);

                    if ((max & ~mask) != 0 || (fixedValue & mask) != 0 ||
                        (a >= mailbox.ArgCount && (mask | max | fixedValue) != 0))
                    {
                        throw Fail($"mailbox_transact opcode 0x{opcode:x2} argument {a} mask/max/fixed invalid");
                    }
                }
            }
        }

        /// <summary>
        /// Generator-only; the mailbox reference is checked per table.
        /// </summary>
        private static void CheckSmuTableOp(ReadOnlySpan<byte> Body)
        {
            var table = MemoryMarshal.Read<DynDataHw.HwSmuTableRead>(Body);

            if (!DynDataHw.DriverKinds.SmuTableBounds.Contains(table.Bound))
                throw Fail($"smu_table_read bound {table.Bound} is not implemented by the driver");
            if (table.BaseLowWord >= DynDataHw.MailboxMaxArgs ||
                (table.BaseHighWord != DynDataHw.SmuTableWordNone && (table.BaseHighWord >= DynDataHw.MailboxMaxArgs || table.BaseHighWord == table.BaseLowWord)) ||
                (table.SizeWord != DynDataHw.SmuTableWordNone && (table.SizeWord >= DynDataHw.MailboxMaxArgs || table.SizeWord == table.BaseLowWord || table.SizeWord == table.BaseHighWord)))
            {
                throw Fail("smu_table_read reply words are out of range or not distinct");
            }
            if (table.VersionCount > DynDataHw.SmuTableMaxVersions)
                throw Fail($"smu_table_read has {table.VersionCount} versions, cap {DynDataHw.SmuTableMaxVersions}");
            if (DynDataHw.SmuTableReadSize + table.VersionCount * DynDataHw.SmuTableVersionSize > Body.Length)
                throw Fail("smu_table_read versions overrun the op");
            if (table.VersionCount < 1 ||
                (table.SetupOpcode != 0 && (table.SetupOpcode == table.BaseOpcode || table.SetupOpcode == table.VersionOpcode || table.SetupOpcode == table.TransferOpcode)) ||
                table.VersionOpcode < 1 || table.VersionOpcode > DynDataHw.MailboxMaxOpcode ||
                table.BaseOpcode < 1 || table.BaseOpcode > DynDataHw.MailboxMaxOpcode ||
                table.TransferOpcode > DynDataHw.MailboxMaxOpcode ||
                table.VersionOpcode == table.BaseOpcode || table.VersionOpcode == table.TransferOpcode)
            {
                throw Fail("smu_table_read has no version, a bad or shared opcode, or a bad setup opcode");
            }

            for (int i = 0; i < table.VersionCount; i++)
            {
                var version = MemoryMarshal.Read<DynDataHw.HwSmuTableVersion>(Body[(DynDataHw.SmuTableReadSize + i * DynDataHw.SmuTableVersionSize)..]);

                if (version.Size < 4 || version.Size > DynDataHw.SmuTableMaxSize || (version.Size % 4) != 0)
                    throw Fail($"smu_table_read version 0x{version.Version:x} size 0x{version.Size:x} invalid");
            }
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        internal static void CheckMailboxRefs(ReadOnlySpan<byte> Table, int Index)
        {
            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(Table).Count;
            var ops = new List<(UInt16 Kind, int Body, int Length)>();
            int offset = DynDataHw.TableSize;

            for (UInt32 j = 0; j < count; j++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(Table[offset..]);

                ops.Add((op.Kind, offset + DynDataHw.OpHeaderSize, op.Length - DynDataHw.OpHeaderSize));
                offset += op.Length;
            }

            foreach (var (kind, bodyOffset, _) in ops)
            {
                if (kind != DynDataHw.KindMailboxTransact)
                    continue;

                var body = Table[bodyOffset..];
                var mailbox = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(body);
                var smn = mailbox.Address;
                bool smnPair = mailbox.Transport == DynDataHw.MailboxTransportSmnPair;
                UInt64 argEnd = smn.Arg0 + 4ul * mailbox.ArgCount;
                var internalOpcodes = new HashSet<UInt32>();

                for (int c = 0; c < mailbox.CommandCount; c++)
                {
                    var command = body.Slice(DynDataHw.MailboxTransactSize + c * DynDataHw.MailboxCommandSize, DynDataHw.MailboxCommandSize);

                    if ((command[6] & DynDataHw.MailboxCommandInternal) != 0)
                        internalOpcodes.Add(BinaryPrimitives.ReadUInt32LittleEndian(command));
                }

                bool Overlaps(UInt64 Begin, UInt64 End)
                {
                    return (Begin < (UInt64)smn.Msg + 4 && smn.Msg < End) ||
                           (Begin < (UInt64)smn.Rsp + 4 && smn.Rsp < End) ||
                           (Begin < argEnd && smn.Arg0 < End);
                }

                foreach (var (otherKind, otherOffset, _) in ops)
                {
                    var other = Table[otherOffset..];

                    if (otherKind == DynDataHw.KindMailboxTransact)
                    {
                        var otherMailbox = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(other);

                        var o = otherMailbox.Address;

                        if (otherMailbox.Transport != mailbox.Transport ||
                            (o.Msg != smn.Msg && o.Msg != smn.Rsp && o.Rsp != smn.Msg && o.Rsp != smn.Rsp))
                        {
                            continue;
                        }

                        for (int c = 0; c < otherMailbox.CommandCount; c++)
                        {
                            var command = other.Slice(DynDataHw.MailboxTransactSize + c * DynDataHw.MailboxCommandSize, DynDataHw.MailboxCommandSize);
                            UInt32 opcode = BinaryPrimitives.ReadUInt32LittleEndian(command);

                            if (opcode == mailbox.VersionOpcode && (command[6] & DynDataHw.MailboxCommandInternal) != 0)
                                throw Fail($"row {Index} mailbox version opcode 0x{opcode:x2} is internal on mailbox 0x{smn.Msg:x}");

                            if (internalOpcodes.Contains(opcode) && (command[6] & DynDataHw.MailboxCommandInternal) == 0)
                                throw Fail($"row {Index} mailbox opcode 0x{opcode:x2} is internal on mailbox 0x{smn.Msg:x} and also a client command there");
                        }
                    }
                    else if (!smnPair)
                    {
                        if (otherKind == DynDataHw.KindAcpiEval)
                            throw Fail($"row {Index} acpi_eval in the table of MMIO mailbox 0x{smn.Msg:x} (what AML reads is not known)");

                        if (otherKind == DynDataHw.KindWindowRead)
                        {
                            for (int r = 0; r < MemoryMarshal.Read<DynDataHw.HwWindowRead>(other).RangeCount; r++)
                            {
                                var range = MemoryMarshal.Read<DynDataHw.HwWindowRange>(other[(DynDataHw.WindowReadSize + r * DynDataHw.WindowRangeSize)..]);

                                if (Overlaps(range.Offset, (UInt64)range.Offset + range.Length))
                                    throw Fail($"row {Index} window range 0x{range.Offset:x}+0x{range.Length:x} overlaps the registers of MMIO mailbox 0x{smn.Msg:x}");
                            }
                        }
                    }
                    else if (otherKind == DynDataHw.KindPciIndirectRead)
                    {
                        var indirect = MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(other);
                        int valuesOffset = DynDataHw.PciIndirectReadSize + indirect.RangeCount * DynDataHw.TargetRangeSize;

                        for (int r = 0; r < indirect.RangeCount; r++)
                        {
                            var range = MemoryMarshal.Read<DynDataHw.HwTargetRange>(other[(DynDataHw.PciIndirectReadSize + r * DynDataHw.TargetRangeSize)..]);

                            for (UInt64 k = 0; k < Math.Max((UInt64)range.StrideCount, 1); k++)
                            {
                                if (Overlaps(range.Begin + range.Stride * k, range.End + range.Stride * k))
                                    throw Fail($"row {Index} pci_indirect_read target 0x{range.Begin:x} overlaps the registers of mailbox 0x{smn.Msg:x}");
                            }
                        }

                        for (int v = 0; v < indirect.ValueCount; v++)
                        {
                            UInt32 value = BinaryPrimitives.ReadUInt32LittleEndian(other[(valuesOffset + v * sizeof(UInt32))..]);

                            if (Overlaps(value, (UInt64)value + 4))
                                throw Fail($"row {Index} pci_indirect_read value 0x{value:x} is a register of mailbox 0x{smn.Msg:x}");
                        }
                    }
                    else if (otherKind == DynDataHw.KindPciRead)
                    {
                        for (int r = 0; r < other[0]; r++)
                        {
                            var range = MemoryMarshal.Read<DynDataHw.HwRange>(other[(DynDataHw.PciReadHeaderSize + r * DynDataHw.RangeSize)..]);
                            UInt32 begin = range.Offset;
                            UInt32 end = begin + range.Length;

                            if ((begin < smn.IndexOffset + 4u && smn.IndexOffset < end) || (begin < smn.DataOffset + 4u && smn.DataOffset < end))
                                throw Fail($"row {Index} pci_read range 0x{range.Offset:x}+0x{range.Length:x} covers the SMN pair of mailbox 0x{smn.Msg:x}");
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckSmuTableRefs(ReadOnlySpan<byte> Table, int Index)
        {
            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(Table).Count;
            var mailboxes = new List<int>();
            var tables = new List<int>();
            int dfMaps = 0;
            int offset = DynDataHw.TableSize;

            for (UInt32 j = 0; j < count; j++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(Table[offset..]);

                if (op.Kind == DynDataHw.KindMailboxTransact)
                    mailboxes.Add(offset + DynDataHw.OpHeaderSize);
                else if (op.Kind == DynDataHw.KindSmuTableRead)
                    tables.Add(offset + DynDataHw.OpHeaderSize);
                else if (op.Kind == DynDataHw.KindDfMap)
                    dfMaps++;

                offset += op.Length;
            }

            foreach (int tableOffset in tables)
            {
                var table = MemoryMarshal.Read<DynDataHw.HwSmuTableRead>(Table[tableOffset..]);

                if (table.MailboxOp >= mailboxes.Count)
                    throw Fail($"row {Index} smu_table_read names mailbox op {table.MailboxOp} of {mailboxes.Count}");
                if (table.DfMapOp != DynDataHw.SmuTableDfMapNone && table.DfMapOp >= dfMaps)
                    throw Fail($"row {Index} smu_table_read names df_map op {table.DfMapOp} of {dfMaps}");

                var mailboxBody = Table[mailboxes[table.MailboxOp]..];
                var mailbox = MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(mailboxBody);

                void Require(ReadOnlySpan<byte> MailboxBody, UInt32 Opcode, int ResponseWords, bool Internal)
                {
                    bool found = false;

                    for (int c = 0; c < mailbox.CommandCount; c++)
                    {
                        var command = MailboxBody.Slice(DynDataHw.MailboxTransactSize + c * DynDataHw.MailboxCommandSize, DynDataHw.MailboxCommandSize);

                        if (BinaryPrimitives.ReadUInt32LittleEndian(command) != Opcode)
                            continue;

                        found = true;

                        if (command[5] < ResponseWords || (Internal && (command[6] & DynDataHw.MailboxCommandInternal) == 0))
                            throw Fail($"row {Index} smu_table_read opcode 0x{Opcode:x2} reads back too few words or is not internal");
                    }

                    if (!found)
                        throw Fail($"row {Index} smu_table_read opcode 0x{Opcode:x2} is not a command of its mailbox op");
                }

                Require(mailboxBody, table.VersionOpcode, 1, false);
                Require(mailboxBody, table.BaseOpcode, DynDataHw.SmuTableBaseWords(table.BaseLowWord, table.BaseHighWord, table.SizeWord), true);

                if (table.TransferOpcode != 0)
                    Require(mailboxBody, table.TransferOpcode, 0, true);
                if (table.SetupOpcode != 0)
                    Require(mailboxBody, table.SetupOpcode, 0, true);
            }
        }

        /// <summary>
        /// smbus_read body values; generator-only, KphpDynHwValidateOp checks the layout.
        /// </summary>
        private static void CheckSmbusOp(ReadOnlySpan<byte> Body)
        {
            var smbus = MemoryMarshal.Read<DynDataHw.HwSmbusRead>(Body);
            var slaves = Body.Slice(DynDataHw.SmbusSlavesOffset, DynDataHw.SmbusSlaveMapSize);
            bool anySlave = false;

            if (!DynDataHw.DriverKinds.SmbusHosts.Contains(smbus.HostKind))
                throw Fail($"smbus_read host kind {smbus.HostKind} is not implemented by the driver");
            if (smbus.RangeCount > DynDataHw.MaxRanges)
                throw Fail($"smbus_read has {smbus.RangeCount} ranges, cap {DynDataHw.MaxRanges}");
            if (smbus.RangeCount < 1 || smbus.Protocols == 0 || (smbus.Protocols & ~DynDataHw.SmbusProtocolMask) != 0)
            {
                throw Fail("smbus_read has no range, no or an unknown protocol, or non-zero reserved bytes");
            }
            if (smbus.HostKind != DynDataHw.SmbusHostI801 && smbus.HostKind != DynDataHw.SmbusHostPiix4)
                throw Fail($"smbus_read host kind {smbus.HostKind} has no check");
            if (smbus.HostKind == DynDataHw.SmbusHostPiix4 ? smbus.Port >= DynDataHw.SmbusPorts : smbus.Port != 0)
                throw Fail($"smbus_read port {smbus.Port} invalid for host kind {smbus.HostKind}");
            if (DynDataHw.SmbusReadSize + smbus.RangeCount * DynDataHw.RangeSize > Body.Length)
                throw Fail("smbus_read ranges overrun the op");

            for (int address = 0; address < 0x80; address++)
            {
                if ((slaves[address >> 3] & (1 << (address & 7))) == 0)
                    continue;
                if (DynDataHw.SmbusAddressReserved(address) || DynDataHw.SmbusAddressNotDevice(address))
                    throw Fail($"smbus_read slave 0x{address:x2} is a reserved, host, alert response, EE1004 select or ARP address");

                anySlave = true;
            }

            if (!anySlave)
                throw Fail("smbus_read has no slave");

            var ranges = new List<DynDataHw.HwRange>();

            for (int i = 0; i < smbus.RangeCount; i++)
            {
                var range = MemoryMarshal.Read<DynDataHw.HwRange>(Body[(DynDataHw.SmbusReadSize + i * DynDataHw.RangeSize)..]);

                if (range.Length < 1 || (UInt32)range.Offset + range.Length > 0x100)
                    throw Fail($"smbus_read command range 0x{range.Offset:x}+0x{range.Length:x} invalid");

                ranges.Add(range);
            }

            bool spdSlave = false;

            for (int address = 0; address < 0x80; address++)
                spdSlave |= DynDataHw.SmbusSpdSlave(address) && (slaves[address >> 3] & (1 << (address & 7))) != 0;

            if (DynDataHw.SmbusWordReadsPastRange(smbus.Protocols, spdSlave, ranges))
                throw Fail("smbus_read allows word reads on a byte-addressed device but a range is not the whole command space");
        }

        /// <summary>
        /// vsec_read body values; generator-only, KphpDynHwValidateOp checks the layout.
        /// </summary>
        private static void CheckVsecOp(ReadOnlySpan<byte> Body)
        {
            var vsec = MemoryMarshal.Read<DynDataHw.HwVsecRead>(Body);

            if (!DynDataHw.DriverKinds.VsecDiscoveries.Contains(vsec.Discovery))
                throw Fail($"vsec_read discovery {vsec.Discovery} is not implemented by the driver");
            if (vsec.RangeCount > DynDataHw.MaxRanges)
                throw Fail($"vsec_read has {vsec.RangeCount} ranges, cap {DynDataHw.MaxRanges}");
            if (vsec.RangeCount < 1 || vsec.FeatureCapsMask != 0)
                throw Fail("vsec_read has no range or sets a field no implemented discovery honours");
            if (vsec.Revision > DynDataHw.VsecRevisionMax)
                throw Fail($"vsec_read header revision {vsec.Revision} above 0x{DynDataHw.VsecRevisionMax:x}");
            if (vsec.MaxRegions < 1 || vsec.MaxRegions > DynDataHw.VsecMaxRegions)
                throw Fail($"vsec_read max regions {vsec.MaxRegions} outside [1, {DynDataHw.VsecMaxRegions}]");
            if (vsec.MaxRegionSize < 4 || vsec.MaxRegionSize > DynDataHw.VsecMaxRegionSize || (vsec.MaxRegionSize % 4) != 0)
                throw Fail($"vsec_read max region size 0x{vsec.MaxRegionSize:x} invalid");
            if ((vsec.Quirks & ~DynDataHw.VsecQuirkMask) != 0)
                throw Fail($"vsec_read quirks 0x{vsec.Quirks:x2} unknown");

            if (vsec.Discovery == DynDataHw.VsecDiscoveryPmtTelemetry)
            {
                if (vsec.FeatureId != 0)
                    throw Fail("vsec_read PMT telemetry has a feature ID");
            }
            else if (vsec.Discovery == DynDataHw.VsecDiscoveryTpmi)
            {
                if (vsec.Guid != 0 || (vsec.Quirks & DynDataHw.VsecQuirkEarlyHw) != 0)
                    throw Fail("vsec_read TPMI has a GUID or the early-hw quirk");
                if (vsec.FeatureId == DynDataHw.VsecTpmiControl)
                    throw Fail("vsec_read TPMI feature 0x80 (TPMI_CONTROL) is a mailbox");
            }
            else
            {
                throw Fail($"vsec_read discovery {vsec.Discovery} has no check");
            }

            if (DynDataHw.VsecReadSize + vsec.RangeCount * DynDataHw.WindowRangeSize > Body.Length)
                throw Fail("vsec_read ranges overrun the op");

            for (int i = 0; i < vsec.RangeCount; i++)
            {
                var rangeBytes = Body.Slice(DynDataHw.VsecReadSize + i * DynDataHw.WindowRangeSize, DynDataHw.WindowRangeSize);
                var range = MemoryMarshal.Read<DynDataHw.HwWindowRange>(rangeBytes);

                if ((range.Width != 4 && range.Width != 8) ||
                    rangeBytes[9..].ContainsAnyExcept((byte)0) ||
                    range.Length < range.Width || (range.Length % range.Width) != 0 || (range.Offset % range.Width) != 0 ||
                    (UInt64)range.Offset + range.Length > vsec.MaxRegionSize)
                {
                    throw Fail($"vsec_read range 0x{range.Offset:x}+0x{range.Length:x} width {range.Width} invalid");
                }
            }
        }

        /// <summary>
        /// window_read body values; generator-only, KphpDynHwValidateOp checks the layout.
        /// </summary>
        private static void CheckWindowOp(ReadOnlySpan<byte> Body)
        {
            var window = MemoryMarshal.Read<DynDataHw.HwWindowRead>(Body);
            var header = Body[..DynDataHw.WindowReadSize];
            bool portSpace = window.BaseKind == DynDataHw.WindowBaseFixedIo;
            bool configFields = window.EnableBit != DynDataHw.WindowEnableNone || window.ConfigOffset != 0 || window.Is64 != 0 || window.ConfigMask != 0;

            if (!DynDataHw.DriverKinds.WindowBaseKinds.Contains(window.BaseKind))
                throw Fail($"window_read base kind {window.BaseKind} is not implemented by the driver");
            if (window.RangeCount > DynDataHw.MaxRanges)
                throw Fail($"window_read has {window.RangeCount} ranges, cap {DynDataHw.MaxRanges}");
            if (DynDataHw.WindowReadSize + window.RangeCount * DynDataHw.WindowRangeSize + window.AuxCount * sizeof(UInt32) > Body.Length)
                throw Fail("window_read ranges and aux words overrun the op");
            if (window.RangeCount < 1 || window.Width != 0 || header[30] != 0 || header[31] != 0)
                throw Fail("window_read has no range or non-zero reserved bytes");
            if (window.WindowSize == 0 || window.WindowSize > DynDataHw.MaxWindowSize || (window.WindowSize & (window.WindowSize - 1)) != 0)
                throw Fail($"window_read size 0x{window.WindowSize:x} invalid");
            if (window.SpaceKind != (portSpace ? DynDataHw.WindowSpaceIo : DynDataHw.WindowSpaceMmio))
                throw Fail($"window_read base kind {window.BaseKind} space kind {window.SpaceKind} invalid (port space is FIXED_IO only)");

            var aux = new UInt32[window.AuxCount];

            for (int i = 0; i < aux.Length; i++)
                aux[i] = BinaryPrimitives.ReadUInt32LittleEndian(Body[(DynDataHw.WindowReadSize + window.RangeCount * DynDataHw.WindowRangeSize + i * sizeof(UInt32))..]);

            if (window.BaseKind == DynDataHw.WindowBaseStandardBar)
            {
                if (window.BarIndex >= DynDataHw.PciBarCount || configFields || window.FixedBase != 0 || window.AuxCount != 0)
                    throw Fail($"window_read bar {window.BarIndex} invalid");
            }
            else if (window.BaseKind == DynDataHw.WindowBaseAcpiCrs || window.BaseKind == DynDataHw.WindowBaseAcpiLpit)
            {
                if (configFields || window.FixedBase != 0 || window.AuxCount != 0)
                    throw Fail($"window_read base kind {window.BaseKind} sets a configuration register field, a fixed base or aux words");

                if (window.BaseKind == DynDataHw.WindowBaseAcpiLpit)
                {
                    var range = MemoryMarshal.Read<DynDataHw.HwWindowRange>(Body[DynDataHw.WindowReadSize..]);

                    //
                    // The one read is the whole counter, at its width.
                    //
                    if (window.BarIndex >= DynDataHw.AcpiLpitMaxEntries || (window.WindowSize != 4 && window.WindowSize != 8) ||
                        window.RangeCount != 1 || range.Offset != 0 || range.Length != window.WindowSize || range.Width != window.WindowSize)
                    {
                        throw Fail($"window_read LPIT entry {window.BarIndex} size 0x{window.WindowSize:x} invalid");
                    }
                }
            }
            else if (window.BaseKind == DynDataHw.WindowBaseConfigReg)
            {
                UInt32 width = window.Is64 != 0 ? 8u : 4u;

                if (window.BarIndex != 0 || window.Is64 > 1 || window.FixedBase != 0 || window.AuxCount != 0 || (window.ConfigOffset % 4) != 0 ||
                    window.ConfigOffset < DynDataHw.PciConfigHeaderLength || window.ConfigOffset + width > DynDataHw.PciConfigLength ||
                    window.ConfigMask == 0 || (width == 4 && (window.ConfigMask >> 32) != 0) ||
                    (window.ConfigMask & (window.WindowSize - 1)) != 0)
                {
                    throw Fail($"window_read config register 0x{window.ConfigOffset:x} mask 0x{window.ConfigMask:x} invalid");
                }

                if (window.EnableBit != DynDataHw.WindowEnableNone &&
                    (window.EnableBit >= width * 8 || (window.ConfigMask & (1ul << window.EnableBit)) != 0))
                {
                    throw Fail($"window_read enable bit {window.EnableBit} invalid");
                }
            }
            else if (window.BaseKind == DynDataHw.WindowBaseSmnDerived)
            {
                if (window.BarIndex != 0 || window.ConfigOffset != 0 || window.Is64 != 0 || window.AuxCount != 2 ||
                    (aux[0] % 4) != 0 || (aux[1] % 4) != 0 || aux[0] == aux[1] ||
                    (window.FixedBase % window.WindowSize) != 0 || window.FixedBase > DynDataHw.SmnApertureSize - window.WindowSize)
                {
                    throw Fail($"window_read SMN_DERIVED offset 0x{window.FixedBase:x} size 0x{window.WindowSize:x} or SMN addresses invalid");
                }

                if (window.EnableBit == DynDataHw.WindowEnableNone || window.EnableBit > 63 ||
                    window.ConfigMask == 0 || (window.ConfigMask & (DynDataHw.SmnApertureSize - 1)) != 0 ||
                    (window.ConfigMask & (1ul << window.EnableBit)) != 0)
                {
                    throw Fail($"window_read SMN_DERIVED mask 0x{window.ConfigMask:x} or enable bit {window.EnableBit} invalid");
                }
            }
            else if (window.BaseKind == DynDataHw.WindowBaseFixedMmio)
            {
                if (window.BarIndex != 0 || configFields || window.AuxCount > 1 ||
                    window.FixedBase == 0 || (window.FixedBase % window.WindowSize) != 0 || window.FixedBase > 0x100000000ul - window.WindowSize)
                {
                    throw Fail($"window_read FIXED_MMIO base 0x{window.FixedBase:x} size 0x{window.WindowSize:x} invalid");
                }

                if (window.AuxCount == 1 &&
                    ((aux[0] & 0xFFFF) == 0 || (aux[0] & 0xFFFF) == 0xFFFF || (aux[0] >> 16) == 0 || (aux[0] >> 16) == 0xFFFF))
                {
                    throw Fail($"window_read FIXED_MMIO presence device 0x{aux[0]:x8} invalid");
                }
            }
            else if (window.BaseKind == DynDataHw.WindowBaseFixedIo)
            {
                UInt32 signatureOffset = window.AuxCount == 2 ? aux[0] & 0xFFFF : 0;
                UInt32 signatureLength = window.AuxCount == 2 ? aux[0] >> 16 : 0;

                if (window.BarIndex != 0 || configFields || window.AuxCount != 2 ||
                    window.WindowSize > DynDataHw.FixedIoMaxSize || window.FixedBase < DynDataHw.FixedIoMinBase ||
                    (window.FixedBase % window.WindowSize) != 0 || window.FixedBase > 0x10000ul - window.WindowSize)
                {
                    throw Fail($"window_read FIXED_IO port 0x{window.FixedBase:x} size 0x{window.WindowSize:x} invalid");
                }

                if (signatureLength < 1 || signatureLength > DynDataHw.FixedIoMaxSignature || signatureLength > window.WindowSize ||
                    signatureOffset > window.WindowSize - signatureLength || (signatureLength < 4 && (aux[1] >> (int)(signatureLength * 8)) != 0) ||
                    aux[1] == UInt32.MaxValue >> (int)(8 * (4 - signatureLength)))
                {
                    throw Fail($"window_read FIXED_IO signature 0x{aux[0]:x8} / 0x{aux[1]:x8} invalid");
                }
            }
            else if (window.BaseKind == DynDataHw.WindowBaseAcpiLpitRelative)
            {
                if (window.BarIndex >= DynDataHw.AcpiLpitMaxEntries || configFields || window.AuxCount != 0 ||
                    window.WindowSize > DynDataHw.AcpiLpitRelativeMaxSize || window.FixedBase == 0 || (window.FixedBase % 4) != 0 ||
                    window.FixedBase >= window.WindowSize)
                {
                    throw Fail($"window_read LPIT_RELATIVE entry {window.BarIndex} offset 0x{window.FixedBase:x} size 0x{window.WindowSize:x} invalid");
                }

                for (int i = 0; i < window.RangeCount; i++)
                {
                    var range = MemoryMarshal.Read<DynDataHw.HwWindowRange>(Body[(DynDataHw.WindowReadSize + i * DynDataHw.WindowRangeSize)..]);

                    if (!DynDataHw.InLpitCounterPage(window.FixedBase, range.Offset, range.Length))
                        throw Fail($"window_read LPIT_RELATIVE range 0x{range.Offset:x}+0x{range.Length:x} leaves the page of the counter at 0x{window.FixedBase:x}");

                    foreach (UInt32 register in DynDataHw.PwrmLockRegisters)
                    {
                        //
                        // The block is size-aligned within the 64 KiB-aligned PWRM, so a register sits at its offset
                        // modulo the size in whichever block holds it.
                        //
                        UInt32 offset = register & (window.WindowSize - 1);

                        if (offset + 4 > range.Offset && offset < (UInt64)range.Offset + range.Length)
                            throw Fail($"window_read LPIT_RELATIVE range 0x{range.Offset:x}+0x{range.Length:x} covers the PWRM lock register 0x{register:x}");
                    }
                }
            }
            else
            {
                throw Fail($"window_read base kind {window.BaseKind} has no check");
            }

            for (int i = 0; i < window.RangeCount; i++)
            {
                var rangeBytes = Body.Slice(DynDataHw.WindowReadSize + i * DynDataHw.WindowRangeSize, DynDataHw.WindowRangeSize);
                var range = MemoryMarshal.Read<DynDataHw.HwWindowRange>(rangeBytes);

                if ((portSpace && range.Width != 1) ||
                    (range.Width != 1 && range.Width != 2 && range.Width != 4 && range.Width != 8) ||
                    rangeBytes[9..].ContainsAnyExcept((byte)0) ||
                    range.Length < range.Width || (range.Length % range.Width) != 0 || (range.Offset % range.Width) != 0 ||
                    (UInt64)range.Offset + range.Length > window.WindowSize)
                {
                    throw Fail($"window_read range 0x{range.Offset:x}+0x{range.Length:x} width {range.Width} invalid");
                }
            }
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckPortIndirectOp(ReadOnlySpan<byte> Body)
        {
            var port = MemoryMarshal.Read<DynDataHw.HwPortIndirectRead>(Body);

            if (!DynDataHw.IoPortPairs.Contains((port.IndexPort, port.DataPort)))
                throw Fail($"port_indirect_read index port 0x{port.IndexPort:x} / data port 0x{port.DataPort:x} is not a pair of DynDataHw.IoPortPairs");
            if (port.RangeCount > DynDataHw.MaxRanges)
                throw Fail($"port_indirect_read has {port.RangeCount} ranges, cap {DynDataHw.MaxRanges}");
            if (DynDataHw.PortIndirectReadSize + port.RangeCount * DynDataHw.RangeSize > Body.Length)
                throw Fail("port_indirect_read ranges overrun the op");
            if (port.PinDevice > 31 || port.PinFunction > 7 || port.RangeCount < 1 || port.Reserved != 0)
                throw Fail("port_indirect_read has a bad pin, no range or non-zero reserved bytes");

            for (int i = 0; i < port.RangeCount; i++)
            {
                var range = MemoryMarshal.Read<DynDataHw.HwRange>(Body[(DynDataHw.PortIndirectReadSize + i * DynDataHw.RangeSize)..]);

                if (range.Length < 1 || (UInt32)range.Offset + range.Length > DynDataHw.PortIndexEnd)
                    throw Fail($"port_indirect_read range 0x{range.Offset:x}+0x{range.Length:x} invalid");
            }
        }

        /// <summary>
        /// Mirrors KphDynHwIndirectTargetAllowed.
        /// </summary>
        private static bool IndirectAllows(ReadOnlySpan<byte> Body, UInt32 Target)
        {
            var indirect = MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(Body);
            int valuesOffset = DynDataHw.PciIndirectReadSize + indirect.RangeCount * DynDataHw.TargetRangeSize;

            if ((Target & 3) != 0)
                return false;

            for (int r = 0; r < indirect.RangeCount; r++)
            {
                var range = MemoryMarshal.Read<DynDataHw.HwTargetRange>(Body[(DynDataHw.PciIndirectReadSize + r * DynDataHw.TargetRangeSize)..]);

                for (UInt64 k = 0; k < Math.Max((UInt64)range.StrideCount, 1); k++)
                {
                    if (Target >= range.Begin + range.Stride * k && Target < range.End + range.Stride * k)
                        return true;
                }
            }

            for (int v = 0; v < indirect.ValueCount; v++)
            {
                if (BinaryPrimitives.ReadUInt32LittleEndian(Body[(valuesOffset + v * sizeof(UInt32))..]) == Target)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// The [begin, end) configuration ranges of a pci_read body.
        /// </summary>
        private static List<(UInt32 Begin, UInt32 End)> PciReadRanges(ReadOnlySpan<byte> Body)
        {
            var ranges = new List<(UInt32 Begin, UInt32 End)>();

            for (int r = 0; r < Body[0]; r++)
            {
                var range = MemoryMarshal.Read<DynDataHw.HwRange>(Body[(DynDataHw.PciReadHeaderSize + r * DynDataHw.RangeSize)..]);
                ranges.Add((range.Offset, (UInt32)range.Offset + range.Length));
            }

            return ranges;
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        internal static void CheckWindowRefs(ReadOnlySpan<byte> Table, int Index)
        {
            UInt32 count = MemoryMarshal.Read<DynDataHw.HwTableHeader>(Table).Count;
            var windows = new List<int>();
            var indirects = new List<int>();
            var mailboxes = new List<DynDataHw.HwMailboxTransact>();
            var reads = new List<(UInt32 Begin, UInt32 End)>();
            int offset = DynDataHw.TableSize;

            for (UInt32 j = 0; j < count; j++)
            {
                var op = MemoryMarshal.Read<DynDataHw.HwOpHeader>(Table[offset..]);

                if (op.Kind == DynDataHw.KindWindowRead && Table[offset + DynDataHw.OpHeaderSize] == DynDataHw.WindowBaseSmnDerived)
                    windows.Add(offset + DynDataHw.OpHeaderSize);
                else if (op.Kind == DynDataHw.KindPciIndirectRead)
                    indirects.Add(offset + DynDataHw.OpHeaderSize);
                else if (op.Kind == DynDataHw.KindMailboxTransact && Table[offset + DynDataHw.OpHeaderSize] == DynDataHw.MailboxTransportSmnPair)
                    mailboxes.Add(MemoryMarshal.Read<DynDataHw.HwMailboxTransact>(Table[(offset + DynDataHw.OpHeaderSize)..]));
                else if (op.Kind == DynDataHw.KindPciRead)
                    reads.AddRange(PciReadRanges(Table[(offset + DynDataHw.OpHeaderSize)..]));

                offset += op.Length;
            }

            foreach (int windowOffset in windows)
            {
                var window = MemoryMarshal.Read<DynDataHw.HwWindowRead>(Table[windowOffset..]);
                int auxOffset = windowOffset + DynDataHw.WindowReadSize + window.RangeCount * DynDataHw.WindowRangeSize;
                UInt32 low = BinaryPrimitives.ReadUInt32LittleEndian(Table[auxOffset..]);
                UInt32 high = BinaryPrimitives.ReadUInt32LittleEndian(Table[(auxOffset + 4)..]);

                if (indirects.Count == 0)
                    throw Fail($"row {Index} SMN_DERIVED window has no pci_indirect_read pair in its table");

                var pair = MemoryMarshal.Read<DynDataHw.HwPciIndirectRead>(Table[indirects[0]..]);

                foreach (var (begin, end) in reads)
                {
                    if ((begin < pair.IndexOffset + 4u && pair.IndexOffset < end) || (begin < pair.DataOffset + 4u && pair.DataOffset < end))
                        throw Fail($"row {Index} pci_read covers the SMN pair 0x{pair.IndexOffset:x}/0x{pair.DataOffset:x} an aperture base is read through (the index is left at the high base register)");
                }

                foreach (int indirectOffset in indirects)
                {
                    if (IndirectAllows(Table[indirectOffset..], low) || IndirectAllows(Table[indirectOffset..], high))
                        throw Fail($"row {Index} pci_indirect_read allows an SMN address 0x{low:x}/0x{high:x} an aperture base is read from");
                }

                foreach (var mailbox in mailboxes)
                {
                    var smn = mailbox.Address;

                    foreach (UInt32 register in new[] { low, high })
                    {
                        if ((register >= smn.Msg && register < smn.Msg + 4ul) || (register >= smn.Rsp && register < smn.Rsp + 4ul) ||
                            (register >= smn.Arg0 && register < smn.Arg0 + 4ul * mailbox.ArgCount))
                            throw Fail($"row {Index} SMN mailbox 0x{smn.Msg:x} has a register at SMN address 0x{register:x} an aperture base is read from");
                    }
                }
            }
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckAcpiEvalOp(ReadOnlySpan<byte> Body)
        {
            var eval = MemoryMarshal.Read<DynDataHw.HwAcpiEval>(Body);
            string method = Encoding.ASCII.GetString(Body[..4]);

            if (eval.ArgCount > DynDataHw.AcpiMaxArgs)
                throw Fail($"acpi_eval has {eval.ArgCount} arguments, cap {DynDataHw.AcpiMaxArgs}");
            if (DynDataHw.AcpiEvalSize + eval.ArgCount * DynDataHw.AcpiArgSize > Body.Length)
                throw Fail("acpi_eval arguments overrun the op");
            if (!DynDataHw.AcpiMethodAllowed(method))
                throw Fail($"acpi_eval method \"{method}\" is not a NameSeg or not a read-only spec method");
            if (eval.ResultTypes == 0 || (eval.ResultTypes & ~DynDataHw.AcpiResultMask) != 0 ||
                eval.MaxOutput < DynDataHw.AcpiMinOutput || eval.MaxOutput > DynDataHw.AcpiMaxOutput)
            {
                throw Fail($"acpi_eval method \"{method}\" result types 0x{eval.ResultTypes:x2}, max output 0x{eval.MaxOutput:x} or reserved invalid");
            }

            for (int i = 0; i < eval.ArgCount; i++)
            {
                var arg = MemoryMarshal.Read<DynDataHw.HwAcpiArg>(Body[(DynDataHw.AcpiEvalSize + i * DynDataHw.AcpiArgSize)..]);

                if ((arg.Max & ~arg.Mask) != 0 || (arg.Fixed & arg.Mask) != 0)
                    throw Fail($"acpi_eval method \"{method}\" argument {i} mask/max/fixed invalid");
            }
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckAcpiRequireOp(ReadOnlySpan<byte> Body)
        {
            var require = MemoryMarshal.Read<DynDataHw.HwAcpiRequire>(Body);
            var ids = new HashSet<string>();

            if (require.IdCount == 0)
                throw Fail("acpi_require has no ID");
            if (Body.Length != DynDataHw.AlignOp(DynDataHw.OpHeaderSize + DynDataHw.AcpiRequireSize + require.IdCount * DynDataHw.AcpiIdSize) - DynDataHw.OpHeaderSize)
                throw Fail("acpi_require op length is not its IDs");

            for (int i = 0; i < require.IdCount; i++)
            {
                var bytes = Body.Slice(DynDataHw.AcpiRequireSize + i * DynDataHw.AcpiIdSize, DynDataHw.AcpiIdSize);
                string id = Encoding.ASCII.GetString(bytes).TrimEnd('\0');

                if (!DynDataHw.IsAcpiId(id) || bytes[id.Length..].ContainsAnyExcept((byte)0) || !ids.Add(id))
                    throw Fail($"acpi_require ID {i} is not a zero-padded ACPI ID or is a duplicate");
            }
        }

        /// <summary>
        /// Generator-only; the driver trusts the signed section for this.
        /// </summary>
        private static void CheckGate(DynDataHw.HwGate Gate)
        {
            if (!DynDataHw.DriverKinds.GateKinds.Contains(Gate.Kind))
                throw Fail($"gate kind {Gate.Kind} is not implemented by the driver");

            if (Gate.Kind == DynDataHw.GateNone)
            {
                if (Gate.Register != 0 || Gate.Shift != 0 || Gate.Width != 0 || Gate.Leaf != 0 || Gate.SubLeaf != 0 || Gate.Value != 0 || Gate.ValueMax != 0)
                    throw Fail("an absent gate has non-zero fields");
            }
            else if (Gate.Kind == DynDataHw.GateChassis)
            {
                if (Gate.Register != 0 || Gate.Shift != 0 || Gate.Width != 0 || Gate.Leaf != 0 || Gate.SubLeaf != 0 || (Gate.Value | Gate.ValueMax) == 0 || (Gate.Value & 1) != 0)
                    throw Fail("chassis gate has non-zero CPUID fields, an empty mask or type 0");
            }
            else if (Gate.Kind == DynDataHw.GateCpuid)
            {
                if (Gate.Register > DynDataHw.CpuidEdx || Gate.Width < 1 || Gate.Width > 32 ||
                    Gate.Shift > 32 - Gate.Width || Gate.Value > Gate.ValueMax ||
                    (Gate.Width < 32 && (Gate.ValueMax >> Gate.Width) != 0))
                {
                    throw Fail($"CPUID gate leaf 0x{Gate.Leaf:x} register {Gate.Register} bits [{Gate.Shift}, +{Gate.Width}) values [{Gate.Value}, {Gate.ValueMax}] invalid");
                }
            }
            else if (Gate.Kind == DynDataHw.GateSysregField)
            {
                if (!DynDataHw.SysregIsIdClass(Gate.Register) || !DynDataHw.DriverKinds.SysregRegisters.Contains(Gate.Register) ||
                    Gate.Width < 1 || Gate.Width > 32 || Gate.Shift > 64 - Gate.Width ||
                    Gate.Leaf != 0 || Gate.SubLeaf != 0 || Gate.Value > Gate.ValueMax ||
                    (Gate.Width < 32 && (Gate.ValueMax >> Gate.Width) != 0))
                {
                    throw Fail($"SYSREG_FIELD gate register {Gate.Register} bits [{Gate.Shift}, +{Gate.Width}) values [{Gate.Value}, {Gate.ValueMax}] invalid");
                }
            }
            else if (Gate.Kind == DynDataHw.GatePciConfig)
            {
                if ((Gate.Register & 3) != 0 || Gate.Register >= DynDataHw.PciConfigHeaderLength ||
                    Gate.Width < 1 || Gate.Width > 32 || Gate.Shift > 32 - Gate.Width ||
                    Gate.Leaf != 0 || Gate.SubLeaf != 0 || Gate.Value > Gate.ValueMax ||
                    (Gate.Width < 32 && (Gate.ValueMax >> Gate.Width) != 0))
                {
                    throw Fail($"PCI_CONFIG gate register 0x{Gate.Register:x2} bits [{Gate.Shift}, +{Gate.Width}) values [{Gate.Value}, {Gate.ValueMax}] invalid");
                }
            }
            else
            {
                throw Fail($"gate kind {Gate.Kind} has no check");
            }
        }

        private static InvalidDataException Fail(string Message)
        {
            return new InvalidDataException($"hardware section: {Message}");
        }

        /// <summary>
        /// Warns when a count reaches half of its validator cap.
        /// </summary>
        private static void WarnCap(string Name, UInt64 Value, UInt64 Cap)
        {
            if (Value * 10 >= Cap * 9)
                Program.PrintColorMessage($"Dynamic hardware -> warning: {Name} {Value} is at least 90% of the cap {Cap}", ConsoleColor.Yellow);
        }
    }
}
