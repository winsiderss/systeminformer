/*
 * Process Hacker Toolchain - 
 *   Build script
 *   
 * Copyright (C) wj32
 * Copyright (C) dmex
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;

namespace CustomBuildTool
{
    public class HeaderFile : IEquatable<HeaderFile>
    {
        public string Name;
        public List<string> Lines;
        public List<HeaderFile> Dependencies;

        public override string ToString()
        {
            return this.Name;
        }

        public override int GetHashCode()
        {
            return this.Name.GetHashCode(StringComparison.OrdinalIgnoreCase);
        }

        public override bool Equals(object obj)
        {
            if (obj == null)
                return false;

            if (!(obj is HeaderFile file))
                return false;

            return this.Name.Equals(file.Name, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(HeaderFile other)
        {
            return this.Name.Equals(other.Name, StringComparison.OrdinalIgnoreCase);
        }
    }

    public class HeaderGen
    {
        private readonly string BaseDirectory;
        private readonly string[] Modes;
        private readonly string[] Files;
        private readonly string OutputFile;
        private readonly string Header;
        private readonly string Footer;

        public HeaderGen()
        {
            this.OutputFile = "..\\sdk\\phapppub.h";
            this.BaseDirectory = "ProcessHacker\\include";
            this.Modes = new[] { "phapppub" };
            this.Files = new[]
            {
                "phapp.h",
                "appsup.h",
                "phfwddef.h",
                "procprv.h",
                "srvprv.h",
                "netprv.h",
                "modprv.h",
                "thrdprv.h",
                "hndlprv.h",
                "memprv.h",
                "phuisup.h",
                "colmgr.h",
                "proctree.h",
                "srvlist.h",
                "netlist.h",
                "thrdlist.h",
                "modlist.h",
                "hndllist.h",
                "memlist.h",
                "extmgr.h",
                "mainwnd.h",
                "notifico.h",
                "phplug.h",
                "actions.h",
                "procprp.h",
                "procprpp.h",
                "phsvccl.h",
                "sysinfo.h",
                "procgrp.h",
                "miniinfo.h"
            };
            this.Header = "#ifndef _PH_PHAPPPUB_H\r\n#define _PH_PHAPPPUB_H\r\n\r\n// This file was automatically generated. Do not edit.\r\n\r\n#ifdef __cplusplus\r\nextern \"C\" {\r\n#endif\r\n";
            this.Footer = "\r\n#ifdef __cplusplus\r\n}\r\n#endif\r\n\r\n#endif\r\n";
        }

        private List<HeaderFile> OrderHeaderFiles(List<HeaderFile> headerFiles)
        {
            var result = new List<HeaderFile>();
            var done = new List<HeaderFile>();

            foreach (var h in headerFiles)
                OrderHeaderFiles(result, done, h);

            return result;
        }

        private void OrderHeaderFiles(List<HeaderFile> result, List<HeaderFile> done, HeaderFile headerFile)
        {
            if (done.Contains(headerFile))
                return;

            done.Add(headerFile);

            foreach (var h in headerFile.Dependencies)
                OrderHeaderFiles(result, done, h);

            result.Add(headerFile);
        }

        private List<string> ProcessHeaderLines(List<string> lines)
        {
            var result = new List<string>();
            var modes = new List<string>();
            var blankLine = false;

            foreach (string line in lines)
            {
                string text = line.Trim();

                if (text.StartsWith("// begin_", StringComparison.OrdinalIgnoreCase))
                {
                    modes.Add(text.Remove(0, "// begin_".Length));
                }
                else if (text.StartsWith("// end_", StringComparison.OrdinalIgnoreCase))
                {
                    modes.Remove(text.Remove(0, "// end_".Length));
                }
                else
                {
                    bool blockMode = this.Modes.Any(mode => modes.Contains(mode, StringComparer.OrdinalIgnoreCase));
                    bool lineMode = this.Modes.Any(mode =>
                    {
                        int indexOfMarker = text.LastIndexOf("// " + mode, StringComparison.OrdinalIgnoreCase);
                        if (indexOfMarker == -1)
                            return false;

                        return text.Substring(indexOfMarker).Trim().All(c => char.IsLetterOrDigit(c) || c == ' ' || c == '/');
                    });

                    if (blockMode || lineMode)
                    {
                        if (blankLine && result.Count != 0)
                            result.Add(string.Empty);

                        result.Add(line);
                        blankLine = false;
                    }
                    else if (text.Length == 0)
                    {
                        blankLine = true;
                    }
                }
            }

            return result;
        }

        public void Execute()
        {
            // Read in all header files.

            var headerFiles = new Dictionary<string, HeaderFile>(StringComparer.OrdinalIgnoreCase);

            foreach (string name in this.Files)
            {
                var file = this.BaseDirectory + Path.DirectorySeparatorChar + name;
                var lines = File.ReadAllLines(file).ToList();

                headerFiles.Add(name, new HeaderFile
                {
                    Name = name,
                    Lines = lines
                });
            }

            foreach (HeaderFile h in headerFiles.Values)
            {
                var partitions = h.Lines.Select(s =>
                {
                    var trimmed = s.Trim();

                    if (trimmed.StartsWith("#include <", StringComparison.OrdinalIgnoreCase) && trimmed.EndsWith(">", StringComparison.OrdinalIgnoreCase))
                    {
                        if (headerFiles.TryGetValue(trimmed.Remove(trimmed.Length - 1).Remove(0, "#include <".Length), out HeaderFile d))
                            return Tuple.Create(s, d);
                        else
                            return Tuple.Create<string, HeaderFile>(s, null);
                    }
                    return Tuple.Create<string, HeaderFile>(s, null);
                })
                .ToLookup(p => p.Item2 != null);

                h.Lines = partitions[false].Select(p => p.Item1).ToList();
                h.Dependencies = partitions[true].Select(p => p.Item2).Distinct().ToList();

                //foreach (var d in h.Dependencies)
                //Console.WriteLine("Dependency: " + h.Name + " -> " + d.Name);
            }

            // Generate the ordering.

            var orderedHeaderFilesList = new List<HeaderFile>();

            foreach (string file in this.Files)
            {
                string name = Path.GetFileName(file);

                if (headerFiles.ContainsKey(name))
                {
                    orderedHeaderFilesList.Add(headerFiles[name]);
                }
            }

            var orderedHeaderFiles = OrderHeaderFiles(orderedHeaderFilesList);

            // Process each header file and remove irrelevant content.
            foreach (HeaderFile h in orderedHeaderFiles)
                h.Lines = ProcessHeaderLines(h.Lines);

            // Write out the result.
            using (StreamWriter sw = new StreamWriter(this.BaseDirectory + Path.DirectorySeparatorChar + this.OutputFile))
            {
                // Header
                sw.Write(this.Header);

                // Header files
                foreach (HeaderFile h in orderedHeaderFiles)
                {
                    //Console.WriteLine("Header file: " + h.Name);
                    sw.WriteLine();
                    sw.WriteLine("//");
                    sw.WriteLine("// " + Path.GetFileNameWithoutExtension(h.Name));
                    sw.WriteLine("//");
                    sw.WriteLine();

                    foreach (string line in h.Lines)
                        sw.WriteLine(line);
                }

                // Footer

                sw.Write(this.Footer);
            }
        }
    }
}
