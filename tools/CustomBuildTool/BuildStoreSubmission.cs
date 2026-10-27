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
    /// Configuration required to call the Microsoft Store submission API for MSI or EXE apps.
    /// </summary>
    /// <remarks>This type is never serialized; it only carries credentials and endpoints.</remarks>
    public class StoreClientConfiguration
    {
        /// <summary>
        /// Client Id of your Microsoft Entra ID Directory app.
        /// Example: 00001111-aaaa-2222-bbbb-3333cccc4444
        /// </summary>
        public string ClientId { get; set; }

        /// <summary>
        /// Client secret of your Microsoft Entra ID Directory app.
        /// </summary>
        public string ClientSecret { get; set; }

        /// <summary>
        /// Service root endpoint.
        /// Example: "https://api.store.microsoft.com"
        /// </summary>
        public string ServiceUrl { get; set; }

        /// <summary>
        /// Token endpoint to which the request is to be made. Specific to your Microsoft Entra ID Directory app.
        /// Example: https://login.microsoftonline.com/d454d300-128e-2d81-334a-27d9b2baf002/oauth2/v2.0/token
        /// </summary>
        public string TokenEndpoint { get; set; }

        /// <summary>
        /// Resource scope. If not provided (set to null), the default one is used for the production API
        /// endpoint ("https://api.store.microsoft.com/.default").
        /// </summary>
        public string Scope { get; set; }

        /// <summary>
        /// Partner Center Application ID.
        /// Example: 3e31a9f9-84e8-4d2d-9eba-487878d02ebf
        /// </summary>
        public string ApplicationId { get; set; }

        /// <summary>
        /// The Partner Center Seller Id.
        /// Example: 123456892
        /// </summary>
        public int SellerId { get; set; }
    }

    /// <summary>
    /// This class is a proxy that abstracts the functionality of the Microsoft Store submission API service.
    /// </summary>
    /// <remarks>Ported from the Microsoft Store submission API documentation sample. The original sample used
    /// Newtonsoft.Json together with <c>dynamic</c> payloads; both are incompatible with NativeAOT and trimming, so
    /// every request and response is bound to a concrete type and serialized through <see
    /// cref="StoreSubmissionJsonContext"/>.</remarks>
    public class SubmissionClient : IDisposable
    {
        public static readonly string Version = "1";
        private HttpClient HttpClient;
        private HttpClient ImageUploadClient;

        private readonly string AccessToken;

        public const string PackagesUrlTemplate = "/submission/v{0}/product/{1}/packages";
        public const string PackageByIdUrlTemplate = "/submission/v{0}/product/{1}/packages/{2}";
        public const string PackagesCommitUrlTemplate = "/submission/v{0}/product/{1}/packages/commit";
        public const string AppMetadataUrlTemplate = "/submission/v{0}/product/{1}/metadata";
        public const string AppListingsFetchMetadataUrlTemplate = "/submission/v{0}/product/{1}/metadata/listings";
        public const string ListingAssetsUrlTemplate = "/submission/v{0}/product/{1}/listings/assets";
        public const string ListingAssetsCreateUrlTemplate = "/submission/v{0}/product/{1}/listings/assets/create";
        public const string ListingAssetsCommitUrlTemplate = "/submission/v{0}/product/{1}/listings/assets/commit";
        public const string ProductDraftStatusPollingUrlTemplate = "/submission/v{0}/product/{1}/status";
        public const string CreateSubmissionUrlTemplate = "/submission/v{0}/product/{1}/submit";
        public const string SubmissionStatusPollingUrlTemplate = "/submission/v{0}/product/{1}/submission/{2}/status";

        private const string DefaultScope = "https://api.store.microsoft.com/.default";
        private const string JsonContentType = "application/json";
        private const string PngContentType = "image/png";
        private const string BinaryStreamContentType = "application/octet-stream";

        /// <summary>
        /// Initializes a new instance of the <see cref="SubmissionClient"/> class.
        /// </summary>
        /// <param name="AccessToken">The access token. This is a JWT token obtained from Microsoft Entra ID Directory
        /// allowing the caller to invoke the API on behalf of a user.</param>
        /// <param name="ServiceUrl">The service URL.</param>
        public SubmissionClient(string AccessToken, string ServiceUrl)
        {
            ArgumentException.ThrowIfNullOrEmpty(AccessToken);
            ArgumentException.ThrowIfNullOrEmpty(ServiceUrl);

            this.AccessToken = AccessToken;
            this.HttpClient = BuildHttpClient.CreateHttpClient();
            this.HttpClient.BaseAddress = new Uri(ServiceUrl);
            this.ImageUploadClient = BuildHttpClient.CreateHttpClient();
            this.DefaultHeaders = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Gets or sets the default headers sent with every service request.
        /// </summary>
        public Dictionary<string, string> DefaultHeaders { get; set; }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            if (this.HttpClient != null)
            {
                this.HttpClient.Dispose();
                this.HttpClient = null;
            }

            if (this.ImageUploadClient != null)
            {
                this.ImageUploadClient.Dispose();
                this.ImageUploadClient = null;
            }

            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Gets the authorization token for the provided client id, client secret, and the scope.
        /// This token is usually valid for 1 hour, so if your submission takes longer than that to complete,
        /// make sure to get a new one periodically.
        /// </summary>
        /// <param name="TokenEndpoint">Token endpoint to which the request is to be made. Specific to your
        /// Microsoft Entra ID Directory app. Example: https://login.microsoftonline.com/d454d300-128e-2d81-334a-27d9b2baf002/oauth2/v2.0/token</param>
        /// <param name="ClientId">Client Id of your Microsoft Entra ID Directory app. Example: 00001111-aaaa-2222-bbbb-3333cccc4444</param>
        /// <param name="ClientSecret">Client secret of your Microsoft Entra ID Directory app.</param>
        /// <param name="Scope">Scope. If not provided, the default one is used for the production API endpoint.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation. Optional.</param>
        /// <returns>Authorization token. Prepend it with "Bearer " and pass it in the request header as the
        /// value for the "Authorization" header.</returns>
        public static async ValueTask<string> GetClientCredentialAccessToken(
            string TokenEndpoint,
            string ClientId,
            string ClientSecret,
            string Scope = null,
            CancellationToken CancellationToken = default
            )
        {
            ArgumentException.ThrowIfNullOrEmpty(TokenEndpoint);
            ArgumentException.ThrowIfNullOrEmpty(ClientId);
            ArgumentException.ThrowIfNullOrEmpty(ClientSecret);

            Scope ??= DefaultScope;

            using (HttpClient client = BuildHttpClient.CreateHttpClient())
            using (HttpRequestMessage request = new HttpRequestMessage(HttpMethod.Post, TokenEndpoint))
            {
                request.Content = new FormUrlEncodedContent([
                    new("grant_type", "client_credentials"),
                    new("client_id", ClientId),
                    new("client_secret", ClientSecret),
                    new("scope", Scope)
                ]);

                using (HttpResponseMessage response = await BuildHttpClient.SendWithRetry(client, request, CancellationToken))
                {
                    var content = await response.Content.ReadAsStreamAsync(CancellationToken);

                    if (!response.IsSuccessStatusCode)
                    {
                        throw new HttpRequestException("GetClientCredentialAccessToken", null, response.StatusCode);
                    }

                    var result = await JsonSerializer.DeserializeAsync(content, StoreSubmissionJsonContext.Default.StoreTokenResponse, CancellationToken);

                    if (result == null || string.IsNullOrWhiteSpace(result.AccessToken))
                        throw new InvalidOperationException("The token endpoint did not return an access token.");

                    return result.AccessToken;
                }
            }
        }

        /// <summary>
        /// Invokes the specified HTTP method without a request body.
        /// </summary>
        /// <typeparam name="TResponse">The type of the deserialized response.</typeparam>
        /// <param name="HttpMethod">The HTTP method.</param>
        /// <param name="RelativeUrl">The relative URL.</param>
        /// <param name="ResponseTypeInfo">The source generated metadata for <typeparamref name="TResponse"/>.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation. Optional.</param>
        /// <returns>An instance of the type <typeparamref name="TResponse"/>.</returns>
        public ValueTask<TResponse> Invoke<TResponse>(
            HttpMethod HttpMethod,
            string RelativeUrl,
            JsonTypeInfo<TResponse> ResponseTypeInfo,
            CancellationToken CancellationToken = default
            )
        {
            return this.InvokeCore(HttpMethod, RelativeUrl, null, ResponseTypeInfo, CancellationToken);
        }

        /// <summary>
        /// Invokes the specified HTTP method with a JSON request body.
        /// </summary>
        /// <typeparam name="TRequest">The type of the request body.</typeparam>
        /// <typeparam name="TResponse">The type of the deserialized response.</typeparam>
        /// <param name="HttpMethod">The HTTP method.</param>
        /// <param name="RelativeUrl">The relative URL.</param>
        /// <param name="RequestContent">Content of the request.</param>
        /// <param name="RequestTypeInfo">The source generated metadata for <typeparamref name="TRequest"/>.</param>
        /// <param name="ResponseTypeInfo">The source generated metadata for <typeparamref name="TResponse"/>.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation. Optional.</param>
        /// <returns>An instance of the type <typeparamref name="TResponse"/>.</returns>
        public ValueTask<TResponse> Invoke<TRequest, TResponse>(
            HttpMethod HttpMethod,
            string RelativeUrl,
            TRequest RequestContent,
            JsonTypeInfo<TRequest> RequestTypeInfo,
            JsonTypeInfo<TResponse> ResponseTypeInfo,
            CancellationToken CancellationToken = default
            )
        {
            HttpContent content = null;

            if (RequestContent != null)
            {
                content = JsonContent.Create(RequestContent, RequestTypeInfo, new MediaTypeHeaderValue(JsonContentType, "utf-8"));
            }

            return this.InvokeCore(HttpMethod, RelativeUrl, content, ResponseTypeInfo, CancellationToken);
        }

        private async ValueTask<TResponse> InvokeCore<TResponse>(
            HttpMethod HttpMethod,
            string RelativeUrl,
            HttpContent RequestContent,
            JsonTypeInfo<TResponse> ResponseTypeInfo,
            CancellationToken CancellationToken
            )
        {
            using (var request = new HttpRequestMessage(HttpMethod, RelativeUrl))
            {
                this.SetRequest(request, RequestContent);

                using (HttpResponseMessage response = await BuildHttpClient.SendWithRetry(this.HttpClient, request, CancellationToken))
                {
                    if (this.TryHandleResponse(response, out TResponse handled))
                    {
                        return handled;
                    }

                    if (!response.IsSuccessStatusCode)
                    {
                        string content = await response.Content.ReadAsStringAsync(CancellationToken);
                        throw new HttpRequestException(content, null, response.StatusCode);
                    }

                    await using var stream = await response.Content.ReadAsStreamAsync(CancellationToken);
                    return await JsonSerializer.DeserializeAsync(stream, ResponseTypeInfo, CancellationToken);
                }
            }
        }

        /// <summary>
        /// Uploads a given image asset file to asset storage.
        /// </summary>
        /// <param name="AssetUploadUrl">Asset storage URL.</param>
        /// <param name="FileStream">The stream instance of the file to be uploaded.</param>
        /// <param name="CancellationToken">A cancellation token that can be used to cancel the operation. Optional.</param>
        public async ValueTask UploadAsset(string AssetUploadUrl, Stream FileStream, CancellationToken CancellationToken = default)
        {
            ArgumentException.ThrowIfNullOrEmpty(AssetUploadUrl);
            ArgumentNullException.ThrowIfNull(FileStream);

            using (var request = new HttpRequestMessage(HttpMethod.Put, AssetUploadUrl))
            {
                request.Headers.Add("x-ms-blob-type", "BlockBlob");
                request.Content = new StreamContent(FileStream);
                request.Content.Headers.ContentType = new MediaTypeHeaderValue(PngContentType);

                using (HttpResponseMessage response = await BuildHttpClient.SendWithRetry(this.ImageUploadClient, request, CancellationToken))
                {
                    if (response.IsSuccessStatusCode)
                        return;

                    throw new HttpRequestException(await response.Content.ReadAsStringAsync(CancellationToken), null, response.StatusCode);
                }
            }
        }

        /// <summary>
        /// Sets the authorization header, the default headers and the request body.
        /// </summary>
        /// <param name="Request">The request.</param>
        /// <param name="RequestContent">Content of the request, or null when the request has no body.</param>
        protected virtual void SetRequest(HttpRequestMessage Request, HttpContent RequestContent)
        {
            Request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", this.AccessToken);

            foreach (var header in this.DefaultHeaders)
            {
                Request.Headers.Add(header.Key, header.Value);
            }

            if (RequestContent != null)
            {
                Request.Content = RequestContent;
            }
        }

        /// <summary>
        /// Tries to handle the response.
        /// </summary>
        /// <typeparam name="TResponse">The type of the deserialized response.</typeparam>
        /// <param name="Response">The response.</param>
        /// <param name="Result">The result.</param>
        /// <returns>true if the response was handled.</returns>
        protected virtual bool TryHandleResponse<TResponse>(HttpResponseMessage Response, out TResponse Result)
        {
            Result = default;
            return false;
        }
    }

    /// <summary>
    /// Demonstrates updating an app submission using several methods of the Microsoft Store submission API.
    /// </summary>
    /// <remarks>This is a step-for-step port of the documentation sample, including its demonstration values, so
    /// that it can be compared against the upstream sample. It is not wired into any build command.</remarks>
    public class AppSubmissionUpdateSample
    {
        private readonly StoreClientConfiguration ClientConfig;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppSubmissionUpdateSample"/> class.
        /// </summary>
        /// <param name="Configuration">An instance of <see cref="StoreClientConfiguration"/> with all parameters populated.</param>
        public AppSubmissionUpdateSample(StoreClientConfiguration Configuration)
        {
            ArgumentNullException.ThrowIfNull(Configuration);

            this.ClientConfig = Configuration;
        }

        /// <summary>
        /// Main method to run the sample application.
        /// </summary>
        /// <exception cref="InvalidOperationException"></exception>
        public async Task RunAppSubmissionUpdateSample()
        {
            // **********************
            //       SETTINGS
            // **********************
            var appId = this.ClientConfig.ApplicationId;
            var clientId = this.ClientConfig.ClientId;
            var clientSecret = this.ClientConfig.ClientSecret;
            var serviceEndpoint = this.ClientConfig.ServiceUrl;
            var tokenEndpoint = this.ClientConfig.TokenEndpoint;
            var scope = this.ClientConfig.Scope;

            // Get authorization token.
            Program.PrintColorMessage("Getting authorization token", ConsoleColor.Cyan);

            var accessToken = await SubmissionClient.GetClientCredentialAccessToken(
                tokenEndpoint,
                clientId,
                clientSecret,
                scope);

            using var client = new SubmissionClient(accessToken, serviceEndpoint);

            client.DefaultHeaders = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                { "X-Seller-Account-Id", this.ClientConfig.SellerId.ToString(CultureInfo.InvariantCulture) }
            };

            Program.PrintColorMessage("Getting Current Application Draft Status", ConsoleColor.Cyan);

            var appDraftStatus = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.ProductDraftStatusPollingUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreProductDraftStatus);

            PrintResponse(appDraftStatus, StoreSubmissionJsonContext.Default.StoreApiResponseStoreProductDraftStatus);

            Program.PrintColorMessage("Getting Application Packages", ConsoleColor.Cyan);

            var packagesResponse = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.PackagesUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStorePackagesData);

            PrintResponse(packagesResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStorePackagesData);

            if (packagesResponse?.ResponseData?.Packages == null || packagesResponse.ResponseData.Packages.Count == 0)
                throw new InvalidOperationException("The current draft submission does not contain any packages.");

            Program.PrintColorMessage("Getting Single Package", ConsoleColor.Cyan);

            var singlePackageResponse = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.PackageByIdUrlTemplate, SubmissionClient.Version, appId, packagesResponse.ResponseData.Packages[0].PackageId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStorePackagesData);

            PrintResponse(singlePackageResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStorePackagesData);

            Program.PrintColorMessage("Updating Entire Package Set", ConsoleColor.Cyan);

            // Update data in Packages list to have final set of updated Packages

            // Example - Updating Installer Parameters
            packagesResponse.ResponseData.Packages[0].InstallerParameters = "/s /r new-args";

            var packagesUpdateRequest = new StorePackagesUpdateRequest
            {
                Packages = packagesResponse.ResponseData.Packages
            };

            var packagesUpdateResponse = await client.Invoke(
                HttpMethod.Put,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.PackagesUrlTemplate, SubmissionClient.Version, appId),
                packagesUpdateRequest,
                StoreSubmissionJsonContext.Default.StorePackagesUpdateRequest,
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            PrintResponse(packagesUpdateResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            Program.PrintColorMessage("Updating Single Package's Download Url", ConsoleColor.Cyan);

            // Update data in the SinglePackage object

            var singlePackageUpdateRequest = singlePackageResponse.ResponseData.Packages[0];

            // Example - Updating Installer Parameters
            singlePackageUpdateRequest.InstallerParameters = "/s /r /t new-args";

            var packageUpdateResponse = await client.Invoke(
                HttpMethod.Patch,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.PackageByIdUrlTemplate, SubmissionClient.Version, appId, singlePackageUpdateRequest.PackageId),
                singlePackageUpdateRequest,
                StoreSubmissionJsonContext.Default.StorePackage,
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            PrintResponse(packageUpdateResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            Program.PrintColorMessage("Committing Packages", ConsoleColor.Cyan);

            var packageCommitResponse = await client.Invoke(
                HttpMethod.Post,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.PackagesCommitUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            PrintResponse(packageCommitResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            Program.PrintColorMessage("Polling Package Upload Status", ConsoleColor.Cyan);

            appDraftStatus = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.ProductDraftStatusPollingUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreProductDraftStatus);

            while (appDraftStatus?.ResponseData == null || !appDraftStatus.ResponseData.IsReady)
            {
                appDraftStatus = await client.Invoke(
                    HttpMethod.Get,
                    string.Format(CultureInfo.InvariantCulture, SubmissionClient.ProductDraftStatusPollingUrlTemplate, SubmissionClient.Version, appId),
                    StoreSubmissionJsonContext.Default.StoreApiResponseStoreProductDraftStatus);

                Program.PrintColorMessage("Waiting for Upload to finish", ConsoleColor.Yellow);

                await Task.Delay(TimeSpan.FromSeconds(2));

                if (appDraftStatus?.Errors != null && appDraftStatus.Errors.Count > 0)
                {
                    for (var index = 0; index < appDraftStatus.Errors.Count; index++)
                    {
                        if (string.Equals(appDraftStatus.Errors[index].Code, "packageuploaderror", StringComparison.OrdinalIgnoreCase))
                        {
                            throw new InvalidOperationException("Package Upload Failed. Please try committing packages again.");
                        }
                    }
                }
            }

            Program.PrintColorMessage("Getting Application Metadata - All Modules", ConsoleColor.Cyan);

            var appMetadata = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.AppMetadataUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreMetadataData);

            PrintResponse(appMetadata, StoreSubmissionJsonContext.Default.StoreApiResponseStoreMetadataData);

            Program.PrintColorMessage("Getting Application Metadata - Listings", ConsoleColor.Cyan);

            var appListingsMetadata = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.AppListingsFetchMetadataUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreMetadataData);

            PrintResponse(appListingsMetadata, StoreSubmissionJsonContext.Default.StoreApiResponseStoreMetadataData);

            if (appListingsMetadata?.ResponseData?.Listings == null || appListingsMetadata.ResponseData.Listings.Count == 0)
                throw new InvalidOperationException("The current draft submission does not contain any listings.");

            Program.PrintColorMessage("Updating Listings Metadata - Description", ConsoleColor.Cyan);

            // Update Required Fields in Listings Metadata Object - Per Language. For eg. AppListingsMetadata.ResponseData.Listings[0]

            // Example - Updating Description
            appListingsMetadata.ResponseData.Listings[0].Description = "New Description Updated By C# Sample Code";

            // Note: the fetch returns 'listings' as an array while the update sends a single object.
            var listingsUpdateRequest = new StoreListingsUpdateRequest
            {
                Listings = appListingsMetadata.ResponseData.Listings[0]
            };

            var updateListingsMetadataResponse = await client.Invoke(
                HttpMethod.Put,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.AppMetadataUrlTemplate, SubmissionClient.Version, appId),
                listingsUpdateRequest,
                StoreSubmissionJsonContext.Default.StoreListingsUpdateRequest,
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            PrintResponse(updateListingsMetadataResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            Program.PrintColorMessage("Getting All Listings Assets", ConsoleColor.Cyan);

            var listingAssets = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.ListingAssetsUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreListingAssetsData);

            PrintResponse(listingAssets, StoreSubmissionJsonContext.Default.StoreApiResponseStoreListingAssetsData);

            if (listingAssets?.ResponseData?.ListingAssets == null || listingAssets.ResponseData.ListingAssets.Count == 0)
                throw new InvalidOperationException("The current draft submission does not contain any listing assets.");

            Program.PrintColorMessage("Creating Listing Assets for 1 Screenshot", ConsoleColor.Cyan);

            var assetCreateRequest = new StoreCreateAssetsRequest
            {
                Language = listingAssets.ResponseData.ListingAssets[0].Language,
                CreateAssetRequest = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase)
                {
                    { "Screenshot", 1 },
                    { "Logo", 0 }
                }
            };

            var assetCreateResponse = await client.Invoke(
                HttpMethod.Post,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.ListingAssetsCreateUrlTemplate, SubmissionClient.Version, appId),
                assetCreateRequest,
                StoreSubmissionJsonContext.Default.StoreCreateAssetsRequest,
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreCreatedAssetsData);

            PrintResponse(assetCreateResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStoreCreatedAssetsData);

            if (assetCreateResponse?.ResponseData?.ListingAssets?.Screenshots == null || assetCreateResponse.ResponseData.ListingAssets.Screenshots.Count == 0)
                throw new InvalidOperationException("The create listing assets request did not return any screenshot upload urls.");

            var createdScreenshot = assetCreateResponse.ResponseData.ListingAssets.Screenshots[0];

            Program.PrintColorMessage("Uploading Listing Assets", ConsoleColor.Cyan);

            // Path to PNG File to be Uploaded as Screenshot / Logo
            var pathToFile = "./Image.png";

            await using (var assetToUpload = File.OpenRead(pathToFile))
            {
                await client.UploadAsset(createdScreenshot.PrimaryAssetUploadUrl, assetToUpload);
            }

            Program.PrintColorMessage("Committing Listing Assets", ConsoleColor.Cyan);

            var assetCommitRequest = new StoreAssetsCommitRequest
            {
                ListingAssets = new StoreAssetCommitSet
                {
                    Language = listingAssets.ResponseData.ListingAssets[0].Language,
                    StoreLogos = ToAssetReferences(listingAssets.ResponseData.ListingAssets[0].StoreLogos),
                    Screenshots = new List<StoreAssetReference>
                    {
                        new StoreAssetReference
                        {
                            Id = createdScreenshot.Id,
                            AssetUrl = createdScreenshot.PrimaryAssetUploadUrl
                        }
                    }
                }
            };

            var assetCommitResponse = await client.Invoke(
                HttpMethod.Put,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.ListingAssetsCommitUrlTemplate, SubmissionClient.Version, appId),
                assetCommitRequest,
                StoreSubmissionJsonContext.Default.StoreAssetsCommitRequest,
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            PrintResponse(assetCommitResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStoreCommitData);

            Program.PrintColorMessage("Getting Current Application Draft Status before Submission", ConsoleColor.Cyan);

            appDraftStatus = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.ProductDraftStatusPollingUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreProductDraftStatus);

            PrintResponse(appDraftStatus, StoreSubmissionJsonContext.Default.StoreApiResponseStoreProductDraftStatus);

            if (appDraftStatus?.ResponseData == null || !appDraftStatus.ResponseData.IsReady)
            {
                throw new InvalidOperationException("Application Current Status is not in Ready Status for All Modules");
            }

            Program.PrintColorMessage("Creating Submission", ConsoleColor.Cyan);

            var submissionCreationResponse = await client.Invoke(
                HttpMethod.Post,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.CreateSubmissionUrlTemplate, SubmissionClient.Version, appId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreSubmissionCreatedData);

            PrintResponse(submissionCreationResponse, StoreSubmissionJsonContext.Default.StoreApiResponseStoreSubmissionCreatedData);

            if (submissionCreationResponse?.ResponseData == null || string.IsNullOrWhiteSpace(submissionCreationResponse.ResponseData.SubmissionId))
                throw new InvalidOperationException("The create submission request did not return a submission id.");

            Program.PrintColorMessage("Current Submission Status", ConsoleColor.Cyan);

            var submissionStatus = await client.Invoke(
                HttpMethod.Get,
                string.Format(CultureInfo.InvariantCulture, SubmissionClient.SubmissionStatusPollingUrlTemplate, SubmissionClient.Version, appId, submissionCreationResponse.ResponseData.SubmissionId),
                StoreSubmissionJsonContext.Default.StoreApiResponseStoreSubmissionStatusData);

            PrintResponse(submissionStatus, StoreSubmissionJsonContext.Default.StoreApiResponseStoreSubmissionStatusData);

            // User can Poll on this API to know if Submission Status is INPROGRESS, PUBLISHED or FAILED.
            // This Process involves File Scanning, App Certification and Publishing and can take more than a day.
        }

        /// <summary>
        /// Projects a set of existing listing assets to the identifier and url pairs expected by the commit API.
        /// </summary>
        private static List<StoreAssetReference> ToAssetReferences(List<StoreImageAsset> Assets)
        {
            if (Assets == null)
                return null;

            var references = new List<StoreAssetReference>(Assets.Count);

            foreach (var asset in Assets)
            {
                references.Add(new StoreAssetReference
                {
                    Id = asset.Id,
                    AssetUrl = asset.AssetUrl
                });
            }

            return references;
        }

        /// <summary>
        /// Writes the JSON representation of a service response to the console.
        /// </summary>
        private static void PrintResponse<TResponse>(TResponse Response, JsonTypeInfo<TResponse> ResponseTypeInfo)
        {
            if (Response == null)
                return;

            Console.WriteLine(JsonSerializer.Serialize(Response, ResponseTypeInfo));
        }
    }
}

namespace CustomBuildTool
{
    /// <summary>
    /// The envelope returned by every Microsoft Store submission API method.
    /// </summary>
    /// <typeparam name="T">The type of the method specific response payload.</typeparam>
    public class StoreApiResponse<T>
    {
        [JsonPropertyName("isSuccess")]
        public bool IsSuccess { get; set; }

        /// <summary>
        /// The list of error or warning messages if any.
        /// </summary>
        [JsonPropertyName("errors")]
        public List<StoreApiError> Errors { get; set; }

        [JsonPropertyName("responseData")]
        public T ResponseData { get; set; }
    }

    public class StoreApiError
    {
        /// <summary>
        /// The error code of the message.
        /// </summary>
        [JsonPropertyName("code")]
        public string Code { get; set; }

        /// <summary>
        /// The description of the error.
        /// </summary>
        [JsonPropertyName("message")]
        public string Message { get; set; }

        /// <summary>
        /// The entity from which the error originated.
        /// </summary>
        [JsonPropertyName("target")]
        public string Target { get; set; }
    }

    /// <summary>
    /// The token response returned by the Microsoft Entra ID token endpoint.
    /// </summary>
    public class StoreTokenResponse
    {
        [JsonPropertyName("access_token")]
        public string AccessToken { get; set; }

        [JsonPropertyName("token_type")]
        public string TokenType { get; set; }

        [JsonPropertyName("expires_in")]
        public long ExpiresIn { get; set; }
    }

    /// <summary>
    /// Product draft status polling payload.
    /// </summary>
    public class StoreProductDraftStatus
    {
        [JsonPropertyName("isReady")]
        public bool IsReady { get; set; }

        [JsonPropertyName("ongoingSubmissionId")]
        public string OngoingSubmissionId { get; set; }
    }

    /// <summary>
    /// The payload shared by the commit and module update methods.
    /// </summary>
    public class StoreCommitData
    {
        [JsonPropertyName("pollingUrl")]
        public string PollingUrl { get; set; }

        [JsonPropertyName("ongoingSubmissionId")]
        public string OngoingSubmissionId { get; set; }
    }

    public class StorePackagesData
    {
        [JsonPropertyName("packages")]
        public List<StorePackage> Packages { get; set; }
    }

    public class StorePackage
    {
        [JsonPropertyName("packageId")]
        public string PackageId { get; set; }

        [JsonPropertyName("packageUrl")]
        public string PackageUrl { get; set; }

        [JsonPropertyName("languages")]
        public List<string> Languages { get; set; }

        /// <summary>
        /// Should contain a single architecture - Neutral, X86, X64, Arm, Arm64.
        /// </summary>
        [JsonPropertyName("architectures")]
        public List<string> Architectures { get; set; }

        /// <summary>
        /// This should be marked as true if the installer runs in silent mode without requiring switches.
        /// </summary>
        [JsonPropertyName("isSilentInstall")]
        public bool IsSilentInstall { get; set; }

        [JsonPropertyName("installerParameters")]
        public string InstallerParameters { get; set; }

        [JsonPropertyName("genericDocUrl")]
        public string GenericDocUrl { get; set; }

        [JsonPropertyName("errorDetails")]
        public List<StorePackageErrorDetail> ErrorDetails { get; set; }

        [JsonPropertyName("packageType")]
        public string PackageType { get; set; }
    }

    public class StorePackageErrorDetail
    {
        /// <summary>
        /// One of [rebootRequired, installCancelled, alreadyInstalled, downgrade, packageRejected,
        /// internalError, otherError, applicationNotFound, contactSupport, invalidParameter,
        /// diskFull, networkFailure, packageInUse, invalidPackage, downloadFailure,
        /// serverFailure, installFailure, uninstallFailure, custom, blockedByPolicy,
        /// systemNotSupported, userCancelled, installationInProgress, installationSuccessful, miscellaneous].
        /// </summary>
        [JsonPropertyName("errorScenario")]
        public string ErrorScenario { get; set; }

        [JsonPropertyName("errorScenarioDetails")]
        public List<StorePackageErrorScenario> ErrorScenarioDetails { get; set; }
    }

    public class StorePackageErrorScenario
    {
        /// <summary>
        /// Error code which can be present during installation.
        /// </summary>
        [JsonPropertyName("errorValue")]
        public string ErrorValue { get; set; }

        [JsonPropertyName("errorUrl")]
        public string ErrorUrl { get; set; }
    }

    /// <summary>
    /// The metadata payload. The listings fetch method populates only <see cref="Listings"/>.
    /// </summary>
    public class StoreMetadataData
    {
        [JsonPropertyName("availability")]
        public StoreAvailability Availability { get; set; }

        [JsonPropertyName("properties")]
        public StoreProperties Properties { get; set; }

        [JsonPropertyName("listings")]
        public List<StoreListing> Listings { get; set; }
    }

    public class StoreAvailability
    {
        [JsonPropertyName("markets")]
        public List<string> Markets { get; set; }

        [JsonPropertyName("discoverability")]
        public string Discoverability { get; set; }

        [JsonPropertyName("enableInFutureMarkets")]
        public bool EnableInFutureMarkets { get; set; }

        [JsonPropertyName("pricing")]
        public string Pricing { get; set; }

        [JsonPropertyName("freeTrial")]
        public string FreeTrial { get; set; }
    }

    public class StoreProperties
    {
        [JsonPropertyName("isPrivacyPolicyRequired")]
        public bool IsPrivacyPolicyRequired { get; set; }

        [JsonPropertyName("privacyPolicyUrl")]
        public string PrivacyPolicyUrl { get; set; }

        [JsonPropertyName("website")]
        public string Website { get; set; }

        [JsonPropertyName("supportContactInfo")]
        public string SupportContactInfo { get; set; }

        [JsonPropertyName("certificationNotes")]
        public string CertificationNotes { get; set; }

        [JsonPropertyName("category")]
        public string Category { get; set; }

        [JsonPropertyName("subcategory")]
        public string Subcategory { get; set; }

        [JsonPropertyName("productDeclarations")]
        public StoreProductDeclarations ProductDeclarations { get; set; }

        [JsonPropertyName("isSystemFeatureRequired")]
        public List<StoreSystemFeature> IsSystemFeatureRequired { get; set; }

        [JsonPropertyName("systemRequirementDetails")]
        public List<StoreSystemRequirement> SystemRequirementDetails { get; set; }
    }

    public class StoreProductDeclarations
    {
        [JsonPropertyName("dependsOnDriversOrNT")]
        public bool DependsOnDriversOrNT { get; set; }

        [JsonPropertyName("accessibilitySupport")]
        public bool AccessibilitySupport { get; set; }

        [JsonPropertyName("penAndInkSupport")]
        public bool PenAndInkSupport { get; set; }
    }

    public class StoreSystemFeature
    {
        [JsonPropertyName("isRequired")]
        public bool IsRequired { get; set; }

        [JsonPropertyName("isRecommended")]
        public bool IsRecommended { get; set; }

        /// <summary>
        /// One of [Touch, Keyboard, Mouse, Camera, NFC_HCE, NFC_Proximity, Bluetooth_LE, Telephony, Microphone].
        /// </summary>
        [JsonPropertyName("hardwareItemType")]
        public string HardwareItemType { get; set; }
    }

    public class StoreSystemRequirement
    {
        [JsonPropertyName("minimumRequirement")]
        public string MinimumRequirement { get; set; }

        [JsonPropertyName("recommendedRequirement")]
        public string RecommendedRequirement { get; set; }

        /// <summary>
        /// One of [Memory, DirectX, Video_Memory, Processor, Graphics].
        /// </summary>
        [JsonPropertyName("hardwareItemType")]
        public string HardwareItemType { get; set; }
    }

    public class StoreListing
    {
        [JsonPropertyName("language")]
        public string Language { get; set; }

        [JsonPropertyName("description")]
        public string Description { get; set; }

        [JsonPropertyName("whatsNew")]
        public string WhatsNew { get; set; }

        [JsonPropertyName("productFeatures")]
        public List<string> ProductFeatures { get; set; }

        [JsonPropertyName("shortDescription")]
        public string ShortDescription { get; set; }

        [JsonPropertyName("searchTerms")]
        public List<string> SearchTerms { get; set; }

        [JsonPropertyName("additionalLicenseTerms")]
        public string AdditionalLicenseTerms { get; set; }

        [JsonPropertyName("copyright")]
        public string Copyright { get; set; }

        [JsonPropertyName("developedBy")]
        public string DevelopedBy { get; set; }

        [JsonPropertyName("sortTitle")]
        public string SortTitle { get; set; }

        [JsonPropertyName("requirements")]
        public List<StoreListingRequirement> Requirements { get; set; }

        [JsonPropertyName("contactInfo")]
        public string ContactInfo { get; set; }
    }

    public class StoreListingRequirement
    {
        [JsonPropertyName("minimumHardware")]
        public string MinimumHardware { get; set; }

        [JsonPropertyName("recommendedHardware")]
        public string RecommendedHardware { get; set; }
    }

    public class StoreListingAssetsData
    {
        [JsonPropertyName("listingAssets")]
        public List<StoreListingAsset> ListingAssets { get; set; }
    }

    public class StoreListingAsset
    {
        [JsonPropertyName("language")]
        public string Language { get; set; }

        [JsonPropertyName("storeLogos")]
        public List<StoreImageAsset> StoreLogos { get; set; }

        [JsonPropertyName("screenshots")]
        public List<StoreImageAsset> Screenshots { get; set; }
    }

    public class StoreImageAsset
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("assetUrl")]
        public string AssetUrl { get; set; }

        [JsonPropertyName("imageSize")]
        public StoreImageSize ImageSize { get; set; }
    }

    public class StoreImageSize
    {
        [JsonPropertyName("width")]
        public int Width { get; set; }

        [JsonPropertyName("height")]
        public int Height { get; set; }
    }

    public class StoreCreatedAssetsData
    {
        [JsonPropertyName("listingAssets")]
        public StoreCreatedAssetSet ListingAssets { get; set; }
    }

    public class StoreCreatedAssetSet
    {
        [JsonPropertyName("language")]
        public string Language { get; set; }

        [JsonPropertyName("storeLogos")]
        public List<StoreCreatedAsset> StoreLogos { get; set; }

        [JsonPropertyName("screenshots")]
        public List<StoreCreatedAsset> Screenshots { get; set; }
    }

    public class StoreCreatedAsset
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        /// <summary>
        /// Short lived SAS url used to upload the asset to the blob store.
        /// </summary>
        [JsonPropertyName("primaryAssetUploadUrl")]
        public string PrimaryAssetUploadUrl { get; set; }

        [JsonPropertyName("secondaryAssetUploadUrl")]
        public string SecondaryAssetUploadUrl { get; set; }
    }

    public class StoreSubmissionCreatedData
    {
        [JsonPropertyName("submissionId")]
        public string SubmissionId { get; set; }

        [JsonPropertyName("pollingUrl")]
        public string PollingUrl { get; set; }

        [JsonPropertyName("ongoingSubmissionId")]
        public string OngoingSubmissionId { get; set; }
    }

    public class StoreSubmissionStatusData
    {
        /// <summary>
        /// Publishing status of the submission - [INPROGRESS, PUBLISHED, FAILED, UNKNOWN].
        /// </summary>
        [JsonPropertyName("publishingStatus")]
        public string PublishingStatus { get; set; }

        /// <summary>
        /// Indicates if publishing has failed and won't be retried.
        /// </summary>
        [JsonPropertyName("hasFailed")]
        public bool HasFailed { get; set; }
    }

    /// <summary>
    /// The request body of the full package module update method.
    /// </summary>
    public class StorePackagesUpdateRequest
    {
        [JsonPropertyName("packages")]
        public List<StorePackage> Packages { get; set; }
    }

    /// <summary>
    /// The request body of the listings metadata update method.
    /// </summary>
    /// <remarks>The fetch method returns the listings as an array while the update method expects a single
    /// object; this asymmetry matches the service contract.</remarks>
    public class StoreListingsUpdateRequest
    {
        [JsonPropertyName("listings")]
        public StoreListing Listings { get; set; }
    }

    /// <summary>
    /// The request body of the create listing assets method.
    /// </summary>
    public class StoreCreateAssetsRequest
    {
        [JsonPropertyName("language")]
        public string Language { get; set; }

        /// <summary>
        /// The number of assets to create per asset type. Screenshot [1 - 10], Logo [1 or 2].
        /// </summary>
        [JsonPropertyName("createAssetRequest")]
        public Dictionary<string, int> CreateAssetRequest { get; set; }
    }

    /// <summary>
    /// The request body of the commit listing assets method.
    /// </summary>
    public class StoreAssetsCommitRequest
    {
        [JsonPropertyName("listingAssets")]
        public StoreAssetCommitSet ListingAssets { get; set; }
    }

    public class StoreAssetCommitSet
    {
        [JsonPropertyName("language")]
        public string Language { get; set; }

        [JsonPropertyName("storeLogos")]
        public List<StoreAssetReference> StoreLogos { get; set; }

        [JsonPropertyName("screenshots")]
        public List<StoreAssetReference> Screenshots { get; set; }
    }

    public class StoreAssetReference
    {
        /// <summary>
        /// Either an existing id retained from the get listing assets method or a new id under which an
        /// asset was uploaded by the create listing assets method.
        /// </summary>
        [JsonPropertyName("id")]
        public string Id { get; set; }

        /// <summary>
        /// Either an existing asset url retained from the get listing assets method or the upload url used
        /// by the create listing assets method.
        /// </summary>
        [JsonPropertyName("assetUrl")]
        public string AssetUrl { get; set; }
    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, WriteIndented = false, GenerationMode = JsonSourceGenerationMode.Default)]
    [JsonSerializable(typeof(StoreTokenResponse))]
    [JsonSerializable(typeof(StoreApiResponse<StoreProductDraftStatus>), TypeInfoPropertyName = "StoreApiResponseStoreProductDraftStatus")]
    [JsonSerializable(typeof(StoreApiResponse<StoreCommitData>), TypeInfoPropertyName = "StoreApiResponseStoreCommitData")]
    [JsonSerializable(typeof(StoreApiResponse<StorePackagesData>), TypeInfoPropertyName = "StoreApiResponseStorePackagesData")]
    [JsonSerializable(typeof(StoreApiResponse<StoreMetadataData>), TypeInfoPropertyName = "StoreApiResponseStoreMetadataData")]
    [JsonSerializable(typeof(StoreApiResponse<StoreListingAssetsData>), TypeInfoPropertyName = "StoreApiResponseStoreListingAssetsData")]
    [JsonSerializable(typeof(StoreApiResponse<StoreCreatedAssetsData>), TypeInfoPropertyName = "StoreApiResponseStoreCreatedAssetsData")]
    [JsonSerializable(typeof(StoreApiResponse<StoreSubmissionCreatedData>), TypeInfoPropertyName = "StoreApiResponseStoreSubmissionCreatedData")]
    [JsonSerializable(typeof(StoreApiResponse<StoreSubmissionStatusData>), TypeInfoPropertyName = "StoreApiResponseStoreSubmissionStatusData")]
    [JsonSerializable(typeof(StorePackage))]
    [JsonSerializable(typeof(StorePackagesUpdateRequest))]
    [JsonSerializable(typeof(StoreListingsUpdateRequest))]
    [JsonSerializable(typeof(StoreCreateAssetsRequest))]
    [JsonSerializable(typeof(StoreAssetsCommitRequest))]
    internal partial class StoreSubmissionJsonContext : JsonSerializerContext;
}
