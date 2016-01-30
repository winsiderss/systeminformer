/*
 * Copyright (c) 2012 Eric Haszlakiewicz
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 */

#ifndef _json_c_version_h_
#define _json_c_version_h_

#define JSON_C_MAJOR_VERSION 0
#define JSON_C_MINOR_VERSION 12
#define JSON_C_MICRO_VERSION 0
#define JSON_C_VERSION_NUM ((JSON_C_MAJOR_VERSION << 16) | \
                            (JSON_C_MINOR_VERSION << 8) | \
                            JSON_C_MICRO_VERSION)
#define JSON_C_VERSION "0.12"

const char *json_c_version(void); /* Returns JSON_C_VERSION */
int json_c_version_num(void);     /* Returns JSON_C_VERSION_NUM */

#endif
