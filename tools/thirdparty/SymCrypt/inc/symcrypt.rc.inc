//
//    Copyright (c) Microsoft Corporation.
//    Licensed under the MIT License.
//


#include "symcrypt_internal_shared.inc"

#define VER_MAJOR SYMCRYPT_CODE_VERSION_API
#define VER_MINOR SYMCRYPT_CODE_VERSION_MINOR
#define VER_PATCH SYMCRYPT_CODE_VERSION_PATCH

#ifndef VER_BUILD_ID
#define VER_BUILD_ID 0
#endif

#ifndef VER_SUFFIX
#define VER_SUFFIX private
#endif

//
// VER_ORIGINALFILENAME_STR, VER_FILEDESCRIPTION_STR, and VER_INTERNALNAME_STR should be defined by the file
// that includes this file.
//

#define VER_COMPANYNAME_STR         "Microsoft Corporation"
#define VER_LEGALCOPYRIGHT_STR      "\251 Microsoft Corporation. All rights reserved."
#define VER_PRODUCTNAME_STR         "Microsoft\256 SymCrypt"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define VER_FILEVERSION             VER_MAJOR,VER_MINOR,VER_PATCH,0
#define VER_FILEVERSION_STR         STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_PATCH) "." STR(VER_BUILD_ID) "\0"

#define VER_PRODUCTVERSION_STR      STR(VER_MAJOR) "." STR(VER_MINOR) "." STR(VER_PATCH) "." STR(VER_BUILD_ID) "-" STR(VER_SUFFIX) "\0"

VS_VERSION_INFO                     VERSIONINFO
FILEVERSION                         VER_FILEVERSION
FILEFLAGSMASK                       VS_FFI_FILEFLAGSMASK
FILEFLAGS                           0
FILEOS                              VOS_NT_WINDOWS32
FILETYPE                            VER_FILETYPE
FILESUBTYPE                         VER_FILESUBTYPE

BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "040904B0"
        BEGIN
            VALUE "CompanyName",      VER_COMPANYNAME_STR
            VALUE "FileDescription",  VER_FILEDESCRIPTION_STR
            VALUE "FileVersion",      VER_FILEVERSION_STR
            VALUE "InternalName",     VER_INTERNALNAME_STR
            VALUE "LegalCopyright",   VER_LEGALCOPYRIGHT_STR
            VALUE "OriginalFilename", VER_ORIGINALFILENAME_STR
            VALUE "ProductName",      VER_PRODUCTNAME_STR
            VALUE "ProductVersion",   VER_PRODUCTVERSION_STR
        END
    END

    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x0409, 0x04B0
    END
END
