/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2024
 *
 */

#pragma once

// alloc

#define KPH_TAG_PAGED_LOOKASIDE_OBJECT          '0ApK'
#define KPH_TAG_NPAGED_LOOKASIDE_OBJECT         '1ApK'

// comms

#define KPH_TAG_CLIENT                          '0CpK'
#define KPH_TAG_MESSAGE                         '1CpK'
#define KPH_TAG_NPAGED_MESSAGE                  '2CpK'
#define KPH_TAG_QUEUE_ITEM                      '3CpK'
#define KPH_TAG_THREAD_POOL                     '4CpK'

// dyndata

#define KPH_TAG_DYNDATA                         '0YpK'

// object

#define KPH_TAG_OBJECT_QUERY                    '0OpK'
#define KPH_TAG_OBJECT_INFO                     '1OpK'

// process

#define KPH_TAG_PROCESS_INFO                    '0PpK'

// thread

#define KPH_TAG_THREAD_BACK_TRACE               '0TpK'
#define KPH_TAG_THREAD_INFO                     '1TpK'

// util

#define KPH_TAG_REG_STRING                      '0UpK'
#define KPH_TAG_REG_BINARY                      '1UpK'
#define KPH_TAG_FILE_OBJECT_NAME                '2UpK'
#define KPH_TAG_CAPTURED_UNICODE_STRING         '3UpK'

// vm

#define KPH_TAG_COPY_VM                         '0vpK'
#define KPH_TAG_SECTION_QUERY                   '1vpK'
#define KPH_TAG_VM_QUERY                        '2vpK'

// debug

#define KPH_TAG_DBG_SLOTS                       '0dpK'

// hash

#define KPH_TAG_HASHING_CONTEXT                 '0HpK'
#define KPH_TAG_HASHING_INFRA                   '1HpK'
#define KPH_TAG_CAPTURED_HASHES                 '2HpK'

// verify

#define KPH_TAG_VERIFY_SIGNATURE                '0VpK'

// informer

#define KPH_TAG_OB_OBJECT_NAME                  '0IpK'
#define KPH_TAG_PROCESS_CREATE_APC              '1IpK'
#define KPH_TAG_FLT_STREAMHANDLE_CONTEXT        '2IpK'
#define KPH_TAG_FLT_FILE_NAME                   '3IpK'
#define KPH_TAG_FLT_CACHED_FILE_NAME            '4IpK'
#define KPH_TAG_FLT_COMPLETION_CONTEXT          '5IpK'
#define KPH_TAG_REG_CALL_CONTEXT                '6IpK'
#define KPH_TAG_REG_OBJECT_NAME                 '7IpK'
#define KPH_TAG_REG_VALUE_NAMES                 '8IpK'
#define KPH_TAG_OB_CALL_CONTEXT                 '9IpK'

// cid_tracking

#define KPH_TAG_CID_TABLE                       '0cpK'
#define KPH_TAG_CID_POPULATE                    '1cpK'
#define KPH_TAG_PROCESS_CONTEXT                 '2cpK'
#define KPH_TAG_THREAD_CONTEXT                  '3cpK'
#define KPH_TAG_CID_APC                         '4cpK'
#define KPH_TAG_PROCESS_IMAGE_FILE_NAME         '5cpK'

// protection

#define KPH_TAG_IMAGE_LOAD_APC                  '0ppK'

// alpc

#define KPH_TAG_ALPC_NAME_QUERY                 '0apK'
#define KPH_TAG_ALPC_QUERY                      '1apK'

// file

#define KPH_TAG_FILE_QUERY                      '0FpK'
#define KPH_TAG_VOL_FILE_QUERY                  '1FpK'

// back_trace

#define KPH_TAG_BACK_TRACE_OBJECT               '0BpK'

// session_token

#define KPH_TAG_SESSION_TOKEN_OBJECT            '0tpK'
#define KPH_TAG_SESSION_TOKEN_SIGNATURE         '1tpK'

// ringbuff

#define KPH_TAG_RING_BUFFER                     '0RpK'
