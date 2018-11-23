/*
 * _mutex_padding.h
 * https://github.com/01org/tbb/blob/tbb_2019/include/tbb/internal/_mutex_padding.h
 *
 *  Created on: Nov 22, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_INTERNAL__MUTEX_PADDING_H_
#define INCLUDE_TBB_INTERNAL__MUTEX_PADDING_H_

#include <cstddef>
#include <iostream>

/*
* Wrapper for padding mutexes to be alone on a cache line, without requiring they be allocated from a pool.
* Becuase we allow them to be defined anywhere they must be two cache lines in size 
*/

namespace tbb 
{
    namespace interface7 
    {
        namespace internal 
        {
            static const size_t cache_line_size = 64;
            /*
            * Pad a mutex to occupy a number of full cache line sufficient
            * to avoid false sharing with other data. 
            */
           template <typename Mutex, bool is_rw> class padded_mutex;
           template <typename Mutex>
           class padded_mutex<Mutex, false> : tbb::interface7::internal::mutex_copy_deprecated_and_disabled {
               typedef long pad_type;
               pad_type my_pad[((sizeof(Mutex)+cache_line_size-1)/cache_line_size+1)*cache_line_size/sizeof(pad_type)];
               Mutex * impl() {return (Mutex*)((uintptr_t(this)|(cache_line_size-1))+1);}

               public:
               static const bool is_rw_mutex = Mutex::is_rw_mutex;
               static const bool is_recursive_mutex = Mutex::is_recursive_mutex;
               static const bool is_fair_mutex = Mutex::is_fair_mutex;

               padded_mutex() {new(impl()) Mutex();}
               ~padded_mutex() {impl()->~Mutex();}

               class scoped_lock : tbb::interface7::internal::no_copy()
               {
                   typename Mutex::scoped_lock my_scoped_lock;
                   public: 
                   scoped_lock() : my_scoped_lock() {}
                   scoped_lock(padded_mutex& m) : my_scoped_lock(*m.impl()) {}
                   ~scoped_lock() {}

                   void acquire(padded_mutex& m) {my_scoped_lock.acquire(*m.impl());}
                   bool try_acquire(padded_mutex& m) {return my_scoped_lock.try_acquire(*m.impl());}
                   void release() {my_scoped_lock.release();}
               };
           };

           template <typename Mutex>
           class padded_mutex<Mutex, true> : tbb::interface7::internal::mutex_copy_deprecated_and_disabled {
               typedef long pad_type;
               pad_type my_pad[((sizeof(Mutex)+cache_line_size-1)/cache_line_size+1)*cache_line_size/sizeof(pad_type)];
               Mutex *impl() {return (Mutex*)((uintptr_t(this)|(cache_line_size-1))+1);}

               public:
               static const bool is_rw_mutex = Mutex::is_rw_mutex;
               static const bool is_recursive_mutex = Mutex::is_recursive_mutex;
               static const bool is_fair_mutex = Mutex::is_fair_mutex;

               padded_mutex() {new(impl()) Mutex();}
               ~padded_mutex() {impl()->~Mutex();}

               class scoped_lock : tbb::interface7::internal::no_copy
               {
                   typename Mutex::scoped_lock my_scoped_lock;
                   public: 
                   scoped_lock() : my_scoped_lock() {}
                   scoped_lock(padded_mutex& m, bool write = true) : my_scoped_lock(*m.impl(), write) {}
                   ~scoped_lock() {}

                   void acquire (padded_mutex& m, bool write = true) {my_scoped_lock.acquire(*m.impl(), write);}
                   bool try_acquire (padded_mutex& m, bool write = true) {return my_scoped_lock.try_acquire(*m.impl(), write);}
                   bool upgrade_to_writer() {return my_scoped_lock.upgrade_to_writer();}
                   bool downgrade_to_reader() {return my_scoped_lock.downgrade_to_reader();}
                   void release() {my_scoped_lock.release();}
               };
           };
        }
    }
}
#endif /* INCLUDE_TBB_INTERNAL__MUTEX_PADDING_H_ */
