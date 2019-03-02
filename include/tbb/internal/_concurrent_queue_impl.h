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
#include "spin_mutex.h"
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
                
                public: 
                    friend class micro_queue_pop_finalizer<T>;

                    struct padded_page: page 
                    {
                        padded_page();
                        void operator=(const padded_page&);
                        T last;
                    };

                    static T& get_ref(page& p, size_t index) 
                    {
                        return (&static_cast<padded_page*>(static_cast<void*>(&p))->last)[index];
                    }

                    atomic<page*> head_page;
                    atomic<ticket> head_counter;
                    atomic<page*> tail_page;
                    atomic<ticket> tail_counter;
                    spin_mutex page_mutex;

                    void push(const void* item, ticket k, concurrent_queue_base_v3<T>& base, 
                                item_constructor_t construct_item);
                    bool pop(void* dst, ticket k, concurrent_queue_base_v3<T>& base);
                    micro_queue& assign(const micro_queue& src, concurrent_queue_base_v3<T>& base, 
                                            item_constructor_t construct_item);
                    page* make_copy(concurrent_queue_base_v3<T>& base, const page* src_page, size_t begin_in_page,
                        size_t end_in_page, ticket& g_index, item_constructor_t construct_item);
                    void invalidate_page_and_rethrow(ticket k);
            };

            template <typename T>
            void micro_queue<T>::spin_wait_until_my_turn(atomic<ticket>& counter, ticket k, concurrent_queue_rep_base& rb) const 
            {
                for (atomic_backoff b(true);; b.pause())
                {
                    ticket c = counter;
                    if (c == k) return;
                    else if (c&1)
                    {
                        ++rb.n_invalid_entries;
                        throw_exception(eid_bad_last_alloc);
                    }
                }
            }

            template <typename T>
            void micro_queue<T>::push(const void* item, ticket k, concurrent_queue_base_v3<T>& base, 
                                        item_constructor_t construct_item)
            {
                k &= -concurrent_queue_rep_base::n_queue;
                page* p = NULL;
                size_t index = modulo_power_of_two(k/concurrent_queue_rep_base::n_queue, 
                                                    base.my_rep->items_per_page);
                if (!index)
                {
                    __TBB_TRY {
                        concurrent_queue_page_allocator& pa = base;
                        p = pa.allocate_page();
                    } __TBB_CATCH(...) {
                        ++base.my_rep->n_invalid_entries;
                        invalidate_page_and_rethrow(k);
                    }
                    p->mask = 0;
                    p->next = NULL;
                }

                if (tail_counter != k) spin_wait_until_my_turn(tail_counter, k, *base.my_rep);
                call_itt_notify(acquired, &tail_counter);

                if (p) 
                {
                    spin_mutex::scoped_lock lock(page_mutex);
                    page* q = tail_page;
                    if (is_valid_page(q))
                    {
                        q->next = p;
                    }
                    else 
                    {
                        head_page = p;
                    }
                    tail_page = p;
                }
                else 
                {
                    p = tail_page;
                }

                __TBB_TRY {
                    copy_item(*p, index, item, construct_item);
                    itt_hide_store_word(p->mask, p->mask | uintptr_t(1)<<index);
                    call_itt_notify(releasing, &tail_counter);
                    tail_counter += concurrent_queue_rep_base::n_queue;
                } __TBB_CATCH(...) {
                    ++base.my_rep->n_invalid_entries;
                    call_itt_notify(releasing, &tail_counter);
                    tail_counter += concurrent_queue_rep_base::n_queue;
                    __TBB_RETHROW();
                }
            }

            template <typename T>
            bool micro_queue<T>::pop(void* dst, ticket k, concurrent_queue_base_v3<T>& base)
            {
                k &= -concurrent_queue_rep_base::n_queue;
                if (head_counter != k) spin_wait_until_eq(head_counter, k);
                call_itt_notify(acquired, &head_counter);
                if (tail_counter == k) spin_wait_while_eq(tail_counter, k);
                call_itt_notify(acquired, &tail_counter);
                page *p = head_page;
                __TBB_ASSERT(p, NULL);
                size_t index = modulo_power_of_two(k/concurrent_queue_rep_base::n_queue, base.my_rep->items_per_page);
                bool success = false;
                {
                    micro_queue_pop_finalizer<T> finalizer(*this, base, k+concurrent_queue_rep_base::n_queue, 
                                                    index==base.my_rep->items_per_page-1 ? p : NULL);
                    if (p->mask & uintptr_t(1)<<index)
                    {
                        success = true;
                        assign_and_destroy_item(dst, *p, index);
                    }
                    else 
                    {
                        --base.my_rep->n_invalid_entries;
                    }
                }
                return success;
            }

            template <typename T>
            micro_queue<T>& micro_queue<T>::assign(const micro_queue<T>& src, concurrent_queue_base_v3<T>& base, 
                item_constructor_t construct_item)
            {
                head_counter = src.head_counter;
                tail_counter = src.tail_counter;

                const page* srcp = src.head_page;
                if (is_valid_page(srcp))
                {
                    ticket g_index = head_counter;
                    __TBB_TRY {
                        size_t n_items = (tail_counter - head_counter)/concurrent_queue_rep_base::n_queue;
                        size_t index = modulo_power_of_two(head_counter/concurrent_queue_rep_base::n_queue, 
                                        base.my_rep->items_per_page);
                        size_t end_in_first_page = (index + n_items < base.my_rep->items_per_page) 
                                ? (index + n_items):base.my_rep->items_per_page;

                        head_page = make_copy(base, srcp, index, end_in_first_page, g_index, construct_item);
                        page* cur_page = head_page;

                        if (srcp != src.tail_page)
                        {
                            for (srcp = srcp->next; srcp != src.tail_page; srcp = srcp->next)
                            {
                                cur_page->next = make_copy(base, srcp, 0, base.my_rep->items_per_page, 
                                                            g_index, construct_item);
                                cur_page = cur_page->next;
                            }

                            __TBB_ASSERT(srcp == src.tail_page, NULL);
                            size_t last_index = modulo_power_of_two(tail_counter/concurrent_queue_rep_base::n_queue,
                                                                    base.my_rep->items_per_page);
                            if (last_index == 0) last_index = base.my_rep->items_per_page;
                            cur_page->next = make_copy(base, srcp, 0, last_index, g_index, construct_item);
                            cur_page = cur_page->next;
                        }
                        tail_page = cur_page;
                    } __TBB_CATCH(...) {
                        invalidate_page_and_rethrow(g_index);
                    }
                } else {
                    head_page = tail_page = NULL;
                }
                return *this;
            }
        }
    } 
}

#endif
