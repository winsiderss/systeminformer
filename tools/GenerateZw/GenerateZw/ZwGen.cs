using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace GenerateZw
{
    class ZwGen
    {
        private string _baseDirectory;
        private string[] _files;

        public void LoadConfig(string fileName)
        {
            string[] lines = File.ReadAllLines(fileName);

            foreach (string line in lines)
            {
                string[] split = line.Split(new char[] { '=' }, 2);

                switch (split[0])
                {
                    case "base":
                        _baseDirectory = split[1];
                        break;
                    case "in":
                        _files = split[1].Split(';');
                        break;
                }
            }
        }

        public void Execute()
        {
            //// Build up a list of definitions.

            foreach (string fileName in _files)
            {
                string currentFilePath = _baseDirectory + "\\" + fileName;

                string text = File.ReadAllText(currentFilePath);

                Regex regex = new Regex(@"NTSYSCALLAPI[\w\s_]*NTAPI\s*(Nt(\w)*)\(.*?\);", RegexOptions.Compiled | RegexOptions.Singleline);
                MatchCollection matches;

                matches = regex.Matches(text);

                foreach (Match match in matches)
                {
                    string currentName = match.Groups[1].Value;
                    string currentText = match.Value;
                    int currentNameIndex = match.Groups[1].Index - match.Index;

                    string newText = currentText + "\r\n\r\n" + currentText.Substring(0, currentNameIndex) + "Zw" + currentText.Substring(currentNameIndex + 2);

                    // Make sure we don't add definitions repeatedly.
                    if (text.IndexOf(newText) == -1)
                    {
                        text = text.Replace(currentText, newText);
                    }
                }

                File.WriteAllText(currentFilePath, text);

            }



        }
    }
}
