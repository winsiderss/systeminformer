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
    /// Generates the NetworkTools <c>ResolvedPortsTable</c> from the authoritative IANA
    /// service-names registry. The CSV is downloaded (and vendored) under tools/thirdparty,
    /// parsed, deduplicated by (port, protocol), and emitted into the marked region of ports.c.
    /// </summary>
    public static class BuildPortServices
    {
        private const string CsvUrl = "https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.csv";
        private static readonly string CsvFile = Path.Join(["tools", "thirdparty", "iana", "service-names-port-numbers.csv"]);
        private static readonly string PortsFile = Path.Join(["plugins", "NetworkTools", "ports.c"]);
        private const string BeginMarker = "// <generated>";
        private const string EndMarker = "// </generated>";

        // Service names whose value should override (or supplement) the IANA registry. Keyed by
        // "port/protocol" (protocol 6 = tcp, 17 = udp). Currently empty: the IANA registry covers
        // every case cleanly. Add entries here to preserve any non-IANA custom names.
        private static readonly Dictionary<string, string> Overrides = new();

        private readonly record struct PortEntry(string Name, int Port, int Protocol);

        /// <summary>
        /// Downloads the IANA CSV, regenerates the port-service table, and rewrites ports.c.
        /// </summary>
        /// <returns>true on success; otherwise false.</returns>
        public static bool GeneratePortServices()
        {
            try
            {
                if (!DownloadCsv())
                    return false;

                List<PortEntry> entries;
                using (var reader = new StreamReader(CsvFile, Utils.UTF8NoBOM, true))
                    entries = ParseEntries(reader);

                if (entries.Count == 0)
                {
                    Program.PrintColorMessage("No port entries parsed from the IANA CSV.", ConsoleColor.Red);
                    return false;
                }

                if (!RewritePortsFile(entries))
                    return false;

                Program.PrintColorMessage($"Generated {entries.Count} port-service entries into {PortsFile}.", ConsoleColor.Green);
                return true;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"GeneratePortServices failed: {ex}", ConsoleColor.Red);
                return false;
            }
        }

        /// <summary>
        /// Downloads the IANA CSV and vendors a copy under tools/thirdparty/iana.
        /// </summary>
        private static bool DownloadCsv()
        {
            Program.PrintColorMessage("Downloading IANA service-names CSV...", ConsoleColor.Cyan);

            using var httpClient = BuildHttpClient.CreateHttpClient();
            using var request = new HttpRequestMessage(HttpMethod.Get, CsvUrl);
            using var response = BuildHttpClient.SendRequestMessage(httpClient, request).GetAwaiter().GetResult();

            if (response == null || !response.IsSuccessStatusCode)
            {
                Program.PrintColorMessage("Failed to download the IANA service-names CSV.", ConsoleColor.Red);
                return false;
            }

            string directory = Path.GetDirectoryName(CsvFile);

            if (!string.IsNullOrWhiteSpace(directory))
                Directory.CreateDirectory(directory);

            using var source = response.Content.ReadAsStream();
            using var destination = File.Create(CsvFile);
            source.CopyTo(destination);
            return destination.Length != 0;
        }

        /// <summary>
        /// Parses the CSV into deduplicated (port, protocol) entries sorted by port then protocol.
        /// </summary>
        private static List<PortEntry> ParseEntries(TextReader Csv)
        {
            var seen = new HashSet<int>();
            var entries = new List<PortEntry>();

            // Naive line splitting is safe here: only the first three columns (Service Name,
            // Port Number, Transport Protocol) are used, and none of them contain newlines.
            // Continuation lines from quoted description fields simply fail the filters below.
            string line;
            while ((line = Csv.ReadLine()) != null)
            {
                List<string> fields = SplitCsvLine(line);

                if (fields.Count < 3)
                    continue;

                string name = fields[0].Trim();
                string portText = fields[1].Trim();
                string protoText = fields[2].Trim().ToLowerInvariant();

                int protocol = protoText switch
                {
                    "tcp" => 6,
                    "udp" => 17,
                    _ => 0
                };

                if (protocol == 0)
                    continue;
                if (string.IsNullOrWhiteSpace(name))
                    continue;
                if (!int.TryParse(portText, NumberStyles.None, CultureInfo.InvariantCulture, out int port))
                    continue;
                if (port < 0 || port > 65535)
                    continue;
                if (!IsValidName(name))
                    continue;

                int key = (protocol << 16) | port;

                if (!seen.Add(key)) // first assignment wins
                    continue;

                if (Overrides.TryGetValue($"{port}/{protocol}", out string overrideName))
                    name = overrideName;

                entries.Add(new PortEntry(name, port, protocol));
            }

            // Apply any overrides for ports not present in the registry.
            foreach (var pair in Overrides)
            {
                string[] parts = pair.Key.Split('/');

                if (parts.Length != 2)
                    continue;
                if (!int.TryParse(parts[0], out int port) || !int.TryParse(parts[1], out int protocol))
                    continue;

                if (seen.Add((protocol << 16) | port))
                    entries.Add(new PortEntry(pair.Value, port, protocol));
            }

            return entries.OrderBy(e => e.Port).ThenBy(e => e.Protocol).ToList();
        }

        /// <summary>
        /// Returns true if the name is safe to emit inside a C wide-string literal.
        /// </summary>
        private static bool IsValidName(string Name)
        {
            foreach (char c in Name)
            {
                if (c < 0x20 || c > 0x7e || c == '"' || c == '\\')
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Splits a single CSV line into fields, honoring quoted fields and escaped quotes.
        /// </summary>
        private static List<string> SplitCsvLine(string Line)
        {
            var fields = new List<string>();
            var sb = new StringBuilder();
            bool inQuotes = false;

            for (int i = 0; i < Line.Length; i++)
            {
                char c = Line[i];

                if (inQuotes)
                {
                    if (c == '"')
                    {
                        if (i + 1 < Line.Length && Line[i + 1] == '"')
                        {
                            sb.Append('"');
                            i++;
                        }
                        else
                        {
                            inQuotes = false;
                        }
                    }
                    else
                    {
                        sb.Append(c);
                    }
                }
                else if (c == '"')
                {
                    inQuotes = true;
                }
                else if (c == ',')
                {
                    fields.Add(sb.ToString());
                    sb.Clear();
                }
                else if (c != '\r')
                {
                    sb.Append(c);
                }
            }

            fields.Add(sb.ToString());

            return fields;
        }

        /// <summary>
        /// Builds the table initializer rows (four entries per line) for the marked region.
        /// </summary>
        private static void WriteTableBlock(TextWriter Writer, List<PortEntry> Entries)
        {
            var line = new StringBuilder("    ");
            int column = 0;
            bool firstLine = true;

            foreach (PortEntry e in Entries)
            {
                string macro = e.Protocol == 6 ? "IPPROTO_TCP" : "IPPROTO_UDP";
                line.Append($"{{ SREF(L\"{e.Name}\"), {e.Port}, {macro} }}, ");

                if (++column == 4)
                {
                    if (!firstLine)
                        Writer.Write("\r\n");
                    Writer.Write(line.ToString().TrimEnd());
                    firstLine = false;
                    line.Clear();
                    line.Append("    ");
                    column = 0;
                }
            }

            if (column > 0)
            {
                if (!firstLine)
                    Writer.Write("\r\n");
                Writer.Write(line.ToString().TrimEnd());
            }
        }

        /// <summary>
        /// Replaces the text between the generated markers in ports.c with the new table block.
        /// </summary>
        private static bool RewritePortsFile(List<PortEntry> Entries)
        {
            bool foundBegin = false;
            bool foundEnd = false;
            foreach (string line in File.ReadLines(PortsFile))
            {
                if (!foundBegin && line.Contains(EndMarker, StringComparison.Ordinal))
                    break;
                if (!foundBegin && line.Contains(BeginMarker, StringComparison.Ordinal))
                    foundBegin = true;
                else if (foundBegin && line.Contains(EndMarker, StringComparison.Ordinal))
                {
                    foundEnd = true;
                    break;
                }
            }

            if (!foundEnd)
            {
                Program.PrintColorMessage($"Could not find the // <generated> markers in {PortsFile}.", ConsoleColor.Red);
                return false;
            }

            Utils.WriteTextIfChanged(PortsFile, writer =>
            {
                bool inBlock = false;
                bool completed = false;
                foreach (string line in Utils.ReadLinesWithEndings(PortsFile))
                {
                    if (!completed && !inBlock && line.Contains(BeginMarker, StringComparison.Ordinal))
                    {
                        writer.Write(line);
                        WriteTableBlock(writer, Entries);
                        writer.Write("\r\n");
                        inBlock = true;
                    }
                    else if (inBlock && line.Contains(EndMarker, StringComparison.Ordinal))
                    {
                        writer.Write(line);
                        inBlock = false;
                        completed = true;
                    }
                    else if (!inBlock)
                        writer.Write(line);
                }
            }, Comparison: StringComparison.Ordinal);

            return true;
        }
    }
}
