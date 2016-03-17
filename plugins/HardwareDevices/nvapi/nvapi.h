#include "nvapi_lite_common.h"

 /***************************************************************************\
|*                                                                           *|
|*      Copyright 2005-2010 NVIDIA Corporation.  All rights reserved.        *|
|*                                                                           *|     
|*   NOTICE TO USER:                                                         *|                 
|*                                                                           *|
|*   This source code is subject to NVIDIA ownership rights under U.S.       *|
|*   and international Copyright laws.  Users and possessors of this         *| 
|*   source code are hereby granted a nonexclusive, royalty-free             *|
|*   license to use this code in individual and commercial software.         *|
|*                                                                           *|
|*   NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE     *|
|*   CODE FOR ANY PURPOSE. IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR         *|
|*   IMPLIED WARRANTY OF ANY KIND. NVIDIA DISCLAIMS ALL WARRANTIES WITH      *|
|*   REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF         *|
|*   MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR          *|
|*   PURPOSE. IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL,            *|
|*   INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES          *|
|*   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN      *|
|*   AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING     *|
|*   OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE      *| 
|*   CODE.                                                                   *|
|*                                                                           *|
|*   U.S. Government End Users. This source code is a "commercial item"      *|
|*   as that term is defined at 48 C.F.R. 2.101 (OCT 1995), consisting       *|
|*   of "commercial computer  software" and "commercial computer software    *|
|*   documentation" as such terms are used in 48 C.F.R. 12.212 (SEPT 1995)   *|
|*   and is provided to the U.S. Government only as a commercial end item.   *|
|*   Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through        *|
|*   227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the       *|
|*   source code with only those rights set forth herein.                    *|
|*                                                                           *|
|*   Any use of this source code in individual and commercial software must  *| 
|*   include, in the user documentation and internal comments to the code,   *| 
|*   the above Disclaimer and U.S. Government End Users Notice.              *|
|*                                                                           *|
|*                                                                           *|
 \***************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//
// Date: Sep 30, 2014 
// File: nvapi.h
//
// NvAPI provides an interface to NVIDIA devices. This file contains the 
// interface constants, structure definitions and function prototypes.
//
//   Target Profile: developer
//  Target Platform: windows
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _NVAPI_H
#define _NVAPI_H

#include <pshpack8.h>  // Make sure we have consistent structure packings

#ifdef __cplusplus
extern "C" {
#endif

// ====================================================
// Universal NvAPI Definitions
// ====================================================
#ifndef _WIN32
#define __cdecl
#endif



//! @}

//!   \ingroup nvapistatus 
#define NVAPI_API_NOT_INTIALIZED        NVAPI_API_NOT_INITIALIZED       //!< Fix typo in error code

//!   \ingroup nvapistatus 
#define NVAPI_INVALID_USER_PRIVILEDGE   NVAPI_INVALID_USER_PRIVILEGE    //!< Fix typo in error code


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Initialize
//
//! This function initializes the NvAPI library. 
//! This must be called before calling other NvAPI_ functions. 
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 80
//!
//! \retval  NVAPI_ERROR            An error occurred during the initialization process (generic error)
//! \retval  NVAPI_LIBRARYNOTFOUND  Failed to load the NVAPI support library
//! \retval  NVAPI_OK               Initialized
//! \sa nvapistatus
//! \ingroup nvapifunctions
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_Initialize();

typedef NvAPI_Status (__cdecl *_NvAPI_Initialize)(VOID);
_NvAPI_Initialize NvAPI_Initialize;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Unload
//
//!   DESCRIPTION: Unloads NVAPI library. This must be the last function called. 
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//!  !! This is not thread safe. In a multithreaded environment, calling NvAPI_Unload       !! \n
//!  !! while another thread is executing another NvAPI_XXX function, results in           !!  \n
//!  !! undefined behaviour and might even cause the application to crash. Developers       !! \n
//!  !! must make sure that they are not in any other function before calling NvAPI_Unload. !! \n
//!
//!
//!  Unloading NvAPI library is not supported when the library is in a resource locked state.
//!  Some functions in the NvAPI library initiates an operation or allocates certain resources
//!  and there are corresponding functions available, to complete the operation or free the
//!  allocated resources. All such function pairs are designed to prevent unloading NvAPI library.
//!
//!  For example, if NvAPI_Unload is called after NvAPI_XXX which locks a resource, it fails with
//!  NVAPI_ERROR. Developers need to call the corresponding NvAPI_YYY to unlock the resources, 
//!  before calling NvAPI_Unload again.
//!
//! \retval ::NVAPI_ERROR            One or more resources are locked and hence cannot unload NVAPI library
//! \retval ::NVAPI_OK               NVAPI library unloaded
//!
//! \ingroup nvapifunctions
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_Unload();

typedef NvAPI_Status (__cdecl *_NvAPI_Unload)(VOID);
_NvAPI_Unload NvAPI_Unload;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetErrorMessage
//
//! This function converts an NvAPI error code into a null terminated string.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 80
//!
//! \param nr      The error code to convert
//! \param szDesc  The string corresponding to the error code
//!
//! \return NULL terminated string (always, never NULL)
//! \ingroup nvapifunctions
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GetErrorMessage(NvAPI_Status nr,NvAPI_ShortString szDesc);

typedef NvAPI_Status (__cdecl *_NvAPI_GetErrorMessage)(NvAPI_Status nr, NvAPI_ShortString szDesc);
_NvAPI_GetErrorMessage NvAPI_GetErrorMessage;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetInterfaceVersionString
//
//! This function returns a string describing the version of the NvAPI library.
//!               The contents of the string are human readable.  Do not assume a fixed
//!                format.
//!
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 80
//!
//! \param  szDesc User readable string giving NvAPI version information
//!
//! \return See \ref nvapistatus for the list of possible return values.
//! \ingroup nvapifunctions
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetInterfaceVersionString(NvAPI_ShortString szDesc);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//              All display port related data types definition starts
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This category is intentionally added before the #ifdef. The #endif should also be in the same scope
#ifndef DISPLAYPORT_STRUCTS_DEFINED
#define DISPLAYPORT_STRUCTS_DEFINED

//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum _NV_DP_LINK_RATE
{
    NV_DP_1_62GBPS            = 6,
    NV_DP_2_70GBPS            = 0xA,
    NV_DP_5_40GBPS            = 0x14
} NV_DP_LINK_RATE;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum _NV_DP_LANE_COUNT
{
    NV_DP_1_LANE              = 1,
    NV_DP_2_LANE              = 2,
    NV_DP_4_LANE              = 4,
} NV_DP_LANE_COUNT;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum _NV_DP_COLOR_FORMAT
{
    NV_DP_COLOR_FORMAT_RGB     = 0,
    NV_DP_COLOR_FORMAT_YCbCr422,
    NV_DP_COLOR_FORMAT_YCbCr444,
} NV_DP_COLOR_FORMAT;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum _NV_DP_COLORIMETRY
{
    NV_DP_COLORIMETRY_RGB     = 0,
    NV_DP_COLORIMETRY_YCbCr_ITU601,
    NV_DP_COLORIMETRY_YCbCr_ITU709,
} NV_DP_COLORIMETRY;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum _NV_DP_DYNAMIC_RANGE
{
    NV_DP_DYNAMIC_RANGE_VESA  = 0,
    NV_DP_DYNAMIC_RANGE_CEA,
} NV_DP_DYNAMIC_RANGE;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum _NV_DP_BPC
{
    NV_DP_BPC_DEFAULT         = 0,
    NV_DP_BPC_6,
    NV_DP_BPC_8,
    NV_DP_BPC_10,
    NV_DP_BPC_12,
    NV_DP_BPC_16,
} NV_DP_BPC;

#endif  //#ifndef DISPLAYPORT_STRUCTS_DEFINED

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//              All display port related data types definitions end
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetEDID
//
//! \fn NvAPI_GPU_GetEDID(NvPhysicalGpuHandle hPhysicalGpu, NvU32 displayOutputId, NV_EDID *pEDID)
//!  This function returns the EDID data for the specified GPU handle and connection bit mask.
//!  displayOutputId should have exactly 1 bit set to indicate a single display. See \ref handles.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 85
//!
//! \retval    NVAPI_INVALID_ARGUMENT              pEDID is NULL; displayOutputId has 0 or > 1 bits set
//! \retval    NVAPI_OK                           *pEDID contains valid data.
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \retval    NVAPI_DATA_NOT_FOUND                The requested display does not contain an EDID.
//
///////////////////////////////////////////////////////////////////////////////


//! \ingroup gpu
//! @{

#define NV_EDID_V1_DATA_SIZE   256

#define NV_EDID_DATA_SIZE      NV_EDID_V1_DATA_SIZE

typedef struct
{
    NvU32   version;        //structure version
    NvU8    EDID_Data[NV_EDID_DATA_SIZE];
} NV_EDID_V1;

//! Used in NvAPI_GPU_GetEDID()
typedef struct
{
    NvU32   version;        //!< Structure version
    NvU8    EDID_Data[NV_EDID_DATA_SIZE];
    NvU32   sizeofEDID;
} NV_EDID_V2;

//! Used in NvAPI_GPU_GetEDID()
typedef struct
{
    NvU32   version;        //!< Structure version
    NvU8    EDID_Data[NV_EDID_DATA_SIZE];
    NvU32   sizeofEDID;
    NvU32   edidId;     //!< ID which always returned in a monotonically increasing counter.
                       //!< Across a split-EDID read we need to verify that all calls returned the same edidId.
                       //!< This counter is incremented if we get the updated EDID. 
    NvU32   offset;    //!< Which 256-byte page of the EDID we want to read. Start at 0.
                       //!< If the read succeeds with edidSize > NV_EDID_DATA_SIZE,
                       //!< call back again with offset+256 until we have read the entire buffer
} NV_EDID_V3;

typedef NV_EDID_V3    NV_EDID;

#define NV_EDID_VER1    MAKE_NVAPI_VERSION(NV_EDID_V1,1)
#define NV_EDID_VER2    MAKE_NVAPI_VERSION(NV_EDID_V2,2)
#define NV_EDID_VER3    MAKE_NVAPI_VERSION(NV_EDID_V3,3)
#define NV_EDID_VER   NV_EDID_VER3

//! @}

//! \ingroup gpu
NVAPI_INTERFACE NvAPI_GPU_GetEDID(NvPhysicalGpuHandle hPhysicalGpu, NvU32 displayOutputId, NV_EDID *pEDID);

//! \ingroup gpu
//! Used in NV_GPU_CONNECTOR_DATA
typedef enum _NV_GPU_CONNECTOR_TYPE
{
    NVAPI_GPU_CONNECTOR_VGA_15_PIN                      = 0x00000000,
    NVAPI_GPU_CONNECTOR_TV_COMPOSITE                    = 0x00000010,
    NVAPI_GPU_CONNECTOR_TV_SVIDEO                       = 0x00000011,
    NVAPI_GPU_CONNECTOR_TV_HDTV_COMPONENT               = 0x00000013,
    NVAPI_GPU_CONNECTOR_TV_SCART                        = 0x00000014,
    NVAPI_GPU_CONNECTOR_TV_COMPOSITE_SCART_ON_EIAJ4120  = 0x00000016,
    NVAPI_GPU_CONNECTOR_TV_HDTV_EIAJ4120                = 0x00000017,
    NVAPI_GPU_CONNECTOR_PC_POD_HDTV_YPRPB               = 0x00000018,
    NVAPI_GPU_CONNECTOR_PC_POD_SVIDEO                   = 0x00000019,
    NVAPI_GPU_CONNECTOR_PC_POD_COMPOSITE                = 0x0000001A,
    NVAPI_GPU_CONNECTOR_DVI_I_TV_SVIDEO                 = 0x00000020,
    NVAPI_GPU_CONNECTOR_DVI_I_TV_COMPOSITE              = 0x00000021,
    NVAPI_GPU_CONNECTOR_DVI_I                           = 0x00000030,
    NVAPI_GPU_CONNECTOR_DVI_D                           = 0x00000031,
    NVAPI_GPU_CONNECTOR_ADC                             = 0x00000032,
    NVAPI_GPU_CONNECTOR_LFH_DVI_I_1                     = 0x00000038,
    NVAPI_GPU_CONNECTOR_LFH_DVI_I_2                     = 0x00000039,
    NVAPI_GPU_CONNECTOR_SPWG                            = 0x00000040,
    NVAPI_GPU_CONNECTOR_OEM                             = 0x00000041,
    NVAPI_GPU_CONNECTOR_DISPLAYPORT_EXTERNAL            = 0x00000046,
    NVAPI_GPU_CONNECTOR_DISPLAYPORT_INTERNAL            = 0x00000047,
    NVAPI_GPU_CONNECTOR_DISPLAYPORT_MINI_EXT            = 0x00000048,
    NVAPI_GPU_CONNECTOR_HDMI_A                          = 0x00000061,
    NVAPI_GPU_CONNECTOR_HDMI_C_MINI                     = 0x00000063,
    NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_1               = 0x00000064,
    NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_2               = 0x00000065,
    NVAPI_GPU_CONNECTOR_VIRTUAL_WFD                     = 0x00000070,    
    NVAPI_GPU_CONNECTOR_UNKNOWN                         = 0xFFFFFFFF,
} NV_GPU_CONNECTOR_TYPE;

////////////////////////////////////////////////////////////////////////////////
//
// NvAPI_TVOutput Information
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup tvapi
//! Used in NV_DISPLAY_TV_OUTPUT_INFO
typedef enum _NV_DISPLAY_TV_FORMAT
{
    NV_DISPLAY_TV_FORMAT_NONE               = 0,
    NV_DISPLAY_TV_FORMAT_SD_NTSCM           = 0x00000001,
    NV_DISPLAY_TV_FORMAT_SD_NTSCJ           = 0x00000002,
    NV_DISPLAY_TV_FORMAT_SD_PALM            = 0x00000004,
    NV_DISPLAY_TV_FORMAT_SD_PALBDGH         = 0x00000008,
    NV_DISPLAY_TV_FORMAT_SD_PALN            = 0x00000010,
    NV_DISPLAY_TV_FORMAT_SD_PALNC           = 0x00000020,
    NV_DISPLAY_TV_FORMAT_SD_576i            = 0x00000100,
    NV_DISPLAY_TV_FORMAT_SD_480i            = 0x00000200,
    NV_DISPLAY_TV_FORMAT_ED_480p            = 0x00000400,
    NV_DISPLAY_TV_FORMAT_ED_576p            = 0x00000800,
    NV_DISPLAY_TV_FORMAT_HD_720p            = 0x00001000,
    NV_DISPLAY_TV_FORMAT_HD_1080i           = 0x00002000,
    NV_DISPLAY_TV_FORMAT_HD_1080p           = 0x00004000,
    NV_DISPLAY_TV_FORMAT_HD_720p50          = 0x00008000,
    NV_DISPLAY_TV_FORMAT_HD_1080p24         = 0x00010000,
    NV_DISPLAY_TV_FORMAT_HD_1080i50         = 0x00020000,
    NV_DISPLAY_TV_FORMAT_HD_1080p50         = 0x00040000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp30          = 0x00080000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp30_3840     = NV_DISPLAY_TV_FORMAT_UHD_4Kp30,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp25          = 0x00100000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp25_3840     = NV_DISPLAY_TV_FORMAT_UHD_4Kp25,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp24          = 0x00200000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp24_3840     = NV_DISPLAY_TV_FORMAT_UHD_4Kp24,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp24_SMPTE    = 0x00400000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp50_3840     = 0x00800000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp60_3840     = 0x00900000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp30_4096     = 0x00A00000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp25_4096     = 0x00B00000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp24_4096     = 0x00C00000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp50_4096     = 0x00D00000,
    NV_DISPLAY_TV_FORMAT_UHD_4Kp60_4096     = 0x00E00000,

    NV_DISPLAY_TV_FORMAT_SD_OTHER           = 0x01000000,
    NV_DISPLAY_TV_FORMAT_ED_OTHER           = 0x02000000,
    NV_DISPLAY_TV_FORMAT_HD_OTHER           = 0x04000000,

    NV_DISPLAY_TV_FORMAT_ANY                = 0x80000000,

} NV_DISPLAY_TV_FORMAT;


//! \ingroup dispcontrol
//! @{
#define NVAPI_MAX_VIEW_TARGET  2
#define NVAPI_ADVANCED_MAX_VIEW_TARGET 4

#ifndef _NV_TARGET_VIEW_MODE_
#define _NV_TARGET_VIEW_MODE_

//! Used in NvAPI_SetView().
typedef enum _NV_TARGET_VIEW_MODE
{
    NV_VIEW_MODE_STANDARD  = 0,
    NV_VIEW_MODE_CLONE     = 1,
    NV_VIEW_MODE_HSPAN     = 2,
    NV_VIEW_MODE_VSPAN     = 3,
    NV_VIEW_MODE_DUALVIEW  = 4,
    NV_VIEW_MODE_MULTIVIEW = 5,
} NV_TARGET_VIEW_MODE;
#endif

//! @}


// Following definitions are used in NvAPI_SetViewEx.

//! Scaling modes - used in NvAPI_SetViewEx().
//! \ingroup dispcontrol
typedef enum _NV_SCALING
{
    NV_SCALING_DEFAULT          = 0,        //!< No change

    // New Scaling Declarations
    NV_SCALING_GPU_SCALING_TO_CLOSEST                   = 1,  //!< Balanced  - Full Screen
    NV_SCALING_GPU_SCALING_TO_NATIVE                    = 2,  //!< Force GPU - Full Screen
    NV_SCALING_GPU_SCANOUT_TO_NATIVE                    = 3,  //!< Force GPU - Centered\No Scaling
    NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_NATIVE  = 5,  //!< Force GPU - Aspect Ratio
    NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_CLOSEST = 6,  //!< Balanced  - Aspect Ratio
    NV_SCALING_GPU_SCANOUT_TO_CLOSEST                   = 7,  //!< Balanced  - Centered\No Scaling
    
    // Legacy Declarations
    NV_SCALING_MONITOR_SCALING                          = NV_SCALING_GPU_SCALING_TO_CLOSEST,
    NV_SCALING_ADAPTER_SCALING                          = NV_SCALING_GPU_SCALING_TO_NATIVE,
    NV_SCALING_CENTERED                                 = NV_SCALING_GPU_SCANOUT_TO_NATIVE,
    NV_SCALING_ASPECT_SCALING                           = NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_NATIVE,

    NV_SCALING_CUSTOMIZED       = 255       //!< For future use
} NV_SCALING;

//! Rotate modes- used in NvAPI_SetViewEx().
//! \ingroup dispcontrol
typedef enum _NV_ROTATE
{
    NV_ROTATE_0           = 0,
    NV_ROTATE_90          = 1,
    NV_ROTATE_180         = 2,
    NV_ROTATE_270         = 3,
    NV_ROTATE_IGNORED     = 4,
} NV_ROTATE;

//! Color formats- used in NvAPI_SetViewEx().
//! \ingroup dispcontrol
#define NVFORMAT_MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                         ((NvU32)(NvU8)(ch0) | ((NvU32)(NvU8)(ch1) << 8) |   \
                     ((NvU32)(NvU8)(ch2) << 16) | ((NvU32)(NvU8)(ch3) << 24 ))



//! Color formats- used in NvAPI_SetViewEx().
//! \ingroup dispcontrol
typedef enum _NV_FORMAT
{
    NV_FORMAT_UNKNOWN           =  0,       //!< unknown. Driver will choose one as following value.
    NV_FORMAT_P8                = 41,       //!< for 8bpp mode
    NV_FORMAT_R5G6B5            = 23,       //!< for 16bpp mode
    NV_FORMAT_A8R8G8B8          = 21,       //!< for 32bpp mode
    NV_FORMAT_A16B16G16R16F     = 113,      //!< for 64bpp(floating point) mode.

} NV_FORMAT;

// TV standard

typedef struct 
{
    float x;    //!<  x-coordinate of the viewport top-left point
    float y;    //!<  y-coordinate of the viewport top-left point
    float w;    //!<  Width of the viewport
    float h;    //!<  Height of the viewport
} NV_VIEWPORTF;



//! \ingroup dispcontrol
//! The timing override is not supported yet; must be set to _AUTO. \n


typedef enum _NV_TIMING_OVERRIDE
{
    NV_TIMING_OVERRIDE_CURRENT = 0,          //!< get the current timing
    NV_TIMING_OVERRIDE_AUTO,                 //!< the timing the driver will use based the current policy
    NV_TIMING_OVERRIDE_EDID,                 //!< EDID timing
    NV_TIMING_OVERRIDE_DMT,                  //!< VESA DMT timing
    NV_TIMING_OVERRIDE_DMT_RB,               //!< VESA DMT timing with reduced blanking
    NV_TIMING_OVERRIDE_CVT,                  //!< VESA CVT timing
    NV_TIMING_OVERRIDE_CVT_RB,               //!< VESA CVT timing with reduced blanking
    NV_TIMING_OVERRIDE_GTF,                  //!< VESA GTF timing
    NV_TIMING_OVERRIDE_EIA861,               //!< EIA 861x pre-defined timing
    NV_TIMING_OVERRIDE_ANALOG_TV,            //!< analog SD/HDTV timing
    NV_TIMING_OVERRIDE_CUST,                 //!< NV custom timings
    NV_TIMING_OVERRIDE_NV_PREDEFINED,        //!< NV pre-defined timing (basically the PsF timings)
    NV_TIMING_OVERRIDE_NV_PSF                = NV_TIMING_OVERRIDE_NV_PREDEFINED,
    NV_TIMING_OVERRIDE_NV_ASPR,
    NV_TIMING_OVERRIDE_SDI,                  //!< Override for SDI timing

    NV_TIMING_OVRRIDE_MAX,                   
}NV_TIMING_OVERRIDE;


#ifndef NV_TIMING_STRUCTS_DEFINED
#define NV_TIMING_STRUCTS_DEFINED

//***********************
// The Timing Structure
//***********************
//
//! \ingroup dispcontrol
//!  NVIDIA-specific timing extras \n
//! Used in NV_TIMING.
typedef struct tagNV_TIMINGEXT
{
    NvU32   flag;          //!< Reserved for NVIDIA hardware-based enhancement, such as double-scan.
    NvU16   rr;            //!< Logical refresh rate to present
    NvU32   rrx1k;         //!< Physical vertical refresh rate in 0.001Hz
    NvU32   aspect;        //!< Display aspect ratio Hi(aspect):horizontal-aspect, Low(aspect):vertical-aspect
    NvU16   rep;           //!< Bit-wise pixel repetition factor: 0x1:no pixel repetition; 0x2:each pixel repeats twice horizontally,..
    NvU32   status;        //!< Timing standard 
    NvU8    name[40];      //!< Timing name
}NV_TIMINGEXT;



//! \ingroup dispcontrol
//!The very basic timing structure based on the VESA standard:
//! \code
//!            |<----------------------------htotal--------------------------->| 
//!             ---------"active" video-------->|<-------blanking------>|<-----  
//!            |<-------hvisible-------->|<-hb->|<-hfp->|<-hsw->|<-hbp->|<-hb->| 
//! --------- -+-------------------------+      |       |       |       |      | 
//!   A      A |                         |      |       |       |       |      | 
//!   :      : |                         |      |       |       |       |      | 
//!   :      : |                         |      |       |       |       |      | 
//!   :vertical|    addressable video    |      |       |       |       |      | 
//!   : visible|                         |      |       |       |       |      | 
//!   :      : |                         |      |       |       |       |      | 
//!   :      : |                         |      |       |       |       |      | 
//! vertical V |                         |      |       |       |       |      | 
//!  total   --+-------------------------+      |       |       |       |      | 
//!   :      vb         border                  |       |       |       |      | 
//!   :      -----------------------------------+       |       |       |      |  
//!   :      vfp        front porch                     |       |       |      |  
//!   :      -------------------------------------------+       |       |      | 
//!   :      vsw        sync width                              |       |      | 
//!   :      ---------------------------------------------------+       |      | 
//!   :      vbp        back porch                                      |      | 
//!   :      -----------------------------------------------------------+      | 
//!   V      vb         border                                                 | 
//! ---------------------------------------------------------------------------+ 
//! \endcode
typedef struct _NV_TIMING
{
    // VESA scan out timing parameters:
    NvU16 HVisible;         //!< horizontal visible 
    NvU16 HBorder;          //!< horizontal border 
    NvU16 HFrontPorch;      //!< horizontal front porch
    NvU16 HSyncWidth;       //!< horizontal sync width
    NvU16 HTotal;           //!< horizontal total
    NvU8  HSyncPol;         //!< horizontal sync polarity: 1-negative, 0-positive

    NvU16 VVisible;         //!< vertical visible
    NvU16 VBorder;          //!< vertical border
    NvU16 VFrontPorch;      //!< vertical front porch
    NvU16 VSyncWidth;       //!< vertical sync width
    NvU16 VTotal;           //!< vertical total
    NvU8  VSyncPol;         //!< vertical sync polarity: 1-negative, 0-positive
    
    NvU16 interlaced;       //!< 1-interlaced, 0-progressive
    NvU32 pclk;             //!< pixel clock in 10 kHz

    //other timing related extras
    NV_TIMINGEXT etc;          
}NV_TIMING;
#endif //NV_TIMING_STRUCTS_DEFINED


//! \addtogroup dispcontrol
//! Timing-related constants
//! @{
#define NV_TIMING_H_SYNC_POSITIVE                             0
#define NV_TIMING_H_SYNC_NEGATIVE                             1
#define NV_TIMING_H_SYNC_DEFAULT                              NV_TIMING_H_SYNC_NEGATIVE
//
#define NV_TIMING_V_SYNC_POSITIVE                             0
#define NV_TIMING_V_SYNC_NEGATIVE                             1
#define NV_TIMING_V_SYNC_DEFAULT                              NV_TIMING_V_SYNC_POSITIVE
//
#define NV_TIMING_PROGRESSIVE                                 0
#define NV_TIMING_INTERLACED                                  1
#define NV_TIMING_INTERLACED_EXTRA_VBLANK_ON_FIELD2           1
#define NV_TIMING_INTERLACED_NO_EXTRA_VBLANK_ON_FIELD2        2
//! @}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_SetView
//
//! \fn NvAPI_SetView(NvDisplayHandle hNvDisplay, NV_VIEW_TARGET_INFO *pTargetInfo, NV_TARGET_VIEW_MODE targetView)
//!  This function lets the caller modify the target display arrangement of the selected source display handle in any nView mode.
//!  It can also modify or extend the source display in Dualview mode.
//!  \note Maps the selected source to the associated target Ids.
//!  \note Display PATH with this API is limited to single GPU. DUALVIEW across GPUs cannot be enabled with this API. 
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_SetDisplayConfig.
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 90
//!
//! \param [in]  hNvDisplay       NVIDIA Display selection. #NVAPI_DEFAULT_HANDLE is not allowed, it has to be a handle enumerated with NvAPI_EnumNVidiaDisplayHandle().
//! \param [in]  pTargetInfo      Pointer to array of NV_VIEW_TARGET_INFO, specifying device properties in this view.
//!                               The first device entry in the array is the physical primary.
//!                               The device entry with the lowest source id is the desktop primary.
//! \param [in]  targetCount      Count of target devices specified in pTargetInfo.
//! \param [in]  targetView       Target view selected from NV_TARGET_VIEW_MODE.
//!
//! \retval  NVAPI_OK               Completed request
//! \retval  NVAPI_ERROR            Miscellaneous error occurred
//! \retval  NVAPI_INVALID_ARGUMENT Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup dispcontrol
//! Used in NvAPI_SetView() and NvAPI_GetView()
typedef struct
{
    NvU32 version;     //!< (IN) structure version
    NvU32 count;       //!< (IN) target count
    struct 
    {
        NvU32 deviceMask;    //!< (IN/OUT) Device mask
        NvU32 sourceId;      //!< (IN/OUT) Source ID - values will be based on the number of heads exposed per GPU.
        NvU32 bPrimary:1;    //!< (OUT) Indicates if this is the GPU's primary view target. This is not the desktop GDI primary.
                             //!< NvAPI_SetView automatically selects the first target in NV_VIEW_TARGET_INFO index 0 as the GPU's primary view.
        NvU32 bInterlaced:1; //!< (IN/OUT) Indicates if the timing being used on this monitor is interlaced.
        NvU32 bGDIPrimary:1; //!< (IN/OUT) Indicates if this is the desktop GDI primary.
        NvU32 bForceModeSet:1;//!< (IN) Used only on Win7 and higher during a call to NvAPI_SetView(). Turns off optimization & forces OS to set supplied mode.
    } target[NVAPI_MAX_VIEW_TARGET];
} NV_VIEW_TARGET_INFO; 

//! \ingroup dispcontrol
#define NV_VIEW_TARGET_INFO_VER  MAKE_NVAPI_VERSION(NV_VIEW_TARGET_INFO,2)


//! \ingroup dispcontrol
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_SetDisplayConfig.")
NVAPI_INTERFACE NvAPI_SetView(NvDisplayHandle hNvDisplay, NV_VIEW_TARGET_INFO *pTargetInfo, NV_TARGET_VIEW_MODE targetView);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_SetViewEx
//
//!  \fn NvAPI_SetViewEx(NvDisplayHandle hNvDisplay, NV_DISPLAY_PATH_INFO *pPathInfo, NV_TARGET_VIEW_MODE displayView)
//!  This function lets caller to modify the display arrangement for selected source display handle in any of the nview modes.
//!  It also allows to modify or extend the source display in dualview mode.
//!   \note Maps the selected source to the associated target Ids.
//!   \note Display PATH with this API is limited to single GPU. DUALVIEW across GPUs cannot be enabled with this API. 
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_SetDisplayConfig.
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 95
//!
//! \param [in]  hNvDisplay   NVIDIA Display selection. #NVAPI_DEFAULT_HANDLE is not allowed, it has to be a handle enumerated with 
//!                           NvAPI_EnumNVidiaDisplayHandle().
//! \param [in]  pPathInfo    Pointer to array of NV_VIEW_PATH_INFO, specifying device properties in this view.
//!                           The first device entry in the array is the physical primary.
//!                           The device entry with the lowest source id is the desktop primary.
//! \param [in]  pathCount    Count of paths specified in pPathInfo.
//! \param [in]  displayView  Display view selected from NV_TARGET_VIEW_MODE.
//!
//! \retval  NVAPI_OK                Completed request
//! \retval  NVAPI_ERROR             Miscellaneous error occurred
//! \retval  NVAPI_INVALID_ARGUMENT  Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup dispcontrol
#define NVAPI_MAX_DISPLAY_PATH  NVAPI_MAX_VIEW_TARGET

//! \ingroup dispcontrol
#define NVAPI_ADVANCED_MAX_DISPLAY_PATH  NVAPI_ADVANCED_MAX_VIEW_TARGET



//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PATH_INFO.
typedef struct
{
    NvU32                   deviceMask;     //!< (IN) Device mask
    NvU32                   sourceId;       //!< (IN) Values will be based on the number of heads exposed per GPU(0, 1?)
    NvU32                   bPrimary:1;     //!< (IN/OUT) Indicates if this is the GPU's primary view target. This is not the desktop GDI primary.
                                            //!< NvAPI_SetViewEx() automatically selects the first target in NV_DISPLAY_PATH_INFO index 0 as the GPU's primary view.
    NV_GPU_CONNECTOR_TYPE   connector;      //!< (IN) Specify connector type. For TV only.

    // source mode information
    NvU32                   width;          //!< (IN) Width of the mode
    NvU32                   height;         //!< (IN) Height of the mode
    NvU32                   depth;          //!< (IN) Depth of the mode
    NV_FORMAT               colorFormat;    //!<      Color format if it needs to be specified. Not used now.

    //rotation setting of the mode
    NV_ROTATE               rotation;       //!< (IN) Rotation setting.

    // the scaling mode
    NV_SCALING              scaling;        //!< (IN) Scaling setting

    // Timing info
    NvU32                   refreshRate;    //!< (IN) Refresh rate of the mode
    NvU32                   interlaced:1;   //!< (IN) Interlaced mode flag

    NV_DISPLAY_TV_FORMAT    tvFormat;       //!< (IN) To choose the last TV format set this value to NV_DISPLAY_TV_FORMAT_NONE

    // Windows desktop position
    NvU32                   posx;           //!< (IN/OUT) X-offset of this display on the Windows desktop
    NvU32                   posy;           //!< (IN/OUT) Y-offset of this display on the Windows desktop
    NvU32                   bGDIPrimary:1;  //!< (IN/OUT) Indicates if this is the desktop GDI primary.

    NvU32                   bForceModeSet:1;//!< (IN) Used only on Win7 and higher during a call to NvAPI_SetViewEx(). Turns off optimization & forces OS to set supplied mode.
    NvU32                   bFocusDisplay:1;//!< (IN) If set, this display path should have the focus after the GPU topology change
    NvU32                   gpuId:24;       //!< (IN) the physical display/target Gpu id which is the owner of the scan out (for SLI multimon, display from the slave Gpu)

} NV_DISPLAY_PATH;

//! \ingroup dispcontrol
//! Used in NvAPI_SetViewEx() and NvAPI_GetViewEx().
typedef struct
{
    NvU32 version;     //!< (IN) Structure version
    NvU32 count;       //!< (IN) Path count
    NV_DISPLAY_PATH path[NVAPI_MAX_DISPLAY_PATH];
} NV_DISPLAY_PATH_INFO_V3; 

//! \ingroup dispcontrol
//! Used in NvAPI_SetViewEx() and NvAPI_GetViewEx().
typedef struct
{
    NvU32 version;     //!< (IN) Structure version
    NvU32 count;       //!< (IN) Path count
    NV_DISPLAY_PATH path[NVAPI_ADVANCED_MAX_DISPLAY_PATH];
} NV_DISPLAY_PATH_INFO; 

//! \addtogroup dispcontrol
//! Macro for constructing the version fields of NV_DISPLAY_PATH_INFO
//! @{
#define NV_DISPLAY_PATH_INFO_VER  NV_DISPLAY_PATH_INFO_VER4
#define NV_DISPLAY_PATH_INFO_VER4 MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,4)
#define NV_DISPLAY_PATH_INFO_VER3 MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,3)
#define NV_DISPLAY_PATH_INFO_VER2 MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,2)
#define NV_DISPLAY_PATH_INFO_VER1 MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,1)
//! @}


//! \ingroup dispcontrol
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_SetDisplayConfig.")
NVAPI_INTERFACE NvAPI_SetViewEx(NvDisplayHandle hNvDisplay, NV_DISPLAY_PATH_INFO *pPathInfo, NV_TARGET_VIEW_MODE displayView);



///////////////////////////////////////////////////////////////////////////////
// SetDisplayConfig/GetDisplayConfig
///////////////////////////////////////////////////////////////////////////////
//! \ingroup dispcontrol

typedef struct _NV_POSITION
{
    NvS32   x;
    NvS32   y;
} NV_POSITION;

//! \ingroup dispcontrol
typedef struct _NV_RESOLUTION
{
    NvU32   width;
    NvU32   height;
    NvU32   colorDepth;
} NV_RESOLUTION;

//! \ingroup dispcontrol
typedef struct _NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_V1
{
    NvU32                   version;

    // Rotation and Scaling
    NV_ROTATE               rotation;       //!< (IN) rotation setting.
    NV_SCALING              scaling;        //!< (IN) scaling setting.

    // Refresh Rate
    NvU32                   refreshRate1K;  //!< (IN) Non-interlaced Refresh Rate of the mode, multiplied by 1000, 0 = ignored
                                            //!< This is the value which driver reports to the OS.
    // Flags
    NvU32                   interlaced:1;   //!< (IN) Interlaced mode flag, ignored if refreshRate == 0
    NvU32                   primary:1;      //!< (IN) Declares primary display in clone configuration. This is *NOT* GDI Primary.
                                            //!< Only one target can be primary per source. If no primary is specified, the first 
                                            //!< target will automatically be primary.
#ifdef NV_PAN_AND_SCAN_DEFINED 
    NvU32                   isPanAndScanTarget:1; //!< Whether on this target Pan and Scan is enabled or has to be enabled. Valid only 
                                                  //!< when the target is part of clone topology.
    NvU32                   reserved:29;  
#else
    NvU32                   reserved:30;  
#endif
    // TV format information
    NV_GPU_CONNECTOR_TYPE   connector;      //!< Specify connector type. For TV only, ignored if tvFormat == NV_DISPLAY_TV_FORMAT_NONE
    NV_DISPLAY_TV_FORMAT    tvFormat;       //!< (IN) to choose the last TV format set this value to NV_DISPLAY_TV_FORMAT_NONE
                                            //!< In case of NvAPI_DISP_GetDisplayConfig(), this field will indicate the currently applied TV format;
                                            //!< if no TV format is applied, this field will have NV_DISPLAY_TV_FORMAT_NONE value.
                                            //!< In case of NvAPI_DISP_SetDisplayConfig(), this field should only be set in case of TVs; 
                                            //!< for other displays this field will be ignored and resolution & refresh rate specified in input will be used to apply the TV format.

    // Backend (raster) timing standard
    NV_TIMING_OVERRIDE      timingOverride;     //!< Ignored if timingOverride == NV_TIMING_OVERRIDE_CURRENT
    NV_TIMING               timing;             //!< Scan out timing, valid only if timingOverride == NV_TIMING_OVERRIDE_CUST
                                                //!< The value NV_TIMING::NV_TIMINGEXT::rrx1k is obtained from the EDID. The driver may 
                                                //!< tweak this value for HDTV, stereo, etc., before reporting it to the OS. 
} NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_V1;

//! \ingroup dispcontrol
typedef NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_V1 NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO;

//! \ingroup dispcontrol
#define NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER1     MAKE_NVAPI_VERSION(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_V1,1)

//! \ingroup dispcontrol
#define NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER      NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER1

//! \ingroup dispcontrol
typedef struct _NV_DISPLAYCONFIG_PATH_TARGET_INFO_V1
{
    NvU32                                           displayId;  //!< Display ID
    NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*     details;    //!< May be NULL if no advanced settings are required. NULL for Non-NVIDIA Display.
} NV_DISPLAYCONFIG_PATH_TARGET_INFO_V1;

//! \ingroup dispcontrol
typedef struct _NV_DISPLAYCONFIG_PATH_TARGET_INFO_V2
{
    NvU32                                           displayId;  //!< Display ID
    NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*     details;    //!< May be NULL if no advanced settings are required
    NvU32                                           targetId;   //!< Windows CCD target ID. Must be present only for non-NVIDIA adapter, for NVIDIA adapter this parameter is ignored.
} NV_DISPLAYCONFIG_PATH_TARGET_INFO_V2;


//! \ingroup dispcontrol
//! As version is not defined for this structure, we will be using version of NV_DISPLAYCONFIG_PATH_INFO
typedef NV_DISPLAYCONFIG_PATH_TARGET_INFO_V2 NV_DISPLAYCONFIG_PATH_TARGET_INFO; 


//! \ingroup dispcontrol
typedef enum _NV_DISPLAYCONFIG_SPANNING_ORIENTATION
{
    NV_DISPLAYCONFIG_SPAN_NONE          = 0,
    NV_DISPLAYCONFIG_SPAN_HORIZONTAL    = 1,
    NV_DISPLAYCONFIG_SPAN_VERTICAL      = 2,
} NV_DISPLAYCONFIG_SPANNING_ORIENTATION;

//! \ingroup dispcontrol
typedef struct _NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V1
{
    NV_RESOLUTION                           resolution;
    NV_FORMAT                               colorFormat;                //!< Ignored at present, must be NV_FORMAT_UNKNOWN (0)
    NV_POSITION                             position;                   //!< Is all positions are 0 or invalid, displays will be automatically
                                                                        //!< positioned from left to right with GDI Primary at 0,0, and all
                                                                        //!< other displays in the order of the path array.
    NV_DISPLAYCONFIG_SPANNING_ORIENTATION   spanningOrientation;        //!< Spanning is only supported on XP
    NvU32                                   bGDIPrimary : 1;
    NvU32                                   bSLIFocus : 1;
    NvU32                                   reserved : 30;              //!< Must be 0
} NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V1;

//! \ingroup dispcontrol
//! As version is not defined for this structure we will be using version of NV_DISPLAYCONFIG_PATH_INFO
typedef NV_DISPLAYCONFIG_SOURCE_MODE_INFO_V1 NV_DISPLAYCONFIG_SOURCE_MODE_INFO; 

//! \ingroup dispcontrol
typedef struct _NV_DISPLAYCONFIG_PATH_INFO_V1
{
    NvU32                                   version;
    NvU32                                   reserved_sourceId;     	//!< This field is reserved. There is ongoing debate if we need this field.
                                                                        //!< Identifies sourceIds used by Windows. If all sourceIds are 0, 
                                                                        //!< these will be computed automatically.
    NvU32                                   targetInfoCount;            //!< Number of elements in targetInfo array
    NV_DISPLAYCONFIG_PATH_TARGET_INFO_V1*   targetInfo;
    NV_DISPLAYCONFIG_SOURCE_MODE_INFO*      sourceModeInfo;             //!< May be NULL if mode info is not important
} NV_DISPLAYCONFIG_PATH_INFO_V1;

//! \ingroup dispcontrol
//! This define is temporary and must be removed once DVS failure is fixed.
#define _NV_DISPLAYCONFIG_PATH_INFO_V2 _NV_DISPLAYCONFIG_PATH_INFO

//! \ingroup dispcontrol
typedef struct _NV_DISPLAYCONFIG_PATH_INFO_V2
{
    NvU32                                   version;
    union {
        NvU32                                   sourceId;            	//!< Identifies sourceId used by Windows CCD. This can be optionally set.
        NvU32                                   reserved_sourceId;      //!< Only for compatibility
    };

    NvU32                                   targetInfoCount;            //!< Number of elements in targetInfo array
    NV_DISPLAYCONFIG_PATH_TARGET_INFO*      targetInfo;
    NV_DISPLAYCONFIG_SOURCE_MODE_INFO*      sourceModeInfo;             //!< May be NULL if mode info is not important
    NvU32                                   IsNonNVIDIAAdapter : 1;     //!< True for non-NVIDIA adapter.
    NvU32                                   reserved : 31;              //!< Must be 0
    void                                    *pOSAdapterID;              //!< Used by Non-NVIDIA adapter for pointer to OS Adapter of LUID 
                                                                        //!< type, type casted to void *.
} NV_DISPLAYCONFIG_PATH_INFO_V2;

//! \ingroup dispcontrol
typedef NV_DISPLAYCONFIG_PATH_INFO_V2 NV_DISPLAYCONFIG_PATH_INFO;

//! \ingroup dispcontrol
#define NV_DISPLAYCONFIG_PATH_INFO_VER1                 MAKE_NVAPI_VERSION(NV_DISPLAYCONFIG_PATH_INFO_V1,1)

//! \ingroup dispcontrol
#define NV_DISPLAYCONFIG_PATH_INFO_VER2                 MAKE_NVAPI_VERSION(NV_DISPLAYCONFIG_PATH_INFO_V2,2)

//! \ingroup dispcontrol
#define NV_DISPLAYCONFIG_PATH_INFO_VER                  NV_DISPLAYCONFIG_PATH_INFO_VER2

//! \ingroup dispcontrol
typedef enum _NV_DISPLAYCONFIG_FLAGS
{
    NV_DISPLAYCONFIG_VALIDATE_ONLY          = 0x00000001,
    NV_DISPLAYCONFIG_SAVE_TO_PERSISTENCE    = 0x00000002, 
    NV_DISPLAYCONFIG_DRIVER_RELOAD_ALLOWED  = 0x00000004,               //!< Driver reload is permitted if necessary
    NV_DISPLAYCONFIG_FORCE_MODE_ENUMERATION = 0x00000008,               //!< Refresh OS mode list.
} NV_DISPLAYCONFIG_FLAGS;


#define NVAPI_UNICODE_STRING_MAX                             2048
#define NVAPI_BINARY_DATA_MAX                                4096

typedef NvU16 NvAPI_UnicodeString[NVAPI_UNICODE_STRING_MAX];
typedef const NvU16 *NvAPI_LPCWSTR;
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetDisplayDriverVersion
//! \fn NvAPI_GetDisplayDriverVersion(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION *pVersion)
//! This function returns a struct that describes aspects of the display driver
//!                build.
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_SYS_GetDriverAndBranchVersion.
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 80
//!
//! \param [in]  hNvDisplay NVIDIA display handle.
//! \param [out] pVersion Pointer to NV_DISPLAY_DRIVER_VERSION struc
//!
//! \retval NVAPI_ERROR
//! \retval NVAPI_OK
///////////////////////////////////////////////////////////////////////////////

//! \ingroup driverapi
//! Used in NvAPI_GetDisplayDriverVersion()
typedef struct _NV_DISPLAY_DRIVER_VERSION
{
    NvU32 version;             // Structure version
    NvU32 drvVersion;
    NvU32 bldChangeListNum;
    NvAPI_ShortString szBuildBranchString;
    NvAPI_ShortString szAdapterString;
} NV_DISPLAY_DRIVER_VERSION;

//! \ingroup driverapi
#define NV_DISPLAY_DRIVER_VERSION_VER  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_VERSION,1)


//! \ingroup driverapi
//__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_SYS_GetDriverAndBranchVersion.")
typedef NvAPI_Status (__cdecl *_NvAPI_GetDisplayDriverVersion)(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION *pVersion);
_NvAPI_GetDisplayDriverVersion NvAPI_GetDisplayDriverVersion;

//NVAPI_INTERFACE NvAPI_GetDisplayDriverVersion(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION *pVersion);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_OGL_ExpertModeSet[Get]
//
//! \name NvAPI_OGL_ExpertModeSet[Get] Functions
//@{
//!  This function configures OpenGL Expert Mode, an API usage feedback and
//!  advice reporting mechanism. The effects of this call are
//!  applied only to the current context, and are reset to the
//!  defaults when the context is destroyed.
//!
//!  \note  This feature is valid at runtime only when GLExpert
//!         functionality has been built into the OpenGL driver
//!         installed on the system. All Windows Vista OpenGL
//!         drivers provided by NVIDIA have this instrumentation
//!         included by default. Windows XP, however, requires a
//!         special display driver available with the NVIDIA
//!         PerfSDK found at developer.nvidia.com.
//!
//!  \note These functions are valid only for the current OpenGL
//!        context. Calling these functions prior to creating a
//!        context and calling MakeCurrent with it will result
//!        in errors and undefined behavior.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \param   expertDetailMask  Mask made up of NVAPI_OGLEXPERT_DETAIL bits,
//!                            this parameter specifies the detail level in
//!                            the feedback stream.
//!
//! \param   expertReportMask  Mask made up of NVAPI_OGLEXPERT_REPORT bits,
//!                            this parameter specifies the areas of
//!                            functional interest.
//!
//! \param   expertOutputMask  Mask made up of NVAPI_OGLEXPERT_OUTPUT bits,
//!                            this parameter specifies the feedback output
//!                            location.
//!
//! \param   expertCallback    Used in conjunction with OUTPUT_TO_CALLBACK,
//!                            this is a simple callback function the user
//!                            may use to obtain the feedback stream. The
//!                            function will be called once per fully
//!                            qualified feedback stream extry.
//!
//! \retval  NVAPI_API_NOT_INTIALIZED          NVAPI not initialized
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND     No NVIDIA GPU found
//! \retval  NVAPI_OPENGL_CONTEXT_NOT_CURRENT  No NVIDIA OpenGL context
//!                                            which supports GLExpert
//!                                            has been made current
//! \retval  NVAPI_ERROR                       OpenGL driver failed to load properly
//! \retval  NVAPI_OK                          Success
//
///////////////////////////////////////////////////////////////////////////////

//! \addtogroup oglapi
//! @{
#define NVAPI_OGLEXPERT_DETAIL_NONE                 0x00000000
#define NVAPI_OGLEXPERT_DETAIL_ERROR                0x00000001
#define NVAPI_OGLEXPERT_DETAIL_SWFALLBACK           0x00000002
#define NVAPI_OGLEXPERT_DETAIL_BASIC_INFO           0x00000004
#define NVAPI_OGLEXPERT_DETAIL_DETAILED_INFO        0x00000008
#define NVAPI_OGLEXPERT_DETAIL_PERFORMANCE_WARNING  0x00000010
#define NVAPI_OGLEXPERT_DETAIL_QUALITY_WARNING      0x00000020
#define NVAPI_OGLEXPERT_DETAIL_USAGE_WARNING        0x00000040
#define NVAPI_OGLEXPERT_DETAIL_ALL                  0xFFFFFFFF

#define NVAPI_OGLEXPERT_REPORT_NONE                 0x00000000
#define NVAPI_OGLEXPERT_REPORT_ERROR                0x00000001
#define NVAPI_OGLEXPERT_REPORT_SWFALLBACK           0x00000002
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_VERTEX      0x00000004
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_GEOMETRY    0x00000008
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_XFB         0x00000010
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_RASTER      0x00000020
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_FRAGMENT    0x00000040
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_ROP         0x00000080
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_FRAMEBUFFER 0x00000100
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_PIXEL       0x00000200
#define NVAPI_OGLEXPERT_REPORT_PIPELINE_TEXTURE     0x00000400
#define NVAPI_OGLEXPERT_REPORT_OBJECT_BUFFEROBJECT  0x00000800
#define NVAPI_OGLEXPERT_REPORT_OBJECT_TEXTURE       0x00001000
#define NVAPI_OGLEXPERT_REPORT_OBJECT_PROGRAM       0x00002000
#define NVAPI_OGLEXPERT_REPORT_OBJECT_FBO           0x00004000
#define NVAPI_OGLEXPERT_REPORT_FEATURE_SLI          0x00008000
#define NVAPI_OGLEXPERT_REPORT_ALL                  0xFFFFFFFF


#define NVAPI_OGLEXPERT_OUTPUT_TO_NONE       0x00000000
#define NVAPI_OGLEXPERT_OUTPUT_TO_CONSOLE    0x00000001
#define NVAPI_OGLEXPERT_OUTPUT_TO_DEBUGGER   0x00000004
#define NVAPI_OGLEXPERT_OUTPUT_TO_CALLBACK   0x00000008
#define NVAPI_OGLEXPERT_OUTPUT_TO_ALL        0xFFFFFFFF

//! @}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION TYPE: NVAPI_OGLEXPERT_CALLBACK
//
//!   DESCRIPTION: Used in conjunction with OUTPUT_TO_CALLBACK, this is a simple 
//!                callback function the user may use to obtain the feedback 
//!                stream. The function will be called once per fully qualified 
//!                feedback stream entry.
//!
//!   \param   categoryId    Contains the bit from the NVAPI_OGLEXPERT_REPORT 
//!                          mask that corresponds to the current message
//!   \param   messageId     Unique ID for the current message
//!   \param   detailLevel   Contains the bit from the NVAPI_OGLEXPERT_DETAIL
//!                          mask that corresponds to the current message
//!   \param   objectId      Unique ID of the object that corresponds to the
//!                          current message
//!   \param   messageStr    Text string from the current message
//!
//!   \ingroup oglapi
///////////////////////////////////////////////////////////////////////////////
typedef void (* NVAPI_OGLEXPERT_CALLBACK) (unsigned int categoryId, unsigned int messageId, unsigned int detailLevel, int objectId, const char *messageStr);



//! \ingroup oglapi
//! SUPPORTED OS:  Windows XP and higher
//!
NVAPI_INTERFACE NvAPI_OGL_ExpertModeSet(NvU32 expertDetailLevel,
                                        NvU32 expertReportMask,
                                        NvU32 expertOutputMask,
                     NVAPI_OGLEXPERT_CALLBACK expertCallback);

//! \addtogroup oglapi
//! SUPPORTED OS:  Windows XP and higher
//!
NVAPI_INTERFACE NvAPI_OGL_ExpertModeGet(NvU32 *pExpertDetailLevel,
                                        NvU32 *pExpertReportMask,
                                        NvU32 *pExpertOutputMask,
                     NVAPI_OGLEXPERT_CALLBACK *pExpertCallback);

//@}
///////////////////////////////////////////////////////////////////////////////
//
//! \name NvAPI_OGL_ExpertModeDefaultsSet[Get] Functions
//!
//@{
//!  This function configures OpenGL Expert Mode global defaults. These settings
//!  apply to any OpenGL application which starts up after these
//!  values are applied (i.e. these settings *do not* apply to
//!  currently running applications).
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \param   expertDetailLevel Value which specifies the detail level in
//!                            the feedback stream. This is a mask made up
//!                            of NVAPI_OGLEXPERT_LEVEL bits.
//!
//! \param   expertReportMask  Mask made up of NVAPI_OGLEXPERT_REPORT bits,
//!                            this parameter specifies the areas of
//!                            functional interest.
//!
//! \param   expertOutputMask  Mask made up of NVAPI_OGLEXPERT_OUTPUT bits,
//!                            this parameter specifies the feedback output
//!                            location. Note that using OUTPUT_TO_CALLBACK
//!                            here is meaningless and has no effect, but
//!                            using it will not cause an error.
//!
//! \return  ::NVAPI_ERROR or ::NVAPI_OK
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup oglapi
//! SUPPORTED OS:  Windows XP and higher
//!
NVAPI_INTERFACE NvAPI_OGL_ExpertModeDefaultsSet(NvU32 expertDetailLevel,
                                                NvU32 expertReportMask,
                                                NvU32 expertOutputMask);

//! \addtogroup oglapi
//! SUPPORTED OS:  Windows XP and higher
//!
NVAPI_INTERFACE NvAPI_OGL_ExpertModeDefaultsGet(NvU32 *pExpertDetailLevel,
                                                NvU32 *pExpertReportMask,
                                                NvU32 *pExpertOutputMask);
//@}




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumTCCPhysicalGPUs
//
//! This function returns an array of physical GPU handles that are in TCC Mode.
//! Each handle represents a physical GPU present in the system in TCC Mode.
//! That GPU may not be visible to the OS directly.
//!
//! The array nvGPUHandle will be filled with physical GPU handle values. The returned
//! gpuCount determines how many entries in the array are valid.
//!
//! NOTE: Handles enumerated by this API are only valid for NvAPIs that are tagged as TCC_SUPPORTED
//!       If handle is passed to any other API, it will fail with NVAPI_INVALID_HANDLE
//!
//!       For WDDM GPU handles please use NvAPI_EnumPhysicalGPUs()
//!
//! SUPPORTED OS:  Windows Vista and higher,  Mac OS X
//!
//!
//! 
//! \param [out]   nvGPUHandle      Physical GPU array that will contain all TCC Physical GPUs
//! \param [out]   pGpuCount        count represent the number of valid entries in nvGPUHandle
//!  
//!
//! \retval NVAPI_INVALID_ARGUMENT         nvGPUHandle or pGpuCount is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumTCCPhysicalGPUs( NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumLogicalGPUs
//
//! This function returns an array of logical GPU handles.
//!
//! Each handle represents one or more GPUs acting in concert as a single graphics device.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! The array nvGPUHandle will be filled with logical GPU handle values.  The returned
//! gpuCount determines how many entries in the array are valid.
//!
//! \note All logical GPUs handles get invalidated on a GPU topology change, so the calling 
//!       application is required to renum the logical GPU handles to get latest physical handle
//!       mapping after every GPU topology change activated by a call to NvAPI_SetGpuTopologies().
//!
//! To detect if SLI rendering is enabled, use NvAPI_D3D_GetCurrentSLIState().
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         nvGPUHandle or pGpuCount is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumLogicalGPUs(NvLogicalGpuHandle nvGPUHandle[NVAPI_MAX_LOGICAL_GPUS], NvU32 *pGpuCount);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUsFromDisplay
//
//! This function returns an array of physical GPU handles associated with the specified display.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! The array nvGPUHandle will be filled with physical GPU handle values.  The returned
//! gpuCount determines how many entries in the array are valid.
//!
//! If the display corresponds to more than one physical GPU, the first GPU returned
//! is the one with the attached active output.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvDisp is not valid; nvGPUHandle or pGpuCount is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  no NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GetPhysicalGPUsFromDisplay(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);

typedef NvAPI_Status (__cdecl *_NvAPI_GetPhysicalGPUsFromDisplay)(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
_NvAPI_GetPhysicalGPUsFromDisplay NvAPI_GetPhysicalGPUsFromDisplay;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUFromUnAttachedDisplay
//
//! This function returns a physical GPU handle associated with the specified unattached display.
//! The source GPU is a physical render GPU which renders the frame buffer but may or may not drive the scan out.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvUnAttachedDisp is not valid or pPhysicalGpu is NULL.
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetPhysicalGPUFromUnAttachedDisplay(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvPhysicalGpuHandle *pPhysicalGpu);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetLogicalGPUFromDisplay
//
//! This function returns the logical GPU handle associated with the specified display.
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvDisp is not valid; pLogicalGPU is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetLogicalGPUFromDisplay(NvDisplayHandle hNvDisp, NvLogicalGpuHandle *pLogicalGPU);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetLogicalGPUFromPhysicalGPU
//
//! This function returns the logical GPU handle associated with specified physical GPU handle.
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         hPhysicalGPU is not valid; pLogicalGPU is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetLogicalGPUFromPhysicalGPU(NvPhysicalGpuHandle hPhysicalGPU, NvLogicalGpuHandle *pLogicalGPU);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUsFromLogicalGPU
//
//!  This function returns the physical GPU handles associated with the specified logical GPU handle.
//!  At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//!  The array hPhysicalGPU will be filled with physical GPU handle values.  The returned
//!  gpuCount determines how many entries in the array are valid.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT             hLogicalGPU is not valid; hPhysicalGPU is NULL
//! \retval NVAPI_OK                           One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND      No NVIDIA GPU driving a display was found
//! \retval NVAPI_EXPECTED_LOGICAL_GPU_HANDLE  hLogicalGPU was not a logical GPU handle
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetPhysicalGPUsFromLogicalGPU(NvLogicalGpuHandle hLogicalGPU,NvPhysicalGpuHandle hPhysicalGPU[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
   
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetGpuCoreCount
//
//!   DESCRIPTION: Retrieves the total number of cores defined for a GPU.
//!                Returns 0 on architectures that don't define GPU cores.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \retval ::NVAPI_INVALID_ARGUMENT              pCount is NULL
//! \retval ::NVAPI_OK                            *pCount is set
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND       no NVIDIA GPU driving a display was found
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \retval ::NVAPI_NOT_SUPPORTED                 API call is not supported on current architecture
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetGpuCoreCount(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pCount);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetGpuCoreCount)(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Out_ NvU32* pCount);
_NvAPI_GPU_GetGpuCoreCount NvAPI_GPU_GetGpuCoreCount;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAllOutputs
//
//!  This function returns set of all GPU-output identifiers as a bitmask.
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetAllDisplayIds.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 85
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL.
//! \retval   NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetAllDisplayIds.")
NVAPI_INTERFACE NvAPI_GPU_GetAllOutputs(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pOutputsMask);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedOutputs
//
//! This function is the same as NvAPI_GPU_GetAllOutputs() but returns only the set of GPU output 
//! identifiers that are connected to display devices.
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL.
//! \retval   NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.")
NVAPI_INTERFACE NvAPI_GPU_GetConnectedOutputs(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedSLIOutputs
//
//!   DESCRIPTION: This function is the same as NvAPI_GPU_GetConnectedOutputs() but returns only the set of GPU-output 
//!                identifiers that can be selected in an SLI configuration. 
//!                 NOTE: This function matches NvAPI_GPU_GetConnectedOutputs()
//!                 - On systems which are not SLI capable.
//!                 - If the queried GPU is not part of a valid SLI group.
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 170
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval   NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//! 
//! \ingroup gpu  
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.")
NVAPI_INTERFACE NvAPI_GPU_GetConnectedSLIOutputs(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);




//! \ingroup gpu
typedef enum _NV_MONITOR_CONN_TYPE
{
    NV_MONITOR_CONN_TYPE_UNINITIALIZED = 0,
    NV_MONITOR_CONN_TYPE_VGA,
    NV_MONITOR_CONN_TYPE_COMPONENT,
    NV_MONITOR_CONN_TYPE_SVIDEO,
    NV_MONITOR_CONN_TYPE_HDMI,
    NV_MONITOR_CONN_TYPE_DVI,
    NV_MONITOR_CONN_TYPE_LVDS,
    NV_MONITOR_CONN_TYPE_DP,
    NV_MONITOR_CONN_TYPE_COMPOSITE,
    NV_MONITOR_CONN_TYPE_UNKNOWN =  -1
} NV_MONITOR_CONN_TYPE;



//! \addtogroup gpu
//! @{
#define NV_GPU_CONNECTED_IDS_FLAG_UNCACHED          NV_BIT(0) //!< Get uncached connected devices
#define NV_GPU_CONNECTED_IDS_FLAG_SLI               NV_BIT(1) //!< Get devices such that those can be selected in an SLI configuration
#define NV_GPU_CONNECTED_IDS_FLAG_LIDSTATE          NV_BIT(2) //!< Get devices such that to reflect the Lid State
#define NV_GPU_CONNECTED_IDS_FLAG_FAKE              NV_BIT(3) //!< Get devices that includes the fake connected monitors
#define NV_GPU_CONNECTED_IDS_FLAG_EXCLUDE_MST       NV_BIT(4) //!< Excludes devices that are part of the multi stream topology.               

//! @}

//! \ingroup gpu
typedef struct _NV_GPU_DISPLAYIDS
{
    NvU32    version;
    NV_MONITOR_CONN_TYPE connectorType; //!< out: vga, tv, dvi, hdmi and dp. This is reserved for future use and clients should not rely on this information. Instead get the 
                                        //!< GPU connector type from NvAPI_GPU_GetConnectorInfo/NvAPI_GPU_GetConnectorInfoEx
    NvU32    displayId;                 //!< this is a unique identifier for each device
    NvU32    isDynamic:1;               //!< if bit is set then this display is part of MST topology and it's a dynamic
    NvU32    isMultiStreamRootNode:1;   //!< if bit is set then this displayID belongs to a multi stream enabled connector(root node). Note that when multi stream is enabled and 
                                        //!< a single multi stream capable monitor is connected to it, the monitor will share the display id with the RootNode. 
                                        //!< When there is more than one monitor connected in a multi stream topology, then the root node will have a separate displayId.
    NvU32    isActive:1;                //!< if bit is set then this display is being actively driven
    NvU32    isCluster:1;               //!< if bit is set then this display is the representative display
    NvU32    isOSVisible:1;             //!< if bit is set, then this display is reported to the OS
    NvU32    isWFD:1;                   //!< if bit is set, then this display is wireless 
    NvU32    isConnected:1;             //!< if bit is set, then this display is connected
    NvU32    reserved: 22;              //!< must be zero
} NV_GPU_DISPLAYIDS;

//! \ingroup gpu
//! Macro for constructing the version field of ::_NV_GPU_DISPLAYIDS
#define NV_GPU_DISPLAYIDS_VER1          MAKE_NVAPI_VERSION(NV_GPU_DISPLAYIDS,1)

#define NV_GPU_DISPLAYIDS_VER NV_GPU_DISPLAYIDS_VER1

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedDisplayIds
//
//! \code
//!   DESCRIPTION: Due to space limitation NvAPI_GPU_GetConnectedOutputs can return maximum 32 devices, but 
//!                this is no longer true for DPMST. NvAPI_GPU_GetConnectedDisplayIds will return all 
//!                the connected display devices in the form of displayIds for the associated hPhysicalGpu.
//!                This function can accept set of flags to request cached, uncached, sli and lid to get the connected devices.
//!                Default value for flags will be cached .
//! HOW TO USE: 1) for each PhysicalGpu, make a call to get the number of connected displayId's 
//!                using NvAPI_GPU_GetConnectedDisplayIds by passing the pDisplayIds as NULL
//!                On call success:
//!             2) Allocate memory based on pDisplayIdCount then make a call NvAPI_GPU_GetConnectedDisplayIds to populate DisplayIds
//! SUPPORTED OS:  Windows XP and higher
//!
//! PARAMETERS:     hPhysicalGpu (IN)  - GPU selection
//!                 flags        (IN)  - One or more defines from NV_GPU_CONNECTED_IDS_FLAG_* as valid flags. 
//!                 pDisplayIds  (IN/OUT) - Pointer to an NV_GPU_DISPLAYIDS struct, each entry represents a one displayID and its attributes
//!                 pDisplayIdCount(OUT)- Number of displayId's.
//!
//! RETURN STATUS: NVAPI_INVALID_ARGUMENT: hPhysicalGpu or pDisplayIds or pDisplayIdCount is NULL
//!                NVAPI_OK: *pDisplayIds contains a set of GPU-output identifiers
//!                NVAPI_NVIDIA_DEVICE_NOT_FOUND: no NVIDIA GPU driving a display was found
//!                NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//! \endcode
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetConnectedDisplayIds(_In_ NvPhysicalGpuHandle hPhysicalGpu,  __inout_ecount_part_opt(*pDisplayIdCount, *pDisplayIdCount) NV_GPU_DISPLAYIDS* pDisplayIds, _Inout_ NvU32* pDisplayIdCount, _In_ NvU32 flags);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAllDisplayIds
//
//!   DESCRIPTION: This API returns display IDs for all possible outputs on the GPU.
//!                For DPMST connector, it will return display IDs for all the video sinks in the topology. \n
//! HOW TO USE: 1. The first call should be made to get the all display ID count. To get the display ID count, send in \n
//!                  a) hPhysicalGpu    - a valid GPU handle(enumerated using NvAPI_EnumPhysicalGPUs()) as input,      \n
//!                  b) pDisplayIds     - NULL, as we just want to get the display ID count.                           \n 
//!                  c) pDisplayIdCount - a valid pointer to NvU32, whose value is set to ZERO.                        \n
//!                If all parameters are correct and this call is successful, this call will return the display ID's count. \n
//!             2. To get the display ID array, make the second call to NvAPI_GPU_GetAllDisplayIds() with              \n
//!                  a) hPhysicalGpu    - should be same value which was sent in first call,                           \n
//!                  b) pDisplayIds     - pointer to the display ID array allocated by caller based on display ID count,    \n 
//!                                       eg. malloc(sizeof(NV_GPU_DISPLAYIDS) * pDisplayIdCount).                     \n
//!                  c) pDisplayIdCount - a valid pointer to NvU32. This indicates for how many display IDs            \n
//!                                       the memory is allocated(pDisplayIds) by the caller.                          \n
//!                If all parameters are correct and this call is successful, this call will return the display ID array and actual
//!                display ID count (which was obtained in the first call to NvAPI_GPU_GetAllDisplayIds). If the input display ID count is
//!                less than the actual display ID count, it will overwrite the input and give the pDisplayIdCount as actual count and the
//!                API will return NVAPI_INSUFFICIENT_BUFFER.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]     hPhysicalGpu         GPU selection.
//! \param [in,out] DisplayIds           Pointer to an array of NV_GPU_DISPLAYIDS structures, each entry represents one displayID 
//!                                      and its attributes.
//! \param [in,out] pDisplayIdCount      As input, this parameter indicates the number of display's id's for which caller has 
//!                                      allocated the memory. As output, it will return the actual number of display IDs.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!          specific meaning for this API, they are listed below.
//!
//! \retval  NVAPI_INSUFFICIENT_BUFFER  When the input buffer(pDisplayIds) is less than the actual number of display IDs, this API 
//!                                     will return NVAPI_INSUFFICIENT_BUFFER. 
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetAllDisplayIds(_In_ NvPhysicalGpuHandle hPhysicalGpu, __inout_ecount_part_opt(*pDisplayIdCount, *pDisplayIdCount) NV_GPU_DISPLAYIDS* pDisplayIds, _Inout_ NvU32* pDisplayIdCount);




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedOutputsWithLidState
//
//!  This function is similar to NvAPI_GPU_GetConnectedOutputs(), and returns the connected display identifiers that are connected 
//!  as an output mask but unlike NvAPI_GPU_GetConnectedOutputs() this API "always" reflects the Lid State in the output mask.
//!  Thus if you expect the LID close state to be available in the connection mask use this API.
//!  - If LID is closed then this API will remove the LID panel from the connected display identifiers. 
//!  - If LID is open then this API will reflect the LID panel in the connected display identifiers. 
//!
//! \note This API should be used on notebook systems and on systems where the LID state is required in the connection 
//!       output mask. On desktop systems the returned identifiers will match NvAPI_GPU_GetConnectedOutputs().
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 95
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval  NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.")
NVAPI_INTERFACE NvAPI_GPU_GetConnectedOutputsWithLidState(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedSLIOutputsWithLidState
//
//!   DESCRIPTION: This function is the same as NvAPI_GPU_GetConnectedOutputsWithLidState() but returns only the set
//!                of GPU-output identifiers that can be selected in an SLI configuration. With SLI disabled,
//!                this function matches NvAPI_GPU_GetConnectedOutputsWithLidState().
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 170
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval  NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_GPU_GetConnectedDisplayIds.")
NVAPI_INTERFACE NvAPI_GPU_GetConnectedSLIOutputsWithLidState(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetSystemType
//
//! \fn NvAPI_GPU_GetSystemType(NvPhysicalGpuHandle hPhysicalGpu, NV_SYSTEM_TYPE *pSystemType)
//!  This function identifies whether the GPU is a notebook GPU or a desktop GPU.
//!       
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 95
//!         
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval  NVAPI_OK                           *pSystemType contains the GPU system type
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu
//! Used in NvAPI_GPU_GetSystemType()
typedef enum _NV_SYSTEM_TYPE
{
    NV_SYSTEM_TYPE_UNKNOWN = 0,
    NV_SYSTEM_TYPE_LAPTOP  = 1,
    NV_SYSTEM_TYPE_DESKTOP = 2,
} NV_SYSTEM_TYPE;



//! \ingroup gpu
NVAPI_INTERFACE NvAPI_GPU_GetSystemType(NvPhysicalGpuHandle hPhysicalGpu, NV_SYSTEM_TYPE *pSystemType);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetActiveOutputs
//
//!  This function is the same as NvAPI_GPU_GetAllOutputs but returns only the set of GPU output 
//!  identifiers that are actively driving display devices.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 85
//!
//! \retval    NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL.
//! \retval    NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers.
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetActiveOutputs(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_SetEDID
//
//!  Thus function sets the EDID data for the specified GPU handle and connection bit mask.
//!  displayOutputId should have exactly 1 bit set to indicate a single display. See \ref handles.
//!  \note The EDID will be cached across the boot session and will be enumerated to the OS in this call.
//!        To remove the EDID set sizeofEDID to zero.
//!        OS and NVAPI connection status APIs will reflect the newly set or removed EDID dynamically.
//!
//!                This feature will NOT be supported on the following boards:
//!                - GeForce
//!                - Quadro VX 
//!                - Tesla  
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 100
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pEDID is NULL; displayOutputId has 0 or > 1 bits set
//! \retval  NVAPI_OK                           *pEDID data was applied to the requested displayOutputId.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle.
//! \retval  NVAPI_NOT_SUPPORTED                 For the above mentioned GPUs
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_SetEDID(NvPhysicalGpuHandle hPhysicalGpu, NvU32 displayOutputId, NV_EDID *pEDID);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetOutputType
//
//! \fn NvAPI_GPU_GetOutputType(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputId, NV_GPU_OUTPUT_TYPE *pOutputType)
//!  This function returns the output type for a specific physical GPU handle and outputId (exactly 1 bit set - see \ref handles).
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \Version Earliest supported ForceWare version: 82.61
//!
//! \retval     NVAPI_INVALID_ARGUMENT              hPhysicalGpu, outputId, or pOutputsMask is NULL; or outputId has > 1 bit set
//! \retval     NVAPI_OK                           *pOutputType contains a NvGpuOutputType value
//! \retval     NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval     NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu
//! used in NvAPI_GPU_GetOutputType()
typedef enum _NV_GPU_OUTPUT_TYPE
{
    NVAPI_GPU_OUTPUT_UNKNOWN  = 0,
    NVAPI_GPU_OUTPUT_CRT      = 1,     //!<  CRT display device
    NVAPI_GPU_OUTPUT_DFP      = 2,     //!<  Digital Flat Panel display device
    NVAPI_GPU_OUTPUT_TV       = 3,     //!<  TV display device
} NV_GPU_OUTPUT_TYPE;




//! \ingroup gpu
NVAPI_INTERFACE NvAPI_GPU_GetOutputType(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputId, NV_GPU_OUTPUT_TYPE *pOutputType);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_ValidateOutputCombination
//
//!  This function determines if a set of GPU outputs can be active 
//!  simultaneously.  While a GPU may have <n> outputs, typically they cannot 
//!  all be active at the same time due to internal resource sharing.
//!
//!  Given a physical GPU handle and a mask of candidate outputs, this call
//!  will return NVAPI_OK if all of the specified outputs can be driven
//!  simultaneously.  It will return NVAPI_INVALID_COMBINATION if they cannot.
//!                
//!  Use NvAPI_GPU_GetAllOutputs() to determine which outputs are candidates.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 85
//!
//! \retval  NVAPI_OK                            Combination of outputs in outputsMask are valid (can be active simultaneously).
//! \retval  NVAPI_INVALID_COMBINATION           Combination of outputs in outputsMask are NOT valid.
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or outputsMask does not have at least 2 bits set.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_ValidateOutputCombination(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputsMask);




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetFullName
//
//!  This function retrieves the full GPU name as an ASCII string - for example, "Quadro FX 1400".
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \return  NVAPI_ERROR or NVAPI_OK
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetFullName(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetFullName)(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);
_NvAPI_GPU_GetFullName NvAPI_GPU_GetFullName;



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPCIIdentifiers
//
//!  This function returns the PCI identifiers associated with this GPU.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \param   DeviceId      The internal PCI device identifier for the GPU.
//! \param   SubSystemId   The internal PCI subsystem identifier for the GPU.
//! \param   RevisionId    The internal PCI device-specific revision identifier for the GPU.
//! \param   ExtDeviceId   The external PCI device identifier for the GPU.
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or an argument is NULL
//! \retval  NVAPI_OK                            Arguments are populated with PCI identifiers
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetPCIIdentifiers)(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Out_ NvU32* pDeviceId, _Out_ NvU32* pSubSystemId, _Out_ NvU32* pRevisionId, _Out_ NvU32* pExtDeviceId);
_NvAPI_GPU_GetPCIIdentifiers NvAPI_GPU_GetPCIIdentifiers;


//! \ingroup gpu
//! Used in NvAPI_GPU_GetGPUType().    
typedef enum _NV_GPU_TYPE
{
    NV_SYSTEM_TYPE_GPU_UNKNOWN     = 0, 
    NV_SYSTEM_TYPE_IGPU            = 1, //!< Integrated GPU
    NV_SYSTEM_TYPE_DGPU            = 2, //!< Discrete GPU
} NV_GPU_TYPE; 


/////////////////////////////////////////////////////////////////////////////// 
// 
// FUNCTION NAME: NvAPI_GPU_GetGPUType 
// 
//!  DESCRIPTION: This function returns the GPU type (integrated or discrete).
//!               See ::NV_GPU_TYPE. 
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 173
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu 
//! \retval  NVAPI_OK                           *pGpuType contains the GPU type 
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found 
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle 
//!
//!  \ingroup gpu 
///////////////////////////////////////////////////////////////////////////////     
//NVAPI_INTERFACE NvAPI_GPU_GetGPUType(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Inout_ NV_GPU_TYPE *pGpuType);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetGPUType)(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Inout_ NV_GPU_TYPE *pGpuType);
_NvAPI_GPU_GetGPUType NvAPI_GPU_GetGPUType;


//! \ingroup gpu
//! Used in NvAPI_GPU_GetBusType()
typedef enum _NV_GPU_BUS_TYPE
{
    NVAPI_GPU_BUS_TYPE_UNDEFINED    = 0,
    NVAPI_GPU_BUS_TYPE_PCI          = 1,
    NVAPI_GPU_BUS_TYPE_AGP          = 2,
    NVAPI_GPU_BUS_TYPE_PCI_EXPRESS  = 3,
    NVAPI_GPU_BUS_TYPE_FPCI         = 4,
    NVAPI_GPU_BUS_TYPE_AXI          = 5,
} NV_GPU_BUS_TYPE;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusType
//
//!  This function returns the type of bus associated with this GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \return      This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!              specific meaning for this API, they are listed below.
//! \retval      NVAPI_INVALID_ARGUMENT             hPhysicalGpu or pBusType is NULL.
//! \retval      NVAPI_OK                          *pBusType contains bus identifier.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetBusType(NvPhysicalGpuHandle hPhysicalGpu,NV_GPU_BUS_TYPE *pBusType);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetBusType)(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_BUS_TYPE* pBusType);
_NvAPI_GPU_GetBusType NvAPI_GPU_GetBusType;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusId
//
//!   DESCRIPTION: Returns the ID of the bus associated with this GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 167
//!
//!  \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBusId is NULL.
//!  \retval  NVAPI_OK                           *pBusId contains the bus ID.
//!  \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//!  \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//!
//!  \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetBusId)(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pBusId);
_NvAPI_GPU_GetBusId NvAPI_GPU_GetBusId;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusSlotId
//
//!   DESCRIPTION: Returns the ID of the bus slot associated with this GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 167
//!
//!  \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBusSlotId is NULL.
//!  \retval  NVAPI_OK                           *pBusSlotId contains the bus slot ID.
//!  \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//!  \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//!
//!  \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetBusSlotId)(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pBusSlotId);
_NvAPI_GPU_GetBusSlotId NvAPI_GPU_GetBusSlotId;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetIRQ
//
//!  This function returns the interrupt number associated with this GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pIRQ is NULL.
//! \retval  NVAPI_OK                           *pIRQ contains interrupt number.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetIRQ)(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pIRQ);
_NvAPI_GPU_GetIRQ NvAPI_GPU_GetIRQ;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosRevision
//
//!  This function returns the revision of the video BIOS associated with this GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval    NVAPI_INVALID_ARGUMENT               hPhysicalGpu or pBiosRevision is NULL.
//! \retval    NVAPI_OK                            *pBiosRevision contains revision number.
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND        No NVIDIA GPU driving a display was found.
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE   hPhysicalGpu was not a physical GPU handle.
//! \ingroup   gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosRevision(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosOEMRevision
//
//!  This function returns the OEM revision of the video BIOS associated with this GPU.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 90
//!
//! \retval    NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBiosRevision is NULL
//! \retval    NVAPI_OK                           *pBiosRevision contains revision number
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup   gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosOEMRevision(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosVersionString
//
//!  This function returns the full video BIOS version string in the form of xx.xx.xx.xx.yy where
//!  - xx numbers come from NvAPI_GPU_GetVbiosRevision() and 
//!  - yy comes from NvAPI_GPU_GetVbiosOEMRevision().
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu is NULL.
//! \retval   NVAPI_OK                            szBiosRevision contains version string.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetVbiosVersionString(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szBiosRevision);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetVbiosVersionString)(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szBiosRevision);
_NvAPI_GPU_GetVbiosVersionString NvAPI_GPU_GetVbiosVersionString;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAGPAperture
//
//!  This function returns the AGP aperture in megabytes.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 90
//!
//! \retval   NVAPI_INVALID_ARGUMENT              pSize is NULL.
//! \retval   NVAPI_OK                            Call successful.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetAGPAperture(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pSize);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetCurrentAGPRate
//
//!  This function returns the current AGP Rate (0 = AGP not present, 1 = 1x, 2 = 2x, etc.).
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 90
//!
//! \retval   NVAPI_INVALID_ARGUMENT              pRate is NULL.
//! \retval   NVAPI_OK                            Call successful.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCurrentAGPRate(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pRate);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetCurrentPCIEDownstreamWidth
//
//!  This function returns the number of PCIE lanes being used for the PCIE interface 
//!  downstream from the GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 90
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pWidth is NULL.
//! \retval  NVAPI_OK                            Call successful.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCurrentPCIEDownstreamWidth(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pWidth);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPhysicalFrameBufferSize
//
//!   This function returns the physical size of framebuffer in KB.  This does NOT include any
//!   system RAM that may be dedicated for use by the GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pSize is NULL
//! \retval  NVAPI_OK                            Call successful
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetPhysicalFrameBufferSize(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pSize);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVirtualFrameBufferSize
//
//!  This function returns the virtual size of framebuffer in KB.  This includes the physical RAM plus any
//!  system RAM that has been dedicated for use by the GPU.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 90
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pSize is NULL.
//! \retval  NVAPI_OK                            Call successful.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVirtualFrameBufferSize(NvPhysicalGpuHandle hPhysicalGpu, NvU32* pSize);



//! \ingroup gpu
typedef struct _NV_BOARD_INFO
{
    NvU32 version;                   //!< structure version
    NvU8 BoardNum[16];               //!< Board Serial Number
}NV_BOARD_INFO_V1;

//! \ingroup gpu
typedef NV_BOARD_INFO_V1    NV_BOARD_INFO;
//! \ingroup gpu
#define NV_BOARD_INFO_VER1  MAKE_NVAPI_VERSION(NV_BOARD_INFO_V1,1)
//! \ingroup gpu
#define NV_BOARD_INFO_VER   NV_BOARD_INFO_VER1

//! SUPPORTED OS:  Windows XP and higher
//!
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBoardInfo
//
//!   DESCRIPTION: This API Retrieves the Board information (a unique GPU Board Serial Number) stored in the InfoROM.
//!
//! \param [in]      hPhysicalGpu       Physical GPU Handle.
//! \param [in,out]  NV_BOARD_INFO      Board Information.
//!
//! TCC_SUPPORTED
//!
//! \retval ::NVAPI_OK                     completed request
//! \retval ::NVAPI_ERROR                  miscellaneous error occurred
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  handle passed is not a physical GPU handle
//! \retval ::NVAPI_API_NOT_INTIALIZED            NVAPI not initialized
//! \retval ::NVAPI_INVALID_POINTER               pBoardInfo is NULL
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION   the version of the INFO struct is not supported
//! 
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetBoardInfo)(NvPhysicalGpuHandle hPhysicalGpu, NV_BOARD_INFO *pBoardInfo);
_NvAPI_GPU_GetBoardInfo NvAPI_GPU_GetBoardInfo;


///////////////////////////////////////////////////////////////////////////////
//
//  GPU Clock Control
//
//  These APIs allow the user to get and set individual clock domains
//  on a per-GPU basis.
//
///////////////////////////////////////////////////////////////////////////////


//! \ingroup gpuclock
//! @{
#define NVAPI_MAX_GPU_CLOCKS 32
#define NVAPI_MAX_GPU_PUBLIC_CLOCKS     32
#define NVAPI_MAX_GPU_PERF_CLOCKS       32
#define NVAPI_MAX_GPU_PERF_VOLTAGES     16
#define NVAPI_MAX_GPU_PERF_PSTATES      16
//! @}

//! \ingroup gpuclock
typedef enum _NV_GPU_PUBLIC_CLOCK_ID
{
    NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS  = 0,
    NVAPI_GPU_PUBLIC_CLOCK_MEMORY    = 4,
    NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR = 7,
    NVAPI_GPU_PUBLIC_CLOCK_UNDEFINED = NVAPI_MAX_GPU_PUBLIC_CLOCKS,
} NV_GPU_PUBLIC_CLOCK_ID;


//! \ingroup gpuclock
typedef enum _NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID
{
    NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE      = 0,
    NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_UNDEFINED = NVAPI_MAX_GPU_PERF_VOLTAGES,
} NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID;



//! \ingroup gpuclock
//! Used in NvAPI_GPU_GetAllClockFrequencies()
typedef struct _NV_GPU_CLOCK_FREQUENCIES_V1 
{
    NvU32   version;    //!< Structure version
    NvU32   reserved;   //!< These bits are reserved for future use.
    struct
    {
        NvU32 bIsPresent:1;         //!< Set if this domain is present on this GPU
        NvU32 reserved:31;          //!< These bits are reserved for future use.
        NvU32 frequency;            //!< Clock frequency (kHz)
    }domain[NVAPI_MAX_GPU_PUBLIC_CLOCKS];
} NV_GPU_CLOCK_FREQUENCIES_V1;

//! \ingroup gpuclock
//! Used in NvAPI_GPU_GetAllClockFrequencies()
typedef enum _NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE
{
    NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ =   0,
    NV_GPU_CLOCK_FREQUENCIES_BASE_CLOCK   =   1,
    NV_GPU_CLOCK_FREQUENCIES_BOOST_CLOCK  =   2,
    NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM = 3
} NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE;

//! \ingroup gpuclock
//! Used in NvAPI_GPU_GetAllClockFrequencies()
typedef struct 
{
    NvU32   version;        //!< Structure version
    NvU32   ClockType:2;    //!< One of NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE. Used to specify the type of clock to be returned.
    NvU32   reserved:22;    //!< These bits are reserved for future use. Must be set to 0.
    NvU32   reserved1:8;    //!< These bits are reserved.
    struct
    {
        NvU32 bIsPresent:1;         //!< Set if this domain is present on this GPU
        NvU32 reserved:31;          //!< These bits are reserved for future use.
        NvU32 frequency;            //!< Clock frequency (kHz)
    }domain[NVAPI_MAX_GPU_PUBLIC_CLOCKS];
} NV_GPU_CLOCK_FREQUENCIES_V2;

//! \ingroup gpuclock
//! Used in NvAPI_GPU_GetAllClockFrequencies()
typedef NV_GPU_CLOCK_FREQUENCIES_V2 NV_GPU_CLOCK_FREQUENCIES;

//! \addtogroup gpuclock
//! @{
#define NV_GPU_CLOCK_FREQUENCIES_VER_1    MAKE_NVAPI_VERSION(NV_GPU_CLOCK_FREQUENCIES_V1,1)
#define NV_GPU_CLOCK_FREQUENCIES_VER_2    MAKE_NVAPI_VERSION(NV_GPU_CLOCK_FREQUENCIES_V2,2)
#define NV_GPU_CLOCK_FREQUENCIES_VER	  NV_GPU_CLOCK_FREQUENCIES_VER_2
//! @}
 
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAllClockFrequencies
//
//!   This function retrieves the NV_GPU_CLOCK_FREQUENCIES structure for the specified physical GPU.
//!
//!   For each clock domain:
//!      - bIsPresent is set for each domain that is present on the GPU
//!      - frequency is the domain's clock freq in kHz
//!
//!   Each domain's info is indexed in the array.  For example:
//!   clkFreqs.domain[NVAPI_GPU_PUBLIC_CLOCK_MEMORY] holds the info for the MEMORY domain.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 295
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, 
//!          they are listed below.
//! \retval  NVAPI_INVALID_ARGUMENT     pClkFreqs is NULL.
//! \ingroup gpuclock
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetAllClockFrequencies(_In_ NvPhysicalGpuHandle hPhysicalGPU, _Inout_ NV_GPU_CLOCK_FREQUENCIES* pClkFreqs);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetAllClockFrequencies)(_In_ NvPhysicalGpuHandle hPhysicalGPU, _Inout_ NV_GPU_CLOCK_FREQUENCIES* pClkFreqs);
_NvAPI_GPU_GetAllClockFrequencies NvAPI_GPU_GetAllClockFrequencies;

//! \addtogroup gpupstate
//! @{

typedef enum _NV_GPU_PERF_PSTATE_ID
{
    NVAPI_GPU_PERF_PSTATE_P0 = 0,
    NVAPI_GPU_PERF_PSTATE_P1,
    NVAPI_GPU_PERF_PSTATE_P2,
    NVAPI_GPU_PERF_PSTATE_P3,
    NVAPI_GPU_PERF_PSTATE_P4,
    NVAPI_GPU_PERF_PSTATE_P5,
    NVAPI_GPU_PERF_PSTATE_P6,
    NVAPI_GPU_PERF_PSTATE_P7,
    NVAPI_GPU_PERF_PSTATE_P8,
    NVAPI_GPU_PERF_PSTATE_P9,
    NVAPI_GPU_PERF_PSTATE_P10,
    NVAPI_GPU_PERF_PSTATE_P11,
    NVAPI_GPU_PERF_PSTATE_P12,
    NVAPI_GPU_PERF_PSTATE_P13,
    NVAPI_GPU_PERF_PSTATE_P14,
    NVAPI_GPU_PERF_PSTATE_P15,
    NVAPI_GPU_PERF_PSTATE_UNDEFINED = NVAPI_MAX_GPU_PERF_PSTATES,
    NVAPI_GPU_PERF_PSTATE_ALL,

} NV_GPU_PERF_PSTATE_ID;

//! @}



//! \ingroup gpupstate
//! Used in NvAPI_GPU_GetPstatesInfoEx()
typedef struct _NV_GPU_PERF_PSTATES_INFO_V1
{
    NvU32   version;
    NvU32   flags;           //!< - bit 0 indicates if perfmon is enabled or not
                             //!< - bit 1 indicates if dynamic Pstate is capable or not
                             //!< - bit 2 indicates if dynamic Pstate is enable or not
                             //!< - all other bits must be set to 0
    NvU32   numPstates;      //!< The number of available p-states 
    NvU32   numClocks;       //!< The number of clock domains supported by each P-State
    struct
    {
        NV_GPU_PERF_PSTATE_ID   pstateId; //!< ID of the p-state.  
        NvU32                   flags;    //!< - bit 0 indicates if the PCIE limit is GEN1 or GEN2
                                          //!< - bit 1 indicates if the Pstate is overclocked or not
                                          //!< - bit 2 indicates if the Pstate is overclockable or not
                                          //!< - all other bits must be set to 0
        struct
        {
            NV_GPU_PUBLIC_CLOCK_ID           domainId;  //!< ID of the clock domain   
            NvU32                               flags;  //!< Reserved. Must be set to 0
            NvU32                                freq;  //!< Clock frequency in kHz

        } clocks[NVAPI_MAX_GPU_PERF_CLOCKS];
    } pstates[NVAPI_MAX_GPU_PERF_PSTATES];

} NV_GPU_PERF_PSTATES_INFO_V1;


//! \ingroup gpupstate
typedef struct _NV_GPU_PERF_PSTATES_INFO_V2
{
    NvU32   version;
    NvU32   flags;             //!< - bit 0 indicates if perfmon is enabled or not
                               //!< - bit 1 indicates if dynamic Pstate is capable or not
                               //!< - bit 2 indicates if dynamic Pstate is enable or not
                               //!< - all other bits must be set to 0
    NvU32   numPstates;        //!< The number of available p-states 
    NvU32   numClocks;         //!< The number of clock domains supported by each P-State   
    NvU32   numVoltages; 
    struct
    {
        NV_GPU_PERF_PSTATE_ID   pstateId;  //!< ID of the p-state. 
        NvU32                   flags;     //!< - bit 0 indicates if the PCIE limit is GEN1 or GEN2
                                           //!< - bit 1 indicates if the Pstate is overclocked or not
                                           //!< - bit 2 indicates if the Pstate is overclockable or not
                                           //!< - all other bits must be set to 0
        struct
        {
            NV_GPU_PUBLIC_CLOCK_ID            domainId;       
            NvU32                                flags; //!< bit 0 indicates if this clock is overclockable
                                                        //!< all other bits must be set to 0
            NvU32                                 freq;

        } clocks[NVAPI_MAX_GPU_PERF_CLOCKS];
        struct
        {
            NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID domainId; //!< ID of the voltage domain, containing flags and mvolt info 
            NvU32                       flags;           //!< Reserved for future use. Must be set to 0
            NvU32                       mvolt;           //!< Voltage in mV  

        } voltages[NVAPI_MAX_GPU_PERF_VOLTAGES];

    } pstates[NVAPI_MAX_GPU_PERF_PSTATES];  //!< Valid index range is 0 to numVoltages-1

} NV_GPU_PERF_PSTATES_INFO_V2;

//! \ingroup gpupstate
typedef  NV_GPU_PERF_PSTATES_INFO_V2 NV_GPU_PERF_PSTATES_INFO;


//! \ingroup gpupstate
//! @{

//! Macro for constructing the version field of NV_GPU_PERF_PSTATES_INFO_V1 
#define NV_GPU_PERF_PSTATES_INFO_VER1  MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES_INFO_V1,1)

//! Macro for constructing the version field of NV_GPU_PERF_PSTATES_INFO_V2 
#define NV_GPU_PERF_PSTATES_INFO_VER2  MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES_INFO_V2,2)

//! Macro for constructing the version field of NV_GPU_PERF_PSTATES_INFO 
#define NV_GPU_PERF_PSTATES_INFO_VER   NV_GPU_PERF_PSTATES_INFO_VER2

//! @}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetPstatesInfoEx
//
//! DESCRIPTION:     This API retrieves all performance states (P-States) information. This is the same as
//!                  NvAPI_GPU_GetPstatesInfo(), but supports an input flag for various options.
//!
//!                  P-States are GPU active/executing performance capability and power consumption states.
//!
//!                  P-States ranges from P0 to P15, with P0 being the highest performance/power state, and
//!                  P15 being the lowest performance/power state. Each P-State, if available, maps to a
//!                  performance level. Not all P-States are available on a given system. The definitions
//!                  of each P-State are currently as follows: \n
//!                  - P0/P1 - Maximum 3D performance
//!                  - P2/P3 - Balanced 3D performance-power
//!                  - P8 - Basic HD video playback
//!                  - P10 - DVD playback
//!                  - P12 - Minimum idle power consumption
//!
//! \deprecated  Do not use this function - it is deprecated in release 304. Instead, use NvAPI_GPU_GetPstates20.
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \param [in]     hPhysicalGPU       GPU selection.
//! \param [out]    pPerfPstatesInfo   P-States information retrieved, as detailed below: \n
//!                  - flags is reserved for future use.
//!                  - numPstates is the number of available P-States
//!                  - numClocks is the number of clock domains supported by each P-State
//!                  - pstates has valid index range from 0 to numPstates - 1
//!                  - pstates[i].pstateId is the ID of the P-State,
//!                      containing the following info:
//!                    - pstates[i].flags containing the following info:
//!                        - bit 0 indicates if the PCIE limit is GEN1 or GEN2
//!                        - bit 1 indicates if the Pstate is overclocked or not
//!                        - bit 2 indicates if the Pstate is overclockable or not
//!                    - pstates[i].clocks has valid index range from 0 to numClocks -1
//!                    - pstates[i].clocks[j].domainId is the public ID of the clock domain,
//!                        containing the following info:
//!                      - pstates[i].clocks[j].flags containing the following info:
//!                          bit 0 indicates if the clock domain is overclockable or not
//!                      - pstates[i].clocks[j].freq is the clock frequency in kHz
//!                    - pstates[i].voltages has a valid index range from 0 to numVoltages - 1
//!                    - pstates[i].voltages[j].domainId is the ID of the voltage domain,
//!                        containing the following info:
//!                      - pstates[i].voltages[j].flags is reserved for future use.
//!                      - pstates[i].voltages[j].mvolt is the voltage in mV
//!                  inputFlags(IN)   - This can be used to select various options:
//!                    - if bit 0 is set, pPerfPstatesInfo would contain the default settings
//!                        instead of the current, possibily overclocked settings.
//!                    - if bit 1 is set, pPerfPstatesInfo would contain the maximum clock 
//!                        frequencies instead of the nominal frequencies.
//!                    - if bit 2 is set, pPerfPstatesInfo would contain the minimum clock 
//!                        frequencies instead of the nominal frequencies.
//!                    - all other bits must be set to 0.
//!
//! \retval ::NVAPI_OK                            Completed request
//! \retval ::NVAPI_ERROR                         Miscellaneous error occurred
//! \retval ::NVAPI_HANDLE_INVALIDATED            Handle passed has been invalidated (see user guide)
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  Handle passed is not a physical GPU handle
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION   The version of the NV_GPU_PERF_PSTATES struct is not supported
//!
//! \ingroup gpupstate 
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 304. Instead, use NvAPI_GPU_GetPstates20.")
NVAPI_INTERFACE NvAPI_GPU_GetPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATES_INFO *pPerfPstatesInfo, NvU32 inputFlags);


//! \addtogroup gpupstate
//! @{

#define NVAPI_MAX_GPU_PSTATE20_PSTATES          16
#define NVAPI_MAX_GPU_PSTATE20_CLOCKS           8
#define NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES    4

//! Used to identify clock type
typedef enum _NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID
{
    //! Clock domains that use single frequency value within given pstate
    NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_SINGLE = 0,

    //! Clock domains that allow range of frequency values within given pstate
    NVAPI_GPU_PERF_PSTATE20_CLOCK_TYPE_RANGE,
} NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID;

//! Used to describe both voltage and frequency deltas
typedef struct _NV_GPU_PERF_PSTATES20_PARAM_DELTA
{
    //! Value of parameter delta (in respective units [kHz, uV])
    NvS32       value;

    struct
    {
        //! Min value allowed for parameter delta (in respective units [kHz, uV])
        NvS32   min;

        //! Max value allowed for parameter delta (in respective units [kHz, uV])
        NvS32   max;
    } valueRange;
} NV_GPU_PERF_PSTATES20_PARAM_DELTA;

//! Used to describe single clock entry
typedef struct _NV_GPU_PSTATE20_CLOCK_ENTRY_V1
{
    //! ID of the clock domain
    NV_GPU_PUBLIC_CLOCK_ID                      domainId;

    //! Clock type ID
    NV_GPU_PERF_PSTATE20_CLOCK_TYPE_ID          typeId;
    NvU32                                       bIsEditable : 1;

    //! These bits are reserved for future use (must be always 0)
    NvU32                                       reserved : 31;

    //! Current frequency delta from nominal settings in (kHz)
    NV_GPU_PERF_PSTATES20_PARAM_DELTA           freqDelta_kHz;

    //! Clock domain type dependant information
    union
    {
        struct
        {
            //! Clock frequency within given pstate in (kHz)
            NvU32                               freq_kHz;
        } single;

        struct
        {
            //! Min clock frequency within given pstate in (kHz)
            NvU32                               minFreq_kHz;

            //! Max clock frequency within given pstate in (kHz)
            NvU32                               maxFreq_kHz;

            //! Voltage domain ID and value range in (uV) required for this clock
            NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID  domainId;
            NvU32                               minVoltage_uV;
            NvU32                               maxVoltage_uV;
        } range;
    } data;
} NV_GPU_PSTATE20_CLOCK_ENTRY_V1;

//! Used to describe single base voltage entry
typedef struct _NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1
{
    //! ID of the voltage domain
    NV_GPU_PERF_VOLTAGE_INFO_DOMAIN_ID  domainId;
    NvU32                               bIsEditable:1;

    //! These bits are reserved for future use (must be always 0)
    NvU32                               reserved:31;

    //! Current base voltage settings in [uV]
    NvU32                               volt_uV;

    NV_GPU_PERF_PSTATES20_PARAM_DELTA   voltDelta_uV; // Current base voltage delta from nominal settings in [uV]
} NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1;

//! Used in NvAPI_GPU_GetPstates20() interface call.

typedef struct _NV_GPU_PERF_PSTATES20_INFO_V1
{
    //! Version info of the structure (NV_GPU_PERF_PSTATES20_INFO_VER<n>)
    NvU32 version; 

    NvU32 bIsEditable:1;

    //! These bits are reserved for future use (must be always 0)
    NvU32 reserved:31;

    //! Number of populated pstates
    NvU32 numPstates;

    //! Number of populated clocks (per pstate)
    NvU32 numClocks;

    //! Number of populated base voltages (per pstate)
    NvU32 numBaseVoltages;

    //! Performance state (P-State) settings
    //! Valid index range is 0 to numPstates-1
    struct
    {
    //! ID of the P-State
        NV_GPU_PERF_PSTATE_ID                   pstateId;

        NvU32                                   bIsEditable:1;

        //! These bits are reserved for future use (must be always 0)
        NvU32                                   reserved:31;

        //! Array of clock entries
        //! Valid index range is 0 to numClocks-1
        NV_GPU_PSTATE20_CLOCK_ENTRY_V1          clocks[NVAPI_MAX_GPU_PSTATE20_CLOCKS];

        //! Array of baseVoltage entries
        //! Valid index range is 0 to numBaseVoltages-1
        NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1   baseVoltages[NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES];
    } pstates[NVAPI_MAX_GPU_PSTATE20_PSTATES];
} NV_GPU_PERF_PSTATES20_INFO_V1;

//! Used in NvAPI_GPU_GetPstates20() interface call.

typedef struct _NV_GPU_PERF_PSTATES20_INFO_V2
{
    //! Version info of the structure (NV_GPU_PERF_PSTATES20_INFO_VER<n>)
    NvU32 version; 

    NvU32 bIsEditable : 1;

    //! These bits are reserved for future use (must be always 0)
    NvU32 reserved : 31;

    //! Number of populated pstates
    NvU32 numPstates;

    //! Number of populated clocks (per pstate)
    NvU32 numClocks;

    //! Number of populated base voltages (per pstate)
    NvU32 numBaseVoltages;

    //! Performance state (P-State) settings
    //! Valid index range is 0 to numPstates-1
    struct
    {
    //! ID of the P-State
        NV_GPU_PERF_PSTATE_ID pstateId;

        NvU32 bIsEditable : 1;

        //! These bits are reserved for future use (must be always 0)
        NvU32 reserved : 31;

        //! Array of clock entries
        //! Valid index range is 0 to numClocks-1
        NV_GPU_PSTATE20_CLOCK_ENTRY_V1 clocks[NVAPI_MAX_GPU_PSTATE20_CLOCKS];

        //! Array of baseVoltage entries
        //! Valid index range is 0 to numBaseVoltages-1
        NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1 baseVoltages[NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES];
    } pstates[NVAPI_MAX_GPU_PSTATE20_PSTATES];

    //! OV settings - Please refer to NVIDIA over-volting recommendation to understand impact of this functionality
    //! Valid index range is 0 to numVoltages-1
    struct
    {
        //! Number of populated voltages
        NvU32 numVoltages;

        //! Array of voltage entries
        //! Valid index range is 0 to numVoltages-1
        NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1 voltages[NVAPI_MAX_GPU_PSTATE20_BASE_VOLTAGES];
    } ov;
} NV_GPU_PERF_PSTATES20_INFO_V2;

typedef NV_GPU_PERF_PSTATES20_INFO_V2 NV_GPU_PERF_PSTATES20_INFO;

//! Macro for constructing the version field of NV_GPU_PERF_PSTATES20_INFO_V1
#define NV_GPU_PERF_PSTATES20_INFO_VER1 MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES20_INFO_V1,1)

//! Macro for constructing the version field of NV_GPU_PERF_PSTATES20_INFO_V2
#define NV_GPU_PERF_PSTATES20_INFO_VER2 MAKE_NVAPI_VERSION(NV_GPU_PERF_PSTATES20_INFO_V2,2)

//! Macro for constructing the version field of NV_GPU_PERF_PSTATES20_INFO
#define NV_GPU_PERF_PSTATES20_INFO_VER NV_GPU_PERF_PSTATES20_INFO_VER2

//! @}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetPstates20
//
//! DESCRIPTION:    This API retrieves all performance states (P-States) 2.0 information.
//!
//!                 P-States are GPU active/executing performance capability states.
//!                 They range from P0 to P15, with P0 being the highest performance state,
//!                 and P15 being the lowest performance state. Each P-State, if available,
//!                 maps to a performance level. Not all P-States are available on a given system.
//!                 The definition of each P-States are currently as follow:
//!                 - P0/P1 - Maximum 3D performance
//!                 - P2/P3 - Balanced 3D performance-power
//!                 - P8 - Basic HD video playback
//!                 - P10 - DVD playback
//!                 - P12 - Minimum idle power consumption
//!
//! TCC_SUPPORTED
//!
//! \since Release: 295
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hPhysicalGPU  GPU selection
//! \param [out]  pPstatesInfo  P-States information retrieved, as documented in declaration above
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, 
//!          they are listed below.
//!
//! \ingroup gpupstate
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetPstates20(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Inout_ NV_GPU_PERF_PSTATES20_INFO* pPstatesInfo);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetPstates20)(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Inout_ NV_GPU_PERF_PSTATES20_INFO* pPstatesInfo);
_NvAPI_GPU_GetPstates20 NvAPI_GPU_GetPstates20;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetCurrentPstate
//
//! DESCRIPTION:     This function retrieves the current performance state (P-State).
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 165
//!
//! TCC_SUPPORTED
//!
//! \param [in]      hPhysicalGPU     GPU selection
//! \param [out]     pCurrentPstate   The ID of the current P-State of the GPU - see \ref NV_GPU_PERF_PSTATES.
//!
//! \retval    NVAPI_OK                             Completed request
//! \retval    NVAPI_ERROR                          Miscellaneous error occurred.
//! \retval    NVAPI_HANDLE_INVALIDATED             Handle passed has been invalidated (see user guide).
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE   Handle passed is not a physical GPU handle.
//! \retval    NVAPI_NOT_SUPPORTED                  P-States is not supported on this setup.
//!
//! \ingroup   gpupstate
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetCurrentPstate(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATE_ID *pCurrentPstate);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetCurrentPstate)(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_PERF_PSTATE_ID *pCurrentPstate);
_NvAPI_GPU_GetCurrentPstate NvAPI_GPU_GetCurrentPstate;


//! \ingroup gpupstate
#define NVAPI_MAX_GPU_UTILIZATIONS 8



//! \ingroup gpupstate
//! Used in NvAPI_GPU_GetDynamicPstatesInfoEx().
typedef struct _NV_GPU_DYNAMIC_PSTATES_INFO_EX
{
    NvU32       version;        //!< Structure version
    NvU32       flags;          //!< bit 0 indicates if the dynamic Pstate is enabled or not
    struct
    {
        NvU32   bIsPresent:1;   //!< Set if this utilization domain is present on this GPU
        NvU32   percentage;     //!< Percentage of time where the domain is considered busy in the last 1 second interval
    } utilization[NVAPI_MAX_GPU_UTILIZATIONS];
} NV_GPU_DYNAMIC_PSTATES_INFO_EX;

//! \ingroup gpupstate
//! Macro for constructing the version field of NV_GPU_DYNAMIC_PSTATES_INFO_EX
#define NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER MAKE_NVAPI_VERSION(NV_GPU_DYNAMIC_PSTATES_INFO_EX,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetDynamicPstatesInfoEx
//
//! DESCRIPTION:   This API retrieves the NV_GPU_DYNAMIC_PSTATES_INFO_EX structure for the specified physical GPU.
//!                Each domain's info is indexed in the array.  For example: 
//!                - pDynamicPstatesInfo->utilization[NVAPI_GPU_UTILIZATION_DOMAIN_GPU] holds the info for the GPU domain. \p
//!                There are currently 4 domains for which GPU utilization and dynamic P-State thresholds can be retrieved:
//!                   graphic engine (GPU), frame buffer (FB), video engine (VID), and bus interface (BUS).
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 185
//! 
//! \retval ::NVAPI_OK 
//! \retval ::NVAPI_ERROR 
//! \retval ::NVAPI_INVALID_ARGUMENT  pDynamicPstatesInfo is NULL
//! \retval ::NVAPI_HANDLE_INVALIDATED 
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE 
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION The version of the INFO struct is not supported
//!
//! \ingroup gpupstate
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetDynamicPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX *pDynamicPstatesInfoEx);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetDynamicPstatesInfoEx)(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX* pDynamicPstatesInfoEx);
_NvAPI_GPU_GetDynamicPstatesInfoEx NvAPI_GPU_GetDynamicPstatesInfoEx;

///////////////////////////////////////////////////////////////////////////////////
//  Thermal API
//  Provides ability to get temperature levels from the various thermal sensors associated with the GPU

//! \ingroup gputhermal
#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

//! \ingroup gputhermal
//! Used in NV_GPU_THERMAL_SETTINGS
typedef enum _NV_THERMAL_TARGET
{
    NVAPI_THERMAL_TARGET_NONE          = 0,
    NVAPI_THERMAL_TARGET_GPU           = 1,     //!< GPU core temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_MEMORY        = 2,     //!< GPU memory temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_POWER_SUPPLY  = 4,     //!< GPU power supply temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_BOARD         = 8,     //!< GPU board ambient temperature requires NvPhysicalGpuHandle
    NVAPI_THERMAL_TARGET_VCD_BOARD     = 9,     //!< Visual Computing Device Board temperature requires NvVisualComputingDeviceHandle
    NVAPI_THERMAL_TARGET_VCD_INLET     = 10,    //!< Visual Computing Device Inlet temperature requires NvVisualComputingDeviceHandle
    NVAPI_THERMAL_TARGET_VCD_OUTLET    = 11,    //!< Visual Computing Device Outlet temperature requires NvVisualComputingDeviceHandle

    NVAPI_THERMAL_TARGET_ALL           = 15,
    NVAPI_THERMAL_TARGET_UNKNOWN       = -1,
} NV_THERMAL_TARGET;

//! \ingroup gputhermal
//! Used in NV_GPU_THERMAL_SETTINGS
typedef enum _NV_THERMAL_CONTROLLER
{
    NVAPI_THERMAL_CONTROLLER_NONE = 0,
    NVAPI_THERMAL_CONTROLLER_GPU_INTERNAL,  
    NVAPI_THERMAL_CONTROLLER_ADM1032,
    NVAPI_THERMAL_CONTROLLER_MAX6649,       
    NVAPI_THERMAL_CONTROLLER_MAX1617,      
    NVAPI_THERMAL_CONTROLLER_LM99,      
    NVAPI_THERMAL_CONTROLLER_LM89,         
    NVAPI_THERMAL_CONTROLLER_LM64,         
    NVAPI_THERMAL_CONTROLLER_ADT7473,
    NVAPI_THERMAL_CONTROLLER_SBMAX6649,
    NVAPI_THERMAL_CONTROLLER_VBIOSEVT,  
    NVAPI_THERMAL_CONTROLLER_OS,    
    NVAPI_THERMAL_CONTROLLER_UNKNOWN = -1,
} NV_THERMAL_CONTROLLER;

//! \ingroup gputhermal
//! Used in NvAPI_GPU_GetThermalSettings()
typedef struct _NV_GPU_THERMAL_SETTINGS_V1
{
    NvU32   version;                //!< structure version 
    NvU32   count;                  //!< number of associated thermal sensors
    struct 
    {
        NV_THERMAL_CONTROLLER       controller;        //!< internal, ADM1032, MAX6649...
        NvU32                       defaultMinTemp;    //!< The min default temperature value of the thermal sensor in degree Celsius 
        NvU32                       defaultMaxTemp;    //!< The max default temperature value of the thermal sensor in degree Celsius 
        NvU32                       currentTemp;       //!< The current temperature value of the thermal sensor in degree Celsius 
        NV_THERMAL_TARGET           target;            //!< Thermal sensor targeted @ GPU, memory, chipset, powersupply, Visual Computing Device, etc.
    } sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS_V1;

//! \ingroup gputhermal
typedef struct _NV_GPU_THERMAL_SETTINGS_V2
{
    NvU32   version;                //!< structure version
    NvU32   count;                  //!< number of associated thermal sensors
    struct
    {
        NV_THERMAL_CONTROLLER       controller;         //!< internal, ADM1032, MAX6649...
        NvS32                       defaultMinTemp;     //!< Minimum default temperature value of the thermal sensor in degree Celsius
        NvS32                       defaultMaxTemp;     //!< Maximum default temperature value of the thermal sensor in degree Celsius
        NvS32                       currentTemp;        //!< Current temperature value of the thermal sensor in degree Celsius
        NV_THERMAL_TARGET           target;             //!< Thermal sensor targeted - GPU, memory, chipset, powersupply, Visual Computing Device, etc
    } sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS_V2;

//! \ingroup gputhermal
typedef NV_GPU_THERMAL_SETTINGS_V2  NV_GPU_THERMAL_SETTINGS;

//! \ingroup gputhermal
//! @{

//! Macro for constructing the version field of NV_GPU_THERMAL_SETTINGS_V1
#define NV_GPU_THERMAL_SETTINGS_VER_1   MAKE_NVAPI_VERSION(NV_GPU_THERMAL_SETTINGS_V1,1)

//! Macro for constructing the version field of NV_GPU_THERMAL_SETTINGS_V2
#define NV_GPU_THERMAL_SETTINGS_VER_2   MAKE_NVAPI_VERSION(NV_GPU_THERMAL_SETTINGS_V2,2)

//! Macro for constructing the version field of NV_GPU_THERMAL_SETTINGS
#define NV_GPU_THERMAL_SETTINGS_VER     NV_GPU_THERMAL_SETTINGS_VER_2
//! @}




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetThermalSettings
//
//!  This function retrieves the thermal information of all thermal sensors or specific thermal sensor associated with the selected GPU.
//!  Thermal sensors are indexed 0 to NVAPI_MAX_THERMAL_SENSORS_PER_GPU-1.
//!
//!  - To retrieve specific thermal sensor info, set the sensorIndex to the required thermal sensor index. 
//!  - To retrieve info for all sensors, set sensorIndex to NVAPI_THERMAL_TARGET_ALL. 
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \since Release: 85
//!
//! \param [in]   hPhysicalGPU      GPU selection.
//! \param [in]   sensorIndex       Explicit thermal sensor index selection. 
//! \param [out]  pThermalSettings  Array of thermal settings.
//!
//! \retval   NVAPI_OK                           Completed request
//! \retval   NVAPI_ERROR                        Miscellaneous error occurred.
//! \retval   NVAPI_INVALID_ARGUMENT             pThermalInfo is NULL.
//! \retval   NVAPI_HANDLE_INVALIDATED           Handle passed has been invalidated (see user guide).
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE Handle passed is not a physical GPU handle.
//! \retval   NVAPI_INCOMPATIBLE_STRUCT_VERSION  The version of the INFO struct is not supported.
//! \ingroup gputhermal
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetThermalSettings(NvPhysicalGpuHandle hPhysicalGpu, NvU32 sensorIndex, NV_GPU_THERMAL_SETTINGS *pThermalSettings);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetThermalSettings)(NvPhysicalGpuHandle hPhysicalGpu, NvU32 sensorIndex, NV_GPU_THERMAL_SETTINGS *pThermalSettings);
_NvAPI_GPU_GetThermalSettings NvAPI_GPU_GetThermalSettings;


///////////////////////////////////////////////////////////////////////////////////
//  I2C API
//  Provides ability to read or write data using I2C protocol.
//  These APIs allow I2C access only to DDC monitors


//! \addtogroup i2capi
//! @{
#define NVAPI_MAX_SIZEOF_I2C_DATA_BUFFER    4096
#define NVAPI_MAX_SIZEOF_I2C_REG_ADDRESS       4
#define NVAPI_DISPLAY_DEVICE_MASK_MAX         24
#define NVAPI_I2C_SPEED_DEPRECATED        0xFFFF

typedef enum _NV_I2C_SPEED
{
    NVAPI_I2C_SPEED_DEFAULT,    //!< Set i2cSpeedKhz to I2C_SPEED_DEFAULT if default I2C speed is to be chosen, ie.use the current frequency setting.
    NVAPI_I2C_SPEED_3KHZ,
    NVAPI_I2C_SPEED_10KHZ,
    NVAPI_I2C_SPEED_33KHZ,
    NVAPI_I2C_SPEED_100KHZ,
    NVAPI_I2C_SPEED_200KHZ,
    NVAPI_I2C_SPEED_400KHZ,
} NV_I2C_SPEED;

//! Used in NvAPI_I2CRead() and NvAPI_I2CWrite()
typedef struct
{
    NvU32                   version;            //!< The structure version.
    NvU32                   displayMask;        //!< The Display Mask of the concerned display.
    NvU8                    bIsDDCPort;         //!< This flag indicates either the DDC port (TRUE) or the communication port
                                                //!< (FALSE) of the concerned display.
    NvU8                    i2cDevAddress;      //!< The address of the I2C slave.  The address should be shifted left by one.  For
                                                //!< example, the I2C address 0x50, often used for reading EDIDs, would be stored
                                                //!< here as 0xA0.  This matches the position within the byte sent by the master, as
                                                //!< the last bit is reserved to specify the read or write direction.
    NvU8*                   pbI2cRegAddress;    //!< The I2C target register address.  May be NULL, which indicates no register
                                                //!< address should be sent.
    NvU32                   regAddrSize;        //!< The size in bytes of target register address.  If pbI2cRegAddress is NULL, this
                                                //!< field must be 0.
    NvU8*                   pbData;             //!< The buffer of data which is to be read or written (depending on the command).
    NvU32                   cbSize;             //!< The size of the data buffer, pbData, to be read or written.
    NvU32                   i2cSpeed;           //!< The target speed of the transaction (between 28Kbps to 40Kbps; not guaranteed).
} NV_I2C_INFO_V1;

//! Used in NvAPI_I2CRead() and NvAPI_I2CWrite()
typedef struct
{
    NvU32                   version;            //!< The structure version.
    NvU32                   displayMask;        //!< The Display Mask of the concerned display.
    NvU8                    bIsDDCPort;         //!< This flag indicates either the DDC port (TRUE) or the communication port
                                                //!< (FALSE) of the concerned display.
    NvU8                    i2cDevAddress;      //!< The address of the I2C slave.  The address should be shifted left by one.  For
                                                //!< example, the I2C address 0x50, often used for reading EDIDs, would be stored
                                                //!< here as 0xA0.  This matches the position within the byte sent by the master, as
                                                //!< the last bit is reserved to specify the read or write direction.
    NvU8*                   pbI2cRegAddress;    //!< The I2C target register address.  May be NULL, which indicates no register
                                                //!< address should be sent.
    NvU32                   regAddrSize;        //!< The size in bytes of target register address.  If pbI2cRegAddress is NULL, this
                                                //!< field must be 0.
    NvU8*                   pbData;             //!< The buffer of data which is to be read or written (depending on the command).
    NvU32                   cbSize;             //!< The size of the data buffer, pbData, to be read or written.
    NvU32                   i2cSpeed;           //!< Deprecated, Must be set to NVAPI_I2C_SPEED_DEPRECATED.
    NV_I2C_SPEED            i2cSpeedKhz;        //!< The target speed of the transaction in (kHz) (Chosen from the enum NV_I2C_SPEED).
} NV_I2C_INFO_V2;

//! Used in NvAPI_I2CRead() and NvAPI_I2CWrite()
typedef struct
{
    NvU32                   version;            //!< The structure version.
    NvU32                   displayMask;        //!< The Display Mask of the concerned display.
    NvU8                    bIsDDCPort;         //!< This flag indicates either the DDC port (TRUE) or the communication port
                                                //!< (FALSE) of the concerned display.
    NvU8                    i2cDevAddress;      //!< The address of the I2C slave.  The address should be shifted left by one.  For
                                                //!< example, the I2C address 0x50, often used for reading EDIDs, would be stored
                                                //!< here as 0xA0.  This matches the position within the byte sent by the master, as
                                                //!< the last bit is reserved to specify the read or write direction.
    NvU8*                   pbI2cRegAddress;    //!< The I2C target register address.  May be NULL, which indicates no register
                                                //!< address should be sent.
    NvU32                   regAddrSize;        //!< The size in bytes of target register address.  If pbI2cRegAddress is NULL, this
                                                //!< field must be 0.
    NvU8*                   pbData;             //!< The buffer of data which is to be read or written (depending on the command).
    NvU32                   cbSize;             //!< The size of the data buffer, pbData, to be read or written.
    NvU32                   i2cSpeed;           //!< Deprecated, Must be set to NVAPI_I2C_SPEED_DEPRECATED.
    NV_I2C_SPEED            i2cSpeedKhz;        //!< The target speed of the transaction in (kHz) (Chosen from the enum NV_I2C_SPEED).
    NvU8                    portId;             //!< The portid on which device is connected (remember to set bIsPortIdSet if this value is set)
                                                //!< Optional for pre-Kepler
    NvU32                   bIsPortIdSet;       //!< set this flag on if and only if portid value is set
} NV_I2C_INFO_V3;

typedef NV_I2C_INFO_V3                     NV_I2C_INFO;

#define NV_I2C_INFO_VER3  MAKE_NVAPI_VERSION(NV_I2C_INFO_V3,3)
#define NV_I2C_INFO_VER2  MAKE_NVAPI_VERSION(NV_I2C_INFO_V2,2)
#define NV_I2C_INFO_VER1  MAKE_NVAPI_VERSION(NV_I2C_INFO_V1,1)

#define NV_I2C_INFO_VER  NV_I2C_INFO_VER3
//! @}

/***********************************************************************************/


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  NvAPI_I2CRead
//
//!  This function reads the data buffer from the I2C port.
//!                 The I2C request must be for a DDC port: pI2cInfo->bIsDDCPort = 1.
//!
//!                 A data buffer size larger than 16 bytes may be rejected if a register address is specified.  In such a case,
//!                 NVAPI_ARGUMENT_EXCEED_MAX_SIZE would be returned.
//!
//!                 If a register address is specified (i.e. regAddrSize is positive), then the transaction will be performed in
//!                 the combined format described in the I2C specification.  The register address will be written, followed by
//!                 reading into the data buffer.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 85
//!
//! \param [in]   hPhysicalGPU     GPU selection.
//! \param [out]  NV_I2C_INFO     *pI2cInfo The I2C data input structure
//!
//! \retval   NVAPI_OK                            Completed request
//! \retval   NVAPI_ERROR                         Miscellaneous error occurred.
//! \retval   NVAPI_HANDLE_INVALIDATED            Handle passed has been invalidated (see user guide).
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  Handle passed is not a physical GPU handle.
//! \retval   NVAPI_INCOMPATIBLE_STRUCT_VERSION   Structure version is not supported.
//! \retval   NVAPI_INVALID_ARGUMENT - argument does not meet specified requirements
//! \retval   NVAPI_ARGUMENT_EXCEED_MAX_SIZE - an argument exceeds the maximum 
//!
//! \ingroup i2capi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_I2CRead(NvPhysicalGpuHandle hPhysicalGpu, NV_I2C_INFO *pI2cInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  NvAPI_I2CWrite
//
//!  This function writes the data buffer to the I2C port.
//!
//!                 The I2C request must be for a DDC port: pI2cInfo->bIsDDCPort = 1.
//!
//!                 A data buffer size larger than 16 bytes may be rejected if a register address is specified.  In such a case,
//!                 NVAPI_ARGUMENT_EXCEED_MAX_SIZE would be returned.
//!
//!                 If a register address is specified (i.e. regAddrSize is positive), then the register address will be written
//!                 and the data buffer will immediately follow without a restart.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 85
//!
//! \param [in]   hPhysicalGPU     GPU selection.
//! \param [in]   pI2cInfo         The I2C data input structure
//!
//! \retval   NVAPI_OK                            Completed request
//! \retval   NVAPI_ERROR                         Miscellaneous error occurred.
//! \retval   NVAPI_HANDLE_INVALIDATED            Handle passed has been invalidated (see user guide).
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  Handle passed is not a physical GPU handle.
//! \retval   NVAPI_INCOMPATIBLE_STRUCT_VERSION    Structure version is not supported.
//! \retval   NVAPI_INVALID_ARGUMENT              Argument does not meet specified requirements
//! \retval   NVAPI_ARGUMENT_EXCEED_MAX_SIZE      Argument exceeds the maximum 
//!
//! \ingroup i2capi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_I2CWrite(NvPhysicalGpuHandle hPhysicalGpu, NV_I2C_INFO *pI2cInfo);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_WorkstationFeatureSetup
//
//! \fn NvAPI_GPU_WorkstationFeatureSetup(NvPhysicalGpuHandle hPhysicalGpu, NvU32 featureEnableMask, NvU32 featureDisableMask)
//!   DESCRIPTION: This API configures the driver for a set of workstation features.
//!                The driver can allocate the memory resources accordingly.
//!
//! SUPPORTED OS:  Windows 7
//!
//!
//! \param [in]   hPhysicalGpu       Physical GPU Handle of the display adapter to be configured. GPU handles may be retrieved
//!                                  using NvAPI_EnumPhysicalGPUs. A value of NULL is permitted and applies the same operation
//!                                  to all GPU handles enumerated by NvAPI_EnumPhysicalGPUs.
//! \param [in]   featureEnableMask  Mask of features the caller requests to enable for use
//! \param [in]   featureDisableMask Mask of features the caller requests to disable 
//!
//!                As a general rule, features in the enable and disable masks are expected to be disjoint, although the disable 
//!                mask has precedence and a feature flagged in both masks will be disabled.
//!
//! \retval ::NVAPI_OK                            configuration request succeeded
//! \retval ::NVAPI_ERROR                         configuration request failed
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu is not a physical GPU handle.
//! \retval ::NVAPI_GPU_WORKSTATION_FEATURE_INCOMPLETE  requested feature set does not have all resources allocated for completeness.
//! \retval ::NVAPI_NO_IMPLEMENTATION             only implemented for Win7
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu 
typedef enum _NVAPI_GPU_WORKSTATION_FEATURE_MASK
{
    NVAPI_GPU_WORKSTATION_FEATURE_MASK_SWAPGROUP     = 0x00000001,
    NVAPI_GPU_WORKSTATION_FEATURE_MASK_STEREO        = 0x00000010,
    NVAPI_GPU_WORKSTATION_FEATURE_MASK_WARPING       = 0x00000100,
    NVAPI_GPU_WORKSTATION_FEATURE_MASK_PIXINTENSITY  = 0x00000200,
    NVAPI_GPU_WORKSTATION_FEATURE_MASK_GRAYSCALE     = 0x00000400,
    NVAPI_GPU_WORKSTATION_FEATURE_MASK_BPC10         = 0x00001000
} NVAPI_GPU_WORKSTATION_FEATURE_MASK;

//! \ingroup gpu
NVAPI_INTERFACE NvAPI_GPU_WorkstationFeatureSetup(_In_ NvPhysicalGpuHandle hPhysicalGpu, _In_ NvU32 featureEnableMask, _In_ NvU32 featureDisableMask);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_WorkstationFeatureQuery
//
//!   DESCRIPTION: This API queries the current set of workstation features.
//!
//! SUPPORTED OS:  Windows 7
//!
//!
//! \param [in]   hPhysicalGpu       Physical GPU Handle of the display adapter to be configured. GPU handles may be retrieved
//!                                  using NvAPI_EnumPhysicalGPUs. 
//! \param [out]  pConfiguredFeatureMask  Mask of features requested for use by client drivers
//! \param [out]  pConsistentFeatureMask  Mask of features that have all resources allocated for completeness.
//!
//! \retval ::NVAPI_OK                            configuration request succeeded
//! \retval ::NVAPI_ERROR                         configuration request failed
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu is not a physical GPU handle.
//! \retval ::NVAPI_NO_IMPLEMENTATION             only implemented for Win7
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu
NVAPI_INTERFACE NvAPI_GPU_WorkstationFeatureQuery(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Out_opt_ NvU32 *pConfiguredFeatureMask, _Out_opt_ NvU32 *pConsistentFeatureMask);

/////////////////////////////////////////////////////////////////////////////// 
// 
// FUNCTION NAME: NvAPI_GPU_GetHDCPSupportStatus 
//
//! \fn NvAPI_GPU_GetHDCPSupportStatus(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_GET_HDCP_SUPPORT_STATUS *pGetHDCPSupportStatus)
//! DESCRIPTION: This function returns a GPU's HDCP support status. 
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 175
//!
//!  \retval ::NVAPI_OK 
//!  \retval ::NVAPI_ERROR 
//!  \retval ::NVAPI_INVALID_ARGUMENT 
//!  \retval ::NVAPI_HANDLE_INVALIDATED 
//!  \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE 
//!  \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION 
// 
////////////////////////////////////////////////////////////////////////////////
    

//! \addtogroup gpu
//! @{


//! HDCP fuse states - used in NV_GPU_GET_HDCP_SUPPORT_STATUS
typedef enum _NV_GPU_HDCP_FUSE_STATE
{
    NV_GPU_HDCP_FUSE_STATE_UNKNOWN  = 0,
    NV_GPU_HDCP_FUSE_STATE_DISABLED = 1,
    NV_GPU_HDCP_FUSE_STATE_ENABLED  = 2,
} NV_GPU_HDCP_FUSE_STATE;


//! HDCP key sources - used in NV_GPU_GET_HDCP_SUPPORT_STATUS
typedef enum _NV_GPU_HDCP_KEY_SOURCE
{
    NV_GPU_HDCP_KEY_SOURCE_UNKNOWN    = 0,
    NV_GPU_HDCP_KEY_SOURCE_NONE       = 1,
    NV_GPU_HDCP_KEY_SOURCE_CRYPTO_ROM = 2,
    NV_GPU_HDCP_KEY_SOURCE_SBIOS      = 3,
    NV_GPU_HDCP_KEY_SOURCE_I2C_ROM    = 4,
    NV_GPU_HDCP_KEY_SOURCE_FUSES      = 5,
} NV_GPU_HDCP_KEY_SOURCE;


//! HDCP key source states - used in NV_GPU_GET_HDCP_SUPPORT_STATUS
typedef enum _NV_GPU_HDCP_KEY_SOURCE_STATE
{
    NV_GPU_HDCP_KEY_SOURCE_STATE_UNKNOWN = 0,
    NV_GPU_HDCP_KEY_SOURCE_STATE_ABSENT  = 1,
    NV_GPU_HDCP_KEY_SOURCE_STATE_PRESENT = 2,
} NV_GPU_HDCP_KEY_SOURCE_STATE;


//! HDPC support status - used in NvAPI_GPU_GetHDCPSupportStatus()
typedef struct 
{
    NvU32                        version;               //! Structure version constucted by macro #NV_GPU_GET_HDCP_SUPPORT_STATUS
    NV_GPU_HDCP_FUSE_STATE       hdcpFuseState;         //! GPU's HDCP fuse state
    NV_GPU_HDCP_KEY_SOURCE       hdcpKeySource;         //! GPU's HDCP key source
    NV_GPU_HDCP_KEY_SOURCE_STATE hdcpKeySourceState;    //! GPU's HDCP key source state    
} NV_GPU_GET_HDCP_SUPPORT_STATUS;


//! Macro for constructing the version for structure NV_GPU_GET_HDCP_SUPPORT_STATUS
#define NV_GPU_GET_HDCP_SUPPORT_STATUS_VER MAKE_NVAPI_VERSION(NV_GPU_GET_HDCP_SUPPORT_STATUS,1)


//! @}


//!  \ingroup gpu 
NVAPI_INTERFACE NvAPI_GPU_GetHDCPSupportStatus(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_GET_HDCP_SUPPORT_STATUS *pGetHDCPSupportStatus);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetTachReading
//
//!   DESCRIPTION: This API retrieves the fan speed tachometer reading for the specified physical GPU.
//!
//!   HOW TO USE:   
//!                 - NvU32 Value = 0;
//!                 - ret = NvAPI_GPU_GetTachReading(hPhysicalGpu, &Value);  
//!                 - On call success:
//!                 - Value contains the tachometer reading
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \param [in]    hPhysicalGpu   GPU selection.
//! \param [out]   pValue         Pointer to a variable to get the tachometer reading
//!
//! \retval ::NVAPI_OK - completed request
//! \retval ::NVAPI_ERROR - miscellaneous error occurred
//! \retval ::NVAPI_NOT_SUPPORTED - functionality not supported 
//! \retval ::NVAPI_API_NOT_INTIALIZED - nvapi not initialized
//! \retval ::NVAPI_INVALID_ARGUMENT - invalid argument passed
//! \retval ::NVAPI_HANDLE_INVALIDATED - handle passed has been invalidated (see user guide)
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE - handle passed is not a physical GPU handle
//!
//! \ingroup gpucooler
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetTachReading(NvPhysicalGpuHandle hPhysicalGPU, NvU32 *pValue);

typedef NvAPI_Status (__cdecl *_NvAPI_GPU_GetTachReading)(NvPhysicalGpuHandle hPhysicalGPU, NvU32 *pValue);
_NvAPI_GPU_GetTachReading NvAPI_GPU_GetTachReading;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetECCStatusInfo
//
//! \fn NvAPI_GPU_GetECCStatusInfo(NvPhysicalGpuHandle hPhysicalGpu, 
//!                                           NV_GPU_ECC_STATUS_INFO *pECCStatusInfo);
//! DESCRIPTION:     This function returns ECC memory status information.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \param [in]      hPhysicalGpu      A handle identifying the physical GPU for which ECC 
//!                                    status information is to be retrieved.
//! \param [out]     pECCStatusInfo    A pointer to an ECC status structure.
//! 
//! \retval ::NVAPI_OK                  The request was completed successfully.
//! \retval ::NVAPI_ERROR               An unknown error occurred.
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE The provided GPU handle is not a physical GPU handle.
//! \retval ::NVAPI_INVALID_HANDLE      The provided GPU handle is invalid.
//! \retval ::NVAPI_HANDLE_INVALIDATED  The provided GPU handle is no longer valid.
//! \retval ::NVAPI_INVALID_POINTER     An invalid argument pointer was provided.
//! \retval ::NVAPI_NOT_SUPPORTED       The request is not supported.
//! \retval ::NVAPI_API_NOT_INTIALIZED  NvAPI was not yet initialized.
//
///////////////////////////////////////////////////////////////////////////////

//! \addtogroup gpuecc
//! Used in NV_GPU_ECC_STATUS_INFO.
typedef enum _NV_ECC_CONFIGURATION
{
    NV_ECC_CONFIGURATION_NOT_SUPPORTED = 0,
    NV_ECC_CONFIGURATION_DEFERRED,           //!< Changes require a POST to take effect
    NV_ECC_CONFIGURATION_IMMEDIATE,          //!< Changes can optionally be made to take effect immediately
} NV_ECC_CONFIGURATION;

//! \ingroup gpuecc
//! Used in NvAPI_GPU_GetECCStatusInfo().
typedef struct _NV_GPU_ECC_STATUS_INFO
{
    NvU32                 version;               //!< Structure version
    NvU32                 isSupported : 1;       //!< ECC memory feature support
    NV_ECC_CONFIGURATION  configurationOptions;  //!< Supported ECC memory feature configuration options
    NvU32                 isEnabled : 1;         //!< Active ECC memory setting
} NV_GPU_ECC_STATUS_INFO;

//! \ingroup gpuecc
//! Macro for constructing the version field of NV_GPU_ECC_STATUS_INFO
#define NV_GPU_ECC_STATUS_INFO_VER MAKE_NVAPI_VERSION(NV_GPU_ECC_STATUS_INFO, 1)

//! \ingroup gpuecc
NVAPI_INTERFACE NvAPI_GPU_GetECCStatusInfo(NvPhysicalGpuHandle hPhysicalGpu, 
                                           NV_GPU_ECC_STATUS_INFO *pECCStatusInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetECCErrorInfo
//
//! \fn NvAPI_GPU_GetECCErrorInfo(NvPhysicalGpuHandle hPhysicalGpu, 
//!                                          NV_GPU_ECC_ERROR_INFO *pECCErrorInfo);
//!
//! DESCRIPTION:     This function returns ECC memory error information.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \param [in]      hPhysicalGpu  A handle identifying the physical GPU for
//!                                which ECC error information is to be
//!                                retrieved.
//! \param [out]     pECCErrorInfo A pointer to an ECC error structure.
//! 
//! \retval ::NVAPI_OK  The request was completed successfully.
//! \retval ::NVAPI_ERROR  An unknown error occurred.
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  The provided GPU handle is not a physical GPU handle.
//! \retval ::NVAPI_INVALID_ARGUMENT  incorrect param value
//! \retval ::NVAPI_INVALID_POINTER  An invalid argument pointer was provided.
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  structure version is not supported, initialize to NV_GPU_ECC_ERROR_INFO_VER.
//! \retval ::NVAPI_HANDLE_INVALIDATED  The provided GPU handle is no longer valid.
//! \retval ::NVAPI_NOT_SUPPORTED  The request is not supported.
//! \retval ::NVAPI_API_NOT_INTIALIZED  NvAPI was not yet initialized.
//
///////////////////////////////////////////////////////////////////////////////


//! \ingroup gpuecc
//! Used in NvAPI_GPU_GetECCErrorInfo()/
typedef struct _NV_GPU_ECC_ERROR_INFO
{
    NvU32   version;             //!< Structure version
    struct {
        NvU64  singleBitErrors;  //!< Number of single-bit ECC errors detected since last boot
        NvU64  doubleBitErrors;  //!< Number of double-bit ECC errors detected since last boot
    } current;
    struct {
        NvU64  singleBitErrors;  //!< Number of single-bit ECC errors detected since last counter reset
        NvU64  doubleBitErrors;  //!< Number of double-bit ECC errors detected since last counter reset
    } aggregate;
} NV_GPU_ECC_ERROR_INFO;

//! \ingroup gpuecc
//! Macro for constructing the version field of NV_GPU_ECC_ERROR_INFO
#define NV_GPU_ECC_ERROR_INFO_VER MAKE_NVAPI_VERSION(NV_GPU_ECC_ERROR_INFO, 1)

//! \ingroup gpuecc
NVAPI_INTERFACE NvAPI_GPU_GetECCErrorInfo(NvPhysicalGpuHandle hPhysicalGpu, 
                                          NV_GPU_ECC_ERROR_INFO *pECCErrorInfo);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_ResetECCErrorInfo
//
//! DESCRIPTION:     This function resets ECC memory error counters.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \param [in]     hPhysicalGpu     A handle identifying the physical GPU for
//!                                  which ECC error information is to be
//!                                  cleared.
//! \param [in]     bResetCurrent    Reset the current ECC error counters.
//! \param [in]     bResetAggregate  Reset the aggregate ECC error counters.
//! 
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!          specific meaning for this API, they are listed below.
//!
//! \retval ::NVAPI_INVALID_USER_PRIVILEGE       - The caller does not have administrative privileges
//!
//! \ingroup gpuecc
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_ResetECCErrorInfo(NvPhysicalGpuHandle hPhysicalGpu, NvU8 bResetCurrent,
                                            NvU8 bResetAggregate);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetECCConfigurationInfo
//
//! \fn NvAPI_GPU_GetECCConfigurationInfo(NvPhysicalGpuHandle hPhysicalGpu, 
//!                             NV_GPU_ECC_CONFIGURATION_INFO *pECCConfigurationInfo);
//! DESCRIPTION:     This function returns ECC memory configuration information.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \param [in]      hPhysicalGpu  A handle identifying the physical GPU for
//!                                which ECC configuration information
//!                               is to be retrieved.
//! \param [out]     pECCConfigurationInfo  A pointer to an ECC 
//!                                                configuration structure.
//! 
//! \retval ::NVAPI_OK  The request was completed successfully.
//! \retval ::NVAPI_ERROR  An unknown error occurred.
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  The provided GPU handle is not a physical GPU handle.
//! \retval ::NVAPI_INVALID_HANDLE  The provided GPU handle is invalid.
//! \retval ::NVAPI_HANDLE_INVALIDATED  The provided GPU handle is no longer valid.
//! \retval ::NVAPI_INVALID_POINTER  An invalid argument pointer was provided.
//! \retval ::NVAPI_NOT_SUPPORTED  The request is not supported.
//! \retval ::NVAPI_API_NOT_INTIALIZED  NvAPI was not yet initialized.
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup gpuecc
//! Used in NvAPI_GPU_GetECCConfigurationInfo(). 
typedef struct _NV_GPU_ECC_CONFIGURATION_INFO
{
    NvU32  version;                 //! Structure version
    NvU32  isEnabled : 1;           //! Current ECC configuration stored in non-volatile memory
    NvU32  isEnabledByDefault : 1;  //! Factory default ECC configuration (static)
} NV_GPU_ECC_CONFIGURATION_INFO;

//! \ingroup gpuecc
//! Macro for consstructing the verion field of NV_GPU_ECC_CONFIGURATION_INFO
#define NV_GPU_ECC_CONFIGURATION_INFO_VER MAKE_NVAPI_VERSION(NV_GPU_ECC_CONFIGURATION_INFO,1)

//! \ingroup gpuecc
NVAPI_INTERFACE NvAPI_GPU_GetECCConfigurationInfo(NvPhysicalGpuHandle hPhysicalGpu, 
                                                  NV_GPU_ECC_CONFIGURATION_INFO *pECCConfigurationInfo);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_SetECCConfiguration
//
//! DESCRIPTION:     This function updates the ECC memory configuration setting.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! TCC_SUPPORTED
//!
//! \param [in]      hPhysicalGpu    A handle identifying the physical GPU for
//!                                  which to update the ECC configuration
//!                                  setting.
//! \param [in]      bEnable         The new ECC configuration setting.
//! \param [in]      bEnableImmediately   Request that the new setting take effect immediately.
//! 
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!          specific meaning for this API, they are listed below.
//!
//! \retval ::NVAPI_INVALID_CONFIGURATION  - Possibly SLI is enabled. Disable SLI and retry.
//! \retval ::NVAPI_INVALID_USER_PRIVILEGE - The caller does not have administrative privileges
//!
//! \ingroup gpuecc
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_SetECCConfiguration(NvPhysicalGpuHandle hPhysicalGpu, NvU8 bEnable,
                                              NvU8 bEnableImmediately);



//! \ingroup gpu
typedef struct
{
    NvU32  version;              //!< version of this structure
    NvU32  width;                //!< width of the input texture
    NvU32  height;               //!< height of the input texture
    float* blendingTexture;      //!< array of floating values building an intensity RGB texture
} NV_SCANOUT_INTENSITY_DATA_V1;

//! \ingroup gpu
typedef struct
{
    NvU32  version;              //!< version of this structure
    NvU32  width;                //!< width of the input texture
    NvU32  height;               //!< height of the input texture
    float* blendingTexture;      //!< array of floating values building an intensity RGB texture
    float* offsetTexture;        //!< array of floating values building an offset texture
    NvU32  offsetTexChannels;    //!< number of channels per pixel in the offset texture
} NV_SCANOUT_INTENSITY_DATA_V2;

typedef NV_SCANOUT_INTENSITY_DATA_V2 NV_SCANOUT_INTENSITY_DATA;

//! \ingroup gpu
#define NV_SCANOUT_INTENSITY_DATA_VER1    MAKE_NVAPI_VERSION(NV_SCANOUT_INTENSITY_DATA_V1, 1)
#define NV_SCANOUT_INTENSITY_DATA_VER2    MAKE_NVAPI_VERSION(NV_SCANOUT_INTENSITY_DATA_V2, 2)
#define NV_SCANOUT_INTENSITY_DATA_VER      NV_SCANOUT_INTENSITY_DATA_VER2

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NvAPI_GPU_SetScanoutIntensity
//
//!   DESCRIPTION: This API enables and sets up per-pixel intensity feature on the specified display.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \param [in]   displayId              combined physical display and GPU identifier of the display to apply the intensity control.
//! \param [in]   scanoutIntensityData   the intensity texture info.
//! \param [out]  pbSticky(OUT)           indicates whether the settings will be kept over a reboot.
//!
//! \retval ::NVAPI_INVALID_ARGUMENT Invalid input parameters.
//! \retval ::NVAPI_API_NOT_INITIALIZED NvAPI not initialized.
//! \retval ::NVAPI_NOT_SUPPORTED Interface not supported by the driver used, or only supported on selected GPUs
//! \retval ::NVAPI_INVALID_ARGUMENT Invalid input data.
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION NV_SCANOUT_INTENSITY_DATA structure version mismatch.
//! \retval ::NVAPI_OK Feature enabled.
//! \retval ::NVAPI_ERROR Miscellaneous error occurred.
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_SetScanoutIntensity(NvU32 displayId, NV_SCANOUT_INTENSITY_DATA* scanoutIntensityData, int *pbSticky);


//! \ingroup gpu
typedef struct _NV_SCANOUT_INTENSITY_STATE_DATA
{
    NvU32  version;                                 //!< version of this structure
    NvU32  bEnabled;                                //!< intensity is enabled or not
} NV_SCANOUT_INTENSITY_STATE_DATA;

//! \ingroup gpu
#define NV_SCANOUT_INTENSITY_STATE_VER    MAKE_NVAPI_VERSION(NV_SCANOUT_INTENSITY_STATE_DATA, 1)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NvAPI_GPU_GetScanoutIntensityState
//
//!   DESCRIPTION: This API queries current state of the intensity feature on the specified display.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \param [in]     displayId                       combined physical display and GPU identifier of the display to query the configuration.
//! \param [in,out] scanoutIntensityStateData       intensity state data.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!          specific meaning for this API, they are listed below.
//!
//! \retval ::NVAPI_INVALID_ARGUMENT Invalid input parameters.
//! \retval ::NVAPI_API_NOT_INITIALIZED NvAPI not initialized.
//! \retval ::NVAPI_NOT_SUPPORTED Interface not supported by the driver used, or only supported on selected GPUs.
//! \retval ::NVAPI_OK Feature enabled.
//! \retval ::NVAPI_ERROR Miscellaneous error occurred.
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetScanoutIntensityState(_In_ NvU32 displayId, _Inout_ NV_SCANOUT_INTENSITY_STATE_DATA* scanoutIntensityStateData);


//! \ingroup gpu
typedef enum
{
    NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLESTRIP_XYUVRQ = 0,
    NV_GPU_WARPING_VERTICE_FORMAT_TRIANGLES_XYUVRQ     = 1,
} NV_GPU_WARPING_VERTICE_FORMAT;

//! \ingroup gpu
typedef struct
{
    NvU32  version;                                 //!< version of this structure
    float* vertices;                                //!< width of the input texture
    NV_GPU_WARPING_VERTICE_FORMAT vertexFormat;     //!< format of the input vertices
    int    numVertices;                             //!< number of the input vertices
    NvSBox* textureRect;                            //!< rectangle in desktop coordinates describing the source area for the warping
} NV_SCANOUT_WARPING_DATA;

//! \ingroup gpu
#define NV_SCANOUT_WARPING_VER    MAKE_NVAPI_VERSION(NV_SCANOUT_WARPING_DATA, 1)


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NvAPI_GPU_SetScanoutWarping
//
//!   DESCRIPTION: This API enables and sets up the warping feature on the specified display.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \param [in]    displayId               Combined physical display and GPU identifier of the display to apply the intensity control
//! \param [in]    scanoutWarpingData      The warping data info
//! \param [out]   pbSticky                Indicates whether the settings will be kept over a reboot.
//!
//! \retval ::NVAPI_INVALID_ARGUMENT Invalid input parameters.
//! \retval ::NVAPI_API_NOT_INITIALIZED NvAPI not initialized.
//! \retval ::NVAPI_NOT_SUPPORTED Interface not supported by the driver used, or only supported on selected GPUs
//! \retval ::NVAPI_INVALID_ARGUMENT Invalid input data.
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION NV_SCANOUT_INTENSITY_DATA structure version mismatch.
//! \retval ::NVAPI_OK Feature enabled.
//! \retval ::NVAPI_ERROR Miscellaneous error occurred.
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////

NVAPI_INTERFACE NvAPI_GPU_SetScanoutWarping(NvU32 displayId, NV_SCANOUT_WARPING_DATA* scanoutWarpingData, int* piMaxNumVertices, int* pbSticky);


//! \ingroup gpu
typedef struct _NV_SCANOUT_WARPING_STATE_DATA
{
    NvU32  version;                                  //!< version of this structure
    NvU32  bEnabled;                                 //!< warping is enabled or not
} NV_SCANOUT_WARPING_STATE_DATA;

//! \ingroup gpu
#define NV_SCANOUT_WARPING_STATE_VER    MAKE_NVAPI_VERSION(NV_SCANOUT_WARPING_STATE_DATA, 1)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NvAPI_GPU_GetScanoutWarpingState
//
//!   DESCRIPTION: This API queries current state of the warping feature on the specified display.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \param [in]     displayId                      combined physical display and GPU identifier of the display to query the configuration.
//! \param [in,out] scanoutWarpingStateData        warping state data.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!          specific meaning for this API, they are listed below.
//!
//! \retval ::NVAPI_INVALID_ARGUMENT Invalid input parameters.
//! \retval ::NVAPI_API_NOT_INITIALIZED NvAPI not initialized.
//! \retval ::NVAPI_NOT_SUPPORTED Interface not supported by the driver used, or only supported on selected GPUs.
//! \retval ::NVAPI_OK Feature enabled.
//! \retval ::NVAPI_ERROR Miscellaneous error occurred.
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetScanoutWarpingState(_In_ NvU32 displayId, _Inout_ NV_SCANOUT_WARPING_STATE_DATA* scanoutWarpingStateData);


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NvAPI_GPU_GetScanoutConfiguration
//
//!   DESCRIPTION: This API queries the desktop and scanout portion of the specified display.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \param [in]     displayId          combined physical display and GPU identifier of the display to query the configuration.
//! \param [in,out] desktopRect        desktop area of the display in desktop coordinates.
//! \param [in,out] scanoutRect        scanout area of the display relative to desktopRect.
//!
//! \retval ::NVAPI_INVALID_ARGUMENT Invalid input parameters.
//! \retval ::NVAPI_API_NOT_INITIALIZED NvAPI not initialized.
//! \retval ::NVAPI_NOT_SUPPORTED Interface not supported by the driver used, or only supported on selected GPUs.
//! \retval ::NVAPI_OK Feature enabled.
//! \retval ::NVAPI_ERROR Miscellaneous error occurred.
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetScanoutConfiguration(NvU32 displayId, NvSBox* desktopRect, NvSBox* scanoutRect);



//! \ingroup gpu
//! Used in NvAPI_GPU_GetScanoutConfigurationEx().
typedef struct _NV_SCANOUT_INFORMATION
{
    NvU32      version;                 //!< Structure version, needs to be initialized with NV_SCANOUT_INFORMATION_VER.

    NvSBox     sourceDesktopRect;       //!< Operating system display device rect in desktop coordinates displayId is scanning out from.
    NvSBox     sourceViewportRect;      //!< Area inside the sourceDesktopRect which is scanned out to the display.
    NvSBox     targetViewportRect;      //!< Area inside the rect described by targetDisplayWidth/Height sourceViewportRect is scanned out to.
    NvU32      targetDisplayWidth;      //!< Horizontal size of the active resolution scanned out to the display.
    NvU32      targetDisplayHeight;     //!< Vertical size of the active resolution scanned out to the display.
    NvU32      cloneImportance;         //!< If targets are cloned views of the sourceDesktopRect the cloned targets have an imporantce assigned (0:primary,1 secondary,...).
    NV_ROTATE  sourceToTargetRotation;  //!< Rotation performed between the sourceViewportRect and the targetViewportRect.
} NV_SCANOUT_INFORMATION;

#define NV_SCANOUT_INFORMATION_VER  MAKE_NVAPI_VERSION(NV_SCANOUT_INFORMATION,1)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME: NvAPI_GPU_GetScanoutConfigurationEx
//
//!   DESCRIPTION: This API queries the desktop and scanout portion of the specified display.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//! \since Release: 331
//!
//! \param [in]     displayId            combined physical display and GPU identifier of the display to query the configuration.
//! \param [in,out] pScanoutInformation  desktop area to displayId mapping information.
//!
//! \return This API can return any of the error codes enumerated in #NvAPI_Status. 
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetScanoutConfigurationEx(_In_ NvU32 displayId, _Inout_ NV_SCANOUT_INFORMATION *pScanoutInformation);


//! Used in NvAPI_GPU_GetPerfDecreaseInfo.
//! Bit masks for knowing the exact reason for performance decrease
typedef enum _NVAPI_GPU_PERF_DECREASE
{
    NV_GPU_PERF_DECREASE_NONE                        = 0,          //!< No Slowdown detected
    NV_GPU_PERF_DECREASE_REASON_THERMAL_PROTECTION   = 0x00000001, //!< Thermal slowdown/shutdown/POR thermal protection
    NV_GPU_PERF_DECREASE_REASON_POWER_CONTROL        = 0x00000002, //!< Power capping / pstate cap 
    NV_GPU_PERF_DECREASE_REASON_AC_BATT              = 0x00000004, //!< AC->BATT event
    NV_GPU_PERF_DECREASE_REASON_API_TRIGGERED        = 0x00000008, //!< API triggered slowdown
    NV_GPU_PERF_DECREASE_REASON_INSUFFICIENT_POWER   = 0x00000010, //!< Power connector missing
    NV_GPU_PERF_DECREASE_REASON_UNKNOWN              = 0x80000000, //!< Unknown reason
} NVAPI_GPU_PERF_DECREASE;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPerfDecreaseInfo
//
//! DESCRIPTION:   This function retrieves - in NvU32 variable - reasons for the current performance decrease.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//! \param [in]      hPhysicalGPU    (IN)    - GPU for which performance decrease is to be evaluated.
//! \param [out]  pPerfDecrInfo    (OUT)    - Pointer to a NvU32 variable containing performance decrease info
//!
//! \return      This API can return any of the error codes enumerated in #NvAPI_Status. 
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetPerfDecreaseInfo(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Inout_ NvU32 *pPerfDecrInfo);

typedef __success(return == NVAPI_OK) NvAPI_Status (__cdecl *_NvAPI_GPU_GetPerfDecreaseInfo)(_In_ NvPhysicalGpuHandle hPhysicalGpu, _Inout_ NVAPI_GPU_PERF_DECREASE* pPerfDecrInfo);
_NvAPI_GPU_GetPerfDecreaseInfo NvAPI_GPU_GetPerfDecreaseInfo;


//! \ingroup gpu
typedef enum _NV_GPU_ILLUMINATION_ATTRIB
{
    NV_GPU_IA_LOGO_BRIGHTNESS  = 0,
    NV_GPU_IA_SLI_BRIGHTNESS   = 1,
} NV_GPU_ILLUMINATION_ATTRIB;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_QueryIlluminationSupport
//
//! \fn NvAPI_GPU_QueryIlluminationSupport(_Inout_  NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM *pIlluminationSupportInfo)
//! DESCRIPTION:   This function reports if the specified illumination attribute is supported.
//!
//! \note Only a single GPU can manage an given attribute on a given HW element,
//!       regardless of how many are attatched. I.E. only one GPU will be used to control
//!       the brightness of the LED on an SLI bridge, regardless of how many are physicaly attached.
//!       You should enumerate thru the GPUs with this call to determine which GPU is managing the attribute.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//! \since Version: 300.05
//!
//! \param [in]  hPhysicalGpu        Physical GPU handle
//! \param       Attribute           An enumeration value specifying the Illumination attribute to be querried
//! \param [out] pSupported          A boolean indicating if the attribute is supported.
//! 
//! \return See \ref nvapistatus for the list of possible return values.
//
//////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu                 
typedef struct _NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_V1 {

    // IN
    NvU32   version;						//!< Version of this structure
    NvPhysicalGpuHandle hPhysicalGpu;		//!< The handle of the GPU that you are checking for the specified attribute.
                                            //!< note that this is the GPU that is managing the attribute.
                                            //!< Only a single GPU can manage an given attribute on a given HW element,
                                            //!< regardless of how many are attatched.
                                            //!< I.E. only one GPU will be used to control the brightness of the LED on an SLI bridge,
                                            //!< regardless of how many are physicaly attached.
                                            //!< You enumerate thru the GPUs with this call to determine which GPU is managing the attribute.
    NV_GPU_ILLUMINATION_ATTRIB Attribute;   //!< An enumeration value specifying the Illumination attribute to be querried.
                                            //!<     refer to enum \ref NV_GPU_ILLUMINATION_ATTRIB.
    
    // OUT
    NvU32    bSupported;                    //!< A boolean indicating if the attribute is supported.
                                    
} NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_V1;

//! \ingroup gpu 
typedef NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_V1      NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM;
//! \ingroup gpu 
#define NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_VER_1   MAKE_NVAPI_VERSION(NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_V1,1)
//! \ingroup gpu 
#define NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_VER     NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM_VER_1

//! \ingroup gpu 
NVAPI_INTERFACE NvAPI_GPU_QueryIlluminationSupport(_Inout_ NV_GPU_QUERY_ILLUMINATION_SUPPORT_PARM *pIlluminationSupportInfo);




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetIllumination
//
//! \fn NvAPI_GPU_GetIllumination(NV_GPU_GET_ILLUMINATION_PARM *pIlluminationInfo)
//! DESCRIPTION:   This function reports value of the specified illumination attribute.
//!
//! \note Only a single GPU can manage an given attribute on a given HW element,
//!       regardless of how many are attatched. I.E. only one GPU will be used to control
//!       the brightness of the LED on an SLI bridge, regardless of how many are physicaly attached.
//!       You should enumerate thru the GPUs with the \ref NvAPI_GPU_QueryIlluminationSupport call to
//!       determine which GPU is managing the attribute.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//! \since Version: 300.05
//!
//! \param [in]  hPhysicalGpu        Physical GPU handle
//! \param       Attribute           An enumeration value specifying the Illumination attribute to be querried
//! \param [out] Value               A ULONG containing the current value for the specified attribute.
//!                                  This is specified as a percentage of the full range of the attribute
//!                                  (0-100; 0 = off, 100 = full brightness)
//! 
//! \return See \ref nvapistatus for the list of possible return values. Return values of special interest are:
//!             NVAPI_INVALID_ARGUMENT The specified attibute is not known to the driver.
//!             NVAPI_NOT_SUPPORTED:   The specified attribute is not supported on the specified GPU
//
//////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu                 
typedef struct _NV_GPU_GET_ILLUMINATION_PARM_V1 {

    // IN
    NvU32   version;						//!< Version of this structure
    NvPhysicalGpuHandle hPhysicalGpu;		//!< The handle of the GPU that you are checking for the specified attribute.
                                            //!< Note that this is the GPU that is managing the attribute.
                                            //!< Only a single GPU can manage an given attribute on a given HW element,
                                            //!< regardless of how many are attatched.
                                            //!< I.E. only one GPU will be used to control the brightness of the LED on an SLI bridge,
                                            //!< regardless of how many are physicaly attached.
                                            //!< You enumerate thru the GPUs with this call to determine which GPU is managing the attribute.
    NV_GPU_ILLUMINATION_ATTRIB Attribute;   //!< An enumeration value specifying the Illumination attribute to be querried.
                                            //!< refer to enum \ref NV_GPU_ILLUMINATION_ATTRIB.
    
    // OUT
    NvU32    Value;                         //!< A ULONG that will contain the current value of the specified attribute.
                                            //! This is specified as a percentage of the full range of the attribute
                                            //! (0-100; 0 = off, 100 = full brightness)
                                    
} NV_GPU_GET_ILLUMINATION_PARM_V1;

//! \ingroup gpu 
typedef NV_GPU_GET_ILLUMINATION_PARM_V1      NV_GPU_GET_ILLUMINATION_PARM;
//! \ingroup gpu 
#define NV_GPU_GET_ILLUMINATION_PARM_VER_1   MAKE_NVAPI_VERSION(NV_GPU_GET_ILLUMINATION_PARM_V1,1)
//! \ingroup gpu 
#define NV_GPU_GET_ILLUMINATION_PARM_VER     NV_GPU_GET_ILLUMINATION_PARM_VER_1

//! \ingroup gpu 
NVAPI_INTERFACE NvAPI_GPU_GetIllumination(NV_GPU_GET_ILLUMINATION_PARM *pIlluminationInfo);




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_SetIllumination
//
//! \fn NvAPI_GPU_SetIllumination(NV_GPU_SET_ILLUMINATION_PARM *pIlluminationInfo)
//! DESCRIPTION:   This function sets the value of the specified illumination attribute.
//!
//! \note Only a single GPU can manage an given attribute on a given HW element,
//!       regardless of how many are attatched. I.E. only one GPU will be used to control
//!       the brightness of the LED on an SLI bridge, regardless of how many are physicaly attached.
//!       You should enumerate thru the GPUs with the \ref NvAPI_GPU_QueryIlluminationSupport call to
//!       determine which GPU is managing the attribute.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//! \since Version: 300.05
//!
//! \param [in]  hPhysicalGpu        Physical GPU handle
//! \param       Attribute           An enumeration value specifying the Illumination attribute to be set
//! \param       Value               The new value for the specified attribute.
//!                                  This should be specified as a percentage of the full range of the attribute
//!                                  (0-100; 0 = off, 100 = full brightness)
//!                                  If a value is specified outside this range, NVAPI_INVALID_ARGUMENT will be returned.
//! 
//! \return See \ref nvapistatus for the list of possible return values. Return values of special interest are:
//!             NVAPI_INVALID_ARGUMENT	The specified attibute is not known to the driver, or the specified value is out of range.
//!             NVAPI_NOT_SUPPORTED     The specified attribute is not supported on the specified GPU.
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu                
typedef struct _NV_GPU_SET_ILLUMINATION_PARM_V1 {

    // IN
    NvU32   version;						//!< Version of this structure
    NvPhysicalGpuHandle hPhysicalGpu;		//!< The handle of the GPU that you are checking for the specified attribute.
                                            //!< Note that this is the GPU that is managing the attribute.
                                            //!< Only a single GPU can manage an given attribute on a given HW element,
                                            //!< regardless of how many are attatched.
                                            //!< I.E. only one GPU will be used to control the brightness of the LED on an SLI bridge,
                                            //!< regardless of how many are physicaly attached.
                                            //!< You enumerate thru the GPUs with this call to determine which GPU is managing the attribute.
    NV_GPU_ILLUMINATION_ATTRIB Attribute;   //!< An enumeration value specifying the Illumination attribute to be querried.
                                            //!< refer to enum \ref NV_GPU_ILLUMINATION_ATTRIB.
    NvU32    Value;                         //!< A ULONG containing the new value for the specified attribute.
                                            //!< This should be specified as a percentage of the full range of the attribute
                                            //!< (0-100; 0 = off, 100 = full brightness)
                                            //!< If a value is specified outside this range, NVAPI_INVALID_ARGUMENT will be returned.
    
    // OUT
                                    
} NV_GPU_SET_ILLUMINATION_PARM_V1;

//! \ingroup gpu 
typedef NV_GPU_SET_ILLUMINATION_PARM_V1      NV_GPU_SET_ILLUMINATION_PARM;
//! \ingroup gpu 
#define NV_GPU_SET_ILLUMINATION_PARM_VER_1   MAKE_NVAPI_VERSION(NV_GPU_SET_ILLUMINATION_PARM_V1,1)
//! \ingroup gpu 
#define NV_GPU_SET_ILLUMINATION_PARM_VER     NV_GPU_SET_ILLUMINATION_PARM_VER_1

//! \ingroup gpu 
NVAPI_INTERFACE NvAPI_GPU_SetIllumination(NV_GPU_SET_ILLUMINATION_PARM *pIlluminationInfo);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumNvidiaDisplayHandle
//
//! This function returns the handle of the NVIDIA display specified by the enum 
//!                index (thisEnum). The client should keep enumerating until it
//!                returns NVAPI_END_ENUMERATION.
//!
//!                Note: Display handles can get invalidated on a modeset, so the calling applications need to
//!                renum the handles after every modeset.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \param [in]  thisEnum      The index of the NVIDIA display.   
//! \param [out] pNvDispHandle Pointer to the NVIDIA display handle.
//!
//! \retval NVAPI_INVALID_ARGUMENT        Either the handle pointer is NULL or enum index too big
//! \retval NVAPI_OK                      Return a valid NvDisplayHandle based on the enum index
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND No NVIDIA device found in the system
//! \retval NVAPI_END_ENUMERATION         No more display device to enumerate
//! \ingroup disphandle
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_EnumNvidiaDisplayHandle(NvU32 thisEnum, NvDisplayHandle *pNvDispHandle);

typedef NvAPI_Status (__cdecl *_NvAPI_EnumNvidiaDisplayHandle)(NvU32 thisEnum, NvDisplayHandle *pNvDispHandle);
_NvAPI_EnumNvidiaDisplayHandle NvAPI_EnumNvidiaDisplayHandle;




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnumNvidiaUnAttachedDisplayHandle
//
//! This function returns the handle of the NVIDIA unattached display specified by the enum 
//!                index (thisEnum). The client should keep enumerating until it
//!                returns error.
//!                Note: Display handles can get invalidated on a modeset, so the calling applications need to
//!                renum the handles after every modeset.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \param [in]  thisEnum                  The index of the NVIDIA display.
//! \param [out] pNvUnAttachedDispHandle   Pointer to the NVIDIA display handle of the unattached display.
//!
//! \retval NVAPI_INVALID_ARGUMENT         Either the handle pointer is NULL or enum index too big
//! \retval NVAPI_OK                       Return a valid NvDisplayHandle based on the enum index
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA device found in the system
//! \retval NVAPI_END_ENUMERATION          No more display device to enumerate.
//! \ingroup disphandle
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnumNvidiaUnAttachedDisplayHandle(NvU32 thisEnum, NvUnAttachedDisplayHandle *pNvUnAttachedDispHandle);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_CreateDisplayFromUnAttachedDisplay
//
//! This function converts the unattached display handle to an active attached display handle.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvUnAttachedDisp is not valid or pNvDisplay is NULL.
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_CreateDisplayFromUnAttachedDisplay(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvDisplayHandle *pNvDisplay);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetAssociatedNVidiaDisplayHandle
//
//!  This function returns the handle of the NVIDIA display that is associated
//!  with the given display "name" (such as "\\.\DISPLAY1").
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT         Either argument is NULL
//! \retval NVAPI_OK                      *pNvDispHandle is now valid
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA device maps to that display name
//! \ingroup disphandle
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetAssociatedNvidiaDisplayHandle(const char *szDisplayName, NvDisplayHandle *pNvDispHandle);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DISP_GetAssociatedUnAttachedNvidiaDisplayHandle
//
//!   DESCRIPTION: This function returns the handle of an unattached NVIDIA display that is 
//!                associated with the given display name (such as "\\DISPLAY1"). 
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 185
//!
//! \retval ::NVAPI_INVALID_ARGUMENT          Either argument is NULL.
//! \retval ::NVAPI_OK                       *pNvUnAttachedDispHandle is now valid.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND   No NVIDIA device maps to that display name.
//!
//! \ingroup disphandle
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_GetAssociatedUnAttachedNvidiaDisplayHandle(const char *szDisplayName, NvUnAttachedDisplayHandle *pNvUnAttachedDispHandle);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetAssociatedNVidiaDisplayName
//
//!  For a given NVIDIA display handle, this function returns a string (such as "\\.\DISPLAY1") to identify the display.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \retval NVAPI_INVALID_ARGUMENT          Either argument is NULL
//! \retval NVAPI_OK                       *pNvDispHandle is now valid
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND   No NVIDIA device maps to that display name
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetAssociatedNvidiaDisplayName(NvDisplayHandle NvDispHandle, NvAPI_ShortString szDisplayName);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetUnAttachedAssociatedDisplayName
//
//!  This function returns the display name given, for example, "\\DISPLAY1", using the unattached NVIDIA display handle
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 95
//!
//! \retval NVAPI_INVALID_ARGUMENT          Either argument is NULL
//! \retval NVAPI_OK                       *pNvDispHandle is now valid
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND   No NVIDIA device maps to that display name
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetUnAttachedAssociatedDisplayName(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvAPI_ShortString szDisplayName);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_EnableHWCursor
//
//!  This function enables hardware cursor support
//!
//! SUPPORTED OS:  Windows XP
//!
//!  
//!
//! \since Release: 80
//!
//! \return NVAPI_ERROR or NVAPI_OK
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnableHWCursor(NvDisplayHandle hNvDisplay);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DisableHWCursor
//
//! This function disables hardware cursor support
//!
//! SUPPORTED OS:  Windows XP
//!
//!
//! \since Release: 80
//!
//! \return  NVAPI_ERROR or NVAPI_OK
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DisableHWCursor(NvDisplayHandle hNvDisplay);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetVBlankCounter
//
//!  This function gets the V-blank counter
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 80
//!
//! \return NVAPI_ERROR or NVAPI_OK
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetVBlankCounter(NvDisplayHandle hNvDisplay, NvU32 *pCounter);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:  NvAPI_SetRefreshRateOverride
//
//!  This function overrides the refresh rate on the given display/outputsMask.
//!  The new refresh rate can be applied right away in this API call or deferred to be applied with the
//!  next OS modeset. The override is good for only one modeset (regardless whether it's deferred or immediate).
//!               
//!
//! SUPPORTED OS:  Windows XP
//!
//!
//! \since Release: 80
//!
//!  \param [in] hNvDisplay    The NVIDIA display handle. It can be NVAPI_DEFAULT_HANDLE or a handle
//!                           enumerated from NvAPI_EnumNVidiaDisplayHandle().
//!  \param [in] outputsMask  A set of bits that identify all target outputs which are associated with the NVIDIA 
//!                           display handle to apply the refresh rate override. When SLI is enabled, the
//!                           outputsMask only applies to the GPU that is driving the display output.
//!  \param [in] refreshRate  The override value. "0.0" means cancel the override.
//!  \param [in] bSetDeferred 
//!              - "0": Apply the refresh rate override immediately in this API call.\p
//!              - "1": Apply refresh rate at the next OS modeset.
//!
//!  \retval  NVAPI_INVALID_ARGUMENT hNvDisplay or outputsMask is invalid
//!  \retval  NVAPI_OK               The refresh rate override is correct set
//!  \retval  NVAPI_ERROR            The operation failed
//!  \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SetRefreshRateOverride(NvDisplayHandle hNvDisplay, NvU32 outputsMask, float refreshRate, NvU32 bSetDeferred);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetAssociatedDisplayOutputId
//
//! This function gets the active outputId associated with the display handle.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 90
//!
//! \param [in]  hNvDisplay  NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//! \param [out] outputId    The active display output ID associated with the selected display handle hNvDisplay.
//!                          The outputid will have only one bit set. In the case of Clone or Span mode, this will indicate the
//!                          display outputId of the primary display that the GPU is driving. See \ref handles.
//!
//! \retval  NVAPI_OK                      Call successful.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_DISPLAY_HANDLE hNvDisplay is not a valid display handle.
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetAssociatedDisplayOutputId(NvDisplayHandle hNvDisplay, NvU32 *pOutputId);


//! \ingroup dispcontrol
//! Used in NvAPI_GetDisplayPortInfo().
typedef struct
{
    NvU32               version;                     //!< Structure version
    NvU32               dpcd_ver;                    //!< DPCD version of the monitor
    NV_DP_LINK_RATE     maxLinkRate;                 //!< Maximum supported link rate
    NV_DP_LANE_COUNT    maxLaneCount;                //!< Maximum supported lane count
    NV_DP_LINK_RATE     curLinkRate;                 //!< Current link rate
    NV_DP_LANE_COUNT    curLaneCount;                //!< Current lane count
    NV_DP_COLOR_FORMAT  colorFormat;                 //!< Current color format
    NV_DP_DYNAMIC_RANGE dynamicRange;                //!< Dynamic range
    NV_DP_COLORIMETRY   colorimetry;                 //!< Ignored in RGB space
    NV_DP_BPC           bpc;                         //!< Current bit-per-component;
    NvU32               isDp                   : 1;  //!< If the monitor is driven by a DisplayPort 
    NvU32               isInternalDp           : 1;  //!< If the monitor is driven by an NV Dp transmitter
    NvU32               isColorCtrlSupported   : 1;  //!< If the color format change is supported
    NvU32               is6BPCSupported        : 1;  //!< If 6 bpc is supported
    NvU32               is8BPCSupported        : 1;  //!< If 8 bpc is supported    
    NvU32               is10BPCSupported       : 1;  //!< If 10 bpc is supported
    NvU32               is12BPCSupported       : 1;  //!< If 12 bpc is supported        
    NvU32               is16BPCSupported       : 1;  //!< If 16 bpc is supported
    NvU32               isYCrCb422Supported    : 1;  //!< If YCrCb422 is supported                                                  
    NvU32               isYCrCb444Supported    : 1;  //!< If YCrCb444 is supported
    
 } NV_DISPLAY_PORT_INFO; 

//! Macro for constructing the version field of NV_DISPLAY_PORT_INFO.
#define NV_DISPLAY_PORT_INFO_VER  MAKE_NVAPI_VERSION(NV_DISPLAY_PORT_INFO,1)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetDisplayPortInfo
//
//! \fn NvAPI_GetDisplayPortInfo(_In_opt_ NvDisplayHandle hNvDisplay, _In_ NvU32 outputId, _Inout_ NV_DISPLAY_PORT_INFO *pInfo)
//! DESCRIPTION:     This function returns the current DisplayPort-related information on the specified device (monitor).
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 165
//!
//! \param [in]     hvDisplay     NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//!                               This parameter is ignored when the outputId is a NvAPI displayId.
//! \param [in]     outputId      This can either be the connection bit mask or the NvAPI displayId. When the legacy connection bit mask is passed, 
//!                               it should have exactly 1 bit set to indicate a single display. If it's "0" then the default outputId from 
//!                               NvAPI_GetAssociatedDisplayOutputId() will be used. See \ref handles.
//! \param [out]    pInfo         The DisplayPort information
//!
//! \retval         NVAPI_OK                Completed request
//! \retval         NVAPI_ERROR             Miscellaneous error occurred
//! \retval         NVAPI_INVALID_ARGUMENT  Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////
//! \ingroup        dispcontrol
NVAPI_INTERFACE NvAPI_GetDisplayPortInfo(_In_opt_ NvDisplayHandle hNvDisplay, _In_ NvU32 outputId, _Inout_ NV_DISPLAY_PORT_INFO *pInfo);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SetDisplayPort
//
//! \fn NvAPI_SetDisplayPort(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_DISPLAY_PORT_CONFIG *pCfg)
//! DESCRIPTION:     This function sets up DisplayPort-related configurations.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release:   165
//!
//! \param [in]       hNvDisplay  NVIDIA display handle. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from
//!                               NvAPI_EnumNVidiaDisplayHandle().
//! \param [in]       outputId    This display output ID, when it's "0" it means the default outputId generated from the return of
//!                               NvAPI_GetAssociatedDisplayOutputId(). See \ref handles.
//! \param [in]       pCfg        The display port config structure. If pCfg is NULL, it means to use the driver's default value to setup.
//!
//! \retval           NVAPI_OK                Completed request
//! \retval           NVAPI_ERROR             Miscellaneous error occurred
//! \retval           NVAPI_INVALID_ARGUMENT  Invalid input parameter
///////////////////////////////////////////////////////////////////////////////


//! \ingroup dispcontrol
//! DisplayPort configuration settings - used in NvAPI_SetDisplayPort().
typedef struct _NV_DISPLAY_PORT_CONFIG
{
    NvU32               version;                     //!< Structure version - 2 is the latest
    NV_DP_LINK_RATE     linkRate;                    //!< Link rate
    NV_DP_LANE_COUNT    laneCount;                   //!< Lane count
    NV_DP_COLOR_FORMAT  colorFormat;                 //!< Color format to set
    NV_DP_DYNAMIC_RANGE dynamicRange;                //!< Dynamic range
    NV_DP_COLORIMETRY   colorimetry;                 //!< Ignored in RGB space
    NV_DP_BPC           bpc;                         //!< Bit-per-component
    NvU32               isHPD               : 1;     //!< If the control panel is making this call due to HPD
    NvU32               isSetDeferred       : 1;     //!< Requires an OS modeset to finalize the setup if set
    NvU32               isChromaLpfOff      : 1;     //!< Force the chroma low_pass_filter to be off
    NvU32               isDitherOff         : 1;     //!< Force to turn off dither
    NvU32               testLinkTrain       : 1;     //!< If testing mode, skip validation
    NvU32               testColorChange     : 1;     //!< If testing mode, skip validation

} NV_DISPLAY_PORT_CONFIG;

//! \addtogroup dispcontrol
//! @{
//! Macro for constructing the version field of NV_DISPLAY_PORT_CONFIG
#define NV_DISPLAY_PORT_CONFIG_VER   MAKE_NVAPI_VERSION(NV_DISPLAY_PORT_CONFIG,2)
//! Macro for constructing the version field of NV_DISPLAY_PORT_CONFIG
#define NV_DISPLAY_PORT_CONFIG_VER_1 MAKE_NVAPI_VERSION(NV_DISPLAY_PORT_CONFIG,1)
//! Macro for constructing the version field of NV_DISPLAY_PORT_CONFIG
#define NV_DISPLAY_PORT_CONFIG_VER_2 MAKE_NVAPI_VERSION(NV_DISPLAY_PORT_CONFIG,2)
//! @}


//! \ingroup          dispcontrol
NVAPI_INTERFACE NvAPI_SetDisplayPort(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_DISPLAY_PORT_CONFIG *pCfg);




//! \ingroup dispcontrol
//! Used in NvAPI_GetHDMISupportInfo().
typedef struct _NV_HDMI_SUPPORT_INFO_V1
{
    NvU32      version;                     //!< Structure version

    NvU32      isGpuHDMICapable       : 1;  //!< If the GPU can handle HDMI
    NvU32      isMonUnderscanCapable  : 1;  //!< If the monitor supports underscan
    NvU32      isMonBasicAudioCapable : 1;  //!< If the monitor supports basic audio
    NvU32      isMonYCbCr444Capable   : 1;  //!< If YCbCr 4:4:4 is supported
    NvU32      isMonYCbCr422Capable   : 1;  //!< If YCbCr 4:2:2 is supported
    NvU32      isMonxvYCC601Capable   : 1;  //!< If xvYCC 601 is supported
    NvU32      isMonxvYCC709Capable   : 1;  //!< If xvYCC 709 is supported
    NvU32      isMonHDMI              : 1;  //!< If the monitor is HDMI (with IEEE's HDMI registry ID)
    NvU32      reserved               : 24; //!< Reserved. 

    NvU32      EDID861ExtRev;               //!< Revision number of the EDID 861 extension
 } NV_HDMI_SUPPORT_INFO_V1; 

typedef struct _NV_HDMI_SUPPORT_INFO_V2
{
    NvU32      version;                      //!< Structure version

    NvU32      isGpuHDMICapable        : 1;  //!< If the GPU can handle HDMI
    NvU32      isMonUnderscanCapable   : 1;  //!< If the monitor supports underscan
    NvU32      isMonBasicAudioCapable  : 1;  //!< If the monitor supports basic audio
    NvU32      isMonYCbCr444Capable    : 1;  //!< If YCbCr 4:4:4 is supported
    NvU32      isMonYCbCr422Capable    : 1;  //!< If YCbCr 4:2:2 is supported
    NvU32      isMonxvYCC601Capable    : 1;  //!< If xvYCC extended colorimetry 601 is supported
    NvU32      isMonxvYCC709Capable    : 1;  //!< If xvYCC extended colorimetry 709 is supported
    NvU32      isMonHDMI               : 1;  //!< If the monitor is HDMI (with IEEE's HDMI registry ID)
    NvU32      isMonsYCC601Capable     : 1;  //!< if sYCC601 extended colorimetry is supported
    NvU32      isMonAdobeYCC601Capable : 1;  //!< if AdobeYCC601 extended colorimetry is supported
    NvU32      isMonAdobeRGBCapable    : 1;  //!< if AdobeRGB extended colorimetry is supported
    NvU32      reserved                : 21; //!< Reserved. 

    NvU32      EDID861ExtRev;                //!< Revision number of the EDID 861 extension
 } NV_HDMI_SUPPORT_INFO_V2; 

#define NV_HDMI_SUPPORT_INFO_VER1  MAKE_NVAPI_VERSION(NV_HDMI_SUPPORT_INFO_V1, 1)
#define NV_HDMI_SUPPORT_INFO_VER2  MAKE_NVAPI_VERSION(NV_HDMI_SUPPORT_INFO_V2, 2)



#ifndef NV_HDMI_SUPPORT_INFO_VER

typedef NV_HDMI_SUPPORT_INFO_V2    NV_HDMI_SUPPORT_INFO;
#define NV_HDMI_SUPPORT_INFO_VER   NV_HDMI_SUPPORT_INFO_VER2

#endif


//! SUPPORTED OS:  Windows Vista and higher
//!
///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetHDMISupportInfo
//
//! \fn NvAPI_GetHDMISupportInfo(_In_opt_ NvDisplayHandle hNvDisplay, _In_ NvU32 outputId, _Inout_ NV_HDMI_SUPPORT_INFO *pInfo)
//!   This API returns the current infoframe data on the specified device(monitor).
//!
//! \since Release: 95
//!
//! \param [in]  hvDisplay  NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//!                         This parameter is ignored when the outputId is a NvAPI displayId.
//! \param [in]  outputId   This can either be the connection bit mask or the NvAPI displayId. When the legacy connection bit mask is passed, 
//!                         it should have exactly 1 bit set to indicate a single display. If it's "0" then the default outputId from 
//!                         NvAPI_GetAssociatedDisplayOutputId() will be used. See \ref handles.
//! \param [out] pInfo      The monitor and GPU's HDMI support info
//!
//! \retval  NVAPI_OK                Completed request
//! \retval  NVAPI_ERROR             Miscellaneous error occurred
//! \retval  NVAPI_INVALID_ARGUMENT  Invalid input parameter.
///////////////////////////////////////////////////////////////////////////////


//! \ingroup dispcontrol
NVAPI_INTERFACE NvAPI_GetHDMISupportInfo(_In_opt_ NvDisplayHandle hNvDisplay, _In_ NvU32 outputId, _Inout_ NV_HDMI_SUPPORT_INFO *pInfo);


//! \ingroup dispcontrol

typedef enum
{
    NV_INFOFRAME_CMD_GET_DEFAULT = 0,     //!< Returns the fields in the infoframe with values set by the manufacturer - NVIDIA/OEM.
    NV_INFOFRAME_CMD_RESET,               //!< Sets the fields in the infoframe to auto, and infoframe to the default infoframe for use in a set.    
    NV_INFOFRAME_CMD_GET,                 //!< Get the current infoframe state.
    NV_INFOFRAME_CMD_SET,                 //!< Set the current infoframe state (flushed to the monitor), the values are one time and do not persist.
    NV_INFOFRAME_CMD_GET_OVERRIDE,        //!< Get the override infoframe state, non-override fields will be set to value = AUTO, overridden fields will have the current override values.
    NV_INFOFRAME_CMD_SET_OVERRIDE,        //!< Set the override infoframe state, non-override fields will be set to value = AUTO, other values indicate override; persist across modeset/reboot
    NV_INFOFRAME_CMD_GET_PROPERTY,        //!< get properties associated with infoframe (each of the infoframe type will have properties)
    NV_INFOFRAME_CMD_SET_PROPERTY,        //!< set properties associated with infoframe
} NV_INFOFRAME_CMD;


typedef enum
{
    NV_INFOFRAME_PROPERTY_MODE_AUTO           = 0, //!< Driver determines whether to send infoframes.
    NV_INFOFRAME_PROPERTY_MODE_ENABLE,             //!< Driver always sends infoframe.
    NV_INFOFRAME_PROPERTY_MODE_DISABLE,            //!< Driver never sends infoframe.
    NV_INFOFRAME_PROPERTY_MODE_ALLOW_OVERRIDE,     //!< Driver only sends infoframe when client requests it via infoframe escape call.
} NV_INFOFRAME_PROPERTY_MODE;


//! Returns whether the current monitor is in blacklist or force this monitor to be in blacklist.
typedef enum
{
    NV_INFOFRAME_PROPERTY_BLACKLIST_FALSE = 0,
    NV_INFOFRAME_PROPERTY_BLACKLIST_TRUE,
} NV_INFOFRAME_PROPERTY_BLACKLIST;

typedef struct
{
    NvU32 mode      :  4;
    NvU32 blackList :  2;
    NvU32 reserved  : 10;
    NvU32 version   :  8;
    NvU32 length    :  8;
} NV_INFOFRAME_PROPERTY;

//! Byte1 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_NODATA    = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_OVERSCAN,
    NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_UNDERSCAN,
    NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_FUTURE,
    NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO_AUTO      = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_SCANINFO;


typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_NOT_PRESENT         = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_VERTICAL_PRESENT,
    NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_HORIZONTAL_PRESENT,
    NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_BOTH_PRESENT,
    NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA_AUTO                = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_BARDATA;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_AFI_ABSENT   = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_AFI_PRESENT,
    NV_INFOFRAME_FIELD_VALUE_AVI_AFI_AUTO     = 3
} NV_INFOFRAME_FIELD_VALUE_AVI_ACTIVEFORMATINFO;


typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_RGB      = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_YCbCr422,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_YCbCr444,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_FUTURE,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT_AUTO     = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_COLORFORMAT;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_F17_FALSE = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_F17_TRUE,
    NV_INFOFRAME_FIELD_VALUE_AVI_F17_AUTO = 3
} NV_INFOFRAME_FIELD_VALUE_AVI_F17;

//! Byte2 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_NO_AFD           = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE01,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE02,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE03,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_LETTERBOX_GT16x9,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE05,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE06,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE07,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_EQUAL_CODEDFRAME = 8,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_4x3,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_16x9,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_CENTER_14x9,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_RESERVE12,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_4x3_ON_14x9,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_16x9_ON_14x9,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_16x9_ON_4x3,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION_AUTO             = 31,
} NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOACTIVEPORTION;


typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_NO_DATA = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_4x3,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_16x9,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_FUTURE,
    NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME_AUTO    = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_ASPECTRATIOCODEDFRAME;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_NO_DATA                   = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_SMPTE_170M,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_ITUR_BT709,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_USE_EXTENDED_COLORIMETRY,
    NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY_AUTO                      = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_COLORIMETRY;

//! Byte 3 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_NO_DATA    = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_HORIZONTAL,
    NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_VERTICAL,
    NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_BOTH,
    NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING_AUTO       = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_NONUNIFORMPICTURESCALING;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_DEFAULT       = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_LIMITED_RANGE,
    NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_FULL_RANGE,
    NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_RESERVED,
    NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION_AUTO          = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_RGBQUANTIZATION;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_XVYCC601     = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_XVYCC709,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_SYCC601,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_ADOBEYCC601,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_ADOBERGB,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED05,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED06,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_RESERVED07,
    NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY_AUTO         = 15
} NV_INFOFRAME_FIELD_VALUE_AVI_EXTENDEDCOLORIMETRY;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_ITC_VIDEO_CONTENT = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_ITC_ITCONTENT,
    NV_INFOFRAME_FIELD_VALUE_AVI_ITC_AUTO          = 3
} NV_INFOFRAME_FIELD_VALUE_AVI_ITC;

//! Byte 4 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_NONE = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X02,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X03,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X04,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X05,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X06,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X07,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X08,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X09,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_X10,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED10,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED11,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED12,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED13,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED14,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_RESERVED15,
    NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION_AUTO         = 31
} NV_INFOFRAME_FIELD_VALUE_AVI_PIXELREPETITION;


typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_GRAPHICS = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_PHOTO,
    NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_CINEMA,
    NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_GAME,
    NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE_AUTO     = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_CONTENTTYPE;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_LIMITED_RANGE = 0,
    NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_FULL_RANGE,
    NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_RESERVED02,
    NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_RESERVED03,
    NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION_AUTO          = 7
} NV_INFOFRAME_FIELD_VALUE_AVI_YCCQUANTIZATION;

//! Adding an Auto bit to each field
typedef struct
{
    NvU32 vic                     : 8;
    NvU32 pixelRepeat             : 5;
    NvU32 colorSpace              : 3;
    NvU32 colorimetry             : 3;
    NvU32 extendedColorimetry     : 4;
    NvU32 rgbQuantizationRange    : 3;
    NvU32 yccQuantizationRange    : 3;
    NvU32 itContent               : 2;
    NvU32 contentTypes            : 3;
    NvU32 scanInfo                : 3;
    NvU32 activeFormatInfoPresent : 2;
    NvU32 activeFormatAspectRatio : 5;
    NvU32 picAspectRatio          : 3;
    NvU32 nonuniformScaling       : 3;
    NvU32 barInfo                 : 3;    
    NvU32 top_bar                 : 17;
    NvU32 bottom_bar              : 17;
    NvU32 left_bar                : 17;
    NvU32 right_bar               : 17;
    NvU32 Future17                : 2;
    NvU32 Future47                : 2;
} NV_INFOFRAME_VIDEO;

//! Byte 1 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_IN_HEADER = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_2,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_3,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_4,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_5,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_6,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_7,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_8,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT_AUTO      = 15
} NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELCOUNT;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_IN_HEADER                  = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_PCM,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AC3,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MPEG1,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MP3,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MPEG2,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AACLC,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DTS,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_ATRAC,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DSD,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_EAC3,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DTSHD,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_MLP,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_DST,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_WMAPRO,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_USE_CODING_EXTENSION_TYPE,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE_AUTO                      = 31
} NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGTYPE;

//! Byte 2 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_IN_HEADER = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_16BITS,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_20BITS,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_24BITS,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE_AUTO      = 7
} NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLESIZE;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_IN_HEADER = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_32000HZ,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_44100HZ,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_48000HZ,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_88200KHZ,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_96000KHZ,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_176400KHZ,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_192000KHZ,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY_AUTO      = 15
} NV_INFOFRAME_FIELD_VALUE_AUDIO_SAMPLEFREQUENCY;



//! Byte 3 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_USE_CODING_TYPE = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_HEAAC,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_HEAACV2,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_MPEGSURROUND,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE04,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE05,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE06,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE07,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE08,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE09,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE10,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE11,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE12,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE13,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE14,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE15,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE16,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE17,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE18,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE19,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE20,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE21,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE22,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE23,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE24,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE25,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE26,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE27,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE28,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE29,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE30,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_RESERVE31,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE_AUTO           = 63
} NV_INFOFRAME_FIELD_VALUE_AUDIO_CODINGEXTENSIONTYPE;


//! Byte 4 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_X_X_FR_FL           =0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_X_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_X_RC_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_X_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_RC_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_RRC_RLC_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_X_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_X_RC_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRC_FLC_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_FCH_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_X_FCH_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_X_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_X_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_X_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_X_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_RC_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_RC_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FCH_RC_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FCH_RC_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_FCH_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_TC_FCH_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRH_FLH_RR_RL_FC_LFE_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_FC_X_FR_FL,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_FRW_FLW_RR_RL_FC_LFE_FR_FL  = 0X31,
    // all other values should default to auto
    NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION_AUTO                        = 0x1FF
} NV_INFOFRAME_FIELD_VALUE_AUDIO_CHANNELALLOCATION;

//! Byte 5 related
typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_NO_DATA    = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_0DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_PLUS10DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_RESERVED03,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL_AUTO       = 7
} NV_INFOFRAME_FIELD_VALUE_AUDIO_LFEPLAYBACKLEVEL;

typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_0DB  = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_1DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_2DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_3DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_4DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_5DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_6DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_7DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_8DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_9DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_10DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_11DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_12DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_13DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_14DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_15DB,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES_AUTO = 31
} NV_INFOFRAME_FIELD_VALUE_AUDIO_LEVELSHIFTVALUES;


typedef enum
{
    NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_PERMITTED  = 0,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_PROHIBITED,
    NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX_AUTO       = 3
} NV_INFOFRAME_FIELD_VALUE_AUDIO_DOWNMIX;

typedef struct
{
    NvU32 codingType          : 5;
    NvU32 codingExtensionType : 6;
    NvU32 sampleSize          : 3;
    NvU32 sampleRate          : 4;
    NvU32 channelCount        : 4;
    NvU32 speakerPlacement    : 9;
    NvU32 downmixInhibit      : 2;
    NvU32 lfePlaybackLevel    : 3;
    NvU32 levelShift          : 5; 
    NvU32 Future12            : 2;
    NvU32 Future2x            : 4;
    NvU32 Future3x            : 4;
    NvU32 Future52            : 2;
    NvU32 Future6             : 9;
    NvU32 Future7             : 9;
    NvU32 Future8             : 9;
    NvU32 Future9             : 9;
    NvU32 Future10            : 9;
} NV_INFOFRAME_AUDIO;

typedef struct
{
    NvU32 version; //!< version of this structure
    NvU16 size;    //!< size of this structure
    NvU8  cmd;     //!< The actions to perform from NV_INFOFRAME_CMD
    NvU8  type;    //!< type of infoframe
    
    union
    {
        NV_INFOFRAME_PROPERTY     property;  //!< This is NVIDIA-specific and corresponds to the property cmds and associated infoframe.
        NV_INFOFRAME_AUDIO        audio;
        NV_INFOFRAME_VIDEO        video;
    } infoframe;
} NV_INFOFRAME_DATA;

//! Macro for constructing the version field of ::NV_INFOFRAME_DATA
#define NV_INFOFRAME_DATA_VER   MAKE_NVAPI_VERSION(NV_INFOFRAME_DATA,1)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_Disp_InfoFrameControl
//
//! DESCRIPTION:     This API controls the InfoFrame values.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \param [in]     displayId         Monitor Identifier
//! \param [in,out] pInfoframeData    Contains data corresponding to InfoFrame
//!                  
//! \return    This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!            specific meaning for this API, they are listed below.
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Disp_InfoFrameControl(_In_ NvU32 displayId, _Inout_ NV_INFOFRAME_DATA *pInfoframeData);






//! \ingroup dispcontrol
//! @{
///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_Disp_ColorControl
//
//! \fn NvAPI_Disp_ColorControl(NvU32 displayId, NV_COLOR_DATA *pColorData)
//! DESCRIPTION:    This API controls the Color values.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \param [in]     displayId         Monitor Identifier
//! \param [in,out] pColorData        Contains data corresponding to color information
//!                  
//! \return  RETURN STATUS:
//! ::NVAPI_OK,  
//! ::NVAPI_ERROR, 
//! ::NVAPI_INVALID_ARGUMENT
//
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
    NV_COLOR_CMD_GET                 = 1,
    NV_COLOR_CMD_SET,
    NV_COLOR_CMD_IS_SUPPORTED_COLOR,
    NV_COLOR_CMD_GET_DEFAULT
} NV_COLOR_CMD;

//!  See Table 14 of CEA-861E.  Not all of this is supported by the GPU.
typedef enum
{
    NV_COLOR_FORMAT_RGB             = 0,
    NV_COLOR_FORMAT_YUV422,
    NV_COLOR_FORMAT_YUV444,
    NV_COLOR_FORMAT_DEFAULT         = 0xFE,
    NV_COLOR_FORMAT_AUTO            = 0xFF
} NV_COLOR_FORMAT;



typedef enum
{
    NV_COLOR_COLORIMETRY_RGB             = 0,
    NV_COLOR_COLORIMETRY_YCC601,
    NV_COLOR_COLORIMETRY_YCC709,
    NV_COLOR_COLORIMETRY_XVYCC601,
    NV_COLOR_COLORIMETRY_XVYCC709,
    NV_COLOR_COLORIMETRY_SYCC601,
    NV_COLOR_COLORIMETRY_ADOBEYCC601,
    NV_COLOR_COLORIMETRY_ADOBERGB,
    NV_COLOR_COLORIMETRY_DEFAULT         = 0xFE,
    NV_COLOR_COLORIMETRY_AUTO            = 0xFF
} NV_COLOR_COLORIMETRY;

typedef struct
{
    NvU32 version; //!< Version of this structure
    NvU16 size;    //!< Size of this structure
    NvU8  cmd;
    struct
    {
        NvU8  colorFormat;
        NvU8  colorimetry;
    } data;
} NV_COLOR_DATA;

NVAPI_INTERFACE NvAPI_Disp_ColorControl(NvU32 displayId, NV_COLOR_DATA *pColorData);

//! Macro for constructing the version field of ::NV_COLOR_DATA
#define NV_COLOR_DATA_VER   MAKE_NVAPI_VERSION(NV_COLOR_DATA,1)

//! @}

//! \ingroup dispcontrol
//! Used in NvAPI_DISP_GetTiming().
typedef struct
{
    NvU32                   isInterlaced   : 4;  //!< To retrieve interlaced/progressive timing
    NvU32                   reserved0      : 12;
    union
    {
        NvU32               tvFormat       : 8;  //!< The actual analog HD/SDTV format. Used when the timing type is 
                                                 //!  NV_TIMING_OVERRIDE_ANALOG_TV and width==height==rr==0.
        NvU32               ceaId          : 8;  //!< The EIA/CEA 861B/D predefined short timing descriptor ID. 
                                                 //!  Used when the timing type is NV_TIMING_OVERRIDE_EIA861
                                                 //!  and width==height==rr==0.
        NvU32               nvPsfId        : 8;  //!< The NV predefined PsF format Id. 
                                                 //!  Used when the timing type is NV_TIMING_OVERRIDE_NV_PREDEFINED.
    };
    NvU32                   scaling        : 8;  //!< Define preferred scaling
}NV_TIMING_FLAG;

//! \ingroup dispcontrol
//! Used in NvAPI_DISP_GetTiming().
typedef struct _NV_TIMING_INPUT
{
    NvU32 version;                       //!< (IN)     structure version
    
    NvU32 width;						//!< Visible horizontal size
    NvU32 height;						//!< Visible vertical size 
    float rr;							//!< Timing refresh rate
        
    NV_TIMING_FLAG flag;				//!< Flag containing additional info for timing calculation.
    
    NV_TIMING_OVERRIDE type;			//!< Timing type(formula) to use for calculating the timing
}NV_TIMING_INPUT;

#define NV_TIMING_INPUT_VER   MAKE_NVAPI_VERSION(NV_TIMING_INPUT,1)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_GetTiming
//
//! DESCRIPTION:  This function calculates the timing from the visible width/height/refresh-rate and timing type info.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 313  
//!
//!
//! \param [in]   displayId		Display ID of the display.
//! \param [in]   timingInput   Inputs used for calculating the timing.
//! \param [out]  pTiming       Pointer to the NV_TIMING structure. 
//!
//! \return        This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!                specific meaning for this API, they are listed below.
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_GetTiming( _In_ NvU32 displayId,_In_ NV_TIMING_INPUT *timingInput, _Out_ NV_TIMING *pTiming); 



///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_GetMonitorCapabilities
//
//! \fn NvAPI_DISP_GetMonitorCapabilities(NvU32 displayId, NV_MONITOR_CAPABILITIES *pMonitorCapabilities)
//! DESCRIPTION:     This API returns the Monitor capabilities 
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \param [in]      displayId                Monitor Identifier
//! \param [out]     pMonitorCapabilities     The monitor support info
//!
//! \return ::NVAPI_OK, 
//!         ::NVAPI_ERROR, 
//!         ::NVAPI_INVALID_ARGUMENT
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup dispcontrol
//! @{


//! HDMI-related and extended CAPs
typedef enum
{
    // hdmi related caps
    NV_MONITOR_CAPS_TYPE_HDMI_VSDB = 0x1000,
    NV_MONITOR_CAPS_TYPE_HDMI_VCDB = 0x1001,
} NV_MONITOR_CAPS_TYPE;



typedef struct _NV_MONITOR_CAPS_VCDB
{
    NvU8    quantizationRangeYcc         : 1;
    NvU8    quantizationRangeRgb         : 1;
    NvU8    scanInfoPreferredVideoFormat : 2;
    NvU8    scanInfoITVideoFormats       : 2;
    NvU8    scanInfoCEVideoFormats       : 2;
} NV_MONITOR_CAPS_VCDB;


//! See NvAPI_DISP_GetMonitorCapabilities().
typedef struct _NV_MONITOR_CAPS_VSDB
{
    // byte 1
    NvU8    sourcePhysicalAddressB         : 4; //!< Byte 1
    NvU8    sourcePhysicalAddressA         : 4; //!< Byte 1
    // byte 2
    NvU8    sourcePhysicalAddressD         : 4; //!< Byte 2
    NvU8    sourcePhysicalAddressC         : 4; //!< Byte 2
    // byte 3
    NvU8    supportDualDviOperation        : 1; //!< Byte 3
    NvU8    reserved6                      : 2; //!< Byte 3
    NvU8    supportDeepColorYCbCr444       : 1; //!< Byte 3
    NvU8    supportDeepColor30bits         : 1; //!< Byte 3
    NvU8    supportDeepColor36bits         : 1; //!< Byte 3
    NvU8    supportDeepColor48bits         : 1; //!< Byte 3
    NvU8    supportAI                      : 1; //!< Byte 3 
    // byte 4
    NvU8    maxTmdsClock;  //!< Bye 4
    // byte 5
    NvU8    cnc0SupportGraphicsTextContent : 1; //!< Byte 5
    NvU8    cnc1SupportPhotoContent        : 1; //!< Byte 5
    NvU8    cnc2SupportCinemaContent       : 1; //!< Byte 5
    NvU8    cnc3SupportGameContent         : 1; //!< Byte 5
    NvU8    reserved8                      : 1; //!< Byte 5
    NvU8    hasVicEntries                  : 1; //!< Byte 5
    NvU8    hasInterlacedLatencyField      : 1; //!< Byte 5
    NvU8    hasLatencyField                : 1; //!< Byte 5    
    // byte 6
    NvU8    videoLatency; //!< Byte 6
    // byte 7
    NvU8    audioLatency; //!< Byte 7
    // byte 8
    NvU8    interlacedVideoLatency; //!< Byte 8
    // byte 9
    NvU8    interlacedAudioLatency; //!< Byte 9
    // byte 10
    NvU8    reserved13                     : 7; //!< Byte 10
    NvU8    has3dEntries                   : 1; //!< Byte 10   
    // byte 11
    NvU8    hdmi3dLength                   : 5; //!< Byte 11
    NvU8    hdmiVicLength                  : 3; //!< Byte 11
    // Remaining bytes
    NvU8    hdmi_vic[7];  //!< Keeping maximum length for 3 bits
    NvU8    hdmi_3d[31];  //!< Keeping maximum length for 5 bits 
} NV_MONITOR_CAPS_VSDB;


//! See NvAPI_DISP_GetMonitorCapabilities().
typedef struct _NV_MONITOR_CAPABILITIES
{
    NvU32    version;
    NvU16    size;
    NvU32    infoType;
    NvU32    connectorType;        //!< Out: VGA, TV, DVI, HDMI, DP
    NvU8     bIsValidInfo : 1;     //!< Boolean : Returns invalid if requested info is not present such as VCDB not present
    union {
        NV_MONITOR_CAPS_VSDB  vsdb;
        NV_MONITOR_CAPS_VCDB  vcdb;
    } data;
} NV_MONITOR_CAPABILITIES;

//! Macro for constructing the version field of ::NV_MONITOR_CAPABILITIES
#define NV_MONITOR_CAPABILITIES_VER   MAKE_NVAPI_VERSION(NV_MONITOR_CAPABILITIES,1)

//! @}

//! SUPPORTED OS:  Windows Vista and higher
//!
//! \ingroup dispcontrol
NVAPI_INTERFACE NvAPI_DISP_GetMonitorCapabilities(_In_ NvU32 displayId, _Inout_ NV_MONITOR_CAPABILITIES *pMonitorCapabilities);

//! \ingroup dispcontrol
typedef struct _NV_MONITOR_COLOR_DATA
{
    NvU32                   version;            
// We are only supporting DP monitors for now. We need to extend this to HDMI panels as well
    NV_DP_COLOR_FORMAT      colorFormat;        //!< One of the supported color formats
    NV_DP_BPC               backendBitDepths;   //!< One of the supported bit depths
} NV_MONITOR_COLOR_CAPS_V1;

typedef NV_MONITOR_COLOR_CAPS_V1 NV_MONITOR_COLOR_CAPS;

//! \ingroup dispcontrol
#define NV_MONITOR_COLOR_CAPS_VER1   MAKE_NVAPI_VERSION(NV_MONITOR_COLOR_CAPS_V1,1)
#define NV_MONITOR_COLOR_CAPS_VER    NV_MONITOR_COLOR_CAPS_VER1

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_GetMonitorColorCapabilities
//
//! DESCRIPTION:    This API returns all the color formats and bit depth values supported by a given DP monitor. 
//!
//! USAGE:         Sequence of calls which caller should make to get the information.
//!                1. First call NvAPI_DISP_GetMonitorColorCapabilities() with pMonitorColorCapabilities as NULL to get the count.
//!                2. Allocate memory for color caps(NV_MONITOR_COLOR_CAPS) array.
//!                3. Call NvAPI_DISP_GetMonitorColorCapabilities() again with the pointer to the memory allocated to get all the 
//!                   color capabilities.
//!                   
//!                Note : 
//!                1. pColorCapsCount should never be NULL, else the API will fail with NVAPI_INVALID_ARGUMENT.
//!                2. *pColorCapsCount returned from the API will always be the actual count in any/every call.
//!                3. Memory size to be allocated should be (*pColorCapsCount * sizeof(NV_MONITOR_COLOR_CAPS)).
//!                4. If the memory allocated is less than what is required to return all the timings, this API will return the
//!                   amount of information which can fit in user provided buffer and API will return NVAPI_INSUFFICIENT_BUFFER.
//!                5. If the caller specifies a greater value for *pColorCapsCount in second call to NvAPI_DISP_GetMonitorColorCapabilities()
//!                   than what was returned from first call, the API will return only the actual number of elements in the color
//!                   capabilities array and the extra buffer will remain unused.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \param [in]      displayId                  Monitor Identifier
//! \param [in, out] pMonitorColorCapabilities  The monitor color capabilities information
//! \param [in, out] pColorCapsCount            - During input, the number of elements allocated for the pMonitorColorCapabilities pointer
//!                                             - During output, the actual number of color data elements the monitor supports
//!
//! \return    This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!            specific meaning for this API, they are listed below.
//!
//! \retval         NVAPI_INSUFFICIENT_BUFFER   The input buffer size is not sufficient to hold the total contents. In this case
//!                                             *pColorCapsCount will hold the required amount of elements.
//! \retval         NVAPI_INVALID_DISPLAY_ID    The input monitor is either not connected or is not a DP panel.
//!
//! \ingroup dispcontrol
//!
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_GetMonitorColorCapabilities(_In_ NvU32 displayId, __inout_ecount_part_opt(*pColorCapsCount, *pColorCapsCount) NV_MONITOR_COLOR_CAPS *pMonitorColorCapabilities, _Inout_ NvU32 *pColorCapsCount);

//! \ingroup dispcontrol
//! Used in NvAPI_DISP_EnumCustomDisplay() and NvAPI_DISP_TryCustomDisplay().
typedef struct
{
    NvU32                   version;
    
    // the source mode information
    NvU32                   width;             //!< Source surface(source mode) width
    NvU32                   height;            //!< Source surface(source mode) height
    NvU32                   depth;             //!< Source surface color depth."0" means all 8/16/32bpp
    NV_FORMAT               colorFormat;       //!< Color format (optional)
  
    NV_VIEWPORTF            srcPartition;      //!< For multimon support, should be set to (0,0,1.0,1.0) for now.
  
    float                   xRatio;            //!< Horizontal scaling ratio
    float                   yRatio;            //!< Vertical scaling ratio
                                                             
    NV_TIMING               timing;            //!< Timing used to program TMDS/DAC/LVDS/HDMI/TVEncoder, etc.
    NvU32                   hwModeSetOnly : 1; //!< If set, it means a hardware modeset without OS update
    
}NV_CUSTOM_DISPLAY; 

//! \ingroup dispcontrol
//! Used in NV_CUSTOM_DISPLAY.
#define NV_CUSTOM_DISPLAY_VER  MAKE_NVAPI_VERSION(NV_CUSTOM_DISPLAY,1)

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_EnumCustomDisplay
//
//! DESCRIPTION:   This API enumerates the custom timing specified by the enum index. 
//!				   The client should keep enumerating until it returns NVAPI_END_ENUMERATION.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 313 
//!
//! \param [in]     displayId   Dispaly ID of the display.
//! \param [in]     index       Enum index
//! \param [inout]  pCustDisp   Pointer to the NV_CUSTOM_DISPLAY structure 
//!
//! \return        This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!                specific meaning for this API, they are listed below.
//! \retval        NVAPI_INVALID_DISPLAY_ID:   Custom Timing is not supported on the Display, whose display id is passed
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_EnumCustomDisplay( _In_ NvU32 displayId, _In_ NvU32 index, _Inout_ NV_CUSTOM_DISPLAY *pCustDisp);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_TryCustomDisplay
//
//! DESCRIPTION:    This API is used to set up a custom display without saving the configuration on multiple displays.
//!
//! \note
//!  All the members of srcPartition, present in NV_CUSTOM_DISPLAY structure, should have their range in (0.0,1.0).
//!  In clone mode the timings can applied to both the target monitors but only one target at a time. \n
//!  For the secondary target the applied timings works under the following conditions:
//!  - If the secondary monitor EDID supports the selected timing, OR
//!  - If the selected custom timings can be scaled by the secondary monitor for the selected source resolution on the primary, OR
//!  - If the selected custom timings matches the existing source resolution on the primary.
//!  Setting up a custom display on non-active but connected monitors is supported only for Win7 and above.
//!
//! SUPPORTED OS:  Windows XP,  Windows 7 and higher
//!
//!
//! \since Release: 313   
//!
//!                               
//! \param [in]    pDisplayIds    Array of the target display Dispaly IDs - See \ref handles.
//! \param [in]    count          Total number of the incoming Display IDs and corresponding NV_CUSTOM_DISPLAY structure. This is for the multi-head support.
//! \param [in]    pCustDisp      Pointer to the NV_CUSTOM_DISPLAY structure array.
//!
//! \return        This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!                specific meaning for this API, they are listed below.
//! \retval        NVAPI_INVALID_DISPLAY_ID:   Custom Timing is not supported on the Display, whose display id is passed
//! 
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_TryCustomDisplay( __in_ecount(count) NvU32 *pDisplayIds, _In_ NvU32 count, __in_ecount(count) NV_CUSTOM_DISPLAY *pCustDisp);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_DeleteCustomDisplay
//
//! DESCRIPTION:    This function deletes the custom display configuration, specified from the registry for  all the displays whose display IDs are passed.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 313 
//!
//!
//! \param [in]     pDisplayIds    Array of Dispaly IDs on which custom display configuration is to be saved.
//! \param [in]     count          Total number of the incoming Dispaly IDs. This is for the multi-head support.
//!	\param [in]     pCustDisp	   Pointer to the NV_CUSTOM_DISPLAY structure
//!
//! \return        This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!                specific meaning for this API, they are listed below.
//! \retval        NVAPI_INVALID_DISPLAY_ID:   Custom Timing is not supported on the Display, whose display id is passed
//!
//! \ingroup dispcontrol 
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_DeleteCustomDisplay( __in_ecount(count) NvU32 *pDisplayIds, _In_ NvU32 count, _In_ NV_CUSTOM_DISPLAY *pCustDisp);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_SaveCustomDisplay
//
//! DESCRIPTION:    This function saves the current hardware display configuration on the specified Display IDs as a custom display configuration.
//!                 This function should be called right after NvAPI_DISP_TryCustomDisplay() to save the custom display from the current
//!                 hardware context. This function will not do anything if the custom display configuration is not tested on the hardware.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 313 
//!
//!
//! \param [in]     pDisplayIds         Array of Dispaly IDs on which custom display configuration is to be saved.
//! \param [in]     count               Total number of the incoming Dispaly IDs. This is for the multi-head support.
//! \param [in]     isThisOutputIdOnly  If set, the saved custom display will only be applied on the monitor with the same outputId (see \ref handles).
//! \param [in]     isThisMonitorIdOnly If set, the saved custom display will only be applied on the monitor with the same EDID ID or 
//!                                     the same TV connector in case of analog TV.
//!
//! \return        This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!                specific meaning for this API, they are listed below.
//! \retval 	   NVAPI_INVALID_DISPLAY_ID:   Custom Timing is not supported on the Display, whose display id is passed
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_SaveCustomDisplay( __in_ecount(count) NvU32 *pDisplayIds, _In_ NvU32 count, _In_ NvU32 isThisOutputIdOnly, _In_ NvU32 isThisMonitorIdOnly);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_RevertCustomDisplayTrial
//
//! DESCRIPTION:    This API is used to restore the display configuration, that was changed by calling NvAPI_DISP_TryCustomDisplay(). This function
//!                 must be called only after a custom display configuration is tested on the hardware, using NvAPI_DISP_TryCustomDisplay(),  
//!                 otherwise no action is taken. On Vista, NvAPI_DISP_RevertCustomDisplayTrial should be called with an active display that  
//!                 was affected during the NvAPI_DISP_TryCustomDisplay() call, per GPU. 
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 313  
//!
//!
//! \param [in]    pDisplayIds   Pointer to display Id, of an active display. 
//! \param [in]    count         Total number of incoming Display IDs. For future use only. Currently it is expected to be passed as 1.
//!
//! \return        This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!                specific meaning for this API, they are listed below.
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_RevertCustomDisplayTrial( __in_ecount(count) NvU32* pDisplayIds, _In_ NvU32 count);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetView
//
//! This API lets caller retrieve the target display arrangement for selected source display handle.
//! \note Display PATH with this API is limited to single GPU. DUALVIEW across GPUs will be returned as STANDARD VIEW. 
//!       Use NvAPI_SYS_GetDisplayTopologies() to query views across GPUs.
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_GetDisplayConfig.
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 85
//!
//!  \param [in]     hNvDisplay             NVIDIA Display selection. It can be #NVAPI_DEFAULT_HANDLE or a handle enumerated from 
//!                                         NvAPI_EnumNVidiaDisplayHandle().
//!  \param [out]    pTargets               User allocated storage to retrieve an array of  NV_VIEW_TARGET_INFO. Can be NULL to retrieve 
//!                                         the targetCount.
//!  \param [in,out] targetMaskCount        Count of target device mask specified in pTargetMask.
//!  \param [out]    targetView             Target view selected from NV_TARGET_VIEW_MODE.
//!
//!  \retval         NVAPI_OK               Completed request
//!  \retval         NVAPI_ERROR            Miscellaneous error occurred
//!  \retval         NVAPI_INVALID_ARGUMENT Invalid input parameter.
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_GetDisplayConfig.")
NVAPI_INTERFACE NvAPI_GetView(NvDisplayHandle hNvDisplay, NV_VIEW_TARGET_INFO *pTargets, NvU32 *pTargetMaskCount, NV_TARGET_VIEW_MODE *pTargetView);







///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetViewEx
//
//! DESCRIPTION:    This API lets caller retrieve the target display arrangement for selected source display handle.
//!                 \note Display PATH with this API is limited to single GPU. DUALVIEW across GPUs will be returned as STANDARD VIEW. 
//!                       Use NvAPI_SYS_GetDisplayTopologies() to query views across GPUs.
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_GetDisplayConfig.
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 165
//!
//! \param [in]     hNvDisplay       NVIDIA Display selection. #NVAPI_DEFAULT_HANDLE is not allowed, it has to be a handle enumerated with
//!                                  NvAPI_EnumNVidiaDisplayHandle().
//! \param [in,out] pPathInfo        Count field should be set to NVAPI_MAX_DISPLAY_PATH. Can be NULL to retrieve just the pathCount.
//! \param [in,out] pPathCount       Number of elements in array pPathInfo->path.
//! \param [out]    pTargetViewMode  Display view selected from NV_TARGET_VIEW_MODE.
//!
//! \retval         NVAPI_OK                      Completed request
//! \retval         NVAPI_API_NOT_INTIALIZED      NVAPI not initialized
//! \retval         NVAPI_ERROR                   Miscellaneous error occurred
//! \retval         NVAPI_INVALID_ARGUMENT        Invalid input parameter.
//! \retval         NVAPI_EXPECTED_DISPLAY_HANDLE hNvDisplay is not a valid display handle.
//! 
//! \ingroup dispcontrol   
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_DISP_GetDisplayConfig.")
NVAPI_INTERFACE NvAPI_GetViewEx(NvDisplayHandle hNvDisplay, NV_DISPLAY_PATH_INFO *pPathInfo, NvU32 *pPathCount, NV_TARGET_VIEW_MODE *pTargetViewMode);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetSupportedViews
//
//!  This API lets caller enumerate all the supported NVIDIA display views - nView and Dualview modes.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 85
//!
//!  \param [in]     hNvDisplay             NVIDIA Display selection. It can be #NVAPI_DEFAULT_HANDLE or a handle enumerated from
//!                                         NvAPI_EnumNVidiaDisplayHandle().
//!  \param [out]    pTargetViews           Array of supported views. Can be NULL to retrieve the pViewCount first.
//!  \param [in,out] pViewCount             Count of supported views.
//!
//!  \retval         NVAPI_OK               Completed request
//!  \retval         NVAPI_ERROR            Miscellaneous error occurred
//!  \retval         NVAPI_INVALID_ARGUMENT Invalid input parameter.
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetSupportedViews(NvDisplayHandle hNvDisplay, NV_TARGET_VIEW_MODE *pTargetViews, NvU32 *pViewCount);


//! SUPPORTED OS:  Windows XP and higher
//!
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_DISP_GetDisplayIdByDisplayName
//
//! DESCRIPTION:     This API retrieves the Display Id of a given display by
//!                  display name. The display must be active to retrieve the
//!                  displayId. In the case of clone mode or Surround gaming,
//!                  the primary or top-left display will be returned.
//!
//! \param [in]     displayName  Name of display (Eg: "\\DISPLAY1" to
//!                              retrieve the displayId for.
//! \param [out]    displayId    Display ID of the requested display.
//!
//! retval ::NVAPI_OK:                          Capabilties have been returned.
//! retval ::NVAPI_INVALID_ARGUMENT:            One or more args passed in are invalid.
//! retval ::NVAPI_API_NOT_INTIALIZED:          The NvAPI API needs to be initialized first
//! retval ::NVAPI_NO_IMPLEMENTATION:           This entrypoint not available
//! retval ::NVAPI_ERROR:                       Miscellaneous error occurred
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_GetDisplayIdByDisplayName(const char *displayName, NvU32* displayId);




///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_GetDisplayConfig
//
//! DESCRIPTION:     This API lets caller retrieve the current global display
//!                  configuration.
//!       USAGE:     The caller might have to call this three times to fetch all the required configuration details as follows:
//!                  First  Pass: Caller should Call NvAPI_DISP_GetDisplayConfig() with pathInfo set to NULL to fetch pathInfoCount.
//!                  Second Pass: Allocate memory for pathInfo with respect to the number of pathInfoCount(from First Pass) to fetch 
//!                               targetInfoCount. If sourceModeInfo is needed allocate memory or it can be initialized to NULL.
//!             Third  Pass(Optional, only required if target information is required): Allocate memory for targetInfo with respect 
//!                               to number of targetInfoCount(from Second Pass).               
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \param [in,out]  pathInfoCount    Number of elements in pathInfo array, returns number of valid topologies, this cannot be null.
//! \param [in,out]  pathInfo         Array of path information
//!
//! \return    This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//!            specific meaning for this API, they are listed below.
//!
//! \retval    NVAPI_INVALID_ARGUMENT  -   Invalid input parameter. Following can be the reason for this return value:
//!                                        -# pathInfoCount is NULL.
//!                                        -# *pathInfoCount is 0 and pathInfo is not NULL.
//!                                        -# *pathInfoCount is not 0 and pathInfo is NULL.
//! \retval    NVAPI_DEVICE_BUSY       -   ModeSet has not yet completed. Please wait and call it again.
//!                                       
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_GetDisplayConfig(_Inout_ NvU32 *pathInfoCount, __out_ecount_full_opt(*pathInfoCount) NV_DISPLAYCONFIG_PATH_INFO *pathInfo);




///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_SetDisplayConfig
//
//
//! DESCRIPTION:     This API lets caller apply a global display configuration
//!                  across multiple GPUs.
//!
//!                  If all sourceIds are zero, then NvAPI will pick up sourceId's based on the following criteria :
//!                  - If user provides sourceModeInfo then we are trying to assign 0th sourceId always to GDIPrimary. 
//!                     This is needed since active windows always moves along with 0th sourceId.
//!                  - For rest of the paths, we are incrementally assigning the sourceId per adapter basis.
//!                  - If user doesn't provide sourceModeInfo then NVAPI just picks up some default sourceId's in incremental order.
//!                  Note : NVAPI will not intelligently choose the sourceIDs for any configs that does not need a modeset.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \param [in]      pathInfoCount   Number of supplied elements in pathInfo
//! \param [in]      pathInfo        Array of path information
//! \param [in]      flags           Flags for applying settings
//! 
//! \retval ::NVAPI_OK - completed request
//! \retval ::NVAPI_API_NOT_INTIALIZED - NVAPI not initialized
//! \retval ::NVAPI_ERROR - miscellaneous error occurred
//! \retval ::NVAPI_INVALID_ARGUMENT - Invalid input parameter.
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_SetDisplayConfig(_In_ NvU32 pathInfoCount, __in_ecount(pathInfoCount) NV_DISPLAYCONFIG_PATH_INFO* pathInfo, _In_ NvU32 flags);





////////////////////////////////////////////////////////////////////////////////////////
//
// MOSAIC allows a multi display target output scanout on a single source. 
//
// SAMPLE of MOSAIC 1x4 topo with 8 pixel horizontal overlap
//
//+-------------------------++-------------------------++-------------------------++-------------------------+
//|                         ||                         ||                         ||                         |
//|                         ||                         ||                         ||                         |
//|                         ||                         ||                         ||                         |
//|        DVI1             ||           DVI2          ||         DVI3            ||          DVI4           |
//|                         ||                         ||                         ||                         |
//|                         ||                         ||                         ||                         |
//|                         ||                         ||                         ||                         |
//|                         ||                         ||                         ||                         |
//+-------------------------++-------------------------++-------------------------++-------------------------+


//! \addtogroup mosaicapi
//! @{

#define NVAPI_MAX_MOSAIC_DISPLAY_ROWS       8
#define NVAPI_MAX_MOSAIC_DISPLAY_COLUMNS    8
//
// These bits are used to describe the validity of a topo.
//
#define NV_MOSAIC_TOPO_VALIDITY_VALID               0x00000000  //!< The topology is valid
#define NV_MOSAIC_TOPO_VALIDITY_MISSING_GPU         0x00000001  //!< Not enough SLI GPUs were found to fill the entire
                                                                //! topology. hPhysicalGPU will be 0 for these.
#define NV_MOSAIC_TOPO_VALIDITY_MISSING_DISPLAY     0x00000002  //!< Not enough displays were found to fill the entire
                                                                //! topology. displayOutputId will be 0 for these.
#define NV_MOSAIC_TOPO_VALIDITY_MIXED_DISPLAY_TYPES 0x00000004  //!< The topoogy is only possible with displays of the same
                                                                //! NV_GPU_OUTPUT_TYPE. Check displayOutputIds to make
                                                                //! sure they are all CRTs, or all DFPs.


//
//! This structure defines the topology details.
typedef struct 
{
    NvU32                version;              //!< Version of this structure
    NvLogicalGpuHandle   hLogicalGPU;          //!< Logical GPU for this topology 
    NvU32                validityMask;         //!< 0 means topology is valid with the current hardware.
                                               //! If not 0, inspect bits against NV_MOSAIC_TOPO_VALIDITY_*.
    NvU32                rowCount;             //!< Number of displays in a row
    NvU32                colCount;             //!< Number of displays in a column

    struct 
    {
        NvPhysicalGpuHandle hPhysicalGPU;      //!< Physical GPU to be used in the topology (0 if GPU missing)
        NvU32               displayOutputId;   //!< Connected display target (0 if no display connected)
        NvS32               overlapX;          //!< Pixels of overlap on left of target: (+overlap, -gap)
        NvS32               overlapY;          //!< Pixels of overlap on top of target: (+overlap, -gap)

    } gpuLayout[NVAPI_MAX_MOSAIC_DISPLAY_ROWS][NVAPI_MAX_MOSAIC_DISPLAY_COLUMNS];

} NV_MOSAIC_TOPO_DETAILS;

//! Macro for constructing te vesion field of NV_MOSAIC_TOPO_DETAILS
#define NVAPI_MOSAIC_TOPO_DETAILS_VER         MAKE_NVAPI_VERSION(NV_MOSAIC_TOPO_DETAILS,1)


//
//! These values refer to the different types of Mosaic topologies that are possible.  When
//! getting the supported Mosaic topologies, you can specify one of these types to narrow down
//! the returned list to only those that match the given type.
typedef enum
{
    NV_MOSAIC_TOPO_TYPE_ALL,                          //!< All mosaic topologies
    NV_MOSAIC_TOPO_TYPE_BASIC,                        //!< Basic Mosaic topologies
    NV_MOSAIC_TOPO_TYPE_PASSIVE_STEREO,               //!< Passive Stereo topologies
    NV_MOSAIC_TOPO_TYPE_SCALED_CLONE,                 //!< Not supported at this time
    NV_MOSAIC_TOPO_TYPE_PASSIVE_STEREO_SCALED_CLONE,  //!< Not supported at this time
    NV_MOSAIC_TOPO_TYPE_MAX,                          //!< Always leave this at end of the enum
} NV_MOSAIC_TOPO_TYPE;


//
//! This is a complete list of supported Mosaic topologies.
//!
//! Using a "Basic" topology combines multiple monitors to create a single desktop.
//!
//! Using a "Passive" topology combines multiples monitors to create a passive stereo desktop.
//! In passive stereo, two identical topologies combine - one topology is used for the right eye and the other identical //! topology (targeting different displays) is used for the left eye.  \n  
//! NOTE: common\inc\nvEscDef.h shadows a couple PASSIVE_STEREO enums.  If this
//!       enum list changes and effects the value of NV_MOSAIC_TOPO_BEGIN_PASSIVE_STEREO
//!       please update the corresponding value in nvEscDef.h
typedef enum
{
    NV_MOSAIC_TOPO_NONE,

    // 'BASIC' topos start here
    //
    // The result of using one of these Mosaic topos is that multiple monitors
    // will combine to create a single desktop.
    //
    NV_MOSAIC_TOPO_BEGIN_BASIC,
    NV_MOSAIC_TOPO_1x2_BASIC = NV_MOSAIC_TOPO_BEGIN_BASIC,
    NV_MOSAIC_TOPO_2x1_BASIC,
    NV_MOSAIC_TOPO_1x3_BASIC,
    NV_MOSAIC_TOPO_3x1_BASIC,
    NV_MOSAIC_TOPO_1x4_BASIC,
    NV_MOSAIC_TOPO_4x1_BASIC,
    NV_MOSAIC_TOPO_2x2_BASIC,
    NV_MOSAIC_TOPO_2x3_BASIC,
    NV_MOSAIC_TOPO_2x4_BASIC,
    NV_MOSAIC_TOPO_3x2_BASIC,
    NV_MOSAIC_TOPO_4x2_BASIC,
    NV_MOSAIC_TOPO_1x5_BASIC,
    NV_MOSAIC_TOPO_1x6_BASIC,
    NV_MOSAIC_TOPO_7x1_BASIC,

    // Add padding for 10 more entries. 6 will be enough room to specify every
    // possible topology with 8 or fewer displays, so this gives us a little
    // extra should we need it.
    NV_MOSAIC_TOPO_END_BASIC = NV_MOSAIC_TOPO_7x1_BASIC + 9,

    // 'PASSIVE_STEREO' topos start here
    //
    // The result of using one of these Mosaic topos is that multiple monitors
    // will combine to create a single PASSIVE STEREO desktop.  What this means is
    // that there will be two topos that combine to create the overall desktop.
    // One topo will be used for the left eye, and the other topo (of the
    // same rows x cols), will be used for the right eye.  The difference between
    // the two topos is that different GPUs and displays will be used.
    //
    NV_MOSAIC_TOPO_BEGIN_PASSIVE_STEREO,    // value shadowed in nvEscDef.h
    NV_MOSAIC_TOPO_1x2_PASSIVE_STEREO = NV_MOSAIC_TOPO_BEGIN_PASSIVE_STEREO,
    NV_MOSAIC_TOPO_2x1_PASSIVE_STEREO,
    NV_MOSAIC_TOPO_1x3_PASSIVE_STEREO,
    NV_MOSAIC_TOPO_3x1_PASSIVE_STEREO,
    NV_MOSAIC_TOPO_1x4_PASSIVE_STEREO,
    NV_MOSAIC_TOPO_4x1_PASSIVE_STEREO,
    NV_MOSAIC_TOPO_2x2_PASSIVE_STEREO,
    NV_MOSAIC_TOPO_END_PASSIVE_STEREO = NV_MOSAIC_TOPO_2x2_PASSIVE_STEREO + 4,


    //
    // Total number of topos.  Always leave this at the end of the enumeration.
    //
    NV_MOSAIC_TOPO_MAX  //! Total number of topologies.

} NV_MOSAIC_TOPO;


//
//! This is a "topology brief" structure.  It tells you what you need to know about
//! a topology at a high level. A list of these is returned when you query for the
//! supported Mosaic information.
//!
//! If you need more detailed information about the topology, call
//! NvAPI_Mosaic_GetTopoGroup() with the topology value from this structure.
typedef struct 
{
    NvU32                        version;            //!< Version of this structure
    NV_MOSAIC_TOPO               topo;               //!< The topology
    NvU32                        enabled;            //!< 1 if topo is enabled, else 0
    NvU32                        isPossible;         //!< 1 if topo *can* be enabled, else 0

} NV_MOSAIC_TOPO_BRIEF;

//! Macro for constructing the version field of NV_MOSAIC_TOPO_BRIEF
#define NVAPI_MOSAIC_TOPO_BRIEF_VER         MAKE_NVAPI_VERSION(NV_MOSAIC_TOPO_BRIEF,1)


//
//! Basic per-display settings that are used in setting/getting the Mosaic mode
typedef struct 
{
    NvU32                        version;            //!< Version of this structure
    NvU32                        width;              //!< Per-display width
    NvU32                        height;             //!< Per-display height
    NvU32                        bpp;                //!< Bits per pixel
    NvU32                        freq;               //!< Display frequency
} NV_MOSAIC_DISPLAY_SETTING;

//! Macro for constructing the version field of NV_MOSAIC_DISPLAY_SETTING
#define NVAPI_MOSAIC_DISPLAY_SETTING_VER         MAKE_NVAPI_VERSION(NV_MOSAIC_DISPLAY_SETTING,1)


//
// Set a reasonable max number of display settings to support
// so arrays are bound.
//
#define NV_MOSAIC_DISPLAY_SETTINGS_MAX 40  //!< Set a reasonable maximum number of display settings to support
                                           //! so arrays are bound.


//
//! This structure is used to contain a list of supported Mosaic topologies
//! along with the display settings that can be used.
typedef struct 
{
    NvU32                       version;                                         //!< Version of this structure
    NvU32                       topoBriefsCount;                                 //!< Number of topologies in below array
    NV_MOSAIC_TOPO_BRIEF        topoBriefs[NV_MOSAIC_TOPO_MAX];                  //!< List of supported topologies with only brief details
    NvU32                       displaySettingsCount;                            //!< Number of display settings in below array
    NV_MOSAIC_DISPLAY_SETTING   displaySettings[NV_MOSAIC_DISPLAY_SETTINGS_MAX]; //!< List of per display settings possible

} NV_MOSAIC_SUPPORTED_TOPO_INFO;

//! Macro forconstructing  the version field of NV_MOSAIC_SUPPORTED_TOPO_INFO
#define NVAPI_MOSAIC_SUPPORTED_TOPO_INFO_VER         MAKE_NVAPI_VERSION(NV_MOSAIC_SUPPORTED_TOPO_INFO,1)


//
// Indices to use to access the topos array within the mosaic topology
#define NV_MOSAIC_TOPO_IDX_DEFAULT       0

#define NV_MOSAIC_TOPO_IDX_LEFT_EYE      0
#define NV_MOSAIC_TOPO_IDX_RIGHT_EYE     1
#define NV_MOSAIC_TOPO_NUM_EYES          2


//
//! This defines the maximum number of topos that can be in a topo group.
//! At this time, it is set to 2 because our largest topo group (passive
//! stereo) only needs 2 topos (left eye and right eye).
//!
//! If a new topo group with more than 2 topos is added above, then this
//! number will also have to be incremented.
#define NV_MOSAIC_MAX_TOPO_PER_TOPO_GROUP 2 


//
//! This structure defines a group of topologies that work together to create one
//! overall layout.  All of the supported topologies are represented with this
//! structure.
//!
//! For example, a 'Passive Stereo' topology would be represented with this
//! structure, and would have separate topology details for the left and right eyes.
//! The count would be 2.  A 'Basic' topology is also represented by this structure,
//! with a count of 1.
//!
//! The structure is primarily used internally, but is exposed to applications in a
//! read-only fashion because there are some details in it that might be useful
//! (like the number of rows/cols, or connected display information).  A user can
//! get the filled-in structure by calling NvAPI_Mosaic_GetTopoGroup().
//!
//! You can then look at the detailed values within the structure.  There are no
//! entrypoints which take this structure as input (effectively making it read-only).
typedef struct 
{
    NvU32                      version;              //!< Version of this structure
    NV_MOSAIC_TOPO_BRIEF       brief;                //!< The brief details of this topo
    NvU32                      count;                //!< Number of topos in array below
    NV_MOSAIC_TOPO_DETAILS     topos[NV_MOSAIC_MAX_TOPO_PER_TOPO_GROUP];

} NV_MOSAIC_TOPO_GROUP;

//! Macro for constructing the version field of NV_MOSAIC_TOPO_GROUP
#define NVAPI_MOSAIC_TOPO_GROUP_VER         MAKE_NVAPI_VERSION(NV_MOSAIC_TOPO_GROUP,1)

//! @}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_GetSupportedTopoInfo
//
//! DESCRIPTION:     This API returns information on the topologies and display resolutions
//!                  supported by Mosaic mode.
//!
//!                  NOTE: Not all topologies returned can be set immediately.
//!                        See 'OUT' Notes below.
//!
//!                  Once you get the list of supported topologies, you can call
//!                  NvAPI_Mosaic_GetTopoGroup() with one of the Mosaic topologies if you need
//!                  more information about it.
//!
//!     <b>'IN' Notes:</b>  pSupportedTopoInfo->version must be set before calling this function.
//!                  If the specified version is not supported by this implementation,
//!                  an error will be returned (NVAPI_INCOMPATIBLE_STRUCT_VERSION).
//!
//!     <b>'OUT' Notes:</b> Some of the topologies returned might not be valid for one reason or
//!                  another.  It could be due to mismatched or missing displays.  It
//!                  could also be because the required number of GPUs is not found.
//!                  At a high level, you can see if the topology is valid and can be enabled
//!                  by looking at the pSupportedTopoInfo->topoBriefs[xxx].isPossible flag.
//!                  If this is true, the topology can be enabled. If it
//!                  is false, you can find out why it cannot be enabled by getting the
//!                  details of the topology via NvAPI_Mosaic_GetTopoGroup().  From there,
//!                  look at the validityMask of the individual topologies.  The bits can
//!                  be tested against the NV_MOSAIC_TOPO_VALIDITY_* bits.
//!
//!                  It is possible for this function to return NVAPI_OK with no topologies
//!                  listed in the return structure.  If this is the case, it means that
//!                  the current hardware DOES support Mosaic, but with the given configuration
//!                  no valid topologies were found.  This most likely means that SLI was not
//!                  enabled for the hardware. Once enabled, you should see valid topologies
//!                  returned from this function.
//!    
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 185
//!
//!
//! \param [in,out]  pSupportedTopoInfo  Information about what topologies and display resolutions
//!                                      are supported for Mosaic.
//! \param [in]      type                The type of topologies the caller is interested in
//!                                      getting. See NV_MOSAIC_TOPO_TYPE for possible values.
//!
//! \retval ::NVAPI_OK                          No errors in returning supported topologies.
//! \retval ::NVAPI_NOT_SUPPORTED               Mosaic is not supported with the existing hardware.
//! \retval ::NVAPI_INVALID_ARGUMENT            One or more arguments passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED          The NvAPI API needs to be initialized first.
//! \retval ::NVAPI_NO_IMPLEMENTATION           This entrypoint not available.
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION The version of the structure passed in is not
//                                              compatible with this entry point.
//! \retval ::NVAPI_ERROR:                      Miscellaneous error occurred.
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_GetSupportedTopoInfo(NV_MOSAIC_SUPPORTED_TOPO_INFO *pSupportedTopoInfo, NV_MOSAIC_TOPO_TYPE type);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_GetTopoGroup
//
//! DESCRIPTION:     This API returns a structure filled with the details
//!                  of the specified Mosaic topology.
//!
//!                  If the pTopoBrief passed in matches the current topology,
//!                  then information in the brief and group structures
//!                  will reflect what is current. Thus the brief would have
//!                  the current 'enable' status, and the group would have the
//!                  current overlap values. If there is no match, then the
//!                  returned brief has an 'enable' status of FALSE (since it
//!                  is obviously not enabled), and the overlap values will be 0.
//!
//!     <b>'IN' Notes:</b>  pTopoGroup->version must be set before calling this function.
//!                  If the specified version is not supported by this implementation,
//!                  an error will be returned (NVAPI_INCOMPATIBLE_STRUCT_VERSION).
//!
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 185
//!
//! \param [in]     pTopoBrief        The topology for getting the details
//!                                   This must be one of the topology briefs
//!                                   returned from NvAPI_Mosaic_GetSupportedTopoInfo().
//! \param [in,out] pTopoGroup        The topology details matching the brief
//!
//! \retval ::NVAPI_OK                          Details were retrieved successfully.
//! \retval ::NVAPI_NOT_SUPPORTED               Mosaic is not supported with the existing hardware.
//! \retval ::NVAPI_INVALID_ARGUMENT            One or more argumentss passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED          The NvAPI API needs to be initialized first.
//! \retval ::NVAPI_NO_IMPLEMENTATION           This entrypoint not available.
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION The version of the structure passed in is not
//                                              compatible with this entry point.
//! \retval ::NVAPI_ERROR:                      Miscellaneous error occurred.
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_GetTopoGroup(NV_MOSAIC_TOPO_BRIEF *pTopoBrief, NV_MOSAIC_TOPO_GROUP *pTopoGroup);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_GetOverlapLimits
//
//! DESCRIPTION:     This API returns the X and Y overlap limits required if
//!                  the given Mosaic topology and display settings are to be used.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 185
//!
//! \param [in]   pTopoBrief          The topology for getting limits
//!                                   This must be one of the topo briefs
//!                                   returned from NvAPI_Mosaic_GetSupportedTopoInfo().
//! \param [in]   pDisplaySetting     The display settings for getting the limits.
//!                                   This must be one of the settings
//!                                   returned from NvAPI_Mosaic_GetSupportedTopoInfo().
//! \param [out]  pMinOverlapX        X overlap minimum
//! \param [out]  pMaxOverlapX        X overlap maximum
//! \param [out]  pMinOverlapY        Y overlap minimum
//! \param [out]  pMaxOverlapY        Y overlap maximum
//!
//! \retval ::NVAPI_OK                          Details were retrieved successfully.
//! \retval ::NVAPI_NOT_SUPPORTED               Mosaic is not supported with the existing hardware.
//! \retval ::NVAPI_INVALID_ARGUMENT            One or more argumentss passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED          The NvAPI API needs to be initialized first.
//! \retval ::NVAPI_NO_IMPLEMENTATION           This entrypoint not available.
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION The version of the structure passed in is not
//!                                             compatible with this entry point.
//! \retval ::NVAPI_ERROR                       Miscellaneous error occurred.
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_GetOverlapLimits(NV_MOSAIC_TOPO_BRIEF *pTopoBrief, NV_MOSAIC_DISPLAY_SETTING *pDisplaySetting, NvS32 *pMinOverlapX, NvS32 *pMaxOverlapX, NvS32 *pMinOverlapY, NvS32 *pMaxOverlapY);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_SetCurrentTopo
//
//! DESCRIPTION:     This API sets the Mosaic topology and performs a mode switch 
//!                  using the given display settings.
//!
//!                  If NVAPI_OK is returned, the current Mosaic topology was set
//!                  correctly.  Any other status returned means the
//!                  topology was not set, and remains what it was before this
//!                  function was called.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 185
//!
//! \param [in]     pTopoBrief        The topology to set. This must be one of the topologies returned from
//!                                   NvAPI_Mosaic_GetSupportedTopoInfo(), and it must have an isPossible value of 1.
//! \param [in]     pDisplaySetting   The per display settings to be used in the Mosaic mode. This must be one of the
//!                                   settings returned from NvAPI_Mosaic_GetSupportedTopoInfo().
//! \param [in]     overlapX          The pixel overlap to use between horizontal displays (use positive a number for
//!                                   overlap, or a negative number to create a gap.) If the overlap is out of bounds
//!                                   for what is possible given the topo and display setting, the overlap will be clamped.
//! \param [in]     overlapY          The pixel overlap to use between vertical displays (use positive a number for
//!                                   overlap, or a negative number to create a gap.) If the overlap is out of bounds for
//!                                   what is possible given the topo and display setting, the overlap will be clamped.
//! \param [in]     enable            If 1, the topology being set will also be enabled, meaning that the mode set will
//!                                   occur.  \n
//!                                   If 0, you don't want to be in Mosaic mode right now, but want to set the current
//!                                   Mosaic topology so you can enable it later with NvAPI_Mosaic_EnableCurrentTopo().
//!
//! \retval  ::NVAPI_OK                          The Mosaic topology was set.
//! \retval  ::NVAPI_NOT_SUPPORTED               Mosaic is not supported with the existing hardware.
//! \retval  ::NVAPI_INVALID_ARGUMENT            One or more argumentss passed in are invalid.
//! \retval  ::NVAPI_TOPO_NOT_POSSIBLE           The topology passed in is not currently possible.
//! \retval  ::NVAPI_API_NOT_INTIALIZED          The NvAPI API needs to be initialized first.
//! \retval  ::NVAPI_NO_IMPLEMENTATION           This entrypoint not available.
//! \retval  ::NVAPI_INCOMPATIBLE_STRUCT_VERSION The version of the structure passed in is not
//!                                              compatible with this entrypoint.
//! \retval  ::NVAPI_MODE_CHANGE_FAILED          There was an error changing the display mode.
//! \retval  ::NVAPI_ERROR                       Miscellaneous error occurred.
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_SetCurrentTopo(NV_MOSAIC_TOPO_BRIEF *pTopoBrief, NV_MOSAIC_DISPLAY_SETTING *pDisplaySetting, NvS32 overlapX, NvS32 overlapY, NvU32 enable);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_GetCurrentTopo
//
//! DESCRIPTION:     This API returns information for the current Mosaic topology.
//!                  This includes topology, display settings, and overlap values.
//!
//!                  You can call NvAPI_Mosaic_GetTopoGroup() with the topology
//!                  if you require more information.
//!
//!                  If there isn't a current topology, then pTopoBrief->topo will
//!                  be NV_MOSAIC_TOPO_NONE.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 185
//!
//! \param [out]     pTopoBrief       The current Mosaic topology
//! \param [out]     pDisplaySetting  The current per-display settings
//! \param [out]     pOverlapX        The pixel overlap between horizontal displays
//! \param [out]     pOverlapY        The pixel overlap between vertical displays
//!
//! \retval ::NVAPI_OK                          Success getting current info.
//! \retval ::NVAPI_NOT_SUPPORTED               Mosaic is not supported with the existing hardware.
//! \retval ::NVAPI_INVALID_ARGUMENT            One or more argumentss passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED          The NvAPI API needs to be initialized first.
//! \retval ::NVAPI_NO_IMPLEMENTATION           This entry point not available.
//! \retval ::NVAPI_ERROR                       Miscellaneous error occurred.
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_GetCurrentTopo(NV_MOSAIC_TOPO_BRIEF *pTopoBrief, NV_MOSAIC_DISPLAY_SETTING *pDisplaySetting, NvS32 *pOverlapX, NvS32 *pOverlapY);


#define NVAPI_MAX_GSYNC_DEVICES                       4


// Sync Display APIs

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_EnumSyncDevices
//
//!   DESCRIPTION: This API returns an array of Sync device handles. A Sync device handle represents a
//!                single Sync device on the system.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 313
//!
//! \param [out] nvGSyncHandles-  The caller provides an array of handles, which must contain at least
//!                               NVAPI_MAX_GSYNC_DEVICES elements. The API will zero out the entire array and then fill in one
//!                               or more handles. If an error occurs, the array is invalid.
//! \param [out] *gsyncCount-     The caller provides the storage space. NvAPI_GSync_EnumSyncDevices
//!                               sets *gsyncCount to indicate how many of the elements in the nvGSyncHandles[] array are valid.
//!                               If an error occurs, *gsyncCount will be set to zero.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//! \retval ::NVAPI_INVALID_ARGUMENT         nvGSyncHandles or gsyncCount is NULL.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND  The queried Graphics system does not have any Sync Device.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_EnumSyncDevices(_Out_ NvGSyncDeviceHandle nvGSyncHandles[NVAPI_MAX_GSYNC_DEVICES], _Out_ NvU32 *gsyncCount);



// GSync boardId values
#define NVAPI_GSYNC_BOARD_ID_P358 856		//!< GSync board ID 0x358, see NV_GSYNC_CAPABILITIES
#define NVAPI_GSYNC_BOARD_ID_P2060 8288		//!< GSync board ID 0x2060, see NV_GSYNC_CAPABILITIES 


//! Used in NvAPI_GSync_QueryCapabilities().
typedef struct _NV_GSYNC_CAPABILITIES
{
    NvU32   version;						//!< Version of the structure
    NvU32   boardId;						//!< Board ID
    NvU32   revision;						//!< FPGA Revision
    NvU32   capFlags;						//!< Capabilities of the Sync board. Reserved for future use
} NV_GSYNC_CAPABILITIES;



//! \ingroup gsyncapi
//! Macro for constructing the version field of NV_GSYNC_CAPABILITIES.
#define NV_GSYNC_CAPABILITIES_VER  MAKE_NVAPI_VERSION(NV_GSYNC_CAPABILITIES,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_QueryCapabilities
//
//!   DESCRIPTION: This API returns the capabilities of the Sync device.
//!
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 313
//!
//! \param [in]    hNvGSyncDevice-        The handle for a Sync device for which the capabilities will be queried.
//! \param [inout] *pNvGSyncCapabilities- The caller provides the storage space. NvAPI_GSync_QueryCapabilities() sets
//!                                       *pNvGSyncCapabilities to the version and capabilities details of the Sync device
//!                                       If an error occurs, *pNvGSyncCapabilities will be set to NULL.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//! \retval ::NVAPI_INVALID_ARGUMENT         hNvGSyncDevice is NULL.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND  The queried Graphics system does not have any Sync Device.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_QueryCapabilities(_In_ NvGSyncDeviceHandle hNvGSyncDevice, _Inout_ NV_GSYNC_CAPABILITIES *pNvGSyncCapabilities);



//! Connector values for a GPU. Used in NV_GSYNC_GPU.
typedef enum _NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR
{
    NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_NONE         = 0,
    NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_PRIMARY      = 1,
    NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_SECONDARY    = 2,
    NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_TERTIARY     = 3,
    NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR_QUARTERNARY  = 4,
} NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR;

//! Display sync states. Used in NV_GSYNC_DISPLAY.
typedef enum _NVAPI_GSYNC_DISPLAY_SYNC_STATE
{
    NVAPI_GSYNC_DISPLAY_SYNC_STATE_UNSYNCED         = 0,
    NVAPI_GSYNC_DISPLAY_SYNC_STATE_SLAVE            = 1,
    NVAPI_GSYNC_DISPLAY_SYNC_STATE_MASTER           = 2,
} NVAPI_GSYNC_DISPLAY_SYNC_STATE;

typedef struct _NV_GSYNC_GPU
{
    NvU32                               version;            //!< Version of the structure
    NvPhysicalGpuHandle                 hPhysicalGpu;       //!< GPU handle
    NVAPI_GSYNC_GPU_TOPOLOGY_CONNECTOR  connector;          //!< Indicates which connector on the device the GPU is connected to.
    NvPhysicalGpuHandle                 hProxyPhysicalGpu;  //!< GPU through which hPhysicalGpu is connected to the Sync device (if not directly connected)
                                                            //!<  - this is NULL otherwise
    NvU32                               isSynced : 1;       //!< Whether this GPU is sync'd or not.
    NvU32                               reserved : 31;      //!< Should be set to ZERO
} NV_GSYNC_GPU;

typedef struct _NV_GSYNC_DISPLAY
{
    NvU32                               version;            //!< Version of the structure
    NvU32                               displayId;          //!< display identifier for displays.The GPU to which it is connected, can be retireved from NvAPI_SYS_GetPhysicalGpuFromDisplayId
    NvU32                               isMasterable : 1;   //!< Can this display be the master? (Read only)
    NvU32                               reserved : 31;      //!< Should be set to ZERO
    NVAPI_GSYNC_DISPLAY_SYNC_STATE      syncState;          //!< Is this display slave/master
                                                            //!< (Retrieved with topology or set by caller for enable/disable sync)
} NV_GSYNC_DISPLAY;

#define NV_GSYNC_DISPLAY_VER  MAKE_NVAPI_VERSION(NV_GSYNC_DISPLAY,1)
#define NV_GSYNC_GPU_VER      MAKE_NVAPI_VERSION(NV_GSYNC_GPU,1)


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_GetTopology
//
//!   DESCRIPTION: This API returns the topology for the specified Sync device.
//!
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 313
//!
//! \param [in]       hNvGSyncDevice-     The caller provides the handle for a Sync device for which the topology will be queried.
//! \param [in, out]  gsyncGpuCount-      It returns number of GPUs connected to Sync device
//! \param [in, out]  gsyncGPUs-          It returns info about GPUs connected to Sync device
//! \param [in, out]  gsyncDisplayCount-  It returns number of active displays that belongs to Sync device
//! \param [in, out]  gsyncDisplays-      It returns info about all active displays that belongs to Sync device
//!
//! HOW TO USE: 1) make a call to get the number of GPUs connected OR displays synced through Sync device
//!                by passing the gsyncGPUs OR gsyncDisplays as NULL respectively. Both gsyncGpuCount and gsyncDisplayCount can be retrieved in same call by passing
//!                both gsyncGPUs and gsyncDisplays as NULL
//!                On call success:
//!             2) Allocate memory based on gsyncGpuCount(for gsyncGPUs) and/or gsyncDisplayCount(for gsyncDisplays) then make a call to populate gsyncGPUs and/or gsyncDisplays respectively.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//! \retval ::NVAPI_INVALID_ARGUMENT               hNvGSyncDevice is NULL.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND        The queried Graphics system does not have any Sync Device.
//! \retval ::NVAPI_INSUFFICIENT_BUFFER            When the actual number of GPUs/displays in the topology exceed the number of elements allocated for SyncGPUs/SyncDisplays respectively.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_GetTopology(_In_ NvGSyncDeviceHandle hNvGSyncDevice, __inout_opt NvU32 *gsyncGpuCount,  __inout_ecount_part_opt(*gsyncGpuCount, *gsyncGpuCount) NV_GSYNC_GPU *gsyncGPUs,
                                        __inout_opt NvU32 *gsyncDisplayCount, __inout_ecount_part_opt(*gsyncDisplayCount, *gsyncDisplayCount) NV_GSYNC_DISPLAY *gsyncDisplays);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_SetSyncStateSettings
//
//!   DESCRIPTION: Sets a new sync state for the displays in system.
//!
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 313
//!
//! \param [in]  gsyncDisplayCount-			The number of displays in gsyncDisplays.
//! \param [in]  pGsyncDisplays-			The caller provides the structure containing all displays that need to be synchronized in the system. 
//!											The displays that are not part of pGsyncDisplays, will be un-synchronized.
//! \param [in]  flags-						Reserved for future use.
//!
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//!
//! \retval ::NVAPI_INVALID_ARGUMENT			If the display topology or count not valid.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND		The queried Graphics system does not have any Sync Device.
//! \retval ::NVAPI_INVALID_SYNC_TOPOLOGY       1.If any mosaic grid is partial.
//!                                             2.If timing(HVisible/VVisible/refreshRate) applied of any display is different. 
//!                                             3.If There is a across GPU mosaic grid in system and that is not a part of pGsyncDisplays.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_SetSyncStateSettings(_In_ NvU32 gsyncDisplayCount, __in_ecount(gsyncDisplayCount) NV_GSYNC_DISPLAY *pGsyncDisplays, _In_ NvU32 flags);


//! \ingroup gsyncapi

//! Source signal edge to be used for output pulse. See NV_GSYNC_CONTROL_PARAMS.
typedef enum _NVAPI_GSYNC_POLARITY
{
    NVAPI_GSYNC_POLARITY_RISING_EDGE     = 0,
    NVAPI_GSYNC_POLARITY_FALLING_EDGE    = 1,
    NVAPI_GSYNC_POLARITY_BOTH_EDGES      = 2,
} NVAPI_GSYNC_POLARITY;

//! Used in NV_GSYNC_CONTROL_PARAMS.
typedef enum _NVAPI_GSYNC_VIDEO_MODE
{
    NVAPI_GSYNC_VIDEO_MODE_NONE          = 0,
    NVAPI_GSYNC_VIDEO_MODE_TTL           = 1,
    NVAPI_GSYNC_VIDEO_MODE_NTSCPALSECAM  = 2,
    NVAPI_GSYNC_VIDEO_MODE_HDTV          = 3,
    NVAPI_GSYNC_VIDEO_MODE_COMPOSITE     = 4,
} NVAPI_GSYNC_VIDEO_MODE;

//! Used in NV_GSYNC_CONTROL_PARAMS.  
typedef enum _NVAPI_GSYNC_SYNC_SOURCE
{
    NVAPI_GSYNC_SYNC_SOURCE_VSYNC        = 0,
    NVAPI_GSYNC_SYNC_SOURCE_HOUSESYNC    = 1,
} NVAPI_GSYNC_SYNC_SOURCE;

//! Used in NV_GSYNC_CONTROL_PARAMS. 
typedef struct _NV_GSYNC_DELAY
{
    NvU32        version;          //!< Version of the structure
    NvU32        numLines;         //!< delay to be induced in number of horizontal lines.
    NvU32        numPixels;        //!< delay to be induced in number of pixels.
    NvU32        maxLines;         //!< maximum number of lines supported at current display mode to induce delay. Updated by NvAPI_GSync_GetControlParameters(). Read only.
    NvU32        minPixels;        //!< minimum number of pixels required at current display mode to induce delay. Updated by NvAPI_GSync_GetControlParameters(). Read only.
} NV_GSYNC_DELAY;

#define NV_GSYNC_DELAY_VER  MAKE_NVAPI_VERSION(NV_GSYNC_DELAY,1)

//! Used in NvAPI_GSync_GetControlParameters() and NvAPI_GSync_SetControlParameters().
typedef struct _NV_GSYNC_CONTROL_PARAMS
{
    NvU32                       version;            //!< Version of the structure
    NVAPI_GSYNC_POLARITY        polarity;           //!< Leading edge / Falling edge / both
    NVAPI_GSYNC_VIDEO_MODE      vmode;              //!< None, TTL, NTSCPALSECAM, HDTV
    NvU32                       interval;           //!< Number of pulses to wait between framelock signal generation
    NVAPI_GSYNC_SYNC_SOURCE     source;             //!< VSync/House sync
    NvU32                       interlaceMode:1;    //!< interlace mode for a Sync device
    NvU32                       reserved:31;        //!< should be set zero
    NV_GSYNC_DELAY              syncSkew;           //!< The time delay between the frame sync signal and the GPUs signal. 
    NV_GSYNC_DELAY              startupDelay;       //!< Sync start delay for master. 
} NV_GSYNC_CONTROL_PARAMS;

#define NV_GSYNC_CONTROL_PARAMS_VER  MAKE_NVAPI_VERSION(NV_GSYNC_CONTROL_PARAMS,1)


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_GetControlParameters
//
//!   DESCRIPTION: This API queries for sync control parameters as defined in NV_GSYNC_CONTROL_PARAMS.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 313
//!
//! \param [in]    hNvGSyncDevice-   The caller provides the handle of the Sync device for which to get parameters
//! \param [inout] *pGsyncControls-  The caller provides the storage space. NvAPI_GSync_GetControlParameters() populates *pGsyncControls with values.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//! \retval ::NVAPI_INVALID_ARGUMENT          hNvGSyncDevice is NULL.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND   The queried Graphics system does not have any Sync Device.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_GetControlParameters(_In_ NvGSyncDeviceHandle hNvGSyncDevice, _Inout_ NV_GSYNC_CONTROL_PARAMS *pGsyncControls);



//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_SetControlParameters
//
//!   DESCRIPTION: This API sets control parameters as defined in NV_SYNC_CONTROL_PARAMS.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 313
//!
//! \param [in]  hNvGSyncDevice-   The caller provides the handle of the Sync device for which to get parameters
//! \param [inout]  *pGsyncControls-  The caller provides NV_GSYNC_CONTROL_PARAMS. skew and startDelay will be updated to the applied values.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//! \retval ::NVAPI_INVALID_ARGUMENT          hNvGSyncDevice is NULL.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND   The queried Graphics system does not have any Sync Device.
//! \retval ::NVAPI_SYNC_MASTER_NOT_FOUND     Control Parameters can only be set if there is a Sync Master enabled on the Gsync card.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_SetControlParameters(_In_ NvGSyncDeviceHandle hNvGSyncDevice, _Inout_ NV_GSYNC_CONTROL_PARAMS *pGsyncControls);




//! Used in NvAPI_GSync_AdjustSyncDelay()
typedef enum _NVAPI_GSYNC_DELAY_TYPE
{
    NVAPI_GSYNC_DELAY_TYPE_UNKNOWN			= 0,
    NVAPI_GSYNC_DELAY_TYPE_SYNC_SKEW     	= 1,
    NVAPI_GSYNC_DELAY_TYPE_STARTUP     		= 2
} NVAPI_GSYNC_DELAY_TYPE;

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_AdjustSyncDelay
//
//!   DESCRIPTION: This API adjusts the skew and startDelay to the closest possible values. Use this API before calling NvAPI_GSync_SetControlParameters for skew or startDelay.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 319
//!
//! \param [in]  hNvGSyncDevice-   	The caller provides the handle of the Sync device for which to get parameters
//! \param [in]  delayType-   		Specifies whether the delay is syncSkew or startupDelay. 
//! \param [inout]  *pGsyncDelay-  	The caller provides NV_GSYNC_DELAY. skew and startDelay will be adjusted and updated to the closest values.
//! \param [out]  *syncSteps-  		This parameter is optional. It returns the sync delay in unit steps. If 0, it means either the NV_GSYNC_DELAY::numPixels is less than NV_GSYNC_DELAY::minPixels or NV_GSYNC_DELAY::numOfLines exceeds the NV_GSYNC_DELAY::maxLines.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_AdjustSyncDelay(_In_ NvGSyncDeviceHandle hNvGSyncDevice, _In_ NVAPI_GSYNC_DELAY_TYPE delayType, _Inout_ NV_GSYNC_DELAY *pGsyncDelay, _Out_opt_ NvU32* syncSteps);



//! Used in NvAPI_GSync_GetSyncStatus().
typedef struct _NV_GSYNC_STATUS
{
    NvU32 version;                          //!< Version of the structure
    NvU32 bIsSynced;                        //!< Is timing in sync?
    NvU32 bIsStereoSynced;                  //!< Does the phase of the timing signal from the GPU = the phase of the master sync signal?
    NvU32 bIsSyncSignalAvailable;           //!< Is the sync signal available?
} NV_GSYNC_STATUS;

//! Macro for constructing the version field for NV_GSYNC_STATUS.
#define NV_GSYNC_STATUS_VER  MAKE_NVAPI_VERSION(NV_GSYNC_STATUS,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_GetSyncStatus
//
//!   DESCRIPTION: This API queries the sync status of a GPU - timing, stereosync and sync signal availability.
//!
//! SUPPORTED OS:  Windows 7 and higher
//!
//!
//! \since Release: 313
//!
//! \param [in]  hNvGSyncDevice-     Handle of the Sync device
//! \param [in]  hPhysicalGpu-       GPU to be queried for sync status.
//! \param [out] *status-            The caller provides the storage space. NvAPI_GSync_GetSyncStatus() populates *status with
//!                                  values - timing, stereosync and signal availability. On error, *status is set to NULL.
//!
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//! \retval ::NVAPI_INVALID_ARGUMENT          hNvGSyncDevice is NULL / SyncTarget is NULL.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND   The queried Graphics system does not have any G-Sync Device.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_GetSyncStatus(_In_ NvGSyncDeviceHandle hNvGSyncDevice, _In_ NvPhysicalGpuHandle hPhysicalGpu, _Inout_ NV_GSYNC_STATUS *status);


//! \ingroup gsyncapi

#define NVAPI_MAX_RJ45_PER_GSYNC    2

//! Used in NV_GSYNC_STATUS_PARAMS.
typedef enum _NVAPI_GSYNC_RJ45_IO
{
    NVAPI_GSYNC_RJ45_OUTPUT     = 0,
    NVAPI_GSYNC_RJ45_INPUT      = 1,
    NVAPI_GSYNC_RJ45_UNUSED     = 2 //!< This field is used to notify that the framelock is not actually present.

} NVAPI_GSYNC_RJ45_IO;

//! \ingroup gsyncapi
//! Used in NvAPI_GSync_GetStatusParameters().
typedef struct _NV_GSYNC_STATUS_PARAMS
{
    NvU32                       version;
    NvU32                       refreshRate;                                //!< The refresh rate
    NVAPI_GSYNC_RJ45_IO         RJ45_IO[NVAPI_MAX_RJ45_PER_GSYNC];          //!< Configured as input / output
    NvU32                       RJ45_Ethernet[NVAPI_MAX_RJ45_PER_GSYNC];    //!< Connected to ethernet hub? [ERRONEOUSLY CONNECTED!]
    NvU32                       houseSyncIncoming;                          //!< Incoming house sync frequency in Hz
    NvU32                       bHouseSync;                                 //!< Is house sync connected?
} NV_GSYNC_STATUS_PARAMS;


//! \ingroup gsyncapi
//! Macro for constructing the version field of NV_GSYNC_STATUS_PARAMS 
#define NV_GSYNC_STATUS_PARAMS_VER  MAKE_NVAPI_VERSION(NV_GSYNC_STATUS_PARAMS,1)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GSync_GetStatusParameters
//
//!   DESCRIPTION: This API queries for sync status parameters as defined in NV_GSYNC_STATUS_PARAMS.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 313
//!
//! \param [in]  hNvGSyncDevice   The caller provides the handle of the GSync device for which to get parameters
//! \param [out] *pStatusParams   The caller provides the storage space. NvAPI_GSync_GetStatusParameters populates *pStatusParams with
//!                               values.
//! 
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, they are listed below.
//! \retval ::NVAPI_INVALID_ARGUMENT            hNvGSyncDevice is NULL / pStatusParams is NULL.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND     The queried Graphics system does not have any GSync Device.
//!
//! \ingroup gsyncapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GSync_GetStatusParameters(NvGSyncDeviceHandle hNvGSyncDevice, NV_GSYNC_STATUS_PARAMS *pStatusParams);

//! @}

//! SUPPORTED OS:  Windows Vista and higher
//!

/////////////////////////////////////////////////////////////////////////
// Video Input Output (VIO) API
/////////////////////////////////////////////////////////////////////////

//! \ingroup vidio
//! Unique identifier for VIO owner (process identifier or NVVIOOWNERID_NONE)
typedef NvU32   NVVIOOWNERID;                               


//! \addtogroup vidio
//! @{


#define NVVIOOWNERID_NONE                   0      //!< Unregistered ownerId        


//! Owner type for device
typedef enum _NVVIOOWNERTYPE                               
{
    NVVIOOWNERTYPE_NONE                             ,       //!<  No owner for the device
    NVVIOOWNERTYPE_APPLICATION                      ,       //!<  Application owns the device
    NVVIOOWNERTYPE_DESKTOP                          ,       //!<  Desktop transparent mode owns the device (not applicable for video input)
}NVVIOOWNERTYPE;

// Access rights for NvAPI_VIO_Open()

//! Read access             (not applicable for video output)
#define NVVIO_O_READ                        0x00000000      

//! Write exclusive access  (not applicable for video input)
#define NVVIO_O_WRITE_EXCLUSIVE             0x00010001      

//! 
#define NVVIO_VALID_ACCESSRIGHTS            (NVVIO_O_READ              | \
                                             NVVIO_O_WRITE_EXCLUSIVE   )

              
//! VIO_DATA.ulOwnerID high-bit is set only if device has been initialized by VIOAPI
//! examined at NvAPI_GetCapabilities|NvAPI_VIO_Open to determine if settings need to be applied from registry or POR state read
#define NVVIO_OWNERID_INITIALIZED  0x80000000

//! VIO_DATA.ulOwnerID next-bit is set only if device is currently in exclusive write access mode from NvAPI_VIO_Open()
#define NVVIO_OWNERID_EXCLUSIVE    0x40000000
                                              
//! VIO_DATA.ulOwnerID lower bits are:
//!  NVGVOOWNERTYPE_xxx enumerations indicating use context
#define NVVIO_OWNERID_TYPEMASK     0x0FFFFFFF //!< mask for NVVIOOWNERTYPE_xxx


//! @}
              
//---------------------------------------------------------------------
// Enumerations
//---------------------------------------------------------------------


//! \addtogroup vidio
//! @{

//! Video signal format and resolution
typedef enum _NVVIOSIGNALFORMAT
{
    NVVIOSIGNALFORMAT_NONE,                //!< Invalid signal format 
    NVVIOSIGNALFORMAT_487I_59_94_SMPTE259_NTSC,     //!< 01  487i    59.94Hz  (SMPTE259) NTSC
    NVVIOSIGNALFORMAT_576I_50_00_SMPTE259_PAL,      //!< 02  576i    50.00Hz  (SMPTE259) PAL
    NVVIOSIGNALFORMAT_1035I_60_00_SMPTE260,         //!< 03  1035i   60.00Hz  (SMPTE260)
    NVVIOSIGNALFORMAT_1035I_59_94_SMPTE260,         //!< 04  1035i   59.94Hz  (SMPTE260)
    NVVIOSIGNALFORMAT_1080I_50_00_SMPTE295,         //!< 05  1080i   50.00Hz  (SMPTE295)
    NVVIOSIGNALFORMAT_1080I_60_00_SMPTE274,         //!< 06  1080i   60.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080I_59_94_SMPTE274,         //!< 07  1080i   59.94Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080I_50_00_SMPTE274,         //!< 08  1080i   50.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080P_30_00_SMPTE274,         //!< 09  1080p   30.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080P_29_97_SMPTE274,         //!< 10  1080p   29.97Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080P_25_00_SMPTE274,         //!< 11  1080p   25.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080P_24_00_SMPTE274,         //!< 12  1080p   24.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080P_23_976_SMPTE274,        //!< 13  1080p   23.976Hz (SMPTE274)
    NVVIOSIGNALFORMAT_720P_60_00_SMPTE296,          //!< 14  720p    60.00Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_720P_59_94_SMPTE296,          //!< 15  720p    59.94Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_720P_50_00_SMPTE296,          //!< 16  720p    50.00Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_1080I_48_00_SMPTE274,         //!< 17  1080I   48.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080I_47_96_SMPTE274,         //!< 18  1080I   47.96Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_720P_30_00_SMPTE296,          //!< 19  720p    30.00Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_720P_29_97_SMPTE296,          //!< 20  720p    29.97Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_720P_25_00_SMPTE296,          //!< 21  720p    25.00Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_720P_24_00_SMPTE296,          //!< 22  720p    24.00Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_720P_23_98_SMPTE296,          //!< 23  720p    23.98Hz  (SMPTE296)
    NVVIOSIGNALFORMAT_2048P_30_00_SMPTE372,         //!< 24  2048p   30.00Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048P_29_97_SMPTE372,         //!< 25  2048p   29.97Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048I_60_00_SMPTE372,         //!< 26  2048i   60.00Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048I_59_94_SMPTE372,         //!< 27  2048i   59.94Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048P_25_00_SMPTE372,         //!< 28  2048p   25.00Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048I_50_00_SMPTE372,         //!< 29  2048i   50.00Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048P_24_00_SMPTE372,         //!< 30  2048p   24.00Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048P_23_98_SMPTE372,         //!< 31  2048p   23.98Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048I_48_00_SMPTE372,         //!< 32  2048i   48.00Hz  (SMPTE372)
    NVVIOSIGNALFORMAT_2048I_47_96_SMPTE372,         //!< 33  2048i   47.96Hz  (SMPTE372)
    
    NVVIOSIGNALFORMAT_1080PSF_25_00_SMPTE274,       //!< 34  1080PsF 25.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080PSF_29_97_SMPTE274,       //!< 35  1080PsF 29.97Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080PSF_30_00_SMPTE274,       //!< 36  1080PsF 30.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080PSF_24_00_SMPTE274,       //!< 37  1080PsF 24.00Hz  (SMPTE274)
    NVVIOSIGNALFORMAT_1080PSF_23_98_SMPTE274,       //!< 38  1080PsF 23.98Hz  (SMPTE274)

    NVVIOSIGNALFORMAT_1080P_50_00_SMPTE274_3G_LEVEL_A, //!< 39  1080P   50.00Hz  (SMPTE274) 3G Level A
    NVVIOSIGNALFORMAT_1080P_59_94_SMPTE274_3G_LEVEL_A, //!< 40  1080P   59.94Hz  (SMPTE274) 3G Level A
    NVVIOSIGNALFORMAT_1080P_60_00_SMPTE274_3G_LEVEL_A, //!< 41  1080P   60.00Hz  (SMPTE274) 3G Level A
    
    NVVIOSIGNALFORMAT_1080P_60_00_SMPTE274_3G_LEVEL_B, //!< 42  1080p   60.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_1080I_60_00_SMPTE274_3G_LEVEL_B, //!< 43  1080i   60.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048I_60_00_SMPTE372_3G_LEVEL_B, //!< 44  2048i   60.00Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080P_50_00_SMPTE274_3G_LEVEL_B, //!< 45  1080p   50.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_1080I_50_00_SMPTE274_3G_LEVEL_B, //!< 46  1080i   50.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048I_50_00_SMPTE372_3G_LEVEL_B, //!< 47  2048i   50.00Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080P_30_00_SMPTE274_3G_LEVEL_B, //!< 48  1080p   30.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048P_30_00_SMPTE372_3G_LEVEL_B, //!< 49  2048p   30.00Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080P_25_00_SMPTE274_3G_LEVEL_B, //!< 50  1080p   25.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048P_25_00_SMPTE372_3G_LEVEL_B, //!< 51  2048p   25.00Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080P_24_00_SMPTE274_3G_LEVEL_B, //!< 52  1080p   24.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048P_24_00_SMPTE372_3G_LEVEL_B, //!< 53  2048p   24.00Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080I_48_00_SMPTE274_3G_LEVEL_B, //!< 54  1080i   48.00Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048I_48_00_SMPTE372_3G_LEVEL_B, //!< 55  2048i   48.00Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080P_59_94_SMPTE274_3G_LEVEL_B, //!< 56  1080p   59.94Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_1080I_59_94_SMPTE274_3G_LEVEL_B, //!< 57  1080i   59.94Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048I_59_94_SMPTE372_3G_LEVEL_B, //!< 58  2048i   59.94Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080P_29_97_SMPTE274_3G_LEVEL_B, //!< 59  1080p   29.97Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048P_29_97_SMPTE372_3G_LEVEL_B, //!< 60  2048p   29.97Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080P_23_98_SMPTE274_3G_LEVEL_B, //!< 61  1080p   29.98Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048P_23_98_SMPTE372_3G_LEVEL_B, //!< 62  2048p   29.98Hz  (SMPTE372) 3G Level B
    NVVIOSIGNALFORMAT_1080I_47_96_SMPTE274_3G_LEVEL_B, //!< 63  1080i   47.96Hz  (SMPTE274) 3G Level B
    NVVIOSIGNALFORMAT_2048I_47_96_SMPTE372_3G_LEVEL_B, //!< 64  2048i   47.96Hz  (SMPTE372) 3G Level B
    
    NVVIOSIGNALFORMAT_END                              //!< 65  To indicate end of signal format list

}NVVIOSIGNALFORMAT;

//! SMPTE standards format
typedef enum _NVVIOVIDEOSTANDARD
{
    NVVIOVIDEOSTANDARD_SMPTE259                        ,       //!< SMPTE259
    NVVIOVIDEOSTANDARD_SMPTE260                        ,       //!< SMPTE260
    NVVIOVIDEOSTANDARD_SMPTE274                        ,       //!< SMPTE274
    NVVIOVIDEOSTANDARD_SMPTE295                        ,       //!< SMPTE295
    NVVIOVIDEOSTANDARD_SMPTE296                        ,       //!< SMPTE296
    NVVIOVIDEOSTANDARD_SMPTE372                        ,       //!< SMPTE372
}NVVIOVIDEOSTANDARD;

//! HD or SD video type
typedef enum _NVVIOVIDEOTYPE
{
    NVVIOVIDEOTYPE_SD                                  ,       //!< Standard-definition (SD)
    NVVIOVIDEOTYPE_HD                                  ,       //!< High-definition     (HD)
}NVVIOVIDEOTYPE;

//! Interlace mode
typedef enum _NVVIOINTERLACEMODE 
{
    NVVIOINTERLACEMODE_PROGRESSIVE                     ,       //!< Progressive               (p)
    NVVIOINTERLACEMODE_INTERLACE                       ,       //!< Interlace                 (i)
    NVVIOINTERLACEMODE_PSF                             ,       //!< Progressive Segment Frame (psf)
}NVVIOINTERLACEMODE;

//! Video data format
typedef enum _NVVIODATAFORMAT
{
    NVVIODATAFORMAT_UNKNOWN   = -1                     ,       //!< Invalid DataFormat
    NVVIODATAFORMAT_R8G8B8_TO_YCRCB444                 ,       //!< R8:G8:B8                => YCrCb  (4:4:4)
    NVVIODATAFORMAT_R8G8B8A8_TO_YCRCBA4444             ,       //!< R8:G8:B8:A8             => YCrCbA (4:4:4:4)
    NVVIODATAFORMAT_R8G8B8Z10_TO_YCRCBZ4444            ,       //!< R8:G8:B8:Z10            => YCrCbZ (4:4:4:4)
    NVVIODATAFORMAT_R8G8B8_TO_YCRCB422                 ,       //!< R8:G8:B8                => YCrCb  (4:2:2)
    NVVIODATAFORMAT_R8G8B8A8_TO_YCRCBA4224             ,       //!< R8:G8:B8:A8             => YCrCbA (4:2:2:4)
    NVVIODATAFORMAT_R8G8B8Z10_TO_YCRCBZ4224            ,       //!< R8:G8:B8:Z10            => YCrCbZ (4:2:2:4)
    NVVIODATAFORMAT_X8X8X8_444_PASSTHRU                ,       //!< R8:G8:B8                => RGB    (4:4:4)
    NVVIODATAFORMAT_X8X8X8A8_4444_PASSTHRU             ,       //!< R8:G8:B8:A8             => RGBA   (4:4:4:4)
    NVVIODATAFORMAT_X8X8X8Z10_4444_PASSTHRU            ,       //!< R8:G8:B8:Z10            => RGBZ   (4:4:4:4)
    NVVIODATAFORMAT_X10X10X10_444_PASSTHRU             ,       //!< Y10:CR10:CB10           => YCrCb  (4:4:4)
    NVVIODATAFORMAT_X10X8X8_444_PASSTHRU               ,       //!< Y10:CR8:CB8             => YCrCb  (4:4:4)
    NVVIODATAFORMAT_X10X8X8A10_4444_PASSTHRU           ,       //!< Y10:CR8:CB8:A10         => YCrCbA (4:4:4:4)
    NVVIODATAFORMAT_X10X8X8Z10_4444_PASSTHRU           ,       //!< Y10:CR8:CB8:Z10         => YCrCbZ (4:4:4:4)
    NVVIODATAFORMAT_DUAL_R8G8B8_TO_DUAL_YCRCB422       ,       //!< R8:G8:B8 + R8:G8:B8     => YCrCb  (4:2:2 + 4:2:2)
    NVVIODATAFORMAT_DUAL_X8X8X8_TO_DUAL_422_PASSTHRU   ,       //!< Y8:CR8:CB8 + Y8:CR8:CB8 => YCrCb  (4:2:2 + 4:2:2)
    NVVIODATAFORMAT_R10G10B10_TO_YCRCB422              ,       //!< R10:G10:B10             => YCrCb  (4:2:2)
    NVVIODATAFORMAT_R10G10B10_TO_YCRCB444              ,       //!< R10:G10:B10             => YCrCb  (4:4:4)
    NVVIODATAFORMAT_X12X12X12_444_PASSTHRU             ,       //!< X12:X12:X12             => XXX    (4:4:4)
    NVVIODATAFORMAT_X12X12X12_422_PASSTHRU             ,       //!< X12:X12:X12             => XXX    (4:2:2)
    NVVIODATAFORMAT_Y10CR10CB10_TO_YCRCB422            ,       //!< Y10:CR10:CB10           => YCrCb  (4:2:2)
    NVVIODATAFORMAT_Y8CR8CB8_TO_YCRCB422               ,       //!< Y8:CR8:CB8              => YCrCb  (4:2:2)
    NVVIODATAFORMAT_Y10CR8CB8A10_TO_YCRCBA4224         ,       //!< Y10:CR8:CB8:A10         => YCrCbA (4:2:2:4)
    NVVIODATAFORMAT_R10G10B10_TO_RGB444                ,       //!< R10:G10:B10             => RGB    (4:4:4)
    NVVIODATAFORMAT_R12G12B12_TO_YCRCB444              ,       //!< R12:G12:B12             => YCrCb  (4:4:4)
    NVVIODATAFORMAT_R12G12B12_TO_YCRCB422              ,       //!< R12:G12:B12             => YCrCb  (4:2:2)
}NVVIODATAFORMAT;

//! Video output area
typedef enum _NVVIOOUTPUTAREA
{
    NVVIOOUTPUTAREA_FULLSIZE                           ,       //!< Output to entire video resolution (full size)
    NVVIOOUTPUTAREA_SAFEACTION                         ,       //!< Output to centered 90% of video resolution (safe action)
    NVVIOOUTPUTAREA_SAFETITLE                          ,       //!< Output to centered 80% of video resolution (safe title)
}NVVIOOUTPUTAREA;

//! Synchronization source
typedef enum _NVVIOSYNCSOURCE
{
    NVVIOSYNCSOURCE_SDISYNC                            ,       //!< SDI Sync  (Digital input)
    NVVIOSYNCSOURCE_COMPSYNC                           ,       //!< COMP Sync (Composite input)
}NVVIOSYNCSOURCE;

//! Composite synchronization type
typedef enum _NVVIOCOMPSYNCTYPE
{
    NVVIOCOMPSYNCTYPE_AUTO                             ,       //!< Auto-detect
    NVVIOCOMPSYNCTYPE_BILEVEL                          ,       //!< Bi-level signal
    NVVIOCOMPSYNCTYPE_TRILEVEL                         ,       //!< Tri-level signal
}NVVIOCOMPSYNCTYPE;

//! Video input output status
typedef enum _NVVIOINPUTOUTPUTSTATUS
{
    NVINPUTOUTPUTSTATUS_OFF                            ,       //!< Not in use
    NVINPUTOUTPUTSTATUS_ERROR                          ,       //!< Error detected
    NVINPUTOUTPUTSTATUS_SDI_SD                         ,       //!< SDI (standard-definition)
    NVINPUTOUTPUTSTATUS_SDI_HD                         ,       //!< SDI (high-definition)
}NVVIOINPUTOUTPUTSTATUS;

//! Synchronization input status
typedef enum _NVVIOSYNCSTATUS
{
    NVVIOSYNCSTATUS_OFF                                ,       //!< Sync not detected
    NVVIOSYNCSTATUS_ERROR                              ,       //!< Error detected
    NVVIOSYNCSTATUS_SYNCLOSS                           ,       //!< Genlock in use, format mismatch with output
    NVVIOSYNCSTATUS_COMPOSITE                          ,       //!< Composite sync
    NVVIOSYNCSTATUS_SDI_SD                             ,       //!< SDI sync (standard-definition)
    NVVIOSYNCSTATUS_SDI_HD                             ,       //!< SDI sync (high-definition)
}NVVIOSYNCSTATUS;

//! Video Capture Status
typedef enum _NVVIOCAPTURESTATUS
{
    NVVIOSTATUS_STOPPED                                ,       //!< Sync not detected
    NVVIOSTATUS_RUNNING                                ,       //!< Error detected
    NVVIOSTATUS_ERROR                                  ,       //!< Genlock in use, format mismatch with output
}NVVIOCAPTURESTATUS;

//! Video Capture Status
typedef enum _NVVIOSTATUSTYPE
{
    NVVIOSTATUSTYPE_IN                                 ,       //!< Input Status
    NVVIOSTATUSTYPE_OUT                                ,       //!< Output Status
}NVVIOSTATUSTYPE;


//! Assumption, maximum 4 SDI input and 4 SDI output cards supported on a system
#define NVAPI_MAX_VIO_DEVICES                 8   

//! 4 physical jacks supported on each SDI input card.
#define NVAPI_MAX_VIO_JACKS                   4   


//! Each physical jack an on SDI input card can have
//! two "channels" in the case of "3G" VideoFormats, as specified
//! by SMPTE 425; for non-3G VideoFormats, only the first channel within
//! a physical jack is valid.
#define NVAPI_MAX_VIO_CHANNELS_PER_JACK       2   

//! 4 Streams, 1 per physical jack
#define NVAPI_MAX_VIO_STREAMS                 4   

#define NVAPI_MIN_VIO_STREAMS                 1   

//! SDI input supports a max of 2 links per stream
#define NVAPI_MAX_VIO_LINKS_PER_STREAM        2   


#define NVAPI_MAX_FRAMELOCK_MAPPING_MODES     20

//! Min number of capture images 
#define NVAPI_GVI_MIN_RAW_CAPTURE_IMAGES      1   

//! Max number of capture images        
#define NVAPI_GVI_MAX_RAW_CAPTURE_IMAGES      32  

//! Default number of capture images
#define NVAPI_GVI_DEFAULT_RAW_CAPTURE_IMAGES  5   



// Data Signal notification events. These need a event handler in RM.
// Register/Unregister and PopEvent NVAPI's are already available.

//! Device configuration
typedef enum _NVVIOCONFIGTYPE
{
    NVVIOCONFIGTYPE_IN                                 ,       //!< Input Status
    NVVIOCONFIGTYPE_OUT                                ,       //!< Output Status
}NVVIOCONFIGTYPE;

typedef enum _NVVIOCOLORSPACE
{
    NVVIOCOLORSPACE_UNKNOWN,
    NVVIOCOLORSPACE_YCBCR,
    NVVIOCOLORSPACE_YCBCRA,
    NVVIOCOLORSPACE_YCBCRD,
    NVVIOCOLORSPACE_GBR,
    NVVIOCOLORSPACE_GBRA,
    NVVIOCOLORSPACE_GBRD,
} NVVIOCOLORSPACE;

//! Component sampling
typedef enum _NVVIOCOMPONENTSAMPLING
{
    NVVIOCOMPONENTSAMPLING_UNKNOWN,
    NVVIOCOMPONENTSAMPLING_4444,
    NVVIOCOMPONENTSAMPLING_4224,
    NVVIOCOMPONENTSAMPLING_444,
    NVVIOCOMPONENTSAMPLING_422
} NVVIOCOMPONENTSAMPLING;

typedef enum _NVVIOBITSPERCOMPONENT
{
    NVVIOBITSPERCOMPONENT_UNKNOWN,
    NVVIOBITSPERCOMPONENT_8,
    NVVIOBITSPERCOMPONENT_10,
    NVVIOBITSPERCOMPONENT_12,
} NVVIOBITSPERCOMPONENT;

typedef enum _NVVIOLINKID 
{
    NVVIOLINKID_UNKNOWN,
    NVVIOLINKID_A,
    NVVIOLINKID_B,
    NVVIOLINKID_C,
    NVVIOLINKID_D
} NVVIOLINKID;


typedef enum _NVVIOANCPARITYCOMPUTATION
{
    NVVIOANCPARITYCOMPUTATION_AUTO,
    NVVIOANCPARITYCOMPUTATION_ON,
    NVVIOANCPARITYCOMPUTATION_OFF
} NVVIOANCPARITYCOMPUTATION;



//! @}


//---------------------------------------------------------------------
// Structures
//---------------------------------------------------------------------

//! \addtogroup vidio
//! @{


//! Supports Serial Digital Interface (SDI) output
#define NVVIOCAPS_VIDOUT_SDI                0x00000001      

//! Supports Internal timing source
#define NVVIOCAPS_SYNC_INTERNAL             0x00000100      

//! Supports Genlock timing source
#define NVVIOCAPS_SYNC_GENLOCK              0x00000200      

//! Supports Serial Digital Interface (SDI) synchronization input
#define NVVIOCAPS_SYNCSRC_SDI               0x00001000      

//! Supports Composite synchronization input
#define NVVIOCAPS_SYNCSRC_COMP              0x00002000      

//! Supports Desktop transparent mode
#define NVVIOCAPS_OUTPUTMODE_DESKTOP        0x00010000      

//! Supports OpenGL application mode
#define NVVIOCAPS_OUTPUTMODE_OPENGL         0x00020000      

//! Supports Serial Digital Interface (SDI) input
#define NVVIOCAPS_VIDIN_SDI                 0x00100000  

//! Supports Packed ANC
#define NVVIOCAPS_PACKED_ANC_SUPPORTED      0x00200000     

//! Supports ANC audio blanking
#define NVVIOCAPS_AUDIO_BLANKING_SUPPORTED  0x00400000  

//! SDI-class interface: SDI output with two genlock inputs
#define NVVIOCLASS_SDI                      0x00000001      

//! Device capabilities
typedef struct _NVVIOCAPS
{
    NvU32             version;                              //!< Structure version
    NvAPI_String      adapterName;                          //!< Graphics adapter name
    NvU32             adapterClass;                         //!< Graphics adapter classes (NVVIOCLASS_SDI mask)
    NvU32             adapterCaps;                          //!< Graphics adapter capabilities (NVVIOCAPS_* mask)
    NvU32             dipSwitch;                            //!< On-board DIP switch settings bits
    NvU32             dipSwitchReserved;                    //!< On-board DIP switch settings reserved bits
    NvU32             boardID;                              //!< Board ID
    //! Driver version
    struct                                                  //
    {                                                      
        NvU32          majorVersion;                        //!< Major version. For GVI, majorVersion contains MajorVersion(HIWORD) And MinorVersion(LOWORD)
        NvU32          minorVersion;                        //!< Minor version. For GVI, minorVersion contains Revison(HIWORD) And Build(LOWORD)
    } driver;                                               //
    //! Firmware version 
    struct                                                  
    {                                                       
        NvU32          majorVersion;                        //!< Major version. In version 2, for both GVI and GVO, majorVersion contains MajorVersion(HIWORD) And MinorVersion(LOWORD)
        NvU32          minorVersion;                        //!< Minor version. In version 2, for both GVI and GVO, minorVersion contains Revison(HIWORD) And Build(LOWORD)
    } firmWare;                                             //
    NVVIOOWNERID      ownerId;                              //!< Unique identifier for owner of video output (NVVIOOWNERID_INVALID if free running)
    NVVIOOWNERTYPE    ownerType;                            //!< Owner type (OpenGL application or Desktop mode)
} NVVIOCAPS;

//! Macro for constructing the version field of NVVIOCAPS
#define NVVIOCAPS_VER1  MAKE_NVAPI_VERSION(NVVIOCAPS,1)
#define NVVIOCAPS_VER2  MAKE_NVAPI_VERSION(NVVIOCAPS,2)
#define NVVIOCAPS_VER   NVVIOCAPS_VER2

//! Input channel status
typedef struct _NVVIOCHANNELSTATUS
{
    NvU32                  smpte352;                         //!< 4-byte SMPTE 352 video payload identifier
    NVVIOSIGNALFORMAT      signalFormat;                     //!< Signal format
    NVVIOBITSPERCOMPONENT  bitsPerComponent;                 //!< Bits per component
    NVVIOCOMPONENTSAMPLING samplingFormat;                   //!< Sampling format
    NVVIOCOLORSPACE        colorSpace;                       //!< Color space
    NVVIOLINKID            linkID;                           //!< Link ID
} NVVIOCHANNELSTATUS;

//! Input device status
typedef struct _NVVIOINPUTSTATUS
{
    NVVIOCHANNELSTATUS     vidIn[NVAPI_MAX_VIO_JACKS][NVAPI_MAX_VIO_CHANNELS_PER_JACK];     //!< Video input status per channel within a jack
    NVVIOCAPTURESTATUS     captureStatus;                  //!< status of video capture
} NVVIOINPUTSTATUS;

//! Output device status
typedef struct _NVVIOOUTPUTSTATUS
{
    NVVIOINPUTOUTPUTSTATUS    vid1Out;                        //!< Video 1 output status
    NVVIOINPUTOUTPUTSTATUS    vid2Out;                        //!< Video 2 output status
    NVVIOSYNCSTATUS        sdiSyncIn;                      //!< SDI sync input status
    NVVIOSYNCSTATUS        compSyncIn;                     //!< Composite sync input status
    NvU32            syncEnable;                     //!< Sync enable (TRUE if using syncSource)
    NVVIOSYNCSOURCE        syncSource;                     //!< Sync source
    NVVIOSIGNALFORMAT        syncFormat;                     //!< Sync format
    NvU32            frameLockEnable;                //!< Framelock enable flag
    NvU32            outputVideoLocked;              //!< Output locked status
    NvU32            dataIntegrityCheckErrorCount;   //!< Data integrity check error count
    NvU32            dataIntegrityCheckEnabled;      //!< Data integrity check status enabled 
    NvU32            dataIntegrityCheckFailed;       //!< Data integrity check status failed 
    NvU32                       uSyncSourceLocked;              //!< genlocked to framelocked to ref signal
    NvU32                       uPowerOn;                       //!< TRUE: indicates there is sufficient power
} NVVIOOUTPUTSTATUS;

//! Video device status.
typedef struct _NVVIOSTATUS
{
    NvU32                 version;                        //!< Structure version
    NVVIOSTATUSTYPE       nvvioStatusType;                //!< Input or Output status
    union                                                   
    {
        NVVIOINPUTSTATUS  inStatus;                       //!<  Input device status
        NVVIOOUTPUTSTATUS outStatus;                      //!<  Output device status
    }vioStatus;      
} NVVIOSTATUS;

//! Macro for constructingthe version field of NVVIOSTATUS
#define NVVIOSTATUS_VER   MAKE_NVAPI_VERSION(NVVIOSTATUS,1)

//! Output region
typedef struct _NVVIOOUTPUTREGION
{
    NvU32              x;                                    //!< Horizontal origin in pixels
    NvU32              y;                                    //!< Vertical origin in pixels
    NvU32              width;                                //!< Width of region in pixels
    NvU32              height;                               //!< Height of region in pixels
} NVVIOOUTPUTREGION;

//! Gamma ramp (8-bit index)
typedef struct _NVVIOGAMMARAMP8
{
    NvU16              uRed[256];                            //!< Red channel gamma ramp (8-bit index, 16-bit values)
    NvU16              uGreen[256];                          //!< Green channel gamma ramp (8-bit index, 16-bit values)
    NvU16              uBlue[256];                           //!< Blue channel gamma ramp (8-bit index, 16-bit values)
} NVVIOGAMMARAMP8;

//! Gamma ramp (10-bit index)
typedef struct _NVVIOGAMMARAMP10
{
    NvU16              uRed[1024];                           //!< Red channel gamma ramp (10-bit index, 16-bit values)
    NvU16              uGreen[1024];                         //!< Green channel gamma ramp (10-bit index, 16-bit values)
    NvU16              uBlue[1024];                          //!< Blue channel gamma ramp (10-bit index, 16-bit values)
} NVVIOGAMMARAMP10;


//! Sync delay
typedef struct _NVVIOSYNCDELAY
{
    NvU32              version;                              //!< Structure version
    NvU32              horizontalDelay;                      //!< Horizontal delay in pixels
    NvU32              verticalDelay;                        //!< Vertical delay in lines
} NVVIOSYNCDELAY;

//! Macro for constructing the version field of NVVIOSYNCDELAY
#define NVVIOSYNCDELAY_VER   MAKE_NVAPI_VERSION(NVVIOSYNCDELAY,1)


//! Video mode information
typedef struct _NVVIOVIDEOMODE
{
    NvU32                horizontalPixels;                   //!< Horizontal resolution (in pixels)
    NvU32                verticalLines;                      //!< Vertical resolution for frame (in lines)
    float                fFrameRate;                         //!< Frame rate
    NVVIOINTERLACEMODE   interlaceMode;                      //!< Interlace mode 
    NVVIOVIDEOSTANDARD   videoStandard;                      //!< SMPTE standards format
    NVVIOVIDEOTYPE       videoType;                          //!< HD or SD signal classification
} NVVIOVIDEOMODE;

//! Signal format details
typedef struct _NVVIOSIGNALFORMATDETAIL
{   
    NVVIOSIGNALFORMAT    signalFormat;                       //!< Signal format enumerated value
    NVVIOVIDEOMODE       videoMode;                          //!< Video mode for signal format
}NVVIOSIGNALFORMATDETAIL;


//! R8:G8:B8
#define NVVIOBUFFERFORMAT_R8G8B8                  0x00000001
 
//! R8:G8:B8:Z24  
#define NVVIOBUFFERFORMAT_R8G8B8Z24               0x00000002
   
//! R8:G8:B8:A8
#define NVVIOBUFFERFORMAT_R8G8B8A8                0x00000004   

//! R8:G8:B8:A8:Z24       
#define NVVIOBUFFERFORMAT_R8G8B8A8Z24             0x00000008   

//! R16FP:G16FP:B16FP
#define NVVIOBUFFERFORMAT_R16FPG16FPB16FP         0x00000010   

//! R16FP:G16FP:B16FP:Z24
#define NVVIOBUFFERFORMAT_R16FPG16FPB16FPZ24      0x00000020   

//! R16FP:G16FP:B16FP:A16FP
#define NVVIOBUFFERFORMAT_R16FPG16FPB16FPA16FP    0x00000040   

//! R16FP:G16FP:B16FP:A16FP:Z24
#define NVVIOBUFFERFORMAT_R16FPG16FPB16FPA16FPZ24 0x00000080   



//! Data format details
typedef struct _NVVIODATAFORMATDETAIL
{
    NVVIODATAFORMAT   dataFormat;                              //!< Data format enumerated value
    NvU32             vioCaps;                                 //!< Data format capabilities (NVVIOCAPS_* mask)
}NVVIODATAFORMATDETAIL;

//! Colorspace conversion
typedef struct _NVVIOCOLORCONVERSION
{
    NvU32       version;                                    //!<  Structure version
    float       colorMatrix[3][3];                          //!<  Output[n] =
    float       colorOffset[3];                             //!<  Input[0] * colorMatrix[n][0] +
    float       colorScale[3];                              //!<  Input[1] * colorMatrix[n][1] +
                                                            //!<  Input[2] * colorMatrix[n][2] +
                                                            //!<  OutputRange * colorOffset[n]
                                                            //!<  where OutputRange is the standard magnitude of
                                                            //!<  Output[n][n] and colorMatrix and colorOffset 
                                                            //!<  values are within the range -1.0 to +1.0
    NvU32      compositeSafe;                               //!<  compositeSafe constrains luminance range when using composite output
} NVVIOCOLORCONVERSION;

//! macro for constructing the version field of _NVVIOCOLORCONVERSION.
#define NVVIOCOLORCONVERSION_VER   MAKE_NVAPI_VERSION(NVVIOCOLORCONVERSION,1)

//! Gamma correction
typedef struct _NVVIOGAMMACORRECTION
{
    NvU32            version;                               //!< Structure version
    NvU32            vioGammaCorrectionType;                //!< Gamma correction type (8-bit or 10-bit)
    //! Gamma correction:
    union                                                   
    {                                                       
        NVVIOGAMMARAMP8  gammaRamp8;                        //!< Gamma ramp (8-bit index, 16-bit values)
        NVVIOGAMMARAMP10 gammaRamp10;                       //!< Gamma ramp (10-bit index, 16-bit values)
    }gammaRamp;                                      
    float            fGammaValueR;            //!< Red Gamma value within gamma ranges. 0.5 - 6.0
    float            fGammaValueG;            //!< Green Gamma value within gamma ranges. 0.5 - 6.0
    float            fGammaValueB;            //!< Blue Gamma value within gamma ranges. 0.5 - 6.0
} NVVIOGAMMACORRECTION;

//! Macro for constructing thevesion field of _NVVIOGAMMACORRECTION
#define NVVIOGAMMACORRECTION_VER   MAKE_NVAPI_VERSION(NVVIOGAMMACORRECTION,1)

//! Maximum number of ranges per channel
#define MAX_NUM_COMPOSITE_RANGE      2                      


typedef struct _NVVIOCOMPOSITERANGE
{
    NvU32   uRange;
    NvU32   uEnabled;
    NvU32   uMin;
    NvU32   uMax;
} NVVIOCOMPOSITERANGE;



// Device configuration (fields masks indicating NVVIOCONFIG fields to use for NvAPI_VIO_GetConfig/NvAPI_VIO_SetConfig() )
//
#define NVVIOCONFIG_SIGNALFORMAT            0x00000001      //!< fields: signalFormat
#define NVVIOCONFIG_DATAFORMAT              0x00000002      //!< fields: dataFormat
#define NVVIOCONFIG_OUTPUTREGION            0x00000004      //!< fields: outputRegion
#define NVVIOCONFIG_OUTPUTAREA              0x00000008      //!< fields: outputArea
#define NVVIOCONFIG_COLORCONVERSION         0x00000010      //!< fields: colorConversion
#define NVVIOCONFIG_GAMMACORRECTION         0x00000020      //!< fields: gammaCorrection
#define NVVIOCONFIG_SYNCSOURCEENABLE        0x00000040      //!< fields: syncSource and syncEnable
#define NVVIOCONFIG_SYNCDELAY               0x00000080      //!< fields: syncDelay
#define NVVIOCONFIG_COMPOSITESYNCTYPE       0x00000100      //!< fields: compositeSyncType
#define NVVIOCONFIG_FRAMELOCKENABLE         0x00000200      //!< fields: EnableFramelock
#define NVVIOCONFIG_422FILTER               0x00000400      //!< fields: bEnable422Filter
#define NVVIOCONFIG_COMPOSITETERMINATE      0x00000800      //!< fields: bCompositeTerminate (Not supported on Quadro FX 4000 SDI)         
#define NVVIOCONFIG_DATAINTEGRITYCHECK      0x00001000      //!< fields: bEnableDataIntegrityCheck (Not supported on Quadro FX 4000 SDI)
#define NVVIOCONFIG_CSCOVERRIDE             0x00002000      //!< fields: colorConversion override
#define NVVIOCONFIG_FLIPQUEUELENGTH         0x00004000      //!< fields: flipqueuelength control
#define NVVIOCONFIG_ANCTIMECODEGENERATION   0x00008000      //!< fields: bEnableANCTimeCodeGeneration
#define NVVIOCONFIG_COMPOSITE               0x00010000      //!< fields: bEnableComposite
#define NVVIOCONFIG_ALPHAKEYCOMPOSITE       0x00020000      //!< fields: bEnableAlphaKeyComposite
#define NVVIOCONFIG_COMPOSITE_Y             0x00040000      //!< fields: compRange
#define NVVIOCONFIG_COMPOSITE_CR            0x00080000      //!< fields: compRange
#define NVVIOCONFIG_COMPOSITE_CB            0x00100000      //!< fields: compRange
#define NVVIOCONFIG_FULL_COLOR_RANGE        0x00200000      //!< fields: bEnableFullColorRange
#define NVVIOCONFIG_RGB_DATA                0x00400000      //!< fields: bEnableRGBData
#define NVVIOCONFIG_RESERVED_SDIOUTPUTENABLE         0x00800000      //!< fields: bEnableSDIOutput
#define NVVIOCONFIG_STREAMS                 0x01000000      //!< fields: streams
#define NVVIOCONFIG_ANC_PARITY_COMPUTATION  0x02000000      //!< fields: ancParityComputation
#define NVVIOCONFIG_ANC_AUDIO_REPEAT		0x04000000      //!< fields: enableAudioBlanking
 

// Don't forget to update NVVIOCONFIG_VALIDFIELDS in nvapi.spec when NVVIOCONFIG_ALLFIELDS changes.
#define NVVIOCONFIG_ALLFIELDS   ( NVVIOCONFIG_SIGNALFORMAT          | \
                                  NVVIOCONFIG_DATAFORMAT            | \
                                  NVVIOCONFIG_OUTPUTREGION          | \
                                  NVVIOCONFIG_OUTPUTAREA            | \
                                  NVVIOCONFIG_COLORCONVERSION       | \
                                  NVVIOCONFIG_GAMMACORRECTION       | \
                                  NVVIOCONFIG_SYNCSOURCEENABLE      | \
                                  NVVIOCONFIG_SYNCDELAY             | \
                                  NVVIOCONFIG_COMPOSITESYNCTYPE     | \
                                  NVVIOCONFIG_FRAMELOCKENABLE       | \
                                  NVVIOCONFIG_422FILTER             | \
                                  NVVIOCONFIG_COMPOSITETERMINATE    | \
                                  NVVIOCONFIG_DATAINTEGRITYCHECK    | \
                                  NVVIOCONFIG_CSCOVERRIDE           | \
                                  NVVIOCONFIG_FLIPQUEUELENGTH       | \
                                  NVVIOCONFIG_ANCTIMECODEGENERATION | \
                                  NVVIOCONFIG_COMPOSITE             | \
                                  NVVIOCONFIG_ALPHAKEYCOMPOSITE     | \
                                  NVVIOCONFIG_COMPOSITE_Y           | \
                                  NVVIOCONFIG_COMPOSITE_CR          | \
                                  NVVIOCONFIG_COMPOSITE_CB          | \
                                  NVVIOCONFIG_FULL_COLOR_RANGE      | \
                                  NVVIOCONFIG_RGB_DATA              | \
                                  NVVIOCONFIG_RESERVED_SDIOUTPUTENABLE | \
                                  NVVIOCONFIG_STREAMS               | \
                                  NVVIOCONFIG_ANC_PARITY_COMPUTATION | \
                                  NVVIOCONFIG_ANC_AUDIO_REPEAT )

#define NVVIOCONFIG_VALIDFIELDS  ( NVVIOCONFIG_SIGNALFORMAT          | \
                                   NVVIOCONFIG_DATAFORMAT            | \
                                   NVVIOCONFIG_OUTPUTREGION          | \
                                   NVVIOCONFIG_OUTPUTAREA            | \
                                   NVVIOCONFIG_COLORCONVERSION       | \
                                   NVVIOCONFIG_GAMMACORRECTION       | \
                                   NVVIOCONFIG_SYNCSOURCEENABLE      | \
                                   NVVIOCONFIG_SYNCDELAY             | \
                                   NVVIOCONFIG_COMPOSITESYNCTYPE     | \
                                   NVVIOCONFIG_FRAMELOCKENABLE       | \
                                   NVVIOCONFIG_RESERVED_SDIOUTPUTENABLE | \
                                   NVVIOCONFIG_422FILTER             | \
                                   NVVIOCONFIG_COMPOSITETERMINATE    | \
                                   NVVIOCONFIG_DATAINTEGRITYCHECK    | \
                                   NVVIOCONFIG_CSCOVERRIDE           | \
                                   NVVIOCONFIG_FLIPQUEUELENGTH       | \
                                   NVVIOCONFIG_ANCTIMECODEGENERATION | \
                                   NVVIOCONFIG_COMPOSITE             | \
                                   NVVIOCONFIG_ALPHAKEYCOMPOSITE     | \
                                   NVVIOCONFIG_COMPOSITE_Y           | \
                                   NVVIOCONFIG_COMPOSITE_CR          | \
                                   NVVIOCONFIG_COMPOSITE_CB          | \
                                   NVVIOCONFIG_FULL_COLOR_RANGE      | \
                                   NVVIOCONFIG_RGB_DATA              | \
                                   NVVIOCONFIG_RESERVED_SDIOUTPUTENABLE | \
                                   NVVIOCONFIG_STREAMS               | \
                                   NVVIOCONFIG_ANC_PARITY_COMPUTATION | \
                                   NVVIOCONFIG_ANC_AUDIO_REPEAT)

#define NVVIOCONFIG_DRIVERFIELDS ( NVVIOCONFIG_OUTPUTREGION          | \
                                   NVVIOCONFIG_OUTPUTAREA            | \
                                   NVVIOCONFIG_COLORCONVERSION       | \
                                   NVVIOCONFIG_FLIPQUEUELENGTH)

#define NVVIOCONFIG_GAMMAFIELDS  ( NVVIOCONFIG_GAMMACORRECTION       )

#define NVVIOCONFIG_RMCTRLFIELDS ( NVVIOCONFIG_SIGNALFORMAT          | \
                                   NVVIOCONFIG_DATAFORMAT            | \
                                   NVVIOCONFIG_SYNCSOURCEENABLE      | \
                                   NVVIOCONFIG_COMPOSITESYNCTYPE     | \
                                   NVVIOCONFIG_FRAMELOCKENABLE       | \
                                   NVVIOCONFIG_422FILTER             | \
                                   NVVIOCONFIG_COMPOSITETERMINATE    | \
                                   NVVIOCONFIG_DATAINTEGRITYCHECK    | \
                                   NVVIOCONFIG_COMPOSITE             | \
                                   NVVIOCONFIG_ALPHAKEYCOMPOSITE     | \
                                   NVVIOCONFIG_COMPOSITE_Y           | \
                                   NVVIOCONFIG_COMPOSITE_CR          | \
                                   NVVIOCONFIG_COMPOSITE_CB)

#define NVVIOCONFIG_RMSKEWFIELDS ( NVVIOCONFIG_SYNCDELAY             )

#define NVVIOCONFIG_ALLOWSDIRUNNING_FIELDS ( NVVIOCONFIG_DATAINTEGRITYCHECK     | \
                                             NVVIOCONFIG_SYNCDELAY              | \
                                             NVVIOCONFIG_CSCOVERRIDE            | \
                                             NVVIOCONFIG_ANCTIMECODEGENERATION  | \
                                             NVVIOCONFIG_COMPOSITE              | \
                                             NVVIOCONFIG_ALPHAKEYCOMPOSITE      | \
                                             NVVIOCONFIG_COMPOSITE_Y            | \
                                             NVVIOCONFIG_COMPOSITE_CR           | \
                                             NVVIOCONFIG_COMPOSITE_CB           | \
                                             NVVIOCONFIG_ANC_PARITY_COMPUTATION)

                                             
 #define NVVIOCONFIG_RMMODESET_FIELDS ( NVVIOCONFIG_SIGNALFORMAT         | \
                                        NVVIOCONFIG_DATAFORMAT           | \
                                        NVVIOCONFIG_SYNCSOURCEENABLE     | \
                                        NVVIOCONFIG_FRAMELOCKENABLE      | \
                                        NVVIOCONFIG_COMPOSITESYNCTYPE	 | \
                                        NVVIOCONFIG_ANC_AUDIO_REPEAT)                                            
                                             

//! Output device configuration 
// No members can be deleted from below structure. Only add new members at the 
// end of the structure.
typedef struct _NVVIOOUTPUTCONFIG_V1
{
    NVVIOSIGNALFORMAT    signalFormat;                         //!< Signal format for video output
    NVVIODATAFORMAT      dataFormat;                           //!< Data format for video output
    NVVIOOUTPUTREGION    outputRegion;                         //!< Region for video output (Desktop mode)
    NVVIOOUTPUTAREA      outputArea;                           //!< Usable resolution for video output (safe area)
    NVVIOCOLORCONVERSION colorConversion;                      //!< Color conversion.
    NVVIOGAMMACORRECTION gammaCorrection;
    NvU32                syncEnable;                           //!< Sync enable (TRUE to use syncSource)
    NVVIOSYNCSOURCE      syncSource;                           //!< Sync source
    NVVIOSYNCDELAY       syncDelay;                            //!< Sync delay
    NVVIOCOMPSYNCTYPE    compositeSyncType;                    //!< Composite sync type
    NvU32                frameLockEnable;                      //!< Flag indicating whether framelock was on/off
    NvU32                psfSignalFormat;                      //!< Indicates whether contained format is PSF Signal format
    NvU32                enable422Filter;                      //!< Enables/Disables 4:2:2 filter
    NvU32                compositeTerminate;                   //!< Composite termination
    NvU32                enableDataIntegrityCheck;             //!< Enable data integrity check: true - enable, false - disable
    NvU32                cscOverride;                          //!< Use provided CSC color matrix to overwrite 
    NvU32                flipQueueLength;                      //!< Number of buffers used for the internal flipqueue
    NvU32                enableANCTimeCodeGeneration;          //!< Enable SDI ANC time code generation
    NvU32                enableComposite;                      //!< Enable composite
    NvU32                enableAlphaKeyComposite;              //!< Enable Alpha key composite
    NVVIOCOMPOSITERANGE  compRange;                            //!< Composite ranges
    NvU8                 reservedData[256];                    //!< Inicates last stored SDI output state TRUE-ON / FALSE-OFF
    NvU32                enableFullColorRange;                 //!< Flag indicating Full Color Range
    NvU32                enableRGBData;                        //!< Indicates data is in RGB format
} NVVIOOUTPUTCONFIG_V1;

typedef struct _NVVIOOUTPUTCONFIG_V2
{
    NVVIOSIGNALFORMAT    signalFormat;                         //!< Signal format for video output
    NVVIODATAFORMAT      dataFormat;                           //!< Data format for video output
    NVVIOOUTPUTREGION    outputRegion;                         //!< Region for video output (Desktop mode)
    NVVIOOUTPUTAREA      outputArea;                           //!< Usable resolution for video output (safe area)
    NVVIOCOLORCONVERSION colorConversion;                      //!< Color conversion.
    NVVIOGAMMACORRECTION gammaCorrection;
    NvU32                syncEnable;                           //!< Sync enable (TRUE to use syncSource)
    NVVIOSYNCSOURCE      syncSource;                           //!< Sync source
    NVVIOSYNCDELAY       syncDelay;                            //!< Sync delay
    NVVIOCOMPSYNCTYPE    compositeSyncType;                    //!< Composite sync type
    NvU32                frameLockEnable;                      //!< Flag indicating whether framelock was on/off
    NvU32                psfSignalFormat;                      //!< Indicates whether contained format is PSF Signal format
    NvU32                enable422Filter;                      //!< Enables/Disables 4:2:2 filter
    NvU32                compositeTerminate;                   //!< Composite termination
    NvU32                enableDataIntegrityCheck;             //!< Enable data integrity check: true - enable, false - disable
    NvU32                cscOverride;                          //!< Use provided CSC color matrix to overwrite
    NvU32                flipQueueLength;                      //!< Number of buffers used for the internal flip queue
    NvU32                enableANCTimeCodeGeneration;          //!< Enable SDI ANC time code generation
    NvU32                enableComposite;                      //!< Enable composite
    NvU32                enableAlphaKeyComposite;              //!< Enable Alpha key composite
    NVVIOCOMPOSITERANGE  compRange;                            //!< Composite ranges
    NvU8                 reservedData[256];                    //!< Indicates last stored SDI output state TRUE-ON / FALSE-OFF
    NvU32                enableFullColorRange;                 //!< Flag indicating Full Color Range
    NvU32                enableRGBData;                        //!< Indicates data is in RGB format
    NVVIOANCPARITYCOMPUTATION ancParityComputation;            //!< Enable HW ANC parity bit computation (auto/on/off)
} NVVIOOUTPUTCONFIG_V2;

typedef struct _NVVIOOUTPUTCONFIG_V3
{
    NVVIOSIGNALFORMAT    signalFormat;                         //!< Signal format for video output
    NVVIODATAFORMAT      dataFormat;                           //!< Data format for video output
    NVVIOOUTPUTREGION    outputRegion;                         //!< Region for video output (Desktop mode)
    NVVIOOUTPUTAREA      outputArea;                           //!< Usable resolution for video output (safe area)
    NVVIOCOLORCONVERSION colorConversion;                      //!< Color conversion.
    NVVIOGAMMACORRECTION gammaCorrection;
    NvU32                syncEnable;                           //!< Sync enable (TRUE to use syncSource)
    NVVIOSYNCSOURCE      syncSource;                           //!< Sync source
    NVVIOSYNCDELAY       syncDelay;                            //!< Sync delay
    NVVIOCOMPSYNCTYPE    compositeSyncType;                    //!< Composite sync type
    NvU32                frameLockEnable;                      //!< Flag indicating whether framelock was on/off
    NvU32                psfSignalFormat;                      //!< Indicates whether contained format is PSF Signal format
    NvU32                enable422Filter;                      //!< Enables/Disables 4:2:2 filter
    NvU32                compositeTerminate;                   //!< Composite termination
    NvU32                enableDataIntegrityCheck;             //!< Enable data integrity check: true - enable, false - disable
    NvU32                cscOverride;                          //!< Use provided CSC color matrix to overwrite
    NvU32                flipQueueLength;                      //!< Number of buffers used for the internal flip queue
    NvU32                enableANCTimeCodeGeneration;          //!< Enable SDI ANC time code generation
    NvU32                enableComposite;                      //!< Enable composite
    NvU32                enableAlphaKeyComposite;              //!< Enable Alpha key composite
    NVVIOCOMPOSITERANGE  compRange;                            //!< Composite ranges
    NvU8                 reservedData[256];                    //!< Indicates last stored SDI output state TRUE-ON / FALSE-OFF
    NvU32                enableFullColorRange;                 //!< Flag indicating Full Color Range
    NvU32                enableRGBData;                        //!< Indicates data is in RGB format
    NVVIOANCPARITYCOMPUTATION ancParityComputation;            //!< Enable HW ANC parity bit computation (auto/on/off)
    NvU32				 enableAudioBlanking;				   //!< Enable HANC audio blanking on repeat frames
} NVVIOOUTPUTCONFIG_V3;

//! Stream configuration
typedef struct _NVVIOSTREAM
{
    NvU32                   bitsPerComponent;                     //!< Bits per component
    NVVIOCOMPONENTSAMPLING  sampling;                             //!< Sampling   
    NvU32                   expansionEnable;                      //!< Enable/disable 4:2:2->4:4:4 expansion
    NvU32                   numLinks;                             //!< Number of active links
    struct
    {
        NvU32               jack;                                 //!< This stream's link[i] will use the specified (0-based) channel within the
        NvU32               channel;                              //!< specified (0-based) jack
    } links[NVAPI_MAX_VIO_LINKS_PER_STREAM];
} NVVIOSTREAM;

//! Input device configuration
typedef struct _NVVIOINPUTCONFIG
{
    NvU32                numRawCaptureImages;                  //!< numRawCaptureImages is the number of frames to keep in the capture queue. 
                                                               //!< must be between NVAPI_GVI_MIN_RAW_CAPTURE_IMAGES and NVAPI_GVI_MAX_RAW_CAPTURE_IMAGES, 
    NVVIOSIGNALFORMAT    signalFormat;                         //!< Signal format.
                                                               //!< Please note that both numRawCaptureImages and signalFormat should be set together.
    NvU32                numStreams;                           //!< Number of active streams.
    NVVIOSTREAM          streams[NVAPI_MAX_VIO_STREAMS];       //!< Stream configurations
    NvU32                bTestMode;                            //!< This attribute controls the GVI test mode.
                                                               //!< Possible values 0/1. When testmode enabled, the
                                                               //!< GVI device will generate fake data as quickly as possible.
} NVVIOINPUTCONFIG;

typedef struct _NVVIOCONFIG_V1
{
    NvU32                version;                              //!< Structure version
    NvU32                fields;                               //!< Caller sets to NVVIOCONFIG_* mask for fields to use
    NVVIOCONFIGTYPE      nvvioConfigType;                      //!< Input or Output configuration
    union                                                   
    {
        NVVIOINPUTCONFIG  inConfig;                            //!<  Input device configuration
        NVVIOOUTPUTCONFIG_V1 outConfig;                           //!<  Output device configuration
    }vioConfig; 
} NVVIOCONFIG_V1;


typedef struct _NVVIOCONFIG_V2
{
    NvU32                version;                              //!< Structure version
    NvU32                fields;                               //!< Caller sets to NVVIOCONFIG_* mask for fields to use
    NVVIOCONFIGTYPE      nvvioConfigType;                      //!< Input or Output configuration
    union
    {
        NVVIOINPUTCONFIG     inConfig;                         //!< Input device configuration
        NVVIOOUTPUTCONFIG_V2 outConfig;                        //!< Output device configuration
    }vioConfig;
} NVVIOCONFIG_V2;

typedef struct _NVVIOCONFIG_V3
{
    NvU32                version;                              //!< Structure version
    NvU32                fields;                               //!< Caller sets to NVVIOCONFIG_* mask for fields to use
    NVVIOCONFIGTYPE      nvvioConfigType;                      //!< Input or Output configuration
    union
    {
        NVVIOINPUTCONFIG     inConfig;                         //!< Input device configuration
        NVVIOOUTPUTCONFIG_V3 outConfig;                        //!< Output device configuration
    }vioConfig;
} NVVIOCONFIG_V3;
typedef NVVIOOUTPUTCONFIG_V3 NVVIOOUTPUTCONFIG;
typedef NVVIOCONFIG_V3 NVVIOCONFIG;

#define NVVIOCONFIG_VER1  MAKE_NVAPI_VERSION(NVVIOCONFIG_V1,1)
#define NVVIOCONFIG_VER2  MAKE_NVAPI_VERSION(NVVIOCONFIG_V2,2)
#define NVVIOCONFIG_VER3  MAKE_NVAPI_VERSION(NVVIOCONFIG_V3,3)
#define NVVIOCONFIG_VER   NVVIOCONFIG_VER3


typedef struct
{
    NvPhysicalGpuHandle                    hPhysicalGpu;                    //!< Handle to Physical GPU (This could be NULL for GVI device if its not binded)
    NvVioHandle                         hVioHandle;                     //!<handle to SDI Input/Output device
    NvU32                               vioId;                          //!<device Id of SDI Input/Output device
    NvU32                               outputId;            //!<deviceMask of the SDI display connected to GVO device. 
                                                                        //!<outputId will be 0 for GVI device.
} NVVIOTOPOLOGYTARGET;                                                    

typedef struct _NV_VIO_TOPOLOGY
{
    NvU32                       version;
    NvU32                       vioTotalDeviceCount;                    //!<How many video I/O targets are valid
    NVVIOTOPOLOGYTARGET         vioTarget[NVAPI_MAX_VIO_DEVICES];       //!<Array of video I/O targets
}NV_VIO_TOPOLOGY, NVVIOTOPOLOGY;


//! Macro for constructing the version field of NV_VIO_TOPOLOGY
#define NV_VIO_TOPOLOGY_VER  MAKE_NVAPI_VERSION(NV_VIO_TOPOLOGY,1)

//! Macro for constructing the version field of NVVIOTOPOLOGY
#define NVVIOTOPOLOGY_VER    MAKE_NVAPI_VERSION(NVVIOTOPOLOGY,1)



//! @} 



//! \addtogroup vidio
//! @{
///////////////////////////////////////////////////////////////////////////////
//!   
//!   Function:    NvAPI_VIO_GetCapabilities
//!  
//!   Description: This API determine the graphics adapter video I/O capabilities.
//!  
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!  
//! \param [in]  NvVioHandle   The caller provides the SDI device handle as input.
//! \param [out] pAdapterCaps  Pointer to receive capabilities
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  NVVIOCAPS struct version used by the app is not compatible
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_GetCapabilities(NvVioHandle     hVioHandle,
                                          NVVIOCAPS       *pAdapterCaps);


////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_Open
//!  
//!   Description: This API opens the graphics adapter for video I/O operations
//!                using the OpenGL application interface.  Read operations
//!                are permitted in this mode by multiple clients, but Write 
//!                operations are application exclusive.
//!  
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!  
//! \param [in]  NvVioHandle      The caller provides the SDI output device handle as input.
//! \param [in]  vioClass         Class interface (NVVIOCLASS_* value)
//! \param [in]  ownerType        Specify NVVIOOWNERTYPE_APPLICATION or NVVIOOWNERTYPE_DESKTOP.
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_Open(NvVioHandle       hVioHandle,
                               NvU32             vioClass,
                               NVVIOOWNERTYPE    ownerType);

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_Close
//!  
//!   Description: This API closes the graphics adapter for graphics-to-video operations
//!                using the OpenGL application interface.  Closing an 
//!                OpenGL handle releases the device.
//!  
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!  
//! \param [in]  NvVioHandle   The caller provides the SDI output device handle as input.
//! \param [in]  bRelease      boolean value to either keep or release ownership
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_Close(NvVioHandle       hVioHandle,
                                NvU32             bRelease);
///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_Status
//!  
//!   Description: This API gets the Video I/O LED status.
//!  
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!  
//! \param [in]  NvVioHandle   The caller provides the SDI device handle as input.
//! \param [out] pStatus       Return pointer to NVVIOSTATUS
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Invalid structure version
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_Status(NvVioHandle     hVioHandle, 
                                 NVVIOSTATUS     *pStatus);

////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SyncFormatDetect
//!  
//!   Description: This API detects the Video I/O incoming sync video format.
//!  
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!  
//! \param [in]  NvVioHandle  The caller provides the SDI device handle as input.
//! \param [out] pWait        Pointer to receive how many milliseconds will lapse 
//!                           before VIOStatus returns the detected syncFormat.
//!  
//! \retval ::NVAPI_OK                          Success
//! \retval ::NVAPI_API_NOT_INTIALIZED          NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT            Arguments passed to API are not valid
//! \retval ::NVAPI_NOT_SUPPORTED               Video I/O not supported
//! \retval ::NVAPI_ERROR                       NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_SyncFormatDetect(NvVioHandle hVioHandle,
                                           NvU32       *pWait);
///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_GetConfig
//!  
//!   Description: This API gets the graphics-to-video configuration.
//!  
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!  
//! \param [in]  NvVioHandle   The caller provides the SDI device handle as input.
//! \param [out] pConfig       Pointer to the graphics-to-video configuration
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Invalid structure version
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_GetConfig(NvVioHandle hVioHandle, NVVIOCONFIG* pConfig); 

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SetConfig
//!  
//!   Description: This API sets the graphics-to-video configuration.
//!  
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!  
//! \param [in]  NvVioHandle      The caller provides the SDI device handle as input.
//! \param [in]  pConfig          Pointer to Graphics-to-Video configuration
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Stucture version invalid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_SetConfig(NvVioHandle            hVioHandle,
                                    const NVVIOCONFIG      *pConfig);

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SetCSC
//!  
//!   Description: This API sets the colorspace conversion parameters.
//!  
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_SetConfig.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [in]  NvVioHandle      The caller provides the SDI device handle as input.
//! \param [in]  pCSC             Pointer to CSC parameters
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Stucture version invalid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
////////////////////////////////////////////////////////////////////////////////----
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_SetConfig.")
NVAPI_INTERFACE NvAPI_VIO_SetCSC(NvVioHandle           hVioHandle,
                                 NVVIOCOLORCONVERSION  *pCSC);
////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_GetCSC
//! 
//!   Description: This API gets the colorspace conversion parameters.
//!
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_GetConfig.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [in]  NvVioHandle      The caller provides the SDI device handle as input.
//! \param [out] pCSC             Pointer to CSC parameters
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Stucture version invalid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
////////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_GetConfig.")
NVAPI_INTERFACE NvAPI_VIO_GetCSC(NvVioHandle           hVioHandle,
                                 NVVIOCOLORCONVERSION  *pCSC);
///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SetGamma
//! 
//!   Description: This API sets the gamma conversion parameters.
//! 
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_SetConfig.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [in]  NvVioHandle       The caller provides the SDI device handle as input.
//! \param [in]  pGamma            Pointer to gamma parameters
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Stucture version invalid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_SetConfig.")
NVAPI_INTERFACE NvAPI_VIO_SetGamma(NvVioHandle           hVioHandle,
                                   NVVIOGAMMACORRECTION  *pGamma);

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_GetGamma
//! 
//!   Description: This API gets the gamma conversion parameters.
//! 
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_GetConfig.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [in]  NvVioHandle      The caller provides the SDI device handle as input.
//! \param [out] pGamma           Pointer to gamma parameters
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Stucture version invalid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_GetConfig.")
NVAPI_INTERFACE NvAPI_VIO_GetGamma(NvVioHandle           hVioHandle,
                                   NVVIOGAMMACORRECTION* pGamma);
////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SetSyncDelay
//! 
//!   Description: This API sets the sync delay parameters.
//! 
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_SetConfig.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//! 
//! \param [in]  NvVioHandle   The caller provides the SDI device handle as input.
//! \param [in]  pSyncDelay    Pointer to sync delay parameters
//!  
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Stucture version invalid
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_SetConfig.")
NVAPI_INTERFACE NvAPI_VIO_SetSyncDelay(NvVioHandle            hVioHandle,
                                       const NVVIOSYNCDELAY   *pSyncDelay);

////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_GetSyncDelay
//! 
//!   Description: This API gets the sync delay parameters.
//! 
//! \deprecated  Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_GetConfig.
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//! 
//! \param [in]  NvVioHandle      The caller provides the SDI device handle as input.
//! \param [out] pSyncDelay       Pointer to sync delay parameters
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Stucture version invalid
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
__nvapi_deprecated_function("Do not use this function - it is deprecated in release 290. Instead, use NvAPI_VIO_GetConfig.")
NVAPI_INTERFACE NvAPI_VIO_GetSyncDelay(NvVioHandle      hVioHandle,
                                       NVVIOSYNCDELAY   *pSyncDelay);

typedef enum _NVVIOPCILINKRATE
{
    NVVIOPCILINKRATE_UNKNOWN          = 0,
    NVVIOPCILINKRATE_GEN1             = 1,                    //<! 2.5 Gbps.
    NVVIOPCILINKRATE_GEN2             = 2,                    //<! 5 Gbps.
    NVVIOPCILINKRATE_GEN3             = 3,                    //<! 8 Gbps.
}NVVIOPCILINKRATE;

typedef enum _NVVIOPCILINKWIDTH
{
    NVVIOPCILINKWIDTH_UNKNOWN         = 0,
    NVVIOPCILINKWIDTH_x1              = 1,
    NVVIOPCILINKWIDTH_x2              = 2,
    NVVIOPCILINKWIDTH_x4              = 4,
    NVVIOPCILINKWIDTH_x8              = 8,
    NVVIOPCILINKWIDTH_x16            = 16,
}NVVIOPCILINKWIDTH;

typedef struct _NVVIOPCIINFO
{
    NvU32                     version;                            //!< Structure version
    
    NvU32                   pciDeviceId;                        //!< specifies the internal PCI device identifier for the GVI.
    NvU32                   pciSubSystemId;                        //!< specifies the internal PCI subsystem identifier for the GVI.
    NvU32                   pciRevisionId;                        //!< specifies the internal PCI device-specific revision identifier for the GVI.
    NvU32                   pciDomain;                            //!< specifies the PCI domain of the GVI device.
    NvU32                   pciBus;                                //!< specifies the PCI bus number of the GVI device.
    NvU32                   pciSlot;                            //!< specifies the PCI slot number of the GVI device.
    NVVIOPCILINKWIDTH       pciLinkWidth;                        //!< specifies the the negotiated PCIE link width.
    NVVIOPCILINKRATE           pciLinkRate;                        //!< specifies the the negotiated PCIE link rate.
} NVVIOPCIINFO_V1;

typedef NVVIOPCIINFO_V1                                         NVVIOPCIINFO;
#define NVVIOPCIINFO_VER1                                          MAKE_NVAPI_VERSION(NVVIOPCIINFO_V1,1)
#define NVVIOPCIINFO_VER                                        NVVIOPCIINFO_VER1

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_VIO_GetPCIInfo()
//
// DESCRIPTION:   This API gets PCI information of the attached SDI(input) capture card.
//
// PARAMETERS:      hVioHandle    (IN)    - Handle to SDI capture card.
//                  pVioPCIInfo    (OUT)    - PCI information of the attached SDI capture card.
//
//! SUPPORTED OS:  Windows XP and higher
//!
//
// RETURN STATUS: This API can return any of the error codes enumerated in #NvAPI_Status. If there are return error codes with 
//                specific meaning for this API, they are listed below.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_GetPCIInfo(_In_ NvVioHandle hVioHandle, 
                                            _Inout_ NVVIOPCIINFO* pVioPCIInfo);

////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_IsRunning
//! 
//!   Description: This API determines if Video I/O is running.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//! 
//! \param [in]  NvVioHandle[IN]          The caller provides the SDI device handle as input.
//! 
//! \retval ::NVAPI_DRIVER_RUNNING        Video I/O running
//! \retval ::NVAPI_DRIVER_NOTRUNNING     Video I/O not running
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_IsRunning(NvVioHandle   hVioHandle);

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_Start
//! 
//!   Description: This API starts Video I/O.
//!              This API should be called for NVVIOOWNERTYPE_DESKTOP only and will not work for OGL applications.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [in]  NvVioHandle[IN]     The caller provides the SDI device handle as input.
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_Start(NvVioHandle     hVioHandle);

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_Stop
//! 
//!   Description: This API stops Video I/O.
//!              This API should be called for NVVIOOWNERTYPE_DESKTOP only and will not work for OGL applications.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//! 
//! \param [in]  NvVioHandle[IN]     The caller provides the SDI device handle as input.
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
//! \retval ::NVAPI_DEVICE_BUSY                  Access denied for requested access
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_Stop(NvVioHandle     hVioHandle);


///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_IsFrameLockModeCompatible
//! 
//!   Description: This API checks whether modes are compatible in frame lock mode.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//! 
//! \param [in]   NvVioHandle          The caller provides the SDI device handle as input.
//! \param [in]   srcEnumIndex         Source Enumeration index
//! \param [in]   destEnumIndex        Destination Enumeration index
//! \param [out]  pbCompatible         Pointer to receive compatibility
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_NOT_SUPPORTED                Video I/O not supported
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_IsFrameLockModeCompatible(NvVioHandle              hVioHandle,
                                                    NvU32                    srcEnumIndex,
                                                    NvU32                    destEnumIndex,
                                                    NvU32*                   pbCompatible);


///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_EnumDevices
//! 
//!   Description: This API enumerate all VIO devices connected to the system.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [out]  NvVioHandle                  User passes the pointer of NvVioHandle[] array to get handles to
//!                                            all the connected video I/O devices.
//! \param [out]  vioDeviceCount               User gets total number of VIO devices connected to the system.
//! 
//! \retval ::NVAPI_OK                         Success
//! \retval ::NVAPI_API_NOT_INTIALIZED         NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT           Arguments passed to API are not valid
//! \retval ::NVAPI_ERROR                      NVAPI Random errors
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND    No SDI Device found
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_EnumDevices(NvVioHandle       hVioHandle[NVAPI_MAX_VIO_DEVICES],
                                      NvU32             *vioDeviceCount);
                                                                                          

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_QueryTopology
//! 
//!   Description: This API queries the valid SDI topologies.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [out] pNvVIOTopology     User passes the pointer to NVVIOTOPOLOGY to fetch all valid SDI topologies.
//! 
//! \retval ::NVAPI_OK                           Success
//! \retval ::NVAPI_API_NOT_INTIALIZED           NVAPI Not Initialized
//! \retval ::NVAPI_INVALID_ARGUMENT             Arguments passed to API are not valid
//! \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION  Invalid structure version
//! \retval ::NVAPI_ERROR                        NVAPI Random errors
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_QueryTopology(NV_VIO_TOPOLOGY   *pNvVIOTopology);


///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_EnumSignalFormats
//! 
//!   Description: This API enumerates signal formats supported by Video I/O.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//!
//! \param [in]   NvVioHandle          The caller provides the SDI device handle as input.
//! \param [in]   enumIndex            Enumeration index
//! \param [out]  pSignalFormatDetail  Pointer to receive detail or NULL
//! 
//! \retval ::NVAPI_OK                  Success
//! \retval ::NVAPI_API_NOT_INTIALIZED  NVAPI not initialized
//! \retval ::NVAPI_INVALID_ARGUMENT    Invalid argument passed
//! \retval ::NVAPI_END_ENUMERATION     No more signal formats to enumerate
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_EnumSignalFormats(NvVioHandle              hVioHandle,
                                            NvU32                    enumIndex,
                                            NVVIOSIGNALFORMATDETAIL  *pSignalFormatDetail);

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_EnumDataFormats
//! 
//!   Description: This API enumerates data formats supported by Video I/O.
//! 
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 190
//! 
//! \param [in]  NvVioHandle         The caller provides the SDI device handle as input.
//! \param [in]  enumIndex           Enumeration index
//! \param [out] pDataFormatDetail   Pointer to receive detail or NULL
//! 
//! \retval ::NVAPI_OK                Success
//! \retval ::NVAPI_END_ENUMERATION   No more data formats to enumerate
//! \retval ::NVAPI_NOT_SUPPORTED     Unsupported NVVIODATAFORMAT_ enumeration
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_VIO_EnumDataFormats(NvVioHandle            hVioHandle,
                                          NvU32                  enumIndex,
                                          NVVIODATAFORMATDETAIL  *pDataFormatDetail);
                                                                                      

//! @}




///////////////////////////////////////////////////////////////////////////////////
//  CAMERA TEST API
//  These APIs allows test apps to perform low level camera tests

//! \addtogroup vidio
//! @{
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Stereo_CreateConfigurationProfileRegistryKey
//
//! \fn NvAPI_Stereo_CreateConfigurationProfileRegistryKey(NV_STEREO_REGISTRY_PROFILE_TYPE registryProfileType)
//!
//! DESCRIPTION:   Creates new configuration registry key for current application.
//!
//!                If there is no configuration profile prior to the function call,
//!                this API tries to create a new configuration profile registry key
//!                for a given application and fill it with the default values.
//!                If an application already has a configuration profile registry key, the API does nothing.
//!                The name of the key is automatically set to the name of the executable that calls this function.
//!                Because of this, the executable should have a distinct and unique name.
//!                If the application is using only one version of DirectX, then the default profile type will be appropriate.
//!                If the application is using more than one version of DirectX from the same executable,
//!                it should use the appropriate profile type for each configuration profile.
//!
//! HOW TO USE:    When there is a need for an application to have default stereo parameter values,
//!                use this function to create a key to store the values.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 180
//!
//! \param [in]    registryProfileType  Type of profile the application wants to create. It should be one of the symbolic constants defined in
//!                                     ::NV_STEREO_REGISTRY_PROFILE_TYPE. Any other value will cause function to do nothing and return
//!                                     ::NV_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED.
//!
//! \retval ::NVAPI_OK                                           Key exists in the registry.
//! \retval ::NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED   This profile type is not supported.
//! \retval ::NVAPI_STEREO_REGISTRY_ACCESS_FAILED                Access to registry failed.
//! \retval ::NVAPI_API_NOT_INTIALIZED           
//! \retval ::NVAPI_STEREO_NOT_INITIALIZED                       Stereo part of NVAPI not initialized.
//! \retval ::NVAPI_ERROR 
//!
///////////////////////////////////////////////////////////////////////////////


//! \ingroup stereoapi
//! Used in NvAPI_Stereo_CreateConfigurationProfileRegistryKey() 
typedef enum _NV_StereoRegistryProfileType
{
    NVAPI_STEREO_DEFAULT_REGISTRY_PROFILE, //!< Default registry configuration profile.
    NVAPI_STEREO_DX9_REGISTRY_PROFILE,     //!< Separate registry configuration profile for a DirectX 9 executable.
    NVAPI_STEREO_DX10_REGISTRY_PROFILE     //!< Separate registry configuration profile for a DirectX 10 executable.
} NV_STEREO_REGISTRY_PROFILE_TYPE;


//! \ingroup stereoapi
NVAPI_INTERFACE NvAPI_Stereo_CreateConfigurationProfileRegistryKey(NV_STEREO_REGISTRY_PROFILE_TYPE registryProfileType);




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Stereo_DeleteConfigurationProfileRegistryKey
//
//! DESCRIPTION:   Removes configuration registry key for current application.
//!
//!                If an application already has a configuration profile prior to this function call,
//!                the function attempts to remove the application's configuration profile registry key from the registry.
//!                If there is no configuration profile registry key prior to the function call,
//!                the function does nothing and does not report an error.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 180
//!
//! \param [in]   registryProfileType   Type of profile that the application wants to delete. This should be one of the symbolic 
//!                                     constants defined in ::NV_STEREO_REGISTRY_PROFILE_TYPE. Any other value will cause the function 
//!                                     to do nothing and return ::NV_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED.
//!
//! \retval ::NVAPI_OK                                           Key does not exist in the registry any more.
//! \retval ::NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED   This profile type is not supported.
//! \retval ::NVAPI_STEREO_REGISTRY_ACCESS_FAILED                Access to registry failed.
//! \retval ::NVAPI_API_NOT_INTIALIZED                           NVAPI is not initialized.
//! \retval ::NVAPI_STEREO_NOT_INITIALIZED                       Stereo part of NVAPI is not initialized.
//! \retval ::NVAPI_ERROR 
//!
//! \ingroup stereoapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Stereo_DeleteConfigurationProfileRegistryKey(NV_STEREO_REGISTRY_PROFILE_TYPE registryProfileType);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Stereo_SetConfigurationProfileValue
//
//! \fn NvAPI_Stereo_SetConfigurationProfileValue(NV_STEREO_REGISTRY_PROFILE_TYPE registryProfileType, NV_STEREO_REGISTRY_ID valueRegistryID, void *pValue)
//!
//! DESCRIPTION:   This API sets the given parameter value under the application's registry key.
//!
//!                If the value does not exist under the application's registry key,
//!                the value will be created under the key.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 180
//!
//! \param [in]     registryProfileType  The type of profile the application wants to access. It should be one of the 
//!                                      symbolic constants defined in ::NV_STEREO_REGISTRY_PROFILE_TYPE. Any other value 
//!                                      will cause function to do nothing and return ::NV_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED.
//! \param [in]     valueRegistryID      ID of the value that is being set. It should be one of the symbolic constants defined in
//!                                      ::NV_STEREO_REGISTRY_PROFILE_TYPE. Any other value will cause function to do nothing
//!                                      and return ::NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED.
//! \param [in]     pValue               Address of the value that is being set. It should be either address of a ULONG or of a float,
//!                                      dependent on the type of the stereo parameter whose value is being set. The API will then cast that
//!                                      address to ULONG* and write whatever is in those 4 bytes as a ULONG to the registry.
//!
//! \retval ::NVAPI_OK                                           Value is written to registry.
//! \retval ::NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED   This profile type is not supported.
//! \retval ::NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED          This value is not supported.
//! \retval ::NVAPI_STEREO_REGISTRY_ACCESS_FAILED                Access to registry failed.
//! \retval ::NVAPI_API_NOT_INTIALIZED                           NVAPI is not initialized.
//! \retval ::NVAPI_STEREO_NOT_INITIALIZED                       Stereo part of NVAPI is not initialized.
//! \retval ::NVAPI_ERROR                                        Something is wrong (generic error).
//
///////////////////////////////////////////////////////////////////////////////


//! \ingroup stereoapi
//! Used in NvAPI_Stereo_SetConfigurationProfileValue()
typedef enum _NV_StereoRegistryID
{
    NVAPI_CONVERGENCE_ID,         //!< Symbolic constant for convergence registry ID.
    NVAPI_FRUSTUM_ADJUST_MODE_ID, //!< Symbolic constant for frustum adjust mode registry ID.
} NV_STEREO_REGISTRY_ID;


//! \ingroup stereoapi
NVAPI_INTERFACE NvAPI_Stereo_SetConfigurationProfileValue(NV_STEREO_REGISTRY_PROFILE_TYPE registryProfileType, NV_STEREO_REGISTRY_ID valueRegistryID, void *pValue);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Stereo_DeleteConfigurationProfileValue
//
//! DESCRIPTION:   This API removes the given value from the application's configuration profile registry key.
//!                If there is no such value, the function does nothing and does not report an error.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 180
//!
//! \param [in]     registryProfileType   The type of profile the application wants to access. It should be one of the 
//!                                       symbolic constants defined in ::NV_STEREO_REGISTRY_PROFILE_TYPE. Any other value will 
//!                                       cause function to do nothing and return ::NV_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED.
//! \param [in]     valueRegistryID       ID of the value that is being deleted. It should be one of the symbolic constants defined in
//!                                       ::NV_STEREO_REGISTRY_PROFILE_TYPE. Any other value will cause function to do nothing and return
//!                                       ::NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED.
//!
//! \retval ::NVAPI_OK                                           Value does not exist in registry any more.
//! \retval ::NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED   This profile type is not supported.
//! \retval ::NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED          This value is not supported.
//! \retval ::NVAPI_STEREO_REGISTRY_ACCESS_FAILED                Access to registry failed.
//! \retval ::NVAPI_API_NOT_INTIALIZED 
//! \retval ::NVAPI_STEREO_NOT_INITIALIZED                       Stereo part of NVAPI not initialized.
//! \retval ::NVAPI_ERROR 
//!
//! \ingroup stereoapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Stereo_DeleteConfigurationProfileValue(NV_STEREO_REGISTRY_PROFILE_TYPE registryProfileType, NV_STEREO_REGISTRY_ID valueRegistryID);






//! \addtogroup stereoapi
//! @{

typedef struct _NVAPI_STEREO_CAPS
{
    NvU32 version;
    NvU32 supportsWindowedModeOff        : 1;
    NvU32 supportsWindowedModeAutomatic  : 1;
    NvU32 supportsWindowedModePersistent : 1;
    NvU32 reserved                       : 29;  // must be 0
    NvU32 reserved2[3];                         // must be 0
} NVAPI_STEREO_CAPS_V1;

#define NVAPI_STEREO_CAPS_VER1  MAKE_NVAPI_VERSION(NVAPI_STEREO_CAPS,1)
#define NVAPI_STEREO_CAPS_VER   NVAPI_STEREO_CAPS_VER1

typedef NVAPI_STEREO_CAPS_V1    NVAPI_STEREO_CAPS;

//! @}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Stereo_GetStereoSupport
//
//! DESCRIPTION:  This API checks what kind of stereo support is currently supported on a particular display.
//!               If the the display is prohibited from showing stereo (e.g. secondary in a multi-mon setup), we will 
//!               return 0 for all stereo modes (full screen exclusive, automatic windowed, persistent windowed).
//!               Otherwise, we will check which stereo mode is supported. On 120Hz display, this will be what
//!               the user chooses in control panel. On HDMI 1.4 display, persistent windowed mode is always assumed to be
//!               supported. Note that this function does not check if the CURRENT RESOLUTION/REFRESH RATE can support
//!               stereo. For HDMI 1.4, it is the app's responsibility to change the resolution/refresh rate to one that is
//!               3D compatible. For 120Hz, the driver will ALWAYS force 120Hz anyway.
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! \since Release: 304
//!
//! \param [in]     hMonitor handle to monitor that app is going to run on
//! \param [out]    pCaps    Address where the result of the inquiry will be placed.
//!                          *pCaps is defined in NVAPI_STEREO_CAPS.
//! \return       This API can return any of the following error codes enumerated in #NvAPI_Status
//! \retval ::NVAPI_OK
//!
//! \ingroup stereoapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Stereo_GetStereoSupport(_In_ NvMonitorHandle hMonitor, _Out_ NVAPI_STEREO_CAPS *pCaps);


//! \ingroup stereoapi
#define NVAPI_STEREO_QUADBUFFERED_API_VERSION   0x2

//! \ingroup stereoapi
 typedef enum _NV_StereoSwapChainMode
 {
   NVAPI_STEREO_SWAPCHAIN_DEFAULT = 0,
   NVAPI_STEREO_SWAPCHAIN_STEREO = 1,
   NVAPI_STEREO_SWAPCHAIN_MONO = 2,
 } NV_STEREO_SWAPCHAIN_MODE;

//! \addtogroup drsapi
//! @{

// GPU Profile APIs

NV_DECLARE_HANDLE(NvDRSSessionHandle);
NV_DECLARE_HANDLE(NvDRSProfileHandle);

#define NVAPI_DRS_GLOBAL_PROFILE                             ((NvDRSProfileHandle) -1)
#define NVAPI_SETTING_MAX_VALUES                             100

typedef enum _NVDRS_SETTING_TYPE
{
     NVDRS_DWORD_TYPE,
     NVDRS_BINARY_TYPE,
     NVDRS_STRING_TYPE,
     NVDRS_WSTRING_TYPE
} NVDRS_SETTING_TYPE;

typedef enum _NVDRS_SETTING_LOCATION
{
     NVDRS_CURRENT_PROFILE_LOCATION,
     NVDRS_GLOBAL_PROFILE_LOCATION,
     NVDRS_BASE_PROFILE_LOCATION,
     NVDRS_DEFAULT_PROFILE_LOCATION
} NVDRS_SETTING_LOCATION;

typedef struct _NVDRS_GPU_SUPPORT
{
    NvU32 geforce    :  1;
    NvU32 quadro     :  1;
    NvU32 nvs        :  1;
    NvU32 reserved4  :  1;
    NvU32 reserved5  :  1;
    NvU32 reserved6  :  1;
    NvU32 reserved7  :  1;
    NvU32 reserved8  :  1;
    NvU32 reserved9  :  1;
    NvU32 reserved10 :  1;
    NvU32 reserved11 :  1;
    NvU32 reserved12 :  1;
    NvU32 reserved13 :  1;
    NvU32 reserved14 :  1;
    NvU32 reserved15 :  1;
    NvU32 reserved16 :  1;
    NvU32 reserved17 :  1;
    NvU32 reserved18 :  1;
    NvU32 reserved19 :  1;
    NvU32 reserved20 :  1;
    NvU32 reserved21 :  1;
    NvU32 reserved22 :  1;
    NvU32 reserved23 :  1;
    NvU32 reserved24 :  1;
    NvU32 reserved25 :  1;
    NvU32 reserved26 :  1;
    NvU32 reserved27 :  1;
    NvU32 reserved28 :  1;
    NvU32 reserved29 :  1;
    NvU32 reserved30 :  1;
    NvU32 reserved31 :  1;
    NvU32 reserved32 :  1;
} NVDRS_GPU_SUPPORT;

//! Enum to decide on the datatype of setting value.
typedef struct _NVDRS_BINARY_SETTING 
{
     NvU32                valueLength;               //!< valueLength should always be in number of bytes.
     NvU8                 valueData[NVAPI_BINARY_DATA_MAX];
} NVDRS_BINARY_SETTING;

typedef struct _NVDRS_SETTING_VALUES
{
     NvU32                      version;                //!< Structure Version
     NvU32                      numSettingValues;       //!< Total number of values available in a setting.
     NVDRS_SETTING_TYPE         settingType;            //!< Type of setting value.  
     union                                              //!< Setting can hold either ULONG or Binary value or string. Not mixed types.
     {
         NvU32                      u32DefaultValue;    //!< Accessing default ULONG value of this setting.
         NVDRS_BINARY_SETTING       binaryDefaultValue; //!< Accessing default Binary value of this setting.
                                                        //!< Must be allocated by caller with valueLength specifying buffer size, or only valueLength will be filled in.
         NvAPI_UnicodeString        wszDefaultValue;    //!< Accessing default unicode string value of this setting.
     };
     union                                                //!< Setting values can be of either ULONG, Binary values or String type,
     {                                                    //!< NOT mixed types.
         NvU32                      u32Value;           //!< All possible ULONG values for a setting
         NVDRS_BINARY_SETTING       binaryValue;        //!< All possible Binary values for a setting
         NvAPI_UnicodeString        wszValue;           //!< Accessing current unicode string value of this setting.
     }settingValues[NVAPI_SETTING_MAX_VALUES];
} NVDRS_SETTING_VALUES;

//! Macro for constructing the version field of ::_NVDRS_SETTING_VALUES
#define NVDRS_SETTING_VALUES_VER    MAKE_NVAPI_VERSION(NVDRS_SETTING_VALUES,1)
     
typedef struct _NVDRS_SETTING
{
     NvU32                      version;                //!< Structure Version
     NvAPI_UnicodeString        settingName;            //!< String name of setting
     NvU32                      settingId;              //!< 32 bit setting Id
     NVDRS_SETTING_TYPE         settingType;            //!< Type of setting value.  
     NVDRS_SETTING_LOCATION     settingLocation;        //!< Describes where the value in CurrentValue comes from. 
     NvU32                      isCurrentPredefined;    //!< It is different than 0 if the currentValue is a predefined Value, 
                                                        //!< 0 if the currentValue is a user value. 
     NvU32                      isPredefinedValid;      //!< It is different than 0 if the PredefinedValue union contains a valid value. 
     union                                              //!< Setting can hold either ULONG or Binary value or string. Not mixed types.
     {
         NvU32                      u32PredefinedValue;    //!< Accessing default ULONG value of this setting.
         NVDRS_BINARY_SETTING       binaryPredefinedValue; //!< Accessing default Binary value of this setting.
                                                           //!< Must be allocated by caller with valueLength specifying buffer size, 
                                                           //!< or only valueLength will be filled in.
         NvAPI_UnicodeString        wszPredefinedValue;    //!< Accessing default unicode string value of this setting.
     };
     union                                              //!< Setting can hold either ULONG or Binary value or string. Not mixed types.
     {
         NvU32                      u32CurrentValue;    //!< Accessing current ULONG value of this setting.
         NVDRS_BINARY_SETTING       binaryCurrentValue; //!< Accessing current Binary value of this setting.
                                                        //!< Must be allocated by caller with valueLength specifying buffer size, 
                                                        //!< or only valueLength will be filled in.
         NvAPI_UnicodeString        wszCurrentValue;    //!< Accessing current unicode string value of this setting.
     };                                                 
} NVDRS_SETTING;

//! Macro for constructing the version field of ::_NVDRS_SETTING
#define NVDRS_SETTING_VER        MAKE_NVAPI_VERSION(NVDRS_SETTING,1)

typedef struct _NVDRS_APPLICATION_V1
{
     NvU32                      version;            //!< Structure Version
     NvU32                      isPredefined;       //!< Is the application userdefined/predefined
     NvAPI_UnicodeString        appName;            //!< String name of the Application
     NvAPI_UnicodeString        userFriendlyName;   //!< UserFriendly name of the Application
     NvAPI_UnicodeString        launcher;           //!< Indicates the name (if any) of the launcher that starts the application  
} NVDRS_APPLICATION_V1;

typedef struct _NVDRS_APPLICATION_V2
{
     NvU32                      version;            //!< Structure Version
     NvU32                      isPredefined;       //!< Is the application userdefined/predefined
     NvAPI_UnicodeString        appName;            //!< String name of the Application
     NvAPI_UnicodeString        userFriendlyName;   //!< UserFriendly name of the Application
     NvAPI_UnicodeString        launcher;           //!< Indicates the name (if any) of the launcher that starts the Application
     NvAPI_UnicodeString        fileInFolder;       //!< Select this application only if this file is found.
                                                    //!< When specifying multiple files, separate them using the ':' character.
} NVDRS_APPLICATION_V2;

typedef struct _NVDRS_APPLICATION_V3
{
     NvU32                      version;            //!< Structure Version
     NvU32                      isPredefined;       //!< Is the application userdefined/predefined
     NvAPI_UnicodeString        appName;            //!< String name of the Application
     NvAPI_UnicodeString        userFriendlyName;   //!< UserFriendly name of the Application
     NvAPI_UnicodeString        launcher;           //!< Indicates the name (if any) of the launcher that starts the Application
     NvAPI_UnicodeString        fileInFolder;       //!< Select this application only if this file is found.
                                                    //!< When specifying multiple files, separate them using the ':' character.
     NvU32                      isMetro:1;          //!< Windows 8 style app
     NvU32                      reserved:31;        //!< Reserved. Should be 0.
} NVDRS_APPLICATION_V3;

#define NVDRS_APPLICATION_VER_V1        MAKE_NVAPI_VERSION(NVDRS_APPLICATION_V1,1)
#define NVDRS_APPLICATION_VER_V2        MAKE_NVAPI_VERSION(NVDRS_APPLICATION_V2,2)
#define NVDRS_APPLICATION_VER_V3        MAKE_NVAPI_VERSION(NVDRS_APPLICATION_V3,3)

typedef NVDRS_APPLICATION_V3 NVDRS_APPLICATION;
#define NVDRS_APPLICATION_VER NVDRS_APPLICATION_VER_V3

typedef struct _NVDRS_PROFILE
{
     NvU32                      version;            //!< Structure Version
     NvAPI_UnicodeString        profileName;        //!< String name of the Profile
     NVDRS_GPU_SUPPORT          gpuSupport;         //!< This read-only flag indicates the profile support on either
                                                    //!< Quadro, or Geforce, or both.
     NvU32                      isPredefined;       //!< Is the Profile user-defined, or predefined
     NvU32                      numOfApps;          //!< Total number of applications that belong to this profile. Read-only
     NvU32                      numOfSettings;      //!< Total number of settings applied for this Profile. Read-only
} NVDRS_PROFILE;

//! Macro for constructing the version field of ::_NVDRS_PROFILE
#define NVDRS_PROFILE_VER        MAKE_NVAPI_VERSION(NVDRS_PROFILE,1)


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_CreateSession
//
//!   DESCRIPTION: This API allocates memory and initializes the session.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [out]  *phSession Return pointer to the session handle.
//!                
//! \retval ::NVAPI_OK SUCCESS
//! \retval ::NVAPI_ERROR: For miscellaneous errors.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_CreateSession(NvDRSSessionHandle *phSession);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_DestroySession
//
//!   DESCRIPTION: This API frees the allocation: cleanup of NvDrsSession.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in] hSession Input to the session handle.
//!                
//! \retval ::NVAPI_OK SUCCESS
//! \retval ::NVAPI_ERROR For miscellaneous errors.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_DestroySession(NvDRSSessionHandle hSession);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_LoadSettings
//
//!   DESCRIPTION: This API loads and parses the settings data.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in] hSession  Input to the session handle.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_LoadSettings(NvDRSSessionHandle hSession);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_SaveSettings
//
//!   DESCRIPTION: This API saves the settings data to the system.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in] hSession  Input to the session handle.
//!                
//! \retval ::NVAPI_OK    SUCCESS
//! \retval ::NVAPI_ERROR For miscellaneous errors.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_SaveSettings(NvDRSSessionHandle hSession);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_LoadSettingsFromFile
//
//!   DESCRIPTION: This API loads settings from the given file path.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession Input to the session handle
//! \param [in]  fileName Binary File Name/Path
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_LoadSettingsFromFile(NvDRSSessionHandle hSession, NvAPI_UnicodeString fileName);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_SaveSettingsToFile
//
//!   DESCRIPTION: This API saves settings to the given file path.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession  Input to the session handle.
//! \param [in]  fileName  Binary File Name/Path
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_SaveSettingsToFile(NvDRSSessionHandle hSession, NvAPI_UnicodeString fileName);

//! @}



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_CreateProfile
//
//!   DESCRIPTION: This API creates an empty profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession        Input to the session handle.
//! \param [in]  *pProfileInfo   Input pointer to NVDRS_PROFILE.
//! \param [in]  *phProfile      Returns pointer to profile handle.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_CreateProfile(NvDRSSessionHandle hSession, NVDRS_PROFILE *pProfileInfo, NvDRSProfileHandle *phProfile);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_DeleteProfile
//
//!   DESCRIPTION: This API deletes a profile or sets it back to a predefined value.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in] hSession  Input to the session handle.
//! \param [in] hProfile  Input profile handle.
//!                
//! \retval ::NVAPI_OK     SUCCESS if the profile is found
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_DeleteProfile(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_SetCurrentGlobalProfile
//
//!   DESCRIPTION: This API sets the current global profile in the driver.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in] hSession                Input to the session handle.
//! \param [in] wszGlobalProfileName    Input current Global profile name.
//!               
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_SetCurrentGlobalProfile(NvDRSSessionHandle hSession, NvAPI_UnicodeString wszGlobalProfileName);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetCurrentGlobalProfile
//
//!   DESCRIPTION: This API returns the handle to the current global profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hSession     Input to the session handle.
//! \param [out]  *phProfile   Returns current Global profile handle.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetCurrentGlobalProfile(NvDRSSessionHandle hSession, NvDRSProfileHandle *phProfile);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetProfileInfo
//
//!   DESCRIPTION: This API gets information about the given profile. User needs to specify the name of the Profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession       Input to the session handle.
//! \param [in]  hProfile       Input profile handle.
//! \param [out] *pProfileInfo  Return the profile info.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetProfileInfo(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NVDRS_PROFILE *pProfileInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_SetProfileInfo
//
//!   DESCRIPTION: Specifies flags for a given profile. Currently only the NVDRS_GPU_SUPPORT is
//!                used to update the profile. Neither the name, number of settings or applications
//!                or other profile information can be changed with this function. 
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession       Input to the session handle.
//! \param [in]  hProfile       Input profile handle.
//! \param [in]  *pProfileInfo  Input the new profile info.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_SetProfileInfo(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NVDRS_PROFILE *pProfileInfo);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_FindProfileByName
//
//!   DESCRIPTION: This API finds a profile in the current session.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hSession      Input to the session handle.
//! \param [in]   profileName   Input profileName.
//! \param [out]  phProfile     Input profile handle.
//!                
//! \retval ::NVAPI_OK                SUCCESS if the profile is found
//! \retval ::NVAPI_PROFILE_NOT_FOUND if profile is not found
//! \retval ::NVAPI_ERROR             For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_FindProfileByName(NvDRSSessionHandle hSession, NvAPI_UnicodeString profileName, NvDRSProfileHandle* phProfile);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_EnumProfiles
//
//!   DESCRIPTION: This API enumerates through all the profiles in the session.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hSession        Input to the session handle.
//! \param [in]   index           Input the index for enumeration.
//! \param [out]  *phProfile      Returns profile handle.
//!                
//!   RETURN STATUS: NVAPI_OK: SUCCESS if the profile is found
//!                  NVAPI_ERROR: For miscellaneous errors.
//!                  NVAPI_END_ENUMERATION: index exceeds the total number of available Profiles in DB.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_EnumProfiles(NvDRSSessionHandle hSession, NvU32 index, NvDRSProfileHandle *phProfile);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetNumProfiles
//
//!   DESCRIPTION: This API obtains the number of profiles in the current session object.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession       Input to the session handle.
//! \param out]  *numProfiles   Returns count of profiles in the current hSession.
//!                
//! \retval ::NVAPI_OK                  SUCCESS
//! \retval ::NVAPI_API_NOT_INTIALIZED  Failed to initialize.
//! \retval ::NVAPI_INVALID_ARGUMENT    Invalid Arguments.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetNumProfiles(NvDRSSessionHandle hSession, NvU32 *numProfiles);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_CreateApplication
//
//!   DESCRIPTION: This API adds an executable name to a profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession       Input to the session handle.
//! \param [in]  hProfile       Input profile handle.
//! \param [in]  *pApplication  Input NVDRS_APPLICATION struct with the executable name to be added.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_CreateApplication(NvDRSSessionHandle hSession, NvDRSProfileHandle  hProfile, NVDRS_APPLICATION *pApplication);
 

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_DeleteApplicationEx
//
//!   DESCRIPTION: This API removes an executable from a profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hSession  - Input to the session handle.
//! \param [in]   hProfile  - Input profile handle.
//! \param [in]   *pApp     - Input all the information about the application to be removed.
//!
//! \retval ::NVAPI_OK  SUCCESS
//! \retval ::NVAPI_ERROR For miscellaneous errors.
//! \retval ::NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS If the path provided could refer to two different executables,
//!                                              this error will be returned.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_DeleteApplicationEx(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NVDRS_APPLICATION *pApp);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_DeleteApplication
//
//!   DESCRIPTION: This API removes an executable name from a profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSessionPARAMETERS   Input to the session handle.
//! \param [in]  hProfile             Input profile handle.
//! \param [in]  appName              Input the executable name to be removed.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//! \retval ::NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS If the path provided could refer to two different executables,
//!                                              this error will be returned
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_DeleteApplication(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NvAPI_UnicodeString appName);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetApplicationInfo
//
//!   DESCRIPTION: This API gets information about the given application.  The input application name
//!                must match exactly what the Profile has stored for the application. 
//!                This function is better used to retrieve application information from a previous
//!                enumeration.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hSession       Input to the session handle.
//! \param [in]   hProfile       Input profile handle.
//! \param [in]   appName        Input application name.
//! \param [out]  *pApplication  Returns NVDRS_APPLICATION struct with all the attributes.
//!                
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!          If there are return error codes with specific meaning for this API, 
//!          they are listed below.
//! \retval ::NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS   The application name could not 
//                                                single out only one executable.
//! \retval ::NVAPI_EXECUTABLE_NOT_FOUND           No application with that name is found on the profile.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetApplicationInfo(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NvAPI_UnicodeString appName, NVDRS_APPLICATION *pApplication);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_EnumApplications
//
//!   DESCRIPTION: This API enumerates all the applications in a given profile from the starting index to the maximum length.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]      hSession         Input to the session handle.
//! \param [in]      hProfile         Input profile handle.
//! \param [in]      startIndex       Indicates starting index for enumeration.
//! \param [in,out]  *appCount        Input maximum length of the passed in arrays. Returns the actual length.
//! \param [out]     *pApplication    Returns NVDRS_APPLICATION struct with all the attributes.
//!                
//! \retval ::NVAPI_OK               SUCCESS
//! \retval ::NVAPI_ERROR            For miscellaneous errors.
//! \retval ::NVAPI_END_ENUMERATION  startIndex exceeds the total appCount.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_EnumApplications(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NvU32 startIndex, NvU32 *appCount, NVDRS_APPLICATION *pApplication);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_FindApplicationByName
//
//!   DESCRIPTION: This API searches the application and the associated profile for the given application name.
//!                If a fully qualified path is provided, this function will always return the profile
//!                the driver will apply upon running the application (on the path provided).
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hSession       Input to the hSession handle
//! \param [in]   appName        Input appName. For best results, provide a fully qualified path of the type
//!                              c:/Folder1/Folder2/App.exe
//! \param [out]  *phProfile     Returns profile handle.
//! \param [out]  *pApplication  Returns NVDRS_APPLICATION struct pointer.
//!                
//! \return  This API can return any of the error codes enumerated in #NvAPI_Status. 
//!                  If there are return error codes with specific meaning for this API, 
//!                  they are listed below:
//! \retval ::NVAPI_APPLICATION_NOT_FOUND          If App not found
//! \retval ::NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS   If the input appName was not fully qualified, this error might return in the case of multiple matches
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_FindApplicationByName(NvDRSSessionHandle hSession, NvAPI_UnicodeString appName, NvDRSProfileHandle *phProfile, NVDRS_APPLICATION *pApplication);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_SetSetting
//
//!   DESCRIPTION: This API adds/modifies a setting to a profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession     Input to the session handle.
//! \param [in]  hProfile     Input profile handle.
//! \param [in]   *pSetting   Input NVDRS_SETTING struct pointer.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_SetSetting(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NVDRS_SETTING *pSetting);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetSetting
//
//!   DESCRIPTION: This API gets information about the given setting.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   hSession   Input to the session handle.
//! \param [in]   hProfile   Input profile handle.
//! \param [in]   settingId  Input settingId.
//! \param [out]  *pSetting  Returns all the setting info
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetSetting(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NvU32 settingId, NVDRS_SETTING *pSetting);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_EnumSettings
//
//!   DESCRIPTION: This API enumerates all the settings of a given profile from startIndex to the maximum length.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]      hSession        Input to the session handle.
//! \param [in]      hProfile        Input profile handle.
//! \param [in]      startIndex      Indicates starting index for enumeration.
//! \param [in,out]  *settingsCount  Input max length of the passed in arrays, Returns the actual length.
//! \param [out]     *pSetting       Returns all the settings info.
//!                
//! \retval ::NVAPI_OK              SUCCESS
//! \retval ::NVAPI_ERROR           For miscellaneous errors.
//! \retval ::NVAPI_END_ENUMERATION startIndex exceeds the total appCount.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_EnumSettings(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NvU32 startIndex, NvU32 *settingsCount, NVDRS_SETTING *pSetting);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_EnumAvailableSettingIds
//
//!   DESCRIPTION: This API enumerates all the Ids of all the settings recognized by NVAPI.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [out]    pSettingIds     User-provided array of length *pMaxCount that NVAPI will fill with IDs.
//! \param [in,out] pMaxCount       Input max length of the passed in array, Returns the actual length.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!                  NVAPI_END_ENUMERATION: the provided pMaxCount is not enough to hold all settingIds.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_EnumAvailableSettingIds(NvU32 *pSettingIds, NvU32 *pMaxCount);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_EnumAvailableSettingValues
//
//!   DESCRIPTION: This API enumerates all available setting values for a given setting.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]      settingId          Input settingId.
//! \param [in,out]  maxNumCount        Input max length of the passed in arrays, Returns the actual length.
//! \param [out]     *pSettingValues    Returns all available setting values and its count.
//!                
//! \retval ::NVAPI_OK     SUCCESS
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_EnumAvailableSettingValues(NvU32 settingId, NvU32 *pMaxNumValues, NVDRS_SETTING_VALUES *pSettingValues);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetSettingIdFromName
//
//!   DESCRIPTION: This API gets the binary ID of a setting given the setting name.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]   settingName  Input Unicode settingName.
//! \param [out]  *pSettingId  Returns corresponding settingId.
//!                
//! \retval ::NVAPI_OK                 SUCCESS if the profile is found
//! \retval ::NVAPI_PROFILE_NOT_FOUND  if profile is not found
//! \retval ::NVAPI_SETTING_NOT_FOUND  if setting is not found
//! \retval ::NVAPI_ERROR              For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetSettingIdFromName(NvAPI_UnicodeString settingName, NvU32 *pSettingId);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetSettingNameFromId
//
//!   DESCRIPTION: This API gets the setting name given the binary ID.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  settingId        Input settingId.
//! \param [in]  *pSettingName    Returns corresponding Unicode settingName.
//!                
//! \retval ::NVAPI_OK                 SUCCESS if the profile is found
//! \retval ::NVAPI_PROFILE_NOT_FOUND  if profile is not found
//! \retval ::NVAPI_SETTING_NOT_FOUND  if setting is not found
//! \retval ::NVAPI_ERROR              For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetSettingNameFromId(NvU32 settingId, NvAPI_UnicodeString *pSettingName);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_DeleteProfileSetting
//
//!   DESCRIPTION: This API deletes a setting or sets it back to predefined value.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession            Input to the session handle.
//! \param [in]  hProfile            Input profile handle.
//! \param [in]  settingId           Input settingId to be deleted.
//!                
//! \retval ::NVAPI_OK     SUCCESS if the profile is found
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
/////////////////////////////////////////////////////////////////////////////// 
NVAPI_INTERFACE NvAPI_DRS_DeleteProfileSetting(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NvU32 settingId);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_RestoreAllDefaults
//
//!   DESCRIPTION: This API restores the whole system to predefined(default) values.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession  Input to the session handle.
//!                
//! \retval ::NVAPI_OK     SUCCESS if the profile is found
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
/////////////////////////////////////////////////////////////////////////////// 
NVAPI_INTERFACE NvAPI_DRS_RestoreAllDefaults(NvDRSSessionHandle hSession);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_RestoreProfileDefault
//
//!   DESCRIPTION: This API restores the given profile to predefined(default) values.
//!                Any and all user specified modifications will be removed. 
//!                If the whole profile was set by the user, the profile will be removed.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession  Input to the session handle.
//! \param [in]  hProfile  Input profile handle.
//!                
//! \retval ::NVAPI_OK              SUCCESS if the profile is found
//! \retval ::NVAPI_ERROR           For miscellaneous errors.
//! \retval ::NVAPI_PROFILE_REMOVED SUCCESS, and the hProfile is no longer valid.
//! \retval ::NVAPI_ERROR           For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_RestoreProfileDefault(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_RestoreProfileDefaultSetting
//
//!   DESCRIPTION: This API restores the given profile setting to predefined(default) values.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession  Input to the session handle.
//! \param [in]  hProfile  Input profile handle.
//! \param [in]  settingId Input settingId.
//!                
//! \retval ::NVAPI_OK     SUCCESS if the profile is found
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_RestoreProfileDefaultSetting(NvDRSSessionHandle hSession, NvDRSProfileHandle hProfile, NvU32 settingId);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_DRS_GetBaseProfile
//
//!   DESCRIPTION: Returns the handle to the current global profile.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]  hSession    Input to the session handle.
//! \param [in]  *phProfile   Returns Base profile handle.
//!                
//! \retval ::NVAPI_OK     SUCCESS if the profile is found
//! \retval ::NVAPI_ERROR  For miscellaneous errors.
//!
//! \ingroup drsapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DRS_GetBaseProfile(NvDRSSessionHandle hSession, NvDRSProfileHandle *phProfile);




//! \addtogroup sysgeneral
//! @{

typedef struct _NV_CHIPSET_INFO_v4
{
    NvU32               version;            //!< structure version
    NvU32               vendorId;           //!< Chipset vendor identification
    NvU32               deviceId;           //!< Chipset device identification
    NvAPI_ShortString   szVendorName;       //!< Chipset vendor Name
    NvAPI_ShortString   szChipsetName;      //!< Chipset device Name
    NvU32               flags;              //!< Chipset info flags - obsolete
    NvU32               subSysVendorId;     //!< Chipset subsystem vendor identification
    NvU32               subSysDeviceId;     //!< Chipset subsystem device identification 
    NvAPI_ShortString   szSubSysVendorName; //!< subsystem vendor Name
    NvU32               HBvendorId;         //!< Host bridge vendor identification
    NvU32               HBdeviceId;         //!< Host bridge device identification
    NvU32               HBsubSysVendorId;   //!< Host bridge subsystem vendor identification
    NvU32               HBsubSysDeviceId;   //!< Host bridge subsystem device identification

} NV_CHIPSET_INFO_v4;

typedef struct _NV_CHIPSET_INFO_v3
{
    NvU32               version;            //!< structure version
    NvU32               vendorId;           //!< vendor ID
    NvU32               deviceId;           //!< device ID
    NvAPI_ShortString   szVendorName;       //!< vendor Name
    NvAPI_ShortString   szChipsetName;      //!< device Name
    NvU32               flags;              //!< Chipset info flags - obsolete
    NvU32               subSysVendorId;     //!< subsystem vendor ID
    NvU32               subSysDeviceId;     //!< subsystem device ID
    NvAPI_ShortString   szSubSysVendorName; //!< subsystem vendor Name
} NV_CHIPSET_INFO_v3;

typedef enum _NV_CHIPSET_INFO_FLAGS
{
    NV_CHIPSET_INFO_HYBRID          = 0x00000001,
} NV_CHIPSET_INFO_FLAGS;

typedef struct _NV_CHIPSET_INFO_v2
{
    NvU32               version;        //!< structure version
    NvU32               vendorId;       //!< vendor ID
    NvU32               deviceId;       //!< device ID
    NvAPI_ShortString   szVendorName;   //!< vendor Name
    NvAPI_ShortString   szChipsetName;  //!< device Name
    NvU32               flags;          //!< Chipset info flags
} NV_CHIPSET_INFO_v2;

typedef struct _NV_CHIPSET_INFO_v1
{
    NvU32               version;        //structure version
    NvU32               vendorId;       //vendor ID
    NvU32               deviceId;       //device ID
    NvAPI_ShortString   szVendorName;   //vendor Name
    NvAPI_ShortString   szChipsetName;  //device Name
} NV_CHIPSET_INFO_v1;

#define NV_CHIPSET_INFO_VER_1  MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v1,1)
#define NV_CHIPSET_INFO_VER_2  MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v2,2)
#define NV_CHIPSET_INFO_VER_3  MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v3,3)
#define NV_CHIPSET_INFO_VER_4  MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v4,4)

#define NV_CHIPSET_INFO        NV_CHIPSET_INFO_v4
#define NV_CHIPSET_INFO_VER    NV_CHIPSET_INFO_VER_4

//! @}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_SYS_GetChipSetInfo
//
//!  This function returns information about the system's chipset.
//!
//! SUPPORTED OS:  Windows XP and higher,  Mac OS X
//!
//!
//! \since Release: 95
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pChipSetInfo is NULL.
//! \retval  NVAPI_OK                           *pChipSetInfo is now set.
//! \retval  NVAPI_INCOMPATIBLE_STRUCT_VERSION   NV_CHIPSET_INFO version not compatible with driver.
//! \ingroup sysgeneral
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_SYS_GetChipSetInfo(NV_CHIPSET_INFO *pChipSetInfo);

typedef NvAPI_Status (__cdecl *_NvAPI_SYS_GetChipSetInfo)(NV_CHIPSET_INFO* pChipSetInfo);
_NvAPI_SYS_GetChipSetInfo NvAPI_SYS_GetChipSetInfo;

//! \ingroup sysgeneral
//! Lid and dock information - used in NvAPI_GetLidDockInfo()
typedef struct _NV_LID_DOCK_PARAMS
{
    NvU32 version;    //! Structure version, constructed from the macro #NV_LID_DOCK_PARAMS_VER
    NvU32 currentLidState;
    NvU32 currentDockState;
    NvU32 currentLidPolicy;
    NvU32 currentDockPolicy;
    NvU32 forcedLidMechanismPresent;
    NvU32 forcedDockMechanismPresent;
} NV_LID_DOCK_PARAMS;


//! ingroup sysgeneral
#define NV_LID_DOCK_PARAMS_VER  MAKE_NVAPI_VERSION(NV_LID_DOCK_PARAMS,1)
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetLidDockInfo
//
//! DESCRIPTION: This function returns the current lid and dock information.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \since Release: 177
//!
//! \retval ::NVAPI_OK  
//! \retval ::NVAPI_ERROR
//! \retval ::NVAPI_NOT_SUPPORTED
//! \retval ::NVAPI_HANDLE_INVALIDATED
//! \retval ::NVAPI_API_NOT_INTIALIZED 
//!
//! \ingroup sysgeneral
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SYS_GetLidAndDockInfo(NV_LID_DOCK_PARAMS *pLidAndDock);




///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SYS_GetDisplayIdFromGpuAndOutputId
//
//! DESCRIPTION:     This API converts a Physical GPU handle and output ID to a
//!                  display ID.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]     hPhysicalGpu   Handle to the physical GPU
//! \param [in]     outputId       Connected display output ID on the 
//!                                target GPU - must only have one bit set
//! \param [out]    displayId      Pointer to an NvU32 which contains
//!                                 the display ID
//!
//! \retval  ::NVAPI_OK - completed request
//! \retval  ::NVAPI_API_NOT_INTIALIZED - NVAPI not initialized
//! \retval  ::NVAPI_ERROR - miscellaneous error occurred
//! \retval  ::NVAPI_INVALID_ARGUMENT - Invalid input parameter.
//!
//! \ingroup sysgeneral
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SYS_GetDisplayIdFromGpuAndOutputId(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputId, NvU32* displayId);


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SYS_GetGpuAndOutputIdFromDisplayId
//
//! DESCRIPTION:     This API converts a display ID to a Physical GPU handle and output ID.
//!
//! SUPPORTED OS:  Windows XP and higher
//!
//!
//! \param [in]     displayId       Display ID of display to retrieve 
//!                                 GPU and outputId for
//! \param [out]    hPhysicalGpu    Handle to the physical GPU
//! \param [out]    outputId )      Connected display output ID on the 
//!                                 target GPU will only have one bit set.
//!
//! \retval ::NVAPI_OK 
//! \retval ::NVAPI_API_NOT_INTIALIZED 
//! \retval ::NVAPI_ID_OUT_OF_RANGE    The DisplayId corresponds to a 
//!                                    display which is not within the
//!                                    normal outputId range.
//! \retval ::NVAPI_ERROR   
//! \retval ::NVAPI_INVALID_ARGUMENT 
//!
//! \ingroup sysgeneral
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SYS_GetGpuAndOutputIdFromDisplayId(NvU32 displayId, NvPhysicalGpuHandle *hPhysicalGpu, NvU32 *outputId);


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SYS_GetPhysicalGpuFromDisplayId
//
//! \code
//! DESCRIPTION:     This API retrieves the Physical GPU handle of the connected display
//!
//! \since Release: 313
//!
//! SUPPORTED OS:  Windows Vista and higher
//!
//!
//! PARAMETERS:      displayId(IN)     - Display ID of display to retrieve 
//!                                      GPU handle
//!                  hPhysicalGpu(OUT) - Handle to the physical GPU
//!
//! RETURN STATUS:
//!                  NVAPI_OK - completed request
//!                  NVAPI_API_NOT_INTIALIZED - NVAPI not initialized
//!                  NVAPI_ERROR - miscellaneous error occurred
//!                  NVAPI_INVALID_ARGUMENT - Invalid input parameter.
//! \endcode
//! \ingroup sysgeneral
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SYS_GetPhysicalGpuFromDisplayId(NvU32 displayId, NvPhysicalGpuHandle *hPhysicalGpu);




#ifdef __cplusplus
}; //extern "C" {

#endif

#include <poppack.h>

#endif // _NVAPI_H