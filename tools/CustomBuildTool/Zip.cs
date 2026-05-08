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
        private static readonly FrozenSet<string> SkipPathPrefixes = new[]
        {
            "bin\\Debug",
            "obj\\",
            "tests\\"
        }.ToFrozenSet(StringComparer.OrdinalIgnoreCase);

        private static readonly FrozenSet<string> SkipExtensions = new[]
        {
            ".pdb",
            ".iobj",
            ".ipdb",
            ".exp",
            ".lib"
        }.ToFrozenSet(StringComparer.OrdinalIgnoreCase);

        private sealed class CompressionProgressReporter : IProgress<SharpCompress.Common.ProgressReport>
        {
            private readonly Dictionary<string, SharpCompress.Common.ProgressReport> Reports = new Dictionary<string, SharpCompress.Common.ProgressReport>(StringComparer.OrdinalIgnoreCase);

            public void Report(SharpCompress.Common.ProgressReport value)
            {
                if (!string.IsNullOrWhiteSpace(value.EntryPath))
                {
                    this.Reports[value.EntryPath] = value;
                }
            }

            public bool TryGetReport(string EntryName, out SharpCompress.Common.ProgressReport Report)
            {
                return this.Reports.TryGetValue(EntryName, out Report);
            }
        }

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
                string name = names[i];
                result[i] = length <= name.Length ? name.AsSpan(length).ToString() : string.Empty;
            }

            return result;
        }

        private static ZipWriterOptions CreateWriterOptions(CompressionProgressReporter ProgressReporter)
        {
            return new ZipWriterOptions(CompressionType.Deflate)
            {
                LeaveStreamOpen = true,
                ArchiveEncoding = new ArchiveEncoding { Default = Utils.UTF8NoBOM },
                Progress = ProgressReporter
            };
        }

        private static string GetEntryName(string name, string sourceFolder, bool includeBaseName)
        {
            if (includeBaseName)
                sourceFolder = Path.GetDirectoryName(sourceFolder);

            int length = string.IsNullOrWhiteSpace(sourceFolder) ? 0 : sourceFolder.Length;
            if (length > 0 && sourceFolder != null && sourceFolder[length - 1] != Path.DirectorySeparatorChar && sourceFolder[length - 1] != Path.AltDirectorySeparatorChar)
                length++;

            return length <= name.Length ? name.AsSpan(length).ToString() : string.Empty;
        }

        private static void WriteEntry(
            ZipWriter Writer,
            Stream ArchiveStream,
            string EntryName,
            string SourceFile,
            CompressionType CompressionType,
            CompressionProgressReporter ProgressReporter,
            BuildFlags Flags
            )
        {
            long startPosition = ArchiveStream.Position;
            var entryOptions = new ZipWriterEntryOptions
            {
                CompressionType = CompressionType,
                CompressionLevel = CompressionType == CompressionType.None ? 0 : 6,
                ModificationDateTime = Build.BuildDateTime
            };

            using (FileStream source = File.OpenRead(SourceFile))
            {
                Writer.Write(EntryName, source, entryOptions);
            }

            long compressedSize = ArchiveStream.Position - startPosition;

            if (Flags.HasFlag(BuildFlags.BuildVerbose))
            {
                PrintCompressionVerboseLine(EntryName, compressedSize, ProgressReporter, Flags);
            }
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
            var progressReporter = new CompressionProgressReporter();
            var PathReplacements = new[] // Path replacements for archive entry names
            {
                ("Release32\\", "i386\\"),
                ("Release64\\", "amd64\\"),
                ("ReleaseARM64\\", "arm64\\")
            };

            using (var fileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (var writer = new ZipWriter(fileStream, CreateWriterOptions(progressReporter)))
            {
                foreach (string file in Directory.EnumerateFiles(sourceDirectoryName, "*", SearchOption.AllDirectories))
                {
                    bool shouldSkip = false;
                    string name = GetEntryName(file, sourceDirectoryName, false);

                    foreach (var prefix in SkipPathPrefixes)
                    {
                        if (name.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
                        {
                            shouldSkip = true;
                            break;
                        }
                    }

                    if (!shouldSkip)
                    {
                        if (SkipExtensions.Contains(Path.GetExtension(file)))
                        {
                            shouldSkip = true;
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

                    WriteEntry(writer, fileStream, name, file, CompressionType.Deflate, progressReporter, Flags);
                }
            }

            //using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            //using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            //{
            //    for (int i = 0; i < filesToAdd.Length; i++)
            //    {
            //        bool shouldSkip = false;
            //        string file = filesToAdd[i];
            //        string name = entryNames[i];
            //
            //        foreach (var prefix in SkipPathPrefixes)
            //        {
            //            if (file.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            //            {
            //                shouldSkip = true;
            //                break;
            //            }
            //        }
            //
            //        if (!shouldSkip)
            //        {
            //            if (SkipExtensions.Contains(Path.GetExtension(file)))
            //            {
            //                shouldSkip = true;
            //            }
            //        }
            //
            //        if (shouldSkip)
            //            continue;
            //
            //        foreach (var (from, to) in PathReplacements)
            //        {
            //            if (name.StartsWith(from, StringComparison.OrdinalIgnoreCase))
            //            {
            //                name = name.Replace(from, to, StringComparison.OrdinalIgnoreCase);
            //                break;
            //            }
            //        }
            //
            //        var entry = archive.CreateEntryFromFile(file, name, System.IO.Compression.CompressionLevel.Optimal);
            //
            //        if (Flags.HasFlag(BuildFlags.BuildVerbose))
            //            PrintCompressionVerboseLine(entry, name, Flags);
            //    }
            //}

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
            var progressReporter = new CompressionProgressReporter();

            using (var fileStream = File.Create(destinationArchiveFileName))
            using (var writer = new ZipWriter(fileStream, CreateWriterOptions(progressReporter)))
            {
                foreach (string file in Directory.EnumerateFiles(sourceDirectoryName, "*", SearchOption.AllDirectories))
                {
                    string name = GetEntryName(file, sourceDirectoryName, false);

                    WriteEntry(writer, fileStream, name, file, CompressionType.Deflate, progressReporter, Flags);
                }
            }

            //using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            //using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            //{
            //    for (int i = 0; i < filesToAdd.Length; i++)
            //    {
            //        var entry = archive.CreateEntryFromFile(filesToAdd[i], entryNames[i], System.IO.Compression.CompressionLevel.Optimal);

            //        if (Flags.HasFlag(BuildFlags.BuildVerbose))
            //            PrintCompressionVerboseLine(entry, entryNames[i], Flags);
            //    }
            //}

            //using (var filestream = File.Create(destinationArchiveFileName))
            //using (var archive = new SevenZipWriter(filestream, new SevenZipWriterOptions 
            //{ 
            //    CompressHeader = true,
            //    CompressionType = SharpCompress.Common.CompressionType.LZMA2
            //}))
            //{
            //    for (int i = 0; i < filesToAdd.Length; i++)
            //    {
            //        string file = filesToAdd[i];
            //        var name = file.Replace(sourceDirectoryName, string.Empty);

            //        archive.Write(file, name);
            //    }
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
            var progressReporter = new CompressionProgressReporter();

            using (var fileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (var writer = new ZipWriter(fileStream, CreateWriterOptions(progressReporter)))
            {
                foreach (string file in Directory.EnumerateFiles(sourceDirectoryName, "*", SearchOption.AllDirectories))
                {
                    bool shouldSkip = false;
                    string name = GetEntryName(file, sourceDirectoryName, false);

                    if (!file.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase))
                        continue;

                    foreach (var prefix in SkipPathPrefixes)
                    {
                        if (name.Contains(prefix, StringComparison.OrdinalIgnoreCase))
                        {
                            shouldSkip = true;
                            break;
                        }
                    }

                    if (shouldSkip)
                        continue;

                    WriteEntry(writer, fileStream, name, file, CompressionType.None, progressReporter, Flags);
                }
            }

            //using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            //using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            //{
            //    for (int i = 0; i < filesToAdd.Length; i++)
            //    {
            //        bool shouldSkip = false;
            //
            //        // Ignore junk files
            //        if (!filesToAdd[i].EndsWith(".pdb", StringComparison.OrdinalIgnoreCase))
            //            continue;
            //
            //        // Ignore junk directories
            //        foreach (var prefix in SkipPathPrefixes)
            //        {
            //            if (filesToAdd[i].Contains(prefix, StringComparison.OrdinalIgnoreCase))
            //            {
            //                shouldSkip = true;
            //                break;
            //            }
            //        }
            //
            //        if (shouldSkip)
            //            continue;
            //
            //        var entry = archive.CreateEntryFromFile(filesToAdd[i], entryNames[i], System.IO.Compression.CompressionLevel.Optimal);
            //
            //        if (Flags.HasFlag(BuildFlags.BuildVerbose))
            //            PrintCompressionVerboseLine(entry, entryNames[i], Flags);
            //    }
            //}
        }

        /// <summary>
        /// Prints a fallback verbose compression line when archive entry sizes are unavailable.
        /// </summary>
        /// <param name="EntryName">Archive entry name.</param>
        /// <param name="Flags">Build flags for output formatting.</param>
        private static void PrintCompressionVerboseLine(string EntryName, BuildFlags Flags)
        {
            Program.PrintColorMessage("Compressing ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(EntryName ?? "?", ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage("...", ConsoleColor.DarkGray, true, Flags);
        }

        /// <summary>
        /// Prints verbose compression progress reported by SharpCompress.
        /// </summary>
        /// <param name="EntryName">Archive entry name.</param>
        /// <param name="CompressedSize">Progress reports captured during writing.</param>
        /// <param name="progressReporter">Progress reports captured during writing.</param>
        /// <param name="Flags">Build flags for output formatting.</param>
        private static void PrintCompressionVerboseLine(string EntryName, long CompressedSize, CompressionProgressReporter progressReporter, BuildFlags Flags)
        {
            if (!progressReporter.TryGetReport(EntryName, out var report))
            {
                PrintCompressionVerboseLine(EntryName, Flags);
                return;
            }

            PrintCompressionProgressColumns(EntryName, CompressedSize, report.BytesTransferred, Flags);
        }

        /// <summary>
        /// Prints compression statistics in aligned columns for verbose output.
        /// </summary>
        /// <param name="entryName">The archive entry name.</param>
        /// <param name="originalText">Formatted original file size.</param>
        /// <param name="compressedText">Formatted compressed size.</param>
        /// <param name="percent">Compression reduction percentage.</param>
        /// <param name="Flags">Build flags for output formatting.</param>
        private static void PrintCompressionColumns(string entryName, string originalText, string compressedText, double percent, BuildFlags Flags)
        {
            const int entryColumnWidth = 48;
            const int sizeColumnWidth = 10;      // e.g. "1446Kb"
            const int percentColumnWidth = 6;    // e.g. " 59.6%"

            string entryText = entryName ?? "?";
            if (entryText.Length > entryColumnWidth)
                entryText = entryText.Substring(0, entryColumnWidth - 1) + "~";

            string entryAligned = $"{entryText,-entryColumnWidth}";
            string originalAligned = $"{(originalText ?? "?"),sizeColumnWidth}";
            string compressedAligned = $"{(compressedText ?? "?"),sizeColumnWidth}";
            string percentAligned = $"{percent,percentColumnWidth - 1:0.0}%";

            Program.PrintColorMessage("Compressing ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(entryAligned, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage("... ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(originalAligned, ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage(" -> ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(compressedAligned, ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage($" ({percentAligned} reduction)", ConsoleColor.DarkGray, true, Flags);
        }

        /// <summary>
        /// Prints SharpCompress transfer progress in aligned columns for verbose output.
        /// </summary>
        /// <param name="entryName">The archive entry name.</param>
        /// <param name="CompressedSize">Compressed byte count.</param>
        /// <param name="OriginalSize">Compressed byte count.</param>
        /// <param name="Flags">Build flags for output formatting.</param>
        private static void PrintCompressionProgressColumns(string entryName, long CompressedSize, long OriginalSize, BuildFlags Flags)
        {
            const int entryColumnWidth = 48;
            const int sizeColumnWidth = 10;
            const int percentColumnWidth = 6;

            string entryText = entryName ?? "?";
            if (entryText.Length > entryColumnWidth)
                entryText = entryText.Substring(0, entryColumnWidth - 1) + "~";

            string entryAligned = $"{entryText,-entryColumnWidth}";
            string originalAligned = $"{OriginalSize.ToPrettySize(),sizeColumnWidth}";
            string compressedAligned = $"{CompressedSize.ToPrettySize(),sizeColumnWidth}";
            string percentAligned = $"{(OriginalSize <= 0 ? 0.0 : (1.0 - ((double)CompressedSize / OriginalSize)) * 100.0),percentColumnWidth - 1:0.0}%";

            Program.PrintColorMessage("Compressing ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(entryAligned, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage("... ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(originalAligned, ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage(" -> ", ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage(compressedAligned, ConsoleColor.Yellow, false, Flags);
            Program.PrintColorMessage($" ({percentAligned} reduction)", ConsoleColor.DarkGray, true, Flags);
        }

    }
}
