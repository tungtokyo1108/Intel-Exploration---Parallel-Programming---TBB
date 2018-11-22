/*
 * concurrent_hash_map.h
 * https://github.com/01org/tbb/blob/tbb_2019/include/tbb/concurrent_hash_map.h
 *
 *  Created on: Nov 22, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_CONCURRENT_HASH_MAP_H_
#define INCLUDE_TBB_CONCURRENT_HASH_MAP_H_

#include "tbb_stddef.h"
#include <iterator>
#include <utility>
#include <cstring>
#include __TBB_STD_SWAP_HEADER

#include "cache_aligned_allocator.h"
#include "tbb_allocator.h"
#include "spin_rw_mutex.h"
#include "atomic.h"
#include "tbb_exception.h"
#include "tbb_profiling.h"
#include "internal/_tbb_hash_compare_impl.h"
#if __TBB_INITIALIZER_LISTS_PRESENT
#include <initializer_list>
#endif
#if TBB_USE_PERFORMANCE_WARNINGS || __TBB_STATISTICS
#include <typeinfo>
#endif
#if __TBB_STATISTICS
#include <stdio.h>
#endif



#endif /* INCLUDE_TBB_CONCURRENT_HASH_MAP_H_ */
