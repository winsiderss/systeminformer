/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 */

namespace CustomBuildTool
{
    public static unsafe partial class Zip
    {
        private static ReadOnlySpan<byte> OidSha256 => "2.16.840.1.101.3.4.2.1\0"u8;

        /// <summary>
        /// Writes a Windows catalog for the files included by <see cref="CreateCompressedFolder"/>.
        /// </summary>
        public static void CreateBuildCatalog(string SourceDirectoryName, string DestinationCatalogFileName)
        {
            string destinationCatalogPath = Path.GetFullPath(DestinationCatalogFileName);
            string catalogDirectory = Path.GetDirectoryName(destinationCatalogPath);
            Directory.CreateDirectory(catalogDirectory);

            var files = new List<(string name, string file)>();

            foreach (string file in Directory.EnumerateFiles(SourceDirectoryName, "*", SearchOption.AllDirectories))
            {
                if (string.Equals(Path.GetFullPath(file), destinationCatalogPath, StringComparison.OrdinalIgnoreCase))
                    continue;

                string name = GetEntryName(file, SourceDirectoryName, false);
                bool shouldSkip = SkipPathPrefixes.Any(prefix => name.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) ||
                    SkipExtensions.Contains(Path.GetExtension(file));

                if (shouldSkip)
                    continue;

                files.Add((name, file));
            }

            files.Sort((left, right) => StringComparer.OrdinalIgnoreCase.Compare(left.name, right.name));

            Win32.DeleteFile(destinationCatalogPath);

            fixed (char* catPathPtr = destinationCatalogPath)
            fixed (byte* oidSha256Ptr = OidSha256)
            {
                HANDLE catalogHandle = PInvoke.CryptCATOpen(
                    catPathPtr,
                    Windows.Win32.Security.Cryptography.Catalog.CRYPTCAT_OPEN_FLAGS.CRYPTCAT_OPEN_CREATENEW,
                    0,
                    Windows.Win32.Security.Cryptography.Catalog.CRYPTCAT_VERSION.CRYPTCAT_VERSION_2,
                    0x00010001 // PKCS_7_ASN_ENCODING | X509_ASN_ENCODING
                    );

                if (catalogHandle.IsNull || catalogHandle.Value == (void*)(-1))
                {
                    int error = Marshal.GetLastWin32Error();
                    throw new Win32Exception(error, $"CryptCATOpen failed for {destinationCatalogPath}: 0x{error:X8}");
                }

                try
                {
                    foreach (var (_, file) in files)
                    {
                        fixed (char* filePathPtr = file)
                        {
                            Guid subjectGuid;
                            if (!PInvoke.CryptSIPRetrieveSubjectGuidForCatalogFile(filePathPtr, HANDLE.Null, &subjectGuid))
                            {
                                int error = Marshal.GetLastWin32Error();
                                throw new Win32Exception(error, $"CryptSIPRetrieveSubjectGuidForCatalogFile failed for {file}: 0x{error:X8}");
                            }

                            var subjectInfo = new Windows.Win32.Security.Cryptography.Sip.SIP_SUBJECTINFO();
                            subjectInfo.cbSize = (uint)sizeof(Windows.Win32.Security.Cryptography.Sip.SIP_SUBJECTINFO);
                            subjectInfo.pgSubjectType = &subjectGuid;
                            subjectInfo.hFile = new HANDLE((nint)(-1));
                            subjectInfo.pwsFileName = filePathPtr;
                            subjectInfo.DigestAlgorithm.pszObjId = new PSTR(oidSha256Ptr);
                            subjectInfo.dwFlags = 0x000100A0; // MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE | SPC_INC_PE_RESOURCES_FLAG | SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG
                            subjectInfo.dwEncodingType = 0x00010001;

                            uint cbIndirectData = 0;
                            if (!PInvoke.CryptSIPCreateIndirectData(&subjectInfo, &cbIndirectData, null) || cbIndirectData == 0)
                            {
                                int error = Marshal.GetLastWin32Error();
                                throw new Win32Exception(error, $"CryptSIPCreateIndirectData query failed for {file}: 0x{error:X8}");
                            }

                            byte[] buffer = ArrayPool<byte>.Shared.Rent((int)cbIndirectData);
                            try
                            {
                                fixed (byte* pbIndirectData = buffer)
                                {
                                    var pIndirectData = (Windows.Win32.Security.Cryptography.Sip.SIP_INDIRECT_DATA*)pbIndirectData;

                                    if (!PInvoke.CryptSIPCreateIndirectData(&subjectInfo, &cbIndirectData, pIndirectData))
                                    {
                                        int error = Marshal.GetLastWin32Error();
                                        throw new Win32Exception(error, $"CryptSIPCreateIndirectData failed for {file}: 0x{error:X8}");
                                    }

                                    ReadOnlySpan<byte> digestSpan = new ReadOnlySpan<byte>(
                                        pIndirectData->Digest.pbData,
                                        (int)pIndirectData->Digest.cbData
                                        );
                                    string tag = Convert.ToHexString(digestSpan);

                                    fixed (char* tagPtr = tag)
                                    {
                                        void* pMember = PInvoke.CryptCATPutMemberInfo(
                                            catalogHandle,
                                            filePathPtr,
                                            tagPtr,
                                            &subjectGuid,
                                            subjectInfo.dwIntVersion,
                                            cbIndirectData,
                                            pbIndirectData
                                            );

                                        if (pMember == null)
                                        {
                                            int error = Marshal.GetLastWin32Error();
                                            throw new Win32Exception(error, $"CryptCATPutMemberInfo failed for {file}: 0x{error:X8}");
                                        }
                                    }
                                }
                            }
                            finally
                            {
                                ArrayPool<byte>.Shared.Return(buffer);
                            }
                        }
                    }

                    if (!PInvoke.CryptCATPersistStore(catalogHandle))
                    {
                        int error = Marshal.GetLastWin32Error();
                        throw new Win32Exception(error, $"CryptCATPersistStore failed for {destinationCatalogPath}: 0x{error:X8}");
                    }
                }
                finally
                {
                    PInvoke.CryptCATClose(catalogHandle);
                }
            }

            if (!File.Exists(destinationCatalogPath))
            {
                throw new FileNotFoundException("Catalog file was not created.", destinationCatalogPath);
            }
        }
    }
}

