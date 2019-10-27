namespace GenerateZw
{
    public static class Program
    {
        public static void Main(string[] args)
        {
            ZwGen gen = new ZwGen();
            gen.LoadConfig(args.Length > 0 ? args[0] : "options.txt");
            gen.Execute();
        }
    }
}
