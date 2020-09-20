/*
 * Process Hacker Extended Notifications -
 *   file logging
 *
 * Copyright (C) 2010 wj32
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phdk.h>
#include <settings.h>
#include "extnoti.h"

VOID NTAPI LoggedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

PPH_FILE_STREAM LogFileStream = NULL;
PH_CALLBACK_REGISTRATION LoggedCallbackRegistration;

VOID FileLogInitialization(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING fileName;

    fileName = PhaGetStringSetting(SETTING_NAME_LOG_FILENAME);

    if (!PhIsNullOrEmptyString(fileName))
    {
        status = PhCreateFileStream(
            &LogFileStream,
            fileName->Buffer,
            FILE_GENERIC_WRITE,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            PH_FILE_STREAM_APPEND | PH_FILE_STREAM_UNBUFFERED
            );

        if (NT_SUCCESS(status))
        {
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackLoggedEvent),
                LoggedCallback,
                NULL,
                &LoggedCallbackRegistration
                );
        }
    }
}

VOID NTAPI LoggedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_LOG_ENTRY logEntry = Parameter;

    if (logEntry)
    {
        PhWriteStringFormatAsUtf8FileStream(
            LogFileStream,
            L"%s: %s\r\n",
            PhaFormatDateTime(NULL)->Buffer,
            PH_AUTO_T(PH_STRING, PhFormatLogEntry(logEntry))->Buffer
            );
    }
}
