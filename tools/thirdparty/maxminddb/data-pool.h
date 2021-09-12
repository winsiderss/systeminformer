#ifndef DATA_POOL_H
#define DATA_POOL_H

#include "maxminddb.h"

#include <stdbool.h>
#include <stddef.h>

// This should be large enough that we never need to grow the array of pointers
// to blocks. 32 is enough. Even starting out of with size 1 (1 struct), the
// 32nd element alone will provide 2**32 structs as we exponentially increase
// the number in each block. Being confident that we do not have to grow the
// array lets us avoid writing code to do that. That code would be risky as it
// would rarely be hit and likely not be well tested.
#define DATA_POOL_NUM_BLOCKS 32

// A pool of memory for MMDB_entry_data_list_s structs. This is so we can
// allocate multiple up front rather than one at a time for performance
// reasons.
//
// The order you add elements to it (by calling data_pool_alloc()) ends up as
// the order of the list.
//
// The memory only grows. There is no support for releasing an element you take
// back to the pool.
typedef struct MMDB_data_pool_s {
    // Index of the current block we're allocating out of.
    size_t index;

    // The size of the current block, counting by structs.
    size_t size;

    // How many used in the current block, counting by structs.
    size_t used;

    // The current block we're allocating out of.
    MMDB_entry_data_list_s *block;

    // The size of each block.
    size_t sizes[DATA_POOL_NUM_BLOCKS];

    // An array of pointers to blocks of memory holding space for list
    // elements.
    MMDB_entry_data_list_s *blocks[DATA_POOL_NUM_BLOCKS];
} MMDB_data_pool_s;

MMDB_data_pool_s *data_pool_new(size_t const);
void data_pool_destroy(MMDB_data_pool_s *const);
MMDB_entry_data_list_s *data_pool_alloc(MMDB_data_pool_s *const);
MMDB_entry_data_list_s *data_pool_to_list(MMDB_data_pool_s *const);

#endif
