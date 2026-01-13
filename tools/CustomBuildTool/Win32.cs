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
    /// Provides utility methods for interacting with the Windows file system, environment variables, processes, and
    /// registry, as well as other Win32-related operations.
    /// </summary>
    public static unsafe class Win32
    {
        private static readonly char[] PathSeparator = [';'];

        /// <summary>
        /// Creates all directories and subdirectories in the specified path unless they already exist.
        /// </summary>
        /// <param name="FileName">The directory name or path.</param>
        public static DirectoryInfo CreateDirectory(string FileName)
        {
            if (string.IsNullOrWhiteSpace(FileName))
                return null;

            if (File.Exists(FileName))
                return null;

            return Directory.CreateDirectory(FileName);
        }

        /// <summary>
        /// Creates a new process and buffers the output into a string.
        /// </summary>
        /// <param name="FileName">The name of the application to start.</param>
        /// <param name="Arguments">The arguments to pass to the application.</param>
        /// <param name="OutputString">The output from the application</param>
        /// <param name="FixNewLines"></param>
        /// <param name="RedirectOutput"></param>
        /// <returns>If the creation succeeds, the return value is nonzero.</returns>
        public static int CreateProcess(string FileName, string Arguments, out string OutputString, bool FixNewLines = true, bool RedirectOutput = true)
        {
            int exitcode = int.MaxValue;
            StringBuilder output = new StringBuilder(0x1000);
            StringBuilder error = new StringBuilder(0x1000);

            try
            {
                using (Process process = new Process())
                {
                    process.StartInfo.FileName = FileName;
                    process.StartInfo.Arguments = Arguments;
                    process.StartInfo.UseShellExecute = false;

                    if (RedirectOutput)
                    {
                        process.StartInfo.RedirectStandardOutput = true;
                        process.StartInfo.RedirectStandardError = true;
                        process.StartInfo.StandardErrorEncoding = Utils.UTF8NoBOM;
                        process.StartInfo.StandardOutputEncoding = Utils.UTF8NoBOM;
                        process.OutputDataReceived += (_, e) => { output.AppendLine(e.Data); };
                        process.ErrorDataReceived += (_, e) => { error.AppendLine(e.Data); };
                    }

                    process.Start();

                    if (RedirectOutput)
                    {
                        process.BeginOutputReadLine();
                        process.BeginErrorReadLine();
                    }

                    process.WaitForExit();

                    exitcode = process.ExitCode;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[CreateProcess] {ex}", ConsoleColor.Red);
            }

            OutputString = (output.ToString().Trim() + error.ToString().Trim());

            if (FixNewLines)
            {
                OutputString = OutputString.Replace("\n\n", "\r\n", StringComparison.OrdinalIgnoreCase).Trim();
            }

            return exitcode;
        }

        /// <summary>
        /// Create an empty file.
        /// </summary>
        /// <param name="FileName">The file name or path.</param>
        public static void CreateEmptyFile(string FileName)
        {
            if (string.IsNullOrWhiteSpace(FileName))
                return;

            FileStreamOptions options = new FileStreamOptions
            {
                Mode = FileMode.Create, // Overwrite
                Access = FileAccess.Write,
                Share = FileShare.None
            };

            new FileStream(FileName, options).Dispose();
        }

        /// <summary>
        /// Deletes the specified file.
        /// </summary>
        /// <param name="FileName">The file name or path.</param>
        public static void DeleteFile(string FileName, BuildFlags Flags = BuildFlags.None)
        {
            if (string.IsNullOrWhiteSpace(FileName))
                return;

            if (File.Exists(FileName))
            {
                if (Flags.HasFlag(BuildFlags.BuildVerbose))
                {
                    Program.PrintColorMessage($"Deleting: {FileName}", ConsoleColor.DarkGray);
                }

                File.Delete(FileName);
            }
        }

        /// <summary>
        /// Searches for a specified file using environment paths.
        /// </summary>
        /// <param name="FileName">The name of the file for which to search.</param>
        /// <returns>The path and file name of the file found.</returns>
        public static string SearchPath(string FileName)
        {
            if (string.IsNullOrWhiteSpace(FileName))
                return null;

            // System32 directory.
            {
                string where = Path.Join([Environment.SystemDirectory, FileName]);

                if (File.Exists(where))
                    return where;
            }

            // Current directory.
            {
                if (File.Exists(FileName))
                {
                    return Path.GetFullPath(FileName);
                }
            }

            // %PATH% directory.
            {
                if (Win32.GetEnvironmentVariable("PATH", out string values))
                {
                    foreach (string path in values.Split(PathSeparator, StringSplitOptions.RemoveEmptyEntries))
                    {
                        string where = Path.Join([path, FileName]);

                        if (File.Exists(where))
                            return where;
                    }
                }
            }

            return null;
        }

        /// <summary>
        /// Copies the source file to the destination file if the source is newer or the destination does not exist.
        /// Preserves creation and last write times, and optionally sets the read-only attribute.
        /// </summary>
        /// <param name="SourceFile">The path to the source file.</param>
        /// <param name="DestinationFile">The path to the destination file.</param>
        /// <param name="Flags">Build flags controlling output verbosity and behavior.</param>
        /// <param name="ReadOnly">If true, sets the destination file as read-only after copying.</param>
        public static void CopyIfNewer(string SourceFile, string DestinationFile, BuildFlags Flags = BuildFlags.None, bool ReadOnly = false)
        {
            bool updated = false;

            if (!File.Exists(SourceFile))
            {
                if (!Build.BuildIntegration && SourceFile.EndsWith(".sig", StringComparison.OrdinalIgnoreCase))
                    return;

                Program.PrintColorMessage($"[SDK] [CopyIfNewer-FileNotFound] {SourceFile}", ConsoleColor.Yellow);
                return;
            }

            if (string.IsNullOrWhiteSpace(DestinationFile))
            {
                Program.PrintColorMessage($"[SDK] [CopyIfNewer-DestinationFile-null]", ConsoleColor.Yellow);
                return;
            }

            {
                string directory = Path.GetDirectoryName(DestinationFile);

                if (string.IsNullOrWhiteSpace(directory))
                {
                    Program.PrintColorMessage($"[SDK] [CopyIfNewer-DestinationFile-null]", ConsoleColor.Yellow);
                    return;
                }

                Win32.CreateDirectory(directory);
            }

            if (File.Exists(DestinationFile))
            {
                GetFileBasicInfo(SourceFile, out var sourceCreationTime, out var sourceWriteTime, out var SourceAttributes);
                GetFileBasicInfo(DestinationFile, out var destinationCreationTime, out var destinationWriteTime, out var DestinationAttributes);

                if (!(sourceCreationTime == destinationCreationTime && sourceWriteTime == destinationWriteTime))
                {
                    if ((DestinationAttributes & FileAttributes.ReadOnly) != 0)
                        File.SetAttributes(DestinationFile, FileAttributes.Normal);
                    File.Copy(SourceFile, DestinationFile, true);
                    SetFileBasicInfo(DestinationFile, sourceCreationTime, sourceWriteTime, ReadOnly);
                    updated = true;
                }
            }
            else
            {
                GetFileBasicInfo(SourceFile, out var sourceCreationTime, out var sourceWriteTime, out _);
                File.Copy(SourceFile, DestinationFile, true);
                SetFileBasicInfo(DestinationFile, sourceCreationTime, sourceWriteTime, ReadOnly);
                updated = true;
            }

            if (updated)
            {
                Program.PrintColorMessage($"[SDK] {SourceFile} copied to {DestinationFile}", ConsoleColor.Green, true, Flags);
            }
            //else
            //    Program.PrintColorMessage($"[SDK] Skipped: {SourceFile} same as {DestinationFile}", ConsoleColor.DarkGray, true, Flags);
        }

        /// <summary>
        /// Copies the source file to the destination file if the source has a newer last write time or a higher file version.
        /// </summary>
        /// <param name="SourceFile">The path to the source file.</param>
        /// <param name="DestinationFile">The path to the destination file.</param>
        /// <param name="Flags">Build flags controlling output verbosity and behavior.</param>
        public static void CopyVersionIfNewer(string SourceFile, string DestinationFile, BuildFlags Flags = BuildFlags.None)
        {
            bool updated = false;

            if (!File.Exists(SourceFile))
            {
                Program.PrintColorMessage($"[CopyVersionIfNewer-FileNotFound] {SourceFile}", ConsoleColor.Yellow);
                return;
            }

            if (string.IsNullOrWhiteSpace(DestinationFile))
            {
                Program.PrintColorMessage($"[CopyVersionIfNewer-DestinationFile]", ConsoleColor.Yellow);
                return;
            }

            {
                string directory = Path.GetDirectoryName(DestinationFile);

                if (string.IsNullOrWhiteSpace(directory))
                    return;

                Win32.CreateDirectory(directory);
            }

            if (File.Exists(DestinationFile))
            {
                if (
                    File.GetLastWriteTime(SourceFile) > File.GetLastWriteTime(DestinationFile) ||
                    Win32.GetFileVersion(SourceFile) > Win32.GetFileVersion(DestinationFile)
                    )
                {
                    File.Copy(SourceFile, DestinationFile, true);
                    updated = true;
                }
            }
            else
            {
                File.Copy(SourceFile, DestinationFile, true);
                updated = true;
            }

            if (updated)
            {
                Program.PrintColorMessage($"[SDK] {SourceFile} copied to {DestinationFile}", ConsoleColor.Green, true, Flags);
            }
            //else
            //    Program.PrintColorMessage($"[SDK] Skipped: {SourceFile} older than {DestinationFile}", ConsoleColor.DarkGray, true, Flags);
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <returns>The value of the environment variable specified by variable, or null if the environment variable is not found.</returns>
        public static string GetEnvironmentVariable(string Name)
        {
            return Environment.GetEnvironmentVariable(Name, EnvironmentVariableTarget.Process);
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <param name="Value">The value of the environment variable.</param>
        /// <returns>True if the environment variable was found.</returns>
        public static bool GetEnvironmentVariable(string Name, out string Value)
        {
            Value = Environment.GetEnvironmentVariable(Name, EnvironmentVariableTarget.Process);

            if (string.IsNullOrWhiteSpace(Value))
            {
                if (Build.BuildToolsDebug)
                {
                    Program.PrintColorMessage($"EnvironmentVariable: {Name} not found.", ConsoleColor.Red);
                }
                return false;
            }

            return true;
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <param name="Value">The value of the environment variable.</param>
        /// <returns>True if the environment variable was found.</returns>
        public static bool GetEnvironmentVariableSpan(string Name, out ReadOnlySpan<char> Value)
        {
            var value = Environment.GetEnvironmentVariable(Name, EnvironmentVariableTarget.Process);
            Value = value.AsSpan();

            if (Utils.IsSpanNullOrWhiteSpace(Value))
            {
                if (Build.BuildToolsDebug)
                {
                    Program.PrintColorMessage($"EnvironmentVariable: {Name} not found.", ConsoleColor.Red);
                }
                return false;
            }

            return true;
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <returns>True if the environment variable was found.</returns>
        public static bool HasEnvironmentVariable(string Name)
        {
            return GetEnvironmentVariableSpan(Name, out ReadOnlySpan<char> _);
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <returns>True if the environment variable was found.</returns>
        public static string ExpandEnvironmentVariables(string Name)
        {
            return Environment.ExpandEnvironmentVariables(Name).Replace(Name, string.Empty, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Returns the current kernel version information.
        /// </summary>
        /// <returns>The version number of the file or null if the file doesn't exist.</returns>
        public static string GetKernelVersion()
        {
            string filePath = ExpandEnvironmentVariables("%SystemRoot%\\System32\\ntoskrnl.exe");

            if (File.Exists(filePath))
            {
                FileVersionInfo versionInfo = FileVersionInfo.GetVersionInfo(filePath);
                return versionInfo.FileVersion;
            }

            return null;
        }

        /// <summary>
        /// Retrieves a string value from the Windows registry.
        /// </summary>
        /// <param name="LocalMachine">If true, queries HKEY_LOCAL_MACHINE; otherwise, HKEY_CURRENT_USER.</param>
        /// <param name="KeyName">The registry key path.</param>
        /// <param name="ValueName">The name of the value to retrieve. (Note: This parameter is not used in the current implementation.)</param>
        /// <param name="DefaultValue">The value to return if the registry value is not found or is empty.</param>
        /// <returns>The string value from the registry, or <paramref name="DefaultValue"/> if not found.</returns>
        public static string GetKeyValue(bool LocalMachine, string KeyName, string ValueName, string DefaultValue)
        {
            string value = string.Empty;
            byte* valueBuffer;
            HKEY keyHandle;

            fixed (char* p = KeyName)
            {
                if (PInvoke.RegOpenKeyEx(
                    LocalMachine ? HKEY.HKEY_LOCAL_MACHINE : HKEY.HKEY_CURRENT_USER,
                    p,
                    0,
                    REG_SAM_FLAGS.KEY_READ,
                    &keyHandle
                    ) == WIN32_ERROR.ERROR_SUCCESS)
                {
                    uint valueLength = 0;
                    REG_VALUE_TYPE valueType = 0;

                    PInvoke.RegQueryValueEx(
                        keyHandle,
                        p,
                        null,
                        &valueType,
                        null,
                        &valueLength
                        );

                    if (valueType == REG_VALUE_TYPE.REG_SZ || valueLength > 4)
                    {
                        valueBuffer = (byte*)NativeMemory.Alloc(valueLength);

                        if (PInvoke.RegQueryValueEx(
                            keyHandle,
                            p,
                            null,
                            null,
                            valueBuffer,
                            &valueLength
                            ) == WIN32_ERROR.ERROR_SUCCESS)
                        {
                            value = new string((char*)valueBuffer, 0, (int)valueLength / 2 - 1);
                        }

                        NativeMemory.Free(valueBuffer);
                    }

                    _ = PInvoke.RegCloseKey(keyHandle);
                }
            }

            return string.IsNullOrWhiteSpace(value) ? DefaultValue : value;
        }

        /// <summary>
        /// Gets the size of a file in bytes.
        /// </summary>
        /// <param name="FileName">The path to the file.</param>
        /// <returns>The size of the file in bytes, or 0 if the file does not exist or cannot be accessed.</returns>
        public static ulong GetFileSize(string FileName)
        {
            WIN32_FILE_ATTRIBUTE_DATA sourceFile = new WIN32_FILE_ATTRIBUTE_DATA();

            if (PInvoke.GetFileAttributesEx(
                FileName,
                GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard,
                &sourceFile
                ))
            {
                return ((ulong)sourceFile.nFileSizeHigh << 32) | (uint)sourceFile.nFileSizeLow;
            }

            return 0;
        }

        /// <summary>
        /// Retrieves the version information of a file.
        /// </summary>
        /// <param name="FileName">The path to the file.</param>
        /// <returns>The <see cref="Version"/> of the file, or 0.0.0.0 if not found.</returns>
        public static Version GetFileVersion(string FileName)
        {
            FileVersionInfo versionInfo = FileVersionInfo.GetVersionInfo(FileName);

            if (string.IsNullOrWhiteSpace(versionInfo.FileVersion))
            {
                return Version.Parse("0.0.0.0");
            }

            return Version.Parse(versionInfo.FileVersion);
        }

        /// <summary>
        /// Sets the current process priority class to high.
        /// </summary>
        public static void SetErrorMode()
        {
            PInvoke.SetPriorityClass(
                PInvoke.GetCurrentProcess(),
                PROCESS_CREATION_FLAGS.HIGH_PRIORITY_CLASS
                );
        }

        /// <summary>
        /// Sets the current process priority class to high.
        /// </summary>
        public static void SetBasePriority()
        {
            //Process.GetCurrentProcess().PriorityClass = ProcessPriorityClass.High;
            PInvoke.SetPriorityClass(
                PInvoke.GetCurrentProcess(),
                PROCESS_CREATION_FLAGS.HIGH_PRIORITY_CLASS
                );
        }

        /// <summary>
        /// Sets the basic file information, including creation and last write times, and optionally the read-only attribute.
        /// </summary>
        /// <param name="FileName">The path to the file.</param>
        /// <param name="CreationDateTime">The creation time to set. If <see cref="DateTime.MinValue"/>, uses current UTC time.</param>
        /// <param name="LastWriteDateTime">The last write time to set. If <see cref="DateTime.MinValue"/>, uses current UTC time.</param>
        /// <param name="ReadOnly">Whether to set the file as read-only.</param>
        /// <exception cref="System.ComponentModel.Win32Exception">Thrown if the file information cannot be set.</exception>
        public static void SetFileBasicInfo(
            string FileName,
            DateTime CreationDateTime,
            DateTime LastWriteDateTime,
            bool ReadOnly
            )
        {
            FILE_BASIC_INFO basicInfo = new FILE_BASIC_INFO();

            using (var fs = new FileStream(
                FileName,
                FileMode.Open,
                FileAccess.ReadWrite,
                FileShare.ReadWrite | FileShare.Delete,
                0,
                FileOptions.None
                ))
            {
                var handle = new HANDLE(fs.SafeFileHandle.DangerousGetHandle());

                if (!PInvoke.GetFileInformationByHandleEx(handle, FILE_INFO_BY_HANDLE_CLASS.FileBasicInfo, &basicInfo, (uint)sizeof(FILE_BASIC_INFO)))
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());

                basicInfo.CreationTime = CreationDateTime == DateTime.MinValue ? DateTime.UtcNow.ToFileTimeUtc() : CreationDateTime.ToFileTimeUtc();
                basicInfo.LastWriteTime = LastWriteDateTime == DateTime.MinValue ? DateTime.UtcNow.ToFileTimeUtc() : LastWriteDateTime.ToFileTimeUtc();

                if (!Build.BuildIntegration) // Skip read-only attribute during build integration (dmex)
                {
                    if (ReadOnly)
                        basicInfo.FileAttributes |= (uint)FileAttributes.ReadOnly;
                    else
                        basicInfo.FileAttributes &= ~(uint)FileAttributes.ReadOnly;
                }

                PInvoke.SetFileInformationByHandle(handle, FILE_INFO_BY_HANDLE_CLASS.FileBasicInfo, &basicInfo, (uint)sizeof(FILE_BASIC_INFO));
            }
        }

        /// <summary>
        /// Retrieves basic file information, including creation time, last write time, and attributes.
        /// </summary>
        /// <param name="FileName">The path to the file.</param>
        /// <param name="CreationTime">When this method returns, contains the creation time of the file.</param>
        /// <param name="LastWriteDateTime">When this method returns, contains the last write time of the file.</param>
        /// <param name="Attributes">When this method returns, contains the file attributes.</param>
        /// <returns>True if the file exists and information was retrieved; otherwise, false.</returns>
        /// <exception cref="System.ComponentModel.Win32Exception">Thrown if the file information cannot be retrieved.</exception>
        public static bool GetFileBasicInfo(
            string FileName,
            out DateTime CreationTime,
            out DateTime LastWriteDateTime,
            out FileAttributes Attributes
            )
        {
            FILE_BASIC_INFO basicInfo = new FILE_BASIC_INFO();

            CreationTime = DateTime.MinValue;
            LastWriteDateTime = DateTime.MinValue;

            if (!File.Exists(FileName))
            {
                Attributes = FileAttributes.None;
                return false;
            }

            using (var fs = File.OpenRead(FileName))
            {
                var handle = new HANDLE(fs.SafeFileHandle.DangerousGetHandle());

                if (!PInvoke.GetFileInformationByHandleEx(handle, FILE_INFO_BY_HANDLE_CLASS.FileBasicInfo, &basicInfo, (uint)sizeof(FILE_BASIC_INFO)))
                    throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());

                CreationTime = DateTime.FromFileTimeUtc(basicInfo.CreationTime);
                LastWriteDateTime = DateTime.FromFileTimeUtc(basicInfo.LastWriteTime);
                Attributes = (FileAttributes)basicInfo.FileAttributes;
            }

            return true;
        }
    }
}
