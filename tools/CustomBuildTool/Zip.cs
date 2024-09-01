/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

namespace CustomBuildTool
{
    public static class Zip
    {
        // Modified based on GetEntryNames https://github.com/phofman/zip/blob/master/src/ZipFile.cs
        private static Dictionary<string, string> GetEntryNames(string SourceDirectory, bool IncludeBaseName, Dictionary<string,string> KeyValuePairs)
        {
            var files = Directory.GetFiles(SourceDirectory, "*", SearchOption.AllDirectories);

            if (files.Length == 0)
                return null;

            if (IncludeBaseName)
                SourceDirectory = Path.GetDirectoryName(SourceDirectory);

            int length = string.IsNullOrWhiteSpace(SourceDirectory) ? 0 : SourceDirectory.Length;
            if (length > 0 && SourceDirectory != null && SourceDirectory[length - 1] != Path.DirectorySeparatorChar && SourceDirectory[length - 1] != Path.AltDirectorySeparatorChar)
                length++;

            var result = new Dictionary<string, string>(files.Length, StringComparer.OrdinalIgnoreCase);
            for (int i = 0; i < files.Length; i++)
            {
                var file = files[i];
                var name = file.Substring(length);

                foreach (var item in KeyValuePairs)
                {
                    if (name.StartsWith(item.Key, StringComparison.OrdinalIgnoreCase))
                    {
                        name = name.Replace(item.Key, item.Value, StringComparison.OrdinalIgnoreCase);
                    }
                }

                result.Add(file, name);
            }

            return result;
        }

        public static void CreateInboxZip(string File, string Name, string ArchiveFileName)
        {
            using (FileStream zipFileStream = new FileStream(ArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                archive.CreateEntryFromFile(File, Name, CompressionLevel.Optimal);
            }
        }

        public static void CreateCompressedFolder(string sourceDirectoryName, string destinationArchiveFileName)
        {
            Dictionary<string, string> archiveDirectoryMappingPairs = new Dictionary<string, string>(3, StringComparer.OrdinalIgnoreCase)
            {
                { "Release32\\", "i386\\" },
                { "Release64\\", "amd64\\" },
                { "ReleaseARM64\\", "arm64\\" }
            };

            var entryNames = GetEntryNames(sourceDirectoryName, false, archiveDirectoryMappingPairs);

            foreach (var entry in entryNames)
            {
                if (
                    entry.Key.StartsWith("bin\\Debug", StringComparison.OrdinalIgnoreCase) || // Remove junk files
                    entry.Key.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                    entry.Key.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                    entry.Key.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                    entry.Key.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                    entry.Key.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)
                    )
                {
                    entryNames.Remove(entry.Key);
                }
            }

            using (var fileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            {
                var options = new ZipWriterOptions(CompressionType.LZMA)
                {
                    ArchiveEncoding = new ArchiveEncoding() { Default = Utils.UTF8NoBOM, },
                    DeflateCompressionLevel = SharpCompress.Compressors.Deflate.CompressionLevel.BestCompression,
                    CompressionType = CompressionType.LZMA
                };

                using (var writer = new ZipWriter(fileStream, options))
                {
                    foreach (var entry in entryNames)
                    {
                        bool binary = entry.Key.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) ||
                                      entry.Key.EndsWith(".exe", StringComparison.OrdinalIgnoreCase);

                        using (var input = File.OpenRead(entry.Key))
                        {
                            writer.Write(entry.Value, input, new ZipWriterEntryOptions
                            {
                                ModificationDateTime = Build.BuildDateTime,
                                CompressionType = binary ? CompressionType.BCJ2 : CompressionType.LZMA
                            });
                        }
                    }
                }
            }
        }

        public static void CreateCompressedSdkFromFolder(string sourceDirectoryName, string destinationArchiveFileName)
        {
            var entryNames = GetEntryNames(sourceDirectoryName, false, null);

            using (var fileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            {
                var options = new ZipWriterOptions(CompressionType.LZMA)
                {
                    ArchiveEncoding = new ArchiveEncoding() { Default = Utils.UTF8NoBOM, },
                    DeflateCompressionLevel = SharpCompress.Compressors.Deflate.CompressionLevel.BestCompression,
                };

                using (var writer = new ZipWriter(fileStream, options))
                {
                    foreach (var entry in entryNames)
                    {
                        bool binary = entry.Key.EndsWith(".dll", StringComparison.OrdinalIgnoreCase) ||
                                      entry.Key.EndsWith(".exe", StringComparison.OrdinalIgnoreCase);

                        using (var input = File.OpenRead(entry.Key))
                        {
                            writer.Write(entry.Value, input, new ZipWriterEntryOptions
                            {
                                ModificationDateTime = Build.BuildDateTime,
                                CompressionType = binary ? CompressionType.BCJ2 : CompressionType.LZMA
                            });
                        }
                    }
                }
            }

            //string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            //
            //Win32.DeleteFile(destinationArchiveFileName);
            //
            //using (var filestream = File.Create(destinationArchiveFileName))
            //using (var archive = new SevenZipArchive(filestream, FileAccess.Write))
            //using (var compressor = archive.Compressor())
            //{
            //    compressor.CompressHeader = true;
            //    compressor.PreserveDirectoryStructure = true;
            //    compressor.Solid = true;
            //
            //    for (int i = 0; i < filesToAdd.Length; i++)
            //    {
            //        string file = filesToAdd[i];
            //        var name = file.Replace(sourceDirectoryName, string.Empty);
            //
            //        compressor.AddFile(file, name);
            //    }
            //
            //    compressor.Finalize();
            //}
        }

        public static void CreateCompressedPdbFromFolder(string sourceDirectoryName, string destinationArchiveFileName)
        {
            var entryNames = GetEntryNames(sourceDirectoryName, false, null);

            foreach (var entry in entryNames)
            {
                string file = entry.Key;
                string name = entry.Value;

                // Ignore junk files
                if (!file.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase))
                {
                    entryNames.Remove(file);
                    continue;
                }

                // Ignore junk directories
                if (file.Contains("bin\\Debug", StringComparison.OrdinalIgnoreCase) ||
                    file.Contains("obj\\", StringComparison.OrdinalIgnoreCase) ||
                    file.Contains("tests\\", StringComparison.OrdinalIgnoreCase))
                {
                    entryNames.Remove(file);
                    continue;
                }
            }

            using (var fileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            {
                var options = new ZipWriterOptions(CompressionType.LZMA)
                {
                    ArchiveEncoding = new ArchiveEncoding() { Default = Utils.UTF8NoBOM, },
                    DeflateCompressionLevel = SharpCompress.Compressors.Deflate.CompressionLevel.BestCompression,
                };

                using (var writer = new ZipWriter(fileStream, options))
                {
                    foreach (var entry in entryNames)
                    {
                        using (var input = File.OpenRead(entry.Key))
                        {
                            writer.Write(entry.Value, input, new ZipWriterEntryOptions
                            {
                                ModificationDateTime = Build.BuildDateTime
                            });
                        }
                    }
                }
            }

            //using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            //using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            //{
            //    foreach (var entry in entryNames)
            //    {
            //        string file = entry.Key;
            //        string name = entry.Value;
            //
            //        // Ignore junk files
            //        if (!file.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase))
            //            continue;
            //
            //        // Ignore junk directories
            //        if (file.Contains("bin\\Debug", StringComparison.OrdinalIgnoreCase) ||
            //            file.Contains("obj\\", StringComparison.OrdinalIgnoreCase) ||
            //            file.Contains("tests\\", StringComparison.OrdinalIgnoreCase))
            //            continue;
            //
            //        archive.CreateEntryFromFile(file, name, CompressionLevel.Optimal);
            //    }
            //}
        }
    }
}
