/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32
 *     dmex
 *
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

            if (obj is not HeaderFile file)
                return false;

            return this.Name.Equals(file.Name, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(HeaderFile other)
        {
            return this.Name.Equals(other.Name, StringComparison.OrdinalIgnoreCase);
        }
    }

    public static class HeaderGen
    {
        private static readonly string Header = "#ifndef _PH_PHAPPPUB_H\r\n#define _PH_PHAPPPUB_H\r\n\r\n// This file was automatically generated. Do not edit.\r\n\r\n#ifdef __cplusplus\r\nextern \"C\" {\r\n#endif\r\n";
        private static readonly string Footer = "\r\n#ifdef __cplusplus\r\n}\r\n#endif\r\n\r\n#endif\r\n";

        private static readonly string BaseDirectory = "SystemInformer\\include";
        private static readonly string OutputFile = "..\\sdk\\phapppub.h";

        private static readonly string[] Modes = new[] { "phapppub" };
        private static readonly string[] Files = new[]
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

        private static List<HeaderFile> OrderHeaderFiles(List<HeaderFile> headerFiles)
        {
            List<HeaderFile> result = new List<HeaderFile>();
            List<HeaderFile> done = new List<HeaderFile>();

            foreach (HeaderFile h in headerFiles)
                OrderHeaderFiles(result, done, h);

            return result;
        }

        private static void OrderHeaderFiles(List<HeaderFile> result, List<HeaderFile> done, HeaderFile headerFile)
        {
            if (done.Contains(headerFile))
                return;

            done.Add(headerFile);

            foreach (HeaderFile h in headerFile.Dependencies)
                OrderHeaderFiles(result, done, h);

            result.Add(headerFile);
        }

        private static List<string> ProcessHeaderLines(List<string> lines)
        {
            List<string> result = new List<string>();
            List<string> modes = new List<string>();
            bool blankLine = false;

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
                    bool blockMode = Modes.Any(mode => modes.Contains(mode, StringComparer.OrdinalIgnoreCase));
                    bool lineMode = Modes.Any(mode =>
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

        public static void Execute()
        {
            // Read in all header files.
            Dictionary<string, HeaderFile> headerFiles = new Dictionary<string, HeaderFile>(StringComparer.OrdinalIgnoreCase);

            foreach (string name in Files)
            {
                string file = BaseDirectory + Path.DirectorySeparatorChar + name;
                List<string> lines = File.ReadAllLines(file).ToList();

                headerFiles.Add(name, new HeaderFile
                {
                    Name = name,
                    Lines = lines
                });
            }

            foreach (HeaderFile h in headerFiles.Values)
            {
                ILookup<bool, Tuple<string, HeaderFile>> partitions = h.Lines.Select(s =>
                {
                    string trimmed = s.Trim();

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

            List<HeaderFile> orderedHeaderFilesList = new List<HeaderFile>();

            foreach (string file in Files)
            {
                string name = Path.GetFileName(file);

                if (headerFiles.ContainsKey(name))
                {
                    orderedHeaderFilesList.Add(headerFiles[name]);
                }
            }

            List<HeaderFile> orderedHeaderFiles = OrderHeaderFiles(orderedHeaderFilesList);

            // Process each header file and remove irrelevant content.
            foreach (HeaderFile h in orderedHeaderFiles)
                h.Lines = ProcessHeaderLines(h.Lines);

            // Write out the result.
            using (StreamWriter sw = new StreamWriter(BaseDirectory + Path.DirectorySeparatorChar + OutputFile))
            {
                // Header
                sw.Write(Header);

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
                sw.Write(Footer);
            }
        }
    }
}
