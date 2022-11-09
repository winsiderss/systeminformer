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
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text.RegularExpressions;

namespace GenerateZw
{
    public static class ZwGen
    {
        private static readonly string BaseDirectory = "phnt\\include";
        private static readonly string OutputFile = "ntzwapi.h";
        private static readonly string Header = "#ifndef _NTZWAPI_H\r\n#define _NTZWAPI_H\r\n\r\n// This file was automatically generated. Do not edit.\r\n\r\n";
        private static readonly string Footer = "#endif\r\n";
        public static readonly string[] Files =
        {
            "ntdbg.h",
            "ntexapi.h",
            "ntgdi.h",
            "ntioapi.h",
            "ntkeapi.h",
            "ntldr.h",
            "ntlpcapi.h",
            "ntmisc.h",
            "ntmmapi.h",
            "ntnls.h",
            "ntobapi.h",
            "ntpebteb.h",
            "ntpfapi.h",
            "ntpnpapi.h",
            "ntpoapi.h",
            "ntpsapi.h",
            "ntregapi.h",
            "ntrtl.h",
            "ntsam.h",
            "ntseapi.h",
            "nttmapi.h",
            "nttp.h",
            "ntwow64.h",
            "ntxcapi.h"
        };

        static ZwGen()
        {
            DirectoryInfo info = new DirectoryInfo(".");

            while (info.Parent != null && info.Parent.Parent != null)
            {
                info = info.Parent;

                if (Directory.Exists(info.FullName + Path.DirectorySeparatorChar + BaseDirectory))
                {
                    Directory.SetCurrentDirectory(info.FullName);
                    break;
                }
            }
        }

        public static void Execute()
        {
            // Build up a list of definitions.

            var definitions = new List<ServiceDefinition>();
            var regex = RegexDefinition.Regex();

            foreach (string fileName in Files)
            {
                var file = BaseDirectory + Path.DirectorySeparatorChar + fileName;
                var text = File.ReadAllText(file);

                MatchCollection matches = regex.Matches(text);

                foreach (Match match in matches)
                {
                    definitions.Add(new ServiceDefinition
                    {
                        Name = match.Groups[1].Value,
                        Text = match.Value,
                        NameIndex = match.Groups[1].Index - match.Index
                    });
                }
            }

            using (StreamWriter sw = new StreamWriter(BaseDirectory + Path.DirectorySeparatorChar + OutputFile))
            {
                // Remove duplicates and sort.

                List<ServiceDefinition> _defs = definitions.Distinct().ToList();
                _defs.Sort((x, y) => string.Compare(x.Name, y.Name, StringComparison.OrdinalIgnoreCase));

                // Header

                sw.Write(Header);

                // Definitions

                foreach (var d in _defs)
                {
                    Console.WriteLine("System service: " + d.Name);

                    // Write the original definition, replacing "Nt" with "Zw".
                    sw.Write(string.Concat(d.Text.AsSpan(0, d.NameIndex), "Zw", d.Text.AsSpan(d.NameIndex + 2), "\r\n\r\n"));
                }

                // Footer

                sw.Write(Footer);
            }
        }
    }

    public static partial class RegexDefinition
    {
        [GeneratedRegex(@"NTSYSCALLAPI[\w\s_]*NTAPI\s*(Nt(\w)*)\(.*?\);", RegexOptions.Singleline)]
        public static partial Regex Regex();
    }

    public class ServiceDefinition : IEquatable<ServiceDefinition>
    {
        public string Name;
        public string Text;
        public int NameIndex;

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

            if (obj is not ServiceDefinition service)
                return false;

            return this.Name.Equals(service.Name, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(ServiceDefinition other)
        {
            return this.Name.Equals(other.Name, StringComparison.OrdinalIgnoreCase);
        }
    }

}
