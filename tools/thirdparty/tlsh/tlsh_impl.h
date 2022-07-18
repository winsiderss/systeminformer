/*
 * TLSH is provided for use under two licenses: Apache OR BSD.
 * Users may opt to use either license depending on the license
 * restictions of the systems with which they plan to integrate
 * the TLSH code.
 */ 

/* ==============
 * Apache License
 * ==============
 * Copyright 2013 Trend Micro Incorporated
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ===========
 * BSD License
 * ===========
 * Copyright (c) 2013, Trend Micro Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined WINDOWS || defined MINGW
#include "tlsh_win_version.h"
#else
#include "tlsh_version.h"
#endif

#ifndef HEADER_TLSH_IMPL_H
#define HEADER_TLSH_IMPL_H

#define SLIDING_WND_SIZE  5

#define BUCKETS           256
#define Q_BITS            2    // 2 bits; quartile value 0, 1, 2, 3

// BUCKETS_256 & CHECKSUM_3B are compiler switches defined in CMakeLists.txt

#if defined BUCKETS_256
  #define EFF_BUCKETS         256
  #define CODE_SIZE           64   // 256 * 2 bits = 64 bytes
  #if defined CHECKSUM_3B
    #define INTERNAL_TLSH_STRING_LEN 138
    #define TLSH_CHECKSUM_LEN 3
    // defined in tlsh.h   #define TLSH_STRING_LEN   138  // 2 + 3 + 64 bytes = 138 hexidecimal chars
  #else
    #define INTERNAL_TLSH_STRING_LEN 134
    #define TLSH_CHECKSUM_LEN 1
    // defined in tlsh.h   #define TLSH_STRING_LEN   134  // 2 + 1 + 64 bytes = 134 hexidecimal chars
  #endif
#endif

#if defined BUCKETS_128
  #define EFF_BUCKETS         128
  #define CODE_SIZE           32   // 128 * 2 bits = 32 bytes
  #if defined CHECKSUM_3B
    #define INTERNAL_TLSH_STRING_LEN 74
    #define TLSH_CHECKSUM_LEN 3
    // defined in tlsh.h   #define TLSH_STRING_LEN   74   // 2 + 3 + 32 bytes = 74 hexidecimal chars
  #else
    #define INTERNAL_TLSH_STRING_LEN 70
    #define TLSH_CHECKSUM_LEN 1
    // defined in tlsh.h   #define TLSH_STRING_LEN   70   // 2 + 1 + 32 bytes = 70 hexidecimal chars
  #endif
#endif

#if defined BUCKETS_48
  #define INTERNAL_TLSH_STRING_LEN 33
  #define EFF_BUCKETS         48
  #define CODE_SIZE           12   // 48 * 2 bits = 12 bytes
  #define TLSH_CHECKSUM_LEN 1
  // defined in tlsh.h   #define TLSH_STRING_LEN   30   // 2 + 1 + 12 bytes = 30 hexidecimal chars
#endif

class TlshImpl
{
public:
    TlshImpl();
    ~TlshImpl();
public:
    void       update(const unsigned char* data, unsigned int len, int tlsh_option);
    void fast_update5(const unsigned char* data, unsigned int len, int tlsh_option);
    void final(int fc_cons_option = 0);
    void reset();
    const char* hash(int showvers) const;
    const char* hash(char *buffer, unsigned int bufSize, int showvers) const;  // saves allocating hash string in TLSH instance - bufSize should be TLSH_STRING_LEN + 1
    int compare(const TlshImpl& other) const;
    int totalDiff(const TlshImpl& other, bool len_diff=true) const;
    int Lvalue();
    int Q1ratio();
    int Q2ratio();
    int Checksum(int k);
    int BucketValue(int bucket);
    int HistogramCount(int bucket);
    int fromTlshStr(const char* str);
    bool isValid() const { return lsh_code_valid; }

private:
    unsigned int *a_bucket;  
    unsigned char slide_window[SLIDING_WND_SIZE];
    unsigned int data_len;
    
    struct lsh_bin_struct {
        unsigned char checksum[TLSH_CHECKSUM_LEN];  // 1 to 3 bytes
        unsigned char Lvalue;                       // 1 byte
        union {
#if defined(__SPARC) || defined(_AIX)
		#pragma pack(1)
#endif
        unsigned char QB;
            struct{
#if defined(__SPARC) || defined(_AIX)
		unsigned char Q2ratio : 4;
		unsigned char Q1ratio : 4;
#else
                unsigned char Q1ratio : 4;
                unsigned char Q2ratio : 4;
#endif
            } QR;
        } Q;                                        // 1 bytes
        unsigned char tmp_code[CODE_SIZE];          // 32/64 bytes
    } lsh_bin;
    
    mutable char *lsh_code;       // allocated when hash() function without buffer is called - 70/134 bytes or 74/138 bytes
    bool lsh_code_valid;  // true iff final() or fromTlshStr complete successfully
};


#endif

