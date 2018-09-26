/*
 * tbb_config.h
 *
 *  Created on: Sep 25, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_TBB_CONFIG_H_
#define INCLUDE_TBB_TBB_CONFIG_H_

/**
 * Macro definitions and C style comments only.
 * The macro defined here are intended to control such aspects of TBB build as
 * - present of compiler features
 * - compilation modes
 * - feature sets
 * - known compiler/platform issues
 */

#define __TBB_TODO 0

// Check which standard libraries are used
// __TBB_SYMBOL is defined only while processing exported list of symbols which C++ is not allowed

#if !defined(__TBB_SYMBOL) && !__TBB_CONFIG_PREPROC_ONLY
#include <cstddef>
#endif

#define __TBB_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

/*
 * Rely on the version of GCC or the user-specified macro below
 */

#ifdef TBB_USE_GLIBCXX_VERSION
#define __TBB_GLIBCXX_VERSION TBB_USE_GLIBCXX_VERSION
#elif __GLIBCPP__ || __GLIBCXX__
#define __TBB_GLIBCXX_VERSION __TBB_GCC_VERSION
#endif

#if __clang__
    #define __TBB_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

#if __ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__
    #define __TBB_IOS 1
#endif

// Preprocessor symbols to determine HW architecture

#if _WIN32||_WIN64
#   if defined(_M_X64)||defined(__x86_64__)  // the latter for MinGW support
#       define __TBB_x86_64 1
#   elif defined(_M_IA64)
#       define __TBB_ipf 1
#   elif defined(_M_IX86)||defined(__i386__) // the latter for MinGW support
#       define __TBB_x86_32 1
#   else
#       define __TBB_generic_arch 1
#   endif
#else
#   if !__linux__ && !__APPLE__
#       define __TBB_generic_os 1
#   endif
#   if __TBB_IOS
#       define __TBB_generic_arch 1
#   elif __x86_64__
#       define __TBB_x86_64 1
#   elif __ia64__
#       define __TBB_ipf 1
#   elif __i386__||__i386
#       define __TBB_x86_32 1
#   else
#       define __TBB_generic_arch 1
#   endif
#endif

#if __MIC__ || __MIC2__
#define __TBB_DEFINE_MIC 1
#endif

#define __TBB_TSX_AVAILABLE ((__TBB_x86_32 || __TBB_x86_64) && !__TBB_DEFINE_MIC)

#if __INTEL_COMPILER == 9999 && __INTEL_COMPILER_BUILD_DATE == 20110811
    #undef __INTEL_COMPILER
    #define __INTEL_COMPILER 1210
#endif

#if __clang__&& !__INTEL_COMPILER
#define __TBB_USE_OPTIONAL_RTTI __has_feature(cxx_rtti)
#elif defined(_CPPRTTI)
#define __TBB_USE_OPTIONAL_RTTI 1
#else
#define __TBB_USE_OPTIONAL_RTTI (__GXX_RTTI || __INTEL_RTTI__)
#endif

#if __TBB_GCC_VERSION >= 40400 && !defined(__INTEL_COMPILER)
    #define __TBB_GCC_WARNING_SUPPRESSION_PRESENT 1
#endif

/*
 * Select particular features of C++11 based on compiler version
 * __TBB_CPP11_PRESENT macro indicates that the compiler supports vast majority of C++11 feature.
 */

#define __TBB_CPP11_PRESENT (__cplusplus >= 201103L || _MSC_VER >= 1900)
#define __TBB_CPP17_FALLTHROUGH_PRESENT (__cplusplus >= 201703L)
#define __TBB_FALLTHROUGH_PRESENT (__TBB_GCC_VERSION >= 70000 && !__INTEL_COMPILER)

#if __INTEL_COMPILER &&  !__INTEL_CXX11_MODE__
    #define __INTEL_CXX11_MODE__ (__GXX_EXPERIMENTAL_CXX0X__ || (_MSC_VER && __STDC_HOSTED__))
#endif

// Intel(R) C++ Compiler offloading API to the Intel(R) Graphics Technology process macro

#if __INTEL_COMPILER >= 1600 && _WIN32
#define __TBB_GFX_PRESENT 1
#endif

#if __INTEL_COMPILER && (!_MSC_VER || __INTEL_CXX11_MODE__)

    #define __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT          (__INTEL_CXX11_MODE__ && __VARIADIC_TEMPLATES)
    #define __TBB_CPP11_RVALUE_REF_PRESENT                  ((_MSC_VER >= 1700 || __GXX_EXPERIMENTAL_CXX0X__ && (__TBB_GLIBCXX_VERSION >= 40500 || _LIBCPP_VERSION)) && __INTEL_COMPILER >= 1400)
    #define __TBB_IMPLICIT_MOVE_PRESENT                     (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1400 && (_MSC_VER >= 1900 || __TBB_GCC_VERSION >= 40600 || __clang__))
    #if  _MSC_VER >= 1600
        #define __TBB_EXCEPTION_PTR_PRESENT                 ( __INTEL_COMPILER > 1300                                                \
                                                            || (__INTEL_COMPILER == 1300 && __INTEL_COMPILER_BUILD_DATE >= 20120530) \
                                                            || (__INTEL_COMPILER == 1210 && __INTEL_COMPILER_BUILD_DATE >= 20120410) )
    #elif __TBB_GLIBCXX_VERSION >= 40404 && __TBB_GLIBCXX_VERSION < 40600
        #define __TBB_EXCEPTION_PTR_PRESENT                 (__GXX_EXPERIMENTAL_CXX0X__ && __INTEL_COMPILER >= 1200)
    #elif __TBB_GLIBCXX_VERSION >= 40600
        #define __TBB_EXCEPTION_PTR_PRESENT                 (__GXX_EXPERIMENTAL_CXX0X__ && __INTEL_COMPILER >= 1300)
    #elif _LIBCPP_VERSION
        #define __TBB_EXCEPTION_PTR_PRESENT                 __GXX_EXPERIMENTAL_CXX0X__
    #else
        #define __TBB_EXCEPTION_PTR_PRESENT                 0
    #endif
    #define __TBB_STATIC_ASSERT_PRESENT                     (__INTEL_CXX11_MODE__ || _MSC_VER >= 1600)
    #define __TBB_CPP11_TUPLE_PRESENT                       (_MSC_VER >= 1600 || __GXX_EXPERIMENTAL_CXX0X__ && (__TBB_GLIBCXX_VERSION >= 40300 || _LIBCPP_VERSION))
    #if (__clang__ && __INTEL_COMPILER > 1400)
        #if (__has_feature(__cxx_generalized_initializers__) && __has_include(<initializer_list>))
            #define __TBB_INITIALIZER_LISTS_PRESENT         1
        #endif
    #else
        #define __TBB_INITIALIZER_LISTS_PRESENT             (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1400 && (_MSC_VER >= 1800 || __TBB_GLIBCXX_VERSION >= 40400 || _LIBCPP_VERSION))
    #endif
    #define __TBB_CONSTEXPR_PRESENT                         (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1400)
    #define __TBB_DEFAULTED_AND_DELETED_FUNC_PRESENT        (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1200)
    #define __TBB_NOEXCEPT_PRESENT                          (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1300 && (__TBB_GLIBCXX_VERSION >= 40600 || _LIBCPP_VERSION || _MSC_VER))
    #define __TBB_CPP11_STD_BEGIN_END_PRESENT               (_MSC_VER >= 1700 || __GXX_EXPERIMENTAL_CXX0X__ && __INTEL_COMPILER >= 1310 && (__TBB_GLIBCXX_VERSION >= 40600 || _LIBCPP_VERSION))
    #define __TBB_CPP11_AUTO_PRESENT                        (_MSC_VER >= 1600 || __GXX_EXPERIMENTAL_CXX0X__ && __INTEL_COMPILER >= 1210)
    #define __TBB_CPP11_DECLTYPE_PRESENT                    (_MSC_VER >= 1600 || __GXX_EXPERIMENTAL_CXX0X__ && __INTEL_COMPILER >= 1210)
    #define __TBB_CPP11_LAMBDAS_PRESENT                     (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1200)
    #define __TBB_CPP11_DEFAULT_FUNC_TEMPLATE_ARGS_PRESENT  (_MSC_VER >= 1800 || __GXX_EXPERIMENTAL_CXX0X__ && __INTEL_COMPILER >= 1210)
    #define __TBB_OVERRIDE_PRESENT                          (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1400)
    #define __TBB_ALIGNAS_PRESENT                           (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1500)
    #define __TBB_CPP11_TEMPLATE_ALIASES_PRESENT            (__INTEL_CXX11_MODE__ && __INTEL_COMPILER >= 1210)
    #define __TBB_CPP14_INTEGER_SEQUENCE_PRESENT            (__cplusplus >= 201402L)
#elif __clang__
/** TODO: these options need to be rechecked **/
    #define __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT          __has_feature(__cxx_variadic_templates__)
    #define __TBB_CPP11_RVALUE_REF_PRESENT                  (__has_feature(__cxx_rvalue_references__) && (_LIBCPP_VERSION || __TBB_GLIBCXX_VERSION >= 40500))
    #define __TBB_IMPLICIT_MOVE_PRESENT                     __has_feature(cxx_implicit_moves)
/** TODO: extend exception_ptr related conditions to cover libstdc++ **/
    #define __TBB_EXCEPTION_PTR_PRESENT                     (__cplusplus >= 201103L && (_LIBCPP_VERSION || __TBB_GLIBCXX_VERSION >= 40600))
    #define __TBB_STATIC_ASSERT_PRESENT                     __has_feature(__cxx_static_assert__)
    #if (__GXX_EXPERIMENTAL_CXX0X__ && __has_include(<tuple>))
        #define __TBB_CPP11_TUPLE_PRESENT                   1
    #endif
    #if (__has_feature(__cxx_generalized_initializers__) && __has_include(<initializer_list>))
        #define __TBB_INITIALIZER_LISTS_PRESENT             1
    #endif
    #define __TBB_CONSTEXPR_PRESENT                         __has_feature(__cxx_constexpr__)
    #define __TBB_DEFAULTED_AND_DELETED_FUNC_PRESENT        (__has_feature(__cxx_defaulted_functions__) && __has_feature(__cxx_deleted_functions__))
    #define __TBB_NOEXCEPT_PRESENT                          (__cplusplus >= 201103L)
    #define __TBB_CPP11_STD_BEGIN_END_PRESENT               (__has_feature(__cxx_range_for__) && (_LIBCPP_VERSION || __TBB_GLIBCXX_VERSION >= 40600))
    #define __TBB_CPP11_AUTO_PRESENT                        __has_feature(__cxx_auto_type__)
    #define __TBB_CPP11_DECLTYPE_PRESENT                    __has_feature(__cxx_decltype__)
    #define __TBB_CPP11_LAMBDAS_PRESENT                     __has_feature(cxx_lambdas)
    #define __TBB_CPP11_DEFAULT_FUNC_TEMPLATE_ARGS_PRESENT  __has_feature(cxx_default_function_template_args)
    #define __TBB_OVERRIDE_PRESENT                          __has_feature(cxx_override_control)
    #define __TBB_ALIGNAS_PRESENT                           __has_feature(cxx_alignas)
    #define __TBB_CPP11_TEMPLATE_ALIASES_PRESENT            __has_feature(cxx_alias_templates)
    #define __TBB_CPP14_INTEGER_SEQUENCE_PRESENT            (__cplusplus >= 201402L)
#elif __GNUC__
    #define __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT          __GXX_EXPERIMENTAL_CXX0X__
    #define __TBB_CPP11_VARIADIC_FIXED_LENGTH_EXP_PRESENT   (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40700)
    #define __TBB_CPP11_RVALUE_REF_PRESENT                  (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40500)
    #define __TBB_IMPLICIT_MOVE_PRESENT                     (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40600)
    /** __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 here is a substitution for _GLIBCXX_ATOMIC_BUILTINS_4, which is a prerequisite
        for exception_ptr but cannot be used in this file because it is defined in a header, not by the compiler.
    **/
    #define __TBB_EXCEPTION_PTR_PRESENT                     (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40404 && __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    #define __TBB_STATIC_ASSERT_PRESENT                     (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40300)
    #define __TBB_CPP11_TUPLE_PRESENT                       (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40300)
    #define __TBB_INITIALIZER_LISTS_PRESENT                 (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40400)
    #define __TBB_CONSTEXPR_PRESENT                         (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40400)
    #define __TBB_DEFAULTED_AND_DELETED_FUNC_PRESENT        (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40400)
    #define __TBB_NOEXCEPT_PRESENT                          (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40600)
    #define __TBB_CPP11_STD_BEGIN_END_PRESENT               (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40600)
    #define __TBB_CPP11_AUTO_PRESENT                        (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40400)
    #define __TBB_CPP11_DECLTYPE_PRESENT                    (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40400)
    #define __TBB_CPP11_LAMBDAS_PRESENT                     (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40500)
    #define __TBB_CPP11_DEFAULT_FUNC_TEMPLATE_ARGS_PRESENT  (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40300)
    #define __TBB_OVERRIDE_PRESENT                          (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40700)
    #define __TBB_ALIGNAS_PRESENT                           (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40800)
    #define __TBB_CPP11_TEMPLATE_ALIASES_PRESENT            (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GCC_VERSION >= 40700)
    #define __TBB_CPP14_INTEGER_SEQUENCE_PRESENT            (__cplusplus >= 201402L     && __TBB_GCC_VERSION >= 50000)
#elif _MSC_VER

    #define __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT          (_MSC_VER >= 1800)
    #define __TBB_CPP11_RVALUE_REF_PRESENT                  (_MSC_VER >= 1700 && (!__INTEL_COMPILER || __INTEL_COMPILER >= 1400))
    #define __TBB_IMPLICIT_MOVE_PRESENT                     (_MSC_VER >= 1900)
    #define __TBB_EXCEPTION_PTR_PRESENT                     (_MSC_VER >= 1600)
    #define __TBB_STATIC_ASSERT_PRESENT                     (_MSC_VER >= 1600)
    #define __TBB_CPP11_TUPLE_PRESENT                       (_MSC_VER >= 1600)
    #define __TBB_INITIALIZER_LISTS_PRESENT                 (_MSC_VER >= 1800)
    #define __TBB_CONSTEXPR_PRESENT                         (_MSC_VER >= 1900)
    #define __TBB_DEFAULTED_AND_DELETED_FUNC_PRESENT        (_MSC_VER >= 1800)
    #define __TBB_NOEXCEPT_PRESENT                          (_MSC_VER >= 1900)
    #define __TBB_CPP11_STD_BEGIN_END_PRESENT               (_MSC_VER >= 1700)
    #define __TBB_CPP11_AUTO_PRESENT                        (_MSC_VER >= 1600)
    #define __TBB_CPP11_DECLTYPE_PRESENT                    (_MSC_VER >= 1600)
    #define __TBB_CPP11_LAMBDAS_PRESENT                     (_MSC_VER >= 1600)
    #define __TBB_CPP11_DEFAULT_FUNC_TEMPLATE_ARGS_PRESENT  (_MSC_VER >= 1800)
    #define __TBB_OVERRIDE_PRESENT                          (_MSC_VER >= 1700)
    #define __TBB_ALIGNAS_PRESENT                           (_MSC_VER >= 1900)
    #define __TBB_CPP11_TEMPLATE_ALIASES_PRESENT            (_MSC_VER >= 1800)
    #define __TBB_CPP14_INTEGER_SEQUENCE_PRESENT            (_MSC_VER >= 1900)
#else
    #define __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT          0
    #define __TBB_CPP11_RVALUE_REF_PRESENT                  0
    #define __TBB_IMPLICIT_MOVE_PRESENT                     0
    #define __TBB_EXCEPTION_PTR_PRESENT                     0
    #define __TBB_STATIC_ASSERT_PRESENT                     0
    #define __TBB_CPP11_TUPLE_PRESENT                       0
    #define __TBB_INITIALIZER_LISTS_PRESENT                 0
    #define __TBB_CONSTEXPR_PRESENT                         0
    #define __TBB_DEFAULTED_AND_DELETED_FUNC_PRESENT        0
    #define __TBB_NOEXCEPT_PRESENT                          0
    #define __TBB_CPP11_STD_BEGIN_END_PRESENT               0
    #define __TBB_CPP11_AUTO_PRESENT                        0
    #define __TBB_CPP11_DECLTYPE_PRESENT                    0
    #define __TBB_CPP11_LAMBDAS_PRESENT                     0
    #define __TBB_CPP11_DEFAULT_FUNC_TEMPLATE_ARGS_PRESENT  0
    #define __TBB_OVERRIDE_PRESENT                          0
    #define __TBB_ALIGNAS_PRESENT                           0
    #define __TBB_CPP11_TEMPLATE_ALIASES_PRESENT            0
    #define __TBB_CPP14_INTEGER_SEQUENCE_PRESENT            (__cplusplus >= 201402L)
#endif

#define __TBB_CPP11_ARRAY_PRESENT                           (_MSC_VER >= 1700 || _LIBCPP_VERSION || __GXX_EXPERIMENTAL_CXX0X__ && __TBB_GLIBCXX_VERSION >= 40300)

#ifndef __TBB_CPP11_VARIADIC_FIXED_LENGTH_EXP_PRESENT
#define __TBB_CPP11_VARIADIC_FIXED_LENGTH_EXP_PRESENT       __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT
#endif
#define __TBB_CPP11_VARIADIC_TUPLE_PRESENT                  (!_MSC_VER || _MSC_VER >=1800)

#define __TBB_CPP11_TYPE_PROPERTIES_PRESENT                 (_LIBCPP_VERSION || _MSC_VER >= 1700 || (__TBB_GLIBCXX_VERSION >= 50000 && __GXX_EXPERIMENTAL_CXX0X__))
#define __TBB_TR1_TYPE_PROPERTIES_IN_STD_PRESENT            (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GLIBCXX_VERSION >= 40300 || _MSC_VER >= 1600)
#define __TBB_CPP11_IS_COPY_CONSTRUCTIBLE_PRESENT           (__GXX_EXPERIMENTAL_CXX0X__ && __TBB_GLIBCXX_VERSION >= 40700 || __TBB_CPP11_TYPE_PROPERTIES_PRESENT)
#define __TBB_MOVE_IF_NOEXCEPT_PRESENT                      (__TBB_NOEXCEPT_PRESENT && (__TBB_GLIBCXX_VERSION >= 40700 || _MSC_VER >= 1900 || _LIBCPP_VERSION))
#define __TBB_ALLOCATOR_TRAITS_PRESENT                      (__cplusplus >= 201103L && _LIBCPP_VERSION  || _MSC_VER >= 1700 ||  \
                                                            __GXX_EXPERIMENTAL_CXX0X__ && __TBB_GLIBCXX_VERSION >= 40700 && !(__TBB_GLIBCXX_VERSION == 40700 && __TBB_DEFINE_MIC))
#define __TBB_MAKE_EXCEPTION_PTR_PRESENT                    (__TBB_EXCEPTION_PTR_PRESENT && (_MSC_VER >= 1700 || __TBB_GLIBCXX_VERSION >= 40600 || _LIBCPP_VERSION))
#define __TBB_CPP11_SMART_POINTERS_PRESENT                  ( _MSC_VER >= 1600 || _LIBCPP_VERSION   \
                                                            || ((__cplusplus >= 201103L || __GXX_EXPERIMENTAL_CXX0X__)  \
                                                            && (__TBB_GLIBCXX_VERSION>=40500 || __TBB_GLIBCXX_VERSION>=40400 && __TBB_USE_OPTIONAL_RTTI)) )

#define __TBB_CPP11_FUTURE_PRESENT                          (_MSC_VER >= 1700 || __TBB_GLIBCXX_VERSION >= 40600 && _GXX_EXPERIMENTAL_CXX0X__ || _LIBCPP_VERSION)

#define __TBB_CPP11_GET_NEW_HANDLER_PRESENT                 (_LIBCPP_VERSION || _MSC_VER >= 1900 || (__TBB_GLIBCXX_VERSION >= 40900 && __GXX_EXPERIMENTAL_CXX0X__))
#if _MSC_VER>=1400 || _LIBCPP_VERSION || __GXX_EXPERIMENTAL_CXX0X__
#define __TBB_STD_SWAP_HEADER <utility>
#else
#define __TBB_STD_SWAP_HEADER <algorithm>
#endif

#if __INTEL_COMPILER && __GNUC__ && __TBB_EXCEPTION_PTR_PRESENT && !defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
    #define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 1
#endif

#if __MINGW32__ && __TBB_EXCEPTION_PTR_PRESENT && !defined(_GLIBCXX_ATOMIC_BUILTINS_4)
    #define _GLIBCXX_ATOMIC_BUILTINS_4
#endif

#if __GNUC__ || __SUNPRO_CC || __IBMCPP__
    #define __TBB_ATTRIBUTE_ALIGNED_PRESENT 1
#elif _MSC_VER && (_MSC_VER >= 1300 || __INTEL_COMPILER)
    #define __TBB_DECLSPEC_ALIGN_PRESENT 1
#endif

#if __TBB_GCC_VERSION >= 40306 || __INTEL_COMPILER >= 1200 || __clang__
    /** built-in atomics available in GCC since 4.1.2 **/
    #define __TBB_GCC_BUILTIN_ATOMICS_PRESENT 1
#endif

#if __INTEL_COMPILER >= 1200
    #define __TBB_ICC_BUILTIN_ATOMICS_PRESENT 1
#endif

#define __TBB_TSX_INTRINSICS_PRESENT ((__RTM__ || _MSC_VER>=1700 || __INTEL_COMPILER>=1300) && !__TBB_DEFINE_MIC && !__ANDROID__)

/** Macro helpers **/
#define __TBB_CONCAT_AUX(A,B) A##B
// The additional level of indirection is needed to expand macros A and B (not to get the AB macro).
#define __TBB_CONCAT(A,B) __TBB_CONCAT_AUX(A,B)
// The IGNORED argument and comma are needed to always have 2 arguments (even when A is empty).
#define __TBB_IS_MACRO_EMPTY(A,IGNORED) __TBB_CONCAT_AUX(__TBB_MACRO_EMPTY,A)
#define __TBB_MACRO_EMPTY 1

#ifndef TBB_USE_DEBUG
/*
There are four cases that are supported:
  1. "_DEBUG is undefined" means "no debug";
  2. "_DEBUG defined to something that is evaluated to 0" (including "garbage", as per [cpp.cond]) means "no debug";
  3. "_DEBUG defined to something that is evaluated to a non-zero value" means "debug";
  4. "_DEBUG defined to nothing (empty)" means "debug".
*/
#ifdef _DEBUG
// Check if _DEBUG is empty.
#define __TBB_IS__DEBUG_EMPTY (__TBB_IS_MACRO_EMPTY(_DEBUG,IGNORED)==__TBB_MACRO_EMPTY)
#if __TBB_IS__DEBUG_EMPTY
#define TBB_USE_DEBUG 1
#else
#define TBB_USE_DEBUG _DEBUG
#endif /* __TBB_IS__DEBUG_EMPTY */
#else
#define TBB_USE_DEBUG 0
#endif
#endif /* TBB_USE_DEBUG */

#ifndef TBB_USE_ASSERT
#define TBB_USE_ASSERT TBB_USE_DEBUG
#endif /* TBB_USE_ASSERT */

#ifndef TBB_USE_THREADING_TOOLS
#define TBB_USE_THREADING_TOOLS TBB_USE_DEBUG
#endif /* TBB_USE_THREADING_TOOLS */

#ifndef TBB_USE_PERFORMANCE_WARNINGS
#ifdef TBB_PERFORMANCE_WARNINGS
#define TBB_USE_PERFORMANCE_WARNINGS TBB_PERFORMANCE_WARNINGS
#else
#define TBB_USE_PERFORMANCE_WARNINGS TBB_USE_DEBUG
#endif
#endif

#if __TBB_DEFINE_MIC
    #if TBB_USE_EXCEPTIONS
        #error The platform does not properly support exception handling. Please do not set TBB_USE_EXCEPTIONS macro or set it to 0.
    #elif !defined(TBB_USE_EXCEPTIONS)
        #define TBB_USE_EXCEPTIONS 0
    #endif
#elif !(__EXCEPTIONS || defined(_CPPUNWIND) || __SUNPRO_CC)
    #if TBB_USE_EXCEPTIONS
        #error Compilation settings do not support exception handling. Please do not set TBB_USE_EXCEPTIONS macro or set it to 0.
    #elif !defined(TBB_USE_EXCEPTIONS)
        #define TBB_USE_EXCEPTIONS 0
    #endif
#elif !defined(TBB_USE_EXCEPTIONS)
    #define TBB_USE_EXCEPTIONS 1
#endif

#ifndef TBB_IMPLEMENT_CPP0X
    #if __clang__
        #if (__INTEL_COMPILER && (__INTEL_COMPILER < 1500 || __INTEL_COMPILER == 1500 && __INTEL_COMPILER_UPDATE <= 1))
            #define TBB_IMPLEMENT_CPP0X (__cplusplus < 201103L || !_LIBCPP_VERSION)
        #else
            #define TBB_IMPLEMENT_CPP0X (__cplusplus < 201103L || (!__has_include(<thread>) && !__has_include(<condition_variable>)))
        #endif
    #elif __GNUC__
        #define TBB_IMPLEMENT_CPP0X (__TBB_GCC_VERSION < 40400 || !__GXX_EXPERIMENTAL_CXX0X__)
    #elif _MSC_VER
        #define TBB_IMPLEMENT_CPP0X (_MSC_VER < 1700)
    #else
        // TODO: Reconsider general approach to be more reliable, e.g. (!(__cplusplus >= 201103L && __ STDC_HOSTED__))
        #define TBB_IMPLEMENT_CPP0X (!__STDCPP_THREADS__)
    #endif
#endif

#ifndef TBB_USE_CAPTURED_EXCEPTION
    #if __TBB_EXCEPTION_PTR_PRESENT && !defined(__ia64__)
        #define TBB_USE_CAPTURED_EXCEPTION 0
    #else
        #define TBB_USE_CAPTURED_EXCEPTION 1
    #endif
#else
    #if !TBB_USE_CAPTURED_EXCEPTION && !__TBB_EXCEPTION_PTR_PRESENT
        #error Current runtime does not support std::exception_ptr. Set TBB_USE_CAPTURED_EXCEPTION and make sure that your code is ready to catch tbb::captured_exception.
    #endif
#endif

/** Check whether the request to use GCC atomics can be satisfied **/
#if TBB_USE_GCC_BUILTINS && !__TBB_GCC_BUILTIN_ATOMICS_PRESENT
    #error "GCC atomic built-ins are not supported."
#endif

/**
 * Internal TBB feature and nodes
 */

// __TBB_WEAK_SYMBOLS_PRESENT denotes that the systems supports the weak symbol mechanism
#ifndef __TBB_WEAK_SYMBOLS_PRESENT
#define __TBB_WEAK_SYMBOLS_PRESENT ( !_WIN32 && !__APPLE__&& !__sun && (__TBB_GCC_VERSION >= 40000 || __INTEL_COMPILER))
#endif

// __TBB_DYNAMIC_LOAD_ENABLED describes the system possibility to load shared libraries at run time
#ifndef __TBB_DYNAMIC_LOAD_ENABLED
#define __TBB_DYNAMIC_LOAD_ENABLED 1
#endif

/**
 * __TBB_SOURCE_DIRECTLY_INCLUDED is a mode used in whitebox testing when
 * it is necessary to test internal functions not exported
 */

#if (_WIN32||_WIN64) && (__TBB_SOURCE_DIRECTLY_INCLUDED || TBB_USE_PREVIEW_BINARY)
#define __TBB_NO_IMPLICIT_LINKAGE 1
#define __TBBMALLOC_NO_IMPLICIT_LINKAGE 1
#endif

#ifndef __TBB_COUNT_TASK_NODES
    #define __TBB_COUNT_TASK_NODES TBB_USE_ASSERT
#endif

#ifndef __TBB_TASK_GROUP_CONTEXT
    #define __TBB_TASK_GROUP_CONTEXT 1
#endif

#ifndef __TBB_SCHEDULER_OBSERVER
    #define __TBB_SCHEDULER_OBSERVER 1
#endif

#ifndef __TBB_FP_CONTEXT
    #define __TBB_FP_CONTEXT __TBB_TASK_GROUP_CONTEXT
#endif

#if __TBB_FP_CONTEXT && !__TBB_TASK_GROUP_CONTEXT
    #error __TBB_FP_CONTEXT requires __TBB_TASK_GROUP_CONTEXT to be enabled
#endif

#define __TBB_RECYCLE_TO_ENQUEUE __TBB_BUILD // keep non-official

#ifndef __TBB_ARENA_OBSERVER
    #define __TBB_ARENA_OBSERVER ((__TBB_BUILD||TBB_PREVIEW_LOCAL_OBSERVER)&& __TBB_SCHEDULER_OBSERVER)
#endif /* __TBB_ARENA_OBSERVER */

#ifndef __TBB_SLEEP_PERMISSION
    #define __TBB_SLEEP_PERMISSION ((__TBB_CPF_BUILD||TBB_PREVIEW_LOCAL_OBSERVER)&& __TBB_SCHEDULER_OBSERVER)
#endif /* __TBB_SLEEP_PERMISSION */

#ifndef __TBB_TASK_ISOLATION
    #define __TBB_TASK_ISOLATION 1
#endif /* __TBB_TASK_ISOLATION */

#if TBB_USE_EXCEPTIONS && !__TBB_TASK_GROUP_CONTEXT
    #error TBB_USE_EXCEPTIONS requires __TBB_TASK_GROUP_CONTEXT to be enabled
#endif

#ifndef __TBB_TASK_PRIORITY
    #define __TBB_TASK_PRIORITY (__TBB_TASK_GROUP_CONTEXT)
#endif /* __TBB_TASK_PRIORITY */

#if __TBB_TASK_PRIORITY && !__TBB_TASK_GROUP_CONTEXT
    #error __TBB_TASK_PRIORITY requires __TBB_TASK_GROUP_CONTEXT to be enabled
#endif

#if TBB_PREVIEW_WAITING_FOR_WORKERS || __TBB_BUILD
    #define __TBB_SUPPORTS_WORKERS_WAITING_IN_TERMINATE 1
#endif

#ifndef __TBB_ENQUEUE_ENFORCED_CONCURRENCY
    #define __TBB_ENQUEUE_ENFORCED_CONCURRENCY 1
#endif

#if !defined(__TBB_SURVIVE_THREAD_SWITCH) && \
          (_WIN32 || _WIN64 || __APPLE__ || (__linux__ && !__ANDROID__))
    #define __TBB_SURVIVE_THREAD_SWITCH 1
#endif /* __TBB_SURVIVE_THREAD_SWITCH */

#ifndef __TBB_DEFAULT_PARTITIONER
#define __TBB_DEFAULT_PARTITIONER tbb::auto_partitioner
#endif

#ifndef __TBB_USE_PROPORTIONAL_SPLIT_IN_BLOCKED_RANGES
#define __TBB_USE_PROPORTIONAL_SPLIT_IN_BLOCKED_RANGES 1
#endif

#ifndef __TBB_ENABLE_RANGE_FEEDBACK
#define __TBB_ENABLE_RANGE_FEEDBACK 0
#endif

#ifdef _VARIADIC_MAX
    #define __TBB_VARIADIC_MAX _VARIADIC_MAX
#else
    #if _MSC_VER == 1700
        #define __TBB_VARIADIC_MAX 5
    #elif _MSC_VER == 1600
        #define __TBB_VARIADIC_MAX 10
    #else
        #define __TBB_VARIADIC_MAX 15
    #endif
#endif

/** __TBB_WIN8UI_SUPPORT enables support of Windows* Store Apps and limit a possibility to load
    shared libraries at run time only from application container **/
#if defined(WINAPI_FAMILY) && WINAPI_FAMILY == WINAPI_FAMILY_APP
    #define __TBB_WIN8UI_SUPPORT 1
#else
    #define __TBB_WIN8UI_SUPPORT 0
#endif

/** Macros of the form __TBB_XXX_BROKEN denote known issues that are caused by
    the bugs in compilers, standard or OS specific libraries. They should be
    removed as soon as the corresponding bugs are fixed or the buggy OS/compiler
    versions go out of the support list.
**/

#if __SIZEOF_POINTER__ < 8 && __ANDROID__ && __TBB_GCC_VERSION <= 40403 && !__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8
    /** Necessary because on Android 8-byte CAS and F&A are not available for some processor architectures,
        but no mandatory warning message appears from GCC 4.4.3. Instead, only a linkage error occurs when
        these atomic operations are used (such as in unit test test_atomic.exe). **/
    #define __TBB_GCC_64BIT_ATOMIC_BUILTINS_BROKEN 1
#elif __TBB_x86_32 && __TBB_GCC_VERSION == 40102 && ! __GNUC_RH_RELEASE__
    /** GCC 4.1.2 erroneously emit call to external function for 64 bit sync_ intrinsics.
        However these functions are not defined anywhere. It seems that this problem was fixed later on
        and RHEL got an updated version of gcc 4.1.2. **/
    #define __TBB_GCC_64BIT_ATOMIC_BUILTINS_BROKEN 1
#endif

#if __GNUC__ && __TBB_x86_64 && __INTEL_COMPILER == 1200
    #define __TBB_ICC_12_0_INL_ASM_FSTCW_BROKEN 1
#endif

#if _MSC_VER && __INTEL_COMPILER && (__INTEL_COMPILER<1110 || __INTEL_COMPILER==1110 && __INTEL_COMPILER_BUILD_DATE < 20091012)
    /** Necessary to avoid ICL error (or warning in non-strict mode):
        "exception specification for implicitly declared virtual destructor is
        incompatible with that of overridden one". **/
    #define __TBB_DEFAULT_DTOR_THROW_SPEC_BROKEN 1
#endif

#if !__INTEL_COMPILER && (_MSC_VER && _MSC_VER < 1500 || __GNUC__ && __TBB_GCC_VERSION < 40102)
    /** gcc 3.4.6 (and earlier) and VS2005 (and earlier) do not allow declaring template class as a friend
        of classes defined in other namespaces. **/
    #define __TBB_TEMPLATE_FRIENDS_BROKEN 1
#endif

#if __GLIBC__==2 && __GLIBC_MINOR__==3 ||  (__APPLE__ && ( __INTEL_COMPILER==1200 && !TBB_USE_DEBUG))
    /** Macro controlling EH usages in TBB tests.
        Some older versions of glibc crash when exception handling happens concurrently. **/
    #define __TBB_THROW_ACROSS_MODULE_BOUNDARY_BROKEN 1
#endif

#if (_WIN32||_WIN64) && __INTEL_COMPILER == 1110
    /** That's a bug in Intel C++ Compiler 11.1.044/IA-32 architecture/Windows* OS, that leads to a worker thread crash on the thread's startup. **/
    #define __TBB_ICL_11_1_CODE_GEN_BROKEN 1
#endif

#if __clang__ || (__GNUC__==3 && __GNUC_MINOR__==3 && !defined(__INTEL_COMPILER))
    /** Bugs with access to nested classes declared in protected area */
    #define __TBB_PROTECTED_NESTED_CLASS_BROKEN 1
#endif

#if __MINGW32__ && __TBB_GCC_VERSION < 40200
    /** MinGW has a bug with stack alignment for routines invoked from MS RTLs.
     **/
    #define __TBB_SSE_STACK_ALIGNMENT_BROKEN 1
#endif

#if __TBB_GCC_VERSION==40300 && !__INTEL_COMPILER && !__clang__
    /* GCC of this version may rashly ignore control dependencies */
    #define __TBB_GCC_OPTIMIZER_ORDERING_BROKEN 1
#endif

#if __FreeBSD__
    /** A bug in FreeBSD 8.0 results in kernel panic when there is contention
        on a mutex created with this attribute. **/
    #define __TBB_PRIO_INHERIT_BROKEN 1

    /** A bug in FreeBSD 8.0 results in test hanging when an exception occurs
        during (concurrent?) object construction by means of placement new operator. **/
    #define __TBB_PLACEMENT_NEW_EXCEPTION_SAFETY_BROKEN 1
#endif /* __FreeBSD__ */

#if (__linux__ || __APPLE__) && __i386__ && defined(__INTEL_COMPILER)
    /** The Intel C++ Compiler for IA-32 architecture (Linux* OS|macOS) crashes or generates
        incorrect code when __asm__ arguments have a cast to volatile. **/
    #define __TBB_ICC_ASM_VOLATILE_BROKEN 1
#endif

#if !__INTEL_COMPILER && (_MSC_VER || __GNUC__==3 && __GNUC_MINOR__<=2)
    /** Bug in GCC 3.2 and MSVC compilers that sometimes return 0 for __alignof(T)
        when T has not yet been instantiated. **/
    #define __TBB_ALIGNOF_NOT_INSTANTIATED_TYPES_BROKEN 1
#endif

#if __TBB_DEFINE_MIC
    /** Main thread and user's thread have different default thread affinity masks. **/
    #define __TBB_MAIN_THREAD_AFFINITY_BROKEN 1
#endif

#if __GXX_EXPERIMENTAL_CXX0X__ && !defined(__EXCEPTIONS) && \
    ((!__INTEL_COMPILER && !__clang__ && (__TBB_GCC_VERSION>=40400 && __TBB_GCC_VERSION<40600)) || \
     (__INTEL_COMPILER<=1400 && (__TBB_GLIBCXX_VERSION>=40400 && __TBB_GLIBCXX_VERSION<=40801)))
/* There is an issue for specific GCC toolchain when C++11 is enabled
   and exceptions are disabled:
   exceprion_ptr.h/nested_exception.h use throw unconditionally.
   GCC can ignore 'throw' since 4.6; but with ICC the issue still exists.
 */
    #define __TBB_LIBSTDCPP_EXCEPTION_HEADERS_BROKEN 1
#endif

#if __INTEL_COMPILER==1300 && __TBB_GLIBCXX_VERSION>=40700 && defined(__GXX_EXPERIMENTAL_CXX0X__)
/* Some C++11 features used inside libstdc++ are not supported by Intel C++ Compiler. */
    #define __TBB_ICC_13_0_CPP11_STDLIB_SUPPORT_BROKEN 1
#endif

#if (__GNUC__==4 && __GNUC_MINOR__==4 ) && !defined(__INTEL_COMPILER) && !defined(__clang__)
    /** excessive warnings related to strict aliasing rules in GCC 4.4 **/
    #define __TBB_GCC_STRICT_ALIASING_BROKEN 1
    /* topical remedy: #pragma GCC diagnostic ignored "-Wstrict-aliasing" */
    #if !__TBB_GCC_WARNING_SUPPRESSION_PRESENT
        #error Warning suppression is not supported, while should.
    #endif
#endif

/* In a PIC mode some versions of GCC 4.1.2 generate incorrect inlined code for 8 byte __sync_val_compare_and_swap intrinsic */
#if __TBB_GCC_VERSION == 40102 && __PIC__ && !defined(__INTEL_COMPILER) && !defined(__clang__)
    #define __TBB_GCC_CAS8_BUILTIN_INLINING_BROKEN 1
#endif

#if __TBB_x86_32 && ( __INTEL_COMPILER || (__GNUC__==5 && __GNUC_MINOR__>=2 && __GXX_EXPERIMENTAL_CXX0X__) \
    || (__GNUC__==3 && __GNUC_MINOR__==3) || (__MINGW32__ && __GNUC__==4 && __GNUC_MINOR__==5) || __SUNPRO_CC )
    // Some compilers for IA-32 architecture fail to provide 8-byte alignment of objects on the stack,
    // even if the object specifies 8-byte alignment. On such platforms, the implementation
    // of 64 bit atomics for IA-32 architecture (e.g. atomic<long long>) use different tactics
    // depending upon whether the object is properly aligned or not.
    #define __TBB_FORCE_64BIT_ALIGNMENT_BROKEN 1
#else
    // Define to 0 explicitly because the macro is used in a compiled code of test_atomic
    #define __TBB_FORCE_64BIT_ALIGNMENT_BROKEN 0
#endif

#if __GNUC__ && !__INTEL_COMPILER && !__clang__ && __TBB_DEFAULTED_AND_DELETED_FUNC_PRESENT && __TBB_GCC_VERSION < 40700
    #define __TBB_ZERO_INIT_WITH_DEFAULTED_CTOR_BROKEN 1
#endif

#if _MSC_VER && _MSC_VER <= 1800 && !__INTEL_COMPILER
    // With MSVC, when an array is passed by const reference to a template function,
    // constness from the function parameter may get propagated to the template parameter.
    #define __TBB_CONST_REF_TO_ARRAY_TEMPLATE_PARAM_BROKEN 1
#endif

// A compiler bug: a disabled copy constructor prevents use of the moving constructor
#define __TBB_IF_NO_COPY_CTOR_MOVE_SEMANTICS_BROKEN (_MSC_VER && (__INTEL_COMPILER >= 1300 && __INTEL_COMPILER <= 1310) && !__INTEL_CXX11_MODE__)

#define __TBB_CPP11_DECLVAL_BROKEN (_MSC_VER == 1600 || (__GNUC__ && __TBB_GCC_VERSION < 40500) )
// Intel C++ Compiler has difficulties with copying std::pair with VC11 std::reference_wrapper being a const member
#define __TBB_COPY_FROM_NON_CONST_REF_BROKEN (_MSC_VER == 1700 && __INTEL_COMPILER && __INTEL_COMPILER < 1600)

// The implicit upcasting of the tuple of a reference of a derived class to a base class fails on icc 13.X if the system's gcc environment is 4.8
// Also in gcc 4.4 standard library the implementation of the tuple<&> conversion (tuple<A&> a = tuple<B&>, B is inherited from A) is broken.
#if __GXX_EXPERIMENTAL_CXX0X__ && __GLIBCXX__ && ((__INTEL_COMPILER >=1300 && __INTEL_COMPILER <=1310 && __TBB_GLIBCXX_VERSION>=40700) || (__TBB_GLIBCXX_VERSION < 40500))
#define __TBB_UPCAST_OF_TUPLE_OF_REF_BROKEN 1
#endif

// In some cases decltype of a function adds a reference to a return type.
#define __TBB_CPP11_DECLTYPE_OF_FUNCTION_RETURN_TYPE_BROKEN (_MSC_VER == 1600 && !__INTEL_COMPILER)

/** End of __TBB_XXX_BROKEN macro section **/

#if defined(_MSC_VER) && _MSC_VER>=1500 && !defined(__INTEL_COMPILER)
    // A macro to suppress erroneous or benign "unreachable code" MSVC warning (4702)
    #define __TBB_MSVC_UNREACHABLE_CODE_IGNORED 1
#endif

#define __TBB_ATOMIC_CTORS     (__TBB_CONSTEXPR_PRESENT && __TBB_DEFAULTED_AND_DELETED_FUNC_PRESENT && (!__TBB_ZERO_INIT_WITH_DEFAULTED_CTOR_BROKEN))

// Many OS versions (Android 4.0.[0-3] for example) need workaround for dlopen to avoid non-recursive loader lock hang
// Setting the workaround for all compile targets ($APP_PLATFORM) below Android 4.4 (android-19)
#if __ANDROID__
#include <android/api-level.h>
#define __TBB_USE_DLOPEN_REENTRANCY_WORKAROUND  (__ANDROID_API__ < 19)
#endif

#define __TBB_ALLOCATOR_CONSTRUCT_VARIADIC      (__TBB_CPP11_VARIADIC_TEMPLATES_PRESENT && __TBB_CPP11_RVALUE_REF_PRESENT)

#define __TBB_VARIADIC_PARALLEL_INVOKE          (TBB_PREVIEW_VARIADIC_PARALLEL_INVOKE && __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT && __TBB_CPP11_RVALUE_REF_PRESENT)
#define __TBB_FLOW_GRAPH_CPP11_FEATURES         (__TBB_CPP11_VARIADIC_TEMPLATES_PRESENT \
                                                && __TBB_CPP11_SMART_POINTERS_PRESENT && __TBB_CPP11_RVALUE_REF_PRESENT && __TBB_CPP11_AUTO_PRESENT) \
                                                && __TBB_CPP11_VARIADIC_TUPLE_PRESENT && __TBB_CPP11_DEFAULT_FUNC_TEMPLATE_ARGS_PRESENT \
                                                && !__TBB_UPCAST_OF_TUPLE_OF_REF_BROKEN
#define __TBB_PREVIEW_STREAMING_NODE            (__TBB_CPP11_VARIADIC_FIXED_LENGTH_EXP_PRESENT && __TBB_FLOW_GRAPH_CPP11_FEATURES \
                                                && TBB_PREVIEW_FLOW_GRAPH_NODES && !TBB_IMPLEMENT_CPP0X && !__TBB_UPCAST_OF_TUPLE_OF_REF_BROKEN)
#define __TBB_PREVIEW_OPENCL_NODE               (__TBB_PREVIEW_STREAMING_NODE && __TBB_CPP11_TEMPLATE_ALIASES_PRESENT)
#define __TBB_PREVIEW_MESSAGE_BASED_KEY_MATCHING (TBB_PREVIEW_FLOW_GRAPH_FEATURES || __TBB_PREVIEW_OPENCL_NODE)
#define __TBB_PREVIEW_ASYNC_MSG                 (TBB_PREVIEW_FLOW_GRAPH_FEATURES && __TBB_FLOW_GRAPH_CPP11_FEATURES)

#define __TBB_PREVIEW_GFX_FACTORY

#endif /* INCLUDE_TBB_TBB_CONFIG_H_ */
