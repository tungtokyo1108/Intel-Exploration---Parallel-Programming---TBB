/*
 * enumerable_thread_specific.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_ENUMERABLE_THREAD_SPECIFIC_H_
#define INCLUDE_TBB_ENUMERABLE_THREAD_SPECIFIC_H_

#include "atomic.h"
#include "concurrent_vector.h"
#include "tbb_thread.h"
#include "tbb_allocator.h"
#include "cache_aligned_allocator.h"
#include "aligned_space.h"
#include "tbb_profiling.h"
#include "internal/_template_helper.h"
#include "internal/_tbb_hash_compare_impl.h"
#include <string.h>

#if _WIN32||_WIN64
#include "machine/windows_api.h"
#else
#include <pthread.h>
#endif

#define __TBB_ETS_USE_CPP11 \
    (__TBB_CPP11_RVALUE_REF_PRESENT && __TBB_CPP11_VARIADIC_TEMPLATES_PRESENT \
&& __TBB_CPP11_DECLTYPE_PRESENT && __TBB_CPP11_LAMBDAS_PRESENT)

namespace tbb {
enum ets_key_usage_type {ets_key_per_instance, ets_no_key};

namespace interface6 {

template <typename T, typename Allocator, ets_key_usage_type ETS_key_type>
class enumerable_thread_specific;

namespace internal {

using namespace tbb::internal;
template <ets_key_usage_type ETS_key_type>
class ets_base: tbb::internal::no_copy {
protected:
	typedef tbb_thread::id key_type;
#if __TBB_PROTECTED_NESTED_CLASS_BROKEN
public:
#endif

	struct slot;

	struct array {
		array* next;
		size_t lg_size;
		slot& at(size_t k) {
			return ((slot*)(void*)(this+1))[k];
		}
		size_t size() const {return size_t(1)<<lg_size;}
		size_t mask() const {return size()-1;}
		size_t start(size_t h) const {
			return h>>(8*sizeof(size_t)-lg_size);
		}
	};

	struct slot {
		key_type key;
		void* ptr;
		bool empty() const {return key == key_type();}
		bool match(key_type k) const {return key == k;}
		bool claim(key_type k) {
			return atomic_compare_and_swap(key,k,key_type()) == key_type();
		}
	};

#if __TBB_PROTECTED_NESTED_CLASS_BROKEN
private:
#endif

	/*
	 * Root of linked list of arrays of decreasing size
	 */
	atomic<array*> my_root;
	atomic<size_t> my_count;
	virtual void* create_local() = 0;
	virtual void* create_array(size_t _size) = 0;
	virtual void free_array(void* ptr, size_t _size) = 0;

	array* allocate(size_t lg_size) {
		size_t n = size_t(1)<<lg_size;
		array* a = static_cast<array*>(create_array(sizeof(array)+n*sizeof(slot)));
		a->lg_size = lg_size;
		std::memset(a+1,0,n*sizeof(slot));
		return a;
	}
	void free(array* a) {
		size_t n = size_t(1)<<(a->lg_size);
		free_array((void*)a, size_t(sizeof(array)+n*sizeof(slot)));
	}

	ets_base() {my_root = NULL; my_count = 0;}
	virtual ~ets_base();
	void* table_lookup(bool& exists);
	void table_clear();

	void table_elementwise_copy(const ets_base& other,
			void *(*add_element)(ets_base&, void*))
	{
		__TBB_ASSERT(!my_root,NULL);
		__TBB_ASSERT(!my_count,NULL);
		if (!other.my_root) return;
		array *root = my_root = allocate(other.my_root->lg_size);
		root->next = NULL;
		my_count = other.my_count;
		size_t mask = root->mask();
		for (array *r = other.my_root; r; r = r->next)
		{
			for (size_t i=0; i < r->size(); ++i)
			{
				slot& s1 = r->at(i);
				if (!s1.empty())
				{
					for (size_t j = root->start(tbb::tbb_hash<key_type>()(s1.key)); ;j=(j+1)&mask)
					{
						slot& s2 = root->at(j);
						if (!s2.empty())
						{
							s2.ptr = add_element(*this,s1.ptr);
							s2.key = s1.key;
							break;
						}
						else if (s2.match(s1.key))
							break;
					}
				}
			}
		}
	}

	void table_swap(ets_base& other)
	{
		__TBB_ASSERT(this!=&other, "Don't swap an instance with itself");
		tbb::internal::swap<relaxed>(my_root,other.my_root);
		tbb::internal::swap<relaxed>(my_count,other.my_count);
	}
};

template <ets_key_usage_type ETS_key_type>
ets_base<ETS_key_type>::~ets_base() {
	__TBB_ASSERT(!my_root,NULL);
}

template <ets_key_usage_type ETS_key_type>
void ets_base<ETS_key_type>::table_clear() {
	while (array *r = my_root)
	{
		my_root = r->next;
		free(r);
	}
	my_count = 0;
}

template <ets_key_usage_type ETS_key_type>
void* ets_base<ETS_key_type>::table_lookup(bool &exists)
{
	const key_type k = tbb::this_tbb_thread::get_id();
	__TBB_ASSERT(k != key_type(),NULL);
	void* found;
	size_t h = tbb::tbb_hash<key_type>()(k);
	for (array *r = my_root; r; r=r->next)
	{
		call_itt_notify(acquired,r);
		size_t mask = r->mask();
		for (size_t i = r->start(h); ; i=(i+1)&mask)
		{
			slot& s = r->at(i);
			if (s.empty()) break;
			if (s.match(k))
			{
				if (r==my_root)
				{
					exists = true;
					return s.ptr;
				}
				else
				{
					exists = true;
					found = s.ptr;
					goto insert;
				}
			}
		}
	}

	/*
	 * Key does not yet exist. The density of slots in the table does not exceed 0.5,
	 * for is this will occur a new table is allocated with double with double the current table size,
	 * which is swapped in as the new root table.
	 */
	exists = false;
	found = create_local();
	{
		size_t c = ++my_count;
		array *r = my_root;
		call_itt_notify(acquired,r);
		if (!r || c > r->size()/2)
		{
			size_t s = r ? r->lg_size : 2;
			while (c > size_t(1)<<(s-1)) ++s;
			array *a = allocate(s);
			for (;;)
			{
				a->next = r;
				call_itt_notify(releasing,a);
				array* new_r = my_root.compare_and_swap(a,r);
				if (new_r == r) break;
				call_itt_notify(acquired,new_r);
				if (new_r->lg_size >= s)
				{
					free(a);
					break;
				}
				r = new_r;
			}
		}
	}

	insert:
	array* ir = my_root;
	call_itt_notify(acquired,ir);
	size_t mask = ir->mask();
	for (size_t i = ir->start(h);;i=(i+1)&mask)
	{
		slot& s = ir->at(i);
		if (s.empty())
		{
			if (s.claim(k))
			{
				s.ptr = found;
				return found;
			}
		}
	}
}

/*
 * Specialization that exploits native TLS
 */
template <>
class ets_base<ets_key_per_instance>: protected ets_base<ets_no_key> {
	typedef ets_base<ets_no_key> super;

  #if _WIN32||_WIN64
  #if __TBB_WIN8UI_SUPPORT
  typedef DWORD tls_key_t;
  void create_key() {my_key = FlsAlloc(NULL);}
  void destroy_key() {FlsFree(my_key);}
  void set_tls(void* value) {FlsSetValue(my_key, (LPVOID)value);}
  void *get_tls() {return (void*)FlsGetValue(my_key);}
  #else
  typedef DWORD tls_key_t;
  void create_key() {my_key = TlsAlloc();}
  void destroy_key() {TlsFree(my_key);}
  void set_tls(void* value) {TlsSetValue(my_key, (LPVOID)value);}
  void *get_tls() {return (void*)TlsGetValue(my_key);}
  #endif
  #else
  typedef pthread_key_t tls_key_t;
  void create_key() {pthread_key_create(&my_key,NULL);}
  void destroy_key() {pthread_key_delete(my_key);}
  void set_tls(void* value) const {pthread_setspecific(my_key,value);}
  void *get_tls() {return (void*)pthread_getspecific(my_key);}
  #endif
  
  tls_key_t my_key;
  virtual void* create_local() __TBB_override = 0;
  virtual void* create_array(size_t _size) __TBB_override = 0;
  virtual void free_array(void* ptr, size_t size) __TBB_override = 0;

protected:
  ets_base() {create_key();}
  ~ets_base() {destroy_key();}

  void* table_lookup(bool& exists)
  {
	  void* found = get_tls();
	  if (found)
	  {
		  exists = true;
	  }
	  else
	  {
		  found = super::table_lookup(exists);
		  set_tls(found);
	  }
	  return found;
  }
  void table_clear() {
	  destroy_key();
	  create_key();
	  super::table_clear();
  }
  void table_swap(ets_base& other)
  {
	  using std::swap;
	  __TBB_ASSERT(this!=&other, "Don't swap an instance with itself");
	  swap(my_key, other.my_key);
	  super::table_swap(other);
  }
};



}

}

}

#endif /* INCLUDE_TBB_ENUMERABLE_THREAD_SPECIFIC_H_ */
