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

using System;
using System.Collections.Generic;
using System.Text;

namespace CustomBuildTool
{
    public struct BuildFile
    {
        public string FileName;
        public bool UploadNightly;

        public BuildFile(string Filename, bool UploadNightly)
        {
            this.FileName = Filename;
            this.UploadNightly = UploadNightly;
        }

        public override string ToString()
        {
            return this.FileName;
        }
    }

    public static class BuildConfig
    {
        public static readonly List<BuildFile> Build_Release_Files = new List<BuildFile>
        {
            new BuildFile("\\systeminformer-build-setup.exe", true), // nightly
            new BuildFile("\\systeminformer-build-bin.zip", true), // nightly
            new BuildFile("\\systeminformer-build-src.zip", false),
            new BuildFile("\\systeminformer-build-sdk.zip", false),
            new BuildFile("\\systeminformer-build-pdb.zip", false),
            //new BuildFile("\\systeminformer-build-checksums.txt", false),
        };

        public static readonly string[] Build_Sdk_Directories =
        {
            "sdk",
            "sdk\\include",
            "sdk\\dbg\\amd64",
            "sdk\\dbg\\i386",
            "sdk\\lib\\amd64",
            "sdk\\lib\\i386",
            //"sdk\\samples\\SamplePlugin",
            //"sdk\\samples\\SamplePlugin\\bin\\Release32"
        };

        public static readonly string[] Build_Phnt_Headers =
        {
            "ntbcd.h",
            "ntd3dkmt.h",
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
            "ntxcapi.h",
            "ntzwapi.h",
            "phnt.h",
            "phnt_ntdef.h",
            "phnt_windows.h",
            "subprocesstag.h",
            "winsta.h"
        };

        public static readonly string[] Build_Phlib_Headers =
        {
            "appresolver.h",
            "banned.h",
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
            "kphapi.h",
            "kphuser.h",
            "lsasup.h",
            "mapimg.h",
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
        };
    }
}
