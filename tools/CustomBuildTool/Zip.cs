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
    /// <summary>
    /// Provides methods for creating compressed ZIP archives from directories.
    /// </summary>
    /// <remarks>
    /// This class supports creating different types of archives:
    /// - Standard release archives with path transformations
    /// - SDK archives
    /// - PDB symbol archives
    /// </remarks>
    public static class Zip
    {
        /// <summary>
        /// Converts absolute file paths to relative entry names for archive entries.
        /// </summary>
        /// <param name="names">Array of absolute file paths.</param>
        /// <param name="sourceFolder">The source folder path to remove from each name.</param>
        /// <param name="includeBaseName">If true, includes the base folder name in the entry path.</param>
        /// <returns>Array of relative entry names suitable for archive entries.</returns>
        /// <remarks>
        /// Based on https://github.com/phofman/zip/blob/master/src/ZipFile.cs
        /// </remarks>
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

        /// <summary>
        /// Creates a compressed ZIP archive from a source directory, excluding debug files and applying path transformations.
        /// </summary>
        /// <param name="sourceDirectoryName">The path to the directory to compress.</param>
        /// <param name="destinationArchiveFileName">The path where the ZIP archive will be created.</param>
        /// <param name="Flags">Build flags controlling verbosity and other options.</param>
        /// <remarks>
        /// This method:
        /// - Skips files in debug directories and files with debug/linker extensions
        /// - Transforms Release configuration paths to architecture names (e.g., Release64 to amd64)
        /// - Uses optimal compression level
        /// </remarks>
        public static void CreateCompressedFolder(string sourceDirectoryName, string destinationArchiveFileName, BuildFlags Flags = BuildFlags.None)
        { 
            // Path prefixes to skip during compression (e.g., debug build directories)
            string[] SkipPathPrefixes = 
            [
                "bin\\Debug"
            ];
            // File extensions to skip during compression (debug symbols, linker artifacts)
            string[] SkipExtensions = 
            [
                ".pdb", 
                ".iobj", 
                ".ipdb", 
                ".exp", 
                ".lib"
            ];
            var PathReplacements = new[] // Path replacements for archive entry names
            {
                ("Release32\\", "i386\\"),
                ("Release64\\", "amd64\\"),
                ("ReleaseARM64\\", "arm64\\")
            };

            string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            string[] entryNames = GetEntryNames(filesToAdd, sourceDirectoryName, false);

            using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                for (int i = 0; i < filesToAdd.Length; i++)
                {
                    bool shouldSkip = false;
                    string file = filesToAdd[i];
                    string name = entryNames[i];

                    foreach (var prefix in SkipPathPrefixes)
                    {
                        if (file.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
                        {
                            shouldSkip = true;
                            break;
                        }
                    }

                    if (!shouldSkip)
                    {
                        foreach (var ext in SkipExtensions)
                        {
                            if (file.EndsWith(ext, StringComparison.OrdinalIgnoreCase))
                            {
                                shouldSkip = true;
                                break;
                            }
                        }
                    }

                    if (shouldSkip)
                        continue;

                    foreach (var (from, to) in PathReplacements)
                    {
                        if (name.StartsWith(from, StringComparison.OrdinalIgnoreCase))
                        {
                            name = name.Replace(from, to, StringComparison.OrdinalIgnoreCase);
                            break;
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                    {
                        Program.PrintColorMessage("Compressing ", ConsoleColor.DarkGray, false);
                        //Program.PrintColorMessage(file, ConsoleColor.Cyan, false);
                        //Program.PrintColorMessage(" as ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(name, ConsoleColor.Green, false);
                        Program.PrintColorMessage("...", ConsoleColor.DarkGray, false);
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
                                string originalText = originalSize >= 0 ? Extensions.ToPrettySize(originalSize) : "?";
                                string compressedText = compressedSize >= 0 ? Extensions.ToPrettySize(compressedSize) : "?";
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

        /// <summary>
        /// Creates a compressed ZIP archive containing SDK files from a source directory.
        /// </summary>
        /// <param name="sourceDirectoryName">The path to the SDK directory to compress.</param>
        /// <param name="destinationArchiveFileName">The path where the ZIP archive will be created.</param>
        /// <param name="Flags">Build flags controlling verbosity and other options.</param>
        /// <remarks>
        /// Unlike <see cref="CreateCompressedFolder"/>, this method includes all files without filtering.
        /// </remarks>
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
                        //Program.PrintColorMessage(filesToAdd[i], ConsoleColor.Cyan, false);
                        //Program.PrintColorMessage(" as ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(entryNames[i], ConsoleColor.Green, false);
                        Program.PrintColorMessage("...", ConsoleColor.DarkGray, false);
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
                                string originalText = originalSize >= 0 ? Extensions.ToPrettySize(originalSize) : "?";
                                string compressedText = compressedSize >= 0 ? Extensions.ToPrettySize(compressedSize) : "?";
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

        /// <summary>
        /// Creates a compressed ZIP archive containing only PDB symbol files from a source directory.
        /// </summary>
        /// <param name="sourceDirectoryName">The path to the directory containing PDB files.</param>
        /// <param name="destinationArchiveFileName">The path where the ZIP archive will be created.</param>
        /// <param name="Flags">Build flags controlling verbosity and other options.</param>
        /// <remarks>
        /// This method only includes .pdb files and excludes files from Debug, obj, and tests directories.
        /// </remarks>
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
                        //Program.PrintColorMessage(filesToAdd[i], ConsoleColor.Cyan, false);
                        //Program.PrintColorMessage(" as ", ConsoleColor.DarkGray, false);
                        Program.PrintColorMessage(entryNames[i], ConsoleColor.Green, false);
                        Program.PrintColorMessage("...", ConsoleColor.DarkGray, false);
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
                                string originalText = originalSize >= 0 ? Extensions.ToPrettySize(originalSize) : "?";
                                string compressedText = compressedSize >= 0 ? Extensions.ToPrettySize(compressedSize) : "?";
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

        /// <summary>
        /// Prints compression statistics in aligned columns for verbose output.
        /// </summary>
        /// <param name="file">The source file path (unused, kept for signature compatibility).</param>
        /// <param name="entryName">The archive entry name.</param>
        /// <param name="originalText">Formatted original file size.</param>
        /// <param name="compressedText">Formatted compressed size.</param>
        /// <param name="percent">Compression reduction percentage.</param>
        /// <param name="Flags">Build flags for output formatting.</param>
        private static void PrintCompressionColumns(string file, string entryName, string originalText, string compressedText, double percent, BuildFlags Flags)
        {
            // Column width for entry name - tuned for console display
            //const int entryCol = 50;
            //string entryDisplay = entryName.Length > entryCol 
            //    ? "..." + entryName[^(entryCol - 3)..] 
            //    : entryName.PadRight(entryCol);

            // [entry]  [orig] -> [comp] (percent)
            //Program.PrintColorMessage(entryDisplay, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage("  ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(originalText, ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage(" -> ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(compressedText, ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage($" ({percent:0.0}% reduction)", ConsoleColor.DarkGray, true, Flags);
        }

        /// <summary>
        /// Retrieves the internal compressed and uncompressed sizes from a ZipArchiveEntry using reflection.
        /// </summary>
        /// <param name="entry">The ZIP archive entry to query.</param>
        /// <returns>A tuple containing the compressed and uncompressed sizes, or null if unavailable.</returns>
        /// <remarks>
        /// Uses reflection to access private fields since the public Length property
        /// may not be available until the entry stream is read.
        /// </remarks>
        private static (long? Compressed, long? Uncompressed) GetInternalSizes(ZipArchiveEntry entry)
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
