/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 */

namespace CustomBuildTool
{
    using unsafe ARCHIVE_WRITE_NEW = delegate* unmanaged<nint>;
    using unsafe ARCHIVE_WRITE_SET_FORMAT_7ZIP = delegate* unmanaged<nint, int>;
    using unsafe ARCHIVE_WRITE_SET_FORMAT_OPTION = delegate* unmanaged<nint, byte*, byte*, byte*, int>;
    using unsafe ARCHIVE_WRITE_OPEN_FILENAME_W = delegate* unmanaged<nint, char*, int>;
    using unsafe ARCHIVE_WRITE_HEADER = delegate* unmanaged<nint, nint, int>;
    using unsafe ARCHIVE_WRITE_DATA = delegate* unmanaged<nint, void*, nuint, nint>;
    using unsafe ARCHIVE_WRITE_FINISH_ENTRY = delegate* unmanaged<nint, int>;
    using unsafe ARCHIVE_WRITE_CLOSE = delegate* unmanaged<nint, int>;
    using unsafe ARCHIVE_WRITE_FREE = delegate* unmanaged<nint, int>;
    using unsafe ARCHIVE_ERROR_STRING = delegate* unmanaged<nint, byte*>;
    using unsafe ARCHIVE_ENTRY_NEW = delegate* unmanaged<nint>;
    using unsafe ARCHIVE_ENTRY_FREE = delegate* unmanaged<nint, void>;
    using unsafe ARCHIVE_ENTRY_COPY_PATHNAME_W = delegate* unmanaged<nint, char*, void>;
    using unsafe ARCHIVE_ENTRY_SET_SIZE = delegate* unmanaged<nint, long, void>;
    using unsafe ARCHIVE_ENTRY_SET_FILETYPE = delegate* unmanaged<nint, uint, void>;
    using unsafe ARCHIVE_ENTRY_SET_PERM = delegate* unmanaged<nint, ushort, void>;
    using unsafe ARCHIVE_ENTRY_SET_MTIME = delegate* unmanaged<nint, long, int, void>;

    public static unsafe partial class Zip
    {
        private const int ARCHIVE_OK = 0;
        private const int ARCHIVE_WARN = -20;
        private const uint AE_IFREG = 0x8000;

        private sealed class LibArchive
        {
            public ARCHIVE_WRITE_NEW ARCHIVE_WRITE_NEW_I;
            public ARCHIVE_WRITE_SET_FORMAT_7ZIP ARCHIVE_WRITE_SET_FORMAT_7ZIP_I;
            public ARCHIVE_WRITE_SET_FORMAT_OPTION ARCHIVE_WRITE_SET_FORMAT_OPTION_I;
            public ARCHIVE_WRITE_OPEN_FILENAME_W ARCHIVE_WRITE_OPEN_FILENAME_W_I;
            public ARCHIVE_WRITE_HEADER ARCHIVE_WRITE_HEADER_I;
            public ARCHIVE_WRITE_DATA ARCHIVE_WRITE_DATA_I;
            public ARCHIVE_WRITE_FINISH_ENTRY ARCHIVE_WRITE_FINISH_ENTRY_I;
            public ARCHIVE_WRITE_CLOSE ARCHIVE_WRITE_CLOSE_I;
            public ARCHIVE_WRITE_FREE ARCHIVE_WRITE_FREE_I;
            public ARCHIVE_ERROR_STRING ARCHIVE_ERROR_STRING_I;
            public ARCHIVE_ENTRY_NEW ARCHIVE_ENTRY_NEW_I;
            public ARCHIVE_ENTRY_FREE ARCHIVE_ENTRY_FREE_I;
            public ARCHIVE_ENTRY_COPY_PATHNAME_W ARCHIVE_ENTRY_COPY_PATHNAME_W_I;
            public ARCHIVE_ENTRY_SET_SIZE ARCHIVE_ENTRY_SET_SIZE_I;
            public ARCHIVE_ENTRY_SET_FILETYPE ARCHIVE_ENTRY_SET_FILETYPE_I;
            public ARCHIVE_ENTRY_SET_PERM ARCHIVE_ENTRY_SET_PERM_I;
            public ARCHIVE_ENTRY_SET_MTIME ARCHIVE_ENTRY_SET_MTIME_I;
        }

        private static LibArchive FindLibArchive()
        {
            string path = Path.Join(Environment.SystemDirectory, "archiveint.dll");

            if (!NativeLibrary.TryLoad(path, out nint module))
                return null;

            try
            {
                nint Get(string Name) => NativeLibrary.GetExport(module, Name);

                return new LibArchive
                {
                    ARCHIVE_WRITE_NEW_I = (ARCHIVE_WRITE_NEW)Get("archive_write_new"),
                    ARCHIVE_WRITE_SET_FORMAT_7ZIP_I = (ARCHIVE_WRITE_SET_FORMAT_7ZIP)Get("archive_write_set_format_7zip"),
                    ARCHIVE_WRITE_SET_FORMAT_OPTION_I = (ARCHIVE_WRITE_SET_FORMAT_OPTION)Get("archive_write_set_format_option"),
                    ARCHIVE_WRITE_OPEN_FILENAME_W_I = (ARCHIVE_WRITE_OPEN_FILENAME_W)Get("archive_write_open_filename_w"),
                    ARCHIVE_WRITE_HEADER_I = (ARCHIVE_WRITE_HEADER)Get("archive_write_header"),
                    ARCHIVE_WRITE_DATA_I = (ARCHIVE_WRITE_DATA)Get("archive_write_data"),
                    ARCHIVE_WRITE_FINISH_ENTRY_I = (ARCHIVE_WRITE_FINISH_ENTRY)Get("archive_write_finish_entry"),
                    ARCHIVE_WRITE_CLOSE_I = (ARCHIVE_WRITE_CLOSE)Get("archive_write_close"),
                    ARCHIVE_WRITE_FREE_I = (ARCHIVE_WRITE_FREE)Get("archive_write_free"),
                    ARCHIVE_ERROR_STRING_I = (ARCHIVE_ERROR_STRING)Get("archive_error_string"),
                    ARCHIVE_ENTRY_NEW_I = (ARCHIVE_ENTRY_NEW)Get("archive_entry_new"),
                    ARCHIVE_ENTRY_FREE_I = (ARCHIVE_ENTRY_FREE)Get("archive_entry_free"),
                    ARCHIVE_ENTRY_COPY_PATHNAME_W_I = (ARCHIVE_ENTRY_COPY_PATHNAME_W)Get("archive_entry_copy_pathname_w"),
                    ARCHIVE_ENTRY_SET_SIZE_I = (ARCHIVE_ENTRY_SET_SIZE)Get("archive_entry_set_size"),
                    ARCHIVE_ENTRY_SET_FILETYPE_I = (ARCHIVE_ENTRY_SET_FILETYPE)Get("archive_entry_set_filetype"),
                    ARCHIVE_ENTRY_SET_PERM_I = (ARCHIVE_ENTRY_SET_PERM)Get("archive_entry_set_perm"),
                    ARCHIVE_ENTRY_SET_MTIME_I = (ARCHIVE_ENTRY_SET_MTIME)Get("archive_entry_set_mtime"),
                };
            }
            catch (EntryPointNotFoundException)
            {
                return null;
            }
        }

        private static void CheckLibArchive(LibArchive Library, nint Archive, int Status, string Operation)
        {
            if (Status == ARCHIVE_OK || Status == ARCHIVE_WARN)
                return;

            string message = Marshal.PtrToStringUTF8((nint)Library.ARCHIVE_ERROR_STRING_I(Archive));
            throw new IOException($"libarchive {Operation} failed ({Status}): {message}");
        }

        private static void SetLibArchiveFormatOption(LibArchive Library, nint Archive, string Option, string Value)
        {
            byte[] module = Encoding.UTF8.GetBytes("7zip\0");
            byte[] option = Encoding.UTF8.GetBytes(Option + "\0");
            byte[] value = Encoding.UTF8.GetBytes(Value + "\0");

            fixed (byte* m = module, o = option, v = value)
            {
                CheckLibArchive(Library, Archive, Library.ARCHIVE_WRITE_SET_FORMAT_OPTION_I(Archive, m, o, v), Option);
            }
        }

        private static void CreateLibArchiveSevenZipArchive(
            LibArchive Library,
            List<(string FileName, string EntryName)> Entries,
            string DestinationArchiveFileName,
            BuildFlags Flags
            )
        {
            // libarchive can write LZMA1/LZMA2 7z archives but not BCJ/BCJ2 filters.
            string destination = Path.GetFullPath(DestinationArchiveFileName);
            string temporaryArchive = Path.Join(Path.GetDirectoryName(destination), $".{Guid.NewGuid():N}.7z");
            nint archive = Library.ARCHIVE_WRITE_NEW_I();
            byte[] buffer = ArrayPool<byte>.Shared.Rent(1024 * 1024);

            if (archive == 0)
                throw new OutOfMemoryException("archive_write_new failed.");

            try
            {
                CheckLibArchive(Library, archive, Library.ARCHIVE_WRITE_SET_FORMAT_7ZIP_I(archive), "set_format_7zip");
                SetLibArchiveFormatOption(Library, archive, "compression", "lzma1");
                SetLibArchiveFormatOption(Library, archive, "compression-level", "9");

                fixed (char* path = temporaryArchive)
                {
                    CheckLibArchive(Library, archive, Library.ARCHIVE_WRITE_OPEN_FILENAME_W_I(archive, path), "open");
                }

                foreach (var (fileName, entryName) in Entries)
                {
                    var info = new FileInfo(fileName);
                    var mtime = new DateTimeOffset(info.LastWriteTimeUtc);
                    nint entry = Library.ARCHIVE_ENTRY_NEW_I();

                    try
                    {
                        fixed (char* name = entryName.Replace('\\', '/'))
                        {
                            Library.ARCHIVE_ENTRY_COPY_PATHNAME_W_I(entry, name);
                        }

                        Library.ARCHIVE_ENTRY_SET_FILETYPE_I(entry, AE_IFREG);
                        Library.ARCHIVE_ENTRY_SET_PERM_I(entry, 0x1A4); // 0644
                        Library.ARCHIVE_ENTRY_SET_SIZE_I(entry, info.Length);
                        Library.ARCHIVE_ENTRY_SET_MTIME_I(entry, mtime.ToUnixTimeSeconds(), (int)(mtime.UtcTicks % TimeSpan.TicksPerSecond * 100));
                        CheckLibArchive(Library, archive, Library.ARCHIVE_WRITE_HEADER_I(archive, entry), "write_header");
                    }
                    finally
                    {
                        Library.ARCHIVE_ENTRY_FREE_I(entry);
                    }

                    using (var inputStream = File.OpenRead(fileName))
                    {
                        int length;

                        while ((length = inputStream.Read(buffer, 0, buffer.Length)) > 0)
                        {
                            fixed (byte* data = buffer)
                            {
                                if (Library.ARCHIVE_WRITE_DATA_I(archive, data, (nuint)length) != length)
                                    CheckLibArchive(Library, archive, -25, "write_data");
                            }
                        }
                    }

                    CheckLibArchive(Library, archive, Library.ARCHIVE_WRITE_FINISH_ENTRY_I(archive), "finish_entry");

                    if (Flags.HasFlag(BuildFlags.BuildVerbose))
                        PrintCompressionVerboseLine(entryName, Flags);
                }

                // 7z data is only written on close; check it before publishing.
                CheckLibArchive(Library, archive, Library.ARCHIVE_WRITE_CLOSE_I(archive), "close");
                Library.ARCHIVE_WRITE_FREE_I(archive);
                archive = 0;

                File.Move(temporaryArchive, destination, true);
            }
            finally
            {
                if (archive != 0)
                    Library.ARCHIVE_WRITE_FREE_I(archive);

                ArrayPool<byte>.Shared.Return(buffer);
                File.Delete(temporaryArchive);
            }
        }
    }
}
