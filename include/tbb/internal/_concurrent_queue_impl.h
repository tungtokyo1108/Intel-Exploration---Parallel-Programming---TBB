/*
 * _concurrent_queue_impl.h
 * https://github.com/01org/tbb/blob/tbb_2019/include/tbb/internal/_concurrent_queue_impl.h
 *
 *  Created on: Feb 28, 2019
 *  Data Scientist: Tung Dang
 */

#ifndef __TBB__concurrent_queue_impl_H
#define __TBB__concurrent_queue_impl_H

#include "tbb_stddef.h"
#include "tbb_machine.h"
#include "atomic.h"
#include "spin_rw_mutex.h"
#include "cache_aligned_allocator.h"
#include "tbb_exception.h"
#include "tbb_profiling.h"
#include <new>
#include __TBB_STD_SWAP_HEADER
#include <iterator>

namespace tbb 
{
    #if !__TBB_TEMPLATE_FRIENDS_BROKEN
    namespace strict_ppl 
    {
        template <typename T, typename A> class concurrent_queue;
    }
    template <typename T, typename A> class concurrent_bounded_queue;
    #endif

    namespace strict_ppl
    {
        namespace internal 
        {
            using namespace tbb::internal;
            typedef size_t ticket;

            template <typename T> class micro_queue;
            template <typename T> class micro_queue_pop_finalizer;
            template <typename T> class concurrent_queue_base_v3;
            template <typename T> struct concurrent_queue_rep;

            struct concurrent_queue_rep_base : no_copy 
            {
                template <typename T> friend class micro_queue;
                template <typename T> friend class concurrent_queue_base_v3;

                protected: 
                // Approximately n_queue/golden ratio
                static const size_t phi = 3;

                public: 
                static const size_t n_queue = 8;
                struct page 
                {
                    page* next;
                    uintptr_t mask;
                };

                atomic<ticket> head_counter;
                char pad1[NFS_MaxLineSize-sizeof(atomic<ticket>)];
                atomic<ticket> tail_counter;
                char pad2[NFS_MaxLineSize-sizeof(atomic<ticket>)];

                size_t items_per_page;
                size_t item_size;

                atomic<size_t> n_invalid_entries;
                char pad3[NFS_MaxLineSize-sizeof(size_t)-sizeof(size_t)-sizeof(atomic<size_t>)];
            };

            inline bool is_valid_page(const concurrent_queue_rep_base::page* p)
            {
                return uintptr_t(p)>1;
            }

            class concurrent_queue_page_allocator
            {
                template <typename T> friend class micro_queue;
                template <typename T> friend class micro_queue_pop_finalizer;

                protected: 
                virtual ~concurrent_queue_page_allocator() {}

                private: 
                virtual concurrent_queue_rep_base::page* allocate_page() = 0;
                virtual void deallocate_page(concurrent_queue_rep_base::page* p) = 0;
            };

            // A queue using simple locking 
            template <typename T>
            class micro_queue : no_copy 
            {
                public: 
                    typedef void (*item_constructor_t)(T* location, const void* src);

                private:
                    typedef concurrent_queue_rep_base::page page;
                    class destroyer : no_copy
                    {
                        T& my_value;
                        public: 
                            destroyer(T& value) : my_value(value) {}
                            ~destroyer() {my_value.~T();}
                    };

                    void copy_item(page& dst, size_t dindex, const void* src, item_constructor_t construct_item)
                    {
                        construct_item(&get_ref(dst, dindex), src);
                    }

                    void copy_item(page& dst, size_t dindex, const page& src, size_t sindex, item_constructor_t construct_item)
                    {
                        T& src_item = get_ref(const_cast<page&>(src), sindex);
                        construct_item(&get_ref(dst, dindex), static_cast<const void*>(&src_item));
                    }

                    void assign_and_destroy_item(void* dst, page& src, size_t index)
                    {
                        T& from = get_ref(src, index);
                        destroyer d(from);
                        *static_cast<T*>(dst) = tbb::internal::move(from);
                    }

                    void spin_wait_until_my_turn(atomic<ticket>& counter, ticket k, concurrent_queue_rep_base& rb) const;
            };
        }
    } 
}

#endif
