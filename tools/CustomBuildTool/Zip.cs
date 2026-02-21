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
        // https://github.com/phofman/zip/blob/master/src/ZipFile.cs
        private static string[] GetEntryNames(string[] names, string sourceFolder, bool includeBaseName)
        {
            if (names == null || names.Length == 0)
                return Array.Empty<string>();

            if (includeBaseName)
                sourceFolder = Path.GetDirectoryName(sourceFolder);

            int length = string.IsNullOrWhiteSpace(sourceFolder) ? 0 : sourceFolder.Length;
            if (length > 0 && sourceFolder != null && sourceFolder[length - 1] != Path.DirectorySeparatorChar && sourceFolder[length - 1] != Path.AltDirectorySeparatorChar)
                length++;

            var result = new string[names.Length];
            for (int i = 0; i < names.Length; i++)
            {
                result[i] = names[i].Substring(length);
            }

            return result;
        }

        public static void CreateCompressedFolder(string sourceDirectoryName, string destinationArchiveFileName, BuildFlags Flags = BuildFlags.None)
        {
            string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            string[] entryNames = GetEntryNames(filesToAdd, sourceDirectoryName, false);

            using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                for (int i = 0; i < filesToAdd.Length; i++)
                {
                    string file = filesToAdd[i];
                    string name = entryNames[i];

                    if (
                       file.StartsWith("bin\\Debug", StringComparison.OrdinalIgnoreCase) || // Ignore junk files
                       file.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                       file.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                       file.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                       file.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                       file.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)
                       )
                    {
                        continue;
                    }

                    if (name.StartsWith("Release32\\", StringComparison.OrdinalIgnoreCase))
                        name = name.Replace("Release32\\", "i386\\", StringComparison.OrdinalIgnoreCase);
                    if (name.StartsWith("Release64\\", StringComparison.OrdinalIgnoreCase))
                        name = name.Replace("Release64\\", "amd64\\", StringComparison.OrdinalIgnoreCase);
                    if (name.StartsWith("ReleaseARM64\\", StringComparison.OrdinalIgnoreCase))
                        name = name.Replace("ReleaseARM64\\", "arm64\\", StringComparison.OrdinalIgnoreCase);

                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                    {
                        // Print file and entry name in different colors
                        Program.PrintColorMessage("Compressing ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(file, ConsoleColor.Cyan, false);
                        Program.PrintColorMessage(" as ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(name + "...", ConsoleColor.Green, true);
                    }

                    var entry = archive.CreateEntryFromFile(file, name, CompressionLevel.Optimal);

                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                    {
                        try
                        {
                            var (compressedSizeOpt, uncompressedSizeOpt) = GetInternalSizes(entry);

                            if (compressedSizeOpt.HasValue || uncompressedSizeOpt.HasValue)
                            {
                                long originalSize = uncompressedSizeOpt ?? -1;
                                long compressedSize = compressedSizeOpt ?? -1;
                                string originalText = originalSize >= 0 ? Extextensions.ToPrettySize(originalSize) : "?";
                                string compressedText = compressedSize >= 0 ? Extextensions.ToPrettySize(compressedSize) : "?";
                                double percent = (originalSize <= 0 || compressedSize < 0) ? 0.0 : (1.0 - ((double)compressedSize / originalSize)) * 100.0;

                                // Print in columns: [file]  as [entry]  [orig] -> [comp] (percent)
                                PrintCompressionColumns(file, name, originalText, compressedText, percent, Flags);
                            }
                        }
                        catch
                        {
                            // Ignore any errors querying sizes for verbose output
                        }
                    }
                }
            }

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
            //
            //        if (
            //           file.StartsWith("Debug", StringComparison.OrdinalIgnoreCase) ||// Ignore directories (Debug32/Debug64)
            //           file.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||  // Ignore files
            //           file.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
            //           file.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
            //           file.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
            //           file.EndsWith(".lib", StringComparison.OrdinalIgnoreCase)
            //           )
            //        {
            //            continue;
            //        }
            //
            //        var name = file.Replace(sourceDirectoryName, string.Empty);
            //        compressor.AddFile(file, name);
            //    }
            //
            //    compressor.Finalize();
            //}
        }

        public static void CreateCompressedSdkFromFolder(string sourceDirectoryName, string destinationArchiveFileName, BuildFlags Flags = BuildFlags.None)
        {
            string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            string[] entryNames = GetEntryNames(filesToAdd, sourceDirectoryName, false);

            using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                for (int i = 0; i < filesToAdd.Length; i++)
                {
                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                    {
                        Program.PrintColorMessage("Compressing ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(filesToAdd[i], ConsoleColor.Cyan, false);
                        Program.PrintColorMessage(" as ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(entryNames[i] + "...", ConsoleColor.Green, true);
                    }

                    var entry = archive.CreateEntryFromFile(filesToAdd[i], entryNames[i], CompressionLevel.Optimal);

                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                    {
                        try
                        {
                            var (compressedSizeOpt, uncompressedSizeOpt) = GetInternalSizes(entry);

                            if (compressedSizeOpt.HasValue || uncompressedSizeOpt.HasValue)
                            {
                                long originalSize = uncompressedSizeOpt ?? -1;
                                long compressedSize = compressedSizeOpt ?? -1;
                                string originalText = originalSize >= 0 ? Extextensions.ToPrettySize(originalSize) : "?";
                                string compressedText = compressedSize >= 0 ? Extextensions.ToPrettySize(compressedSize) : "?";
                                double percent = (originalSize <= 0 || compressedSize < 0) ? 0.0 : (1.0 - ((double)compressedSize / originalSize)) * 100.0;

                                PrintCompressionColumns(filesToAdd[i], entryNames[i], originalText, compressedText, percent, Flags);
                            }
                        }
                        catch
                        {
                            // Ignore any errors querying sizes for verbose output
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

        public static void CreateCompressedPdbFromFolder(string sourceDirectoryName, string destinationArchiveFileName, BuildFlags Flags = BuildFlags.None)
        {
            string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            string[] entryNames = GetEntryNames(filesToAdd, sourceDirectoryName, false);

            using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                for (int i = 0; i < filesToAdd.Length; i++)
                {
                    // Ignore junk files
                    if (!filesToAdd[i].EndsWith(".pdb", StringComparison.OrdinalIgnoreCase))
                        continue;

                    // Ignore junk directories
                    if (filesToAdd[i].Contains("bin\\Debug", StringComparison.OrdinalIgnoreCase) ||
                        filesToAdd[i].Contains("obj\\", StringComparison.OrdinalIgnoreCase) ||
                        filesToAdd[i].Contains("tests\\", StringComparison.OrdinalIgnoreCase))
                        continue;

                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                    {
                        Program.PrintColorMessage("Compressing ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(filesToAdd[i], ConsoleColor.Cyan, false);
                        Program.PrintColorMessage(" as ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(entryNames[i] + "...", ConsoleColor.Green, true);
                    }

                    var entry = archive.CreateEntryFromFile(filesToAdd[i], entryNames[i], CompressionLevel.Optimal);

                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                    {
                        try
                        {
                            var (compressedSizeOpt, uncompressedSizeOpt) = GetInternalSizes(entry);

                            if (compressedSizeOpt.HasValue || uncompressedSizeOpt.HasValue)
                            {
                                long originalSize = uncompressedSizeOpt ?? -1;
                                long compressedSize = compressedSizeOpt ?? -1;
                                string originalText = originalSize >= 0 ? Extextensions.ToPrettySize(originalSize) : "?";
                                string compressedText = compressedSize >= 0 ? Extextensions.ToPrettySize(compressedSize) : "?";
                                double percent = (originalSize <= 0 || compressedSize < 0) ? 0.0 : (1.0 - ((double)compressedSize / originalSize)) * 100.0;

                                PrintCompressionColumns(filesToAdd[i], entryNames[i], originalText, compressedText, percent, Flags);
                            }
                        }
                        catch
                        {
                            // Ignore any errors querying sizes for verbose output
                        }
                    }
                }
            }
        }

        // Helper that prints compression info in aligned columns
        private static void PrintCompressionColumns(string file, string entryName, string originalText, string compressedText, double percent, BuildFlags Flags)
        {
            // Column widths - tuned for console display
            const int fileCol = 60;
            const int entryCol = 30;

            string fileDisplay = file.Length > fileCol ? "..." + file[^ (fileCol - 3) ..] : file.PadRight(fileCol);
            string entryDisplay = entryName.Length > entryCol ? entryName.Substring(0, entryCol - 3) + "..." : entryName.PadRight(entryCol);

            // [file] as [entry]  [orig] -> [comp] (percent)
            Program.PrintColorMessage(fileDisplay, ConsoleColor.Cyan, false, Flags);
            Program.PrintColorMessage(" as ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(entryDisplay, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage("  ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(originalText.PadLeft(8), ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage(" -> ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(compressedText.PadLeft(8), ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage($" ({percent:0.0}% reduction)", ConsoleColor.DarkGray, true, Flags);
        }

        static (long? Compressed, long? Uncompressed) GetInternalSizes(ZipArchiveEntry entry)
        {
            var _compressedSize = typeof(ZipArchiveEntry).GetField("_compressedSize", BindingFlags.NonPublic | BindingFlags.Instance);
            var _uncompressedSize = typeof(ZipArchiveEntry).GetField("_uncompressedSize", BindingFlags.NonPublic | BindingFlags.Instance);

            long? GetLong(FieldInfo f)
            {
                if (f == null) return null;
                var v = f.GetValue(entry);
                return v switch
                {
                    long l => l,
                    int i => i,
                    uint ui => (long)ui,
                    short s => s,
                    null => null,
                    _ => null
                };
            }

            return (GetLong(_compressedSize), GetLong(_uncompressedSize));
        }
    }
}
