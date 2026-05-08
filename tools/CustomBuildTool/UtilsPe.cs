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
    /// Provides utility methods for working with Portable Executable (PE) images, including resolving relative virtual
    /// addresses, reading data structures, and validating export tables.
    /// </summary>
    internal static class PEImage
    {
        /// <summary>
        /// Converts a relative virtual address (RVA) to its corresponding file offset within a PE file, based on the
        /// provided PE headers.
        /// </summary>
        /// <param name="Rva">The relative virtual address to convert. Must correspond to a valid address within one of the sections
        /// described by the headers.</param>
        /// <param name="ImageHeaders">The PE headers containing section information used to map the RVA to a file offset.</param>
        /// <returns>The file offset that corresponds to the specified RVA.</returns>
        /// <exception cref="InvalidOperationException">Thrown if the specified RVA does not fall within any section defined in the PE headers.</exception>
        internal static int RvaToFileOffset(int Rva, PEHeaders ImageHeaders)
        {
            ResolveRva(Rva, ImageHeaders, out int fileOffset, out _);
            return fileOffset;
        }

        /// <summary>
        /// Calculates the file offset and maximum readable bytes for a given relative virtual address (RVA) within the
        /// specified PE headers.
        /// </summary>
        /// <param name="Rva">The relative virtual address to resolve.</param>
        /// <param name="ImageHeaders">The PE headers containing section information.</param>
        /// <param name="FileOffset">The calculated file offset corresponding to the RVA.</param>
        /// <param name="MaxReadableBytes">The maximum number of readable bytes from the RVA.</param>
        /// <exception cref="InvalidOperationException">Thrown when the specified RVA is not found in any section.</exception>
        internal static void ResolveRva(int Rva, PEHeaders ImageHeaders, out int FileOffset, out int MaxReadableBytes)
        {
            foreach (var section in ImageHeaders.SectionHeaders)
            {
                int start = section.VirtualAddress;
                int end = start + Math.Max(section.VirtualSize, section.SizeOfRawData);

                if (Rva >= start && Rva < end)
                {
                    FileOffset = section.PointerToRawData + (Rva - start);
                    MaxReadableBytes = end - Rva;
                    return;
                }
            }

            throw new InvalidOperationException($"RVA 0x{Rva:X} not found in any section");
        }

        /// <summary>
        /// Reads the specified number of bytes from a stream starting at the given offset.
        /// </summary>
        /// <param name="imageStream">The stream to read from.</param>
        /// <param name="fileOffset">The zero-based byte offset in the stream at which to begin reading.</param>
        /// <param name="buffer">The buffer to fill with the read bytes.</param>
        /// <exception cref="EndOfStreamException">Thrown if the end of the stream is reached before the buffer is completely filled.</exception>
        internal static void ReadExactlyAt(Stream imageStream, int fileOffset, Span<byte> buffer)
        {
            imageStream.Seek(fileOffset, SeekOrigin.Begin);

            int totalRead = 0;

            while (totalRead < buffer.Length)
            {
                int read = imageStream.Read(buffer[totalRead..]);

                if (read == 0)
                {
                    throw new EndOfStreamException($"Unexpected end of file while reading at offset 0x{fileOffset + totalRead:X}.");
                }

                totalRead += read;
            }
        }

        /// <summary>
        /// Reads a structure of type T from the specified relative virtual address (RVA) in the provided image stream.
        /// </summary>
        /// <typeparam name="T">The type of the structure to read, which must be an unmanaged type.</typeparam>
        /// <param name="imageStream">The stream containing the image data from which the structure is read.</param>
        /// <param name="imageHeaders">The PE headers of the image, used to resolve the RVA to a file offset.</param>
        /// <param name="rva">The relative virtual address from which to read the structure.</param>
        /// <returns>An instance of the structure of type T read from the specified RVA.</returns>
        /// <exception cref="InvalidOperationException">Thrown when the RVA does not contain enough data to read the structure of type T.</exception>
        internal static unsafe T ReadStructureAtRva<T>(Stream imageStream, PEHeaders imageHeaders, int rva) where T : unmanaged
        {
            ResolveRva(rva, imageHeaders, out int fileOffset, out int maxReadableBytes);

            if (maxReadableBytes < sizeof(T))
                throw new InvalidOperationException($"RVA 0x{rva:X8} does not contain enough data for {typeof(T).Name}.");

            Span<byte> buffer = stackalloc byte[sizeof(T)];
            ReadExactlyAt(imageStream, fileOffset, buffer);

            return MemoryMarshal.Read<T>(buffer);
        }

        /// <summary>
        /// Reads a 32-bit unsigned integer from the specified relative virtual address (RVA) in the image stream.
        /// </summary>
        /// <param name="imageStream">The stream containing the image data.</param>
        /// <param name="imageHeaders">The PE headers of the image, used to resolve the RVA.</param>
        /// <param name="rva">The relative virtual address from which to read the 32-bit unsigned integer.</param>
        /// <returns>The 32-bit unsigned integer read from the specified RVA.</returns>
        /// <exception cref="InvalidOperationException">Thrown when the RVA does not contain enough data to read a UInt32.</exception>
        internal static uint ReadUInt32AtRva(Stream imageStream, PEHeaders imageHeaders, int rva)
        {
            ResolveRva(rva, imageHeaders, out int fileOffset, out int maxReadableBytes);

            if (maxReadableBytes < sizeof(uint))
                throw new InvalidOperationException($"RVA 0x{rva:X8} does not contain enough data for UInt32.");

            Span<byte> buffer = stackalloc byte[sizeof(uint)];
            ReadExactlyAt(imageStream, fileOffset, buffer);

            return BinaryPrimitives.ReadUInt32LittleEndian(buffer);
        }

        /// <summary>
        /// Reads a null-terminated UTF-8 string from a specified RVA in a PE image.
        /// </summary>
        /// <param name="imageStream">The stream containing the PE image data.</param>
        /// <param name="imageHeaders">The headers of the PE image used to resolve the RVA.</param>
        /// <param name="rva">The relative virtual address from which to read the null-terminated string.</param>
        /// <returns>The null-terminated UTF-8 string read from the specified RVA.</returns>
        /// <exception cref="InvalidOperationException">Thrown when the RVA does not contain any readable bytes or the string is not null-terminated.</exception>
        internal static string ReadNullTerminatedUtf8AtRva(Stream imageStream, PEHeaders imageHeaders, int rva)
        {
            ResolveRva(rva, imageHeaders, out int fileOffset, out int maxReadableBytes);

            if (maxReadableBytes <= 0)
                throw new InvalidOperationException($"RVA 0x{rva:X8} does not contain any readable bytes.");

            const int ChunkSize = 256;
            var chunk = new byte[Math.Min(ChunkSize, maxReadableBytes)];
            var utf8Bytes = new ArrayBufferWriter<byte>(Math.Min(maxReadableBytes, ChunkSize));

            int remaining = maxReadableBytes;
            int currentOffset = fileOffset;

            while (remaining > 0)
            {
                var toRead = Math.Min(chunk.Length, remaining);
                var span = chunk.AsSpan(0, toRead);
                ReadExactlyAt(imageStream, currentOffset, span);

                int terminator = span.IndexOf((byte)0);

                if (terminator >= 0)
                {
                    utf8Bytes.Write(span[..terminator]);
                    return Encoding.UTF8.GetString(utf8Bytes.WrittenSpan);
                }

                utf8Bytes.Write(span);
                currentOffset += toRead;
                remaining -= toRead;
            }

            throw new InvalidOperationException($"Export name at RVA 0x{rva:X8} is not null-terminated within its section.");
        }

        /// <summary>
        /// Validates the export table of a PE file specified by the given file name.
        /// </summary>
        /// <param name="FileName">The path to the PE file to validate.</param>
        /// <returns>True if the export table is valid; otherwise, false.</returns>
        /// <exception cref="InvalidOperationException">Thrown when the export directory reports names but the AddressOfNames is NULL, or when the export name
        /// pointer table is truncated.</exception>
        internal static bool ValidateImageExports(string FileName)
        {
            try
            {
                using var stream = File.OpenRead(FileName);
                using var image = new PEReader(stream);

                PEHeaders headers = image.PEHeaders;
                if (headers.PEHeader == null)
                {
                    Program.PrintColorMessage("Invalid PE header", ConsoleColor.Red);
                    return false;
                }

                DirectoryEntry exportTableDirectory = headers.PEHeader.ExportTableDirectory;
                if (exportTableDirectory.RelativeVirtualAddress == 0 || exportTableDirectory.Size == 0)
                {
                    Program.PrintColorMessage("IMAGE_DIRECTORY_ENTRY_EXPORT", ConsoleColor.Red);
                    return true;
                }

                IMAGE_EXPORT_DIRECTORY exportDirectory = ReadStructureAtRva<IMAGE_EXPORT_DIRECTORY>(stream, headers, exportTableDirectory.RelativeVirtualAddress);
                if (exportDirectory.NumberOfNames == 0)
                    return true;

                if (exportDirectory.AddressOfNames == 0)
                {
                    throw new InvalidOperationException("Export directory reports names, but AddressOfNames is NULL.");
                }

                var namesTableRva = checked((int)exportDirectory.AddressOfNames);
                var requiredBytes = checked((int)exportDirectory.NumberOfNames * sizeof(uint));

                ResolveRva(namesTableRva, headers, out _, out int namesTableAvailableBytes);

                if (namesTableAvailableBytes < requiredBytes)
                {
                    throw new InvalidOperationException($"Export name pointer table is truncated. Need {requiredBytes} bytes, have {namesTableAvailableBytes} bytes.");
                }

                for (uint i = 0; i < exportDirectory.NumberOfNames; i++)
                {
                    var entryRva = checked(namesTableRva + checked((int)(i * sizeof(uint))));
                    var nameRva = checked((int)ReadUInt32AtRva(stream, headers, entryRva));

                    if (nameRva == 0)
                    {
                        throw new InvalidOperationException($"Export name RVA[{i}] is NULL.");
                    }

                    string exportName = ReadNullTerminatedUtf8AtRva(stream, headers, nameRva);

                    Program.PrintColorMessage($"{i}: {exportName}", ConsoleColor.Yellow);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"ValidateImageExports_PeFile: {ex}", ConsoleColor.Red);
            }

            return false;
        }

        /// <summary>
        /// Validates the export directory of a PE image file to determine if exported functions are defined.
        /// </summary>
        /// <remarks>This method checks the export directory of the specified PE image file and reports
        /// missing exported functions. If exported functions are missing, a message is printed to the console. The
        /// method unmaps the image file after validation.</remarks>
        /// <param name="FileName">The path to the PE image file to validate. Cannot be null or empty.</param>
        /// <returns>true if the image file contains an export directory with no exported function names; otherwise, false.</returns>
        //public static bool ValidateImageExports(string FileName)
        //{
        //    using var stream = File.OpenRead(FileName);
        //    using var pe = new PEReader(stream);
        //
        //    var headers = pe.PEHeaders;
        //    var exportDir = headers.PEHeader!.ExportTableDirectory;
        //
        //
        //    LOADED_IMAGE loadedMappedImage = default;
        //
        //    try
        //    {
        //        if (!PInvoke.MapAndLoad(FileName, null, out loadedMappedImage, false, true))
        //            return false;
        //
        //        try
        //        {
        //            var exportDirectory = (IMAGE_EXPORT_DIRECTORY*)PInvoke.ImageDirectoryEntryToData(
        //                loadedMappedImage.MappedAddress, false,
        //                IMAGE_DIRECTORY_ENTRY.IMAGE_DIRECTORY_ENTRY_EXPORT, out uint DirectorySize
        //                );
        //
        //            if (exportDirectory != null)
        //            {
        //                if (exportDirectory->NumberOfNames == 0)
        //                    return true;
        //
        //                Program.PrintColorMessage("Exported functions missing from module export definition file: ", ConsoleColor.Yellow);
        //
        //                uint* exportNameTable = (uint*)PInvoke.ImageRvaToVa(loadedMappedImage.FileHeader, loadedMappedImage.MappedAddress, exportDirectory->AddressOfNames, null);
        //
        //                for (uint i = 0; i < exportDirectory->NumberOfNames; i++)
        //                {
        //                    uint nameRva = exportNameTable[i];
        //                    IntPtr namePtr = (IntPtr)PInvoke.ImageRvaToVa(loadedMappedImage.FileHeader, loadedMappedImage.MappedAddress, nameRva, null);
        //                    var exportName = Marshal.PtrToStringUTF8(namePtr);
        //
        //                    Program.PrintColorMessage($"{i}: {exportName}", ConsoleColor.Yellow);
        //                }
        //            }
        //            else
        //            {
        //                Program.PrintColorMessage("IMAGE_DIRECTORY_ENTRY_EXPORT", ConsoleColor.Red);
        //            }
        //        }
        //        catch (Exception ex)
        //        {
        //            Program.PrintColorMessage($"ValidateImageExports: {ex}", ConsoleColor.Red);
        //        }
        //    }
        //    finally
        //    {
        //        PInvoke.UnMapAndLoad(ref loadedMappedImage);
        //    }
        //
        //    return false;
        //}


    }
}
