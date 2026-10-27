// IIDs for every 7-Zip interface are instantiated in this translation unit
// (MyInitGuid.h must precede the first interface declaration).
#include "CPP/Common/MyInitGuid.h"

#include "CPP/7zip/ICoder.h"
#include "CPP/7zip/IPassword.h"
#include "CPP/7zip/IProgress.h"
#include "CPP/7zip/IStream.h"
#include "CPP/7zip/Archive/IArchive.h"

#include "CPP/Common/MyCom.h"
#include "CPP/Windows/PropVariant.h"
#include "CPP/7zip/Common/RegisterCodec.h"
#include "CPP/7zip/Common/StreamObjects.h"
#include "CPP/7zip/Archive/7z/7zHandler.h"
#include "CPP/7zip/Archive/Zip/ZipHandler.h"
#include "CPP/7zip/Compress/Bcj2Coder.h"
#include "CPP/7zip/Compress/BcjCoder.h"
#include "CPP/7zip/Compress/BranchMisc.h"
#include "CPP/7zip/Compress/BZip2Decoder.h"
#include "CPP/7zip/Compress/CopyCoder.h"
#include "CPP/7zip/Compress/DeflateDecoder.h"
#include "CPP/7zip/Compress/Lzma2Decoder.h"
#include "CPP/7zip/Compress/LzmaDecoder.h"
#include "CPP/7zip/Compress/PpmdDecoder.h"
#include "CPP/7zip/Crypto/7zAes.h"

#include "si_7zip.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#define SI7Z_ZIP_LOCAL_FILE_HEADER_SIGNATURE 0x04034b50u
#define SI7Z_ZIP_CENTRAL_DIRECTORY_SIGNATURE 0x02014b50u
#define SI7Z_ZIP_END_OF_CENTRAL_DIRECTORY_SIGNATURE 0x06054b50u
#define SI7Z_ZIP_VERSION_NEEDED 20u
#define SI7Z_ZIP_METHOD_STORE 0u

typedef struct _SI7Z_BUFFER
{
    unsigned char* Data;
    size_t Size;
    size_t Capacity;
} SI7Z_BUFFER;

static int Si7zCompareExtension(const wchar_t* left, const wchar_t* right)
{
#ifdef _WIN32
    return _wcsicmp(left, right);
#else
    while (*left && *right)
    {
        wchar_t a = towlower(*left++);
        wchar_t b = towlower(*right++);
        if (a != b)
            return (int)(a - b);
    }
    return (int)(*left - *right);
#endif
}

static SI7Z_FORMAT Si7zNormalizeFormatFromPath(const wchar_t* archivePath, const SI7Z_OPTIONS* options)
{
    if (options && options->Format != SI7Z_FORMAT_AUTO)
        return options->Format;

    if (!archivePath)
        return SI7Z_FORMAT_AUTO;

    const wchar_t* ext = wcsrchr(archivePath, L'.');
    if (!ext)
        return SI7Z_FORMAT_AUTO;

    if (Si7zCompareExtension(ext, L".7z") == 0)
        return SI7Z_FORMAT_7Z;
    if (Si7zCompareExtension(ext, L".zip") == 0 || Si7zCompareExtension(ext, L".zipx") == 0)
        return SI7Z_FORMAT_ZIP;

    return SI7Z_FORMAT_AUTO;
}

static SI7Z_FORMAT Si7zNormalizeFormatForBuffer(const SI7Z_OPTIONS* options)
{
    if (options && options->Format != SI7Z_FORMAT_AUTO)
        return options->Format;

    return SI7Z_FORMAT_ZIP;
}

static SI7Z_RESULT Si7zValidateMethod(SI7Z_FORMAT format, SI7Z_METHOD method)
{
    if (method == SI7Z_METHOD_DEFAULT)
        return format == SI7Z_FORMAT_AUTO ? SI7Z_ERROR_UNSUPPORTED_FORMAT : SI7Z_OK;

    if (format == SI7Z_FORMAT_7Z)
    {
        switch (method)
        {
        case SI7Z_METHOD_COPY:
        case SI7Z_METHOD_LZMA:
        case SI7Z_METHOD_LZMA2:
        case SI7Z_METHOD_PPMD:
        case SI7Z_METHOD_BZIP2:
            return SI7Z_OK;
        default:
            return SI7Z_ERROR_UNSUPPORTED_METHOD;
        }
    }

    if (format == SI7Z_FORMAT_ZIP)
    {
        switch (method)
        {
        case SI7Z_METHOD_COPY:
        case SI7Z_METHOD_DEFLATE:
        case SI7Z_METHOD_DEFLATE64:
        case SI7Z_METHOD_BZIP2:
        case SI7Z_METHOD_LZMA:
        case SI7Z_METHOD_XZ:
        case SI7Z_METHOD_PPMD:
        case SI7Z_METHOD_ZSTD:
            return SI7Z_OK;
        default:
            return SI7Z_ERROR_UNSUPPORTED_METHOD;
        }
    }

    return SI7Z_ERROR_UNSUPPORTED_FORMAT;
}

static void Si7zWriteUInt16(unsigned char* data, uint16_t value)
{
    data[0] = (unsigned char)value;
    data[1] = (unsigned char)(value >> 8);
}

static void Si7zWriteUInt32(unsigned char* data, uint32_t value)
{
    data[0] = (unsigned char)value;
    data[1] = (unsigned char)(value >> 8);
    data[2] = (unsigned char)(value >> 16);
    data[3] = (unsigned char)(value >> 24);
}

static void Si7zCrc32Init(uint32_t table[256])
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(crc & 1));
        table[i] = crc;
    }
}

static uint32_t Si7zCrc32(const void* data, size_t size)
{
    uint32_t table[256];
    uint32_t crc = 0xffffffffu;
    const unsigned char* bytes = (const unsigned char*)data;

    Si7zCrc32Init(table);

    for (size_t i = 0; i < size; i++)
        crc = table[(crc ^ bytes[i]) & 0xffu] ^ (crc >> 8);

    return crc ^ 0xffffffffu;
}

static int Si7zBufferReserve(SI7Z_BUFFER* buffer, size_t required)
{
    if (required <= buffer->Capacity)
        return 1;

    size_t capacity = buffer->Capacity ? buffer->Capacity : 256;
    while (capacity < required)
    {
        if (capacity > ((size_t)-1) / 2)
            return 0;
        capacity *= 2;
    }

    unsigned char* data = (unsigned char*)realloc(buffer->Data, capacity);
    if (!data)
        return 0;

    buffer->Data = data;
    buffer->Capacity = capacity;
    return 1;
}

static int Si7zBufferAppend(SI7Z_BUFFER* buffer, const void* data, size_t size)
{
    if (!size)
        return 1;

    if (size > (size_t)-1 - buffer->Size)
        return 0;

    if (!Si7zBufferReserve(buffer, buffer->Size + size))
        return 0;

    memcpy(buffer->Data + buffer->Size, data, size);
    buffer->Size += size;
    return 1;
}

static int Si7zBufferAppendZeros(SI7Z_BUFFER* buffer, size_t size)
{
    if (size > (size_t)-1 - buffer->Size)
        return 0;

    if (!Si7zBufferReserve(buffer, buffer->Size + size))
        return 0;

    memset(buffer->Data + buffer->Size, 0, size);
    buffer->Size += size;
    return 1;
}

static char* Si7zWideToUtf8(const wchar_t* string, size_t* length)
{
    if (!string)
        string = L"data";

#ifdef _WIN32
    int required = WideCharToMultiByte(CP_UTF8, 0, string, -1, NULL, 0, NULL, NULL);
    if (required <= 0)
        return NULL;

    char* result = (char*)malloc((size_t)required);
    if (!result)
        return NULL;

    if (!WideCharToMultiByte(CP_UTF8, 0, string, -1, result, required, NULL, NULL))
    {
        free(result);
        return NULL;
    }

    *length = (size_t)required - 1;
    return result;
#else
    size_t required = wcstombs(NULL, string, 0);
    if (required == (size_t)-1)
        return NULL;

    char* result = (char*)malloc(required + 1);
    if (!result)
        return NULL;

    wcstombs(result, string, required + 1);
    *length = required;
    return result;
#endif
}

static SI7Z_RESULT Si7zValidateBufferZipOptions(const SI7Z_OPTIONS* options)
{
    SI7Z_FORMAT format = Si7zNormalizeFormatForBuffer(options);
    SI7Z_METHOD method = options ? options->Method : SI7Z_METHOD_DEFAULT;
    SI7Z_RESULT result = Si7zValidateMethod(format, method);

    if (options && options->Password && options->Password[0])
        return SI7Z_ERROR_NOT_IMPLEMENTED;

    if (result != SI7Z_OK)
        return result;
    if (format != SI7Z_FORMAT_ZIP)
        return SI7Z_ERROR_UNSUPPORTED_FORMAT;
    if (method != SI7Z_METHOD_DEFAULT && method != SI7Z_METHOD_COPY)
        return SI7Z_ERROR_UNSUPPORTED_METHOD;

    return SI7Z_OK;
}

//
// Decoder registration.
//
// The vendored *Register.cpp files register codecs from static constructors.
// When this code is linked from a static library those objects are never
// referenced, so the linker drops them and CreateCoder_Id() cannot find the
// method. Register the decoders the 7z and ZIP handlers need from here
// instead, skipping any method id that is already present.
//

extern unsigned g_NumCodecs;
extern const CCodecInfo* g_Codecs[];

static void* Si7zCreateCopyDecoder() { return (void*)(ICompressCoder*)(new NCompress::CCopyCoder); }
static void* Si7zCreateDeflateDecoder() { return (void*)(ICompressCoder*)(new NCompress::NDeflate::NDecoder::CCOMCoder); }
static void* Si7zCreateDeflate64Decoder() { return (void*)(ICompressCoder*)(new NCompress::NDeflate::NDecoder::CCOMCoder64); }
static void* Si7zCreateBZip2Decoder() { return (void*)(ICompressCoder*)(new NCompress::NBZip2::CDecoder); }
static void* Si7zCreateLzmaDecoder() { return (void*)(ICompressCoder*)(new NCompress::NLzma::CDecoder); }
static void* Si7zCreateLzma2Decoder() { return (void*)(ICompressCoder*)(new NCompress::NLzma2::CDecoder); }
static void* Si7zCreatePpmdDecoder() { return (void*)(ICompressCoder*)(new NCompress::NPpmd::CDecoder); }
static void* Si7zCreateBcj2Decoder() { return (void*)(ICompressCoder2*)(new NCompress::NBcj2::CDecoder); }
static void* Si7zCreateBcjDecoder() { return (void*)(ICompressFilter*)(new NCompress::NBcj::CCoder2(z7_BranchConvSt_X86_Dec)); }
static void* Si7zCreateArmDecoder() { return (void*)(ICompressFilter*)(new NCompress::NBranch::CDecoder(Z7_BRANCH_CONV_DEC(ARM), 4)); }
static void* Si7zCreateArm64Decoder() { return (void*)(ICompressFilter*)(new NCompress::NBranch::CDecoder(Z7_BRANCH_CONV_DEC(ARM64), 4)); }
static void* Si7zCreate7zAesDecoder() { return (void*)(ICompressFilter*)(new NCrypto::N7z::CDecoder); }

static const CCodecInfo Si7zDecoders[] =
{
    { Si7zCreateCopyDecoder, NULL, 0x00, "Copy", 1, false },
    { Si7zCreateDeflateDecoder, NULL, 0x40108, "Deflate", 1, false },
    { Si7zCreateDeflate64Decoder, NULL, 0x40109, "Deflate64", 1, false },
    { Si7zCreateBZip2Decoder, NULL, 0x40202, "BZip2", 1, false },
    { Si7zCreateLzmaDecoder, NULL, 0x30101, "LZMA", 1, false },
    { Si7zCreateLzma2Decoder, NULL, 0x21, "LZMA2", 1, false },
    { Si7zCreatePpmdDecoder, NULL, 0x30401, "PPMD", 1, false },
    { Si7zCreateBcj2Decoder, NULL, 0x303011B, "BCJ2", 4, false },
    { Si7zCreateBcjDecoder, NULL, 0x3030103, "BCJ", 1, true },
    { Si7zCreateArmDecoder, NULL, 0x3030501, "ARM", 1, true },
    { Si7zCreateArm64Decoder, NULL, 0xA, "ARM64", 1, true },
    { Si7zCreate7zAesDecoder, NULL, 0x6F10701, "7zAES", 1, true },
};

static bool Si7zRegisterDecoders()
{
    for (size_t i = 0; i < Z7_ARRAY_SIZE(Si7zDecoders); i++)
    {
        bool registered = false;

        for (unsigned j = 0; j < g_NumCodecs; j++)
        {
            if (g_Codecs[j]->Id == Si7zDecoders[i].Id)
            {
                registered = true;
                break;
            }
        }

        if (!registered)
            RegisterCodec(&Si7zDecoders[i]);
    }

    return true;
}

static void Si7zEnsureDecoders()
{
    static const bool registered = Si7zRegisterDecoders();
    (void)registered;
}

//
// Archive reading.
//

static const unsigned char Si7zSignature7z[] = { '7', 'z', 0xBC, 0xAF, 0x27, 0x1C };

static HRESULT Si7zGetPassword(const wchar_t* Password, BSTR* password)
{
    *password = NULL;

    if (!Password || !Password[0])
        return E_ABORT;

    *password = SysAllocString(Password);

    return *password ? S_OK : E_OUTOFMEMORY;
}

Z7_CLASS_IMP_COM_2(
    CSi7zOpenCallback
    , IArchiveOpenCallback
    , ICryptoGetTextPassword
)
public:
    const wchar_t* Password;

    CSi7zOpenCallback() : Password(NULL) {}
};

Z7_COM7F_IMF(CSi7zOpenCallback::SetTotal(const UInt64*, const UInt64*))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zOpenCallback::SetCompleted(const UInt64*, const UInt64*))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zOpenCallback::CryptoGetTextPassword(BSTR* password))
{
    return Si7zGetPassword(Password, password);
}

Z7_CLASS_IMP_COM_2(
    CSi7zExtractCallback
    , IArchiveExtractCallback
    , ICryptoGetTextPassword
)
    Z7_IFACE_COM7_IMP(IProgress)
public:
    UInt32 Index;
    const wchar_t* Password;
    CDynBufSeqOutStream* OutStreamSpec;
    CMyComPtr<ISequentialOutStream> OutStream;
    Int32 OperationResult;
    bool Completed;

    CSi7zExtractCallback() : Index(0), Password(NULL), OutStreamSpec(NULL), OperationResult(0), Completed(false) {}
};

Z7_COM7F_IMF(CSi7zExtractCallback::SetTotal(UInt64))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zExtractCallback::SetCompleted(const UInt64*))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zExtractCallback::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode))
{
    *outStream = NULL;

    if (index != Index || askExtractMode != NArchive::NExtract::NAskMode::kExtract)
        return S_OK;

    if (!OutStreamSpec)
    {
        OutStreamSpec = new CDynBufSeqOutStream;
        OutStream = OutStreamSpec;
    }

    OutStreamSpec->Init();

    CMyComPtr<ISequentialOutStream> stream = OutStream;
    *outStream = stream.Detach();

    return S_OK;
}

Z7_COM7F_IMF(CSi7zExtractCallback::PrepareOperation(Int32))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zExtractCallback::SetOperationResult(Int32 opRes))
{
    OperationResult = opRes;
    Completed = true;
    return S_OK;
}

Z7_COM7F_IMF(CSi7zExtractCallback::CryptoGetTextPassword(BSTR* password))
{
    return Si7zGetPassword(Password, password);
}

static SI7Z_RESULT Si7zResultFromHresult(HRESULT hr)
{
    switch (hr)
    {
    case S_OK:
        return SI7Z_OK;
    case E_OUTOFMEMORY:
        return SI7Z_ERROR_OUT_OF_MEMORY;
    case E_NOTIMPL:
        return SI7Z_ERROR_UNSUPPORTED_METHOD;
    case E_INVALIDARG:
        return SI7Z_ERROR_INVALID_ARGUMENT;
    case E_ABORT: // returned by the callbacks when a password is required but none was supplied
        return SI7Z_ERROR_DATA_ERROR;
    default:
        return SI7Z_ERROR_ARCHIVE_FAILED;
    }
}

static SI7Z_FORMAT Si7zDetectBufferFormat(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options)
{
    const unsigned char* data = (const unsigned char*)archiveData;

    if (options && options->Format != SI7Z_FORMAT_AUTO)
        return options->Format;

    if (archiveSize >= sizeof(Si7zSignature7z) && memcmp(data, Si7zSignature7z, sizeof(Si7zSignature7z)) == 0)
        return SI7Z_FORMAT_7Z;
    if (archiveSize >= 4 && data[0] == 'P' && data[1] == 'K' &&
        ((data[2] == 3 && data[3] == 4) || (data[2] == 5 && data[3] == 6)))
        return SI7Z_FORMAT_ZIP;

    return SI7Z_FORMAT_AUTO;
}

// Opens archiveData with the 7-Zip handler for its format. The buffer is not
// copied and must stay valid while *archive is in use.
static SI7Z_RESULT Si7zOpenBuffer(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, CMyComPtr<IInArchive>& archive)
{
    CMyComPtr<IInArchive> handler;

    switch (Si7zDetectBufferFormat(archiveData, archiveSize, options))
    {
    case SI7Z_FORMAT_7Z:
        handler = new NArchive::N7z::CHandler;
        break;
    case SI7Z_FORMAT_ZIP:
        handler = new NArchive::NZip::CHandler;
        break;
    default:
        return SI7Z_ERROR_UNSUPPORTED_FORMAT;
    }

    CBufInStream* streamSpec = new CBufInStream;
    CMyComPtr<IInStream> stream = streamSpec;
    streamSpec->Init((const Byte*)archiveData, archiveSize);

    CSi7zOpenCallback* openCallbackSpec = new CSi7zOpenCallback;
    CMyComPtr<IArchiveOpenCallback> openCallback = openCallbackSpec;
    openCallbackSpec->Password = options ? options->Password : NULL;

    const UInt64 maxCheckStartPosition = 0;
    const HRESULT hr = handler->Open(stream, &maxCheckStartPosition, openCallback);

    if (hr == E_OUTOFMEMORY)
        return SI7Z_ERROR_OUT_OF_MEMORY;
    if (hr == E_ABORT) // encrypted headers and no password
        return SI7Z_ERROR_DATA_ERROR;
    if (hr != S_OK)
        return SI7Z_ERROR_OPEN_FAILED;

    archive = handler;
    return SI7Z_OK;
}

static HRESULT Si7zGetItemPath(IInArchive* archive, UInt32 index, UString& path)
{
    NWindows::NCOM::CPropVariant prop;

    path.Empty();

    RINOK(archive->GetProperty(index, kpidPath, &prop))

    if (prop.vt == VT_BSTR)
        path = prop.bstrVal;
    else if (prop.vt != VT_EMPTY)
        return E_FAIL;

    return S_OK;
}

static HRESULT Si7zGetItemBool(IInArchive* archive, UInt32 index, PROPID propId, bool& value)
{
    NWindows::NCOM::CPropVariant prop;

    value = false;

    RINOK(archive->GetProperty(index, propId, &prop))

    if (prop.vt == VT_BOOL)
        value = prop.boolVal != VARIANT_FALSE;
    else if (prop.vt != VT_EMPTY)
        return E_FAIL;

    return S_OK;
}

static HRESULT Si7zGetItemUInt64(IInArchive* archive, UInt32 index, PROPID propId, UInt64& value)
{
    NWindows::NCOM::CPropVariant prop;

    value = 0;

    RINOK(archive->GetProperty(index, propId, &prop))

    switch (prop.vt)
    {
    case VT_UI8:
        value = prop.uhVal.QuadPart;
        break;
    case VT_UI4:
        value = prop.ulVal;
        break;
    case VT_EMPTY:
        break;
    default:
        return E_FAIL;
    }

    return S_OK;
}

// Maps a 7-Zip method description ("LZMA2:24", "BCJ LZMA2:24", "AES-256 Deflate")
// onto SI7Z_METHOD. Filters and encryption are skipped.
static SI7Z_METHOD Si7zMethodFromString(const wchar_t* method)
{
    static const struct
    {
        const wchar_t* Name;
        SI7Z_METHOD Method;
    } methods[] =
    {
        { L"Copy", SI7Z_METHOD_COPY },
        { L"Store", SI7Z_METHOD_COPY },
        { L"Deflate", SI7Z_METHOD_DEFLATE },
        { L"Deflate64", SI7Z_METHOD_DEFLATE64 },
        { L"BZip2", SI7Z_METHOD_BZIP2 },
        { L"LZMA", SI7Z_METHOD_LZMA },
        { L"LZMA2", SI7Z_METHOD_LZMA2 },
        { L"xz", SI7Z_METHOD_XZ },
        { L"PPMd", SI7Z_METHOD_PPMD },
        { L"PPMD", SI7Z_METHOD_PPMD },
        { L"Zstd", SI7Z_METHOD_ZSTD },
        { L"ZSTD", SI7Z_METHOD_ZSTD },
    };

    while (method && *method)
    {
        size_t length;
        size_t nameLength;

        while (*method == L' ')
            method++;

        length = wcscspn(method, L" ");
        nameLength = wcscspn(method, L": ");

        if (nameLength > length)
            nameLength = length;

        for (size_t i = 0; i < Z7_ARRAY_SIZE(methods); i++)
        {
            if (wcslen(methods[i].Name) == nameLength && _wcsnicmp(method, methods[i].Name, nameLength) == 0)
                return methods[i].Method;
        }

        method += length;
    }

    return SI7Z_METHOD_DEFAULT;
}

static HRESULT Si7zGetItemMethod(IInArchive* archive, UInt32 index, uint32_t& value)
{
    NWindows::NCOM::CPropVariant prop;

    value = SI7Z_METHOD_DEFAULT;

    RINOK(archive->GetProperty(index, kpidMethod, &prop))

    if (prop.vt == VT_BSTR)
        value = (uint32_t)Si7zMethodFromString(prop.bstrVal);
    else if (prop.vt != VT_EMPTY)
        return E_FAIL;

    return S_OK;
}

// Compares two archive paths case-insensitively, treating '/' and '\' as equal.
static bool Si7zEqualItemPath(const wchar_t* left, const wchar_t* right)
{
    for (;; left++, right++)
    {
        wchar_t a = *left == L'/' ? L'\\' : (wchar_t)towlower(*left);
        wchar_t b = *right == L'/' ? L'\\' : (wchar_t)towlower(*right);

        if (a != b)
            return false;
        if (!a)
            return true;
    }
}

// Finds the item to extract. With itemName == NULL the archive must contain
// exactly one file, otherwise the file whose path matches itemName is used.
static SI7Z_RESULT Si7zFindItem(IInArchive* archive, const wchar_t* itemName, UInt32& index)
{
    UInt32 numItems = 0;
    bool found = false;

    if (archive->GetNumberOfItems(&numItems) != S_OK)
        return SI7Z_ERROR_ARCHIVE_FAILED;

    for (UInt32 i = 0; i < numItems; i++)
    {
        bool isDir;

        if (Si7zGetItemBool(archive, i, kpidIsDir, isDir) != S_OK)
            return SI7Z_ERROR_ARCHIVE_FAILED;
        if (isDir)
            continue;

        if (itemName)
        {
            UString path;

            if (Si7zGetItemPath(archive, i, path) != S_OK)
                return SI7Z_ERROR_ARCHIVE_FAILED;

            if (Si7zEqualItemPath(path, itemName))
            {
                index = i;
                return SI7Z_OK;
            }
        }
        else
        {
            if (found)
                return SI7Z_ERROR_INVALID_ARGUMENT; // more than one file: the caller must name one

            index = i;
            found = true;
        }
    }

    return found ? SI7Z_OK : SI7Z_ERROR_ITEM_NOT_FOUND;
}

static SI7Z_RESULT Si7zExtractItem(IInArchive* archive, UInt32 index, const SI7Z_OPTIONS* options, void** outputData, size_t* outputSize)
{
    CSi7zExtractCallback* callbackSpec = new CSi7zExtractCallback;
    CMyComPtr<IArchiveExtractCallback> callback = callbackSpec;

    callbackSpec->Index = index;
    callbackSpec->Password = options ? options->Password : NULL;

    const HRESULT hr = archive->Extract(&index, 1, 0, callback);

    if (hr != S_OK)
        return Si7zResultFromHresult(hr);
    if (!callbackSpec->Completed)
        return SI7Z_ERROR_ARCHIVE_FAILED;

    switch (callbackSpec->OperationResult)
    {
    case NArchive::NExtract::NOperationResult::kOK:
        break;
    case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
        return SI7Z_ERROR_UNSUPPORTED_METHOD;
    default:
        return SI7Z_ERROR_DATA_ERROR;
    }

    const size_t size = callbackSpec->OutStreamSpec ? callbackSpec->OutStreamSpec->GetSize() : 0;
    void* output = malloc(size ? size : 1);

    if (!output)
        return SI7Z_ERROR_OUT_OF_MEMORY;

    if (size)
        memcpy(output, callbackSpec->OutStreamSpec->GetBuffer(), size);

    *outputData = output;
    *outputSize = size;
    return SI7Z_OK;
}

static SI7Z_RESULT Si7zValidateReadOptions(const SI7Z_OPTIONS* options)
{
    // The method is a compression setting; when reading it comes from the archive.
    if (options && options->Format != SI7Z_FORMAT_AUTO && options->Format != SI7Z_FORMAT_7Z && options->Format != SI7Z_FORMAT_ZIP)
        return SI7Z_ERROR_UNSUPPORTED_FORMAT;

    return SI7Z_OK;
}

static void Si7zFreeItems(SI7Z_ITEM* items, uint32_t count)
{
    if (!items)
        return;

    for (uint32_t i = 0; i < count; i++)
        free(items[i].Name);

    free(items);
}

SI7Z_RESULT SI7ZAPI Si7zCompressArchive(const wchar_t* archivePath, const wchar_t* sourcePath, const SI7Z_OPTIONS* options)
{
    if (!archivePath || !sourcePath)
        return SI7Z_ERROR_INVALID_ARGUMENT;

    const SI7Z_FORMAT format = Si7zNormalizeFormatFromPath(archivePath, options);
    const SI7Z_METHOD method = options ? options->Method : SI7Z_METHOD_DEFAULT;
    SI7Z_RESULT result = Si7zValidateMethod(format, method);
    if (result != SI7Z_OK)
        return result;

    return SI7Z_ERROR_NOT_IMPLEMENTED;
}

SI7Z_RESULT SI7ZAPI Si7zExtractArchive(const wchar_t* archivePath, const wchar_t* destinationPath, const SI7Z_OPTIONS* options)
{
    if (!archivePath || !destinationPath)
        return SI7Z_ERROR_INVALID_ARGUMENT;

    const SI7Z_FORMAT format = Si7zNormalizeFormatFromPath(archivePath, options);
    SI7Z_RESULT result = Si7zValidateMethod(format, options ? options->Method : SI7Z_METHOD_DEFAULT);
    if (result != SI7Z_OK)
        return result;

    return SI7Z_ERROR_NOT_IMPLEMENTED;
}

SI7Z_RESULT SI7ZAPI Si7zListArchive(const wchar_t* archivePath, const SI7Z_OPTIONS* options, SI7Z_LIST* list)
{
    if (!archivePath || !list)
        return SI7Z_ERROR_INVALID_ARGUMENT;

    list->Items = NULL;
    list->Count = 0;

    const SI7Z_FORMAT format = Si7zNormalizeFormatFromPath(archivePath, options);
    SI7Z_RESULT result = Si7zValidateMethod(format, options ? options->Method : SI7Z_METHOD_DEFAULT);
    if (result != SI7Z_OK)
        return result;

    return SI7Z_ERROR_NOT_IMPLEMENTED;
}

SI7Z_RESULT SI7ZAPI Si7zCompressBuffer(const void* sourceData, size_t sourceSize, const wchar_t* itemName, const SI7Z_OPTIONS* options, void** archiveData, size_t* archiveSize)
{
    if ((!sourceData && sourceSize) || !archiveData || !archiveSize)
        return SI7Z_ERROR_INVALID_ARGUMENT;

    *archiveData = NULL;
    *archiveSize = 0;

    SI7Z_RESULT result = Si7zValidateBufferZipOptions(options);
    if (result != SI7Z_OK)
        return result;
    if (sourceSize > UINT32_MAX)
        return SI7Z_ERROR_INVALID_ARGUMENT;

    size_t nameLength = 0;
    char* name = Si7zWideToUtf8(itemName, &nameLength);
    if (!name)
        return SI7Z_ERROR_OUT_OF_MEMORY;
    if (nameLength > UINT16_MAX)
    {
        free(name);
        return SI7Z_ERROR_INVALID_ARGUMENT;
    }

    const uint32_t crc = Si7zCrc32(sourceData, sourceSize);
    const uint32_t dataSize = (uint32_t)sourceSize;
    const uint16_t nameSize = (uint16_t)nameLength;
    SI7Z_BUFFER buffer = { 0 };

    const size_t localHeaderOffset = buffer.Size;
    if (!Si7zBufferAppendZeros(&buffer, 30) ||
        !Si7zBufferAppend(&buffer, name, nameLength) ||
        !Si7zBufferAppend(&buffer, sourceData, sourceSize))
    {
        free(name);
        free(buffer.Data);
        return SI7Z_ERROR_OUT_OF_MEMORY;
    }

    Si7zWriteUInt32(buffer.Data + localHeaderOffset, SI7Z_ZIP_LOCAL_FILE_HEADER_SIGNATURE);
    Si7zWriteUInt16(buffer.Data + localHeaderOffset + 4, SI7Z_ZIP_VERSION_NEEDED);
    Si7zWriteUInt16(buffer.Data + localHeaderOffset + 6, 0x0800u);
    Si7zWriteUInt16(buffer.Data + localHeaderOffset + 8, SI7Z_ZIP_METHOD_STORE);
    Si7zWriteUInt32(buffer.Data + localHeaderOffset + 14, crc);
    Si7zWriteUInt32(buffer.Data + localHeaderOffset + 18, dataSize);
    Si7zWriteUInt32(buffer.Data + localHeaderOffset + 22, dataSize);
    Si7zWriteUInt16(buffer.Data + localHeaderOffset + 26, nameSize);

    const size_t centralDirectoryOffset = buffer.Size;
    if (!Si7zBufferAppendZeros(&buffer, 46) ||
        !Si7zBufferAppend(&buffer, name, nameLength))
    {
        free(name);
        free(buffer.Data);
        return SI7Z_ERROR_OUT_OF_MEMORY;
    }

    Si7zWriteUInt32(buffer.Data + centralDirectoryOffset, SI7Z_ZIP_CENTRAL_DIRECTORY_SIGNATURE);
    Si7zWriteUInt16(buffer.Data + centralDirectoryOffset + 4, SI7Z_ZIP_VERSION_NEEDED);
    Si7zWriteUInt16(buffer.Data + centralDirectoryOffset + 6, SI7Z_ZIP_VERSION_NEEDED);
    Si7zWriteUInt16(buffer.Data + centralDirectoryOffset + 8, 0x0800u);
    Si7zWriteUInt16(buffer.Data + centralDirectoryOffset + 10, SI7Z_ZIP_METHOD_STORE);
    Si7zWriteUInt32(buffer.Data + centralDirectoryOffset + 16, crc);
    Si7zWriteUInt32(buffer.Data + centralDirectoryOffset + 20, dataSize);
    Si7zWriteUInt32(buffer.Data + centralDirectoryOffset + 24, dataSize);
    Si7zWriteUInt16(buffer.Data + centralDirectoryOffset + 28, nameSize);
    Si7zWriteUInt32(buffer.Data + centralDirectoryOffset + 42, (uint32_t)localHeaderOffset);

    const size_t centralDirectorySize = buffer.Size - centralDirectoryOffset;
    if (centralDirectoryOffset > UINT32_MAX || centralDirectorySize > UINT32_MAX)
    {
        free(name);
        free(buffer.Data);
        return SI7Z_ERROR_INVALID_ARGUMENT;
    }

    const size_t eocdOffset = buffer.Size;
    if (!Si7zBufferAppendZeros(&buffer, 22))
    {
        free(name);
        free(buffer.Data);
        return SI7Z_ERROR_OUT_OF_MEMORY;
    }

    Si7zWriteUInt32(buffer.Data + eocdOffset, SI7Z_ZIP_END_OF_CENTRAL_DIRECTORY_SIGNATURE);
    Si7zWriteUInt16(buffer.Data + eocdOffset + 8, 1);
    Si7zWriteUInt16(buffer.Data + eocdOffset + 10, 1);
    Si7zWriteUInt32(buffer.Data + eocdOffset + 12, (uint32_t)centralDirectorySize);
    Si7zWriteUInt32(buffer.Data + eocdOffset + 16, (uint32_t)centralDirectoryOffset);

    free(name);
    *archiveData = buffer.Data;
    *archiveSize = buffer.Size;
    return SI7Z_OK;
}

SI7Z_RESULT SI7ZAPI Si7zExtractBufferItem(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, const wchar_t* itemName, void** outputData, size_t* outputSize)
{
    if ((!archiveData && archiveSize) || !outputData || !outputSize)
        return SI7Z_ERROR_INVALID_ARGUMENT;

    *outputData = NULL;
    *outputSize = 0;

    SI7Z_RESULT result = Si7zValidateReadOptions(options);
    if (result != SI7Z_OK)
        return result;

    try
    {
        CMyComPtr<IInArchive> archive;
        UInt32 index = 0;

        Si7zEnsureDecoders();

        result = Si7zOpenBuffer(archiveData, archiveSize, options, archive);
        if (result != SI7Z_OK)
            return result;

        result = Si7zFindItem(archive, itemName, index);
        if (result == SI7Z_OK)
            result = Si7zExtractItem(archive, index, options, outputData, outputSize);

        archive->Close();
        return result;
    }
    catch (...)
    {
        // 7-Zip reports allocation failures and malformed input by throwing.
        return SI7Z_ERROR_ARCHIVE_FAILED;
    }
}

SI7Z_RESULT SI7ZAPI Si7zExtractBuffer(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, void** outputData, size_t* outputSize)
{
    return Si7zExtractBufferItem(archiveData, archiveSize, options, NULL, outputData, outputSize);
}

//
// Extraction to a directory.
//

Z7_CLASS_IMP_COM_1(
    CSi7zFileOutStream
    , ISequentialOutStream
)
public:
    HANDLE FileHandle;

    CSi7zFileOutStream() : FileHandle(INVALID_HANDLE_VALUE) {}
    ~CSi7zFileOutStream() { Close(NULL); }

    bool Close(const FILETIME* modifiedTime)
    {
        bool result = true;

        if (FileHandle != INVALID_HANDLE_VALUE)
        {
            if (modifiedTime && !SetFileTime(FileHandle, NULL, NULL, modifiedTime))
                result = false;
            if (!CloseHandle(FileHandle))
                result = false;

            FileHandle = INVALID_HANDLE_VALUE;
        }

        return result;
    }
};

Z7_COM7F_IMF(CSi7zFileOutStream::Write(const void* data, UInt32 size, UInt32* processedSize))
{
    DWORD written = 0;

    if (processedSize)
        *processedSize = 0;

    if (!WriteFile(FileHandle, data, size, &written, NULL))
        return HRESULT_FROM_WIN32(GetLastError());

    if (processedSize)
        *processedSize = written;

    return S_OK;
}

struct CSi7zTarget
{
    UString Path; // full path; empty when the entry is skipped
    bool IsDirectory;
    bool HasModifiedTime;
    FILETIME ModifiedTime;
};

// Win32 maps these names to devices in any directory, with or without an extension
// ("nul.txt" opens NUL), so an entry must never use one as a path component.
static bool Si7zIsReservedDeviceName(const wchar_t* component, size_t length)
{
    static const wchar_t* const names[] = { L"CON", L"PRN", L"AUX", L"NUL", L"CONIN$", L"CONOUT$" };
    size_t baseLength = 0;

    while (baseLength < length && component[baseLength] != L'.')
        baseLength++;

    // Trailing spaces before the extension are ignored by Win32 too ("nul .txt").
    while (baseLength && component[baseLength - 1] == L' ')
        baseLength--;

    for (size_t i = 0; i < Z7_ARRAY_SIZE(names); i++)
    {
        if (wcslen(names[i]) == baseLength && _wcsnicmp(component, names[i], baseLength) == 0)
            return true;
    }

    // COM1-COM9 and LPT1-LPT9, including the superscript digits Win32 also accepts.
    if (baseLength == 4 && (_wcsnicmp(component, L"COM", 3) == 0 || _wcsnicmp(component, L"LPT", 3) == 0))
    {
        const wchar_t digit = component[3];

        if ((digit >= L'1' && digit <= L'9') || digit == 0x00B9 || digit == 0x00B2 || digit == 0x00B3)
            return true;
    }

    return false;
}

// Converts an archive entry path into a path relative to the destination. Drive
// letters, leading separators and "." components are dropped; components that could
// escape the destination or that Windows would silently rewrite are rejected.
static SI7Z_RESULT Si7zMakeRelativePath(const wchar_t* path, UString& relative)
{
    relative.Empty();

    if (path[0] && path[1] == L':')
        path += 2;

    while (*path)
    {
        const wchar_t* start;
        size_t length;

        while (*path == L'\\' || *path == L'/')
            path++;

        start = path;

        while (*path && *path != L'\\' && *path != L'/')
        {
            if (*path < 0x20 || wcschr(L":*?\"<>|", *path))
                return SI7Z_ERROR_UNSAFE_PATH;

            path++;
        }

        length = (size_t)(path - start);

        if (length == 0 || (length == 1 && start[0] == L'.'))
            continue;

        // ".." escapes the destination, and Windows strips trailing dots and spaces
        // (".. " would become ".."), so neither is allowed at the end of a component.
        if (start[length - 1] == L'.' || start[length - 1] == L' ')
            return SI7Z_ERROR_UNSAFE_PATH;

        if (Si7zIsReservedDeviceName(start, length))
            return SI7Z_ERROR_UNSAFE_PATH;

        if (!relative.IsEmpty())
            relative += L'\\';

        for (size_t i = 0; i < length; i++)
            relative += start[i];
    }

    return SI7Z_OK;
}

// Creates directory (and its parents below root). Existing reparse points are refused
// so an entry can never be written through a link planted inside the destination.
static SI7Z_RESULT Si7zCreateDirectoryChain(const UString& root, const UString& directory)
{
    UString path = root;
    const wchar_t* relative = directory.Ptr() + root.Len();

    while (*relative)
    {
        DWORD attributes;

        do
        {
            path += *relative++;
        } while (*relative && *relative != L'\\');

        attributes = GetFileAttributesW(path);

        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
                return SI7Z_ERROR_WRITE_FAILED;
        }
        else if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            return SI7Z_ERROR_UNSAFE_PATH;
        }
        else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            return SI7Z_ERROR_WRITE_FAILED;
        }
    }

    return SI7Z_OK;
}

Z7_CLASS_IMP_COM_2(
    CSi7zDirectoryExtractCallback
    , IArchiveExtractCallback
    , ICryptoGetTextPassword
)
    Z7_IFACE_COM7_IMP(IProgress)
public:
    UString Root;
    const CObjectVector<CSi7zTarget>* Targets;
    const wchar_t* Password;
    CSi7zFileOutStream* OutStreamSpec;
    CMyComPtr<ISequentialOutStream> OutStream;
    UInt32 CurrentIndex;
    SI7Z_RESULT Result;

    CSi7zDirectoryExtractCallback() : Targets(NULL), Password(NULL), OutStreamSpec(NULL), CurrentIndex(0), Result(SI7Z_OK) {}
};

Z7_COM7F_IMF(CSi7zDirectoryExtractCallback::SetTotal(UInt64))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zDirectoryExtractCallback::SetCompleted(const UInt64*))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zDirectoryExtractCallback::GetStream(UInt32 index, ISequentialOutStream** outStream, Int32 askExtractMode))
{
    *outStream = NULL;

    if (askExtractMode != NArchive::NExtract::NAskMode::kExtract)
        return S_OK;

    const CSi7zTarget& target = (*Targets)[index];
    CurrentIndex = index;

    if (target.IsDirectory)
    {
        Result = Si7zCreateDirectoryChain(Root, target.Path);
        return Result == SI7Z_OK ? S_OK : E_ABORT;
    }

    const int separator = target.Path.ReverseFind(L'\\');

    if (separator > (int)Root.Len())
    {
        UString parent = target.Path;

        parent.DeleteFrom((unsigned)separator);
        Result = Si7zCreateDirectoryChain(Root, parent);

        if (Result != SI7Z_OK)
            return E_ABORT;
    }

    const DWORD attributes = GetFileAttributesW(target.Path);

    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)))
    {
        Result = (attributes & FILE_ATTRIBUTE_REPARSE_POINT) ? SI7Z_ERROR_UNSAFE_PATH : SI7Z_ERROR_WRITE_FAILED;
        return E_ABORT;
    }

    const HANDLE fileHandle = CreateFileW(
        target.Path,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL
        );

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        Result = SI7Z_ERROR_WRITE_FAILED;
        return E_ABORT;
    }

    OutStreamSpec = new CSi7zFileOutStream;
    OutStream = OutStreamSpec;
    OutStreamSpec->FileHandle = fileHandle;

    CMyComPtr<ISequentialOutStream> stream = OutStream;
    *outStream = stream.Detach();

    return S_OK;
}

Z7_COM7F_IMF(CSi7zDirectoryExtractCallback::PrepareOperation(Int32))
{
    return S_OK;
}

Z7_COM7F_IMF(CSi7zDirectoryExtractCallback::SetOperationResult(Int32 opRes))
{
    if (OutStreamSpec)
    {
        const CSi7zTarget& target = (*Targets)[CurrentIndex];

        if (!OutStreamSpec->Close(target.HasModifiedTime ? &target.ModifiedTime : NULL) && Result == SI7Z_OK)
            Result = SI7Z_ERROR_WRITE_FAILED;

        OutStream.Release();
        OutStreamSpec = NULL;
    }

    if (Result == SI7Z_OK)
    {
        switch (opRes)
        {
        case NArchive::NExtract::NOperationResult::kOK:
            break;
        case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
            Result = SI7Z_ERROR_UNSUPPORTED_METHOD;
            break;
        default:
            Result = SI7Z_ERROR_DATA_ERROR;
            break;
        }
    }

    return Result == SI7Z_OK ? S_OK : E_ABORT;
}

Z7_COM7F_IMF(CSi7zDirectoryExtractCallback::CryptoGetTextPassword(BSTR* password))
{
    return Si7zGetPassword(Password, password);
}

SI7Z_RESULT SI7ZAPI Si7zExtractBufferToDirectory(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, const wchar_t* destinationPath)
{
    if ((!archiveData && archiveSize) || !destinationPath || !destinationPath[0])
        return SI7Z_ERROR_INVALID_ARGUMENT;

    SI7Z_RESULT result = Si7zValidateReadOptions(options);
    if (result != SI7Z_OK)
        return result;

    try
    {
        CMyComPtr<IInArchive> archive;
        CObjectVector<CSi7zTarget> targets;
        CRecordVector<UInt32> indices;
        UString root = destinationPath;
        UInt32 numItems = 0;

        while (root.Len() > 1 && (root.Back() == L'\\' || root.Back() == L'/'))
            root.DeleteBack();

        Si7zEnsureDecoders();

        result = Si7zOpenBuffer(archiveData, archiveSize, options, archive);
        if (result != SI7Z_OK)
            return result;

        if (archive->GetNumberOfItems(&numItems) != S_OK)
        {
            archive->Close();
            return SI7Z_ERROR_ARCHIVE_FAILED;
        }

        // Validate every entry before anything is written.
        for (UInt32 i = 0; i < numItems; i++)
        {
            CSi7zTarget& target = targets.AddNew();
            NWindows::NCOM::CPropVariant prop;
            UString path;
            UString relative;

            if (Si7zGetItemPath(archive, i, path) != S_OK ||
                Si7zGetItemBool(archive, i, kpidIsDir, target.IsDirectory) != S_OK ||
                archive->GetProperty(i, kpidMTime, &prop) != S_OK)
            {
                archive->Close();
                return SI7Z_ERROR_ARCHIVE_FAILED;
            }

            target.HasModifiedTime = prop.vt == VT_FILETIME;
            if (target.HasModifiedTime)
                target.ModifiedTime = prop.filetime;

            result = Si7zMakeRelativePath(path, relative);
            if (result != SI7Z_OK)
            {
                archive->Close();
                return result;
            }

            if (relative.IsEmpty())
                continue;

            target.Path = root;
            target.Path += L'\\';
            target.Path += relative;
            indices.Add(i);
        }

        // Create the destination and any missing parents. Failures on the intermediate
        // components (drive roots, UNC shares) are expected; only the result is checked.
        for (unsigned i = 1; i <= root.Len(); i++)
        {
            if (i == root.Len() || root[i] == L'\\' || root[i] == L'/')
            {
                UString prefix = root;

                prefix.DeleteFrom(i);
                CreateDirectoryW(prefix, NULL);
            }
        }

        const DWORD rootAttributes = GetFileAttributesW(root);

        if (rootAttributes == INVALID_FILE_ATTRIBUTES || !(rootAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            archive->Close();
            return SI7Z_ERROR_WRITE_FAILED;
        }

        if (indices.IsEmpty())
        {
            archive->Close();
            return SI7Z_OK;
        }

        CSi7zDirectoryExtractCallback* callbackSpec = new CSi7zDirectoryExtractCallback;
        CMyComPtr<IArchiveExtractCallback> callback = callbackSpec;

        callbackSpec->Root = root;
        callbackSpec->Targets = &targets;
        callbackSpec->Password = options ? options->Password : NULL;

        const HRESULT hr = archive->Extract(indices.ConstData(), indices.Size(), 0, callback);

        archive->Close();

        if (callbackSpec->Result != SI7Z_OK)
            return callbackSpec->Result;

        return Si7zResultFromHresult(hr);
    }
    catch (...)
    {
        return SI7Z_ERROR_ARCHIVE_FAILED;
    }
}

SI7Z_RESULT SI7ZAPI Si7zListBuffer(const void* archiveData, size_t archiveSize, const SI7Z_OPTIONS* options, SI7Z_LIST* list)
{
    if ((!archiveData && archiveSize) || !list)
        return SI7Z_ERROR_INVALID_ARGUMENT;

    list->Items = NULL;
    list->Count = 0;

    SI7Z_RESULT result = Si7zValidateReadOptions(options);
    if (result != SI7Z_OK)
        return result;

    SI7Z_ITEM* items = NULL;
    UInt32 count = 0;

    try
    {
        CMyComPtr<IInArchive> archive;

        Si7zEnsureDecoders();

        result = Si7zOpenBuffer(archiveData, archiveSize, options, archive);
        if (result != SI7Z_OK)
            return result;

        if (archive->GetNumberOfItems(&count) != S_OK)
        {
            archive->Close();
            return SI7Z_ERROR_ARCHIVE_FAILED;
        }

        items = (SI7Z_ITEM*)calloc(count ? count : 1, sizeof(SI7Z_ITEM));
        if (!items)
        {
            archive->Close();
            return SI7Z_ERROR_OUT_OF_MEMORY;
        }

        for (UInt32 i = 0; i < count; i++)
        {
            UString path;
            UInt64 attributes;
            bool isDirectory;

            if (Si7zGetItemPath(archive, i, path) != S_OK ||
                Si7zGetItemUInt64(archive, i, kpidSize, items[i].Size) != S_OK ||
                Si7zGetItemUInt64(archive, i, kpidPackSize, items[i].PackedSize) != S_OK ||
                Si7zGetItemUInt64(archive, i, kpidAttrib, attributes) != S_OK ||
                Si7zGetItemBool(archive, i, kpidIsDir, isDirectory) != S_OK ||
                Si7zGetItemMethod(archive, i, items[i].Method) != S_OK)
            {
                Si7zFreeItems(items, count);
                archive->Close();
                return SI7Z_ERROR_ARCHIVE_FAILED;
            }

            const size_t length = path.Len();

            items[i].Name = (wchar_t*)malloc((length + 1) * sizeof(wchar_t));
            if (!items[i].Name)
            {
                Si7zFreeItems(items, count);
                archive->Close();
                return SI7Z_ERROR_OUT_OF_MEMORY;
            }

            memcpy(items[i].Name, path.Ptr(), (length + 1) * sizeof(wchar_t));
            items[i].Attributes = (uint32_t)attributes;
            items[i].IsDirectory = isDirectory;
        }

        archive->Close();
    }
    catch (...)
    {
        Si7zFreeItems(items, count);
        return SI7Z_ERROR_ARCHIVE_FAILED;
    }

    list->Items = items;
    list->Count = count;
    return SI7Z_OK;
}

void SI7ZAPI Si7zFreeBuffer(void* buffer)
{
    free(buffer);
}

void SI7ZAPI Si7zFreeList(SI7Z_LIST* list)
{
    if (!list)
        return;

    for (uint32_t i = 0; i < list->Count; i++)
        free(list->Items[i].Name);

    free(list->Items);
    list->Items = NULL;
    list->Count = 0;
}
