/*
 * combinable.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_COMBINABLE_H_
#define INCLUDE_TBB_COMBINABLE_H_

#include "enumerable_thread_specific.h"
#include "cache_aligned_allocator.h"

namespace tbb {

template <typename T>
class combinable {
private:
	typedef typename tbb::cache_aligned_allocator<T> my_alloc;
	typedef typename tbb::interface6::enumerable_thread_specific<T,my_alloc,ets_no_key> my_ets_type;
	my_ets_type my_ets;

public:
	combinable() {}

	template <typename finit>
	explicit combinable(finit _finit) : my_ets(_finit) {}

#if __TBB_ETS_USE_CPP11
        combinable( combinable&& other) : my_ets( std::move(other.my_ets)) { }
#endif

    combinable & operator=(const combinable & other) {
    	my_ets = other.my_ets;
    	return *this;
    }

#if __TBB_ETS_USE_CPP11
        combinable & operator=( combinable && other) {
            my_ets=std::move(other.my_ets);
            return *this;
        }
#endif

    void clear() {my_ets.clear();}

    T& local() {return my_ets.local();}
    T& local(bool &exists) {return my_ets.local(exists);}

    template <typename combine_func_f>
    T combine(combine_func_f f_combine) {return my_ets.combine(f_combine);}

    template <typename combine_func_f>
    void combine_each(combine_func_f f_combine) {my_ets.combine_each(f_combine);}
};

}

#endif /* INCLUDE_TBB_COMBINABLE_H_ */
