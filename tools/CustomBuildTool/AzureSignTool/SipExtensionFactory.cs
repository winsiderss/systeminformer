namespace CustomBuildTool
{
    public enum SipKind
    {
        None,
        Appx
    }

    public static class SipExtensionFactory
    {
        public static SipKind GetSipKind(ReadOnlySpan<char> filePath)
        {
            var extension = Path.GetExtension(filePath.ToString());

            if (extension.Equals(".appx", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            if (extension.Equals(".eappx", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            if (extension.Equals(".appxbundle", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            if (extension.Equals(".eappxbundle", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            if (extension.Equals(".msix", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            if (extension.Equals(".emsix", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            if (extension.Equals(".msixbundle", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            if (extension.Equals(".emsixbundle", StringComparison.OrdinalIgnoreCase))
            {
                return SipKind.Appx;
            }

            return SipKind.None;
        }
    }
}
