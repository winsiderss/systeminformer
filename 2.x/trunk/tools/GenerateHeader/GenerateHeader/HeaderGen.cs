using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GenerateHeader
{
    class HeaderFile
    {
        public string Name;
        public List<string> Lines;
        public List<HeaderFile> Dependencies;
    }

    class HeaderGen
    {
        private string _baseDirectory;
        private string[] _modes;
        private string[] _files;
        private string _outputFile;
        private string _header = "";
        private string _footer = "";

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
                    case "modes":
                        _modes = split[1].ToLowerInvariant().Split(';');
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
                    bool blockMode = _modes.Any(modes.Contains);
                    bool lineMode = _modes.Any(mode =>
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

            var headerFiles = _files.Select(fileName =>
            {
                var fullFileName = _baseDirectory + "\\" + fileName;
                var lines = File.ReadAllLines(fullFileName).ToList();

                return new HeaderFile { Name = Path.GetFileName(fullFileName).ToLowerInvariant(), Lines = lines };
            }).ToDictionary(h => h.Name);

            foreach (var h in headerFiles.Values)
            {
                var partitions =
                    h.Lines
                    .Select(s =>
                    {
                        var trimmed = s.Trim().ToLowerInvariant();
                        if (trimmed.StartsWith("#include <") && trimmed.EndsWith(">"))
                        {
                            HeaderFile d;
                            if (headerFiles.TryGetValue(trimmed.Remove(trimmed.Length - 1).Remove(0, "#include <".Length), out d))
                                return Tuple.Create(s, d);
                            else
                                return Tuple.Create<string, HeaderFile>(s, null);
                        }
                        return Tuple.Create<string, HeaderFile>(s, null);
                    })
                    .ToLookup(p => p.Item2 != null);

                h.Lines = partitions[false].Select(p => p.Item1).ToList();
                h.Dependencies = partitions[true].Select(p => p.Item2).Distinct().ToList();

                foreach (var d in h.Dependencies)
                    Console.WriteLine("Dependency: " + h.Name + " -> " + d.Name);
            }

            // Generate the ordering.

            var orderedHeaderFiles = OrderHeaderFiles(_files.Select(s => headerFiles[Path.GetFileName(s).ToLower()]).ToList());

            // Process each header file and remove irrelevant content.

            foreach (var h in orderedHeaderFiles)
                h.Lines = ProcessHeaderLines(h.Lines);

            // Write out the result.

            StreamWriter sw = new StreamWriter(_baseDirectory + "\\" + _outputFile);

            // Header

            sw.Write(_header);

            // Header files

            foreach (var h in orderedHeaderFiles)
            {
                Console.WriteLine("Header file: " + h.Name);

                sw.WriteLine();
                sw.WriteLine("//");
                sw.WriteLine("// " + Path.GetFileNameWithoutExtension(h.Name));
                sw.WriteLine("//");
                sw.WriteLine();

                foreach (var line in h.Lines)
                    sw.WriteLine(line);
            }

            // Footer

            sw.Write(_footer);

            sw.Close();
        }
    }
}
