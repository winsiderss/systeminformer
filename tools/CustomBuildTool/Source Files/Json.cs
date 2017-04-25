using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization.Json;
using System.Text;
using System.Threading.Tasks;

namespace CustomBuildTool
{
    public static class Json<T> where T : class
    {
        public static string Serialize(T instance)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(T));

            using (MemoryStream stream = new MemoryStream())
            {
                serializer.WriteObject(stream, instance);

                return Encoding.Default.GetString(stream.ToArray());
            }
        }

        public static T DeSerialize(string json)
        {
            using (MemoryStream stream = new MemoryStream(Encoding.Default.GetBytes(json)))
            {
                DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(T));

                return (T)serializer.ReadObject(stream);
            }
        }
    }
}
