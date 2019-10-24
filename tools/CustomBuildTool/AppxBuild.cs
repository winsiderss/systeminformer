/*
 * Process Hacker Toolchain - 
 *   Build script
 * 
 * Copyright (C) dmex
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

using System;
using System.IO;
using System.Text;
using System.Security.Cryptography.X509Certificates;
using Microsoft.Win32;

namespace CustomBuildTool
{
    public static class AppxBuild
    {
        public static void BuildAppxPackage(string BuildOutputFolder, string BuildLongVersion, BuildFlags Flags)
        {
            string sdkRootPath = string.Empty;
            string[] cleanupAppxArray =
            {
                BuildOutputFolder + "\\AppxManifest32.xml",
                BuildOutputFolder + "\\AppxManifest64.xml",
                BuildOutputFolder + "\\package32.map",
                BuildOutputFolder + "\\package64.map",
                BuildOutputFolder + "\\bundle.map",
                BuildOutputFolder + "\\ProcessHacker-44.png",
                BuildOutputFolder + "\\ProcessHacker-50.png",
                BuildOutputFolder + "\\ProcessHacker-150.png"
            };

            Program.PrintColorMessage("Building processhacker-build-package.appxbundle...", ConsoleColor.Cyan);

            using (var view32 = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
            {
                using (var kitsroot = view32.OpenSubKey("Software\\Microsoft\\Windows Kits\\Installed Roots", false))
                {
                    sdkRootPath = (string)kitsroot.GetValue("WdkBinRootVersioned", string.Empty);
                }
            }

            if (string.IsNullOrEmpty(sdkRootPath))
            {
                Program.PrintColorMessage("[Skipped] Windows SDK", ConsoleColor.Red);
                return ;
            }

            string makeAppxExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\MakeAppx.exe");
            string signToolExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\SignTool.exe");

            try
            {
                //Win32.ImageResizeFile(44, "ProcessHacker\\resources\\ProcessHacker.png", BuildOutputFolder + "\\ProcessHacker-44.png");
                //Win32.ImageResizeFile(50, "ProcessHacker\\resources\\ProcessHacker.png", BuildOutputFolder + "\\ProcessHacker-50.png");
                //Win32.ImageResizeFile(150, "ProcessHacker\\resources\\ProcessHacker.png", BuildOutputFolder + "\\ProcessHacker-150.png");

                if ((Flags & BuildFlags.Build32bit) == BuildFlags.Build32bit)
                {
                    // create the package manifest
                    string appxManifestString = Properties.Resources.AppxManifest;
                    appxManifestString = appxManifestString.Replace("PH_APPX_ARCH", "x86");
                    appxManifestString = appxManifestString.Replace("PH_APPX_VERSION", BuildLongVersion);
                    File.WriteAllText(BuildOutputFolder + "\\AppxManifest32.xml", appxManifestString);

                    // create the package mapping file
                    StringBuilder packageMap32 = new StringBuilder(0x100);
                    packageMap32.AppendLine("[Files]");
                    packageMap32.AppendLine("\"" + BuildOutputFolder + "\\AppxManifest32.xml\" \"AppxManifest.xml\"");
                    packageMap32.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-44.png\" \"Assets\\ProcessHacker-44.png\"");
                    packageMap32.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-50.png\" \"Assets\\ProcessHacker-50.png\"");
                    packageMap32.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-150.png\" \"Assets\\ProcessHacker-150.png\"");

                    var filesToAdd = Directory.GetFiles("bin\\Release32", "*", SearchOption.AllDirectories);
                    foreach (string filePath in filesToAdd)
                    {
                        // Ignore junk files
                        if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase))
                        {
                            continue;
                        }

                        packageMap32.AppendLine("\"" + filePath + "\" \"" + filePath.Substring("bin\\Release32\\".Length) + "\"");
                    }
                    File.WriteAllText(BuildOutputFolder + "\\package32.map", packageMap32.ToString());

                    // create the package
                    var error = Win32.ShellExecute(
                        makeAppxExePath,
                        "pack /o /f " + BuildOutputFolder + "\\package32.map /p " +
                        BuildOutputFolder + "\\processhacker-build-package-x32.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);

                    // sign the package
                    error = Win32.ShellExecute(
                        signToolExePath,
                        "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\processhacker-appx.pfx /td SHA256 " +
                        BuildOutputFolder + "\\processhacker-build-package-x32.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
                }

                if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
                {
                    // create the package manifest
                    string appxManifestString = Properties.Resources.AppxManifest;
                    appxManifestString = appxManifestString.Replace("PH_APPX_ARCH", "x64");
                    appxManifestString = appxManifestString.Replace("PH_APPX_VERSION", BuildLongVersion);
                    File.WriteAllText(BuildOutputFolder + "\\AppxManifest64.xml", appxManifestString);

                    StringBuilder packageMap64 = new StringBuilder(0x100);
                    packageMap64.AppendLine("[Files]");
                    packageMap64.AppendLine("\"" + BuildOutputFolder + "\\AppxManifest64.xml\" \"AppxManifest.xml\"");
                    packageMap64.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-44.png\" \"Assets\\ProcessHacker-44.png\"");
                    packageMap64.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-50.png\" \"Assets\\ProcessHacker-50.png\"");
                    packageMap64.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-150.png\" \"Assets\\ProcessHacker-150.png\"");

                    var filesToAdd = Directory.GetFiles("bin\\Release64", "*", SearchOption.AllDirectories);
                    foreach (string filePath in filesToAdd)
                    {
                        // Ignore junk files
                        if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase))
                        {
                            continue;
                        }

                        packageMap64.AppendLine("\"" + filePath + "\" \"" + filePath.Substring("bin\\Release64\\".Length) + "\"");
                    }
                    File.WriteAllText(BuildOutputFolder + "\\package64.map", packageMap64.ToString());

                    // create the package
                    var error = Win32.ShellExecute(
                        makeAppxExePath,
                        "pack /o /f " + BuildOutputFolder + "\\package64.map /p " +
                        BuildOutputFolder + "\\processhacker-build-package-x64.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);

                    // sign the package
                    error = Win32.ShellExecute(
                        signToolExePath,
                        "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\processhacker-appx.pfx /td SHA256 " +
                        BuildOutputFolder + "\\processhacker-build-package-x64.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
                }

                {
                    // create the appx bundle map
                    StringBuilder bundleMap = new StringBuilder(0x100);
                    bundleMap.AppendLine("[Files]");
                    bundleMap.AppendLine("\"" + BuildOutputFolder + "\\processhacker-build-package-x32.appx\" \"processhacker-build-package-x32.appx\"");
                    bundleMap.AppendLine("\"" + BuildOutputFolder + "\\processhacker-build-package-x64.appx\" \"processhacker-build-package-x64.appx\"");
                    File.WriteAllText(BuildOutputFolder + "\\bundle.map", bundleMap.ToString());

                    if (File.Exists(BuildOutputFolder + "\\processhacker-build-package.appxbundle"))
                        File.Delete(BuildOutputFolder + "\\processhacker-build-package.appxbundle");

                    // create the appx bundle package
                    var error = Win32.ShellExecute(
                        makeAppxExePath,
                        "bundle /f " + BuildOutputFolder + "\\bundle.map /p " +
                        BuildOutputFolder + "\\processhacker-build-package.appxbundle"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);

                    // sign the appx bundle package
                    error = Win32.ShellExecute(
                        signToolExePath,
                        "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\processhacker-appx.pfx /td SHA256 " +
                        BuildOutputFolder + "\\processhacker-build-package.appxbundle"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
                }

                foreach (string file in cleanupAppxArray)
                {
                    if (File.Exists(file))
                        File.Delete(file);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
            }
        }

        public static bool BuildAppxSignature(string BuildOutputFolder)
        {
            string sdkRootPath = string.Empty;
            string[] cleanupAppxArray =
            {
                BuildOutputFolder + "\\processhacker-appx.pvk",
                BuildOutputFolder + "\\processhacker-appx.cer",
                BuildOutputFolder + "\\processhacker-appx.pfx"
            };

            Program.PrintColorMessage("Building Appx Signature...", ConsoleColor.Cyan);

            using (var view32 = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
            {
                using (var kitsroot = view32.OpenSubKey("Software\\Microsoft\\Windows Kits\\Installed Roots", false))
                {
                    sdkRootPath = (string)kitsroot.GetValue("WdkBinRootVersioned", string.Empty);
                }
            }

            if (string.IsNullOrEmpty(sdkRootPath))
            {
                Program.PrintColorMessage("[Skipped] Windows SDK", ConsoleColor.Red);
                return false;
            }

            string makeCertExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\MakeCert.exe");
            string pvk2PfxExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\Pvk2Pfx.exe");
            string certUtilExePath = Win32.SearchFile("certutil.exe");

            try
            {
                foreach (string file in cleanupAppxArray)
                {
                    if (File.Exists(file))
                        File.Delete(file);
                }

                string output = Win32.ShellExecute(makeCertExePath,
                    "/n " +
                    "\"CN=ProcessHacker, O=ProcessHacker, C=AU\" " +
                    "/r /h 0 " +
                    "/eku \"1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13\" " +
                    "/sv " +
                    BuildOutputFolder + "\\processhacker-appx.pvk " +
                    BuildOutputFolder + "\\processhacker-appx.cer "
                    );

                if (!string.IsNullOrEmpty(output) && !output.Equals("Succeeded", StringComparison.OrdinalIgnoreCase))
                {
                    Program.PrintColorMessage("[MakeCert] " + output, ConsoleColor.Red);
                    return false;
                }

                output = Win32.ShellExecute(pvk2PfxExePath,
                    "/pvk " + BuildOutputFolder + "\\processhacker-appx.pvk " +
                    "/spc " + BuildOutputFolder + "\\processhacker-appx.cer " +
                    "/pfx " + BuildOutputFolder + "\\processhacker-appx.pfx "
                    );

                if (!string.IsNullOrEmpty(output))
                {
                    Program.PrintColorMessage("[Pvk2Pfx] " + output, ConsoleColor.Red);
                    return false;
                }

                output = Win32.ShellExecute(certUtilExePath,
                    "-addStore TrustedPeople " + BuildOutputFolder + "\\processhacker-appx.cer"
                    );

                if (!string.IsNullOrEmpty(output) && !output.Contains("command completed successfully", StringComparison.OrdinalIgnoreCase))
                {
                    Program.PrintColorMessage("[Certutil] " + output, ConsoleColor.Red);
                    return false;
                }

                Program.PrintColorMessage("[Pvk2Pfx] " + output, ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CleanupAppxSignature()
        {
            try
            {
                X509Store store = new X509Store(StoreName.TrustedPeople, StoreLocation.LocalMachine);

                store.Open(OpenFlags.ReadWrite);

                foreach (X509Certificate2 c in store.Certificates)
                {
                    if (c.Subject.Equals("CN=ProcessHacker, O=ProcessHacker, C=AU", StringComparison.OrdinalIgnoreCase))
                    {
                        Console.WriteLine("Removing: {0}", c.Subject);
                        store.Remove(c);
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }
    }
}
