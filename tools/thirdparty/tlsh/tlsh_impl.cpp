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

#define _CRT_SECURE_NO_WARNINGS 1

#include "tlsh.h"
#include "tlsh_impl.h"
#include "tlsh_util.h"

#include <string>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <algorithm>
//#include <string.h>
#include <errno.h>

#define RANGE_LVALUE 256
#define RANGE_QRATIO 16

static void find_quartile(unsigned int *q1, unsigned int *q2, unsigned int *q3, const unsigned int * a_bucket);
static unsigned int partition(unsigned int * buf, unsigned int left, unsigned int right);

////////////////////////////////////////////////////////////////////////////////////////////////

TlshImpl::TlshImpl() : a_bucket(NULL), data_len(0), lsh_code(NULL), lsh_code_valid(false)
{
    memset(this->slide_window, 0, sizeof this->slide_window);
    memset(&this->lsh_bin, 0, sizeof this->lsh_bin);

    assert (sizeof (this->lsh_bin.Q.QR) == sizeof (this->lsh_bin.Q.QB));
}

TlshImpl::~TlshImpl()
{
    delete [] this->a_bucket;
    delete [] this->lsh_code;
}

void TlshImpl::reset()
{
    delete [] this->a_bucket; this->a_bucket = NULL;
    memset(this->slide_window, 0, sizeof this->slide_window);
    delete [] this->lsh_code; this->lsh_code = NULL; 
    memset(&this->lsh_bin, 0, sizeof this->lsh_bin);
    this->data_len = 0;
    this->lsh_code_valid = false;   
}

////////////////////////////////////////////////////////////////////////////////////////////

// Pearson's sample random table
static unsigned char v_table[256] = {
    1, 87, 49, 12, 176, 178, 102, 166, 121, 193, 6, 84, 249, 230, 44, 163,
    14, 197, 213, 181, 161, 85, 218, 80, 64, 239, 24, 226, 236, 142, 38, 200,
    110, 177, 104, 103, 141, 253, 255, 50, 77, 101, 81, 18, 45, 96, 31, 222,
    25, 107, 190, 70, 86, 237, 240, 34, 72, 242, 20, 214, 244, 227, 149, 235,
    97, 234, 57, 22, 60, 250, 82, 175, 208, 5, 127, 199, 111, 62, 135, 248,
    174, 169, 211, 58, 66, 154, 106, 195, 245, 171, 17, 187, 182, 179, 0, 243,
    132, 56, 148, 75, 128, 133, 158, 100, 130, 126, 91, 13, 153, 246, 216, 219,
    119, 68, 223, 78, 83, 88, 201, 99, 122, 11, 92, 32, 136, 114, 52, 10,
    138, 30, 48, 183, 156, 35, 61, 26, 143, 74, 251, 94, 129, 162, 63, 152,
    170, 7, 115, 167, 241, 206, 3, 150, 55, 59, 151, 220, 90, 53, 23, 131,
    125, 173, 15, 238, 79, 95, 89, 16, 105, 137, 225, 224, 217, 160, 37, 123,
    118, 73, 2, 157, 46, 116, 9, 145, 134, 228, 207, 212, 202, 215, 69, 229,
    27, 188, 67, 124, 168, 252, 42, 4, 29, 108, 21, 247, 19, 205, 39, 203,
    233, 40, 186, 147, 198, 192, 155, 33, 164, 191, 98, 204, 165, 180, 117, 76,
    140, 36, 210, 172, 41, 54, 159, 8, 185, 232, 113, 196, 231, 47, 146, 120,
    51, 65, 28, 144, 254, 221, 93, 189, 194, 139, 112, 43, 71, 109, 184, 209
};

static unsigned char v_table48[256] = {
	1, 39, 1, 12, 32, 34, 6, 22, 25, 1, 6, 36, 48, 38, 44, 19,
	14, 5, 21, 37, 17, 37, 26, 32, 16, 47, 24, 34, 44, 46, 38, 8,
	14, 33, 8, 7, 45, 48, 48, 2, 29, 5, 33, 18, 45, 0, 31, 30,
	25, 11, 46, 22, 38, 45, 48, 34, 24, 48, 20, 22, 48, 35, 5, 43,
	1, 42, 9, 22, 12, 48, 34, 31, 16, 5, 31, 7, 15, 14, 39, 48,
	30, 25, 19, 10, 18, 10, 10, 3, 48, 27, 17, 43, 38, 35, 0, 48,
	36, 8, 4, 27, 32, 37, 14, 4, 34, 30, 43, 13, 9, 48, 24, 27,
	23, 20, 31, 30, 35, 40, 9, 3, 26, 11, 44, 32, 40, 18, 4, 10,
	42, 30, 0, 39, 12, 35, 13, 26, 47, 26, 48, 46, 33, 18, 15, 8,
	26, 7, 19, 23, 48, 14, 3, 6, 7, 11, 7, 28, 42, 5, 23, 35,
	29, 29, 15, 46, 31, 47, 41, 16, 9, 41, 33, 32, 25, 16, 37, 27,
	22, 25, 2, 13, 46, 20, 9, 1, 38, 36, 15, 20, 10, 23, 21, 37,
	27, 44, 19, 28, 24, 48, 42, 4, 29, 12, 21, 48, 19, 13, 39, 11,
	41, 40, 42, 3, 6, 0, 11, 33, 20, 47, 2, 12, 21, 36, 21, 28,
	44, 36, 18, 28, 41, 6, 15, 8, 41, 40, 17, 4, 39, 47, 2, 24,
	3, 17, 28, 0, 48, 29, 45, 45, 2, 43, 16, 43, 23, 13, 40, 17,
};

// Pearson's algorithm
unsigned char b_mapping(unsigned char salt, unsigned char i, unsigned char j, unsigned char k) {
    unsigned char h = 0;
    
    h = v_table[h ^ salt];
    h = v_table[h ^ i];
    h = v_table[h ^ j];
    h = v_table[h ^ k];
    return h;
}

/*
NEVER USED - showing a step in the optimization sequence
unsigned char faster_b_mapping(unsigned char mod_salt, unsigned char i, unsigned char j, unsigned char k) {
    unsigned char h;
    
    h = v_table[mod_salt ^ i];
    h = v_table[h ^ j];
    h = v_table[h ^ k];
    return h;
}
*/

#if defined BUCKETS_48
#define fast_b_mapping(ms,i,j,k) (v_table48[ v_table[ v_table[ms^i] ^ j] ^ k ])
#else
#define fast_b_mapping(ms,i,j,k) (v_table  [ v_table[ v_table[ms^i] ^ j] ^ k ])
#endif

////////////////////////////////////////////////////////////////////////////////////////////

#if SLIDING_WND_SIZE==5
	#define SLIDING_WND_SIZE_M1	4
#elif SLIDING_WND_SIZE==4
	#define SLIDING_WND_SIZE_M1	3
#elif SLIDING_WND_SIZE==6
	#define SLIDING_WND_SIZE_M1	5
#elif SLIDING_WND_SIZE==7
	#define SLIDING_WND_SIZE_M1	6
#elif SLIDING_WND_SIZE==8
	#define SLIDING_WND_SIZE_M1	7
#endif

#define RNG_SIZE    	SLIDING_WND_SIZE
#define RNG_IDX(i)	((i+RNG_SIZE)%RNG_SIZE)

void TlshImpl::update(const unsigned char* data, unsigned int len, int tlsh_option)
{
    if (this->lsh_code_valid) {
      //fprintf(stderr, "call to update() on a tlsh that is already valid\n");
      return;
    }   
	
    unsigned int fed_len = this->data_len;

    if (this->a_bucket == NULL) {
        this->a_bucket = new unsigned int [BUCKETS];
        memset(this->a_bucket, 0, sizeof(int)*BUCKETS);
    }

#if SLIDING_WND_SIZE==5
    if (TLSH_CHECKSUM_LEN == 1) {
	fast_update5(data, len, tlsh_option);
#ifndef CHECKSUM_0B
	if ((tlsh_option & TLSH_OPTION_THREADED) || (tlsh_option & TLSH_OPTION_PRIVATE)) {
		this->lsh_bin.checksum[0] = 0;
	}
#endif
	return;
    }
#endif
    int j = (int)(this->data_len % RNG_SIZE);

    for( unsigned int i=0; i<len; i++, fed_len++, j=RNG_IDX(j+1) ) {
        this->slide_window[j] = data[i];
        
        if ( fed_len >= SLIDING_WND_SIZE_M1 ) {
            //only calculate when input >= 5 bytes
            int j_1 = RNG_IDX(j-1);
            int j_2 = RNG_IDX(j-2);
            int j_3 = RNG_IDX(j-3);
#if SLIDING_WND_SIZE>=5
            int j_4 = RNG_IDX(j-4);
#endif
#if SLIDING_WND_SIZE>=6
            int j_5 = RNG_IDX(j-5);
#endif
#if SLIDING_WND_SIZE>=7
            int j_6 = RNG_IDX(j-6);
#endif
#if SLIDING_WND_SIZE>=8
            int j_7 = RNG_IDX(j-7);
#endif
           
#ifndef CHECKSUM_0B
            for (int k = 0; k < TLSH_CHECKSUM_LEN; k++) {
		if (k == 0) {
			//				 b_mapping(0, ... )
		 	this->lsh_bin.checksum[k] = fast_b_mapping(1, this->slide_window[j], this->slide_window[j_1], this->lsh_bin.checksum[k]);
		} else {
			// use calculated 1 byte checksums to expand the total checksum to 3 bytes
			this->lsh_bin.checksum[k] = b_mapping(this->lsh_bin.checksum[k-1], this->slide_window[j], this->slide_window[j_1], this->lsh_bin.checksum[k]);
		}
            }
#endif

            unsigned char r;
	    //	     b_mapping(2, ... )
	    r = fast_b_mapping(49, this->slide_window[j], this->slide_window[j_1], this->slide_window[j_2]);
            this->a_bucket[r]++;
	    //	     b_mapping(3, ... )
	    r = fast_b_mapping(12, this->slide_window[j], this->slide_window[j_1], this->slide_window[j_3]);
            this->a_bucket[r]++;
	    //	     b_mapping(5, ... )
	    r = fast_b_mapping(178, this->slide_window[j], this->slide_window[j_2], this->slide_window[j_3]);
            this->a_bucket[r]++;
#if SLIDING_WND_SIZE>=5
	    //	     b_mapping(7, ... )
	    r = fast_b_mapping(166, this->slide_window[j], this->slide_window[j_2], this->slide_window[j_4]);
            this->a_bucket[r]++;
	    //	     b_mapping(11, ... )
	    r = fast_b_mapping(84, this->slide_window[j], this->slide_window[j_1], this->slide_window[j_4]);
            this->a_bucket[r]++;
	    //	     b_mapping(13, ... )
	    r = fast_b_mapping(230, this->slide_window[j], this->slide_window[j_3], this->slide_window[j_4]);
            this->a_bucket[r]++;
#endif
#if SLIDING_WND_SIZE>=6
	    //	     b_mapping(17, ... )
	    r = fast_b_mapping(197, this->slide_window[j], this->slide_window[j_1], this->slide_window[j_5]);
	    this->a_bucket[r]++;
	    //	     b_mapping(19, ... )
	    r = fast_b_mapping(181, this->slide_window[j], this->slide_window[j_2], this->slide_window[j_5]);
	    this->a_bucket[r]++;
	    //	     b_mapping(23, ... )
	    r = fast_b_mapping(80, this->slide_window[j], this->slide_window[j_3], this->slide_window[j_5]);
	    this->a_bucket[r]++;
	    //	     b_mapping(29, ... )
	    r = fast_b_mapping(142, this->slide_window[j], this->slide_window[j_4], this->slide_window[j_5]);
	    this->a_bucket[r]++;
#endif
#if SLIDING_WND_SIZE>=7
	    //	     b_mapping(31, ... )
	    r = fast_b_mapping(200, this->slide_window[j], this->slide_window[j_1], this->slide_window[j_6]);
	    this->a_bucket[r]++;
	    //	     b_mapping(37, ... )
	    r = fast_b_mapping(253, this->slide_window[j], this->slide_window[j_2], this->slide_window[j_6]);
	    this->a_bucket[r]++;
	    //	     b_mapping(41, ... )
	    r = fast_b_mapping(101, this->slide_window[j], this->slide_window[j_3], this->slide_window[j_6]);
	    this->a_bucket[r]++;
	    //	     b_mapping(43, ... )
	    r = fast_b_mapping(18, this->slide_window[j], this->slide_window[j_4], this->slide_window[j_6]);
	    this->a_bucket[r]++;
	    //	     b_mapping(47, ... )
	    r = fast_b_mapping(222, this->slide_window[j], this->slide_window[j_5], this->slide_window[j_6]);
	    this->a_bucket[r]++;
#endif
#if SLIDING_WND_SIZE>=8
	    //	     b_mapping(53, ... )
	    r = fast_b_mapping(237, this->slide_window[j], this->slide_window[j_1], this->slide_window[j_7]);
	    this->a_bucket[r]++;
	    //	     b_mapping(59, ... )
	    r = fast_b_mapping(214, this->slide_window[j], this->slide_window[j_2], this->slide_window[j_7]);
	    this->a_bucket[r]++;
	    //	     b_mapping(61, ... )
	    r = fast_b_mapping(227, this->slide_window[j], this->slide_window[j_3], this->slide_window[j_7]);
	    this->a_bucket[r]++;
	    //	     b_mapping(67, ... )
	    r = fast_b_mapping(22, this->slide_window[j], this->slide_window[j_4], this->slide_window[j_7]);
	    this->a_bucket[r]++;
	    //	     b_mapping(71, ... )
	    r = fast_b_mapping(175, this->slide_window[j], this->slide_window[j_5], this->slide_window[j_7]);
	    this->a_bucket[r]++;
	    //	     b_mapping(73, ... )
	    r = fast_b_mapping(5, this->slide_window[j], this->slide_window[j_6], this->slide_window[j_7]);
	    this->a_bucket[r]++;
#endif
        }
    }
    this->data_len += len;
#ifndef CHECKSUM_0B
    if ((tlsh_option & TLSH_OPTION_THREADED) || (tlsh_option & TLSH_OPTION_PRIVATE)) {
	for (int k = 0; k < TLSH_CHECKSUM_LEN; k++) {
		this->lsh_bin.checksum[k] = 0;
	}
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////
// update for the case when SLIDING_WND_SIZE==5
// have different optimized functions for
//	default TLSH
//	threaded TLSH
//	private TLSH
/////////////////////////////////////////////////////////////////////////////
static void raw_fast_update5(
	// inputs
	const unsigned char* data,
	unsigned int len,
	unsigned int fed_len,
	// outputs
	unsigned int *a_bucket,
	unsigned char *ret_checksum,
	unsigned char *slide_window
	);

static void raw_fast_update5_private(
	// inputs
	const unsigned char* data,
	unsigned int len,
	unsigned int fed_len,
	// outputs
	unsigned int *a_bucket,
	unsigned char *slide_window
	);

#ifdef THREADING_IMPLEMENTED
#include <thread>
#endif

struct raw_args {
	// inputs
	const unsigned char* data;
	unsigned int len;
	unsigned int fed_len;
	// outputs
	unsigned int bucket[256];
	unsigned char slide_window[5];
};

static void raw_fast_update5_nochecksum( struct raw_args *ra );

static struct raw_args call1;
static struct raw_args call2;

void thread1()
{
	raw_fast_update5_nochecksum( &call1 );
}
void thread2()
{
	raw_fast_update5_nochecksum( &call2 );
}

void TlshImpl::fast_update5(const unsigned char* data, unsigned int len, int tlsh_option)
{
#ifdef THREADING_IMPLEMENTED
	if ((len >= 10000) && (tlsh_option & TLSH_OPTION_THREADED)) {
		unsigned len2A = len / 2;
		unsigned len2B = len - len2A;
		// printf("method 2	len=%d	len2A=%d	len2B=%d\n", len, len2A, len2B);

		for (int bi=0; bi<256; bi++) {
			call1.bucket[bi] = 0;
			call2.bucket[bi] = 0;
		}
		for (int di=0; di<5; di++) {
			call1.slide_window[di] = 0;
			int didx = len2A - 5 + di;
			int wi = didx % 5;
			call2.slide_window[wi] = data[didx];
		}

		call1.data	= data;
		call1.len	= len2A;
		call1.fed_len	= 0;

		call2.data	= &data[len2A];
		call2.len	= len2B;
		call2.fed_len	= len2A;

		std::thread t1(thread1);
		std::thread t2(thread2);
		t1.join();
		t2.join();

		this->data_len += len;

		for (int bi=0; bi<128; bi++) {
			this->a_bucket[bi] += (call1.bucket[bi] + call2.bucket[bi]) ;
			// printf("bucket	%d	=%d\n", bi, this->a_bucket[bi] );
		}
		return;
	}
#endif
	if (tlsh_option & TLSH_OPTION_PRIVATE) {
		raw_fast_update5_private(data, len, this->data_len, this->a_bucket,                               this->slide_window);
		this->data_len += len;
		this->lsh_bin.checksum[0] = 0;
	} else {
		raw_fast_update5        (data, len, this->data_len, this->a_bucket, &(this->lsh_bin.checksum[0]), this->slide_window);
		this->data_len += len;
	}
}

static void raw_fast_update5(
	// inputs
	const unsigned char* data,
	unsigned int len,
	unsigned int fed_len,
	// outputs
	unsigned int *a_bucket,
	unsigned char *ret_checksum,
	unsigned char *slide_window
	)
{
	int j = (int)(fed_len % RNG_SIZE);
	unsigned char checksum = *ret_checksum;

	unsigned int start_i=0;
	if ( fed_len < SLIDING_WND_SIZE_M1 ) {
		int extra = SLIDING_WND_SIZE_M1 - fed_len;
		start_i	= extra;
		j	= (j + extra) % RNG_SIZE;
	}
	for( unsigned int i=start_i; i<len;  ) {
		//only calculate when input >= 5 bytes
		if ((i >= 4) && (i+5 < len)) {
			unsigned char a0 = data[i-4];
			unsigned char a1 = data[i-3];
			unsigned char a2 = data[i-2];
			unsigned char a3 = data[i-1];
			unsigned char a4 = data[i];
			unsigned char a5 = data[i+1];
			unsigned char a6 = data[i+2];
			unsigned char a7 = data[i+3];
			unsigned char a8 = data[i+4];

			checksum = fast_b_mapping(1, a4, a3, checksum );
			a_bucket[ fast_b_mapping(49,  a4, a3, a2 ) ]++;
			a_bucket[ fast_b_mapping(12,  a4, a3, a1 ) ]++;
			a_bucket[ fast_b_mapping(178, a4, a2, a1 ) ]++;
			a_bucket[ fast_b_mapping(166, a4, a2, a0 ) ]++;
			a_bucket[ fast_b_mapping(84,  a4, a3, a0 ) ]++;
			a_bucket[ fast_b_mapping(230, a4, a1, a0 ) ]++;

			checksum = fast_b_mapping(1, a5, a4, checksum );
			a_bucket[ fast_b_mapping(49,  a5, a4, a3 ) ]++;
			a_bucket[ fast_b_mapping(12,  a5, a4, a2 ) ]++;
			a_bucket[ fast_b_mapping(178, a5, a3, a2 ) ]++;
			a_bucket[ fast_b_mapping(166, a5, a3, a1 ) ]++;
			a_bucket[ fast_b_mapping(84,  a5, a4, a1 ) ]++;
			a_bucket[ fast_b_mapping(230, a5, a2, a1 ) ]++;

			checksum = fast_b_mapping(1, a6, a5, checksum );
			a_bucket[ fast_b_mapping(49,  a6, a5, a4 ) ]++;
			a_bucket[ fast_b_mapping(12,  a6, a5, a3 ) ]++;
			a_bucket[ fast_b_mapping(178, a6, a4, a3 ) ]++;
			a_bucket[ fast_b_mapping(166, a6, a4, a2 ) ]++;
			a_bucket[ fast_b_mapping(84,  a6, a5, a2 ) ]++;
			a_bucket[ fast_b_mapping(230, a6, a3, a2 ) ]++;

			checksum = fast_b_mapping(1, a7, a6, checksum );
			a_bucket[ fast_b_mapping(49,  a7, a6, a5 ) ]++;
			a_bucket[ fast_b_mapping(12,  a7, a6, a4 ) ]++;
			a_bucket[ fast_b_mapping(178, a7, a5, a4 ) ]++;
			a_bucket[ fast_b_mapping(166, a7, a5, a3 ) ]++;
			a_bucket[ fast_b_mapping(84,  a7, a6, a3 ) ]++;
			a_bucket[ fast_b_mapping(230, a7, a4, a3 ) ]++;

			checksum = fast_b_mapping(1, a8, a7, checksum );
			a_bucket[ fast_b_mapping(49,  a8, a7, a6 ) ]++;
			a_bucket[ fast_b_mapping(12,  a8, a7, a5 ) ]++;
			a_bucket[ fast_b_mapping(178, a8, a6, a5 ) ]++;
			a_bucket[ fast_b_mapping(166, a8, a6, a4 ) ]++;
			a_bucket[ fast_b_mapping(84,  a8, a7, a4 ) ]++;
			a_bucket[ fast_b_mapping(230, a8, a5, a4 ) ]++;

			i=i+5;
			j=RNG_IDX(j+5);
		} else {
			slide_window[j] = data[i];
			int j_1 = RNG_IDX(j-1); if (i >= 1) { slide_window[j_1] = data[i-1]; }
			int j_2 = RNG_IDX(j-2); if (i >= 2) { slide_window[j_2] = data[i-2]; }
			int j_3 = RNG_IDX(j-3); if (i >= 3) { slide_window[j_3] = data[i-3]; }
			int j_4 = RNG_IDX(j-4); if (i >= 4) { slide_window[j_4] = data[i-4]; }

			checksum = fast_b_mapping(1, slide_window[j], slide_window[j_1], checksum );
			a_bucket[ fast_b_mapping(49,  slide_window[j], slide_window[j_1], slide_window[j_2] ) ]++;
			a_bucket[ fast_b_mapping(12,  slide_window[j], slide_window[j_1], slide_window[j_3] ) ]++;
			a_bucket[ fast_b_mapping(178, slide_window[j], slide_window[j_2], slide_window[j_3] ) ]++;
			a_bucket[ fast_b_mapping(166, slide_window[j], slide_window[j_2], slide_window[j_4] ) ]++;
			a_bucket[ fast_b_mapping(84,  slide_window[j], slide_window[j_1], slide_window[j_4] ) ]++;
			a_bucket[ fast_b_mapping(230, slide_window[j], slide_window[j_3], slide_window[j_4] ) ]++;
			i++;
			j=RNG_IDX(j+1);
		}
	}
	*ret_checksum = checksum;
}

static void raw_fast_update5_private(
	// inputs
	const unsigned char* data,
	unsigned int len,
	unsigned int fed_len,
	// outputs
	unsigned int *a_bucket,
	unsigned char *slide_window
	)
{
	int j = (int)(fed_len % RNG_SIZE);

	unsigned int start_i=0;
	if ( fed_len < SLIDING_WND_SIZE_M1 ) {
		int extra = SLIDING_WND_SIZE_M1 - fed_len;
		start_i	= extra;
		j	= (j + extra) % RNG_SIZE;
	}
	for( unsigned int i=start_i; i<len;  ) {
		//only calculate when input >= 5 bytes
		if ((i >= 4) && (i+5 < len)) {
			unsigned char a0 = data[i-4];
			unsigned char a1 = data[i-3];
			unsigned char a2 = data[i-2];
			unsigned char a3 = data[i-1];
			unsigned char a4 = data[i];
			unsigned char a5 = data[i+1];
			unsigned char a6 = data[i+2];
			unsigned char a7 = data[i+3];
			unsigned char a8 = data[i+4];

			a_bucket[ fast_b_mapping(49,  a4, a3, a2 ) ]++;
			a_bucket[ fast_b_mapping(12,  a4, a3, a1 ) ]++;
			a_bucket[ fast_b_mapping(178, a4, a2, a1 ) ]++;
			a_bucket[ fast_b_mapping(166, a4, a2, a0 ) ]++;
			a_bucket[ fast_b_mapping(84,  a4, a3, a0 ) ]++;
			a_bucket[ fast_b_mapping(230, a4, a1, a0 ) ]++;

			a_bucket[ fast_b_mapping(49,  a5, a4, a3 ) ]++;
			a_bucket[ fast_b_mapping(12,  a5, a4, a2 ) ]++;
			a_bucket[ fast_b_mapping(178, a5, a3, a2 ) ]++;
			a_bucket[ fast_b_mapping(166, a5, a3, a1 ) ]++;
			a_bucket[ fast_b_mapping(84,  a5, a4, a1 ) ]++;
			a_bucket[ fast_b_mapping(230, a5, a2, a1 ) ]++;

			a_bucket[ fast_b_mapping(49,  a6, a5, a4 ) ]++;
			a_bucket[ fast_b_mapping(12,  a6, a5, a3 ) ]++;
			a_bucket[ fast_b_mapping(178, a6, a4, a3 ) ]++;
			a_bucket[ fast_b_mapping(166, a6, a4, a2 ) ]++;
			a_bucket[ fast_b_mapping(84,  a6, a5, a2 ) ]++;
			a_bucket[ fast_b_mapping(230, a6, a3, a2 ) ]++;

			a_bucket[ fast_b_mapping(49,  a7, a6, a5 ) ]++;
			a_bucket[ fast_b_mapping(12,  a7, a6, a4 ) ]++;
			a_bucket[ fast_b_mapping(178, a7, a5, a4 ) ]++;
			a_bucket[ fast_b_mapping(166, a7, a5, a3 ) ]++;
			a_bucket[ fast_b_mapping(84,  a7, a6, a3 ) ]++;
			a_bucket[ fast_b_mapping(230, a7, a4, a3 ) ]++;

			a_bucket[ fast_b_mapping(49,  a8, a7, a6 ) ]++;
			a_bucket[ fast_b_mapping(12,  a8, a7, a5 ) ]++;
			a_bucket[ fast_b_mapping(178, a8, a6, a5 ) ]++;
			a_bucket[ fast_b_mapping(166, a8, a6, a4 ) ]++;
			a_bucket[ fast_b_mapping(84,  a8, a7, a4 ) ]++;
			a_bucket[ fast_b_mapping(230, a8, a5, a4 ) ]++;

			i=i+5;
			j=RNG_IDX(j+5);
		} else {
			slide_window[j] = data[i];
			int j_1 = RNG_IDX(j-1); if (i >= 1) { slide_window[j_1] = data[i-1]; }
			int j_2 = RNG_IDX(j-2); if (i >= 2) { slide_window[j_2] = data[i-2]; }
			int j_3 = RNG_IDX(j-3); if (i >= 3) { slide_window[j_3] = data[i-3]; }
			int j_4 = RNG_IDX(j-4); if (i >= 4) { slide_window[j_4] = data[i-4]; }

			a_bucket[ fast_b_mapping(49,  slide_window[j], slide_window[j_1], slide_window[j_2] ) ]++;
			a_bucket[ fast_b_mapping(12,  slide_window[j], slide_window[j_1], slide_window[j_3] ) ]++;
			a_bucket[ fast_b_mapping(178, slide_window[j], slide_window[j_2], slide_window[j_3] ) ]++;
			a_bucket[ fast_b_mapping(166, slide_window[j], slide_window[j_2], slide_window[j_4] ) ]++;
			a_bucket[ fast_b_mapping(84,  slide_window[j], slide_window[j_1], slide_window[j_4] ) ]++;
			a_bucket[ fast_b_mapping(230, slide_window[j], slide_window[j_3], slide_window[j_4] ) ]++;
			i++;
			j=RNG_IDX(j+1);
		}
	}
}

static void raw_fast_update5_nochecksum( struct raw_args *ra )
{
	const unsigned char* data	= ra->data;
	unsigned int len		= ra->len;
	// outputs
	unsigned int *a_bucket		= ra->bucket;
	unsigned char *slide_window	= ra->slide_window;

	int j = (int)(ra->fed_len % RNG_SIZE);

	unsigned int start_i=0;
	if ( ra->fed_len < SLIDING_WND_SIZE_M1 ) {
		int extra = SLIDING_WND_SIZE_M1 - ra->fed_len;
		start_i	= extra;
		j	= (j + extra) % RNG_SIZE;
	}
	for( unsigned int i=start_i; i<len;  ) {
		//only calculate when input >= 5 bytes
		if ((i >= 4) && (i+5 < len)) {
			unsigned char a0 = data[i-4];
			unsigned char a1 = data[i-3];
			unsigned char a2 = data[i-2];
			unsigned char a3 = data[i-1];
			unsigned char a4 = data[i];
			unsigned char a5 = data[i+1];
			unsigned char a6 = data[i+2];
			unsigned char a7 = data[i+3];
			unsigned char a8 = data[i+4];

			a_bucket[ fast_b_mapping(49,  a4, a3, a2 ) ]++;
			a_bucket[ fast_b_mapping(12,  a4, a3, a1 ) ]++;
			a_bucket[ fast_b_mapping(178, a4, a2, a1 ) ]++;
			a_bucket[ fast_b_mapping(166, a4, a2, a0 ) ]++;
			a_bucket[ fast_b_mapping(84,  a4, a3, a0 ) ]++;
			a_bucket[ fast_b_mapping(230, a4, a1, a0 ) ]++;

			a_bucket[ fast_b_mapping(49,  a5, a4, a3 ) ]++;
			a_bucket[ fast_b_mapping(12,  a5, a4, a2 ) ]++;
			a_bucket[ fast_b_mapping(178, a5, a3, a2 ) ]++;
			a_bucket[ fast_b_mapping(166, a5, a3, a1 ) ]++;
			a_bucket[ fast_b_mapping(84,  a5, a4, a1 ) ]++;
			a_bucket[ fast_b_mapping(230, a5, a2, a1 ) ]++;

			a_bucket[ fast_b_mapping(49,  a6, a5, a4 ) ]++;
			a_bucket[ fast_b_mapping(12,  a6, a5, a3 ) ]++;
			a_bucket[ fast_b_mapping(178, a6, a4, a3 ) ]++;
			a_bucket[ fast_b_mapping(166, a6, a4, a2 ) ]++;
			a_bucket[ fast_b_mapping(84,  a6, a5, a2 ) ]++;
			a_bucket[ fast_b_mapping(230, a6, a3, a2 ) ]++;

			a_bucket[ fast_b_mapping(49,  a7, a6, a5 ) ]++;
			a_bucket[ fast_b_mapping(12,  a7, a6, a4 ) ]++;
			a_bucket[ fast_b_mapping(178, a7, a5, a4 ) ]++;
			a_bucket[ fast_b_mapping(166, a7, a5, a3 ) ]++;
			a_bucket[ fast_b_mapping(84,  a7, a6, a3 ) ]++;
			a_bucket[ fast_b_mapping(230, a7, a4, a3 ) ]++;

			a_bucket[ fast_b_mapping(49,  a8, a7, a6 ) ]++;
			a_bucket[ fast_b_mapping(12,  a8, a7, a5 ) ]++;
			a_bucket[ fast_b_mapping(178, a8, a6, a5 ) ]++;
			a_bucket[ fast_b_mapping(166, a8, a6, a4 ) ]++;
			a_bucket[ fast_b_mapping(84,  a8, a7, a4 ) ]++;
			a_bucket[ fast_b_mapping(230, a8, a5, a4 ) ]++;

			i=i+5;
			j=RNG_IDX(j+5);
		} else {
			slide_window[j] = data[i];
			int j_1 = RNG_IDX(j-1); if (i >= 1) { slide_window[j_1] = data[i-1]; }
			int j_2 = RNG_IDX(j-2); if (i >= 2) { slide_window[j_2] = data[i-2]; }
			int j_3 = RNG_IDX(j-3); if (i >= 3) { slide_window[j_3] = data[i-3]; }
			int j_4 = RNG_IDX(j-4); if (i >= 4) { slide_window[j_4] = data[i-4]; }

			a_bucket[ fast_b_mapping(49,  slide_window[j], slide_window[j_1], slide_window[j_2] ) ]++;
			a_bucket[ fast_b_mapping(12,  slide_window[j], slide_window[j_1], slide_window[j_3] ) ]++;
			a_bucket[ fast_b_mapping(178, slide_window[j], slide_window[j_2], slide_window[j_3] ) ]++;
			a_bucket[ fast_b_mapping(166, slide_window[j], slide_window[j_2], slide_window[j_4] ) ]++;
			a_bucket[ fast_b_mapping(84,  slide_window[j], slide_window[j_1], slide_window[j_4] ) ]++;
			a_bucket[ fast_b_mapping(230, slide_window[j], slide_window[j_3], slide_window[j_4] ) ]++;
			i++;
			j=RNG_IDX(j+1);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// fc_cons_option - a bitfield
//	0	default
//	1	force (now the default)
//	2	conservative
//	4	do not delete a_bucket
/////////////////////////////////////////////////////////////////////////////

/* to signal the class there is no more data to be added */
void TlshImpl::final(int fc_cons_option) 
{
    if (this->lsh_code_valid) {
      //fprintf(stderr, "call to final() on a tlsh that is already valid\n");
      return;
    }   
    // incoming data must more than or equal to MIN_DATA_LENGTH bytes
    if (((fc_cons_option & TLSH_OPTION_CONSERVATIVE) == 0) && (this->data_len < MIN_DATA_LENGTH)) {
      // this->lsh_code be empty
      delete [] this->a_bucket; this->a_bucket = NULL;
      return;
    }
    if ((fc_cons_option & TLSH_OPTION_CONSERVATIVE) && (this->data_len < MIN_CONSERVATIVE_DATA_LENGTH)) {
      // this->lsh_code be empty
      delete [] this->a_bucket; this->a_bucket = NULL;
      return;
    }

    unsigned int q1, q2, q3;
    find_quartile(&q1, &q2, &q3, this->a_bucket);

    // issue #79 - divide by 0 if q3 == 0
    if (q3 == 0) {
      delete [] this->a_bucket; this->a_bucket = NULL;
      return;
    }

    // buckets must be more than 50% non-zero
    int nonzero = 0;
    for(unsigned int i=0; i<CODE_SIZE; i++) {
      for(unsigned int j=0; j<4; j++) {
        if (this->a_bucket[4*i + j] > 0) {
          nonzero++;
        }
      }
    }
#if defined BUCKETS_48
    if (nonzero < 18) {
      // printf("nonzero=%d\n", nonzero);
      delete [] this->a_bucket; this->a_bucket = NULL;
      return;
    }
#else
    if (nonzero <= 4*CODE_SIZE/2) {
      delete [] this->a_bucket; this->a_bucket = NULL;
      return;
    }
#endif
    
    for(unsigned int i=0; i<CODE_SIZE; i++) {
        unsigned char h=0;
        for(unsigned int j=0; j<4; j++) {
            unsigned int k = this->a_bucket[4*i + j];
            if( q3 < k ) {
                h += 3 << (j*2);  // leave the optimization j*2 = j<<1 or j*2 = j+j for compiler
            } else if( q2 < k ) {
                h += 2 << (j*2);
            } else if( q1 < k ) {
                h += 1 << (j*2);
	    }
        }
        this->lsh_bin.tmp_code[i] = h;
    }

    if ((fc_cons_option & TLSH_OPTION_KEEP_BUCKET) == 0) {
        //Done with a_bucket so deallocate
        delete [] this->a_bucket; this->a_bucket = NULL;
    }
    
    this->lsh_bin.Lvalue = l_capturing(this->data_len);
    this->lsh_bin.Q.QR.Q1ratio = (unsigned int) ((float)(q1*100)/(float) q3) % 16;
    this->lsh_bin.Q.QR.Q2ratio = (unsigned int) ((float)(q2*100)/(float) q3) % 16;
    this->lsh_code_valid = true;   
}

int TlshImpl::fromTlshStr(const char* str)
{
	// Assume that we have 128 Buckets
	int start = 0;
        if (strncmp(str, "T1", 2) == 0) {
		start = 2;
	} else {
		start = 0;
	}
	// Validate input string
	for( int ii=0; ii < INTERNAL_TLSH_STRING_LEN; ii++ ) {
		int i = ii + start;
		if (!(	(str[i] >= '0' && str[i] <= '9') || 
			(str[i] >= 'A' && str[i] <= 'F') ||
			(str[i] >= 'a' && str[i] <= 'f') ))
        	{
			// printf("warning ii=%d str[%d]='%c'\n", ii, i, str[i]);
			return 1;
		}
	}
	int xi = INTERNAL_TLSH_STRING_LEN + start;
	if ((	(str[xi] >= '0' && str[xi] <= '9') || 
		(str[xi] >= 'A' && str[xi] <= 'F') ||
		(str[xi] >= 'a' && str[xi] <= 'f') ))
        {
		// printf("warning xi=%d\n", xi);
		return 1;
	}

	this->reset();

	lsh_bin_struct tmp;
	from_hex( &str[start], INTERNAL_TLSH_STRING_LEN, (unsigned char*)&tmp );

	// Reconstruct checksum, Qrations & lvalue
	for (int k = 0; k < TLSH_CHECKSUM_LEN; k++) {    
		this->lsh_bin.checksum[k] = swap_byte(tmp.checksum[k]);
	}
	this->lsh_bin.Lvalue = swap_byte( tmp.Lvalue );
	this->lsh_bin.Q.QB = swap_byte(tmp.Q.QB);
	for( int i=0; i < CODE_SIZE; i++ ){
		this->lsh_bin.tmp_code[i] = (tmp.tmp_code[CODE_SIZE-1-i]);
	}
	this->lsh_code_valid = true;   

	return 0;
}

const char* TlshImpl::hash(char *buffer, unsigned int bufSize, int showvers) const
{
    if (bufSize < TLSH_STRING_LEN_REQ + 1) {
        strncpy(buffer, "", bufSize);
        return buffer;
    }
    if (this->lsh_code_valid == false) {
        strncpy(buffer, "", bufSize);
        return buffer;
    }

    lsh_bin_struct tmp;
    for (int k = 0; k < TLSH_CHECKSUM_LEN; k++) {    
      tmp.checksum[k] = swap_byte( this->lsh_bin.checksum[k] );
    }
    tmp.Lvalue = swap_byte( this->lsh_bin.Lvalue );
    tmp.Q.QB = swap_byte( this->lsh_bin.Q.QB );
    for( int i=0; i < CODE_SIZE; i++ ){
        tmp.tmp_code[i] = (this->lsh_bin.tmp_code[CODE_SIZE-1-i]);
    }

	if (showvers) {
		buffer[0] = 'T';
		buffer[1] = '0' + showvers;
		to_hex( (unsigned char*)&tmp, sizeof(tmp), &buffer[2]);
	} else {
		to_hex( (unsigned char*)&tmp, sizeof(tmp), buffer);
	}
	return buffer;
}

/* to get the hex-encoded hash code */
const char* TlshImpl::hash(int showvers) const
{
    if (this->lsh_code != NULL) {
        // lsh_code has been previously calculated, so just return it
        return this->lsh_code;
    }

    this->lsh_code = new char [TLSH_STRING_LEN_REQ+1];
    memset(this->lsh_code, 0, TLSH_STRING_LEN_REQ+1);
	
    return hash(this->lsh_code, TLSH_STRING_LEN_REQ+1, showvers);
}


// compare
int TlshImpl::compare(const TlshImpl& other) const
{
    return (memcmp( &(this->lsh_bin), &(other.lsh_bin), sizeof(this->lsh_bin)));
}

////////////////////////////////////////////
// the default for these parameters is 12
////////////////////////////////////////////

static int length_mult = 12;
static int qratio_mult = 12;

#ifdef TLSH_DISTANCE_PARAMETERS

       int hist_diff1_add = 1;
       int hist_diff2_add = 2;
       int hist_diff3_add = 6;

void set_tlsh_distance_parameters(int length_mult_value, int qratio_mult_value, int hist_diff1_add_value, int hist_diff2_add_value, int hist_diff3_add_value)
{
	if (length_mult_value != -1) {
		length_mult = length_mult_value;
	}
	if (qratio_mult_value != -1) {
		qratio_mult = qratio_mult_value;
	}
	if (hist_diff1_add_value != -1) {
		hist_diff1_add = hist_diff1_add_value;
	}
	if (hist_diff2_add_value != -1) {
		hist_diff2_add = hist_diff2_add_value;
	}
	if (hist_diff3_add_value != -1) {
		hist_diff3_add = hist_diff3_add_value;
	}
}
#endif

int TlshImpl::Lvalue()
{
	return(this->lsh_bin.Lvalue);
}
int TlshImpl::Q1ratio()
{
	return(this->lsh_bin.Q.QR.Q1ratio);
}
int TlshImpl::Q2ratio()
{
	return(this->lsh_bin.Q.QR.Q2ratio);
}
int TlshImpl::Checksum(int k)
{
	if ((k >= TLSH_CHECKSUM_LEN) || (k < 0)) {
		return(0);
	}
	return(this->lsh_bin.checksum[k]);
}
int TlshImpl::BucketValue(int bucket)
{
int idx;
int elem;
unsigned char bv;
//  default TLSH
//  #define EFF_BUCKETS         128
//  #define CODE_SIZE           32   // 128 * 2 bits = 32 bytes

	idx	= (CODE_SIZE - (bucket / 4)) - 1;
//	if ((idx < 0) || (idx >= CODE_SIZE)) {
//		printf("error in BucketValue: idx=%d\n", idx);
//		exit(1);
//	}
	elem	= bucket % 4;
	bv = this->lsh_bin.tmp_code[idx];
	int h1	= bv  / 16;
	int h2	= bv  % 16;
	int p1	= h1 / 4;
	int p2	= h1 % 4;
	int p3	= h2 / 4;
	int p4	= h2 % 4;
	if (elem == 0) {
		return(p1);
	}
	if (elem == 1) {
		return(p2);
	}
	if (elem == 2) {
		return(p3);
	}
	return(p4);
}
int TlshImpl::HistogramCount(int bucket)
{
	if (this->a_bucket == NULL)
		return(-1);
	return(this->a_bucket[EFF_BUCKETS - 1 - bucket]);
}

int TlshImpl::totalDiff(const TlshImpl& other, bool len_diff) const
{
    int diff = 0;
    
    if (len_diff) {
        int ldiff = mod_diff( this->lsh_bin.Lvalue, other.lsh_bin.Lvalue, RANGE_LVALUE);
        if ( ldiff == 0 )
            diff = 0;
        else if ( ldiff == 1 )
            diff = 1;
        else
           diff += ldiff*length_mult;
    }
    
    int q1diff = mod_diff( this->lsh_bin.Q.QR.Q1ratio, other.lsh_bin.Q.QR.Q1ratio, RANGE_QRATIO);
    if ( q1diff <= 1 )
        diff += q1diff;
    else           
        diff += (q1diff-1)*qratio_mult;
    
    int q2diff = mod_diff( this->lsh_bin.Q.QR.Q2ratio, other.lsh_bin.Q.QR.Q2ratio, RANGE_QRATIO);
    if ( q2diff <= 1)
        diff += q2diff;
    else
        diff += (q2diff-1)*qratio_mult;
    
    for (int k = 0; k < TLSH_CHECKSUM_LEN; k++) {    
      if (this->lsh_bin.checksum[k] != other.lsh_bin.checksum[k] ) {
        diff ++;
        break;
      }
    }
    
    diff += h_distance( CODE_SIZE, this->lsh_bin.tmp_code, other.lsh_bin.tmp_code );

    return (diff);
}



#define SWAP_UINT(x,y) do {\
    unsigned int int_tmp = (x);  \
    (x) = (y); \
    (y) = int_tmp; } while(0)

void find_quartile(unsigned int *q1, unsigned int *q2, unsigned int *q3, const unsigned int * a_bucket) 
{
    unsigned int bucket_copy[EFF_BUCKETS], short_cut_left[EFF_BUCKETS], short_cut_right[EFF_BUCKETS], spl=0, spr=0;
    unsigned int p1 = EFF_BUCKETS/4-1;
    unsigned int p2 = EFF_BUCKETS/2-1;
    unsigned int p3 = EFF_BUCKETS-EFF_BUCKETS/4-1;
    unsigned int end = EFF_BUCKETS-1;

    for(unsigned int i=0; i<=end; i++) {
        bucket_copy[i] = a_bucket[i];
    }

    for( unsigned int l=0, r=end; ; ) {
        unsigned int ret = partition( bucket_copy, l, r );
        if( ret > p2 ) {
            r = ret - 1;
            short_cut_right[spr] = ret;
            spr++;
        } else if( ret < p2 ){
            l = ret + 1;
            short_cut_left[spl] = ret;
            spl++;
        } else {
            *q2 = bucket_copy[p2];
            break;
        }
    }
    
    short_cut_left[spl] = p2-1;
    short_cut_right[spr] = p2+1;

    for( unsigned int i=0, l=0; i<=spl; i++ ) {
        unsigned int r = short_cut_left[i];
        if( r > p1 ) {
            for( ; ; ) {
                unsigned int ret = partition( bucket_copy, l, r );
                if( ret > p1 ) {
                    r = ret-1;
                } else if( ret < p1 ) {
                    l = ret+1;
                } else {
                    *q1 = bucket_copy[p1];
                    break;
                }
            }
            break;
        } else if( r < p1 ) {
            l = r;
        } else {
            *q1 = bucket_copy[p1];
            break;
        }
    }

    for( unsigned int i=0, r=end; i<=spr; i++ ) {
        unsigned int l = short_cut_right[i];
        if( l < p3 ) {
            for( ; ; ) {
                unsigned int ret = partition( bucket_copy, l, r );
                if( ret > p3 ) {
                    r = ret-1;
                } else if( ret < p3 ) {
                    l = ret+1;
                } else {
                    *q3 = bucket_copy[p3];
                    break;
                }
            }
            break;
        } else if( l > p3 ) {
            r = l;
        } else {
            *q3 = bucket_copy[p3];
            break;
        }
    }

}

unsigned int partition(unsigned int * buf, unsigned int left, unsigned int right) 
{
    if( left == right ) {
        return left;
    }
    if( left+1 == right ) {
        if( buf[left] > buf[right] ) {
            SWAP_UINT( buf[left], buf[right] );
        }
        return left;
    }
        
    unsigned int ret = left, pivot = (left + right)>>1;
    
    unsigned int val = buf[pivot];
    
    buf[pivot] = buf[right];
    buf[right] = val;
    
    for( unsigned int i = left; i < right; i++ ) {
        if( buf[i] < val ) {
            SWAP_UINT( buf[ret], buf[i] );
            ret++;
        }
    }
    buf[right] = buf[ret];
    buf[ret] = val;
    
    return ret;
}


