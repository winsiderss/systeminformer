/*
 * Process Hacker Driver - 
 *   debug definitions
 * 
 * Copyright (C) 2009 wj32
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

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef DBG
#define dprintf(fs, ...) DbgPrint("KProcessHacker: " fs, __VA_ARGS__)
#else
#define dprintf
#endif

#define dfprintf(fs, ...) DbgPrint("KProcessHacker: " fs, __VA_ARGS__)
#define dwprintf DbgPrint

#endif
