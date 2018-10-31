/*
 * concurrent_vector.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_CONCURRENT_VECTOR_H_
#define INCLUDE_TBB_CONCURRENT_VECTOR_H_

#include "tbb_stddef.h"
#include "tbb_exception.h"
#include "atomic.h"
#include "cache_aligned_allocator.h"
#include "blocked_range.h"
#include "tbb_machine.h"
#include "tbb_profiling.h"
#include <new>
#include <cstring>
#include __TBB_STD_SWAP_HEADER
#include <algorithm>
#include <iterator>

#if _MSC_VER==1500 && !__INTEL_COMPILER
    #pragma warning( push )
    #pragma warning( disable: 4985 )
#endif
#include <limits> /* std::numeric_limits */
#if _MSC_VER==1500 && !__INTEL_COMPILER
    #pragma warning( pop )
#endif

#if __TBB_INITIALIZER_LISTS_PRESENT
    #include <initializer_list>
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
    #pragma warning (push)
#if defined(_Wp64)
    #pragma warning (disable: 4267)
#endif
    #pragma warning (disable: 4127) //warning C4127: conditional expression is constant
#endif

namespace tbb {

template <typename T, class A = cache_aligned_allocator<T>>
class concurrent_vector;

namespace internal {

template <typename Container, typename Value>
class vector_iterator;

static void *const vector_allocator_error_flag = reinterpret_cast<void*>(size_t(63));

template <typename T>
void handle_unconstructed_elements(T* array, size_t n_of_elements) {
	std::memset(static_cast<void*>(array), 0, n_of_elements *sizeof(T));
}

// Base class of concurrent vector implementation

class concurrent_vector_base_v3 {
protected:
	typedef size_t segment_index_t;
	typedef size_t size_type;

	enum {
		default_initial_segments = 1,
		pointers_per_short_table = 3,
		pointers_per_long_table = sizeof(segment_index_t) * 8
	};

	struct segment_not_used {};
	struct segment_allocated {};
	struct segment_allocation_failed {};

	class segment_t;
	class segment_value_t {
		void* array;
	private:
		friend class segment_t;
		explicit segment_value_t(void* an_array):array(an_array) {}
	public:
		friend bool operator == (segment_value_t const& lhs, segment_not_used) {return lhs.array == 0;}
		friend bool operator == (segment_value_t const& lhs, segment_allocated) {
			return lhs.array > internal::vector_allocator_error_flag;
		}
		friend bool operator == (segment_value_t const& lhs, segment_allocation_failed) {
			return lhs.array == internal::vector_allocator_error_flag;
		}
	    template <typename argument_type>
	    friend bool operator != (segment_value_t const& lhs, argument_type arg) {return !(lhs == arg);}
	    template <typename T>
	    T *pointer() const {return static_cast<T*>(const_cast<void*>(array));}
	};
	friend void enfore_segment_allocated(segment_value_t const& s, internal::exception_id exception = eid_bad_last_alloc) {
		if (s != segment_allocated())
		{
			internal::throw_exception(exception);
		}
	}

	// Segment pointer
	class segment_t {
		atomic<void*> array;
	public:
		segment_t() {store<relaxed>(segment_not_used());}

		/*
		 * Copy ctor and assignment operator are defined to ease using of stl algorithm
		 * These algorithm usually not a synchronization point, so semantic is intentionally relaxed here.
		 */
		segment_t(segment_t const& rhs) {array.store<relaxed>(rhs.array.load<relaxed>());}

		void swap(segment_t& rhs) {
			tbb::internal::swap<relaxed>(array,rhs.array);
		}

		segment_t& operator=(segment_t const& rhs) {
			array.store<relaxed>(rhs.array.load<relaxed>());
			return *this;
		}

		template <memory_semantics M>
		segment_value_t load() const {return segment_value_t(array.load<M>());}

		template <memory_semantics M>
		void store(segment_not_used) {
			array.store<M>(0);
		}

		template <memory_semantics M>
		void store(segment_allocation_failed) {
			__TBB_ASSERT(load<relaxed>() != segment_allocated(), "transition from \"allocated\" to \"allocation failed\" state looks non-logical");
			array.store<M>(internal::vector_allocator_error_flag);
		}

		template <memory_semantics M>
		void store(void* allocated_segment_pointer) __TBB_NOEXCEPT(true) {
			__TBB_ASSERT(segment_value_t(allocated_segment_pointer) == segment_allocated(),
					"other overloads of store should be used for marking segment as not_used or allocation_failed");
			array.store<M>(allocated_segment_pointer);
		}
#if TBB_USE_ASSERT
            ~segment_t() {
                __TBB_ASSERT(load<relaxed>() != segment_allocated(), "should have been freed by clear" );
            }
#endif
	};
	friend void swap(segment_t &, segment_t &) __TBB_NOEXCEPT(true);

	// allocator function pointer
	void* (*vector_allocator_ptr)(concurrent_vector_base_v3 &, size_t);

	// count of segments in the first block
	atomic<size_type> my_first_block;

	// Requested size of vector
	atomic<size_type> my_early_size;

	// Pointer to the segment table
	atomic<segment_t*> my_segment;

	// embedded storage of segment pointer
	segment_t my_storage[pointers_per_short_table];

	concurrent_vector_base_v3() {
		my_early_size.store<relaxed>(0);
		my_first_block.store<relaxed>(0);
		my_segment.store<relaxed>(my_storage);
	}

	__TBB_EXPORTED_METHOD ~concurrent_vector_base_v3();

	/*
	 * These helpers methods use the fact that segments are allocated
	 * so that every segments size is a increasing power of 2,
	 * with one exception 0 segment has size of 2 as well segment 1
	 */
	static segment_index_t segment_index_of (size_type index) {
		return segment_index_t(__TBB_Log2(index|1));
	}
	static segment_index_t segment_base(segment_index_t k) {
		return (segment_index_t(1)<<k & ~segment_index_t(1));
	}
	static inline segment_index_t segment_base_index_of(segment_index_t& index) {
		segment_index_t k = segment_index_of(index);
		index -= segment_base(k);
		return k;
	}
	static size_type segment_size(segment_index_t k) {
		return segment_index_t(1)<<k;
	}
	static bool is_first_element_in_segment(size_type element_index) {
		__TBB_ASSERT(element_index, "there should be no need to call "
		"is_first_element_in_segment for 0th element" );
		return is_power_of_two_at_least(element_index,2);
	}

	// An operation on an n-element array starting at begin
	typedef void(__TBB_EXPORTED_FUNC *internal_array_op1)(void* begin, size_type n);

	// An operation on n-element destination array and n-element source array
	typedef void(__TBB_EXPORTED_FUNC *internal_array_op2)(void* dst, const void* src, size_type n);

	// Internal structure for compact()
	struct internal_segments_table {
		segment_index_t first_block;
		segment_t table[pointers_per_long_table];
	};

	void __TBB_EXPORTED_METHOD internal_reserve(size_type n, size_type element_size, size_type max_size);
	size_type __TBB_EXPORTED_METHOD internal_capacity() const;
	void internal_grow(size_type start, size_type finish, size_type element_size, internal_array_op2 init, const void* src);
	size_type __TBB_EXPORTED_METHOD internal_grow_by(size_type delta, size_type element_size,
			                                         internal_array_op2 init, const void* src);
	void* __TBB_EXPORTED_METHOD internal_push_back(size_type element_size, size_type& index);
	segment_index_t __TBB_EXPORTED_METHOD internal_clear(internal_array_op1 destroy);
	void* __TBB_EXPORTED_METHOD intenal_compact(size_type element_size, void* table, internal_array_op1 destroy,
			                                    internal_array_op2 copy);
	void __TBB_EXPORTED_METHOD internal_copy(const concurrent_vector_base_v3& src, size_type element_size,
			                                 internal_array_op2 copy);
	void __TBB_EXPORTED_METHOD internal_assign(const concurrent_vector_base_v3& src, size_type element_size,
			                                   internal_array_op1 destroy, internal_array_op2 assign, internal_array_op2 copy);
	void __TBB_EXPORTED_METHOD internal_throw_exception(size_type) const;
	void __TBB_EXPORTED_METHOD internal_swap(concurrent_vector_base_v3& v);
	void __TBB_EXPORTED_METHOD internal_resize(size_type n, size_type element_size, size_type max_size, const void* src,
			                                   internal_array_op1 destroy, internal_array_op2 init);
	size_type __TBB_EXPORTED_METHOD internal_grow_to_at_least_with_result(size_type new_size, size_type element_size,
			                                                              internal_array_op2 init, const void* src);
	void __TBB_EXPORTED_METHOD internal_grow_to_at_least(size_type new_size, size_type element_size,
			                                             internal_array_op2 init, const void *src);

private:
	class helper;
	friend class helper;
	template<typename Container, typename name>
	friend class vector_iterator;
};

inline void swap(concurrent_vector_base_v3::segment_t& lhs, concurrent_vector_base_v3::segment_t& rhs) __TBB_NOEXCEPT(true) {
	lhs.swap(rhs);
}

typedef concurrent_vector_base_v3 concurrent_vector_base;

// Meet requirements of a forward iterator for STL and a value for a blocked_range
template <typename Container, typename Value>
class vector_iterator {
	// Concurrent vector over which we are iterating
	Container* my_vector;

	// Index into the vector
	size_t my_index;

	mutable Value* my_item;

	template <typename C, typename T>
	friend vector_iterator<C,T> operator+(ptrdiff_t offset, const vector_iterator<C,T>& v);

	template <typename C, typename T, typename U>
	friend bool operator == (const vector_iterator<C,T>& i, const vector_iterator<C,U>& j);

	template <typename C, typename T, typename U>
    friend bool operator < (const vector_iterator<C,T>& i, const vector_iterator<C,U>& j);

	template <typename C, typename T, typename U>
	friend ptrdiff_t operator - (const vector_iterator<C,T>&i, const vector_iterator<C,U>& j);

	template <typename C, typename U>
	friend class internal::vector_iterator;

#if !__TBB_TEMPLATE_FRIENDS_BROKEN
	template <typename T, class A>
	friend class tbb::concurrent_vector;
#else
public:
#endif

	vector_iterator(const Container& vector, size_t index, void* ptr = 0) :
		my_vector(const_cast<Container*>(&vector)),
		my_index(index),
		my_item(static_cast<Value*>(ptr))
    {}

public:
	vector_iterator() : my_vector(NULL), my_index(~size_t(0)), my_item(NULL) {}

	vector_iterator (const vector_iterator<Container,typename Container::value_type>& other) :
		my_vector(other.my_vector),
		my_index(other.my_index),
		my_item(other.my_item)
	{}

	vector_iterator operator+ (ptrdiff_t offset) const {
		return vector_iterator(*my_vector, my_index+offset);
	}

	vector_iterator &operator+=(ptrdiff_t offset) {
		my_index += offset;
		my_item = NULL;
		return *this;
	}

	vector_iterator operator- (ptrdiff_t offset) const {
		return vector_iterator(*my_vector, my_index-offset);
	}

	vector_iterator &operator-=(ptrdiff_t offset) {
		my_index -= offset;
		my_item = NULL;
		return *this;
	}

	Value& operator*() const {
		Value* item = my_item;
		if (!item) {
			item = my_item = &my_vector->internal_subscript(my_index);
		}
		__TBB_ASSERT(item == &my_vector->internal_subscript(my_index), "corrupt cache");
		return *item;
	}

	Value& operator[] (ptrdiff_t k) const {
		return my_vector->internal_subscript(my_index+k);
	}

	Value* operator-> () const {return &operator*();}

	// Pre decrement
	vector_iterator &operator++() {
		size_t element_index = ++my_index;
		if (my_item) {
			if (concurrent_vector_base::is_first_element_in_segment(element_index))
			{
				my_item = NULL;
			}
			else {
				++my_item;
			}
		}
		return *this;
	}

	// Pre decrement
	vector_iterator &operator--() {
		size_t element_index = my_index--;
		if (my_item)
		{
			if (concurrent_vector_base::is_first_element_in_segment(element_index))
			{
				my_item = NULL;
			}
			else
			{
				--my_item;
			}
		}
		return *this;
	}

	vector_iterator operator++(int) {
		vector_iterator result = *this;
		operator++();
		return result;
	}

	vector_iterator operator--(int) {
		vector_iterator result = *this;
		operator--();
		return result;
	}

	typedef ptrdiff_t difference_type;
	typedef Value value_type;
	typedef Value* pointer;
	typedef Value& reference;
	typedef std::random_access_iterator_tag iterator_category;
};

template <typename Container, typename T>
vector_iterator<Container,T> operator+(ptrdiff_t offset, const vector_iterator<Container,T>&v) {
	return vector_iterator<Container,T>(*v.my_vector, v.my_index+offset);
}

template <typename Container, typename T, typename U>
bool operator==(const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j) {
	return i.my_index == j.my_index && i.my_vector == j.my_vector;
}

template <typename Container, typename T, typename U>
bool operator!=(const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j) {
	return !(i=j);
}

template <typename Container, typename T, typename U>
bool operator < (const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j) {
	return i.my_index < j.my_index;
}

template <typename Container, typename T, typename U>
bool operator > (const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j) {
	return j < i;
}

template <typename Container, typename T, typename U>
bool operator <= (const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j) {
	return !(j<i);
}

template <typename Container, typename T, typename U>
bool operator >= (const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j) {
	return !(i<j);
}

template <typename Container, typename T, typename U>
ptrdiff_t operator - (const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j) {
	return ptrdiff_t(i.my_index) - ptrdiff_t(j.my_index);
}

template <typename T, class A>
class allocator_base {
public:
	typedef typename A::template
			rebind<T>::other allocator_type;
	allocator_type my_allocator;
	allocator_base(const allocator_type& a = allocator_type()) : my_allocator(a) {}
};
}

template <typename T, class A>
class concurrent_vector: protected internal::allocator_base<T,A>,
                         private internal::concurrent_vector_base {
private:
	template <typename I>
	class generic_range_type : public blocked_range<I> {
	public:
		typedef T value_type;
		typedef T& reference;
		typedef const T& const_reference;
		typedef I iterator;
		typedef ptrdiff_t difference_type;
		generic_range_type(I begin_, I end_, size_t grainsize_ = 1) : blocked_range<I>(begin_, end_, grainsize_) {}
		template <typename U>
		generic_range_type(const generic_range_type<U>& r) : blocked_range<I>(r.begin(), r.end(), r.grainsize()) {}
		generic_range_type(generic_range_type& r, split) : blocked_range<I>(r,split()) {}
	};
	
	template <typename C, typename U>
	friend class internal::vector_iterator;
	
	
	
};

}

#endif /* INCLUDE_TBB_CONCURRENT_VECTOR_H_ */
