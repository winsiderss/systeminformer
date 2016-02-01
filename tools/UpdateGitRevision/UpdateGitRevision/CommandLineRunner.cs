using System.Diagnostics;

namespace UpdateRevision
{
    public class CommandLineRunner
    {
        public string Result { get; set; }

        public bool RunCommandAndGetOutput(string command, string arguments, string workingFolder)
        {
            var p = new Process();
            p.StartInfo.FileName = command;
            p.StartInfo.Arguments = arguments;
            p.StartInfo.WorkingDirectory = workingFolder;
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardInput = true;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.CreateNoWindow = true;
            p.OutputDataReceived += OutputDataReceived;

            try
            {
                p.Start();
            }
            catch
            {
                return false;
            }
            p.BeginOutputReadLine(); // Async reading of output to prevent deadlock
            if (p.WaitForExit(5000))
            {
                return p.ExitCode == 0;
            }
            return false;
        }

        private void OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (e != null && e.Data != null)
                Result = e.Data;
        }
    }
}
