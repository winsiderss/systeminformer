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
    public static class Win32
    {
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
        /// <returns>If the creation succeeds, the return value is nonzero.</returns>
        public static int CreateProcess(string FileName, string Arguments, out string OutputString, bool FixNewLines = true)
        {
            int exitcode = int.MaxValue;
            StringBuilder output = new StringBuilder();
            StringBuilder error = new StringBuilder();

            try
            {
                using (Process process = new Process())
                {
                    process.StartInfo.FileName = FileName;
                    process.StartInfo.Arguments = Arguments;
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.RedirectStandardOutput = true;
                    process.StartInfo.RedirectStandardError = true;
                    process.StartInfo.StandardErrorEncoding = Utils.UTF8NoBOM;
                    process.StartInfo.StandardOutputEncoding = Utils.UTF8NoBOM;

                    using (AutoResetEvent outputWaitHandle = new AutoResetEvent(false))
                    using (AutoResetEvent errorWaitHandle = new AutoResetEvent(false))
                    {
                        process.OutputDataReceived += (sender, e) =>
                        {
                            if (e.Data == null)
                            {
                                outputWaitHandle.Set();
                            }
                            else
                            {
                                output.AppendLine(e.Data);
                            }
                        };
                        process.ErrorDataReceived += (sender, e) =>
                        {
                            if (e.Data == null)
                            {
                                errorWaitHandle.Set();
                            }
                            else
                            {
                                error.AppendLine(e.Data);
                            }
                        };

                        process.Start();
                        process.BeginOutputReadLine();
                        process.BeginErrorReadLine();
                        process.WaitForExit();

                        if (outputWaitHandle.WaitOne() && errorWaitHandle.WaitOne())
                        {
                            exitcode = process.ExitCode;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[CreateProcess] {ex}", ConsoleColor.Red);
            }

            OutputString = output.ToString() + error.ToString();

            if (FixNewLines)
            {
                //OutputString = OutputString.Replace("\n\n", "\r\n", StringComparison.OrdinalIgnoreCase).Trim();
                OutputString = OutputString.Replace("\r\n", string.Empty, StringComparison.OrdinalIgnoreCase).Trim();
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

            using (FileStream file = new FileStream(FileName, options))
            {
                file.Dispose();
            }
        }

        /// <summary>
        /// Deletes the specified file.
        /// </summary>
        /// <param name="FileName">The file name or path.</param>
        public static void DeleteFile(string FileName)
        {
            if (string.IsNullOrWhiteSpace(FileName))
                return;

            if (File.Exists(FileName))
                File.Delete(FileName);
        }

        public static string ShellExecute(string FileName, string Arguments, bool FixNewLines = true)
        {
            CreateProcess(FileName, Arguments, out string outputstring, FixNewLines);

            return outputstring;
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
                string where = Path.Combine(Environment.SystemDirectory, FileName);

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
                    foreach (string path in values.Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries))
                    {
                        string where = Path.Combine(path, FileName);

                        if (File.Exists(where))
                            return where;
                    }
                }
            }

            return null;
        }

        public static void CopyIfNewer(string SourceFile, string DestinationFile)
        {
            if (!File.Exists(SourceFile))
                return;

            {
                string directory = Path.GetDirectoryName(DestinationFile);

                if (string.IsNullOrWhiteSpace(directory))
                    return;

                Win32.CreateDirectory(directory);
            }

            //if (SourceFile.EndsWith(".sys", StringComparison.OrdinalIgnoreCase))
            //{
            //    if (File.Exists(DestinationFile))
            //    {
            //        FileInfo currentFileInfo = new FileInfo(SourceFile);
            //        FileInfo latestFileInfo = new FileInfo(DestinationFile);
            //        FileVersionInfo currentInfo = FileVersionInfo.GetVersionInfo(SourceFile);
            //        FileVersionInfo latestInfo = FileVersionInfo.GetVersionInfo(DestinationFile);
            //        Version currentVersion = Version.Parse(currentInfo.FileVersion ?? "0.0.0.0");
            //        Version latestVersion = Version.Parse(latestInfo.FileVersion ?? "0.0.0.0");
            //
            //        if (
            //            currentVersion > latestVersion ||
            //            currentFileInfo.CreationTimeUtc > latestFileInfo.CreationTimeUtc ||
            //            currentFileInfo.LastWriteTimeUtc > latestFileInfo.LastWriteTimeUtc
            //            )
            //        {
            //            File.Copy(SourceFile, DestinationFile, true);
            //        }
            //    }
            //    else
            //    {
            //        File.Copy(SourceFile, DestinationFile, true);
            //    }
            //}

            if (File.Exists(DestinationFile))
            {
                FileInfo sourceFile = new FileInfo(SourceFile);
                FileInfo destinationFile = new FileInfo(DestinationFile);

                if (
                    sourceFile.CreationTimeUtc > destinationFile.CreationTimeUtc ||
                    sourceFile.LastWriteTimeUtc > destinationFile.LastWriteTimeUtc
                    )
                {
                    File.Copy(SourceFile, DestinationFile, true);
                }
            }
            else
            {
                File.Copy(SourceFile, DestinationFile, true);
            }
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <returns>The value of the environment variable specified by variable, or null if the environment variable is not found.</returns>
        public static string GetEnvironmentVariable(string Name)
        {
            return Environment.ExpandEnvironmentVariables(Name).Replace(Name, string.Empty, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <param name="Value">The value of the environment variable.</param>
        /// <returns>True if the environment variable was found.</returns>
        public static bool GetEnvironmentVariable(string Name, out string Value)
        {
            Value = Environment.ExpandEnvironmentVariables(Name).Replace(Name, string.Empty, StringComparison.OrdinalIgnoreCase);

            return !string.IsNullOrWhiteSpace(Value);
        }

        /// <summary>
        /// Retrieves the value of an environment variable from the current process.
        /// </summary>
        /// <param name="Name">The name of the environment variable.</param>
        /// <returns>True if the environment variable was found.</returns>
        public static bool HasEnvironmentVariable(string Name)
        {
            return GetEnvironmentVariable(Name, out _);
        }

        /// <summary>
        /// Returns the current kernel version information.
        /// </summary>
        /// <returns>The version number of the file or null if the file doesn't exist.</returns>
        public static string GetKernelVersion()
        {
            string filePath = GetEnvironmentVariable("%SystemRoot%\\System32\\ntoskrnl.exe");

            if (File.Exists(filePath))
            {
                FileVersionInfo versionInfo = FileVersionInfo.GetVersionInfo(filePath);
                return versionInfo.FileVersion ?? null;
            }

            return null;
        }

        public static unsafe string GetKeyValue(bool LocalMachine, string KeyName, string ValueName, string DefaultValue)
        {
            string value = string.Empty;
            void* valueBuffer;
            IntPtr keyHandle;

            if (NativeMethods.RegOpenKeyEx(
                LocalMachine ? NativeMethods.HKEY_LOCAL_MACHINE : NativeMethods.HKEY_CURRENT_USER,
                KeyName,
                0,
                NativeMethods.KEY_READ,
                &keyHandle
                ) == 0)
            {
                int valueLength = 0;
                int valueType = 0;

                NativeMethods.RegQueryValueEx(
                    keyHandle,
                    ValueName,
                    IntPtr.Zero,
                    &valueType,
                    null,
                    &valueLength
                    );

                if (valueType == 1 || valueLength > 4)
                {
                    valueBuffer = NativeMemory.Alloc((nuint)valueLength);

                    if (NativeMethods.RegQueryValueEx(
                        keyHandle,
                        ValueName,
                        IntPtr.Zero,
                        null,
                        valueBuffer,
                        &valueLength
                        ) == 0)
                    {
                        value = Marshal.PtrToStringUni((nint)valueBuffer, valueLength / 2 - 1);
                    }

                    NativeMemory.Free(valueBuffer);
                }

                _ = NativeMethods.RegCloseKey(keyHandle);
            }

            return string.IsNullOrWhiteSpace(value) ? DefaultValue : value;
        }

        public static long GetFileSize(string FileName)
        {
            if (NativeMethods.GetFileAttributesEx(FileName, NativeMethods.GetFileExInfoStandard, out var fileAttributeData))
            {
                return ((long)fileAttributeData.FileSizeHigh) << 32 | fileAttributeData.FileSizeLow & 0xFFFFFFFFL;
            }

            return 0;
        }

        public static void SetErrorMode()
        {
            _ = NativeMethods.SetErrorMode(NativeMethods.SEM_NOGPFAULTERRORBOX | NativeMethods.SEM_NOOPENFILEERRORBOX);
        }
    }
}
