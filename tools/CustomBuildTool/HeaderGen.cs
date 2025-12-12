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

namespace CustomBuildTool
{
    public static class HeaderGen
    {
        private static readonly string Notice = "/*\r\n * Copyright (c) Winsider Seminars & Solutions, Inc.  All rights reserved.\r\n *\r\n * This file is part of System Informer.\r\n *\r\n */\r\n\r\n";
        private static readonly string Header = "#ifndef _PH_PHAPPPUB_H\r\n#define _PH_PHAPPPUB_H\r\n\r\n// This file was automatically generated. Do not edit.\r\n\r\n#ifdef __cplusplus\r\nextern \"C\" {\r\n#endif\r\n";
        private const string Footer = "\r\n#ifdef __cplusplus\r\n}\r\n#endif\r\n\r\n#endif\r\n";

        private const string BaseDirectory = "SystemInformer\\include";
        private const string OutputFile = "..\\sdk\\phapppub.h";

        private static readonly string[] Modes = ["phapppub"];
        private static readonly string[] Files =
        [
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
            "devprv.h",
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
            "miniinfo.h",
            "hndlmenu.h"
        ];

        private static List<HeaderFile> OrderHeaderFiles(List<HeaderFile> headerFiles)
        {
            var result = new List<HeaderFile>();
            var done = new HashSet<HeaderFile>();

            foreach (HeaderFile h in headerFiles)
                OrderHeaderFiles(result, done, h);

            return result;
        }

        private static void OrderHeaderFiles(List<HeaderFile> result, HashSet<HeaderFile> done, HeaderFile headerFile)
        {
            if (!done.Add(headerFile))
                return;

            foreach (HeaderFile h in headerFile.Dependencies)
                OrderHeaderFiles(result, done, h);

            result.Add(headerFile);
        }

        private static List<string> ProcessHeaderLines(List<string> lines)
        {
            var result = new List<string>();
            var modes = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            bool blankLine = false;

            foreach (string line in lines)
            {
                string text = line.Trim();

                if (text.StartsWith("// begin_", StringComparison.OrdinalIgnoreCase))
                {
                    modes.Add(text.Substring("// begin_".Length));
                }
                else if (text.StartsWith("// end_", StringComparison.OrdinalIgnoreCase))
                {
                    modes.Remove(text.Substring("// end_".Length));
                }
                else
                {
                    bool blockMode = Modes.Any(mode => modes.Contains(mode));
                    bool lineMode = Modes.Any(mode =>
                    {
                        int indexOfMarker = text.LastIndexOf($"// {mode}", StringComparison.OrdinalIgnoreCase);
                        if (indexOfMarker == -1)
                            return false;

                        return text.Substring(indexOfMarker).Trim().All(c => char.IsLetterOrDigit(c) || c == ' ' || c == '/');
                    });

                    if (blockMode || lineMode)
                    {
                        if (blankLine && result.Count > 0)
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
                string file = Path.Join([BaseDirectory, name]);
                List<string> lines = File.ReadAllLines(file).ToList();

                headerFiles.Add(name, new HeaderFile(name, lines));
            }

            foreach (HeaderFile h in headerFiles.Values)
            {
                var partitions = new List<KeyValuePair<string, HeaderFile>>();
                var dependencies = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

                foreach (string line in h.Lines)
                {
                    var trimmed = line.Trim();

                    if (trimmed.StartsWith("#include <", StringComparison.OrdinalIgnoreCase) && trimmed.EndsWith(">", StringComparison.OrdinalIgnoreCase))
                    {
                        var dependencyName = trimmed.Substring("#include <".Length, trimmed.Length - "#include <".Length - 1);

                        if (headerFiles.TryGetValue(dependencyName, out HeaderFile dependency))
                        {
                            partitions.Add(new KeyValuePair<string, HeaderFile>(line, dependency));
                            dependencies.Add(dependencyName);
                        }
                        else
                        {
                            partitions.Add(new KeyValuePair<string, HeaderFile>(line, null));
                        }
                    }
                    else
                    {
                        partitions.Add(new KeyValuePair<string, HeaderFile>(line, null));
                    }
                }

                h.Lines = partitions.Where(p => p.Value == null).Select(p => p.Key).ToList();
                h.Dependencies = dependencies.Select(dependencyName => headerFiles[dependencyName]).ToList();
            }

            // Generate the ordering.

            List<HeaderFile> orderedHeaderFilesList = new List<HeaderFile>();

            foreach (string file in Files)
            {
                string name = Path.GetFileName(file);

                if (headerFiles.TryGetValue(name, out HeaderFile value))
                {
                    orderedHeaderFilesList.Add(value);
                }
            }

            List<HeaderFile> orderedHeaderFiles = OrderHeaderFiles(orderedHeaderFilesList);

            // Process each header file and remove irrelevant content.
            foreach (HeaderFile h in orderedHeaderFiles)
            {
                h.Lines = ProcessHeaderLines(h.Lines);
            }

            // Write out the result.

            StringBuilder sw = new StringBuilder();
            {
                // Copyright
                sw.Append(Notice);

                // Header
                sw.Append(Header);

                // Header files
                foreach (HeaderFile h in orderedHeaderFiles)
                {
                    //Console.WriteLine("Header file: " + h.Name);
                    sw.AppendLine();
                    sw.AppendLine("//");
                    sw.AppendLine($"// {Path.GetFileNameWithoutExtension(h.Name)}");
                    sw.AppendLine("//");
                    sw.AppendLine();

                    foreach (string line in h.Lines)
                    {
                        sw.AppendLine(line);
                    }
                }

                // Footer
                sw.Append(Footer);
            }

            // Check for new or modified content. We don't want to touch the file if it's not needed.
            {
                string headerFileName = Path.Join([BaseDirectory, OutputFile]);
                string headerUpdateText = sw.ToString();

                if (File.Exists(headerFileName))
                {
                    string headerCurrentText = Utils.ReadAllText(headerFileName);

                    if (!string.Equals(headerUpdateText, headerCurrentText, StringComparison.OrdinalIgnoreCase))
                    {
                        Program.PrintColorMessage($"HeaderGen -> {headerFileName}", ConsoleColor.Cyan);
                        Utils.WriteAllText(headerFileName, headerUpdateText);
                    }
                }
                else
                {
                    Program.PrintColorMessage($"HeaderGen -> {headerFileName}", ConsoleColor.Cyan);
                    Utils.WriteAllText(headerFileName, headerUpdateText);
                }
            }
        }
    }

    public class HeaderFile : IEquatable<HeaderFile>
    {
        public readonly string Name;
        public List<string> Lines;
        public List<HeaderFile> Dependencies;

        public HeaderFile(string Name, List<string> Lines)
        {
            this.Name = Name;
            this.Lines = Lines;
        }

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
            if (obj is not HeaderFile file)
                return false;

            return string.Equals(this.Name, file.Name, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(HeaderFile other)
        {
            return other != null && string.Equals(this.Name, other.Name, StringComparison.OrdinalIgnoreCase);
        }
    }

    public static class SingleHeaderGen
    {
        private static readonly string[] Headers =
        {
            "phnt_ntdef.h",
            "ntnls.h",
            "ntkeapi.h",
            "ntldr.h",
            "ntexapi.h",
            "ntmmapi.h",
            "ntobapi.h",
            "ntpsapi.h",
            "ntbcd.h",
            "ntdbg.h",
            "ntintsafe.h",
            "ntimage.h",
            "ntioapi.h",
            "ntlsa.h",
            "ntlpcapi.h",
            "ntmisc.h",
            "ntpfapi.h",
            "ntpnpapi.h",
            "ntpoapi.h",
            "ntregapi.h",
            "ntrtl.h",
            "ntsam.h",
            "ntseapi.h",
            "nttmapi.h",
            "nttp.h",
            "ntuser.h",
            "ntwmi.h",
            "ntwow64.h",
            "ntxcapi.h",
        };

        public static bool Execute()
        {
            try
            {
                if (File.Exists("phnt\\include\\nt.h"))
                    File.Delete("phnt\\include\\nt.h");

                var config = File.ReadAllLines("phnt\\include\\phnt.h");

                using (var output = new StreamWriter("phnt\\include\\nt.h"))
                {
                    output.WriteLine("/*\r\n * This file was automatically generated. Do not edit.\r\n */");

                    {
                        int startIndex = Array.FindIndex(config, l => l.StartsWith("EXTERN_C_START", StringComparison.OrdinalIgnoreCase));
                        int endIndex = Array.FindLastIndex(config, l => l.StartsWith("#endif", StringComparison.OrdinalIgnoreCase));

                        for (long i = 0; i < config.LongLength; i++)
                        {
                            // Skip everything between startIndex and endIndex (inclusive)
                            if (startIndex != -1 && endIndex != -1 && i >= startIndex && i < endIndex)
                                continue;

                            output.WriteLine(config[i]);
                        }
                    }

                    foreach (string file in Headers)
                    {
                        if (
                            file.Equals("ntzwapi.h", StringComparison.OrdinalIgnoreCase) ||
                            file.Equals("nt.h", StringComparison.OrdinalIgnoreCase)
                            )
                        {
                            continue;
                        }

                        string[] content;

                        if (File.Exists($"phnt\\include\\{file}"))
                        {
                            content = File.ReadAllLines($"phnt\\include\\{file}");
                        }
                        else
                        {
                            var sdk_include_path = Utils.GetWindowsSdkIncludePath();
                            if (string.IsNullOrWhiteSpace(sdk_include_path))
                            {
                                Console.WriteLine($"[ERROR] Could not find Windows SDK include paths.");
                                continue;
                            }

                            var found = Directory.EnumerateFiles(sdk_include_path, file, SearchOption.AllDirectories).FirstOrDefault();
                            if (string.IsNullOrWhiteSpace(found))
                            {
                                Console.WriteLine($"[ERROR] Could not find {file} in phnt or Windows SDK include paths.");
                                continue;
                            }

                            content = File.ReadAllLines(found);
                        }

                        if (content.LongLength == 0)
                            continue;

                        long startIndex = 0;

                        // Skip the /* ... */ block
                        {
                            if (content.LongLength > 0 && content[0].Contains("/*", StringComparison.OrdinalIgnoreCase))
                            {
                                bool foundBlockEnd = false;

                                for (long i = 0; i < content.LongLength; i++)
                                {
                                    if (content[i].Contains("*/", StringComparison.OrdinalIgnoreCase))
                                    {
                                        startIndex = i + 1; // skip through the end of the block
                                        foundBlockEnd = true;
                                        break;
                                    }
                                }

                                if (!foundBlockEnd)
                                {
                                    startIndex = 0; // unexpected comment block, dont skip anything
                                }
                            }
                        }

                        for (long i = startIndex; i < content.LongLength; i++)
                        {
                            string line = content[i];

                            if (
                                line.Equals("#include <ntpebteb.h>", StringComparison.OrdinalIgnoreCase) ||
                                line.Equals("#include <ntsxs.h>", StringComparison.OrdinalIgnoreCase)
                                )
                            {
                                continue;
                            }

                            output.WriteLine(line);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Unable to merge headers: {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }
    }
}
