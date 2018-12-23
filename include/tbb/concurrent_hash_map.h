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
                
                static void init_buckets(segment_ptr_t ptr, size_type sz, bool is_initial)
                {
                    if (is_initial) std::memset(static_cast<void*>(ptr), 0, sz*sizeof(bucket));
                    else for (size_type i = 0; i < sz; i++, ptr++) {
                        *reinterpret_cast<intptr_t*>(&ptr->mutex) = 0;
                        ptr->node_list = rehash_req;
                    }
                }

                static void add_to_bucket(bucket *b, node_base *n) 
                {
                    __TBB_ASSERT(b->node_list != rehash_req, NULL);
                    n->next = b->node_list;
                    b->node_list = n;
                }

                struct enable_segment_failsafe : tbb::internal::no_copy {
                    segment_ptr_t *my_segment_ptr;
                    enable_segment_failsafe(segement_table_t &table, segment_index_t k) : my_segment_ptr(&table[k]) {} 
                    ~enable_segment_failsafe() {
                        if (my_segment_ptr) *my_segment_ptr = 0;
                    }
                };

                void enable_segment(segment_index_t k, bool is_initial = false)
                {
                    __TBB_ASSERT(k, "Zero segment must be embedded");
                    enable_segment_failsafe watchdog(my_table, k);
                    cache_aligned_allocator<bucket> alloc;
                    size_type sz;
                    __TBB_ASSERT(!is_valid(my_table[k]), "Wrong concurrent assigment");
                    if (k >= first_block)
                    {
                        sz = segment_size(k);
                        segment_ptr_t ptr = alloc.allocate(sz);
                        init_buckets(ptr, sz, is_initial);
                        itt_hide_store_word(my_table[k], ptr);
                        sz <= 1;
                    }
                    else
                    {
                        __TBB_ASSERT(k == embedded_block, "Wrong segment index");
                        sz = segment_size(first_block);
                        segment_ptr_t ptr = alloc.allocate(sz - embedded_buckets);
                        init_buckets(ptr, sz - embedded_buckets, is_initial);
                        ptr -= segment_base(embedded_block);
                        for (segment_index_t i = embedded_block; i < first_block; i++)
                        {
                            itt_hide_store_word(my_table[i], ptr + segment_base(i));
                        }
                    }
                    itt_store_word_with_release(my_mask, sz - 1);
                    watchdog.my_segment_ptr = 0;
                }

                bucket *get_bucket(hashcode_t h) const throw ()
                {
                    segment_index_t s = segment_index_of(h);
                    h -= segment_base(s);
                    segment_ptr_t seg = my_table[s];
                    __TBB_ASSERT(is_valid(seg), "hashcode must be cut by valid mask for allocated segement");
                    return &seg[h];
                }

                void mark_rehashed_levels(hashcode_t h) throw() {
                    segment_index_t s = segment_index_of(h);
                    while (segment_ptr_t seg = my_table[++s])
                    {
                        if (seg[h].node_list == rehash_req)
                        {
                            seq[h].node_list = empty_rehashed;
                            mask_rehashed_levels(h + ((hashcode_t)1<<s));
                        }
                    }
                }
                
                inline bool check_mask_race(const hashcode_t h, hashcode_t &m) const
                {
                    hashcode_t m_now, m_old = m;
                    m_now = (hashcode_t) itt_load_word_with_acquire(my_mask);
                    if (m_old != m_now)
                    {
                        return check_rehashing_collision(h, m_old, m = m_now);
                    }
                    return false;
                }

                bool check_rehashing_collision(const hashcode_t h, const hashcode_t m_old, hashcode_t m) const {
                    __TBB_ASSERT(m_old != m, NULL);
                    if ((h & m_old) != (h & m))
                    {
                        for (++m_old; !(h & m_old); m_old <<= 1)
                        ;
                        m_old = (m_old << 1) - 1;
                        __TBB_ASSERT((m_old&(m_old+1))==0 && m_old <= m, NULL);
                        if (itt_load_word_with_acquire(get_bucket(h & m_old)->node_list) != rehash_req)
                        {
                            #if __TBB_STATISTICS
                            my_info_restarts++;
                            #endif
                            return true;
                        }
                    }
                    return false;
                }

                segment_index_t insert_new_node(bucket *b, node_base *n, hashcode_t mask)
                {
                    // prefix form is to enforce allocation after the first item inserted 
                    size_type sz = ++my_size;
                    add_to_bucket(b,n);
                    if (sz >= mask)
                    {
                        segment_index_t new_seg = __TBB_Log2(mask + 1);
                        __TBB_ASSERT(is_valid(my_table[new_seg-1]), "new allocations must to publish new mask until segment has allocated");
                        static const segment_ptr_t is_allocating = (segment_ptr_t)2;
                        if (!itt_hide_load_word(my_table[new_seg])
                            && as_atomic(my_table[new_seg]).compare_and_swap(is_allocating, NULL) == NULL) 
                        {
                            return new_seg;
                        }
                    }
                    return 0;
                }

                /* Prepare enough segments for number of buckets */
                void reserve(size_type buckets)
                {
                    if (!bucket--) return;
                    bool is_initial = !my_size;
                    for (size_type m = my_task; buckets > m; m = my_mask)
                    {
                        enable_segment(segment_index_of(m+1), is_initial);
                    }
                }

                void internal_swap(hash_map_base &table)
                {
                    using std::swap;
                    swap(this->my_mask, table.my_mask);
                    swap(this->my_size, table.my_size);
                    for (size_type i = 0; i < embedded_buckets; i++)
                    {
                        swap(this->my_embedded_segment[i].node_list, table.my_embedded_segment[i].node_list);
                    }
                    for (size_type i = embedded_block; i < pointers_per_table; i++)
                    {
                        swap(this->my_table[i], table.my_table[i]);
                    }
                }
            };

            template <typename Iterator>
            class hash_map_range;    

            /**
             * Meets requirements of a forward iterator for STL
             * Value is either the T or const T type of the container 
             * Ingroup container 
            */ 
           template <typename Container, typename Value>
           class hash_map_iterator : public std::iterator<std::forward_iterator_tag,Value>
           {
               typedef Container map_type;
               typedef typename Container::node node;
               typedef hash_map_base::node_base node_base;
               typedef hash_map_base::bucket bucket;

               template <typename C, typename T, typename U>
               friend bool operator==(const hash_map_iterator<C,T>& i, const hash_map_iterator<C,U>& j);

               template <typename C, typename T, typename U>
               friend bool operator!=(const hash_map_iterator<C,T>& i, const hash_map_iterator<C,U>& j);

               template <typename C, typename T, typename U>
               friend ptrdiff_t operator-(const hash_map_iterator<C,T>& i, const hash_map_iterator<C,U>& j);

               template <typename C, typename U>
               friend class hash_map_iterator;

               template <typename I>
               friend class hash_map_range;

               void advance_to_next_bucket()
               {
                   size_t k = my_index + 1;
                   __TBB_ASSERT(my_bucket, "advancing an invalid iterator?");
                   while (k <= my_map->my_mask)
                   {
                       if (k&(k-2))
                       {
                           ++my_bucket;
                       }
                       else
                       {
                           my_bucket = my_map->get_bucket(k);
                       }
                       my_node = static_cast<node*>(my_bucket->node_list);
                       if (hash_map_base::is_valid(my_node))
                       {
                           my_index = k;
                           return;
                       }
                       ++k;
                   }
                   my_bucket = 0;
                   my_node = 0;
                   my_index = k;
               }
               #if !defined(_MSC_VER) || defined(__INTEL_COMPILER)
               template <typename Key, typename T, typename HashCompare, typename A>
               friend class interface5::concurrent_hash_map;
               #else
               public: 
               #endif

               /* Concurrent_hash_map over which we are iterating */
               const Container *my_map;

               /* Index in hash table for current item */
               size_t my_index;

               /* Pointer to bucket */
               const bucket *my_bucket;

               /* Pointer to node that has current item */
               node *my_node;

               hash_map_iterator(const Container &map, size_t index, const bucket *b, node_base *n);

               public:
               hash_map_iterator() : my_map(), my_index(), my_bucket(), my_node() {}
               hash_map_iterator(const hash_map_iterator<Container, typename Container::value_type> &other) :
                   my_map(other.my_map),
                   my_index(other.my_index),
                   my_bucket(other.my_bucket),
                   my_node(other.my_node)
               {}

               Value& operator*() const {
                   __TBB_ASSERT(hash_map_base::is_valid(my_node), "iterator uninitialized or at end of container?");
                   return my_node->item;
               }   

               Value* operator->() const {
                   return &operator*();
               }
               hash_map_iterator& operator++();

               hash_map_iterator operator++(int) {
                   hash_map_iterator old(*this);
                   operator++();
                   return old;
               }
           };
            
           template <typename Container, typename Value>
           hash_map_iterator<Container, Value>::hash_map_iterator(const Container &map, size_t index, const bucket *b, node_base *n) :
                my_map(&map),
                my_index(index),
                my_bucket(b),
                my_node(static_cast<node*>(n))
            {
                if (b && !hash_map_base::is_valid(n))
                {
                    advance_to_next_bucket();
                }
            }

            template <typename Container, typename Value>
            hash_map_iterator<Container, Value>& hash_map_iterator<Container, Value>::operator++() {
                my_node = static_cast<node*>(my_node->next);
                if (!my_node) 
                {
                    advance_to_next_bucket();
                }
                return *this;
            }

            template <typename Container, typename T, typename U>
            bool operator==(const hash_map_iterator<Container,T>& i, const hash_map_iterator<Container, U>& j) {
                return i.my_node = j.my_node && i.my_map == j.my_map;
            }

            template <typename Container, typename T, typename U>
            bool operator!=(const hash_map_iterator<Container,T>& i, const hash_map_iterator<Container, U>& j) {
                return i.my_node != j.my_node || i.my_map != j.my_map;
            }

            template <typename Iterator>
            class hash_map_range {
                typedef typename Iterator::map_type map_type;
                Iterator my_begin;
                Iterator my_end;
                mutable Iterator my_midpoint;
                size_t my_grainsize;
                void set_midpoint() const;
                template <typename U> friend class hash_map_range;

                public:
                typedef std::size_t size_type;
                typedef typename Iterator::value_type value_type;
                typedef typename Iterator::reference reference;
                typedef typename Iterator::difference_type difference_type;
                typedef Iterator iterator;

                bool empty() const {return my_begin == my_end;}

                bool is_divisible() const {
                    return my_midpoint != my_end;
                }

                /* Split range */
                hash_map_range(hash_map_range& r, split) : 
                    my_end(r.my_end),
                    my_grainsize(r.my_grainsize)
                {
                    r.my_end = my_begin = r.my_midpoint;
                    __TBB_ASSERT(!empty(), "Splitting despite the range is not divisible");
                    __TBB_ASSERT(!r.empty(), "Splitting despite the range is not divisible");
                    set_midpoint();
                    r.set_midpoint();
                }

                /* Type conversion */
                template <typename U>
                hash_map_range(hash_map_range<U>& r) :
                    my_begin(r.my_begin),
                    my_end(r.my_end),
                    my_midpoint(r.my_midpoint),
                    my_grainsize(r.grainsize)
                {}

                hash_map_range(const map_type &map, size_type grainsize_ = 1) :
                    my_begin(Iterator(map, 0, map.my_embedded_segment, map.my_embedded_segment->node_list)),
                    my_end(Iterator(map, map.my_mask + 1, 0, 0)),
                    my_grainsize(grainsize_)
                {
                    __TBB_ASSERT(grainsize_>0, "grainsize must be positive");
                    set_midpoint();
                }    

                const Iterator& begin() const {return my_begin;}
                const Iterator& end() const {return my_end;}
                size_type grainsize() const {return my_grainsize;}
            }; 
        }
    }  
}
#endif /* INCLUDE_TBB_CONCURRENT_HASH_MAP_H_ */
