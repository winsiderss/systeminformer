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

using System.Text.RegularExpressions;

namespace CustomBuildTool
{
    /// <summary>
    /// Updates the per-author copyright years in source-file headers based on git history.
    /// For each author already listed in a file's <c>Authors:</c> block, the end-year is
    /// extended to the most recent year that author actually committed to that file. New
    /// authors are never added and the company copyright line is never touched.
    /// </summary>
    public static partial class BuildCopyright
    {
        private static readonly string[] SourceExtensions = [".c", ".h", ".cpp", ".cxx", ".cs"];

        // Directories that never carry first-party author headers worth rewriting.
        private static readonly string[] ExcludeDirectories =
        [
            "\\.git\\",
            "\\bin\\",
            "\\obj\\",
            "\\thirdparty\\",
            "\\packages\\"
        ];

        // Matches a single author line inside the Authors: block, e.g. "     dmex    2017-2026".
        // prefix = " * " + indent, name = author (may contain spaces), start/end = year range.
        [GeneratedRegex(@"^(?<prefix>[ \t]*\*[ \t]+)(?<name>\S.*?)(?<sep>[ \t]+)(?<start>\d{4})(?:-(?<end>\d{4}))?[ \t]*$", RegexOptions.Multiline)]
        private static partial Regex AuthorLineRegex();

        private readonly record struct CommitAuthor(string Email, string Name, int Year);

        /// <summary>
        /// Updates copyright years for the given file or directory (defaults to the repo root).
        /// </summary>
        /// <param name="Path">A file or directory path. Empty means the repo working folder.</param>
        /// <returns>true on success; false on hard failure (git unavailable).</returns>
        public static bool UpdateCopyrightYears(string Path)
        {
            string repoRoot = Build.BuildWorkingFolder;

            if (string.IsNullOrWhiteSpace(repoRoot))
                repoRoot = Environment.CurrentDirectory;

            if (string.IsNullOrWhiteSpace(Utils.GetGitFilePath()))
            {
                Program.PrintColorMessage("[UpdateCopyrightYears] git executable not found.", ConsoleColor.Red);
                return false;
            }

            string target = string.IsNullOrWhiteSpace(Path) ? repoRoot : Utils.ExpandFullPath(Path);

            IEnumerable<string> files;

            if (File.Exists(target))
            {
                files = [target];
            }
            else if (Directory.Exists(target))
            {
                files = Utils.EnumerateDirectory(target, SourceExtensions);
            }
            else
            {
                Program.PrintColorMessage($"[UpdateCopyrightYears] path not found: {target}", ConsoleColor.Red);
                return false;
            }

            int changedFiles = 0;

            foreach (string file in files)
            {
                if (ExcludeDirectories.Any(d => file.Contains(d, StringComparison.OrdinalIgnoreCase)))
                    continue;

                if (UpdateFile(repoRoot, file))
                    changedFiles++;
            }

            Program.PrintColorMessage($"[UpdateCopyrightYears] Updated {changedFiles} file(s).", ConsoleColor.Green);
            return true;
        }

        /// <summary>
        /// Updates the Authors: block of a single file. Returns true if the file was rewritten.
        /// </summary>
        private static bool UpdateFile(string RepoRoot, string File)
        {
            string content = Utils.ReadAllText(File);

            if (string.IsNullOrEmpty(content))
                return false;

            int authorsIndex = content.IndexOf("Authors:", StringComparison.Ordinal);

            if (authorsIndex < 0)
                return false;

            // Bound the rewrite to the leading comment block (Authors: .. closing */).
            int blockEnd = content.IndexOf("*/", authorsIndex, StringComparison.Ordinal);

            if (blockEnd < 0)
                return false;

            List<CommitAuthor> commits = null; // lazily populated only when an author line is found

            string region = content.Substring(authorsIndex, blockEnd - authorsIndex);

            string updatedRegion = AuthorLineRegex().Replace(region, match =>
            {
                // The "Authors:" label line itself has no trailing year, so it never matches.
                string name = match.Groups["name"].Value.Trim();
                int start = int.Parse(match.Groups["start"].Value);
                int? existingEnd = match.Groups["end"].Success ? int.Parse(match.Groups["end"].Value) : null;

                commits ??= GetFileCommitAuthors(RepoRoot, File);

                int lastYear = 0;
                foreach (var commit in commits)
                {
                    if (commit.Name.Contains(name, StringComparison.OrdinalIgnoreCase) ||
                        commit.Email.Contains(name, StringComparison.OrdinalIgnoreCase))
                    {
                        if (commit.Year > lastYear)
                            lastYear = commit.Year;
                    }
                }

                if (lastYear == 0)
                    return match.Value; // no matching commits for this author; leave unchanged

                int newEnd = Math.Max(existingEnd ?? start, lastYear);

                if (newEnd < start)
                    newEnd = start;

                string yearText = newEnd > start ? $"{start}-{newEnd}" : $"{start}";

                return $"{match.Groups["prefix"].Value}{match.Groups["name"].Value}{match.Groups["sep"].Value}{yearText}";
            });

            if (updatedRegion == region)
                return false;

            content = string.Concat(content.AsSpan(0, authorsIndex), updatedRegion, content.AsSpan(blockEnd));
            Utils.WriteAllText(File, content);

            Program.PrintColorMessage($"  updated {Path.GetRelativePath(RepoRoot, File)}", ConsoleColor.Cyan);
            return true;
        }

        /// <summary>
        /// Returns the (email, name, year) of every commit that touched the file, following renames.
        /// </summary>
        private static List<CommitAuthor> GetFileCommitAuthors(string RepoRoot, string File)
        {
            var commits = new List<CommitAuthor>();

            string relativePath = Path.GetRelativePath(RepoRoot, File).Replace('\\', '/');

            string output = Utils.ExecuteGitCommand(RepoRoot,
                ["log", "--follow", "--format=%ae|%an|%ad", "--date=format:%Y", "--", relativePath]);

            if (string.IsNullOrWhiteSpace(output))
                return commits;

            foreach (string line in output.Split('\n', StringSplitOptions.RemoveEmptyEntries))
            {
                string[] parts = line.Trim().Split('|');

                if (parts.Length != 3)
                    continue;

                if (int.TryParse(parts[2], out int year))
                    commits.Add(new CommitAuthor(parts[0], parts[1], year));
            }

            return commits;
        }
    }
}
