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
    public static class BuildConfig
    {
        // N.B. Order is important, SortedDictionary is used on purpose.
        public static readonly SortedDictionary<string, int> Build_Channels = new SortedDictionary<string, int>(StringComparer.OrdinalIgnoreCase)
        {
            ["release"] = 0, // PhReleaseChannel
            //["preview"] = 1, // PhPreviewChannel
            ["canary"] = 2, // PhCanaryChannel
            //["developer"] = 3, // PhDeveloperChannel
        };

        public static readonly ImmutableArray<string> Build_Sdk_Directories =
        [
            "sdk",
            "sdk\\include",
            "sdk\\dbg\\amd64",
            "sdk\\dbg\\i386",
            "sdk\\dbg\\arm64",
            "sdk\\lib\\amd64",
            "sdk\\lib\\i386",
            "sdk\\lib\\arm64",
            //"sdk\\samples\\SamplePlugin",
            //"sdk\\samples\\SamplePlugin\\bin\\Release32"
        ];

        public static readonly ImmutableArray<string> Build_Phnt_Headers =
        [
            "ntafd.h",
            "ntbcd.h",
            "ntdbg.h",
            "ntexapi.h",
            "ntgdi.h",
            "ntimage.h",
            "ntintsafe.h",
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
            "ntsmss.h",
            "ntstrsafe.h",
            "ntsxs.h",
            "nttmapi.h",
            "nttp.h",
            "ntuser.h",
            "ntwmi.h",
            "ntwow64.h",
            "ntxcapi.h",
            "ntzwapi.h",
            "phnt.h",
            "phnt_ntdef.h",
            "phnt_windows.h",
            "smbios.h",
            "subprocesstag.h",
            "usermgr.h",
            "winsta.h"
        ];

        public static readonly ImmutableArray<string> Build_Phlib_Headers =
        [
            "appresolver.h",
            "circbuf.h",
            "circbuf_h.h",
            "cpysave.h",
            "dltmgr.h",
            "dspick.h",
            "emenu.h",
            "exlf.h",
            "exprodid.h",
            "fastlock.h",
            "filestream.h",
            "graph.h",
            "guisup.h",
            "guisupview.h",
            "hexedit.h",
            "hndlinfo.h",
            "json.h",
            "kphuser.h",
            "kphcomms.h",
            "lsasup.h",
            "mapimg.h",
            "mapldr.h",
            "ph.h",
            "phbase.h",
            "phbasesup.h",
            "phconfig.h",
            "phconsole.h",
            "phdata.h",
            "phfirmware.h",
            "phnative.h",
            "phnativeinl.h",
            "phnet.h",
            "phsup.h",
            "phutil.h",
            "provider.h",
            "queuedlock.h",
            "ref.h",
            "searchbox.h",
            "secedit.h",
            "settings.h",
            "svcsup.h",
            "symprv.h",
            "templ.h",
            "trace.h",
            "treenew.h",
            "verify.h",
            "workqueue.h"
        ];

        public static readonly ImmutableArray<string> Build_Kphlib_Headers =
        [
            "kphapi.h",
            "kphmsg.h",
            "kphmsgdefs.h"
        ];
    }

    public class DeployFile(string Name, string Filename, string FileHash, string FileSignature, string FileLength)
    {
        public readonly string Name = Name;
        public readonly string FileName = Filename;
        public readonly string FileHash = FileHash;
        public readonly string FileSignature = FileSignature;
        public readonly string FileLength = FileLength;

        public string DownloadHash { get; private set; }
        public string DownloadLink { get; private set; }

        public void UpdateAssetsResponse(GithubAssetsResponse response)
        {
            if (response == null)
            {
                Program.PrintColorMessage("[UpdateAssetsResponse] response null.", ConsoleColor.Red);
                return;
            }
            if (string.IsNullOrWhiteSpace(response.HashValue))
            {
                Program.PrintColorMessage("[UpdateAssetsResponse] HashValue null.", ConsoleColor.Red);
                return;
            }
            if (string.IsNullOrWhiteSpace(response.DownloadUrl))
            {
                Program.PrintColorMessage("[UpdateAssetsResponse] DownloadUrl null.", ConsoleColor.Red);
                return;
            }

            this.DownloadHash = response.HashValue;
            this.DownloadLink = response.DownloadUrl;
        }

        public override string ToString()
        {
            return this.Name;
        }

        public override int GetHashCode()
        {
            return this.Name.GetHashCode();
        }
    }

}
