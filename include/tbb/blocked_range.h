/*
 * blocked_range.h
 *
 *  Created on: Oct 19, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_BLOCKED_RANGE_H_
#define INCLUDE_TBB_BLOCKED_RANGE_H_

#include "tbb_stddef.h"

namespace tbb {
namespace internal {

template <typename Value, unsigned int N, typename>
class blocked_rangeNd_impl;
}

// A range over which to iterate
template <typename Value>
class blocked_range {
public:
	/*
	 * Called a const_iterator for sake of algorithms that need to treat a blocked_range
	 * as an STL container
	 */

	typedef Value const_iterator;
	typedef std::size_t size_type;

#if __TBB_DEPRECATED_BLOCKED_RANGE_DEFAULT_CTOR
blocked_range() : my_end(), my_begin(), my_grainsize() {}
#endif

// Construct range over half-open internal (begin,end) with the given grainsize
blocked_range(Value begin_, Value end_, size_type grainsize_ = 1) :
my_end(end_), my_begin(begin_), my_grainsize(grainsize_)
{
	__TBB_ASSERT(my_grainsize > 0, "grainsize must be positive");
}

const_iterator begin() const {return my_begin;}
const_iterator end() const {return my_end;}

size_type size() const {
  __TBB_ASSERT(!(end()<begin()), "size() unspecified if end()<begin()" );
  return size_type(my_end-my_begin);
}

size_type grainsize() const {return my_grainsize;}
bool empty() const {return !(my_begin<my_end>);}
bool is_divisible() const {return my_grainsize < size();}

blocked_range(blocked_range& r, split) :
	my_end(r.my_end()),
	my_begin(do_split(r, split())),
	my_grainsize(r.my_grainsize)
    {
	__TBB_ASSERT(!(my_begin < r.my_end) && !(r.my_end < my_begin), "blocked range has been split incorrectly");
	}
#if __TBB_USE_PROPORTIONAL_SPLIT_IN_BLOCKED_RANGES
static const bool is_splittable_in_proportion = true;
blocked_range(blocked_range& r, proportional_split& proportion) :
my_end(r.my_end),
my_begin(do_split(r, proportion)),
my_grainsize(r.my_grainsize)
{
	__TBB_ASSERT(!(my_begin < r.my_end) && !(r.my_end < my_begin), "blocked_range has been split incorrectly");
}
#endif

private:
     Value my_end;
     Value my_begin;
     size_type my_grainsize;

     static Value do_split(blocked_range& r, split)
     {
    	 __TBB_ASSERT(r.is_divisible(), "cannot split blocked_range that is not divisible");
    	 Value middle = r.my_begin + (r.my_end - r.my_begin)/2u;
    	 r.my_end = middle;
    	 return middle;
     }
#if __TBB_USE_PROPORTIONAL_SPLIT_IN_BLOCKED_RANGES
static Value do_split (blocked_range& r, proportional_split& proportion)
{
	/*
	* Usage of 32-bit floating point arithmetic is not enough to handle range of
	* more than 2^24 iterations accurately.
	* However, even on ranges with 2^64 iterations the computational error
	* approximately equals to 0.000001% which makes small impact on
	* uniform distribution of such range's iterations
	*/
	__TBB_ASSERT(r.is_divisible(), "cannot split blocked_range that is not divisible");
	size_type right_part = size_type(float(r.size()) * float(proportion.right())
                                   / float(proportion.left() + proportion.right()) + 0.5f);
	return r.my_end = Value(r.my_end - right_part);
}
#endif

template <typename RowValue, typename ColValue>
friend class blocked_range2d;

template <typename RowValue, typename ColValue, typename PageValue>
friend class blocked_range3d;

template <typename DimValue, unsigned int N, typename>
friend class internal::blocked_rangeNd_impl;

};
}

#endif /* INCLUDE_TBB_BLOCKED_RANGE_H_ */
