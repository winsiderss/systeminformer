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
            { "release",   0 }, // PhReleaseChannel
            //{ "preview",   1 }, // PhPreviewChannel
            { "canary",    2 }, // PhCanaryChannel
            //{ "developer", 3 }, // PhDeveloperChannel
        };

        public static readonly ImmutableArray<BuildFile> Build_Release_Files =
        [
            new BuildFile("\\systeminformer-build-release-setup.exe", true),
            new BuildFile("\\systeminformer-build-release-bin.zip", true),
            //new BuildFile("\\systeminformer-build-preview-setup.exe", true),
            //new BuildFile("\\systeminformer-build-preview-bin.zip", true),
            new BuildFile("\\systeminformer-build-canary-setup.exe", true),
            new BuildFile("\\systeminformer-build-canary-bin.zip", true),
            //new BuildFile("\\systeminformer-build-developer-setup.exe", true),
            //new BuildFile("\\systeminformer-build-developer-bin.zip", true),
            new BuildFile("\\systeminformer-build-src.zip", false),
            new BuildFile("\\systeminformer-build-sdk.zip", false),
            new BuildFile("\\systeminformer-build-pdb.zip", true),
            //new BuildFile("\\systeminformer-build-checksums.txt", false),
        ];

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
            "ntbcd.h",
            "ntdbg.h",
            "ntexapi.h",
            "ntgdi.h",
            "ntimage.h",
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
            "ntsxs.h",
            "nttmapi.h",
            "nttp.h",
            "ntwmi.h",
            "ntwow64.h",
            "ntxcapi.h",
            "ntzwapi.h",
            "phnt.h",
            "phnt_ntdef.h",
            "phnt_windows.h",
            "subprocesstag.h",
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
            "phdata.h",
            "phnative.h",
            "phnativeinl.h",
            "phnet.h",
            "phsup.h",
            "phutil.h",
            "provider.h",
            "queuedlock.h",
            "ref.h",
            "secedit.h",
            "settings.h",
            "svcsup.h",
            "symprv.h",
            "templ.h",
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

    public readonly struct BuildFile
    {
        public readonly string FileName;
        public readonly bool UploadCanary;

        public BuildFile(string Filename, bool UploadCanary)
        {
            this.FileName = Filename;
            this.UploadCanary = UploadCanary;
        }

        public override string ToString()
        {
            return this.FileName;
        }

        public override int GetHashCode()
        {
            return this.FileName.GetHashCode();
        }
    }
}
