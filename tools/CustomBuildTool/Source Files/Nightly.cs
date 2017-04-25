using System.Runtime.Serialization;

namespace CustomBuildTool
{
    [DataContract]
    public class BuildUpdateRequest
    {
        [DataMember(Name = "updated")]
        public string Updated { get; set; }

        [DataMember(Name = "version")]
        public string Version { get; set; }

        [DataMember(Name = "size")]
        public string FileLength { get; set; }

        [DataMember(Name = "forum_url")]
        public string ForumUrl { get; set; }

        [DataMember(Name = "setup_url")]
        public string Setupurl { get; set; }

        [DataMember(Name = "bin_url")]
        public string Binurl { get; set; }

        [DataMember(Name = "hash_setup")]
        public string HashSetup { get; set; }

        [DataMember(Name = "hash_bin")]
        public string HashBin { get; set; }

        [DataMember(Name = "sig")]
        public string sig { get; set; }

        [DataMember(Name = "message")]
        public string Message { get; set; }
    }
}
