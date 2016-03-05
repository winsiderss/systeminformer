#ifndef _COMMONUTIL_H
#define _COMMONUTIL_H

#ifdef _DEBUG
#define DEBUG_MSG(Format, ...) \
{ \
    PPH_STRING debugString = PhFormatString(Format, __VA_ARGS__); \
    if (debugString) \
    {                \
        OutputDebugString(debugString->Buffer); \
        PhDereferenceObject(debugString); \
    } \
}
#else
#define DEBUG_MSG(Format, ...)
#endif

FORCEINLINE
HFONT 
CommonDuplicateFont(
    _In_ HFONT Font
    )
{
    LOGFONT logFont;

    if (GetObject(Font, sizeof(LOGFONT), &logFont))
        return CreateFontIndirect(&logFont);
    else
        return NULL;
}

#endif