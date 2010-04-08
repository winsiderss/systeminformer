/*
 * Process Hacker Driver - 
 *   I/O manager
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

#ifndef _IO_H
#define _IO_H

extern POBJECT_TYPE *IoAdapterObjectType;
extern POBJECT_TYPE *IoControllerObjectType;
extern POBJECT_TYPE *IoDeviceHandlerObjectType; /* not used anymore */
extern POBJECT_TYPE *IoDeviceObjectType;
extern POBJECT_TYPE *IoDriverObjectType;

#endif