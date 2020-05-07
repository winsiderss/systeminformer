/*
 * Process Hacker -
 *   Direct3D kernel support functions
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

#ifndef _NTD3DKMT_H
#define _NTD3DKMT_H

#define USE_D3DKMT_SINGLE_HEADER

#ifndef USE_D3DKMT_SINGLE_HEADER
#include <dxmini.h>
#include <d3dkmddi.h>

typedef D3DKMT_HANDLE *PD3DKMT_HANDLE;
#else
typedef UINT32 D3DKMT_HANDLE;
typedef D3DKMT_HANDLE *PD3DKMT_HANDLE;
#endif

typedef enum _KMTQUERYADAPTERINFOTYPE
{
    KMTQAITYPE_UMDRIVERPRIVATE,
    KMTQAITYPE_UMDRIVERNAME, // D3DKMT_UMDFILENAMEINFO
    KMTQAITYPE_UMOPENGLINFO, // D3DKMT_OPENGLINFO
    KMTQAITYPE_GETSEGMENTSIZE, // D3DKMT_SEGMENTSIZEINFO
    KMTQAITYPE_ADAPTERGUID, // GUID
    KMTQAITYPE_FLIPQUEUEINFO, // D3DKMT_FLIPQUEUEINFO
    KMTQAITYPE_ADAPTERADDRESS, // D3DKMT_ADAPTERADDRESS
    KMTQAITYPE_SETWORKINGSETINFO, // D3DKMT_WORKINGSETINFO
    KMTQAITYPE_ADAPTERREGISTRYINFO, // D3DKMT_ADAPTERREGISTRYINFO
    KMTQAITYPE_CURRENTDISPLAYMODE, // D3DKMT_CURRENTDISPLAYMODE
    KMTQAITYPE_MODELIST, // D3DKMT_DISPLAYMODE[] // 10
    KMTQAITYPE_CHECKDRIVERUPDATESTATUS,
    KMTQAITYPE_VIRTUALADDRESSINFO, // D3DKMT_VIRTUALADDRESSINFO
    KMTQAITYPE_DRIVERVERSION, // D3DKMT_DRIVERVERSION
    KMTQAITYPE_UNKNOWN,
    KMTQAITYPE_ADAPTERTYPE, // D3DKMT_ADAPTERTYPE // since WIN8
    KMTQAITYPE_OUTPUTDUPLCONTEXTSCOUNT, // D3DKMT_OUTPUTDUPLCONTEXTSCOUNT
    KMTQAITYPE_WDDM_1_2_CAPS, // D3DKMT_WDDM_1_2_CAPS
    KMTQAITYPE_UMD_DRIVER_VERSION, // D3DKMT_UMD_DRIVER_VERSION
    KMTQAITYPE_DIRECTFLIP_SUPPORT, // D3DKMT_DIRECTFLIP_SUPPORT
    KMTQAITYPE_MULTIPLANEOVERLAY_SUPPORT, // D3DKMT_MULTIPLANEOVERLAY_SUPPORT // since WDDM1_3 // 20
    KMTQAITYPE_DLIST_DRIVER_NAME, // D3DKMT_DLIST_DRIVER_NAME
    KMTQAITYPE_WDDM_1_3_CAPS, // D3DKMT_WDDM_1_3_CAPS
    KMTQAITYPE_MULTIPLANEOVERLAY_HUD_SUPPORT, // D3DKMT_MULTIPLANEOVERLAY_HUD_SUPPORT
    KMTQAITYPE_WDDM_2_0_CAPS, // D3DKMT_WDDM_2_0_CAPS // since WDDM2_0
    KMTQAITYPE_NODEMETADATA, // D3DKMT_NODEMETADATA
    KMTQAITYPE_CPDRIVERNAME, // D3DKMT_CPDRIVERNAME
    KMTQAITYPE_XBOX, // D3DKMT_XBOX
    KMTQAITYPE_INDEPENDENTFLIP_SUPPORT, // D3DKMT_INDEPENDENTFLIP_SUPPORT
    KMTQAITYPE_MIRACASTCOMPANIONDRIVERNAME, // D3DKMT_MIRACASTCOMPANIONDRIVERNAME
    KMTQAITYPE_PHYSICALADAPTERCOUNT, // D3DKMT_PHYSICAL_ADAPTER_COUNT // 30
    KMTQAITYPE_PHYSICALADAPTERDEVICEIDS, // D3DKMT_QUERY_DEVICE_IDS
    KMTQAITYPE_DRIVERCAPS_EXT, // D3DKMT_DRIVERCAPS_EXT
    KMTQAITYPE_QUERY_MIRACAST_DRIVER_TYPE, // D3DKMT_QUERY_MIRACAST_DRIVER_TYPE
    KMTQAITYPE_QUERY_GPUMMU_CAPS, // D3DKMT_QUERY_GPUMMU_CAPS
    KMTQAITYPE_QUERY_MULTIPLANEOVERLAY_DECODE_SUPPORT, // D3DKMT_MULTIPLANEOVERLAY_DECODE_SUPPORT
    KMTQAITYPE_QUERY_HW_PROTECTION_TEARDOWN_COUNT, // UINT32
    KMTQAITYPE_QUERY_ISBADDRIVERFORHWPROTECTIONDISABLED, // D3DKMT_ISBADDRIVERFORHWPROTECTIONDISABLED
    KMTQAITYPE_MULTIPLANEOVERLAY_SECONDARY_SUPPORT, // D3DKMT_MULTIPLANEOVERLAY_SECONDARY_SUPPORT
    KMTQAITYPE_INDEPENDENTFLIP_SECONDARY_SUPPORT, // D3DKMT_INDEPENDENTFLIP_SECONDARY_SUPPORT
    KMTQAITYPE_PANELFITTER_SUPPORT, // D3DKMT_PANELFITTER_SUPPORT // since WDDM2_1 // 40
    KMTQAITYPE_PHYSICALADAPTERPNPKEY, // D3DKMT_QUERY_PHYSICAL_ADAPTER_PNP_KEY // since WDDM2_2
    KMTQAITYPE_GETSEGMENTGROUPSIZE, // D3DKMT_SEGMENTGROUPSIZEINFO
    KMTQAITYPE_MPO3DDI_SUPPORT, // D3DKMT_MPO3DDI_SUPPORT
    KMTQAITYPE_HWDRM_SUPPORT, // D3DKMT_HWDRM_SUPPORT
    KMTQAITYPE_MPOKERNELCAPS_SUPPORT, // D3DKMT_MPOKERNELCAPS_SUPPORT
    KMTQAITYPE_MULTIPLANEOVERLAY_STRETCH_SUPPORT, // D3DKMT_MULTIPLANEOVERLAY_STRETCH_SUPPORT
    KMTQAITYPE_GET_DEVICE_VIDPN_OWNERSHIP_INFO, // D3DKMT_GET_DEVICE_VIDPN_OWNERSHIP_INFO
    KMTQAITYPE_QUERYREGISTRY, // D3DDDI_QUERYREGISTRY_INFO // since WDDM2_4
    KMTQAITYPE_KMD_DRIVER_VERSION, // D3DKMT_KMD_DRIVER_VERSION
    KMTQAITYPE_BLOCKLIST_KERNEL, // D3DKMT_BLOCKLIST_INFO // 50
    KMTQAITYPE_BLOCKLIST_RUNTIME, // D3DKMT_BLOCKLIST_INFO
    KMTQAITYPE_ADAPTERGUID_RENDER, // GUID
    KMTQAITYPE_ADAPTERADDRESS_RENDER, // D3DKMT_ADAPTERADDRESS
    KMTQAITYPE_ADAPTERREGISTRYINFO_RENDER, // D3DKMT_ADAPTERREGISTRYINFO
    KMTQAITYPE_CHECKDRIVERUPDATESTATUS_RENDER,
    KMTQAITYPE_DRIVERVERSION_RENDER, // D3DKMT_DRIVERVERSION
    KMTQAITYPE_ADAPTERTYPE_RENDER, // D3DKMT_ADAPTERTYPE
    KMTQAITYPE_WDDM_1_2_CAPS_RENDER, // D3DKMT_WDDM_1_2_CAPS
    KMTQAITYPE_WDDM_1_3_CAPS_RENDER, // D3DKMT_WDDM_1_3_CAPS
    KMTQAITYPE_QUERY_ADAPTER_UNIQUE_GUID, // D3DKMT_QUERY_ADAPTER_UNIQUE_GUID // 60
    KMTQAITYPE_NODEPERFDATA, // D3DKMT_NODE_PERFDATA
    KMTQAITYPE_ADAPTERPERFDATA, // D3DKMT_ADAPTER_PERFDATA
    KMTQAITYPE_ADAPTERPERFDATA_CAPS, // D3DKMT_ADAPTER_PERFDATACAPS
    KMTQUITYPE_GPUVERSION, // D3DKMT_GPUVERSION
    KMTQAITYPE_DRIVER_DESCRIPTION, // D3DKMT_DRIVER_DESCRIPTION // since WDDM2_6
    KMTQAITYPE_DRIVER_DESCRIPTION_RENDER, // D3DKMT_DRIVER_DESCRIPTION
    KMTQAITYPE_SCANOUT_CAPS, // D3DKMT_QUERY_SCANOUT_CAPS
    KMTQAITYPE_DISPLAY_UMDRIVERNAME,
    KMTQAITYPE_PARAVIRTUALIZATION_RENDER,
    KMTQAITYPE_SERVICENAME, // 70
    KMTQAITYPE_WDDM_2_7_CAPS, // D3DKMT_WDDM_2_7_CAPS
    KMTQAITYPE_TRACKEDWORKLOAD_SUPPORT
} KMTQUERYADAPTERINFOTYPE;

typedef enum _KMTUMDVERSION
{
    KMTUMDVERSION_DX9,
    KMTUMDVERSION_DX10,
    KMTUMDVERSION_DX11,
    KMTUMDVERSION_DX12,
    NUM_KMTUMDVERSIONS
} KMTUMDVERSION;

// The D3DKMT_UMDFILENAMEINFO structure contains the name of an OpenGL ICD that is based on the specified version of the DirectX runtime.
typedef struct _D3DKMT_UMDFILENAMEINFO
{
    _In_ KMTUMDVERSION Version; // A KMTUMDVERSION-typed value that indicates the version of the DirectX runtime to retrieve the name of an OpenGL ICD for.
    _Out_ WCHAR UmdFileName[MAX_PATH]; // A string that contains the name of the OpenGL ICD.
} D3DKMT_UMDFILENAMEINFO;

// The D3DKMT_OPENGLINFO structure describes OpenGL installable client driver (ICD) information.
typedef struct _D3DKMT_OPENGLINFO
{
    _Out_ WCHAR UmdOpenGlIcdFileName[MAX_PATH]; // An array of wide characters that contains the file name of the OpenGL ICD.
    _Out_ ULONG Version; // The version of the OpenGL ICD.
    _In_ ULONG Flags; // This member is currently unused.
} D3DKMT_OPENGLINFO;

// The D3DKMT_SEGMENTSIZEINFO structure describes the size, in bytes, of memory and aperture segments.
typedef struct _D3DKMT_SEGMENTSIZEINFO
{
    _Out_ ULONGLONG DedicatedVideoMemorySize; // The size, in bytes, of memory that is dedicated from video memory.
    _Out_ ULONGLONG DedicatedSystemMemorySize; // The size, in bytes, of memory that is dedicated from system memory.
    _Out_ ULONGLONG SharedSystemMemorySize; // The size, in bytes, of memory from system memory that can be shared by many users.
} D3DKMT_SEGMENTSIZEINFO;

// The D3DKMT_FLIPINFOFLAGS structure identifies flipping capabilities of the display miniport driver.
typedef struct _D3DKMT_FLIPINFOFLAGS 
{
    UINT32 FlipInterval : 1; // A UINT value that specifies whether the display miniport driver natively supports the scheduling of a flip command to take effect after two, three or four vertical syncs occur. 
    UINT32 Reserved : 31;
} D3DKMT_FLIPINFOFLAGS;

// The D3DKMT_FLIPQUEUEINFO structure describes information about the graphics adapter's queue of flip operations.
typedef struct _D3DKMT_FLIPQUEUEINFO 
{
    _Out_ UINT32 MaxHardwareFlipQueueLength; // The maximum number of flip operations that can be queued for hardware-flip queuing.
    _Out_ UINT32 MaxSoftwareFlipQueueLength; // The maximum number of flip operations that can be queued for software-flip queuing on hardware that supports memory mapped I/O (MMIO)-based flips.
    _Out_ D3DKMT_FLIPINFOFLAGS FlipFlags; // indicates, in bit-field flags, flipping capabilities.
} D3DKMT_FLIPQUEUEINFO;

// The D3DKMT_ADAPTERADDRESS structure describes the physical location of the graphics adapter.
typedef struct _D3DKMT_ADAPTERADDRESS 
{
    _Out_ UINT32 BusNumber; // The number of the bus that the graphics adapter's physical device is located on.
    _Out_ UINT32 DeviceNumber; // The index of the graphics adapter's physical device on the bus.
    _Out_ UINT32 FunctionNumber; // The function number of the graphics adapter on the physical device.
} D3DKMT_ADAPTERADDRESS;

typedef struct _D3DKMT_WORKINGSETFLAGS
{
    UINT32 UseDefault : 1; // A UINT value that specifies whether the display miniport driver uses the default working set.
    UINT32 Reserved : 31; // This member is reserved and should be set to zero.
} D3DKMT_WORKINGSETFLAGS;

// The D3DKMT_WORKINGSETINFO structure describes information about the graphics adapter's working set.
typedef struct _D3DKMT_WORKINGSETINFO
{
    _Out_ D3DKMT_WORKINGSETFLAGS Flags; // A D3DKMT_WORKINGSETFLAGS structure that indicates, in bit-field flags, working-set properties.
    _Out_ ULONG MinimumWorkingSetPercentile; // The minimum working-set percentile.
    _Out_ ULONG MaximumWorkingSetPercentile; // The maximum working-set percentile.
} D3DKMT_WORKINGSETINFO;

// The D3DKMT_ADAPTERREGISTRYINFO structure contains registry information about the graphics adapter.
typedef struct _D3DKMT_ADAPTERREGISTRYINFO 
{
    _Out_ WCHAR AdapterString[MAX_PATH]; // A string that contains the name of the graphics adapter.
    _Out_ WCHAR BiosString[MAX_PATH]; // A string that contains the name of the BIOS for the graphics adapter.
    _Out_ WCHAR DacType[MAX_PATH]; // A string that contains the DAC type for the graphics adapter.
    _Out_ WCHAR ChipType[MAX_PATH]; // A string that contains the chip type for the graphics adapter.
} D3DKMT_ADAPTERREGISTRYINFO;

#ifdef USE_D3DKMT_SINGLE_HEADER
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |       \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

typedef enum _D3DDDIFORMAT
{
    D3DDDIFMT_UNKNOWN = 0,
    D3DDDIFMT_R8G8B8 = 20,
    D3DDDIFMT_A8R8G8B8 = 21,
    D3DDDIFMT_X8R8G8B8 = 22,
    D3DDDIFMT_R5G6B5 = 23,
    D3DDDIFMT_X1R5G5B5 = 24,
    D3DDDIFMT_A1R5G5B5 = 25,
    D3DDDIFMT_A4R4G4B4 = 26,
    D3DDDIFMT_R3G3B2 = 27,
    D3DDDIFMT_A8 = 28,
    D3DDDIFMT_A8R3G3B2 = 29,
    D3DDDIFMT_X4R4G4B4 = 30,
    D3DDDIFMT_A2B10G10R10 = 31,
    D3DDDIFMT_A8B8G8R8 = 32,
    D3DDDIFMT_X8B8G8R8 = 33,
    D3DDDIFMT_G16R16 = 34,
    D3DDDIFMT_A2R10G10B10 = 35,
    D3DDDIFMT_A16B16G16R16 = 36,
    D3DDDIFMT_A8P8 = 40,
    D3DDDIFMT_P8 = 41,
    D3DDDIFMT_L8 = 50,
    D3DDDIFMT_A8L8 = 51,
    D3DDDIFMT_A4L4 = 52,
    D3DDDIFMT_V8U8 = 60,
    D3DDDIFMT_L6V5U5 = 61,
    D3DDDIFMT_X8L8V8U8 = 62,
    D3DDDIFMT_Q8W8V8U8 = 63,
    D3DDDIFMT_V16U16 = 64,
    D3DDDIFMT_W11V11U10 = 65,
    D3DDDIFMT_A2W10V10U10 = 67,

    D3DDDIFMT_UYVY = MAKEFOURCC('U', 'Y', 'V', 'Y'),
    D3DDDIFMT_R8G8_B8G8 = MAKEFOURCC('R', 'G', 'B', 'G'),
    D3DDDIFMT_YUY2 = MAKEFOURCC('Y', 'U', 'Y', '2'),
    D3DDDIFMT_G8R8_G8B8 = MAKEFOURCC('G', 'R', 'G', 'B'),
    D3DDDIFMT_DXT1 = MAKEFOURCC('D', 'X', 'T', '1'),
    D3DDDIFMT_DXT2 = MAKEFOURCC('D', 'X', 'T', '2'),
    D3DDDIFMT_DXT3 = MAKEFOURCC('D', 'X', 'T', '3'),
    D3DDDIFMT_DXT4 = MAKEFOURCC('D', 'X', 'T', '4'),
    D3DDDIFMT_DXT5 = MAKEFOURCC('D', 'X', 'T', '5'),

    D3DDDIFMT_D16_LOCKABLE = 70,
    D3DDDIFMT_D32 = 71,
    D3DDDIFMT_D15S1 = 73,
    D3DDDIFMT_D24S8 = 75,
    D3DDDIFMT_D24X8 = 77,
    D3DDDIFMT_D24X4S4 = 79,
    D3DDDIFMT_D16 = 80,
    D3DDDIFMT_D32F_LOCKABLE = 82,
    D3DDDIFMT_D24FS8 = 83,
    D3DDDIFMT_D32_LOCKABLE = 84,
    D3DDDIFMT_S8_LOCKABLE = 85,
    D3DDDIFMT_S1D15 = 72,
    D3DDDIFMT_S8D24 = 74,
    D3DDDIFMT_X8D24 = 76,
    D3DDDIFMT_X4S4D24 = 78,
    D3DDDIFMT_L16 = 81,
    D3DDDIFMT_G8R8 = 91, // WDDM1_3
    D3DDDIFMT_R8 = 92, // WDDM1_3
    D3DDDIFMT_VERTEXDATA = 100,
    D3DDDIFMT_INDEX16 = 101,
    D3DDDIFMT_INDEX32 = 102,
    D3DDDIFMT_Q16W16V16U16 = 110,

    D3DDDIFMT_MULTI2_ARGB8 = MAKEFOURCC('M', 'E', 'T', '1'),

    // Floating point surface formats

    // s10e5 formats (16-bits per channel)
    D3DDDIFMT_R16F = 111,
    D3DDDIFMT_G16R16F = 112,
    D3DDDIFMT_A16B16G16R16F = 113,

    // IEEE s23e8 formats (32-bits per channel)
    D3DDDIFMT_R32F = 114,
    D3DDDIFMT_G32R32F = 115,
    D3DDDIFMT_A32B32G32R32F = 116,

    D3DDDIFMT_CxV8U8 = 117,

    // Monochrome 1 bit per pixel format
    D3DDDIFMT_A1 = 118,

    // 2.8 biased fixed point
    D3DDDIFMT_A2B10G10R10_XR_BIAS = 119,

    // Decode compressed buffer formats
    D3DDDIFMT_DXVACOMPBUFFER_BASE = 150,
    D3DDDIFMT_PICTUREPARAMSDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE + 0,    // 150
    D3DDDIFMT_MACROBLOCKDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE + 1,    // 151
    D3DDDIFMT_RESIDUALDIFFERENCEDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE + 2,    // 152
    D3DDDIFMT_DEBLOCKINGDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE + 3,    // 153
    D3DDDIFMT_INVERSEQUANTIZATIONDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE + 4,    // 154
    D3DDDIFMT_SLICECONTROLDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE + 5,    // 155
    D3DDDIFMT_BITSTREAMDATA = D3DDDIFMT_DXVACOMPBUFFER_BASE + 6,    // 156
    D3DDDIFMT_MOTIONVECTORBUFFER = D3DDDIFMT_DXVACOMPBUFFER_BASE + 7,    // 157
    D3DDDIFMT_FILMGRAINBUFFER = D3DDDIFMT_DXVACOMPBUFFER_BASE + 8,    // 158
    D3DDDIFMT_DXVA_RESERVED9 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 9,    // 159
    D3DDDIFMT_DXVA_RESERVED10 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 10,   // 160
    D3DDDIFMT_DXVA_RESERVED11 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 11,   // 161
    D3DDDIFMT_DXVA_RESERVED12 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 12,   // 162
    D3DDDIFMT_DXVA_RESERVED13 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 13,   // 163
    D3DDDIFMT_DXVA_RESERVED14 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 14,   // 164
    D3DDDIFMT_DXVA_RESERVED15 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 15,   // 165
    D3DDDIFMT_DXVA_RESERVED16 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 16,   // 166
    D3DDDIFMT_DXVA_RESERVED17 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 17,   // 167
    D3DDDIFMT_DXVA_RESERVED18 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 18,   // 168
    D3DDDIFMT_DXVA_RESERVED19 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 19,   // 169
    D3DDDIFMT_DXVA_RESERVED20 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 20,   // 170
    D3DDDIFMT_DXVA_RESERVED21 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 21,   // 171
    D3DDDIFMT_DXVA_RESERVED22 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 22,   // 172
    D3DDDIFMT_DXVA_RESERVED23 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 23,   // 173
    D3DDDIFMT_DXVA_RESERVED24 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 24,   // 174
    D3DDDIFMT_DXVA_RESERVED25 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 25,   // 175
    D3DDDIFMT_DXVA_RESERVED26 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 26,   // 176
    D3DDDIFMT_DXVA_RESERVED27 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 27,   // 177
    D3DDDIFMT_DXVA_RESERVED28 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 28,   // 178
    D3DDDIFMT_DXVA_RESERVED29 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 29,   // 179
    D3DDDIFMT_DXVA_RESERVED30 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 30,   // 180
    D3DDDIFMT_DXVA_RESERVED31 = D3DDDIFMT_DXVACOMPBUFFER_BASE + 31,   // 181
    D3DDDIFMT_DXVACOMPBUFFER_MAX = D3DDDIFMT_DXVA_RESERVED31,

    D3DDDIFMT_BINARYBUFFER = 199,

    D3DDDIFMT_FORCE_UINT = 0x7fffffff
} D3DDDIFORMAT;

typedef struct _D3DDDI_RATIONAL
{
    UINT32 Numerator;
    UINT32 Denominator;
} D3DDDI_RATIONAL;

typedef enum _D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING
{
    D3DDDI_VSSLO_UNINITIALIZED = 0,
    D3DDDI_VSSLO_PROGRESSIVE = 1,
    D3DDDI_VSSLO_INTERLACED_UPPERFIELDFIRST = 2,
    D3DDDI_VSSLO_INTERLACED_LOWERFIELDFIRST = 3,
    D3DDDI_VSSLO_OTHER = 255
} D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING;

typedef enum _D3DDDI_ROTATION
{
    D3DDDI_ROTATION_IDENTITY = 1, // No rotation.
    D3DDDI_ROTATION_90 = 2, // Rotated 90 degrees.
    D3DDDI_ROTATION_180 = 3, // Rotated 180 degrees.
    D3DDDI_ROTATION_270 = 4 // Rotated 270 degrees.
} D3DDDI_ROTATION;
#endif

typedef enum _D3DKMDT_MODE_PRUNING_REASON
{
    D3DKMDT_MPR_UNINITIALIZED = 0, // mode was pruned or is supported because of:
    D3DKMDT_MPR_ALLCAPS = 1, //   all of the monitor caps (only used to imply lack of support - for support, specific reason is always indicated)
    D3DKMDT_MPR_DESCRIPTOR_MONITOR_SOURCE_MODE = 2, //   monitor source mode in the monitor descriptor
    D3DKMDT_MPR_DESCRIPTOR_MONITOR_FREQUENCY_RANGE = 3, //   monitor frequency range in the monitor descriptor
    D3DKMDT_MPR_DESCRIPTOR_OVERRIDE_MONITOR_SOURCE_MODE = 4, //   monitor source mode in the monitor descriptor override
    D3DKMDT_MPR_DESCRIPTOR_OVERRIDE_MONITOR_FREQUENCY_RANGE = 5, //   monitor frequency range in the monitor descriptor override
    D3DKMDT_MPR_DEFAULT_PROFILE_MONITOR_SOURCE_MODE = 6, //   monitor source mode in the default monitor profile
    D3DKMDT_MPR_DRIVER_RECOMMENDED_MONITOR_SOURCE_MODE = 7, //   monitor source mode recommended by the driver
    D3DKMDT_MPR_MONITOR_FREQUENCY_RANGE_OVERRIDE = 8, //   monitor frequency range override
    D3DKMDT_MPR_CLONE_PATH_PRUNED = 9, //   Mode is pruned because other path(s) in clone cluster has(have) no mode supported by monitor
    D3DKMDT_MPR_MAXVALID = 10
} D3DKMDT_MODE_PRUNING_REASON;

// This structure takes 8 bytes.
// The unnamed UINT of size 0 forces alignment of the structure to
// make it exactly occupy 8 bytes, see MSDN docs on C++ bitfields
// for more details
typedef struct _D3DKMDT_DISPLAYMODE_FLAGS
{
    UINT32 ValidatedAgainstMonitorCaps : 1;
    UINT32 RoundedFakeMode : 1;
    UINT32 : 0;
    D3DKMDT_MODE_PRUNING_REASON ModePruningReason : 4;
    UINT32 Stereo : 1; // since WIN8
    UINT32 AdvancedScanCapable : 1;
    UINT32 PreferredTiming : 1; // since WDDM2_0
    UINT32 PhysicalModeSupported : 1;
    UINT32 Reserved : 24;
} D3DKMDT_DISPLAYMODE_FLAGS;

// The D3DKMT_DISPLAYMODE structure describes a display mode.
typedef struct _D3DKMT_DISPLAYMODE 
{
    _Out_ UINT32 Width; // The screen width of the display mode, in pixels.
    _Out_ UINT32 Height; // The screen height of the display mode, in pixels.
    _Out_ D3DDDIFORMAT Format; // A D3DDDIFORMAT-typed value that indicates the pixel format of the display mode.
    _Out_ UINT32 IntegerRefreshRate; // A UINT value that indicates the refresh rate of the display mode.
    _Out_ D3DDDI_RATIONAL RefreshRate; // A D3DDDI_RATIONAL structure that indicates the refresh rate of the display mode.
    _Out_ D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING ScanLineOrdering; // A D3DDDI_VIDEO_SIGNAL_SCANLINE_ORDERING-typed value that indicates how scan lines are ordered in the display mode.
    _Out_ D3DDDI_ROTATION DisplayOrientation; // A D3DDDI_ROTATION-typed value that identifies the orientation of the display mode.
    _Out_ UINT32 DisplayFixedOutput; // The fixed output of the display mode.
    _Out_ D3DKMDT_DISPLAYMODE_FLAGS Flags; // A D3DKMDT_DISPLAYMODE_FLAGS structure that indicates information about the display mode.
} D3DKMT_DISPLAYMODE;

// The D3DKMT_CURRENTDISPLAYMODE structure describes the current display mode of the specified video source.
typedef struct _D3DKMT_CURRENTDISPLAYMODE
{
    _In_ UINT32 VidPnSourceId; // The zero-based identification number of the video present source in a path of a video present network (VidPN) topology that the display mode applies to.
    _Out_ D3DKMT_DISPLAYMODE DisplayMode; // A D3DKMT_DISPLAYMODE structure that represents the current display mode.
} D3DKMT_CURRENTDISPLAYMODE;

// private
typedef struct _D3DKMT_VIRTUALADDRESSFLAGS
{
    UINT32 VirtualAddressSupported : 1;
    UINT32 Reserved : 31;
} D3DKMT_VIRTUALADDRESSFLAGS;

// private
typedef struct _D3DKMT_VIRTUALADDRESSINFO
{
    D3DKMT_VIRTUALADDRESSFLAGS VirtualAddressFlags;
} D3DKMT_VIRTUALADDRESSINFO;

// The D3DKMT_DRIVERVERSION enumeration type contains values that indicate the version of the display driver model that the display miniport driver supports.
typedef enum D3DKMT_DRIVERVERSION // QAI_DRIVERVERSION
{
    KMT_DRIVERVERSION_WDDM_1_0 = 1000, // The display miniport driver supports the Windows Vista display driver model (WDDM) without Windows 7 features.
    KMT_DRIVERVERSION_WDDM_1_1_PRERELEASE = 1102, // The display miniport driver supports the Windows Vista display driver model with prereleased Windows 7 features.
    KMT_DRIVERVERSION_WDDM_1_1 = 1105, // The display miniport driver supports the Windows Vista display driver model with released Windows 7 features.
    KMT_DRIVERVERSION_WDDM_1_2 = 1200, // The display miniport driver supports the Windows Vista display driver model with released Windows 8 features. Supported starting with Windows 8.
    KMT_DRIVERVERSION_WDDM_1_3 = 1300, // The display miniport driver supports the Windows display driver model with released Windows 8.1 features. Supported starting with Windows 8.1.
    KMT_DRIVERVERSION_WDDM_2_0 = 2000, // The display miniport driver supports the Windows display driver model with released Windows 10 features. Supported starting with Windows 10.
    KMT_DRIVERVERSION_WDDM_2_1 = 2100, // 1607
    KMT_DRIVERVERSION_WDDM_2_2 = 2200, // 1703
    KMT_DRIVERVERSION_WDDM_2_3 = 2300, // 1709
    KMT_DRIVERVERSION_WDDM_2_4 = 2400, // 1803
    KMT_DRIVERVERSION_WDDM_2_5 = 2500, // 1809
    KMT_DRIVERVERSION_WDDM_2_6 = 2600, // 19H1
    KMT_DRIVERVERSION_WDDM_2_7 = 2700 // 20H1
} D3DKMT_DRIVERVERSION;

// Specifies the type of display device that the graphics adapter supports.
typedef struct _D3DKMT_ADAPTERTYPE
{
    union
    {
        struct
        {
            UINT32 RenderSupported : 1;
            UINT32 DisplaySupported : 1;
            UINT32 SoftwareDevice : 1;
            UINT32 PostDevice : 1;
            UINT32 HybridDiscrete : 1; // since WDDM1_3
            UINT32 HybridIntegrated : 1;
            UINT32 IndirectDisplayDevice : 1;
            UINT32 Paravirtualized : 1; // since WDDM2_3
            UINT32 ACGSupported : 1;
            UINT32 SupportSetTimingsFromVidPn : 1;
            UINT32 Detachable : 1;
            UINT32 ComputeOnly : 1; // since WDDM2_7
            UINT32 Prototype : 1;
            UINT32 Reserved : 19;
        };
        UINT32 Value;
    };
} D3DKMT_ADAPTERTYPE;

// Specifies the number of current Desktop Duplication API (DDA) clients that are attached to a given video present network (VidPN).
typedef struct _D3DKMT_OUTPUTDUPLCONTEXTSCOUNT
{
    UINT32 VidPnSourceId; // The ID of the video present network (VidPN).
    UINT32 OutputDuplicationCount; // The number of current DDA clients that are attached to the VidPN specified by the VidPnSourceId member.
} D3DKMT_OUTPUTDUPLCONTEXTSCOUNT;

#ifdef USE_D3DKMT_SINGLE_HEADER
typedef enum _D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY
{
    D3DKMDT_GRAPHICS_PREEMPTION_NONE = 0,
    D3DKMDT_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY = 100,
    D3DKMDT_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY = 200,
    D3DKMDT_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY = 300,
    D3DKMDT_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY = 400,
    D3DKMDT_GRAPHICS_PREEMPTION_SHADER_BOUNDARY = 500,
} D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY;

typedef enum _D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY
{
    D3DKMDT_COMPUTE_PREEMPTION_NONE = 0,
    D3DKMDT_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY = 100,
    D3DKMDT_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY = 200,
    D3DKMDT_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY = 300,
    D3DKMDT_COMPUTE_PREEMPTION_THREAD_BOUNDARY = 400,
    D3DKMDT_COMPUTE_PREEMPTION_SHADER_BOUNDARY = 500,
} D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY;

typedef struct _D3DKMDT_PREEMPTION_CAPS
{
    D3DKMDT_GRAPHICS_PREEMPTION_GRANULARITY GraphicsPreemptionGranularity;
    D3DKMDT_COMPUTE_PREEMPTION_GRANULARITY ComputePreemptionGranularity;
} D3DKMDT_PREEMPTION_CAPS;

typedef struct _D3DKMT_WDDM_1_2_CAPS
{
    D3DKMDT_PREEMPTION_CAPS PreemptionCaps;
    union
    {
        struct
        {
            UINT32 SupportNonVGA : 1;
            UINT32 SupportSmoothRotation : 1;
            UINT32 SupportPerEngineTDR : 1;
            UINT32 SupportKernelModeCommandBuffer : 1;
            UINT32 SupportCCD : 1;
            UINT32 SupportSoftwareDeviceBitmaps : 1;
            UINT32 SupportGammaRamp : 1;
            UINT32 SupportHWCursor : 1;
            UINT32 SupportHWVSync : 1;
            UINT32 SupportSurpriseRemovalInHibernation : 1;
            UINT32 Reserved : 22;
        };
        UINT32 Value;
    };
} D3DKMT_WDDM_1_2_CAPS;
#endif

// Indicates the version number of the user-mode driver.
typedef struct _D3DKMT_UMD_DRIVER_VERSION 
{
    LARGE_INTEGER DriverVersion; // The user-mode driver version.
} D3DKMT_UMD_DRIVER_VERSION;

// Indicates whether the user-mode driver supports Direct Flip operations, in which video memory is seamlessly flipped between an application's managed primary allocations and the Desktop Window Manager (DWM) managed primary allocations.
typedef struct _D3DKMT_DIRECTFLIP_SUPPORT
{
    BOOL Supported; // If TRUE, the driver supports Direct Flip operations. Otherwise, the driver does not support Direct Flip operations.
} D3DKMT_DIRECTFLIP_SUPPORT;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_SUPPORT
{
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_SUPPORT;

typedef struct _D3DKMT_DLIST_DRIVER_NAME
{
    _Out_ WCHAR DListFileName[MAX_PATH]; // DList driver file name
} D3DKMT_DLIST_DRIVER_NAME;

#ifdef USE_D3DKMT_SINGLE_HEADER
typedef struct _D3DKMT_WDDM_1_3_CAPS
{
    union
    {
        struct
        {
            UINT32 SupportMiracast : 1;
            UINT32 IsHybridIntegratedGPU : 1;
            UINT32 IsHybridDiscreteGPU : 1;
            UINT32 SupportPowerManagementPStates : 1;
            UINT32 SupportVirtualModes : 1;
            UINT32 SupportCrossAdapterResource : 1;
            UINT32 Reserved : 26;
        };
        UINT32 Value;
    };
} D3DKMT_WDDM_1_3_CAPS;
#endif

typedef struct _D3DKMT_MULTIPLANEOVERLAY_HUD_SUPPORT
{
    UINT32 VidPnSourceId; // Not yet used.
    BOOL Update;
    BOOL KernelSupported;
    BOOL HudSupported;
} D3DKMT_MULTIPLANEOVERLAY_HUD_SUPPORT;

#ifdef USE_D3DKMT_SINGLE_HEADER
typedef struct _D3DKMT_WDDM_2_0_CAPS
{
    union
    {
        struct
        {
            UINT32 Support64BitAtomics : 1;
            UINT32 GpuMmuSupported : 1;
            UINT32 IoMmuSupported : 1;
            UINT32 FlipOverwriteSupported : 1; // since WDDM2_4
            UINT32 SupportContextlessPresent : 1;
            UINT32 SupportSurpriseRemoval : 1; // since WDDM2_7
            UINT32 Reserved : 26;
        };
        UINT32 Value;
    };
} D3DKMT_WDDM_2_0_CAPS;
#endif

#ifdef USE_D3DKMT_SINGLE_HEADER
#include <pshpack1.h>

#define DXGK_MAX_METADATA_NAME_LENGTH 32

typedef enum _DXGK_ENGINE_TYPE
{
    DXGK_ENGINE_TYPE_OTHER = 0, // This value is used for proprietary or unique functionality that is not exposed by typical adapters, as well as for an engine that performs work that doesn't fall under another category.
    DXGK_ENGINE_TYPE_3D = 1, // The adapter's 3-D processing engine. All adapters that are not a display-only device have one 3-D engine.
    DXGK_ENGINE_TYPE_VIDEO_DECODE = 2, // The engine that handles video decoding, including decompression of video frames from an input stream into typical YUV surfaces. The workload packets for an H.264 video codec workload test must appear on either the decode engine or the 3-D engine.
    DXGK_ENGINE_TYPE_VIDEO_ENCODE = 3, // The engine that handles video encoding, including compression of typical video frames into an encoded video format.
    DXGK_ENGINE_TYPE_VIDEO_PROCESSING = 4, // The engine that is responsible for any video processing that is done after a video input stream is decoded. Such processing can include RGB surface conversion, filtering, stretching, color correction, deinterlacing, or other steps that are required before the final image is rendered to the display screen. The workload packets for workload tests must appear on either the video processing engine or the 3-D engine.
    DXGK_ENGINE_TYPE_SCENE_ASSEMBLY = 5, // The engine that performs vertex processing of 3-D workloads as a preliminary pass prior to the remainder of the 3-D rendering. This engine also stores vertices in bins that tile-based rendering engines use.
    DXGK_ENGINE_TYPE_COPY = 6, // The engine that is a copy engine used for moving data. This engine can perform subresource updates, blitting, paging, or other similar data handling. The workload packets for calls to CopySubresourceRegion or UpdateSubResource methods of Direct3D 10 and Direct3D 11 must appear on either the copy engine or the 3-D engine.
    DXGK_ENGINE_TYPE_OVERLAY = 7, // The virtual engine that is used for synchronized flipping of overlays in Direct3D 9.
    DXGK_ENGINE_TYPE_CRYPTO,
    DXGK_ENGINE_TYPE_MAX
} DXGK_ENGINE_TYPE;

typedef struct _DXGK_NODEMETADATA_FLAGS
{
    union
    {
        struct
        {
            UINT32 ContextSchedulingSupported : 1;
            UINT32 RingBufferFenceRelease : 1;
            UINT32 SupportTrackedWorkload : 1;
            UINT32 Reserved : 13;
            UINT32 MaxInFlightHwQueueBuffers : 16;
        };
        UINT32 Value;
    };
} DXGK_NODEMETADATA_FLAGS;

typedef struct _DXGK_NODEMETADATA
{
    DXGK_ENGINE_TYPE EngineType;
    WCHAR FriendlyName[DXGK_MAX_METADATA_NAME_LENGTH];
    DXGK_NODEMETADATA_FLAGS Flags;
    BOOLEAN GpuMmuSupported;
    BOOLEAN IoMmuSupported;
} DXGK_NODEMETADATA;

typedef DXGK_NODEMETADATA DXGKARG_GETNODEMETADATA;

typedef struct _D3DKMT_NODEMETADATA
{
    _In_ UINT32 NodeOrdinalAndAdapterIndex;
    _Out_ DXGK_NODEMETADATA NodeData;
} D3DKMT_NODEMETADATA;

#include <poppack.h>
#endif

typedef struct _D3DKMT_CPDRIVERNAME
{
    WCHAR ContentProtectionFileName[MAX_PATH];
} D3DKMT_CPDRIVERNAME;

typedef struct _D3DKMT_XBOX
{
    BOOL IsXBOX;
} D3DKMT_XBOX;

typedef struct _D3DKMT_INDEPENDENTFLIP_SUPPORT
{
    BOOL Supported;
} D3DKMT_INDEPENDENTFLIP_SUPPORT;

typedef struct _D3DKMT_MIRACASTCOMPANIONDRIVERNAME
{
    WCHAR MiracastCompanionDriverName[MAX_PATH];
} D3DKMT_MIRACASTCOMPANIONDRIVERNAME;

typedef struct _D3DKMT_PHYSICAL_ADAPTER_COUNT
{
    UINT32 Count;
} D3DKMT_PHYSICAL_ADAPTER_COUNT;

typedef struct _D3DKMT_DEVICE_IDS
{
    UINT32 VendorID;
    UINT32 DeviceID;
    UINT32 SubVendorID;
    UINT32 SubSystemID;
    UINT32 RevisionID;
    UINT32 BusType;
} D3DKMT_DEVICE_IDS;

typedef struct _D3DKMT_QUERY_DEVICE_IDS
{
    _In_ UINT32 PhysicalAdapterIndex;
    _Out_ D3DKMT_DEVICE_IDS DeviceIds;
} D3DKMT_QUERY_DEVICE_IDS;

#ifdef USE_D3DKMT_SINGLE_HEADER
typedef struct _D3DKMT_DRIVERCAPS_EXT
{
    union
    {
        struct
        {
            UINT32 VirtualModeSupport : 1;
            UINT32 Reserved : 31;
        };
        UINT32 Value;
    };
} D3DKMT_DRIVERCAPS_EXT;
#endif

typedef enum _D3DKMT_MIRACAST_DRIVER_TYPE
{
    D3DKMT_MIRACAST_DRIVER_NOT_SUPPORTED = 0,
    D3DKMT_MIRACAST_DRIVER_IHV = 1,
    D3DKMT_MIRACAST_DRIVER_MS = 2,
} D3DKMT_MIRACAST_DRIVER_TYPE;

typedef struct _D3DKMT_QUERY_MIRACAST_DRIVER_TYPE
{
    D3DKMT_MIRACAST_DRIVER_TYPE MiracastDriverType;
} D3DKMT_QUERY_MIRACAST_DRIVER_TYPE;

typedef struct _D3DKMT_GPUMMU_CAPS
{
    union
    {
        struct
        {
            UINT32 ReadOnlyMemorySupported : 1;
            UINT32 NoExecuteMemorySupported : 1;
            UINT32 CacheCoherentMemorySupported : 1;
            UINT32 Reserved : 29;
        };
        UINT32 Value;
    } Flags;
    UINT32 VirtualAddressBitCount;
} D3DKMT_GPUMMU_CAPS;

typedef struct _D3DKMT_QUERY_GPUMMU_CAPS
{
    _In_ UINT32 PhysicalAdapterIndex;
    _Out_ D3DKMT_GPUMMU_CAPS Caps;
} D3DKMT_QUERY_GPUMMU_CAPS;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_DECODE_SUPPORT
{
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_DECODE_SUPPORT;

typedef struct _D3DKMT_ISBADDRIVERFORHWPROTECTIONDISABLED
{
    BOOL Disabled;
} D3DKMT_ISBADDRIVERFORHWPROTECTIONDISABLED;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_SECONDARY_SUPPORT
{
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_SECONDARY_SUPPORT;

typedef struct _D3DKMT_INDEPENDENTFLIP_SECONDARY_SUPPORT
{
    BOOL Supported;
} D3DKMT_INDEPENDENTFLIP_SECONDARY_SUPPORT;

typedef struct _D3DKMT_PANELFITTER_SUPPORT
{
    BOOL Supported;
} D3DKMT_PANELFITTER_SUPPORT;

typedef enum _D3DKMT_PNP_KEY_TYPE
{
    D3DKMT_PNP_KEY_HARDWARE = 1,
    D3DKMT_PNP_KEY_SOFTWARE = 2
} D3DKMT_PNP_KEY_TYPE;

// A structure that holds information to query the physical adapter PNP key.
typedef struct _D3DKMT_QUERY_PHYSICAL_ADAPTER_PNP_KEY
{
    _In_ UINT32 PhysicalAdapterIndex; // The physical adapter index.
    _In_ D3DKMT_PNP_KEY_TYPE PnPKeyType; // The type of the PNP key being queried.
    _Field_size_opt_(*pCchDest) WCHAR *pDest; // A WCHAR value respresenting the pDest.
    _Inout_ UINT32 *pCchDest; // A UINT value representing the pCchDest.
} D3DKMT_QUERY_PHYSICAL_ADAPTER_PNP_KEY;

// A structure that holds information about the segment group size.
typedef struct _D3DKMT_SEGMENTGROUPSIZEINFO
{
    _In_ UINT32 PhysicalAdapterIndex; // An index to the physical adapter.
    _Out_ D3DKMT_SEGMENTSIZEINFO LegacyInfo; // Legacy segment size info.
    _Out_ ULONGLONG LocalMemory; // The size of local memory.
    _Out_ ULONGLONG NonLocalMemory; // The size of non-local memory.
    _Out_ ULONGLONG NonBudgetMemory; // The size of non-budget memory.
} D3DKMT_SEGMENTGROUPSIZEINFO;

// A structure that holds the support status.
typedef struct _D3DKMT_MPO3DDI_SUPPORT
{
    BOOL Supported; // Indicates whether support exists.
} D3DKMT_MPO3DDI_SUPPORT;

typedef struct _D3DKMT_HWDRM_SUPPORT
{
    BOOLEAN Supported;
} D3DKMT_HWDRM_SUPPORT;

typedef struct _D3DKMT_MPOKERNELCAPS_SUPPORT
{
    BOOL Supported;
} D3DKMT_MPOKERNELCAPS_SUPPORT;

typedef struct _D3DKMT_MULTIPLANEOVERLAY_STRETCH_SUPPORT
{
    UINT32 VidPnSourceId;
    BOOL Update;
    BOOL Supported;
} D3DKMT_MULTIPLANEOVERLAY_STRETCH_SUPPORT;

typedef struct _D3DKMT_GET_DEVICE_VIDPN_OWNERSHIP_INFO
{
    _In_ D3DKMT_HANDLE hDevice; // Indentifies the device
    _Out_ BOOLEAN bFailedDwmAcquireVidPn; // True if Dwm Acquire VidPn failed due to another Dwm device having ownership
} D3DKMT_GET_DEVICE_VIDPN_OWNERSHIP_INFO;

#ifdef USE_D3DKMT_SINGLE_HEADER
typedef struct _D3DDDI_QUERYREGISTRY_FLAGS
{
    union
    {
        struct
        {
            UINT32 TranslatePath : 1;
            UINT32 MutableValue : 1;
            UINT32 Reserved : 30;
        };
        UINT32 Value;
    };
} D3DDDI_QUERYREGISTRY_FLAGS;

typedef enum _D3DDDI_QUERYREGISTRY_TYPE
{
    D3DDDI_QUERYREGISTRY_SERVICEKEY = 0, // HKLM\System\CurrentControlSet\Services\nvlddmkm
    D3DDDI_QUERYREGISTRY_ADAPTERKEY = 1, // HKLM\System\CurrentControlSet\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}\0000
    D3DDDI_QUERYREGISTRY_DRIVERSTOREPATH = 2,
    D3DDDI_QUERYREGISTRY_DRIVERIMAGEPATH = 3, // REDSTONE5
    D3DDDI_QUERYREGISTRY_MAX,
} D3DDDI_QUERYREGISTRY_TYPE;

typedef enum _D3DDDI_QUERYREGISTRY_STATUS
{
    D3DDDI_QUERYREGISTRY_STATUS_SUCCESS = 0,
    D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW = 1,
    D3DDDI_QUERYREGISTRY_STATUS_FAIL = 2,
    D3DDDI_QUERYREGISTRY_STATUS_MAX,
} D3DDDI_QUERYREGISTRY_STATUS;

// Contains information about the query registry.
// PrivateDriverSize must be sizeof(D3DDDI_QUERYREGISTRY_INFO) + (size of the the key value in bytes)
typedef struct _D3DDDI_QUERYREGISTRY_INFO
{
    _In_ D3DDDI_QUERYREGISTRY_TYPE QueryType;
    _In_ D3DDDI_QUERYREGISTRY_FLAGS QueryFlags;
    _In_ WCHAR ValueName[MAX_PATH]; // The name of the registry value.
    _In_ ULONG ValueType; // REG_XX types (https://msdn.microsoft.com/en-us/library/windows/desktop/ms724884.aspx)
    _In_ ULONG PhysicalAdapterIndex;   // The physical adapter index in a LDA chain.
    _Out_ ULONG OutputValueSize;// Number of bytes written to the output value or required in case of D3DDDI_QUERYREGISTRY_STATUS_BUFFER_OVERFLOW.
    _Out_ D3DDDI_QUERYREGISTRY_STATUS Status;
    union
    {
        _Out_ ULONG OutputDword;
        _Out_ UINT64 OutputQword;
        _Out_ WCHAR OutputString[1];
        _Out_ BYTE OutputBinary[1];
    };
} D3DDDI_QUERYREGISTRY_INFO;
#endif

// Contains the kernel mode driver version.
typedef struct _D3DKMT_KMD_DRIVER_VERSION
{
    LARGE_INTEGER DriverVersion; // The driver version.
} D3DKMT_KMD_DRIVER_VERSION;

typedef struct _D3DKMT_BLOCKLIST_INFO
{
    UINT32 Size; // The size of the block list.
    WCHAR BlockList[1]; // The block list.
} D3DKMT_BLOCKLIST_INFO;

// Used to query for a unique guid.
typedef struct _D3DKMT_QUERY_ADAPTER_UNIQUE_GUID
{
    WCHAR AdapterUniqueGUID[40];
} D3DKMT_QUERY_ADAPTER_UNIQUE_GUID;

// Represents performance data collected per engine from an adapter on an interval basis.
typedef struct _D3DKMT_NODE_PERFDATA
{
    _In_ UINT32 NodeOrdinal; // Node ordinal of the requested engine.
    _In_ UINT32 PhysicalAdapterIndex; // The physical adapter index in a LDA chain.
    _Out_ ULONGLONG Frequency; // Clock frequency of the requested engine, represented in hertz.
    _Out_ ULONGLONG MaxFrequency; // The max frequency the engine can normally reach in hertz while not overclocked.
    _Out_ ULONGLONG MaxFrequencyOC; // The max frequency the engine can reach with it�s current overclock in hertz.
    _Out_ ULONG Voltage; // Voltage of the engine in milli volts mV
    _Out_ ULONG VoltageMax; // The max voltage of the engine in milli volts while not overclocked.
    _Out_ ULONG VoltageMaxOC; // The max voltage of the engine while overclocked in milli volts.
    _Out_ ULONGLONG MaxTransitionLatency; // Max transition latency to change the frequency in 100 nanoseconds // REDSTONE5
} D3DKMT_NODE_PERFDATA;

// Represents performance data collected per adapter on an interval basis.
typedef struct _D3DKMT_ADAPTER_PERFDATA
{
    _In_ UINT32 PhysicalAdapterIndex; // The physical adapter index in a LDA chain.
    _Out_ ULONGLONG MemoryFrequency; // Clock frequency of the memory in hertz
    _Out_ ULONGLONG MaxMemoryFrequency; // Max clock frequency of the memory while not overclocked, represented in hertz.
    _Out_ ULONGLONG MaxMemoryFrequencyOC; // Clock frequency of the memory while overclocked in hertz.
    _Out_ ULONGLONG MemoryBandwidth; // Amount of memory transferred in bytes
    _Out_ ULONGLONG PCIEBandwidth; // Amount of memory transferred over PCI-E in bytes
    _Out_ ULONG FanRPM; // Fan rpm
    _Out_ ULONG Power; // Power draw of the adapter in tenths of a percentage
    _Out_ ULONG Temperature; // Temperature in deci-Celsius 1 = 0.1C
    _Out_ UCHAR PowerStateOverride; // Overrides dxgkrnls power view of linked adapters.
} D3DKMT_ADAPTER_PERFDATA;

// Represents data capabilities that are static and queried once per GPU during initialization.
typedef struct _D3DKMT_ADAPTER_PERFDATACAPS
{
    _In_ UINT32 PhysicalAdapterIndex; // The physical adapter index in a LDA chain.
    _Out_ ULONGLONG MaxMemoryBandwidth; // Max memory bandwidth in bytes for 1 second
    _Out_ ULONGLONG MaxPCIEBandwidth; // Max pcie bandwidth in bytes for 1 second
    _Out_ ULONG MaxFanRPM; // Max fan rpm
    _Out_ ULONG TemperatureMax; // Max temperature before damage levels
    _Out_ ULONG TemperatureWarning; // The temperature level where throttling begins.
} D3DKMT_ADAPTER_PERFDATACAPS;

#define DXGK_MAX_GPUVERSION_NAME_LENGTH 32

// Used to collect the bios version and gpu architecture name once during GPU initialization.
typedef struct _D3DKMT_GPUVERSION
{
    _In_ UINT32 PhysicalAdapterIndex; // The physical adapter index in a LDA chain.
    _Out_ WCHAR BiosVersion[DXGK_MAX_GPUVERSION_NAME_LENGTH]; // The current bios of the adapter.
    _Out_ WCHAR GpuArchitecture[DXGK_MAX_GPUVERSION_NAME_LENGTH]; // The gpu architecture of the adapter.
} D3DKMT_GPUVERSION;

// Describes the kernel mode display driver.
typedef struct _D3DKMT_DRIVER_DESCRIPTION
{
    WCHAR DriverDescription[4096]; // out: Pointer to a string of characters that represent the driver description.
} D3DKMT_DRIVER_DESCRIPTION;

typedef struct _D3DKMT_QUERY_SCANOUT_CAPS
{
    ULONG VidPnSourceId;
    UINT Caps;
} D3DKMT_QUERY_SCANOUT_CAPS;

typedef struct _D3DKMT_WDDM_2_7_CAPS
{
    union
    {
        struct
        {
            UINT32 HwSchSupported : 1;
            UINT32 HwSchEnabled : 1;
            UINT32 HwSchEnabledByDefault : 1;
            UINT32 ReseIndependentVidPnVSyncControlrved : 1;
            UINT32 Reserved : 28;
        };
        UINT32 Value;
    };
} D3DKMT_WDDM_2_7_CAPS;

// Describes the mapping of the given name of a device to a graphics adapter handle and monitor output.
typedef struct _D3DKMT_OPENADAPTERFROMDEVICENAME
{
    _In_ PWSTR DeviceName; // A Null-terminated string that contains the name of the device from which to open an adapter instance.
    _Out_ D3DKMT_HANDLE AdapterHandle; // A handle to the graphics adapter for the device that DeviceName specifies.
    _Out_ LUID AdapterLuid; // The locally unique identifier (LUID) of the graphics adapter for the device that DeviceName specifies.
} D3DKMT_OPENADAPTERFROMDEVICENAME;

// Describes the mapping of the given name of a GDI device to a graphics adapter handle and monitor output.
typedef struct _D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME
{
    _In_ WCHAR DeviceName[32]; // A Unicode string that contains the name of the GDI device from which to open an adapter instance.
    _Out_ D3DKMT_HANDLE AdapterHandle; // A handle to the graphics adapter for the GDI device that DeviceName specifies.
    _Out_ LUID AdapterLuid; // The locally unique identifier (LUID) of the graphics adapter for the GDI device that DeviceName specifies. 
    _Out_ ULONG VidPnSourceId; // D3DDDI_VIDEO_PRESENT_SOURCE_ID // The zero-based identification number of the video present source.
} D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME;

// Describes the mapping of a device context handle (HDC) to a graphics adapter handle and monitor output.
typedef struct _D3DKMT_OPENADAPTERFROMHDC
{
    _In_ HDC hDc; // The HDC for the graphics adapter and monitor output that are retrieved.
    _Out_ D3DKMT_HANDLE AdapterHandle; // A handle to the graphics adapter for the HDC that hDc specifies.
    _Out_ LUID AdapterLuid; // The locally unique identifier (LUID) of the graphics adapter for the HDC that hDc specifies.
    _Out_ ULONG VidPnSourceId; // D3DDDI_VIDEO_PRESENT_SOURCE_ID // The zero-based identification number of the video present source.
} D3DKMT_OPENADAPTERFROMHDC;

// Describes the mapping of the given locally unique identifier (LUID) of a device to a graphics adapter handle.
typedef struct _D3DKMT_OPENADAPTERFROMLUID
{
    _In_ LUID AdapterLuid;
    _Out_ D3DKMT_HANDLE AdapterHandle;
} D3DKMT_OPENADAPTERFROMLUID;

// Supplies configuration information about a graphics adapter.
typedef struct _D3DKMT_ADAPTERINFO
{
    D3DKMT_HANDLE AdapterHandle; // A handle to the adapter.
    LUID AdapterLuid; // A LUID that serves as an identifier for the adapter.
    ULONG NumOfSources; // The number of video present sources supported by the adapter.
    BOOL bPresentMoveRegionsPreferred; // If TRUE, the adapter prefers move regions.
} D3DKMT_ADAPTERINFO;

#define MAX_ENUM_ADAPTERS 16

// Supplies information for enumerating all graphics adapters on the system.
typedef struct _D3DKMT_ENUMADAPTERS
{
    _In_ ULONG NumAdapters; // The number of graphics adapters.
    _Out_ D3DKMT_ADAPTERINFO Adapters[MAX_ENUM_ADAPTERS]; // An array of D3DKMT_ADAPTERINFO structures that supply configuration information for each adapter.
} D3DKMT_ENUMADAPTERS;

// Supplies information for enumerating all graphics adapters on the system.
typedef struct _D3DKMT_ENUMADAPTERS2
{
    _Inout_ ULONG NumAdapters; // On input, the count of the pAdapters array buffer. On output, the number of adapters enumerated.
    _Out_ D3DKMT_ADAPTERINFO* Adapters; // Array of enumerated adapters containing NumAdapters elements.
} D3DKMT_ENUMADAPTERS2;

typedef union _D3DKMT_ENUMADAPTERS_FILTER
{
    ULONG64 IncludeComputeOnly : 1;
    ULONG64 IncludeDisplayOnly : 1;
    ULONG64 Reserved : 62;
} D3DKMT_ENUMADAPTERS_FILTER;

typedef struct _D3DKMT_ENUMADAPTERS3
{
    _In_ D3DKMT_ENUMADAPTERS_FILTER Filter;
    _Inout_ ULONG NumAdapters;
    _Out_ D3DKMT_ADAPTERINFO* Adapters;
} D3DKMT_ENUMADAPTERS3;

// The D3DKMT_CLOSEADAPTER structure specifies the graphics adapter to close.
typedef struct _D3DKMT_CLOSEADAPTER
{
    _In_ D3DKMT_HANDLE AdapterHandle; // A handle to the graphics adapter to close.
} D3DKMT_CLOSEADAPTER;

typedef struct _D3DKMT_QUERYADAPTERINFO
{
    _In_ D3DKMT_HANDLE AdapterHandle;
    _In_ KMTQUERYADAPTERINFOTYPE Type;
    _Inout_bytecount_(PrivateDriverDataSize) PVOID PrivateDriverData;
    _Out_ UINT32 PrivateDriverDataSize;
} D3DKMT_QUERYADAPTERINFO;

typedef enum _D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT
{
    D3DKMT_PreemptionAttempt = 0,
    D3DKMT_PreemptionAttemptSuccess = 1,
    D3DKMT_PreemptionAttemptMissNoCommand = 2,
    D3DKMT_PreemptionAttemptMissNotEnabled = 3,
    D3DKMT_PreemptionAttemptMissNextFence = 4,
    D3DKMT_PreemptionAttemptMissPagingCommand = 5,
    D3DKMT_PreemptionAttemptMissSplittedCommand = 6,
    D3DKMT_PreemptionAttemptMissFenceCommand= 7,
    D3DKMT_PreemptionAttemptMissRenderPendingFlip = 8,
    D3DKMT_PreemptionAttemptMissNotMakingProgress = 9,
    D3DKMT_PreemptionAttemptMissLessPriority = 10,
    D3DKMT_PreemptionAttemptMissRemainingQuantum = 11,
    D3DKMT_PreemptionAttemptMissRemainingPreemptionQuantum = 12,
    D3DKMT_PreemptionAttemptMissAlreadyPreempting = 13,
    D3DKMT_PreemptionAttemptMissGlobalBlock = 14,
    D3DKMT_PreemptionAttemptMissAlreadyRunning = 15,
    D3DKMT_PreemptionAttemptStatisticsMax
} D3DKMT_QUERYRESULT_PREEMPTION_ATTEMPT_RESULT;

typedef enum _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE
{
    D3DKMT_ClientRenderBuffer = 0,
    D3DKMT_ClientPagingBuffer = 1,
    D3DKMT_SystemPagingBuffer = 2,
    D3DKMT_SystemPreemptionBuffer = 3,
    D3DKMT_DmaPacketTypeMax
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE;

typedef enum _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE
{
    D3DKMT_RenderCommandBuffer = 0,
    D3DKMT_DeferredCommandBuffer = 1,
    D3DKMT_SystemCommandBuffer = 2,
    D3DKMT_MmIoFlipCommandBuffer = 3,
    D3DKMT_WaitCommandBuffer = 4,
    D3DKMT_SignalCommandBuffer = 5,
    D3DKMT_DeviceCommandBuffer = 6,
    D3DKMT_SoftwareCommandBuffer = 7,
    D3DKMT_QueuePacketTypeMax
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE;

typedef enum _D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS
{
    D3DKMT_AllocationPriorityClassMinimum = 0,
    D3DKMT_AllocationPriorityClassLow = 1,
    D3DKMT_AllocationPriorityClassNormal = 2,
    D3DKMT_AllocationPriorityClassHigh = 3,
    D3DKMT_AllocationPriorityClassMaximum = 4,
    D3DKMT_MaxAllocationPriorityClass
} D3DKMT_QUERYSTATISTICS_ALLOCATION_PRIORITY_CLASS;

#define D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX 5

typedef struct _D3DKMT_QUERYSTATISTICS_COUNTER
{
    ULONG Count;
    ULONGLONG Bytes;
} D3DKMT_QUERYSTATISTICS_COUNTER;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION
{
    ULONG PacketSubmited;
    ULONG PacketCompleted;
    ULONG PacketPreempted;
    ULONG PacketFaulted;
} D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION
{
    ULONG PacketSubmited;
    ULONG PacketCompleted;
} D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_QUEUE_PACKET_TYPE_INFORMATION QueuePacket[D3DKMT_QueuePacketTypeMax];
    D3DKMT_QUERYSTATISTICS_DMA_PACKET_TYPE_INFORMATION DmaPacket[D3DKMT_DmaPacketTypeMax];
} D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION
{
    ULONG PreemptionCounter[D3DKMT_PreemptionAttemptStatisticsMax];
} D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION
{
    LARGE_INTEGER RunningTime; // 100ns
    ULONG ContextSwitch;
    D3DKMT_QUERYSTATISTICS_PREEMPTION_INFORMATION PreemptionStatistics;
    D3DKMT_QUERYSTATISTICS_PACKET_INFORMATION PacketStatistics;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_NODE_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION GlobalInformation; // global
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION SystemInformation; // system thread
    //ULONG NodeId; // Win10
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_NODE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION
{
    ULONG Frame;
    ULONG CancelledFrame;
    ULONG QueuedPresent;
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION
{
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION GlobalInformation; // global
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION SystemInformation; // system thread
    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION;

typedef struct _D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER
{
    ULONG NbCall;
    ULONG NbAllocationsReferenced;
    ULONG MaxNbAllocationsReferenced;
    ULONG NbNULLReference;
    ULONG NbWriteReference;
    ULONG NbRenamedAllocationsReferenced;
    ULONG NbIterationSearchingRenamedAllocation;
    ULONG NbLockedAllocationReferenced;
    ULONG NbAllocationWithValidPrepatchingInfoReferenced;
    ULONG NbAllocationWithInvalidPrepatchingInfoReferenced;
    ULONG NbDMABufferSuccessfullyPrePatched;
    ULONG NbPrimariesReferencesOverflow;
    ULONG NbAllocationWithNonPreferredResources;
    ULONG NbAllocationInsertedInMigrationTable;
} D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER;

typedef struct _D3DKMT_QUERYSTATSTICS_RENAMING
{
    ULONG NbAllocationsRenamed;
    ULONG NbAllocationsShrinked;
    ULONG NbRenamedBuffer;
    ULONG MaxRenamingListLength;
    ULONG NbFailuresDueToRenamingLimit;
    ULONG NbFailuresDueToCreateAllocation;
    ULONG NbFailuresDueToOpenAllocation;
    ULONG NbFailuresDueToLowResource;
    ULONG NbFailuresDueToNonRetiredLimit;
} D3DKMT_QUERYSTATSTICS_RENAMING;

typedef struct _D3DKMT_QUERYSTATSTICS_PREPRATION
{
    ULONG BroadcastStall;
    ULONG NbDMAPrepared;
    ULONG NbDMAPreparedLongPath;
    ULONG ImmediateHighestPreparationPass;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsTrimmed;
} D3DKMT_QUERYSTATSTICS_PREPRATION;

typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_FAULT
{
    D3DKMT_QUERYSTATISTICS_COUNTER Faults;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsFirstTimeAccess;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsReclaimed;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsMigration;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsIncorrectResource;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsLostContent;
    D3DKMT_QUERYSTATISTICS_COUNTER FaultsEvicted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsMEM_RESET;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocationsUnresetFail;
    ULONG AllocationsUnresetSuccessRead;
    ULONG AllocationsUnresetFailRead;

    D3DKMT_QUERYSTATISTICS_COUNTER Evictions;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPreparation;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToLock;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToClose;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToPurge;
    D3DKMT_QUERYSTATISTICS_COUNTER EvictionsDueToSuspendCPUAccess;
} D3DKMT_QUERYSTATSTICS_PAGING_FAULT;

typedef struct _D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER
{
    ULONGLONG BytesFilled;
    ULONGLONG BytesDiscarded;
    ULONGLONG BytesMappedIntoAperture;
    ULONGLONG BytesUnmappedFromAperture;
    ULONGLONG BytesTransferredFromMdlToMemory;
    ULONGLONG BytesTransferredFromMemoryToMdl;
    ULONGLONG BytesTransferredFromApertureToMemory;
    ULONGLONG BytesTransferredFromMemoryToAperture;
} D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER;

typedef struct _D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE
{
    ULONG NbRangesAcquired;
    ULONG NbRangesReleased;
} D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE;

typedef struct _D3DKMT_QUERYSTATSTICS_LOCKS
{
    ULONG NbLocks;
    ULONG NbLocksWaitFlag;
    ULONG NbLocksDiscardFlag;
    ULONG NbLocksNoOverwrite;
    ULONG NbLocksNoReadSync;
    ULONG NbLocksLinearization;
    ULONG NbComplexLocks;
} D3DKMT_QUERYSTATSTICS_LOCKS;

typedef struct _D3DKMT_QUERYSTATSTICS_ALLOCATIONS
{
    D3DKMT_QUERYSTATISTICS_COUNTER Created;
    D3DKMT_QUERYSTATISTICS_COUNTER Destroyed;
    D3DKMT_QUERYSTATISTICS_COUNTER Opened;
    D3DKMT_QUERYSTATISTICS_COUNTER Closed;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedSuccess;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedFail;
    D3DKMT_QUERYSTATISTICS_COUNTER MigratedAbandoned;
} D3DKMT_QUERYSTATSTICS_ALLOCATIONS;

typedef struct _D3DKMT_QUERYSTATSTICS_TERMINATIONS
{
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER TerminatedNonShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedShared;
    D3DKMT_QUERYSTATISTICS_COUNTER DestroyedNonShared;
} D3DKMT_QUERYSTATSTICS_TERMINATIONS;

typedef struct _D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    ULONG VSyncEnabled;
    ULONG TdrDetectedCount;

    LONGLONG ZeroLengthDmaBuffers;
    ULONGLONG RestartedPeriod;

    D3DKMT_QUERYSTATSTICS_REFERENCE_DMA_BUFFER ReferenceDmaBuffer;
    D3DKMT_QUERYSTATSTICS_RENAMING Renaming;
    D3DKMT_QUERYSTATSTICS_PREPRATION Preparation;
    D3DKMT_QUERYSTATSTICS_PAGING_FAULT PagingFault;
    D3DKMT_QUERYSTATSTICS_PAGING_TRANSFER PagingTransfer;
    D3DKMT_QUERYSTATSTICS_SWIZZLING_RANGE SwizzlingRange;
    D3DKMT_QUERYSTATSTICS_LOCKS Locks;
    D3DKMT_QUERYSTATSTICS_ALLOCATIONS Allocations;
    D3DKMT_QUERYSTATSTICS_TERMINATIONS Terminations;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY
{
    ULONGLONG BytesAllocated;
    ULONGLONG BytesReserved;
    ULONG SmallAllocationBlocks;
    ULONG LargeAllocationBlocks;
    ULONGLONG WriteCombinedBytesAllocated;
    ULONGLONG WriteCombinedBytesReserved;
    ULONGLONG CachedBytesAllocated;
    ULONGLONG CachedBytesReserved;
    ULONGLONG SectionBytesAllocated;
    ULONGLONG SectionBytesReserved;
} D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION
{
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    D3DKMT_QUERYSTATISTICS_SYSTEM_MEMORY SystemMemory;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_DMA_BUFFER
{
    D3DKMT_QUERYSTATISTICS_COUNTER Size;
    ULONG AllocationListBytes;
    ULONG PatchLocationListBytes;
} D3DKMT_QUERYSTATISTICS_DMA_BUFFER;

typedef struct _D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA
{
    ULONG64 TotalBytesEvictedFromProcess;
    ULONG64 BytesBySegmentPreference[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
} D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA;

typedef struct _D3DKMT_QUERYSTATISTICS_POLICY
{
    ULONGLONG PreferApertureForRead[D3DKMT_MaxAllocationPriorityClass];
    ULONGLONG PreferAperture[D3DKMT_MaxAllocationPriorityClass];
    ULONGLONG MemResetOnPaging;
    ULONGLONG RemovePagesFromWorkingSetOnPaging;
    ULONGLONG MigrationEnabled;
} D3DKMT_QUERYSTATISTICS_POLICY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION
{
    ULONG NbSegments;
    ULONG NodeCount;
    ULONG VidPnSourceCount;

    ULONG VirtualMemoryUsage;

    D3DKMT_QUERYSTATISTICS_DMA_BUFFER DmaBuffer;
    D3DKMT_QUERYSTATISTICS_COMMITMENT_DATA CommitmentData;
    D3DKMT_QUERYSTATISTICS_POLICY _Policy;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_MEMORY
{
    ULONGLONG TotalBytesEvicted;
    ULONG AllocsCommitted;
    ULONG AllocsResident;
} D3DKMT_QUERYSTATISTICS_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1
{
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;

    D3DKMT_QUERYSTATISTICS_MEMORY Memory;

    ULONG Aperture; // boolean

    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];

    ULONG64 SystemMemoryEndAddress;
    struct
    {
        ULONG64 PreservedDuringStandby : 1;
        ULONG64 PreservedDuringHibernate : 1;
        ULONG64 PartiallyPreservedDuringHibernate : 1;
        ULONG64 Reserved : 61;
    } PowerFlags;

    ULONG64 Reserved[7];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1;

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION
{
    ULONGLONG CommitLimit;
    ULONGLONG BytesCommitted;
    ULONGLONG BytesResident;

    D3DKMT_QUERYSTATISTICS_MEMORY Memory;

    ULONG Aperture; // boolean

    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];

    ULONG64 SystemMemoryEndAddress;
    struct
    {
        ULONG64 PreservedDuringStandby : 1;
        ULONG64 PreservedDuringHibernate : 1;
        ULONG64 PartiallyPreservedDuringHibernate : 1;
        ULONG64 Reserved : 61;
    } PowerFlags;

    ULONG64 Reserved[6];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION;

typedef struct _D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY
{
    ULONG AllocsCommitted;
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInP[D3DKMT_QUERYSTATISTICS_SEGMENT_PREFERENCE_MAX];
    D3DKMT_QUERYSTATISTICS_COUNTER AllocsResidentInNonPreferred;
    ULONGLONG TotalBytesEvictedDueToPreparation;
} D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY
{
    ULONGLONG UseMRU;
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY;

typedef struct _D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION
{
    ULONGLONG BytesCommitted;
    ULONGLONG MaximumWorkingSet;
    ULONGLONG MinimumWorkingSet;

    ULONG NbReferencedAllocationEvictedInPeriod;

    D3DKMT_QUERYSTATISTICS_VIDEO_MEMORY VideoMemory;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_POLICY _Policy;

    ULONG64 Reserved[8];
} D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION;

typedef enum _D3DKMT_QUERYSTATISTICS_TYPE
{
    D3DKMT_QUERYSTATISTICS_ADAPTER,
    D3DKMT_QUERYSTATISTICS_PROCESS,
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER,
    D3DKMT_QUERYSTATISTICS_SEGMENT,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT,
    D3DKMT_QUERYSTATISTICS_NODE,
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE,
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE,
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE,
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP,
    D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER
} D3DKMT_QUERYSTATISTICS_TYPE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT
{
    ULONG SegmentId;
} D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_NODE
{
    ULONG NodeId;
} D3DKMT_QUERYSTATISTICS_QUERY_NODE;

typedef struct _D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE
{
    ULONG VidPnSourceId;
} D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE;

typedef union _D3DKMT_QUERYSTATISTICS_RESULT
{
    D3DKMT_QUERYSTATISTICS_ADAPTER_INFORMATION AdapterInformation;
    // D3DKMT_QUERYSTATISTICS_PHYSICAL_ADAPTER_INFORMATION PhysAdapterInformation;
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1 SegmentInformationV1; // WIN7
    D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION SegmentInformation; // WIN8
    D3DKMT_QUERYSTATISTICS_NODE_INFORMATION NodeInformation;
    D3DKMT_QUERYSTATISTICS_VIDPNSOURCE_INFORMATION VidPnSourceInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_INFORMATION ProcessInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION ProcessAdapterInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_INFORMATION ProcessSegmentInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_NODE_INFORMATION ProcessNodeInformation;
    D3DKMT_QUERYSTATISTICS_PROCESS_VIDPNSOURCE_INFORMATION ProcessVidPnSourceInformation;
    // D3DKMT_QUERYSTATISTICS_PROCESS_SEGMENT_GROUP_INFORMATION ProcessSegmentGroupInformation;
} D3DKMT_QUERYSTATISTICS_RESULT;

typedef struct _D3DKMT_QUERYSTATISTICS
{
    _In_ D3DKMT_QUERYSTATISTICS_TYPE Type;
    _In_ LUID AdapterLuid;
    _In_opt_ HANDLE ProcessHandle;
    _Out_ D3DKMT_QUERYSTATISTICS_RESULT QueryResult;

    union
    {
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QuerySegment;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_SEGMENT QueryProcessSegment;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryNode;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_NODE QueryProcessNode;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryVidPnSource;
        _In_ D3DKMT_QUERYSTATISTICS_QUERY_VIDPNSOURCE QueryProcessVidPnSource;
    };
} D3DKMT_QUERYSTATISTICS;

typedef enum _D3DKMT_MEMORY_SEGMENT_GROUP
{
    D3DKMT_MEMORY_SEGMENT_GROUP_LOCAL = 0,
    D3DKMT_MEMORY_SEGMENT_GROUP_NON_LOCAL = 1
} D3DKMT_MEMORY_SEGMENT_GROUP;

typedef struct _D3DKMT_QUERYVIDEOMEMORYINFO
{
    _In_opt_ HANDLE ProcessHandle; // A handle to a process. If NULL, the current process is used. The process handle must be opened with PROCESS_QUERY_INFORMATION privileges.
    _In_ D3DKMT_HANDLE AdapterHandle; // The adapter to query for this process
    _In_ D3DKMT_MEMORY_SEGMENT_GROUP MemorySegmentGroup; // The memory segment group to query.
    _Out_ UINT64 Budget; // Total memory the application may use
    _Out_ UINT64 CurrentUsage; // Current memory usage of the device
    _Out_ UINT64 CurrentReservation; // Current reservation of the device
    _Out_ UINT64 AvailableForReservation; // Total that the device may reserve
    _In_ UINT32 PhysicalAdapterIndex; // Zero based physical adapter index in the LDA configuration.
} D3DKMT_QUERYVIDEOMEMORYINFO;

typedef enum _D3DKMT_ESCAPETYPE
{
    D3DKMT_ESCAPE_DRIVERPRIVATE = 0,
    D3DKMT_ESCAPE_VIDMM = 1, // D3DKMT_VIDMM_ESCAPE
    D3DKMT_ESCAPE_TDRDBGCTRL = 2, // D3DKMT_TDRDBGCTRLTYPE
    D3DKMT_ESCAPE_VIDSCH = 3, // D3DKMT_VIDSCH_ESCAPE
    D3DKMT_ESCAPE_DEVICE = 4, // D3DKMT_DEVICE_ESCAPE
    D3DKMT_ESCAPE_DMM = 5, // D3DKMT_DMM_ESCAPE
    D3DKMT_ESCAPE_DEBUG_SNAPSHOT = 6, // D3DKMT_DEBUG_SNAPSHOT_ESCAPE
    // unused (7 was previously used to set driver update in-progress status, D3DKMT_ESCAPE_SETDRIVERUPDATESTATUS)
    D3DKMT_ESCAPE_DRT_TEST = 8,
    D3DKMT_ESCAPE_DIAGNOSTICS = 9, // since WIN8
    D3DKMT_ESCAPE_OUTPUTDUPL_SNAPSHOT = 10,
    D3DKMT_ESCAPE_OUTPUTDUPL_DIAGNOSTICS = 11,
    D3DKMT_ESCAPE_BDD_PNP = 12,
    D3DKMT_ESCAPE_BDD_FALLBACK = 13,
    D3DKMT_ESCAPE_ACTIVATE_SPECIFIC_DIAG = 14, // D3DKMT_ACTIVATE_SPECIFIC_DIAG_ESCAPE
    D3DKMT_ESCAPE_MODES_PRUNED_OUT = 15,
    D3DKMT_ESCAPE_WHQL_INFO = 16, // UINT32 ??
    D3DKMT_ESCAPE_BRIGHTNESS = 17,
    D3DKMT_ESCAPE_EDID_CACHE = 18,  // UINT32 ??
    D3DKMT_ESCAPE_GENERIC_ADAPTER_DIAG_INFO = 19,
    D3DKMT_ESCAPE_MIRACAST_DISPLAY_REQUEST = 20, // since WDDM1_3
    D3DKMT_ESCAPE_HISTORY_BUFFER_STATUS = 21,
    // 22 can be reused for future needs as it was never exposed for external purposes
    D3DKMT_ESCAPE_MIRACAST_ADAPTER_DIAG_INFO = 23,
    D3DKMT_ESCAPE_FORCE_BDDFALLBACK_HEADLESS = 24, // since WDDM2_0
    D3DKMT_ESCAPE_REQUEST_MACHINE_CRASH = 25, // D3DKMT_REQUEST_MACHINE_CRASH_ESCAPE
    D3DKMT_ESCAPE_HMD_GET_EDID_BASE_BLOCK = 26,
    D3DKMT_ESCAPE_SOFTGPU_ENABLE_DISABLE_HMD = 27,
    D3DKMT_ESCAPE_PROCESS_VERIFIER_OPTION = 28,
    D3DKMT_ESCAPE_ADAPTER_VERIFIER_OPTION = 29,
    D3DKMT_ESCAPE_IDD_REQUEST = 30, // since WDDM2_1
    D3DKMT_ESCAPE_DOD_SET_DIRTYRECT_MODE = 31,
    D3DKMT_ESCAPE_LOG_CODEPOINT_PACKET = 32,
    D3DKMT_ESCAPE_LOG_USERMODE_DAIG_PACKET = 33, // since WDDM2_2
    D3DKMT_ESCAPE_GET_EXTERNAL_DIAGNOSTICS = 34,
    // unused (35 previously was D3DKMT_ESCAPE_GET_PREFERRED_MODE)
    D3DKMT_ESCAPE_GET_DISPLAY_CONFIGURATIONS = 36, // since WDDM2_3
    D3DKMT_ESCAPE_QUERY_IOMMU_STATUS = 37, // since WDDM2_4
    D3DKMT_ESCAPE_CCD_DATABASE = 38, // since WDDM2_6

    D3DKMT_ESCAPE_WIN32K_START = 1024,
    D3DKMT_ESCAPE_WIN32K_HIP_DEVICE_INFO = 1024,
    D3DKMT_ESCAPE_WIN32K_QUERY_CD_ROTATION_BLOCK = 1025,
    D3DKMT_ESCAPE_WIN32K_DPI_INFO = 1026, // Use hContext for the desired hdev // since WDDM1_3
    D3DKMT_ESCAPE_WIN32K_PRESENTER_VIEW_INFO = 1027,
    D3DKMT_ESCAPE_WIN32K_SYSTEM_DPI = 1028,
    D3DKMT_ESCAPE_WIN32K_BDD_FALLBACK = 1029, // since WDDM2_0
    D3DKMT_ESCAPE_WIN32K_DDA_TEST_CTL = 1030,
    D3DKMT_ESCAPE_WIN32K_USER_DETECTED_BLACK_SCREEN = 1031,
    D3DKMT_ESCAPE_WIN32K_HMD_ENUM = 1032,
    D3DKMT_ESCAPE_WIN32K_HMD_CONTROL = 1033,
    D3DKMT_ESCAPE_WIN32K_LPMDISPLAY_CONTROL = 1034,
} D3DKMT_ESCAPETYPE;

typedef enum _D3DKMT_VIDMMESCAPETYPE
{
    D3DKMT_VIDMMESCAPETYPE_SETFAULT = 0,
    D3DKMT_VIDMMESCAPETYPE_RUN_COHERENCY_TEST = 1,
    D3DKMT_VIDMMESCAPETYPE_RUN_UNMAP_TO_DUMMY_PAGE_TEST = 2,
    D3DKMT_VIDMMESCAPETYPE_APERTURE_CORRUPTION_CHECK = 3,
    D3DKMT_VIDMMESCAPETYPE_SUSPEND_CPU_ACCESS_TEST = 4,
    D3DKMT_VIDMMESCAPETYPE_EVICT = 5,
    D3DKMT_VIDMMESCAPETYPE_EVICT_BY_NT_HANDLE = 6,
    D3DKMT_VIDMMESCAPETYPE_GET_VAD_INFO = 7,
    D3DKMT_VIDMMESCAPETYPE_SET_BUDGET = 8,
    D3DKMT_VIDMMESCAPETYPE_SUSPEND_PROCESS = 9,
    D3DKMT_VIDMMESCAPETYPE_RESUME_PROCESS = 10,
    D3DKMT_VIDMMESCAPETYPE_GET_BUDGET = 11,
    D3DKMT_VIDMMESCAPETYPE_SET_TRIM_INTERVALS = 12,
    D3DKMT_VIDMMESCAPETYPE_EVICT_BY_CRITERIA = 13,
    D3DKMT_VIDMMESCAPETYPE_WAKE = 14,
    D3DKMT_VIDMMESCAPETYPE_DEFRAG = 15,
} D3DKMT_VIDMMESCAPETYPE;

typedef struct _D3DKMT_VAD_DESC
{
    _In_ UINT32 VadIndex; // 0xFFFFFFFF to use the VAD address
    _In_ UINT64 VadAddress;
    _Out_ UINT32 NumMappedRanges;
    _Out_ UINT32 VadType; // 0 - reserved, 1 - Mapped
    _Out_ UINT64 StartAddress;
    _Out_ UINT64 EndAddress;
} D3DKMT_VAD_DESC;

typedef struct _D3DKMT_VA_RANGE_DESC
{
    _In_ UINT64 VadAddress;
    _In_ UINT32 VaRangeIndex;
    _In_ UINT32 PhysicalAdapterIndex;
    _Out_ UINT64 StartAddress;
    _Out_ UINT64 EndAddress;
    _Out_ UINT64 DriverProtection;
    _Out_ UINT32 OwnerType; // VIDMM_VAD_OWNER_TYPE
    _Out_ UINT64 pOwner;
    _Out_ UINT64 OwnerOffset;
    _Out_ UINT32 Protection; // D3DDDIGPUVIRTUALADDRESS_PROTECTION_TYPE
} D3DKMT_VA_RANGE_DESC;

typedef struct _D3DKMT_PAGE_TABLE_LEVEL_DESC
{
    UINT32 IndexBitCount;
    UINT64 IndexMask;
    UINT64 IndexShift;
    UINT64 LowerLevelsMask;
    UINT64 EntryCoverageInPages;
} D3DKMT_PAGE_TABLE_LEVEL_DESC;

#define DXGK_MAX_PAGE_TABLE_LEVEL_COUNT 6
#define DXGK_MIN_PAGE_TABLE_LEVEL_COUNT 2

typedef struct _DXGK_ESCAPE_GPUMMUCAPS
{
    BOOLEAN ReadOnlyMemorySupported;
    BOOLEAN NoExecuteMemorySupported;
    BOOLEAN ZeroInPteSupported;
    BOOLEAN CacheCoherentMemorySupported;
    BOOLEAN LargePageSupported;
    BOOLEAN DualPteSupported;
    BOOLEAN AllowNonAlignedLargePageAddress;
    UINT32 VirtualAddressBitCount;
    UINT32 PageTableLevelCount;
    D3DKMT_PAGE_TABLE_LEVEL_DESC PageTableLevelDesk[DXGK_MAX_PAGE_TABLE_LEVEL_COUNT];
} DXGK_ESCAPE_GPUMMUCAPS;

typedef struct _D3DKMT_GET_GPUMMU_CAPS
{
    UINT32 PhysicalAdapterIndex; // In
    DXGK_ESCAPE_GPUMMUCAPS GpuMmuCaps; // Out
} D3DKMT_GET_GPUMMU_CAPS;

#ifdef USE_D3DKMT_SINGLE_HEADER
typedef enum _DXGK_PTE_PAGE_SIZE
{
    DXGK_PTE_PAGE_TABLE_PAGE_4KB = 0,
    DXGK_PTE_PAGE_TABLE_PAGE_64KB = 1,
} DXGK_PTE_PAGE_SIZE;

typedef struct _DXGK_PTE
{
    union
    {
        struct
        {
            ULONGLONG Valid : 1;
            ULONGLONG Zero : 1;
            ULONGLONG CacheCoherent : 1;
            ULONGLONG ReadOnly : 1;
            ULONGLONG NoExecute : 1;
            ULONGLONG Segment : 5;
            ULONGLONG LargePage : 1;
            ULONGLONG PhysicalAdapterIndex : 6;
            ULONGLONG PageTablePageSize : 2; // DXGK_PTE_PAGE_SIZE
            ULONGLONG SystemReserved0 : 1;
            ULONGLONG Reserved : 44;
        };
        ULONGLONG Flags;
    };
    union
    {
        ULONGLONG PageAddress;      // High 52 bits of 64 bit physical address. Low 12 bits are zero.
        ULONGLONG PageTableAddress; // High 52 bits of 64 bit physical address. Low 12 bits are zero.
    };
} DXGK_PTE;
#endif

#define D3DKMT_GET_PTE_MAX 64

typedef struct _D3DKMT_GET_PTE
{
    _In_ UINT32 PhysicalAdapterIndex;
    _In_ UINT32 PageTableLevel;
    _In_ UINT32 PageTableIndex[DXGK_MAX_PAGE_TABLE_LEVEL_COUNT];
    _In_ BOOLEAN b64KBPte; // Valid only when dual PTEs are supported. Out - PT is 64KB.
    _In_ UINT32 NumPtes; // Number of PTEs to fill. Out - number of filled PTEs
    _Out_ DXGK_PTE Pte[D3DKMT_GET_PTE_MAX];
    _Out_ UINT32 NumValidEntries;
} D3DKMT_GET_PTE;

#define D3DKMT_MAX_SEGMENT_COUNT 32

typedef struct _D3DKMT_SEGMENT_CAPS
{
    UINT64 Size;
    UINT32 PageSize;
    ULONG SegmentId;
    BOOLEAN bAperture;
    BOOLEAN bReservedSysMem;
    D3DKMT_MEMORY_SEGMENT_GROUP BudgetGroup;
} D3DKMT_SEGMENT_CAPS;

typedef struct _D3DKMT_GET_SEGMENT_CAPS
{
    _In_ UINT32 PhysicalAdapterIndex;
    _Out_ UINT32 NumSegments;
    _Out_ D3DKMT_SEGMENT_CAPS SegmentCaps[D3DKMT_MAX_SEGMENT_COUNT];
} D3DKMT_GET_SEGMENT_CAPS;

typedef struct _D3DKMT_EVICTION_CRITERIA
{
    UINT64 MinimumSize;
    UINT64 MaximumSize;
    struct
    {
        union
        {
            struct
            {
                UINT32 Primary : 1;
                UINT32 Reserved : 31;
            } Flags;
            UINT32 Value;
        };
    };
} D3DKMT_EVICTION_CRITERIA;

typedef enum _D3DKMT_VAD_ESCAPE_COMMAND
{
    D3DKMT_VAD_ESCAPE_GETNUMVADS,
    D3DKMT_VAD_ESCAPE_GETVAD,
    D3DKMT_VAD_ESCAPE_GETVADRANGE,
    D3DKMT_VAD_ESCAPE_GET_PTE,
    D3DKMT_VAD_ESCAPE_GET_GPUMMU_CAPS,
    D3DKMT_VAD_ESCAPE_GET_SEGMENT_CAPS,
} D3DKMT_VAD_ESCAPE_COMMAND;

typedef enum _D3DKMT_DEFRAG_ESCAPE_OPERATION
{
    D3DKMT_DEFRAG_ESCAPE_GET_FRAGMENTATION_STATS = 0,
    D3DKMT_DEFRAG_ESCAPE_DEFRAG_UPWARD = 1,
    D3DKMT_DEFRAG_ESCAPE_DEFRAG_DOWNWARD = 2,
    D3DKMT_DEFRAG_ESCAPE_DEFRAG_PASS = 3,
    D3DKMT_DEFRAG_ESCAPE_VERIFY_TRANSFER = 4,
} D3DKMT_DEFRAG_ESCAPE_OPERATION;

typedef struct _D3DKMT_VIDMM_ESCAPE
{
    D3DKMT_VIDMMESCAPETYPE Type;
    union
    {
        struct
        {
            union
            {
                struct
                {
                    ULONG ProbeAndLock : 1;
                    ULONG SplitPoint : 1;
                    ULONG NoDemotion : 1;
                    ULONG SwizzlingAperture : 1;
                    ULONG PagingPathLockSubRange : 1;
                    ULONG PagingPathLockMinRange : 1;
                    ULONG ComplexLock : 1;
                    ULONG FailVARotation : 1;
                    ULONG NoWriteCombined : 1;
                    ULONG NoPrePatching : 1;
                    ULONG AlwaysRepatch : 1;
                    ULONG ExpectPreparationFailure : 1;
                    ULONG FailUserModeVAMapping : 1;
                    ULONG NeverDiscardOfferedAllocation : 1; // since WIN8
                    ULONG AlwaysDiscardOfferedAllocation : 1;
                    ULONG Reserved : 17;
                };
                ULONG Value;
            };
        } SetFault;
        struct
        {
            D3DKMT_HANDLE ResourceHandle;
            D3DKMT_HANDLE AllocationHandle;
            HANDLE hProcess; // 0 to evict memory for the current process, otherwise it is a process handle from OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId).
        } Evict;
        struct
        {
            UINT64 NtHandle; // Used by D3DKMT_VIDMMESCAPETYPE_EVICT_BY_NT_HANDLE
        } EvictByNtHandle;
        struct
        {
            union
            {
                struct 
                {
                    UINT32 NumVads;
                } GetNumVads;
                D3DKMT_VAD_DESC GetVad;
                D3DKMT_VA_RANGE_DESC GetVadRange;
                D3DKMT_GET_GPUMMU_CAPS GetGpuMmuCaps;
                D3DKMT_GET_PTE GetPte;
                D3DKMT_GET_SEGMENT_CAPS GetSegmentCaps;
            };
            _In_ D3DKMT_VAD_ESCAPE_COMMAND Command;
            _Out_ NTSTATUS Status;
        } GetVads;
        struct
        {
            ULONGLONG LocalMemoryBudget;
            ULONGLONG SystemMemoryBudget;
        } SetBudget;
        struct
        {
            HANDLE hProcess;
            BOOL bAllowWakeOnSubmission;
        } SuspendProcess;
        struct
        {
            HANDLE hProcess;
        } ResumeProcess;
        struct
        {
            UINT64 NumBytesToTrim;
        } GetBudget;
        struct
        {
            ULONG MinTrimInterval; // In 100ns units
            ULONG MaxTrimInterval; // In 100ns units
            ULONG IdleTrimInterval; // In 100ns units
        } SetTrimIntervals;
        D3DKMT_EVICTION_CRITERIA EvictByCriteria;
        struct
        {
            BOOL bFlush;
        } Wake;
        struct
        {
            D3DKMT_DEFRAG_ESCAPE_OPERATION Operation;          
            UINT32 SegmentId;
            ULONGLONG TotalCommitted;
            ULONGLONG TotalFree;
            ULONGLONG LargestGapBefore;
            ULONGLONG LargestGapAfter;
        } Defrag;
    };
} D3DKMT_VIDMM_ESCAPE;

typedef enum _D3DKMT_TDRDBGCTRLTYPE
{
    D3DKMT_TDRDBGCTRLTYPE_FORCETDR = 0, // Simulate a TDR
    D3DKMT_TDRDBGCTRLTYPE_DISABLEBREAK = 1, // Disable DebugBreak on timeout
    D3DKMT_TDRDBGCTRLTYPE_ENABLEBREAK = 2, // Enable DebugBreak on timeout
    D3DKMT_TDRDBGCTRLTYPE_UNCONDITIONAL = 3, // Disables all safety conditions (e.g. check for consecutive recoveries)
    D3DKMT_TDRDBGCTRLTYPE_VSYNCTDR = 4, // Simulate a Vsync TDR
    D3DKMT_TDRDBGCTRLTYPE_GPUTDR = 5, // Simulate a GPU TDR
    D3DKMT_TDRDBGCTRLTYPE_FORCEDODTDR = 6, // Simulate a Display Only Present TDR // since WIN8
    D3DKMT_TDRDBGCTRLTYPE_FORCEDODVSYNCTDR = 7, // Simulate a Display Only Vsync TDR
    D3DKMT_TDRDBGCTRLTYPE_ENGINETDR = 8, // Simulate an engine TDR
} D3DKMT_TDRDBGCTRLTYPE;

typedef enum _D3DKMT_VIDSCHESCAPETYPE
{
    D3DKMT_VIDSCHESCAPETYPE_PREEMPTIONCONTROL = 0, // Enable/Disable preemption
    D3DKMT_VIDSCHESCAPETYPE_SUSPENDSCHEDULER = 1, // Suspend/Resume scheduler (obsolate)
    D3DKMT_VIDSCHESCAPETYPE_TDRCONTROL = 2, // Tdr control
    D3DKMT_VIDSCHESCAPETYPE_SUSPENDRESUME = 3, // Suspend/Resume scheduler
    D3DKMT_VIDSCHESCAPETYPE_ENABLECONTEXTDELAY = 4, // Enable/Disable context delay // since WIN8
    D3DKMT_VIDSCHESCAPETYPE_CONFIGURE_TDR_LIMIT = 5, // Configure TdrLimitCount and TdrLimitTime
    D3DKMT_VIDSCHESCAPETYPE_VGPU_RESET = 6, // Trigger VGPU reset 
    D3DKMT_VIDSCHESCAPETYPE_PFN_CONTROL = 7, // Periodic frame notification control
} D3DKMT_VIDSCHESCAPETYPE;

typedef enum _D3DKMT_ESCAPE_PFN_CONTROL_COMMAND
{
    D3DKMT_ESCAPE_PFN_CONTROL_DEFAULT,
    D3DKMT_ESCAPE_PFN_CONTROL_FORCE_CPU,
    D3DKMT_ESCAPE_PFN_CONTROL_FORCE_GPU
} D3DKMT_ESCAPE_PFN_CONTROL_COMMAND;

typedef struct _D3DKMT_VIDSCH_ESCAPE
{
    D3DKMT_VIDSCHESCAPETYPE Type;
    union
    {
        BOOL PreemptionControl; // enable/disable preemption
        BOOL EnableContextDelay; // enable/disable context delay // since WIN8
        struct
        {
            ULONG TdrControl; // control tdr
            union
            {
                ULONG NodeOrdinal; // valid if TdrControl is set to D3DKMT_TDRDBGCTRLTYPE_ENGINETDR
            };
        } TdrControl2;
        BOOL SuspendScheduler; // suspend/resume scheduler (obsolate) // since Vista
        ULONG TdrControl; // control tdr
        ULONG SuspendTime; // time period to suspend.
        struct
        {
            UINT Count;
            UINT Time; // In seconds
        } TdrLimit;
        D3DKMT_ESCAPE_PFN_CONTROL_COMMAND PfnControl; // periodic frame notification control
    };
} D3DKMT_VIDSCH_ESCAPE;

typedef enum _D3DKMT_DEVICEESCAPE_TYPE
{
    D3DKMT_DEVICEESCAPE_VIDPNFROMALLOCATION = 0,
    D3DKMT_DEVICEESCAPE_RESTOREGAMMA = 1,
} D3DKMT_DEVICEESCAPE_TYPE;

typedef struct _D3DKMT_DEVICE_ESCAPE
{
    D3DKMT_DEVICEESCAPE_TYPE Type;
    union
    {
        struct
        {
            _In_ D3DKMT_HANDLE hPrimaryAllocation; // Primary allocation handle
            _Out_ UINT32 VidPnSourceId; // VidPnSoureId of primary allocation
        } VidPnFromAllocation;
    };
} D3DKMT_DEVICE_ESCAPE;

typedef enum _D3DKMT_DMMESCAPETYPE
{
    D3DKMT_DMMESCAPETYPE_UNINITIALIZED = 0,
    D3DKMT_DMMESCAPETYPE_GET_SUMMARY_INFO = 1,
    D3DKMT_DMMESCAPETYPE_GET_VIDEO_PRESENT_SOURCES_INFO = 2,
    D3DKMT_DMMESCAPETYPE_GET_VIDEO_PRESENT_TARGETS_INFO = 3,
    D3DKMT_DMMESCAPETYPE_GET_ACTIVEVIDPN_INFO = 4,
    D3DKMT_DMMESCAPETYPE_GET_MONITORS_INFO = 5,
    D3DKMT_DMMESCAPETYPE_RECENTLY_COMMITTED_VIDPNS_INFO = 6,
    D3DKMT_DMMESCAPETYPE_RECENT_MODECHANGE_REQUESTS_INFO = 7,
    D3DKMT_DMMESCAPETYPE_RECENTLY_RECOMMENDED_VIDPNS_INFO = 8,
    D3DKMT_DMMESCAPETYPE_RECENT_MONITOR_PRESENCE_EVENTS_INFO = 9,
    D3DKMT_DMMESCAPETYPE_ACTIVEVIDPN_SOURCEMODESET_INFO = 10,
    D3DKMT_DMMESCAPETYPE_ACTIVEVIDPN_COFUNCPATHMODALITY_INFO = 11,
    D3DKMT_DMMESCAPETYPE_GET_LASTCLIENTCOMMITTEDVIDPN_INFO = 12,
    D3DKMT_DMMESCAPETYPE_GET_VERSION_INFO = 13,
    D3DKMT_DMMESCAPETYPE_VIDPN_MGR_DIAGNOSTICS = 14
} D3DKMT_DMMESCAPETYPE;

typedef struct _D3DKMT_DMM_ESCAPE
{
    _In_ D3DKMT_DMMESCAPETYPE Type;
    _In_ SIZE_T ProvidedBufferSize; // actual size of Data[] array, in bytes.
    _Out_ SIZE_T MinRequiredBufferSize; // minimum required size of Data[] array to contain requested data.
    _Out_writes_bytes_(ProvidedBufferSize) UCHAR Data[1];
} D3DKMT_DMM_ESCAPE;

typedef struct _D3DKMT_DEBUG_SNAPSHOT_ESCAPE
{
    ULONG Length; // out: Actual length of the snapshot written in Buffer
    BYTE Buffer[1]; // out: Buffer to place snapshot
} D3DKMT_DEBUG_SNAPSHOT_ESCAPE;

typedef enum _D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE
{
    D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE_EXTRA_CCD_DATABASE_INFO = 0,
    D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE_MODES_PRUNED = 15,
}D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE;

typedef struct _D3DKMT_ACTIVATE_SPECIFIC_DIAG_ESCAPE
{
    D3DKMT_ACTIVATE_SPECIFIC_DIAG_TYPE Type; // The escape type that needs to be (de)activated
    BOOL Activate; // FALSE means deactivate
} D3DKMT_ACTIVATE_SPECIFIC_DIAG_ESCAPE;

typedef struct _D3DKMT_REQUEST_MACHINE_CRASH_ESCAPE
{
    ULONG_PTR Param1;
    ULONG_PTR Param2;
    ULONG_PTR Param3;
} D3DKMT_REQUEST_MACHINE_CRASH_ESCAPE;

#ifdef USE_D3DKMT_SINGLE_HEADER
typedef struct _D3DDDI_ESCAPEFLAGS
{
    union
    {
        struct
        {
            UINT32 HardwareAccess : 1;
            UINT32 DeviceStatusQuery : 1; // since WDDM1_3
            UINT32 ChangeFrameLatency : 1;
            UINT32 NoAdapterSynchronization : 1; // since WDDM2_0
            UINT32 Reserved : 1; // Used internally by DisplayOnly present // since WDDM2_2
            UINT32 VirtualMachineData : 1; // Cannot be set from user mode
            UINT32 DriverKnownEscape : 1; // Driver private data points to a well known structure
            UINT32 DriverCommonEscape : 1; // Private data points runtime defined structure
            UINT32 Reserved2 : 24;
        };
        UINT32 Value;
    };
} D3DDDI_ESCAPEFLAGS;
#endif

// The D3DKMT_ESCAPE structure describes information that is exchanged with the display miniport driver.
typedef struct _D3DKMT_ESCAPE 
{
    _In_ D3DKMT_HANDLE AdapterHandle;
    _In_opt_ D3DKMT_HANDLE DeviceHandle;
    _In_ D3DKMT_ESCAPETYPE Type;
    _In_ D3DDDI_ESCAPEFLAGS Flags;
    _Inout_ PVOID PrivateDriverData;
    _In_ UINT32 PrivateDriverDataSize;
    _In_opt_ D3DKMT_HANDLE ContextHandle;
} D3DKMT_ESCAPE;

// Function pointers

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTOpenAdapterFromDeviceName(
    _Inout_ CONST D3DKMT_OPENADAPTERFROMDEVICENAME *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTOpenAdapterFromGdiDisplayName(
    _Inout_ CONST D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTOpenAdapterFromHdc(
    _Inout_ CONST D3DKMT_OPENADAPTERFROMHDC *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTOpenAdapterFromLuid(
    _Inout_ CONST D3DKMT_OPENADAPTERFROMLUID *pAdapter
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTEnumAdapters(
    _Inout_ CONST D3DKMT_ENUMADAPTERS *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTEnumAdapters2(
    _Inout_ CONST D3DKMT_ENUMADAPTERS2 *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTEnumAdapters3(
    _Inout_ CONST D3DKMT_ENUMADAPTERS2* pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTCloseAdapter(
    _In_ CONST D3DKMT_CLOSEADAPTER *pData
    );

NTSYSAPI
NTSTATUS
NTAPI 
D3DKMTQueryAdapterInfo(
    _Inout_ CONST D3DKMT_QUERYADAPTERINFO *pData
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
D3DKMTQueryStatistics(
    _Inout_ CONST D3DKMT_QUERYSTATISTICS *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTQueryVideoMemoryInfo(
    _Inout_ CONST D3DKMT_QUERYVIDEOMEMORYINFO *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTEscape(
    _Inout_ CONST D3DKMT_ESCAPE *pData
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTSetProcessSchedulingPriorityClass(
    _In_ HANDLE, 
    _In_ enum D3DKMT_SCHEDULINGPRIORITYCLASS
    );

NTSYSAPI
NTSTATUS
NTAPI
D3DKMTGetProcessSchedulingPriorityClass(
    _In_ HANDLE, 
    _Out_ enum D3DKMT_SCHEDULINGPRIORITYCLASS*
    );

#endif
