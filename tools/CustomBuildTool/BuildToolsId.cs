/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s
 *
 */

namespace CustomBuildTool
{
    /// <summary>
    /// Provides methods for managing and verifying the build tools identity hash.
    /// </summary>
    public static class BuildToolsId
    {
        /// <summary>
        /// Checks if the build tools are out of date by comparing the current tools hash
        /// with the previously stored hash in the <c>ToolsId.txt</c> file.
        /// Prints a warning message if the hashes do not match.
        /// </summary>
        public static void CheckForOutOfDateTools()
        {
#if RELEASE
            string directory = Path.GetDirectoryName(Environment.ProcessPath);
            string fileameId = Path.Join([directory, "\\ToolsId.txt"]);
            string currentId = GetToolsId();
            string previousId = string.Empty;

            if (File.Exists(fileameId))
            {
                previousId = Utils.ReadAllText(fileameId);
            }

            if (string.IsNullOrWhiteSpace(previousId) || !previousId.Equals(currentId, StringComparison.OrdinalIgnoreCase))
            {
                Program.PrintColorMessage($"[WARNING] Build tools are out of date!", ConsoleColor.Yellow);
            }
#endif
        }

        /// <summary>
        /// Writes the current build tools hash to the <c>ToolsId.txt</c> file
        /// and prints the hash to the console.
        /// </summary>
        public static void WriteToolsId()
        {
            string directory = Path.GetDirectoryName(Environment.ProcessPath);
            string fileameId = Path.Join([directory, "\\ToolsId.txt"]);
            string currentHash = GetToolsId();

            Utils.WriteAllText(fileameId, currentHash);

            Program.PrintColorMessage("Tools Hash: ", ConsoleColor.Gray, false);
            Program.PrintColorMessage($"{currentHash}", ConsoleColor.Green);
        }

        /// <summary>
        /// Computes a SHA-256 hash of all <c>.cs</c> files in the specified build tool directories.
        /// The hash represents the current state of the build tools source files.
        /// </summary>
        /// <returns>
        /// A hexadecimal string representing the SHA-256 hash of the build tools source files,
        /// or an empty string if the hash could not be computed.
        /// </returns>
        public static string GetToolsId()
        {
            const int bufferSize = 0x4000;
            string[] directories =
            [
                "tools\\CustomBuildTool",
                "tools\\CustomBuildTool\\AzureSignTool",
                "tools\\CustomBuildTool\\AzureSignTool\\RSAKeyVaultProvider",
            ];

            using (var sha256 = SHA256.Create())
            {
                byte[] buffer = ArrayPool<byte>.Shared.Rent(bufferSize);

                try
                {
                    foreach (var directory in directories)
                    {
                        foreach (string source in Directory.EnumerateFiles(directory, "*.cs", SearchOption.TopDirectoryOnly))
                        {
                            int bytesRead;

                            using (var filestream = new FileStream(source, FileMode.Open, FileAccess.Read, FileShare.Read, bufferSize, FileOptions.SequentialScan))
                            {
                                while ((bytesRead = filestream.Read(buffer, 0, buffer.Length)) > 0)
                                {
                                    sha256.TransformBlock(buffer, 0, bytesRead, null, 0);
                                }
                            }
                        }
                    }
                }
                finally
                {
                    ArrayPool<byte>.Shared.Return(buffer);
                }

                sha256.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
                return sha256.Hash == null ? string.Empty : Convert.ToHexString(sha256.Hash);
            }
        }
    }
}
