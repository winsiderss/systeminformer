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
    public static unsafe partial class Win32
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
            // required for the ESDK
            string value = string.Empty;
            byte* valueBuffer;
            HKEY keyHandle;

            fixed (char* pKey = KeyName)
            fixed (char* pValue = ValueName)
            {
                if (PInvoke.RegOpenKeyEx(
                    LocalMachine ? HKEY.HKEY_LOCAL_MACHINE : HKEY.HKEY_CURRENT_USER,
                    pKey,
                    0,
                    REG_SAM_FLAGS.KEY_READ,
                    &keyHandle
                    ) == WIN32_ERROR.ERROR_SUCCESS)
                {
                    uint valueLength = 0;
                    REG_VALUE_TYPE valueType = 0;

                    PInvoke.RegQueryValueEx(
                        keyHandle,
                        pValue,
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
                            pValue,
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
        /// Filters the PATH environment variable to include only directories essential for building System Informer.
        /// Removes scripting languages (Python, Ruby, Node.js) and other non-essential tools.
        /// </summary>
        /// <remarks>
        /// This method is called during build initialization and keeps essential build tools 
        /// while removing potentially unnecessary dependencies.
        /// </remarks>
        public static void SetPathEnvironment()
        {
            if (!GetEnvironmentVariable("PATH", out string currentPath))
                return;

            List<string> allowedPaths = new List<string>();
            string[] pathEntries = currentPath.Split(PathSeparator, StringSplitOptions.RemoveEmptyEntries);

            string[] requiredEntries =
            [
                // Windows system directories
                "\\Windows\\System32",
                "\\Windows\\",
                "\\Windows\\System32\\Wbem",
                "\\Windows\\System32\\WindowsPowerShell",
                "\\Windows\\System32\\OpenSSH",
                
                // Build tools
                "\\Microsoft Visual Studio",
                "\\MSBuild",
                "\\Windows Kits",
                "\\dotnet",
                
                // Version control
                "\\git",
                "\\GitHub CLI",
                
                // CMake and build tools
                "\\CMake",
                "\\vcpkg",
                "\\ninja",
                "\\LLVM",
                "\\mingw",
            ];

            foreach (string entry in pathEntries)
            {
                bool isEssential = false;

                foreach (string pattern in requiredEntries)
                {
                    if (entry.Contains(pattern, StringComparison.OrdinalIgnoreCase))
                    {
                        isEssential = true;
                        break;
                    }
                }

                if (isEssential)
                {
                    allowedPaths.Add(entry);
                }
            }

            if (allowedPaths.Count > 0)
            {
                string filteredPath = string.Join(';', allowedPaths);

                if (!string.IsNullOrWhiteSpace(filteredPath))
                {
                    Environment.SetEnvironmentVariable("PATH", filteredPath, EnvironmentVariableTarget.Process);
                }
                
                //Program.PrintColorMessage($"[PATH] filtered: {pathEntries.Length} entries -> {allowedPaths.Count} entries", ConsoleColor.DarkGray);
            }
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

        /// <summary>
        /// Sets low (untrusted) integrity level for processes.
        /// </summary>
        /// <remarks>
        /// This method enumerates all running processes, identifies those with executables in
        /// temporary folders (containing "\Temp\"), and attempts to lower their integrity level
        /// to Untrusted (S-1-16-0). This is a security mitigation technique.
        /// </remarks>
        public static void SetLowIntegrityForProcesses()
        {
            const uint PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;
            const uint TOKEN_QUERY = 0x0008;
            const uint TOKEN_ADJUST_DEFAULT = 0x0080;
            const uint SE_GROUP_INTEGRITY = 0x00000020;
            const uint SECURITY_MANDATORY_UNTRUSTED_RID = 0x00000000;
            const int TokenIntegrityLevel = 25;

            var processesToModify = new List<Process>();
            
            try
            {
                var allProcesses = Process.GetProcesses();

                foreach (var process in allProcesses)
                {
                    try
                    {
                        var filename = process.MainModule?.FileName;
                        if (!string.IsNullOrEmpty(filename) && 
                            filename.Contains("\\Temp\\", StringComparison.OrdinalIgnoreCase))
                        {
                            Program.PrintColorMessage($"Process: {filename}", ConsoleColor.DarkGray);
                            processesToModify.Add(process);
                        }
                        else
                        {
                            process.Dispose();
                        }
                    }
                    catch
                    {
                        process.Dispose();
                    }
                }

                foreach (var process in processesToModify)
                {
                    try
                    {
                        ApplyLowIntegrity(
                            process, 
                            PROCESS_QUERY_LIMITED_INFORMATION,
                            TOKEN_QUERY | TOKEN_ADJUST_DEFAULT,
                            SE_GROUP_INTEGRITY,
                            SECURITY_MANDATORY_UNTRUSTED_RID,
                            TokenIntegrityLevel
                            );
                    }
                    catch (Exception ex)
                    {
                        Program.PrintColorMessage(
                            $"Failed to set integrity for {process.ProcessName} (PID: {process.Id}): {ex.Message}", 
                            ConsoleColor.Yellow
                            );
                    }
                    finally
                    {
                        process.Dispose();
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Error enumerating processes: {ex.Message}", ConsoleColor.Red);
            }
        }

        private static void ApplyLowIntegrity(
            Process process,
            uint processAccess,
            uint tokenAccess,
            uint sidAttributes,
            uint mandatoryRid,
            int tokenInfoClass
            )
        {
            HANDLE processHandle = default;
            HANDLE tokenHandle = default;

            try
            {
                processHandle = PInvoke.OpenProcess((PROCESS_ACCESS_RIGHTS)processAccess, false, (uint)process.Id);

                if (processHandle.IsNull)
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(), $"OpenProcess failed for {process.ProcessName}");
                }

                if (!PInvoke.OpenProcessToken(processHandle, (TOKEN_ACCESS_MASK)tokenAccess, &tokenHandle))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "OpenProcessToken failed");
                }

                // Manually construct the Untrusted Integrity SID (S-1-16-0)
                // SID structure: Revision(1) + SubAuthorityCount(1) + Authority(6 bytes) + SubAuthority(4 bytes)
                byte* sidBuffer = stackalloc byte[12];
                NativeMemory.Clear(sidBuffer, 12);
                sidBuffer[0] = 1;  // SID_REVISION
                sidBuffer[1] = 1;  // SubAuthorityCount
                sidBuffer[2] = 0;  // IdentifierAuthority[0]
                sidBuffer[3] = 0;  // IdentifierAuthority[1]
                sidBuffer[4] = 0;  // IdentifierAuthority[2]
                sidBuffer[5] = 0;  // IdentifierAuthority[3]
                sidBuffer[6] = 0;  // IdentifierAuthority[4]
                sidBuffer[7] = 16; // IdentifierAuthority[5] - SECURITY_MANDATORY_LABEL_AUTHORITY (S-1-16)          
                *(uint*)(sidBuffer + 8) = mandatoryRid; // SubAuthority[0] = mandatoryRid (little-endian)
                var integritySid = new PSID(sidBuffer);

                var tokenMandatoryLabel = new TOKEN_MANDATORY_LABEL
                {
                    Label = new SID_AND_ATTRIBUTES
                    {
                        Attributes = sidAttributes,
                        Sid = integritySid
                    }
                };

                if (!PInvoke.SetTokenInformation(
                    tokenHandle,
                    (TOKEN_INFORMATION_CLASS)tokenInfoClass,
                    &tokenMandatoryLabel,
                    (uint)sizeof(TOKEN_MANDATORY_LABEL)
                    ))
                {
                    throw new Win32Exception(Marshal.GetLastWin32Error(), "SetTokenInformation failed");
                }

                Program.PrintColorMessage($"Successfully set low integrity for {process.ProcessName} (PID: {process.Id})", ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Error setting integrity: {ex.Message}", ConsoleColor.Red);
            }
            finally
            {
                if (!tokenHandle.IsNull)
                    PInvoke.CloseHandle(tokenHandle);
                
                if (!processHandle.IsNull)
                    PInvoke.CloseHandle(processHandle);
            }
        }
    }
}
