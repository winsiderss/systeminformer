using System;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;

namespace UpdateRevision
{
    internal class Program
    {
        private static readonly Regex LongGitTagRegex; // e.g.: v3.4-226-g7037fef
        private static readonly Regex LongVersionRegex; // e.g.: v3.4.226
        private static readonly Regex ShortVersionRegex; // e.g.: v3.4 (w/o commits)
        static Program()
        {
            var options = RegexOptions.Compiled | RegexOptions.ExplicitCapture | RegexOptions.CultureInvariant;
            LongGitTagRegex = new Regex(@"\Av(?<major>[0-9]+)\.(?<minor>[0-9]+)-(?<commits>[0-9]+)-g[0-9a-z]+\z", options);
            LongVersionRegex = new Regex(@"\Av(?<major>[0-9]+)\.(?<minor>[0-9]+)\.(?<commits>[0-9]+)\z", options);
            ShortVersionRegex = new Regex(@"\A(?<major>[0-9]+)\.(?<minor>[0-9]+)\z", options);
            options |= RegexOptions.Multiline;
        }

        private const int UnknownBuild = 9999;

        private class VersionInfo : IComparable<VersionInfo>, IEquatable<VersionInfo>
        {
            public int Major { get; private set; }
            public int Minor { get; private set; }
            public int Commits { get; private set; }
            public string RevisionGuid { get; private set; }

            public VersionInfo()
            {
                RevisionGuid = string.Empty;
            }

            public VersionInfo(string version, string guid = null)
            {
                var match = LongGitTagRegex.Match(version);
                if (!match.Success)
                    match = LongVersionRegex.Match(version);
                if (!match.Success || string.IsNullOrWhiteSpace(guid))
                {
                    Commits = UnknownBuild;
                    RevisionGuid = string.Empty;
                }
                else
                {
                    Commits = int.Parse(match.Groups["commits"].Value, NumberStyles.None, CultureInfo.InvariantCulture);
                    RevisionGuid = guid.Trim().ToLowerInvariant();
                }
                if (!match.Success)
                    match = ShortVersionRegex.Match(version);
                if (!match.Success)
                    throw new ArgumentException("Invalid version identifier: '" + version + "'");
                Major = int.Parse(match.Groups["major"].Value, NumberStyles.None, CultureInfo.InvariantCulture);
                Minor = int.Parse(match.Groups["minor"].Value, NumberStyles.None, CultureInfo.InvariantCulture);
            }

            public int CompareTo(VersionInfo vi)
            {
                int cmp = 1;
                if (!object.ReferenceEquals(vi, null))
                {
                    cmp = Major.CompareTo(vi.Major);
                    if (cmp == 0)
                        cmp = Minor.CompareTo(vi.Minor);
                }
                return cmp;
            }

            public bool Equals(VersionInfo vi)
            {
                return object.ReferenceEquals(vi, null) ? false : Major.Equals(vi.Major) && Minor.Equals(vi.Minor) && Commits.Equals(vi.Commits) && RevisionGuid.Equals(vi.RevisionGuid, StringComparison.Ordinal);
            }

            public override bool Equals(object obj)
            {
                return Equals(obj as VersionInfo);
            }

            public override string ToString()
            {
                return string.Format(CultureInfo.InvariantCulture, "{0:D}.{1:D}+{2:D}: {3}", Major, Minor, Commits, RevisionGuid);
            }

            public override int GetHashCode()
            {
                return this.ToString().GetHashCode();
            }

            public static bool operator ==(VersionInfo vi1, VersionInfo vi2)
            {
                return object.ReferenceEquals(vi2, null) ? object.ReferenceEquals(vi1, null) : vi2.Equals(vi1);
            }

            public static bool operator !=(VersionInfo vi1, VersionInfo vi2)
            {
                return object.ReferenceEquals(vi2, null) ? !object.ReferenceEquals(vi1, null) : !vi2.Equals(vi1);
            }
        }

        private static void GetRepositoryVersions(out VersionInfo currentRepositoryVersion, out VersionInfo latestRepositoryVersion)
        {
            var workingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
            var clrHash = new CommandLineRunner();
            var clrTags = new CommandLineRunner();
            var gitPath = GetGitPath();
            if (clrHash.RunCommandAndGetOutput(gitPath, "rev-parse --verify HEAD", workingDirectory) && clrTags.RunCommandAndGetOutput(gitPath, "describe --long --tags", workingDirectory))
            {
                if (!LongGitTagRegex.IsMatch(clrTags.Result))
                {
                    throw new Exception("Invalid Git version tag: '" + clrTags.Result + "' (vmajor.minor-build expected)");
                }
                currentRepositoryVersion = new VersionInfo(clrTags.Result, clrHash.Result);
            }
            else
            {
                currentRepositoryVersion = new VersionInfo(); // no git repository
            }
            if (clrHash.RunCommandAndGetOutput(gitPath, "rev-parse --verify refs/heads/master", workingDirectory) && clrTags.RunCommandAndGetOutput(gitPath, "describe --long --tags refs/heads/master", workingDirectory))
            {
                if (!LongGitTagRegex.IsMatch(clrTags.Result))
                {
                    throw new Exception("Invalid Git version tag: '" + clrTags.Result + "' (vmajor.minor-build expected)");
                }
                latestRepositoryVersion = new VersionInfo(clrTags.Result, clrHash.Result);
            }
            else
            {
                latestRepositoryVersion = new VersionInfo(); // no git repository
            }
        }
        
        private static void WriteWarning(string message)
        {
            Console.WriteLine();
            Console.WriteLine("WARNING: " + message);
        }

        private static int Main(string[] args)
        {
            var myName = Environment.GetCommandLineArgs()[0];
            myName = Path.GetFileNameWithoutExtension(string.IsNullOrWhiteSpace(myName) ? System.Reflection.Assembly.GetEntryAssembly().Location : myName);
            if (args.Length != 2)
            {
                Console.WriteLine("Usage: " + myName + " input-file output-file");
                return 1;
            }

            try
            {
                var inputFileName = Environment.GetCommandLineArgs()[1];
                var outputFileName = Environment.GetCommandLineArgs()[2];
                VersionInfo currentRepositoryVersion;
                VersionInfo latestRepositoryVersion;

                GetRepositoryVersions(out currentRepositoryVersion, out latestRepositoryVersion);

                Console.WriteLine("Current version: " + currentRepositoryVersion.ToString());
                Console.WriteLine("Latest version: " + latestRepositoryVersion.ToString());

                var template = File.ReadAllText(inputFileName);
                var output = template.Replace("$COMMITS$", currentRepositoryVersion.Commits.ToString());

                if (!File.Exists(outputFileName) || File.ReadAllText(outputFileName) != output)
                    File.WriteAllText(outputFileName, output);

                return 0;
            }
            catch (Exception exception)
            {
                Console.WriteLine();
                Console.WriteLine("ERROR: " + myName + ": " + exception.Message + Environment.NewLine + exception.StackTrace);

                return 2;
            }
        }

        private static string GetGitPath()
        {
            var envPath = Environment.GetEnvironmentVariable("PATH");
            if (!string.IsNullOrWhiteSpace(envPath))
            {
                foreach (var p in envPath.Split(Path.PathSeparator))
                {
                    var path = Path.Combine(p, "git.exe");
                    if (File.Exists(path))
                        return path;
                }
            }

            var gitPath = Path.Combine("Git", "bin", "git.exe");

            var envProgramFiles = Environment.GetEnvironmentVariable("ProgramFiles");
            if (!string.IsNullOrWhiteSpace(envProgramFiles))
            {
                var path = Path.Combine(envProgramFiles, gitPath);
                if (File.Exists(path))
                    return path;
            }

            envProgramFiles = Environment.GetEnvironmentVariable("ProgramFiles(x86)");
            if (!string.IsNullOrWhiteSpace(envProgramFiles))
            {
                var path = Path.Combine(envProgramFiles, gitPath);
                if (File.Exists(path))
                    return path;
            }

            var envSystemDrive = Environment.GetEnvironmentVariable("SystemDrive");
            if (string.IsNullOrEmpty(envSystemDrive))
            {
                WriteWarning(@"Environment.GetEnvironmentVariable(""SystemDrive"") returned null!");
            }
            if (!string.IsNullOrWhiteSpace(envSystemDrive))
            {
                var path = Path.Combine(envSystemDrive, "Program Files", gitPath);
                if (File.Exists(path))
                    return path;

                path = Path.Combine(envSystemDrive, "Program Files (x86)", gitPath);
                if (File.Exists(path))
                    return path;

                path = Path.Combine(envSystemDrive, gitPath);
                if (File.Exists(path))
                    return path;
            }

            try
            {
                var cRoot = new DriveInfo("C").RootDirectory.FullName;
                if (string.IsNullOrWhiteSpace(envSystemDrive) || !cRoot.StartsWith(envSystemDrive, StringComparison.OrdinalIgnoreCase))
                {
                    var path = Path.Combine(cRoot, "Program Files", gitPath);
                    if (File.Exists(path))
                        return path;

                    path = Path.Combine(cRoot, "Program Files (x86)", gitPath);
                    if (File.Exists(path))
                        return path;

                    path = Path.Combine(cRoot, gitPath);
                    if (File.Exists(path))
                        return path;
                }
            }
            catch (Exception exception)
            {
                WriteWarning(@"System.IO.DriveInfo(""C"") exception: " + exception.Message);
            }

            WriteWarning("Might not be able to run Git command line tool!");
            return "git";
        }

    }
}
