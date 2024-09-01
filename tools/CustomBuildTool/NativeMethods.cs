/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

namespace CustomBuildTool
{
    public static class NativeMethods
    {
        public static DateTime FileTimeToDateTime(this System.Runtime.InteropServices.ComTypes.FILETIME FileTime)
        {
            long fileTime = ((long)FileTime.dwHighDateTime << 32) + FileTime.dwLowDateTime;

            return DateTime.FromFileTime(fileTime);
        }

        public static long FileTimeFromFileTime(this System.Runtime.InteropServices.ComTypes.FILETIME FileTime)
        {
            long fileTime = ((long)FileTime.dwHighDateTime << 32) + FileTime.dwLowDateTime;

            return fileTime;
        }
    }
}
