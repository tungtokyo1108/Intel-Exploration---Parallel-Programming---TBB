/*
 * _concurrent_queue_impl.h
 * https://github.com/01org/tbb/blob/tbb_2019/include/tbb/internal/_concurrent_queue_impl.h
 *
 *  Created on: Feb 28, 2019
 *  Data Scientist: Tung Dang
 */

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

    
}
