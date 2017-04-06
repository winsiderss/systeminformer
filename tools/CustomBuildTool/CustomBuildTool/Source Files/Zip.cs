using System;
using System.IO;
using ICSharpCode.SharpZipLib.Zip;

namespace CustomBuildTool
{
    public static class Zip
    {
        public static void CreateCompressedFolder(string outPathname, string folderName)
        {
            using (FileStream fs = File.Create(outPathname))
            using (ZipOutputStream zipStream = new ZipOutputStream(fs))
            {
                zipStream.SetLevel(9);

                int folderOffset = folderName.Length + (folderName.EndsWith("\\", StringComparison.OrdinalIgnoreCase) ? 0 : 1);

                CompressFolder(folderName, zipStream, folderOffset);

                zipStream.IsStreamOwner = true;
            }
        }

        private static void CompressFolder(string path, ZipOutputStream zipStream, int folderOffset)
        {
            string[] files = Directory.GetFiles(path);

            foreach (string filename in files)
            {
                //    "-x!*.pdb " +  // # Ignore junk files
                //    "-x!*.iobj " +
                //    "-x!*.ipdb " +
                //    "-x!*.exp " +
                //    "-x!*.lib "
                if (filename.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                    filename.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                    filename.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                    filename.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                    filename.EndsWith(".lib", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                FileInfo fi = new FileInfo(filename);
                string entryName = filename.Substring(folderOffset);
                entryName = ZipEntry.CleanName(entryName);

                ZipEntry newEntry = new ZipEntry(entryName);
                newEntry.DateTime = fi.LastWriteTime;
                newEntry.Size = fi.Length;

                zipStream.PutNextEntry(newEntry);

                using (FileStream streamReader = File.OpenRead(filename))
                {
                    streamReader.CopyTo(zipStream);
                }

                zipStream.CloseEntry();
            }

            string[] folders = Directory.GetDirectories(path);
            foreach (string folder in folders)
            {                
                //    "-xr!Debug32 " + // # Ignore junk directories
                //    "-xr!Debug64 " +
                if (!folder.Contains("bin\\Debug"))
                {
                    CompressFolder(folder, zipStream, folderOffset);
                }
            }
        }
    }
}