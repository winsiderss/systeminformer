/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#pragma once

// alloc

#define KPH_TAG_PAGED_LOOKASIDE_OBJECT          '0AcK'
#define KPH_TAG_NPAGED_LOOKASIDE_OBJECT         '1AcK'

// comms

#define KPH_TAG_CLIENT                          '0CpK'
#define KPH_TAG_MESSAGE                         '1CpK'
#define KPH_TAG_NPAGED_MESSAGE                  '2CpK'
#define KPH_TAG_QUEUE_ITEM                      '3CpK'
#define KPH_TAG_THREAD_POOL                     '4CpK'
#define KPH_TAG_CONNECT_PARAMTERS               '5CpK'

// dyndata

#define KPH_TAG_DYNDATA                         '6DpK'

// object

#define KPH_TAG_OBJECT_QUERY                    '0OpK'

// process

#define KPH_TAG_PROCESS_INFO                    '0PpK'

// thread

#define KPH_TAG_BACKTRACE                       '0TpK'
#define KPH_TAG_THREAD_INFO                     '1TpK'

// util

#define KPH_TAG_MODULES                         '0UpK'
#define KPH_TAG_FILE_NAME                       '1UpK'
#define KPH_TAG_REG_STRING                      '2UpK'
#define KPH_TAG_REG_BINARY                      '3UpK'
#define KPH_TAG_FILE_OBJECT_NAME                '4UpK'
#define KPH_TAG_ACL                             '5UpK'

// verify

#define KPH_TAG_VERIFY                          '0VpK'

// vm

#define KPH_TAG_COPY_VM                         '0vpK'

// debug

#define KPH_TAG_DBG_SLOTS                       '0dpK'

// hash

#define KPH_TAG_HASHING_BUFFER                  '0HpK'
#define KPH_TAG_AUTHENTICODE_SIG                '1HpK'
#define KPH_TAG_HASHING_INFRA                   '2HpK'

// sign

#define KPH_TAG_SIGNING_INFRA                   '0SpK'

// informer

#define KPH_TAG_INFORMER_OB_NAME                '0IpK'
#define KPH_TAG_PROCESS_CREATE_APC              '1IpK'

// cid_tracking

#define KPH_TAG_CID_TABLE                       '0cpK'
#define KPH_TAG_CID_POPULATE                    '1cpK'
#define KPH_TAG_PROCESS_CONTEXT                 '2cpK'
#define KPH_TAG_THREAD_CONTEXT                  '3cpK'

// protection

#define KPH_TAG_IMAGE_LOAD_APC                  '0ppK'
