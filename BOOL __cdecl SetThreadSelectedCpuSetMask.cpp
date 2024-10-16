BOOL __cdecl SetThreadSelectedCpuSetMasks(HANDLE Thread, PGROUP_AFFINITY CpuSetMasks, USHORT CpuSetMaskCount)
{
  unsigned __int64 v4; // rbx
  __int64 v5; // r10
  void *v6; // rsp
  NTSTATUS v8; // eax
  _QWORD ThreadInformation[4]; // [rsp+20h] [rbp+0h] BYREF

  v4 = 8 * (unsigned int)MEMORY[0x7FFE03C4];
  v5 = v4 + 15;
  if ( v4 + 15 <= v4 )
    v5 = 0xFFFFFFFFFFFFFF0LL;
  v6 = alloca(v5 & 0xFFFFFFFFFFFFFFF0uLL);
  if ( !(unsigned int)BasepCpuSetGroupAffinitiesToMasks(
                        (__int64)CpuSetMasks,
                        CpuSetMaskCount,
                        ThreadInformation,
                        MEMORY[0x7FFE03C4]) )
    return 0;
  v8 = NtSetInformationThread(Thread, ThreadSelectedCpuSets, ThreadInformation, v4);
  if ( v8 < 0 )
  {
    BaseSetLastNTError((unsigned int)v8);
    return 0;
  }
  return 1;
}