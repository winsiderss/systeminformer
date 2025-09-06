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
    public static class BuildToolsId
    {
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

        public static void WriteToolsId()
        {
            string directory = Path.GetDirectoryName(Environment.ProcessPath);
            string fileameId = Path.Join([directory, "\\ToolsId.txt"]);
            string currentHash = GetToolsId();
            Utils.WriteAllText(fileameId, currentHash);
            Program.PrintColorMessage("Tools Hash: ", ConsoleColor.Gray, false);
            Program.PrintColorMessage($"{currentHash}", ConsoleColor.Green);
        }

        public static string GetToolsId()
        {
            const int bufferSize = 0x1000;
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

                            using (var filestream = File.OpenRead(source))
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
