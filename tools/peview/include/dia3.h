#ifndef __dia3_h__
#define __dia3_h__

// "6D31CB3B-EDD4-4C3E-AB44-12B9F7A3828E"
static IID IID_IDiaDataSource2 = { 0x6D31CB3B, 0xEDD4, 0x4C3E, { 0xAB, 0x44, 0x12, 0xB9, 0xF7, 0xA3, 0x82, 0x8E } };

// "65A23C15-BAB3-45DA-8639-F06DE86B9EA8"
static IID IID_IDiaDataSource3 = { 0x65A23C15, 0xBAB3, 0x45DA, { 0x86, 0x39, 0xF0, 0x6D, 0xE8, 0x6B, 0x9E, 0xA8 } };

#undef INTERFACE
#define INTERFACE IDiaDataSource2
DECLARE_INTERFACE_IID(IDiaDataSource2, IDiaDataSource)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IDiaDataSource2

    STDMETHOD(getRawPDBPtr)(THIS,
        _Out_ PVOID PdbBaseAddress
        ) PURE;

    STDMETHOD(loadDataFromRawPDBPtr)(THIS,
        _In_ PVOID PdbBaseAddress
        ) PURE;

    END_INTERFACE
};

#undef INTERFACE
#define INTERFACE IDiaDataSource3
DECLARE_INTERFACE_IID(IDiaDataSource3, IDiaDataSource2)
{
    BEGIN_INTERFACE

    // IUnknown
    STDMETHOD(QueryInterface)(THIS, REFIID riid, PVOID *ppvObject) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IDiaDataSource3

    STDMETHOD(getStreamSize)(THIS,
        _In_ PWSTR StreamName,
        _Out_ PULONG StreamSize
        ) PURE;

    STDMETHOD(getStreamRawData)(THIS,
        _In_ PWSTR StreamName,
        _In_ ULONG BytesToRead,
        _Out_ PBYTE StreamData
        ) PURE;

    END_INTERFACE
};

#endif
