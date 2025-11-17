/*
 * National Language Support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTNLS_H
#define _NTNLS_H

#define MAXIMUM_LEADBYTES 12

/**
 * Stores the NLS file formats.
 *
 * \sa https://learn.microsoft.com/en-us/previous-versions/mt791523(v=vs.85)
 */
typedef struct _CPTABLEINFO
{
    USHORT CodePage;                        // Specifies the code page number.
    USHORT MaximumCharacterSize;            // Specifies the maximum length in bytes of a character.
    USHORT DefaultChar;                     // Specifies the default character (MB).
    USHORT UniDefaultChar;                  // Specifies the default character (Unicode).
    USHORT TransDefaultChar;                // Specifies the translation of the default character (Unicode).
    USHORT TransUniDefaultChar;             // Specifies the translation of the Unicode default character (MB).
    USHORT DBCSCodePage;                    // Specifies non-zero for DBCS code pages.
    UCHAR LeadByte[MAXIMUM_LEADBYTES];      // Specifies the lead byte ranges.
    PUSHORT MultiByteTable;                 // Specifies a pointer to a MB translation table.
    PVOID WideCharTable;                    // Specifies a pointer to a WC translation table.
    PUSHORT DBCSRanges;                     // Specifies a pointer to DBCS ranges.
    PUSHORT DBCSOffsets;                    // Specifies a pointer to DBCS offsets.
} CPTABLEINFO, *PCPTABLEINFO;

/**
 * Stores the NLS file formats.
 *
 * \sa https://learn.microsoft.com/en-us/previous-versions/mt791531(v=vs.85)
 */
typedef struct _NLSTABLEINFO
{
    CPTABLEINFO OemTableInfo;               // Specifies OEM table.
    CPTABLEINFO AnsiTableInfo;              // Specifies an ANSI table.
    PUSHORT UpperCaseTable;                 // Specifies an 844 format uppercase table.
    PUSHORT LowerCaseTable;                 // Specifies an 844 format lowercase table.
} NLSTABLEINFO, *PNLSTABLEINFO;

//
// Data exports (ntdll.lib/ntdllp.lib)
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)
NTSYSAPI USHORT NlsAnsiCodePage;
NTSYSAPI BOOLEAN NlsMbCodePageTag;
NTSYSAPI BOOLEAN NlsMbOemCodePageTag;
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif // _NTNLS_H
