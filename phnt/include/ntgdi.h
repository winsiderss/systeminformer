/*
 * Graphics device interface support
 *
 * This file is part of System Informer.
 */

#ifndef _NTGDI_H
#define _NTGDI_H

#define GDI_MAX_HANDLE_COUNT 0xFFFF // 0x4000
#define GDIK_MAX_HANDLE_COUNT 0x1000000 // GetMaxGdiHandleCount

#define GDI_HANDLE_INDEX_SHIFT 0
#define GDI_HANDLE_INDEX_BITS 16
#define GDI_HANDLE_INDEX_MASK 0xffff

#define GDI_HANDLE_TYPE_SHIFT 16
#define GDI_HANDLE_TYPE_BITS 5
#define GDI_HANDLE_TYPE_MASK 0x1f

#define GDI_HANDLE_ALTTYPE_SHIFT 21
#define GDI_HANDLE_ALTTYPE_BITS 2
#define GDI_HANDLE_ALTTYPE_MASK 0x3

#define GDI_HANDLE_STOCK_SHIFT 23
#define GDI_HANDLE_STOCK_BITS 1
#define GDI_HANDLE_STOCK_MASK 0x1

#define GDI_HANDLE_UNIQUE_SHIFT 24
#define GDI_HANDLE_UNIQUE_BITS 8
#define GDI_HANDLE_UNIQUE_MASK 0xff

#define GDI_HANDLE_INDEX(Handle) ((ULONG)(Handle) & GDI_HANDLE_INDEX_MASK)
#define GDI_HANDLE_TYPE(Handle) (((ULONG)(Handle) >> GDI_HANDLE_TYPE_SHIFT) & GDI_HANDLE_TYPE_MASK)
#define GDI_HANDLE_ALTTYPE(Handle) (((ULONG)(Handle) >> GDI_HANDLE_ALTTYPE_SHIFT) & GDI_HANDLE_ALTTYPE_MASK)
#define GDI_HANDLE_STOCK(Handle) (((ULONG)(Handle) >> GDI_HANDLE_STOCK_SHIFT) & GDI_HANDLE_STOCK_MASK)

#define GDI_MAKE_HANDLE(Index, Unique) ((ULONG)(((ULONG)(Unique) << GDI_HANDLE_INDEX_BITS) | (ULONG)(Index)))

//
// GDI server-side types
//

#define GDI_DEF_TYPE 0 // invalid handle
#define GDI_DC_TYPE 1
#define GDI_DD_DIRECTDRAW_TYPE 2
#define GDI_DD_SURFACE_TYPE 3
#define GDI_RGN_TYPE 4
#define GDI_SURF_TYPE 5
#define GDI_CLIENTOBJ_TYPE 6
#define GDI_PATH_TYPE 7
#define GDI_PAL_TYPE 8
#define GDI_ICMLCS_TYPE 9
#define GDI_LFONT_TYPE 10
#define GDI_RFONT_TYPE 11
#define GDI_PFE_TYPE 12
#define GDI_PFT_TYPE 13
#define GDI_ICMCXF_TYPE 14
#define GDI_ICMDLL_TYPE 15
#define GDI_BRUSH_TYPE 16
#define GDI_PFF_TYPE 17 // unused
#define GDI_CACHE_TYPE 18 // unused
#define GDI_SPACE_TYPE 19
#define GDI_DBRUSH_TYPE 20 // unused
#define GDI_META_TYPE 21
#define GDI_EFSTATE_TYPE 22
#define GDI_BMFD_TYPE 23 // unused
#define GDI_VTFD_TYPE 24 // unused
#define GDI_TTFD_TYPE 25 // unused
#define GDI_RC_TYPE 26 // unused
#define GDI_TEMP_TYPE 27 // unused
#define GDI_DRVOBJ_TYPE 28
#define GDI_DCIOBJ_TYPE 29 // unused
#define GDI_SPOOL_TYPE 30

//
// GDI client-side types
//

#define GDI_CLIENT_TYPE_FROM_HANDLE(Handle) ((ULONG)(Handle) & ((GDI_HANDLE_ALTTYPE_MASK << GDI_HANDLE_ALTTYPE_SHIFT) | \
    (GDI_HANDLE_TYPE_MASK << GDI_HANDLE_TYPE_SHIFT)))
#define GDI_CLIENT_TYPE_FROM_UNIQUE(Unique) GDI_CLIENT_TYPE_FROM_HANDLE((ULONG)(Unique) << 16)

#define GDI_ALTTYPE_1 (1 << GDI_HANDLE_ALTTYPE_SHIFT)
#define GDI_ALTTYPE_2 (2 << GDI_HANDLE_ALTTYPE_SHIFT)
#define GDI_ALTTYPE_3 (3 << GDI_HANDLE_ALTTYPE_SHIFT)

#define GDI_CLIENT_BITMAP_TYPE (GDI_SURF_TYPE << GDI_HANDLE_TYPE_SHIFT)
#define GDI_CLIENT_BRUSH_TYPE (GDI_BRUSH_TYPE << GDI_HANDLE_TYPE_SHIFT)
#define GDI_CLIENT_CLIENTOBJ_TYPE (GDI_CLIENTOBJ_TYPE << GDI_HANDLE_TYPE_SHIFT)
#define GDI_CLIENT_DC_TYPE (GDI_DC_TYPE << GDI_HANDLE_TYPE_SHIFT)
#define GDI_CLIENT_FONT_TYPE (GDI_LFONT_TYPE << GDI_HANDLE_TYPE_SHIFT)
#define GDI_CLIENT_PALETTE_TYPE (GDI_PAL_TYPE << GDI_HANDLE_TYPE_SHIFT)
#define GDI_CLIENT_REGION_TYPE (GDI_RGN_TYPE << GDI_HANDLE_TYPE_SHIFT)

#define GDI_CLIENT_ALTDC_TYPE (GDI_CLIENT_DC_TYPE | GDI_ALTTYPE_1)
#define GDI_CLIENT_DIBSECTION_TYPE (GDI_CLIENT_BITMAP_TYPE | GDI_ALTTYPE_1)
#define GDI_CLIENT_EXTPEN_TYPE (GDI_CLIENT_BRUSH_TYPE | GDI_ALTTYPE_2)
#define GDI_CLIENT_METADC16_TYPE (GDI_CLIENT_CLIENTOBJ_TYPE | GDI_ALTTYPE_3)
#define GDI_CLIENT_METAFILE_TYPE (GDI_CLIENT_CLIENTOBJ_TYPE | GDI_ALTTYPE_2)
#define GDI_CLIENT_METAFILE16_TYPE (GDI_CLIENT_CLIENTOBJ_TYPE | GDI_ALTTYPE_1)
#define GDI_CLIENT_PEN_TYPE (GDI_CLIENT_BRUSH_TYPE | GDI_ALTTYPE_1)

typedef struct _GDI_HANDLE_ENTRY
{
    union
    {
        PVOID Object;
        PVOID NextFree;
    } DUMMYUNIONNAME;
    union
    {
        ULONG Value;
        struct
        {
            USHORT ProcessId;
            USHORT Lock : 1;
            USHORT Count : 15;
        } DUMMYSTRUCTNAME;
    } Owner;
    USHORT Unique;
    UCHAR Type;
    UCHAR Flags;
    PVOID UserPointer;
} GDI_HANDLE_ENTRY, *PGDI_HANDLE_ENTRY;

typedef struct _GDI_SHARED_MEMORY
{
    GDI_HANDLE_ENTRY Handles[GDI_MAX_HANDLE_COUNT];
} GDI_SHARED_MEMORY, *PGDI_SHARED_MEMORY;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

DECLARE_HANDLE(BRUSHOBJ);
DECLARE_HANDLE(SURFOBJ);
DECLARE_HANDLE(CLIPOBJ);
DECLARE_HANDLE(XLATEOBJ);
DECLARE_HANDLE(PATHOBJ);
DECLARE_HANDLE(XFORMOBJ);
DECLARE_HANDLE(BLENDOBJ);
DECLARE_HANDLE(STROBJ);

DECLARE_HANDLE(FONTOBJ);
DECLARE_HANDLE(HSURF);
DECLARE_HANDLE(HDEV);
DECLARE_HANDLE(HGLYPH);
DECLARE_HANDLE(UNIVERSAL_FONT_ID);
DECLARE_HANDLE(KERNEL_PVOID);
DECLARE_HANDLE(ARCTYPE);
DECLARE_HANDLE(TMDIFF);
DECLARE_HANDLE(DHSURF);

typedef struct _WIDTHDATA WIDTHDATA, *PWIDTHDATA;
typedef struct _FONT_FILE_INFO FONT_FILE_INFO, *PFONT_FILE_INFO;
typedef struct _EXTTEXTMETRIC EXTTEXTMETRIC, *PEXTTEXTMETRIC;
typedef struct _PERBANDINFO PERBANDINFO, *PPERBANDINFO;
typedef struct _TMW_INTERNAL TMW_INTERNAL, *PTMW_INTERNAL;
typedef ULONG LFTYPE;
typedef ULONG ROP4;
typedef ULONG MIX;
typedef struct _POINTFIX POINTFIX, *PPOINTFIX;
typedef struct _LINEATTRS LINEATTRS, *PLINEATTRS;
typedef struct _XFORML XFORML, *PXFORML;
typedef struct _FONTINFO FONTINFO, *PFONTINFO;
typedef struct _POINTQF POINTQF, *PPOINTQF;
typedef struct _PATHDATA PATHDATA, *PPATHDATA;
typedef struct _CLIPLINE CLIPLINE, *PCLIPLINE;
typedef struct _IFIMETRICS IFIMETRICS, *PIFIMETRICS;
typedef struct _FD_GLYPHSET FD_GLYPHSET, *PFD_GLYPHSET;
typedef struct _FD_GLYPHATTR FD_GLYPHATTR, *PFD_GLYPHATTR;
typedef struct _DPI_INFORMATION DPI_INFORMATION, *PDPI_INFORMATION;
typedef struct _CHWIDTHINFO *PCHWIDTHINFO;
typedef struct _DEVCAPS *PDEVCAPS;
typedef struct _FONT_REALIZATION_INFO2 *PFONT_REALIZATION_INFO2;
typedef struct _GLYPHPOS *PGLYPHPOS;
typedef struct _RECTFX *PRECTFX;
typedef struct _POLYPATBLT *PPOLYPATBLT;
typedef PVOID DHPDEV;
typedef ULONG HLSURF_INFORMATION_CLASS;

typedef UNIVERSAL_FONT_ID* PUNIVERSAL_FONT_ID;

#ifdef _WIN64
#define ENTRY_SIZE 24
#else
#define ENTRY_SIZE 20
#endif

//typedef struct _HLSURF_INFORMATION_PROBE
//{
//    union
//    {
//        HLSURF_INFORMATION_SURFACE       Surface;
//        HLSURF_INFORMATION_PRESENTFLAGS  PresentFlags;
//        HLSURF_INFORMATION_TOKENUPDATEID UpdateId;
//        HLSURF_INFORMATION_SET_SIGNALING SetSignaling;
//        DWMSURFACEDATA                   SurfaceData;
//        HLSURF_INFORMATION_DIRTYREGIONS  DirtyRegions;
//        HLSURF_INFORMATION_REDIRSTYLE    RedirStyle;
//        HLSURF_INFORMATION_SET_GERNERATE_MOVE_DATA SetGenerateMoveData;
//    } u;
//} HLSURF_INFORMATION_PROBE, *PHLSURF_INFORMATION_PROBE;

#define MAX_COLORTABLE 256
#define GAMMARAMP_SIZE (MAX_COLORTABLE * sizeof(WORD) * 3)

_Kernel_entry_
NTSYSCALLAPI
BOOL 
NTAPI
NtGdiInit(
    VOID
    );

_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR 
NTAPI
NtGdiInit2(
    VOID
    );

_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtGdiSetDIBitsToDeviceInternal(
    _In_ HDC hdcDest,
    _In_ LONG xDst,
    _In_ LONG yDst,
    _In_ ULONG cx,
    _In_ ULONG cy,
    _In_ LONG xSrc,
    _In_ LONG ySrc,
    _In_ ULONG iStartScan,
    _In_ ULONG cNumScan,
    _In_reads_bytes_(cjMaxBits) PBYTE pInitBits,
    _In_ LPBITMAPINFO pbmi,
    _In_ ULONG iUsage,
    _In_ ULONG cjMaxBits,
    _In_ ULONG cjMaxInfo,
    _In_ BOOL bTransformCoordinates,
    _In_opt_ HANDLE hcmXform
    );

_Kernel_entry_
NTSYSCALLAPI
HBITMAP 
NTAPI
NtGdiCreateSessionMappedDIBSection(
    _In_opt_ HDC hdc,
    _In_opt_ HANDLE hSectionApp,
    _In_ ULONG dwOffset,
    _In_reads_bytes_opt_(cjHeader) LPBITMAPINFO pbmi,
    _In_ ULONG iUsage,
    _In_ ULONG cjHeader,
    _In_ FLONG fl,
    _In_ ULONG_PTR dwColorSpace
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetFontResourceInfoInternalW(
    _In_reads_(cwc) PWSTR pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ ULONG cjIn,
    _Out_ PULONG pdwBytes,
    _Out_writes_bytes_(cjIn) PVOID pvBuf,
    _In_ ULONG iType
    );

_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetGlyphIndicesW(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) PWSTR pwc,
    _In_ LONG cwc,
    _Out_writes_opt_(cwc) PUSHORT pgi,
    _In_ ULONG iMode
    );

_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetGlyphIndicesWInternal(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) PWSTR pwc,
    _In_ LONG cwc,
    _Out_writes_opt_(cwc) PUSHORT pgi,
    _In_ ULONG iMode,
    _In_ BOOL bSubset
    );

//
// pLogPal is annotated as _In_reads_bytes_(cEntries * 4  + 4) because the
// current SAL doesn't support sizeof operator. The size of related buffer is
// sizeof(LOGPALETTE) - sizeof(PALETTEENTRY) + sizeof(PALETTEENTRY) * cEntries.
//

/**
 * NtGdiCreatePaletteInternal contract notes.
 *
 * Creates an owned palette from a LOGPALETTE header and cEntries four-byte PALETTEENTRY records. The caller
 * retains ownership of the input buffer.
 */
_Kernel_entry_
NTSYSCALLAPI
HPALETTE
NTAPI
NtGdiCreatePaletteInternal(
    _In_reads_bytes_(cEntries * 4  + 4) LPLOGPALETTE pLogPal,
    _In_ ULONG cEntries
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiArcInternal(
    _In_ ARCTYPE arctype,
    _In_ HDC hdc,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_ LONG x3,
    _In_ LONG y3,
    _In_ LONG x4,
    _In_ LONG y4
    );

_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtGdiStretchDIBitsInternal(
    _In_ HDC hdc,
    _In_ LONG xDst,
    _In_ LONG yDst,
    _In_ LONG cxDst,
    _In_ LONG cyDst,
    _In_ LONG xSrc,
    _In_ LONG ySrc,
    _In_ LONG cxSrc,
    _In_ LONG cySrc,
    _In_reads_bytes_opt_(cjMaxBits) PBYTE pjInit,
    _In_ LPBITMAPINFO pbmi,
    _In_ ULONG dwUsage,
    _In_ ULONG dwRop4,
    _In_ ULONG cjMaxInfo,
    _In_ ULONG cjMaxBits,
    _In_ HANDLE hcmXform
    );

_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetOutlineTextMetricsInternalW(
    _In_ HDC hdc,
    _In_ ULONG cjotm,
    _Out_writes_bytes_opt_(cjotm) OUTLINETEXTMETRICW *potmw,
    _Out_ TMDIFF *ptmd
    );

/**
 * NtGdiGetAndSetDCULONG contract notes.
 *
 * Historical declaration: this name was not located in either inspected win32u export table or matching
 * WIN32KBASE/WIN32KFULL names. Do not assume it is callable merely because the prototype is available.
 */
_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetAndSetDCULONG(
    _In_ HDC hdc,
    _In_ ULONG u,
    _In_ ULONG dwIn,
    _Out_ ULONG *pdwResult
    );

_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtGdiGetDCObject(
    _In_  HDC hdc,
    _In_  LONG itype
    );

_Kernel_entry_
NTSYSCALLAPI
HDC
NTAPI
NtGdiGetDCforBitmap(
    _In_ HBITMAP hsurf
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetMonitorID(
    _In_  HDC hdc,
    _In_  ULONG dwSize,
    _Out_writes_bytes_(dwSize) PWSTR pszMonitorID
    );

// flags returned from GetUFI and passed to GetUFIBits
#define FL_UFI_PRIVATEFONT      1
#define FL_UFI_DESIGNVECTOR_PFF 2
#define FL_UFI_MEMORYFONT       4

_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetLinkedUFIs(
    _In_ HDC hdc,
    _Out_writes_opt_(BufferSize) PUNIVERSAL_FONT_ID pufiLinkedUFIs,
    _In_ LONG BufferSize
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetLinkedUFIs(
    _In_ HDC hdc,
    _In_reads_(uNumUFIs) PUNIVERSAL_FONT_ID pufiLinks,
    _In_ ULONG uNumUFIs
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetUFI(
    _In_  HDC hdc,
    _Out_ PUNIVERSAL_FONT_ID pufi,
    _Out_opt_ DESIGNVECTOR *pdv,
    _Out_ ULONG *pcjDV,
    _Out_ ULONG *pulBaseCheckSum,
    _Out_ FLONG *pfl
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiForceUFIMapping(
    _In_ HDC hdc,
    _In_ PUNIVERSAL_FONT_ID pufi
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetUFIPathname(
    _In_ PUNIVERSAL_FONT_ID pufi,
    _Deref_out_range_(0, MAX_PATH * 3) ULONG* pcwc,
    _Out_writes_to_opt_(MAX_PATH * 3, *pcwc) PWSTR pwszPathname,
    _Out_opt_ ULONG* pcNumFiles,
    _In_ FLONG fl,
    _Out_opt_ BOOL *pbMemFont,
    _Out_opt_ ULONG *pcjView,
    _Out_opt_ PVOID pvView,
    _Out_opt_ BOOL *pbTTC,
    _Out_opt_ ULONG *piTTC
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAddRemoteFontToDC(
    _In_ HDC hdc,
    _In_reads_bytes_(cjBuffer) PVOID pvBuffer,
    _In_ ULONG cjBuffer,
    _In_opt_ PUNIVERSAL_FONT_ID pufi
    );

_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtGdiAddFontMemResourceEx(
    _In_reads_bytes_(cjBuffer) PVOID pvBuffer,
    _In_ ULONG cjBuffer,
    _In_reads_bytes_opt_(cjDV) DESIGNVECTOR *pdv,
    _In_ ULONG cjDV,
    _Out_ ULONG *pNumFonts
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRemoveFontMemResourceEx(
    _In_ HANDLE hMMFont
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiUnmapMemFont(
    _In_ PVOID pvView
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRemoveMergeFont(
    _In_ HDC hdc,
    _In_ UNIVERSAL_FONT_ID *pufi
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAnyLinkedFonts(
    VOID
    );

//
// Local printing with embedded fonts
//

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetEmbUFI(
    _In_ HDC hdc,
    _Out_ PUNIVERSAL_FONT_ID pufi,
    _Out_opt_ DESIGNVECTOR *pdv,
    _Out_ ULONG *pcjDV,
    _Out_ ULONG *pulBaseCheckSum,
    _Out_ FLONG  *pfl,
    _Out_ KERNEL_PVOID *embFontID
    );

/**
 * NtGdiGetEmbedFonts contract notes.
 *
 * Queries the session font table's embedded-font count without taking arguments. The result is a count and
 * can legitimately be zero.
 */
_Kernel_entry_
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetEmbedFonts(
    VOID
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiChangeGhostFont(
    _In_  KERNEL_PVOID *pfontID,
    _In_  BOOL bLoad
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAddEmbFontToDC(
    _In_ HDC hdc,
    _In_ VOID **pFontID
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiFontIsLinked(
    _In_ HDC hdc
    );

_Kernel_entry_
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtGdiPolyPolyDraw(
    _In_ HDC hdc,
    _In_ PPOINT ppt,
    _In_reads_(ccpt) PULONG pcpt,
    _In_ ULONG ccpt,
    _In_ LONG iFunc
    );

/**
 * NtGdiDoPalette contract notes.
 *
 * Dispatches palette operations by iFunc: 0 AnimatePalette, 1 SetPaletteEntries, 2 GetPaletteEntries, 3
 * GetSystemPaletteEntries, 4 GetDIBColorTable, 5 SetDIBColorTable. Selectors 0/1/5 consume entries; 2/3/4
 * produce entries. The first handle is interpreted according to the selector. Values above 5 return zero on
 * the inspected build; bUnused is not consumed by the native worker.
 */
_Kernel_entry_
NTSYSCALLAPI
LONG
NTAPI
NtGdiDoPalette(
    _In_ HPALETTE hpal,
    _In_ WORD iStart,
    _In_ WORD cEntries,
    _When_(iFunc == 0 || iFunc == 1 || iFunc == 5, _In_reads_opt_(cEntries))
    _When_(iFunc >= 2 && iFunc <= 4, _Out_writes_opt_(cEntries))
    PALETTEENTRY *pPalEntries,
    _In_ ULONG iFunc,
    _In_ BOOL bUnused
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiComputeXformCoefficients(
    _In_ HDC hdc
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetWidthTable(
    _In_ HDC hdc,
    _In_ ULONG cSpecial,
    _In_reads_(cwc) const WCHAR *pwc,
    _In_ ULONG cwc,
    _Out_writes_(cwc) USHORT *psWidth,
    _Out_opt_ WIDTHDATA *pwd,
    _Out_ FLONG *pflInfo
    );

/**
 * NtGdiDescribePixelFormat contract notes.
 *
 * With cjpfd=0, permits ppfd=NULL for a format-count query. On the inspected native entry output copying is
 * capped at the 40-byte PIXELFORMATDESCRIPTOR; the return value is a format count, not bytes copied.
 * Availability depends on the DC/device.
 */
_Kernel_entry_
NTSYSCALLAPI
_Success_(return > 0) 
LONG
NTAPI
NtGdiDescribePixelFormat(
    _In_ HDC hdc,
    _In_ LONG ipfd,
    _In_ ULONG cjpfd,
    _Out_writes_bytes_opt_(cjpfd) PPIXELFORMATDESCRIPTOR ppfd
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetPixelFormat(
    _In_ HDC hdc,
    _In_ LONG ipfd
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSwapBuffers(
    _In_ HDC hdc
    );

//
// Image32
//

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAlphaBlend(
    _In_ HDC hdcDst,
    _In_ LONG DstX,
    _In_ LONG DstY,
    _In_ LONG DstCx,
    _In_ LONG DstCy,
    _In_ HDC hdcSrc,
    _In_ LONG SrcX,
    _In_ LONG SrcY,
    _In_ LONG SrcCx,
    _In_ LONG SrcCy,
    _In_ BLENDFUNCTION BlendFunction,
    _In_ HANDLE hcmXform
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGradientFill(
    _In_ HDC hdc,
    _In_reads_(nVertex) PTRIVERTEX pVertex,
    _In_ ULONG nVertex,
    _In_ PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ ULONG ulMode
    );

//
// ICM (Image Color Matching)
//

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetIcmMode(
    _In_ HDC hdc,
    _In_ ULONG nCommand,
    _In_ ULONG ulMode
    );

#define ICM_SET_MODE             1
#define ICM_SET_CALIBRATE_MODE   2
#define ICM_SET_COLOR_MODE       3
#define ICM_CHECK_COLOR_MODE     4

typedef struct _LOGCOLORSPACEEXW
{
    LOGCOLORSPACEW lcsColorSpace;
    ULONG dwFlags;
} LOGCOLORSPACEEXW, *PLOGCOLORSPACEEXW;

#define LCSEX_ANSICREATED    0x0001 // Created by CreateColorSpaceA()
#define LCSEX_TEMPPROFILE    0x0002 // Color profile is temporary file

_Kernel_entry_
NTSYSCALLAPI
HANDLE
NTAPI
NtGdiCreateColorSpace(
    _In_ PLOGCOLORSPACEEXW pLogColorSpace
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiDeleteColorSpace(
    _In_ HANDLE hColorSpace
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetColorSpace(
    _In_ HDC hdc,
    _In_ HCOLORSPACE hColorSpace
    );

_Kernel_entry_ 
NTSYSCALLAPI
HANDLE
NTAPI
NtGdiCreateColorTransform(
    _In_ HDC hdc,
    _In_ LPLOGCOLORSPACEW pLogColorSpaceW,
    _In_reads_bytes_opt_(cjSrcProfile) PVOID pvSrcProfile,
    _In_ ULONG cjSrcProfile,
    _In_reads_bytes_opt_(cjDestProfile) PVOID pvDestProfile,
    _In_ ULONG cjDestProfile,
    _In_reads_bytes_opt_(cjTargetProfile) PVOID pvTargetProfile,
    _In_ ULONG cjTargetProfile
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiDeleteColorTransform(
    _In_ HDC hdc,
    _In_ HANDLE hColorTransform
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiCheckBitmapBits(
    _In_ HDC hdc,
    _In_ HANDLE hColorTransform,
    _In_ PVOID pvBits,
    _In_ ULONG bmFormat,
    _In_ ULONG dwWidth,
    _In_ ULONG dwHeight,
    _In_ ULONG dwStride,
    _Out_writes_bytes_(dwWidth * dwHeight) PBYTE paResults
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiColorCorrectPalette(
    _In_ HDC hdc,
    _In_ HPALETTE hpal,
    _In_ ULONG FirstEntry,
    _In_ ULONG NumberOfEntries,
    _Inout_updates_(NumberOfEntries) PALETTEENTRY *ppalEntry,
    _In_ ULONG Command
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG_PTR
NTAPI
NtGdiGetColorSpaceforBitmap(
    _In_ HBITMAP hsurf
    );

typedef enum _COLORPALETTEINFO
{
    ColorPaletteQuery,
    ColorPaletteSet
} COLORPALETTEINFO, *PCOLORPALETTEINFO;

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiIcmBrushInfo(
    _In_ HDC hdc,
    _In_ HBRUSH hbrush,
    _Inout_updates_bytes_(sizeof(BITMAPINFO) + ((/*MAX_COLORTABLE*/256 - 1) * sizeof(RGBQUAD))) PBITMAPINFO pbmiDIB,
    _Inout_updates_bytes_(*pulBits) PVOID pvBits,
    _Inout_ ULONG *pulBits,
    _Out_opt_ ULONG *piUsage,
    _Out_opt_ BOOL *pbAlreadyTran,
    _In_ ULONG Command
    );

typedef enum _ICM_DIB_INFO_CMD
{
    IcmQueryBrush,
    IcmSetBrush
} ICM_DIB_INFO, *PICM_DIB_INFO;

/**
 * NtGdiFlush contract notes.
 *
 * Runs the current GDI flush callbacks and returns STATUS_SUCCESS (zero) on the inspected build. This return
 * must not be tested as a BOOL; it does not establish that every possible device operation succeeded.
 */
_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiFlush(
    VOID
    );

_Kernel_entry_ 
NTSYSCALLAPI
HDC
NTAPI
NtGdiCreateMetafileDC(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiMakeInfoDC(
    _In_ HDC hdc,
    _In_ BOOL bSet
    );

_Kernel_entry_ 
NTSYSCALLAPI
HANDLE
NTAPI
NtGdiCreateClientObj(
    _In_ ULONG ulType
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiDeleteClientObj(
    _In_ HANDLE h
    );

/**
 * NtGdiGetBitmapBits contract notes.
 *
 * Reads packed device-dependent bitmap bits. NULL output returns the required byte count; a non-NULL output
 * is bounded by cjMax and the bitmap extent. The return is bytes copied, not a Boolean success flag.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetBitmapBits(
    _In_ HBITMAP hbm,
    _In_ ULONG cjMax,
    _Out_writes_bytes_opt_(cjMax) PBYTE pjOut
    );

/**
 * NtGdiDeleteObjectApp contract notes.
 *
 * Deletes a GDI object through its object-kind dispatch, including owned memory DCs in the tested subset.
 * Restore selected objects before deleting caller-owned resources. Do not use NtClose for GDI handles or
 * delete borrowed stock objects.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiDeleteObjectApp(
    _In_ HANDLE hobj
    );

/**
 * NtGdiGetPath contract notes.
 *
 * Queries a completed DC path as POINTs and one-byte type codes. cptBuf=0 is a point-count query. A nonzero
 * buffer must accommodate the complete path; short capacity returns -1 rather than a successfully truncated
 * path. Both arrays are probed with 4-byte alignment on the inspected native entry.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetPath(
    _In_ HDC hdc,
    _Out_writes_opt_(cptBuf) PPOINT pptlBuf,
    _Out_writes_opt_(cptBuf) PBYTE pjTypes,
    _In_ LONG cptBuf
    );

/**
 * NtGdiCreateCompatibleDC contract notes.
 *
 * Creates an owned memory DC, optionally using an existing DC as the compatibility source. NULL selects the
 * default compatible device. This does not create a window or authorize drawing into another process.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HDC
NTAPI
NtGdiCreateCompatibleDC(
    _In_opt_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiCreateDIBitmapInternal(
    _In_ HDC hdc,
    _In_ LONG cx,
    _In_ LONG cy,
    _In_ ULONG fInit,
    _In_reads_bytes_opt_(cjMaxBits) PBYTE pjInit,
    _In_reads_bytes_opt_(cjMaxInitInfo) LPBITMAPINFO pbmi,
    _In_ ULONG iUsage,
    _In_ ULONG cjMaxInitInfo,
    _In_ ULONG cjMaxBits,
    _In_ FLONG f,
    _In_ HANDLE hcmXform
    );

/**
 * NtGdiCreateDIBSection contract notes.
 *
 * Creates a bitmap with process-accessible pixel storage. NULL section handle requests private allocation;
 * otherwise the supplied section and offset back the bits. On success ppvBits receives the pixel address;
 * deleting the bitmap releases its associated mapping. Bitmap header size and pixel layout are distinct
 * capacities.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiCreateDIBSection(
    _In_opt_ HDC hdc,
    _In_opt_ HANDLE hSectionApp,
    _In_ ULONG dwOffset,
    _In_reads_bytes_opt_(cjHeader) LPBITMAPINFO pbmi,
    _In_ ULONG iUsage,
    _In_ ULONG cjHeader,
    _In_ FLONG fl,
    _In_ ULONG_PTR dwColorSpace,
    _Outptr_ PVOID *ppvBits
    );

/**
 * NtGdiCreateSolidBrush contract notes.
 *
 * Creates a solid-color GDI brush; the optional brush parameter participates in the client allocation
 * protocol. Use NULL for ordinary creation, restore any selected previous brush before deletion, and do not
 * NtClose it.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBRUSH
NTAPI
NtGdiCreateSolidBrush(
    _In_ COLORREF cr,
    _In_opt_ HBRUSH hbr
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBRUSH
NTAPI
NtGdiCreateDIBBrush(
    _In_reads_bytes_(cj) PVOID pv,
    _In_ FLONG fl,
    _In_ ULONG  cj,
    _In_ BOOL  b8X8,
    _In_ BOOL bPen,
    _In_ PVOID pClient
    );

/**
 * NtGdiCreatePatternBrushInternal contract notes.
 *
 * Creates a brush from a bitmap pattern with internal pen/8-by-8 controls. The brush and source bitmap are
 * distinct handles with independently managed lifetimes.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBRUSH
NTAPI
NtGdiCreatePatternBrushInternal(
    _In_ HBITMAP hbm,
    _In_ BOOL bPen,
    _In_ BOOL b8X8
    );

/**
 * NtGdiCreateHatchBrushInternal contract notes.
 *
 * Creates an owned hatch brush with a color and an internal pen-use selector. Ordinary brush creation uses
 * bPen=FALSE. The style is a hatch selector, not arbitrary pixel data.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBRUSH
NTAPI
NtGdiCreateHatchBrushInternal(
    _In_ ULONG ulStyle,
    _In_ COLORREF clrr,
    _In_ BOOL bPen
    );

_Kernel_entry_ 
NTSYSCALLAPI
HPEN
NTAPI
NtGdiExtCreatePen(
    _In_ ULONG flPenStyle,
    _In_ ULONG ulWidth,
    _In_ ULONG iBrushStyle,
    _In_ ULONG ulColor,
    _In_ ULONG_PTR lClientHatch,
    _In_ ULONG_PTR lHatch,
    _In_ ULONG cstyle,
    _In_reads_opt_(cstyle) PULONG pulStyle,
    _In_ ULONG cjDIB,
    _In_ BOOL bOldStylePen,
    _In_opt_ HBRUSH hbrush
    );

/**
 * NtGdiCreateEllipticRgn contract notes.
 *
 * Creates an owned region describing an ellipse for clipping and membership queries. The result is a GDI
 * handle, not an NtClose handle. Delete it when no longer needed.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HRGN
NTAPI
NtGdiCreateEllipticRgn(
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom
    );

/**
 * NtGdiCreateRoundRectRgn contract notes.
 *
 * Creates an owned region from a rounded rectangle; zero corner dimensions still follow this constructor's
 * coordinate convention, not necessarily CreateRectRgn bounds. Use NtGdiSetRectRgn when exact rectangular
 * bounds are needed. Release the resulting region through the GDI deletion path.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HRGN
NTAPI
NtGdiCreateRoundRectRgn(
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom,
    _In_ LONG xWidth,
    _In_ LONG yHeight
    );

_Kernel_entry_ 
NTSYSCALLAPI
HANDLE 
NTAPI
NtGdiCreateServerMetaFile(
    _In_ ULONG iType,
    _In_ ULONG cjData,
    _In_reads_bytes_(cjData) PBYTE pjData,
    _In_ ULONG mm,
    _In_ ULONG xExt,
    _In_ ULONG yExt
    );

/**
 * NtGdiExtCreateRegion contract notes.
 *
 * Builds an owned region from a counted RGNDATA buffer, optionally transforming the supplied rectangles. The
 * byte count covers the header and rectangle array; the source storage remains caller-owned.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HRGN
NTAPI
NtGdiExtCreateRegion(
    _In_opt_ LPXFORM px,
    _In_ ULONG cj,
    _In_reads_bytes_(cj) LPRGNDATA prgn
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiMakeFontDir(
    _In_ FLONG flEmbed,
    _Out_writes_bytes_(cjFontDir) PBYTE pjFontDir,
    _In_ unsigned cjFontDir,
    _In_reads_bytes_(cjPathname) PWSTR pwszPathname,
    _In_ unsigned cjPathname
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPolyDraw(
    _In_ HDC hdc,
    _In_reads_(cpt) PPOINT ppt,
    _In_reads_(cpt) PBYTE pjAttr,
    _In_ ULONG cpt
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPolyTextOutW(
    _In_ HDC hdc,
    _In_reads_(cStr) POLYTEXTW *pptw,
    _In_ ULONG cStr,
    _In_ ULONG dwCodePage
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetServerMetaFileBits(
    _In_ HANDLE hmo,
    _In_ ULONG cjData,
    _Out_writes_bytes_opt_(cjData) PBYTE pjData,
    _Out_ PULONG piType,
    _Out_ PULONG pmm,
    _Out_ PULONG pxExt,
    _Out_ PULONG pyExt
    );

/**
 * NtGdiEqualRgn contract notes.
 *
 * Compares region geometry rather than handle identity. Both arguments must refer to initialized regions;
 * distinct handles can compare equal.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEqualRgn(
    _In_ HRGN hrgn1,
    _In_ HRGN hrgn2
    );

/**
 * NtGdiGetBitmapDimension contract notes.
 *
 * Retrieves the bitmap's stored logical dimensions in a SIZE record. These dimensions are not necessarily
 * the pixel width/height returned in BITMAP metadata.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetBitmapDimension(
    _In_ HBITMAP hbm,
    _Out_ PSIZE psize
    );

/**
 * NtGdiGetNearestPaletteIndex contract notes.
 *
 * Returns the index of the closest logical-palette color. Index zero can be a valid result; do not test it
 * as a generic BOOL success value.
 */
_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetNearestPaletteIndex(
    _In_ HPALETTE hpal,
    _In_ COLORREF crColor
    );

/**
 * NtGdiPtVisible contract notes.
 *
 * Tests whether a point lies in the effective visible/clipping area of a DC. This is a BOOL property query;
 * FALSE is not necessarily an invalid handle error.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPtVisible(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRectVisible(
    _In_ HDC hdc,
    _In_ PRECT prc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRemoveFontResourceW(
    _In_reads_(cwc) WCHAR *pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ ULONG fl,
    _In_ ULONG dwPidTid,
    _In_opt_ DESIGNVECTOR *pdv
    );

/**
 * NtGdiResizePalette contract notes.
 *
 * Changes the entry capacity of an existing logical palette without returning a new handle. This is a
 * mutation of that palette, not a request to alter the system palette mode.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiResizePalette(
    _In_ HPALETTE hpal,
    _In_ ULONG cEntry
    );

/**
 * NtGdiSetBitmapDimension contract notes.
 *
 * Updates the bitmap's logical dimensions and optionally returns the previous SIZE. This does not resize or
 * reallocate its pixel buffer.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetBitmapDimension(
    _In_ HBITMAP hbm,
    _In_ LONG cx,
    _In_ LONG cy,
    _Out_opt_ PSIZE psizeOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiOffsetClipRgn(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiSetMetaRgn(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetTextJustification(
    _In_ HDC hdc,
    _In_ LONG lBreakExtra,
    _In_ LONG cBreak
    );

/**
 * NtGdiGetAppClipBox contract notes.
 *
 * Returns the application clipping bounds and a region-complexity code, not an NTSTATUS. The RECT output is
 * meaningful only when the returned code is not ERROR.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetAppClipBox(
    _In_ HDC hdc,
    _Out_ PRECT prc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetTextExtentExW(
    _In_ HDC hdc,
    _In_reads_opt_(cwc) PWSTR lpwsz,
    _In_ ULONG cwc,
    _In_ ULONG dxMax,
    _Out_opt_ ULONG *pcCh,   // range(0, cwc)
    _Out_writes_to_opt_(cwc, *pcCh) PULONG pdxOut,
    _Out_ PSIZE psize,
    _In_ FLONG fl
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetCharABCWidthsW(
    _In_ HDC hdc,
    _In_ ULONG wchFirst,
    _In_ ULONG cwch,
    _In_reads_opt_(cwch) PWCHAR pwch,
    _In_ FLONG fl,
    _At_((ABC *)pvBuf, _Out_writes_bytes_(cwch * sizeof(ABC))) PVOID pvBuf
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetCharacterPlacementW(
    _In_ HDC hdc,
    _In_reads_(nCount) PWSTR pwsz,
    _In_ LONG nCount,
    _In_ LONG nMaxExtent,
    _Inout_ LPGCP_RESULTSW pgcpw,
    _In_ ULONG dwFlags
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAngleArc(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ ULONG dwRadius,
    _In_ ULONG dwStartAngle,
    _In_ ULONG dwSweepAngle
    );

/**
 * NtGdiBeginPath contract notes.
 *
 * Begins a path bracket in a DC; subsequent supported drawing operations record path geometry. Finish with
 * NtGdiEndPath or cancel with NtGdiAbortPath before consuming the path.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiBeginPath(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSelectClipPath(
    _In_ HDC hdc,
    _In_ LONG iMode
    );

/**
 * NtGdiCloseFigure contract notes.
 *
 * Closes the current path figure back to its start. It is a path operation, not an EndPath replacement; the
 * path bracket still requires completion.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiCloseFigure(
    _In_ HDC hdc
    );

/**
 * NtGdiEndPath contract notes.
 *
 * Closes the path bracket, making the accumulated geometry available to path queries and
 * rendering/conversion operations.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEndPath(
    _In_ HDC hdc
    );

/**
 * NtGdiAbortPath contract notes.
 *
 * Discards the DC's current path state. Use it to cancel an unfinished or unneeded path rather than deleting
 * the DC merely for cleanup.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAbortPath(
    _In_ HDC hdc
    );

/**
 * NtGdiFillPath contract notes.
 *
 * Fills the completed path using the selected brush and consumes that path. Begin a new path bracket before
 * another independent fill/stroke operation.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiFillPath(
    _In_ HDC hdc
    );

/**
 * NtGdiStrokeAndFillPath contract notes.
 *
 * Strokes and fills the completed path using the selected pen/brush and consumes its stored geometry.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiStrokeAndFillPath(
    _In_ HDC hdc
    );

/**
 * NtGdiStrokePath contract notes.
 *
 * Strokes the completed path using the selected pen and consumes it. The drawing target and pen must remain
 * valid throughout the call.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiStrokePath(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiWidenPath(
    _In_ HDC hdc
    );

/**
 * NtGdiFlattenPath contract notes.
 *
 * Converts curves in a completed DC path to line segments in place. The entry changes stored path geometry;
 * it does not return a newly allocated path handle.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiFlattenPath(
    _In_ HDC hdc
    );

/**
 * NtGdiPathToRegion contract notes.
 *
 * Converts the completed DC path into an owned region and consumes the path. The result must be deleted
 * separately from the DC.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HRGN
NTAPI
NtGdiPathToRegion(
    _In_ HDC hdc
    );

/**
 * NtGdiSetMiterLimit contract notes.
 *
 * Sets the miter limit using IEEE-754 single-precision bits carried in ULONG, not an integer-valued limit or
 * a floating-point ABI argument. The optional old-limit output is write-only. Values below 1.0 fail without
 * changing that output on the inspected build.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetMiterLimit(
    _In_ HDC hdc,
    _In_ ULONG dwNew,
    _Out_opt_ PULONG pdwOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetFontXform(
    _In_ HDC hdc,
    _In_ ULONG dwxScale,
    _In_ ULONG dwyScale
    );

/**
 * NtGdiGetMiterLimit contract notes.
 *
 * Returns the DC miter limit as IEEE-754 single-precision bits in the ULONG output. Reinterpret the bits as
 * FLOAT rather than numerically converting the ULONG value.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetMiterLimit(
    _In_ HDC hdc,
    _Out_ PULONG pdwOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEllipse(
    _In_ HDC hdc,
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRectangle(
    _In_ HDC hdc,
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRoundRect(
    _In_ HDC hdc,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_ LONG x3,
    _In_ LONG y3
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPlgBlt(
    _In_ HDC hdcTrg,
    _In_reads_(3) PPOINT pptlTrg,
    _In_ HDC hdcSrc,
    _In_ LONG xSrc,
    _In_ LONG ySrc,
    _In_ LONG cxSrc,
    _In_ LONG cySrc,
    _In_opt_ HBITMAP hbmMask,
    _In_ LONG xMask,
    _In_ LONG yMask,
    _In_ ULONG crBackColor
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiMaskBlt(
    _In_ HDC hdc,
    _In_ LONG xDst,
    _In_ LONG yDst,
    _In_ LONG cx,
    _In_ LONG cy,
    _In_ HDC hdcSrc,
    _In_ LONG xSrc,
    _In_ LONG ySrc,
    _In_ HBITMAP hbmMask,
    _In_ LONG xMask,
    _In_ LONG yMask,
    _In_ ULONG dwRop4,
    _In_ ULONG crBackColor
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiExtFloodFill(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ COLORREF crColor,
    _In_ ULONG iFillType
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiFillRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiFrameRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HBRUSH hbrush,
    _In_ LONG xWidth,
    _In_ LONG yHeight
    );

/**
 * NtGdiSetPixel contract notes.
 *
 * Writes a pixel in the specified DC using a COLORREF and returns the resulting color or CLR_INVALID. Use a
 * caller-owned memory bitmap for off-screen work rather than assuming this entry is side-effect-free.
 */
_Kernel_entry_ 
NTSYSCALLAPI
COLORREF
NTAPI
NtGdiSetPixel(
    _In_ HDC hdcDst,
    _In_ LONG x,
    _In_ LONG y,
    _In_ COLORREF crColor
    );

/**
 * NtGdiGetPixel contract notes.
 *
 * Reads a COLORREF from the selected surface. CLR_INVALID is failure; it is not an NTSTATUS. DC clipping,
 * transforms and device capabilities affect the coordinate interpretation.
 */
_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetPixel(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiStartPage(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEndPage(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiStartDoc(
    _In_ HDC hdc,
    _In_ DOCINFOW *pdi,
    _Out_ BOOL *pbBanding,
    _In_ LONG iJob
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEndDoc(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAbortDoc(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiUpdateColors(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetCharWidthW(
    _In_ HDC hdc,
    _In_ ULONG wcFirst,
    _In_ ULONG cwc,
    _In_reads_opt_(cwc) PWCHAR pwc,
    _In_ FLONG fl,
    _At_((PULONG)pvBuf, _Out_writes_bytes_(cwc * sizeof(ULONG))) PVOID pvBuf
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetCharWidthInfo(
    _In_ HDC hdc,
    _Out_ PCHWIDTHINFO pChWidthInfo
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiDrawEscape(
    _In_ HDC hdc,
    _In_ LONG iEsc,
    _In_ LONG cjIn,
    _In_reads_bytes_opt_(cjIn) PSTR pjIn
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiExtEscape(
    _In_opt_ HDC hdc,
    _In_reads_opt_(nDriver) PWCHAR pDriver,
    _In_ LONG nDriver,
    _In_ LONG iEsc,
    _In_ LONG cjIn,
    _In_reads_bytes_opt_(cjIn) PSTR pjIn,
    _In_ LONG cjOut,
    _Out_writes_bytes_opt_(cjOut) PSTR pjOut
    );

_Success_(return != GDI_ERROR)
_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetFontData(
    _In_ HDC hdc,
    _In_ ULONG dwTable,
    _In_ ULONG dwOffset,
    _Out_writes_bytes_to_opt_(cjBuf, return) PVOID pvBuf,
    _In_ ULONG cjBuf
    );

/**
 * NtGdiGetFontFileData contract notes.
 *
 * Copies bytes from a font file selected by collection ID and file index. The return is a Win32 error code:
 * zero on success, ERROR_INVALID_PARAMETER (87) for an invalid selection/range on the inspected path. It is
 * neither BOOL nor a byte count.
 */
_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetFontFileData(
    _In_ ULONG uFileCollectionID,
    _In_ ULONG uFileIndex,
    _In_ PULONGLONG pullFileOffset,
    _Out_writes_bytes_(cbSize) void* pBuffer,
    _In_ SIZE_T cbSize
    );

/**
 * NtGdiGetFontFileInfo contract notes.
 *
 * Returns file metadata and its path, with optional SIZE_T required-size output. A NULL/short metadata
 * buffer returns ERROR_INSUFFICIENT_BUFFER (122) for a valid selection; invalid collection/index returns
 * ERROR_INVALID_PARAMETER (87). Zero is success, not FALSE.
 */
_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetFontFileInfo(
    _In_ ULONG uFileCollectionID,
    _In_ ULONG uFileIndex,
    _Out_writes_bytes_opt_(cbSize) FONT_FILE_INFO * pfi,
    _In_ SIZE_T cbSize,
    _Out_opt_ PSIZE_T pcbActualSize
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetGlyphOutline(
    _In_ HDC hdc,
    _In_ WCHAR wch,
    _In_ ULONG iFormat,
    _Out_ LPGLYPHMETRICS pgm,
    _In_ ULONG cjBuf,
    _Out_writes_bytes_opt_(cjBuf) PVOID pvBuf,
    _In_ LPMAT2 pmat2,
    _In_ BOOL bIgnoreRotation
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetETM(
    _In_ HDC hdc,
    _Out_opt_ EXTTEXTMETRIC *petm
    );

_Kernel_entry_ 
NTSYSCALLAPI
_Success_(return == 1)
BOOL
NTAPI
NtGdiGetRasterizerCaps(
    _Out_writes_bytes_(cjBytes) LPRASTERIZER_STATUS praststat,
    _In_ ULONG cjBytes
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetKerningPairs(
    _In_ HDC hdc,
    _In_ ULONG cPairs,
    _Out_writes_to_opt_(cPairs, return) KERNINGPAIR *pkpDst
    );

/**
 * NtGdiMonoBitmap contract notes.
 *
 * Checks whether a bitmap is monochrome. The BOOL result is a property query, not a byte size or an owning
 * reference.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiMonoBitmap(
    _In_ HBITMAP hbm
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiGetObjectBitmapHandle(
    _In_ HBRUSH hbr,
    _Out_ ULONG *piUsage
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiEnumObjects(
    _In_ HDC hdc,
    _In_ LONG iObjectType,
    _In_ ULONG cjBuf,
    _Out_writes_bytes_opt_(cjBuf) PVOID pvBuf
    );

//
// NtGdiResetDC
//
// The actual size of the buffer at pdm is pdm->dmSize + pdm->dmDriverExtra.
// But this can not be specified with current annotation language.
//

typedef struct _DRIVER_INFO_2W DRIVER_INFO_2W;

#ifdef _PREFAST_
typedef struct _UMDHPDEV UMDHPDEV, *PUMDHPDEV;
#endif

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiResetDC(
    _In_ HDC hdc,
    _In_ LPDEVMODEW pdm,
    _Out_ PBOOL pbBanding,
    _In_opt_ DRIVER_INFO_2W *pDriverInfo2,
    _At_((PUMDHPDEV *)ppUMdhpdev, _Out_) VOID *ppUMdhpdev
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiSetBoundsRect(
    _In_ HDC hdc,
    _In_ PRECT prc,
    _In_ ULONG f
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetColorAdjustment(
    _In_ HDC hdc,
    _Out_ PCOLORADJUSTMENT pcaOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetColorAdjustment(
    _In_ HDC hdc,
    _In_ PCOLORADJUSTMENT pca
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiCancelDC(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
HDC
NTAPI
NtGdiOpenDCW(
    _In_opt_ PUNICODE_STRING pustrDevice,
    _In_ DEVMODEW *pdm,
    _In_ PUNICODE_STRING pustrLogAddr,
    _In_ ULONG iType,
    _In_ BOOL bDisplay,
    _In_ BOOL bSkipRedirection,
    _In_opt_ HANDLE hspool,
    _In_opt_ DRIVER_INFO_2W *pDriverInfo2,
    _At_((PUMDHPDEV *)pUMdhpdev, _Out_) VOID *pUMdhpdev
    );

/**
 * NtGdiGetDCULONG contract notes.
 *
 * Historical declaration: not found in the inspected x64/x86 win32u exports or matching
 * WIN32KBASE/WIN32KFULL names. Availability on other versions/modules is not established by this review.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetDCULONG(
    _In_ HDC hdc,
    _In_ ULONG u,
    _Out_ ULONG *Result
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetDCPoint(
    _In_ HDC hdc,
    _In_ ULONG iPoint,
    _Out_ PPOINTL pptOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiScaleViewportExtEx(
    _In_ HDC hdc,
    _In_ LONG xNum,
    _In_ LONG xDenom,
    _In_ LONG yNum,
    _In_ LONG yDenom,
    _Out_opt_ PSIZE pszOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiScaleWindowExtEx(
    _In_ HDC hdc,
    _In_ LONG xNum,
    _In_ LONG xDenom,
    _In_ LONG yNum,
    _In_ LONG yDenom,
    _Out_opt_ PSIZE pszOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetVirtualResolution(
    _In_ HDC hdc,
    _In_ LONG cxVirtualDevicePixel,
    _In_ LONG cyVirtualDevicePixel,
    _In_ LONG cxVirtualDeviceMm,
    _In_ LONG cyVirtualDeviceMm
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetSizeDevice(
    _In_ HDC hdc,
    _In_ LONG cxVirtualDevice,
    _In_ LONG cyVirtualDevice
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetTransform(
    _In_ HDC hdc,
    _In_ ULONG iXform,
    _Out_ LPXFORM pxf
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiModifyWorldTransform(
    _In_ HDC hdc,
    _In_opt_ LPXFORM pxf,
    _In_ ULONG iXform
    );

/**
 * NtGdiCombineTransform contract notes.
 *
 * Combines two affine transforms into caller-owned output storage. The XFORM members are floating-point data
 * in memory; this entry does not modify a DC or retain the source pointers.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiCombineTransform(
    _Out_ LPXFORM pxfDst,
    _In_ LPXFORM pxfSrc1,
    _In_ LPXFORM pxfSrc2
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiTransformPoints(
    _In_ HDC hdc,
    _In_reads_(c) PPOINT pptIn,
    _Out_writes_(c) PPOINT pptOut,
    _In_ LONG c,
    _In_ LONG iMode
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiConvertMetafileRect(
    _In_ HDC hdc,
    _Inout_ PRECTL prect
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetTextCharsetInfo(
    _In_ HDC hdc,
    _Out_opt_ LPFONTSIGNATURE lpSig,
    _In_ ULONG dwFlags
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiDoBanding(
    _In_ HDC hdc,
    _In_ BOOL bStart,
    _Out_ POINTL *pptl,
    _Out_ PSIZE pSize
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetPerBandInfo(
    _In_ HDC hdc,
    _Inout_ PERBANDINFO *ppbi
    );

#define GS_NUM_OBJS_ALL    0
#define GS_HANDOBJ_CURRENT 1
#define GS_HANDOBJ_MAX     2
#define GS_HANDOBJ_ALLOC   3
#define GS_LOOKASIDE_INFO  4

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetStats(
    _In_ HANDLE hProcess,
    _In_ LONG iIndex,
    _In_ LONG iPidType,
    _Out_writes_bytes_(cjResultSize) PVOID pResults,
    _In_ ULONG cjResultSize
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetMagicColors(
    _In_ HDC hdc,
    _In_ PALETTEENTRY peMagic,
    _In_ ULONG Index
    );

/**
 * NtGdiSelectBrush contract notes.
 *
 * Selects a brush into a DC and returns the previously selected brush. Save that borrowed handle and restore
 * it before deleting the brush created by the caller.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBRUSH
NTAPI
NtGdiSelectBrush(
    _In_ HDC hdc,
    _In_ HBRUSH hbrush
    );

/**
 * NtGdiSelectPen contract notes.
 *
 * Selects a pen into a DC and returns the previous selection for later restoration. Selection does not
 * transfer caller ownership of the new pen.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HPEN
NTAPI
NtGdiSelectPen(
    _In_ HDC hdc,
    _In_ HPEN hpen
    );

/**
 * NtGdiSelectBitmap contract notes.
 *
 * Selects a bitmap into a memory DC and returns the previous bitmap. Restore the old selection before
 * deleting an owned bitmap. The returned previous bitmap can be a stock/default object and must not be
 * deleted as caller-owned.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiSelectBitmap(
    _In_ HDC hdc,
    _In_ HBITMAP hbm
    );

_Kernel_entry_ 
NTSYSCALLAPI
HFONT
NTAPI
NtGdiSelectFont(
    _In_ HDC hdc,
    _In_ HFONT hf
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiExtSelectClipRgn(
    _In_     HDC hdc,
    _In_opt_ HRGN hrgn,
    _In_     LONG iMode
    );

/**
 * NtGdiCreatePen contract notes.
 *
 * Creates a logical pen from style, width and color. The returned GDI handle must be deselected before
 * deletion; an optional client brush is not required for ordinary creation.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HPEN
NTAPI
NtGdiCreatePen(
    _In_ LONG iPenStyle,
    _In_ LONG iPenWidth,
    _In_ COLORREF cr,
    _In_opt_ HBRUSH hbr
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiBitBlt(
    _In_ HDC hdcDst,
    _In_ LONG x,
    _In_ LONG y,
    _In_ LONG cx,
    _In_ LONG cy,
    _In_opt_ HDC hdcSrc,
    _In_ LONG xSrc,
    _In_ LONG ySrc,
    _In_ ULONG rop4,
    _In_ ULONG crBackColor,
    _In_ FLONG fl
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiTransparentBlt(
    _In_ HDC hdcDst,
    _In_ LONG xDst,
    _In_ LONG yDst,
    _In_ LONG cxDst,
    _In_ LONG cyDst,
    _In_ HDC hdcSrc,
    _In_ LONG xSrc,
    _In_ LONG ySrc,
    _In_ LONG cxSrc,
    _In_ LONG cySrc,
    _In_ COLORREF TransColor
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetTextExtent(
    _In_ HDC hdc,
    _In_reads_(cwc) PWSTR lpwsz,
    _In_ LONG cwc,
    _Out_ PSIZE psize,
    _In_ ULONG flOpts
    );

/**
 * NtGdiGetTextMetricsW contract notes.
 *
 * Retrieves the selected font metrics using the internal record, not just TEXTMETRICW. On the inspected
 * build cj must be at least 68; success copies exactly 68 bytes and leaves any larger buffer tail unchanged.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetTextMetricsW(
    _In_ HDC hdc,
    _Out_writes_bytes_(cj) TMW_INTERNAL * ptm,
    _In_ ULONG cj
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetTextFaceW(
    _In_ HDC hdc,
    _In_ LONG cChar,
    _Out_writes_to_opt_(cChar, return) PWSTR pszOut,
    _In_ BOOL bAliasName
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetRandomRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ LONG iRgn
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiExtTextOutW(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y,
    _In_ ULONG flOpts,
    _In_opt_ PRECT prcl,
    _In_reads_opt_(cwc) PWSTR pwsz,
    _In_range_(0, 0xffff) LONG cwc,
    _In_reads_opt_(_Inexpressible_(cwc)) LPINT pdx,
    _In_ ULONG dwCodePage
    );

/**
 * NtGdiIntersectClipRect contract notes.
 *
 * Intersects the DC clipping region with the supplied rectangle and returns a region-complexity code or
 * ERROR. Use SaveDC/RestoreDC to scope temporary clipping changes.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiIntersectClipRect(
    _In_ HDC hdc,
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom
    );

/**
 * NtGdiCreateRectRgn contract notes.
 *
 * Low-level region creation is not the complete GDI32 CreateRectRgn protocol. The inspected kernel allocates
 * a user attribute block without the GDI32 initialization; direct use of the returned handle in region
 * queries can fail until the client state is initialized. GDI32 initializes the 24-byte attribute record
 * (flags 0x30, complexity, rectangle). Coordinates are restricted to -134217728..134217727. Prefer the
 * supported Win32 constructor unless implementing the complete client protocol.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HRGN
NTAPI
NtGdiCreateRectRgn(
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom
    );

/**
 * NtGdiPatBlt contract notes.
 *
 * Fills a rectangle using the selected brush and a raster-operation code. Source-independent operations such
 * as PATCOPY require no source DC; the destination remains a mutable drawing surface.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPatBlt(
    _In_ HDC hdcDst,
    _In_ LONG x,
    _In_ LONG y,
    _In_ LONG cx,
    _In_ LONG cy,
    _In_ ULONG rop4
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPolyPatBlt(
    _In_ HDC hdc,
    _In_ ULONG rop4,
    _In_reads_(Count) PPOLYPATBLT pPoly,
    _In_ ULONG Count,
    _In_ ULONG Mode
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiUnrealizeObject(
    _In_ HANDLE h
    );

/**
 * NtGdiCreateCompatibleBitmap contract notes.
 *
 * Creates an owned bitmap compatible with a DC. Compatibility depends on the bitmap/device currently
 * associated with that DC, not just the requested dimensions.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiCreateCompatibleBitmap(
    _In_ HDC hdc,
    _In_ LONG cx,
    _In_ LONG cy
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiCreateBitmapFromDxSurface(
    _In_ HDC hdc,
    _In_ ULONG uiWidth,
    _In_ ULONG uiHeight,
    _In_ ULONG Format,
    _In_opt_ HANDLE hDxSharedSurface
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiCreateBitmapFromDxSurface2(
    _In_ HDC hdc,
    _In_ ULONG uiWidth,
    _In_ ULONG uiHeight,
    _In_ ULONG Format,
    _In_ ULONG SubresourceIndex,
    _In_ BOOL bSharedSurfaceNtHandle,
    _In_opt_ HANDLE hDxSharedSurface
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiBeginGdiRendering(
    _In_ HBITMAP hbm,
    _In_ BOOL bDiscard,
    _In_ PVOID KernelModeDeviceHandle
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEndGdiRendering(
    _In_ HBITMAP hbm,
    _In_ BOOL bDiscard,
    _Out_ BOOL* pbDeviceRemoved,
    _In_ PVOID KernelModeDeviceHandle
    );

/**
 * NtGdiLineTo contract notes.
 *
 * Adds or draws a line from the current position to the specified endpoint and updates the position. Whether
 * it records a path or renders depends on the DC path-bracket state.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiLineTo(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y
    );

/**
 * NtGdiMoveTo contract notes.
 *
 * Changes the current DC position and optionally returns the previous POINT. In a path bracket it
 * establishes the next figure start without drawing a line.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiMoveTo(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y,
    _Out_opt_ PPOINT pptOut
    );

/**
 * NtGdiExtGetObjectW contract notes.
 *
 * Copies object-specific GDI metadata into caller storage and returns the byte count. The record type
 * depends on the handle kind; bitmap metadata can contain a borrowed pixel pointer rather than an allocation
 * to free.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiExtGetObjectW(
    _In_ HANDLE h,
    _In_ LONG cj,
    _Out_writes_bytes_opt_(cj) PVOID pvOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetDeviceCaps(
    _In_ HDC hdc,
    _In_ LONG i
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetDeviceCapsAll (
    _In_opt_ HDC hdc,
    _Out_ PDEVCAPS pDevCaps
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiStretchBlt(
    _In_ HDC hdcDst,
    _In_ LONG xDst,
    _In_ LONG yDst,
    _In_ LONG cxDst,
    _In_ LONG cyDst,
    _In_opt_ HDC hdcSrc,
    _In_ LONG xSrc,
    _In_ LONG ySrc,
    _In_ LONG cxSrc,
    _In_ LONG cySrc,
    _In_ ULONG dwRop,
    _In_ ULONG dwBackColor
    );

/**
 * NtGdiSetBrushOrg contract notes.
 *
 * Changes the DC brush origin and optionally returns its previous POINT. NULL pptOut is supported; non-NULL
 * output is written only after a successful update.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetBrushOrg(
    _In_ HDC hdc,
    _In_ LONG x,
    _In_ LONG y,
    _Out_opt_ PPOINT pptOut
    );

/**
 * NtGdiCreateBitmap contract notes.
 *
 * Creates a device-dependent bitmap from optional packed source bits. The input extent is bytes: each scan
 * line is padded to a 16-bit boundary and the padded bit count is divided by 8 before multiplication by
 * height. For example a 3 by 2, 1-plane, 1-bpp bitmap consumes 4 bytes, not 32.
 */
_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiCreateBitmap(
    _In_ LONG cx,
    _In_ LONG cy,
    _In_ ULONG cPlanes,
    _In_ ULONG cBPP,
    _In_reads_bytes_opt_(((((ULONG64)cx * cPlanes * cBPP + 15) & ~15ull) / 8) * cy) PBYTE pjInit
    );

_Kernel_entry_ 
NTSYSCALLAPI
HPALETTE
NTAPI
NtGdiCreateHalftonePalette(
    _In_ HDC hdc
    );

/**
 * NtGdiRestoreDC contract notes.
 *
 * Restores a saved DC state by level, including selected objects and clipping state. Restoring state does
 * not delete caller-owned brushes, pens or bitmaps.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRestoreDC(
    _In_ HDC hdc,
    _In_ LONG iLevel
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiExcludeClipRect(
    _In_ HDC hdc,
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom
    );

/**
 * NtGdiSaveDC contract notes.
 *
 * Pushes DC drawing state and returns a positive saved-state level on success. Pair it with NtGdiRestoreDC;
 * this is not a kernel synchronization or object handle.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiSaveDC(
    _In_ HDC hdc
    );

/**
 * NtGdiCombineRgn contract notes.
 *
 * Combines source geometry into an existing destination using the RGN_* mode. The return is the resulting
 * complexity or ERROR, not BOOL/NTSTATUS. Keep all required source/destination handles valid for the
 * operation.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiCombineRgn(
    _In_ HRGN hrgnDst,
    _In_ HRGN hrgnSrc1,
    _In_ HRGN hrgnSrc2,
    _In_ LONG iMode
    );

/**
 * NtGdiSetRectRgn contract notes.
 *
 * Replaces the geometry of a valid region with a rectangle. This is a mutation of the referenced GDI object;
 * it does not allocate a new region. It is not a substitute for missing user-attribute initialization after
 * bare NtGdiCreateRectRgn.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetRectRgn(
    _In_ HRGN hrgn,
    _In_ LONG xLeft,
    _In_ LONG yTop,
    _In_ LONG xRight,
    _In_ LONG yBottom
    );

/**
 * NtGdiSetBitmapBits contract notes.
 *
 * Copies counted packed source bytes into a bitmap and returns the number transferred. The storage is
 * caller-owned and not retained by this entry. Use the bitmap's scan-line layout rather than assuming DIB
 * DWORD padding.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiSetBitmapBits(
    _In_ HBITMAP hbm,
    _In_ ULONG cj,
    _In_reads_bytes_(cj) PBYTE pjInit
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetDIBitsInternal(
    _In_ HDC hdc,
    _In_ HBITMAP hbm,
    _In_ ULONG iStartScan,
    _In_ ULONG cScans,
    _Out_writes_bytes_opt_(cjMaxBits) PBYTE pBits,
    _Inout_ LPBITMAPINFO pbmi,
    _In_ ULONG iUsage,
    _In_ ULONG cjMaxBits,
    _In_ ULONG cjMaxInfo
    );

/**
 * NtGdiOffsetRgn contract notes.
 *
 * Translates an existing region and returns its resulting complexity or ERROR. The offsets are signed
 * coordinates and the handle remains the same object.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiOffsetRgn(
    _In_ HRGN hrgn,
    _In_ LONG cx,
    _In_ LONG cy
    );

/**
 * NtGdiGetRgnBox contract notes.
 *
 * Returns a region-complexity code and copies a 16-byte bounding RECT on success. ERROR (zero) is failure;
 * NULLREGION, SIMPLEREGION and COMPLEXREGION are not NTSTATUS values. A valid initialized region is
 * required.
 */
_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetRgnBox(
    _In_ HRGN hrgn,
    _Out_ PRECT prcOut
    );

/**
 * NtGdiRectInRegion contract notes.
 *
 * Tests rectangle intersection and, on a TRUE result, copies the normalized rectangle back to the caller.
 * The RECT is deliberately input/output, not const input. A nonintersecting rectangle can remain unchanged,
 * including reversed coordinates.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiRectInRegion(
    _In_ HRGN hrgn,
    _Inout_ PRECT prcl
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetBoundsRect(
    _In_ HDC hdc,
    _Out_ PRECT prc,
    _In_ ULONG f
    );

/**
 * NtGdiPtInRegion contract notes.
 *
 * Tests point membership in a region. Ordinary rectangular membership excludes the right and bottom
 * boundaries. The result is BOOL, not a region-complexity code.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPtInRegion(
    _In_ HRGN hrgn,
    _In_ LONG x,
    _In_ LONG y
    );

_Kernel_entry_ 
NTSYSCALLAPI
COLORREF
NTAPI
NtGdiGetNearestColor(
    _In_ HDC hdc,
    _In_ COLORREF cr
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetSystemPaletteUse(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiSetSystemPaletteUse(
    _In_ HDC hdc,
    _In_ ULONG ui
    );

/**
 * NtGdiGetRegionData contract notes.
 *
 * Serializes a region as RGNDATAHEADER followed by rectangles. NULL output performs a byte-size query. With
 * output storage, the successful byte count controls initialized data; do not infer that the entire supplied
 * capacity was written.
 */
_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetRegionData(
    _In_ HRGN hrgn,
    _In_ ULONG nCount,
    _Out_writes_bytes_to_opt_(nCount, return) LPRGNDATA lpRgnData
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiInvertRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn
    );

//
// MISC FONT API's
//

_Kernel_entry_ 
LONG 
NTSYSCALLAPI
NTAPI
NtGdiAddFontResourceW(
    _In_reads_(cwc) WCHAR *pwszFiles,
    _In_ ULONG cwc,
    _In_ ULONG cFiles,
    _In_ FLONG f,
    _In_ ULONG dwPidTid,
    _In_opt_ DESIGNVECTOR *pdv
    );

_Kernel_entry_ 
NTSYSCALLAPI
HFONT
NTAPI
NtGdiHfontCreate(
    _In_reads_bytes_(cjElfw) ENUMLOGFONTEXDVW *pelfw,
    _In_ ULONG cjElfw,
    _In_ LFTYPE lft,
    _In_ FLONG  fl,
    _In_ PVOID pvCliData
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiSetFontEnumeration(
    _In_ ULONG ulType
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEnumFonts(
    _In_ HDC hdc,
    _In_ ULONG iEnumType,
    _In_ FLONG flWin31Compat,
    _In_ ULONG cchFaceName,
    _In_reads_opt_(cchFaceName) LPCWSTR pwszFaceName,
    _In_ ULONG lfCharSet,
    _Inout_ ULONG *pulCount,
    _Out_writes_bytes_opt_(*pulCount) void * pvUserModeBuffer
    );

#define TYPE_ENUMFONTS          1
#define TYPE_ENUMFONTFAMILIES   2
#define TYPE_ENUMFONTFAMILIESEX 3

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiQueryFonts(
    _Out_writes_(nBufferSize) PUNIVERSAL_FONT_ID pufiFontList,
    _In_ ULONG nBufferSize,
    _Out_ PLARGE_INTEGER pTimeStamp
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetCharSet(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEnableEudc(
    _In_ BOOL Enable
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEudcLoadUnloadLink(
    _In_reads_opt_(cwcBaseFaceName) LPCWSTR pBaseFaceName,
    _In_ ULONG cwcBaseFaceName,
    _In_reads_(cwcEudcFontPath) LPCWSTR pEudcFontPath,
    _In_ ULONG cwcEudcFontPath,
    _In_ LONG iPriority,
    _In_ LONG iFontLinkType,
    _In_ BOOL bLoadLin
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetStringBitmapW(
    _In_ HDC hdc,
    _In_ PWSTR pwsz,
    _In_ ULONG cwc,
    _In_ ULONG cj,
    _Out_writes_bytes_(cj) BYTE *lpSB
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetEudcTimeStampEx(
    _In_reads_opt_(cwcBaseFaceName) PWSTR lpBaseFaceName,
    _In_ ULONG cwcBaseFaceName,
    _In_ BOOL bSystemTimeStamp
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiQueryFontAssocInfo(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetFontUnicodeRanges(
    _In_ HDC hdc,
    _Out_writes_bytes_opt_(return) LPGLYPHSET pgs
    );

/**
 * NtGdiGetRealizationInfo contract notes.
 *
 * Reads the caller's size field before producing font-realization metadata, so pri is input/output. The
 * inspected native entry accepts record sizes 16, 24 and 812 bytes and copies the selected extent on
 * success. Do not advertise a larger size than the actual backing allocation.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiGetRealizationInfo(
    _In_ HDC hdc,
    _Inout_ PFONT_REALIZATION_INFO2 pri
    );

typedef struct tagDOWNLOADDESIGNVECTOR 
{
    UNIVERSAL_FONT_ID ufiBase;
    DESIGNVECTOR dv;
} DOWNLOADDESIGNVECTOR;

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiAddRemoteMMInstanceToDC(
    _In_ HDC hdc,
    _In_reads_bytes_(cjDDV) DOWNLOADDESIGNVECTOR *pddv,
    _In_ ULONG cjDDV
    );

//
// user-mode printer support
//

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiUnloadPrinterDriver(
    _In_reads_bytes_(cbDriverName) PWSTR pDriverName,
    _In_ ULONG cbDriverName
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngAssociateSurface(
    _In_ HSURF hsurf,
    _In_ HDEV hdev,
    _In_ FLONG flHooks
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngEraseSurface(
    _In_ SURFOBJ *pso,
    _In_ RECTL *prcl,
    _In_ ULONG iColor
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiEngCreateBitmap(
    _In_ SIZEL sizl,
    _In_ LONG lWidth,
    _In_ ULONG iFormat,
    _In_ FLONG fl,
    _In_opt_ PVOID pvBits
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngDeleteSurface(
    _In_ HSURF hsurf
    );

_Kernel_entry_ 
NTSYSCALLAPI
SURFOBJ*
NTAPI
NtGdiEngLockSurface(
    _In_ HSURF hsurf
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiEngUnlockSurface(
    _In_ SURFOBJ *
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngMarkBandingSurface(
    _In_ HSURF hsurf
    );

_Kernel_entry_ 
NTSYSCALLAPI
HSURF
NTAPI
NtGdiEngCreateDeviceSurface(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiEngCreateDeviceBitmap(
    _In_ DHSURF dhsurf,
    _In_ SIZEL sizl,
    _In_ ULONG iFormatCompat
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngCopyBits(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ POINTL *pptlSrc
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngStretchBlt(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlHTOrg,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_ POINTL *pptlMask,
    _In_ ULONG iMode
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngBitBlt(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ POINTL *pptlSrc,
    _In_ POINTL *pptlMask,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrush,
    _In_ ROP4 rop4
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngPlgBlt(
    _In_ SURFOBJ *psoTrg,
    _In_ SURFOBJ *psoSrc,
    _In_opt_ SURFOBJ *psoMsk,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlBrushOrg,
    _In_ POINTFIX *pptfxDest,
    _In_ RECTL *prclSrc,
    _In_opt_ POINTL *pptlMask,
    _In_ ULONG iMode
    );

_Kernel_entry_ 
NTSYSCALLAPI
HPALETTE
NTAPI
NtGdiEngCreatePalette(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_ ULONG *pulColors,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngDeletePalette(
    _In_ HPALETTE hPal
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngStrokePath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ LINEATTRS *plineattrs,
    _In_ MIX mix
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngStrokeAndFillPath(
    _In_ SURFOBJ *pso,
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ XFORMOBJ *pxo,
    _In_ BRUSHOBJ *pboStroke,
    _In_ LINEATTRS *plineattrs,
    _In_ BRUSHOBJ *pboFill,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix,
    _In_ FLONG flOptions
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngPaint(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ MIX mix
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngLineTo(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ LONG x1,
    _In_ LONG y1,
    _In_ LONG x2,
    _In_ LONG y2,
    _In_ RECTL *prclBounds,
    _In_ MIX mix
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngAlphaBlend(
    _In_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_ BLENDOBJ *pBlendObj
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngGradientFill(
    _In_ SURFOBJ *psoDest,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_reads_(nVertex) TRIVERTEX *pVertex,
    _In_ ULONG nVertex,
    _In_reads_(nMesh) PVOID pMesh,
    _In_ ULONG nMesh,
    _In_ RECTL *prclExtents,
    _In_ POINTL *pptlDitherOrg,
    _In_ ULONG ulMode
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngTransparentBlt(
    _In_ SURFOBJ *psoDst,
    _In_ SURFOBJ *psoSrc,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDst,
    _In_ RECTL *prclSrc,
    _In_ ULONG iTransColor,
    _In_ ULONG ulReserved
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngTextOut(
    _In_ SURFOBJ *pso,
    _In_ STROBJ *pstro,
    _In_ FONTOBJ *pfo,
    _In_ CLIPOBJ *pco,
    _In_ RECTL *prclExtra,
    _In_ RECTL *prclOpaque,
    _In_ BRUSHOBJ *pboFore,
    _In_ BRUSHOBJ *pboOpaque,
    _In_ POINTL *pptlOrg,
    _In_ MIX mix
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngStretchBltROP(
    _In_ SURFOBJ *psoTrg,
    _In_ SURFOBJ *psoSrc,
    _In_ SURFOBJ *psoMask,
    _In_ CLIPOBJ *pco,
    _In_ XLATEOBJ *pxlo,
    _In_ COLORADJUSTMENT *pca,
    _In_ POINTL *pptlBrushOrg,
    _In_ RECTL *prclTrg,
    _In_ RECTL *prclSrc,
    _In_ POINTL *pptlMask,
    _In_ ULONG iMode,
    _In_ BRUSHOBJ *pbo,
    _In_ ROP4 rop4
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiXLATEOBJ_cGetPalette(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iPal,
    _In_ ULONG cPal,
    _Out_writes_(cPal) ULONG *pPal
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiCLIPOBJ_cEnumStart(
    _In_ CLIPOBJ *pco,
    _In_ BOOL bAll,
    _In_ ULONG iType,
    _In_ ULONG iDirection,
    _In_ ULONG cLimit
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiCLIPOBJ_bEnum(
    _In_ CLIPOBJ *pco,
    _In_ ULONG cj,
    _Out_writes_bytes_(cj) ULONG *pul
    );

_Kernel_entry_ 
NTSYSCALLAPI
PATHOBJ*
NTAPI
NtGdiCLIPOBJ_ppoGetPath(
    _In_ CLIPOBJ *pco
    );

_Kernel_entry_ 
NTSYSCALLAPI
CLIPOBJ*
NTAPI
NtGdiEngCreateClip(
    VOID
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiEngDeleteClip(
    _In_ CLIPOBJ*pco
    );

_Kernel_entry_ 
NTSYSCALLAPI
PVOID
NTAPI
NtGdiBRUSHOBJ_pvAllocRbrush(
    _In_ BRUSHOBJ *pbo,
    _In_ ULONG cj
    );

_Kernel_entry_ 
NTSYSCALLAPI
PVOID
NTAPI
NtGdiBRUSHOBJ_pvGetRbrush(
    _In_ BRUSHOBJ *pbo
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiBRUSHOBJ_ulGetBrushColor(
    _In_ BRUSHOBJ *pbo
    );

_Kernel_entry_ 
NTSYSCALLAPI
HANDLE
NTAPI
NtGdiBRUSHOBJ_hGetColorTransform(
    _In_ BRUSHOBJ *pbo
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiXFORMOBJ_bApplyXform(
    _In_ XFORMOBJ *pxo,
    _In_ ULONG iMode,
    _In_ ULONG cPoints,
    _In_reads_(cPoints) POINTL *pvIn,
    _Out_writes_(cPoints) POINTL *pvOut
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiXFORMOBJ_iGetXform(
    _In_ XFORMOBJ *pxo,
    _Out_opt_ XFORML *pxform
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiFONTOBJ_vGetInfo(
    _In_ FONTOBJ *pfo,
    _In_ ULONG cjSize,
    _Out_writes_bytes_(cjSize) FONTINFO *pfi
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiFONTOBJ_cGetGlyphs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode,
    _In_ ULONG cGlyph,
    _In_ HGLYPH *phg,
    _At_((GLYPHDATA **)ppvGlyph, _Outptr_) PVOID *ppvGlyph
    );

_Kernel_entry_ 
NTSYSCALLAPI
XFORMOBJ*
NTAPI
NtGdiFONTOBJ_pxoGetXform(
    _In_ FONTOBJ *pfo
    );

_Kernel_entry_ 
NTSYSCALLAPI
IFIMETRICS*
NTAPI
NtGdiFONTOBJ_pifi(
    _In_ FONTOBJ *pfo
    );

_Kernel_entry_ 
NTSYSCALLAPI
FD_GLYPHSET*
NTAPI
NtGdiFONTOBJ_pfdg(
    _In_ FONTOBJ *pfo
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiFONTOBJ_cGetAllGlyphHandles(
    _In_ FONTOBJ *pfo,
    _Out_writes_opt_(return) HGLYPH *phg
    );

_Kernel_entry_ 
NTSYSCALLAPI
PVOID
NTAPI
NtGdiFONTOBJ_pvTrueTypeFontFile(
    _In_ FONTOBJ *pfo,
    _Out_ ULONG *pcjFile
    );

_Kernel_entry_ 
NTSYSCALLAPI
PFD_GLYPHATTR
NTAPI
NtGdiFONTOBJ_pQueryGlyphAttrs(
    _In_ FONTOBJ *pfo,
    _In_ ULONG iMode
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSTROBJ_bEnum(
    _In_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Outptr_result_buffer_(*pc) PGLYPHPOS *ppgpos
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSTROBJ_bEnumPositionsOnly(
    _In_ STROBJ *pstro,
    _Out_ ULONG *pc,
    _Outptr_result_buffer_(*pc) PGLYPHPOS *ppgpos
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiSTROBJ_vEnumStart(
    _In_ STROBJ *pstro
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiSTROBJ_dwGetCodePage(
    _In_ STROBJ *pstro
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSTROBJ_bGetAdvanceWidths(
    _In_ STROBJ*pstro,
    _In_ ULONG iFirst,
    _In_ ULONG c,
    _Out_writes_(c) POINTQF*pptqD
    );

_Kernel_entry_ 
NTSYSCALLAPI
FD_GLYPHSET*
NTAPI
NtGdiEngComputeGlyphSet(
    _In_ LONG nCodePage,
    _In_ LONG nFirstChar,
    _In_ LONG cChars
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiXLATEOBJ_iXlate(
    _In_ XLATEOBJ *pxlo,
    _In_ ULONG iColor
    );

_Kernel_entry_ 
NTSYSCALLAPI
HANDLE
NTAPI
NtGdiXLATEOBJ_hGetColorTransform(
    _In_ XLATEOBJ *pxlo
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiPATHOBJ_vGetBounds(
    _In_ PATHOBJ *ppo,
    _Out_ PRECTFX prectfx
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPATHOBJ_bEnum(
    _In_ PATHOBJ *ppo,
    _Out_ PATHDATA *ppd
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiPATHOBJ_vEnumStart(
    _In_ PATHOBJ *ppo
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiEngDeletePath(
    _In_ PATHOBJ *ppo
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiPATHOBJ_vEnumStartClipLines(
    _In_ PATHOBJ *ppo,
    _In_ CLIPOBJ *pco,
    _In_ SURFOBJ *pso,
    _In_ LINEATTRS *pla
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiPATHOBJ_bEnumClipLines(
    _In_ PATHOBJ *ppo,
    _In_ ULONG cb,
    _Out_writes_bytes_(cb) CLIPLINE *pcl
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiEngCheckAbort(
    _In_ SURFOBJ *pso
    );

_Kernel_entry_ 
NTSYSCALLAPI
DHPDEV
NTAPI
NtGdiGetDhpdev(
    _In_ HDEV hdev
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiHT_Get8BPPFormatPalette(
    _Out_writes_opt_(return) LPPALETTEENTRY pPaletteEntry,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiHT_Get8BPPMaskPalette(
    _Out_writes_opt_(return) LPPALETTEENTRY pPaletteEntry,
    _In_ BOOL Use8BPPMaskPal,
    _In_ BYTE CMYMask,
    _In_ USHORT RedGamma,
    _In_ USHORT GreenGamma,
    _In_ USHORT BlueGamma
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiUpdateTransform(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiSetLayout(
    _In_ HDC hdc,
    _In_ LONG wox,
    _In_ ULONG dwLayout
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiMirrorWindowOrg(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetDeviceWidth(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiSetUMPDSandboxState(
    _In_ BOOL bEnabled
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiSetPUMPDOBJ(
    _In_opt_ HUMPD humpd,
    _In_ BOOL bStoreID,
    _Inout_opt_ HUMPD *phumpd,
    _Out_opt_ BOOL *pbWOW64
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiBRUSHOBJ_DeleteRbrush(
    _In_opt_ BRUSHOBJ *pbo,
    _In_opt_ BRUSHOBJ *pboB
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiUMPDEngFreeUserMem(
    _In_ KERNEL_PVOID *ppv
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiSetBitmapAttributes(
    _In_ HBITMAP hbm,
    _In_ ULONG dwFlags
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBITMAP
NTAPI
NtGdiClearBitmapAttributes(
    _In_ HBITMAP hbm,
    _In_ ULONG dwFlags
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBRUSH
NTAPI
NtGdiSetBrushAttributes(
    _In_ HBRUSH hbm,
    _In_ ULONG dwFlags
    );

_Kernel_entry_ 
NTSYSCALLAPI
HBRUSH
NTAPI
NtGdiClearBrushAttributes(
    _In_ HBRUSH hbr,
    _In_ ULONG dwFlags
    );

//
// Private draw stream interface
//

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiDrawStream(
    _In_ HDC hdcDst,
    _In_ ULONG cjIn,
    _In_reads_bytes_(cjIn) VOID *pvIn
    );

//
// Private Xfer interfaces
//

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI NtGdiMakeObjectXferable(
    _In_ HANDLE h, 
    _In_ ULONG dwProcessId
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI NtGdiMakeObjectUnXferable(
    _In_ HANDLE h
    );

//
// SpriteS
//

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdCreateFullscreenSprite(
    _In_ HDC hdc,
    _In_ COLORREF crKey,
    _Out_ HANDLE* phSprite,
    _Out_ HDC* phdcSprite
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdNotifyFullscreenSpriteUpdate(
    _In_ HDC hdc,
    _In_ HANDLE hSprite
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiDdDestroyFullscreenSprite(
    _In_ HDC hdc,
    _In_ HANDLE hSprite
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiDdQueryVisRgnUniqueness(
    VOID
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI NtGdiHLSurfGetInformation(
    _In_ HLSURF hLsurf,
    _In_ HLSURF_INFORMATION_CLASS InformationClass,
    _In_reads_bytes_opt_(*pulInformationLength) PVOID InformationBuffer,
    _Inout_ PULONG pulInformationLength
    );

_Kernel_entry_
NTSYSCALLAPI
BOOL
NTAPI
NtGdiHLSurfSetInformation(
    _In_ HLSURF hLsurf,
    _In_ HLSURF_INFORMATION_CLASS InformationClass,
    _In_reads_bytes_opt_(InformationLength) PVOID InformationBuffer,
    _In_ ULONG InformationLength
    );

_Kernel_entry_
BOOL
NTAPI
NtGdiDwmCreatedBitmapRemotingOutput(
    VOID
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetCurrentDpiInfo(
    _In_ HMONITOR hmon,
    _Out_ DPI_INFORMATION* dpiInfo
    );

_Must_inspect_result_
_Success_(return == STATUS_SUCCESS)
_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetProcessSessionFonts(
    _In_ HANDLE hProcess,
    _Out_writes_(*pdwFileCount) HANDLE *phFile,
    _Inout_ ULONG *pdwFileCount,
    _Out_writes_all_(*pdwFilePathsSize) PWSTR pwszFilePaths,
    _Inout_ ULONG *pdwFilePathsSize
    );

_Must_inspect_result_
_Success_(return == STATUS_SUCCESS)
_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiAddInitialFonts(
    VOID
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiGetEntry(
    _In_ LONG index,
    _Out_writes_bytes_(ENTRY_SIZE) PVOID pEntry
    );

_Kernel_entry_ 
NTSYSCALLAPI
ULONG
NTAPI
NtGdiGetPublicFontTableChangeCookie(
    VOID
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiScaleRgn(
    _In_ HDC hdc, 
    _In_ HRGN hrgn
    );

/**
 * NtGdiScaleValues contract notes.
 *
 * Applies the DC DPI scaling to a writable array of signed LONGs. The inspected entry copies cl*4 input
 * bytes into temporary storage and copies the transformed values back on success, so pl is input/output. A
 * scale factor of one leaves the values unchanged.
 */
_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiScaleValues(
    _In_ HDC hdc,
    _Inout_updates_(cl) LPLONG pl,
    _In_ ULONG cl
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG 
NTAPI
NtGdiGetDCDpiScaleValue(
    _In_ HDC hdc
    );

_Kernel_entry_ 
NTSYSCALLAPI
LONG
NTAPI
NtGdiGetBitmapDpiScaleValue(
    _In_ HBITMAP hbm
    );

_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiEnsureDpiDepDefaultGuiFontForPlateau(
    _In_ LONG iDpi
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiWaitForTextReady(
    VOID
    );

_Kernel_entry_ 
NTSYSCALLAPI
BOOL
NTAPI
NtGdiDisableUMPDSandboxing(
    VOID
    );

/**
 * NtGdiIsDcInXfer contract notes.
 *
 * This declaration was not located in the inspected x64/x86 win32u exports or matching WIN32KBASE/WIN32KFULL
 * names; do not treat that bounded absence as proof about every Windows version.
 */
_Kernel_entry_ 
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGdiIsDcInXfer(
    _In_ HDC hdc,
    _Out_ BOOL* DcInXfer
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif // _NTGDI_H
