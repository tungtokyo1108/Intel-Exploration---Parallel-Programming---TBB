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

namespace tbb 
{
    namespace interface5 
    {
        template <typename Key, typename T, typename HashCompare = tbb_hash_compare<Key>, typename A = tbb_allocator<std::pair<Key,T>>>
        class concurrent_hash_map;

        namespace internal 
        {
            using namespace tbb::internal;

            // Type of a hash code
            typedef size_t hashcode_t;

            struct hash_map_node_base : tbb::internal::no_copy {
                typedef spin_rw_mutex mutex_t;
                typedef mutex_t::scoped_lock scoped_t;

                // Next node in chain 
                hash_map_node_base *next;
                mutex_t mutex;
            };
            static hash_map_node_base *const rehash_req = reinterpret_cast<hash_map_node_base*>(size_t(3));
            static hash_map_node_base *const empty_rehash = reinterpret_cast<hash_map_node_base*>(size_t(0));
            
            class hash_map_base 
            {
                public:
                typedef size_t size_type;
                
                // Type of a hash code
                typedef size_t hashcode_t;
                
                // Segment of index type 
                typedef size_t segment_index_t;
                
                typedef hash_map_node_base node_base;

                struct bucket : tbb::internal::no_copy {
                    typedef spin_rw_mutex mutex_t;
                    typedef mutex_t::scoped_lock scoped_t;
                    mutex_t mutex;
                    node_base *node_list;
                };

                static size_type const embedded_block = 1;
                static size_type const embedded_buckets = 1<<embedded_block;
                static size_type const first_block = 8;
                
                // Size of a pointer
                static size_type const pointers_per_table = sizeof(segment_index_t) * 8;
                
                // Segment pointers 
                typedef bucket *segment_ptr_t;
                
                // Segment pointers table type 
                typedef segment_ptr_t segments_table_t[pointers_per_table];
                atomic<hashcode_t> my_mask;
                
                // Segment pointers table. Also prevents false sharing between my_mask and my_size
                segments_table_t my_table;

                /*
                * Size of container in stored items 
                * It must be in separate cache line from my_mask due to performance effects
                */
                atomic<size_type> my_size;
                bucket my_embedded_segment[embedded_buckets];

                #if __TBB_STATISTICS
                atomic<unsigned> my_info_resizes;
                mutable atomic<unsigned> my_info_restarts;
                atomic<unsigned>my_info_rehashes;
                #endif
                
                hash_map_base()
                {
                    std::memset(this, 0, pointers_per_table * sizeof(segment_ptr_t) 
                    + sizeof(my_size) + sizeof(my_mask) 
                    + embedded_buckets*sizeof(bucket));
                    for (size_type i = 0; i < embedded_block; i++)
                        my_table[i] = my_embedded_segment + segment_base(i);
                    my_mask = embedded_buckets - 1;
                    __TBB_ASSERT(embedded_block <= first_block, "The first block number must include embedded blocks");  
                    #if __TBB_STATISTICS
                    my_info_resizes = 0;
                    my_info_restarts = 0;
                    my_info_rehashes = 0;
                    #endif  
                }

                static segment_index_t segment_index_of(size_type index)
                {
                    return segment_index_t(__TBB_Log2( index|1 ));
                }

                static segment_index_t segment_base(segment_index_t k) 
                {
                    return (segment_index_t(1)<<k & ~segment_index_t(1));
                }

                static size_type segement_size(segment_index_t k) 
                {
                    return size_type(1)<<k;
                }

                static bool is_valid(void *ptr)
                {
                    return reinterpret_cast<uintptr_t>(ptr) > uintptr_t(63);
                }
            };
        }
    }  
}

#endif /* INCLUDE_TBB_CONCURRENT_HASH_MAP_H_ */
