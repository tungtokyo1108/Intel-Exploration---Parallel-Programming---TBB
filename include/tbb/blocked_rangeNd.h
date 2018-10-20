/*
 * blocked_rangeNd.h
 *
 *  Created on: Oct 19, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_BLOCKED_RANGEND_H_
#define INCLUDE_TBB_BLOCKED_RANGEND_H_

#include "tbb_config.h"
#include <array>
#include <algorithm>
#include <type_traits>
#include "tbb/blocked_range.h"
#include "internal/_template_helper.h"

namespace tbb {
namespace internal {

template <typename Value, unsigned int N, typename = make_index_sequence<N>>
		class blocked_rangeNd_impl;

template <typename Value, unsigned int N, std::size_t... Is>
class blocked_rangeNd_impl<Value, N, index_sequence<Is...>> {
public:
	using value_type = Value;
private:
	template <std::size_t>
	using dim_type_helper = tbb::blocked_range<value_type>;
	__TBB_STATIC_ASSERT(N != 0, "zero dimensional blocked_rangeNd can't be constructed");
	std::array<tbb::blocked_range<value_type>, N> my_dims;

public:
	blocked_rangeNd_impl() = delete;

	// Constructs N-dimensional range over N half-open internals each represented as tbb::blocked_range<Value>
	blocked_rangeNd_impl(const dim_type_helper<Is>&... args) : my_dims{{args...}} {}

	static constexpr unsigned int ndims() {return N;}

	const tbb::blocked_range<value_type>& dim(unsigned int dimension) const {
		__TBB_ASSERT(dimension < N, "out of bound");
		return my_dims[dimension];
	}

	bool empty() const {
		return std::any_of(my_dims.begin(), my_dims.end(), [](const tbb::blocked_range<value_type>& d) {
			return d.empty();
		});
	}

	bool is_divisible() const {
		return std::any_of(my_dims.begin(), my_dims.end(), [](const tbb::blocked_range<value_type>& d) {
			return d.is_divisible();
		});
	}

#if __TBB_USE_PROPORTIONAL_SPLIT_IN_BLOCKED_RANGES
	static const bool is_splittable_in_proportion = true;
	blocked_rangeNd_impl(blocked_rangeNd_impl& r, proportional_split proportion) : my_dims(r.my_dims) {
		do_split(r,proportion);
	}
#endif

	blocked_rangeNd_impl(blocked_rangeNd_impl& r, split proportion) : my_dims(r.my_dims) {
		do_split(r, proportion);
	}

private:

	template <typename split_type>
	void do_split(blocked_rangeNd_impl& r, split_type proportion)
	{
		__TBB_STATIC_ASSERT((is_same_type<split_type, split>::value ||
				             is_same_type<split_type, proportional_split>::value),
				             "type of split object is incorrect");
		auto my_it = std::max_element(my_dims.begin(), my_dims.end(),
				[](const tbb::blocked_range<value_type>& first, const tbb::blocked_range<value_type>& second) {
			return (first.size() * second.grainsize() < second.size() * first.grainsize());
		});

		auto r_it = r.my_dims.begin() + (my_it - my_dims.begin());
		my_it->my_begin = tbb::blocked_range<value_type>::do_split(*r_it, proportion);

	}
};

}

template <typename Value, unsigned int N>
using blocked_rangeNd = internal::blocked_rangeNd_impl<Value, N>;

}

#endif /* INCLUDE_TBB_BLOCKED_RANGEND_H_ */
