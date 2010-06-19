#ifndef NTCM_H
#define NTCM_H

// This header file provides configuration manager definitions.
// Source: NT headers

typedef enum _PNP_VETO_TYPE
{
    PNP_VetoTypeUnknown, // unspecified
    PNP_VetoLegacyDevice, // instance path
    PNP_VetoPendingClose, // instance path
    PNP_VetoWindowsApp, // module
    PNP_VetoWindowsService, // service
    PNP_VetoOutstandingOpen, // instance path
    PNP_VetoDevice, // instance path
    PNP_VetoDriver, // driver service name
    PNP_VetoIllegalDeviceRequest, // instance path
    PNP_VetoInsufficientPower, // unspecified
    PNP_VetoNonDisableable, // instance path
    PNP_VetoLegacyDriver, // service
    PNP_VetoInsufficientRights  // unspecified
} PNP_VETO_TYPE, *PPNP_VETO_TYPE;

#endif
