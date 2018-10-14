/*
 * aligned_space.h
 *
 *  Created on: Oct 14, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_ALIGNED_SPACE_H_
#define INCLUDE_TBB_ALIGNED_SPACE_H_

#include "tbb_stddef.h"
#include "tbb_machine.h"

/*
 * Block of space aligned sufficiently to construct an array T with N elements
 */
namespace tbb {
template<typename T, size_t N=1>
class aligned_space {
private:
	typedef __TBB_TypeWithAlignmentAtLeastAsStrict(T) element_type;
	element_type array[(sizeof(T)*N+sizeof(element_type)-1)/sizeof(element_type)];
public:
	// Pointer to beginning of array
	T *begin() const {return internal::punned_cast<T*>(this);}
	// Pointer to one past last element in array
	T *end() const {return begin()+N;}
};
}

#endif /* INCLUDE_TBB_ALIGNED_SPACE_H_ */
