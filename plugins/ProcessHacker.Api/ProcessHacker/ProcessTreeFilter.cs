/*
 * Process Hacker - 
 *   Process Tree Filter API
 * 
 * Copyright (C) 2011 wj32
 * Copyright (C) 2011 dmex
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

using System;

namespace ProcessHacker.Api
{
    public unsafe class ProcessTreeFilter : IDisposable
    {
        public static void Apply()
        {
            NativeApi.PhApplyProcessTreeFilters();
        }

        private void* _filter;

        public ProcessTreeFilter(PhProcessTreeFilter function)
        {
            _filter = NativeApi.PhAddProcessTreeFilter(function, IntPtr.Zero);
        }

        public void Dispose()
        {
            if (_filter != null)
            {
                NativeApi.PhRemoveProcessTreeFilter(_filter);
                _filter = null;
            }
        }
    }
}
