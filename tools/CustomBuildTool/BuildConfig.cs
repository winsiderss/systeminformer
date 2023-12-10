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
        /// <summary>
        /// Container for files uploaded to Github.
        /// </summary>
        public static readonly BuildUploadFile[] Build_Release_Files =
        {
            new BuildUploadFile("\\systeminformer-build-setup.exe", true), // nightly
            new BuildUploadFile("\\systeminformer-build-bin.zip", true), // nightly
            new BuildUploadFile("\\systeminformer-build-src.zip", false),
            new BuildUploadFile("\\systeminformer-build-sdk.zip", false),
            new BuildUploadFile("\\systeminformer-build-pdb.zip", true), // nightly
            //new BuildUploadFile("\\systeminformer-build-checksums.txt", false),
        };

        /// <summary>
        /// Debug headers copied into the SDK directory.
        /// </summary>
        public static readonly string[] Build_Sdk_Directories =
        {
            "sdk",
            "sdk\\include",
            "sdk\\dbg\\amd64",
            "sdk\\dbg\\i386",
            "sdk\\dbg\\arm64",
            "sdk\\lib\\amd64",
            "sdk\\lib\\i386",
            "sdk\\lib\\arm64",
        };

        /// <summary>
        /// Native headers copied into the SDK directory.
        /// </summary>
        public static readonly string[] Build_Native_Headers =
        {
            "ntbcd.h",
            "ntd3dkmt.h",
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
        };

        /// <summary>
        /// Application headers copied into the SDK directory.
        /// </summary>
        public static readonly string[] Build_App_Headers =
        {
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
        };

        /// <summary>
        /// Kernel headers copied into the SDK directory.
        /// </summary>
        public static readonly string[] Build_Ksilib_Headers =
        {
            "kphapi.h",
            "kphmsg.h",
            "kphmsgdefs.h",
        };
    }

    /// <summary>
    /// Container for files uploaded to Github.
    /// </summary>
    public readonly struct BuildUploadFile
    {
        public readonly string FileName;
        public readonly bool UploadNightly;

        public BuildUploadFile(string Filename, bool UploadNightly)
        {
            this.FileName = Filename;
            this.UploadNightly = UploadNightly;
        }

        public override string ToString()
        {
            return this.FileName;
        }
    }
}
