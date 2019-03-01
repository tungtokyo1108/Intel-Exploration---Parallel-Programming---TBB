/*
 * spin_mutex.h
 * 
 * https://github.com/01org/tbb/blob/tbb_2019/include/tbb/spin_mutex.h
 *
 *  Created on: Mar 1, 2019
 *  Data Scientist: Tung Dang
 */

#ifndef __TBB_spin_mutex_H
#define __TBB_spin_mutex_H

#include <cstddef>
#include <new>
#include "aligned_space.h"
#include "tbb_stddef.h"
#include "tbb_machine.h"
#include "tbb_profiling.h"
#include "internal/_mutex_padding.h"

namespace tbb 
{
    /**
     * A spin_mutex is a spin mutex that fits in single byte.
     * It should be used only for locking short critical sections
     * (typically less than 20 instructions) when fairness is not an issue.
    */  
    
    class spin_mutex : internal::mutex_copy_deprecated_and_disabled
    {
        __TBB_atomic_flag flag;
        public: 
            spin_mutex() : flag(0) 
            {
            #if TBB_USE_THREADING_TOOLS
                internal_construct();
            #endif   
            }

            class scoped_lock : internal::no_copy 
            {
                private: 
                    // Points to currently held mutex, or NULL if no lock is held.
                    spin_mutex* my_mutex;

                    /**
                     * Value to store into spin_mutex::flag to unclock the mutex
                     * This variable is no longer used. 
                     * Instead, 0 and 1 are used to represent that the lock is free and acquired, respectively 
                    */
                    __TBB_Flag my_unlock_value;

                    void __TBB_EXPORTED_METHOD internal_acquire(spin_mutex& m);
                    bool __TBB_EXPORTED_METHOD internal_try_acquire(spin_mutex& m);
                    void __TBB_EXPORTED_METHOD internal_release(spin_mutex& m);
                    friend class spin_mutex;
                
                public: 
                    scoped_lock() 
                        : my_mutex(NULL), my_unlock_value(0) 
                    {}

                    scoped_lock(spin_mutex& m) 
                        : my_unlock_value(0)
                    {
                        internal::suppress_unused_warning(my_unlock_value);
                    #if TBB_USE_THREADING_TOOLS||TBB_USE_ASSERT
                        my_mutex = NULL;
                        internal_acquire(m);
                    #else
                        my_mutex = &m;
                        __TBB_LockByte(m.flag);
                    #endif
                    }

                    void acquire(spin_mutex& m)
                    {
                    #if TBB_USE_THREADING_TOOLS||TBB_USE_ASSERT
                        internal_acquire(m);
                    #else
                        my_mutex = &m;
                        __TBB_LockByte(m.flag);
                    #endif
                    }

                    bool try_acquire(spin_mutex& m)
                    {
                    #if TBB_USE_THREADING_TOOLS||TBB_USE_ASSERT
                        return internal_try_acquire(m);
                    #else
                        bool result = __TBB_TryLockByte(m.flag);
                        if (result)
                            my_mutex = &m;
                        return result;
                    #endif
                    }

                    void release() 
                    {
                    #if TBB_USE_THREADING_TOOLS||TBB_USE_ASSERT
                        internal_release();
                    #else
                        __TBB_UnlockByte(my_mutex->flag);
                        my_mutex = NULL;
                    #endif
                    }

                    ~scoped_lock() 
                    {
                        if (my_mutex)
                        {
                    #if TBB_USE_THREADING_TOOLS||TBB_USE_ASSERT
                        internal_release();
                    #else
                        __TBB_UnlockByte(my_mutex->flag);
                    #endif
                        }
                    }
            };

            void __TBB_EXPORTED_METHOD internal_construct();

            static const bool is_rw_mutex = false;
            static const bool is_recursive_mutex = false;
            static const bool is_fair_mutex = false;

            void lock()
            {
            #if TBB_USE_THREADING_TOOLS
                aligned_space <scoped_lock> tmp;
                new(tmp.begin()) scoped_lock(*this);
            #else
                __TBB_UnlockByte(my_mutex->flag);
            #endif
            }

            bool try_lock()
            {
            #if TBB_USE_THREADING_TOOLS
                aligned_space <scoped_lock> tmp;
                return (new(tmp.begin()) scoped_lock)->internal_try_acquire(*this);
            #else
                __TBB_UnlockByte(my_mutex->flag);
            #endif
            }

            void unlock() 
            {
            #if TBB_USE_THREADING_TOOLS
                aligned_space <scoped_lock> tmp;
                scoped_lock& s = *tmp.begin();
                s.my_mutex = this;
                s.internal_release();
            #else
                __TBB_UnlockByte(my_mutex->flag);
            #endif
            }
            friend class scoped_lock;
    };
    __TBB_DEFINE_PROFILING_SET_NAME(spin_mutex);
}

#endif
