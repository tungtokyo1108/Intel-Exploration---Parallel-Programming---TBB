/*
 * tbb_machine.h
 *
 *  Created on: Oct 4, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_TBB_MACHINE_H_
#define INCLUDE_TBB_TBB_MACHINE_H_

#include "tbb_stddef.h"
namespace tbb {
namespace internal {

template<typename T, std::size_t S>
struct machine_load_store;

template<typename T, std::size_t S>
struct machine_load_store_relaxed;

template<typename T, std::size_t S>
struct machine_load_store_seq_cst;

template<size_t S> struct atomic_selector;
template<> struct atomic_selector<1> {
  typedef int8_t word;
  inline static word fetch_store(volatile void* location, word value);
};

template<> struct atomic_selector<2> {
  typedef int16_t word;
  inline static word fetch_store(volatile void* location, word value);
};

template<> struct atomic_selector<4> {
  #if _MSC_VER && !_WIN64
  typedef intptr_t word;
  #else
  typedef int32_t word;
  #endif
  inline static word fetch_store (volatile void* location, word value);
};

template<> struct atomic_selector<8> {
  typedef int64_t word;
  inline static word fetch_store (volatile void* location, word value);
};
}}

#define __TBB_MACHINE_DEFINE_STORE8_GENERIC_FENCED(M)                           \
inline void __TBB_machine_generic_store8##M(volatile void *ptr, int64_t value) {\
  for(;;) {                                                                     \
    int64_t result = *(volatile int64_t *)ptr;                                  \
    if (__TBB_machine_cmpswp8##M(ptr,value,result)==result) break;              \
  }                                                                             \
}

#define __TBB_MACHINE_DEFINE_LOAD8_GENERIC_FENCED(M)                            \
inline int64_t __TBB_machine_generic_load8##M(const volatile void *ptr) {       \
  const int64_t anyvalue = 2305843009213693951LL;                               \
  return __TBB_machine_cmpswp8##M(const_cast<volatile void*>(ptr),anyvalue,anyvalue);\
}

#define __TBB_ENDIAN_UNSUPPORTED -1
#define __TBB_ENDIAN_LITTLE       0
#define __TBB_ENDIAN_BIG          1
#define __TBB_ENDIAN_DETECT       2

#if _WIN32||_WIN64

#ifdef _MANAGED
#pragma managed(push, off)
#endif

    #if __MINGW64__ || __MINGW32__
        extern "C" __declspec(dllimport) int __stdcall SwitchToThread( void );
        #define __TBB_Yield()  SwitchToThread()
        #if (TBB_USE_GCC_BUILTINS && __TBB_GCC_BUILTIN_ATOMICS_PRESENT)
            #include "machine/gcc_generic.h"
        #elif __MINGW64__
            #include "machine/linux_intel64.h"
        #elif __MINGW32__
            #include "machine/linux_ia32.h"
        #endif
    #elif (TBB_USE_ICC_BUILTINS && __TBB_ICC_BUILTIN_ATOMICS_PRESENT)
        #include "machine/icc_generic.h"
    #elif defined(_M_IX86) && !defined(__TBB_WIN32_USE_CL_BUILTINS)
        #include "machine/windows_ia32.h"
    #elif defined(_M_X64)
        #include "machine/windows_intel64.h"
    #elif defined(_M_ARM) || defined(__TBB_WIN32_USE_CL_BUILTINS)
        #include "machine/msvc_armv7.h"
    #endif

#ifdef _MANAGED
#pragma managed(pop)
#endif

#elif __TBB_DEFINE_MIC

    #include "machine/mic_common.h"
    #if (TBB_USE_ICC_BUILTINS && __TBB_ICC_BUILTIN_ATOMICS_PRESENT)
        #include "machine/icc_generic.h"
    #else
        #include "machine/linux_intel64.h"
    #endif

#elif __linux__ || __FreeBSD__ || __NetBSD__ || __OpenBSD__

    #if (TBB_USE_GCC_BUILTINS && __TBB_GCC_BUILTIN_ATOMICS_PRESENT)
        #include "machine/gcc_generic.h"
    #elif (TBB_USE_ICC_BUILTINS && __TBB_ICC_BUILTIN_ATOMICS_PRESENT)
        #include "machine/icc_generic.h"
    #elif __i386__
        #include "machine/linux_ia32.h"
    #elif __x86_64__
        #include "machine/linux_intel64.h"
    #elif __ia64__
        #include "machine/linux_ia64.h"
    #elif __powerpc__
        #include "machine/mac_ppc.h"
    #elif __ARM_ARCH_7A__ || __aarch64__
        #include "machine/gcc_arm.h"
    #elif __TBB_GCC_BUILTIN_ATOMICS_PRESENT
        #include "machine/gcc_generic.h"
    #endif
    #include "machine/linux_common.h"

#elif __APPLE__
    //TODO:  TBB_USE_GCC_BUILTINS is not used for Mac, Sun, Aix
    #if (TBB_USE_ICC_BUILTINS && __TBB_ICC_BUILTIN_ATOMICS_PRESENT)
        #include "machine/icc_generic.h"
    #elif __TBB_x86_32
        #include "machine/linux_ia32.h"
    #elif __TBB_x86_64
        #include "machine/linux_intel64.h"
    #elif __POWERPC__
        #include "machine/mac_ppc.h"
    #endif
    #include "machine/macos_common.h"

#elif _AIX

    #include "machine/ibm_aix51.h"

#elif __sun || __SUNPRO_CC

    #define __asm__ asm
    #define __volatile__ volatile

    #if __i386  || __i386__
        #include "machine/linux_ia32.h"
    #elif __x86_64__
        #include "machine/linux_intel64.h"
    #elif __sparc
        #include "machine/sunos_sparc.h"
    #endif
    #include <sched.h>

    #define __TBB_Yield() sched_yield()

#endif /* OS selection */

#include <sched.h>
#define __TBB_Yield() sched_yield()

#ifndef __TBB_64BIT_ATOMICS
    #define __TBB_64BIT_ATOMICS 1
#endif

#if __TBB_USE_FENCED_ATOMICS
#define __TBB_machine_cmpswp1 __TBB_machine_cmpswp1full_fence
#define __TBB_machine_cmpswp2 __TBB_machine_cmpswp2full_fence
#define __TBB_machine_cmpswp4 __TBB_machine_cmpswp4full_fence
#define __TBB_machine_cmpswp8 __TBB_machine_cmpswp8full_fence

#if __TBB_WORDSIZE==8
#define __TBB_machine_fetchadd8 __TBB_machine_fetchadd8full_fence
#define __TBB_machine_fetchstore8 __TBB_machine_fetchstore8full_fence
#define __TBB_FetchAndAddWrelease(P,V) __TBB_machine_fetchadd8release(P,V)
#define __TBB_FetchAndIncrementWacquire(P) __TBB_machine_fetchadd8acquire(P,1)
#define __TBB_FetchAndDecrementWrelease(P) __TBB_machine_fetchadd8release(P,(-1))
#else
#define __TBB_machine_fetchadd4 __TBB_machine_fetchadd4full_fence
#define __TBB_machine_fetchstore4 __TBB_machine_fetchstore4full_fence
#define __TBB_FetchAndAddWrelease(P,V) __TBB_machine_fetchadd4release(P,V)
#define __TBB_FetchAndIncrementWacquire(P) __TBB_machine_fetchadd4acquire(P,1)
#define __TBB_FetchAndDecrementWrelease(P) __TBB_machine_fetchadd4release(P,(-1))
#endif

#else
#define __TBB_FetchAndAddWrelease(P,V) __TBB_FetchAndAddW(P,V)
#define __TBB_FetchAndIncrementWacquire(P) __TBB_FetchAndAddW(P,1)
#define __TBB_FetchAndDecrementWrelease(P) __TBB_FetchAndAddW(P,(-1))
#endif

#if __TBB_WORDSIZE==4
#define __TBB_CompareAndSwapW(P,V,C) __TBB_machine_cmpswp4(P,V,C)
#define __TBB_FetchAndAddW(P,V) __TBB_machine_fetchadd4(P,V)
#define __TBB_FetchAndStoreW(P,V) __TBB_machine_fetchstore4(P,V)
#elif __TBB_WORDSIZE==8
#if __TBB_USE_GENERIC_DWORD_LOAD_STORE || __TBB_USE_GENERIC_DWORD_FETCH_ADD || __TBB_USE_GENERIC_DWORD_FETCH_STORE
        #error These macros should only be used on 32-bit platforms.
    #endif

    #define __TBB_CompareAndSwapW(P,V,C)    __TBB_machine_cmpswp8(P,V,C)
    #define __TBB_FetchAndAddW(P,V)         __TBB_machine_fetchadd8(P,V)
    #define __TBB_FetchAndStoreW(P,V)       __TBB_machine_fetchstore8(P,V)
#else /* __TBB_WORDSIZE != 8 */
    #error Unsupported machine word size.
#endif /* __TBB_WORDSIZE */

#ifndef __TBB_Pause
    inline void __TBB_Pause(int32_t) {
        __TBB_Yield();
    }
#endif

namespace tbb {
  inline void atomic_fence() {__TBB_full_memory_fence();}
  namespace internal {
    class atomic_backoff : no_copy {
      // Time deplay, in units of "pause" instructions
      static const int32_t LOOPS_BEFORE_YIELD = 16;
      int32_t count;
    public:
      atomic_backoff() : count(1) {}
      atomic_backoff(bool) : count(1) {pause();}
      void pause() {
        if (count <= LOOPS_BEFORE_YIELD) {
          __TBB_Pause(count);
          count *= 2;
        } else {
          __TBB_Yield();
        }
      }
      bool bounded_pause() {
        __TBB_Pause(count);
        if (count < LOOPS_BEFORE_YIELD) {
          count *= 2;
          return true;
        } else {
          return false;
        }
      }
      void reset() {
        count = 1;
      }
    };

template<typename T, typename U>
void spin_wait_while_eq(const volatile T& location, U value) {
  atomic_backoff backoff;
  while(location == value) backoff.pause();
}

template<typename T, typename U>
void spin_wait_until_eq(const volatile T& location, U value) {
  atomic_backoff backoff;
  while(location != value) backoff.pause();
}

template<typename predicate_type>
void spin_wait_while(predicate_type condition) {
  atomic_backoff backoff;
  while(condition()) backoff.pause();
}

#ifndef __TBB_ENDIANNESS
#define __TBB_ENDIANNESS __TBB_ENDIAN_DETECT
#endif

#if __TBB_USE_GENERIC_PART_WORD_CAS && __TBB_ENDIANNESS==__TBB_ENDIAN_UNSUPPORTED
#error Generic implementation of part-word CAS may not be used with __TBB_ENDIAN_UNSUPPORTED
#endif

/*
The follwing restriction and limitations apply for this operation:
- T must be an integer type of at most 4 bytes for the casts and calculations to work
- T must also be less than 4 bytes to avoid compiler warnings when computing mask]
- The architecture must consistently use either little-endian or big-endian
*/

#if __TBB_ENDIANNESS!= __TBB_ENDIAN_UNSUPPORTED
template<typename T>
inline T __TBB_MaskCompareAndSwap(volatile T * const ptr, const T value, const T comparand) {
  struct endianness(static bool is_big_endian()) {
    #if __TBB_ENDIANNESS == __TBB_ENDIAN_DETECT
    const uint32_t probe = 0x03020100;
    return (((const char*)(&probe))[0]==0x03);
    #elif __TBB_ENDIANNESS == __TBB_ENDIAN_BIG || __TBB_ENDIANNESS == __TBB_ENDIAN_LITTLE
    return __TBB_ENDIANNESS == __TBB_ENDIAN_BIG;
    #else
    #error
    #endif
  }};
  const uint32_t byte_offset = (uint32_t) ((uintptr_t)ptr & 0x03);
  volatile uint32_t * const aligned_ptr = (uint32_t*)((uintptr_t)ptr - byte_offset);
  // Location of T within uint32_t for C++ shift operation
  const uint32_t bits_to_shift = 8*(endianness::is_big_endian() ? (4 - sizeof(T) - (byte_offset)) : byte_offset);
  const uint32_t mask = (((uint32_t)1<<(sizeof(T)*8))-1)<<bits_to_shift;

  // for signed T, any sign extension btis in cast value/comparand are immediately clipped by mask
  const uint32_t shifted_comparand = ((uint32_t)comparand << bits_to_shift)&mask;
  const uint32_t shifted_value = ((uint32_t)value << bits_to_shift)&mask;

  for (atomic_backoff b;;b.pause()) {
    const uint32_t surroundings = *aligned_ptr & ~mask;
    const uint32_t big_comparand = surroundings | shifted_comparand;
    const uint32_t big_value = surroundings | shifted_value;
    const uint32_t big_result = (uint32_t)__TBB_machine_cmpswp4(aligned_ptr,big_value,big_comparand);
    if (big_result == big_comparand || ((big_result ^ big_comparand) & mask) != 0)
    {
      return T((big_result & mask) >> bits_to_shift);
    }
    else continue;
  }
#endif

template<size_t S, typename T>
inline T __TBB_CompareAndSwapGeneric(volatile void *ptr, T value, T comparand);

template<>
inline int8_t __TBB_CompareAndSwapGeneric<1, int8_t>(volatile void *ptr, int8_t value, int8_t comparand) {
  #if __TBB_USE_GENERIC_PART_WORD_CAS
  return __TBB_MaskCompareAndSwap<int8_t>((volatile int8_t *)ptr, value, comparand);
  #else
  return __TBB_machine_cmpswp1(ptr, value, comparand);
  #endif
}

template<>
inline int16_t __TBB_CompareAndSwapGeneric<2,int16_t>(volatile void *ptr, int16_t value, int16_t comparand) {
  #if __TBB_USE_GENERIC_PART_WORD_CAS
  return __TBB_MaskCompareAndSwap<int16_t>((volatile int16_t *)ptr, value, comparand);
  #else
  return __TBB_machine_cmpswp2(ptr, value, comparand);
  #endif
}

template<>
inline int32_t __TBB_CompareAndSwapGeneric<4,int32_t>(volatile void *ptr, int32_t value, int32_t comparand) {
  return (int32_t)__TBB_machine_cmpswp4(ptr,value,comparand);
}

#if __TBB_64BIT_ATOMICS
template<>
inline int64_t __TBB_CompareAndSwapGeneric<8, int64_t>(volatile void *ptr, int64_t value, int64_t comparand) {
  return __TBB_machine_cmpswp8(ptr, value, comparand);
}
#endif

template<size_t S, typename T>
inline T __TBB_FetchAndAddGeneric(volatile void *ptr, T addend) {
  T result;
  for (atomic_backoff b;;b.pause())
  {
    result = *reinterpret_cast<volatile T *>(ptr);
    if (__TBB_CompareAndSwapGeneric<S,T> (ptr, result + addend, result) == result)
    break;
  }
  return result;
}

template<size_t S, typename T>
inline T __TBB_FetchAndStoreGeneric(volatile void *ptr, T value) {
  T result;
  for (atomic_backoff b;;b.pause())
  {
    result = *reinterpret_cast<volatile T*>(ptr);
    if (__TBB_CompareAndSwapGeneric<S,T>(ptr,value,result)==result)
    break;
  }
  return result;
}

#if __TBB_USE_GENERIC_PART_WORD_CAS
#define __TBB_machine_cmpswp1 tbb::internal::__TBB_CompareAndSwapGeneric<1,int8_t>
#define __TBB_machine_cmpswp2 tbb::internal::__TBB_CompareAndSwapGeneric<2,int16_t>
#endif

#if __TBB_USE_GENERIC_FETCH_ADD || __TBB_USE_GENERIC_PART_WORD_FETCH_ADD
#define __TBB_machine_fetchadd1 tbb::internal::__TBB_FetchAndAddGeneric<1,int8_t>
#define __TBB_machine_fetchadd2 tbb::internal::__TBB_FetchAndAddGeneric<2,int16_t>
#endif

#if __TBB_USE_GENERIC_FETCH_ADD
#define __TBB_machine_fetchadd4 tbb::internal::__TBB_FetchAndAddGeneric<4,int32_t>
#endif

#if __TBB_USE_GENERIC_FETCH_STORE || __TBB_USE_GENERIC_DWORD_FETCH_STORE
#define __TBB_machine_fetchstore1 tbb::internal::__TBB_FetchAndStoreGeneric<1,int8_t>
#define __TBB_machine_fetchstore2 tbb::internal::__TBB_FetchAndStoreGeneric<2,int16_t>
#endif

#if __TBB_USE_GENERIC_FETCH_STORE
#define __TBB_machine_fetchstore4 tbb::internal::__TBB_FetchAndStoreGeneric<4,int32_t>
#endif

#if __TBB_USE_GENERIC_FETCH_STORE || __TBB_USE_GENERIC_DWORD_FETCH_STORE
#define __TBB_machine_fetchstore8 tbb::internal::__TBB_FetchAndStoreGeneric<8,int64_t>
#endif

#if __TBB_USE_FETCHSTORE_AS_FULL_FENCED_STORE
#define __TBB_MACHINE_DEFINE_ATOMIC_SELECTOR_FETCH_STORE(S) \
atomic_selector<S>::word atomic_selector<S>::fetch_store (volatile void* location, word value) {\
  return __TBB_machine_fetchstore##S(location,value);
}

__TBB_MACHINE_DEFINE_ATOMIC_SELECTOR_FETCH_STORE(1)
__TBB_MACHINE_DEFINE_ATOMIC_SELECTOR_FETCH_STORE(2)
__TBB_MACHINE_DEFINE_ATOMIC_SELECTOR_FETCH_STORE(4)
__TBB_MACHINE_DEFINE_ATOMIC_SELECTOR_FETCH_STORE(8)

#undef
#endif 

}
}

#endif /* INCLUDE_TBB_TBB_MACHINE_H_ */
