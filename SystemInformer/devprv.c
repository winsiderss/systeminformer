/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2023
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <settings.h>
#include <mapldr.h>

#include <devprv.h>

#include <devquery.h>

DEFINE_GUID(GUID_DEVINTERFACE_A2DP_SIDEBAND_AUDIO, 0xf3b1362f, 0xc9f4, 0x4dd1, 0x9d, 0x55, 0xe0, 0x20, 0x38, 0xa1, 0x29, 0xfb);
DEFINE_GUID(GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS, 0xbe446647, 0xf655, 0x4919, 0x8b, 0xd0, 0x12, 0x5b, 0xa5, 0xd4, 0xce, 0x65);
DEFINE_GUID(GUID_DEVINTERFACE_CHARGING_ARBITRATION, 0xec0a1cc9, 0x4294, 0x43fb, 0xbf, 0x37, 0xb8, 0x50, 0xce, 0x95, 0xf3, 0x37);
DEFINE_GUID(GUID_DEVINTERFACE_CONFIGURABLE_USBFN_CHARGER, 0x7158c35c, 0xc1bc, 0x4d90, 0xac, 0xb1, 0x80, 0x20, 0xbd, 0xe, 0x19, 0xca);
DEFINE_GUID(GUID_DEVINTERFACE_CONFIGURABLE_WIRELESS_CHARGER, 0x3612b1c8, 0x3633, 0x47d3, 0x8a, 0xf5, 0x0, 0xa4, 0xdf, 0xa0, 0x47, 0x93);
DEFINE_GUID(GUID_DEVINTERFACE_I2C, 0x2564AA4F, 0xDDDB, 0x4495, 0xB4, 0x97, 0x6A, 0xD4, 0xA8, 0x41, 0x63, 0xD7);
DEFINE_GUID(GUID_DEVINTERFACE_OPM, 0xBF4672DE, 0x6B4E, 0x4BE4, 0xA3, 0x25, 0x68, 0xA9, 0x1E, 0xA4, 0x9C, 0x09);
DEFINE_GUID(GUID_DEVINTERFACE_OPM_2_JTP, 0xE929EEA4, 0xB9F1, 0x407B, 0xAA, 0xB9, 0xAB, 0x08, 0xBB, 0x44, 0xFB, 0xF4);
DEFINE_GUID(GUID_DEVINTERFACE_OPM_2, 0x7F098726, 0x2EBB, 0x4FF3, 0xA2, 0x7F, 0x10, 0x46, 0xB9, 0x5D, 0xC5, 0x17);
DEFINE_GUID(GUID_DEVINTERFACE_OPM_3, 0x693a2cb1, 0x8c8d, 0x4ab6, 0x95, 0x55, 0x4b, 0x85, 0xef, 0x2c, 0x7c, 0x6b);
DEFINE_GUID(GUID_DEVINTERFACE_BRIGHTNESS, 0xFDE5BBA4, 0xB3F9, 0x46FB, 0xBD, 0xAA, 0x07, 0x28, 0xCE, 0x31, 0x00, 0xB4);
DEFINE_GUID(GUID_DEVINTERFACE_BRIGHTNESS_2, 0x148A3C98, 0x0ECD, 0x465A, 0xB6, 0x34, 0xB0, 0x5F, 0x19, 0x5F, 0x77, 0x39);
DEFINE_GUID(GUID_DEVINTERFACE_MIRACAST_DISPLAY, 0xaf03f190, 0x22af, 0x48cb, 0x94, 0xbb, 0xb7, 0x8e, 0x76, 0xa2, 0x51, 0x7);
DEFINE_GUID(GUID_DEVINTERFACE_BRIGHTNESS_3, 0x197a4a6e, 0x391, 0x4322, 0x96, 0xea, 0xc2, 0x76, 0xf, 0x88, 0x1d, 0x3a);
DEFINE_GUID(GUID_DEVINTERFACE_HPMI, 0xdedae202, 0x1d20, 0x4c40, 0xa6, 0xf3, 0x18, 0x97, 0xe3, 0x19, 0xd5, 0x4f);
DEFINE_GUID(GUID_DEVINTERFACE_VIRTUALIZABLE_DEVICE, 0xa13a7a93, 0x11f0, 0x4bd2, 0xa9, 0xf5, 0x6b, 0x5c, 0x5b, 0x88, 0x52, 0x7d);
DEFINE_GUID(GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB, 0x27447c21L, 0xbcc3, 0x4d07, 0xa0, 0x5b, 0xa3, 0x39, 0x5b, 0xb4, 0xee, 0xe7);
DEFINE_GUID(GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_GPP, 0x2e0e2e39L, 0x1f19, 0x4595, 0xa9, 0x06, 0x88, 0x78, 0x82, 0xe7, 0x39, 0x03);
DEFINE_GUID(GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HS_HCIBYPASS, 0x2baa4b5, 0x33b5, 0x4d97, 0xae, 0x4f, 0xe8, 0x6d, 0xde, 0x17, 0x53, 0x6f);
DEFINE_GUID(GUID_BUS_VPCI, 0xc066f39a, 0xde00, 0x4667, 0x89, 0x41, 0x33, 0x68, 0xed, 0x5d, 0x83, 0xb5);
DEFINE_GUID(GUID_VPCI_INTERFACE_STANDARD, 0x12e65e71, 0xb651, 0x4067, 0x83, 0x1a, 0x13, 0x83, 0x20, 0x3c, 0xb0, 0xcb);
DEFINE_GUID(GUID_DEVINTERFACE_VPCI, 0x57863182, 0xc948, 0x4692, 0x97, 0xe3, 0x34, 0xb5, 0x76, 0x62, 0xa3, 0xe0);
DEFINE_GUID(GUID_DEVINTERFACE_WWAN_CONTROLLER, 0x669159fd, 0xe3c0, 0x45cb, 0xbc, 0x5f, 0x95, 0x99, 0x5b, 0xcd, 0x6, 0xcd);
DEFINE_GUID(GUID_DEVINTERFACE_WDDM3_ON_VB, 0xe922004d, 0xeb9c, 0x4de1, 0x92, 0x24, 0xa9, 0xce, 0xaa, 0x95, 0x9b, 0xce);
DEFINE_GUID(GUID_DEVINTERFACE_GRAPHICSPOWER, 0xea5c6870, 0xe93c, 0x4588, 0xbe, 0xf1, 0xfe, 0xc4, 0x2f, 0xc9, 0x42, 0x9a);
DEFINE_GUID(GUID_DEVINTERFACE_GNSS, 0x3336e5e4, 0x18a, 0x4669, 0x84, 0xc5, 0xbd, 0x5, 0xf3, 0xbd, 0x36, 0x8b);
DEFINE_GUID(GUID_DEVINTERFACE_HID, 0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);
DEFINE_GUID(GUID_DEVINTERFACE_LAMP, 0x6c11e9e3, 0x8238, 0x4f0a, 0x0a, 0x19, 0xaa, 0xec, 0x26, 0xca, 0x5e, 0x98);
DEFINE_GUID(GUID_DEVINTERFACE_NET, 0xcac88484, 0x7515, 0x4c03, 0x82, 0xe6, 0x71, 0xa8, 0x7a, 0xba, 0xc3, 0x61);
DEFINE_GUID(GUID_DEVINTERFACE_NETUIO, 0x08336f60, 0x0679, 0x4c6c, 0x85, 0xd2, 0xae, 0x7c, 0xed, 0x65, 0xff, 0xf7);
DEFINE_GUID(GUID_DEVINTERFACE_NFCDTA, 0x7fd3f30b, 0x5e49, 0x4be1, 0xb3, 0xaa, 0xaf, 0x06, 0x26, 0x0d, 0x23, 0x6a);
DEFINE_GUID(GUID_DEVINTERFACE_NFCSE, 0x8dc7c854, 0xf5e5, 0x4bed, 0x81, 0x5d, 0xc, 0x85, 0xad, 0x4, 0x77, 0x25);
DEFINE_GUID(GUID_DEVINTERFACE_NFP, 0xFB3842CD, 0x9E2A, 0x4F83, 0x8F, 0xCC, 0x4B, 0x07, 0x61, 0x13, 0x9A, 0xE9);
DEFINE_GUID(GUID_DEVINTERFACE_KEYBOARD, 0x884b96c3, 0x56ef, 0x11d1, 0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd);
DEFINE_GUID(GUID_DEVINTERFACE_MODEM, 0x2c7089aa, 0x2e0e, 0x11d1, 0xb1, 0x14, 0x00, 0xc0, 0x4f, 0xc2, 0xaa, 0xe4);
DEFINE_GUID(GUID_DEVINTERFACE_MOUSE, 0x378de44c, 0x56ef, 0x11d1, 0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd );
DEFINE_GUID(GUID_DEVINTERFACE_PARALLEL, 0x97F76EF0, 0xF883, 0x11D0, 0xAF, 0x1F, 0x00, 0x00, 0xF8, 0x00, 0x84, 0x5C);
DEFINE_GUID(GUID_DEVINTERFACE_PARCLASS, 0x811FC6A5, 0xF728, 0x11D0, 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1);
DEFINE_GUID(GUID_PARALLEL_DEVICE, 0x97F76EF0, 0xF883, 0x11D0, 0xAF, 0x1F, 0x00, 0x00, 0xF8, 0x00, 0x84, 0x5C);
DEFINE_GUID(GUID_PARCLASS_DEVICE, 0x811FC6A5, 0xF728, 0x11D0, 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1);
DEFINE_GUID(GUID_DEVINTERFACE_DISPLAY_ADAPTER, 0x5b45201d, 0xf2f2, 0x4f3b, 0x85, 0xbb, 0x30, 0xff, 0x1f, 0x95, 0x35, 0x99);
DEFINE_GUID(GUID_DEVINTERFACE_MONITOR, 0xe6f07b5f, 0xee97, 0x4a90, 0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7);
DEFINE_GUID(GUID_DEVICE_BATTERY, 0x72631e54L, 0x78A4, 0x11d0, 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a);
DEFINE_GUID(GUID_DEVICE_APPLICATIONLAUNCH_BUTTON, 0x629758eel, 0x986e, 0x4d9e, 0x8e, 0x47, 0xde, 0x27, 0xf8, 0xab, 0x05, 0x4d);
DEFINE_GUID(GUID_DEVICE_SYS_BUTTON, 0x4AFA3D53L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57);
DEFINE_GUID(GUID_DEVICE_LID, 0x4AFA3D52L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57);
DEFINE_GUID(GUID_DEVICE_THERMAL_ZONE, 0x4AFA3D51L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57);
DEFINE_GUID(GUID_DEVICE_FAN, 0x05ecd13dL, 0x81da, 0x4a2a, 0x8a, 0x4c, 0x52, 0x4f, 0x23, 0xdd, 0x4d, 0xc9);
DEFINE_GUID(GUID_DEVICE_PROCESSOR, 0x97fadb10L, 0x4e33, 0x40ae, 0x35, 0x9c, 0x8b, 0xef, 0x02, 0x9d, 0xbd, 0xd0);
DEFINE_GUID(GUID_DEVICE_MEMORY, 0x3fd0f03dL, 0x92e0, 0x45fb, 0xb7, 0x5c, 0x5e, 0xd8, 0xff, 0xb0, 0x10, 0x21);
DEFINE_GUID(GUID_DEVICE_ACPI_TIME, 0x97f99bf6L, 0x4497, 0x4f18, 0xbb, 0x22, 0x4b, 0x9f, 0xb2, 0xfb, 0xef, 0x9c);
DEFINE_GUID(GUID_DEVICE_MESSAGE_INDICATOR, 0XCD48A365L, 0xfa94, 0x4ce2, 0xa2, 0x32, 0xa1, 0xb7, 0x64, 0xe5, 0xd8, 0xb4);
DEFINE_GUID(GUID_CLASS_INPUT, 0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30);
DEFINE_GUID(GUID_DEVINTERFACE_THERMAL_COOLING, 0xdbe4373d, 0x3c81, 0x40cb, 0xac, 0xe4, 0xe0, 0xe5, 0xd0, 0x5f, 0xc, 0x9f);
DEFINE_GUID(GUID_DEVINTERFACE_THERMAL_MANAGER, 0x927ec093, 0x69a4, 0x4bc0, 0xbd, 0x2, 0x71, 0x16, 0x64, 0x71, 0x44, 0x63);
DEFINE_GUID(GUID_DEVINTERFACE_PWM_CONTROLLER, 0x60824b4c, 0xeed1, 0x4c9c, 0xb4, 0x9c, 0x1b, 0x96, 0x14, 0x61, 0xa8, 0x19);
DEFINE_GUID(GUID_DEVINTERFACE_USBPRINT, 0x28d78fad, 0x5a12, 0x11d1, 0xae, 0x5b, 0x0, 0x0, 0xf8, 0x3, 0xa8, 0xc2);
DEFINE_GUID(GUID_DEVINTERFACE_IPPUSB_PRINT, 0xf2f40381, 0xf46d, 0x4e51, 0xbc, 0xe7, 0x62, 0xde, 0x6c, 0xf2, 0xd0, 0x98);
DEFINE_GUID(GUID_DEVINTERFACE_VM_GENCOUNTER, 0x3ff2c92b, 0x6598, 0x4e60, 0x8e, 0x1c, 0x0c, 0xcf, 0x49, 0x27, 0xe3, 0x19);
DEFINE_GUID(GUID_DEVINTERFACE_BIOMETRIC_READER, 0xe2b5183a, 0x99ea, 0x4cc3, 0xad, 0x6b, 0x80, 0xca, 0x8d, 0x71, 0x5b, 0x80);
DEFINE_GUID(GUID_DEVINTERFACE_SMARTCARD_READER, 0x50DD5230, 0xBA8A, 0x11D1, 0xBF, 0x5D, 0x00, 0x00, 0xF8, 0x05, 0xF5, 0x30);
DEFINE_GUID(GUID_DEVINTERFACE_DMR, 0xD0875FB4, 0x2196, 0x4c7a, 0xA6, 0x3D, 0xE4, 0x16, 0xAD, 0xDD, 0x60, 0xA1);
DEFINE_GUID(GUID_DEVINTERFACE_DMP, 0x25B4E268, 0x2A05, 0x496e, 0x80, 0x3B, 0x26, 0x68, 0x37, 0xFB, 0xDA, 0x4B);
DEFINE_GUID(GUID_DEVINTERFACE_DMS, 0xC96037AE, 0xA558, 0x4470, 0xB4, 0x32, 0x11, 0x5A, 0x31, 0xB8, 0x55, 0x53);
DEFINE_GUID(GUID_DEVINTERFACE_ENHANCED_STORAGE_SILO, 0x3897f6a4, 0xfd35, 0x4bc8, 0xa0, 0xb7, 0x5d, 0xbb, 0xa3, 0x6a, 0xda, 0xfa);
DEFINE_GUID(GUID_DEVINTERFACE_WPD, 0x6AC27878, 0xA6FA, 0x4155, 0xBA, 0x85, 0xF9, 0x8F, 0x49, 0x1D, 0x4F, 0x33);
DEFINE_GUID(GUID_DEVINTERFACE_WPD_PRIVATE, 0xBA0C718F, 0x4DED, 0x49B7, 0xBD, 0xD3, 0xFA, 0xBE, 0x28, 0x66, 0x12, 0x11);
DEFINE_GUID(GUID_DEVINTERFACE_WPD_SERVICE, 0x9EF44F80, 0x3D64, 0x4246, 0xA6, 0xAA, 0x20, 0x6F, 0x32, 0x8D, 0x1E, 0xDC);
DEFINE_GUID(GUID_DEVINTERFACE_SENSOR, 0XBA1BB692, 0X9B7A, 0X4833, 0X9A, 0X1E, 0X52, 0X5E, 0XD1, 0X34, 0XE7, 0XE2);
DEFINE_GUID(GUID_DEVINTERFACE_IMAGE, 0x6bdd1fc6L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f);
DEFINE_GUID(GUID_DEVINTERFACE_SIDESHOW, 0x152e5811, 0xfeb9, 0x4b00, 0x90, 0xf4, 0xd3, 0x29, 0x47, 0xae, 0x16, 0x81);
DEFINE_GUID(GUID_DEVINTERFACE_WIFIDIRECT_DEVICE, 0x439b20af, 0x8955, 0x405b, 0x99, 0xf0, 0xa6, 0x2a, 0xf0, 0xc6, 0x8d, 0x43);
DEFINE_GUID(GUID_AEPSERVICE_WIFIDIRECT_DEVICE, 0xcc29827c, 0x9caf, 0x4928, 0x99, 0xa9, 0x18, 0xf7, 0xc2, 0x38, 0x13, 0x89);
DEFINE_GUID(GUID_DEVINTERFACE_ASP_INFRA_DEVICE, 0xff823995, 0x7a72, 0x4c80, 0x87, 0x57, 0xc6, 0x7e, 0xe1, 0x3d, 0x1a, 0x49);
DEFINE_GUID(GUID_DXGKDDI_MITIGABLE_DEVICE_INTERFACE, 0x1387f270, 0x121a, 0x4a4a, 0xb2, 0x5e, 0x3b, 0x15, 0x89, 0x97, 0x6c, 0x61);
DEFINE_GUID(GUID_DXGKDDI_FLEXIOV_DEVICE_INTERFACE, 0x7b73a997, 0x48e8, 0x4cab, 0x9f, 0xab, 0xe0, 0x77, 0x4b, 0x44, 0xf5, 0x99);
DEFINE_GUID(GUID_SRIOV_DEVICE_INTERFACE_STANDARD, 0x937ee9b6, 0xed3, 0x411c, 0x98, 0x2b, 0x1f, 0x56, 0x4a, 0xfb, 0xab, 0xd3);
DEFINE_GUID(GUID_MITIGABLE_DEVICE_INTERFACE, 0xadfd9567, 0x4245, 0x497e, 0x85, 0x72, 0x78, 0xd4, 0x5c, 0x16, 0x22, 0xb8);
DEFINE_GUID(GUID_IO_VOLUME_DEVICE_INTERFACE, 0x53f5630d, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
DEFINE_GUID(GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE,  0x4d51e930, 0x750d, 0x4a36, 0xa9, 0xf7, 0x91, 0xdc, 0x54, 0x0f, 0xcd, 0x30);
DEFINE_GUID(GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE, 0xef8ba08f, 0x148d, 0x4116, 0x83, 0xef, 0xa2, 0x67, 0x9d, 0xfc, 0x3f, 0xa5);
DEFINE_GUID(GUID_BLUETOOTHLE_DEVICE_INTERFACE, 0x781aee18, 0x7733, 0x4ce4, 0xad, 0xd0, 0x91, 0xf4, 0x1c, 0x67, 0xb5, 0x92);
DEFINE_GUID(GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, 0x6e3bb679, 0x4372, 0x40c8, 0x9e, 0xaa, 0x45, 0x09, 0xdf, 0x26, 0x0c, 0xd8);

#include <SetupAPI.h>
#include <cfgmgr32.h>
#include <wdmguid.h>
#include <bthdef.h>
#include <devguid.h>
#include <usbiodef.h>

#undef DEFINE_GUID
#include <pciprop.h>
#include <ntddstor.h>

static PH_STRINGREF RootInstanceId = PH_STRINGREF_INIT(L"HTREE\\ROOT\\0");
PPH_OBJECT_TYPE PhDeviceTreeType = NULL;
PPH_OBJECT_TYPE PhDeviceItemType = NULL;
PPH_OBJECT_TYPE PhDeviceNotifyType = NULL;
static PPH_OBJECT_TYPE PhpDeviceInfoType = NULL;
static PPH_DEVICE_TREE PhpDeviceTree = NULL;
static PH_FAST_LOCK PhpDeviceTreeLock = PH_FAST_LOCK_INIT;
static HCMNOTIFICATION PhpDeviceNotification = NULL;
static HCMNOTIFICATION PhpDeviceInterfaceNotification = NULL;
static PH_FAST_LOCK PhpDeviceNotifyLock = PH_FAST_LOCK_INIT;
static HANDLE PhpDeviceNotifyEvent = NULL;
static LIST_ENTRY PhpDeviceNotifyList = { 0 };

#if !defined(NTDDI_WIN10_NI) || (NTDDI_VERSION < NTDDI_WIN10_NI)
// Note: This propkey is required for building with 22H1 and older Windows SDK (dmex)
DEFINE_DEVPROPKEY(DEVPKEY_Device_FirmwareVendor, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 26);   // DEVPROP_TYPE_STRING
#endif

#define DEVPROP_FILL_FLAG_CLASS  0x00000001

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
typedef
VOID
NTAPI
PH_DEVICE_PROPERTY_FILL_CALLBACK(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    );
typedef PH_DEVICE_PROPERTY_FILL_CALLBACK* PPH_DEVICE_PROPERTY_FILL_CALLBACK;

typedef struct _PH_DEVICE_PROPERTY_TABLE_ENTRY
{
    PH_DEVICE_PROPERTY_CLASS PropClass;
    const DEVPROPKEY* PropKey;
    PPH_DEVICE_PROPERTY_FILL_CALLBACK Callback;
    ULONG CallbackFlags;
} PH_DEVICE_PROPERTY_TABLE_ENTRY, *PPH_DEVICE_PROPERTY_TABLE_ENTRY;

BOOLEAN PhpGetDevicePropertyGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PGUID Guid
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(GUID);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Guid,
        sizeof(GUID),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_GUID))
    {
        return TRUE;
    }

    RtlZeroMemory(Guid, sizeof(GUID));

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PGUID Guid
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(GUID);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Guid,
        sizeof(GUID),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_GUID))
    {
        return TRUE;
    }

    RtlZeroMemory(Guid, sizeof(GUID));

    return FALSE;
}

BOOLEAN PhpGetClassPropertyGuid(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PGUID Guid
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(GUID);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Guid,
        sizeof(GUID),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_GUID))
    {
        return TRUE;
    }

    RtlZeroMemory(Guid, sizeof(GUID));

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyUInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG64);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG64),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyUInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG64);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG64),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyUInt64(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PULONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG64);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG64),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyUInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyUInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyUInt32(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PULONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyInt32(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PLONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyNTSTATUS(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PNTSTATUS Status
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(NTSTATUS);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Status,
        sizeof(NTSTATUS),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_NTSTATUS))
    {
        return TRUE;
    }

    *Status = 0;

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyNTSTATUS(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PNTSTATUS Status
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(NTSTATUS);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Status,
        sizeof(NTSTATUS),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_NTSTATUS))
    {
        return TRUE;
    }

    *Status = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyNTSTATUS(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PNTSTATUS Status
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(NTSTATUS);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Status,
        sizeof(NTSTATUS),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_NTSTATUS))
    {
        return TRUE;
    }

    *Status = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyBoolean(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PBOOLEAN Boolean
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    DEVPROP_BOOLEAN boolean;
    ULONG requiredLength = sizeof(DEVPROP_BOOLEAN);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&boolean,
        sizeof(DEVPROP_BOOLEAN),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_BOOLEAN))
    {
        *Boolean = boolean == DEVPROP_TRUE;

        return TRUE;
    }

    *Boolean = FALSE;

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyBoolean(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PBOOLEAN Boolean
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    DEVPROP_BOOLEAN boolean;
    ULONG requiredLength = sizeof(DEVPROP_BOOLEAN);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&boolean,
        sizeof(DEVPROP_BOOLEAN),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_BOOLEAN))
    {
        *Boolean = boolean == DEVPROP_TRUE;

        return TRUE;
    }

    *Boolean = FALSE;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyBoolean(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PBOOLEAN Boolean
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    DEVPROP_BOOLEAN boolean;
    ULONG requiredLength = sizeof(DEVPROP_BOOLEAN);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&boolean,
        sizeof(DEVPROP_BOOLEAN),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_BOOLEAN))
    {
        *Boolean = boolean == DEVPROP_TRUE;

        return TRUE;
    }

    *Boolean = FALSE;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyTimeStamp(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    FILETIME fileTime;
    ULONG requiredLength = sizeof(FILETIME);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&fileTime,
        sizeof(FILETIME),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_FILETIME))
    {
        TimeStamp->HighPart = fileTime.dwHighDateTime;
        TimeStamp->LowPart = fileTime.dwLowDateTime;

        return TRUE;
    }

    TimeStamp->QuadPart = 0;

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyTimeStamp(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    FILETIME fileTime;
    ULONG requiredLength = sizeof(FILETIME);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&fileTime,
        sizeof(FILETIME),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_FILETIME))
    {
        TimeStamp->HighPart = fileTime.dwHighDateTime;
        TimeStamp->LowPart = fileTime.dwLowDateTime;

        return TRUE;
    }

    TimeStamp->QuadPart = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyTimeStamp(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    FILETIME fileTime;
    ULONG requiredLength = sizeof(FILETIME);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&fileTime,
        sizeof(FILETIME),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_FILETIME))
    {
        TimeStamp->HighPart = fileTime.dwHighDateTime;
        TimeStamp->LowPart = fileTime.dwLowDateTime;

        return TRUE;
    }

    TimeStamp->QuadPart = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_STRING* String
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *String = NULL;

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_STRING) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    *String = PhCreateString(buffer);

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetDeviceInterfacePropertyString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_STRING* String
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *String = NULL;

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_STRING) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    *String = PhCreateString(buffer);

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetClassPropertyString(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PPH_STRING* String
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *String = NULL;

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        Flags
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_STRING) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        Flags
        );
    if (!result)
    {
        goto Exit;
    }

    *String = PhCreateString(buffer);

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetDevicePropertyStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_LIST* StringList
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;
    PPH_LIST stringList;

    *StringList = NULL;

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (devicePropertyType != DEVPROP_TYPE_STRING_LIST))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    stringList = PhCreateList(1);

    for (PZZWSTR item = buffer;;)
    {
        UNICODE_STRING string;

        RtlInitUnicodeString(&string, item);

        if (string.Length == 0)
        {
            break;
        }

        PhAddItemList(stringList, PhCreateStringFromUnicodeString(&string));

        item = PTR_ADD_OFFSET(item, string.MaximumLength);
    }

    *StringList = stringList;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetDeviceInterfacePropertyStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_LIST* StringList
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;
    PPH_LIST stringList;

    *StringList = NULL;

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (devicePropertyType != DEVPROP_TYPE_STRING_LIST))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    stringList = PhCreateList(1);

    for (PZZWSTR item = buffer;;)
    {
        UNICODE_STRING string;

        RtlInitUnicodeString(&string, item);

        if (string.Length == 0)
        {
            break;
        }

        PhAddItemList(stringList, PhCreateStringFromUnicodeString(&string));

        item = PTR_ADD_OFFSET(item, string.MaximumLength);
    }

    *StringList = stringList;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetClassPropertyStringList(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PPH_LIST* StringList
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;
    PPH_LIST stringList;

    *StringList = NULL;

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        Flags
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (devicePropertyType != DEVPROP_TYPE_STRING_LIST))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        Flags
        );
    if (!result)
    {
        goto Exit;
    }

    stringList = PhCreateList(1);

    for (PZZWSTR item = buffer;;)
    {
        PH_STRINGREF string;

        PhInitializeStringRefLongHint(&string, item);

        if (string.Length == 0)
        {
            break;
        }

        PhAddItemList(stringList, PhCreateString2(&string));

        item = PTR_ADD_OFFSET(item, string.Length + sizeof(UNICODE_NULL));
    }

    *StringList = stringList;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetDevicePropertyBinary(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PBYTE* Buffer,
    _Out_ PULONG Size
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *Buffer = NULL;
    *Size = 0;

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_BINARY) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    *Size = requiredLength;
    *Buffer = buffer;
    buffer = NULL;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetDeviceInterfacePropertyBinary(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PBYTE* Buffer,
    _Out_ PULONG Size
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *Buffer = NULL;
    *Size = 0;

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_BINARY) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    *Size = requiredLength;
    *Buffer = buffer;
    buffer = NULL;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetClassPropertyBinary(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PBYTE* Buffer,
    _Out_ PULONG Size
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *Buffer = NULL;
    *Size = 0;

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        Flags
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_BINARY) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        Flags
        );
    if (!result)
    {
        goto Exit;
    }

    *Size = requiredLength;
    *Buffer = buffer;
    buffer = NULL;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeString;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyString(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->String
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyString(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->String
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyString(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->String
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyString(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->String
                );
        }
    }

    if (Property->Valid)
    {
        Property->AsString = Property->String;
        PhReferenceObject(Property->AsString);
    }
}

VOID PhpDevPropFillUInt64Common(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt64;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyUInt64(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->UInt64
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyUInt64(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->UInt64
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyUInt64(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->UInt64
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyUInt64(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->UInt64
                );
        }
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillUInt64Common(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatI64U(&format[0], Property->UInt64);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt64Hex(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillUInt64Common(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        PH_FORMAT format[2];

        PhInitFormatI64X(&format[1], Property->UInt64);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);
    }
}

VOID PhpDevPropFillUInt32Common(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt32;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyUInt32(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->UInt32
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyUInt32(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->UInt32
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyUInt32(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->UInt32
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyUInt32(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->UInt32
                );
        }
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillUInt32Common(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatU(&format[0], Property->UInt32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt32Hex(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillUInt32Common(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatIX(&format[1], Property->UInt32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt32;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyInt32(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->Int32
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyInt32(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->Int32
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyInt32(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->Int32
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyInt32(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->Int32
                );
        }
    }

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatD(&format[0], Property->Int32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillNTSTATUS(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeNTSTATUS;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyNTSTATUS(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->Status
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyNTSTATUS(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->Status
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyNTSTATUS(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->Status
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyNTSTATUS(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->Status
                );
        }
    }

    if (Property->Valid && Property->Status != STATUS_SUCCESS)
    {
        Property->AsString = PhGetStatusMessage(Property->Status, 0);
    }
}

typedef struct _PH_WELL_KNOWN_GUID
{
    const GUID* Guid;
    PH_STRINGREF Symbol;
} PH_WELL_KNOWN_GUID, *PPH_WELL_KNOWN_GUID;
#define PH_DEFINE_WELL_KNOWN_GUID(guid) const PH_WELL_KNOWN_GUID PH_WELL_KNOWN_##guid = { &guid, PH_STRINGREF_INIT(TEXT(#guid)) }

PH_DEFINE_WELL_KNOWN_GUID(GUID_HWPROFILE_QUERY_CHANGE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_HWPROFILE_CHANGE_CANCELLED);
PH_DEFINE_WELL_KNOWN_GUID(GUID_HWPROFILE_CHANGE_COMPLETE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_INTERFACE_ARRIVAL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_INTERFACE_REMOVAL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_TARGET_DEVICE_QUERY_REMOVE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_TARGET_DEVICE_REMOVE_CANCELLED);
PH_DEFINE_WELL_KNOWN_GUID(GUID_TARGET_DEVICE_REMOVE_COMPLETE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PNP_CUSTOM_NOTIFICATION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PNP_POWER_NOTIFICATION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PNP_POWER_SETTING_CHANGE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_TARGET_DEVICE_TRANSPORT_RELATIONS_CHANGED);
PH_DEFINE_WELL_KNOWN_GUID(GUID_KERNEL_SOFT_RESTART_PREPARE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_KERNEL_SOFT_RESTART_CANCEL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_RECOVERY_PCI_PREPARE_SHUTDOWN);
PH_DEFINE_WELL_KNOWN_GUID(GUID_RECOVERY_NVMED_PREPARE_SHUTDOWN);
PH_DEFINE_WELL_KNOWN_GUID(GUID_KERNEL_SOFT_RESTART_FINALIZE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_BUS_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_BUS_INTERFACE_STANDARD2);
PH_DEFINE_WELL_KNOWN_GUID(GUID_ARBITER_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_TRANSLATOR_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_ACPI_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_INT_ROUTE_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCMCIA_BUS_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_ACPI_REGS_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_LEGACY_DEVICE_DETECTION_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_DEVICE_PRESENT_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_MF_ENUMERATION_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_REENUMERATE_SELF_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_AGP_TARGET_BUS_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_ACPI_CMOS_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_ACPI_PORT_RANGES_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_ACPI_INTERFACE_STANDARD2);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PNP_LOCATION_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_EXPRESS_LINK_QUIESCENT_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_EXPRESS_ROOT_PORT_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_MSIX_TABLE_CONFIG_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_D3COLD_SUPPORT_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PROCESSOR_PCC_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_VIRTUALIZATION_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCC_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCC_INTERFACE_INTERNAL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_THERMAL_COOLING_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DMA_CACHE_COHERENCY_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_RESET_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_IOMMU_BUS_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_SECURITY_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SCM_BUS_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SECURE_DRIVER_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SDEV_IDENTIFIER_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SCM_BUS_NVD_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SCM_BUS_LD_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SCM_PHYSICAL_NVDIMM_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PNP_EXTENDED_ADDRESS_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_D3COLD_AUX_POWER_AND_TIMING_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_FPGA_CONTROL_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_PTM_CONTROL_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_RESOURCE_UPDATE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_NPEM_CONTROL_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PCI_ATS_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_INTERNAL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_PCMCIA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_PCI);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_ISAPNP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_EISA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_MCA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_SERENUM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_USB);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_LPTENUM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_USBPRINT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_DOT4PRT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_1394);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_HID);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_AVC);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_IRDA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_SD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_ACPI);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_SW_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_SCM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_POWER_DEVICE_ENABLE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_POWER_DEVICE_TIMEOUTS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_POWER_DEVICE_WAKE_ENABLE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_WUDF_DEVICE_HOST_PROBLEM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PARTITION_UNIT_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_QUERY_CRASHDUMP_FUNCTIONS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_DISK);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_CDROM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_PARTITION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_TAPE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_WRITEONCEDISK);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_VOLUME);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_MEDIUMCHANGER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_FLOPPY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_CDCHANGER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_STORAGEPORT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_VMLUN);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_SES);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_ZNSDISK);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_SERVICE_VOLUME);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_HIDDEN_VOLUME);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_UNIFIED_ACCESS_RPMB);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_SCM_PHYSICAL_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SCM_PD_HEALTH_NOTIFICATION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SCM_PD_PASSTHROUGH_INVDIMM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_COMPORT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BTHPORT_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BTH_RFCOMM_SERVICE_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_1394);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_1394DEBUG);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_61883);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_ADAPTER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_APMSUPPORT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_AVC);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_BATTERY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_BIOMETRIC);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_BLUETOOTH);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_CAMERA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_CDROM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_COMPUTEACCELERATOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_COMPUTER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_DECODER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_DISKDRIVE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_DISPLAY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_DOT4);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_DOT4PRINT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_EHSTORAGESILO);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_ENUM1394);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_EXTENSION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FDC);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FIRMWARE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FLOPPYDISK);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_GENERIC);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_GPS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_HDC);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_HIDCLASS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_HOLOGRAPHIC);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_IMAGE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_INFINIBAND);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_INFRARED);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_KEYBOARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_LEGACYDRIVER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MEDIA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MEDIUM_CHANGER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MEMORY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MODEM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MONITOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MOUSE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MTD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MULTIFUNCTION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_MULTIPORTSERIAL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_NET);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_NETCLIENT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_NETDRIVER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_NETSERVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_NETTRANS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_NETUIO);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_NODRIVER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PCMCIA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PNPPRINTERS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PORTS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PRIMITIVE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PRINTER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PRINTERUPGRADE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PRINTQUEUE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_PROCESSOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SBP2);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SCMDISK);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SCMVOLUME);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SCSIADAPTER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SECURITYACCELERATOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SENSOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SIDESHOW);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SMARTCARDREADER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SMRDISK);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SMRVOLUME);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SOFTWARECOMPONENT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SOUND);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_SYSTEM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_TAPEDRIVE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_UNKNOWN);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_UCM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_USB);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_VOLUME);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_VOLUMESNAPSHOT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_WCEUSBS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_WPD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_TOP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_ACTIVITYMONITOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_UNDELETE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_ANTIVIRUS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_REPLICATION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_CONTINUOUSBACKUP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_CONTENTSCREENER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_QUOTAMANAGEMENT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_SYSTEMRECOVERY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_CFSMETADATASERVER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_HSM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_COMPRESSION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_ENCRYPTION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_VIRTUALIZATION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_PHYSICALQUOTAMANAGEMENT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_OPENFILEBACKUP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_SECURITYENHANCER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_COPYPROTECTION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_BOTTOM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_SYSTEM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVCLASS_FSFILTER_INFRASTRUCTURE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_USB_HUB);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_USB_BILLBOARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_USB_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_USB_HOST_CONTROLLER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_A2DP_SIDEBAND_AUDIO);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_CHARGING_ARBITRATION);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_CONFIGURABLE_USBFN_CHARGER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_CONFIGURABLE_WIRELESS_CHARGER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_I2C);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_OPM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_OPM_2_JTP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_OPM_2);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_OPM_3);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_BRIGHTNESS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_BRIGHTNESS_2);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_MIRACAST_DISPLAY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_BRIGHTNESS_3);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_HPMI);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_VIRTUALIZABLE_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_GPP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HS_HCIBYPASS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_VPCI);
PH_DEFINE_WELL_KNOWN_GUID(GUID_VPCI_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_VPCI);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_WWAN_CONTROLLER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_WDDM3_ON_VB);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_GRAPHICSPOWER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_GNSS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_HID);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_LAMP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_NET);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_NETUIO);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_NFCDTA);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_NFCSE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_NFP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_KEYBOARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_MODEM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_MOUSE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_PARALLEL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_PARCLASS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PARALLEL_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_PARCLASS_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_DISPLAY_ADAPTER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_MONITOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_BATTERY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_APPLICATIONLAUNCH_BUTTON);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_SYS_BUTTON);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_LID);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_THERMAL_ZONE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_FAN);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_PROCESSOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_MEMORY);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_ACPI_TIME);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_MESSAGE_INDICATOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_CLASS_INPUT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_THERMAL_COOLING);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_THERMAL_MANAGER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_PWM_CONTROLLER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_USBPRINT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_IPPUSB_PRINT);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_VM_GENCOUNTER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_BIOMETRIC_READER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_SMARTCARD_READER);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_DMR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_DMP);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_DMS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_ENHANCED_STORAGE_SILO);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_WPD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_WPD_PRIVATE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_WPD_SERVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_SENSOR);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_IMAGE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_SIDESHOW);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_WIFIDIRECT_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_AEPSERVICE_WIFIDIRECT_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVINTERFACE_ASP_INFRA_DEVICE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DXGKDDI_MITIGABLE_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DXGKDDI_FLEXIOV_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_SRIOV_DEVICE_INTERFACE_STANDARD);
PH_DEFINE_WELL_KNOWN_GUID(GUID_MITIGABLE_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_IO_VOLUME_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BLUETOOTHLE_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE);

static PH_INITONCE PhpWellKnownGuidsInitOnce = PH_INITONCE_INIT;
static const PH_WELL_KNOWN_GUID* PhpWellKnownGuids[] =
{
    &PH_WELL_KNOWN_GUID_HWPROFILE_QUERY_CHANGE,
    &PH_WELL_KNOWN_GUID_HWPROFILE_CHANGE_CANCELLED,
    &PH_WELL_KNOWN_GUID_HWPROFILE_CHANGE_COMPLETE,
    &PH_WELL_KNOWN_GUID_DEVICE_INTERFACE_ARRIVAL,
    &PH_WELL_KNOWN_GUID_DEVICE_INTERFACE_REMOVAL,
    &PH_WELL_KNOWN_GUID_TARGET_DEVICE_QUERY_REMOVE,
    &PH_WELL_KNOWN_GUID_TARGET_DEVICE_REMOVE_CANCELLED,
    &PH_WELL_KNOWN_GUID_TARGET_DEVICE_REMOVE_COMPLETE,
    &PH_WELL_KNOWN_GUID_PNP_CUSTOM_NOTIFICATION,
    &PH_WELL_KNOWN_GUID_PNP_POWER_NOTIFICATION,
    &PH_WELL_KNOWN_GUID_PNP_POWER_SETTING_CHANGE,
    &PH_WELL_KNOWN_GUID_TARGET_DEVICE_TRANSPORT_RELATIONS_CHANGED,
    &PH_WELL_KNOWN_GUID_KERNEL_SOFT_RESTART_PREPARE,
    &PH_WELL_KNOWN_GUID_KERNEL_SOFT_RESTART_CANCEL,
    &PH_WELL_KNOWN_GUID_RECOVERY_PCI_PREPARE_SHUTDOWN,
    &PH_WELL_KNOWN_GUID_RECOVERY_NVMED_PREPARE_SHUTDOWN,
    &PH_WELL_KNOWN_GUID_KERNEL_SOFT_RESTART_FINALIZE,
    &PH_WELL_KNOWN_GUID_BUS_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_PCI_BUS_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_PCI_BUS_INTERFACE_STANDARD2,
    &PH_WELL_KNOWN_GUID_ARBITER_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_TRANSLATOR_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_ACPI_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_INT_ROUTE_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_PCMCIA_BUS_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_ACPI_REGS_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_LEGACY_DEVICE_DETECTION_STANDARD,
    &PH_WELL_KNOWN_GUID_PCI_DEVICE_PRESENT_INTERFACE,
    &PH_WELL_KNOWN_GUID_MF_ENUMERATION_INTERFACE,
    &PH_WELL_KNOWN_GUID_REENUMERATE_SELF_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_AGP_TARGET_BUS_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_ACPI_CMOS_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_ACPI_PORT_RANGES_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_ACPI_INTERFACE_STANDARD2,
    &PH_WELL_KNOWN_GUID_PNP_LOCATION_INTERFACE,
    &PH_WELL_KNOWN_GUID_PCI_EXPRESS_LINK_QUIESCENT_INTERFACE,
    &PH_WELL_KNOWN_GUID_PCI_EXPRESS_ROOT_PORT_INTERFACE,
    &PH_WELL_KNOWN_GUID_MSIX_TABLE_CONFIG_INTERFACE,
    &PH_WELL_KNOWN_GUID_D3COLD_SUPPORT_INTERFACE,
    &PH_WELL_KNOWN_GUID_PROCESSOR_PCC_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_PCI_VIRTUALIZATION_INTERFACE,
    &PH_WELL_KNOWN_GUID_PCC_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_PCC_INTERFACE_INTERNAL,
    &PH_WELL_KNOWN_GUID_THERMAL_COOLING_INTERFACE,
    &PH_WELL_KNOWN_GUID_DMA_CACHE_COHERENCY_INTERFACE,
    &PH_WELL_KNOWN_GUID_DEVICE_RESET_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_IOMMU_BUS_INTERFACE,
    &PH_WELL_KNOWN_GUID_PCI_SECURITY_INTERFACE,
    &PH_WELL_KNOWN_GUID_SCM_BUS_INTERFACE,
    &PH_WELL_KNOWN_GUID_SECURE_DRIVER_INTERFACE,
    &PH_WELL_KNOWN_GUID_SDEV_IDENTIFIER_INTERFACE,
    &PH_WELL_KNOWN_GUID_SCM_BUS_NVD_INTERFACE,
    &PH_WELL_KNOWN_GUID_SCM_BUS_LD_INTERFACE,
    &PH_WELL_KNOWN_GUID_SCM_PHYSICAL_NVDIMM_INTERFACE,
    &PH_WELL_KNOWN_GUID_PNP_EXTENDED_ADDRESS_INTERFACE,
    &PH_WELL_KNOWN_GUID_D3COLD_AUX_POWER_AND_TIMING_INTERFACE,
    &PH_WELL_KNOWN_GUID_PCI_FPGA_CONTROL_INTERFACE,
    &PH_WELL_KNOWN_GUID_PCI_PTM_CONTROL_INTERFACE,
    &PH_WELL_KNOWN_GUID_BUS_RESOURCE_UPDATE_INTERFACE,
    &PH_WELL_KNOWN_GUID_NPEM_CONTROL_INTERFACE,
    &PH_WELL_KNOWN_GUID_PCI_ATS_INTERFACE,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_INTERNAL,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_PCMCIA,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_PCI,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_ISAPNP,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_EISA,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_MCA,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_SERENUM,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_USB,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_LPTENUM,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_USBPRINT,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_DOT4PRT,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_1394,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_HID,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_AVC,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_IRDA,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_SD,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_ACPI,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_SW_DEVICE,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_SCM,
    &PH_WELL_KNOWN_GUID_POWER_DEVICE_ENABLE,
    &PH_WELL_KNOWN_GUID_POWER_DEVICE_TIMEOUTS,
    &PH_WELL_KNOWN_GUID_POWER_DEVICE_WAKE_ENABLE,
    &PH_WELL_KNOWN_GUID_WUDF_DEVICE_HOST_PROBLEM,
    &PH_WELL_KNOWN_GUID_PARTITION_UNIT_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_QUERY_CRASHDUMP_FUNCTIONS,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_DISK,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_CDROM,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_PARTITION,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_TAPE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_WRITEONCEDISK,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_VOLUME,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_MEDIUMCHANGER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_FLOPPY,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_CDCHANGER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_STORAGEPORT,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_VMLUN,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_SES,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_ZNSDISK,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_SERVICE_VOLUME,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_HIDDEN_VOLUME,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_UNIFIED_ACCESS_RPMB,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_SCM_PHYSICAL_DEVICE,
    &PH_WELL_KNOWN_GUID_SCM_PD_HEALTH_NOTIFICATION,
    &PH_WELL_KNOWN_GUID_SCM_PD_PASSTHROUGH_INVDIMM,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_COMPORT,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR,
    &PH_WELL_KNOWN_GUID_BTHPORT_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_BTH_RFCOMM_SERVICE_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_1394,
    &PH_WELL_KNOWN_GUID_DEVCLASS_1394DEBUG,
    &PH_WELL_KNOWN_GUID_DEVCLASS_61883,
    &PH_WELL_KNOWN_GUID_DEVCLASS_ADAPTER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_APMSUPPORT,
    &PH_WELL_KNOWN_GUID_DEVCLASS_AVC,
    &PH_WELL_KNOWN_GUID_DEVCLASS_BATTERY,
    &PH_WELL_KNOWN_GUID_DEVCLASS_BIOMETRIC,
    &PH_WELL_KNOWN_GUID_DEVCLASS_BLUETOOTH,
    &PH_WELL_KNOWN_GUID_DEVCLASS_CAMERA,
    &PH_WELL_KNOWN_GUID_DEVCLASS_CDROM,
    &PH_WELL_KNOWN_GUID_DEVCLASS_COMPUTEACCELERATOR,
    &PH_WELL_KNOWN_GUID_DEVCLASS_COMPUTER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_DECODER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_DISKDRIVE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_DISPLAY,
    &PH_WELL_KNOWN_GUID_DEVCLASS_DOT4,
    &PH_WELL_KNOWN_GUID_DEVCLASS_DOT4PRINT,
    &PH_WELL_KNOWN_GUID_DEVCLASS_EHSTORAGESILO,
    &PH_WELL_KNOWN_GUID_DEVCLASS_ENUM1394,
    &PH_WELL_KNOWN_GUID_DEVCLASS_EXTENSION,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FDC,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FIRMWARE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FLOPPYDISK,
    &PH_WELL_KNOWN_GUID_DEVCLASS_GENERIC,
    &PH_WELL_KNOWN_GUID_DEVCLASS_GPS,
    &PH_WELL_KNOWN_GUID_DEVCLASS_HDC,
    &PH_WELL_KNOWN_GUID_DEVCLASS_HIDCLASS,
    &PH_WELL_KNOWN_GUID_DEVCLASS_HOLOGRAPHIC,
    &PH_WELL_KNOWN_GUID_DEVCLASS_IMAGE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_INFINIBAND,
    &PH_WELL_KNOWN_GUID_DEVCLASS_INFRARED,
    &PH_WELL_KNOWN_GUID_DEVCLASS_KEYBOARD,
    &PH_WELL_KNOWN_GUID_DEVCLASS_LEGACYDRIVER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MEDIA,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MEDIUM_CHANGER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MEMORY,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MODEM,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MONITOR,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MOUSE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MTD,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MULTIFUNCTION,
    &PH_WELL_KNOWN_GUID_DEVCLASS_MULTIPORTSERIAL,
    &PH_WELL_KNOWN_GUID_DEVCLASS_NET,
    &PH_WELL_KNOWN_GUID_DEVCLASS_NETCLIENT,
    &PH_WELL_KNOWN_GUID_DEVCLASS_NETDRIVER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_NETSERVICE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_NETTRANS,
    &PH_WELL_KNOWN_GUID_DEVCLASS_NETUIO,
    &PH_WELL_KNOWN_GUID_DEVCLASS_NODRIVER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PCMCIA,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PNPPRINTERS,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PORTS,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PRIMITIVE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PRINTER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PRINTERUPGRADE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PRINTQUEUE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_PROCESSOR,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SBP2,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SCMDISK,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SCMVOLUME,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SCSIADAPTER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SECURITYACCELERATOR,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SENSOR,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SIDESHOW,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SMARTCARDREADER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SMRDISK,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SMRVOLUME,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SOFTWARECOMPONENT,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SOUND,
    &PH_WELL_KNOWN_GUID_DEVCLASS_SYSTEM,
    &PH_WELL_KNOWN_GUID_DEVCLASS_TAPEDRIVE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_UNKNOWN,
    &PH_WELL_KNOWN_GUID_DEVCLASS_UCM,
    &PH_WELL_KNOWN_GUID_DEVCLASS_USB,
    &PH_WELL_KNOWN_GUID_DEVCLASS_VOLUME,
    &PH_WELL_KNOWN_GUID_DEVCLASS_VOLUMESNAPSHOT,
    &PH_WELL_KNOWN_GUID_DEVCLASS_WCEUSBS,
    &PH_WELL_KNOWN_GUID_DEVCLASS_WPD,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_TOP,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_ACTIVITYMONITOR,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_UNDELETE,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_ANTIVIRUS,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_REPLICATION,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_CONTINUOUSBACKUP,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_CONTENTSCREENER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_QUOTAMANAGEMENT,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_SYSTEMRECOVERY,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_CFSMETADATASERVER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_HSM,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_COMPRESSION,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_ENCRYPTION,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_VIRTUALIZATION,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_PHYSICALQUOTAMANAGEMENT,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_OPENFILEBACKUP,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_SECURITYENHANCER,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_COPYPROTECTION,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_BOTTOM,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_SYSTEM,
    &PH_WELL_KNOWN_GUID_DEVCLASS_FSFILTER_INFRASTRUCTURE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_USB_HUB,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_USB_BILLBOARD,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_USB_DEVICE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_USB_HOST_CONTROLLER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_A2DP_SIDEBAND_AUDIO,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_CHARGING_ARBITRATION,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_CONFIGURABLE_USBFN_CHARGER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_CONFIGURABLE_WIRELESS_CHARGER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_I2C,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_OPM,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_OPM_2_JTP,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_OPM_2,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_OPM_3,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_BRIGHTNESS,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_BRIGHTNESS_2,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_MIRACAST_DISPLAY,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_BRIGHTNESS_3,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_HPMI,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_VIRTUALIZABLE_DEVICE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_GPP,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HS_HCIBYPASS,
    &PH_WELL_KNOWN_GUID_BUS_VPCI,
    &PH_WELL_KNOWN_GUID_VPCI_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_VPCI,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_WWAN_CONTROLLER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_WDDM3_ON_VB,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_GRAPHICSPOWER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_GNSS,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_HID,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_LAMP,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_NET,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_NETUIO,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_NFCDTA,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_NFCSE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_NFP,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_KEYBOARD,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_MODEM,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_MOUSE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_PARALLEL,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_PARCLASS,
    &PH_WELL_KNOWN_GUID_PARALLEL_DEVICE,
    &PH_WELL_KNOWN_GUID_PARCLASS_DEVICE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_DISPLAY_ADAPTER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_MONITOR,
    &PH_WELL_KNOWN_GUID_DEVICE_BATTERY,
    &PH_WELL_KNOWN_GUID_DEVICE_APPLICATIONLAUNCH_BUTTON,
    &PH_WELL_KNOWN_GUID_DEVICE_SYS_BUTTON,
    &PH_WELL_KNOWN_GUID_DEVICE_LID,
    &PH_WELL_KNOWN_GUID_DEVICE_THERMAL_ZONE,
    &PH_WELL_KNOWN_GUID_DEVICE_FAN,
    &PH_WELL_KNOWN_GUID_DEVICE_PROCESSOR,
    &PH_WELL_KNOWN_GUID_DEVICE_MEMORY,
    &PH_WELL_KNOWN_GUID_DEVICE_ACPI_TIME,
    &PH_WELL_KNOWN_GUID_DEVICE_MESSAGE_INDICATOR,
    &PH_WELL_KNOWN_GUID_CLASS_INPUT,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_THERMAL_COOLING,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_THERMAL_MANAGER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_PWM_CONTROLLER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_USBPRINT,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_IPPUSB_PRINT,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_VM_GENCOUNTER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_BIOMETRIC_READER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_SMARTCARD_READER,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_DMR,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_DMP,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_DMS,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_ENHANCED_STORAGE_SILO,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_WPD,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_WPD_PRIVATE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_WPD_SERVICE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_SENSOR,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_IMAGE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_SIDESHOW,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_WIFIDIRECT_DEVICE,
    &PH_WELL_KNOWN_GUID_AEPSERVICE_WIFIDIRECT_DEVICE,
    &PH_WELL_KNOWN_GUID_DEVINTERFACE_ASP_INFRA_DEVICE,
    &PH_WELL_KNOWN_GUID_DXGKDDI_MITIGABLE_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_DXGKDDI_FLEXIOV_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_SRIOV_DEVICE_INTERFACE_STANDARD,
    &PH_WELL_KNOWN_GUID_MITIGABLE_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_IO_VOLUME_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_BLUETOOTHLE_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE,
};

static int __cdecl PhpWellKnownGuidSortFunction(
    const void* Left,
    const void* Right
    )
{
    PPH_WELL_KNOWN_GUID lhsItem;
    PPH_WELL_KNOWN_GUID rhsItem;

    lhsItem = *(PPH_WELL_KNOWN_GUID*)Left;
    rhsItem = *(PPH_WELL_KNOWN_GUID*)Right;

    return memcmp(lhsItem->Guid, rhsItem->Guid, sizeof(GUID));
}

static int __cdecl PhpWellKnownGuidSearchFunction(
    const GUID* Guid,
    const void* Item 
    )
{
    PPH_WELL_KNOWN_GUID item;

    item = *(PPH_WELL_KNOWN_GUID*)Item;

    return memcmp(Guid, item->Guid, sizeof(GUID));
}

PPH_STRING PhpDevPropWellKnownGuidToString(
    _In_ PGUID Guid 
    )
{
    const PH_WELL_KNOWN_GUID** entry;

    if (PhBeginInitOnce(&PhpWellKnownGuidsInitOnce))
    {
        qsort(
            (void*)PhpWellKnownGuids,
            RTL_NUMBER_OF(PhpWellKnownGuids),
            sizeof(PPH_WELL_KNOWN_GUID),
            PhpWellKnownGuidSortFunction
            );

#ifdef DEBUG
        // check for collisions 
        for (ULONG i = 0; i < RTL_NUMBER_OF(PhpWellKnownGuids) - 1; i++)
        {
            const PH_WELL_KNOWN_GUID* lhs = PhpWellKnownGuids[i];
            const PH_WELL_KNOWN_GUID* rhs = PhpWellKnownGuids[i + 1];
            assert(!IsEqualGUID(&lhs->Guid, &rhs->Guid));
        }
#endif

        PhEndInitOnce(&PhpWellKnownGuidsInitOnce);
    }

    entry = bsearch(
        Guid,
        PhpWellKnownGuids,
        RTL_NUMBER_OF(PhpWellKnownGuids),
        sizeof(PPH_WELL_KNOWN_GUID),
        PhpWellKnownGuidSearchFunction
        );
    if (!entry)
        return NULL;

    return PhCreateString2((PPH_STRINGREF)&(*entry)->Symbol);
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeGUID;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyGuid(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->Guid
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyGuid(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->Guid
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyGuid(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->Guid
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyGuid(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->Guid
                );
        }
    }

    if (Property->Valid)
    {
        Property->AsString = PhpDevPropWellKnownGuidToString(&Property->Guid);

        if (!Property->AsString)
            Property->AsString = PhFormatGuid(&Property->Guid);
    }
}

PPH_STRING PhpDevPropPciDeviceInterruptSupportToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_LineBased))
        PhAppendStringBuilder2(&stringBuilder, L"Line based, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_Msi))
        PhAppendStringBuilder2(&stringBuilder, L"Msi, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_MsiX))
        PhAppendStringBuilder2(&stringBuilder, L"MsiX, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillPciDeviceInterruptSupport(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillUInt32Common(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        Property->AsString = PhpDevPropPciDeviceInterruptSupportToString(Property->UInt32);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillBoolean(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeBoolean;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyBoolean(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->Boolean
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyBoolean(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->Boolean
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyBoolean(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->Boolean
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyBoolean(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->Boolean
                );
        }
    }

    if (Property->Valid)
    {
        if (Property->Boolean)
            Property->AsString = PhCreateString(L"true");
        else
            Property->AsString = PhCreateString(L"false");
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillTimeStamp(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeTimeStamp;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyTimeStamp(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->TimeStamp
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyTimeStamp(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->TimeStamp
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyTimeStamp(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->TimeStamp
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyTimeStamp(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->TimeStamp
                );
        }
    }

    if (Property->Valid)
    {
        SYSTEMTIME systemTime;

        PhLargeIntegerToLocalSystemTime(&systemTime, &Property->TimeStamp);

        Property->AsString = PhFormatDateTime(&systemTime);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeStringList;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyStringList(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->StringList
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyStringList(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->StringList
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyStringList(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->StringList
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyStringList(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->StringList
                );
        }
    }

    if (Property->Valid && Property->StringList->Count > 0)
    {
        PH_STRING_BUILDER stringBuilder;
        PPH_STRING string;

        PhInitializeStringBuilder(&stringBuilder, Property->StringList->Count);

        for (ULONG i = 0; i < Property->StringList->Count - 1; i++)
        {
            string = Property->StringList->Items[i];

            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        string = Property->StringList->Items[Property->StringList->Count - 1];

        PhAppendStringBuilder(&stringBuilder, &string->sr);

        Property->AsString = PhFinalStringBuilderString(&stringBuilder);
        PhReferenceObject(Property->AsString);

        PhDeleteStringBuilder(&stringBuilder);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillStringOrStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillString(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );
    if (!Property->Valid)
    {
        PhpDevPropFillStringList(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            Property,
            Flags
            );
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillBinary(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeBinary;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyBinary(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->Binary.Buffer,
                &Property->Binary.Size
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyBinary(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->Binary.Buffer,
                &Property->Binary.Size
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyBinary(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->Binary.Buffer,
                &Property->Binary.Size
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyBinary(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->Binary.Buffer,
                &Property->Binary.Size
                );
        }
    }

    if (Property->Valid)
    {
        Property->AsString = PhBufferToHexString(Property->Binary.Buffer, Property->Binary.Size);
    }
}

static const PH_DEVICE_PROPERTY_TABLE_ENTRY PhpDeviceItemPropertyTable[] =
{
    { PhDevicePropertyName, &DEVPKEY_NAME, PhpDevPropFillString, 0 },
    { PhDevicePropertyManufacturer, &DEVPKEY_Device_Manufacturer, PhpDevPropFillString, 0 },
    { PhDevicePropertyService, &DEVPKEY_Device_Service, PhpDevPropFillString, 0 },
    { PhDevicePropertyClass, &DEVPKEY_Device_Class, PhpDevPropFillString, 0 },
    { PhDevicePropertyEnumeratorName, &DEVPKEY_Device_EnumeratorName, PhpDevPropFillString, 0 },
    { PhDevicePropertyInstallDate, &DEVPKEY_Device_InstallDate, PhpDevPropFillTimeStamp, 0 },

    { PhDevicePropertyFirstInstallDate, &DEVPKEY_Device_FirstInstallDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyLastArrivalDate, &DEVPKEY_Device_LastArrivalDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyLastRemovalDate, &DEVPKEY_Device_LastRemovalDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyDeviceDesc, &DEVPKEY_Device_DeviceDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyFriendlyName, &DEVPKEY_Device_FriendlyName, PhpDevPropFillString, 0 },
    { PhDevicePropertyInstanceId, &DEVPKEY_Device_InstanceId, PhpDevPropFillString, 0 },
    { PhDevicePropertyParentInstanceId, &DEVPKEY_Device_Parent, PhpDevPropFillString, 0 },
    { PhDevicePropertyPDOName, &DEVPKEY_Device_PDOName, PhpDevPropFillString, 0 },
    { PhDevicePropertyLocationInfo, &DEVPKEY_Device_LocationInfo, PhpDevPropFillString, 0 },
    { PhDevicePropertyClassGuid, &DEVPKEY_Device_ClassGuid, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyDriver, &DEVPKEY_Device_Driver, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverVersion, &DEVPKEY_Device_DriverVersion, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverDate, &DEVPKEY_Device_DriverDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyFirmwareDate, &DEVPKEY_Device_FirmwareDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyFirmwareVersion, &DEVPKEY_Device_FirmwareVersion, PhpDevPropFillString, 0 },
    { PhDevicePropertyFirmwareRevision, &DEVPKEY_Device_FirmwareRevision, PhpDevPropFillString, 0 },
    { PhDevicePropertyHasProblem, &DEVPKEY_Device_HasProblem, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyProblemCode, &DEVPKEY_Device_ProblemCode, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyProblemStatus, &DEVPKEY_Device_ProblemStatus, PhpDevPropFillNTSTATUS, 0 },
    { PhDevicePropertyDevNodeStatus, &DEVPKEY_Device_DevNodeStatus, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyDevCapabilities, &DEVPKEY_Device_Capabilities, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyUpperFilters, &DEVPKEY_Device_UpperFilters, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyLowerFilters, &DEVPKEY_Device_LowerFilters, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyHardwareIds, &DEVPKEY_Device_HardwareIds, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyCompatibleIds, &DEVPKEY_Device_CompatibleIds, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyConfigFlags, &DEVPKEY_Device_ConfigFlags, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyUINumber, &DEVPKEY_Device_UINumber, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyBusTypeGuid, &DEVPKEY_Device_BusTypeGuid, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyLegacyBusType, &DEVPKEY_Device_LegacyBusType, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyBusNumber, &DEVPKEY_Device_BusNumber, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertySecurity, &DEVPKEY_Device_Security, PhpDevPropFillBinary, 0 }, // DEVPROP_TYPE_SECURITY_DESCRIPTOR as binary, PhDevicePropertySecuritySDS for string
    { PhDevicePropertySecuritySDS, &DEVPKEY_Device_SecuritySDS, PhpDevPropFillString, 0 },
    { PhDevicePropertyDevType, &DEVPKEY_Device_DevType, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyExclusive, &DEVPKEY_Device_Exclusive, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyCharacteristics, &DEVPKEY_Device_Characteristics, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyAddress, &DEVPKEY_Device_Address, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyPowerData, &DEVPKEY_Device_PowerData, PhpDevPropFillBinary, 0 }, // TODO(jxy-s) CM_POWER_DATA could be formatted AsString nicer
    { PhDevicePropertyRemovalPolicy, &DEVPKEY_Device_RemovalPolicy, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyRemovalPolicyDefault, &DEVPKEY_Device_RemovalPolicyDefault, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyRemovalPolicyOverride, &DEVPKEY_Device_RemovalPolicyOverride, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyInstallState, &DEVPKEY_Device_InstallState, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyLocationPaths, &DEVPKEY_Device_LocationPaths, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyBaseContainerId, &DEVPKEY_Device_BaseContainerId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyEjectionRelations, &DEVPKEY_Device_EjectionRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyRemovalRelations, &DEVPKEY_Device_RemovalRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyPowerRelations, &DEVPKEY_Device_PowerRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyBusRelations, &DEVPKEY_Device_BusRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyChildren, &DEVPKEY_Device_Children, PhpDevPropFillStringList, 0 },
    { PhDevicePropertySiblings, &DEVPKEY_Device_Siblings, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyTransportRelations, &DEVPKEY_Device_TransportRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyReported, &DEVPKEY_Device_Reported, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyLegacy, &DEVPKEY_Device_Legacy, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerId, &DEVPKEY_Device_ContainerId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyInLocalMachineContainer, &DEVPKEY_Device_InLocalMachineContainer, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyModel, &DEVPKEY_Device_Model, PhpDevPropFillString, 0 },
    { PhDevicePropertyModelId, &DEVPKEY_Device_ModelId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyFriendlyNameAttributes, &DEVPKEY_Device_FriendlyNameAttributes, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyManufacturerAttributes, &DEVPKEY_Device_ManufacturerAttributes, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyPresenceNotForDevice, &DEVPKEY_Device_PresenceNotForDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertySignalStrength, &DEVPKEY_Device_SignalStrength, PhpDevPropFillInt32, 0 },
    { PhDevicePropertyIsAssociateableByUserAction, &DEVPKEY_Device_IsAssociateableByUserAction, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyShowInUninstallUI, &DEVPKEY_Device_ShowInUninstallUI, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyNumaProximityDomain, &DEVPKEY_Device_Numa_Proximity_Domain, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyDHPRebalancePolicy, &DEVPKEY_Device_DHP_Rebalance_Policy, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyNumaNode, &DEVPKEY_Device_Numa_Node, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyBusReportedDeviceDesc, &DEVPKEY_Device_BusReportedDeviceDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyIsPresent, &DEVPKEY_Device_IsPresent, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyConfigurationId, &DEVPKEY_Device_ConfigurationId, PhpDevPropFillString, 0 },
    { PhDevicePropertyReportedDeviceIdsHash, &DEVPKEY_Device_ReportedDeviceIdsHash, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPhysicalDeviceLocation, &DEVPKEY_Device_PhysicalDeviceLocation, PhpDevPropFillBinary, 0 }, // TODO(jxy-s) look into ACPI 4.0a Specification, section 6.1.6 for AsString formatting 
    { PhDevicePropertyBiosDeviceName, &DEVPKEY_Device_BiosDeviceName, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverProblemDesc, &DEVPKEY_Device_DriverProblemDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyDebuggerSafe, &DEVPKEY_Device_DebuggerSafe, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPostInstallInProgress, &DEVPKEY_Device_PostInstallInProgress, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStack, &DEVPKEY_Device_Stack, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyExtendedConfigurationIds, &DEVPKEY_Device_ExtendedConfigurationIds, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyIsRebootRequired, &DEVPKEY_Device_IsRebootRequired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyDependencyProviders, &DEVPKEY_Device_DependencyProviders, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyDependencyDependents, &DEVPKEY_Device_DependencyDependents, PhpDevPropFillStringList, 0 },
    { PhDevicePropertySoftRestartSupported, &DEVPKEY_Device_SoftRestartSupported, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyExtendedAddress, &DEVPKEY_Device_ExtendedAddress, PhpDevPropFillUInt64Hex, 0 },
    { PhDevicePropertyAssignedToGuest, &DEVPKEY_Device_AssignedToGuest, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyCreatorProcessId, &DEVPKEY_Device_CreatorProcessId, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyFirmwareVendor, &DEVPKEY_Device_FirmwareVendor, PhpDevPropFillString, 0 },
    { PhDevicePropertySessionId, &DEVPKEY_Device_SessionId, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyDriverDesc, &DEVPKEY_Device_DriverDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverInfPath, &DEVPKEY_Device_DriverInfPath, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverInfSection, &DEVPKEY_Device_DriverInfSection, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverInfSectionExt, &DEVPKEY_Device_DriverInfSectionExt, PhpDevPropFillString, 0 },
    { PhDevicePropertyMatchingDeviceId, &DEVPKEY_Device_MatchingDeviceId, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverProvider, &DEVPKEY_Device_DriverProvider, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverPropPageProvider, &DEVPKEY_Device_DriverPropPageProvider, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverCoInstallers, &DEVPKEY_Device_DriverCoInstallers, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyResourcePickerTags, &DEVPKEY_Device_ResourcePickerTags, PhpDevPropFillString, 0 },
    { PhDevicePropertyResourcePickerExceptions, &DEVPKEY_Device_ResourcePickerExceptions, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverRank, &DEVPKEY_Device_DriverRank, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyDriverLogoLevel, &DEVPKEY_Device_DriverLogoLevel, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyNoConnectSound, &DEVPKEY_Device_NoConnectSound, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyGenericDriverInstalled, &DEVPKEY_Device_GenericDriverInstalled, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyAdditionalSoftwareRequested, &DEVPKEY_Device_AdditionalSoftwareRequested, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertySafeRemovalRequired, &DEVPKEY_Device_SafeRemovalRequired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertySafeRemovalRequiredOverride, &DEVPKEY_Device_SafeRemovalRequiredOverride, PhpDevPropFillBoolean, 0 },

    { PhDevicePropertyPkgModel, &DEVPKEY_DrvPkg_Model, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgVendorWebSite, &DEVPKEY_DrvPkg_VendorWebSite, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgDetailedDescription, &DEVPKEY_DrvPkg_DetailedDescription, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgDocumentationLink, &DEVPKEY_DrvPkg_DocumentationLink, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgIcon, &DEVPKEY_DrvPkg_Icon, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyPkgBrandingIcon, &DEVPKEY_DrvPkg_BrandingIcon, PhpDevPropFillStringList, 0 },

    { PhDevicePropertyClassUpperFilters, &DEVPKEY_DeviceClass_UpperFilters, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassLowerFilters, &DEVPKEY_DeviceClass_LowerFilters, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassSecurity, &DEVPKEY_DeviceClass_Security, PhpDevPropFillBinary, DEVPROP_FILL_FLAG_CLASS }, // DEVPROP_TYPE_SECURITY_DESCRIPTOR as binary, PhDevicePropertyClassSecuritySDS for string
    { PhDevicePropertyClassSecuritySDS, &DEVPKEY_DeviceClass_SecuritySDS, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassDevType, &DEVPKEY_DeviceClass_DevType, PhpDevPropFillUInt32, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassExclusive, &DEVPKEY_DeviceClass_Exclusive, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassCharacteristics, &DEVPKEY_DeviceClass_Characteristics, PhpDevPropFillUInt32Hex, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassName, &DEVPKEY_DeviceClass_Name, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassClassName, &DEVPKEY_DeviceClass_ClassName, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassIcon, &DEVPKEY_DeviceClass_Icon, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassClassInstaller, &DEVPKEY_DeviceClass_ClassInstaller, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassPropPageProvider, &DEVPKEY_DeviceClass_PropPageProvider, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassNoInstallClass, &DEVPKEY_DeviceClass_NoInstallClass, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassNoDisplayClass, &DEVPKEY_DeviceClass_NoDisplayClass, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassSilentInstall, &DEVPKEY_DeviceClass_SilentInstall, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassNoUseClass, &DEVPKEY_DeviceClass_NoUseClass, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassDefaultService, &DEVPKEY_DeviceClass_DefaultService, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassIconPath, &DEVPKEY_DeviceClass_IconPath, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassDHPRebalanceOptOut, &DEVPKEY_DeviceClass_DHPRebalanceOptOut, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyClassClassCoInstallers, &DEVPKEY_DeviceClass_ClassCoInstallers, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS },

    { PhDevicePropertyInterfaceFriendlyName, &DEVPKEY_DeviceInterface_FriendlyName, PhpDevPropFillString, 0 },
    { PhDevicePropertyInterfaceEnabled, &DEVPKEY_DeviceInterface_Enabled, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyInterfaceClassGuid, &DEVPKEY_DeviceInterface_ClassGuid, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyInterfaceReferenceString, &DEVPKEY_DeviceInterface_ReferenceString, PhpDevPropFillString, 0 },
    { PhDevicePropertyInterfaceRestricted, &DEVPKEY_DeviceInterface_Restricted, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyInterfaceUnrestrictedAppCapabilities, &DEVPKEY_DeviceInterface_UnrestrictedAppCapabilities, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyInterfaceSchematicName, &DEVPKEY_DeviceInterface_SchematicName, PhpDevPropFillString, 0 },

    { PhDevicePropertyInterfaceClassDefaultInterface, &DEVPKEY_DeviceInterfaceClass_DefaultInterface, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },
    { PhDevicePropertyInterfaceClassName, &DEVPKEY_DeviceInterfaceClass_Name, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS },

    { PhDevicePropertyContainerAddress, &DEVPKEY_DeviceContainer_Address, PhpDevPropFillStringOrStringList, 0 },
    { PhDevicePropertyContainerDiscoveryMethod, &DEVPKEY_DeviceContainer_DiscoveryMethod, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerIsEncrypted, &DEVPKEY_DeviceContainer_IsEncrypted, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsAuthenticated, &DEVPKEY_DeviceContainer_IsAuthenticated, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsConnected, &DEVPKEY_DeviceContainer_IsConnected, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsPaired, &DEVPKEY_DeviceContainer_IsPaired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIcon, &DEVPKEY_DeviceContainer_Icon, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerVersion, &DEVPKEY_DeviceContainer_Version, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerLastSeen, &DEVPKEY_DeviceContainer_Last_Seen, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyContainerLastConnected, &DEVPKEY_DeviceContainer_Last_Connected, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyContainerIsShowInDisconnectedState, &DEVPKEY_DeviceContainer_IsShowInDisconnectedState, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsLocalMachine, &DEVPKEY_DeviceContainer_IsLocalMachine, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerMetadataPath, &DEVPKEY_DeviceContainer_MetadataPath, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerIsMetadataSearchInProgress, &DEVPKEY_DeviceContainer_IsMetadataSearchInProgress, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsMetadataChecksum, &DEVPKEY_DeviceContainer_MetadataChecksum, PhpDevPropFillBinary, 0 },
    { PhDevicePropertyContainerIsNotInterestingForDisplay, &DEVPKEY_DeviceContainer_IsNotInterestingForDisplay, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerLaunchDeviceStageOnDeviceConnect, &DEVPKEY_DeviceContainer_LaunchDeviceStageOnDeviceConnect, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerLaunchDeviceStageFromExplorer, &DEVPKEY_DeviceContainer_LaunchDeviceStageFromExplorer, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerBaselineExperienceId, &DEVPKEY_DeviceContainer_BaselineExperienceId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyContainerIsDeviceUniquelyIdentifiable, &DEVPKEY_DeviceContainer_IsDeviceUniquelyIdentifiable, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerAssociationArray, &DEVPKEY_DeviceContainer_AssociationArray, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerDeviceDescription1, &DEVPKEY_DeviceContainer_DeviceDescription1, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerDeviceDescription2, &DEVPKEY_DeviceContainer_DeviceDescription2, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerHasProblem, &DEVPKEY_DeviceContainer_HasProblem, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsSharedDevice, &DEVPKEY_DeviceContainer_IsSharedDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsNetworkDevice, &DEVPKEY_DeviceContainer_IsNetworkDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsDefaultDevice, &DEVPKEY_DeviceContainer_IsDefaultDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerMetadataCabinet, &DEVPKEY_DeviceContainer_MetadataCabinet, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerRequiresPairingElevation, &DEVPKEY_DeviceContainer_RequiresPairingElevation, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerExperienceId, &DEVPKEY_DeviceContainer_ExperienceId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyContainerCategory, &DEVPKEY_DeviceContainer_Category, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryDescSingular, &DEVPKEY_DeviceContainer_Category_Desc_Singular, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryDescPlural, &DEVPKEY_DeviceContainer_Category_Desc_Plural, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryIcon, &DEVPKEY_DeviceContainer_Category_Icon, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerCategoryGroupDesc, &DEVPKEY_DeviceContainer_CategoryGroup_Desc, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryGroupIcon, &DEVPKEY_DeviceContainer_CategoryGroup_Icon, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerPrimaryCategory, &DEVPKEY_DeviceContainer_PrimaryCategory, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerUnpairUninstall, &DEVPKEY_DeviceContainer_UnpairUninstall, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerRequiresUninstallElevation, &DEVPKEY_DeviceContainer_RequiresUninstallElevation, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerDeviceFunctionSubRank, &DEVPKEY_DeviceContainer_DeviceFunctionSubRank, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyContainerAlwaysShowDeviceAsConnected, &DEVPKEY_DeviceContainer_AlwaysShowDeviceAsConnected, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerConfigFlags, &DEVPKEY_DeviceContainer_ConfigFlags, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyContainerPrivilegedPackageFamilyNames, &DEVPKEY_DeviceContainer_PrivilegedPackageFamilyNames, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCustomPrivilegedPackageFamilyNames, &DEVPKEY_DeviceContainer_CustomPrivilegedPackageFamilyNames, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerIsRebootRequired, &DEVPKEY_DeviceContainer_IsRebootRequired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerFriendlyName, &DEVPKEY_DeviceContainer_FriendlyName, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerManufacturer, &DEVPKEY_DeviceContainer_Manufacturer, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerModelName, &DEVPKEY_DeviceContainer_ModelName, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerModelNumber, &DEVPKEY_DeviceContainer_ModelNumber, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerInstallInProgress, &DEVPKEY_DeviceContainer_InstallInProgress, PhpDevPropFillBoolean, 0 },

    { PhDevicePropertyObjectType, &DEVPKEY_DevQuery_ObjectType, PhpDevPropFillUInt32, 0 },

    { PhDevicePropertyPciInterruptSupport, &DEVPKEY_PciDevice_InterruptSupport, PhpDevPropFillPciDeviceInterruptSupport, 0 },
    { PhDevicePropertyPciExpressCapabilityControl, &DEVPKEY_PciRootBus_PCIExpressCapabilityControl, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyPciNativeExpressControl, &DEVPKEY_PciRootBus_NativePciExpressControl, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyPciSystemMsiSupport, &DEVPKEY_PciRootBus_SystemMsiSupport, PhpDevPropFillBoolean, 0 },

    { PhDevicePropertyStoragePortable, &DEVPKEY_Storage_Portable, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageRemovableMedia, &DEVPKEY_Storage_Removable_Media, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageSystemCritical, &DEVPKEY_Storage_System_Critical, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageDiskNumber, &DEVPKEY_Storage_Disk_Number, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyStoragePartitionNumber, &DEVPKEY_Storage_Partition_Number, PhpDevPropFillUInt32, 0 },
};
C_ASSERT(RTL_NUMBER_OF(PhpDeviceItemPropertyTable) == PhMaxDeviceProperty);

#ifdef DEBUG
static BOOLEAN PhpBreakOnDeviceProperty = FALSE;
static PH_DEVICE_PROPERTY_CLASS PhpBreakOnDevicePropertyClass = 0;
#endif

PPH_DEVICE_PROPERTY PhGetDeviceProperty(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PH_DEVICE_PROPERTY_CLASS Class
    )
{
    PPH_DEVICE_PROPERTY prop;

    prop = &Item->Properties[Class];
    if (!prop->Initialized)
    {
        const PH_DEVICE_PROPERTY_TABLE_ENTRY* entry;

        entry = &PhpDeviceItemPropertyTable[Class];

#ifdef DEBUG
        if (PhpBreakOnDeviceProperty && (PhpBreakOnDevicePropertyClass == Class))
            __debugbreak();
#endif

        entry->Callback(
            Item->DeviceInfo->Handle,
            &Item->DeviceInfoData,
            entry->PropKey,
            prop,
            entry->CallbackFlags
            );

        prop->Initialized = TRUE;
    }

    return prop;
}

BOOLEAN PhLookupDevicePropertyClass(
    _In_ const DEVPROPKEY* Key,
    _Out_ PPH_DEVICE_PROPERTY_CLASS Class
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(PhpDeviceItemPropertyTable); i++)
    {
        const PH_DEVICE_PROPERTY_TABLE_ENTRY* entry = &PhpDeviceItemPropertyTable[i];

        if (IsEqualDevPropKey(*Key, *entry->PropKey))
        {
            *Class = entry->PropClass;
            return TRUE;
        }
    }

    *Class = PhMaxDeviceProperty;
    return FALSE;
}

HICON PhGetDeviceIcon(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PPH_INTEGER_PAIR IconSize
    )
{
    HICON iconHandle;

    if (Item->DeviceInfoData.Interface)
    {
        // try to give the icon for the parent device 
        if (Item->Parent && !Item->Parent->DeviceInfoData.Interface)
        {
            if (!SetupDiLoadDeviceIcon(
                Item->Parent->DeviceInfo->Handle,
                &Item->Parent->DeviceInfoData.DeviceData,
                IconSize->X,
                IconSize->Y,
                0,
                &iconHandle
                ))
            {
                iconHandle = NULL;
            }
        }
        else
        {
            iconHandle = NULL;
        }
    }
    else
    {
        if (!SetupDiLoadDeviceIcon(
            Item->DeviceInfo->Handle,
            &Item->DeviceInfoData.DeviceData,
            IconSize->X,
            IconSize->Y,
            0,
            &iconHandle
            ))
        {
            iconHandle = NULL;
        }
    }

    return iconHandle;
}

PPH_DEVICE_TREE PhReferenceDeviceTree(
    VOID
    )
{
    PPH_DEVICE_TREE deviceTree;

    PhAcquireFastLockShared(&PhpDeviceTreeLock);
    PhSetReference(&deviceTree, PhpDeviceTree);
    PhReleaseFastLockShared(&PhpDeviceTreeLock);

    return deviceTree;
}

ULONG PhpGenerateInstanceIdHash(
    _In_ PPH_STRINGREF InstanceId
    )
{
    return PhHashStringRefEx(InstanceId, TRUE, PH_STRING_HASH_X65599);
}

static int __cdecl PhpDeviceItemSortFunction(
    const void* Left,
    const void* Right
    )
{
    PPH_DEVICE_ITEM lhsItem;
    PPH_DEVICE_ITEM rhsItem;

    lhsItem = *(PPH_DEVICE_ITEM*)Left;
    rhsItem = *(PPH_DEVICE_ITEM*)Right;

    if (lhsItem->InstanceIdHash < rhsItem->InstanceIdHash)
        return -1;
    else if (lhsItem->InstanceIdHash > rhsItem->InstanceIdHash)
        return 1;
    else
        return 0;
}

static int __cdecl PhpDeviceItemSearchFunction(
    const void* Hash,
    const void* Item 
    )
{
    PPH_DEVICE_ITEM item;

    item = *(PPH_DEVICE_ITEM*)Item;

    if (PtrToUlong(Hash) < item->InstanceIdHash)
        return -1;
    else if (PtrToUlong(Hash) > item->InstanceIdHash)
        return 1;
    else
        return 0;
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhLookupDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_STRINGREF InstanceId
    )
{
    ULONG hash;
    PPH_DEVICE_ITEM* deviceItem;

    hash = PhpGenerateInstanceIdHash(InstanceId);

    deviceItem = bsearch(
        UlongToPtr(hash),
        Tree->DeviceList->Items,
        Tree->DeviceList->Count,
        sizeof(PVOID),
        PhpDeviceItemSearchFunction
        );

    return deviceItem ? *deviceItem : NULL;
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhReferenceDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_STRINGREF InstanceId
    )
{
    PPH_DEVICE_ITEM deviceItem;

    PhSetReference(&deviceItem, PhLookupDeviceItem(Tree, InstanceId));

    return deviceItem;
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhReferenceDeviceItem2(
    _In_ PPH_STRINGREF InstanceId
    )
{
    PPH_DEVICE_TREE deviceTree;
    PPH_DEVICE_ITEM deviceItem;

    deviceTree = PhReferenceDeviceTree();
    PhSetReference(&deviceItem, PhLookupDeviceItem(deviceTree, InstanceId));
    PhDereferenceObject(deviceTree);

    return deviceItem;
}

VOID PhpDeviceInfoDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVINFO info = Object;

    if (info->Handle != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(info->Handle);
        info->Handle = INVALID_HANDLE_VALUE;
    }
}

VOID PhpDeviceItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVICE_ITEM item = Object;

    PhClearReference(&item->Children);
    PhClearReference(&item->Interfaces);

    for (ULONG i = 0; i < ARRAYSIZE(item->Properties); i++)
    {
        PPH_DEVICE_PROPERTY prop;

        prop = &item->Properties[i];

        if (prop->Valid)
        {
            if (prop->Type == PhDevicePropertyTypeString)
            {
                PhDereferenceObject(prop->String);
            }
            else if (prop->Type == PhDevicePropertyTypeStringList)
            {
                for (ULONG j = 0; j < prop->StringList->Count; j++)
                {
                    PhDereferenceObject(prop->StringList->Items[j]);
                }

                PhDereferenceObject(prop->StringList);
            }
            else if (prop->Type == PhDevicePropertyTypeBinary)
            {
                PhFree(prop->Binary.Buffer);
            }
        }

        PhClearReference(&prop->AsString);
    }

    PhClearReference(&item->InstanceId);
    PhClearReference(&item->ParentInstanceId);
}

VOID PhpDeviceTreeDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVICE_TREE tree = Object;

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;

        item = tree->DeviceList->Items[i];

        PhDereferenceObject(item);
    }

    for (ULONG i = 0; i < tree->DeviceInterfaceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;

        item = tree->DeviceInterfaceList->Items[i];

        PhDereferenceObject(item);
    }

    PhDereferenceObject(tree->DeviceList);
    PhDereferenceObject(tree->DeviceInterfaceList);
    PhDereferenceObject(tree->DeviceInfo);
}

VOID PhpDeviceNotifyDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVICE_NOTIFY notify = Object;

    if (notify->Action == PhDeviceNotifyInstanceEnumerated ||
        notify->Action == PhDeviceNotifyInstanceStarted ||
        notify->Action == PhDeviceNotifyInstanceRemoved)
    {
        PhDereferenceObject(notify->DeviceInstance.InstanceId);
    }
}

PPH_DEVICE_ITEM NTAPI PhpAddDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    PPH_DEVICE_ITEM item;

    item = PhCreateObjectZero(sizeof(PH_DEVICE_ITEM), PhDeviceItemType);

    item->DeviceInfo = PhReferenceObject(Tree->DeviceInfo);
    RtlCopyMemory(&item->DeviceInfoData.DeviceData, DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    RtlCopyMemory(&item->ClassGuid, &DeviceInfoData->ClassGuid, sizeof(GUID));

    item->Children = PhCreateList(1);
    item->Interfaces = PhCreateList(1);

    //
    // Only get the minimum here, the rest will be retrieved later if necessary.
    // For convenience later, other frequently referenced items are gotten here too.
    //

    item->InstanceId = PhGetDeviceProperty(item, PhDevicePropertyInstanceId)->AsString;
    if (item->InstanceId)
    {
        item->InstanceIdHash = PhpGenerateInstanceIdHash(&item->InstanceId->sr);

        //
        // If this is the root enumerator override some properties.
        //
        if (PhEqualStringRef(&item->InstanceId->sr, &RootInstanceId, TRUE))
        {
            PPH_DEVICE_PROPERTY prop;

            prop = &item->Properties[PhDevicePropertyName];
            assert(!prop->Initialized);

            prop->AsString = PhGetActiveComputerName();
            prop->Initialized = TRUE;
        }

        PhReferenceObject(item->InstanceId);
    }

    item->ParentInstanceId = PhGetDeviceProperty(item, PhDevicePropertyParentInstanceId)->AsString;
    if (item->ParentInstanceId)
        PhReferenceObject(item->ParentInstanceId);

    PhGetDeviceProperty(item, PhDevicePropertyProblemCode);
    if (item->Properties[PhDevicePropertyProblemCode].Valid)
        item->ProblemCode = item->Properties[PhDevicePropertyProblemCode].UInt32;
    else
        item->ProblemCode = CM_PROB_PHANTOM;

    PhGetDeviceProperty(item, PhDevicePropertyDevNodeStatus);
    if (item->Properties[PhDevicePropertyDevNodeStatus].Valid)
        item->DevNodeStatus = item->Properties[PhDevicePropertyDevNodeStatus].UInt32;
    else
        item->DevNodeStatus = 0;

    PhGetDeviceProperty(item, PhDevicePropertyDevCapabilities);
    if (item->Properties[PhDevicePropertyDevCapabilities].Valid)
        item->Capabilities = item->Properties[PhDevicePropertyDevCapabilities].UInt32;
    else
        item->Capabilities = 0;

    PhGetDeviceProperty(item, PhDevicePropertyUpperFilters);
    PhGetDeviceProperty(item, PhDevicePropertyClassUpperFilters);
    if ((item->Properties[PhDevicePropertyUpperFilters].Valid &&
         (item->Properties[PhDevicePropertyUpperFilters].StringList->Count > 0)) ||
        (item->Properties[PhDevicePropertyClassUpperFilters].Valid &&
         (item->Properties[PhDevicePropertyClassUpperFilters].StringList->Count > 0)))
    {
        item->HasUpperFilters = TRUE;
    }

    PhGetDeviceProperty(item, PhDevicePropertyLowerFilters);
    PhGetDeviceProperty(item, PhDevicePropertyClassLowerFilters);
    if ((item->Properties[PhDevicePropertyLowerFilters].Valid &&
         (item->Properties[PhDevicePropertyLowerFilters].StringList->Count > 0)) ||
        (item->Properties[PhDevicePropertyClassLowerFilters].Valid &&
         (item->Properties[PhDevicePropertyClassLowerFilters].StringList->Count > 0)))
    {
        item->HasLowerFilters = TRUE;
    }

    PhAddItemList(Tree->DeviceList, item);

    return item;
}

PPH_DEVICE_ITEM NTAPI PhpAddDeviceInterfaceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_DEVICE_ITEM DeviceItem,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData
    )
{
    PPH_DEVICE_ITEM item;
    PPH_DEVICE_PROPERTY prop;

    item = PhCreateObjectZero(sizeof(PH_DEVICE_ITEM), PhDeviceItemType);

    item->DeviceInfo = PhReferenceObject(Tree->DeviceInfo);
    item->DeviceInfoData.Interface = TRUE;
    RtlCopyMemory(&item->DeviceInfoData.InterfaceData, DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
    RtlCopyMemory(&item->ClassGuid, &DeviceInterfaceData->InterfaceClassGuid, sizeof(GUID));

    item->Parent = DeviceItem;
    item->InstanceId = PhGetDeviceProperty(item, PhDevicePropertyInstanceId)->AsString;
    if (item->InstanceId)
        PhReferenceObject(item->InstanceId);
    item->ParentInstanceId = PhReferenceObject(DeviceItem->InstanceId);
    item->DeviceInterface = TRUE;

    if (DeviceItem->Interfaces->Count > 0)
    {
        PPH_DEVICE_ITEM sibling = DeviceItem->Interfaces->Items[DeviceItem->Interfaces->Count - 1];
        assert(!sibling->Sibling);
        sibling->Sibling = item;
    }

    // if the interface has no name, override it to be the best possible name
    prop = PhGetDeviceProperty(item, PhDevicePropertyName);
    if (!prop->AsString)
    {
        PPH_STRING string;

        if (string = PhGetDeviceProperty(item, PhDevicePropertyInterfaceFriendlyName)->AsString)
            prop->AsString = PhReferenceObject(string);
        else if (string = PhGetDeviceProperty(item, PhDevicePropertyInterfaceClassName)->AsString)
            prop->AsString = PhReferenceObject(string);
        else if (string = PhGetDeviceProperty(item, PhDevicePropertyInstanceId)->AsString)
            prop->AsString = PhReferenceObject(string);
        else
            prop->AsString = PhFormatGuid(&DeviceInterfaceData->InterfaceClassGuid);
    }

    PhAddItemList(DeviceItem->Interfaces, item);
    PhAddItemList(Tree->DeviceInterfaceList, item);

    return item;
}

VOID PhpGetInterfaceClassList(
    _Out_ PGUID* InterfaceClassList,
    _Out_ PULONG InterfaceClassListCount
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT(WINAPI * DevGetObjects_I)(
        _In_ DEV_OBJECT_TYPE ObjectType,
        _In_ ULONG QueryFlags,
        _In_ ULONG cRequestedProperties,
        _In_reads_opt_(cRequestedProperties) const DEVPROPCOMPKEY *pRequestedProperties,
        _In_ ULONG cFilterExpressionCount,
        _In_reads_opt_(cFilterExpressionCount) const DEVPROP_FILTER_EXPRESSION *pFilter,
        _Out_ PULONG pcObjectCount,
        _Outptr_result_buffer_maybenull_(*pcObjectCount) const DEV_OBJECT **ppObjects) = NULL;
    static VOID(WINAPI * DevFreeObjects_I)(
        _In_ ULONG cObjectCount,
        _In_reads_(cObjectCount) const DEV_OBJECT *pObjects) = NULL;

    const DEV_OBJECT* objects = NULL;
    ULONG objectCount;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID cfgmgr32;

        if (cfgmgr32 = PhLoadLibrary(L"cfgmgr32.dll"))
        {
            DevGetObjects_I = PhGetProcedureAddress(cfgmgr32, "DevGetObjects", 0);
            DevFreeObjects_I = PhGetProcedureAddress(cfgmgr32, "DevFreeObjects", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (FAILED(DevGetObjects_I(
        DevObjectTypeDeviceInterfaceClass,
        DevQueryFlagNone,
        0,
        NULL,
        0,
        NULL,
        &objectCount,
        &objects
        )))
    {
        *InterfaceClassList = NULL;
        *InterfaceClassListCount = 0;
        return;
    }

    *InterfaceClassList = PhAllocateZero(objectCount * sizeof(GUID));
    *InterfaceClassListCount = objectCount;

    for (ULONG i = 0; i < objectCount; i++)
    {
        GUID interfaceClassGuid;
        UNICODE_STRING interfaceClassGuidString;

        RtlInitUnicodeString(&interfaceClassGuidString, objects[i].pszObjectId);

        if (!NT_SUCCESS(RtlGUIDFromString(&interfaceClassGuidString, &interfaceClassGuid)))
            continue;

        RtlCopyMemory(&(*InterfaceClassList)[i], &interfaceClassGuid, sizeof(GUID));
    }

    DevFreeObjects_I(objectCount, objects);
}

VOID PhpAssociateDeviceIntefaces(
    _Inout_ PPH_DEVICE_TREE Tree,
    _In_ PPH_DEVICE_ITEM DeviceItem,
    _In_ PGUID InterfaceClassList,
    _In_ ULONG InterfaceClassListCount
    )
{
    assert(!DeviceItem->DeviceInfoData.Interface);

    for (ULONG i = 0; i < InterfaceClassListCount; i++)
    {
        for (ULONG j = 0;; j++)
        {
            SP_DEVICE_INTERFACE_DATA deviceInterfaceData;

            RtlZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
            deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

            if (!SetupDiEnumDeviceInterfaces(
                Tree->DeviceInfo->Handle,
                &DeviceItem->DeviceInfoData.DeviceData,
                &InterfaceClassList[i],
                j,
                &deviceInterfaceData
                ))
            {
                break;
            }

            PhpAddDeviceInterfaceItem(Tree, DeviceItem, &deviceInterfaceData);
        }
    }
}

PPH_DEVICE_TREE PhpCreateDeviceTree(
    VOID
    )
{
    PPH_DEVICE_TREE tree;
    PPH_DEVICE_ITEM root;
    PGUID interfaceClassList;
    ULONG interfaceClassListCount;

    tree = PhCreateObjectZero(sizeof(PH_DEVICE_TREE), PhDeviceTreeType);

    tree->DeviceList = PhCreateList(40);
    tree->DeviceInterfaceList = PhCreateList(40);
    tree->DeviceInfo = PhCreateObject(sizeof(PH_DEVINFO), PhpDeviceInfoType);

    tree->DeviceInfo->Handle = SetupDiGetClassDevsW(
        NULL,
        NULL,
        NULL,
        DIGCF_ALLCLASSES
        );
    if (tree->DeviceInfo->Handle == INVALID_HANDLE_VALUE)
    {
        return tree;
    }

    for (ULONG i = 0;; i++)
    {
        SP_DEVINFO_DATA deviceInfoData;

        RtlZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(tree->DeviceInfo->Handle, i, &deviceInfoData))
            break;

        PhpAddDeviceItem(tree, &deviceInfoData);
    }

    // now add interfaces to the device info set, the return value will be the
    // same pointer as the existing handle
    (VOID)SetupDiGetClassDevsExW(
        NULL,
        NULL,
        NULL,
        DIGCF_ALLCLASSES | DIGCF_DEVICEINTERFACE,
        tree->DeviceInfo->Handle,
        NULL,
        NULL
        );

    PhpGetInterfaceClassList(&interfaceClassList, &interfaceClassListCount);

    // Link the device relations.
    root = NULL;

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        BOOLEAN found;
        PPH_DEVICE_ITEM item;
        PPH_DEVICE_ITEM other;

        found = FALSE;

        item = tree->DeviceList->Items[i];

        // for this device item associate any device interfaces 
        PhpAssociateDeviceIntefaces(tree, item, interfaceClassList, interfaceClassListCount);

        for (ULONG j = 0; j < tree->DeviceList->Count; j++)
        {
            other = tree->DeviceList->Items[j];

            if (item == other)
            {
                continue;
            }

            if (!other->InstanceId || !item->ParentInstanceId)
            {
                continue;
            }

            if (!PhEqualString(other->InstanceId, item->ParentInstanceId, TRUE))
            {
                continue;
            }

            found = TRUE;

            item->Parent = other;

            if (!other->Child)
            {
                other->Child = item;
                break;
            }

            other = other->Child;
            while (other->Sibling)
            {
                other = other->Sibling;
            }

            other->Sibling = item;
            break;
        }

        if (found)
        {
            continue;
        }

        other = root;
        if (!other)
        {
            root = item;
            continue;
        }

        while (other->Sibling)
        {
            other = other->Sibling;
        }

        other->Sibling = item;
    }

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;
        PPH_DEVICE_ITEM child;

        item = tree->DeviceList->Items[i];

        child = item->Child;
        while (child)
        {
            PhAddItemList(item->Children, child);
            child = child->Sibling;
        }
    }

    assert(root && !root->Sibling);
    tree->Root = root;

    // sort the list for faster lookups later
    qsort(
        tree->DeviceList->Items,
        tree->DeviceList->Count,
        sizeof(PVOID),
        PhpDeviceItemSortFunction
        );

    if (interfaceClassList)
        PhFree(interfaceClassList);

    return tree;
}

VOID PhpDeviceNotify(
    _In_ PLIST_ENTRY List
    )
{
    PPH_DEVICE_TREE newTree;
    PPH_DEVICE_TREE oldTree;

    // We process device notifications in blocks so that bursts of device changes
    // don't each trigger a new tree each time.

    newTree = PhpCreateDeviceTree();
    PhAcquireFastLockExclusive(&PhpDeviceTreeLock);
    oldTree = PhpDeviceTree;
    PhpDeviceTree = newTree;
    PhReleaseFastLockExclusive(&PhpDeviceTreeLock);

    while (!IsListEmpty(List))
    {
        PPH_DEVICE_NOTIFY entry;

        entry = CONTAINING_RECORD(RemoveHeadList(List), PH_DEVICE_NOTIFY, ListEntry);

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent), entry);

        PhDereferenceObject(entry);
    }

    PhDereferenceObject(oldTree);
}

_Function_class_(PUSER_THREAD_START_ROUTINE)
NTSTATUS NTAPI PhpDeviceNotifyWorker(
    _In_ PVOID ThreadParameter
    )
{
    PhSetThreadName(NtCurrentThread(), L"DeviceNotifyWorker");

    for (;;)
    {
        LIST_ENTRY list;

        // delay to process events in blocks
        PhDelayExecution(1000);

        PhAcquireFastLockExclusive(&PhpDeviceNotifyLock);

        if (IsListEmpty(&PhpDeviceNotifyList))
        {
            NtResetEvent(PhpDeviceNotifyEvent, NULL);
            PhReleaseFastLockExclusive(&PhpDeviceNotifyLock);
            NtWaitForSingleObject(PhpDeviceNotifyEvent, FALSE, NULL);
            continue;
        }

        // drain the list
        list = PhpDeviceNotifyList;
        list.Flink->Blink = &list;
        list.Blink->Flink = &list;
        InitializeListHead(&PhpDeviceNotifyList);

        PhReleaseFastLockExclusive(&PhpDeviceNotifyLock);

        PhpDeviceNotify(&list);
    }

    return STATUS_SUCCESS;
}

_Function_class_(PCM_NOTIFY_CALLBACK)
ULONG CALLBACK PhpCmNotifyCallback(
    _In_ HCMNOTIFICATION hNotify,
    _In_opt_ PVOID Context,
    _In_ CM_NOTIFY_ACTION Action,
    _In_reads_bytes_(EventDataSize) PCM_NOTIFY_EVENT_DATA EventData,
    _In_ ULONG EventDataSize
    )
{
    PPH_DEVICE_NOTIFY entry = PhCreateObjectZero(sizeof(PH_DEVICE_NOTIFY), PhDeviceNotifyType);

    switch (Action)
    {
    case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
        {
            entry->Action = PhDeviceNotifyInterfaceArrival;
            RtlCopyMemory(&entry->DeviceInterface.ClassGuid, &EventData->u.DeviceInterface.ClassGuid, sizeof(GUID));
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
        {
            entry->Action = PhDeviceNotifyInterfaceRemoval;
            RtlCopyMemory(&entry->DeviceInterface.ClassGuid, &EventData->u.DeviceInterface.ClassGuid, sizeof(GUID));
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED:
        {
            entry->Action = PhDeviceNotifyInstanceEnumerated;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED:
        {
            entry->Action = PhDeviceNotifyInstanceStarted;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED:
        {
            entry->Action = PhDeviceNotifyInstanceRemoved;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);
        }
        break;
    default:
        return ERROR_SUCCESS;
    }

    PhAcquireFastLockExclusive(&PhpDeviceNotifyLock);
    InsertTailList(&PhpDeviceNotifyList, &entry->ListEntry);
    NtSetEvent(PhpDeviceNotifyEvent, NULL);
    PhReleaseFastLockExclusive(&PhpDeviceNotifyLock);

    return ERROR_SUCCESS;
}

BOOLEAN PhDeviceProviderInitialization(
    VOID
    )
{
    NTSTATUS status;
    CM_NOTIFY_FILTER cmFilter;

    if (WindowsVersion < WINDOWS_10 || !PhGetIntegerSetting(L"EnableDeviceSupport"))
        return TRUE;

    PhDeviceItemType = PhCreateObjectType(L"DeviceItem", 0, PhpDeviceItemDeleteProcedure);
    PhDeviceTreeType = PhCreateObjectType(L"DeviceTree", 0, PhpDeviceTreeDeleteProcedure);
    PhDeviceNotifyType = PhCreateObjectType(L"DeviceNotify", 0, PhpDeviceNotifyDeleteProcedure);
    PhpDeviceInfoType = PhCreateObjectType(L"DeviceInfo", 0, PhpDeviceInfoDeleteProcedure);

    PhpDeviceTree = PhpCreateDeviceTree();

    InitializeListHead(&PhpDeviceNotifyList);
    if (!NT_SUCCESS(status = NtCreateEvent(&PhpDeviceNotifyEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE)))
        return FALSE;
    if (!NT_SUCCESS(status = PhCreateThread2(PhpDeviceNotifyWorker, NULL)))
        return FALSE;

    RtlZeroMemory(&cmFilter, sizeof(CM_NOTIFY_FILTER));
    cmFilter.cbSize = sizeof(CM_NOTIFY_FILTER);

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_DEVICE_INSTANCES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        PhpCmNotifyCallback,
        &PhpDeviceNotification
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        PhpCmNotifyCallback,
        &PhpDeviceInterfaceNotification
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    return TRUE;
}
