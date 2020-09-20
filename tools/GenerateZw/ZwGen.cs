using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Text.RegularExpressions;

namespace GenerateZw
{
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

    public class ZwGen
    {
        private string BaseDirectory;
        private string OutputFile;
        private string Header;
        private string Footer;
        private string[] Files;

        public ZwGen()
        {
            this.OutputFile = "ntzwapi.h";
            this.BaseDirectory = "phnt\\include";
            this.Files = new[]
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
            this.Header = "#ifndef _NTZWAPI_H\r\n#define _NTZWAPI_H\r\n\r\n// This file was automatically generated. Do not edit.\r\n\r\n";
            this.Footer = "#endif\r\n";
        }

        private string UnEscape(string text)
        {
            return text.Replace("\\r", "\r", StringComparison.OrdinalIgnoreCase)
                       .Replace("\\n", "\n", StringComparison.OrdinalIgnoreCase)
                       .Replace("\\\\", "\\", StringComparison.OrdinalIgnoreCase);
        }

        public void LoadConfig(string fileName)
        {
            if (File.Exists(fileName))
            {
                string[] lines = File.ReadAllLines(fileName);

                foreach (string line in lines)
                {
                    string[] split = line.Split(new char[] { '=' }, 2);

                    switch (split[0])
                    {
                        case "base":
                            this.BaseDirectory = split[1];
                            break;
                        case "in":
                            this.Files = split[1].Split(';');
                            break;
                        case "out":
                            this.OutputFile = split[1];
                            break;
                        case "header":
                            this.Header = UnEscape(split[1]);
                            break;
                        case "footer":
                            this.Footer = UnEscape(split[1]);
                            break;
                    }
                }
            }
            else
            {
                try
                {
                    DirectoryInfo info = new DirectoryInfo(".");

                    while (info.Parent != null && info.Parent.Parent != null)
                    {
                        info = info.Parent;

                        if (Directory.Exists(info.FullName + "\\phnt\\include"))
                        {
                            Directory.SetCurrentDirectory(info.FullName);
                            break;
                        }
                    }
                }
                catch { }
            }
        }

        public void Execute()
        {
            // Build up a list of definitions.

            var definitions = new List<ServiceDefinition>();
            var regex = new Regex(@"NTSYSCALLAPI[\w\s_]*NTAPI\s*(Nt(\w)*)\(.*?\);", RegexOptions.Compiled | RegexOptions.Singleline);

            foreach (string fileName in this.Files)
            {
                var file = this.BaseDirectory + Path.DirectorySeparatorChar + fileName;
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

            using (StreamWriter sw = new StreamWriter(this.BaseDirectory + Path.DirectorySeparatorChar + this.OutputFile))
            {
                // Remove duplicates and sort.

                List<ServiceDefinition> _defs = definitions.Distinct().ToList();
                _defs.Sort((x, y) => string.Compare(x.Name, y.Name, StringComparison.OrdinalIgnoreCase));

                // Header

                sw.Write(this.Header);

                // Definitions

                foreach (var d in _defs)
                {
                    Console.WriteLine("System service: " + d.Name);

                    // Write the original definition, replacing "Nt" with "Zw".
                    sw.Write(d.Text.Substring(0, d.NameIndex) + "Zw" + d.Text.Substring(d.NameIndex + 2) + "\r\n\r\n");
                }

                // Footer

                sw.Write(this.Footer);
            }
        }
    }
}
