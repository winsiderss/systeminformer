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
    /// Creates and verifies MSDelta (msdelta.dll / msdelta.h) deltas between the latest published setup and the local build output setup.
    /// </summary>
    /// <remarks>
    /// MSDelta (Windows Vista and later) supersedes PatchAPI (see <see cref="BuildPatch"/>) and is the preferred
    /// API for delta updates and for analyzing update binaries. CreateDeltaB detects the PE file type from the
    /// DELTA_FILE_TYPE set and applies executable-specific transforms selected by DELTA_FLAG_TYPE
    /// (e.g. DELTA_FLAG_E8 for x86 call instruction translation, or the DELTA_DEFAULT_FLAGS_* sets).
    /// Source and target are limited to DELTA_FILE_SIZE_LIMIT unless DELTA_FLAG_IGNORE_FILE_SIZE_LIMIT is set.
    /// </remarks>
    public static class BuildDelta
    {
        // msdelta.h
        private const long DELTA_FILE_SIZE_LIMIT = 32 * 1024 * 1024;
        private const long DELTA_FILE_TYPE_RAW = 0x00000001;
        private const long DELTA_FILE_TYPE_IA64 = 0x00000004;
        private const long DELTA_FILE_TYPE_CLI4_I386 = 0x00000010;
        private const long DELTA_FILE_TYPE_CLI4_AMD64 = 0x00000020;
        private const long DELTA_FILE_TYPE_CLI4_ARM = 0x00000040;
        private const long DELTA_FILE_TYPE_CLI4_ARM64 = 0x00000080;
        private const long DELTA_FILE_TYPE_SET_EXECUTABLES_LATEST =
            DELTA_FILE_TYPE_RAW | DELTA_FILE_TYPE_CLI4_I386 | DELTA_FILE_TYPE_IA64 |
            DELTA_FILE_TYPE_CLI4_AMD64 | DELTA_FILE_TYPE_CLI4_ARM | DELTA_FILE_TYPE_CLI4_ARM64;

        private const long DELTA_FLAG_NONE = 0x00000000;
        private const long DELTA_FLAG_MARK = 0x00000002;
        private const long DELTA_FLAG_IMPORTS = 0x00000004;
        private const long DELTA_FLAG_EXPORTS = 0x00000008;
        private const long DELTA_FLAG_RESOURCES = 0x00000010;
        private const long DELTA_FLAG_RELOCS = 0x00000020;
        private const long DELTA_FLAG_I386_SMASHLOCK = 0x00000040;
        private const long DELTA_FLAG_I386_JMPS = 0x00000080;
        private const long DELTA_FLAG_I386_CALLS = 0x00000100;
        private const long DELTA_FLAG_AMD64_DISASM = 0x00000200;
        private const long DELTA_FLAG_AMD64_PDATA = 0x00000400;
        private const long DELTA_FLAG_UNBIND = 0x00002000;
        private const long DELTA_FLAG_CLI_DISASM = 0x00004000;
        private const long DELTA_FLAG_CLI_METADATA = 0x00008000;
        private const long DELTA_FLAG_IGNORE_FILE_SIZE_LIMIT = 0x00020000;

        private const long DELTA_DEFAULT_FLAGS_RAW = DELTA_FLAG_NONE;
        private const long DELTA_DEFAULT_FLAGS_I386 =
            DELTA_FLAG_MARK | DELTA_FLAG_IMPORTS | DELTA_FLAG_EXPORTS | DELTA_FLAG_RESOURCES |
            DELTA_FLAG_RELOCS | DELTA_FLAG_I386_SMASHLOCK | DELTA_FLAG_I386_JMPS | DELTA_FLAG_I386_CALLS |
            DELTA_FLAG_UNBIND | DELTA_FLAG_CLI_DISASM | DELTA_FLAG_CLI_METADATA;
        private const long DELTA_DEFAULT_FLAGS_AMD64 =
            DELTA_FLAG_MARK | DELTA_FLAG_IMPORTS | DELTA_FLAG_EXPORTS | DELTA_FLAG_RESOURCES |
            DELTA_FLAG_RELOCS | DELTA_FLAG_AMD64_DISASM | DELTA_FLAG_AMD64_PDATA |
            DELTA_FLAG_UNBIND | DELTA_FLAG_CLI_DISASM | DELTA_FLAG_CLI_METADATA;

        private const uint CALG_SHA_256 = 0x0000800C;
        private const int DELTA_MAX_HASH_SIZE = 32;

        /// <summary>
        /// Downloads the latest published setup, creates a delta to the local build output setup,
        /// applies the delta to a copy of the source and verifies the result.
        /// </summary>
        /// <param name="Channel">The build channel (release or canary).</param>
        /// <returns>True if the delta was created and verified; otherwise, false.</returns>
        public static async Task<bool> CreateDelta(string Channel)
        {
            string sourceFile = Path.Join([Build.BuildOutputFolder, $"systeminformer-latest-{Channel}-setup.exe"]);
            string targetFile = Path.Join([Build.BuildOutputFolder, $"systeminformer-build-{Channel}-setup.exe"]);
            string deltaFile = Path.Join([Build.BuildOutputFolder, $"systeminformer-build-{Channel}-setup.delta"]);
            string testFolder = Path.Join([Build.BuildOutputFolder, "delta-test"]);
            string testSourceFile = Path.Join([testFolder, "source.exe"]);
            string testTargetFile = Path.Join([testFolder, $"systeminformer-build-{Channel}-setup.exe"]);

            if (!await PrepareSourceAndTarget(Channel, sourceFile, targetFile))
                return false;

            byte[] sourceBuffer = File.ReadAllBytes(sourceFile);
            byte[] targetBuffer = File.ReadAllBytes(targetFile);

            (long setFlags, string setFlagsName) = GetDefaultFlags(targetBuffer);

            // DELTA_FILE_SIZE_LIMIT applies to the source and target (not the delta output).
            // Allow larger inputs instead of failing.
            if (sourceBuffer.LongLength > DELTA_FILE_SIZE_LIMIT || targetBuffer.LongLength > DELTA_FILE_SIZE_LIMIT)
            {
                setFlags |= DELTA_FLAG_IGNORE_FILE_SIZE_LIMIT;
                setFlagsName += " | DELTA_FLAG_IGNORE_FILE_SIZE_LIMIT";

                Program.PrintColorMessage(
                    $"[WARN] File exceeds DELTA_FILE_SIZE_LIMIT ({DELTA_FILE_SIZE_LIMIT:N0} bytes): source {sourceBuffer.LongLength:N0}, target {targetBuffer.LongLength:N0}",
                    ConsoleColor.Yellow
                    );
            }

            //
            // Create the delta.
            //

            Program.PrintColorMessage($"Creating delta ({setFlagsName} 0x{setFlags:X})... ", ConsoleColor.Cyan, false);

            byte[] deltaBuffer;

            unsafe
            {
                DELTA_OUTPUT* output = stackalloc DELTA_OUTPUT[1];

                fixed (byte* sourcePtr = sourceBuffer)
                fixed (byte* targetPtr = targetBuffer)
                {
                    DELTA_INPUT source = new() { lpStart = sourcePtr, uSize = (nuint)sourceBuffer.Length, Editable = false };
                    DELTA_INPUT target = new() { lpStart = targetPtr, uSize = (nuint)targetBuffer.Length, Editable = false };
                    DELTA_INPUT empty = default;

                    if (!PInvoke.CreateDeltaB(
                        DELTA_FILE_TYPE_SET_EXECUTABLES_LATEST,
                        setFlags,
                        DELTA_FLAG_NONE,
                        source,
                        target,
                        empty,
                        empty,
                        empty,
                        null,
                        ALG_ID.CALG_SHA_256,
                        output
                        ))
                    {
                        Program.PrintColorMessage($"[ERROR] CreateDeltaB failed: {Marshal.GetLastPInvokeErrorMessage()}", ConsoleColor.Red);
                        return false;
                    }
                }

                deltaBuffer = new ReadOnlySpan<byte>(output->lpStart, checked((int)output->uSize)).ToArray();
                PInvoke.DeltaFree(output->lpStart);
            }

            File.WriteAllBytes(deltaFile, deltaBuffer);
            Program.PrintColorMessage("done.", ConsoleColor.Green);

            //
            // Read the delta header.
            //
           
            unsafe
            {
                fixed (byte* deltaPtr = deltaBuffer)
                {
                    DELTA_INPUT delta = new() { lpStart = deltaPtr, uSize = (nuint)deltaBuffer.Length, Editable = false };
                    DELTA_HEADER_INFO* headerInfo = stackalloc DELTA_HEADER_INFO[1];

                    if (PInvoke.GetDeltaInfoB(delta, headerInfo))
                    {
                        var hash = new ReadOnlySpan<byte>(headerInfo->TargetHash.HashValue.Value, (int)Math.Min(headerInfo->TargetHash.HashSize, DELTA_MAX_HASH_SIZE));  // .AsSpan()

                        Program.PrintColorMessage("Delta header:", ConsoleColor.Cyan);
                        Program.PrintColorMessage($"  FileTypeSet:  0x{headerInfo->FileTypeSet:X}", ConsoleColor.Gray);
                        Program.PrintColorMessage($"  FileType:     0x{headerInfo->FileType:X}", ConsoleColor.Gray);
                        Program.PrintColorMessage($"  Flags:        0x{headerInfo->Flags:X}", ConsoleColor.Gray);
                        Program.PrintColorMessage($"  TargetSize:   {headerInfo->TargetSize:N0}", ConsoleColor.Gray);
                        Program.PrintColorMessage($"  TargetHashAlg 0x{headerInfo->TargetHashAlgId:X}", ConsoleColor.Gray);
                        Program.PrintColorMessage($"  TargetHash:   {Convert.ToHexString(hash)}", ConsoleColor.Gray);
                    }
                    else
                    {
                        Program.PrintColorMessage($"[WARN] GetDeltaInfoB failed: {Marshal.GetLastPInvokeErrorMessage()}", ConsoleColor.Yellow);
                    }
                }
            }

            //
            // Apply the delta to a copy of the source.
            //

            Program.PrintColorMessage("Applying delta to copy... ", ConsoleColor.Cyan, false);

            Directory.CreateDirectory(testFolder);
            File.Copy(sourceFile, testSourceFile, true);

            byte[] testSourceBuffer = File.ReadAllBytes(testSourceFile);
            byte[] testDeltaBuffer = File.ReadAllBytes(deltaFile);
            byte[] appliedBuffer;

            unsafe
            {
                fixed (byte* sourcePtr = testSourceBuffer)
                fixed (byte* deltaPtr = testDeltaBuffer)
                {
                    DELTA_INPUT source = new() { lpStart = sourcePtr, uSize = (nuint)testSourceBuffer.Length, Editable = false };
                    DELTA_INPUT delta = new() { lpStart = deltaPtr, uSize = (nuint)testDeltaBuffer.Length, Editable = false };
                    DELTA_OUTPUT* output = stackalloc DELTA_OUTPUT[1];

                    if (!PInvoke.ApplyDeltaB(DELTA_FLAG_NONE, source, delta, output))
                    {
                        Program.PrintColorMessage($"[ERROR] ApplyDeltaB failed: {Marshal.GetLastPInvokeErrorMessage()}", ConsoleColor.Red);
                        return false;
                    }

                    appliedBuffer = new ReadOnlySpan<byte>(output->lpStart, checked((int)output->uSize)).ToArray();
                    PInvoke.DeltaFree(output->lpStart);
                }
            }

            File.WriteAllBytes(testTargetFile, appliedBuffer);
            Program.PrintColorMessage("done.", ConsoleColor.Green);

            //
            // Results
            //

            Program.PrintColorMessage(string.Empty, ConsoleColor.Gray);
            PrintFileInfo("Source (latest)", sourceFile, sourceBuffer);
            PrintFileInfo("Target (build)", targetFile, targetBuffer);
            PrintFileInfo("Delta", deltaFile, deltaBuffer);
            PrintFileInfo("Applied (copy)", testTargetFile, appliedBuffer);

            double ratio = targetBuffer.Length == 0 ? 0 : (double)deltaBuffer.Length * 100.0 / targetBuffer.Length;
            Program.PrintColorMessage($"Delta ratio: {ratio:F2}% of target ({deltaBuffer.Length:N0} / {targetBuffer.Length:N0} bytes)", ConsoleColor.Cyan);

            if (appliedBuffer.AsSpan().SequenceEqual(targetBuffer))
            {
                Program.PrintColorMessage("Round-trip: OK (applied output matches build target)", ConsoleColor.Green);
                return true;
            }
            else
            {
                Program.PrintColorMessage("Round-trip: MISMATCH (applied output differs from build target)", ConsoleColor.Red);
                return false;
            }
        }

        /// <summary>
        /// Verifies the build output setup exists and downloads the latest published setup
        /// (GitHub redirects to the latest release asset).
        /// </summary>
        internal static async Task<bool> PrepareSourceAndTarget(string Channel, string SourceFile, string TargetFile)
        {
            string LatestSetupUrlFormat = "https://github.com/winsiderss/builds/releases/latest/download/systeminformer-build-{0}-setup.exe";
            string latestUrl = string.Format(CultureInfo.InvariantCulture, LatestSetupUrlFormat, Channel);

            if (!File.Exists(TargetFile))
            {
                Program.PrintColorMessage($"[ERROR] Target not found: {TargetFile}", ConsoleColor.Red);
                return false;
            }

            Program.PrintColorMessage($"Downloading {latestUrl}... ", ConsoleColor.Cyan, false);

            try
            {
                using var httpClient = BuildHttpClient.CreateHttpClient();
                using var requestMessage = new HttpRequestMessage(HttpMethod.Get, latestUrl);
                using var response = await BuildHttpClient.SendRequestMessage(httpClient, requestMessage);

                if (!response.IsSuccessStatusCode)
                {
                    Program.PrintColorMessage($"[ERROR] HTTP {(int)response.StatusCode} {response.ReasonPhrase}", ConsoleColor.Red);
                    return false;
                }

                byte[] buffer = await response.Content.ReadAsByteArrayAsync();

                if (buffer.Length == 0)
                {
                    Program.PrintColorMessage("[ERROR] Empty response.", ConsoleColor.Red);
                    return false;
                }

                await File.WriteAllBytesAsync(SourceFile, buffer);

                Program.PrintColorMessage("done.", ConsoleColor.Green);
                Program.PrintColorMessage($"Resolved URL: {response.RequestMessage?.RequestUri}", ConsoleColor.Gray);
            }
            catch (Exception exception)
            {
                Program.PrintColorMessage($"[ERROR] {exception.Message}", ConsoleColor.Red);
                return false;
            }

            Program.PrintColorMessage($"Latest version: {GetFileVersion(SourceFile)}", ConsoleColor.Gray);
            Program.PrintColorMessage($"Build version:  {GetFileVersion(TargetFile)}", ConsoleColor.Gray);
            return true;
        }

        private static (long Flags, string Name) GetDefaultFlags(byte[] Buffer)
        {
            try
            {
                using var stream = new MemoryStream(Buffer, false);
                using var reader = new PEReader(stream);

                switch (reader.PEHeaders.CoffHeader.Machine)
                {
                    case Machine.I386:
                        return (DELTA_DEFAULT_FLAGS_I386, "DELTA_DEFAULT_FLAGS_I386");
                    case Machine.Amd64:
                        return (DELTA_DEFAULT_FLAGS_AMD64, "DELTA_DEFAULT_FLAGS_AMD64");
                }
            }
            catch (BadImageFormatException)
            {
            }

            return (DELTA_DEFAULT_FLAGS_RAW, "DELTA_DEFAULT_FLAGS_RAW");
        }

        internal static string GetFileVersion(string FileName)
        {
            try
            {
                return FileVersionInfo.GetVersionInfo(FileName).FileVersion ?? "(none)";
            }
            catch (Exception)
            {
                return "(unknown)";
            }
        }

        internal static void PrintFileInfo(string Label, string FileName, byte[] Buffer)
        {
            Program.PrintColorMessage($"{Label,-16} {Buffer.LongLength,14:N0} bytes  SHA256 {Convert.ToHexString(SHA256.HashData(Buffer))}", ConsoleColor.Gray);
            Program.PrintColorMessage($"{string.Empty,-16} {FileName}", ConsoleColor.DarkGray);
        }
    }


    /// <summary>
    /// Creates and verifies PatchAPI (mspatchc/mspatcha) patches between the latest published setup and the local build output setup.
    /// </summary>
    /// <remarks>
    /// PatchAPI was largely superseded by the MSDelta API (msdelta.dll / msdelta.h) starting in Windows Vista.
    /// For delta updates or analyzing update binaries prefer <see cref="BuildDelta"/>, which uses CreateDeltaB
    /// and selects executable-specific transforms via DELTA_FLAG_TYPE (e.g. DELTA_FLAG_E8 for x86 executables).
    /// This command is kept for comparison against the legacy format.
    /// </remarks>
    //public static class BuildPatch
    //{
    //    // patchapi.h
    //    private const uint PATCH_OPTION_USE_LZX_BEST = 0x00000003;
    //    private const uint PATCH_OPTION_USE_LZX_LARGE = 0x00000004;
    //    private const uint PATCH_OPTION_FAIL_IF_SAME_FILE = 0x00080000;
    //    private const uint APPLY_OPTION_NONE = 0x00000000;
    //
    //    // Files larger than this use PATCH_OPTION_USE_LZX_LARGE (requires a 5.1 or higher applyer).
    //    private const long PATCH_LZX_LARGE_THRESHOLD = 8 * 1024 * 1024;
    //
    //    /// <summary>
    //    /// Downloads the latest published setup, creates a PatchAPI patch to the local build output setup,
    //    /// applies the patch to a copy of the source and verifies the result.
    //    /// </summary>
    //    /// <param name="Channel">The build channel (release or canary).</param>
    //    /// <returns>True if the patch was created and verified; otherwise, false.</returns>
    //    public static async Task<bool> CreatePatch(string Channel)
    //    {
    //        string sourceFile = Path.Join([Build.BuildOutputFolder, $"systeminformer-latest-{Channel}-setup.exe"]);
    //        string targetFile = Path.Join([Build.BuildOutputFolder, $"systeminformer-build-{Channel}-setup.exe"]);
    //        string patchFile = Path.Join([Build.BuildOutputFolder, $"systeminformer-build-{Channel}-setup.patch"]);
    //        string testFolder = Path.Join([Build.BuildOutputFolder, "patch-test"]);
    //        string testSourceFile = Path.Join([testFolder, "source.exe"]);
    //        string testTargetFile = Path.Join([testFolder, $"systeminformer-build-{Channel}-setup.exe"]);
    //
    //        if (!await BuildDelta.PrepareSourceAndTarget(Channel, sourceFile, targetFile))
    //            return false;
    //
    //        //
    //        // Create the patch.
    //        //
    //
    //        long largest = Math.Max(new FileInfo(sourceFile).Length, new FileInfo(targetFile).Length);
    //        uint optionFlags = PATCH_OPTION_USE_LZX_BEST | PATCH_OPTION_FAIL_IF_SAME_FILE;
    //
    //        if (largest > PATCH_LZX_LARGE_THRESHOLD)
    //            optionFlags |= PATCH_OPTION_USE_LZX_LARGE;
    //
    //        Program.PrintColorMessage($"Creating patch (options 0x{optionFlags:X8})... ", ConsoleColor.Cyan, false);
    //
    //        if (File.Exists(patchFile))
    //            File.Delete(patchFile);
    //
    //        if (!PInvoke.CreatePatchFileW(sourceFile, targetFile, patchFile, optionFlags, null))
    //        {
    //            Program.PrintColorMessage($"[ERROR] CreatePatchFileW failed: {Marshal.GetLastPInvokeErrorMessage()}", ConsoleColor.Red);
    //            return false;
    //        }
    //
    //        Program.PrintColorMessage("done.", ConsoleColor.Green);
    //
    //        //
    //        // Apply the patch to a copy of the source.
    //        //
    //
    //        Program.PrintColorMessage("Applying patch to copy... ", ConsoleColor.Cyan, false);
    //
    //        Directory.CreateDirectory(testFolder);
    //        File.Copy(sourceFile, testSourceFile, true);
    //
    //        if (File.Exists(testTargetFile))
    //            File.Delete(testTargetFile);
    //
    //        byte[] testSourceBuffer = File.ReadAllBytes(testSourceFile);
    //        byte[] patchBuffer = File.ReadAllBytes(patchFile);
    //        uint* expectedSize = stackalloc uint[1];
    //
    //        fixed (byte* patchPtr = patchBuffer)
    //        fixed (byte* sourcePtr = testSourceBuffer)
    //        {
    //            if (!PInvoke.TestApplyPatchToFileByBuffers(patchPtr, (uint)patchBuffer.Length, sourcePtr, (uint)testSourceBuffer.Length, expectedSize, APPLY_OPTION_NONE))
    //            {
    //                Program.PrintColorMessage($"[ERROR] TestApplyPatchToFileByBuffers failed: {Marshal.GetLastPInvokeErrorMessage()}", ConsoleColor.Red);
    //                return false;
    //            }
    //        }
    //
    //        if (!PInvoke.ApplyPatchToFileW(patchFile, testSourceFile, testTargetFile, APPLY_OPTION_NONE))
    //        {
    //            Program.PrintColorMessage($"[ERROR] ApplyPatchToFileW failed: {Marshal.GetLastPInvokeErrorMessage()}", ConsoleColor.Red);
    //            return false;
    //        }
    //
    //        Program.PrintColorMessage("done.", ConsoleColor.Green);
    //
    //        //
    //        // Results
    //        //
    //
    //        byte[] sourceBuffer = File.ReadAllBytes(sourceFile);
    //        byte[] targetBuffer = File.ReadAllBytes(targetFile);
    //        byte[] appliedBuffer = File.ReadAllBytes(testTargetFile);
    //
    //        Program.PrintColorMessage(string.Empty, ConsoleColor.Gray);
    //        BuildDelta.PrintFileInfo("Source (latest)", sourceFile, sourceBuffer);
    //        BuildDelta.PrintFileInfo("Target (build)", targetFile, targetBuffer);
    //        BuildDelta.PrintFileInfo("Patch", patchFile, patchBuffer);
    //        BuildDelta.PrintFileInfo("Applied (copy)", testTargetFile, appliedBuffer);
    //
    //        Program.PrintColorMessage($"Expected target size (TestApply): {*expectedSize:N0} bytes", ConsoleColor.Gray);
    //
    //        double ratio = targetBuffer.Length == 0 ? 0 : (double)patchBuffer.Length * 100.0 / targetBuffer.Length;
    //        Program.PrintColorMessage($"Patch ratio: {ratio:F2}% of target ({patchBuffer.Length:N0} / {targetBuffer.Length:N0} bytes)", ConsoleColor.Cyan);
    //
    //        if (appliedBuffer.AsSpan().SequenceEqual(targetBuffer))
    //        {
    //            Program.PrintColorMessage("Round-trip: OK (applied output matches build target)", ConsoleColor.Green);
    //            return true;
    //        }
    //        else
    //        {
    //            Program.PrintColorMessage("Round-trip: MISMATCH (applied output differs from build target)", ConsoleColor.Red);
    //            return false;
    //        }
    //    }
    //}
}
