namespace CustomBuildTool
{
    /// <summary>
    /// Contains configuration for timestamping authenticode signatures.
    /// </summary>
    public struct TimeStampConfiguration
    {
        /// <summary>
        /// The URL to the timestamp authority.
        /// </summary>
        public string Url { get; }

        /// <summary>
        /// The digest algorithm the timestamp service authority should use on timestamp signatures.
        /// </summary>
        //public HashAlgorithmName DigestAlgorithm { get; }

        /// <summary>
        /// The type of timestamp to use. See <see cref="TimeStampType" /> for details, or null if
        /// no timestamping should be performed.
        /// </summary>
        public TimeStampType? Type { get; }

        /// <summary>
        /// A default timestamp configuration indicating no timestamp should be generated.
        /// </summary>
        public static TimeStampConfiguration None { get; } = new TimeStampConfiguration();

        /// <summary>
        /// Creates a new instance of a <see cref="TimeStampConfiguration" />.
        /// </summary>
        /// <param name="url">The URL to the timestamp authority.</param>
        /// <param name="digestAlgorithm">The digest algorithm the timestamp service authority should use on timestamp signatures.</param>
        /// <param name="type">The type of timestamp to use. See <see cref="TimeStampType" /> for details.</param>
        public TimeStampConfiguration(string url, TimeStampType type)
        {
            Url = url;
            Type = type;
        }

        public TimeStampConfiguration()
        {
            Type = null;
        }
    }

    /// <summary>
    /// An enumeration of possible timestamp kinds.
    /// </summary>
    public enum TimeStampType
    {
        /// <summary>
        /// Indicates that a timestamp authority should use the legacy Authenticode style of timestamps.
        /// This option should only be used for backward compatibility with Windows XP and only supports
        /// <see cref="HashAlgorithmName.SHA1" /> timestamp signatures.
        /// </summary>
        Authenticode,

        /// <summary>
        /// Indicates that a timestamp authority should use an RFC3161 timestamp signatures.
        /// </summary>
        RFC3161
    }
}