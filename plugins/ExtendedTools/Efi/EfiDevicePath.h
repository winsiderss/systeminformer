/*++

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

EfiDevicePath.h

Abstract:

EFI Device Path definitions

--*/

#ifndef _EFI_DEVICE_PATH_H
#define _EFI_DEVICE_PATH_H

#pragma pack(1)


//
// Device Path defines and macros
//
#define EFI_DP_TYPE_MASK                    0x7F
#define EFI_DP_TYPE_UNPACKED                0x80
#define END_DEVICE_PATH_TYPE                0x7f
#define END_ENTIRE_DEVICE_PATH_SUBTYPE      0xff
#define END_INSTANCE_DEVICE_PATH_SUBTYPE    0x01
#define END_DEVICE_PATH_LENGTH              (sizeof(EFI_DEVICE_PATH_PROTOCOL))

#define DP_IS_END_TYPE(a)
#define DP_IS_END_SUBTYPE(a)        ( ((a)->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE )

#define DevicePathType(a)           ( ((a)->Type) & EFI_DP_TYPE_MASK )
#define DevicePathSubType(a)        ( (a)->SubType )
#define DevicePathNodeLength(a)     ( ((a)->Length[0]) | ((a)->Length[1] << 8) )
#define NextDevicePathNode(a)       ( (EFI_DEVICE_PATH_PROTOCOL *) ( ((UINT8 *) (a)) + DevicePathNodeLength(a)))
#define IsDevicePathEndType(a)      ( DevicePathType(a) == END_DEVICE_PATH_TYPE )
#define IsDevicePathEndSubType(a)   ( (a)->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE )
#define IsDevicePathEnd(a)          ( IsDevicePathEndType(a) && IsDevicePathEndSubType(a) )
#define IsDevicePathUnpacked(a)     ( (a)->Type & EFI_DP_TYPE_UNPACKED )


#define SetDevicePathNodeLength(a,l) {                \
          (a)->Length[0] = (UINT8) (l);               \
          (a)->Length[1] = (UINT8) ((l) >> 8);        \
                    }

#define SetDevicePathEndNode(a)  {                       \
          (a)->Type = END_DEVICE_PATH_TYPE;              \
          (a)->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE; \
          (a)->Length[0] = sizeof(EFI_DEVICE_PATH_PROTOCOL);      \
          (a)->Length[1] = 0;                            \
                    }



//
// Device Path protocol
//
#define EFI_DEVICE_PATH_PROTOCOL_GUID \
  { \
    0x9576e91, 0x6d3f, 0x11d2, 0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b \
  }

#pragma pack(push, 1)

typedef struct _EFI_DEVICE_PATH_PROTOCOL
{
    UINT8 Type;
    UINT8 SubType;
    UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

#pragma pack(pop)

#define EFI_END_ENTIRE_DEVICE_PATH            0xff
#define EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE    0xff
#define EFI_END_INSTANCE_DEVICE_PATH          0x01
#define EFI_END_DEVICE_PATH_LENGTH            (sizeof (EFI_DEVICE_PATH_PROTOCOL))

#define EfiDevicePathNodeLength(a)            (((a)->Length[0]) | ((a)->Length[1] << 8))
#define EfiNextDevicePathNode(a)              ((EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *) (a)) + EfiDevicePathNodeLength (a)))

#define EfiDevicePathType(a)                  (((a)->Type) & 0x7f)
#define EfiIsDevicePathEndType(a)             (EfiDevicePathType (a) == 0x7f)

#define EfiIsDevicePathEndSubType(a)          ((a)->SubType == EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE)
#define EfiIsDevicePathEndInstanceSubType(a)  ((a)->SubType == EFI_END_INSTANCE_DEVICE_PATH)

#define EfiIsDevicePathEnd(a)                 (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndSubType (a))
#define EfiIsDevicePathEndInstance(a)         (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndInstanceSubType (a))










//
// Hardware Device Paths
//
#define HARDWARE_DEVICE_PATH      0x01

#define HW_PCI_DP                 0x01
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT8                           Function;
    UINT8                           Device;
} PCI_DEVICE_PATH;

#define HW_PCCARD_DP              0x02
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT8                           FunctionNumber;
} PCCARD_DEVICE_PATH;

#define HW_MEMMAP_DP              0x03
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          MemoryType;
    EFI_PHYSICAL_ADDRESS            StartingAddress;
    EFI_PHYSICAL_ADDRESS            EndingAddress;
} MEMMAP_DEVICE_PATH;

#define HW_VENDOR_DP              0x04

typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    EFI_GUID                        Guid;
} VENDOR_DEVICE_PATH;

#define HW_CONTROLLER_DP          0x05
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          Controller;
} CONTROLLER_DEVICE_PATH;

//
// ACPI Device Paths
//
#define ACPI_DEVICE_PATH          0x02

#define ACPI_DP                   0x01
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          HID;
    UINT32                          UID;
} ACPI_HID_DEVICE_PATH;

#define ACPI_EXTENDED_DP          0x02
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          HID;
    UINT32                          UID;
    UINT32                          CID;
    //
    // Optional variable length _HIDSTR
    // Optional variable length _UIDSTR
    //
} ACPI_EXTENDED_HID_DEVICE_PATH;

#define ACPI_ADR_DP               0x03

typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          ADR;
} ACPI_ADR_DEVICE_PATH;

#define ACPI_ADR_DISPLAY_TYPE_OTHER             0
#define ACPI_ADR_DISPLAY_TYPE_VGA               1
#define ACPI_ADR_DISPLAY_TYPE_TV                2
#define ACPI_ADR_DISPLAY_TYPE_EXTERNAL_DIGITAL  3
#define ACPI_ADR_DISPLAY_TYPE_INTERNAL_DIGITAL  4

#define ACPI_DISPLAY_ADR(_DeviceIdScheme, _HeadId, _NonVgaOutput, _BiosCanDetect, _VendorInfo, _Type, _Port, _Index) \
          ((UINT32) ( (((_DeviceIdScheme) & 0x1) << 31) |  \
                      (((_HeadId)         & 0x7) << 18) |  \
                      (((_NonVgaOutput)   & 0x1) << 17) |  \
                      (((_BiosCanDetect)  & 0x1) << 16) |  \
                      (((_VendorInfo)     & 0xf) << 12) |  \
                      (((_Type)           & 0xf) << 8)  |  \
                      (((_Port)           & 0xf) << 4)  |  \
                       ((_Index)          & 0xf) ))

// 
//  EISA ID Macro
//  EISA ID Definition 32-bits
//   bits[15:0] - three character compressed ASCII EISA ID.
//   bits[31:16] - binary number
//    Compressed ASCII is 5 bits per character 0b00001 = 'A' 0b11010 = 'Z'
//
#define PNP_EISA_ID_CONST         0x41d0    
#define EISA_ID(_Name, _Num)      ((UINT32) ((_Name) | (_Num) << 16))   
#define EISA_PNP_ID(_PNPId)       (EISA_ID(PNP_EISA_ID_CONST, (_PNPId)))
#define EFI_PNP_ID(_PNPId)        (EISA_ID(PNP_EISA_ID_CONST, (_PNPId)))

#define PNP_EISA_ID_MASK          0xffff
#define EISA_ID_TO_NUM(_Id)       ((_Id) >> 16)

//
// Messaging Device Paths
//
#define MESSAGING_DEVICE_PATH     0x03

#define MSG_ATAPI_DP              0x01
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT8                           PrimarySecondary;
    UINT8                           SlaveMaster;
    UINT16                          Lun;
} ATAPI_DEVICE_PATH;

#define MSG_SCSI_DP               0x02
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT16                          Pun;
    UINT16                          Lun;
} SCSI_DEVICE_PATH;

#define MSG_FIBRECHANNEL_DP       0x03
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          Reserved;
    UINT64                          WWN;
    UINT64                          Lun;
} FIBRECHANNEL_DEVICE_PATH;

#define MSG_1394_DP               0x04
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          Reserved;
    UINT64                          Guid;
} F1394_DEVICE_PATH;

#define MSG_USB_DP                0x05
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT8                           ParentPortNumber;
    UINT8                           InterfaceNumber;
} USB_DEVICE_PATH;

#define MSG_USB_CLASS_DP          0x0f
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT16                          VendorId;
    UINT16                          ProductId;
    UINT8                           DeviceClass;
    UINT8                           DeviceSubClass;
    UINT8                           DeviceProtocol;
} USB_CLASS_DEVICE_PATH;

#define MSG_USB_WWID_DP           0x10
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT16                          InterfaceNumber;
    UINT16                          VendorId;
    UINT16                          ProductId;
    //
    // CHAR16                     SerialNumber[];
    //
} USB_WWID_DEVICE_PATH;

#define MSG_DEVICE_LOGICAL_UNIT_DP  0x11
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT8                           Lun;
} DEVICE_LOGICAL_UNIT_DEVICE_PATH;

#define MSG_SATA_DP               0x12
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT16                          HBAPortNumber;
    UINT16                          PortMultiplierPortNumber;
    UINT16                          Lun;
} SATA_DEVICE_PATH;

#define SATA_HBA_DIRECT_CONNECT_FLAG 0x8000

#define MSG_I2O_DP                0x06
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          Tid;
} I2O_DEVICE_PATH;

#define MSG_MAC_ADDR_DP           0x0b
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    EFI_MAC_ADDRESS                 MacAddress;
    UINT8                           IfType;
} MAC_ADDR_DEVICE_PATH;

#define MSG_IPv4_DP               0x0c
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    EFI_IPv4_ADDRESS                LocalIpAddress;
    EFI_IPv4_ADDRESS                RemoteIpAddress;
    UINT16                          LocalPort;
    UINT16                          RemotePort;
    UINT16                          Protocol;
    BOOLEAN                         StaticIpAddress;
} IPv4_DEVICE_PATH;

#define MSG_IPv6_DP               0x0d
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    EFI_IPv6_ADDRESS                LocalIpAddress;
    EFI_IPv6_ADDRESS                RemoteIpAddress;
    UINT16                          LocalPort;
    UINT16                          RemotePort;
    UINT16                          Protocol;
    BOOLEAN                         StaticIpAddress;
} IPv6_DEVICE_PATH;

#define MSG_INFINIBAND_DP         0x09
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          ResourceFlags;
    UINT8                           PortGid[16];
    UINT64                          ServiceId;
    UINT64                          TargetPortId;
    UINT64                          DeviceId;
} INFINIBAND_DEVICE_PATH;

#define INFINIBAND_RESOURCE_FLAG_IOC_SERVICE                0x01
#define INFINIBAND_RESOURCE_FLAG_EXTENDED_BOOT_ENVIRONMENT  0x02
#define INFINIBAND_RESOURCE_FLAG_CONSOLE_PROTOCOL           0x04
#define INFINIBAND_RESOURCE_FLAG_STORAGE_PROTOCOL           0x08
#define INFINIBAND_RESOURCE_FLAG_NETWORK_PROTOCOL           0x10

#define MSG_UART_DP               0x0e
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          Reserved;
    UINT64                          BaudRate;
    UINT8                           DataBits;
    UINT8                           Parity;
    UINT8                           StopBits;
} UART_DEVICE_PATH;

//
// Use VENDOR_DEVICE_PATH struct
//
#define MSG_VENDOR_DP             0x0a

#define DEVICE_PATH_MESSAGING_PC_ANSI \
    { 0xe0c14753, 0xf9be, 0x11d2,  0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d  }

#define DEVICE_PATH_MESSAGING_VT_100 \
    { 0xdfa66065, 0xb419, 0x11d3,  0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d  }

#define DEVICE_PATH_MESSAGING_VT_100_PLUS \
    { 0x7baec70b, 0x57e0, 0x4c76, 0x8e, 0x87, 0x2f, 0x9e, 0x28, 0x08, 0x83, 0x43  }

#define DEVICE_PATH_MESSAGING_VT_UTF8 \
    { 0xad15a0d6, 0x8bec, 0x4acf, 0xa0, 0x73, 0xd0, 0x1d, 0xe7, 0x7e, 0x2d, 0x88 }

#define DEVICE_PATH_MESSAGING_UART_FLOW_CONTROL \
    { 0x37499a9d, 0x542f, 0x4c89, 0xa0, 0x26, 0x35, 0xda, 0x14, 0x20, 0x94, 0xe4 }

typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    EFI_GUID                        Guid;
    UINT32                          FlowControlMap;
} UART_FLOW_CONTROL_DEVICE_PATH;

#define DEVICE_PATH_MESSAGING_SAS \
    { 0xd487ddb4, 0x008b, 0x11d9, 0xaf, 0xdc, 0x00, 0x10, 0x83, 0xff, 0xca, 0x4d }

typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    EFI_GUID                        Guid;
    UINT32                          Reserved;
    UINT64                          SasAddress;
    UINT64                          Lun;
    UINT16                          DeviceTopology;
    UINT16                          RelativeTargetPort;
} SAS_DEVICE_PATH;

#define MSG_ISCSI_DP              0x13
typedef struct
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT16                          NetworkProtocol;
    UINT16                          LoginOption;
    UINT64                          Lun;
    UINT16                          TargetPortalGroupTag;
    // CHAR8                        iSCSI Target Name
} ISCSI_DEVICE_PATH;

#define ISCSI_LOGIN_OPTION_NO_HEADER_DIGEST             0x0000
#define ISCSI_LOGIN_OPTION_HEADER_DIGEST_USING_CRC32C   0x0002
#define ISCSI_LOGIN_OPTION_NO_DATA_DIGEST               0x0000
#define ISCSI_LOGIN_OPTION_DATA_DIGEST_USING_CRC32C     0x0008
#define ISCSI_LOGIN_OPTION_AUTHMETHOD_CHAP              0x0000
#define ISCSI_LOGIN_OPTION_AUTHMETHOD_NON               0x1000
#define ISCSI_LOGIN_OPTION_CHAP_BI                      0x0000
#define ISCSI_LOGIN_OPTION_CHAP_UNI                     0x2000

//
// Media Device Path
//
#define MEDIA_DEVICE_PATH         0x04

#define MEDIA_HARDDRIVE_DP        0x01
typedef struct _HARDDRIVE_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          PartitionNumber;
    UINT64                          PartitionStart;
    UINT64                          PartitionSize;
    UINT8                           Signature[16];
    UINT8                           MBRType;
    UINT8                           SignatureType;
} HARDDRIVE_DEVICE_PATH;

#define MBR_TYPE_PCAT             0x01
#define MBR_TYPE_EFI_PARTITION_TABLE_HEADER 0x02

#define SIGNATURE_TYPE_MBR        0x01
#define SIGNATURE_TYPE_GUID       0x02

#define MEDIA_CDROM_DP            0x02
typedef struct _CDROM_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT32                          BootEntry;
    UINT64                          PartitionStart;
    UINT64                          PartitionSize;
} CDROM_DEVICE_PATH;

//
// Use VENDOR_DEVICE_PATH struct
//
#define MEDIA_VENDOR_DP           0x03

#define MEDIA_FILEPATH_DP         0x04
typedef struct _FILEPATH_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    WCHAR                           PathName[1];
} FILEPATH_DEVICE_PATH;

#define SIZE_OF_FILEPATH_DEVICE_PATH EFI_FIELD_OFFSET(FILEPATH_DEVICE_PATH,PathName)

#define MEDIA_PROTOCOL_DP         0x05
typedef struct _MEDIA_PROTOCOL_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    EFI_GUID                        Protocol;
} MEDIA_PROTOCOL_DEVICE_PATH;

#define MEDIA_FV_DP  0x07
typedef struct _MEDIA_FW_VOL_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL  Header;
    EFI_GUID                  NameGuid;
} MEDIA_FW_VOL_DEVICE_PATH;

#define MEDIA_FV_FILEPATH_DP  0x06
typedef struct _MEDIA_FW_VOL_FILEPATH_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL  Header;
    EFI_GUID                  NameGuid;
} MEDIA_FW_VOL_FILEPATH_DEVICE_PATH;

#define MEDIA_RELATIVE_OFFSET_RANGE_DP 0x08
typedef struct _MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL  Header;
    UINT64                    StartingOffset;
    UINT64                    EndingOffset;
} MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH;

//
// BBS Device Path
//
#define BBS_DEVICE_PATH           0x05
#define BBS_BBS_DP                0x01
typedef struct _BBS_BBS_DEVICE_PATH
{
    EFI_DEVICE_PATH_PROTOCOL        Header;
    UINT16                          DeviceType;
    UINT16                          StatusFlag;
    UCHAR                           String[ANYSIZE_ARRAY];
} BBS_BBS_DEVICE_PATH;

//
// DeviceType definitions - from BBS specification
//
#define BBS_TYPE_FLOPPY           0x01
#define BBS_TYPE_HARDDRIVE        0x02
#define BBS_TYPE_CDROM            0x03
#define BBS_TYPE_PCMCIA           0x04
#define BBS_TYPE_USB              0x05
#define BBS_TYPE_EMBEDDED_NETWORK 0x06
#define BBS_TYPE_BEV              0x80
#define BBS_TYPE_UNKNOWN          0xFF

#define UNKNOWN_DEVICE_GUID \
  { 0xcf31fac5, 0xc24e, 0x11d2,  0x85, 0xf3, 0x0, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b  }

typedef struct _UNKNOWN_DEVICE_VENDOR_DEVICE_PATH
{
    VENDOR_DEVICE_PATH              DevicePath;
    UINT8                           LegacyDriveLetter;
} UNKNOWN_DEVICE_VENDOR_DEVICE_PATH;


//
// Union of all possible Device Paths and pointers to Device Paths
//

typedef union _EFI_DEV_PATH
{
    EFI_DEVICE_PATH_PROTOCOL                    DevPath;
    PCI_DEVICE_PATH                             Pci;
    PCCARD_DEVICE_PATH                          PcCard;
    MEMMAP_DEVICE_PATH                          MemMap;
    VENDOR_DEVICE_PATH                          Vendor;

    UNKNOWN_DEVICE_VENDOR_DEVICE_PATH           UnknownVendor;

    CONTROLLER_DEVICE_PATH                      Controller;
    ACPI_HID_DEVICE_PATH                        Acpi;
    ACPI_EXTENDED_HID_DEVICE_PATH               ExtendedAcpi;
    ACPI_ADR_DEVICE_PATH                        AdrAcpi;

    ATAPI_DEVICE_PATH                           Atapi;
    SCSI_DEVICE_PATH                            Scsi;
    FIBRECHANNEL_DEVICE_PATH                    FibreChannel;
    SATA_DEVICE_PATH                            Sata;

    F1394_DEVICE_PATH                           F1394;
    USB_DEVICE_PATH                             Usb;
    USB_CLASS_DEVICE_PATH                       UsbClass;
    USB_WWID_DEVICE_PATH                        UsbWwid;
    DEVICE_LOGICAL_UNIT_DEVICE_PATH             LogicUnit;
    I2O_DEVICE_PATH                             I2O;
    MAC_ADDR_DEVICE_PATH                        MacAddr;
    IPv4_DEVICE_PATH                            Ipv4;
    IPv6_DEVICE_PATH                            Ipv6;
    INFINIBAND_DEVICE_PATH                      InfiniBand;
    UART_DEVICE_PATH                            Uart;
    UART_FLOW_CONTROL_DEVICE_PATH               UartFlowControl;
    SAS_DEVICE_PATH                             Sas;
    ISCSI_DEVICE_PATH                           Iscsi;
    HARDDRIVE_DEVICE_PATH                       HardDrive;
    CDROM_DEVICE_PATH                           CD;

    FILEPATH_DEVICE_PATH                        FilePath;
    MEDIA_PROTOCOL_DEVICE_PATH                  MediaProtocol;

    MEDIA_FW_VOL_DEVICE_PATH                    PiwgFirmwareVolume;
    MEDIA_FW_VOL_FILEPATH_DEVICE_PATH           PiwgFirmwareFile;
    MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH     Offset;

    BBS_BBS_DEVICE_PATH                         Bbs;
} EFI_DEV_PATH;



typedef union _EFI_DEV_PATH_PTR
{
    EFI_DEVICE_PATH_PROTOCOL             *DevPath;
    PCI_DEVICE_PATH                      *Pci;
    PCCARD_DEVICE_PATH                   *PcCard;
    MEMMAP_DEVICE_PATH                   *MemMap;
    VENDOR_DEVICE_PATH                   *Vendor;

    UNKNOWN_DEVICE_VENDOR_DEVICE_PATH    *UnknownVendor;

    CONTROLLER_DEVICE_PATH               *Controller;
    ACPI_HID_DEVICE_PATH                 *Acpi;
    ACPI_EXTENDED_HID_DEVICE_PATH        *ExtendedAcpi;
    ACPI_ADR_DEVICE_PATH                 *AdrAcpi;

    ATAPI_DEVICE_PATH                    *Atapi;
    SCSI_DEVICE_PATH                     *Scsi;
    FIBRECHANNEL_DEVICE_PATH             *FibreChannel;
    SATA_DEVICE_PATH                     *Sata;

    F1394_DEVICE_PATH                    *F1394;
    USB_DEVICE_PATH                      *Usb;
    USB_CLASS_DEVICE_PATH                *UsbClass;
    USB_WWID_DEVICE_PATH                 *UsbWwid;
    DEVICE_LOGICAL_UNIT_DEVICE_PATH      *LogicUnit;
    I2O_DEVICE_PATH                      *I2O;
    MAC_ADDR_DEVICE_PATH                 *MacAddr;
    IPv4_DEVICE_PATH                     *Ipv4;
    IPv6_DEVICE_PATH                     *Ipv6;
    INFINIBAND_DEVICE_PATH               *InfiniBand;
    UART_DEVICE_PATH                     *Uart;
    UART_FLOW_CONTROL_DEVICE_PATH        *UartFlowControl;

    SAS_DEVICE_PATH                      *Sas;
    ISCSI_DEVICE_PATH                    *Iscsi;

    HARDDRIVE_DEVICE_PATH                *HardDrive;
    CDROM_DEVICE_PATH                    *CD;

    FILEPATH_DEVICE_PATH                 *FilePath;
    MEDIA_PROTOCOL_DEVICE_PATH           *MediaProtocol;

    MEDIA_FW_VOL_DEVICE_PATH             *PiwgFirmwareVolume;
    MEDIA_FW_VOL_FILEPATH_DEVICE_PATH    *PiwgFirmwareFile;
    MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH     *Offset;

    BBS_BBS_DEVICE_PATH                  *Bbs;
    UINT8                                *Raw;
} EFI_DEV_PATH_PTR;

#pragma pack()

// Windows types (dmex)

#define PH_FIRST_BOOT_ENTRY(Variables) ((PBOOT_ENTRY_LIST)(Variables))
#define PH_NEXT_BOOT_ENTRY(Variable) ( \
    ((PBOOT_ENTRY_LIST)(Variable))->NextEntryOffset ? \
    (PBOOT_ENTRY_LIST)((PCHAR)(Variable) + \
    ((PBOOT_ENTRY_LIST)(Variable))->NextEntryOffset) : \
    NULL \
    )

#define FILE_PATH_TYPE_MIN FILE_PATH_TYPE_ARC
#define FILE_PATH_TYPE_MAX FILE_PATH_TYPE_EFI

#define WINDOWS_OS_OPTIONS_VERSION 1
#define WINDOWS_OS_OPTIONS_SIGNATURE "WINDOWS"

typedef struct _WINDOWS_OS_OPTIONS
{
    UCHAR Signature[8];
    ULONG Version;
    ULONG Length;
    ULONG OsLoadPathOffset; //FILE_PATH OsLoadPath;
    WCHAR OsLoadOptions[1];
} WINDOWS_OS_OPTIONS, *PWINDOWS_OS_OPTIONS;

#endif
