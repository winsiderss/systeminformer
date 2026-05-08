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
    /// Provides methods for creating, uploading, and managing build artifacts and server configuration updates for
    /// deployment, including integration with GitHub releases.
    /// </summary>
    public static class BuildDeploy
    {
        /// <summary>
        /// Creates a DeployFile object for a build artifact, optionally generating a signature and hash.
        /// </summary>
        /// <param name="Channel">The release channel (e.g., "release", "canary").</param>
        /// <param name="Name">The display name of the file.</param>
        /// <param name="FileName">The path to the file.</param>
        /// <param name="CreateSignature">If true, generates a signature and hash for the file.</param>
        /// <returns>A DeployFile object if successful; otherwise, null.</returns>
        private static DeployFile CreateBuildDeployFile(string Channel, string Name, string FileName, bool CreateSignature = true)
        {
            if (CreateSignature)
            {
                string fileSign = BuildVerify.CreateSigString(Channel, FileName);

                if (string.IsNullOrWhiteSpace(fileSign))
                {
                    Program.PrintColorMessage("[ERROR] File signature failed.", ConsoleColor.Red);
                    return null;
                }

                string fileHash = BuildVerify.HashFile(FileName);
                string fileSize = Win32.GetFileSize(FileName).ToString();

                if (string.IsNullOrWhiteSpace(fileHash))
                {
                    Program.PrintColorMessage("[ERROR] File hash failed.", ConsoleColor.Red);
                    return null;
                }

                return new DeployFile(Name, FileName, fileHash, fileSign, fileSize);
            }
            else
            {
                string fileSize = Win32.GetFileSize(FileName).ToString();

                return new DeployFile(Name, FileName, null, null, fileSize);
            }
        }

        /// <summary>
        /// Updates the server configuration with build information and asset links after a canary build.
        /// Uploads a list of build artifacts to GitHub as release assets and returns the release information.
        /// </summary>
        /// <returns>True if the server configuration is updated successfully; otherwise, false.</returns>
        public static async Task<bool> BuildUpdateServerConfig()
        {
            if (!Build.BuildCanary)
                return true;
            if (Build.BuildToolsDebug)
                return true;

            // Define the files to upload (bool true create hash and signature)
            var Build_Upload_Files = new Dictionary<string, bool>(4, StringComparer.OrdinalIgnoreCase)
            {
                ["systeminformer-build-win32-bin.zip"] = true,
                ["systeminformer-build-win64-bin.zip"] = true,
                ["systeminformer-build-arm64-bin.zip"] = true,
                //["systeminformer-build-bin.zip"] = true,
                ["systeminformer-build-pdb.zip"] = false,
                //["systeminformer-build-release-setup.exe"] = true,
            };

            List<DeployFile> deployFiles = new List<DeployFile>();

            // N.B. HACK we only produce a "release" (default setting) build for the binary (portable).
            // Sign it using the "release" key. (jxy-s)

            var portable_zip = CreateBuildDeployFile("release", "systeminformer-build-bin.zip", Path.Join([Build.BuildOutputFolder, "systeminformer-build-bin.zip"]));
            var release_exe = CreateBuildDeployFile("release", "systeminformer-build-release-setup.exe", Path.Join([Build.BuildOutputFolder, "systeminformer-build-release-setup.exe"]));
            var canary_exe = CreateBuildDeployFile("canary", "systeminformer-build-canary-setup.exe", Path.Join([Build.BuildOutputFolder, "systeminformer-build-canary-setup.exe"]));

            if (portable_zip == null || release_exe == null || canary_exe == null)
            {
                Program.PrintColorMessage("[ERROR] CreateBuildDeployFile.", ConsoleColor.Red);
                return false;
            }

            deployFiles.Add(portable_zip);
            deployFiles.Add(release_exe);
            deployFiles.Add(canary_exe);

            foreach (var file in Build_Upload_Files)
            {
                deployFiles.Add(CreateBuildDeployFile(
                    "release",
                    file.Key,
                    Path.Join([Build.BuildOutputFolder, file.Key]),
                    file.Value
                    ));
            }

            // Upload the assets to github

            var githubMirrorUpload = await BuildUploadFilesToGithubAsync(deployFiles);
            if (githubMirrorUpload == null)
                return false;

            // Check assets uploaded to github
            var github_release_id = githubMirrorUpload.ReleaseId.ToString();
            var binzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-bin.zip"); // $"systeminformer-{Build.BuildLongVersion}-bin.zip"
            var relzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-release-setup.exe");
            var canzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-canary-setup.exe");

            if (
                string.IsNullOrWhiteSpace(github_release_id) || 
                string.IsNullOrWhiteSpace(binzipdownloadlink) ||
                string.IsNullOrWhiteSpace(relzipdownloadlink) || 
                string.IsNullOrWhiteSpace(canzipdownloadlink)
                )
            {
                Program.PrintColorMessage("[ERROR] GetDeployInfo failed.", ConsoleColor.Red);
                return false;
            }

            // Update the build information.

            if (!await BuildUploadServerConfig(portable_zip, release_exe, canary_exe, github_release_id))
            {
                Program.PrintColorMessage("[ERROR] BuildUploadServerConfig failed.", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Uploads a list of build artifacts to GitHub as release assets and returns the release information.
        /// </summary>
        /// <param name="Files">The list of DeployFile objects to upload.</param>
        /// <returns>A GithubRelease object with asset information if successful; otherwise, null.</returns>
        private static async Task<GithubRelease> BuildUploadFilesToGithubAsync(List<DeployFile> Files)
        {
            try
            {
                // Create a new github release.

                var newGithubRelease = await BuildGithub.CreateRelease(
                    Build.BuildLongVersion,
                    true, // draft
                    false, // prerelease
                    true
                    );

                if (newGithubRelease == null)
                {
                    Program.PrintColorMessage("[Github.CreateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Upload the files.

                foreach (DeployFile deployfile in Files)
                {
                    if (!File.Exists(deployfile.FileName))
                    {
                        Program.PrintColorMessage($"[Github.UploadAssets-NotFound] {deployfile.FileName}", ConsoleColor.Yellow);
                        continue;
                    }

                    string sourceName = Path.GetFileName(deployfile.FileName);

                    if (string.IsNullOrWhiteSpace(sourceName))
                    {
                        Program.PrintColorMessage("[Github.UploadAssets] Path.GetFileName", ConsoleColor.Red);
                        return null;
                    }

                    var label = sourceName.Replace(
                        "-build-",
                        $"-{Build.BuildLongVersion}-",
                        StringComparison.OrdinalIgnoreCase
                        );

                    var result = await BuildGithub.UploadAsset(
                        newGithubRelease.UploadUrl,
                        deployfile.FileName,
                        sourceName,
                        label
                        );

                    if (result == null)
                    {
                        Program.PrintColorMessage("[Github.UploadAssets]", ConsoleColor.Red);
                        return null;
                    }

                    if (string.IsNullOrWhiteSpace(result.HashValue))
                    {
                        Program.PrintColorMessage("[UpdateAssetsResponse] HashValue null.", ConsoleColor.Red);
                        return null;
                    }
                    if (string.IsNullOrWhiteSpace(result.DownloadUrl))
                    {
                        Program.PrintColorMessage("[UpdateAssetsResponse] DownloadUrl null.", ConsoleColor.Red);
                        return null;
                    }
                }

                // Update the release and make it public.

                var update = await BuildGithub.UpdateRelease(newGithubRelease.ReleaseId, false, false);

                if (update == null)
                {
                    Program.PrintColorMessage("[Github.UpdateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Get the release information.

                var release = await BuildGithub.GetRelease(update.ReleaseId);

                if (release == null)
                {
                    Program.PrintColorMessage("[Github.GetRelease]", ConsoleColor.Red);
                    return null;
                }

                var mirror = new GithubRelease(release.ReleaseId);

                // Grab the download urls.

                foreach (var file in release.Assets)
                {
                    if (!file.Uploaded)
                    {
                        Program.PrintColorMessage($"[GithubReleaseAsset-NotUploaded] {file.Name}", ConsoleColor.Red);
                        return null;
                    }

                    var deployFile = Files.Find(f => f.Name.Equals(file.Name, StringComparison.OrdinalIgnoreCase));
                    if (deployFile == null)
                    {
                        Program.PrintColorMessage($"[GithubReleaseAsset-MissingDeployFile] {file.Name}", ConsoleColor.Red);
                        return null;
                    }

                    if (!deployFile.UpdateAssetsResponse(file))
                    {
                        Program.PrintColorMessage($"[GithubReleaseAsset-UpdateAssetsResponse] {file.Name}", ConsoleColor.Red);
                        return null;
                    }

                    mirror.Files.Add(new GithubReleaseAsset(file.Name, file.DownloadUrl, deployFile));
                }

                return mirror;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Github] {ex}", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Updates the server configuration with build and asset information after uploading to GitHub.
        /// </summary>
        /// <param name="BinFile">The DeployFile for the binary zip.</param>
        /// <param name="ReleaseFile">The DeployFile for the release setup executable.</param>
        /// <param name="CanaryFile">The DeployFile for the canary setup executable.</param>
        /// <param name="GithubReleaseId">The GitHub release ID.</param>
        /// <returns>True if the server configuration is updated successfully; otherwise, false.</returns>
        private static async Task<bool> BuildUploadServerConfig(
            DeployFile BinFile,
            DeployFile ReleaseFile,
            DeployFile CanaryFile,
            string GithubReleaseId
            )
        {
            if (!Build.BuildCanary)
                return true;
            if (Build.BuildToolsDebug)
                return true;

            if (!Win32.GetEnvironmentVariable("BUILD_BUILDID", out string buildBuildId))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_CF_API", out string buildPostUrl))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_CF_KEY", out string buildPostKey))
                return false;

            if (string.IsNullOrWhiteSpace(buildBuildId) ||
                string.IsNullOrWhiteSpace(buildPostUrl) ||
                string.IsNullOrWhiteSpace(buildPostKey) ||
                string.IsNullOrWhiteSpace(GithubReleaseId))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid config.", ConsoleColor.Red);
                return false;
            }

            if (string.IsNullOrWhiteSpace(BinFile.DownloadHash) ||
                string.IsNullOrWhiteSpace(ReleaseFile.DownloadHash) ||
                string.IsNullOrWhiteSpace(CanaryFile.DownloadHash))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid DownloadLinks.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(BinFile.FileSignature) ||
                string.IsNullOrWhiteSpace(ReleaseFile.FileSignature) ||
                string.IsNullOrWhiteSpace(CanaryFile.FileSignature))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid FileSignatures.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(BinFile.FileHash) ||
                string.IsNullOrWhiteSpace(ReleaseFile.FileHash) ||
                string.IsNullOrWhiteSpace(CanaryFile.FileHash))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid FileHashs.", ConsoleColor.Red);
                return false;
            }

            try
            {
                BuildUpdateRequest buildUpdateRequest = new BuildUpdateRequest
                {
                    BuildUpdated = Build.BuildUpdated,
                    BuildDisplay = Build.BuildShortVersion,
                    BuildVersion = Build.BuildLongVersion,
                    BuildCommit = Build.BuildCommitHash,
                    BuildGithubId = GithubReleaseId,
                    BuildId = buildBuildId,
                    BinUrl = BinFile.DownloadLink,
                    BinLength = BinFile.FileLength,
                    BinHash = BinFile.FileHash,
                    BinSig = BinFile.FileSignature,
                    ReleaseSetupUrl = ReleaseFile.DownloadLink,
                    ReleaseSetupLength = ReleaseFile.FileLength,
                    ReleaseSetupHash = ReleaseFile.FileHash,
                    ReleaseSetupSig = ReleaseFile.FileSignature,
                    CanarySetupUrl = CanaryFile.DownloadLink,
                    CanarySetupLength = CanaryFile.FileLength,
                    CanarySetupHash = CanaryFile.FileHash,
                    CanarySetupSig = CanaryFile.FileSignature,
                };

                var buffer = buildUpdateRequest.SerializeToBytes();
                var bufferHash = BuildVerify.HashData(buffer);
                //var bufferSignature = BuildVerify.CreateSigString("buk", buildUpdateRequest.BuildId);

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, buildPostUrl))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Add("X-Build-Key", buildPostKey);
                    requestMessage.Headers.Add("X-Build-Hash", bufferHash);
                    //requestMessage.Headers.Add("X-Build-Sig", bufferSignature);

                    requestMessage.Content = new ByteArrayContent(buffer);
                    requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                    using var httpClient = BuildHttpClient.CreateHttpClient();
                    using var httpResult = await BuildHttpClient.SendMessageResponse(httpClient, requestMessage);

                    if (httpResult == null)
                    {
                        Program.PrintColorMessage("[UpdateBuildWebService-SF-TransportFailure]", ConsoleColor.Red);
                        return false;
                    }

                    if (!httpResult.IsSuccessStatusCode)
                    {
                        Program.PrintColorMessage(
                            $"[UpdateBuildWebService-SF-HTTP] {(int)httpResult.StatusCode} {httpResult.ReasonPhrase}",
                            ConsoleColor.Red);
                        return false;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[UpdateBuildWebService-SF] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }
    }
}
