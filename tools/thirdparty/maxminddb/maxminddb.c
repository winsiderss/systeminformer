//#if HAVE_CONFIG_H
//#include <config.h>
//#endif

#include <ph.h>

#include "data-pool.h"
#include "maxminddb-compat-util.h"
#include "maxminddb.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#include <ws2ipdef.h>
#else
#include <arpa/inet.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#define MMDB_DATA_SECTION_SEPARATOR (16)
#define MAXIMUM_DATA_STRUCTURE_DEPTH (512)

#ifdef MMDB_DEBUG
#define DEBUG_MSG(msg) fprintf(stderr, msg "\n")
#define DEBUG_MSGF(fmt, ...) fprintf(stderr, fmt "\n", __VA_ARGS__)
#define DEBUG_BINARY(fmt, byte)                                                \
    do {                                                                       \
        char *binary = byte_to_binary(byte);                                   \
        if (NULL == binary) {                                                  \
            fprintf(stderr, "Calloc failed in DEBUG_BINARY\n");                \
            abort();                                                           \
        }                                                                      \
        fprintf(stderr, fmt "\n", binary);                                     \
        free(binary);                                                          \
    } while (0)
#define DEBUG_NL fprintf(stderr, "\n")
#else
#define DEBUG_MSG(...)
#define DEBUG_MSGF(...)
#define DEBUG_BINARY(...)
#define DEBUG_NL
#endif

#ifdef MMDB_DEBUG
char *byte_to_binary(uint8_t byte) {
    char *bits = calloc(9, sizeof(char));
    if (NULL == bits) {
        return bits;
    }

    for (uint8_t i = 0; i < 8; i++) {
        bits[i] = byte & (128 >> i) ? '1' : '0';
    }
    bits[8] = '\0';

    return bits;
}

char *type_num_to_name(uint8_t num) {
    switch (num) {
        case 0:
            return "extended";
        case 1:
            return "pointer";
        case 2:
            return "utf8_string";
        case 3:
            return "double";
        case 4:
            return "bytes";
        case 5:
            return "uint16";
        case 6:
            return "uint32";
        case 7:
            return "map";
        case 8:
            return "int32";
        case 9:
            return "uint64";
        case 10:
            return "uint128";
        case 11:
            return "array";
        case 12:
            return "container";
        case 13:
            return "end_marker";
        case 14:
            return "boolean";
        case 15:
            return "float";
        default:
            return "unknown type";
    }
}
#endif

/* None of the values we check on the lhs are bigger than uint32_t, so on
 * platforms where SIZE_MAX is a 64-bit integer, this would be a no-op, and it
 * makes the compiler complain if we do the check anyway. */
#if SIZE_MAX == UINT32_MAX
#define MAYBE_CHECK_SIZE_OVERFLOW(lhs, rhs, error)                             \
    if ((lhs) > (rhs)) {                                                       \
        return error;                                                          \
    }
#else
#define MAYBE_CHECK_SIZE_OVERFLOW(...)
#endif

typedef struct record_info_s {
    uint16_t record_length;
    uint32_t (*left_record_getter)(const uint8_t *);
    uint32_t (*right_record_getter)(const uint8_t *);
    uint8_t right_record_offset;
} record_info_s;

#define METADATA_MARKER "\xab\xcd\xefMaxMind.com"
/* This is 128kb */
#define METADATA_BLOCK_MAX_SIZE 131072

// 64 leads us to allocating 4 KiB on a 64bit system.
#define MMDB_POOL_INIT_SIZE 64

static int map_file(MMDB_s *const mmdb);
static const uint8_t *find_metadata(const uint8_t *file_content,
                                    ssize_t file_size,
                                    uint32_t *metadata_size);
static int read_metadata(MMDB_s *mmdb);
static MMDB_s make_fake_metadata_db(const MMDB_s *const mmdb);
static int
value_for_key_as_uint16(MMDB_entry_s *start, char *key, uint16_t *value);
static int
value_for_key_as_uint32(MMDB_entry_s *start, char *key, uint32_t *value);
static int
value_for_key_as_uint64(MMDB_entry_s *start, char *key, uint64_t *value);
static int
value_for_key_as_string(MMDB_entry_s *start, char *key, char const **value);
static int populate_languages_metadata(MMDB_s *mmdb,
                                       MMDB_s *metadata_db,
                                       MMDB_entry_s *metadata_start);
static int populate_description_metadata(MMDB_s *mmdb,
                                         MMDB_s *metadata_db,
                                         MMDB_entry_s *metadata_start);
static int resolve_any_address(const char *ipstr, struct addrinfo **addresses);
static int find_address_in_search_tree(const MMDB_s *const mmdb,
                                       uint8_t *address,
                                       sa_family_t address_family,
                                       MMDB_lookup_result_s *result);
static record_info_s record_info_for_database(const MMDB_s *const mmdb);
static int find_ipv4_start_node(MMDB_s *const mmdb);
static uint8_t record_type(const MMDB_s *const mmdb, uint64_t record);
static uint32_t get_left_28_bit_record(const uint8_t *record);
static uint32_t get_right_28_bit_record(const uint8_t *record);
static uint32_t data_section_offset_for_record(const MMDB_s *const mmdb,
                                               uint64_t record);
static int path_length(va_list va_path);
static int lookup_path_in_array(const char *path_elem,
                                const MMDB_s *const mmdb,
                                MMDB_entry_data_s *entry_data);
static int lookup_path_in_map(const char *path_elem,
                              const MMDB_s *const mmdb,
                              MMDB_entry_data_s *entry_data);
static int skip_map_or_array(const MMDB_s *const mmdb,
                             MMDB_entry_data_s *entry_data);
static int decode_one_follow(const MMDB_s *const mmdb,
                             uint32_t offset,
                             MMDB_entry_data_s *entry_data);
static int decode_one(const MMDB_s *const mmdb,
                      uint32_t offset,
                      MMDB_entry_data_s *entry_data);
static int get_ext_type(int raw_ext_type);
static uint32_t
get_ptr_from(uint8_t ctrl, uint8_t const *const ptr, int ptr_size);
static int get_entry_data_list(const MMDB_s *const mmdb,
                               uint32_t offset,
                               MMDB_entry_data_list_s *const entry_data_list,
                               MMDB_data_pool_s *const pool,
                               int depth);
static float get_ieee754_float(const uint8_t *restrict p);
static double get_ieee754_double(const uint8_t *restrict p);
static uint32_t get_uint32(const uint8_t *p);
static uint32_t get_uint24(const uint8_t *p);
static uint32_t get_uint16(const uint8_t *p);
static uint64_t get_uintX(const uint8_t *p, int length);
static int32_t get_sintX(const uint8_t *p, int length);
static void free_mmdb_struct(MMDB_s *const mmdb);
static void free_languages_metadata(MMDB_s *mmdb);
static void free_descriptions_metadata(MMDB_s *mmdb);
static MMDB_entry_data_list_s *
dump_entry_data_list(FILE *stream,
                     MMDB_entry_data_list_s *entry_data_list,
                     int indent,
                     int *status);
static void print_indentation(FILE *stream, int i);
static char *bytes_to_hex(uint8_t *bytes, uint32_t size);

#define CHECKED_DECODE_ONE(mmdb, offset, entry_data)                           \
    do {                                                                       \
        int status = decode_one(mmdb, offset, entry_data);                     \
        if (MMDB_SUCCESS != status) {                                          \
            DEBUG_MSGF("CHECKED_DECODE_ONE failed."                            \
                       " status = %d (%s)",                                    \
                       status,                                                 \
                       MMDB_strerror(status));                                 \
            return status;                                                     \
        }                                                                      \
    } while (0)

#define CHECKED_DECODE_ONE_FOLLOW(mmdb, offset, entry_data)                    \
    do {                                                                       \
        int status = decode_one_follow(mmdb, offset, entry_data);              \
        if (MMDB_SUCCESS != status) {                                          \
            DEBUG_MSGF("CHECKED_DECODE_ONE_FOLLOW failed."                     \
                       " status = %d (%s)",                                    \
                       status,                                                 \
                       MMDB_strerror(status));                                 \
            return status;                                                     \
        }                                                                      \
    } while (0)

#define FREE_AND_SET_NULL(p)                                                   \
    {                                                                          \
        free((void *)(p));                                                     \
        (p) = NULL;                                                            \
    }

int MMDB_open(wchar_t* filename, uint32_t flags, MMDB_s *const mmdb) {
    int status = MMDB_SUCCESS;

    mmdb->file_content = NULL;
    mmdb->data_section = NULL;
    mmdb->metadata.database_type = NULL;
    mmdb->metadata.languages.count = 0;
    mmdb->metadata.languages.names = NULL;
    mmdb->metadata.description.count = 0;

    mmdb->filename = filename; // dmex: modified for wchar_t
    if (NULL == mmdb->filename) {
        status = MMDB_OUT_OF_MEMORY_ERROR;
        goto cleanup;
    }

    if ((flags & MMDB_MODE_MASK) == 0) {
        flags |= MMDB_MODE_MMAP;
    }
    mmdb->flags = flags;

    if (MMDB_SUCCESS != (status = map_file(mmdb))) {
        goto cleanup;
    }

//#ifdef _WIN32 // dmex: disabled since hostname lookup is not used.
//    WSADATA wsa;
//    WSAStartup(MAKEWORD(2, 2), &wsa);
//#endif

    uint32_t metadata_size = 0;
    const uint8_t *metadata =
        find_metadata(mmdb->file_content, mmdb->file_size, &metadata_size);
    if (NULL == metadata) {
        status = MMDB_INVALID_METADATA_ERROR;
        goto cleanup;
    }

    mmdb->metadata_section = metadata;
    mmdb->metadata_section_size = metadata_size;

    status = read_metadata(mmdb);
    if (MMDB_SUCCESS != status) {
        goto cleanup;
    }

    if (mmdb->metadata.binary_format_major_version != 2) {
        status = MMDB_UNKNOWN_DATABASE_FORMAT_ERROR;
        goto cleanup;
    }

    uint32_t search_tree_size =
        mmdb->metadata.node_count * mmdb->full_record_byte_size;

    mmdb->data_section =
        mmdb->file_content + search_tree_size + MMDB_DATA_SECTION_SEPARATOR;
    if (search_tree_size + MMDB_DATA_SECTION_SEPARATOR >
        (uint32_t)mmdb->file_size) {
        status = MMDB_INVALID_METADATA_ERROR;
        goto cleanup;
    }
    mmdb->data_section_size = (uint32_t)mmdb->file_size - search_tree_size -
                              MMDB_DATA_SECTION_SEPARATOR;

    // Although it is likely not possible to construct a database with valid
    // valid metadata, as parsed above, and a data_section_size less than 3,
    // we do this check as later we assume it is at least three when doing
    // bound checks.
    if (mmdb->data_section_size < 3) {
        status = MMDB_INVALID_DATA_ERROR;
        goto cleanup;
    }

    mmdb->metadata_section = metadata;
    mmdb->ipv4_start_node.node_value = 0;
    mmdb->ipv4_start_node.netmask = 0;

    // We do this immediately as otherwise there is a race to set
    // ipv4_start_node.node_value and ipv4_start_node.netmask.
    if (mmdb->metadata.ip_version == 6) {
        status = find_ipv4_start_node(mmdb);
        if (status != MMDB_SUCCESS) {
            goto cleanup;
        }
    }

cleanup:
    if (MMDB_SUCCESS != status) {
        int saved_errno = errno;
        free_mmdb_struct(mmdb);
        errno = saved_errno;
    }
    return status;
}

#ifdef _WIN32

static LPWSTR utf8_to_utf16(const char *utf8_str) {
    int wide_chars = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    wchar_t *utf16_str = (wchar_t *)calloc(wide_chars, sizeof(wchar_t));
    if (!utf16_str) {
        return NULL;
    }

    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, utf16_str, wide_chars) <
        1) {
        free(utf16_str);
        return NULL;
    }

    return utf16_str;
}

static int map_file(MMDB_s *const mmdb) // dmex: modified for NTAPI
{
    NTSTATUS status;
    HANDLE fileHandle;
    HANDLE sectionHandle;
    LARGE_INTEGER fileSize;
    SIZE_T viewSize;
    PVOID viewBase;

    status = PhCreateFileWin32(
        &fileHandle,
        mmdb->filename,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return MMDB_FILE_OPEN_ERROR;

    status = PhGetFileSize(fileHandle, &fileSize);

    if (!NT_SUCCESS(status))
    {
        NtClose(fileHandle);
        return MMDB_FILE_OPEN_ERROR;
    }

    status = NtCreateSection(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ,
        NULL,
        &fileSize,
        PAGE_READONLY,
        SEC_COMMIT,
        fileHandle
        );

    NtClose(fileHandle);

    if (!NT_SUCCESS(status))
        return MMDB_FILE_OPEN_ERROR;

    viewSize = (SIZE_T)fileSize.QuadPart;
    viewBase = NULL;

    status = NtMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &viewBase,
        0,
        0,
        NULL,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        );

    NtClose(sectionHandle);

    if (!NT_SUCCESS(status))
        return MMDB_FILE_OPEN_ERROR;

    mmdb->file_size = viewSize;
    mmdb->file_content = viewBase;
    return MMDB_SUCCESS;
}

#else // _WIN32

static int map_file(MMDB_s *const mmdb) {
    ssize_t size;
    int status = MMDB_SUCCESS;

    int flags = O_RDONLY;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif
    int fd = open(mmdb->filename, flags);
    struct stat s;
    if (fd < 0 || fstat(fd, &s)) {
        status = MMDB_FILE_OPEN_ERROR;
        goto cleanup;
    }

    size = s.st_size;
    if (size < 0 || size != s.st_size) {
        status = MMDB_OUT_OF_MEMORY_ERROR;
        goto cleanup;
    }

    uint8_t *file_content =
        (uint8_t *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (MAP_FAILED == file_content) {
        if (ENOMEM == errno) {
            status = MMDB_OUT_OF_MEMORY_ERROR;
        } else {
            status = MMDB_IO_ERROR;
        }
        goto cleanup;
    }

    mmdb->file_size = size;
    mmdb->file_content = file_content;

cleanup:;
    int saved_errno = errno;
    if (fd >= 0) {
        close(fd);
    }
    errno = saved_errno;

    return status;
}

#endif // _WIN32

static const uint8_t *find_metadata(const uint8_t *file_content,
                                    ssize_t file_size,
                                    uint32_t *metadata_size) {
    const ssize_t marker_len = sizeof(METADATA_MARKER) - 1;
    ssize_t max_size = file_size > METADATA_BLOCK_MAX_SIZE
                           ? METADATA_BLOCK_MAX_SIZE
                           : file_size;

    uint8_t *search_area = (uint8_t *)(file_content + (file_size - max_size));
    uint8_t *start = search_area;
    uint8_t *tmp;
    do {
        tmp = mmdb_memmem(search_area, max_size, METADATA_MARKER, marker_len);

        if (NULL != tmp) {
            max_size -= tmp - search_area;
            search_area = tmp;

            /* Continue searching just after the marker we just read, in case
             * there are multiple markers in the same file. This would be odd
             * but is certainly not impossible. */
            max_size -= marker_len;
            search_area += marker_len;
        }
    } while (NULL != tmp);

    if (search_area == start) {
        return NULL;
    }

    *metadata_size = (uint32_t)max_size;

    return search_area;
}

static int read_metadata(MMDB_s *mmdb) {
    /* We need to create a fake MMDB_s struct in order to decode values from
       the metadata. The metadata is basically just like the data section, so we
       want to use the same functions we use for the data section to get
       metadata values. */
    MMDB_s metadata_db = make_fake_metadata_db(mmdb);

    MMDB_entry_s metadata_start = {.mmdb = &metadata_db, .offset = 0};

    int status = value_for_key_as_uint32(
        &metadata_start, "node_count", &mmdb->metadata.node_count);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (!mmdb->metadata.node_count) {
        DEBUG_MSG("could not find node_count value in metadata");
        return MMDB_INVALID_METADATA_ERROR;
    }

    status = value_for_key_as_uint16(
        &metadata_start, "record_size", &mmdb->metadata.record_size);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (!mmdb->metadata.record_size) {
        DEBUG_MSG("could not find record_size value in metadata");
        return MMDB_INVALID_METADATA_ERROR;
    }

    if (mmdb->metadata.record_size != 24 && mmdb->metadata.record_size != 28 &&
        mmdb->metadata.record_size != 32) {
        DEBUG_MSGF("bad record size in metadata: %i",
                   mmdb->metadata.record_size);
        return MMDB_UNKNOWN_DATABASE_FORMAT_ERROR;
    }

    status = value_for_key_as_uint16(
        &metadata_start, "ip_version", &mmdb->metadata.ip_version);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (!mmdb->metadata.ip_version) {
        DEBUG_MSG("could not find ip_version value in metadata");
        return MMDB_INVALID_METADATA_ERROR;
    }
    if (!(mmdb->metadata.ip_version == 4 || mmdb->metadata.ip_version == 6)) {
        DEBUG_MSGF("ip_version value in metadata is not 4 or 6 - it was %i",
                   mmdb->metadata.ip_version);
        return MMDB_INVALID_METADATA_ERROR;
    }

    status = value_for_key_as_string(
        &metadata_start, "database_type", &mmdb->metadata.database_type);
    if (MMDB_SUCCESS != status) {
        DEBUG_MSG("error finding database_type value in metadata");
        return status;
    }

    status = populate_languages_metadata(mmdb, &metadata_db, &metadata_start);
    if (MMDB_SUCCESS != status) {
        DEBUG_MSG("could not populate languages from metadata");
        return status;
    }

    status =
        value_for_key_as_uint16(&metadata_start,
                                "binary_format_major_version",
                                &mmdb->metadata.binary_format_major_version);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (!mmdb->metadata.binary_format_major_version) {
        DEBUG_MSG(
            "could not find binary_format_major_version value in metadata");
        return MMDB_INVALID_METADATA_ERROR;
    }

    status =
        value_for_key_as_uint16(&metadata_start,
                                "binary_format_minor_version",
                                &mmdb->metadata.binary_format_minor_version);
    if (MMDB_SUCCESS != status) {
        return status;
    }

    status = value_for_key_as_uint64(
        &metadata_start, "build_epoch", &mmdb->metadata.build_epoch);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (!mmdb->metadata.build_epoch) {
        DEBUG_MSG("could not find build_epoch value in metadata");
        return MMDB_INVALID_METADATA_ERROR;
    }

    status = populate_description_metadata(mmdb, &metadata_db, &metadata_start);
    if (MMDB_SUCCESS != status) {
        DEBUG_MSG("could not populate description from metadata");
        return status;
    }

    mmdb->full_record_byte_size = mmdb->metadata.record_size * 2 / 8U;

    mmdb->depth = mmdb->metadata.ip_version == 4 ? 32 : 128;

    return MMDB_SUCCESS;
}

static MMDB_s make_fake_metadata_db(const MMDB_s *const mmdb) {
    MMDB_s fake_metadata_db = {.data_section = mmdb->metadata_section,
                               .data_section_size =
                                   mmdb->metadata_section_size};

    return fake_metadata_db;
}

static int
value_for_key_as_uint16(MMDB_entry_s *start, char *key, uint16_t *value) {
    MMDB_entry_data_s entry_data;
    const char *path[] = {key, NULL};
    int status = MMDB_aget_value(start, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (MMDB_DATA_TYPE_UINT16 != entry_data.type) {
        DEBUG_MSGF("expect uint16 for %s but received %s",
                   key,
                   type_num_to_name(entry_data.type));
        return MMDB_INVALID_METADATA_ERROR;
    }
    *value = entry_data.uint16;
    return MMDB_SUCCESS;
}

static int
value_for_key_as_uint32(MMDB_entry_s *start, char *key, uint32_t *value) {
    MMDB_entry_data_s entry_data;
    const char *path[] = {key, NULL};
    int status = MMDB_aget_value(start, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (MMDB_DATA_TYPE_UINT32 != entry_data.type) {
        DEBUG_MSGF("expect uint32 for %s but received %s",
                   key,
                   type_num_to_name(entry_data.type));
        return MMDB_INVALID_METADATA_ERROR;
    }
    *value = entry_data.uint32;
    return MMDB_SUCCESS;
}

static int
value_for_key_as_uint64(MMDB_entry_s *start, char *key, uint64_t *value) {
    MMDB_entry_data_s entry_data;
    const char *path[] = {key, NULL};
    int status = MMDB_aget_value(start, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (MMDB_DATA_TYPE_UINT64 != entry_data.type) {
        DEBUG_MSGF("expect uint64 for %s but received %s",
                   key,
                   type_num_to_name(entry_data.type));
        return MMDB_INVALID_METADATA_ERROR;
    }
    *value = entry_data.uint64;
    return MMDB_SUCCESS;
}

static int
value_for_key_as_string(MMDB_entry_s *start, char *key, char const **value) {
    MMDB_entry_data_s entry_data;
    const char *path[] = {key, NULL};
    int status = MMDB_aget_value(start, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (MMDB_DATA_TYPE_UTF8_STRING != entry_data.type) {
        DEBUG_MSGF("expect string for %s but received %s",
                   key,
                   type_num_to_name(entry_data.type));
        return MMDB_INVALID_METADATA_ERROR;
    }
    *value = mmdb_strndup((char *)entry_data.utf8_string, entry_data.data_size);
    if (NULL == *value) {
        return MMDB_OUT_OF_MEMORY_ERROR;
    }
    return MMDB_SUCCESS;
}

static int populate_languages_metadata(MMDB_s *mmdb,
                                       MMDB_s *metadata_db,
                                       MMDB_entry_s *metadata_start) {
    MMDB_entry_data_s entry_data;

    const char *path[] = {"languages", NULL};
    int status = MMDB_aget_value(metadata_start, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        return status;
    }
    if (MMDB_DATA_TYPE_ARRAY != entry_data.type) {
        return MMDB_INVALID_METADATA_ERROR;
    }

    MMDB_entry_s array_start = {.mmdb = metadata_db,
                                .offset = entry_data.offset};

    MMDB_entry_data_list_s *member;
    status = MMDB_get_entry_data_list(&array_start, &member);
    if (MMDB_SUCCESS != status) {
        return status;
    }

    MMDB_entry_data_list_s *first_member = member;

    uint32_t array_size = member->entry_data.data_size;
    MAYBE_CHECK_SIZE_OVERFLOW(
        array_size, SIZE_MAX / sizeof(char *), MMDB_INVALID_METADATA_ERROR);

    mmdb->metadata.languages.count = 0;
    mmdb->metadata.languages.names = calloc(array_size, sizeof(char *));
    if (NULL == mmdb->metadata.languages.names) {
        return MMDB_OUT_OF_MEMORY_ERROR;
    }

    for (uint32_t i = 0; i < array_size; i++) {
        member = member->next;
        if (MMDB_DATA_TYPE_UTF8_STRING != member->entry_data.type) {
            return MMDB_INVALID_METADATA_ERROR;
        }

        mmdb->metadata.languages.names[i] =
            mmdb_strndup((char *)member->entry_data.utf8_string,
                         member->entry_data.data_size);

        if (NULL == mmdb->metadata.languages.names[i]) {
            return MMDB_OUT_OF_MEMORY_ERROR;
        }
        // We assign this as we go so that if we fail a calloc and need to
        // free it, the count is right.
        mmdb->metadata.languages.count = i + 1;
    }

    MMDB_free_entry_data_list(first_member);

    return MMDB_SUCCESS;
}

static int populate_description_metadata(MMDB_s *mmdb,
                                         MMDB_s *metadata_db,
                                         MMDB_entry_s *metadata_start) {
    MMDB_entry_data_s entry_data;

    const char *path[] = {"description", NULL};
    int status = MMDB_aget_value(metadata_start, &entry_data, path);
    if (MMDB_SUCCESS != status) {
        return status;
    }

    if (MMDB_DATA_TYPE_MAP != entry_data.type) {
        DEBUG_MSGF("Unexpected entry_data type: %d", entry_data.type);
        return MMDB_INVALID_METADATA_ERROR;
    }

    MMDB_entry_s map_start = {.mmdb = metadata_db, .offset = entry_data.offset};

    MMDB_entry_data_list_s *member;
    status = MMDB_get_entry_data_list(&map_start, &member);
    if (MMDB_SUCCESS != status) {
        DEBUG_MSGF(
            "MMDB_get_entry_data_list failed while populating description."
            " status = %d (%s)",
            status,
            MMDB_strerror(status));
        return status;
    }

    MMDB_entry_data_list_s *first_member = member;

    uint32_t map_size = member->entry_data.data_size;
    mmdb->metadata.description.count = 0;
    if (0 == map_size) {
        mmdb->metadata.description.descriptions = NULL;
        goto cleanup;
    }
    MAYBE_CHECK_SIZE_OVERFLOW(map_size,
                              SIZE_MAX / sizeof(MMDB_description_s *),
                              MMDB_INVALID_METADATA_ERROR);

    mmdb->metadata.description.descriptions =
        calloc(map_size, sizeof(MMDB_description_s *));
    if (NULL == mmdb->metadata.description.descriptions) {
        status = MMDB_OUT_OF_MEMORY_ERROR;
        goto cleanup;
    }

    for (uint32_t i = 0; i < map_size; i++) {
        mmdb->metadata.description.descriptions[i] =
            calloc(1, sizeof(MMDB_description_s));
        if (NULL == mmdb->metadata.description.descriptions[i]) {
            status = MMDB_OUT_OF_MEMORY_ERROR;
            goto cleanup;
        }

        mmdb->metadata.description.count = i + 1;
        mmdb->metadata.description.descriptions[i]->language = NULL;
        mmdb->metadata.description.descriptions[i]->description = NULL;

        member = member->next;

        if (MMDB_DATA_TYPE_UTF8_STRING != member->entry_data.type) {
            status = MMDB_INVALID_METADATA_ERROR;
            goto cleanup;
        }

        mmdb->metadata.description.descriptions[i]->language =
            mmdb_strndup((char *)member->entry_data.utf8_string,
                         member->entry_data.data_size);

        if (NULL == mmdb->metadata.description.descriptions[i]->language) {
            status = MMDB_OUT_OF_MEMORY_ERROR;
            goto cleanup;
        }

        member = member->next;

        if (MMDB_DATA_TYPE_UTF8_STRING != member->entry_data.type) {
            status = MMDB_INVALID_METADATA_ERROR;
            goto cleanup;
        }

        mmdb->metadata.description.descriptions[i]->description =
            mmdb_strndup((char *)member->entry_data.utf8_string,
                         member->entry_data.data_size);

        if (NULL == mmdb->metadata.description.descriptions[i]->description) {
            status = MMDB_OUT_OF_MEMORY_ERROR;
            goto cleanup;
        }
    }

cleanup:
    MMDB_free_entry_data_list(first_member);

    return status;
}

MMDB_lookup_result_s MMDB_lookup_string(const MMDB_s *const mmdb,
                                        const char *const ipstr,
                                        int *const gai_error,
                                        int *const mmdb_error) {
    MMDB_lookup_result_s result = {.found_entry = false,
                                   .netmask = 0,
                                   .entry = {.mmdb = mmdb, .offset = 0}};

    struct addrinfo *addresses = NULL;
    *gai_error = resolve_any_address(ipstr, &addresses);

    if (!*gai_error) {
        result = MMDB_lookup_sockaddr(mmdb, addresses->ai_addr, mmdb_error);
    }

    if (NULL != addresses) {
        freeaddrinfo(addresses);
    }

    return result;
}

static int resolve_any_address(const char *ipstr, struct addrinfo **addresses) {
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_flags = AI_NUMERICHOST,
        // We set ai_socktype so that we only get one result back
        .ai_socktype = SOCK_STREAM};

    int gai_status = getaddrinfo(ipstr, NULL, &hints, addresses);
    if (gai_status) {
        return gai_status;
    }

    return 0;
}

MMDB_lookup_result_s MMDB_lookup_sockaddr(const MMDB_s *const mmdb,
                                          const struct sockaddr *const sockaddr,
                                          int *const mmdb_error) {
    MMDB_lookup_result_s result = {.found_entry = false,
                                   .netmask = 0,
                                   .entry = {.mmdb = mmdb, .offset = 0}};

    uint8_t mapped_address[16], *address;
    if (mmdb->metadata.ip_version == 4) {
        if (sockaddr->sa_family == AF_INET6) {
            *mmdb_error = MMDB_IPV6_LOOKUP_IN_IPV4_DATABASE_ERROR;
            return result;
        }
        address = (uint8_t *)&((struct sockaddr_in *)sockaddr)->sin_addr.s_addr;
    } else {
        if (sockaddr->sa_family == AF_INET6) {
            address = (uint8_t *)&((struct sockaddr_in6 *)sockaddr)
                          ->sin6_addr.s6_addr;
        } else {
            address = mapped_address;
            memset(address, 0, 12);
            memcpy(address + 12,
                   &((struct sockaddr_in *)sockaddr)->sin_addr.s_addr,
                   4);
        }
    }

    *mmdb_error = find_address_in_search_tree(
        mmdb, address, sockaddr->sa_family, &result);

    return result;
}

static int find_address_in_search_tree(const MMDB_s *const mmdb,
                                       uint8_t *address,
                                       sa_family_t address_family,
                                       MMDB_lookup_result_s *result) {
    record_info_s record_info = record_info_for_database(mmdb);
    if (0 == record_info.right_record_offset) {
        return MMDB_UNKNOWN_DATABASE_FORMAT_ERROR;
    }

    uint32_t value = 0;
    uint16_t current_bit = 0;
    if (mmdb->metadata.ip_version == 6 && address_family == AF_INET) {
        value = mmdb->ipv4_start_node.node_value;
        current_bit = mmdb->ipv4_start_node.netmask;
    }

    uint32_t node_count = mmdb->metadata.node_count;
    const uint8_t *search_tree = mmdb->file_content;
    const uint8_t *record_pointer;
    for (; current_bit < mmdb->depth && value < node_count; current_bit++) {
        uint8_t bit =
            1U & (address[current_bit >> 3] >> (7 - (current_bit % 8)));

        record_pointer = &search_tree[value * record_info.record_length];
        if (record_pointer + record_info.record_length > mmdb->data_section) {
            return MMDB_CORRUPT_SEARCH_TREE_ERROR;
        }
        if (bit) {
            record_pointer += record_info.right_record_offset;
            value = record_info.right_record_getter(record_pointer);
        } else {
            value = record_info.left_record_getter(record_pointer);
        }
    }

    result->netmask = current_bit;

    if (value >= node_count + mmdb->data_section_size) {
        // The pointer points off the end of the database.
        return MMDB_CORRUPT_SEARCH_TREE_ERROR;
    }

    if (value == node_count) {
        // record is empty
        result->found_entry = false;
        return MMDB_SUCCESS;
    }
    result->found_entry = true;
    result->entry.offset = data_section_offset_for_record(mmdb, value);

    return MMDB_SUCCESS;
}

static record_info_s record_info_for_database(const MMDB_s *const mmdb) {
    record_info_s record_info = {.record_length = mmdb->full_record_byte_size,
                                 .right_record_offset = 0};

    if (record_info.record_length == 6) {
        record_info.left_record_getter = &get_uint24;
        record_info.right_record_getter = &get_uint24;
        record_info.right_record_offset = 3;
    } else if (record_info.record_length == 7) {
        record_info.left_record_getter = &get_left_28_bit_record;
        record_info.right_record_getter = &get_right_28_bit_record;
        record_info.right_record_offset = 3;
    } else if (record_info.record_length == 8) {
        record_info.left_record_getter = &get_uint32;
        record_info.right_record_getter = &get_uint32;
        record_info.right_record_offset = 4;
    } else {
        assert(false);
    }

    return record_info;
}

static int find_ipv4_start_node(MMDB_s *const mmdb) {
    /* In a pathological case of a database with a single node search tree,
     * this check will be true even after we've found the IPv4 start node, but
     * that doesn't seem worth trying to fix. */
    if (mmdb->ipv4_start_node.node_value != 0) {
        return MMDB_SUCCESS;
    }

    record_info_s record_info = record_info_for_database(mmdb);

    const uint8_t *search_tree = mmdb->file_content;
    uint32_t node_value = 0;
    const uint8_t *record_pointer;
    uint16_t netmask;
    uint32_t node_count = mmdb->metadata.node_count;

    for (netmask = 0; netmask < 96 && node_value < node_count; netmask++) {
        record_pointer = &search_tree[node_value * record_info.record_length];
        if (record_pointer + record_info.record_length > mmdb->data_section) {
            return MMDB_CORRUPT_SEARCH_TREE_ERROR;
        }
        node_value = record_info.left_record_getter(record_pointer);
    }

    mmdb->ipv4_start_node.node_value = node_value;
    mmdb->ipv4_start_node.netmask = netmask;

    return MMDB_SUCCESS;
}

static uint8_t record_type(const MMDB_s *const mmdb, uint64_t record) {
    uint32_t node_count = mmdb->metadata.node_count;

    /* Ideally we'd check to make sure that a record never points to a
     * previously seen value, but that's more complicated. For now, we can
     * at least check that we don't end up at the top of the tree again. */
    if (record == 0) {
        DEBUG_MSG("record has a value of 0");
        return MMDB_RECORD_TYPE_INVALID;
    }

    if (record < node_count) {
        return MMDB_RECORD_TYPE_SEARCH_NODE;
    }

    if (record == node_count) {
        return MMDB_RECORD_TYPE_EMPTY;
    }

    if (record - node_count < mmdb->data_section_size) {
        return MMDB_RECORD_TYPE_DATA;
    }

    DEBUG_MSG("record has a value that points outside of the database");
    return MMDB_RECORD_TYPE_INVALID;
}

static uint32_t get_left_28_bit_record(const uint8_t *record) {
    return record[0] * 65536 + record[1] * 256 + record[2] +
           ((record[3] & 0xf0) << 20);
}

static uint32_t get_right_28_bit_record(const uint8_t *record) {
    uint32_t value = get_uint32(record);
    return value & 0xfffffff;
}

int MMDB_read_node(const MMDB_s *const mmdb,
                   uint32_t node_number,
                   MMDB_search_node_s *const node) {
    record_info_s record_info = record_info_for_database(mmdb);
    if (0 == record_info.right_record_offset) {
        return MMDB_UNKNOWN_DATABASE_FORMAT_ERROR;
    }

    if (node_number > mmdb->metadata.node_count) {
        return MMDB_INVALID_NODE_NUMBER_ERROR;
    }

    const uint8_t *search_tree = mmdb->file_content;
    const uint8_t *record_pointer =
        &search_tree[node_number * record_info.record_length];
    node->left_record = record_info.left_record_getter(record_pointer);
    record_pointer += record_info.right_record_offset;
    node->right_record = record_info.right_record_getter(record_pointer);

    node->left_record_type = record_type(mmdb, node->left_record);
    node->right_record_type = record_type(mmdb, node->right_record);

    // Note that offset will be invalid if the record type is not
    // MMDB_RECORD_TYPE_DATA, but that's ok. Any use of the record entry
    // for other data types is a programming error.
    node->left_record_entry = (struct MMDB_entry_s){
        .mmdb = mmdb,
        .offset = data_section_offset_for_record(mmdb, node->left_record),
    };
    node->right_record_entry = (struct MMDB_entry_s){
        .mmdb = mmdb,
        .offset = data_section_offset_for_record(mmdb, node->right_record),
    };

    return MMDB_SUCCESS;
}

static uint32_t data_section_offset_for_record(const MMDB_s *const mmdb,
                                               uint64_t record) {
    return (uint32_t)record - mmdb->metadata.node_count -
           MMDB_DATA_SECTION_SEPARATOR;
}

int MMDB_get_value(MMDB_entry_s *const start,
                   MMDB_entry_data_s *const entry_data,
                   ...) {
    va_list path;
    va_start(path, entry_data);
    int status = MMDB_vget_value(start, entry_data, path);
    va_end(path);
    return status;
}

int MMDB_vget_value(MMDB_entry_s *const start,
                    MMDB_entry_data_s *const entry_data,
                    va_list va_path) {
    int length = path_length(va_path);
    const char *path_elem;
    int i = 0;

    MAYBE_CHECK_SIZE_OVERFLOW(length,
                              SIZE_MAX / sizeof(const char *) - 1,
                              MMDB_INVALID_METADATA_ERROR);

    const char **path = calloc(length + 1, sizeof(const char *));
    if (NULL == path) {
        return MMDB_OUT_OF_MEMORY_ERROR;
    }

    while (NULL != (path_elem = va_arg(va_path, char *))) {
        path[i] = path_elem;
        i++;
    }
    path[i] = NULL;

    int status = MMDB_aget_value(start, entry_data, path);

    free((char **)path);

    return status;
}

static int path_length(va_list va_path) {
    int i = 0;
    const char *ignore;
    va_list path_copy;
    va_copy(path_copy, va_path);

    while (NULL != (ignore = va_arg(path_copy, char *))) {
        i++;
    }

    va_end(path_copy);

    return i;
}

int MMDB_aget_value(MMDB_entry_s *const start,
                    MMDB_entry_data_s *const entry_data,
                    const char *const *const path) {
    const MMDB_s *const mmdb = start->mmdb;
    uint32_t offset = start->offset;

    memset(entry_data, 0, sizeof(MMDB_entry_data_s));
    DEBUG_NL;
    DEBUG_MSG("looking up value by path");

    CHECKED_DECODE_ONE_FOLLOW(mmdb, offset, entry_data);

    DEBUG_NL;
    DEBUG_MSGF("top level element is a %s", type_num_to_name(entry_data->type));

    /* Can this happen? It'd probably represent a pathological case under
     * normal use, but there's nothing preventing someone from passing an
     * invalid MMDB_entry_s struct to this function */
    if (!entry_data->has_data) {
        return MMDB_INVALID_LOOKUP_PATH_ERROR;
    }

    const char *path_elem;
    int i = 0;
    while (NULL != (path_elem = path[i++])) {
        DEBUG_NL;
        DEBUG_MSGF("path elem = %s", path_elem);

        /* XXX - it'd be good to find a quicker way to skip through these
           entries that doesn't involve decoding them
           completely. Basically we need to just use the size from the
           control byte to advance our pointer rather than calling
           decode_one(). */
        if (entry_data->type == MMDB_DATA_TYPE_ARRAY) {
            int status = lookup_path_in_array(path_elem, mmdb, entry_data);
            if (MMDB_SUCCESS != status) {
                memset(entry_data, 0, sizeof(MMDB_entry_data_s));
                return status;
            }
        } else if (entry_data->type == MMDB_DATA_TYPE_MAP) {
            int status = lookup_path_in_map(path_elem, mmdb, entry_data);
            if (MMDB_SUCCESS != status) {
                memset(entry_data, 0, sizeof(MMDB_entry_data_s));
                return status;
            }
        } else {
            /* Once we make the code traverse maps & arrays without calling
             * decode_one() we can get rid of this. */
            memset(entry_data, 0, sizeof(MMDB_entry_data_s));
            return MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR;
        }
    }

    return MMDB_SUCCESS;
}

static int lookup_path_in_array(const char *path_elem,
                                const MMDB_s *const mmdb,
                                MMDB_entry_data_s *entry_data) {
    uint32_t size = entry_data->data_size;
    char *first_invalid;

    int saved_errno = errno;
    errno = 0;
    int array_index = strtol(path_elem, &first_invalid, 10);
    if (ERANGE == errno) {
        errno = saved_errno;
        return MMDB_INVALID_LOOKUP_PATH_ERROR;
    }
    errno = saved_errno;

    if (array_index < 0) {
        array_index += size;

        if (array_index < 0) {
            return MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR;
        }
    }

    if (*first_invalid || (uint32_t)array_index >= size) {
        return MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR;
    }

    for (int i = 0; i < array_index; i++) {
        /* We don't want to follow a pointer here. If the next element is a
         * pointer we simply skip it and keep going */
        CHECKED_DECODE_ONE(mmdb, entry_data->offset_to_next, entry_data);
        int status = skip_map_or_array(mmdb, entry_data);
        if (MMDB_SUCCESS != status) {
            return status;
        }
    }

    MMDB_entry_data_s value;
    CHECKED_DECODE_ONE_FOLLOW(mmdb, entry_data->offset_to_next, &value);
    memcpy(entry_data, &value, sizeof(MMDB_entry_data_s));

    return MMDB_SUCCESS;
}

static int lookup_path_in_map(const char *path_elem,
                              const MMDB_s *const mmdb,
                              MMDB_entry_data_s *entry_data) {
    uint32_t size = entry_data->data_size;
    uint32_t offset = entry_data->offset_to_next;
    size_t path_elem_len = strlen(path_elem);

    while (size-- > 0) {
        MMDB_entry_data_s key, value;
        CHECKED_DECODE_ONE_FOLLOW(mmdb, offset, &key);

        uint32_t offset_to_value = key.offset_to_next;

        if (MMDB_DATA_TYPE_UTF8_STRING != key.type) {
            return MMDB_INVALID_DATA_ERROR;
        }

        if (key.data_size == path_elem_len &&
            !memcmp(path_elem, key.utf8_string, path_elem_len)) {

            DEBUG_MSG("found key matching path elem");

            CHECKED_DECODE_ONE_FOLLOW(mmdb, offset_to_value, &value);
            memcpy(entry_data, &value, sizeof(MMDB_entry_data_s));
            return MMDB_SUCCESS;
        } else {
            /* We don't want to follow a pointer here. If the next element is
             * a pointer we simply skip it and keep going */
            CHECKED_DECODE_ONE(mmdb, offset_to_value, &value);
            int status = skip_map_or_array(mmdb, &value);
            if (MMDB_SUCCESS != status) {
                return status;
            }
            offset = value.offset_to_next;
        }
    }

    memset(entry_data, 0, sizeof(MMDB_entry_data_s));
    return MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR;
}

static int skip_map_or_array(const MMDB_s *const mmdb,
                             MMDB_entry_data_s *entry_data) {
    if (entry_data->type == MMDB_DATA_TYPE_MAP) {
        uint32_t size = entry_data->data_size;
        while (size-- > 0) {
            CHECKED_DECODE_ONE(
                mmdb, entry_data->offset_to_next, entry_data); // key
            CHECKED_DECODE_ONE(
                mmdb, entry_data->offset_to_next, entry_data); // value
            int status = skip_map_or_array(mmdb, entry_data);
            if (MMDB_SUCCESS != status) {
                return status;
            }
        }
    } else if (entry_data->type == MMDB_DATA_TYPE_ARRAY) {
        uint32_t size = entry_data->data_size;
        while (size-- > 0) {
            CHECKED_DECODE_ONE(
                mmdb, entry_data->offset_to_next, entry_data); // value
            int status = skip_map_or_array(mmdb, entry_data);
            if (MMDB_SUCCESS != status) {
                return status;
            }
        }
    }

    return MMDB_SUCCESS;
}

static int decode_one_follow(const MMDB_s *const mmdb,
                             uint32_t offset,
                             MMDB_entry_data_s *entry_data) {
    CHECKED_DECODE_ONE(mmdb, offset, entry_data);
    if (entry_data->type == MMDB_DATA_TYPE_POINTER) {
        uint32_t next = entry_data->offset_to_next;
        CHECKED_DECODE_ONE(mmdb, entry_data->pointer, entry_data);
        /* Pointers to pointers are illegal under the spec */
        if (entry_data->type == MMDB_DATA_TYPE_POINTER) {
            DEBUG_MSG("pointer points to another pointer");
            return MMDB_INVALID_DATA_ERROR;
        }

        /* The pointer could point to any part of the data section but the
         * next entry for this particular offset may be the one after the
         * pointer, not the one after whatever the pointer points to. This
         * depends on whether the pointer points to something that is a simple
         * value or a compound value. For a compound value, the next one is
         * the one after the pointer result, not the one after the pointer. */
        if (entry_data->type != MMDB_DATA_TYPE_MAP &&
            entry_data->type != MMDB_DATA_TYPE_ARRAY) {

            entry_data->offset_to_next = next;
        }
    }

    return MMDB_SUCCESS;
}

#if !MMDB_UINT128_IS_BYTE_ARRAY
static mmdb_uint128_t get_uint128(const uint8_t *p, int length) {
    mmdb_uint128_t value = 0;
    while (length-- > 0) {
        value <<= 8;
        value += *p++;
    }
    return value;
}
#endif

static int decode_one(const MMDB_s *const mmdb,
                      uint32_t offset,
                      MMDB_entry_data_s *entry_data) {
    const uint8_t *mem = mmdb->data_section;

    // We subtract rather than add as it possible that offset + 1
    // could overflow for a corrupt database while an underflow
    // from data_section_size - 1 should not be possible.
    if (offset > mmdb->data_section_size - 1) {
        DEBUG_MSGF("Offset (%d) past data section (%d)",
                   offset,
                   mmdb->data_section_size);
        return MMDB_INVALID_DATA_ERROR;
    }

    entry_data->offset = offset;
    entry_data->has_data = true;

    DEBUG_NL;
    DEBUG_MSGF("Offset: %i", offset);

    uint8_t ctrl = mem[offset++];
    DEBUG_BINARY("Control byte: %s", ctrl);

    int type = (ctrl >> 5) & 7;
    DEBUG_MSGF("Type: %i (%s)", type, type_num_to_name(type));

    if (type == MMDB_DATA_TYPE_EXTENDED) {
        // Subtracting 1 to avoid possible overflow on offset + 1
        if (offset > mmdb->data_section_size - 1) {
            DEBUG_MSGF("Extended type offset (%d) past data section (%d)",
                       offset,
                       mmdb->data_section_size);
            return MMDB_INVALID_DATA_ERROR;
        }
        type = get_ext_type(mem[offset++]);
        DEBUG_MSGF("Extended type: %i (%s)", type, type_num_to_name(type));
    }

    entry_data->type = type;

    if (type == MMDB_DATA_TYPE_POINTER) {
        uint8_t psize = ((ctrl >> 3) & 3) + 1;
        DEBUG_MSGF("Pointer size: %i", psize);

        // We check that the offset does not extend past the end of the
        // database and that the subtraction of psize did not underflow.
        if (offset > mmdb->data_section_size - psize ||
            mmdb->data_section_size < psize) {
            DEBUG_MSGF("Pointer offset (%d) past data section (%d)",
                       offset + psize,
                       mmdb->data_section_size);
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->pointer = get_ptr_from(ctrl, &mem[offset], psize);
        DEBUG_MSGF("Pointer to: %i", entry_data->pointer);

        entry_data->data_size = psize;
        entry_data->offset_to_next = offset + psize;
        return MMDB_SUCCESS;
    }

    uint32_t size = ctrl & 31;
    switch (size) {
        case 29:
            // We subtract when checking offset to avoid possible overflow
            if (offset > mmdb->data_section_size - 1) {
                DEBUG_MSGF("String end (%d, case 29) past data section (%d)",
                           offset,
                           mmdb->data_section_size);
                return MMDB_INVALID_DATA_ERROR;
            }
            size = 29 + mem[offset++];
            break;
        case 30:
            // We subtract when checking offset to avoid possible overflow
            if (offset > mmdb->data_section_size - 2) {
                DEBUG_MSGF("String end (%d, case 30) past data section (%d)",
                           offset,
                           mmdb->data_section_size);
                return MMDB_INVALID_DATA_ERROR;
            }
            size = 285 + get_uint16(&mem[offset]);
            offset += 2;
            break;
        case 31:
            // We subtract when checking offset to avoid possible overflow
            if (offset > mmdb->data_section_size - 3) {
                DEBUG_MSGF("String end (%d, case 31) past data section (%d)",
                           offset,
                           mmdb->data_section_size);
                return MMDB_INVALID_DATA_ERROR;
            }
            size = 65821 + get_uint24(&mem[offset]);
            offset += 3;
        default:
            break;
    }

    DEBUG_MSGF("Size: %i", size);

    if (type == MMDB_DATA_TYPE_MAP || type == MMDB_DATA_TYPE_ARRAY) {
        entry_data->data_size = size;
        entry_data->offset_to_next = offset;
        return MMDB_SUCCESS;
    }

    if (type == MMDB_DATA_TYPE_BOOLEAN) {
        entry_data->boolean = size ? true : false;
        entry_data->data_size = 0;
        entry_data->offset_to_next = offset;
        DEBUG_MSGF("boolean value: %s", entry_data->boolean ? "true" : "false");
        return MMDB_SUCCESS;
    }

    // Check that the data doesn't extend past the end of the memory
    // buffer and that the calculation in doing this did not underflow.
    if (offset > mmdb->data_section_size - size ||
        mmdb->data_section_size < size) {
        DEBUG_MSGF("Data end (%d) past data section (%d)",
                   offset + size,
                   mmdb->data_section_size);
        return MMDB_INVALID_DATA_ERROR;
    }

    if (type == MMDB_DATA_TYPE_UINT16) {
        if (size > 2) {
            DEBUG_MSGF("uint16 of size %d", size);
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->uint16 = (uint16_t)get_uintX(&mem[offset], size);
        DEBUG_MSGF("uint16 value: %u", entry_data->uint16);
    } else if (type == MMDB_DATA_TYPE_UINT32) {
        if (size > 4) {
            DEBUG_MSGF("uint32 of size %d", size);
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->uint32 = (uint32_t)get_uintX(&mem[offset], size);
        DEBUG_MSGF("uint32 value: %u", entry_data->uint32);
    } else if (type == MMDB_DATA_TYPE_INT32) {
        if (size > 4) {
            DEBUG_MSGF("int32 of size %d", size);
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->int32 = get_sintX(&mem[offset], size);
        DEBUG_MSGF("int32 value: %i", entry_data->int32);
    } else if (type == MMDB_DATA_TYPE_UINT64) {
        if (size > 8) {
            DEBUG_MSGF("uint64 of size %d", size);
            return MMDB_INVALID_DATA_ERROR;
        }
        entry_data->uint64 = get_uintX(&mem[offset], size);
        DEBUG_MSGF("uint64 value: %" PRIu64, entry_data->uint64);
    } else if (type == MMDB_DATA_TYPE_UINT128) {
        if (size > 16) {
            DEBUG_MSGF("uint128 of size %d", size);
            return MMDB_INVALID_DATA_ERROR;
        }
#if MMDB_UINT128_IS_BYTE_ARRAY
        memset(entry_data->uint128, 0, 16);
        if (size > 0) {
            memcpy(entry_data->uint128 + 16 - size, &mem[offset], size);
        }
#else
        entry_data->uint128 = get_uint128(&mem[offset], size);
#endif
    } else if (type == MMDB_DATA_TYPE_FLOAT) {
        if (size != 4) {
            DEBUG_MSGF("float of size %d", size);
            return MMDB_INVALID_DATA_ERROR;
        }
        size = 4;
        entry_data->float_value = get_ieee754_float(&mem[offset]);
        DEBUG_MSGF("float value: %f", entry_data->float_value);
    } else if (type == MMDB_DATA_TYPE_DOUBLE) {
        if (size != 8) {
            DEBUG_MSGF("double of size %d", size);
            return MMDB_INVALID_DATA_ERROR;
        }
        size = 8;
        entry_data->double_value = get_ieee754_double(&mem[offset]);
        DEBUG_MSGF("double value: %f", entry_data->double_value);
    } else if (type == MMDB_DATA_TYPE_UTF8_STRING) {
        entry_data->utf8_string = size == 0 ? "" : (char *)&mem[offset];
        entry_data->data_size = size;
#ifdef MMDB_DEBUG
        char *string =
            mmdb_strndup(entry_data->utf8_string, size > 50 ? 50 : size);
        if (NULL == string) {
            abort();
        }
        DEBUG_MSGF("string value: %s", string);
        free(string);
#endif
    } else if (type == MMDB_DATA_TYPE_BYTES) {
        entry_data->bytes = &mem[offset];
        entry_data->data_size = size;
    }

    entry_data->offset_to_next = offset + size;

    return MMDB_SUCCESS;
}

static int get_ext_type(int raw_ext_type) { return 7 + raw_ext_type; }

static uint32_t
get_ptr_from(uint8_t ctrl, uint8_t const *const ptr, int ptr_size) {
    uint32_t new_offset;
    switch (ptr_size) {
        case 1:
            new_offset = ((ctrl & 7) << 8) + ptr[0];
            break;
        case 2:
            new_offset = 2048 + ((ctrl & 7) << 16) + (ptr[0] << 8) + ptr[1];
            break;
        case 3:
            new_offset = 2048 + 524288 + ((ctrl & 7) << 24) + get_uint24(ptr);
            break;
        case 4:
        default:
            new_offset = get_uint32(ptr);
            break;
    }
    return new_offset;
}

int MMDB_get_metadata_as_entry_data_list(
    const MMDB_s *const mmdb, MMDB_entry_data_list_s **const entry_data_list) {
    MMDB_s metadata_db = make_fake_metadata_db(mmdb);

    MMDB_entry_s metadata_start = {.mmdb = &metadata_db, .offset = 0};

    return MMDB_get_entry_data_list(&metadata_start, entry_data_list);
}

int MMDB_get_entry_data_list(MMDB_entry_s *start,
                             MMDB_entry_data_list_s **const entry_data_list) {
    MMDB_data_pool_s *const pool = data_pool_new(MMDB_POOL_INIT_SIZE);
    if (!pool) {
        return MMDB_OUT_OF_MEMORY_ERROR;
    }

    MMDB_entry_data_list_s *const list = data_pool_alloc(pool);
    if (!list) {
        data_pool_destroy(pool);
        return MMDB_OUT_OF_MEMORY_ERROR;
    }

    int const status =
        get_entry_data_list(start->mmdb, start->offset, list, pool, 0);

    *entry_data_list = data_pool_to_list(pool);
    if (!*entry_data_list) {
        data_pool_destroy(pool);
        return MMDB_OUT_OF_MEMORY_ERROR;
    }

    return status;
}

static int get_entry_data_list(const MMDB_s *const mmdb,
                               uint32_t offset,
                               MMDB_entry_data_list_s *const entry_data_list,
                               MMDB_data_pool_s *const pool,
                               int depth) {
    if (depth >= MAXIMUM_DATA_STRUCTURE_DEPTH) {
        DEBUG_MSG("reached the maximum data structure depth");
        return MMDB_INVALID_DATA_ERROR;
    }
    depth++;
    CHECKED_DECODE_ONE(mmdb, offset, &entry_data_list->entry_data);

    switch (entry_data_list->entry_data.type) {
        case MMDB_DATA_TYPE_POINTER: {
            uint32_t next_offset = entry_data_list->entry_data.offset_to_next;
            uint32_t last_offset;
            CHECKED_DECODE_ONE(mmdb,
                               last_offset =
                                   entry_data_list->entry_data.pointer,
                               &entry_data_list->entry_data);

            /* Pointers to pointers are illegal under the spec */
            if (entry_data_list->entry_data.type == MMDB_DATA_TYPE_POINTER) {
                DEBUG_MSG("pointer points to another pointer");
                return MMDB_INVALID_DATA_ERROR;
            }

            if (entry_data_list->entry_data.type == MMDB_DATA_TYPE_ARRAY ||
                entry_data_list->entry_data.type == MMDB_DATA_TYPE_MAP) {

                int status = get_entry_data_list(
                    mmdb, last_offset, entry_data_list, pool, depth);
                if (MMDB_SUCCESS != status) {
                    DEBUG_MSG("get_entry_data_list on pointer failed.");
                    return status;
                }
            }
            entry_data_list->entry_data.offset_to_next = next_offset;
        } break;
        case MMDB_DATA_TYPE_ARRAY: {
            uint32_t array_size = entry_data_list->entry_data.data_size;
            uint32_t array_offset = entry_data_list->entry_data.offset_to_next;
            while (array_size-- > 0) {
                MMDB_entry_data_list_s *entry_data_list_to =
                    data_pool_alloc(pool);
                if (!entry_data_list_to) {
                    return MMDB_OUT_OF_MEMORY_ERROR;
                }

                int status = get_entry_data_list(
                    mmdb, array_offset, entry_data_list_to, pool, depth);
                if (MMDB_SUCCESS != status) {
                    DEBUG_MSG("get_entry_data_list on array element failed.");
                    return status;
                }

                array_offset = entry_data_list_to->entry_data.offset_to_next;
            }
            entry_data_list->entry_data.offset_to_next = array_offset;

        } break;
        case MMDB_DATA_TYPE_MAP: {
            uint32_t size = entry_data_list->entry_data.data_size;

            offset = entry_data_list->entry_data.offset_to_next;
            while (size-- > 0) {
                MMDB_entry_data_list_s *list_key = data_pool_alloc(pool);
                if (!list_key) {
                    return MMDB_OUT_OF_MEMORY_ERROR;
                }

                int status =
                    get_entry_data_list(mmdb, offset, list_key, pool, depth);
                if (MMDB_SUCCESS != status) {
                    DEBUG_MSG("get_entry_data_list on map key failed.");
                    return status;
                }

                offset = list_key->entry_data.offset_to_next;

                MMDB_entry_data_list_s *list_value = data_pool_alloc(pool);
                if (!list_value) {
                    return MMDB_OUT_OF_MEMORY_ERROR;
                }

                status =
                    get_entry_data_list(mmdb, offset, list_value, pool, depth);
                if (MMDB_SUCCESS != status) {
                    DEBUG_MSG("get_entry_data_list on map element failed.");
                    return status;
                }
                offset = list_value->entry_data.offset_to_next;
            }
            entry_data_list->entry_data.offset_to_next = offset;
        } break;
        default:
            break;
    }

    return MMDB_SUCCESS;
}

static float get_ieee754_float(const uint8_t *restrict p) {
    volatile float f;
    uint8_t *q = (void *)&f;
/* Windows builds don't use autoconf but we can assume they're all
 * little-endian. */
#if MMDB_LITTLE_ENDIAN || _WIN32
    q[3] = p[0];
    q[2] = p[1];
    q[1] = p[2];
    q[0] = p[3];
#else
    memcpy(q, p, 4);
#endif
    return f;
}

static double get_ieee754_double(const uint8_t *restrict p) {
    volatile double d;
    uint8_t *q = (void *)&d;
#if MMDB_LITTLE_ENDIAN || _WIN32
    q[7] = p[0];
    q[6] = p[1];
    q[5] = p[2];
    q[4] = p[3];
    q[3] = p[4];
    q[2] = p[5];
    q[1] = p[6];
    q[0] = p[7];
#else
    memcpy(q, p, 8);
#endif

    return d;
}

static uint32_t get_uint32(const uint8_t *p) {
    return p[0] * 16777216U + p[1] * 65536 + p[2] * 256 + p[3];
}

static uint32_t get_uint24(const uint8_t *p) {
    return p[0] * 65536U + p[1] * 256 + p[2];
}

static uint32_t get_uint16(const uint8_t *p) { return p[0] * 256U + p[1]; }

static uint64_t get_uintX(const uint8_t *p, int length) {
    uint64_t value = 0;
    while (length-- > 0) {
        value <<= 8;
        value += *p++;
    }
    return value;
}

static int32_t get_sintX(const uint8_t *p, int length) {
    return (int32_t)get_uintX(p, length);
}

void MMDB_free_entry_data_list(MMDB_entry_data_list_s *const entry_data_list) {
    if (entry_data_list == NULL) {
        return;
    }
    data_pool_destroy(entry_data_list->pool);
}

void MMDB_close(MMDB_s *const mmdb) { free_mmdb_struct(mmdb); }

static void free_mmdb_struct(MMDB_s *const mmdb) {
    if (!mmdb) {
        return;
    }

    //if (NULL != mmdb->filename) {
    //    FREE_AND_SET_NULL(mmdb->filename);
    //}
    if (NULL != mmdb->file_content) {
#ifdef _WIN32
        NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)mmdb->file_content);
        /* Winsock is only initialized if open was successful so we only have
         * to cleanup then. */
        //WSACleanup();
#else
        munmap((void *)mmdb->file_content, mmdb->file_size);
#endif
    }

    if (NULL != mmdb->metadata.database_type) {
        FREE_AND_SET_NULL(mmdb->metadata.database_type);
    }

    free_languages_metadata(mmdb);
    free_descriptions_metadata(mmdb);
}

static void free_languages_metadata(MMDB_s *mmdb) {
    if (!mmdb->metadata.languages.names) {
        return;
    }

    for (size_t i = 0; i < mmdb->metadata.languages.count; i++) {
        FREE_AND_SET_NULL(mmdb->metadata.languages.names[i]);
    }
    FREE_AND_SET_NULL(mmdb->metadata.languages.names);
}

static void free_descriptions_metadata(MMDB_s *mmdb) {
    if (!mmdb->metadata.description.count) {
        return;
    }

    for (size_t i = 0; i < mmdb->metadata.description.count; i++) {
        if (NULL != mmdb->metadata.description.descriptions[i]) {
            if (NULL != mmdb->metadata.description.descriptions[i]->language) {
                FREE_AND_SET_NULL(
                    mmdb->metadata.description.descriptions[i]->language);
            }

            if (NULL !=
                mmdb->metadata.description.descriptions[i]->description) {
                FREE_AND_SET_NULL(
                    mmdb->metadata.description.descriptions[i]->description);
            }
            FREE_AND_SET_NULL(mmdb->metadata.description.descriptions[i]);
        }
    }

    FREE_AND_SET_NULL(mmdb->metadata.description.descriptions);
}

const char *MMDB_lib_version(void) { return PACKAGE_VERSION; }

int MMDB_dump_entry_data_list(FILE *const stream,
                              MMDB_entry_data_list_s *const entry_data_list,
                              int indent) {
    int status;
    dump_entry_data_list(stream, entry_data_list, indent, &status);
    return status;
}

static MMDB_entry_data_list_s *
dump_entry_data_list(FILE *stream,
                     MMDB_entry_data_list_s *entry_data_list,
                     int indent,
                     int *status) {
    switch (entry_data_list->entry_data.type) {
        case MMDB_DATA_TYPE_MAP: {
            uint32_t size = entry_data_list->entry_data.data_size;

            print_indentation(stream, indent);
            fprintf(stream, "{\n");
            indent += 2;

            for (entry_data_list = entry_data_list->next;
                 size && entry_data_list;
                 size--) {

                if (MMDB_DATA_TYPE_UTF8_STRING !=
                    entry_data_list->entry_data.type) {
                    *status = MMDB_INVALID_DATA_ERROR;
                    return NULL;
                }
                char *key = mmdb_strndup(
                    (char *)entry_data_list->entry_data.utf8_string,
                    entry_data_list->entry_data.data_size);
                if (NULL == key) {
                    *status = MMDB_OUT_OF_MEMORY_ERROR;
                    return NULL;
                }

                print_indentation(stream, indent);
                fprintf(stream, "\"%s\": \n", key);
                free(key);

                entry_data_list = entry_data_list->next;
                entry_data_list = dump_entry_data_list(
                    stream, entry_data_list, indent + 2, status);

                if (MMDB_SUCCESS != *status) {
                    return NULL;
                }
            }

            indent -= 2;
            print_indentation(stream, indent);
            fprintf(stream, "}\n");
        } break;
        case MMDB_DATA_TYPE_ARRAY: {
            uint32_t size = entry_data_list->entry_data.data_size;

            print_indentation(stream, indent);
            fprintf(stream, "[\n");
            indent += 2;

            for (entry_data_list = entry_data_list->next;
                 size && entry_data_list;
                 size--) {
                entry_data_list = dump_entry_data_list(
                    stream, entry_data_list, indent, status);
                if (MMDB_SUCCESS != *status) {
                    return NULL;
                }
            }

            indent -= 2;
            print_indentation(stream, indent);
            fprintf(stream, "]\n");
        } break;
        case MMDB_DATA_TYPE_UTF8_STRING: {
            char *string =
                mmdb_strndup((char *)entry_data_list->entry_data.utf8_string,
                             entry_data_list->entry_data.data_size);
            if (NULL == string) {
                *status = MMDB_OUT_OF_MEMORY_ERROR;
                return NULL;
            }
            print_indentation(stream, indent);
            fprintf(stream, "\"%s\" <utf8_string>\n", string);
            free(string);
            entry_data_list = entry_data_list->next;
        } break;
        case MMDB_DATA_TYPE_BYTES: {
            char *hex_string =
                bytes_to_hex((uint8_t *)entry_data_list->entry_data.bytes,
                             entry_data_list->entry_data.data_size);

            if (NULL == hex_string) {
                *status = MMDB_OUT_OF_MEMORY_ERROR;
                return NULL;
            }

            print_indentation(stream, indent);
            fprintf(stream, "%s <bytes>\n", hex_string);
            free(hex_string);

            entry_data_list = entry_data_list->next;
        } break;
        case MMDB_DATA_TYPE_DOUBLE:
            print_indentation(stream, indent);
            fprintf(stream,
                    "%f <double>\n",
                    entry_data_list->entry_data.double_value);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_FLOAT:
            print_indentation(stream, indent);
            fprintf(stream,
                    "%f <float>\n",
                    entry_data_list->entry_data.float_value);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_UINT16:
            print_indentation(stream, indent);
            fprintf(
                stream, "%u <uint16>\n", entry_data_list->entry_data.uint16);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_UINT32:
            print_indentation(stream, indent);
            fprintf(
                stream, "%u <uint32>\n", entry_data_list->entry_data.uint32);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_BOOLEAN:
            print_indentation(stream, indent);
            fprintf(stream,
                    "%s <boolean>\n",
                    entry_data_list->entry_data.boolean ? "true" : "false");
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_UINT64:
            print_indentation(stream, indent);
            fprintf(stream,
                    "%" PRIu64 " <uint64>\n",
                    entry_data_list->entry_data.uint64);
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_UINT128:
            print_indentation(stream, indent);
#if MMDB_UINT128_IS_BYTE_ARRAY
            char *hex_string = bytes_to_hex(
                (uint8_t *)entry_data_list->entry_data.uint128, 16);
            if (NULL == hex_string) {
                *status = MMDB_OUT_OF_MEMORY_ERROR;
                return NULL;
            }
            fprintf(stream, "0x%s <uint128>\n", hex_string);
            free(hex_string);
#else
            uint64_t high = entry_data_list->entry_data.uint128 >> 64;
            uint64_t low = (uint64_t)entry_data_list->entry_data.uint128;
            fprintf(stream,
                    "0x%016" PRIX64 "%016" PRIX64 " <uint128>\n",
                    high,
                    low);
#endif
            entry_data_list = entry_data_list->next;
            break;
        case MMDB_DATA_TYPE_INT32:
            print_indentation(stream, indent);
            fprintf(stream, "%d <int32>\n", entry_data_list->entry_data.int32);
            entry_data_list = entry_data_list->next;
            break;
        default:
            *status = MMDB_INVALID_DATA_ERROR;
            return NULL;
    }

    *status = MMDB_SUCCESS;
    return entry_data_list;
}

static void print_indentation(FILE *stream, int i) {
    char buffer[1024];
    int size = i >= 1024 ? 1023 : i;
    memset(buffer, 32, size);
    buffer[size] = '\0';
    fputs(buffer, stream);
}

#pragma warning(push)
#pragma warning(disable : 4996)
static char *bytes_to_hex(uint8_t *bytes, uint32_t size) {
    char *hex_string;
    MAYBE_CHECK_SIZE_OVERFLOW(size, SIZE_MAX / 2 - 1, NULL);

    hex_string = calloc((size * 2) + 1, sizeof(char));
    if (NULL == hex_string) {
        return NULL;
    }

    for (uint32_t i = 0; i < size; i++) {
        sprintf(hex_string + (2 * i), "%02X", bytes[i]);
    }

    return hex_string;
}
#pragma warning(pop)

const char *MMDB_strerror(int error_code) {
    switch (error_code) {
        case MMDB_SUCCESS:
            return "Success (not an error)";
        case MMDB_FILE_OPEN_ERROR:
            return "Error opening the specified MaxMind DB file";
        case MMDB_CORRUPT_SEARCH_TREE_ERROR:
            return "The MaxMind DB file's search tree is corrupt";
        case MMDB_INVALID_METADATA_ERROR:
            return "The MaxMind DB file contains invalid metadata";
        case MMDB_IO_ERROR:
            return "An attempt to read data from the MaxMind DB file failed";
        case MMDB_OUT_OF_MEMORY_ERROR:
            return "A memory allocation call failed";
        case MMDB_UNKNOWN_DATABASE_FORMAT_ERROR:
            return "The MaxMind DB file is in a format this library can't "
                   "handle (unknown record size or binary format version)";
        case MMDB_INVALID_DATA_ERROR:
            return "The MaxMind DB file's data section contains bad data "
                   "(unknown data type or corrupt data)";
        case MMDB_INVALID_LOOKUP_PATH_ERROR:
            return "The lookup path contained an invalid value (like a "
                   "negative integer for an array index)";
        case MMDB_LOOKUP_PATH_DOES_NOT_MATCH_DATA_ERROR:
            return "The lookup path does not match the data (key that doesn't "
                   "exist, array index bigger than the array, expected array "
                   "or map where none exists)";
        case MMDB_INVALID_NODE_NUMBER_ERROR:
            return "The MMDB_read_node function was called with a node number "
                   "that does not exist in the search tree";
        case MMDB_IPV6_LOOKUP_IN_IPV4_DATABASE_ERROR:
            return "You attempted to look up an IPv6 address in an IPv4-only "
                   "database";
        default:
            return "Unknown error code";
    }
}
