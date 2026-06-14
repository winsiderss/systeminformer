#ifndef _json_c_number_fast_h_
#define _json_c_number_fast_h_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int json_c_parse_double(const unsigned char *buf, double *retval);
int json_c_parse_double_full(const unsigned char *buf, size_t len, double *retval);
int json_c_parse_double_string(const unsigned char *buf, double *retval);
int json_c_parse_int64(const unsigned char *buf, int64_t *retval);
int json_c_parse_uint64(const unsigned char *buf, uint64_t *retval);
size_t json_c_print_int64(unsigned char *buf, size_t len, int64_t value);
size_t json_c_print_uint64(unsigned char *buf, size_t len, uint64_t value);
int json_c_print_double(unsigned char *buf, size_t len, double value, size_t *retval);

#ifdef __cplusplus
}
#endif

#endif /* _json_c_number_fast_h_ */
