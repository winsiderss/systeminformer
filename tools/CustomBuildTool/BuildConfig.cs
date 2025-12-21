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
    /// Defines build configuration constants for the System Informer custom build tool.
    /// </summary>
    /// <remarks>
    /// This class contains static read-only collections that configure various aspects of the build process,
    /// including supported build channels, SDK directory structure, and header file collections.
    /// </remarks>
    public static class BuildConfig
    {
        /// <summary>
        /// A sorted dictionary that maps build channel names to their corresponding channel identifiers.
        /// </summary>
        /// <remarks>
        /// The dictionary uses case-insensitive string comparison for channel name lookups.
        /// Currently active channels are:
        /// - "release" (0): Stable release channel
        /// - "canary" (2): Canary/testing channel
        /// 
        /// N.B. Order is important, SortedDictionary is used on purpose.
        /// 
        public static readonly SortedDictionary<string, int> Build_Channels = new SortedDictionary<string, int>(StringComparer.OrdinalIgnoreCase)
        {
            ["release"] = 0, // PhReleaseChannel
            //["preview"] = 1, // PhPreviewChannel
            ["canary"] = 2, // PhCanaryChannel
            //["developer"] = 3, // PhDeveloperChannel
        };

        /// <summary>
        /// Gets an immutable array of SDK directory paths to be included in the build output.
        /// </summary>
        /// <remarks>
        /// Includes the main SDK directory and subdirectories for headers, debugger symbols, and libraries
        /// across multiple processor architectures (amd64, i386, arm64).
        /// Additional sample directories are available but currently commented out.
        /// </remarks>
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

        /// <summary>
        /// Gets an immutable array of PHNT (Platform Header Native Types) header file names.
        /// </summary>
        /// <remarks>
        /// Contains native Windows API headers required for low-level system integration and type definitions,
        /// and required for the SDK used by plugins and extensions.
        /// </remarks>
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

        /// <summary>
        /// An immutable array containing the header filenames for the phlib library build.
        /// This collection includes header files for various system information, UI components,
        /// and utility functions used throughout the System Informer project and are merged into
        /// the SDK for plugins and extensions.
        /// </summary>
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

        /// <summary>
        /// An immutable array containing the header filenames for the kphlib (Kernel System Informer) library build.
        /// </summary>
        /// <remarks>
        public static readonly ImmutableArray<string> Build_Kphlib_Headers =
        [
            "kphapi.h",
            "kphmsg.h",
            "kphmsgdefs.h"
        ];
    }
}
