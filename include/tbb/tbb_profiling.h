/*
 * tbb_profiling.h
 *
 *  Created on: Oct 15, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_TBB_PROFILING_H_
#define INCLUDE_TBB_TBB_PROFILING_H_

namespace tbb {
	namespace internal {
#define TBB_STRING_RESOURCE(index_name,str) index_name,
		enum string_index {
            #include "internal/_tbb_strings.h"
			NUM_STRINGS
		};
        #undef TBB_STRING_RESOURCE

		enum itt_relation {
			__itt_relation_is_unknown = 0,
			__itt_relation_is_dependent_on,
			__itt_relation_is_sibling_of,
			__itt_relation_is_parent_of,
			__itt_relation_is_continuation_of,
			__itt_relation_is_child_of,
			__itt_relation_is_continued_by,
			__itt_relation_is_predecessor_to
		};
	}
}

#if (_WIN32||_WIN64||__linux__) && !__MINGW32__ && TBB_USE_THREADING_TOOLS
#if _WIN32||_WIN64
#include <stdlib.h>
#endif
#include "tbb_stddef.h"

namespace tbb {
	namespace internal {
#if _WIN32||_WIN64
		void __TBB_EXPORTED_FUNC itt_set_sync_name_v3(void *obj, const wchar_t* name);
		inline size_t multibyte_to_widechar(wchar_t* wcs, const char *mbs, size_t bufsize) {
#if _MSC_VER >= 1400
			size_t len;
			mbstowcs_s(&len, wcs, bufsize, mbs, _TRUNCATE);
			return len;
#else 
			size_t len = mbstowcs(wcs, mbs, bufsize);
			if (wcs && len != size_t(-1))
				wcs[len < bufsize - 1 ? len : bufsize - 1] = wchar_t('\0');
			return len + 1;
#endif 
		}
#else 
		void __TBB_EXPORTED_FUNC itt_set_sync_name_v3(void *obj, const char *name);
#endif 
	}
}

#if _WIN32||_WIN64
#define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type) \
namspace profiling { \
inline void set_name(sync_object_type& obj, const wchar_t *name) { \
tbb::internal::itt_set_sync_name_v3(&obj,name);\
}\
inline void set_name(sync_object_type& obj, const char* name) {\
size_t len = tbb::internal::multibyte_to_widechar(NULL,name,0); \
wchar_t *wname = new wchar_t[len]; \
tbb::internal::multibyte_to_widechar(wname, name, len); \
set_name(obj,wname);\
delete[] wname; \
}\
}
#else 
#define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type) \
namespace profiling {\
inline void set_name(sync_object_type& obj, const char* name) {\
tbb::internal::itt_set_sync_name_v3(&obj,name);\
}\
}
#endif 

#else 
#if _WIN32||_WIN64
#define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type) \
namespace profiling { \
inline void set_name(sync_object_tpye&, const wchar_t*) {} \
inline void set_name(sync_object_type&, const char*) {} \
}
#else 
#define __TBB_DEFINE_PROFILING_SET_NAME(sync_object_type) \
namespace profiling { \
inline void set_name(sync_object_type&, const char*) {} \
}
#endif
#endif

#include "atomic.h"
namespace tbb {
	namespace internal {
		enum notify_type {prepare=0, cancel, acquired, releasing};
		const uintptr_t NUM_NOTIFY_TYPES = 4;

		void __TBB_EXPORTED_FUNC call_itt_notify_v5(int t, void *ptr);
		void __TBB_EXPORTED_FUNC itt_store_pointer_with_release_v3(void *dst, void *src);
		void* __TBB_EXPORTED_FUNC itt_load_pointer_with_acquire_v3(const void *src);
		void* __TBB_EXPORTED_FUNC itt_load_pointer_v3(const void* src);
		enum itt_domain_enum { ITT_DOMAIN_FLOW = 0, ITT_DOMAIN_MAIN = 1, ITT_DOMAIN_ALGO = 2, ITT_NUM_DOMAINS };

		void __TBB_EXPORTED_FUNC itt_make_task_group_v7(itt_domain_enum domain, void *group, unsigned long long group_extra,
			void *parent, unsigned long long parent_extra, string_index name_index);
		void __TBB_EXPORTED_FUNC itt_metadata_str_add_v7(itt_domain_enum domain, void *addr, unsigned long long addr_extra,
			string_index key, const char *value);
		void __TBB_EXPORTED_FUNC itt_relation_add_v7(itt_domain_enum domain, void *addr0, unsigned long long addr0_extra,
			itt_relation relation, void *addr1, unsigned long long addr1_extra);
		void __TBB_EXPORTED_FUNC itt_task_begin_v7(itt_domain_enum domain, void *task, unsigned long long task_extra,
			void *parent, unsigned long long parent_extra, string_index name_index);
		void __TBB_EXPORTED_FUNC itt_task_end_v7(itt_domain_enum domain);

		void __TBB_EXPORTED_FUNC itt_region_begin_v9(itt_domain_enum domain, void *region, unsigned long long region_extra,
			void *parent, unsigned long long parent_extra, string_index name_index);
		void __TBB_EXPORTED_FUNC itt_region_end_v9(itt_domain_enum domain, void *region, unsigned long long region_extra);

		template<typename T, typename U>
		inline void itt_store_word_with_release(tbb::atomic<T>& dst, U src) {
#if TBB_USE_THREADING_TOOLS
			__TBB_ASSERT(sizeof(T) == sizeof(void *), "Type must be word-sized.");
			itt_store_pointer_with_release_v3(&dst, (void*)uintptr_t(src));
#else 
			dst = src;
#endif 
		}

		template <typename T>
		inline T itt_load_word_with_acquire(const tbb::atomic<T>& src) {
#if TBB_USE_THREADING_TOOLS
			__TBB_ASSERT(sizeof(T) == sizeof(void *), "Type must be word-sized.");
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning (push)
#pragma warning (disable: 4311)
#endif
			T result = (T)itt_load_pointer_with_acquire_v3(%src);
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning (pop)
#endif 
			return result;
#else 
			return src;
#endif
		}

		template<typename T>
		inline void itt_store_word_with_release(T& dst, T& src) {
#if TBB_USE_THREADING_TOOLS
			__TBB_ASSERT(sizeof(T) == sizeof(void *), "Type must be word-sized.");
			itt_store_pointer_with_release_v3(&dst, (void *)src);
#else 
			__TBB_store_with_release(dst, src);
#endif
		}

		template<typename T>
		inline T itt_load_word_with_acquire(const T& src) {
#if TBB_USE_THREADING_TOOLS
			__TBB_ASSERT(sizeof(T) == sizeof(void *), "Type must be word-sized.");
			return (T)itt_load_pointer_with_acquire_v3(&src);
#endif
			return __TBB_load_with_acquire(src);
		}

		template<typename T> 
		inline void itt_hide_store_word(T& dst, T src) {
#if TBB_USE_THREADING_TOOLS
			__TBB_ASSERT(sizeof(T) == sizeof(void *), "Type must be word-sized.");
			itt_store_pointer_with_release_v3(&dst, (void*)src);
#else 
			dst = src;
#endif
		}

		template <typename T>
		inline T itt_hide_load_word(const T& src) {
#if TBB_USE_THREADING_TOOLS
			__TBB_ASSERT(sizeof(T) == sizeof(void *), "Type must be word-sized.");
			return (T)itt_load_pointer_v3(&src);
#else 
			return src;
#endif 
		}

#if TBB_USE_THREADING_TOOLS
		inline void call_itt_notify(notify_type t, void *ptr) {
			call_itt_notify_v5((int)t, ptr);
		}

		inline void itt_make_task_group(itt_domain_enum domain, void* group, unsigned long long group_extra,
			void *parent, unsigned long long parent_extra, string_index name_index) {
			itt_make_task_group_v7(domain, group, group_extra, parent, parent_extra, name_index);
		}

		inline void itt_metadata_str_add(itt_domain_enum domain, void *addr, unsigned long long addr_extra,
			string_index key, const char *value) {
			itt_metadata_str_add_v7(domain, addr, addr_extra, key, value);
		}

		inline void itt_relation_add(itt_domain_enum domain, void *addr0, unsigned long long addr0_extra,
			itt_relation relation, void *addr1, unsigned long long addr1_extra) {
			itt_relation_add_v7(domain, addr0, addr0_extra, relation, addr1, addr1_extra);
		}

		inline void itt_task_begin(itt_domain_enum domain, void *task, unsigned long long task_extra,
			void *parent, unsigned long long parent_extra, string_index name_index) {
			itt_task_begin_v7(domain, task, task_extra, parent, parent_extra, name_index);
		}

		inline void itt_task_end(itt_domain_enum domain) {
			itt_task_end_v7(domain);
		}

		inline void itt_region_begin(itt_domain_enum domain, void *region, unsigned long long region_extra,
			void *parent, unsigned long long parent_extra, string_index name_index) {
			itt_region_begin_v9(domain, region, region_extra, parent, parent_extra, name_index);
		}

		inline void itt_region_end(itt_domain_enum domain, void *region, unsigned long long region_extra) {
			itt_region_end_v9(domain, region, region_extra);
		}
#else 
		inline void call_itt_notify(notify_type /*t*/, void* /*ptr*/) {}

		inline void itt_make_task_group(itt_domain_enum /*domain*/, void* /*group*/, unsigned long long /*group_extra*/,
			void* /*parent*/, unsigned long long /*parent_extra*/, string_index /*name_index*/) {}

		inline void itt_metadata_str_add(itt_domain_enum /*domain*/, void* /*addr*/, unsigned long long /*addr_extra*/,
			string_index /*key*/, const char* /*value*/) {}

		inline void itt_relation_add(itt_domain_enum /*domain*/, void* /*addr0*/, unsigned long long /*addr0_extra*/,
			itt_relation /*relation*/, void* /*addr1*/, unsigned long long /*addr1_extra*/) {}

		inline void itt_task_begin(itt_domain_enum /*domain*/, void* /*task*/, unsigned long long /*task_extra*/,
			void* /*parent*/, unsigned long long /*parent_extra*/, string_index /*name_index*/) {}

		inline void itt_task_end(itt_domain_enum /*domain*/) {}

		inline void itt_region_begin(itt_domain_enum /*domain*/, void* /*region*/, unsigned long long /*region_extra*/,
			void* /*parent*/, unsigned long long /*parent_extra*/, string_index /*name_index*/) {}

		inline void itt_region_end(itt_domain_enum /*domain*/, void* /*region*/, unsigned long long /*region_extra*/) {}
#endif
	}
}

#if TBB_PREVIEW_FLOW_GRAPH_TRACE
#include <string>
namespace tbb {
	namespace profiling {
		namespace interface10 {
#if TBB_USE_THREADING_TOOLS && !(TBB_USE_THREADING_TOOLS == 2)
/*
Support user event traces through itt.
Common use-case is tagging data flow graph tasks (data-id)
and visualization by Intel Advisor Flow Graph Analyzer
*/
			class event 
			{
				const std::string my_name;
				static void emit_trace(const std::string &input) {
					itt_metadata_str_add(tbb::internal::ITT_DOMAIN_FLOW, NULL, tbb::internal::FLOW_NULL,
						tbb::internal::USER_EVENT, ("FGA::DATAID::" + input).c_str());
				}

			public:
				event(const std::string &input) : my_name(input) {}
				void emit() { emit_trace(my_name); }
				static void emit(const std::string &description) { emit_trace(description); }
			};
#else
			struct event {
				event(const std::string &) {}
				void emit() {}
				static void emit(const std::string &) {}
			};
#endif
		}
	}
}
#endif

#endif /* INCLUDE_TBB_TBB_PROFILING_H_ */
