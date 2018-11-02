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
template <typename Container, typename Value>
class enumerable_thread_specific_iterator
#if defined(_WIN64) && defined(_MSC_VER)
            // Ensure that Microsoft's internal template function _Val_type works correctly.
            : public std::iterator<std::random_access_iterator_tag,Value>
#endif
{
	Container *my_container;
	typename Container::size_type my_index;
	mutable Value *my_value;

	template <typename C, typename T>
	friend enumerable_thread_specific_iterator<C,T>
	operator+(ptrdiff_t offset, const enumerable_thread_specific_iterator<C,T>& v);

	template <typename C, typename T, typename U>
	friend bool operator==(const enumerable_thread_specific_iterator<C,T>& i,
			               const enumerable_thread_specific_iterator<C,U>& j);

	template <typename C, typename T, typename U>
	friend bool operator<(const enumerable_thread_specific_iterator<C,T>& i,
				          const enumerable_thread_specific_iterator<C,U>& j);

	template <typename C, typename T, typename U>
    friend ptrdiff_t operator-(const enumerable_thread_specific_iterator<C,T>& i,
				               const enumerable_thread_specific_iterator<C,U>& j);

	template <typename C, typename U>
	friend class enumerable_thread_specific_iterator;

public:
	enumerable_thread_specific_iterator(const Container &container, typename Container::size_type index) :
		my_container(&const_cast<Container &>(container)), my_index(index), my_value(NULL) {}

	enumerable_thread_specific_iterator() : my_container(NULL), my_index(0), my_value(NULL) {}

	template <typename U>
	enumerable_thread_specific_iterator(const enumerable_thread_specific_iterator<Container,U>& other) :
	    my_container(other.my_container), my_index(other.my_index), my_value(const_cast<Value*>(other.my_value)) {}

	enumerable_thread_specific_iterator operator+(ptrdiff_t offset) const {
		return enumerable_thread_specific_iterator(*my_container, my_index + offset);
	}

	enumerable_thread_specific_iterator &operator+=(ptrdiff_t offset) {
		my_index += offset;
		my_value = NULL;
		return *this;
	}

	enumerable_thread_specific_iterator operator-(ptrdiff_t offset) const {
	    return enumerable_thread_specific_iterator(*my_container, my_index - offset);
	}

	enumerable_thread_specific_iterator &operator-=(ptrdiff_t offset) {
		my_index -= offset;
		my_value = NULL;
		return *this;
	}

	Value& operator*() const {
		Value* value = my_value;
		if (!value)
		{
			value = my_value = (*my_container)[my_index].value();
		}
		__TBB_ASSERT( value==(*my_container)[my_index].value(), "corrupt cache" );
		return *value;
	}

	Value& operator[](ptrdiff_t k) const {
		return (*my_container)[my_index + k].value;
	}

	Value *operator->() const {return &operator*();}

	enumerable_thread_specific_iterator& operator++() {
		++my_index;
		my_value = NULL;
		return *this;
	}

	enumerable_thread_specific_iterator& operator--() {
		--my_index;
		my_value = NULL;
		return *this;
	}

	enumerable_thread_specific_iterator operator++(int) {
		enumerable_thread_specific_iterator result = *this;
		++my_index;
		my_value = NULL;
		return result;
	}

	enumerable_thread_specific_iterator operator--(int) {
		enumerable_thread_specific_iterator result = *this;
		--my_index;
		my_value = NULL;
		return result;
	}

	typedef ptrdiff_t difference_type;
	typedef Value value_type;
	typedef Value* pointer;
	typedef Value& reference;
	typedef std::random_access_iterator_tag interator_category;
};

template <typename Container, typename T>
enumerable_thread_specific_iterator<Container,T>
operator+(ptrdiff_t offset, const enumerable_thread_specific_iterator<Container,T>& v) {
	return enumerable_thread_specific_iterator<Container,T>(v.my_container, v.my_index + offset);
}

template <typename Container, typename T, typename U>
bool operator==(const enumerable_thread_specific_iterator<Container,T>& i,
	         	const enumerable_thread_specific_iterator<Container,U>& j) {
	return i.my_index == j.my_index && i.my_container == j.my_container;
}

template <typename Container, typename T, typename U>
bool operator!=(const enumerable_thread_specific_iterator<Container,T>& i,
	         	const enumerable_thread_specific_iterator<Container,U>& j) {
	return !(i==j);
}

template <typename Container, typename T, typename U>
bool operator<(const enumerable_thread_specific_iterator<Container,T>& i,
	         	const enumerable_thread_specific_iterator<Container,U>& j) {
	return i.my_index < j.my_index;
}

template <typename Container, typename T, typename U>
bool operator>(const enumerable_thread_specific_iterator<Container,T>& i,
	         	const enumerable_thread_specific_iterator<Container,U>& j) {
	return j<i;
}

template <typename Container, typename T, typename U>
bool operator>=(const enumerable_thread_specific_iterator<Container,T>& i,
	         	const enumerable_thread_specific_iterator<Container,U>& j) {
	return !(i<j);
}

template <typename Container, typename T, typename U>
bool operator<=(const enumerable_thread_specific_iterator<Container,T>& i,
	         	const enumerable_thread_specific_iterator<Container,U>& j) {
	return !(j<i);
}

template <typename Container, typename T, typename U>
ptrdiff_t operator-(const enumerable_thread_specific_iterator<Container,T>& i,
	         	const enumerable_thread_specific_iterator<Container,U>& j) {
	return i.my_index - j.my_index;
}

template <typename SegmentedContainer, typename Value>
class segmented_iterator
#if defined(_WIN64) && defined(_MSC_VER)
        : public std::iterator<std::input_iterator_tag, Value>
#endif
{
	template <typename C, typename T, typename U>
	friend bool operator == (const segmented_iterator<C,T>& i, const segmented_iterator<C,U>& j);
	template <typename C, typename T, typename U>
	friend bool operator != (const segmented_iterator<C,T>& i, const segmented_iterator<C,U>& j);
	template <typename C, typename U>
	friend class segmented_iterator;

public:
	segmented_iterator() {my_segcont = NULL;}
	segmented_iterator(const SegmentedContainer& _segmented_container) :
		my_segcont(const_cast<SegmentedContainer*>(&_segmented_container)),
		outer_iter(my_segcont->end()) {}

	typedef typename SegmentedContainer::iterator outer_iterator;
	typedef typename SegmentedContainer::value_type InnerContainer;
	typedef typename InnerContainer::iterator inner_iterator;

	typedef ptrdiff_t difference_type;
	typedef Value value_type;
	typedef typename SegmentedContainer::size_type size_type;
	typedef Value* pointer;
	typedef Value& reference;
	typedef std::input_iterator_tag iterator_category;

	template <typename U>
	segmented_iterator(const segmented_iterator<SegmentedContainer,U>& other) :
	   my_segcont(other.my_segcont),
	   outer_iter(other.outer_iter),
	   inner_iter(other.inner_iter)
	{}

	template <typename U>
	segmented_iterator& operator=(const segmented_iterator<SegmentedContainer, U>& other) {
		if (this != &other)
		{
			my_segcont = other.my_segcont;
			outer_iter = other.outer_iter;
			if (outer_iter != my_segcont->end())
				inner_iter = other.inner_iter;
		}
		return *this;
	}

	/*
	 * Allow assignment of outer iterator to segmented iterator.
	 * Once it is assigned, move forward until a non-empty inner container is found
	 * or the end of the outer container is reached
	 */
	segmented_iterator& operator=(const outer_iterator& new_outer_iter) {
		__TBB_ASSERT(my_segcont != NULL, NULL);
		for (outer_iter = new_outer_iter; outer_iter != my_segcont->end(); ++outer_iter)
		{
			if (!outer_iter->empty())
			{
				inner_iter = outer_iter->begin();
				break;
			}
		}
		return *this;
	}

	segmented_iterator& operator++()
	{
		advance_me();
		return *this;
	}

	segmented_iterator operator++(int)
	{
		segmented_iterator tmp = *this;
		operator++();
		return tmp;
	}

	bool operator==(const outer_iterator& other_outer) const {
		__TBB_ASSERT(my_segcont != NULL, NULL);
		return (outer_iter == other_outer &&
				(outer_iter == my_segcont->end() || inner_iter == outer_iter->begin()));
	}

	bool operator!=(const outer_iterator& other_outer) const {
		return !operator==(other_outer);
	}

	reference operator*() const {
		__TBB_ASSERT(my_segcont != NULL, NULL);
		__TBB_ASSERT(outer_iter != my_segcont->end(), "Dereferencing a pointer at end of container");
		__TBB_ASSERT(inner_iter != outer_iter->end(), NULL);
		return *inner_iter;
	}

	pointer operator->() const { return &operator*();}

private:
	SegmentedContainer* my_segcont;
	outer_iterator outer_iter;
	inner_iterator inner_iter;

	void advance_me() {
		__TBB_ASSERT(my_segcont != NULL, NULL);
		__TBB_ASSERT(outer_iter != my_segcont->end(), NULL);
		__TBB_ASSERT(inner_iter != outer_iter->end(), NULL);
		++inner_iter;
		while (inner_iter == outer_iter->end() && ++outer_iter != my_segcont->end()) {
			inner_iter = outer_iter->begin();
		}
	}
};

template <typename SegmentedContainer, typename T, typename U>
bool operator!=(const segmented_iterator<SegmentedContainer,T>& i,
		        const segmented_iterator<SegmentedContainer,U>& j) {
	return !(i==j);
}

template <typename T>
struct construct_by_default: tbb::internal::no_assign {
	void construct(void* where) {new(where) T();}
	construct_by_default(int) {}
};

template <typename T>
struct construct_by_exemplar: tbb::internal::no_assign {
	const T exemplar;
	void construct(void*where) {new(where) T(exemplar);}
	construct_by_exemplar(const T& t) : exemplar(t) {}
#if __TBB_ETS_USE_CPP11
            construct_by_exemplar( T&& t ) : exemplar(std::move(t)) {}
#endif
};

template <typename T, typename Finit>
struct construct_by_finit: tbb::internal::no_assign {
	Finit f;
	void construct(void* where) {new(where) T(f());}
	construct_by_finit( const Finit& f_ ) : f(f_) {}
#if __TBB_ETS_USE_CPP11
            construct_by_finit( Finit&& f_ ) : f(std::move(f_)) {}
#endif
};

#if __TBB_ETS_USE_CPP11
template <typename T, typename... P>
struct construct_by_args: tbb::internal::no_assign {
  internal::stored_pack<P...> pack;
  void construct(void* where) {
    internal::call( [where](const typename strip<P>::type&... args) {
       new(where) T(args...);
    }, pack);
  }
  construct_by_args(P&& ... args) : pack(std::forward<P>(args)...) {}
};
#endif

template <typename T>
class callback_base {
public:
	virtual callback_base* clone() const = 0;
	virtual void destroy() = 0;
	virtual ~callback_base() {}
	virtual void construct (void* where) = 0;
};

template <typename T, typename Constructor>
class callback_leaf: public callback_base<T>, Constructor {
#if __TBB_ETS_USE_CPP11
	template<typename... P> callback_leaf( P&& ... params ) : Constructor(std::forward<P>(params)...) {}
#else
	template<typename X> callback_leaf(const X& x) : Constructor(x) {}
#endif

	typedef typename tbb::tbb_allocator<callback_leaf> my_allocator_type;
	callback_base<T>* clone() const __TBB_override {
		return make(*this);
	}
	void destroy() __TBB_override {
		my_allocator_type().destroy(this);
		my_allocator_type().deallocate(this,1);
	}
	void construct(void* where) __TBB_override {
		Constructor::construct(where);
	}
public:
#if __TBB_ETS_USE_CPP11
	template<typename... P>
	            static callback_base<T>* make( P&& ... params ) {
	                void* where = my_allocator_type().allocate(1);
	                return new(where) callback_leaf( std::forward<P>(params)... );
	}
#else
	template <typename X>
	static callback_base<T>* make(const X& x) {
		void* where = my_allocator_type().allocate(1);
		return new(where) callback_leaf(x);
	}
#endif
};

/*
 * Template for recording construction of objects in table
 *
 * All maintenance of the space will be done explicitly on push_back,
 * and all thread local copies must be destroyed
 * before concurrent vector is deleted
 */
template <typename U>
struct ets_element {
	tbb::aligned_space<U> my_space;
	bool is_built;
	ets_element() {is_built = false;}
	U* value() {return my_space.begin();}
	U* value_committed() {
		is_built = true;
		return my_space.begin();
	}
	~ets_element() {
		if (is_built)
		{
			my_space.begin()->~U();
			is_built = false;
		}
	}
};

/*
 * A predicate that can be used for a compile-time compatibility check of ETS instances
 */
template <typename T, typename ETS> struct is_compatible_ets {static const bool value = false;};
template <typename T, typename U, typename A, ets_key_usage_type C>
struct is_compatible_ets <T,enumerable_thread_specific<U,A,C>> {
	static const bool value = internal::is_same_type<T,U>::value;
};


#if __TBB_ETS_USE_CPP11
        // A predicate that checks whether, for a variable 'foo' of type T, foo() is a valid expression
        template <typename T>
        class is_callable_no_args {
        private:
            typedef char yes[1];
            typedef char no [2];

            template<typename U> static yes& decide( decltype(declval<U>()())* );
            template<typename U> static no&  decide(...);
        public:
            static const bool value = (sizeof(decide<T>(NULL)) == sizeof(yes));
        };
#endif

}

/*
 * The enumerable_thread_specific container
 *
 * The properties:
 * - thread-local copies are created, with default, exemplar or function initialization
 * - thread-local copies do not move so the address of a copy is invariant
 * - thread-local copies can be managed by hash-table, or can be accessed via TLS storage for speed
 * - the contained objects need not have operator=() defined if combined is not used
 * - enumerable_thread_specific containers may be copy-constructed or assigned
 * - outside of parallel contexts, the contents of all thread-local copies are accessible by iterator
 */
template <typename T,
          typename Allocator=cache_aligned_allocator<T>,
		  ets_key_usage_type ETS_key_type=ets_no_key>
class enumerable_thread_specific: internal::ets_base<ETS_key_type> {
	template <typename U, typename A, ets_key_usage_type C> friend class enumerable_thread_specific;
	typedef internal::padded<internal::ets_element<T> > padded_element;

	template <typename I>
	class generic_range_type : public blocked_range<I> {
	public:
		typedef T value_type;
		typedef T& reference;
		typedef const T& const_reference;
		typedef I iterator;
		typedef ptrdiff_t difference_type;
		generic_range_type(I begin_, I end_, size_t grainsize_ = 1) : blocked_range<I>(begin_,end_, grainsize_) {}
		template <typename U>
		generic_range_type(const generic_range_type<U>& r) : blocked_range<I>(r.begin(), r.end(), r.gransize()) {}
		generic_range_type(generic_range_type& r, split) : blocked_range<I>(r,split()) {}
	};

	typedef typename Allocator::template rebind<padded_element>::other padded_allocator_type;
	typedef tbb::concurrent_vector<padded_element,padded_allocator_type> internal_collection_type;

	internal::callback_base<T> *my_construct_callback;
	internal_collection_type my_locals;

	void* create_local() __TBB_override {
		padded_element& lref = *my_locals.grow_by(1);
		my_construct_callback->construct(lref.value());
		return lref.value_committed();
	}

	static void* create_local_by_copy(internal::ets_base<ets_no_key>& base, void* p) {
		enumerable_thread_specific& ets = static_cast<enumerable_thread_specific&>(base);
		padded_element& lref = *ets.my_count.grow_by(1);
		new(lref.value()) T(*static_cast<T*>(p));
		return lref.value_committed();
	}

#if __TBB_ETS_USE_CPP11
        static void* create_local_by_move( internal::ets_base<ets_no_key>& base, void* p ) {
            enumerable_thread_specific& ets = static_cast<enumerable_thread_specific&>(base);
            padded_element& lref = *ets.my_locals.grow_by(1);
            new(lref.value()) T(std::move(*static_cast<T*>(p)));
            return lref.value_committed();
        }
#endif

typedef typename Allocator::template rebind<uinptr_t>::other array_allocator_type;

void* create_array(size_t size) __TBB_override {
	size_t nelements = (_size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
	return array_allocator_type().allocate(elements);
}

void free_array(void* _ptr, size_t _size) __TBB_override {
	size_t nelements = (_size + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);
	array_allocator_type().deallocate(reinterpret_cast<uintptr_t*>(_ptr),nelements);
}



};



}

}

#endif /* INCLUDE_TBB_ENUMERABLE_THREAD_SPECIFIC_H_ */
