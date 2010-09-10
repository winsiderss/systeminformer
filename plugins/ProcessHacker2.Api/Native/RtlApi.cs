using System;
using System.Runtime.InteropServices;

namespace ProcessHacker2.Api
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ListEntry
    {
        public ListEntry* Flink;
        public ListEntry* Blink;
    }
}
