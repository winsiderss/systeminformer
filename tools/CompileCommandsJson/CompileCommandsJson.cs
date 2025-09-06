/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s
 *
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Encodings.Web;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.Unicode;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;

class CompileCommand
{
    [JsonPropertyName("directory")] public string Directory { get; set; }
    [JsonPropertyName("command")] public string Command { get; set; }
    [JsonPropertyName("file")] public string File { get; set; }
}

static class Context
{
    public static string CompileCommandFile = Path.Combine(Environment.CurrentDirectory, "compile_commands.json");
    public static List<CompileCommand> CompileCommands = new List<CompileCommand>();
    public static bool Initialized = false;
}

public class CompileCommandsJson : Logger
{
    public override void Initialize(IEventSource eventSource)
    {
        if (Context.Initialized)
        {
            return;
        }

        Context.Initialized = true;

        if (!String.IsNullOrEmpty(Parameters))
        {
            Context.CompileCommandFile = Parameters;
        }

        if (!File.Exists(Context.CompileCommandFile))
        {
            return;
        }

        string compileCommandsContent;

        try
        {
            compileCommandsContent = File.ReadAllText(Context.CompileCommandFile);
        }
        catch (Exception ex)
        {
            throw new LoggerException($"Failed to read {Context.CompileCommandFile}: {ex.Message}");
        }

        try
        {
            Context.CompileCommands = JsonSerializer.Deserialize<List<CompileCommand>>(compileCommandsContent);
        }
        catch (Exception ex)
        {
            throw new LoggerException($"Failed to parse {Context.CompileCommandFile}.json: {ex.Message}");
        }

        eventSource.AnyEventRaised += EventSource_AnyEventRaised;
    }

    private void EventSource_AnyEventRaised(object sender, BuildEventArgs args)
    {
        //
        // Based on MIT licensed:
        // https://github.com/0xabu/MsBuildCompileCommandsJson/blob/main/CompileCommandsJson.cs
        //

        if (!(args is TaskCommandLineEventArgs taskArgs) || taskArgs.TaskName != "CL")
        {
            return;
        }

        const string clExe = "cl.exe ";
        int clExeIndex = taskArgs.CommandLine.ToLower().IndexOf(clExe);
        if (clExeIndex == -1)
        {
            throw new LoggerException("Unexpected lack of CL.exe in " + taskArgs.CommandLine);
        }

        string compilerPath = taskArgs.CommandLine.Substring(0, clExeIndex + clExe.Length - 1);
        string argsString = taskArgs.CommandLine.Substring(clExeIndex + clExe.Length).TrimStart();
        string[] cmdArgs = CommandLineToArgs(argsString);

        //
        // Options that consume the following argument.
        //
        string[] optionsWithParam = {
                "D",
                "I",
                "F",
                "U",
                "FI",
                "FU",
                "analyze:log",
                "analyze:stacksize",
                "analyze:max_paths",
                "analyze:ruleset",
                "analyze:plugin"
            };

        List<string> maybeFilenames = new List<string>();
        List<string> filenames = new List<string>();
        bool allFilenamesAreSources = false;

        for (int i = 0; i < cmdArgs.Length; i++)
        {
            bool isOption = cmdArgs[i].StartsWith("/") || cmdArgs[i].StartsWith("-");
            string option = isOption ? cmdArgs[i].Substring(1) : "";

            if (isOption && Array.Exists(optionsWithParam, e => e == option))
            {
                //
                // Skip the next arg.
                //
                i++;
            }
            else if (option == "Tc" || option == "Tp")
            {
                //
                // Next arg is definitely a source file.
                //
                if (i + 1 < cmdArgs.Length)
                {
                    filenames.Add(cmdArgs[i + 1]);
                }
            }
            else if (option.StartsWith("Tc") || option.StartsWith("Tp"))
            {
                //
                // Rest of this arg is definitely a source file.
                //
                filenames.Add(option.Substring(2));
            }
            else if (option == "TC" || option == "TP")
            {
                //
                // All inputs are treated as source files.
                //
                allFilenamesAreSources = true;
            }
            else if (option == "link")
            {
                //
                // Only linker options follow.
                //
                break;
            }
            else if (isOption || cmdArgs[i].StartsWith("@"))
            {
                //
                // Other argument, ignore it.
                //
            }
            else
            {
                //
                // Non-argument, add it to our list of potential sources.
                //
                maybeFilenames.Add(cmdArgs[i]);
            }
        }

        //
        // Iterate over potential sources, and decide (based on the filename)
        // whether they are source inputs.
        //
        foreach (string filename in maybeFilenames)
        {
            if (allFilenamesAreSources)
            {
                filenames.Add(filename);
            }
            else
            {
                int suffixPos = filename.LastIndexOf('.');
                if (suffixPos != -1)
                {
                    string ext = filename.Substring(suffixPos + 1).ToLowerInvariant();
                    if (ext == "c" || ext == "cxx" || ext == "cpp")
                    {
                        filenames.Add(filename);
                    }
                }
            }
        }

        //
        // Simplify the compile command to avoid .. etc.
        //
        string compileCommand = '"' + Path.GetFullPath(compilerPath) + "\" " + argsString;
        string dirname = Path.GetDirectoryName(taskArgs.ProjectFile);

        //
        // For each source file, emit a JSON entry, update if it exists.
        //
        foreach (string filename in filenames)
        {
            var command = Context.CompileCommands.FirstOrDefault(
                p => p.File == filename && p.Directory == dirname
                );
            if (command != null)
            {
                command.Command = compileCommand;
            }
            else
            {
                Context.CompileCommands.Add(new CompileCommand
                {
                    Directory = dirname,
                    Command = compileCommand,
                    File = filename,
                });
            }
        }
    }

    [DllImport("shell32.dll", SetLastError = true)]
    static extern IntPtr CommandLineToArgvW([MarshalAs(UnmanagedType.LPWStr)] string lpCmdLine, out int pNumArgs);

    static string[] CommandLineToArgs(string commandLine)
    {
        int argc;
        var argv = CommandLineToArgvW(commandLine, out argc);
        if (argv == IntPtr.Zero)
            throw new System.ComponentModel.Win32Exception();
        try
        {
            var args = new string[argc];
            for (var i = 0; i < args.Length; i++)
            {
                var p = Marshal.ReadIntPtr(argv, i * IntPtr.Size);
                args[i] = Marshal.PtrToStringUni(p);
            }

            return args;
        }
        finally
        {
            Marshal.FreeHGlobal(argv);
        }
    }

    public override void Shutdown()
    {
        string compileCommandsContent;

        try
        {
            compileCommandsContent = JsonSerializer.Serialize(
                Context.CompileCommands,
                new JsonSerializerOptions
                {
                    WriteIndented = true,
                    IndentSize = 4,
                    IndentCharacter = ' ',
                    Encoder = JavaScriptEncoder.UnsafeRelaxedJsonEscaping,
                });
        }
        catch (Exception ex)
        {
            throw new LoggerException($"Failed to serialize compile_commands.json: {ex.Message}");
        }

        try
        {
            File.WriteAllText(Context.CompileCommandFile, compileCommandsContent);
        }
        catch (Exception ex)
        {
            throw new LoggerException($"Failed to write {Context.CompileCommands}: {ex.Message}");
        }

        base.Shutdown();
    }
}
