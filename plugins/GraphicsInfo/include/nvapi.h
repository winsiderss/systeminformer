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
// Date: Jun 5, 2011
// File: nvapi.h
//
// NvAPI provides an interface to NVIDIA devices. This file contains the 
// interface constants, structure definitions and function prototypes.
//
//   Target Profile: developer
//  Target Platform: windows
//
///////////////////////////////////////////////////////////////////////////////

#pragma pack(push,8) // Make sure we have consistent structure packings

#ifndef _WIN32
#define __cdecl
#endif

#define NVAPI_INTERFACE extern NvStatus __cdecl

/* 64-bit types for compilers that support them, plus some obsolete variants */
typedef unsigned __int64   NvU64; /* 0 to 18446744073709551615  */
typedef signed int         NvS32; /* -2147483648 to 2147483647 */  
// mac os 32-bit still needs this
typedef unsigned long NvU32; /* 0 to 4294967295                         */
typedef signed short NvS16;
typedef unsigned short NvU16;
typedef unsigned char NvU8;

typedef struct _NV_RECT
{
    NvU32    left;
    NvU32    top;
    NvU32    right;
    NvU32    bottom;
} NV_RECT;

#define NV_DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name

//! \addtogroup nvapihandles
//! NVAPI Handles - These handles are retrieved from various calls and passed in to others in NvAPI
//!                 These are meant to be opaque types.  Do not assume they correspond to indices, HDCs,
//!                 display indexes or anything else.
//!
//!                 Most handles remain valid until a display re-configuration (display mode set) or GPU
//!                 reconfiguration (going into or out of SLI modes) occurs.  If NVAPI_HANDLE_INVALIDATED
//!                 is received by an app, it should discard all handles, and re-enumerate them.
//! @{  
NV_DECLARE_HANDLE(NvDisplayHandle);                //!< Display Device driven by NVIDIA GPU(s) (an attached display)
NV_DECLARE_HANDLE(NvMonitorHandle);                //!< Monitor handle
NV_DECLARE_HANDLE(NvUnAttachedDisplayHandle);      //!< Unattached Display Device driven by NVIDIA GPU(s)
NV_DECLARE_HANDLE(NvLogicalGpuHandle);             //!< One or more physical GPUs acting in concert (SLI)
NV_DECLARE_HANDLE(NvPhysicalGpuHandle);            //!< A single physical GPU
NV_DECLARE_HANDLE(NvEventHandle);                  //!< A handle to an event registration instance
NV_DECLARE_HANDLE(NvVisualComputingDeviceHandle);  //!< A handle to a Visual Computing Device
NV_DECLARE_HANDLE(NvHICHandle);                    //!< A handle to a Host Interface Card
NV_DECLARE_HANDLE(NvGSyncDeviceHandle);            //!< A handle to a G-Sync device
NV_DECLARE_HANDLE(NvVioHandle);                    //!< A handle to an SDI device
NV_DECLARE_HANDLE(NvTransitionHandle);             //!< A handle to address a single transition request
NV_DECLARE_HANDLE(NvAudioHandle);                  //!< NVIDIA HD Audio Device
NV_DECLARE_HANDLE(Nv3DVPContextHandle);            //!< A handle for a 3D Vision Pro (3DVP) context
NV_DECLARE_HANDLE(Nv3DVPTransceiverHandle);        //!< A handle for a 3DVP RF transceiver
NV_DECLARE_HANDLE(Nv3DVPGlassesHandle);            //!< A handle for a pair of 3DVP RF shutter glasses
NV_DECLARE_HANDLE(NvSourceHandle);                 //!< Unique source handle on the system
NV_DECLARE_HANDLE(NvTargetHandle);                 //!< Unique target handle on the system
NV_DECLARE_HANDLE(NVDX_SwapChainHandle);           //!< DirectX SwapChain objects
static const NVDX_SwapChainHandle NVDX_SWAPCHAIN_NONE = 0;

//! \ingroup nvapihandles
#define NVAPI_DEFAULT_HANDLE 0
#define NV_BIT(x)(1 << (x)) 

//! \addtogroup nvapitypes
#define NVAPI_GENERIC_STRING_MAX    4096
#define NVAPI_LONG_STRING_MAX       256
#define NVAPI_SHORT_STRING_MAX      64


typedef struct 
{
    NvS32   sX;
    NvS32   sY;
    NvS32   sWidth;
    NvS32   sHeight;
} NvSBox;

typedef struct
{
    NvU32 data1;
    NvU16 data2;
    NvU16 data3;
    NvU8  data4[8];
} NvGUID, NvLUID;

#define NVAPI_MAX_PHYSICAL_GPUS             64
#define NVAPI_PHYSICAL_GPUS                 32
#define NVAPI_MAX_LOGICAL_GPUS              64
#define NVAPI_MAX_AVAILABLE_GPU_TOPOLOGIES  256
#define NVAPI_MAX_AVAILABLE_SLI_GROUPS      256
#define NVAPI_MAX_GPU_TOPOLOGIES            NVAPI_MAX_PHYSICAL_GPUS
#define NVAPI_MAX_GPU_PER_TOPOLOGY          8
#define NVAPI_MAX_DISPLAY_HEADS             2
#define NVAPI_ADVANCED_DISPLAY_HEADS        4
#define NVAPI_MAX_DISPLAYS                  NVAPI_PHYSICAL_GPUS * NVAPI_ADVANCED_DISPLAY_HEADS
#define NVAPI_MAX_ACPI_IDS                  16
#define NVAPI_MAX_VIEW_MODES                8
#define NV_MAX_HEADS                        4   //!< Maximum heads, each with NVAPI_DESKTOP_RES resolution
#define NVAPI_MAX_HEADS_PER_GPU             32

#define NV_MAX_HEADS        4   //!< Maximum number of heads, each with #NVAPI_DESKTOP_RES resolution
#define NV_MAX_VID_STREAMS  4   //!< Maximum number of input video streams, each with a #NVAPI_VIDEO_SRC_INFO
#define NV_MAX_VID_PROFILES 4   //!< Maximum number of output video profiles supported

#define NVAPI_SYSTEM_MAX_DISPLAYS           NVAPI_MAX_PHYSICAL_GPUS * NV_MAX_HEADS

#define NVAPI_SYSTEM_MAX_HWBCS              128
#define NVAPI_SYSTEM_HWBC_INVALID_ID        0xffffffff
#define NVAPI_MAX_AUDIO_DEVICES             16

typedef char NvAPI_String[NVAPI_GENERIC_STRING_MAX];
typedef char NvAPI_LongString[NVAPI_LONG_STRING_MAX];
typedef char NvAPI_ShortString[NVAPI_SHORT_STRING_MAX];

// =========================================================================================
//!  NvAPI Version Definition \n
//!  Maintain per structure specific version define using the MAKE_NVAPI_VERSION macro. \n
//!  Usage: #define NV_GENLOCK_STATUS_VER  MAKE_NVAPI_VERSION(NV_GENLOCK_STATUS, 1)
//!  \ingroup nvapitypes
// =========================================================================================
#define MAKE_NVAPI_VERSION(typeName,ver) (NvU32)(sizeof(typeName) | ((ver)<<16))

//!  \ingroup nvapitypes
#define GET_NVAPI_VERSION(ver) (NvU32)((ver)>>16)

//!  \ingroup nvapitypes
#define GET_NVAPI_SIZE(ver) (NvU32)((ver) & 0xffff)


// ====================================================
//! NvAPI Status Values
//!   All NvAPI functions return one of these codes.
//!   \ingroup nvapistatus 
// ====================================================

typedef enum _NvStatus
{
    NVAPI_OK                                    =  0,      //!< Success. Request is completed.
    NVAPI_ERROR                                 = -1,      //!< Generic error
    NVAPI_LIBRARY_NOT_FOUND                     = -2,      //!< NVAPI support library cannot be loaded.
    NVAPI_NO_IMPLEMENTATION                     = -3,      //!< not implemented in current driver installation
    NVAPI_API_NOT_INITIALIZED                   = -4,      //!< NvAPI_Initialize has not been called (successfully)
    NVAPI_INVALID_ARGUMENT                      = -5,      //!< The argument/parameter value is not valid or NULL.
    NVAPI_NVIDIA_DEVICE_NOT_FOUND               = -6,      //!< No NVIDIA display driver, or NVIDIA GPU driving a display, was found.
    NVAPI_END_ENUMERATION                       = -7,      //!< No more items to enumerate
    NVAPI_INVALID_HANDLE                        = -8,      //!< Invalid handle
    NVAPI_INCOMPATIBLE_STRUCT_VERSION           = -9,      //!< An argument's structure version is not supported
    NVAPI_HANDLE_INVALIDATED                    = -10,     //!< The handle is no longer valid (likely due to GPU or display re-configuration)
    NVAPI_OPENGL_CONTEXT_NOT_CURRENT            = -11,     //!< No NVIDIA OpenGL context is current (but needs to be)
    NVAPI_INVALID_POINTER                       = -14,     //!< An invalid pointer, usually NULL, was passed as a parameter
    NVAPI_NO_GL_EXPERT                          = -12,     //!< OpenGL Expert is not supported by the current drivers
    NVAPI_INSTRUMENTATION_DISABLED              = -13,     //!< OpenGL Expert is supported, but driver instrumentation is currently disabled
    NVAPI_NO_GL_NSIGHT                          = -15,     //!< OpenGL does not support Nsight

    NVAPI_EXPECTED_LOGICAL_GPU_HANDLE           = -100,    //!< Expected a logical GPU handle for one or more parameters
    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE          = -101,    //!< Expected a physical GPU handle for one or more parameters
    NVAPI_EXPECTED_DISPLAY_HANDLE               = -102,    //!< Expected an NV display handle for one or more parameters
    NVAPI_INVALID_COMBINATION                   = -103,    //!< The combination of parameters is not valid. 
    NVAPI_NOT_SUPPORTED                         = -104,    //!< Requested feature is not supported in the selected GPU
    NVAPI_PORTID_NOT_FOUND                      = -105,    //!< No port ID was found for the I2C transaction
    NVAPI_EXPECTED_UNATTACHED_DISPLAY_HANDLE    = -106,    //!< Expected an unattached display handle as one of the input parameters.
    NVAPI_INVALID_PERF_LEVEL                    = -107,    //!< Invalid perf level 
    NVAPI_DEVICE_BUSY                           = -108,    //!< Device is busy; request not fulfilled
    NVAPI_NV_PERSIST_FILE_NOT_FOUND             = -109,    //!< NV persist file is not found
    NVAPI_PERSIST_DATA_NOT_FOUND                = -110,    //!< NV persist data is not found
    NVAPI_EXPECTED_TV_DISPLAY                   = -111,    //!< Expected a TV output display
    NVAPI_EXPECTED_TV_DISPLAY_ON_DCONNECTOR     = -112,    //!< Expected a TV output on the D Connector - HDTV_EIAJ4120.
    NVAPI_NO_ACTIVE_SLI_TOPOLOGY                = -113,    //!< SLI is not active on this device.
    NVAPI_SLI_RENDERING_MODE_NOTALLOWED         = -114,    //!< Setup of SLI rendering mode is not possible right now.
    NVAPI_EXPECTED_DIGITAL_FLAT_PANEL           = -115,    //!< Expected a digital flat panel.
    NVAPI_ARGUMENT_EXCEED_MAX_SIZE              = -116,    //!< Argument exceeds the expected size.
    NVAPI_DEVICE_SWITCHING_NOT_ALLOWED          = -117,    //!< Inhibit is ON due to one of the flags in NV_GPU_DISPLAY_CHANGE_INHIBIT or SLI active.
    NVAPI_TESTING_CLOCKS_NOT_SUPPORTED          = -118,    //!< Testing of clocks is not supported.
    NVAPI_UNKNOWN_UNDERSCAN_CONFIG              = -119,    //!< The specified underscan config is from an unknown source (e.g. INF)
    NVAPI_TIMEOUT_RECONFIGURING_GPU_TOPO        = -120,    //!< Timeout while reconfiguring GPUs
    NVAPI_DATA_NOT_FOUND                        = -121,    //!< Requested data was not found
    NVAPI_EXPECTED_ANALOG_DISPLAY               = -122,    //!< Expected an analog display
    NVAPI_NO_VIDLINK                            = -123,    //!< No SLI video bridge is present
    NVAPI_REQUIRES_REBOOT                       = -124,    //!< NVAPI requires a reboot for the settings to take effect
    NVAPI_INVALID_HYBRID_MODE                   = -125,    //!< The function is not supported with the current Hybrid mode.
    NVAPI_MIXED_TARGET_TYPES                    = -126,    //!< The target types are not all the same
    NVAPI_SYSWOW64_NOT_SUPPORTED                = -127,    //!< The function is not supported from 32-bit on a 64-bit system.
    NVAPI_IMPLICIT_SET_GPU_TOPOLOGY_CHANGE_NOT_ALLOWED = -128,    //!< There is no implicit GPU topology active. Use NVAPI_SetHybridMode to change topology.
    NVAPI_REQUEST_USER_TO_CLOSE_NON_MIGRATABLE_APPS = -129,      //!< Prompt the user to close all non-migratable applications.    
    NVAPI_OUT_OF_MEMORY                         = -130,    //!< Could not allocate sufficient memory to complete the call.
    NVAPI_WAS_STILL_DRAWING                     = -131,    //!< The previous operation that is transferring information to or from this surface is incomplete.
    NVAPI_FILE_NOT_FOUND                        = -132,    //!< The file was not found.
    NVAPI_TOO_MANY_UNIQUE_STATE_OBJECTS         = -133,    //!< There are too many unique instances of a particular type of state object.
    NVAPI_INVALID_CALL                          = -134,    //!< The method call is invalid. For example, a method's parameter may not be a valid pointer.
    NVAPI_D3D10_1_LIBRARY_NOT_FOUND             = -135,    //!< d3d10_1.dll cannot be loaded.
    NVAPI_FUNCTION_NOT_FOUND                    = -136,    //!< Couldn't find the function in the loaded DLL.
    NVAPI_INVALID_USER_PRIVILEGE                = -137,    //!< Current User is not Admin.
    NVAPI_EXPECTED_NON_PRIMARY_DISPLAY_HANDLE   = -138,    //!< The handle corresponds to GDIPrimary.
    NVAPI_EXPECTED_COMPUTE_GPU_HANDLE           = -139,    //!< Setting Physx GPU requires that the GPU is compute-capable.
    NVAPI_STEREO_NOT_INITIALIZED                = -140,    //!< The Stereo part of NVAPI failed to initialize completely. Check if the stereo driver is installed.
    NVAPI_STEREO_REGISTRY_ACCESS_FAILED         = -141,    //!< Access to stereo-related registry keys or values has failed.
    NVAPI_STEREO_REGISTRY_PROFILE_TYPE_NOT_SUPPORTED = -142, //!< The given registry profile type is not supported.
    NVAPI_STEREO_REGISTRY_VALUE_NOT_SUPPORTED   = -143,    //!< The given registry value is not supported.
    NVAPI_STEREO_NOT_ENABLED                    = -144,    //!< Stereo is not enabled and the function needed it to execute completely.
    NVAPI_STEREO_NOT_TURNED_ON                  = -145,    //!< Stereo is not turned on and the function needed it to execute completely.
    NVAPI_STEREO_INVALID_DEVICE_INTERFACE       = -146,    //!< Invalid device interface.
    NVAPI_STEREO_PARAMETER_OUT_OF_RANGE         = -147,    //!< Separation percentage or JPEG image capture quality is out of [0-100] range.
    NVAPI_STEREO_FRUSTUM_ADJUST_MODE_NOT_SUPPORTED = -148, //!< The given frustum adjust mode is not supported.
    NVAPI_TOPO_NOT_POSSIBLE                     = -149,    //!< The mosaic topology is not possible given the current state of the hardware.
    NVAPI_MODE_CHANGE_FAILED                    = -150,    //!< An attempt to do a display resolution mode change has failed.        
    NVAPI_D3D11_LIBRARY_NOT_FOUND               = -151,    //!< d3d11.dll/d3d11_beta.dll cannot be loaded.
    NVAPI_INVALID_ADDRESS                       = -152,    //!< Address is outside of valid range.
    NVAPI_STRING_TOO_SMALL                      = -153,    //!< The pre-allocated string is too small to hold the result.
    NVAPI_MATCHING_DEVICE_NOT_FOUND             = -154,    //!< The input does not match any of the available devices.
    NVAPI_DRIVER_RUNNING                        = -155,    //!< Driver is running.
    NVAPI_DRIVER_NOTRUNNING                     = -156,    //!< Driver is not running.
    NVAPI_ERROR_DRIVER_RELOAD_REQUIRED          = -157,    //!< A driver reload is required to apply these settings.
    NVAPI_SET_NOT_ALLOWED                       = -158,    //!< Intended setting is not allowed.
    NVAPI_ADVANCED_DISPLAY_TOPOLOGY_REQUIRED    = -159,    //!< Information can't be returned due to "advanced display topology".
    NVAPI_SETTING_NOT_FOUND                     = -160,    //!< Setting is not found.
    NVAPI_SETTING_SIZE_TOO_LARGE                = -161,    //!< Setting size is too large.
    NVAPI_TOO_MANY_SETTINGS_IN_PROFILE          = -162,    //!< There are too many settings for a profile. 
    NVAPI_PROFILE_NOT_FOUND                     = -163,    //!< Profile is not found.
    NVAPI_PROFILE_NAME_IN_USE                   = -164,    //!< Profile name is duplicated.
    NVAPI_PROFILE_NAME_EMPTY                    = -165,    //!< Profile name is empty.
    NVAPI_EXECUTABLE_NOT_FOUND                  = -166,    //!< Application not found in the Profile.
    NVAPI_EXECUTABLE_ALREADY_IN_USE             = -167,    //!< Application already exists in the other profile.
    NVAPI_DATATYPE_MISMATCH                     = -168,    //!< Data Type mismatch 
    NVAPI_PROFILE_REMOVED                       = -169,    //!< The profile passed as parameter has been removed and is no longer valid.
    NVAPI_UNREGISTERED_RESOURCE                 = -170,    //!< An unregistered resource was passed as a parameter. 
    NVAPI_ID_OUT_OF_RANGE                       = -171,    //!< The DisplayId corresponds to a display which is not within the normal outputId range.
    NVAPI_DISPLAYCONFIG_VALIDATION_FAILED       = -172,    //!< Display topology is not valid so the driver cannot do a mode set on this configuration.
    NVAPI_DPMST_CHANGED                         = -173,    //!< Display Port Multi-Stream topology has been changed.
    NVAPI_INSUFFICIENT_BUFFER                   = -174,    //!< Input buffer is insufficient to hold the contents.    
    NVAPI_ACCESS_DENIED                         = -175,    //!< No access to the caller.
    NVAPI_MOSAIC_NOT_ACTIVE                     = -176,    //!< The requested action cannot be performed without Mosaic being enabled.
    NVAPI_SHARE_RESOURCE_RELOCATED              = -177,    //!< The surface is relocated away from video memory.
    NVAPI_REQUEST_USER_TO_DISABLE_DWM           = -178,    //!< The user should disable DWM before calling NvAPI.
    NVAPI_D3D_DEVICE_LOST                       = -179,    //!< D3D device status is D3DERR_DEVICELOST or D3DERR_DEVICENOTRESET - the user has to reset the device.
    NVAPI_INVALID_CONFIGURATION                 = -180,    //!< The requested action cannot be performed in the current state.
    NVAPI_STEREO_HANDSHAKE_NOT_DONE             = -181,    //!< Call failed as stereo handshake not completed.
    NVAPI_EXECUTABLE_PATH_IS_AMBIGUOUS          = -182,    //!< The path provided was too short to determine the correct NVDRS_APPLICATION
} NvStatus;
 
#define NVAPI_API_NOT_INTIALIZED NVAPI_API_NOT_INITIALIZED       //!< Fix typo in error code
#define NVAPI_INVALID_USER_PRIVILEDGE NVAPI_INVALID_USER_PRIVILEGE    //!< Fix typo in error code

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Initialize
//
//! This function initializes the NvAPI library. 
//! This must be called before calling other NvAPI_ functions. 
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since 
//!  Version: 82.61
//!
//! \retval  NVAPI_ERROR            An error occurred during the initialization process (generic error)
//! \retval  NVAPI_LIBRARYNOTFOUND  Failed to load the NVAPI support library
//! \retval  NVAPI_OK               Initialized
//! \sa nvapistatus
//! \ingroup nvapifunctions
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_Initialize();
typedef NvStatus (__cdecl *P_NvAPI_Initialize)();
P_NvAPI_Initialize NvAPI_Initialize;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Unload
//
//!   DESCRIPTION: Unloads NVAPI library. This must be the last function called. 
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
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
typedef NvStatus (__cdecl *P_NvAPI_Unload)();
P_NvAPI_Unload NvAPI_Unload;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetErrorMessage
//
//! This function converts an NvAPI error code into a null terminated string.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since 
//!  Version: 82.61
//!
//! \param nr      The error code to convert
//! \param szDesc  The string corresponding to the error code
//!
//! \return NULL terminated string (always, never NULL)
//! \ingroup nvapifunctions
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GetErrorMessage(NvStatus nr, NvAPI_ShortString szDesc);
typedef NvStatus (__cdecl *P_NvAPI_GetErrorMessage)(NvStatus nr, NvAPI_ShortString szDesc);
P_NvAPI_GetErrorMessage NvAPI_GetErrorMessage;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetInterfaceVersionString
//
//! This function returns a string describing the version of the NvAPI library.
//!               The contents of the string are human readable.  Do not assume a fixed
//!                format.
//!
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since 
//!  Version: 82.61
//!
//! \param  szDesc User readable string giving NvAPI version information
//!
//! \return See \ref nvapistatus for the list of possible return values.
//! \ingroup nvapifunctions
///////////////////////////////////////////////////////////////////////////////
typedef NvStatus (__cdecl *P_NvAPI_GetInterfaceVersionString)(NvAPI_ShortString szDesc);
P_NvAPI_GetInterfaceVersionString NvAPI_GetInterfaceVersionString;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetDisplayDriverVersion
//! \fn NvAPI_GetDisplayDriverVersion(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION *pVersion)
//! This function returns a struct that describes aspects of the display driver
//!                build.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since
//!  Version: 82.61
//!
//! \param [in]  hNvDisplay NVIDIA display handle.
//! \param [out] pVersion Pointer to NV_DISPLAY_DRIVER_VERSION struc
//!
//! \retval NVAPI_ERROR
//! \retval NVAPI_OK
///////////////////////////////////////////////////////////////////////////////

//! \ingroup driverapi
//! Used in NvAPI_GetDisplayDriverVersion()
typedef struct 
{
    NvU32              Version;             // Structure version
    NvU32              drvVersion;           
    NvU32              bldChangeListNum;     
    NvAPI_ShortString  szBuildBranchString; 
    NvAPI_ShortString  szAdapterString;
} NV_DISPLAY_DRIVER_VERSION;

#define NV_DISPLAY_DRIVER_VERSION_VER  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_VERSION, 1)

typedef NvStatus (__cdecl *P_NvAPI_GetDisplayDriverVersion)(NvDisplayHandle hNvDisplay, NV_DISPLAY_DRIVER_VERSION *pVersion);
P_NvAPI_GetDisplayDriverVersion NvAPI_GetDisplayDriverVersion;


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_SYS_GetDriverAndBranchVersion
//
//!   DESCRIPTION: This API returns display driver version and driver-branch string.
//!
//  SUPPORTED OS: Windows XP and higher
//! 
//! \param [out]  pDriverVersion         Contains the driver version after successful return.
//! \param [out]  szBuildBranchString    Contains the driver-branch string after successful return.
//!
//! \retval ::NVAPI_INVALID_ARGUMENT: either pDriverVersion is NULL or enum index too big
//! \retval ::NVAPI_OK - completed request
//! \retval ::NVAPI_API_NOT_INTIALIZED - NVAPI not initialized
//! \retval ::NVAPI_ERROR - miscellaneous error occurred
//! 
//! \ingroup driverapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SYS_GetDriverAndBranchVersion(NvU32* pDriverVersion, NvAPI_ShortString szBuildBranchString);



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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since
//!   Version: 82.61
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
typedef NvStatus (__cdecl *P_NvAPI_EnumNvidiaDisplayHandle)(NvU32 thisEnum, NvDisplayHandle *pNvDispHandle);
P_NvAPI_EnumNvidiaDisplayHandle NvAPI_EnumNvidiaDisplayHandle;

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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since
//!  Version: 82.61
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
// FUNCTION NAME: NvAPI_EnumPhysicalGPUs
//
//! This function returns an array of physical GPU handles.
//! Each handle represents a physical GPU present in the system.
//! That GPU may be part of an SLI configuration, or may not be visible to the OS directly.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//! The array nvGPUHandle will be filled with physical GPU handle values. The returned
//! gpuCount determines how many entries in the array are valid.
//!
//! \note In drivers older than 105.00, all physical GPU handles get invalidated on a
//!       modeset. So the calling applications need to renum the handles after every modeset.\n
//!       With drivers 105.00 and up, all physical GPU handles are constant.
//!       Physical GPU handles are constant as long as the GPUs are not physically moved and 
//!       the SBIOS VGA order is unchanged.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \par Introduced in
//!  Version: 82.61
//!
//! \retval NVAPI_INVALID_ARGUMENT         nvGPUHandle or pGpuCount is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_EnumPhysicalGPUs(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
typedef NvStatus (__cdecl *P_NvAPI_EnumPhysicalGPUs)(NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
P_NvAPI_EnumPhysicalGPUs NvAPI_EnumPhysicalGPUs;

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
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since 
//!  Version: 82.61
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since 
//!  Version: 82.61
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvDisp is not valid; nvGPUHandle or pGpuCount is NULL
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  no NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GetPhysicalGPUsFromDisplay(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
typedef NvStatus (__cdecl *P_NvAPI_GetPhysicalGPUsFromDisplay)(NvDisplayHandle hNvDisp, NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS], NvU32 *pGpuCount);
P_NvAPI_GetPhysicalGPUsFromDisplay NvAPI_GetPhysicalGPUsFromDisplay;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetPhysicalGPUFromUnAttachedDisplay
//
//! This function returns a physical GPU handle associated with the specified unattached display.
//! The source GPU is a physical render GPU which renders the frame buffer but may or may not drive the scan out.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since
//!  Version: 82.61
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvUnAttachedDisp is not valid or pPhysicalGpu is NULL.
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetPhysicalGPUFromUnAttachedDisplay(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvPhysicalGpuHandle *pPhysicalGpu);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_CreateDisplayFromUnAttachedDisplay
//
//! This function converts the unattached display handle to an active attached display handle.
//!
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 82.61
//!
//! \retval NVAPI_INVALID_ARGUMENT         hNvUnAttachedDisp is not valid or pNvDisplay is NULL.
//! \retval NVAPI_OK                       One or more handles were returned
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_CreateDisplayFromUnAttachedDisplay(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvDisplayHandle *pNvDisplay);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetLogicalGPUFromDisplay
//
//! This function returns the logical GPU handle associated with the specified display.
//! At least one GPU must be present in the system and running an NVIDIA display driver.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 82.61
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
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 82.61
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 82.61
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
// FUNCTION NAME: NvAPI_GetAssociatedNVidiaDisplayHandle
//
//!  This function returns the handle of the NVIDIA display that is associated
//!  with the given display "name" (such as "\\.\DISPLAY1").
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 82.61
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 187.07
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 82.61
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 97.60, 100.06
//!
//! \retval NVAPI_INVALID_ARGUMENT          Either argument is NULL
//! \retval NVAPI_OK                       *pNvDispHandle is now valid
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND   No NVIDIA device maps to that display name
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetUnAttachedAssociatedDisplayName(NvUnAttachedDisplayHandle hNvUnAttachedDisp, NvAPI_ShortString szDisplayName);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetAssociatedDisplayOutputId
//
//! This function gets the active outputId associated with the display handle.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 93.60
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

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetDisplayPortInfo
//
//! \fn NvAPI_GetDisplayPortInfo(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_DISPLAY_PORT_INFO *pInfo)
//! DESCRIPTION:     This function returns the current DisplayPort-related information on the specified device (monitor).
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 165.22
//!
//! \param [in]     hNvDisplay    NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from 
//!                               NvAPI_EnumNVidiaDisplayHandle().
//! \param [in]     outputId      The display output ID. If "0", then the default outputId from
//!                               NvAPI_GetAssociatedDisplayOutputId() will be used. See \ref handles.
//! \param [out]    pInfo         The DisplayPort information
//!
//! \retval         NVAPI_OK                Completed request
//! \retval         NVAPI_ERROR             Miscellaneous error occurred
//! \retval         NVAPI_INVALID_ARGUMENT  Invalid input parameter.
//
///////////////////////////////////////////////////////////////////////////////


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum
{
    NV_DP_1_62GBPS            = 6,
    NV_DP_2_70GBPS            = 0xA,
} NV_DP_LINK_RATE;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum
{
    NV_DP_1_LANE              = 1,
    NV_DP_2_LANE              = 2,
    NV_DP_4_LANE              = 4,
} NV_DP_LANE_COUNT;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum
{
    NV_DP_COLOR_FORMAT_RGB     = 0,
    NV_DP_COLOR_FORMAT_YCbCr422,
    NV_DP_COLOR_FORMAT_YCbCr444,
} NV_DP_COLOR_FORMAT;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum
{
    NV_DP_COLORIMETRY_RGB     = 0,
    NV_DP_COLORIMETRY_YCbCr_ITU601,
    NV_DP_COLORIMETRY_YCbCr_ITU709,
} NV_DP_COLORIMETRY;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum
{
    NV_DP_DYNAMIC_RANGE_VESA  = 0,
    NV_DP_DYNAMIC_RANGE_CEA,
} NV_DP_DYNAMIC_RANGE;


//! \ingroup dispcontrol
//! Used in NV_DISPLAY_PORT_INFO.
typedef enum
{
    NV_DP_BPC_DEFAULT         = 0,
    NV_DP_BPC_6,
    NV_DP_BPC_8,
    NV_DP_BPC_10,
    NV_DP_BPC_12,
    NV_DP_BPC_16,
} NV_DP_BPC;


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SetDisplayPort
//
//! \fn NvAPI_SetDisplayPort(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_DISPLAY_PORT_CONFIG *pCfg)
//! DESCRIPTION:     This function sets up DisplayPort-related configurations.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version:   165.22
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
typedef struct
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


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetHDMISupportInfo
//
//! \fn NvAPI_GetHDMISupportInfo(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_HDMI_SUPPORT_INFO *pInfo)
//!   This API returns the current infoframe data on the specified device(monitor).
//!
//  SUPPORTED OS: Windows Vista and higher
//!
//! \since Version: 97.00
//!
//! \param [in]  hvDisplay  NVIDIA Display selection. It can be NVAPI_DEFAULT_HANDLE or a handle enumerated from NvAPI_EnumNVidiaDisplayHandle().
//! \param [in]  outputId   The display output id. If it's "0" then the default outputId from NvAPI_GetAssociatedDisplayOutputId() will be used. See \ref handles.
//! \param [out] pInfo      The monitor and GPU's HDMI support info
//!
//! \retval  NVAPI_OK                Completed request
//! \retval  NVAPI_ERROR             Miscellaneous error occurred
//! \retval  NVAPI_INVALID_ARGUMENT  Invalid input parameter.
///////////////////////////////////////////////////////////////////////////////

//! \ingroup dispcontrol
//! Used in NvAPI_GetHDMISupportInfo().
typedef struct
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
    NvU32      EDID861ExtRev;               //!< Revision number of the EDID 861 extension
 } NV_HDMI_SUPPORT_INFO; 


//! \ingroup dispcontrol
#define NV_HDMI_SUPPORT_INFO_VER  MAKE_NVAPI_VERSION(NV_HDMI_SUPPORT_INFO,1)



//! \ingroup dispcontrol
NVAPI_INTERFACE NvAPI_GetHDMISupportInfo(NvDisplayHandle hNvDisplay, NvU32 outputId, NV_HDMI_SUPPORT_INFO *pInfo);


//! \ingroup dispcontrol
//! @{
///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_Disp_InfoFrameControl
//
//! \fn NvAPI_Disp_InfoFrameControl(NvU32 displayId, NV_INFOFRAME_DATA *pInfoframeData)
//! DESCRIPTION:     This API controls the InfoFrame values.
//!
//  SUPPORTED OS: Windows Vista and higher
//!
//! \param [in]     displayId         Monitor Identifier
//! \param [in,out] pInfoframeData    Contains data corresponding to InfoFrame
//!                  
//! \return 
//! ::NVAPI_OK,  
//! ::NVAPI_ERROR,  
//! ::NVAPI_INVALID_ARGUMENT
//
///////////////////////////////////////////////////////////////////////////////

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

NVAPI_INTERFACE NvAPI_Disp_InfoFrameControl(NvU32 displayId, NV_INFOFRAME_DATA *pInfoframeData);

//! @}



//! \ingroup dispcontrol
//! @{
///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_Disp_ColorControl
//
//! \fn NvAPI_Disp_ColorControl(NvU32 displayId, NV_COLOR_DATA *pColorData)
//! DESCRIPTION:    This API controls the Color values.
//!
//  SUPPORTED OS: Windows Vista and higher
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


//-----------------------------------------------------------------------------
// DirectX APIs
//-----------------------------------------------------------------------------


//! \ingroup dx
//! Used in NvAPI_D3D10_GetCurrentSLIState(), and NvAPI_D3D_GetCurrentSLIState().
typedef struct
{
    NvU32 version;                    //!< Structure version
    NvU32 maxNumAFRGroups;            //!< [OUT] The maximum possible value of numAFRGroups
    NvU32 numAFRGroups;               //!< [OUT] The number of AFR groups enabled in the system
    NvU32 currentAFRIndex;            //!< [OUT] The AFR group index for the frame currently being rendered
    NvU32 nextFrameAFRIndex;          //!< [OUT] What the AFR group index will be for the next frame (i.e. after calling Present)
    NvU32 previousFrameAFRIndex;      //!< [OUT] The AFR group index that was used for the previous frame (~0 if more than one frame has not been rendered yet)
    NvU32 bIsCurAFRGroupNew;          //!< [OUT] Boolean: Is this frame the first time running on the current AFR group

} NV_GET_CURRENT_SLI_STATE;

//! \ingroup dx
#define NV_GET_CURRENT_SLI_STATE_VER  MAKE_NVAPI_VERSION(NV_GET_CURRENT_SLI_STATE,1)




#if defined(_D3D9_H_) || defined(__d3d10_h__) || defined(__d3d11_h__)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_D3D_GetCurrentSLIState
//
//! DESCRIPTION:     This function returns the current SLI state for the specified device.  The structure
//!                  contains the number of AFR groups, the current AFR group index,
//!                  and what the AFR group index will be for the next frame. \p
//!                  pDevice can be either a IDirect3DDevice9 or ID3D10Device pointer.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 174.80
//!
//! \retval         NVAPI_OK     Completed request
//! \retval         NVAPI_ERROR  Error occurred
//!
//! \ingroup  dx
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_D3D_GetCurrentSLIState(IUnknown *pDevice, NV_GET_CURRENT_SLI_STATE *pSliState);

#endif //if defined(_D3D9_H_) || defined(__d3d10_h__) || defined(__d3d11_h__)



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetGpuCoreCount
//
//!   DESCRIPTION: Retrieves the total number of cores defined for a GPU.
//!                Returns 0 on architectures that don't define GPU cores.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \retval ::NVAPI_INVALID_ARGUMENT              pCount is NULL
//! \retval ::NVAPI_OK                            *pCount is set
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND       no NVIDIA GPU driving a display was found
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \retval ::NVAPI_NOT_SUPPORTED                 API call is not supported on current architecture
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetGpuCoreCount(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pCount);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAllOutputs
//
//!  This function returns set of all GPU-output identifiers as a bitmask.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 87.00
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL.
//! \retval   NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetAllOutputs(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pOutputsMask);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedOutputs
//
//! This function is the same as NvAPI_GPU_GetAllOutputs() but returns only the set of GPU output 
//! identifiers that are connected to display devices.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 82.61
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL.
//! \retval   NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 171.14
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval   NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//! 
//! \ingroup gpu  
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetConnectedSLIOutputs(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);




//! \ingroup gpu
typedef enum
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 97.20
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval  NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetConnectedOutputsWithLidState(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetConnectedSLIOutputsWithLidState
//
//!   DESCRIPTION: This function is the same as NvAPI_GPU_GetConnectedOutputsWithLidState() but returns only the set
//!                of GPU-output identifiers that can be selected in an SLI configuration. With SLI disabled,
//!                this function matches NvAPI_GPU_GetConnectedOutputsWithLidState().
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 171.14
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval  NVAPI_OK                           *pOutputsMask contains a set of GPU-output identifiers
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetConnectedSLIOutputsWithLidState(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pOutputsMask);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetSystemType
//
//! \fn NvAPI_GPU_GetSystemType(NvPhysicalGpuHandle hPhysicalGpu, NV_SYSTEM_TYPE *pSystemType)
//!  This function identifies whether the GPU is a notebook GPU or a desktop GPU.
//!       
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 97.20
//!         
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pOutputsMask is NULL
//! \retval  NVAPI_OK                           *pSystemType contains the GPU system type
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup gpu
//! Used in NvAPI_GPU_GetSystemType()
typedef enum
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 87.10
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
// FUNCTION NAME: NvAPI_GPU_GetEDID
//
//! \fn NvAPI_GPU_GetEDID(NvPhysicalGpuHandle hPhysicalGpu, NvU32 displayOutputId, NV_EDID *pEDID)
//!  This function returns the EDID data for the specified GPU handle and connection bit mask.
//!  displayOutputId should have exactly 1 bit set to indicate a single display. See \ref handles.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 88.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 100.40
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
//  SUPPORTED OS: Windows XP and higher
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 87.10
//!
//! \retval  NVAPI_OK                            Combination of outputs in outputsMask are valid (can be active simultaneously).
//! \retval  NVAPI_INVALID_COMBINATION           Combination of outputs in outputsMask are NOT valid.
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or outputsMask does not have at least 2 bits set.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_ValidateOutputCombination(NvPhysicalGpuHandle hPhysicalGpu, NvU32 outputsMask);




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
    NVAPI_GPU_CONNECTOR_UNKNOWN                         = 0xFFFFFFFF,
} NV_GPU_CONNECTOR_TYPE;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetFullName
//
//!  This function retrieves the full GPU name as an ASCII string - for example, "Quadro FX 1400".
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \return  NVAPI_ERROR or NVAPI_OK
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetFullName(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);
typedef NvStatus (__cdecl *P_NvAPI_GPU_GetFullName)(NvPhysicalGpuHandle hPhysicalGpu, NvAPI_ShortString szName);
P_NvAPI_GPU_GetFullName NvAPI_GetFullName;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPCIIdentifiers
//
//!  This function returns the PCI identifiers associated with this GPU.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 92.10
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
NVAPI_INTERFACE NvAPI_GPU_GetPCIIdentifiers(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pDeviceId,NvU32 *pSubSystemId,NvU32 *pRevisionId,NvU32 *pExtDeviceId);
    


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
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 174.32
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu 
//! \retval  NVAPI_OK                           *pGpuType contains the GPU type 
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found 
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE: hPhysicalGpu was not a physical GPU handle 
//!
//!  \ingroup gpu 
///////////////////////////////////////////////////////////////////////////////     
NVAPI_INTERFACE NvAPI_GPU_GetGPUType(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_TYPE *pGpuType);




//! \ingroup gpu
//! Used in NvAPI_GPU_GetBusType()
typedef enum _NV_GPU_BUS_TYPE
{
    NVAPI_GPU_BUS_TYPE_UNDEFINED    = 0,
    NVAPI_GPU_BUS_TYPE_PCI          = 1,
    NVAPI_GPU_BUS_TYPE_AGP          = 2,
    NVAPI_GPU_BUS_TYPE_PCI_EXPRESS  = 3,
    NVAPI_GPU_BUS_TYPE_FPCI         = 4,
} NV_GPU_BUS_TYPE;
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusType
//
//!  This function returns the type of bus associated with this GPU.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval NVAPI_INVALID_ARGUMENT             hPhysicalGpu or pBusType is NULL.
//! \retval NVAPI_OK                          *pBusType contains bus identifier.
//! \retval NVAPI_NVIDIA_DEVICE_NOT_FOUND      No NVIDIA GPU driving a display was found.
//! \retval NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetBusType(NvPhysicalGpuHandle hPhysicalGpu,NV_GPU_BUS_TYPE *pBusType);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusId
//
//!   DESCRIPTION: Returns the ID of the bus associated with this GPU.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 167.00
//!
//!  \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBusId is NULL.
//!  \retval  NVAPI_OK                           *pBusId contains the bus ID.
//!  \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//!  \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//!
//!  \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetBusId(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pBusId);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBusSlotId
//
//!   DESCRIPTION: Returns the ID of the bus slot associated with this GPU.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 167.00
//!
//!  \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBusSlotId is NULL.
//!  \retval  NVAPI_OK                           *pBusSlotId contains the bus slot ID.
//!  \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//!  \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//!
//!  \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetBusSlotId(NvPhysicalGpuHandle hPhysicalGpu, NvU32 *pBusSlotId);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetIRQ
//
//!  This function returns the interrupt number associated with this GPU.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval  NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pIRQ is NULL.
//! \retval  NVAPI_OK                           *pIRQ contains interrupt number.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetIRQ(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pIRQ);
    
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosRevision
//
//!  This function returns the revision of the video BIOS associated with this GPU.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval    NVAPI_INVALID_ARGUMENT               hPhysicalGpu or pBiosRevision is NULL.
//! \retval    NVAPI_OK                            *pBiosRevision contains revision number.
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND        No NVIDIA GPU driving a display was found.
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE   hPhysicalGpu was not a physical GPU handle.
//! \ingroup   gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosRevision(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosOEMRevision
//
//!  This function returns the OEM revision of the video BIOS associated with this GPU.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval    NVAPI_INVALID_ARGUMENT              hPhysicalGpu or pBiosRevision is NULL
//! \retval    NVAPI_OK                           *pBiosRevision contains revision number
//! \retval    NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval    NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup   gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosOEMRevision(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pBiosRevision);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVbiosVersionString
//
//!  This function returns the full video BIOS version string in the form of xx.xx.xx.xx.yy where
//!  - xx numbers come from NvAPI_GPU_GetVbiosRevision() and 
//!  - yy comes from NvAPI_GPU_GetVbiosOEMRevision().
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval   NVAPI_INVALID_ARGUMENT              hPhysicalGpu is NULL.
//! \retval   NVAPI_OK                            szBiosRevision contains version string.
//! \retval   NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval   NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVbiosVersionString(NvPhysicalGpuHandle hPhysicalGpu,NvAPI_ShortString szBiosRevision);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetAGPAperture
//
//!  This function returns the AGP aperture in megabytes.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
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
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
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
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pWidth is NULL.
//! \retval  NVAPI_OK                            Call successful.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetCurrentPCIEDownstreamWidth(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pWidth);



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetPhysicalFrameBufferSize
//
//!   This function returns the physical size of framebuffer in KB.  This does NOT include any
//!   system RAM that may be dedicated for use by the GPU.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pSize is NULL
//! \retval  NVAPI_OK                            Call successful
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetPhysicalFrameBufferSize(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pSize);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetVirtualFrameBufferSize
//
//!  This function returns the virtual size of framebuffer in KB.  This includes the physical RAM plus any
//!  system RAM that has been dedicated for use by the GPU.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 92.10
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pSize is NULL.
//! \retval  NVAPI_OK                            Call successful.
//! \retval  NVAPI_NVIDIA_DEVICE_NOT_FOUND       No NVIDIA GPU driving a display was found.
//! \retval  NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  hPhysicalGpu was not a physical GPU handle.
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_GetVirtualFrameBufferSize(NvPhysicalGpuHandle hPhysicalGpu,NvU32 *pSize);


 


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

//  SUPPORTED OS: Windows XP and higher
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GPU_GetBoardInfo
//
//!   DESCRIPTION: This API Retrieves the Board information (a unique GPU Board Serial Number) stored in the InfoROM.
//!
//! \param [in]      hPhysicalGpu       Physical GPU Handle.
//! \param [in,out]  NV_BOARD_INFO      Board Information.
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
NVAPI_INTERFACE NvAPI_GPU_GetBoardInfo(NvPhysicalGpuHandle hPhysicalGpu, NV_BOARD_INFO *pBoardInfo);






//! \ingroup gpupstate
#define NVAPI_MAX_GPU_UTILIZATIONS 8



//! \ingroup gpupstate
//! Used in NvAPI_GPU_GetDynamicPstatesInfoEx().
typedef struct
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
//!                There are currently 3 domains for which GPU utilization and dynamic P-State thresholds can be retrieved:
//!                   graphic engine (GPU), frame buffer (FB), and video engine (VID).
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since version 185.50
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
NVAPI_INTERFACE NvAPI_GPU_GetDynamicPstatesInfoEx(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_DYNAMIC_PSTATES_INFO_EX *pDynamicPstatesInfoEx);


///////////////////////////////////////////////////////////////////////////////////
//  Thermal API
//  Provides ability to get temperature levels from the various thermal sensors associated with the GPU

//! \ingroup gputhermal
#define NVAPI_MAX_THERMAL_SENSORS_PER_GPU 3

//! \ingroup gputhermal
//! Used in NV_GPU_THERMAL_SETTINGS
typedef enum 
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
typedef enum
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
typedef struct
{
    NvU32   version;                //!< structure version 
    NvU32   Count;                  //!< number of associated thermal sensors
    struct 
    {
        NV_THERMAL_CONTROLLER       controller;        //!< internal, ADM1032, MAX6649...
        NvU32                       defaultMinTemp;    //!< The min default temperature value of the thermal sensor in degrees centigrade 
        NvU32                       defaultMaxTemp;    //!< The max default temperature value of the thermal sensor in degrees centigrade 
        NvU32                       currentTemp;       //!< The current temperature value of the thermal sensor in degrees centigrade 
        NV_THERMAL_TARGET           target;            //!< Thermal sensor targeted @ GPU, memory, chipset, powersupply, Visual Computing Device, etc.
    } sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS_V1;

//! \ingroup gputhermal
typedef struct
{
    NvU32   Version;                //!< structure version
    NvU32   Count;                  //!< number of associated thermal sensors
    struct
    {
        NV_THERMAL_CONTROLLER       Controller;         //!< internal, ADM1032, MAX6649...
        NvS32                       DefaultMinTemp;     //!< Minimum default temperature value of the thermal sensor in degrees C
        NvS32                       DefaultMaxTemp;     //!< Maximum default temperature value of the thermal sensor in degrees C
        NvS32                       CurrentTemp;        //!< Current temperature value of the thermal sensor in degrees C
        NV_THERMAL_TARGET           Target;             //!< Thermal sensor targeted - GPU, memory, chipset, powersupply, Visual Computing Device, etc
    } Sensor[NVAPI_MAX_THERMAL_SENSORS_PER_GPU];

} NV_GPU_THERMAL_SETTINGS_V2;

//! \ingroup gputhermal
typedef NV_GPU_THERMAL_SETTINGS_V2  NV_GPU_THERMAL_SETTINGS;

//! Macro for constructing the version field of NV_GPU_THERMAL_SETTINGS_V1
#define NV_GPU_THERMAL_SETTINGS_VER_1   MAKE_NVAPI_VERSION(NV_GPU_THERMAL_SETTINGS_V1, 1)

//! Macro for constructing the version field of NV_GPU_THERMAL_SETTINGS_V2
#define NV_GPU_THERMAL_SETTINGS_VER_2   MAKE_NVAPI_VERSION(NV_GPU_THERMAL_SETTINGS_V2, 2)

//! Macro for constructing the version field of NV_GPU_THERMAL_SETTINGS
#define NV_GPU_THERMAL_SETTINGS_VER     NV_GPU_THERMAL_SETTINGS_VER_2

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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 87.60 (Rel85), 92.40 (Rel90)
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
typedef NvStatus (__cdecl *P_NvAPI_GPU_GetThermalSettings)(NvPhysicalGpuHandle hPhysicalGpu, NvU32 sensorIndex, NV_GPU_THERMAL_SETTINGS *pThermalSettings);
P_NvAPI_GPU_GetThermalSettings NvAPI_GetThermalSettings;


////////////////////////////////////////////////////////////////////////////////
//
// NvAPI_TVOutput Information
//
///////////////////////////////////////////////////////////////////////////////

//! \ingroup tvapi
//! Used in NV_DISPLAY_TV_OUTPUT_INFO
typedef enum _NV_DISPLAY_TV_FORMAT
{
    NV_DISPLAY_TV_FORMAT_NONE         = 0,
    NV_DISPLAY_TV_FORMAT_SD_NTSCM     = 0x00000001,
    NV_DISPLAY_TV_FORMAT_SD_NTSCJ     = 0x00000002,
    NV_DISPLAY_TV_FORMAT_SD_PALM      = 0x00000004,
    NV_DISPLAY_TV_FORMAT_SD_PALBDGH   = 0x00000008,
    NV_DISPLAY_TV_FORMAT_SD_PALN      = 0x00000010,
    NV_DISPLAY_TV_FORMAT_SD_PALNC     = 0x00000020,
    NV_DISPLAY_TV_FORMAT_SD_576i      = 0x00000100,
    NV_DISPLAY_TV_FORMAT_SD_480i      = 0x00000200,
    NV_DISPLAY_TV_FORMAT_ED_480p      = 0x00000400,
    NV_DISPLAY_TV_FORMAT_ED_576p      = 0x00000800,
    NV_DISPLAY_TV_FORMAT_HD_720p      = 0x00001000,
    NV_DISPLAY_TV_FORMAT_HD_1080i     = 0x00002000,
    NV_DISPLAY_TV_FORMAT_HD_1080p     = 0x00004000,
    NV_DISPLAY_TV_FORMAT_HD_720p50    = 0x00008000,
    NV_DISPLAY_TV_FORMAT_HD_1080p24   = 0x00010000,
    NV_DISPLAY_TV_FORMAT_HD_1080i50   = 0x00020000,
    NV_DISPLAY_TV_FORMAT_HD_1080p50   = 0x00040000,

    NV_DISPLAY_TV_FORMAT_SD_OTHER     = 0x01000000,
    NV_DISPLAY_TV_FORMAT_ED_OTHER     = 0x02000000,
    NV_DISPLAY_TV_FORMAT_HD_OTHER     = 0x04000000,

    NV_DISPLAY_TV_FORMAT_ANY          = 0x80000000,

} NV_DISPLAY_TV_FORMAT;

///////////////////////////////////////////////////////////////////////////////////
//  I2C API
//  Provides ability to read or write data using I2C protocol.
//  These APIs allow I2C access only to DDC monitors


//! \addtogroup i2capi
//! @{
#define NVAPI_MAX_SIZEOF_I2C_DATA_BUFFER 4096
#define NVAPI_MAX_SIZEOF_I2C_REG_ADDRESS 4
#define NVAPI_DISPLAY_DEVICE_MASK_MAX 24

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
    NvU32                   i2cSpeed;           //!< The target speed of the transaction (between 28kbps to 40kbps; not guaranteed).
} NV_I2C_INFO;

#define NV_I2C_INFO_VER  MAKE_NVAPI_VERSION(NV_I2C_INFO,1)
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
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 87.90
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
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 87.90
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




//! \addtogroup sysgeneral
//! @{

typedef struct
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

typedef struct
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

typedef enum
{
    NV_CHIPSET_INFO_HYBRID          = 0x00000001,
} NV_CHIPSET_INFO_FLAGS;

typedef struct
{
    NvU32               version;        //!< structure version
    NvU32               vendorId;       //!< vendor ID
    NvU32               deviceId;       //!< device ID
    NvAPI_ShortString   szVendorName;   //!< vendor Name
    NvAPI_ShortString   szChipsetName;  //!< device Name
    NvU32               flags;          //!< Chipset info flags
} NV_CHIPSET_INFO_v2;

typedef struct
{
    NvU32               version;        //structure version
    NvU32               vendorId;       //vendor ID
    NvU32               deviceId;       //device ID
    NvAPI_ShortString   szVendorName;   //vendor Name
    NvAPI_ShortString   szChipsetName;  //device Name
} NV_CHIPSET_INFO_v1;

#define NV_CHIPSET_INFO_VER_1  MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v1,1)
#define NV_CHIPSET_INFO_VER_2   MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v2,2)
#define NV_CHIPSET_INFO_VER_3   MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v3,3)
#define NV_CHIPSET_INFO_VER_4   MAKE_NVAPI_VERSION(NV_CHIPSET_INFO_v4,4)

#define NV_CHIPSET_INFO         NV_CHIPSET_INFO_v4
#define NV_CHIPSET_INFO_VER     NV_CHIPSET_INFO_VER_4

//! @}




///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_SYS_GetChipSetInfo
//
//!  This function returns information about the system's chipset.
//!
//  SUPPORTED OS: Mac OS X, Windows XP and higher
//!
//! \since Version: 96.60
//!
//! \retval  NVAPI_INVALID_ARGUMENT              pChipSetInfo is NULL.
//! \retval  NVAPI_OK                           *pChipSetInfo is now set.
//! \retval  NVAPI_INCOMPATIBLE_STRUCT_VERSION   NV_CHIPSET_INFO version not compatible with driver.
//! \ingroup sysgeneral
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SYS_GetChipSetInfo(NV_CHIPSET_INFO *pChipSetInfo);



//! \ingroup sysgeneral
//! Lid and dock information - used in NvAPI_GetLidDockInfo()
typedef struct 
{
    NvU32 version;    //! Structure version, constructed from the macro #NV_LID_DOCK_PARAMS_VER
    NvU32 currentLidState;
    NvU32 currentDockState;
    NvU32 currentLidPolicy;
    NvU32 currentDockPolicy;
    NvU32 forcedLidMechanismPresent;
    NvU32 forcedDockMechanismPresent;
}NV_LID_DOCK_PARAMS;


//! ingroup sysgeneral
#define NV_LID_DOCK_PARAMS_VER  MAKE_NVAPI_VERSION(NV_LID_DOCK_PARAMS,1)



///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_GetLidDockInfo
//
//! DESCRIPTION: This function returns the current lid and dock information.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 177.61
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 84.11 (Rel80) / 88.00 (Rel85)
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
//  SUPPORTED OS: Windows XP and higher
NVAPI_INTERFACE NvAPI_OGL_ExpertModeSet(NvU32 expertDetailLevel,
                                        NvU32 expertReportMask,
                                        NvU32 expertOutputMask,
                     NVAPI_OGLEXPERT_CALLBACK expertCallback);

//! \addtogroup oglapi
//  SUPPORTED OS: Windows XP and higher
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 84.11 (Rel80) / 88.00 (Rel85)
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
//  SUPPORTED OS: Windows XP and higher
NVAPI_INTERFACE NvAPI_OGL_ExpertModeDefaultsSet(NvU32 expertDetailLevel,
                                                NvU32 expertReportMask,
                                                NvU32 expertOutputMask);

//! \addtogroup oglapi
//  SUPPORTED OS: Windows XP and higher
NVAPI_INTERFACE NvAPI_OGL_ExpertModeDefaultsGet(NvU32 *pExpertDetailLevel,
                                                NvU32 *pExpertReportMask,
                                                NvU32 *pExpertOutputMask);
//@}


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


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_DISP_GetMonitorCapabilities
//
//! \fn NvAPI_DISP_GetMonitorCapabilities(NvU32 displayId, NV_MONITOR_CAPABILITIES *pMonitorCapabilities)
//! DESCRIPTION:     This API returns the Monitor capabilities 
//!
//  SUPPORTED OS: Windows Vista and higher
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

//  SUPPORTED OS: Windows Vista and higher
//! \ingroup dispcontrol
NVAPI_INTERFACE NvAPI_DISP_GetMonitorCapabilities(NvU32 displayId, NV_MONITOR_CAPABILITIES *pMonitorCapabilities);



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
//  SUPPORTED OS: Windows Vista and higher
//!
//! \since Version: 90.18
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
NVAPI_INTERFACE NvAPI_SetView(NvDisplayHandle hNvDisplay, NV_VIEW_TARGET_INFO *pTargetInfo, NV_TARGET_VIEW_MODE targetView);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetView
//
//! This API lets caller retrieve the target display arrangement for selected source display handle.
//! \note Display PATH with this API is limited to single GPU. DUALVIEW across GPUs will be returned as STANDARD VIEW. 
//!       Use NvAPI_SYS_GetDisplayTopologies() to query views across GPUs.
//!
//  SUPPORTED OS: Windows Vista and higher
//!
//! \since Version: 86.90
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
NVAPI_INTERFACE NvAPI_GetView(NvDisplayHandle hNvDisplay, NV_VIEW_TARGET_INFO *pTargets, NvU32 *pTargetMaskCount, NV_TARGET_VIEW_MODE *pTargetView);

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
//  SUPPORTED OS: Windows Vista and higher
//!
//! \since Version: 97.20
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
} NV_DISPLAY_PATH_INFO; 

//! \addtogroup dispcontrol
//! Macro for constructing the version fields of NV_DISPLAY_PATH_INFO
//! @{
#define NV_DISPLAY_PATH_INFO_VER  NV_DISPLAY_PATH_INFO_VER3
#define NV_DISPLAY_PATH_INFO_VER3 MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,3)
#define NV_DISPLAY_PATH_INFO_VER2 MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,2)
#define NV_DISPLAY_PATH_INFO_VER1 MAKE_NVAPI_VERSION(NV_DISPLAY_PATH_INFO,1)
//! @}


//! \ingroup dispcontrol
NVAPI_INTERFACE NvAPI_SetViewEx(NvDisplayHandle hNvDisplay, NV_DISPLAY_PATH_INFO *pPathInfo, NV_TARGET_VIEW_MODE displayView);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetViewEx
//
//! DESCRIPTION:    This API lets caller retrieve the target display arrangement for selected source display handle.
//!                 \note Display PATH with this API is limited to single GPU. DUALVIEW across GPUs will be returned as STANDARD VIEW. 
//!                       Use NvAPI_SYS_GetDisplayTopologies() to query views across GPUs.
//!
//  SUPPORTED OS: Windows Vista and higher
//!
//! \since Version: 165.02
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
NVAPI_INTERFACE NvAPI_GetViewEx(NvDisplayHandle hNvDisplay, NV_DISPLAY_PATH_INFO *pPathInfo, NvU32 *pPathCount, NV_TARGET_VIEW_MODE *pTargetViewMode);

///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_GetSupportedViews
//
//!  This API lets caller enumerate all the supported NVIDIA display views - nView and Dualview modes.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 86.90
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

#define NV_MOSAIC_MAX_DISPLAYS      (64)

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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 185.20
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 185.20
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 185.20
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 185.20
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 185.20
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


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_EnableCurrentTopo
//
//! DESCRIPTION:     This API enables or disables the current Mosaic topology
//!                  based on the setting of the incoming 'enable' parameter.
//!
//!                  An "enable" setting enables the current (previously set) Mosaic topology.
//!                  Note that when the current Mosaic topology is retrieved, it must have an isPossible value of 1 or
//!                  an error will occur.
//!
//!                  A "disable" setting disables the current Mosaic topology.
//!                  The topology information will persist, even across reboots.
//!                  To re-enable the Mosaic topology, call this function
//!                  again with the enable parameter set to 1.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 185.20
//!
//! \param [in]   enable               1 to enable the current Mosaic topo, 0 to disable it.
//!
//! \retval ::NVAPI_OK                 The Mosaic topo was enabled/disabled.
//! \retval ::NVAPI_NOT_SUPPORTED      Mosaic is not supported with the existing hardware.
//! \retval ::NVAPI_INVALID_ARGUMENT   One or more arguments passed in are invalid.
//! \retval ::NVAPI_TOPO_NOT_POSSIBLE  The current topology is not currently possible.
//! \retval ::NVAPI_MODE_CHANGE_FAILED There was an error changing the display mode.
//! \retval ::NVAPI_ERROR:             Miscellaneous error occurred.
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_EnableCurrentTopo(NvU32 enable);


//  SUPPORTED OS: Windows Vista and higher
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_Mosaic_GetDisplayViewportsByResolution
//
//! DESCRIPTION:     This API returns the viewports that would be applied on
//!                  the requested display.
//!
//! \param [in]      displayId       Display ID of a single display in the active
//!                                  mosaic topology to query.
//! \param [in]      srcWidth        Width of full display topology. If both
//!                                  width and height are 0, the current
//!                                  resolution is used.
//! \param [in]      srcHeight       Height of full display topology. If both
//!                                  width and height are 0, the current
//!                                  resolution is used.
//! \param [out]     viewports       Array of NV_RECT viewports which represent
//!                                  the displays as identified in
//!                                  NvAPI_Mosaic_EnumGridTopologies. If the
//!                                  requested resolution is a single-wide
//!                                  resolution, only viewports[0] will
//!                                  contain the viewport details, regardless
//!                                  of which display is driving the display.
//! \param [out]     bezelCorrected  Returns 1 if the requested resolution is
//!                                  bezel corrected. May be NULL.
//!
//! \retval ::NVAPI_OK                          Capabilties have been returned.
//! \retval ::NVAPI_INVALID_ARGUMENT            One or more args passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED          The NvAPI API needs to be initialized first
//! \retval ::NVAPI_MOSAIC_NOT_ACTIVE           The display does not belong to an active Mosaic Topology
//! \retval ::NVAPI_NO_IMPLEMENTATION           This entrypoint not available
//! \retval ::NVAPI_ERROR                       Miscellaneous error occurred
//!
//! \ingroup mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Mosaic_GetDisplayViewportsByResolution(NvU32 displayId, NvU32 srcWidth, NvU32 srcHeight, NV_RECT viewports[NV_MOSAIC_MAX_DISPLAYS], NvU8* bezelCorrected);




////////////////////////////////////////////////////////////////////////////////////////
//
// ###########################################################################
// DELME_RUSS - DELME_RUSS - DELME_RUSS - DELME_RUSS - DELME_RUSS - DELME_RUSS
//
//   Below is the Phase 1 Mosaic stuff, the Phase 2 stuff above is what will remain
//   once Phase 2 is complete.  For a small amount of time, the two will co-exist.  As
//   soon as apps (nvapichk, NvAPITestMosaic, and CPL) are updated to use the Phase 2
//   entrypoints, the code below will be deleted.
//
// DELME_RUSS - DELME_RUSS - DELME_RUSS - DELME_RUSS - DELME_RUSS - DELME_RUSS
// ###########################################################################
//
// Supported topos 1x4, 4x1 and 2x2 to start with.
// 
// Selected scan out targets can be one per GPU or more than one on the same GPU.
//
// SAMPLE of MOSAIC 1x4 SCAN OUT TOPO with 8 pixel horizontal overlap
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

//! Used in NV_MOSAIC_TOPOLOGY.
#define NVAPI_MAX_MOSAIC_DISPLAY_ROWS       8

//! Used in NV_MOSAIC_TOPOLOGY.
#define NVAPI_MAX_MOSAIC_DISPLAY_COLUMNS    8 

//! Used in NV_MOSAIC_TOPOLOGY.
#define NVAPI_MAX_MOSAIC_TOPOS              16

//! Used in NvAPI_GetCurrentMosaicTopology() and NvAPI_SetCurrentMosaicTopology().
typedef struct 
{
    NvU32 version;                             //!< Version number of the mosaic topology
    NvU32 rowCount;                            //!< Horizontal display count
    NvU32 colCount;                            //!< Vertical display count

    struct 
    {
        NvPhysicalGpuHandle hPhysicalGPU;      //!< Physical GPU to be used in the topology
        NvU32               displayOutputId;   //!< Connected display target
        NvS32               overlapX;          //!< Pixels of overlap on the left of target: (+overlap, -gap)
        NvS32               overlapY;          //!< Pixels of overlap on the top of target: (+overlap, -gap)

    } gpuLayout[NVAPI_MAX_MOSAIC_DISPLAY_ROWS][NVAPI_MAX_MOSAIC_DISPLAY_COLUMNS];

} NV_MOSAIC_TOPOLOGY;

//! Used in NV_MOSAIC_TOPOLOGY.
#define NVAPI_MOSAIC_TOPOLOGY_VER         MAKE_NVAPI_VERSION(NV_MOSAIC_TOPOLOGY,1)

//! Used in NvAPI_GetSupportedMosaicTopologies().
typedef struct 
{
    NvU32                   version;                                    
    NvU32                   totalCount;                     //!< Count of valid topologies
    NV_MOSAIC_TOPOLOGY      topos[NVAPI_MAX_MOSAIC_TOPOS];  //!< Maximum number of topologies

} NV_MOSAIC_SUPPORTED_TOPOLOGIES;

//! Used in NV_MOSAIC_SUPPORTED_TOPOLOGIES. 
#define NVAPI_MOSAIC_SUPPORTED_TOPOLOGIES_VER         MAKE_NVAPI_VERSION(NV_MOSAIC_SUPPORTED_TOPOLOGIES,1)

//!@}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GetSupportedMosaicTopologies
//
//! DESCRIPTION:     This API returns all valid Mosaic topologies.
//!
//  SUPPORTED OS: Windows XP
//!
//! \since Version: R177_00
//!
//! \param [out] pMosaicTopos                   An array of valid Mosaic topologies.
//!
//! \retval      NVAPI_OK                       Call succeeded; 1 or more topologies were returned
//! \retval      NVAPI_INVALID_ARGUMENT         One or more arguments are invalid
//! \retval      NVAPI_MIXED_TARGET_TYPES       Mosaic topology is only possible with all targets of the same NV_GPU_OUTPUT_TYPE.
//! \retval      NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \retval      NVAPI_NOT_SUPPORTED            Mosaic is not supported with GPUs on this system.
//! \retval      NVAPI_NO_ACTIVE_SLI_TOPOLOGY   SLI is not enabled, yet needs to be, in order for this function to succeed.
//!
//! \ingroup     mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetSupportedMosaicTopologies(NV_MOSAIC_SUPPORTED_TOPOLOGIES *pMosaicTopos);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GetCurrentMosaicTopology
//
//! DESCRIPTION:     This API gets the current Mosaic topology.
//!
//  SUPPORTED OS: Windows XP
//!
//! \since Version: R177_00
//!
//! \param [out] pMosaicTopo                    The current Mosaic topology
//! \param [out] pEnabled                       TRUE if returned topology is currently enabled, else FALSE
//!
//! \retval      NVAPI_OK                       Call succeeded
//! \retval      NVAPI_INVALID_ARGUMENT         One or more arguments are invalid
//! \retval      NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \retval      NVAPI_NOT_SUPPORTED            Mosaic is not supported with GPUs on this system.
//! \retval      NVAPI_NO_ACTIVE_SLI_TOPOLOGY   SLI is not enabled, yet needs to be, in order for this function to succeed.
//!
//! \ingroup     mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GetCurrentMosaicTopology(NV_MOSAIC_TOPOLOGY *pMosaicTopo, NvU32 *pEnabled);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_SetCurrentMosaicTopology
//
//! DESCRIPTION:     This API sets the Mosaic topology, and enables it so that the
//!                  Mosaic display settings are enumerated upon request.
//!
//  SUPPORTED OS: Windows XP
//!
//! \since Version: R177_00
//!
//! \param [in]  pMosaicTopo                    A valid Mosaic topology
//!
//! \retval      NVAPI_OK                       Call succeeded
//! \retval      NVAPI_INVALID_ARGUMENT         One or more arguments are invalid
//! \retval      NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \retval      NVAPI_NOT_SUPPORTED            Mosaic is not supported with GPUs on this system.
//! \retval      NVAPI_NO_ACTIVE_SLI_TOPOLOGY   SLI is not enabled, yet needs to be, in order for this function to succeed.
//!
//! \ingroup     mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_SetCurrentMosaicTopology(NV_MOSAIC_TOPOLOGY *pMosaicTopo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_EnableCurrentMosaicTopology
//
//! DESCRIPTION:    This API enables or disables the current Mosaic topology. 
//!                 When enabling, the last Mosaic topology will be set.
//!
//!                  - If enabled, enumeration of display settings will include valid Mosaic resolutions.  
//!                  - If disabled, enumeration of display settings will not include Mosaic resolutions.
//!
//  SUPPORTED OS: Windows XP
//!
//! \since Version: R177_00
//!
//! \param [in]  enable                         TRUE to enable the Mosaic Topology, FALSE to disable it.
//!
//! \retval      NVAPI_OK                       Call succeeded
//! \retval      NVAPI_INVALID_ARGUMENT         One or more arguments are invalid
//! \retval      NVAPI_NVIDIA_DEVICE_NOT_FOUND  No NVIDIA GPU driving a display was found
//! \retval      NVAPI_NOT_SUPPORTED            Mosaic is not supported with GPUs on this system.
//! \retval      NVAPI_NO_ACTIVE_SLI_TOPOLOGY   SLI is not enabled, yet needs to be, in order for this function to succeed.
//!
//! \ingroup     mosaicapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_EnableCurrentMosaicTopology(NvU32 enable);




/////////////////////////////////////////////////////////////////////////////// 
// 
// FUNCTION NAME: NvAPI_GPU_GetHDCPSupportStatus 
//
//! \fn NvAPI_GPU_GetHDCPSupportStatus(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_GET_HDCP_SUPPORT_STATUS *pGetHDCPSupportStatus)
//! DESCRIPTION: This function returns a GPU's HDCP support status. 
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version: 175.86
//!
//!  \retval ::NVAPI_OK 
//!  \retval ::NVAPI_ERROR 
//!  \retval ::NVAPI_INVALID_ARGUMENT 
//!  \retval ::NVAPI_HANDLE_INVALIDATED 
//!  \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE 
//!  \retval ::NVAPI_INCOMPATIBLE_STRUCT_VERSION 
// 
////////////////////////////////////////////////////////////////////////////////

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

//!  \ingroup gpu 
NVAPI_INTERFACE NvAPI_GPU_GetHDCPSupportStatus(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_GET_HDCP_SUPPORT_STATUS *pGetHDCPSupportStatus);

#if defined(_D3D9_H_) || defined(__d3d10_h__) || defined(__d3d11_h__)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_Stereo_CreateHandleFromIUnknown
//
//! DESCRIPTION:   This API creates a stereo handle that is used in subsequent calls related to a given device interface.
//!                This must be called before any other NvAPI_Stereo_ function for that handle.
//!                Multiple devices can be used at one time using multiple calls to this function (one per each device). 
//!
//! HOW TO USE:    After the Direct3D device is created, create the stereo handle.
//!                On call success:
//!                -# Use all other NvAPI_Stereo_ functions that have stereo handle as first parameter.
//!                -# After the device interface that corresponds to the the stereo handle is destroyed,
//!                the application should call NvAPI_DestroyStereoHandle() for that stereo handle. 
//!
//! WHEN TO USE:   After the stereo handle for the device interface is created via successfull call to the appropriate NvAPI_Stereo_CreateHandleFrom() function.
//!
//  SUPPORTED OS: Windows Vista and higher
//!
//! \since Version 180.51
//!
//! \param [in]     pDevice        Pointer to IUnknown interface that is IDirect3DDevice9* in DX9, ID3D10Device*.
//! \param [out]    pStereoHandle  Pointer to the newly created stereo handle.
//!
//! \retval ::NVAPI_OK                       Stereo handle is created for given device interface.
//! \retval ::NVAPI_INVALID_ARGUMENT         Provided device interface is invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED  
//! \retval ::NVAPI_STEREO_NOT_INITIALIZED   Stereo part of NVAPI not initialized.
//! \retval ::NVAPI_ERROR 
//!
//! \ingroup stereoapi
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_Stereo_CreateHandleFromIUnknown(IUnknown *pDevice, StereoHandle *pStereoHandle);

#endif // defined(_D3D9_H_) || defined(__d3d10_h__)

//! \ingroup vidio
//! Unique identifier for VIO owner (process identifier or NVVIOOWNERID_NONE)
typedef NvU32  NVVIOOWNERID;                               
//! \addtogroup vidio
//! @{
#define NVVIOOWNERID_NONE 0      //!< Unregistered ownerId        
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
    NVVIOSIGNALFORMAT_NONE,			    //!< Invalid signal format 
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
        NvU32          majorVersion;                        //!< Major version
        NvU32          minorVersion;                        //!< Minor version
    } driver;                                               //
    //! Firmware version 
    struct                                                  
    {                                                       
        NvU32          majorVersion;                        //!< Major version
        NvU32          minorVersion;                        //!< Minor version
    } firmWare;                                             //
    NVVIOOWNERID      ownerId;                              //!< Unique identifier for owner of video output (NVVIOOWNERID_INVALID if free running)
    NVVIOOWNERTYPE    ownerType;                            //!< Owner type (OpenGL application or Desktop mode)
} NVVIOCAPS;

//! Macro for constructing the version field of NVVIOCAPS
#define NVVIOCAPS_VER   MAKE_NVAPI_VERSION(NVVIOCAPS,1)

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
    NVVIOINPUTOUTPUTSTATUS	vid1Out;                        //!< Video 1 output status
    NVVIOINPUTOUTPUTSTATUS	vid2Out;                        //!< Video 2 output status
    NVVIOSYNCSTATUS		sdiSyncIn;                      //!< SDI sync input status
    NVVIOSYNCSTATUS		compSyncIn;                     //!< Composite sync input status
    NvU32			syncEnable;                     //!< Sync enable (TRUE if using syncSource)
    NVVIOSYNCSOURCE		syncSource;                     //!< Sync source
    NVVIOSIGNALFORMAT		syncFormat;                     //!< Sync format
    NvU32			frameLockEnable;                //!< Framelock enable flag
    NvU32			outputVideoLocked;              //!< Output locked status
    NvU32			dataIntegrityCheckErrorCount;   //!< Data integrity check error count
    NvU32			dataIntegrityCheckEnabled;      //!< Data integrity check status enabled 
    NvU32			dataIntegrityCheckFailed;       //!< Data integrity check status failed 
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
    float            fGammaValueR;			//!< Red Gamma value within gamma ranges. 0.5 - 6.0
    float            fGammaValueG;			//!< Green Gamma value within gamma ranges. 0.5 - 6.0
    float            fGammaValueB;			//!< Blue Gamma value within gamma ranges. 0.5 - 6.0
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
                                  NVVIOCONFIG_ANC_PARITY_COMPUTATION)

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
                                   NVVIOCONFIG_ANC_PARITY_COMPUTATION)

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
                                        NVVIOCONFIG_COMPOSITESYNCTYPE )                                            
                                             

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

typedef NVVIOOUTPUTCONFIG_V2 NVVIOOUTPUTCONFIG;
typedef NVVIOCONFIG_V2 NVVIOCONFIG;

#define NVVIOCONFIG_VER1  MAKE_NVAPI_VERSION(NVVIOCONFIG_V1,1)
#define NVVIOCONFIG_VER2  MAKE_NVAPI_VERSION(NVVIOCONFIG_V2,2)
#define NVVIOCONFIG_VER   NVVIOCONFIG_VER2


typedef struct
{
    NvPhysicalGpuHandle					hPhysicalGpu;					//!< Handle to Physical GPU (This could be NULL for GVI device if its not binded)
    NvVioHandle                         hVioHandle;                     //!<handle to SDI Input/Output device
    NvU32                               vioId;                          //!<device Id of SDI Input/Output device
    NvU32                               outputId;			//!<deviceMask of the SDI display connected to GVO device. 
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_GetCapabilities(NvVioHandle     hVioHandle,      NVVIOCAPS       *pAdapterCaps);

////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_Open
//!  
//!   Description: This API opens the graphics adapter for video I/O operations
//!                using the OpenGL application interface.  Read operations
//!                are permitted in this mode by multiple clients, but Write 
//!                operations are application exclusive.
//!  
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_GetConfig(NvVioHandle        hVioHandle,
                                    NVVIOCONFIG        *pConfig); 


///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SetConfig
//!  
//!   Description: This API sets the graphics-to-video configuration.
//!  
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_SetCSC(NvVioHandle           hVioHandle,
                                 NVVIOCOLORCONVERSION  *pCSC);

////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_GetCSC
//! 
//!   Description: This API gets the colorspace conversion parameters.
//! 
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_GetCSC(NvVioHandle           hVioHandle,
                                 NVVIOCOLORCONVERSION  *pCSC);

///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SetGamma
//! 
//!   Description: This API sets the gamma conversion parameters.
//! 
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_SetGamma(NvVioHandle           hVioHandle,
                                   NVVIOGAMMACORRECTION  *pGamma);


///////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_GetGamma
//! 
//!   Description: This API gets the gamma conversion parameters.
//! 
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_GetGamma(NvVioHandle           hVioHandle,
                                   NVVIOGAMMACORRECTION* pGamma);
 
////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_SetSyncDelay
//! 
//!   Description: This API sets the sync delay parameters.
//! 
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_SetSyncDelay(NvVioHandle            hVioHandle,
                                       const NVVIOSYNCDELAY   *pSyncDelay);


////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_GetSyncDelay
//! 
//!   Description: This API gets the sync delay parameters.
//! 
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
NVAPI_INTERFACE NvAPI_VIO_GetSyncDelay(NvVioHandle      hVioHandle,
                                       NVVIOSYNCDELAY   *pSyncDelay);

////////////////////////////////////////////////////////////////////////////////
//!   Function:    NvAPI_VIO_IsRunning
//! 
//!   Description: This API determines if Video I/O is running.
//! 
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
//!
//! \since Version 190.50
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
//  SUPPORTED OS: Windows XP and higher
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
// TODO
//typedef NvStatus (__cdecl *P_NvAPI_GPU_GetTachReading)(NvPhysicalGpuHandle hPhysicalGPU, NvU32 *pValue);
//P_NvAPI_GPU_GetTachReading NvAPI_GetTachReading;


///////////////////////////////////////////////////////////////////////////////
// FUNCTION NAME:   NvAPI_SYS_GetDisplayIdFromGpuAndOutputId
//
//! DESCRIPTION:     This API converts a Physical GPU handle and output ID to a
//!                  display ID.
//!
//  SUPPORTED OS: Windows XP and higher
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
//  SUPPORTED OS: Windows XP and higher
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

//  SUPPORTED OS: Windows XP and higher
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

//  SUPPORTED OS: Windows XP and higher
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_DISP_GetGDIPrimaryDisplayId
//
//! DESCRIPTION:     This API returns the Display ID of the GDI Primary.
//!
//! \param [out]     displayId   Display ID of the GDI Primary display.
//!
//! \retval ::NVAPI_OK:                          Capabilties have been returned.
//! \retval ::NVAPI_NVIDIA_DEVICE_NOT_FOUND:     GDI Primary not on an NVIDIA GPU.
//! \retval ::NVAPI_INVALID_ARGUMENT:            One or more args passed in are invalid.
//! \retval ::NVAPI_API_NOT_INTIALIZED:          The NvAPI API needs to be initialized first
//! \retval ::NVAPI_NO_IMPLEMENTATION:           This entrypoint not available
//! \retval ::NVAPI_ERROR:                       Miscellaneous error occurred
//!
//! \ingroup dispcontrol
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_DISP_GetGDIPrimaryDisplayId(NvU32* displayId);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetECCStatusInfo
//
//! \fn NvAPI_GPU_GetECCStatusInfo(NvPhysicalGpuHandle hPhysicalGpu, 
//!                                           NV_GPU_ECC_STATUS_INFO *pECCStatusInfo);
//! DESCRIPTION:     This function returns ECC memory status information.
//!
//  SUPPORTED OS: Windows XP and higher
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
typedef struct
{
    NvU32                 version;               //!< Structure version
    NvU32                 isSupported : 1;       //!< ECC memory feature support
    NV_ECC_CONFIGURATION  configurationOptions;  //!< Supported ECC memory feature configuration options
    NvU32                 isEnabled : 1;         //!< Active ECC memory setting
} NV_GPU_ECC_STATUS_INFO;

//! \ingroup gpuecc
//! Macro for constructing the version field of NV_GPU_ECC_STATUS_INFO
#define NV_GPU_ECC_STATUS_INFO_VER MAKE_NVAPI_VERSION(NV_GPU_ECC_STATUS_INFO,1)

//! \ingroup gpuecc
NVAPI_INTERFACE NvAPI_GPU_GetECCStatusInfo(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_ECC_STATUS_INFO *pECCStatusInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_GetECCErrorInfo
//
//! \fn NvAPI_GPU_GetECCErrorInfo(NvPhysicalGpuHandle hPhysicalGpu, 
//!                                          NV_GPU_ECC_ERROR_INFO *pECCErrorInfo);
//!
//! DESCRIPTION:     This function returns ECC memory error information.
//!
//  SUPPORTED OS: Windows XP and higher
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
typedef struct
{
    NvU32   version;             //!< Structure version
    struct 
    {
        NvU64  singleBitErrors;  //!< Number of single-bit ECC errors detected since last boot
        NvU64  doubleBitErrors;  //!< Number of double-bit ECC errors detected since last boot
    } current;
    struct 
    {
        NvU64  singleBitErrors;  //!< Number of single-bit ECC errors detected since last counter reset
        NvU64  doubleBitErrors;  //!< Number of double-bit ECC errors detected since last counter reset
    } aggregate;
} NV_GPU_ECC_ERROR_INFO;

//! \ingroup gpuecc
//! Macro for constructing the version field of NV_GPU_ECC_ERROR_INFO
#define NV_GPU_ECC_ERROR_INFO_VER MAKE_NVAPI_VERSION(NV_GPU_ECC_ERROR_INFO,1)

//! \ingroup gpuecc
NVAPI_INTERFACE NvAPI_GPU_GetECCErrorInfo(NvPhysicalGpuHandle hPhysicalGpu, NV_GPU_ECC_ERROR_INFO *pECCErrorInfo);

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:   NvAPI_GPU_ResetECCErrorInfo
//
//! DESCRIPTION:     This function resets ECC memory error counters.
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \param [in]     hPhysicalGpu     A handle identifying the physical GPU for
//!                                  which ECC error information is to be
//!                                  cleared.
//! \param [in]     bResetCurrent    Reset the current ECC error counters.
//! \param [in]     bResetAggregate  Reset the aggregate ECC error counters.
//! 
//! \retval ::NVAPI_OK  The request was completed successfully.
//! \retval ::NVAPI_ERROR  An unknown error occurred.
//! \retval ::NVAPI_EXPECTED_PHYSICAL_GPU_HANDLE  The provided GPU handle is not a physical GPU handle.
//! \retval ::NVAPI_INVALID_HANDLE  The provided GPU handle is invalid.
//! \retval ::NVAPI_HANDLE_INVALIDATED  The provided GPU handle is no longer valid.
//! \retval ::NVAPI_NOT_SUPPORTED  The request is not supported.
//! \retval ::NVAPI_API_NOT_INTIALIZED  NvAPI was not yet initialized.
//!
//! \ingroup gpuecc
///////////////////////////////////////////////////////////////////////////////
NVAPI_INTERFACE NvAPI_GPU_ResetECCErrorInfo(NvPhysicalGpuHandle hPhysicalGpu, NvU8 bResetCurrent,
                                            NvU8 bResetAggregate);



//#if defined(_D3D9_H_) || defined(__d3d10_h__) || defined(__d3d10_1_h__) || defined(__d3d11_h__)
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: NvAPI_D3D_SetFPSIndicatorState
//
//!   DESCRIPTION: Display an overlay that tracks the number of times the app presents per second, or,   
//!      the number of frames-per-second (FPS)
//!
//  SUPPORTED OS: Windows XP and higher
//!
//! \param [in] bool    Whether or not to enable the fps indicator.
//!                
//! \return ::NVAPI_OK, 
//!         ::NVAPI_ERROR
//!
//! \ingroup dx 
/////////////////////////////////////////////////////////////////////////////// 
//NVAPI_INTERFACE NvAPI_D3D_SetFPSIndicatorState(IUnknown *pDev, NvU8 doEnable);
//typedef NvStatus (__cdecl *P_NvAPI_D3D_SetFPSIndicatorState)(NvU8 doEnable);
//P_NvAPI_D3D_SetFPSIndicatorState NvAPI_SetFPSIndicatorState;


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
//  SUPPORTED OS: Windows XP and higher
//! \param [in]	  hPhysicalGPU	(IN)	- GPU for which performance decrease is to be evaluated.
//! \param [out]  pPerfDecrInfo	(OUT)	- Pointer to a NvU32 variable containing performance decrease info
//!
//! \return      This API can return any of the error codes enumerated in #NvStatus. 
//!
//! \ingroup gpu
///////////////////////////////////////////////////////////////////////////////
//NVAPI_INTERFACE NvAPI_GPU_GetPerfDecreaseInfo(__in NvPhysicalGpuHandle hPhysicalGpu, __inout NvU32 *pPerfDecrInfo);
typedef NvStatus (__cdecl *P_NvAPI_GPU_GetPerfDecreaseInfo)(NvPhysicalGpuHandle hPhysicalGpu, NVAPI_GPU_PERF_DECREASE *pPerfDecrInfo);
P_NvAPI_GPU_GetPerfDecreaseInfo NvAPI_GetPerfDecreaseInfo;

#pragma pack(pop)