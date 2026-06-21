# escape=`

# This stage is used when running from VS in fast mode (Default for Debug configuration)
FROM mcr.microsoft.com/dotnet/sdk:10.0-windowsservercore-ltsc2025 AS base
USER ContainerUser
WORKDIR /app

# This stage is used to build the project
FROM mcr.microsoft.com/dotnet/sdk:10.0-windowsservercore-ltsc2025 AS build
# Install Visual Studio Build Tools, they are required for aot publish
RUN curl -SL --output vs_buildtools.exe https://aka.ms/vs/26/release/vs_buildtools.exe
RUN vs_buildtools.exe --installPath C:\BuildTools --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.22000 --quiet --wait --norestart --nocache
ARG BUILD_CONFIGURATION=Release
WORKDIR /src
COPY ["CustomBuildTool.csproj", "."]
RUN dotnet restore "./CustomBuildTool.csproj"
COPY . .
WORKDIR "/src/."
RUN dotnet build "./CustomBuildTool.csproj" -c %BUILD_CONFIGURATION% -o /app/build

# This stage is used to publish the service project to be copied to the final stage
FROM build AS publish
ARG BUILD_CONFIGURATION=Release
RUN dotnet publish "./CustomBuildTool.csproj" -c %BUILD_CONFIGURATION% -o /app/publish

# This stage is used in production or when running from VS in regular mode (Default when not using the Debug configuration)
FROM mcr.microsoft.com/dotnet/sdk:10.0-windowsservercore-ltsc2025 AS final
WORKDIR /app
COPY --from=publish /app/publish .
ENTRYPOINT ["CustomBuildTool.exe"]