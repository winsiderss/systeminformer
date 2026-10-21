// Windows/NationalTime.h

#ifndef ZIP7_INC_WINDOWS_NATIONAL_TIME_H
#define ZIP7_INC_WINDOWS_NATIONAL_TIME_H

#include "../Common/MyString.h"

namespace NWindows {
namespace NNational {
namespace NTime {

bool MyGetTimeFormat(LCID locale, DWORD flags, CONST SYSTEMTIME *time,
    LPCTSTR format, CSysString &resultString);

bool MyGetDateFormat(LCID locale, DWORD flags, CONST SYSTEMTIME *time,
    LPCTSTR format, CSysString &resultString);

}}}

#endif
