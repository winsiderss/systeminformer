using System;
using System.Runtime.InteropServices;

namespace ProcessHacker.Api
{
    public enum SecurityInformation : uint
    {
        Owner = 0x1,
        Group = 0x2,
        Dacl = 0x4,
        Sacl = 0x8,
        Label = 0x10,

        ProtectedDacl = 0x80000000,
        ProtectedSacl = 0x40000000,
        UnprotectedDacl = 0x20000000,
        UnprotectedSacl = 0x10000000
    }

    public enum TokenElevationType
    {
        Default = 1,
        Full,
        Limited
    }
}
