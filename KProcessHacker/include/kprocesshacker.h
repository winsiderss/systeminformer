/*
 * Process Hacker Driver - 
 *   main header file
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

#ifndef KPROCESSHACKER_H
#define KPROCESSHACKER_H

/* KPH Configuration */

//#define KPH_REQUIRE_DEBUG_PRIVILEGE

/* Device */

#define KPH_DEVICE_TYPE (0x9999)
#define KPH_DEVICE_NAME (L"\\Device\\KProcessHacker2")
#define KPH_DEVICE_DOS_NAME (L"\\DosDevices\\KProcessHacker2")

/* Features */

#define KPHF_PSTERMINATEPROCESS 0x1
#define KPHF_PSPTERMINATETHREADBPYPOINTER 0x2

/* Control Codes */

#define KPH_CTL_CODE(x) CTL_CODE(KPH_DEVICE_TYPE, 0x800 + x, METHOD_BUFFERED, FILE_ANY_ACCESS)

/* General */
#define KPH_GETFEATURES KPH_CTL_CODE(0)

/* Processes */
#define KPH_OPENPROCESS KPH_CTL_CODE(50)
#define KPH_OPENPROCESSTOKEN KPH_CTL_CODE(51)
#define KPH_OPENPROCESSJOB KPH_CTL_CODE(52)
#define KPH_SUSPENDPROCESS KPH_CTL_CODE(53)
#define KPH_RESUMEPROCESS KPH_CTL_CODE(54)
#define KPH_TERMINATEPROCESS KPH_CTL_CODE(55)
#define KPH_READVIRTUALMEMORY KPH_CTL_CODE(56)
#define KPH_WRITEVIRTUALMEMORY KPH_CTL_CODE(57)
#define KPH_UNSAFEREADVIRTUALMEMORY KPH_CTL_CODE(58)
#define KPH_GETPROCESSPROTECTED KPH_CTL_CODE(59)
#define KPH_SETPROCESSPROTECTED KPH_CTL_CODE(60)
#define KPH_SETEXECUTEOPTIONS KPH_CTL_CODE(61)
#define KPH_SETPROCESSTOKEN KPH_CTL_CODE(62)
#define KPH_QUERYINFORMATIONPROCESS KPH_CTL_CODE(63)
#define KPH_QUERYINFORMATIONTHREAD KPH_CTL_CODE(64)
#define KPH_SETINFORMATIONPROCESS KPH_CTL_CODE(65)
#define KPH_SETINFORMATIONTHREAD KPH_CTL_CODE(66)

/* Threads */
#define KPH_OPENTHREAD KPH_CTL_CODE(100)
#define KPH_OPENTHREADPROCESS KPH_CTL_CODE(101)
#define KPH_TERMINATETHREAD KPH_CTL_CODE(102)
#define KPH_DANGEROUSTERMINATETHREAD KPH_CTL_CODE(103)
#define KPH_GETCONTEXTTHREAD KPH_CTL_CODE(104)
#define KPH_SETCONTEXTTHREAD KPH_CTL_CODE(105)
#define KPH_CAPTURESTACKBACKTRACETHREAD KPH_CTL_CODE(106)
#define KPH_GETTHREADWIN32THREAD KPH_CTL_CODE(107)
#define KPH_ASSIGNIMPERSONATIONTOKEN KPH_CTL_CODE(108)

/* Handles */
#define KPH_QUERYPROCESSHANDLES KPH_CTL_CODE(150)
#define KPH_GETHANDLEOBJECTNAME KPH_CTL_CODE(151)
#define KPH_ZWQUERYOBJECT KPH_CTL_CODE(152)
#define KPH_DUPLICATEOBJECT KPH_CTL_CODE(153)
#define KPH_SETHANDLEATTRIBUTES KPH_CTL_CODE(154)
#define KPH_SETHANDLEGRANTEDACCESS KPH_CTL_CODE(155)
#define KPH_GETPROCESSID KPH_CTL_CODE(156)
#define KPH_GETTHREADID KPH_CTL_CODE(157)

/* Objects */
#define KPH_OPENNAMEDOBJECT KPH_CTL_CODE(200)
#define KPH_OPENDIRECTORYOBJECT KPH_CTL_CODE(201)
#define KPH_OPENDRIVER KPH_CTL_CODE(202)
#define KPH_QUERYINFORMATIONDRIVER KPH_CTL_CODE(203)
#define KPH_OPENTYPE KPH_CTL_CODE(204)

/* Standard Driver Routines */

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID DriverUnload(PDRIVER_OBJECT DriverObject);
NTSTATUS KphDispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS KphDispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS KphDispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS KphDispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS KphUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp);

#endif