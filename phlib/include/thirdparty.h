/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2024
 *
 */

#pragma once

#define PCRE2_CODE_UNIT_WIDTH 16

#if __has_include("../../tools/thirdparty/pcre/pcre2.h")
#include "../../tools/thirdparty/pcre/pcre2.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/jsonc/json.h")
#include "../../tools/thirdparty/jsonc/json.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/mxml/mxml.h")
#include "../../tools/thirdparty/mxml/mxml.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/detours/detours.h")
#include "../../tools/thirdparty/detours/detours.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/md5/md5.h")
#include "../../tools/thirdparty/md5/md5.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/sha/sha.h")
#include "../../tools/thirdparty/sha/sha.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/sha256/sha256.h")
#include "../../tools/thirdparty/sha256/sha256.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/winsdk/dia2.h")
#include "../../tools/thirdparty/winsdk/dia2.h"
#else
#error "ThirdParty.lib is mssing"
#endif

#if __has_include("../../tools/thirdparty/winsdk/dia3.h")
#include "../../tools/thirdparty/winsdk/dia3.h"
#else
#error "ThirdParty.lib is mssing"
#endif
