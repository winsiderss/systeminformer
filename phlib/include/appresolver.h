/*
 * Process Hacker -
 *   Appmodel support functions
 *
 * Copyright (C) 2017-2018 dmex
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

#ifndef _PH_APPRESOLVER_H
#define _PH_APPRESOLVER_H

#ifdef __cplusplus
extern "C" {
#endif

BOOLEAN PhAppResolverGetAppIdForProcess(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *ApplicationUserModelId
    );

PPH_LIST PhGetPackageAssetsFromResourceFile(
    _In_ PWSTR FilePath
    );

#ifdef __cplusplus
}
#endif

#endif
