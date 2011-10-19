//
//  Copyright (c) 2008 - 2009 Advanced Micro Devices, Inc.
 
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
//  EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

/// \file adl_defines.h
/// \brief Contains all definitions exposed by ADL for \ALL platforms.\n <b>Included in ADL SDK</b>
///
/// This file contains all definitions used by ADL.  
/// The ADL definitions include the following:
/// \li ADL error codes
/// \li Enumerations for the ADLDisplayInfo structure
/// \li Maximum limits
///

#ifndef ADL_DEFINES_H_
#define ADL_DEFINES_H_

/// \defgroup DEFINES Constants and Definitions
// @{

/// \defgroup define_misc Muscelaneous Constant Definitions
// @{

/// \name General Definitions
// @{

/// Defines ADL_TRUE
#define ADL_TRUE	1
/// Defines ADL_FALSE
#define ADL_FALSE		0

/// Defines the maximum string length
#define ADL_MAX_CHAR                                    4096
/// Defines the maximum string length
#define ADL_MAX_PATH                                    256
/// Defines the maximum number of supported adapters
#define ADL_MAX_ADAPTERS                               150
/// Defines the maxumum number of supported displays
#define ADL_MAX_DISPLAYS                                150
/// Defines the maxumum string length for device name
#define ADL_MAX_DEVICENAME								32
/// Defines for all adapters
#define ADL_ADAPTER_INDEX_ALL							-1
///	Defines APIs with iOption none
#define ADL_MAIN_API_OPTION_NONE						0
// @}

/// \name Definitions for iOption parameter used by
/// ADL_Display_DDCBlockAccess_Get()
// @{

/// Switch to DDC line 2 before sending the command to the display.
#define ADL_DDC_OPTION_SWITCHDDC2              0x00000001
/// Save command in the registry under a unique key, corresponding to parameter \b iCommandIndex
#define ADL_DDC_OPTION_RESTORECOMMAND 0x00000002
// @}

/// \name Values for
/// ADLI2C.iAction used with ADL_Display_WriteAndReadI2C()
// @{

#define ADL_DL_I2C_ACTIONREAD									0x00000001
#define ADL_DL_I2C_ACTIONWRITE								0x00000002
#define ADL_DL_I2C_ACTIONREAD_REPEATEDSTART    0x00000003
// @}


// @}		//Misc

/// \defgroup define_adl_results Result Codes
/// This group of definitions are the various results returned by all ADL functions \n
// @{
/// All OK, but need to wait
#define ADL_OK_WAIT				4
/// All OK, but need restart
#define ADL_OK_RESTART				3
/// All OK but need mode change
#define ADL_OK_MODE_CHANGE			2
/// All OK, but with warning
#define ADL_OK_WARNING				1
/// ADL function completed successfully 
#define ADL_OK					0
/// Generic Error. Most likely one or more of the Escape calls to the driver failed!
#define ADL_ERR					-1
/// ADL not initialized
#define ADL_ERR_NOT_INIT			-2
/// One of the parameter passed is invalid
#define ADL_ERR_INVALID_PARAM			-3
/// One of the parameter size is invalid
#define ADL_ERR_INVALID_PARAM_SIZE		-4
/// Invalid ADL index passed
#define ADL_ERR_INVALID_ADL_IDX			-5
/// Invalid controller index passed
#define ADL_ERR_INVALID_CONTROLLER_IDX		-6
/// Invalid display index passed
#define ADL_ERR_INVALID_DIPLAY_IDX		-7
/// Function  not supported by the driver
#define ADL_ERR_NOT_SUPPORTED			-8
/// Null Pointer error
#define ADL_ERR_NULL_POINTER			-9
/// Call can't be made due to disabled adapter
#define ADL_ERR_DISABLED_ADAPTER		-10
/// Invalid Callback
#define ADL_ERR_INVALID_CALLBACK        	-11
/// Display Resource conflict
#define ADL_ERR_RESOURCE_CONFLICT				-12

// @}
/// </A>

/// \defgroup define_display_type Display Type
/// Define Monitor/CRT display type
// @{
/// Define Monitor display type
#define ADL_DT_MONITOR          		0
/// Define TV display type
#define ADL_DT_TELEVISION                	1
/// Define LCD display type
#define ADL_DT_LCD_PANEL               		2
/// Define DFP display type
#define ADL_DT_DIGITAL_FLAT_PANEL		3
/// Define Componment Video display type
#define ADL_DT_COMPONENT_VIDEO           	4
/// Define Projector display type
#define ADL_DT_PROJECTOR           	        5
// @}

/// \defgroup define_display_connection_type Display Connection Type
// @{
/// Define unknown display output type
#define ADL_DOT_UNKNOWN				0
/// Define composite display output type
#define ADL_DOT_COMPOSITE			1
/// Define SVideo display output type
#define ADL_DOT_SVIDEO				2
/// Define analog display output type
#define ADL_DOT_ANALOG				3
/// Define digital display output type
#define ADL_DOT_DIGITAL				4
// @}

/// \defgroup define_color_type Display Color Type and Source
/// Define  Display Color Type and Source
// @{
#define ADL_DISPLAY_COLOR_BRIGHTNESS	(1 << 0)
#define ADL_DISPLAY_COLOR_CONTRAST	(1 << 1)
#define ADL_DISPLAY_COLOR_SATURATION	(1 << 2)
#define ADL_DISPLAY_COLOR_HUE		(1 << 3)
#define ADL_DISPLAY_COLOR_TEMPERATURE	(1 << 4)	

/// Color Temperature Source is EDID
#define ADL_DISPLAY_COLOR_TEMPERATURE_SOURCE_EDID	(1 << 5)
/// Color Temperature Source is User
#define ADL_DISPLAY_COLOR_TEMPERATURE_SOURCE_USER	(1 << 6)
// @}

/// \defgroup define_adjustment_capabilities Display Adjustment Capabilities
/// Display adjustment capabilities values.  Returned by ADL_Display_AdjustCaps_Get
// @{
#define ADL_DISPLAY_ADJUST_OVERSCAN		(1 << 0)
#define ADL_DISPLAY_ADJUST_VERT_POS		(1 << 1)
#define ADL_DISPLAY_ADJUST_HOR_POS		(1 << 2)
#define ADL_DISPLAY_ADJUST_VERT_SIZE		(1 << 3)
#define ADL_DISPLAY_ADJUST_HOR_SIZE		(1 << 4)
#define ADL_DISPLAY_ADJUST_SIZEPOS		(ADL_DISPLAY_ADJUST_VERT_POS | ADL_DISPLAY_ADJUST_HOR_POS | ADL_DISPLAY_ADJUST_VERT_SIZE | ADL_DISPLAY_ADJUST_HOR_SIZE)
#define ADL_DISPLAY_CUSTOMMODES			(1<<5)
#define ADL_DISPLAY_ADJUST_UNDERSCAN		(1<<6)
// @}


/// \defgroup define_desktop_config Desktop Configuration Flags
/// These flags are used by ADL_DesktopConfig_xxx
// @{
#define ADL_DESKTOPCONFIG_UNKNOWN    0    	  /* UNKNOWN desktop config   */
#define ADL_DESKTOPCONFIG_SINGLE     (1 <<  0)    /* Single                   */
#define ADL_DESKTOPCONFIG_CLONE      (1 <<  2)    /* Clone                    */
#define ADL_DESKTOPCONFIG_BIGDESK_H  (1 <<  4)    /* Big Desktop Horizontal   */
#define ADL_DESKTOPCONFIG_BIGDESK_V  (1 <<  5)    /* Big Desktop Vertical     */
#define ADL_DESKTOPCONFIG_BIGDESK_HR (1 <<  6)    /* Big Desktop Reverse Horz */
#define ADL_DESKTOPCONFIG_BIGDESK_VR (1 <<  7)    /* Big Desktop Reverse Vert */
#define ADL_DESKTOPCONFIG_RANDR12    (1 <<  8)    /* RandR 1.2 Multi-display */
// @}

/// needed for ADLDDCInfo structure
#define ADL_MAX_DISPLAY_NAME                                256

/// \defgroup define_edid_flags Values for ulDDCInfoFlag
/// defines for ulDDCInfoFlag EDID flag
// @{
#define ADL_DISPLAYDDCINFOEX_FLAG_PROJECTORDEVICE       (1 << 0)
#define ADL_DISPLAYDDCINFOEX_FLAG_EDIDEXTENSION         (1 << 1)
#define ADL_DISPLAYDDCINFOEX_FLAG_DIGITALDEVICE         (1 << 2)
#define ADL_DISPLAYDDCINFOEX_FLAG_HDMIAUDIODEVICE       (1 << 3)
#define ADL_DISPLAYDDCINFOEX_FLAG_SUPPORTS_AI           (1 << 4)
#define ADL_DISPLAYDDCINFOEX_FLAG_SUPPORT_xvYCC601      (1 << 5)
#define ADL_DISPLAYDDCINFOEX_FLAG_SUPPORT_xvYCC709      (1 << 6)
// @}

/// \defgroup define_displayinfo_connector Display Connector Type
/// defines for ADLDisplayInfo.iDisplayConnector
// @{
#define ADL_DISPLAY_CONTYPE_UNKNOWN			0
#define ADL_DISPLAY_CONTYPE_VGA				1
#define ADL_DISPLAY_CONTYPE_DVI_D			2
#define ADL_DISPLAY_CONTYPE_DVI_I			3
#define ADL_DISPLAY_CONTYPE_ATICVDONGLE_NTSC		4
#define ADL_DISPLAY_CONTYPE_ATICVDONGLE_JPN		5
#define ADL_DISPLAY_CONTYPE_ATICVDONGLE_NONI2C_JPN	6
#define ADL_DISPLAY_CONTYPE_ATICVDONGLE_NONI2C_NTSC	7
#define ADL_DISPLAY_CONTYPE_HDMI_TYPE_A			10
#define ADL_DISPLAY_CONTYPE_HDMI_TYPE_B			11
#define ADL_DISPLAY_CONTYPE_SVIDEO               	12
#define ADL_DISPLAY_CONTYPE_COMPOSITE                	13
#define ADL_DISPLAY_CONTYPE_RCA_3COMPONENT             	14
#define ADL_DISPLAY_CONTYPE_DISPLAYPORT			15
// @}

/// TV Capabilities and Standards
/// \defgroup define_tv_caps TV Capabilities and Standards
// @{
#define ADL_TV_STANDARDS			(1 << 0)
#define ADL_TV_SCART				(1 << 1)

/// TV Standards Definitions
#define ADL_STANDARD_NTSC_M		(1 << 0)
#define ADL_STANDARD_NTSC_JPN		(1 << 1)
#define ADL_STANDARD_NTSC_N		(1 << 2)
#define ADL_STANDARD_PAL_B		(1 << 3)
#define ADL_STANDARD_PAL_COMB_N		(1 << 4)
#define ADL_STANDARD_PAL_D		(1 << 5)
#define ADL_STANDARD_PAL_G		(1 << 6)
#define ADL_STANDARD_PAL_H		(1 << 7)
#define ADL_STANDARD_PAL_I		(1 << 8)
#define ADL_STANDARD_PAL_K		(1 << 9)
#define ADL_STANDARD_PAL_K1		(1 << 10)
#define ADL_STANDARD_PAL_L		(1 << 11)
#define ADL_STANDARD_PAL_M		(1 << 12)
#define ADL_STANDARD_PAL_N		(1 << 13)
#define ADL_STANDARD_PAL_SECAM_D	(1 << 14)
#define ADL_STANDARD_PAL_SECAM_K	(1 << 15)
#define ADL_STANDARD_PAL_SECAM_K1	(1 << 16)
#define ADL_STANDARD_PAL_SECAM_L	(1 << 17)
// @}


/// \defgroup define_video_custom_mode Video Custom Mode flags
/// Component Video Custom Mode flags.  This is used by the iFlags parameter in ADLCustomMode
// @{
#define ADL_CUSTOMIZEDMODEFLAG_MODESUPPORTED	(1 << 0)
#define ADL_CUSTOMIZEDMODEFLAG_NOTDELETETABLE	(1 << 1)
#define ADL_CUSTOMIZEDMODEFLAG_INSERTBYDRIVER	(1 << 2)
#define ADL_CUSTOMIZEDMODEFLAG_INTERLACED	(1 << 3)
#define ADL_CUSTOMIZEDMODEFLAG_BASEMODE		(1 << 4)
// @}

/// \defgroup define_ddcinfoflag Values used for DDCInfoFlag
/// ulDDCInfoFlag field values used by the ADLDDCInfo structure
// @{
#define ADL_DISPLAYDDCINFOEX_FLAG_PROJECTORDEVICE	(1 << 0)
#define ADL_DISPLAYDDCINFOEX_FLAG_EDIDEXTENSION		(1 << 1)
#define ADL_DISPLAYDDCINFOEX_FLAG_DIGITALDEVICE		(1 << 2)
#define ADL_DISPLAYDDCINFOEX_FLAG_HDMIAUDIODEVICE	(1 << 3)
#define ADL_DISPLAYDDCINFOEX_FLAG_SUPPORTS_AI		(1 << 4)
#define ADL_DISPLAYDDCINFOEX_FLAG_SUPPORT_xvYCC601	(1 << 5)
#define ADL_DISPLAYDDCINFOEX_FLAG_SUPPORT_xvYCC709	(1 << 6)
// @}

/// \defgroup define_cv_dongle Values used by ADL_CV_DongleSettings_xxx
/// The following is applicable to ADL_DISPLAY_CONTYPE_ATICVDONGLE_JP and ADL_DISPLAY_CONTYPE_ATICVDONGLE_NONI2C_D only
// @{
#define ADL_DISPLAY_CV_DONGLE_D1          (1 << 0)
#define ADL_DISPLAY_CV_DONGLE_D2          (1 << 1)
#define ADL_DISPLAY_CV_DONGLE_D3          (1 << 2)
#define ADL_DISPLAY_CV_DONGLE_D4          (1 << 3)
#define ADL_DISPLAY_CV_DONGLE_D5          (1 << 4)

/// The following is applicable to ADL_DISPLAY_CONTYPE_ATICVDONGLE_NA and ADL_DISPLAY_CONTYPE_ATICVDONGLE_NONI2C only

#define ADL_DISPLAY_CV_DONGLE_480I        (1 << 0)
#define ADL_DISPLAY_CV_DONGLE_480P        (1 << 1)
#define ADL_DISPLAY_CV_DONGLE_540P        (1 << 2)
#define ADL_DISPLAY_CV_DONGLE_720P        (1 << 3)
#define ADL_DISPLAY_CV_DONGLE_1080I       (1 << 4)
#define ADL_DISPLAY_CV_DONGLE_1080P       (1 << 5)
#define ADL_DISPLAY_CV_DONGLE_16_9        (1 << 6)
#define ADL_DISPLAY_CV_DONGLE_720P50      (1 << 7)
#define ADL_DISPLAY_CV_DONGLE_1080I25     (1 << 8)
#define ADL_DISPLAY_CV_DONGLE_576I25      (1 << 9)
#define ADL_DISPLAY_CV_DONGLE_576P50      (1 << 10)
#define ADL_DISPLAY_CV_DONGLE_1080P24      (1 << 11)
#define ADL_DISPLAY_CV_DONGLE_1080P25      (1 << 12)
#define ADL_DISPLAY_CV_DONGLE_1080P30      (1 << 13)
#define ADL_DISPLAY_CV_DONGLE_1080P50      (1 << 14)
// @}

/// \defgroup define_formats_ovr	Formats Override Settings
/// Display force modes flags
// @{
///
#define ADL_DISPLAY_FORMAT_FORCE_720P		0x00000001
#define ADL_DISPLAY_FORMAT_FORCE_1080I		0x00000002
#define ADL_DISPLAY_FORMAT_FORCE_1080P		0x00000004
#define ADL_DISPLAY_FORMAT_FORCE_720P50		0x00000008
#define ADL_DISPLAY_FORMAT_FORCE_1080I25	0x00000010
#define ADL_DISPLAY_FORMAT_FORCE_576I25		0x00000020
#define ADL_DISPLAY_FORMAT_FORCE_576P50		0x00000040
#define ADL_DISPLAY_FORMAT_FORCE_1080P24	0x00000080
#define ADL_DISPLAY_FORMAT_FORCE_1080P25	0x00000100
#define ADL_DISPLAY_FORMAT_FORCE_1080P30	0x00000200
#define ADL_DISPLAY_FORMAT_FORCE_1080P50	0x00000400

///< Below are \b EXTENDED display mode flags

#define ADL_DISPLAY_FORMAT_CVDONGLEOVERIDE  0x00000001
#define ADL_DISPLAY_FORMAT_CVMODEUNDERSCAN  0x00000002
#define ADL_DISPLAY_FORMAT_FORCECONNECT_SUPPORTED  0x00000004
#define ADL_DISPLAY_FORMAT_RESTRICT_FORMAT_SELECTION 0x00000008
#define ADL_DISPLAY_FORMAT_SETASPECRATIO 0x00000010
#define ADL_DISPLAY_FORMAT_FORCEMODES    0x00000020
#define ADL_DISPLAY_FORMAT_LCDRTCCOEFF   0x00000040
// @}

/// Defines used by OD5
#define ADL_PM_PARAM_DONT_CHANGE    0

/// The following defines Bus types
// @{
#define ADL_BUSTYPE_PCI           0       /* PCI bus                          */
#define ADL_BUSTYPE_AGP           1       /* AGP bus                          */
#define ADL_BUSTYPE_PCIE          2       /* PCI Express bus                  */
#define ADL_BUSTYPE_PCIE_GEN2     3       /* PCI Express 2nd generation bus   */
// @}

/// \defgroup define_ws_caps	Workstation Capabilities
/// Workstation values
// @{

/// This value indicates that the workstation card supports active stereo though stereo output connector
#define ADL_STEREO_SUPPORTED		(1 << 2)
/// This value indicates that the workstation card supports active stereo via "blue-line"
#define ADL_STEREO_BLUE_LINE		(1 << 3)
/// This value is used to turn off stereo mode.
#define ADL_STEREO_OFF				0
/// This value indicates that the workstation card supports active stereo.  This is also used to set the stereo mode to active though the stereo output connector
#define ADL_STEREO_ACTIVE	 		(1 << 1)
/// This value indicates that the workstation card supports auto-stereo monitors with horizontal interleave. This is also used to set the stereo mode to use the auto-stereo monitor with horizontal interleave
#define ADL_STEREO_AUTO_HORIZONTAL	(1 << 30)
/// This value indicates that the workstation card supports auto-stereo monitors with vertical interleave. This is also used to set the stereo mode to use the auto-stereo monitor with vertical interleave
#define ADL_STEREO_AUTO_VERTICAL	(1 << 31)
/// This value indicates that the workstation card supports passive stereo, ie. non stereo sync
#define ADL_STEREO_PASSIVE              (1 << 6) 
/// This value indicates that the workstation card supports auto-stereo monitors with vertical interleave. This is also used to set the stereo mode to use the auto-stereo monitor with vertical interleave
#define ADL_STEREO_PASSIVE_HORIZ        (1 << 7)
/// This value indicates that the workstation card supports auto-stereo monitors with vertical interleave. This is also used to set the stereo mode to use the auto-stereo monitor with vertical interleave
#define ADL_STEREO_PASSIVE_VERT         (1 << 8)


/// Load balancing is supported.
#define ADL_WORKSTATION_LOADBALANCING_SUPPORTED         0x00000001
/// Load balancing is available.
#define ADL_WORKSTATION_LOADBALANCING_AVAILABLE         0x00000002

/// Load balancing is disabled.
#define ADL_WORKSTATION_LOADBALANCING_DISABLED          0x00000000
/// Load balancing is Enabled.
#define ADL_WORKSTATION_LOADBALANCING_ENABLED           0x00000001

// @}

/// \defgroup define_adapterspeed speed setting from the adapter
// @{
#define ADL_CONTEXT_SPEED_UNFORCED		0		/* default asic running speed */
#define ADL_CONTEXT_SPEED_FORCEHIGH		1		/* asic running speed is forced to high */
#define ADL_CONTEXT_SPEED_FORCELOW		2		/* asic running speed is forced to low */

#define ADL_ADAPTER_SPEEDCAPS_SUPPORTED		(1 << 0)	/* change asic running speed setting is supported */
// @}

/// \defgroup define_glsync Genlock related values
/// GL-Sync port types (unique values)
// @{
/// Unknown port of GL-Sync module
#define ADL_GLSYNC_PORT_UNKNOWN		0
/// BNC port of of GL-Sync module
#define ADL_GLSYNC_PORT_BNC			1
/// RJ45(1) port of of GL-Sync module
#define ADL_GLSYNC_PORT_RJ45PORT1	2
/// RJ45(2) port of of GL-Sync module
#define ADL_GLSYNC_PORT_RJ45PORT2	3

// GL-Sync Genlock settings mask (bit-vector)

/// None of the ADLGLSyncGenlockConfig members are valid
#define ADL_GLSYNC_CONFIGMASK_NONE				0
/// The ADLGLSyncGenlockConfig.lSignalSource member is valid
#define ADL_GLSYNC_CONFIGMASK_SIGNALSOURCE		(1 << 0)
/// The ADLGLSyncGenlockConfig.iSyncField member is valid
#define ADL_GLSYNC_CONFIGMASK_SYNCFIELD			(1 << 1)
/// The ADLGLSyncGenlockConfig.iSampleRate member is valid
#define ADL_GLSYNC_CONFIGMASK_SAMPLERATE		(1 << 2)
/// The ADLGLSyncGenlockConfig.lSyncDelay member is valid
#define ADL_GLSYNC_CONFIGMASK_SYNCDELAY			(1 << 3)
/// The ADLGLSyncGenlockConfig.iTriggerEdge member is valid
#define ADL_GLSYNC_CONFIGMASK_TRIGGEREDGE		(1 << 4)
/// The ADLGLSyncGenlockConfig.iScanRateCoeff member is valid
#define ADL_GLSYNC_CONFIGMASK_SCANRATECOEFF		(1 << 5)
/// The ADLGLSyncGenlockConfig.lFramelockCntlVector member is valid
#define ADL_GLSYNC_CONFIGMASK_FRAMELOCKCNTL		(1 << 6)


// GL-Sync Framelock control mask (bit-vector)

/// Framelock is disabled
#define ADL_GLSYNC_FRAMELOCKCNTL_NONE			0
/// Framelock is enabled
#define ADL_GLSYNC_FRAMELOCKCNTL_ENABLE			( 1 << 0)

#define ADL_GLSYNC_FRAMELOCKCNTL_DISABLE		( 1 << 1)
#define ADL_GLSYNC_FRAMELOCKCNTL_SWAP_COUNTER_RESET	( 1 << 2)
#define ADL_GLSYNC_FRAMELOCKCNTL_SWAP_COUNTER_ACK	( 1 << 3)

#define ADL_GLSYNC_FRAMELOCKCNTL_STATE_ENABLE		( 1 << 0)

// GL-Sync Framelock counters mask (bit-vector)
#define ADL_GLSYNC_COUNTER_SWAP				( 1 << 0 )

// GL-Sync Signal Sources (unique values)

/// GL-Sync signal source is undefined
#define ADL_GLSYNC_SIGNALSOURCE_UNDEFINED    0x00000100
/// GL-Sync signal source is Free Run
#define ADL_GLSYNC_SIGNALSOURCE_FREERUN      0x00000101
/// GL-Sync signal source is the BNC GL-Sync port
#define ADL_GLSYNC_SIGNALSOURCE_BNCPORT      0x00000102
/// GL-Sync signal source is the RJ45(1) GL-Sync port
#define ADL_GLSYNC_SIGNALSOURCE_RJ45PORT1    0x00000103
/// GL-Sync signal source is the RJ45(2) GL-Sync port
#define ADL_GLSYNC_SIGNALSOURCE_RJ45PORT2    0x00000104


// GL-Sync Signal Types (unique values)

/// GL-Sync signal type is unknown
#define ADL_GLSYNC_SIGNALTYPE_UNDEFINED      0
/// GL-Sync signal type is 480I
#define ADL_GLSYNC_SIGNALTYPE_480I           1
/// GL-Sync signal type is 576I
#define ADL_GLSYNC_SIGNALTYPE_576I           2
/// GL-Sync signal type is 480P
#define ADL_GLSYNC_SIGNALTYPE_480P           3
/// GL-Sync signal type is 576P
#define ADL_GLSYNC_SIGNALTYPE_576P           4
/// GL-Sync signal type is 720P
#define ADL_GLSYNC_SIGNALTYPE_720P           5
/// GL-Sync signal type is 1080P
#define ADL_GLSYNC_SIGNALTYPE_1080P          6
/// GL-Sync signal type is 1080I
#define ADL_GLSYNC_SIGNALTYPE_1080I          7
/// GL-Sync signal type is SDI
#define ADL_GLSYNC_SIGNALTYPE_SDI            8
/// GL-Sync signal type is TTL
#define ADL_GLSYNC_SIGNALTYPE_TTL            9
/// GL_Sync signal type is Analog
#define ADL_GLSYNC_SIGNALTYPE_ANALOG		10

// GL-Sync Sync Field options (unique values)

///GL-Sync sync field option is undefined
#define ADL_GLSYNC_SYNCFIELD_UNDEFINED		0
///GL-Sync sync field option is Sync to Field 1 (used for Interlaced signal types)
#define ADL_GLSYNC_SYNCFIELD_BOTH			1
///GL-Sync sync field option is Sync to Both fields (used for Interlaced signal types)
#define ADL_GLSYNC_SYNCFIELD_1				2


// GL-Sync trigger edge options (unique values)

/// GL-Sync trigger edge is undefined
#define ADL_GLSYNC_TRIGGEREDGE_UNDEFINED     0
/// GL-Sync trigger edge is the rising edge
#define ADL_GLSYNC_TRIGGEREDGE_RISING        1
/// GL-Sync trigger edge is the falling edge
#define ADL_GLSYNC_TRIGGEREDGE_FALLING       2
/// GL-Sync trigger edge is both the rising and the falling edge
#define ADL_GLSYNC_TRIGGEREDGE_BOTH          3


// GL-Sync scan rate coefficient/multiplier options (unique values)

/// GL-Sync scan rate coefficient/multiplier is undefined
#define ADL_GLSYNC_SCANRATECOEFF_UNDEFINED   0
/// GL-Sync scan rate coefficient/multiplier is 5
#define ADL_GLSYNC_SCANRATECOEFF_x5          1
/// GL-Sync scan rate coefficient/multiplier is 4
#define ADL_GLSYNC_SCANRATECOEFF_x4          2
/// GL-Sync scan rate coefficient/multiplier is 3
#define ADL_GLSYNC_SCANRATECOEFF_x3          3
/// GL-Sync scan rate coefficient/multiplier is 5:2 (SMPTE)
#define ADL_GLSYNC_SCANRATECOEFF_x5_DIV_2    4
/// GL-Sync scan rate coefficient/multiplier is 2
#define ADL_GLSYNC_SCANRATECOEFF_x2          5
/// GL-Sync scan rate coefficient/multiplier is 3 : 2
#define ADL_GLSYNC_SCANRATECOEFF_x3_DIV_2    6
/// GL-Sync scan rate coefficient/multiplier is 5 : 4
#define ADL_GLSYNC_SCANRATECOEFF_x5_DIV_4    7
/// GL-Sync scan rate coefficient/multiplier is 1 (default)
#define ADL_GLSYNC_SCANRATECOEFF_x1          8
/// GL-Sync scan rate coefficient/multiplier is 4 : 5
#define ADL_GLSYNC_SCANRATECOEFF_x4_DIV_5    9
/// GL-Sync scan rate coefficient/multiplier is 2 : 3
#define ADL_GLSYNC_SCANRATECOEFF_x2_DIV_3    10
/// GL-Sync scan rate coefficient/multiplier is 1 : 2
#define ADL_GLSYNC_SCANRATECOEFF_x1_DIV_2    11
/// GL-Sync scan rate coefficient/multiplier is 2 : 5 (SMPTE)
#define ADL_GLSYNC_SCANRATECOEFF_x2_DIV_5    12
/// GL-Sync scan rate coefficient/multiplier is 1 : 3
#define ADL_GLSYNC_SCANRATECOEFF_x1_DIV_3    13
/// GL-Sync scan rate coefficient/multiplier is 1 : 4
#define ADL_GLSYNC_SCANRATECOEFF_x1_DIV_4    14
/// GL-Sync scan rate coefficient/multiplier is 1 : 5
#define ADL_GLSYNC_SCANRATECOEFF_x1_DIV_5    15


// GL-Sync port (signal presence) states (unique values)

/// GL-Sync port state is undefined
#define ADL_GLSYNC_PORTSTATE_UNDEFINED       0
/// GL-Sync port is not connected
#define ADL_GLSYNC_PORTSTATE_NOCABLE         1
/// GL-Sync port is Idle
#define ADL_GLSYNC_PORTSTATE_IDLE            2
/// GL-Sync port has an Input signal
#define ADL_GLSYNC_PORTSTATE_INPUT           3
/// GL-Sync port is Output
#define ADL_GLSYNC_PORTSTATE_OUTPUT          4


// GL-Sync LED types (used index within ADL_Workstation_GLSyncPortState_Get returned ppGlSyncLEDs array) (unique values)

/// Index into the ADL_Workstation_GLSyncPortState_Get returned ppGlSyncLEDs array for the one LED of the BNC port
#define ADL_GLSYNC_LEDTYPE_BNC               0
/// Index into the ADL_Workstation_GLSyncPortState_Get returned ppGlSyncLEDs array for the Left LED of the RJ45(1) or RJ45(2) port
#define ADL_GLSYNC_LEDTYPE_RJ45_LEFT         0
/// Index into the ADL_Workstation_GLSyncPortState_Get returned ppGlSyncLEDs array for the Right LED of the RJ45(1) or RJ45(2) port
#define ADL_GLSYNC_LEDTYPE_RJ45_RIGHT        1


// GL-Sync LED colors (unique values)

/// GL-Sync LED undefined color
#define ADL_GLSYNC_LEDCOLOR_UNDEFINED        0
/// GL-Sync LED is unlit
#define ADL_GLSYNC_LEDCOLOR_NOLIGHT          1
/// GL-Sync LED is yellow
#define ADL_GLSYNC_LEDCOLOR_YELLOW           2
/// GL-Sync LED is red
#define ADL_GLSYNC_LEDCOLOR_RED              3
/// GL-Sync LED is green
#define ADL_GLSYNC_LEDCOLOR_GREEN            4
/// GL-Sync LED is flashing green
#define ADL_GLSYNC_LEDCOLOR_FLASH_GREEN      5


// GL-Sync Port Control (refers one GL-Sync Port) (unique values)

/// Used to configure the RJ54(1) or RJ42(2) port of GL-Sync is as Idle
#define ADL_GLSYNC_PORTCNTL_NONE             0x00000000
/// Used to configure the RJ54(1) or RJ42(2) port of GL-Sync is as Output
#define ADL_GLSYNC_PORTCNTL_OUTPUT           0x00000001


// GL-Sync Mode Control (refers one Display/Controller) (bitfields)

/// Used to configure the display to use internal timing (not genlocked)
#define ADL_GLSYNC_MODECNTL_NONE             0x00000000
/// Bitfield used to configure the display as genlocked (either as Timing Client or as Timing Server)
#define ADL_GLSYNC_MODECNTL_GENLOCK          0x00000001
/// Bitfield used to configure the display as Timing Server
#define ADL_GLSYNC_MODECNTL_TIMINGSERVER     0x00000002

// GL-Sync Mode Status
/// Display is currently not genlocked
#define ADL_GLSYNC_MODECNTL_STATUS_NONE		 0x00000000
/// Display is currently genlocked
#define ADL_GLSYNC_MODECNTL_STATUS_GENLOCK   0x00000001
/// Display requires a mode switch
#define ADL_GLSYNC_MODECNTL_STATUS_SETMODE_REQUIRED 0x00000002
/// Display is capable of being genlocked
#define ADL_GLSYNC_MODECNTL_STATUS_GENLOCK_ALLOWED 0x00000004

#define ADL_MAX_GLSYNC_PORTS							8
#define ADL_MAX_GLSYNC_PORT_LEDS						8

// @}

/// \defgroup define_crossfirestate CrossfireX state of a particular adapter CrossfireX combination
// @{
#define ADL_XFIREX_STATE_NOINTERCONNECT			( 1 << 0 )	/* Dongle / cable is missing */
#define ADL_XFIREX_STATE_DOWNGRADEPIPES			( 1 << 1 )	/* CrossfireX can be enabled if pipes are downgraded */
#define ADL_XFIREX_STATE_DOWNGRADEMEM			( 1 << 2 )	/* CrossfireX cannot be enabled unless mem downgraded */
#define ADL_XFIREX_STATE_REVERSERECOMMENDED		( 1 << 3 )	/* Card reversal recommended, CrossfireX cannot be enabled. */
#define ADL_XFIREX_STATE_3DACTIVE			( 1 << 4 )	/* 3D client is active - CrossfireX cannot be safely enabled */
#define ADL_XFIREX_STATE_MASTERONSLAVE			( 1 << 5 )	/* Dongle is OK but master is on slave */
#define ADL_XFIREX_STATE_NODISPLAYCONNECT		( 1 << 6 )	/* No (valid) display connected to master card. */
#define ADL_XFIREX_STATE_NOPRIMARYVIEW			( 1 << 7 )	/* CrossfireX is enabled but master is not current primary device */
#define ADL_XFIREX_STATE_DOWNGRADEVISMEM		( 1 << 8 )	/* CrossfireX cannot be enabled unless visible mem downgraded */
#define ADL_XFIREX_STATE_LESSTHAN8LANE_MASTER		( 1 << 9 ) 	/* CrossfireX can be enabled however performance not optimal due to <8 lanes */
#define ADL_XFIREX_STATE_LESSTHAN8LANE_SLAVE		( 1 << 10 )	/* CrossfireX can be enabled however performance not optimal due to <8 lanes */
#define ADL_XFIREX_STATE_PEERTOPEERFAILED		( 1 << 11 )	/* CrossfireX cannot be enabled due to failed peer to peer test */
#define ADL_XFIREX_STATE_MEMISDOWNGRADED		( 1 << 16 )	/* Notification that memory is currently downgraded */
#define ADL_XFIREX_STATE_PIPESDOWNGRADED		( 1 << 17 )	/* Notification that pipes are currently downgraded */
#define ADL_XFIREX_STATE_XFIREXACTIVE			( 1 << 18 )	/* CrossfireX is enabled on current device */
#define ADL_XFIREX_STATE_VISMEMISDOWNGRADED		( 1 << 19 )	/* Notification that visible FB memory is currently downgraded */
#define ADL_XFIREX_STATE_INVALIDINTERCONNECTION		( 1 << 20 )	/* Cannot support current inter-connection configuration */
#define ADL_XFIREX_STATE_NONP2PMODE			( 1 << 21 )	/* CrossfireX will only work with clients supporting non P2P mode */
#define ADL_XFIREX_STATE_DOWNGRADEMEMBANKS		( 1 << 22 )	/* CrossfireX cannot be enabled unless memory banks downgraded */
#define ADL_XFIREX_STATE_MEMBANKSDOWNGRADED		( 1 << 23 )	/* Notification that memory banks are currently downgraded */
#define ADL_XFIREX_STATE_DUALDISPLAYSALLOWED		( 1 << 24 )	/* Extended desktop or clone mode is allowed. */
#define ADL_XFIREX_STATE_P2P_APERTURE_MAPPING		( 1 << 25 )	/* P2P mapping was through peer aperture */
#define ADL_XFIREX_STATE_P2PFLUSH_REQUIRED		ADL_XFIREX_STATE_P2P_APERTURE_MAPPING	/* For back compatible */
#define ADL_XFIREX_STATE_XSP_CONNECTED			( 1 << 26 )	/* There is CrossfireX side port connection between GPUs */
#define ADL_XFIREX_STATE_ENABLE_CF_REBOOT_REQUIRED	( 1 << 27 )	/* System needs a reboot bofore enable CrossfireX */
#define ADL_XFIREX_STATE_DISABLE_CF_REBOOT_REQUIRED	( 1 << 28 )	/* System needs a reboot after disable CrossfireX */
#define ADL_XFIREX_STATE_DRV_HANDLE_DOWNGRADE_KEY	( 1 << 29 )	/* Indicate base driver handles the downgrade key updating */
#define ADL_XFIREX_STATE_CF_RECONFIG_REQUIRED		( 1 << 30 )	/* CrossfireX need to be reconfigured by CCC because of a LDA chain broken */
#define ADL_XFIREX_STATE_ERRORGETTINGSTATUS		( 1 << 31 )	/* Could not obtain current status */
// @}

///////////////////////////////////////////////////////////////////////////
// ADL_DISPLAY_ADJUSTMENT_PIXELFORMAT adjustment values
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
/// \defgroup define_pixel_formats Pixel Formats values
/// This group defines the various Pixel Formats that a particular digital display can support. \n
/// Since a display can support multiple formats, these values can be bit-or'ed to indicate the various formats \n
// @{
#define ADL_DISPLAY_PIXELFORMAT_UNKNOWN             0
#define ADL_DISPLAY_PIXELFORMAT_RGB                       (1 << 0)
#define ADL_DISPLAY_PIXELFORMAT_YCRCB444                  (1 << 1)    //Limited range
#define ADL_DISPLAY_PIXELFORMAT_YCRCB422                 (1 << 2)    //Limited range
#define ADL_DISPLAY_PIXELFORMAT_RGB_LIMITED_RANGE      (1 << 3)
#define ADL_DISPLAY_PIXELFORMAT_RGB_FULL_RANGE    ADL_DISPLAY_PIXELFORMAT_RGB  //Full range
// @}

/// \defgroup define_contype Connector Type Values
/// ADLDisplayConfig.ulConnectorType defines
// @{
#define ADL_DL_DISPLAYCONFIG_CONTYPE_UNKNOWN      0
#define ADL_DL_DISPLAYCONFIG_CONTYPE_CV_NONI2C_JP 1
#define ADL_DL_DISPLAYCONFIG_CONTYPE_CV_JPN       2
#define ADL_DL_DISPLAYCONFIG_CONTYPE_CV_NA        3
#define ADL_DL_DISPLAYCONFIG_CONTYPE_CV_NONI2C_NA 4
#define ADL_DL_DISPLAYCONFIG_CONTYPE_VGA          5
#define ADL_DL_DISPLAYCONFIG_CONTYPE_DVI_D        6
#define ADL_DL_DISPLAYCONFIG_CONTYPE_DVI_I        7
#define ADL_DL_DISPLAYCONFIG_CONTYPE_HDMI_TYPE_A  8
#define ADL_DL_DISPLAYCONFIG_CONTYPE_HDMI_TYPE_B  9
#define ADL_DL_DISPLAYCONFIG_CONTYPE_DISPLAYPORT  10
// @}


///////////////////////////////////////////////////////////////////////////
// ADL_DISPLAY_DISPLAYINFO_ Definitions 
// for ADLDisplayInfo.iDisplayInfoMask and ADLDisplayInfo.iDisplayInfoValue
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
/// \defgroup define_displayinfomask Display Info Mask Values
// @{
#define ADL_DISPLAY_DISPLAYINFO_DISPLAYCONNECTED			0x00000001
#define ADL_DISPLAY_DISPLAYINFO_DISPLAYMAPPED				0x00000002
#define ADL_DISPLAY_DISPLAYINFO_NONLOCAL					0x00000004
#define ADL_DISPLAY_DISPLAYINFO_FORCIBLESUPPORTED			0x00000008
#define ADL_DISPLAY_DISPLAYINFO_GENLOCKSUPPORTED			0x00000010
#define ADL_DISPLAY_DISPLAYINFO_MULTIVPU_SUPPORTED			0x00000020

#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_SINGLE			0x00000100
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_CLONE			0x00000200

/// Legacy support for XP 
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_2VSTRETCH		0x00000400
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_2HSTRETCH		0x00000800
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_EXTENDED		0x00001000

/// More support manners  
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_NSTRETCH1GPU	0x00010000
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_NSTRETCHNGPU	0x00020000 
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_RESERVED2		0x00040000
#define ADL_DISPLAY_DISPLAYINFO_MANNER_SUPPORTED_RESERVED3		0x00080000

// @}


///////////////////////////////////////////////////////////////////////////
// ADL_ADAPTER_DISPLAY_MANNER_SUPPORTED_ Definitions 
// for ADLAdapterDisplayCap of ADL_Adapter_Display_Cap()
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
/// \defgroup define_adaptermanner Adapter Manner Support Values
// @{
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_NOTACTIVE		0x00000001
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_SINGLE			0x00000002
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_CLONE			0x00000004
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_NSTRETCH1GPU	0x00000008
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_NSTRETCHNGPU	0x00000010

/// Legacy support for XP 
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_2VSTRETCH		0x00000020
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_2HSTRETCH		0x00000040
#define ADL_ADAPTER_DISPLAYCAP_MANNER_SUPPORTED_EXTENDED		0x00000080

#define ADL_ADAPTER_DISPLAYCAP_PREFERDISPLAY_SUPPORTED			0x00000100
#define ADL_ADAPTER_DISPLAYCAP_BEZEL_SUPPORTED					0x00000200


///////////////////////////////////////////////////////////////////////////
// ADL_DISPLAY_DISPLAYMAP_MANNER_ Definitions 
// for ADLDisplayMap.iDisplayMapMask and ADLDisplayMap.iDisplayMapValue
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
#define ADL_DISPLAY_DISPLAYMAP_MANNER_RESERVED			0x00000001
#define ADL_DISPLAY_DISPLAYMAP_MANNER_NOTACTIVE			0x00000002
#define ADL_DISPLAY_DISPLAYMAP_MANNER_SINGLE			0x00000004
#define ADL_DISPLAY_DISPLAYMAP_MANNER_CLONE				0x00000008
#define ADL_DISPLAY_DISPLAYMAP_MANNER_RESERVED1			0x00000010  // Removed NSTRETCH
#define ADL_DISPLAY_DISPLAYMAP_MANNER_HSTRETCH			0x00000020
#define ADL_DISPLAY_DISPLAYMAP_MANNER_VSTRETCH			0x00000040
#define ADL_DISPLAY_DISPLAYMAP_MANNER_VLD				0x00000080

// @}

///////////////////////////////////////////////////////////////////////////
// ADL_DISPLAY_DISPLAYMAP_OPTION_ Definitions 
// for iOption in function ADL_Display_DisplayMapConfig_Get
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
#define ADL_DISPLAY_DISPLAYMAP_OPTION_GPUINFO			0x00000001

///////////////////////////////////////////////////////////////////////////
// ADL_DISPLAY_DISPLAYTARGET_ Definitions 
// for ADLDisplayTarget.iDisplayTargetMask and ADLDisplayTarget.iDisplayTargetValue
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
#define ADL_DISPLAY_DISPLAYTARGET_PREFERRED			0x00000001

///////////////////////////////////////////////////////////////////////////
// ADL_DISPLAY_POSSIBLEMAPRESULT_VALID Definitions 
// for ADLPossibleMapResult.iPossibleMapResultMask and ADLPossibleMapResult.iPossibleMapResultValue
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
#define ADL_DISPLAY_POSSIBLEMAPRESULT_VALID				0x00000001
#define ADL_DISPLAY_POSSIBLEMAPRESULT_BEZELSUPPORTED	0x00000002


///////////////////////////////////////////////////////////////////////////
// ADL_DISPLAY_MODE_ Definitions 
// for ADLMode.iModeMask, ADLMode.iModeValue, and ADLMode.iModeFlag
// (bit-vector)
///////////////////////////////////////////////////////////////////////////
/// \defgroup define_displaymode Display Mode Values
// @{
#define ADL_DISPLAY_MODE_COLOURFORMAT_565				0x00000001
#define ADL_DISPLAY_MODE_COLOURFORMAT_8888				0x00000002
#define ADL_DISPLAY_MODE_ORIENTATION_SUPPORTED_000		0x00000004
#define ADL_DISPLAY_MODE_ORIENTATION_SUPPORTED_090		0x00000008
#define ADL_DISPLAY_MODE_ORIENTATION_SUPPORTED_180		0x00000010
#define ADL_DISPLAY_MODE_ORIENTATION_SUPPORTED_270		0x00000020
#define ADL_DISPLAY_MODE_REFRESHRATE_ROUNDED			0x00000040
#define ADL_DISPLAY_MODE_REFRESHRATE_ONLY				0x00000080

#define ADL_DISPLAY_MODE_PROGRESSIVE_FLAG	0
#define ADL_DISPLAY_MODE_INTERLACED_FLAG	2
// @}

///////////////////////////////////////////////////////////////////////////
// ADL_OSMODEINFO Definitions 
///////////////////////////////////////////////////////////////////////////
/// \defgroup define_osmode OS Mode Values
// @{
#define ADL_OSMODEINFOXPOS_DEFAULT				-640
#define ADL_OSMODEINFOYPOS_DEFAULT				0
#define ADL_OSMODEINFOXRES_DEFAULT				640
#define ADL_OSMODEINFOYRES_DEFAULT				480
#define ADL_OSMODEINFOXRES_DEFAULT800			800
#define ADL_OSMODEINFOYRES_DEFAULT600			600
#define ADL_OSMODEINFOREFRESHRATE_DEFAULT		60
#define ADL_OSMODEINFOCOLOURDEPTH_DEFAULT		8
#define ADL_OSMODEINFOCOLOURDEPTH_DEFAULT16		16
#define ADL_OSMODEINFOCOLOURDEPTH_DEFAULT24		24
#define ADL_OSMODEINFOCOLOURDEPTH_DEFAULT32		32
#define ADL_OSMODEINFOORIENTATION_DEFAULT		0
#define ADL_OSMODEINFOORIENTATION_DEFAULT_WIN7	DISPLAYCONFIG_ROTATION_FORCE_UINT32
#define ADL_OSMODEFLAG_DEFAULT					0
// @}


///////////////////////////////////////////////////////////////////////////
// ADLPurposeCode Enumeration
///////////////////////////////////////////////////////////////////////////
enum ADLPurposeCode
{ 
	ADL_PURPOSECODE_NORMAL	= 0, 
	ADL_PURPOSECODE_HIDE_MODE_SWITCH, 
	ADL_PURPOSECODE_MODE_SWITCH, 
	ADL_PURPOSECODE_ATTATCH_DEVICE, 
	ADL_PURPOSECODE_DETACH_DEVICE, 
	ADL_PURPOSECODE_SETPRIMARY_DEVICE, 
	ADL_PURPOSECODE_GDI_ROTATION, 
	ADL_PURPOSECODE_ATI_ROTATION, 
};
///////////////////////////////////////////////////////////////////////////
// ADLAngle Enumeration
///////////////////////////////////////////////////////////////////////////
enum ADLAngle
{
    ADL_ANGLE_LANDSCAPE = 0,
    ADL_ANGLE_ROTATERIGHT = 90,
    ADL_ANGLE_ROTATE180 = 180,
    ADL_ANGLE_ROTATELEFT = 270,
};

///////////////////////////////////////////////////////////////////////////
// ADLOrientationDataType Enumeration
///////////////////////////////////////////////////////////////////////////
enum ADLOrientationDataType 
{ 
	ADL_ORIENTATIONTYPE_OSDATATYPE, 
	ADL_ORIENTATIONTYPE_NONOSDATATYPE 
};

///////////////////////////////////////////////////////////////////////////
// ADLPanningMode Enumeration
///////////////////////////////////////////////////////////////////////////
enum ADLPanningMode
{ 
	ADL_PANNINGMODE_NO_PANNING = 0,
	ADL_PANNINGMODE_AT_LEAST_ONE_NO_PANNING = 1,
	ADL_PANNINGMODE_ALLOW_PANNING = 2, 
};

///////////////////////////////////////////////////////////////////////////
// ADLLARGEDESKTOPTYPE Enumeration
///////////////////////////////////////////////////////////////////////////
enum ADLLARGEDESKTOPTYPE
{
	ADL_LARGEDESKTOPTYPE_NORMALDESKTOP = 0,
    ADL_LARGEDESKTOPTYPE_PSEUDOLARGEDESKTOP = 1,
    ADL_LARGEDESKTOPTYPE_VERYLARGEDESKTOP = 2,
};

// Other Definitions for internal use

// Values for ADL_Display_WriteAndReadI2CRev_Get()

#define ADL_I2C_MAJOR_API_REV           0x00000001
#define ADL_I2C_MINOR_DEFAULT_API_REV   0x00000000
#define ADL_I2C_MINOR_OEM_API_REV       0x00000001

// Values for ADL_Display_WriteAndReadI2C()
#define ADL_DL_I2C_LINE_OEM                0x00000001
#define ADL_DL_I2C_LINE_OD_CONTROL         0x00000002
#define ADL_DL_I2C_LINE_OEM2               0x00000003

// Max size of I2C data buffer
#define ADL_DL_I2C_MAXDATASIZE             0x00000040
#define ADL_DL_I2C_MAXWRITEDATASIZE        0x0000000C
#define ADL_DL_I2C_MAXADDRESSLENGTH        0x00000006
#define ADL_DL_I2C_MAXOFFSETLENGTH         0x00000004


//values for ADLDisplayProperty.iPropertyType
#define ADL_DL_DISPLAYPROPERTY_TYPE_UNKNOWN          0
#define ADL_DL_DISPLAYPROPERTY_TYPE_EXPANSIONMODE    1
#define ADL_DL_DISPLAYPROPERTY_TYPE_USEUNDERSCANSCALING	2

//values for ADLDisplayProperty.iExpansionMode
#define ADL_DL_DISPLAYPROPERTY_EXPANSIONMODE_CENTER        0
#define ADL_DL_DISPLAYPROPERTY_EXPANSIONMODE_FULLSCREEN    1
#define ADL_DL_DISPLAYPROPERTY_EXPANSIONMODE_ASPECTRATIO   2

//values for ADL_Display_DitherState_Get
#define ADL_DL_DISPLAY_DITHER_UNKNOWN    0
#define ADL_DL_DISPLAY_DITHER_DISABLED   1
#define ADL_DL_DISPLAY_DITHER_ENABLED    2

/// Display Get Cached EDID flag
#define ADL_MAX_EDIDDATA_SIZE              256 // number of UCHAR
#define ADL_MAX_EDID_EXTENSION_BLOCKS      3

#define ADL_DL_CONTROLLER_OVERLAY_ALPHA         0
#define ADL_DL_CONTROLLER_OVERLAY_ALPHAPERPIX   1

#define ADL_DL_DISPLAY_DATA_PACKET__INFO_PACKET_RESET      0x00000000
#define ADL_DL_DISPLAY_DATA_PACKET__INFO_PACKET_SET        0x00000001

///\defgroup define_display_packet Display Data Packet Types
// @{
#define ADL_DL_DISPLAY_DATA_PACKET__TYPE__AVI              0x00000001
#define ADL_DL_DISPLAY_DATA_PACKET__TYPE__RESERVED         0x00000002
#define ADL_DL_DISPLAY_DATA_PACKET__TYPE__VENDORINFO       0x00000004
// @}

// matrix types
#define ADL_GAMUT_MATRIX_SD         1   // SD matrix i.e. BT601
#define ADL_GAMUT_MATRIX_HD         2   // HD matrix i.e. BT709

///\defgroup define_clockinfo_flags Clock flags
/// Used by ADLAdapterODClockInfo.iFlag
// @{
#define ADL_DL_CLOCKINFO_FLAG_FULLSCREEN3DONLY         0x00000001
#define ADL_DL_CLOCKINFO_FLAG_ALWAYSFULLSCREEN3D       0x00000002
#define ADL_DL_CLOCKINFO_FLAG_VPURECOVERYREDUCED       0x00000004
#define ADL_DL_CLOCKINFO_FLAG_THERMALPROTECTION        0x00000008
// @}

// Supported GPUs
// ADL_Display_PowerXpressActiveGPU_Get()
#define ADL_DL_POWERXPRESS_GPU_INTEGRATED		1
#define ADL_DL_POWERXPRESS_GPU_DISCRETE			2

// Possible values for lpOperationResult
// ADL_Display_PowerXpressActiveGPU_Get()
#define ADL_DL_POWERXPRESS_SWITCH_RESULT_STARTED         1 // Switch procedure has been started
#define ADL_DL_POWERXPRESS_SWITCH_RESULT_DECLINED        2 // Switch procedure cannot be started
#define ADL_DL_POWERXPRESS_SWITCH_RESULT_ALREADY         3 // System already has required status 

// PowerXpress support version
// ADL_Display_PowerXpressVersion_Get()
#define ADL_DL_POWERXPRESS_VERSION_MAJOR			2	// Current PowerXpress support version 2.0
#define ADL_DL_POWERXPRESS_VERSION_MINOR			0

#define ADL_DL_POWERXPRESS_VERSION	(((ADL_DL_POWERXPRESS_VERSION_MAJOR) << 16) | ADL_DL_POWERXPRESS_VERSION_MINOR)

//values for ADLThermalControllerInfo.iThermalControllerDomain
#define ADL_DL_THERMAL_DOMAIN_OTHER      0
#define ADL_DL_THERMAL_DOMAIN_GPU        1

//values for ADLThermalControllerInfo.iFlags
#define ADL_DL_THERMAL_FLAG_INTERRUPT    1
#define ADL_DL_THERMAL_FLAG_FANCONTROL   2

///\defgroup define_fanctrl Fan speed cotrol
/// Values for ADLFanSpeedInfo.iFlags
// @{
#define ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ     1
#define ADL_DL_FANCTRL_SUPPORTS_PERCENT_WRITE    2
#define ADL_DL_FANCTRL_SUPPORTS_RPM_READ         4
#define ADL_DL_FANCTRL_SUPPORTS_RPM_WRITE        8
// @}

//values for ADLFanSpeedValue.iSpeedType
#define ADL_DL_FANCTRL_SPEED_TYPE_PERCENT    1
#define ADL_DL_FANCTRL_SPEED_TYPE_RPM        2

//values for ADLFanSpeedValue.iFlags
#define ADL_DL_FANCTRL_FLAG_USER_DEFINED_SPEED   1

// MVPU interfaces
#define ADL_DL_MAX_MVPU_ADAPTERS   4
#define MVPU_ADAPTER_0	      0x00000001
#define MVPU_ADAPTER_1		  0x00000002
#define MVPU_ADAPTER_2		  0x00000004
#define MVPU_ADAPTER_3		  0x00000008
#define ADL_DL_MAX_REGISTRY_PATH   256

//values for ADLMVPUStatus.iStatus
#define ADL_DL_MVPU_STATUS_OFF   0
#define ADL_DL_MVPU_STATUS_ON    1

// values for ASIC family
///\defgroup define_Asic_type Detailed asic types
/// Defines for Adapter ASIC family type
// @{
#define ADL_ASIC_UNDEFINED	0
#define ADL_ASIC_DISCRETE	(1 << 0)
#define ADL_ASIC_INTEGRATED	(1 << 1)
#define ADL_ASIC_FIREGL		(1 << 2)
#define ADL_ASIC_FIREMV		(1 << 3)
#define ADL_ASIC_XGP		(1 << 4)
#define ADL_ASIC_FUSION		(1 << 5)
// @}

///\defgroup define_detailed_timing_flags Detailed Timimg Flags
/// Defines for ADLDetailedTiming.sTimingFlags field
// @{
#define ADL_DL_TIMINGFLAG_DOUBLE_SCAN              0x0001
#define ADL_DL_TIMINGFLAG_INTERLACED               0x0002
#define ADL_DL_TIMINGFLAG_H_SYNC_POLARITY          0x0004
#define ADL_DL_TIMINGFLAG_V_SYNC_POLARITY          0x0008
// @}

///\defgroup define_modetiming_standard Timing Standards
/// Defines for ADLDisplayModeInfo.iTimingStandard field
// @{
#define ADL_DL_MODETIMING_STANDARD_CVT             0x00000001 // CVT Standard
#define ADL_DL_MODETIMING_STANDARD_GTF             0x00000002 // GFT Standard
#define ADL_DL_MODETIMING_STANDARD_DMT             0x00000004 // DMT Standard
#define ADL_DL_MODETIMING_STANDARD_CUSTOM          0x00000008 // User-defined standard
#define ADL_DL_MODETIMING_STANDARD_DRIVER_DEFAULT  0x00000010 // Remove Mode from overriden list
// @}

// \defgroup define_xserverinfo driver x-server info
/// These flags are used by ADL_XServerInfo_Get()
// @

/// Xinerama is active in the x-server, Xinerama extension may report it to be active but it
/// may not be active in x-server
#define ADL_XSERVERINFO_XINERAMAACTIVE            (1<<0)

/// RandR 1.2 is supported by driver, RandR extension may report version 1.2 
/// but driver may not support it
#define ADL_XSERVERINFO_RANDR12SUPPORTED          (1<<1)
// @


///\defgroup define_eyefinity_constants Eyefinity Definitions
// @{

#define ADL_CONTROLLERVECTOR_0		1	// ADL_CONTROLLERINDEX_0 = 0, (1 << ADL_CONTROLLERINDEX_0)
#define ADL_CONTROLLERVECTOR_1		2	// ADL_CONTROLLERINDEX_1 = 1, (1 << ADL_CONTROLLERINDEX_1)

#define ADL_DISPLAY_SLSGRID_ORIENTATION_000		0x00000001
#define ADL_DISPLAY_SLSGRID_ORIENTATION_090		0x00000002
#define ADL_DISPLAY_SLSGRID_ORIENTATION_180		0x00000004
#define ADL_DISPLAY_SLSGRID_ORIENTATION_270		0x00000008
#define ADL_DISPLAY_SLSGRID_CAP_OPTION_RELATIVETO_LANDSCAPE 	0x00000001
#define ADL_DISPLAY_SLSGRID_CAP_OPTION_RELATIVETO_CURRENTANGLE 	0x00000002
#define ADL_DISPLAY_SLSGRID_PORTAIT_MODE 						0x00000004


#define ADL_DISPLAY_SLSMAPCONFIG_GET_OPTION_RELATIVETO_LANDSCAPE 		0x00000001
#define ADL_DISPLAY_SLSMAPCONFIG_GET_OPTION_RELATIVETO_CURRENTANGLE 	0x00000002

#define ADL_DISPLAY_SLSMAPCONFIG_CREATE_OPTION_RELATIVETO_LANDSCAPE 		0x00000001
#define ADL_DISPLAY_SLSMAPCONFIG_CREATE_OPTION_RELATIVETO_CURRENTANGLE 	0x00000002

#define ADL_DISPLAY_SLSMAPCONFIG_REARRANGE_OPTION_RELATIVETO_LANDSCAPE 	0x00000001
#define ADL_DISPLAY_SLSMAPCONFIG_REARRANGE_OPTION_RELATIVETO_CURRENTANGLE 	0x00000002


#define ADL_DISPLAY_SLSGRID_RELATIVETO_LANDSCAPE 		0x00000010
#define ADL_DISPLAY_SLSGRID_RELATIVETO_CURRENTANGLE 	0x00000020


/// The bit mask identifies displays is currently in bezel mode.
#define ADL_DISPLAY_SLSMAP_BEZELMODE			0x00000010
/// The bit mask identifies displays from this map is arranged.
#define ADL_DISPLAY_SLSMAP_DISPLAYARRANGED		0x00000002
/// The bit mask identifies this map is currently in used for the current adapter.
#define ADL_DISPLAY_SLSMAP_CURRENTCONFIG		0x00000004

 ///For onlay active SLS  map info
#define ADL_DISPLAY_SLSMAPINDEXLIST_OPTION_ACTIVE		0x00000001

///For Bezel
#define ADL_DISPLAY_BEZELOFFSET_STEPBYSTEPSET			0x00000004
#define ADL_DISPLAY_BEZELOFFSET_COMMIT					0x00000008

// @}

// @}
#endif /* ADL_DEFINES_H_ */


