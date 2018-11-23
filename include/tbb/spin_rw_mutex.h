/*
 * spin_rw_mutex.h
 * https://github.com/01org/tbb/blob/tbb_2019/include/tbb/spin_rw_mutex.h
 *
 *  Created on: Nov 22, 2018
 *      Author: tungdang
 */

#ifndef INCLUDE_TBB_SPIN_RW_MUTEX_H_
#define INCLUDE_TBB_SPIN_RW_MUTEX_H_

#include "tbb_stddef.h"
#include "tbb_machine.h"
#include "tbb_profiling.h"
#include "internal/_mutex_padding.h"

namespace tbb 
{
    #if __TBB_TSX_AVAILABLE
    namespace interface8 { namespace internal {
        class x86_rtm_rw_mutex;
    }}
    #endif

    class spin_rw_mutex_v3;
    typedef spin_rw_mutex_v3 spin_rw_mutex;

    class spin_rw_mutex_v3 : internal::mutex_copy_deprecated_and_disabled {
        bool __TBB_EXPORTED_METHOD internal_acquire_writer();
        void __TBB_EXPORTED_METHOD internal_release_writer();
        void __TBB_EXPORTED_METHOD internal_acquire_reader();
        bool __TBB_EXPORTED_METHOD internal_upgrade();
        void __TBB_EXPORTED_METHOD internal_downgrade();
        void __TBB_EXPORTED_METHOD internal_release_reader();
        bool __TBB_EXPORTED_METHOD internal_try_acquire_writer();
        bool __TBB_EXPORTED_METHOD internal_try_acquire_reader();

        public: 
        spin_rw_mutex_v3() : state(0) {
            #if TBB_USE_THREADING_TOOLS
            internal_construct();
            #endif
        }

        #if TBB_USE_ASSERT
        ~spin_rw_mutex_v3() {
            __TBB_ASSERT(!state, "destruction of an acquired mutex");
        };
        #endif

        /*
        * The scoped locking pattern: 
        * - helps to avoid the common problem of forgetting to release lock.
        * - provides the node for quering locks  
        */
       class scoped_lock : internal::no_copy {
           #if __TBB_TSE_AVAILABLE
           friend class tbb::interface8::internal::x86_rtm_rw_mutex;
           #endif
           public:
           scoped_lock() : mutex(NULL), is_writer(false) {}
           scoped_lock(spin_rw_mutex& m, bool write = true) : mutex(NULL) {
               acquire(m, write);
           }

           ~scoped_lock() {
               if (mutex) release();
           }

           void acquire(spin_rw_mutex& m, bool write = true) 
           {
               __TBB_ASSERT(!mutex, "holding mutex already");
               is_writer = write;
               mutex = &m;
               if (write)
               {
                   mutex->internal_acquire_writer();
               }
               else
               {
                   mutex->internal_acquire_reader();
               }
           }

           void release() 
           {
               __TBB_ASSERT( mutex, "mutex is not acquired" );
               spin_rw_mutex *m = mutex;
               mutex = NULL;
               #if TBB_USE_THREADING_TOOLS || TBB_USE_ASSERT
               if (is_writer)
               {
                   m->internal_release_writer();
               }
               else
               {
                   m->internal_release_reader();
               }
               #else
               if (is_writer)
               {
                   __TBB_AtomicAND(&m->state, READERS);
               }
               else
               {
                   __TBB_FetchAndAddWrelease(&m->state, -(intptr_t)ONE_READER);
               }
               #endif
           }

           bool downgrade_to_reader() 
           {
               __TBB_ASSERT(mutex, "mutex is not acquired");
               __TBB_ASSERT(is_writer, "not a writer");
               #if TBB_USE_THREADING_TOOLS || TBB_USE_ASSERT
               mutex->internal_downgrade();
               #else
               __TBB_FetchAndAddWrelease(&mutex->state, ((intptr_t)ONE_READER-WRITER));
               #endif
               is_writer = false;
               return true;
           }

           bool try_acquire(spin_rw_mutex& m, bool write = true) 
           {
               __TBB_ASSERT(!mutex, "holding mutex already");
               bool result;
               is_writer = write;
               result == write ? m.internal_try_acquire_writer()
                              : m.internal_try_acquire_reader();
               if (result)
               {
                   mutex = &m;
               } 
               return result;              
           }

           protected:
           /* The pointer to the current mutex that is held, or NULL if no mutex is held*/
           spin_rw_mutex* mutex;
           bool is_writer;
       };

       static const bool is_rw_mutex = true;
       static const bool is_recursive_mutex = false;
       static const bool is_fair_mutex = false;

       void lock() {internal_acquire_writer();}
       bool try_lock() {return internal_try_acquire_writer();}
       void unlock()
       {
           #if TBB_USE_THREADING_TOOLS || TBB_USE_ASSERT
           if (state&WRITER) internal_release_writer();
           else              internal_release_reader();
           #else
           if (state&WRITER)
           {
               __TBB_AtomicAND(&state, READERS);
           }
           else
           {
               __TBB_FetchAndAddWrelease(&state, -(intptr_t)ONE_READER);
           }
           #endif
       }

       void lock_read() {internal_acquire_reader();}
       bool try_lock_read() {return internal_try_acquire_reader();}

       protected:
       typedef intptr_t state_t;
       static const state_t WRITER = 1;
       static const state_t WRITER_PENDING = 2;
       static const state_t READERS = ~(WRITER | WRITER_PENDING);
       static const state_t ONE_READER = 4;
       static const state_t BUSY = WRITER | READERS;
       state_t state;

       private:
       void __TBB_EXPORTED_METHOD internal_construct();
    };

    __TBB_DEFINE_PROFILING_SET_NAME(spin_rw_mutex);
}

namespace tbb 
{
    namespace interface8 
    {
        /*
        * A cross-platform spin reader/writer mutex with speculative lock acquisition
        * - may speculatively execute its critical sections,
        * - using HW mechanism to detect real data races and ensure atomicity of the critical section.
        */
        #if __TBB_TSX_AVAILABLE
        typedef interface7::internal::padded_mutex<tbb::interface8::internal::x86_rtm_rw_mutex, true> speculative_spin_rw_mutex;
        #else
        typedef interface7::internal::padded_mutex<tbb::spin_rw_mutex,true> speculative_spin_rw_mutex;
        #endif
    }

    using interface8::speculative_spin_rw_mutex;
    __TBB_DEFINE_PROFILING_SET_NAME(speculative_spin_rw_mutex);
}

#endif /* INCLUDE_TBB_SPIN_RW_MUTEX_H_ */
