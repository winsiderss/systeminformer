// Windows/ProcessUtils.h

#ifndef ZIP7_INC_WINDOWS_PROCESS_UTILS_H
#define ZIP7_INC_WINDOWS_PROCESS_UTILS_H

#include "../Common/MyWindows.h"

#ifndef Z7_OLD_WIN_SDK

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <psapi.h>
#else
#include <Psapi.h>
#endif

#else // Z7_OLD_WIN_SDK

typedef struct _MODULEINFO {
    LPVOID lpBaseOfDll;
    DWORD SizeOfImage;
    LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb;
    DWORD PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;
typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;

#endif // Z7_OLD_WIN_SDK

#include "../Common/MyString.h"

#include "Defs.h"
#include "Handle.h"

namespace NWindows {

class CProcess: public CHandle
{
public:
  bool Open(DWORD desiredAccess, bool inheritHandle, DWORD processId)
  {
    _handle = ::OpenProcess(desiredAccess, inheritHandle, processId);
    return (_handle != NULL);
  }

  #ifndef UNDER_CE

  bool GetExitCodeProcess(LPDWORD lpExitCode) { return BOOLToBool(::GetExitCodeProcess(_handle, lpExitCode)); }
  bool Terminate(UINT exitCode) { return BOOLToBool(::TerminateProcess(_handle, exitCode)); }
  #if (WINVER >= 0x0500)
  DWORD GetGuiResources (DWORD uiFlags) { return ::GetGuiResources(_handle, uiFlags); }
  #endif
  bool SetPriorityClass(DWORD dwPriorityClass) { return BOOLToBool(::SetPriorityClass(_handle, dwPriorityClass)); }
  DWORD GetPriorityClass() { return ::GetPriorityClass(_handle); }
  // bool GetIoCounters(PIO_COUNTERS lpIoCounters ) { return BOOLToBool(::GetProcessIoCounters(_handle, lpIoCounters )); }

  bool GetTimes(LPFILETIME creationTime, LPFILETIME exitTime, LPFILETIME kernelTime, LPFILETIME userTime)
    { return BOOLToBool(::GetProcessTimes(_handle, creationTime, exitTime, kernelTime, userTime)); }

  DWORD WaitForInputIdle(DWORD milliseconds) { return ::WaitForInputIdle(_handle, milliseconds);  }

  // Debug

  bool ReadMemory(LPCVOID baseAddress, LPVOID buffer, SIZE_T size, SIZE_T* numberOfBytesRead)
    { return BOOLToBool(::ReadProcessMemory(_handle, baseAddress, buffer, size, numberOfBytesRead));  }

  bool WriteMemory(LPVOID baseAddress, LPCVOID buffer, SIZE_T size, SIZE_T* numberOfBytesWritten)
    { return BOOLToBool(::WriteProcessMemory(_handle, baseAddress,
      #ifdef Z7_OLD_WIN_SDK
        (LPVOID)
      #endif
      buffer,
      size, numberOfBytesWritten)); }

  bool FlushInstructionCache(LPCVOID baseAddress = NULL, SIZE_T size = 0)
    { return BOOLToBool(::FlushInstructionCache(_handle, baseAddress, size)); }

  LPVOID VirtualAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect)
    { return VirtualAllocEx(_handle, address, size, allocationType, protect);  }

  bool VirtualFree(LPVOID address, SIZE_T size, DWORD freeType)
    { return BOOLToBool(::VirtualFreeEx(_handle, address, size, freeType)); }

  // Process Status API (PSAPI)

  /*
  bool EmptyWorkingSet()
    { return BOOLToBool(::EmptyWorkingSet(_handle)); }
  bool EnumModules(HMODULE *hModules, DWORD arraySizeInBytes, LPDWORD receivedBytes)
    { return BOOLToBool(::EnumProcessModules(_handle, hModules, arraySizeInBytes, receivedBytes)); }
  DWORD MyGetModuleBaseName(HMODULE hModule, LPTSTR baseName, DWORD size)
    { return ::GetModuleBaseName(_handle, hModule, baseName, size); }
  bool MyGetModuleBaseName(HMODULE hModule, CSysString &name)
  {
    const unsigned len = MAX_PATH + 100;
    const DWORD resultLen = MyGetModuleBaseName(hModule, name.GetBuf(len), len);
    name.ReleaseBuf_CalcLen(len);
    return (resultLen != 0);
  }

  DWORD MyGetModuleFileNameEx(HMODULE hModule, LPTSTR baseName, DWORD size)
    { return ::GetModuleFileNameEx(_handle, hModule, baseName, size); }
  bool MyGetModuleFileNameEx(HMODULE hModule, CSysString &name)
  {
    const unsigned len = MAX_PATH + 100;
    const DWORD resultLen = MyGetModuleFileNameEx(hModule, name.GetBuf(len), len);
    name.ReleaseBuf_CalcLen(len);
    return (resultLen != 0);
  }

  bool GetModuleInformation(HMODULE hModule, LPMODULEINFO moduleInfo)
    { return BOOLToBool(::GetModuleInformation(_handle, hModule, moduleInfo, sizeof(MODULEINFO))); }
  bool GetMemoryInfo(PPROCESS_MEMORY_COUNTERS memCounters)
    { return BOOLToBool(::GetProcessMemoryInfo(_handle, memCounters, sizeof(PROCESS_MEMORY_COUNTERS))); }
  */

  #endif

  WRes Create(LPCWSTR imageName, const UString &params, LPCWSTR curDir);

  DWORD Wait() { return ::WaitForSingleObject(_handle, INFINITE); }
};

WRes MyCreateProcess(LPCWSTR imageName, const UString &params);

}

#endif
