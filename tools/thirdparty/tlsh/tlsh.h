// tlsh.h - TrendLSH  Hash Algorithm

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

#ifndef HEADER_TLSH_H
#define HEADER_TLSH_H

#define WINDOWS // dmex

// -force option no longer used
// #define	TLSH_OPTION_FORCE	1
#define	TLSH_OPTION_CONSERVATIVE	2
#define	TLSH_OPTION_KEEP_BUCKET		4
#define	TLSH_OPTION_PRIVATE		8
#define	TLSH_OPTION_THREADED		16

#if defined WINDOWS || defined MINGW
#include "tlsh_win_version.h"
#else
#include "tlsh_version.h"
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus

class TlshImpl;

// Define TLSH_STRING_LEN_REQ, which is the string length of "T1" + the hex value of the Tlsh hash.  
// BUCKETS_256 & CHECKSUM_3B are compiler switches defined in CMakeLists.txt
#if defined BUCKETS_256
  #define TLSH_STRING_LEN_REQ 136
  // changed the minimum data length to 256 for version 3.3
  #define MIN_DATA_LENGTH		50
  // added the -force option for version 3.5
  // added the -conservatibe option for version 3.17
  #define MIN_CONSERVATIVE_DATA_LENGTH	256
#endif

#if defined BUCKETS_128
  #define TLSH_STRING_LEN_REQ 72
  // changed the minimum data length to 256 for version 3.3
  #define MIN_DATA_LENGTH		50
  // added the -force option for version 3.5
  // added the -conservatibe option for version 3.17
  #define MIN_CONSERVATIVE_DATA_LENGTH	256
#endif

#if defined BUCKETS_48
  // No 3 Byte checksum option for 48 Bucket min hash
  #define TLSH_STRING_LEN 30
  // changed the minimum data length to 256 for version 3.3
  #define MIN_DATA_LENGTH		10
  // added the -force option for version 3.5
  #define MIN_CONSERVATIVE_DATA_LENGTH	10
#endif

#define TLSH_STRING_BUFFER_LEN (TLSH_STRING_LEN_REQ+1)

#ifdef WINDOWS
// 27/Nov/2020
// #include <WinFunctions.h>
	#define TLSH_API
#else 
	#if defined(__SPARC) || defined(_AS_MK_OS_RH73)
	   #define TLSH_API
	#else
	   #define TLSH_API __attribute__ ((visibility("default")))
	#endif
#endif

class TLSH_API Tlsh{

public:
    Tlsh();
    Tlsh(const Tlsh& other);

    /* allow the user to add data in multiple iterations */
    void update(const unsigned char* data, unsigned int len);

    /* to signal the class there is no more data to be added */
    void final(const unsigned char* data = NULL, unsigned int len = 0, int fc_cons_option = 0);

    /* to get the hex-encoded hash code */
    const char* getHash(int showvers=0) const ;

    /* to get the hex-encoded hash code without allocating buffer in TlshImpl - bufSize should be TLSH_STRING_BUFFER_LEN */
    const char* getHash(char *buffer, unsigned int bufSize, int showvers=0) const;  

    /* to bring to object back to the initial state */
    void reset();
    
    // access functions
    int Lvalue();
    int Q1ratio();
    int Q2ratio();
    int Checksum(int k);
    int BucketValue(int bucket);
    int HistogramCount(int bucket);

    /* calculate difference */
    /* The len_diff parameter specifies if the file length is to be included in the difference calculation (len_diff=true) or if it */
    /* is to be excluded (len_diff=false).  In general, the length should be considered in the difference calculation, but there */
    /* could be applications where a part of the adversarial activity might be to add a lot of content.  For example to add 1 million */
    /* zero bytes at the end of a file.  In that case, the caller would want to exclude the length from the calculation. */
    int totalDiff(const Tlsh *, bool len_diff=true) const;
    
    /* validate TrendLSH string and reset the hash according to it */
    int fromTlshStr(const char* str);

    /* check if Tlsh object is valid to operate */
    bool isValid() const;

    /* display the contents of NOTICE.txt */
    //static void display_notice();

    /* Return the version information used to build this library */
    //static const char *version();

    // operators
    Tlsh& operator=(const Tlsh& other);
    bool operator==(const Tlsh& other) const;
    bool operator!=(const Tlsh& other) const;

    ~Tlsh();

private:
    TlshImpl* impl;
};

#ifdef TLSH_DISTANCE_PARAMETERS
void set_tlsh_distance_parameters(int length_mult_value, int qratio_mult_value, int hist_diff1_add_value, int hist_diff2_add_value, int hist_diff3_add_value);
#endif

#endif

#endif

