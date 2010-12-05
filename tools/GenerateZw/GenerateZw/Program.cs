using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace GenerateZw
{
    class Program
    {
        static void Main(string[] args)
        {
            ZwGen gen = new ZwGen();
            string configFile = args.Length > 0 ? args[0] : "options.txt";

            try
            {
                gen.LoadConfig(configFile);
                gen.Execute();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }
    }
}
