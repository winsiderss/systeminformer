using System;
using System.IO;
using System.Linq;

namespace CustomBuildTool
{
    [System.Security.SuppressUnmanagedCodeSecurity]
    public static class VisualStudio
    {
        public static string GetMsbuildFilePath()
        {
            string vswhere = Environment.ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe");

            // Note: vswere.exe was only released with build 15.0.26418.1
            if (File.Exists(vswhere))
            {
                string vswhereResult = Win32.ShellExecute(vswhere,
                    "-latest " +
                    "-requires Microsoft.Component.MSBuild " +
                    "-property installationPath "
                    );

                if (string.IsNullOrEmpty(vswhereResult))
                    return null;

                if (File.Exists(vswhereResult + "\\MSBuild\\15.0\\Bin\\MSBuild.exe"))
                    return vswhereResult + "\\MSBuild\\15.0\\Bin\\MSBuild.exe";

                return null;
            }
            else
            {
                VisualStudioInstance instance = FindVisualStudioInstance();

                if (instance != null)
                {
                    if (File.Exists(instance.Path + "\\MSBuild\\15.0\\Bin\\MSBuild.exe"))
                        return instance.Path + "\\MSBuild\\15.0\\Bin\\MSBuild.exe";
                }

                return null;
            }
        }

        private static VisualStudioInstance FindVisualStudioInstance()
        {
            var setupConfiguration = new SetupConfiguration() as ISetupConfiguration2;
            var instanceEnumerator = setupConfiguration.EnumAllInstances();
            var instances = new ISetupInstance2[3];

            instanceEnumerator.Next(instances.Length, instances, out var instancesFetched);

            if (instancesFetched == 0)
                return null;

            do
            {
                for (int i = 0; i < instancesFetched; i++)
                {
                    var instance = new VisualStudioInstance(instances[i]);                            
                    var state = instances[i].GetState();                                                  
                    var packages = instances[i].GetPackages().Where(package => package.GetId().Contains("Microsoft.Component.MSBuild"));

                    if (
                        state.HasFlag(InstanceState.Local | InstanceState.Registered | InstanceState.Complete) &&
                        packages.Count() > 0 &&
                        instance.Version.StartsWith("15.0", StringComparison.OrdinalIgnoreCase)
                        )
                    {
                        return instance;
                    }
                }

                instanceEnumerator.Next(instances.Length, instances, out instancesFetched);
            }
            while (instancesFetched != 0);

            return null;
        }
    }


    public class VisualStudioInstance
    {
        public bool IsLaunchable { get; }
        public bool IsComplete { get; }
        public string Name { get; }
        public string Path { get; }
        public string Version { get; }
        public string DisplayName { get; }
        public string Description { get; }
        public string ResolvePath { get; }
        public string EnginePath { get; }
        public string ProductPath { get; }
        public string InstanceId { get; }
        public DateTime InstallDate { get; }

        public VisualStudioInstance(ISetupInstance2 FromInstance)
        {
            this.IsLaunchable = FromInstance.IsLaunchable();
            this.IsComplete = FromInstance.IsComplete();
            this.Name = FromInstance.GetInstallationName();
            this.Path = FromInstance.GetInstallationPath();
            this.Version = FromInstance.GetInstallationVersion();
            this.DisplayName = FromInstance.GetDisplayName();
            this.Description = FromInstance.GetDescription();
            this.ResolvePath = FromInstance.ResolvePath();
            this.EnginePath = FromInstance.GetEnginePath();
            this.InstanceId = FromInstance.GetInstanceId();
            this.ProductPath = FromInstance.GetProductPath();

            try
            {
                var time = FromInstance.GetInstallDate();
                ulong high = (ulong)time.dwHighDateTime;
                uint low = (uint)time.dwLowDateTime;
                long fileTime = (long)((high << 32) + low);

                this.InstallDate = DateTime.FromFileTimeUtc(fileTime);
            }
            catch
            {
                this.InstallDate = DateTime.UtcNow;
            }

            // FromInstance.GetState();
            // FromInstance.GetPackages();
            // FromInstance.GetProduct();            
            // FromInstance.GetProperties();
            // FromInstance.GetErrors();
        }
    }
}
