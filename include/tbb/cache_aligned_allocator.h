/*
 * cache_aligned_allocator.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_CACHE_ALIGNED_ALLOCATOR_H_
#define INCLUDE_TBB_CACHE_ALIGNED_ALLOCATOR_H_

#include <new>
#include "tbb_stddef.h"
#if __TBB_ALLOCATOR_CONSTRUCT_VARIADIC
#include <utility>
#endif

namespace tbb {
namespace internal {

/* Cache/section line size */
size_t __TBB_EXPORTED_FUNC NFS_GetLineSize();

/* Allocate memory on cache/sector line boundary */
void* __TBB_EXPORTED_FUNC NFS_Allocate(size_t n_element, size_t element_size, void* hint);

/* Free memory allocated */
void __TBB_EXPORTED_FUNC NFS_Free(void*);
}

#if _MSC_VER && !defined(__INTEL_COMPILER)
    // Workaround for erroneous "unreferenced parameter" warning in method destroy.
    #pragma warning (push)
    #pragma warning (disable: 4100)
#endif

template <typename T>
class cache_aligned_allocator {
public:
	typedef typename internal::allocator_type<T>::value_type value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	template <typename U> struct rebind {
		typedef cache_aligned_allocator<U> other;
	};

	cache_aligned_allocator() throw() {}
	cache_aligned_allocator(const cache_aligned_allocator& ) throw () {}
	template <typename U> cache_aligned_allocator(const cache_aligned_allocator<U>&) throw () {}

	pointer address(reference x) const {return &x;}
	const_pointer address(const_reference x) const {return &x;}

	/* Allocate space for n objects, starting on a cache/sector line */
	pointer allocate(size_type n, const void* hint=0)
	{
		return pointer(internal::NFS_Allocate(n,sizeof(value_type),const_cast<void*>(hint)));
	}

	/* Free block of memory that start on a cache line*/
	void deallocator(pointer p, size_type)
	{
		internal::NFS_Free(p);
	}

	size_type max_size() const throw() {
		return (~size_t(0)-internal::NFS_MaxLineSize)/sizeof(value_type);
	}

  #if __TBB_ALLOCATOR_CONSTRUCT_VARIADIC
  template<typename U, typename... Args>
  void construct(U* p, Args&&... args) {
    ::new((void *)p) U(std::forward<Args>(args)...);
  }
  #else
  #if __TBB_CPP11_RVALUE_REF_PRESENT
  void construct(pointer p, value_type&& value) {
    ::new((void*)(p)) value_type(std::move(value));
  }
  #endif
  void construct(pointer p, const value_type& value) {
    ::new((void*)(p)) value_type(value);
  }
  #endif

  void destroy(pointer p) {p->~value_type();}
};

#if _MSC_VER && !defined(__INTEL_COMPILER)
    #pragma warning (pop)
#endif

template <>
class cache_aligned_allocator<void> {
public:
	typedef void* pointer;
	typedef const void* const_pointer;
	typedef void value_type;
	template<typename U> struct rebind {
		typedef cache_aligned_allocator<U> other;
	};
};

template <typename T, typename U>
inline bool operator==(const cache_aligned_allocator<T>&, const cache_aligned_allocator<U>&) {return true;}

template <typename T, typename U>
inline bool operator!=(const cache_aligned_allocator<T>&, const cache_aligned_allocator<U>&) {return false;}

}

#endif /* INCLUDE_TBB_CACHE_ALIGNED_ALLOCATOR_H_ */
