// Windows/Console.h

#ifndef ZIP7_INC_WINDOWS_CONSOLE_H
#define ZIP7_INC_WINDOWS_CONSOLE_H

#include "Defs.h"

namespace NWindows {
namespace NConsole {

class CBase
{
protected:
  HANDLE m_Object;
public:
  void Attach(HANDLE handle) { m_Object = handle; }
  bool GetMode(DWORD &mode)
    { return BOOLToBool(::GetConsoleMode(m_Object, &mode)); }
  bool SetMode(DWORD mode)
    { return BOOLToBool(::SetConsoleMode(m_Object, mode)); }
};

class CIn: public CBase
{
public:
  bool PeekEvents(PINPUT_RECORD events, DWORD numEvents, DWORD &numEventsRead)
    {  return BOOLToBool(::PeekConsoleInput(m_Object, events, numEvents, &numEventsRead)); }
  bool PeekEvent(INPUT_RECORD &event, DWORD &numEventsRead)
    {  return PeekEvents(&event, 1, numEventsRead); }
  bool ReadEvents(PINPUT_RECORD events, DWORD numEvents, DWORD &numEventsRead)
    {  return BOOLToBool(::ReadConsoleInput(m_Object, events, numEvents, &numEventsRead)); }
  bool ReadEvent(INPUT_RECORD &event, DWORD &numEventsRead)
    {  return ReadEvents(&event, 1, numEventsRead); }
  bool GetNumberOfEvents(DWORD &numEvents)
    {  return BOOLToBool(::GetNumberOfConsoleInputEvents(m_Object, &numEvents)); }

  bool WriteEvents(const INPUT_RECORD *events, DWORD numEvents, DWORD &numEventsWritten)
    {  return BOOLToBool(::WriteConsoleInput(m_Object, events, numEvents, &numEventsWritten)); }
  bool WriteEvent(const INPUT_RECORD &event, DWORD &numEventsWritten)
    {  return WriteEvents(&event, 1, numEventsWritten); }
  
  bool Read(LPVOID buffer, DWORD numChars, DWORD &numCharsRead)
    {  return BOOLToBool(::ReadConsole(m_Object, buffer, numChars, &numCharsRead, NULL)); }

  bool Flush()
    {  return BOOLToBool(::FlushConsoleInputBuffer(m_Object)); }

};

}}

#endif
