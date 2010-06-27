/*
 * Process Hacker - 
 *   error codes
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

#include <phbase.h>

NTSTATUS PhDosErrorToNtStatus(
    __in ULONG DosError
    )
{
    switch (DosError)
    {
    case ERROR_INVALID_FUNCTION: return STATUS_ILLEGAL_FUNCTION;
    case ERROR_FILE_NOT_FOUND: return STATUS_NO_SUCH_FILE;
    case ERROR_ACCESS_DENIED: return STATUS_ACCESS_DENIED;
    case ERROR_INVALID_HANDLE: return STATUS_INVALID_HANDLE;
    case ERROR_HANDLE_EOF: return STATUS_END_OF_FILE;
    case ERROR_NOT_SUPPORTED: return STATUS_NOT_SUPPORTED;
    case ERROR_INVALID_PARAMETER: return STATUS_INVALID_PARAMETER;
    case ERROR_NOT_LOCKED: return STATUS_NOT_LOCKED;
    case ERROR_MORE_DATA: return STATUS_MORE_ENTRIES;
    case ERROR_NOACCESS: return STATUS_ACCESS_VIOLATION;
    case ERROR_STACK_OVERFLOW: return STATUS_STACK_OVERFLOW;
    case ERROR_INTERNAL_ERROR: return STATUS_INTERNAL_ERROR;
    default: return NTSTATUS_FROM_WIN32(DosError);
    }
}
