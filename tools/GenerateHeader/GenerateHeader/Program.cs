using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GenerateHeader
{
    class Program
    {
        static void Main(string[] args)
        {
            HeaderGen gen = new HeaderGen();
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
