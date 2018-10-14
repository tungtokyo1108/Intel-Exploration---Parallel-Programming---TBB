/*
 * atomic.h
 *
 *  Created on: Oct 7, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_ATOMIC_H_
#define INCLUDE_TBB_ATOMIC_H_

#include <cstddef>

#if _MSC_VER
#define __TBB_LONG_LONG __int64
#else
#define __TBB_LONG_LONG long long
#endif

#include "tbb_machine.h"
#if _MSC_VER && !__INTEL_COMPILER
#pragma warning (push)
#pragma warning (disable: 4244 4267 4512)
#endif

namespace tbb
{
enum memory_semantics {
  full_fence,
  acquire,
  release,
  relaxed
};

namespace internal {

#if __TBB_ALIGNAS_PRESENT
#define __TBB_DECL_ATOMIC_FIELD(t,f,a) alignas(a) t f;
#elif __TBB_ATTRIBUTE_ALIGNED_PRESENT
#define __TBB_DECL_ATOMIC_FIELD(t,f,a) t f __attribute__ ((aligned(a)));
#elif __TBB_DECLSPEC_ALIGN_PRESENT
#define __TBB_DECL_ATOMIC_FIELD(t,f,a) __declspec(align(a)) t f;
#else
#error Do not know syntax forcing alignment
#endif

template<size_t S>
struct atomic_rep;

template<>
struct atomic_rep<1> {
  typedef int8_t word;
};

template<>
struct atomic_rep<2> {
  typedef int16_t word;
};

template<>
struct atomic_rep<4> {
#if _MSC_VER && !_WIN64
typedef intptr_t word;
#else
typedef int32_t word;
#endif
};

#if __TBB_64BIT_ATOMICS
template<>
struct atomic_rep<8> {
  typedef int64_t word;
};
#endif

template<typename value_type, size_t size>
struct aligned_storage;

// The specializations are needed to please MSVC syntax of __declspec(align()) which accept _literal_ constants only
#if __TBB_ATOMIC_CTORS
#define ATOMIC_STORAGE_PARTIAL_SPECIALIZATION(S)              \
template<typename value_type>                                 \
struct aligned_storage<value_type, S> {                       \
  __TBB_DECL_ATOMIC_FIELD(value_type, my_value,S)             \
  aligned_storage() = default;                                \
  constexpr aligned_storage(value_type value):my_value(value){}\
};                                                            \

#else
#define ATOMIC_STORAGE_PARTIAL_SPECIALIZATION(S)              \
template<typename value_type>                                 \
struct aligned_storage<value_type,S> {                        \
  __TBB_DECL_ATOMIC_FIELD(value_type,my_value,S)              \
};                                                            \

#endif

template<typename value_type>
struct aligned_storage<value_type,1> {
  value_type my_value;
#if __TBB_ATOMIC_CTORS
aligned_storage() = default;
constexpr aligned_storage(value_type value):my_value(value){}
#endif
};

ATOMIC_STORAGE_PARTIAL_SPECIALIZATION(2)
ATOMIC_STORAGE_PARTIAL_SPECIALIZATION(4)
#if __TBB_64BIT_ATOMICS
ATOMIC_STORAGE_PARTIAL_SPECIALIZATION(8)
#endif

template<size_t Size, memory_semantics M>
struct atomic_traits;

#define __TBB_DECL_FENCED_ATOMIC_PRIMITIVES(S,M)                                \
template<> struct atomic_traits<S,M> {                                          \
  typedef atomic_rep<S>::word word;                                             \
  inline static word compare_and_swap(volatile void* location, word new_value, word comparand) { \
    return __TBB_machine_cmpswp##S##M(location,new_value,comparand);            \
  }                                                                             \
  inline static word fetch_and_add(volatile void* location, word addend) {      \
    return __TBB_machine_fetchadd##S##M(location,addend);                       \
  }                                                                             \
  inline static word fetch_and_store(volatile void* location, word value) {     \
    return __TBB_machine_fetchstore##S##M(location,value);                      \
  }                                                                             \
};

#define __TBB_DECL_ATOMIC_PRIMITIVES(S)                                         \
template<memory_semantics M>                                                    \
struct atomic_traits<S,M> {                                                     \
  typedef atomic_rep<S>::word word;                                             \
  inline static word compare_and_swap(volatile void* location, word new_value, word comparand) { \
    return __TBB_machine_cmpswp##S(location,new_value,comparand);               \
  }                                                                             \
  inline static word fetch_and_add(volatile void* location, word addend) {      \
    return __TBB_machine_fetchadd##S(location,addend);                          \
  }                                                                             \
  inline static word fetch_and_store(volatile void* location, word value) {     \
    return __TBB_machine_fetchstore##S(location,value);                         \
  }                                                                             \
};

template<memory_semantics M>
struct atomic_load_store_traits;

#define __TBB_DECL_ATOMIC_LOAD_STORE_PRIMITIVES(M)                              \
template<> struct atomic_load_store_traits<M> {                                 \
  template<typename T>                                                          \
  inline static T load(const volatile T& location) {                            \
    return __TBB_load_##M(location);                                            \
  }                                                                             \
  template<typename T>                                                          \
  inline static void store(volatile T& location, T value)  {                    \
    __TBB_store_##M(location, value);                                           \
  }                                                                             \
};

#if __TBB_USE_FENCED_ATOMICS
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(1,full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(2,full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(4,full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(1,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(2,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(4,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(1,release)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(2,release)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(4,release)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(1,relaxed)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(2,relaxed)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(4,relaxed)
#if __TBB_64BIT_ATOMICS
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(8,full_fence)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(8,acquire)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(8,release)
__TBB_DECL_FENCED_ATOMIC_PRIMITIVES(8,relaxed)
#endif
#else
__TBB_DECL_ATOMIC_PRIMITIVES(1)
__TBB_DECL_ATOMIC_PRIMITIVES(2)
__TBB_DECL_ATOMIC_PRIMITIVES(4)
#if __TBB_64BIT_ATOMICS
__TBB_DECL_ATOMIC_PRIMITIVES(8)
#endif
#endif

__TBB_DECL_ATOMIC_LOAD_STORE_PRIMITIVES(full_fence);
__TBB_DECL_ATOMIC_LOAD_STORE_PRIMITIVES(acquire);
__TBB_DECL_ATOMIC_LOAD_STORE_PRIMITIVES(release);
__TBB_DECL_ATOMIC_LOAD_STORE_PRIMITIVES(relaxed);

#define __TBB_MINUS_ONE(T) (T(T(0)-T(1)))

/*
 * Base class that provides basic functionality for atomic<T> without fetch_and_add
 * Works for any type T that has the same size as an integral type,
 * has a trivial constructor/destructor, can be copied/compared by memcpy/memcmp
 */
template<typename T>
struct atomic_impl {
protected:
	aligned_storage<T,sizeof(T)>my_storage;
private:
	template<typename value_type>
	union converter {
		typedef typename atomic_rep<sizeof(value_type)>::word bits_type;
		converter() {}
		converter(value_type a_value) : value(a_value) {}
		value_type value;
		bits_type bits;
	};

	template<typename value_t>
	static typename converter<value_t>::bits_type to_bits(value_t value) {
		return converter<value_t>(value).bits;
	}

	template<typename value_t>
	static value_t to_value(typename converter<value_t>::bits_type bits) {
		converter<value_t> u;
		u.bits = bits;
		return u.value;
	}

	template<typename value_t>
	union ptr_converter;

	template<typename value_t>
	union ptr_converter<value_t *> {
		ptr_converter(){}
		ptr_converter(value_t* a_value) : value(a_value) {}
		value_t* value;
		uintptr_t bits;
	};
	template<typename value_t>
	static typename converter<value_t>::bits_type & to_bits_ref(value_t& value) {
#if !__SUNPRO_CC
		return *(typename converter<value_t>::bits_type*)ptr_converter<value_t*>(&value).bits;
#else
		return *(typename converter<value_t>::bits_type*)(&value);
#endif
	}

public:
	typedef T value_type;
#if __TBB_ATOMIC_CTORS
atomic_impl() = default;
constexpr atomic_impl(value_type value):my_storage(value){}
#endif

    template<memory_semantics M>
    value_type fetch_and_store(value_type value) {
    	return to_value<value_type>(
    			internal::atomic_traits<sizeof(value_type),M>::fetch_and_store(&my_storage.my_value, to_bits(value))
    	);
    }
    
    
};


}
}

#endif /* INCLUDE_TBB_ATOMIC_H_ */
