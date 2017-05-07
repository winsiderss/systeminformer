using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;

namespace CustomBuildTool
{
    public class HeaderFile
    {
        public string Name;
        public List<string> Lines;
        public List<HeaderFile> Dependencies;
    }

    public class HeaderGen
    {
        private string BaseDirectory;
        private string[] Modes;
        private string[] Files;
        private string OutputFile;
        private string Header;
        private string Footer;

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
            var done = new HashSet<HeaderFile>();

            foreach (var h in headerFiles)
                OrderHeaderFiles(result, done, h);

            return result;
        }

        private void OrderHeaderFiles(List<HeaderFile> result, HashSet<HeaderFile> done, HeaderFile headerFile)
        {
            if (done.Contains(headerFile))
                return;

            done.Add(headerFile);

            foreach (var h in headerFile.Dependencies)
                OrderHeaderFiles(result, done, h);

            result.Add(headerFile);
        }

        private List<string> ProcessHeaderLines(IEnumerable<string> lines)
        {
            var result = new List<string>();
            var modes = new HashSet<string>();
            var blankLine = false;

            foreach (var line in lines)
            {
                var s = line.Trim();

                if (s.StartsWith("// begin_"))
                {
                    modes.Add(s.Remove(0, "// begin_".Length));
                }
                else if (s.StartsWith("// end_"))
                {
                    modes.Remove(s.Remove(0, "// end_".Length));
                }
                else
                {
                    bool blockMode = this.Modes.Any(modes.Contains);
                    bool lineMode = this.Modes.Any(mode =>
                    {
                        int indexOfMarker = s.LastIndexOf("// " + mode);
                        if (indexOfMarker == -1)
                            return false;

                        return s.Substring(indexOfMarker).Trim().All(c => char.IsLetterOrDigit(c) || c == ' ' || c == '/');
                    });

                    if (blockMode || lineMode)
                    {
                        if (blankLine && result.Count != 0)
                            result.Add(string.Empty);

                        result.Add(line);
                        blankLine = false;
                    }
                    else if (s.Length == 0)
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

            var headerFiles = this.Files.Select(fileName =>
            {
                var fullFileName = this.BaseDirectory + "\\" + fileName;
                var lines = File.ReadAllLines(fullFileName).ToList();

                return new HeaderFile { Name = Path.GetFileName(fullFileName).ToLowerInvariant(), Lines = lines };
            })
            .ToDictionary(h => h.Name);

            foreach (HeaderFile h in headerFiles.Values)
            {
                var partitions = h.Lines.Select(s =>
                {
                    var trimmed = s.Trim().ToLowerInvariant();

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

            var orderedHeaderFiles = OrderHeaderFiles(this.Files.Select(s => headerFiles[Path.GetFileName(s).ToLower()]).ToList());

            // Process each header file and remove irrelevant content.
            foreach (HeaderFile h in orderedHeaderFiles)
                h.Lines = ProcessHeaderLines(h.Lines);

            // Write out the result.
            StreamWriter sw = new StreamWriter(this.BaseDirectory + "\\" + this.OutputFile);

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

            sw.Close();
        }
    }
}
