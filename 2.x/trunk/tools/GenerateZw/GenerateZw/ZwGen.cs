using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

namespace GenerateZw
{
    class ServiceDefinition
    {
        public string Name;
        public string Text;
        public int NameIndex;
    }

    class ServiceDefinitionComparer : IEqualityComparer<ServiceDefinition>
    {
        public bool Equals(ServiceDefinition x, ServiceDefinition y)
        {
            return string.Equals(x.Name, y.Name);
        }

        public int GetHashCode(ServiceDefinition obj)
        {
            return obj.Name.GetHashCode();
        }
    }

    class ZwGen
    {
        private string _baseDirectory;
        private string[] _files;
        private string _outputFile;
        private string _header = "";
        private string _footer = "";

        private List<ServiceDefinition> _defs;

        private string UnEscape(string text)
        {
            return text.Replace("\\r", "\r").Replace("\\n", "\n").Replace("\\\\", "\\"); 
        }

        public void LoadConfig(string fileName)
        {
            string[] lines = File.ReadAllLines(fileName);

            foreach (string line in lines)
            {
                string[] split = line.Split(new char[] { '=' }, 2);

                switch (split[0])
                {
                    case "base":
                        _baseDirectory = split[1];
                        break;
                    case "in":
                        _files = split[1].Split(';');
                        break;
                    case "out":
                        _outputFile = split[1];
                        break;
                    case "header":
                        _header = UnEscape(split[1]);
                        break;
                    case "footer":
                        _footer = UnEscape(split[1]);
                        break;
                }
            }
        }

        private void Parse(string text)
        {
            Regex regex = new Regex(@"NTSYSCALLAPI[\w\s_]*NTAPI\s*(Nt(\w)*)\(.*?\);", RegexOptions.Compiled | RegexOptions.Singleline);
            MatchCollection matches;

            matches = regex.Matches(text);

            foreach (Match match in matches)
            {
                _defs.Add(new ServiceDefinition() { Name = match.Groups[1].Value, Text = match.Value, NameIndex = match.Groups[1].Index - match.Index });
            }
        }

        public void Execute()
        {
            // Build up a list of definitions.

            _defs = new List<ServiceDefinition>();

            foreach (string fileName in _files)
                Parse(File.ReadAllText(_baseDirectory + "\\" + fileName));

            StreamWriter sw = new StreamWriter(_baseDirectory + "\\" + _outputFile);

            // Remove duplicates and sort.
            _defs = new List<ServiceDefinition>(_defs.Distinct(new ServiceDefinitionComparer()));
            _defs.Sort((x, y) => string.CompareOrdinal(x.Name, y.Name));

            // Header

            sw.Write(_header);

            // Definitions

            foreach (var d in _defs)
            {
                Console.WriteLine("System service: " + d.Name);

                // Write the original definition, replacing "Nt" with "Zw".
                sw.Write(d.Text.Substring(0, d.NameIndex) + "Zw" + d.Text.Substring(d.NameIndex + 2) + "\r\n\r\n");
            }

            // Footer

            sw.Write(_footer);

            sw.Close();
        }
    }
}
