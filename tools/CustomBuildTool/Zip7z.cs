/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 */

namespace CustomBuildTool
{
    public static partial class Zip
    {
        private static string FindSevenZipEncoder()
        {
            foreach (string name in new[] { "7z.exe", "7zz.exe", "7za.exe" })
            {
                string path = Win32.SearchPath(name);

                if (path != null)
                    return path;
            }

            foreach (var folder in new[] { Environment.SpecialFolder.ProgramFiles, Environment.SpecialFolder.ProgramFilesX86 })
            {
                string path = Path.Join(Environment.GetFolderPath(folder), "7-Zip", "7z.exe");

                if (File.Exists(path))
                    return path;
            }

            return null;
        }

        private static bool IsSevenZipBcj2Image(string FileName)
        {
            using var stream = File.OpenRead(FileName);

            try
            {
                using var image = new PEReader(stream);
                var headers = image.PEHeaders;

                return headers.PEHeader != null &&
                    (headers.CoffHeader.Characteristics & Characteristics.ExecutableImage) != 0 &&
                    headers.CoffHeader.Machine is Machine.I386 or Machine.Amd64;
            }
            catch (BadImageFormatException)
            {
                return false;
            }
        }

        private static void CreateSevenZipArchive(
            string Encoder,
            List<(string FileName, string EntryName)> Entries,
            string DestinationArchiveFileName,
            BuildFlags Flags
            )
        {
            // SharpCompress 0.50.4 cannot encode BCJ2 or describe a per-entry filter pipeline.
            string encoder = Encoder;
            string destination = Path.GetFullPath(DestinationArchiveFileName);
            string temporaryArchive = Path.Join(Path.GetDirectoryName(destination), $".{Guid.NewGuid():N}.7z");
            string temporaryDirectory = Directory.CreateTempSubdirectory("CustomBuildTool-7z-").FullName;

            try
            {
                string payloadDirectory = Path.Join(temporaryDirectory, "payload");
                Directory.CreateDirectory(payloadDirectory);
                var plainEntries = new List<string>();
                var bcj2Entries = new List<string>();

                // Stage the transformed entry names without changing the process working directory.
                foreach (var (fileName, entryName) in Entries)
                {
                    string stagedFile = Path.Join(payloadDirectory, entryName);
                    Directory.CreateDirectory(Path.GetDirectoryName(stagedFile));
                    File.Copy(fileName, stagedFile);
                    // Do not inherit read-only attributes that would prevent staging cleanup.
                    File.SetAttributes(stagedFile, FileAttributes.Normal);

                    if (IsSevenZipBcj2Image(stagedFile))
                        bcj2Entries.Add(entryName);
                    else
                        plainEntries.Add(entryName);
                }

                // Separate non-solid update passes prevent BCJ2 from being applied to ARM64 or data.
                if (plainEntries.Count != 0 || bcj2Entries.Count == 0)
                    WriteSevenZipEntries(encoder, temporaryArchive, temporaryDirectory, payloadDirectory, plainEntries, false, Flags);

                if (bcj2Entries.Count != 0)
                    WriteSevenZipEntries(encoder, temporaryArchive, temporaryDirectory, payloadDirectory, bcj2Entries, true, Flags);

                File.Move(temporaryArchive, destination, true);
            }
            finally
            {
                File.Delete(temporaryArchive);
                Directory.Delete(temporaryDirectory, true);
            }
        }

        private static void WriteSevenZipEntries(
            string Encoder,
            string ArchiveFileName,
            string TemporaryDirectory,
            string PayloadDirectory,
            List<string> Entries,
            bool UseBcj2,
            BuildFlags Flags
            )
        {
            string listFile = Path.Join(TemporaryDirectory, UseBcj2 ? "bcj2.txt" : "plain.txt");
            File.WriteAllLines(listFile, Entries, Utils.UTF8NoBOM);

            var startInfo = new ProcessStartInfo(Encoder)
            {
                WorkingDirectory = PayloadDirectory,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8
            };

            foreach (string argument in new[] { "a", "-t7z", "-mx=7", "-ms=off", "-mmt=1", "-mf=off", "-mtc=off", "-mta=off", "-scsUTF-8", "-sccUTF-8", "-spd", "-sse", "-y", Flags.HasFlag(BuildFlags.BuildVerbose) ? "-bb1" : "-bb0" })
                startInfo.ArgumentList.Add(argument);

            if (UseBcj2)
            {
                foreach (string argument in new[] { "-m0=BCJ2", "-m1=LZMA:d=32m:fb=64:mf=bt4", "-m2=LZMA:d=32m:fb=64:mf=bt4", "-m3=LZMA:d=32m:fb=64:mf=bt4", "-mb0:1", "-mb0s1:2", "-mb0s2:3" })
                    startInfo.ArgumentList.Add(argument);
            }
            else
            {
                startInfo.ArgumentList.Add("-m0=LZMA:d=32m:fb=64:mf=bt4");
            }

            startInfo.ArgumentList.Add(ArchiveFileName);
            startInfo.ArgumentList.Add("@" + listFile);

            using var process = Process.Start(startInfo);
            var output = process.StandardOutput.ReadToEndAsync();
            var error = process.StandardError.ReadToEndAsync();
            process.WaitForExit();
            string outputText = output.GetAwaiter().GetResult();
            string errorText = error.GetAwaiter().GetResult();

            // Warnings can mean missing input files; do not publish an incomplete package.
            if (process.ExitCode != 0)
                throw new IOException($"7-Zip failed with exit code {process.ExitCode}: {outputText}{errorText}");

            if (Flags.HasFlag(BuildFlags.BuildVerbose))
                Program.PrintColorMessage(outputText, ConsoleColor.Gray);
        }
    }
}
