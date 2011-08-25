/*
 * Process Hacker -
 *   Security enumerations
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

namespace ProcessHacker.Api
{
    public enum SecurityInformation : uint
    {
        Owner = 0x1,
        Group = 0x2,
        Dacl = 0x4,
        Sacl = 0x8,
        Label = 0x10,

        ProtectedDacl = 0x80000000,
        ProtectedSacl = 0x40000000,
        UnprotectedDacl = 0x20000000,
        UnprotectedSacl = 0x10000000
    }

    public enum TokenElevationType
    {
        Default = 1,
        Full,
        Limited
    }
}
