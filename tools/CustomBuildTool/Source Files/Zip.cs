using System;
using System.IO;
using System.IO.Compression;
using System.Text;

namespace CustomBuildTool
{
    public static class Zip
    {
        // https://github.com/phofman/zip/blob/master/src/ZipFile.cs
        private static string[] GetEntryNames(string[] names, string sourceFolder, bool includeBaseName)
        {
            if (names == null || names.Length == 0)
                return new string[0];

            if (includeBaseName)
                sourceFolder = Path.GetDirectoryName(sourceFolder);

            int length = string.IsNullOrEmpty(sourceFolder) ? 0 : sourceFolder.Length;
            if (length > 0 && sourceFolder != null && sourceFolder[length - 1] != Path.DirectorySeparatorChar && sourceFolder[length - 1] != Path.AltDirectorySeparatorChar)
                length++;

            var result = new string[names.Length];
            for (int i = 0; i < names.Length; i++)
            {
                result[i] = names[i].Substring(length);
            }

            return result;
        }

        public static void CreateCompressedFolder(string sourceDirectoryName, string destinationArchiveFileName)
        {
            string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            string[] entryNames = GetEntryNames(filesToAdd, sourceDirectoryName, false);

            if (File.Exists(destinationArchiveFileName))
                File.Delete(destinationArchiveFileName);

            using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                for (int i = 0; i < filesToAdd.Length; i++)
                {
                    // Ignore junk files
                    if (filesToAdd[i].EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                        filesToAdd[i].EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                        filesToAdd[i].EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                        filesToAdd[i].EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                        filesToAdd[i].EndsWith(".lib", StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }

                    // Ignore junk directories
                    if (filesToAdd[i].Contains("bin\\Debug")) // Debug32 Debug64
                    {
                        continue;
                    }

                    archive.CreateEntryFromFile(filesToAdd[i], entryNames[i], CompressionLevel.Optimal);
                }
            }
        }

        public static void CreateCompressedSdkFromFolder(string sourceDirectoryName, string destinationArchiveFileName)
        {
            string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            string[] entryNames = GetEntryNames(filesToAdd, sourceDirectoryName, false);

            if (File.Exists(destinationArchiveFileName))
                File.Delete(destinationArchiveFileName);

            using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                for (int i = 0; i < filesToAdd.Length; i++)
                {
                    archive.CreateEntryFromFile(filesToAdd[i], entryNames[i], CompressionLevel.Optimal);
                }
            }
        }

        public static void CreateCompressedPdbFromFolder(string sourceDirectoryName, string destinationArchiveFileName)
        {
            string[] filesToAdd = Directory.GetFiles(sourceDirectoryName, "*", SearchOption.AllDirectories);
            string[] entryNames = GetEntryNames(filesToAdd, sourceDirectoryName, false);

            if (File.Exists(destinationArchiveFileName))
                File.Delete(destinationArchiveFileName);

            using (FileStream zipFileStream = new FileStream(destinationArchiveFileName, FileMode.Create))
            using (ZipArchive archive = new ZipArchive(zipFileStream, ZipArchiveMode.Create))
            {
                for (int i = 0; i < filesToAdd.Length; i++)
                {
                    // Ignore junk files
                    if (!filesToAdd[i].EndsWith(".pdb", StringComparison.OrdinalIgnoreCase))
                        continue;

                    // Ignore junk directories
                    if (filesToAdd[i].Contains("bin\\Debug") || 
                        filesToAdd[i].Contains("obj\\") || 
                        filesToAdd[i].Contains("tests\\"))
                        continue;

                    archive.CreateEntryFromFile(filesToAdd[i], entryNames[i], CompressionLevel.Optimal);
                }
            }
        }
    }
}