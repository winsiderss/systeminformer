#include <setup.h>
#include <Windows.h>
#include "appsup.h"
#include "netio.h"

#define TIME_DAYS_IN_YEAR       365
#define TIME_HOURS_IN_DAY       24
#define TIME_MINUTES_IN_HOUR    60
#define TIME_SECONDS_IN_MINUTE  60

HWND _hwndProgress = NULL;
static BOOLEAN _TotalChanged = FALSE;
static BOOLEAN _CompletedChanged = FALSE;
static STOPWATCH InstallTimer = { 0 };
static volatile LONG _Completed = 0;                    // progress completed
static volatile LONG _Total = 0;                        // total progress
static volatile LONG _PrevRate = 0;                     // previous progress rate (used for computing time remaining)
static volatile LONG _PrevTickCount = 0;                // the tick count when we last updated the progress time
static volatile LONG _PrevCompleted = 0;                // the amount we had completed when we last updated the progress time
static volatile LONG _LastUpdatedTimeRemaining = 0;     // tick count when we last update the "Time remaining" field, we only update it every 5 seconds
static volatile LONG _LastUpdatedTickCount = 0;         // tick count when SetProgress was last called, used to calculate the rate
static volatile LONG _NumTimesSetProgressCalled = 0;    // how many times has the user called SetProgress?

VOID StartProgress(VOID)
{
    StopwatchInitialize(&InstallTimer);
    StopwatchStart(&InstallTimer);

    _PrevRate = 0;
    _PrevCompleted = 0;
    _Completed = 0;              // progress completed
    //_dwTotal = 0;
    _PrevRate = 0;                // previous progress rate (used for computing time remaining)
    _PrevTickCount = 0;          // the tick count when we last updated the progress time
    _PrevCompleted = 0;           // the amount we had completed when we last updated the progress time
    _LastUpdatedTimeRemaining = 0;// tick count when we last update the "Time remaining" field, we only update it every 5 seconds
    _LastUpdatedTickCount = 0;   // tick count when SetProgress was last called, used to calculate the rate
    _NumTimesSetProgressCalled = 0;// how many times has the user called SetProgress?

    SetProgress(0, 0);
}

VOID _SetProgressTime(VOID)
{
    volatile LONG dwTotal = 0;
    volatile LONG dwCompleted = 0;
    volatile LONG dwCurrentRate = 0;
    volatile LONG dwTickDelta = 0;
    volatile LONG dwLeft = 0;
    volatile LONG dwCurrentTickCount = 0;
    volatile LONG dwAverageRate = 0;

    LONG dwSecondsLeft = 0;
    LONG dwTickCount = 0;
    LONG time_taken = 0;
    LONG download_speed = 0;
    PPH_STRING timeRemainingString = NULL;

    InterlockedIncrement(&_NumTimesSetProgressCalled);
    InterlockedExchange(&dwTotal, _Total);
    InterlockedExchange(&dwCompleted, _Completed);
    InterlockedExchange(&dwCurrentTickCount, _LastUpdatedTickCount);

    dwLeft = dwTotal - dwCompleted;
    dwTickDelta = dwCurrentTickCount - _PrevTickCount;
    dwTickCount = StopwatchGetMilliseconds(&InstallTimer);

    if (_TotalChanged)
    {
        _TotalChanged = FALSE;
        SendDlgItemMessage(_hwndProgress, IDC_PROGRESS1, PBM_SETRANGE32, 0, (LPARAM)dwTotal);
    }

    if (_CompletedChanged)
    {
        _CompletedChanged = FALSE;
        SendDlgItemMessage(_hwndProgress, IDC_PROGRESS1, PBM_SETPOS, (WPARAM)dwCompleted, 0);
    }

    if (dwCompleted <= _PrevCompleted)
    {
        // we are going backwards...
        dwCurrentRate = (_PrevRate ? _PrevRate : 2);
    }
    else
    {
        // calculate the current rate in points per tenth of a second
        dwTickDelta /= 100;

        if (dwTickDelta == 0)
            dwTickDelta = __max(dwTickDelta, 1); // Protect from divide by zero

        dwCurrentRate = (dwCompleted - _PrevCompleted) / dwTickDelta;
    }

    // we divide the TickDelta by 100 to give tenths of seconds, so if we have recieved an update faster than that, just skip it
    //if (dwTickDelta < 100)
    //    return;

    // time remaining in seconds (we take a REAL average to smooth out random fluxuations)
    dwAverageRate = (ULONG)((dwCurrentRate + (__int64)_PrevRate * _NumTimesSetProgressCalled) / (_NumTimesSetProgressCalled + 1));
    dwAverageRate = __max(dwAverageRate, 1); // Protect from divide by zero
    dwSecondsLeft = (dwLeft / dwAverageRate) / 10;

    // Skip the first five calls (we need an average)...
    //if (_NumTimesSetProgressCalled > 5)// ((dwSecondsLeft >= MIN_MINTIME4FEEDBACK) && (_iNumTimesSetProgressCalled >= 5))
    {
        time_taken = dwCurrentTickCount - dwTickDelta;
        time_taken = __max(time_taken, 1); // Protect from divide by zero
        download_speed = (dwCompleted / time_taken) * 1024;

        //if (download_speed)
        {
            LONGLONG nPercent = 0;
            PPH_STRING statusRemaningBytesString = PhFormatSize(dwCompleted, -1);
            PPH_STRING statusLengthString = PhFormatSize(dwTotal, -1);
            PPH_STRING statusSpeedString = PhFormatSize(download_speed, -1);

            if (dwTotal)
            {
                // Will scaling it up cause a wrap?
                if ((100 * 100) <= dwTotal)
                {
                    // Yes, so scale down.
                    nPercent = (dwCompleted / (dwTotal / 100));
                }
                else
                {
                    // No, so scale up.
                    nPercent = ((100 * dwCompleted) / dwTotal);
                }
            }

            PPH_STRING statusText1 = PhFormatString(L"Transfer rate: %s/s ", statusSpeedString->Buffer);
            PPH_STRING statusText2 = PhFormatString(L"Downloaded: %s of %s (%u%% Completed)",
                statusRemaningBytesString->Buffer,
                statusLengthString->Buffer,
                nPercent
                );

            SetDlgItemText(_hwndProgress, IDC_REMAINTIME, statusText1->Buffer);
            SetDlgItemText(_hwndProgress, IDC_REMAINTIME3, statusText2->Buffer);

            PhDereferenceObject(statusText2);
            PhDereferenceObject(statusText1);
            PhDereferenceObject(statusSpeedString);
            PhDereferenceObject(statusLengthString);
            PhDereferenceObject(statusRemaningBytesString);
        }

        // Is it more than an hour?
        if (dwSecondsLeft > (TIME_SECONDS_IN_MINUTE * TIME_MINUTES_IN_HOUR))
        {
            LONGLONG dwMinutes = dwSecondsLeft / TIME_SECONDS_IN_MINUTE;
            LONGLONG dwHours = dwMinutes / TIME_MINUTES_IN_HOUR;
            LONGLONG dwDays = dwHours / TIME_HOURS_IN_DAY;

            if (dwDays)
            {
                dwHours %= TIME_HOURS_IN_DAY;

                // It's more than a day, so display days and hours.
                if (dwDays == 1)
                {
                    if (dwHours == 1)
                        timeRemainingString = PhFormatString(L"Remaining time: %u day and %u hour", dwHours, dwMinutes);
                    else
                        timeRemainingString = PhFormatString(L"Remaining time: %u day and %u hours", dwHours, dwMinutes);
                }
                else
                {
                    if (dwHours == 1)
                        timeRemainingString = PhFormatString(L"Remaining time: %u days and %u hour", dwHours, dwMinutes);
                    else
                        timeRemainingString = PhFormatString(L"Remaining time: %u days and %u hours", dwHours, dwMinutes);
                }
            }
            else
            {
                // It's less than a day, so display hours and minutes.
                dwMinutes %= TIME_MINUTES_IN_HOUR;

                if (dwHours == 1)
                {
                    if (dwMinutes == 1)
                        timeRemainingString = PhFormatString(L"Remaining time: %u hour and %u minute", dwHours, dwMinutes);
                    else
                        timeRemainingString = PhFormatString(L"Remaining time: %u hour and %u minutes", dwHours, dwMinutes);
                }
                else
                {
                    if (dwMinutes == 1)
                        timeRemainingString = PhFormatString(L"Remaining time: %u hours and %u minute", dwHours, dwMinutes);
                    else
                    {
                        ULONG stime =  dwSecondsLeft %= TIME_MINUTES_IN_HOUR;
                        timeRemainingString = PhFormatString(L"Remaining time: %u hours, %u minutes and %u seconds", dwHours, dwMinutes, stime);
                    }
                }
            }
        }
        else
        {
            if (dwSecondsLeft > TIME_SECONDS_IN_MINUTE)
            {
                ULONG time = dwSecondsLeft / TIME_SECONDS_IN_MINUTE;
                ULONG stime =  dwSecondsLeft %= TIME_MINUTES_IN_HOUR;

                timeRemainingString = PhFormatString(L"Remaining time: %u minutes and %u seconds", time, stime);
            }
            else
            {
                if (dwSecondsLeft > 0)
                {
                    // Round up to 5 seconds so it doesn't look so random
                    //ULONG stime = ((dwSecondsLeft + 4) / 5) * 5;
                    timeRemainingString = PhFormatString(L"Remaining time: %u seconds", dwSecondsLeft);
                }
                else
                {
                    timeRemainingString = PhFormatString(L"Remaining time: 0 seconds");
                }
            }
        }

        // update the Time remaining field
        SetDlgItemText(_hwndProgress, IDC_REMAINTIME2, timeRemainingString->Buffer);
        PhDereferenceObject(timeRemainingString);
    }

    // we are updating now, so set all the stuff for next time
    InterlockedExchange(&_LastUpdatedTimeRemaining, dwTickCount);
    InterlockedExchange(&_PrevRate, dwAverageRate);
    InterlockedExchange(&_PrevTickCount, dwCurrentTickCount);
    InterlockedExchange(&_PrevCompleted, dwCompleted);
}

VOID SetProgress(
    _In_ LONG Completed,
    _In_ LONG Total
    )
{
    if (InterlockedCompareExchange(&_Completed, _Completed, Completed) != Completed)
    {
        InterlockedExchange(&_Completed, Completed);
        _CompletedChanged = TRUE;
    }

    if (InterlockedCompareExchange(&_Total, _Total, Total) != Total)
    {
        InterlockedExchange(&_Total, Total);
        _TotalChanged = TRUE;
    }

    InterlockedExchange(&_LastUpdatedTickCount, StopwatchGetMilliseconds(&InstallTimer));
}