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
#include <secedit.h>

#include <devprv.h>
#include <devquery.h>

#define GUID_HWPROFILE_QUERY_CHANGE                        0xcb3a4001L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_HWPROFILE_CHANGE_CANCELLED                    0xcb3a4002L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_HWPROFILE_CHANGE_COMPLETE                     0xcb3a4003L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_DEVICE_INTERFACE_ARRIVAL                      0xcb3a4004L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_DEVICE_INTERFACE_REMOVAL                      0xcb3a4005L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_TARGET_DEVICE_QUERY_REMOVE                    0xcb3a4006L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_TARGET_DEVICE_REMOVE_CANCELLED                0xcb3a4007L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_TARGET_DEVICE_REMOVE_COMPLETE                 0xcb3a4008L, 0x46f0, 0x11d0, 0xb0, 0x8f, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3f
#define GUID_PNP_CUSTOM_NOTIFICATION                       0xACA73F8EL, 0x8D23, 0x11D1, 0xAC, 0x7D, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xD0
#define GUID_PNP_POWER_NOTIFICATION                        0xC2CF0660L, 0xEB7A, 0x11D1, 0xBD, 0x7F, 0x00, 0x00, 0xF8, 0x75, 0x71, 0xD0
#define GUID_PNP_POWER_SETTING_CHANGE                      0x29C69B3EL, 0xC79A, 0x43BF, 0xBB, 0xDE, 0xA9, 0x32, 0xFA, 0x1B, 0xEA, 0x7E
#define GUID_TARGET_DEVICE_TRANSPORT_RELATIONS_CHANGED     0xfcf528f6L, 0xa82f, 0x47b1, 0xad, 0x3a, 0x80, 0x50, 0x59, 0x4c, 0xad, 0x28
#define GUID_KERNEL_SOFT_RESTART_PREPARE                   0xde373defL, 0xa85c, 0x4f76, 0x8c, 0xbf, 0xf9, 0x6b, 0xea, 0x8b, 0xd1, 0x0f
#define GUID_KERNEL_SOFT_RESTART_CANCEL                    0x31d737e7L, 0x8c0b, 0x468a, 0x95, 0x6e, 0x9f, 0x43, 0x3e, 0xc3, 0x58, 0xfb
#define GUID_RECOVERY_PCI_PREPARE_SHUTDOWN                 0x90d889deL, 0x8704, 0x44cf, 0x81, 0x15, 0xed, 0x85, 0x28, 0xd2, 0xb2, 0xda
#define GUID_RECOVERY_NVMED_PREPARE_SHUTDOWN               0x4b9770eaL, 0xbde7, 0x400b, 0xa9, 0xb9, 0x4f, 0x68, 0x4f, 0x54, 0xcc, 0x2a
#define GUID_KERNEL_SOFT_RESTART_FINALIZE                  0x20e91abdL, 0x350a, 0x4d4f, 0x85, 0x77, 0x99, 0xc8, 0x15, 0x07, 0x47, 0x3a
#define GUID_BUS_INTERFACE_STANDARD                        0x496B8280L, 0x6F25, 0x11D0, 0xBE, 0xAF, 0x08, 0x00, 0x2B, 0xE2, 0x09, 0x2F
#define GUID_PCI_BUS_INTERFACE_STANDARD                    0x496B8281L, 0x6F25, 0x11D0, 0xBE, 0xAF, 0x08, 0x00, 0x2B, 0xE2, 0x09, 0x2F
#define GUID_PCI_BUS_INTERFACE_STANDARD2                   0xde94e966L, 0xfdff, 0x4c9c, 0x99, 0x98, 0x67, 0x47, 0xb1, 0x50, 0xe7, 0x4c
#define GUID_ARBITER_INTERFACE_STANDARD                    0xe644f185L, 0x8c0e, 0x11d0, 0xbe, 0xcf, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_TRANSLATOR_INTERFACE_STANDARD                 0x6c154a92L, 0xaacf, 0x11d0, 0x8d, 0x2a, 0x00, 0xa0, 0xc9, 0x06, 0xb2, 0x44
#define GUID_ACPI_INTERFACE_STANDARD                       0xb091a08aL, 0xba97, 0x11d0, 0xbd, 0x14, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a
#define GUID_INT_ROUTE_INTERFACE_STANDARD                  0x70941bf4L, 0x0073, 0x11d1, 0xa0, 0x9e, 0x00, 0xc0, 0x4f, 0xc3, 0x40, 0xb1
#define GUID_PCMCIA_BUS_INTERFACE_STANDARD                 0x76173af0L, 0xc504, 0x11d1, 0x94, 0x7f, 0x00, 0xc0, 0x4f, 0xb9, 0x60, 0xee
#define GUID_ACPI_REGS_INTERFACE_STANDARD                  0x06141966L, 0x7245, 0x6369, 0x46, 0x2e, 0x4e, 0x65, 0x6c, 0x73, 0x6f, 0x6e
#define GUID_LEGACY_DEVICE_DETECTION_STANDARD              0x50feb0deL, 0x596a, 0x11d2, 0xa5, 0xb8, 0x00, 0x00, 0xf8, 0x1a, 0x46, 0x19
#define GUID_PCI_DEVICE_PRESENT_INTERFACE                  0xd1b82c26L, 0xbf49, 0x45ef, 0xb2, 0x16, 0x71, 0xcb, 0xd7, 0x88, 0x9b, 0x57
#define GUID_MF_ENUMERATION_INTERFACE                      0xaeb895f0L, 0x5586, 0x11d1, 0x8d, 0x84, 0x00, 0xa0, 0xc9, 0x06, 0xb2, 0x44
#define GUID_REENUMERATE_SELF_INTERFACE_STANDARD           0x2aeb0243L, 0x6a6e, 0x486b, 0x82, 0xfc, 0xd8, 0x15, 0xf6, 0xb9, 0x70, 0x06
#define GUID_AGP_TARGET_BUS_INTERFACE_STANDARD             0xB15CFCE8L, 0x06D1, 0x4D37, 0x9D, 0x4C, 0xBE, 0xDD, 0xE0, 0xC2, 0xA6, 0xFF
#define GUID_ACPI_CMOS_INTERFACE_STANDARD                  0x3a8d0384L, 0x6505, 0x40ca, 0xbc, 0x39, 0x56, 0xc1, 0x5f, 0x8c, 0x5f, 0xed
#define GUID_ACPI_PORT_RANGES_INTERFACE_STANDARD           0xf14f609bL, 0xcbbd, 0x4957, 0xa6, 0x74, 0xbc, 0x00, 0x21, 0x3f, 0x1c, 0x97
#define GUID_ACPI_INTERFACE_STANDARD2                      0xe8695f63L, 0x1831, 0x4870, 0xa8, 0xcf, 0x9c, 0x2f, 0x03, 0xf9, 0xdc, 0xb5
#define GUID_PNP_LOCATION_INTERFACE                        0x70211b0eL, 0x0afb, 0x47db, 0xaf, 0xc1, 0x41, 0x0b, 0xf8, 0x42, 0x49, 0x7a
#define GUID_PCI_EXPRESS_LINK_QUIESCENT_INTERFACE          0x146cd41cL, 0xdae3, 0x4437, 0x8a, 0xff, 0x2a, 0xf3, 0xf0, 0x38, 0x09, 0x9b
#define GUID_PCI_EXPRESS_ROOT_PORT_INTERFACE               0x83a7734aL, 0x84c7, 0x4161, 0x9a, 0x98, 0x60, 0x00, 0xed, 0x0c, 0x4a, 0x33
#define GUID_MSIX_TABLE_CONFIG_INTERFACE                   0x1a6a460bL, 0x194f, 0x455d, 0xb3, 0x4b, 0xb8, 0x4c, 0x5b, 0x05, 0x71, 0x2b
#define GUID_D3COLD_SUPPORT_INTERFACE                      0xb38290e5L, 0x3cd0, 0x4f9d, 0x99, 0x37, 0xf5, 0xfe, 0x2b, 0x44, 0xd4, 0x7a
#define GUID_PROCESSOR_PCC_INTERFACE_STANDARD              0x37b17e9aL, 0xc21c, 0x4296, 0x97, 0x2d, 0x11, 0xc4, 0xb3, 0x2b, 0x28, 0xf0
#define GUID_PCI_VIRTUALIZATION_INTERFACE                  0x64897b47L, 0x3a4a, 0x4d75, 0xbc, 0x74, 0x89, 0xdd, 0x6c, 0x07, 0x82, 0x93
#define GUID_PCC_INTERFACE_STANDARD                        0x3ee8ba63L, 0x0f59, 0x4a24, 0x8a, 0x45, 0x35, 0x80, 0x8b, 0xdd, 0x12, 0x49
#define GUID_PCC_INTERFACE_INTERNAL                        0x7cce62ceL, 0xc189, 0x4814, 0xa6, 0xa7, 0x12, 0x11, 0x20, 0x89, 0xe9, 0x38
#define GUID_THERMAL_COOLING_INTERFACE                     0xecbe47a8L, 0xc498, 0x4bb9, 0xbd, 0x70, 0xe8, 0x67, 0xe0, 0x94, 0x0d, 0x22
#define GUID_DMA_CACHE_COHERENCY_INTERFACE                 0xb520f7faL, 0x8a5a, 0x4e40, 0xa3, 0xf6, 0x6b, 0xe1, 0xe1, 0x62, 0xd9, 0x35
#define GUID_DEVICE_RESET_INTERFACE_STANDARD               0x649fdf26L, 0x3bc0, 0x4813, 0xad, 0x24, 0x7e, 0x0c, 0x1e, 0xda, 0x3f, 0xa3
#define GUID_IOMMU_BUS_INTERFACE                           0x1efee0b2L, 0xd278, 0x4ae4, 0xbd, 0xdc, 0x1b, 0x34, 0xdd, 0x64, 0x80, 0x43
#define GUID_PCI_SECURITY_INTERFACE                        0x6e7f1451L, 0x199e, 0x4acc, 0xba, 0x2d, 0x76, 0x2b, 0x4e, 0xdf, 0x46, 0x74
#define GUID_SCM_BUS_INTERFACE                             0x25944783L, 0xce79, 0x4232, 0x81, 0x5e, 0x4a, 0x30, 0x01, 0x4e, 0x8e, 0xb4
#define GUID_SECURE_DRIVER_INTERFACE                       0x370f67e1L, 0x4ff5, 0x4a94, 0x9a, 0x35, 0x06, 0xc5, 0xd9, 0xcc, 0x30, 0xe2
#define GUID_SDEV_IDENTIFIER_INTERFACE                     0x49d67af8L, 0x916c, 0x4ee8, 0x9d, 0xf1, 0x88, 0x9f, 0x17, 0xd2, 0x1e, 0x91
#define GUID_SCM_BUS_NVD_INTERFACE                         0x8de064ffL, 0xb630, 0x42e4, 0x88, 0xea, 0x6f, 0x24, 0xc8, 0x64, 0x11, 0x75
#define GUID_SCM_BUS_LD_INTERFACE                          0x9b89307dL, 0xd76b, 0x4f48, 0xb1, 0x86, 0x54, 0x04, 0x1a, 0xe9, 0x2e, 0x8d
#define GUID_SCM_PHYSICAL_NVDIMM_INTERFACE                 0x0079c21bL, 0x917e, 0x405e, 0xa9, 0xce, 0x07, 0x32, 0xb5, 0xbb, 0xce, 0xbd
#define GUID_PNP_EXTENDED_ADDRESS_INTERFACE                0xb8e992ecL, 0xa797, 0x4dc4, 0x88, 0x46, 0x84, 0xd0, 0x41, 0x70, 0x74, 0x46
#define GUID_D3COLD_AUX_POWER_AND_TIMING_INTERFACE         0x0044d8aaL, 0xf664, 0x4588, 0x9f, 0xfc, 0x2a, 0xfe, 0xaf, 0x59, 0x50, 0xb9
#define GUID_PCI_FPGA_CONTROL_INTERFACE                    0x2df3f7a8L, 0xb9b3, 0x4063, 0x92, 0x15, 0xb5, 0xd1, 0x4a, 0x0b, 0x26, 0x6e
#define GUID_PCI_PTM_CONTROL_INTERFACE                     0x348a5ebbL, 0xba24, 0x44b7, 0x99, 0x16, 0x28, 0x56, 0x87, 0x73, 0x51, 0x17
#define GUID_BUS_RESOURCE_UPDATE_INTERFACE                 0x27d0102dL, 0xbfb2, 0x4164, 0x81, 0xdd, 0xdb, 0xb8, 0x2f, 0x96, 0x8b, 0x48
#define GUID_NPEM_CONTROL_INTERFACE                        0x4d95573dL, 0xb774, 0x488a, 0xb1, 0x20, 0x4f, 0x28, 0x4a, 0x9e, 0xff, 0x51
#define GUID_PCI_ATS_INTERFACE                             0x010a7fe8L, 0x96f5, 0x4943, 0xbe, 0xdf, 0x95, 0xe6, 0x51, 0xb9, 0x34, 0x12
#define GUID_BUS_TYPE_INTERNAL                             0x1530ea73L, 0x086b, 0x11d1, 0xa0, 0x9f, 0x00, 0xc0, 0x4f, 0xc3, 0x40, 0xb1
#define GUID_BUS_TYPE_PCMCIA                               0x09343630L, 0xaf9f, 0x11d0, 0x92, 0xE9, 0x00, 0x00, 0xf8, 0x1e, 0x1b, 0x30
#define GUID_BUS_TYPE_PCI                                  0xc8ebdfb0L, 0xb510, 0x11d0, 0x80, 0xe5, 0x00, 0xa0, 0xc9, 0x25, 0x42, 0xe3
#define GUID_BUS_TYPE_ISAPNP                               0xe676f854L, 0xd87d, 0x11d0, 0x92, 0xb2, 0x00, 0xa0, 0xc9, 0x05, 0x5f, 0xc5
#define GUID_BUS_TYPE_EISA                                 0xddc35509L, 0xf3fc, 0x11d0, 0xa5, 0x37, 0x00, 0x00, 0xf8, 0x75, 0x3e, 0xd1
#define GUID_BUS_TYPE_MCA                                  0x1c75997aL, 0xdc33, 0x11d0, 0x92, 0xb2, 0x00, 0xa0, 0xc9, 0x05, 0x5f, 0xc5
#define GUID_BUS_TYPE_SERENUM                              0x77114a87L, 0x8944, 0x11d1, 0xbd, 0x90, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0x2d
#define GUID_BUS_TYPE_USB                                  0x9d7debbcL, 0xc85d, 0x11d1, 0x9e, 0xb4, 0x00, 0x60, 0x08, 0xc3, 0xa1, 0x9a
#define GUID_BUS_TYPE_LPTENUM                              0xc4ca1000L, 0x2ddc, 0x11d5, 0xa1, 0x7a, 0x00, 0xc0, 0x4f, 0x60, 0x52, 0x4d
#define GUID_BUS_TYPE_USBPRINT                             0x441ee000L, 0x4342, 0x11d5, 0xa1, 0x84, 0x00, 0xc0, 0x4f, 0x60, 0x52, 0x4d
#define GUID_BUS_TYPE_DOT4PRT                              0x441ee001L, 0x4342, 0x11d5, 0xa1, 0x84, 0x00, 0xc0, 0x4f, 0x60, 0x52, 0x4d
#define GUID_BUS_TYPE_1394                                 0xf74e73ebL, 0x9ac5, 0x45eb, 0xbe, 0x4d, 0x77, 0x2c, 0xc7, 0x1d, 0xdf, 0xb3
#define GUID_BUS_TYPE_HID                                  0xeeaf37d0L, 0x1963, 0x47c4, 0xaa, 0x48, 0x72, 0x47, 0x6d, 0xb7, 0xcf, 0x49
#define GUID_BUS_TYPE_AVC                                  0xc06ff265L, 0xae09, 0x48f0, 0x81, 0x2c, 0x16, 0x75, 0x3d, 0x7c, 0xba, 0x83
#define GUID_BUS_TYPE_IRDA                                 0x7ae17dc1L, 0xc944, 0x44d6, 0x88, 0x1f, 0x4c, 0x2e, 0x61, 0x05, 0x3b, 0xc1
#define GUID_BUS_TYPE_SD                                   0xe700cc04L, 0x4036, 0x4e89, 0x95, 0x79, 0x89, 0xeb, 0xf4, 0x5f, 0x00, 0xcd
#define GUID_BUS_TYPE_ACPI                                 0xd7b46895L, 0x001a, 0x4942, 0x89, 0x1f, 0xa7, 0xd4, 0x66, 0x10, 0xa8, 0x43
#define GUID_BUS_TYPE_SW_DEVICE                            0x06d10322L, 0x7de0, 0x4cef, 0x8e, 0x25, 0x19, 0x7d, 0x0e, 0x74, 0x42, 0xe2
#define GUID_BUS_TYPE_SCM                                  0x375a5912L, 0x804c, 0x45aa, 0xbd, 0xc2, 0xfd, 0xd2, 0x5a, 0x1d, 0x95, 0x12
#define GUID_POWER_DEVICE_ENABLE                           0x827c0a6fL, 0xfeb0, 0x11d0, 0xbd, 0x26, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a
#define GUID_POWER_DEVICE_TIMEOUTS                         0xa45da735L, 0xfeb0, 0x11d0, 0xbd, 0x26, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a
#define GUID_POWER_DEVICE_WAKE_ENABLE                      0xa9546a82L, 0xfeb0, 0x11d0, 0xbd, 0x26, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a
#define GUID_WUDF_DEVICE_HOST_PROBLEM                      0xc43d25bdL, 0x9346, 0x40ee, 0xa2, 0xd2, 0xd7, 0x0c, 0x15, 0xf8, 0xb7, 0x5b
#define GUID_PARTITION_UNIT_INTERFACE_STANDARD             0x52363f5bL, 0xd891, 0x429b, 0x81, 0x95, 0xae, 0xc5, 0xfe, 0xf6, 0x85, 0x3c
#define GUID_QUERY_CRASHDUMP_FUNCTIONS                     0x9cc6b8ffL, 0x32e2, 0x4834, 0xb1, 0xde, 0xb3, 0x2e, 0xf8, 0x88, 0x0a, 0x4b
#define GUID_DEVINTERFACE_DISK                             0x53f56307L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_CDROM                            0x53f56308L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_PARTITION                        0x53f5630aL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_TAPE                             0x53f5630bL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_WRITEONCEDISK                    0x53f5630cL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_VOLUME                           0x53f5630dL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_MEDIUMCHANGER                    0x53f56310L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_FLOPPY                           0x53f56311L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_CDCHANGER                        0x53f56312L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_STORAGEPORT                      0x2accfe60L, 0xc130, 0x11d2, 0xb0, 0x82, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_DEVINTERFACE_VMLUN                            0x6f416619L, 0x9f29, 0x42a5, 0xb2, 0x0b, 0x37, 0xe2, 0x19, 0xca, 0x02, 0xb0
#define GUID_DEVINTERFACE_SES                              0x1790c9ecL, 0x47d5, 0x4df3, 0xb5, 0xaf, 0x9a, 0xdf, 0x3c, 0xf2, 0x3e, 0x48
#define GUID_DEVINTERFACE_ZNSDISK                          0xb87941c5L, 0xffdb, 0x43c7, 0xb6, 0xb1, 0x20, 0xb6, 0x32, 0xf0, 0xb1, 0x09
#define GUID_DEVINTERFACE_SERVICE_VOLUME                   0x6ead3d82L, 0x25ec, 0x46bc, 0xb7, 0xfd, 0xc1, 0xf0, 0xdf, 0x8f, 0x50, 0x37
#define GUID_DEVINTERFACE_HIDDEN_VOLUME                    0x7f108a28L, 0x9833, 0x4b3b, 0xb7, 0x80, 0x2c, 0x6b, 0x5f, 0xa5, 0xc0, 0x62
#define GUID_DEVINTERFACE_UNIFIED_ACCESS_RPMB              0x27447c21L, 0xbcc3, 0x4d07, 0xa0, 0x5b, 0xa3, 0x39, 0x5b, 0xb4, 0xee, 0xe7
#define GUID_DEVINTERFACE_SCM_PHYSICAL_DEVICE              0x4283609dL, 0x4dc2, 0x43be, 0xbb, 0xb4, 0x4f, 0x15, 0xdf, 0xce, 0x2c, 0x61
#define GUID_SCM_PD_HEALTH_NOTIFICATION                    0x9da2d386L, 0x72f5, 0x4ee3, 0x81, 0x55, 0xec, 0xa0, 0x67, 0x8e, 0x3b, 0x06
#define GUID_SCM_PD_PASSTHROUGH_INVDIMM                    0x4309AC30L, 0x0D11, 0x11E4, 0x91, 0x91, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66
#define GUID_DEVINTERFACE_COMPORT                          0X86E0D1E0L, 0X8089, 0X11D0, 0X9C, 0XE4, 0X08, 0X00, 0X3E, 0X30, 0X1F, 0X73
#define GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR           0x4D36E978L, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18
#define GUID_BTHPORT_DEVICE_INTERFACE                      0x0850302aL, 0xb344, 0x4fda, 0x9b, 0xe9, 0x90, 0x57, 0x6b, 0x8d, 0x46, 0xf0
#define GUID_BTH_RFCOMM_SERVICE_DEVICE_INTERFACE           0xb142fc3eL, 0xfa4e, 0x460b, 0x8a, 0xbc, 0x07, 0x2b, 0x62, 0x8b, 0x3c, 0x70
#define GUID_DEVCLASS_1394                                 0x6bdd1fc1L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_DEVCLASS_1394DEBUG                            0x66f250d6L, 0x7801, 0x4a64, 0xb1, 0x39, 0xee, 0xa8, 0x0a, 0x45, 0x0b, 0x24
#define GUID_DEVCLASS_61883                                0x7ebefbc0L, 0x3200, 0x11d2, 0xb4, 0xc2, 0x00, 0xa0, 0xc9, 0x69, 0x7d, 0x07
#define GUID_DEVCLASS_ADAPTER                              0x4d36e964L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_APMSUPPORT                           0xd45b1c18L, 0xc8fa, 0x11d1, 0x9f, 0x77, 0x00, 0x00, 0xf8, 0x05, 0xf5, 0x30
#define GUID_DEVCLASS_AVC                                  0xc06ff265L, 0xae09, 0x48f0, 0x81, 0x2c, 0x16, 0x75, 0x3d, 0x7c, 0xba, 0x83
#define GUID_DEVCLASS_BATTERY                              0x72631e54L, 0x78a4, 0x11d0, 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a
#define GUID_DEVCLASS_BIOMETRIC                            0x53d29ef7L, 0x377c, 0x4d14, 0x86, 0x4b, 0xeb, 0x3a, 0x85, 0x76, 0x93, 0x59
#define GUID_DEVCLASS_BLUETOOTH                            0xe0cbf06cL, 0xcd8b, 0x4647, 0xbb, 0x8a, 0x26, 0x3b, 0x43, 0xf0, 0xf9, 0x74
#define GUID_DEVCLASS_CAMERA                               0xca3e7ab9L, 0xb4c3, 0x4ae6, 0x82, 0x51, 0x57, 0x9e, 0xf9, 0x33, 0x89, 0x0f
#define GUID_DEVCLASS_CDROM                                0x4d36e965L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_COMPUTEACCELERATOR                   0xf01a9d53L, 0x3ff6, 0x48d2, 0x9f, 0x97, 0xc8, 0xa7, 0x00, 0x4b, 0xe1, 0x0c
#define GUID_DEVCLASS_COMPUTER                             0x4d36e966L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_DECODER                              0x6bdd1fc2L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_DEVCLASS_DISKDRIVE                            0x4d36e967L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_DISPLAY                              0x4d36e968L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_DOT4                                 0x48721b56L, 0x6795, 0x11d2, 0xb1, 0xa8, 0x00, 0x80, 0xc7, 0x2e, 0x74, 0xa2
#define GUID_DEVCLASS_DOT4PRINT                            0x49ce6ac8L, 0x6f86, 0x11d2, 0xb1, 0xe5, 0x00, 0x80, 0xc7, 0x2e, 0x74, 0xa2
#define GUID_DEVCLASS_EHSTORAGESILO                        0x9da2b80fL, 0xf89f, 0x4a49, 0xa5, 0xc2, 0x51, 0x1b, 0x08, 0x5b, 0x9e, 0x8a
#define GUID_DEVCLASS_ENUM1394                             0xc459df55L, 0xdb08, 0x11d1, 0xb0, 0x09, 0x00, 0xa0, 0xc9, 0x08, 0x1f, 0xf6
#define GUID_DEVCLASS_EXTENSION                            0xe2f84ce7L, 0x8efa, 0x411c, 0xaa, 0x69, 0x97, 0x45, 0x4c, 0xa4, 0xcb, 0x57
#define GUID_DEVCLASS_FDC                                  0x4d36e969L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_FIRMWARE                             0xf2e7dd72L, 0x6468, 0x4e36, 0xb6, 0xf1, 0x64, 0x88, 0xf4, 0x2c, 0x1b, 0x52
#define GUID_DEVCLASS_FLOPPYDISK                           0x4d36e980L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_GENERIC                              0xff494df1L, 0xc4ed, 0x4fac, 0x9b, 0x3f, 0x37, 0x86, 0xf6, 0xe9, 0x1e, 0x7e
#define GUID_DEVCLASS_GPS                                  0x6bdd1fc3L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_DEVCLASS_HDC                                  0x4d36e96aL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_HIDCLASS                             0x745a17a0L, 0x74d3, 0x11d0, 0xb6, 0xfe, 0x00, 0xa0, 0xc9, 0x0f, 0x57, 0xda
#define GUID_DEVCLASS_HOLOGRAPHIC                          0xd612553dL, 0x06b1, 0x49ca, 0x89, 0x38, 0xe3, 0x9e, 0xf8, 0x0e, 0xb1, 0x6f
#define GUID_DEVCLASS_IMAGE                                0x6bdd1fc6L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_DEVCLASS_INFINIBAND                           0x30ef7132L, 0xd858, 0x4a0c, 0xac, 0x24, 0xb9, 0x02, 0x8a, 0x5c, 0xca, 0x3f
#define GUID_DEVCLASS_INFRARED                             0x6bdd1fc5L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_DEVCLASS_KEYBOARD                             0x4d36e96bL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_LEGACYDRIVER                         0x8ecc055dL, 0x047f, 0x11d1, 0xa5, 0x37, 0x00, 0x00, 0xf8, 0x75, 0x3e, 0xd1
#define GUID_DEVCLASS_MEDIA                                0x4d36e96cL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_MEDIUM_CHANGER                       0xce5939aeL, 0xebde, 0x11d0, 0xb1, 0x81, 0x00, 0x00, 0xf8, 0x75, 0x3e, 0xc4
#define GUID_DEVCLASS_MEMORY                               0x5099944aL, 0xf6b9, 0x4057, 0xa0, 0x56, 0x8c, 0x55, 0x02, 0x28, 0x54, 0x4c
#define GUID_DEVCLASS_MODEM                                0x4d36e96dL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_MONITOR                              0x4d36e96eL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_MOUSE                                0x4d36e96fL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_MTD                                  0x4d36e970L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_MULTIFUNCTION                        0x4d36e971L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_MULTIPORTSERIAL                      0x50906cb8L, 0xba12, 0x11d1, 0xbf, 0x5d, 0x00, 0x00, 0xf8, 0x05, 0xf5, 0x30
#define GUID_DEVCLASS_NET                                  0x4d36e972L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_NETCLIENT                            0x4d36e973L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_NETDRIVER                            0x87ef9ad1L, 0x8f70, 0x49ee, 0xb2, 0x15, 0xab, 0x1f, 0xca, 0xdc, 0xbe, 0x3c
#define GUID_DEVCLASS_NETSERVICE                           0x4d36e974L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_NETTRANS                             0x4d36e975L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_NETUIO                               0x78912bc1L, 0xcb8e, 0x4b28, 0xa3, 0x29, 0xf3, 0x22, 0xeb, 0xad, 0xbe, 0x0f
#define GUID_DEVCLASS_NODRIVER                             0x4d36e976L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_PCMCIA                               0x4d36e977L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_PNPPRINTERS                          0x4658ee7eL, 0xf050, 0x11d1, 0xb6, 0xbd, 0x00, 0xc0, 0x4f, 0xa3, 0x72, 0xa7
#define GUID_DEVCLASS_PORTS                                0x4d36e978L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_PRIMITIVE                            0x242681d1L, 0xeed3, 0x41d2, 0xa1, 0xef, 0x14, 0x68, 0xfc, 0x84, 0x31, 0x06
#define GUID_DEVCLASS_PRINTER                              0x4d36e979L, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_PRINTERUPGRADE                       0x4d36e97aL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_PRINTQUEUE                           0x1ed2bbf9L, 0x11f0, 0x4084, 0xb2, 0x1f, 0xad, 0x83, 0xa8, 0xe6, 0xdc, 0xdc
#define GUID_DEVCLASS_PROCESSOR                            0x50127dc3L, 0x0f36, 0x415e, 0xa6, 0xcc, 0x4c, 0xb3, 0xbe, 0x91, 0x0B, 0x65
#define GUID_DEVCLASS_SBP2                                 0xd48179beL, 0xec20, 0x11d1, 0xb6, 0xb8, 0x00, 0xc0, 0x4f, 0xa3, 0x72, 0xa7
#define GUID_DEVCLASS_SCMDISK                              0x53966cb1L, 0x4d46, 0x4166, 0xbf, 0x23, 0xc5, 0x22, 0x40, 0x3c, 0xd4, 0x95
#define GUID_DEVCLASS_SCMVOLUME                            0x53ccb149L, 0xe543, 0x4c84, 0xb6, 0xe0, 0xbc, 0xe4, 0xf6, 0xb7, 0xe8, 0x06
#define GUID_DEVCLASS_SCSIADAPTER                          0x4d36e97bL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_SECURITYACCELERATOR                  0x268c95a1L, 0xedfe, 0x11d3, 0x95, 0xc3, 0x00, 0x10, 0xdc, 0x40, 0x50, 0xa5
#define GUID_DEVCLASS_SENSOR                               0x5175d334L, 0xc371, 0x4806, 0xb3, 0xba, 0x71, 0xfd, 0x53, 0xc9, 0x25, 0x8d
#define GUID_DEVCLASS_SIDESHOW                             0x997b5d8dL, 0xc442, 0x4f2e, 0xba, 0xf3, 0x9c, 0x8e, 0x67, 0x1e, 0x9e, 0x21
#define GUID_DEVCLASS_SMARTCARDREADER                      0x50dd5230L, 0xba8a, 0x11d1, 0xbf, 0x5d, 0x00, 0x00, 0xf8, 0x05, 0xf5, 0x30
#define GUID_DEVCLASS_SMRDISK                              0x53487c23L, 0x680f, 0x4585, 0xac, 0xc3, 0x1f, 0x10, 0xd6, 0x77, 0x7e, 0x82
#define GUID_DEVCLASS_SMRVOLUME                            0x53b3cf03L, 0x8f5a, 0x4788, 0x91, 0xb6, 0xd1, 0x9e, 0xd9, 0xfc, 0xcc, 0xbf
#define GUID_DEVCLASS_SOFTWARECOMPONENT                    0x5c4c3332L, 0x344d, 0x483c, 0x87, 0x39, 0x25, 0x9e, 0x93, 0x4c, 0x9c, 0xc8
#define GUID_DEVCLASS_SOUND                                0x4d36e97cL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_SYSTEM                               0x4d36e97dL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_TAPEDRIVE                            0x6d807884L, 0x7d21, 0x11cf, 0x80, 0x1c, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_UNKNOWN                              0x4d36e97eL, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18
#define GUID_DEVCLASS_UCM                                  0xe6f1aa1cL, 0x7f3b, 0x4473, 0xb2, 0xe8, 0xc9, 0x7d, 0x8a, 0xc7, 0x1d, 0x53
#define GUID_DEVCLASS_USB                                  0x36fc9e60L, 0xc465, 0x11cf, 0x80, 0x56, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00
#define GUID_DEVCLASS_VOLUME                               0x71a27cddL, 0x812a, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_DEVCLASS_VOLUMESNAPSHOT                       0x533c5b84L, 0xec70, 0x11d2, 0x95, 0x05, 0x00, 0xc0, 0x4f, 0x79, 0xde, 0xaf
#define GUID_DEVCLASS_WCEUSBS                              0x25dbce51L, 0x6c8f, 0x4a72, 0x8a, 0x6d, 0xb5, 0x4c, 0x2b, 0x4f, 0xc8, 0x35
#define GUID_DEVCLASS_WPD                                  0xeec5ad98L, 0x8080, 0x425f, 0x92, 0x2a, 0xda, 0xbf, 0x3d, 0xe3, 0xf6, 0x9a
#define GUID_DEVCLASS_FSFILTER_TOP                         0xb369baf4L, 0x5568, 0x4e82, 0xa8, 0x7e, 0xa9, 0x3e, 0xb1, 0x6b, 0xca, 0x87
#define GUID_DEVCLASS_FSFILTER_ACTIVITYMONITOR             0xb86dff51L, 0xa31e, 0x4bac, 0xb3, 0xcf, 0xe8, 0xcf, 0xe7, 0x5c, 0x9f, 0xc2
#define GUID_DEVCLASS_FSFILTER_UNDELETE                    0xfe8f1572L, 0xc67a, 0x48c0, 0xbb, 0xac, 0x0b, 0x5c, 0x6d, 0x66, 0xca, 0xfb
#define GUID_DEVCLASS_FSFILTER_ANTIVIRUS                   0xb1d1a169L, 0xc54f, 0x4379, 0x81, 0xdb, 0xbe, 0xe7, 0xd8, 0x8d, 0x74, 0x54
#define GUID_DEVCLASS_FSFILTER_REPLICATION                 0x48d3ebc4L, 0x4cf8, 0x48ff, 0xb8, 0x69, 0x9c, 0x68, 0xad, 0x42, 0xeb, 0x9f
#define GUID_DEVCLASS_FSFILTER_CONTINUOUSBACKUP            0x71aa14f8L, 0x6fad, 0x4622, 0xad, 0x77, 0x92, 0xbb, 0x9d, 0x7e, 0x69, 0x47
#define GUID_DEVCLASS_FSFILTER_CONTENTSCREENER             0x3e3f0674L, 0xc83c, 0x4558, 0xbb, 0x26, 0x98, 0x20, 0xe1, 0xeb, 0xa5, 0xc5
#define GUID_DEVCLASS_FSFILTER_QUOTAMANAGEMENT             0x8503c911L, 0xa6c7, 0x4919, 0x8f, 0x79, 0x50, 0x28, 0xf5, 0x86, 0x6b, 0x0c
#define GUID_DEVCLASS_FSFILTER_SYSTEMRECOVERY              0x2db15374L, 0x706e, 0x4131, 0xa0, 0xc7, 0xd7, 0xc7, 0x8e, 0xb0, 0x28, 0x9a
#define GUID_DEVCLASS_FSFILTER_CFSMETADATASERVER           0xcdcf0939L, 0xb75b, 0x4630, 0xbf, 0x76, 0x80, 0xf7, 0xba, 0x65, 0x58, 0x84
#define GUID_DEVCLASS_FSFILTER_HSM                         0xd546500aL, 0x2aeb, 0x45f6, 0x94, 0x82, 0xf4, 0xb1, 0x79, 0x9c, 0x31, 0x77
#define GUID_DEVCLASS_FSFILTER_COMPRESSION                 0xf3586bafL, 0xb5aa, 0x49b5, 0x8d, 0x6c, 0x05, 0x69, 0x28, 0x4c, 0x63, 0x9f
#define GUID_DEVCLASS_FSFILTER_ENCRYPTION                  0xa0a701c0L, 0xa511, 0x42ff, 0xaa, 0x6c, 0x06, 0xdc, 0x03, 0x95, 0x57, 0x6f
#define GUID_DEVCLASS_FSFILTER_VIRTUALIZATION              0xf75a86c0L, 0x10d8, 0x4c3a, 0xb2, 0x33, 0xed, 0x60, 0xe4, 0xcd, 0xfa, 0xac
#define GUID_DEVCLASS_FSFILTER_PHYSICALQUOTAMANAGEMENT     0x6a0a8e78L, 0xbba6, 0x4fc4, 0xa7, 0x09, 0x1e, 0x33, 0xcd, 0x09, 0xd6, 0x7e
#define GUID_DEVCLASS_FSFILTER_OPENFILEBACKUP              0xf8ecafa6L, 0x66d1, 0x41a5, 0x89, 0x9b, 0x66, 0x58, 0x5d, 0x72, 0x16, 0xb7
#define GUID_DEVCLASS_FSFILTER_SECURITYENHANCER            0xd02bc3daL, 0x0c8e, 0x4945, 0x9b, 0xd5, 0xf1, 0x88, 0x3c, 0x22, 0x6c, 0x8c
#define GUID_DEVCLASS_FSFILTER_COPYPROTECTION              0x89786ff1L, 0x9c12, 0x402f, 0x9c, 0x9e, 0x17, 0x75, 0x3c, 0x7f, 0x43, 0x75
#define GUID_DEVCLASS_FSFILTER_BOTTOM                      0x37765ea0L, 0x5958, 0x4fc9, 0xb0, 0x4b, 0x2f, 0xdf, 0xef, 0x97, 0xe5, 0x9e
#define GUID_DEVCLASS_FSFILTER_SYSTEM                      0x5d1b9aaaL, 0x01e2, 0x46af, 0x84, 0x9f, 0x27, 0x2b, 0x3f, 0x32, 0x4c, 0x46
#define GUID_DEVCLASS_FSFILTER_INFRASTRUCTURE              0xe55fa6f9L, 0x128c, 0x4d04, 0xab, 0xab, 0x63, 0x0c, 0x74, 0xb1, 0x45, 0x3a
#define GUID_DEVINTERFACE_USB_HUB                          0xf18a0e88L, 0xc30c, 0x11d0, 0x88, 0x15, 0x00, 0xa0, 0xc9, 0x06, 0xbe, 0xd8
#define GUID_DEVINTERFACE_USB_BILLBOARD                    0x5e9adaefL, 0xf879, 0x473f, 0xb8, 0x07, 0x4e, 0x5e, 0xa7, 0x7d, 0x1b, 0x1c
#define GUID_DEVINTERFACE_USB_DEVICE                       0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED
#define GUID_DEVINTERFACE_USB_HOST_CONTROLLER              0x3abf6f2dL, 0x71c4, 0x462a, 0x8a, 0x92, 0x1e, 0x68, 0x61, 0xe6, 0xaf, 0x27
#define GUID_DEVINTERFACE_A2DP_SIDEBAND_AUDIO              0xf3b1362fL, 0xc9f4, 0x4dd1, 0x9d, 0x55, 0xe0, 0x20, 0x38, 0xa1, 0x29, 0xfb
#define GUID_DEVINTERFACE_BLUETOOTH_HFP_SCO_HCIBYPASS      0xbe446647L, 0xf655, 0x4919, 0x8b, 0xd0, 0x12, 0x5b, 0xa5, 0xd4, 0xce, 0x65
#define GUID_DEVINTERFACE_CHARGING_ARBITRATION             0xec0a1cc9L, 0x4294, 0x43fb, 0xbf, 0x37, 0xb8, 0x50, 0xce, 0x95, 0xf3, 0x37
#define GUID_DEVINTERFACE_CONFIGURABLE_USBFN_CHARGER       0x7158c35cL, 0xc1bc, 0x4d90, 0xac, 0xb1, 0x80, 0x20, 0xbd, 0x0e, 0x19, 0xca
#define GUID_DEVINTERFACE_CONFIGURABLE_WIRELESS_CHARGER    0x3612b1c8L, 0x3633, 0x47d3, 0x8a, 0xf5, 0x00, 0xa4, 0xdf, 0xa0, 0x47, 0x93
#define GUID_DEVINTERFACE_I2C                              0x2564AA4FL, 0xDDDB, 0x4495, 0xB4, 0x97, 0x6A, 0xD4, 0xA8, 0x41, 0x63, 0xD7
#define GUID_DEVINTERFACE_OPM                              0xBF4672DEL, 0x6B4E, 0x4BE4, 0xA3, 0x25, 0x68, 0xA9, 0x1E, 0xA4, 0x9C, 0x09
#define GUID_DEVINTERFACE_OPM_2_JTP                        0xE929EEA4L, 0xB9F1, 0x407B, 0xAA, 0xB9, 0xAB, 0x08, 0xBB, 0x44, 0xFB, 0xF4
#define GUID_DEVINTERFACE_OPM_2                            0x7F098726L, 0x2EBB, 0x4FF3, 0xA2, 0x7F, 0x10, 0x46, 0xB9, 0x5D, 0xC5, 0x17
#define GUID_DEVINTERFACE_OPM_3                            0x693a2cb1L, 0x8c8d, 0x4ab6, 0x95, 0x55, 0x4b, 0x85, 0xef, 0x2c, 0x7c, 0x6b
#define GUID_DEVINTERFACE_BRIGHTNESS                       0xFDE5BBA4L, 0xB3F9, 0x46FB, 0xBD, 0xAA, 0x07, 0x28, 0xCE, 0x31, 0x00, 0xB4
#define GUID_DEVINTERFACE_BRIGHTNESS_2                     0x148A3C98L, 0x0ECD, 0x465A, 0xB6, 0x34, 0xB0, 0x5F, 0x19, 0x5F, 0x77, 0x39
#define GUID_DEVINTERFACE_MIRACAST_DISPLAY                 0xaf03f190L, 0x22af, 0x48cb, 0x94, 0xbb, 0xb7, 0x8e, 0x76, 0xa2, 0x51, 0x07
#define GUID_DEVINTERFACE_BRIGHTNESS_3                     0x197a4a6eL, 0x0391, 0x4322, 0x96, 0xea, 0xc2, 0x76, 0x0f, 0x88, 0x1d, 0x3a
#define GUID_DEVINTERFACE_HPMI                             0xdedae202L, 0x1d20, 0x4c40, 0xa6, 0xf3, 0x18, 0x97, 0xe3, 0x19, 0xd5, 0x4f
#define GUID_DEVINTERFACE_VIRTUALIZABLE_DEVICE             0xa13a7a93L, 0x11f0, 0x4bd2, 0xa9, 0xf5, 0x6b, 0x5c, 0x5b, 0x88, 0x52, 0x7d
#define GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_RPMB       0x27447c21L, 0xbcc3, 0x4d07, 0xa0, 0x5b, 0xa3, 0x39, 0x5b, 0xb4, 0xee, 0xe7
#define GUID_DEVINTERFACE_EMMC_PARTITION_ACCESS_GPP        0x2e0e2e39L, 0x1f19, 0x4595, 0xa9, 0x06, 0x88, 0x78, 0x82, 0xe7, 0x39, 0x03
#define GUID_DEVINTERFACE_USB_SIDEBAND_AUDIO_HS_HCIBYPASS  0x02baa4b5L, 0x33b5, 0x4d97, 0xae, 0x4f, 0xe8, 0x6d, 0xde, 0x17, 0x53, 0x6f
#define GUID_BUS_VPCI                                      0xc066f39aL, 0xde00, 0x4667, 0x89, 0x41, 0x33, 0x68, 0xed, 0x5d, 0x83, 0xb5
#define GUID_VPCI_INTERFACE_STANDARD                       0x12e65e71L, 0xb651, 0x4067, 0x83, 0x1a, 0x13, 0x83, 0x20, 0x3c, 0xb0, 0xcb
#define GUID_DEVINTERFACE_VPCI                             0x57863182L, 0xc948, 0x4692, 0x97, 0xe3, 0x34, 0xb5, 0x76, 0x62, 0xa3, 0xe0
#define GUID_DEVINTERFACE_WWAN_CONTROLLER                  0x669159fdL, 0xe3c0, 0x45cb, 0xbc, 0x5f, 0x95, 0x99, 0x5b, 0xcd, 0x06, 0xcd
#define GUID_DEVINTERFACE_WDDM3_ON_VB                      0xe922004dL, 0xeb9c, 0x4de1, 0x92, 0x24, 0xa9, 0xce, 0xaa, 0x95, 0x9b, 0xce
#define GUID_DEVINTERFACE_GRAPHICSPOWER                    0xea5c6870L, 0xe93c, 0x4588, 0xbe, 0xf1, 0xfe, 0xc4, 0x2f, 0xc9, 0x42, 0x9a
#define GUID_DEVINTERFACE_GNSS                             0x3336e5e4L, 0x018a, 0x4669, 0x84, 0xc5, 0xbd, 0x05, 0xf3, 0xbd, 0x36, 0x8b
#define GUID_DEVINTERFACE_HID                              0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30
#define GUID_DEVINTERFACE_LAMP                             0x6c11e9e3L, 0x8238, 0x4f0a, 0x0a, 0x19, 0xaa, 0xec, 0x26, 0xca, 0x5e, 0x98
#define GUID_DEVINTERFACE_NFCDTA                           0x7fd3f30bL, 0x5e49, 0x4be1, 0xb3, 0xaa, 0xaf, 0x06, 0x26, 0x0d, 0x23, 0x6a
#define GUID_DEVINTERFACE_NET                              0xcac88484L, 0x7515, 0x4c03, 0x82, 0xe6, 0x71, 0xa8, 0x7a, 0xba, 0xc3, 0x61
#define GUID_DEVINTERFACE_NETUIO                           0x08336f60L, 0x0679, 0x4c6c, 0x85, 0xd2, 0xae, 0x7c, 0xed, 0x65, 0xff, 0xf7
#define GUID_DEVINTERFACE_NFCSE                            0x8dc7c854L, 0xf5e5, 0x4bed, 0x81, 0x5d, 0x0c, 0x85, 0xad, 0x04, 0x77, 0x25
#define GUID_DEVINTERFACE_NFP                              0xFB3842CDL, 0x9E2A, 0x4F83, 0x8F, 0xCC, 0x4B, 0x07, 0x61, 0x13, 0x9A, 0xE9
#define GUID_DEVINTERFACE_KEYBOARD                         0x884b96c3L, 0x56ef, 0x11d1, 0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd
#define GUID_DEVINTERFACE_MODEM                            0x2c7089aaL, 0x2e0e, 0x11d1, 0xb1, 0x14, 0x00, 0xc0, 0x4f, 0xc2, 0xaa, 0xe4
#define GUID_DEVINTERFACE_MOUSE                            0x378de44cL, 0x56ef, 0x11d1, 0xbc, 0x8c, 0x00, 0xa0, 0xc9, 0x14, 0x05, 0xdd
#define GUID_DEVINTERFACE_PARALLEL                         0x97F76EF0L, 0xF883, 0x11D0, 0xAF, 0x1F, 0x00, 0x00, 0xF8, 0x00, 0x84, 0x5C
#define GUID_DEVINTERFACE_PARCLASS                         0x811FC6A5L, 0xF728, 0x11D0, 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1
#define GUID_PARALLEL_DEVICE                               0x97F76EF0L, 0xF883, 0x11D0, 0xAF, 0x1F, 0x00, 0x00, 0xF8, 0x00, 0x84, 0x5C
#define GUID_PARCLASS_DEVICE                               0x811FC6A5L, 0xF728, 0x11D0, 0xA5, 0x37, 0x00, 0x00, 0xF8, 0x75, 0x3E, 0xD1
#define GUID_DEVINTERFACE_DISPLAY_ADAPTER                  0x5b45201dL, 0xf2f2, 0x4f3b, 0x85, 0xbb, 0x30, 0xff, 0x1f, 0x95, 0x35, 0x99
#define GUID_DEVINTERFACE_MONITOR                          0xe6f07b5fL, 0xee97, 0x4a90, 0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7
#define GUID_DEVICE_BATTERY                                0x72631e54L, 0x78A4, 0x11d0, 0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a
#define GUID_DEVICE_APPLICATIONLAUNCH_BUTTON               0x629758eel, 0x986e, 0x4d9e, 0x8e, 0x47, 0xde, 0x27, 0xf8, 0xab, 0x05, 0x4d
#define GUID_DEVICE_SYS_BUTTON                             0x4AFA3D53L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57
#define GUID_DEVICE_LID                                    0x4AFA3D52L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57
#define GUID_DEVICE_THERMAL_ZONE                           0x4AFA3D51L, 0x74A7, 0x11d0, 0xbe, 0x5e, 0x00, 0xA0, 0xC9, 0x06, 0x28, 0x57
#define GUID_DEVICE_POWER_METER                            0xe849804eL, 0xc719, 0x43d8, 0xac, 0x88, 0x96, 0xb8, 0x94, 0xc1, 0x91, 0xe2
#define GUID_DEVICE_FAN                                    0x05ecd13dL, 0x81da, 0x4a2a, 0x8a, 0x4c, 0x52, 0x4f, 0x23, 0xdd, 0x4d, 0xc9
#define GUID_DEVICE_PROCESSOR                              0x97fadb10L, 0x4e33, 0x40ae, 0x35, 0x9c, 0x8b, 0xef, 0x02, 0x9d, 0xbd, 0xd0
#define GUID_DEVICE_MEMORY                                 0x3fd0f03dL, 0x92e0, 0x45fb, 0xb7, 0x5c, 0x5e, 0xd8, 0xff, 0xb0, 0x10, 0x21
#define GUID_DEVICE_ACPI_TIME                              0x97f99bf6L, 0x4497, 0x4f18, 0xbb, 0x22, 0x4b, 0x9f, 0xb2, 0xfb, 0xef, 0x9c
#define GUID_DEVICE_MESSAGE_INDICATOR                      0XCD48A365L, 0xfa94, 0x4ce2, 0xa2, 0x32, 0xa1, 0xb7, 0x64, 0xe5, 0xd8, 0xb4
#define GUID_CLASS_INPUT                                   0x4D1E55B2L, 0xF16F, 0x11CF, 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30
#define GUID_DEVINTERFACE_THERMAL_COOLING                  0xdbe4373dL, 0x3c81, 0x40cb, 0xac, 0xe4, 0xe0, 0xe5, 0xd0, 0x5f, 0x0c, 0x9f
#define GUID_DEVINTERFACE_THERMAL_MANAGER                  0x927ec093L, 0x69a4, 0x4bc0, 0xbd, 0x02, 0x71, 0x16, 0x64, 0x71, 0x44, 0x63
#define GUID_DEVINTERFACE_PWM_CONTROLLER                   0x60824b4cL, 0xeed1, 0x4c9c, 0xb4, 0x9c, 0x1b, 0x96, 0x14, 0x61, 0xa8, 0x19
#define GUID_DEVINTERFACE_USBPRINT                         0x28d78fadL, 0x5a12, 0x11d1, 0xae, 0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2
#define GUID_DEVINTERFACE_IPPUSB_PRINT                     0xf2f40381L, 0xf46d, 0x4e51, 0xbc, 0xe7, 0x62, 0xde, 0x6c, 0xf2, 0xd0, 0x98
#define GUID_DEVINTERFACE_VM_GENCOUNTER                    0x3ff2c92bL, 0x6598, 0x4e60, 0x8e, 0x1c, 0x0c, 0xcf, 0x49, 0x27, 0xe3, 0x19
#define GUID_DEVINTERFACE_BIOMETRIC_READER                 0xe2b5183aL, 0x99ea, 0x4cc3, 0xad, 0x6b, 0x80, 0xca, 0x8d, 0x71, 0x5b, 0x80
#define GUID_DEVINTERFACE_SMARTCARD_READER                 0x50DD5230L, 0xBA8A, 0x11D1, 0xBF, 0x5D, 0x00, 0x00, 0xF8, 0x05, 0xF5, 0x30
#define GUID_DEVINTERFACE_DMR                              0xD0875FB4L, 0x2196, 0x4c7a, 0xA6, 0x3D, 0xE4, 0x16, 0xAD, 0xDD, 0x60, 0xA1
#define GUID_DEVINTERFACE_DMP                              0x25B4E268L, 0x2A05, 0x496e, 0x80, 0x3B, 0x26, 0x68, 0x37, 0xFB, 0xDA, 0x4B
#define GUID_DEVINTERFACE_DMS                              0xC96037AEL, 0xA558, 0x4470, 0xB4, 0x32, 0x11, 0x5A, 0x31, 0xB8, 0x55, 0x53
#define GUID_DEVINTERFACE_ENHANCED_STORAGE_SILO            0x3897f6a4L, 0xfd35, 0x4bc8, 0xa0, 0xb7, 0x5d, 0xbb, 0xa3, 0x6a, 0xda, 0xfa
#define GUID_DEVINTERFACE_WPD                              0x6AC27878L, 0xA6FA, 0x4155, 0xBA, 0x85, 0xF9, 0x8F, 0x49, 0x1D, 0x4F, 0x33
#define GUID_DEVINTERFACE_WPD_PRIVATE                      0xBA0C718FL, 0x4DED, 0x49B7, 0xBD, 0xD3, 0xFA, 0xBE, 0x28, 0x66, 0x12, 0x11
#define GUID_DEVINTERFACE_WPD_SERVICE                      0x9EF44F80L, 0x3D64, 0x4246, 0xA6, 0xAA, 0x20, 0x6F, 0x32, 0x8D, 0x1E, 0xDC
#define GUID_DEVINTERFACE_SENSOR                           0XBA1BB692L, 0X9B7A, 0X4833, 0X9A, 0X1E, 0X52, 0X5E, 0XD1, 0X34, 0XE7, 0XE2
#define GUID_DEVINTERFACE_IMAGE                            0x6bdd1fc6L, 0x810f, 0x11d0, 0xbe, 0xc7, 0x08, 0x00, 0x2b, 0xe2, 0x09, 0x2f
#define GUID_DEVINTERFACE_SIDESHOW                         0x152e5811L, 0xfeb9, 0x4b00, 0x90, 0xf4, 0xd3, 0x29, 0x47, 0xae, 0x16, 0x81
#define GUID_DEVINTERFACE_WIFIDIRECT_DEVICE                0x439b20afL, 0x8955, 0x405b, 0x99, 0xf0, 0xa6, 0x2a, 0xf0, 0xc6, 0x8d, 0x43
#define GUID_AEPSERVICE_WIFIDIRECT_DEVICE                  0xcc29827cL, 0x9caf, 0x4928, 0x99, 0xa9, 0x18, 0xf7, 0xc2, 0x38, 0x13, 0x89
#define GUID_DEVINTERFACE_ASP_INFRA_DEVICE                 0xff823995L, 0x7a72, 0x4c80, 0x87, 0x57, 0xc6, 0x7e, 0xe1, 0x3d, 0x1a, 0x49
#define GUID_DXGKDDI_MITIGABLE_DEVICE_INTERFACE            0x1387f270L, 0x121a, 0x4a4a, 0xb2, 0x5e, 0x3b, 0x15, 0x89, 0x97, 0x6c, 0x61
#define GUID_DXGKDDI_FLEXIOV_DEVICE_INTERFACE              0x7b73a997L, 0x48e8, 0x4cab, 0x9f, 0xab, 0xe0, 0x77, 0x4b, 0x44, 0xf5, 0x99
#define GUID_SRIOV_DEVICE_INTERFACE_STANDARD               0x937ee9b6L, 0x0ed3, 0x411c, 0x98, 0x2b, 0x1f, 0x56, 0x4a, 0xfb, 0xab, 0xd3
#define GUID_MITIGABLE_DEVICE_INTERFACE                    0xadfd9567L, 0x4245, 0x497e, 0x85, 0x72, 0x78, 0xd4, 0x5c, 0x16, 0x22, 0xb8
#define GUID_IO_VOLUME_DEVICE_INTERFACE                    0x53f5630dL, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b
#define GUID_NDIS_LAN_CLASS                                0xad498944L, 0x762f, 0x11d0, 0x8d, 0xcb, 0x00, 0xc0, 0x4f, 0xc3, 0x35, 0x8c
#define GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE              0x4d51e930L, 0x750d, 0x4a36, 0xa9, 0xf7, 0x91, 0xdc, 0x54, 0x0f, 0xcd, 0x30
#define GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE            0xef8ba08fL, 0x148d, 0x4116, 0x83, 0xef, 0xa2, 0x67, 0x9d, 0xfc, 0x3f, 0xa5
#define GUID_BLUETOOTHLE_DEVICE_INTERFACE                  0x781aee18L, 0x7733, 0x4ce4, 0xad, 0xd0, 0x91, 0xf4, 0x1c, 0x67, 0xb5, 0x92
#define GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE       0x6e3bb679L, 0x4372, 0x40c8, 0x9e, 0xaa, 0x45, 0x09, 0xdf, 0x26, 0x0c, 0xd8
#define GUID_BUS_TYPE_TREE                                 0x4E815EE1L, 0x20F8, 0x41EF, 0x8C, 0xFF, 0x3C, 0x28, 0x3F, 0x02, 0xD7, 0x22
#define GUID_TREE_TPM_SERVICE_FDTPM                        0x36deaa79L, 0xc5dd, 0x447c, 0x95, 0xe6, 0xb3, 0x85, 0x95, 0x89, 0x29, 0x1a
#define GUID_TREE_TPM_SERVICE_STPM                         0x1F75CF6DL, 0xF709, 0x4C0C, 0x8B, 0xCB, 0x0C, 0xDC, 0xA1, 0x28, 0x9D, 0xDD
#define GUID_DISPLAY_DEVICE_ARRIVAL                        0x1CA05180L, 0xA699, 0x450A, 0x9A, 0x0C, 0xDE, 0x4F, 0xBE, 0x3D, 0xDD, 0x89
#define GUID_COMPUTE_DEVICE_ARRIVAL                        0x1024e4c9L, 0x47c9, 0x48d3, 0xb4, 0xa8, 0xf9, 0xdf, 0x78, 0x52, 0x3b, 0x53

DEFINE_DEVPROPKEY(DEVPKEY_Gpu_Luid, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2); // DEVPROP_TYPE_UINT64
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_PhysicalAdapterIndex, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 3); // DEVPROP_TYPE_UINT32

#include <SetupAPI.h>
#include <cfgmgr32.h>

#pragma push_macro("DEFINE_GUID")
#undef DEFINE_GUID
#include <pciprop.h>
#include <ntddstor.h>
#pragma pop_macro("DEFINE_GUID")

static CONST PH_STRINGREF RootInstanceId = PH_STRINGREF_INIT(L"HTREE\\ROOT\\0");
static ULONG RootInstanceIdHash = 2387464428; // PhHashStringRefEx(TRUE, PH_STRING_HASH_X65599);
PPH_OBJECT_TYPE PhDeviceTreeType = NULL;
PPH_OBJECT_TYPE PhDeviceItemType = NULL;
PPH_OBJECT_TYPE PhDeviceNotifyType = NULL;
static PPH_OBJECT_TYPE PhpDeviceInfoType = NULL;
static PH_FAST_LOCK PhpDeviceTreeLock = PH_FAST_LOCK_INIT;
static PPH_DEVICE_TREE PhpDeviceTree = NULL;
static HCMNOTIFICATION PhpDeviceNotification = NULL;
static HCMNOTIFICATION PhpDeviceInterfaceNotification = NULL;
static PH_CALLBACK_REGISTRATION ServiceProviderUpdatedRegistration;
static SLIST_HEADER PhDeviceNotifyListHead;
static PH_FREE_LIST PhDeviceNotifyFreeList;

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

BOOLEAN PhpGetDevicePropertyInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG64);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG64),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDeviceInterfacePropertyInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG64);

    result = SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG64),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyInt64(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PLONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG64);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG64),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT64))
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
    PPH_STRING string;

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
        return FALSE;
    }

    if (requiredLength < sizeof(UNICODE_NULL))
    {
        *String = PhReferenceEmptyString();
        return TRUE;
    }

    string = PhCreateStringEx(NULL, requiredLength - sizeof(UNICODE_NULL));

    if (SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)string->Buffer,
        requiredLength,
        &requiredLength,
        0
        ))
    {
        *String = string;
        return TRUE;
    }

    PhClearReference(&string);
    return FALSE;
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
    PPH_STRING string;

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
        return FALSE;
    }

    if (requiredLength < sizeof(UNICODE_NULL))
    {
        *String = PhReferenceEmptyString();
        return TRUE;
    }

    string = PhCreateStringEx(NULL, requiredLength - sizeof(UNICODE_NULL));

    if (SetupDiGetDeviceInterfacePropertyW(
        DeviceInfoSet,
        DeviceInterfaceData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)string->Buffer,
        requiredLength,
        &requiredLength,
        0
        ))
    {
        *String = string;
        return TRUE;
    }

    PhClearReference(&string);
    return FALSE;
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
    PPH_STRING string;

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
        return FALSE;
    }

    if (requiredLength < sizeof(UNICODE_NULL))
    {
        *String = PhReferenceEmptyString();
        return TRUE;
    }

    string = PhCreateStringEx(NULL, requiredLength - sizeof(UNICODE_NULL));

    if (SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)string->Buffer,
        requiredLength,
        &requiredLength,
        Flags
        ))
    {
        *String = string;
        return TRUE;
    }

    PhClearReference(&string);
    return FALSE;
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

VOID PhpDevPropFillInt64Common(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeInt64;

    if (DeviceInfoData->Interface)
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyInt64(
                &DeviceInfoData->InterfaceData.InterfaceClassGuid,
                PropertyKey,
                DICLASSPROP_INTERFACE,
                &Property->Int64
                );
        }
        else
        {
            Property->Valid = PhpGetDeviceInterfacePropertyInt64(
                DeviceInfoSet,
                &DeviceInfoData->InterfaceData,
                PropertyKey,
                &Property->Int64
                );
        }
    }
    else
    {
        if (Flags & DEVPROP_FILL_FLAG_CLASS)
        {
            Property->Valid = PhpGetClassPropertyInt64(
                &DeviceInfoData->DeviceData.ClassGuid,
                PropertyKey,
                DICLASSPROP_INSTALLER,
                &Property->Int64
                );
        }
        else
        {
            Property->Valid = PhpGetDevicePropertyInt64(
                DeviceInfoSet,
                &DeviceInfoData->DeviceData,
                PropertyKey,
                &Property->Int64
                );
        }
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillInt64Common(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatI64D(&format[0], Property->Int64);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
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
    const GUID Guid;
    PH_STRINGREF Symbol;
} PH_WELL_KNOWN_GUID, *PPH_WELL_KNOWN_GUID;
#define PH_DEFINE_WELL_KNOWN_GUID(guid) const PH_WELL_KNOWN_GUID PH_WELL_KNOWN_##guid = { { guid }, PH_STRINGREF_INIT(TEXT(#guid)) }

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
PH_DEFINE_WELL_KNOWN_GUID(GUID_DEVICE_POWER_METER);
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
PH_DEFINE_WELL_KNOWN_GUID(GUID_NDIS_LAN_CLASS);
PH_DEFINE_WELL_KNOWN_GUID(GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BLUETOOTHLE_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_BUS_TYPE_TREE);
PH_DEFINE_WELL_KNOWN_GUID(GUID_TREE_TPM_SERVICE_FDTPM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_TREE_TPM_SERVICE_STPM);
PH_DEFINE_WELL_KNOWN_GUID(GUID_DISPLAY_DEVICE_ARRIVAL);
PH_DEFINE_WELL_KNOWN_GUID(GUID_COMPUTE_DEVICE_ARRIVAL);

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
    &PH_WELL_KNOWN_GUID_DEVICE_POWER_METER,
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
    &PH_WELL_KNOWN_GUID_NDIS_LAN_CLASS,
    &PH_WELL_KNOWN_GUID_NFC_RADIO_MEDIA_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_NFCSE_RADIO_MEDIA_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_BLUETOOTHLE_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE,
    &PH_WELL_KNOWN_GUID_BUS_TYPE_TREE,
    &PH_WELL_KNOWN_GUID_TREE_TPM_SERVICE_FDTPM,
    &PH_WELL_KNOWN_GUID_TREE_TPM_SERVICE_STPM,
    &PH_WELL_KNOWN_GUID_DISPLAY_DEVICE_ARRIVAL,
    &PH_WELL_KNOWN_GUID_COMPUTE_DEVICE_ARRIVAL,
};

static int __cdecl PhpWellKnownGuidSortFunction(
    const void* Left,
    const void* Right
    )
{
    const PH_WELL_KNOWN_GUID* lhsItem = *(PPH_WELL_KNOWN_GUID*)Left;
    const PH_WELL_KNOWN_GUID* rhsItem = *(PPH_WELL_KNOWN_GUID*)Right;

    return IsEqualGUID(&lhsItem->Guid, &rhsItem->Guid);
}

static int __cdecl PhpWellKnownGuidSearchFunction(
    const GUID* Guid,
    const void* Item
    )
{
    const PH_WELL_KNOWN_GUID* item = *(PPH_WELL_KNOWN_GUID*)Item;

    return memcmp(Guid, &item->Guid, sizeof(GUID));
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

PPH_STRING PhpDevPropPciDeviceTypeToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, DevProp_PciDevice_DeviceType_PciConventional))
        PhAppendStringBuilder2(&stringBuilder, L"PciConventional, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_DeviceType_PciX))
        PhAppendStringBuilder2(&stringBuilder, L"PciX, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_DeviceType_PciExpressEndpoint))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressEndpoint, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_DeviceType_PciExpressLegacyEndpoint))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressLegacyEndpoint, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_DeviceType_PciExpressRootComplexIntegratedEndpoint))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressRootComplexIntegratedEndpoint, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_DeviceType_PciExpressTreatedAsPci))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressTreatedAsPci, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciConventional))
        PhAppendStringBuilder2(&stringBuilder, L"PciConventional, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciX))
        PhAppendStringBuilder2(&stringBuilder, L"PciX, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciExpressRootPort))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressRootPort, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciExpressUpstreamSwitchPort))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressUpstreamSwitchPort, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciExpressDownstreamSwitchPort))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressDownstreamSwitchPort, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciExpressToPciXBridge))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressToPciXBridge, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciXToExpressBridge))
        PhAppendStringBuilder2(&stringBuilder, L"PciXToExpressBridge, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciExpressTreatedAsPci))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressTreatedAsPci, ");
    else if (BooleanFlagOn(Flags, DevProp_PciDevice_BridgeType_PciExpressEventCollector))
        PhAppendStringBuilder2(&stringBuilder, L"PciExpressEventCollector, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    if (Flags)
    {
        PhPrintPointer(pointer, UlongToPtr(Flags));
        PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillPciDeviceType(
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
        Property->AsString = PhpDevPropPciDeviceTypeToString(Property->UInt32);
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

PPH_STRING PhpDevPropPciDeviceRequestSizeToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, DevProp_PciExpressDevice_PayloadOrRequestSize_128Bytes))
        PhAppendStringBuilder2(&stringBuilder, L"128B, ");
    else if (BooleanFlagOn(Flags, DevProp_PciExpressDevice_PayloadOrRequestSize_256Bytes))
        PhAppendStringBuilder2(&stringBuilder, L"256B, ");
    else if (BooleanFlagOn(Flags, DevProp_PciExpressDevice_PayloadOrRequestSize_512Bytes))
        PhAppendStringBuilder2(&stringBuilder, L"512B, ");
    else if (BooleanFlagOn(Flags, DevProp_PciExpressDevice_PayloadOrRequestSize_1024Bytes))
        PhAppendStringBuilder2(&stringBuilder, L"1024B, ");
    else if (BooleanFlagOn(Flags, DevProp_PciExpressDevice_PayloadOrRequestSize_2048Bytes))
        PhAppendStringBuilder2(&stringBuilder, L"2048B, ");
    else if (BooleanFlagOn(Flags, DevProp_PciExpressDevice_PayloadOrRequestSize_4096Bytes))
        PhAppendStringBuilder2(&stringBuilder, L"4096B, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillPciDeviceRequestSize(
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
        Property->AsString = PhpDevPropPciDeviceRequestSizeToString(Property->UInt32);
    }
}

PPH_STRING PhpDevPropPciDeviceSriovSupportToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, DevProp_PciDevice_SriovSupport_Ok))
        PhAppendStringBuilder2(&stringBuilder, L"Ok, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_SriovSupport_MissingAcs))
        PhAppendStringBuilder2(&stringBuilder, L"MissingAcs, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_SriovSupport_MissingPfDriver))
        PhAppendStringBuilder2(&stringBuilder, L"MissingPfDriver, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_SriovSupport_NoBusResource))
        PhAppendStringBuilder2(&stringBuilder, L"NoBusResource, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_SriovSupport_DidntGetVfBarSpace))
        PhAppendStringBuilder2(&stringBuilder, L"DidntGetVfBarSpace, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillPciDeviceSriovSupport(
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
        Property->AsString = PhpDevPropPciDeviceSriovSupportToString(Property->UInt32);
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
        {
            static CONST PH_STRINGREF string = PH_STRINGREF_INIT(L"true");
            Property->AsString = PhCreateString2(&string);
        }
        else
        {
            static CONST PH_STRINGREF string = PH_STRINGREF_INIT(L"false");
            Property->AsString = PhCreateString2(&string);
        }
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

VOID PhpDevPropFillBinaryCommon(
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
    PhpDevPropFillBinaryCommon(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        Property->AsString = PhBufferToHexString(Property->Binary.Buffer, Property->Binary.Size);
    }
}

static const PH_STRINGREF UnknownString = PH_STRINGREF_INIT(L"Unk");

PCPH_STRINGREF PhpDevPowerStateString(
    _In_ DEVICE_POWER_STATE PowerState
    )
{
    static const PH_STRINGREF states[] =
    {
        PH_STRINGREF_INIT(L"Uns"), // PowerDeviceUnspecified
        PH_STRINGREF_INIT(L"D0"),  // PowerDeviceD0
        PH_STRINGREF_INIT(L"D1"),  // PowerDeviceD1
        PH_STRINGREF_INIT(L"D2"),  // PowerDeviceD2
        PH_STRINGREF_INIT(L"D3"),  // PowerDeviceD3
        PH_STRINGREF_INIT(L"D4"),  // PowerDeviceD4
        PH_STRINGREF_INIT(L"Max"), // PowerDeviceMaximum
    };

    if (PowerState >= 0 && PowerState < RTL_NUMBER_OF(states))
        return &states[PowerState];

    return &UnknownString;
}

PCPH_STRINGREF PhpDevSysPowerStateString(
    _In_ SYSTEM_POWER_STATE PowerState
    )
{
    static const PH_STRINGREF states[] =
    {
        PH_STRINGREF_INIT(L"Uns"), // PowerSystemUnspecified
        PH_STRINGREF_INIT(L"S0"),  // PowerSystemWorking
        PH_STRINGREF_INIT(L"S1"),  // PowerSystemSleep1
        PH_STRINGREF_INIT(L"S2"),  // PowerSystemSleep2
        PH_STRINGREF_INIT(L"S3"),  // PowerSystemSleep3
        PH_STRINGREF_INIT(L"S4"),  // PowerSystemHybernate
        PH_STRINGREF_INIT(L"S5"),  // PowerSystemShutdown
        PH_STRINGREF_INIT(L"Max"), // PowerSystemMaximum
    };

    if (PowerState >= 0 && PowerState < RTL_NUMBER_OF(states))
        return &states[PowerState];

    return &UnknownString;
}

PPH_STRING PhpDevSysPowerPowerDataString(
    _In_ PCM_POWER_DATA PowerData
    )
{
    static const PH_ACCESS_ENTRY pdCap[] =
    {
        { L"PDCAP_D0_SUPPORTED", PDCAP_D0_SUPPORTED, FALSE, FALSE, L"D0" },
        { L"PDCAP_D1_SUPPORTED", PDCAP_D1_SUPPORTED, FALSE, FALSE, L"D1" },
        { L"PDCAP_D2_SUPPORTED", PDCAP_D2_SUPPORTED, FALSE, FALSE, L"D2" },
        { L"PDCAP_D3_SUPPORTED", PDCAP_D3_SUPPORTED, FALSE, FALSE, L"D3" },
        { L"PDCAP_WAKE_FROM_D0_SUPPORTED", PDCAP_WAKE_FROM_D0_SUPPORTED, FALSE, FALSE, L"Wake from D0" },
        { L"PDCAP_WAKE_FROM_D1_SUPPORTED", PDCAP_WAKE_FROM_D1_SUPPORTED, FALSE, FALSE, L"Wake from D1" },
        { L"PDCAP_WAKE_FROM_D2_SUPPORTED", PDCAP_WAKE_FROM_D2_SUPPORTED, FALSE, FALSE, L"Wake from D2" },
        { L"PDCAP_WAKE_FROM_D3_SUPPORTED", PDCAP_WAKE_FROM_D3_SUPPORTED, FALSE, FALSE, L"Wake from D3" },
        { L"PDCAP_WARM_EJECT_SUPPORTED", PDCAP_WARM_EJECT_SUPPORTED, FALSE, FALSE, L"Warm eject" },
    };

    PH_FORMAT format[29];
    ULONG count = 0;
    PPH_STRING capabilities;
    PPH_STRING string;

    PhInitFormatSR(&format[count++], *PhpDevPowerStateString(PowerData->PD_MostRecentPowerState));
    if (PowerData->PD_D1Latency)
    {
        PhInitFormatS(&format[count++], L", ");
        PhInitFormatU(&format[count++], PowerData->PD_D1Latency);
        PhInitFormatS(&format[count++], L" D1 latency");
    }
    if (PowerData->PD_D2Latency)
    {
        PhInitFormatS(&format[count++], L", ");
        PhInitFormatU(&format[count++], PowerData->PD_D2Latency);
        PhInitFormatS(&format[count++], L" D2 latency");
    }
    if (PowerData->PD_D2Latency)
    {
        PhInitFormatS(&format[count++], L", ");
        PhInitFormatU(&format[count++], PowerData->PD_D3Latency);
        PhInitFormatS(&format[count++], L" D3 latency");
    }
    PhInitFormatS(&format[count++], L", S0->");
    PhInitFormatSR(&format[count++], *PhpDevPowerStateString(PowerData->PD_PowerStateMapping[PowerSystemWorking]));
    PhInitFormatS(&format[count++], L", S1->");
    PhInitFormatSR(&format[count++], *PhpDevPowerStateString(PowerData->PD_PowerStateMapping[PowerSystemSleeping1]));
    PhInitFormatS(&format[count++], L", S2->");
    PhInitFormatSR(&format[count++], *PhpDevPowerStateString(PowerData->PD_PowerStateMapping[PowerSystemSleeping2]));
    PhInitFormatS(&format[count++], L", S3->");
    PhInitFormatSR(&format[count++], *PhpDevPowerStateString(PowerData->PD_PowerStateMapping[PowerSystemSleeping3]));
    PhInitFormatS(&format[count++], L", S4->");
    PhInitFormatSR(&format[count++], *PhpDevPowerStateString(PowerData->PD_PowerStateMapping[PowerSystemHibernate]));
    PhInitFormatS(&format[count++], L", S5->");
    PhInitFormatSR(&format[count++], *PhpDevPowerStateString(PowerData->PD_PowerStateMapping[PowerSystemShutdown]));
    capabilities = PhGetAccessString(PowerData->PD_Capabilities, (PVOID)pdCap, RTL_NUMBER_OF(pdCap));
    PhInitFormatS(&format[count++], L", (");
    PhInitFormatSR(&format[count++], capabilities->sr);
    PhInitFormatS(&format[count++], L" (0x");
    PhInitFormatX(&format[count++], PowerData->PD_Capabilities);
    PhInitFormatS(&format[count++], L"))");
    if (PowerData->PD_DeepestSystemWake != PowerSystemUnspecified)
    {
        PhInitFormatS(&format[count++], L", Deepest wake ");
        PhInitFormatSR(&format[count++], *PhpDevSysPowerStateString(PowerData->PD_DeepestSystemWake));
    }

    string = PhFormat(format, count, 20);

    PhClearReference(&capabilities);

    return string;
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillPowerData(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillBinaryCommon(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (Property->Valid)
    {
        if (Property->Binary.Size >= sizeof(CM_POWER_DATA))
            Property->AsString = PhpDevSysPowerPowerDataString((PCM_POWER_DATA)Property->Binary.Buffer);
        else
            Property->AsString = PhBufferToHexString(Property->Binary.Buffer, Property->Binary.Size);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt32Flags(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PPH_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
#define PH_DEVICE_FLAG(x, n) { TEXT(#x), x, FALSE, FALSE, n }

    static const PH_ACCESS_ENTRY deviceCapabilities[] =
    {
        PH_DEVICE_FLAG(CM_DEVCAP_LOCKSUPPORTED, L"Lock supported"),
        PH_DEVICE_FLAG(CM_DEVCAP_EJECTSUPPORTED, L"Eject supported"),
        PH_DEVICE_FLAG(CM_DEVCAP_REMOVABLE, L"Removable"),
        PH_DEVICE_FLAG(CM_DEVCAP_DOCKDEVICE, L"Dock device"),
        PH_DEVICE_FLAG(CM_DEVCAP_UNIQUEID, L"Unique ID"),
        PH_DEVICE_FLAG(CM_DEVCAP_SILENTINSTALL, L"Silent install"),
        PH_DEVICE_FLAG(CM_DEVCAP_RAWDEVICEOK, L"Raw device ok"),
        PH_DEVICE_FLAG(CM_DEVCAP_SURPRISEREMOVALOK, L"Surprise removal ok"),
        PH_DEVICE_FLAG(CM_DEVCAP_HARDWAREDISABLED, L"Hardware disabled"),
        PH_DEVICE_FLAG(CM_DEVCAP_NONDYNAMIC, L"No dynamic"),
        PH_DEVICE_FLAG(CM_DEVCAP_SECUREDEVICE, L"Secure device"),
    };

    static const PH_ACCESS_ENTRY devNodeStatus[] =
    {
        PH_DEVICE_FLAG(DN_ROOT_ENUMERATED, L"Enumerated"),
        PH_DEVICE_FLAG(DN_DRIVER_LOADED, L"Driver loaded"),
        PH_DEVICE_FLAG(DN_ENUM_LOADED, L"Enumerator loaded"),
        PH_DEVICE_FLAG(DN_STARTED, L"Started"),
        PH_DEVICE_FLAG(DN_MANUAL, L"Manually installed"),
        PH_DEVICE_FLAG(DN_NEED_TO_ENUM, L"Needs enumerated"),
        PH_DEVICE_FLAG(DN_DRIVER_BLOCKED, L"Driver blocked"),
        PH_DEVICE_FLAG(DN_HARDWARE_ENUM, L"Hardware enum"),
        PH_DEVICE_FLAG(DN_NEED_RESTART, L"Needs reboot"),
        PH_DEVICE_FLAG(DN_CHILD_WITH_INVALID_ID, L"Child with invalid ID"),
        PH_DEVICE_FLAG(DN_HAS_PROBLEM, L"Has problem"),
        PH_DEVICE_FLAG(DN_FILTERED, L"Filtered"),
        PH_DEVICE_FLAG(DN_LEGACY_DRIVER, L"Legacy driver"),
        PH_DEVICE_FLAG(DN_DISABLEABLE, L"Disabled"),
        PH_DEVICE_FLAG(DN_REMOVABLE, L"Removable"),
        PH_DEVICE_FLAG(DN_PRIVATE_PROBLEM, L"Has problem"),
        PH_DEVICE_FLAG(DN_QUERY_REMOVE_PENDING, L"Query remove pending"),
        PH_DEVICE_FLAG(DN_QUERY_REMOVE_ACTIVE, L"Query remove active"),
        PH_DEVICE_FLAG(DN_WILL_BE_REMOVED, L"Being removed"),
        PH_DEVICE_FLAG(DN_NOT_FIRST_TIMEE, L"Received config"),
        PH_DEVICE_FLAG(DN_STOP_FREE_RES, L"To be freed"),
        PH_DEVICE_FLAG(DN_REBAL_CANDIDATE, L"Rebalance candidate"),
        PH_DEVICE_FLAG(DN_BAD_PARTIAL, L"Bad partial"),
        PH_DEVICE_FLAG(DN_NT_ENUMERATOR, L"NT enumerator"),
        PH_DEVICE_FLAG(DN_NT_DRIVER, L"NT driver"),
        PH_DEVICE_FLAG(DN_DEVICE_DISCONNECTED, L"Disconnected"),
        PH_DEVICE_FLAG(DN_ARM_WAKEUP, L"Wakeup device"),
        PH_DEVICE_FLAG(DN_APM_ENUMERATOR, L"Advanced power enumerator"),
        PH_DEVICE_FLAG(DN_APM_DRIVER, L"Advanced power aware"),
        PH_DEVICE_FLAG(DN_SILENT_INSTALL, L"Silent install"),
        PH_DEVICE_FLAG(DN_NO_SHOW_IN_DM, L"Hidden from device manager"),
        PH_DEVICE_FLAG(DN_BOOT_LOG_PROB, L"Boot log problem"),
    };

    static const PH_ACCESS_ENTRY characteristics[] =
    {
        PH_DEVICE_FLAG(FILE_REMOVABLE_MEDIA, L"Removable media"),
        PH_DEVICE_FLAG(FILE_READ_ONLY_DEVICE, L"Read only device"),
        PH_DEVICE_FLAG(FILE_FLOPPY_DISKETTE, L"Floppy disk"),
        PH_DEVICE_FLAG(FILE_WRITE_ONCE_MEDIA, L"Write once media"),
        PH_DEVICE_FLAG(FILE_REMOTE_DEVICE, L"Remote device"),
        PH_DEVICE_FLAG(FILE_DEVICE_IS_MOUNTED, L"Mounted"),
        PH_DEVICE_FLAG(FILE_VIRTUAL_VOLUME, L"Virtual volume"),
        PH_DEVICE_FLAG(FILE_AUTOGENERATED_DEVICE_NAME, L"Autogenerated device name"),
        PH_DEVICE_FLAG(FILE_DEVICE_SECURE_OPEN, L"Secure open"),
        PH_DEVICE_FLAG(FILE_CHARACTERISTIC_PNP_DEVICE, L"PnP device"),
        PH_DEVICE_FLAG(FILE_CHARACTERISTIC_TS_DEVICE, L"Terminal services device"),
        PH_DEVICE_FLAG(FILE_CHARACTERISTIC_WEBDAV_DEVICE, L"WebDAV device"),
        PH_DEVICE_FLAG(FILE_CHARACTERISTIC_CSV, L"Cluster shared volume"),
        PH_DEVICE_FLAG(FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL, L"Allow app container traversal"),
        PH_DEVICE_FLAG(FILE_PORTABLE_DEVICE, L"Portable device"),
        PH_DEVICE_FLAG(FILE_REMOTE_DEVICE_VSMB, L"Virtual SCSI device"),
        PH_DEVICE_FLAG(FILE_DEVICE_REQUIRE_SECURITY_CHECK, L"Require security check"),
    };

    static const PH_ACCESS_ENTRY nameAttributes[] =
    {
        PH_DEVICE_FLAG(CM_NAME_ATTRIBUTE_NAME_RETRIEVED_FROM_DEVICE, L"Retrieved from device"),
        PH_DEVICE_FLAG(CM_NAME_ATTRIBUTE_USER_ASSIGNED_NAME, L"User assigned name"),
    };

    PPH_ACCESS_ENTRY entries = NULL;
    ULONG count = 0;

    PhpDevPropFillUInt32Common(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );

    if (PropertyKey == &DEVPKEY_Device_DevNodeStatus)
    {
        entries = (PPH_ACCESS_ENTRY)devNodeStatus;
        count = RTL_NUMBER_OF(devNodeStatus);
    }
    else if (PropertyKey == &DEVPKEY_Device_Capabilities)
    {
        entries = (PPH_ACCESS_ENTRY)deviceCapabilities;
        count = RTL_NUMBER_OF(deviceCapabilities);
    }
    else if (PropertyKey == &DEVPKEY_Device_Characteristics ||
             PropertyKey == &DEVPKEY_DeviceClass_Characteristics)
    {
        entries = (PPH_ACCESS_ENTRY)characteristics;
        count = RTL_NUMBER_OF(characteristics);
    }
    else if (PropertyKey == &DEVPKEY_Device_FriendlyNameAttributes)
    {
        entries = (PPH_ACCESS_ENTRY)nameAttributes;
        count = RTL_NUMBER_OF(nameAttributes);
    }

    if (entries)
    {
        if (Property->UInt32)
        {
            PH_FORMAT format[4];
            PPH_STRING string;

            string = PhGetAccessString(Property->UInt32, entries, count);
            PhInitFormatSR(&format[0], string->sr);
            PhInitFormatS(&format[1], L" (0x");
            PhInitFormatX(&format[2], Property->UInt32);
            PhInitFormatS(&format[3], L")");

            Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);

            PhDereferenceObject(string);
        }
        else
        {
            Property->AsString = PhReferenceEmptyString();
        }
    }
    else
    {
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatIX(&format[1], Property->UInt32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);
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
    { PhDevicePropertyDevNodeStatus, &DEVPKEY_Device_DevNodeStatus, PhpDevPropFillUInt32Flags, 0 },
    { PhDevicePropertyDevCapabilities, &DEVPKEY_Device_Capabilities, PhpDevPropFillUInt32Flags, 0 },
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
    { PhDevicePropertyCharacteristics, &DEVPKEY_Device_Characteristics, PhpDevPropFillUInt32Flags, 0 },
    { PhDevicePropertyAddress, &DEVPKEY_Device_Address, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyPowerData, &DEVPKEY_Device_PowerData, PhpDevPropFillPowerData, 0 },
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
    { PhDevicePropertyFriendlyNameAttributes, &DEVPKEY_Device_FriendlyNameAttributes, PhpDevPropFillUInt32Flags, 0 },
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
    { PhDevicePropertyClassCharacteristics, &DEVPKEY_DeviceClass_Characteristics, PhpDevPropFillUInt32Flags, DEVPROP_FILL_FLAG_CLASS },
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

    { PhDevicePropertyPciDeviceType, &DEVPKEY_PciDevice_DeviceType, PhpDevPropFillPciDeviceType, 0 },
    { PhDevicePropertyPciCurrentSpeedAndMode, &DEVPKEY_PciDevice_CurrentSpeedAndMode, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciBaseClass, &DEVPKEY_PciDevice_BaseClass, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciSubClass, &DEVPKEY_PciDevice_SubClass, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciProgIf, &DEVPKEY_PciDevice_ProgIf, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciCurrentPayloadSize, &DEVPKEY_PciDevice_CurrentPayloadSize, PhpDevPropFillPciDeviceRequestSize, 0 },
    { PhDevicePropertyPciMaxPayloadSize, &DEVPKEY_PciDevice_MaxPayloadSize, PhpDevPropFillPciDeviceRequestSize, 0 },
    { PhDevicePropertyPciMaxReadRequestSize, &DEVPKEY_PciDevice_MaxReadRequestSize, PhpDevPropFillPciDeviceRequestSize, 0 },
    { PhDevicePropertyPciCurrentLinkSpeed, &DEVPKEY_PciDevice_CurrentLinkSpeed, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciCurrentLinkWidth, &DEVPKEY_PciDevice_CurrentLinkWidth, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciMaxLinkSpeed, &DEVPKEY_PciDevice_MaxLinkSpeed, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciMaxLinkWidth, &DEVPKEY_PciDevice_MaxLinkWidth, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciExpressSpecVersion, &DEVPKEY_PciDevice_ExpressSpecVersion, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciInterruptSupport, &DEVPKEY_PciDevice_InterruptSupport, PhpDevPropFillPciDeviceInterruptSupport, 0 },
    { PhDevicePropertyPciInterruptMessageMaximum, &DEVPKEY_PciDevice_InterruptMessageMaximum, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciBarTypes, &DEVPKEY_PciDevice_BarTypes, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciSriovSupport, &DEVPKEY_PciDevice_SriovSupport, PhpDevPropFillPciDeviceSriovSupport, 0 },
    { PhDevicePropertyPciLabel_Id, &DEVPKEY_PciDevice_Label_Id, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciLabel_String, &DEVPKEY_PciDevice_Label_String, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPciSerialNumber, &DEVPKEY_PciDevice_SerialNumber, PhpDevPropFillUInt32, 0 },

    { PhDevicePropertyPciExpressCapabilityControl, &DEVPKEY_PciRootBus_PCIExpressCapabilityControl, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyPciNativeExpressControl, &DEVPKEY_PciRootBus_NativePciExpressControl, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyPciSystemMsiSupport, &DEVPKEY_PciRootBus_SystemMsiSupport, PhpDevPropFillBoolean, 0 },

    { PhDevicePropertyStoragePortable, &DEVPKEY_Storage_Portable, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageRemovableMedia, &DEVPKEY_Storage_Removable_Media, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageSystemCritical, &DEVPKEY_Storage_System_Critical, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageDiskNumber, &DEVPKEY_Storage_Disk_Number, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyStoragePartitionNumber, &DEVPKEY_Storage_Partition_Number, PhpDevPropFillUInt32, 0 },

    { PhDevicePropertyGpuLuid, &DEVPKEY_Gpu_Luid, PhpDevPropFillUInt64, 0 },
    { PhDevicePropertyGpuPhysicalAdapterIndex, &DEVPKEY_Gpu_PhysicalAdapterIndex, PhpDevPropFillUInt32, 0 },
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

    return uintcmp(lhsItem->InstanceIdHash, rhsItem->InstanceIdHash);
}

static int __cdecl PhpDeviceItemSearchFunction(
    const void* Hash,
    const void* Item
    )
{
    PPH_DEVICE_ITEM item;

    item = *(PPH_DEVICE_ITEM*)Item;

    return uintcmp(PtrToUlong(Hash), item->InstanceIdHash);
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhLookupDeviceItemByHash(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ ULONG InstanceIdHash
    )
{
    PPH_DEVICE_ITEM* deviceItem;

    deviceItem = bsearch(
        UlongToPtr(InstanceIdHash),
        Tree->DeviceList->Items,
        Tree->DeviceList->Count,
        sizeof(PVOID),
        PhpDeviceItemSearchFunction
        );

    return deviceItem ? *deviceItem : NULL;
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhLookupDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_STRINGREF InstanceId
    )
{
    return PhLookupDeviceItemByHash(Tree, PhpGenerateInstanceIdHash(InstanceId));
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhReferenceDeviceItemByHash(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ ULONG InstanceIdHash
    )
{
    PPH_DEVICE_ITEM deviceItem;

    PhSetReference(&deviceItem, PhLookupDeviceItemByHash(Tree, InstanceIdHash));

    return deviceItem;
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
    PPH_DEVICE_TREE tree;
    PPH_DEVICE_ITEM deviceItem;

    tree = PhReferenceDeviceTree();
    PhSetReference(&deviceItem, PhLookupDeviceItem(tree, InstanceId));
    PhClearReference(&tree);

    return deviceItem;
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID PhpDeviceItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVICE_ITEM item = Object;

    for (ULONG i = 0; i < ARRAYSIZE(item->Properties); i++)
    {
        PPH_DEVICE_PROPERTY prop;

        prop = &item->Properties[i];

        if (prop->Valid)
        {
            if (prop->Type == PhDevicePropertyTypeString)
            {
                PhClearReference(&prop->String);
            }
            else if (prop->Type == PhDevicePropertyTypeStringList)
            {
                for (ULONG j = 0; j < prop->StringList->Count; j++)
                {
                    PhClearReference(&prop->StringList->Items[j]);
                }

                PhClearReference(&prop->StringList);
            }
            else if (prop->Type == PhDevicePropertyTypeBinary)
            {
#pragma warning(suppress : 6001) // buffer is valid and initialized
                PhFree(prop->Binary.Buffer);
            }
        }

        PhClearReference(&prop->AsString);
    }

    PhClearReference(&item->InstanceId);
    PhClearReference(&item->ParentInstanceId);
    PhClearReference(&item->DeviceInfo);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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

        PhClearReference(&item);
    }

    for (ULONG i = 0; i < tree->DeviceInterfaceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;

        item = tree->DeviceInterfaceList->Items[i];

        PhClearReference(&item);
    }

    PhClearReference(&tree->DeviceList);
    PhClearReference(&tree->DeviceInterfaceList);
    PhClearReference(&tree->DeviceInfo);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
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
        PhClearReference(&notify->DeviceInstance.InstanceId);
    }
}

PPH_DEVICE_ITEM NTAPI PhpAddDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    PPH_DEVICE_ITEM item;

    item = PhCreateObjectZero(sizeof(PH_DEVICE_ITEM), PhDeviceItemType);

    item->Tree = Tree;
    item->DeviceInfo = PhReferenceObject(Tree->DeviceInfo);
    RtlCopyMemory(&item->DeviceInfoData.DeviceData, DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    RtlCopyMemory(&item->ClassGuid, &DeviceInfoData->ClassGuid, sizeof(GUID));

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
        //if (PhEqualStringRef(&item->InstanceId->sr, &RootInstanceId, TRUE))
        if (item->InstanceIdHash == RootInstanceIdHash)
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

    item->Tree = Tree;
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

    // link the interface as a "child" of the device
    DeviceItem->InterfaceCount++;
    if (DeviceItem->Child)
    {
        PPH_DEVICE_ITEM sibling = DeviceItem->Child;

        while (sibling->Sibling)
            sibling = sibling->Sibling;

        sibling->Sibling = item;
    }
    else
    {
        DeviceItem->Child = item;
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

    PhAddItemList(Tree->DeviceInterfaceList, item);

    return item;
}

VOID PhpGetInterfaceClassList(
    _Out_ PGUID* InterfaceClassList,
    _Out_ PSIZE_T InterfaceClassListCount
    )
{
    const DEV_OBJECT* objects = NULL;
    ULONG objectCount;
    PH_ARRAY objectArray;

    if (HR_FAILED(PhDevGetObjects(
        DevObjectTypeDeviceInterfaceClass,
        DevQueryFlagNone,
        0,
        NULL,
        0,
        NULL,
        &objectCount,
        &objects
        )) || !objects)
    {
        *InterfaceClassList = NULL;
        *InterfaceClassListCount = 0;
        return;
    }

    PhInitializeArray(&objectArray, sizeof(GUID), objectCount);

    for (ULONG i = 0; i < objectCount; i++)
    {
        GUID interfaceClassGuid;
        PH_STRINGREF interfaceClassGuidString;

        PhInitializeStringRefLongHint(&interfaceClassGuidString, objects[i].pszObjectId);

        if (!NT_SUCCESS(PhStringToGuid(&interfaceClassGuidString, &interfaceClassGuid)))
            continue;

        PhAddItemArray(&objectArray, &interfaceClassGuid);
    }

    *InterfaceClassList = PhFinalArrayItems(&objectArray);
    *InterfaceClassListCount = PhFinalArrayCount(&objectArray);

    PhDevFreeObjects(objectCount, objects);
}

VOID PhpAssociateDeviceInterfaces(
    _Inout_ PPH_DEVICE_TREE Tree,
    _In_ PPH_DEVICE_ITEM DeviceItem,
    _In_ PGUID InterfaceClassList,
    _In_ SIZE_T InterfaceClassListCount
    )
{
    assert(!DeviceItem->DeviceInfoData.Interface);

    for (SIZE_T i = 0; i < InterfaceClassListCount; i++)
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

#ifdef DEBUG
VOID PhpCheckDeviceTree(
    _In_ PPH_DEVICE_TREE Tree
    )
{
    static ULONG runawayLimit = 5000;
    ULONG count;

    assert(!Tree->Root->Sibling);
    assert(!Tree->Root->Parent);

    for (ULONG i = 0; i < Tree->DeviceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;
        PPH_DEVICE_ITEM other;

        item = Tree->DeviceList->Items[i];

        // ensure children terminate
        other = item->Child;
        count = 0;
        while (other)
        {
            other = other->Child;
            assert(count < runawayLimit);
        }

        // ensure siblings terminate
        other = item->Sibling;
        count = 0;
        while (other)
        {
            other = other->Sibling;
            assert(count < runawayLimit);
        }

        // ensure parents terminate
        other = item->Parent;
        count = 0;
        while (other)
        {
            other = other->Parent;
            assert(count < runawayLimit);
        }
    }

    for (ULONG i = 0; i < Tree->DeviceInterfaceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;
        PPH_DEVICE_ITEM other;

        item = Tree->DeviceInterfaceList->Items[i];

        // ensure children terminate
        other = item->Child;
        count = 0;
        while (other)
        {
            other = other->Child;
            assert(count < runawayLimit);
        }

        // ensure siblings terminate
        other = item->Sibling;
        count = 0;
        while (other)
        {
            other = other->Sibling;
            assert(count < runawayLimit);
        }

        // ensure parents terminate
        other = item->Parent;
        count = 0;
        while (other)
        {
            other = other->Parent;
            assert(count < runawayLimit);
        }
    }
}
#endif

PPH_DEVICE_TREE PhpCreateDeviceTree(
    VOID
    )
{
    PPH_DEVICE_TREE tree;
    PPH_DEVICE_ITEM root;
    PGUID interfaceClassList;
    SIZE_T interfaceClassListCount;

    tree = PhCreateObjectZero(sizeof(PH_DEVICE_TREE), PhDeviceTreeType);
    tree->DeviceList = PhCreateList(250);
    tree->DeviceInterfaceList = PhCreateList(1024);
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
        PhpAssociateDeviceInterfaces(tree, item, interfaceClassList, interfaceClassListCount);

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
            item->Parent->ChildrenCount++;

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

        if (!found)
        {
            assert(!root);
            root = item;
        }
    }

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

#ifdef DEBUG
    PhpCheckDeviceTree(tree);
#endif

    return tree;
}

PPH_DEVICE_TREE PhpPublishDeviceTree(
    VOID
    )
{
    PPH_DEVICE_TREE newTree;
    PPH_DEVICE_TREE oldTree;

    newTree = PhpCreateDeviceTree();
    PhReferenceObject(newTree);

    PhAcquireFastLockExclusive(&PhpDeviceTreeLock);
    oldTree = PhpDeviceTree;
    PhpDeviceTree = newTree;
    PhReleaseFastLockExclusive(&PhpDeviceTreeLock);

    PhClearReference(&oldTree);

    return newTree;
}

VOID PhpDeviceNotify(
    _In_ PSLIST_ENTRY List
    )
{
    PPH_DEVICE_TREE deviceTree;
    PPH_DEVICE_NOTIFY entry;

    // We process device notifications in blocks so that bursts of device changes
    // don't each trigger a new tree each time. THe internal is determined by the
    // interval update and the Service Provider Updated Event. (500ms/1000ms)

    if (deviceTree = PhpPublishDeviceTree())
    {
        while (List)
        {
            entry = CONTAINING_RECORD(List, PH_DEVICE_NOTIFY, ListEntry);
            List = List->Next;

            // Restore TypeIndex and Flags.
            PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent), entry);

            PhFreeToFreeList(&PhDeviceNotifyFreeList, entry);
        }
    }

    PhClearReference(&deviceTree);
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
    PPH_DEVICE_NOTIFY entry;

    switch (Action)
    {
    case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
        {
            entry = PhAllocateFromFreeList(&PhDeviceNotifyFreeList);
            memset(entry, 0, sizeof(PH_DEVICE_NOTIFY));
            entry->Action = PhDeviceNotifyInterfaceArrival;
            RtlCopyMemory(&entry->DeviceInterface.ClassGuid, &EventData->u.DeviceInterface.ClassGuid, sizeof(GUID));

            InterlockedPushEntrySList(&PhDeviceNotifyListHead, &entry->ListEntry);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
        {
            entry = PhAllocateFromFreeList(&PhDeviceNotifyFreeList);
            memset(entry, 0, sizeof(PH_DEVICE_NOTIFY));
            entry->Action = PhDeviceNotifyInterfaceRemoval;
            RtlCopyMemory(&entry->DeviceInterface.ClassGuid, &EventData->u.DeviceInterface.ClassGuid, sizeof(GUID));

            InterlockedPushEntrySList(&PhDeviceNotifyListHead, &entry->ListEntry);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED:
        {
            entry = PhAllocateFromFreeList(&PhDeviceNotifyFreeList);
            memset(entry, 0, sizeof(PH_DEVICE_NOTIFY));
            entry->Action = PhDeviceNotifyInstanceEnumerated;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);

            InterlockedPushEntrySList(&PhDeviceNotifyListHead, &entry->ListEntry);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED:
        {
            entry = PhAllocateFromFreeList(&PhDeviceNotifyFreeList);
            memset(entry, 0, sizeof(PH_DEVICE_NOTIFY));
            entry->Action = PhDeviceNotifyInstanceStarted;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);

            InterlockedPushEntrySList(&PhDeviceNotifyListHead, &entry->ListEntry);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED:
        {
            entry = PhAllocateFromFreeList(&PhDeviceNotifyFreeList);
            memset(entry, 0, sizeof(PH_DEVICE_NOTIFY));
            entry->Action = PhDeviceNotifyInstanceRemoved;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);

            InterlockedPushEntrySList(&PhDeviceNotifyListHead, &entry->ListEntry);
        }
        break;
    }

    return ERROR_SUCCESS;
}

_Function_class_(PH_CALLBACK_FUNCTION)
static VOID NTAPI ServiceProviderUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PSLIST_ENTRY list;

    // drain the list

    if (list = RtlInterlockedFlushSList(&PhDeviceNotifyListHead))
    {
        PhpDeviceNotify(list);
    }
}

BOOLEAN PhpDeviceProviderInitialization(
    VOID
    )
{
    CM_NOTIFY_FILTER cmFilter;

    if (WindowsVersion < WINDOWS_10 || !PhGetIntegerSetting(L"EnableDeviceSupport"))
        return FALSE;

    PhDeviceItemType = PhCreateObjectType(L"DeviceItem", 0, PhpDeviceItemDeleteProcedure);
    PhDeviceTreeType = PhCreateObjectType(L"DeviceTree", 0, PhpDeviceTreeDeleteProcedure);
    PhDeviceNotifyType = PhCreateObjectType(L"DeviceNotify", 0, PhpDeviceNotifyDeleteProcedure);
    PhpDeviceInfoType = PhCreateObjectType(L"DeviceInfo", 0, PhpDeviceInfoDeleteProcedure);

    PhInitializeSListHead(&PhDeviceNotifyListHead);
    PhInitializeFreeList(&PhDeviceNotifyFreeList, sizeof(PH_DEVICE_NOTIFY), 16);

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

    RtlZeroMemory(&cmFilter, sizeof(CM_NOTIFY_FILTER));
    cmFilter.cbSize = sizeof(CM_NOTIFY_FILTER);
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

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackServiceProviderUpdatedEvent),
        ServiceProviderUpdatedCallback,
        NULL,
        &ServiceProviderUpdatedRegistration
        );

    return TRUE;
}

BOOLEAN PhDeviceProviderInitialization(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN initialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        initialized = PhpDeviceProviderInitialization();

        PhEndInitOnce(&initOnce);
    }

    return initialized;
}

PPH_DEVICE_TREE PhReferenceDeviceTree(
    VOID
    )
{
    return PhReferenceDeviceTreeEx(FALSE);
}

PPH_DEVICE_TREE PhReferenceDeviceTreeEx(
    _In_ BOOLEAN ForceRefresh
    )
{
    PPH_DEVICE_TREE deviceTree;

    PhAcquireFastLockShared(&PhpDeviceTreeLock);
    PhSetReference(&deviceTree, PhpDeviceTree);
    PhReleaseFastLockShared(&PhpDeviceTreeLock);

    if (ForceRefresh || !deviceTree)
    {
        PhMoveReference(&deviceTree, PhpPublishDeviceTree());
    }

    return deviceTree;
}

BOOLEAN PhEnumDeviceResources(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ ULONG LogicalConfig,
    _In_ PPH_DEVICE_ENUM_RESOURCES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    BOOLEAN done;
    LOG_CONF logicalConfig;
    PVOID buffer;
    ULONG length;

    if (CM_Get_First_Log_Conf(
        &logicalConfig,
        Item->DeviceInfoData.DeviceData.DevInst,
        LogicalConfig
        ) != CR_SUCCESS)
        return FALSE;

    done = FALSE;
    buffer = PhAllocate(64);
    length = 64;

    while (!done)
    {
        LOG_CONF nextConfig;
        ULONG size;
        RES_DES deviceResource;
        RESOURCEID resourceId;

        if (CM_Get_Next_Res_Des(
            &deviceResource,
            logicalConfig,
            ResType_All,
            &resourceId,
            0
            ) == CR_SUCCESS)
        {
            while (!done)
            {
                RES_DES nextResource;

                if (CM_Get_Res_Des_Data_Size(&size, deviceResource, 0) == CR_SUCCESS)
                {
                    if (size > length)
                    {
                        buffer = PhReAllocate(buffer, size);
                        length = size;
                    }

                    assert(buffer);

                    if (CM_Get_Res_Des_Data(deviceResource, buffer, size, 0) == CR_SUCCESS)
                    {
                        done = Callback(LogicalConfig, resourceId, buffer, size, Context);
                        if (done)
                            break;
                    }
                }

                if (CM_Get_Next_Res_Des(
                    &nextResource,
                    deviceResource,
                    ResType_All,
                    &resourceId,
                    0
                    ) != CR_SUCCESS)
                    break;

                CM_Free_Res_Des_Handle(deviceResource);
                deviceResource = nextResource;
            }
        }

        CM_Free_Res_Des_Handle(deviceResource);

        if (done)
            break;

        if (CM_Get_Next_Log_Conf(&nextConfig, logicalConfig, 0) != CR_SUCCESS)
            break;

        CM_Free_Log_Conf_Handle(logicalConfig);
        logicalConfig = nextConfig;
    }

    CM_Free_Log_Conf_Handle(logicalConfig);

    PhFree(buffer);

    return TRUE;
}
