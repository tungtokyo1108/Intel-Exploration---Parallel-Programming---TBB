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

#endif /* INCLUDE_TBB_TBB_MACHINE_H_ */
