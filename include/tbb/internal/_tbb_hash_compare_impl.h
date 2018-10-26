/*
 * _tbb_hash_compare_impl.h
 *
 *  Created on: Oct 26, 2018
 *      Student (MIG Virtual Developer): Tung Dang
 */

#ifndef INCLUDE_TBB_INTERNAL__TBB_HASH_COMPARE_IMPL_H_
#define INCLUDE_TBB_INTERNAL__TBB_HASH_COMPARE_IMPL_H_

#include <string>
namespace tbb {
namespace interface5 {
namespace internal {

template<typename Key, typename Hasher, typename Key_equality>
class hash_compare
{
public:
	typedef Hasher hasher;
	typedef Key_equality key_equal;

	Hasher my_hash_object;
	Key_equality my_key_compare_object;

	hash_compare() {}
	hash_compare(Hasher a_hasher) : my_hash_object(a_hasher) {}
	hash_compare(Hasher a_hasher, Key_equality a_keyeq) : my_hash_object(a_hasher), my_key_compare_object(a_keyeq) {}

	size_t operator() (const Key& key) const {
		return ((size_t)my_hash_object(key));
	}

	bool operator() (const Key& key1, const Key& key2) const {
		return (!my_key_compare_object(key1,key2));
	}

};

static const size_t hash_multiplier = tbb::internal::select_size_t_constant<2654435769U, 11400714819323198485ULL>::value;

}

template <typename T>
inline size_t tbb_hasher(const T& t)
{
	return static_cast<size_t>(t) + internal::hash_multiplier;
}

template <typename P>
inline size_t tbb_hasher(P* ptr)
{
	size_t const h = reinterpret_cast<size_t>(ptr);
	return (h >> 3) ^ h;
}

template <typename E, typename S, typename A>
inline size_t tbb_hasher(const std::basic_string<E,S,A>& s)
{
	size_t h = 0;
	for (const E* c = s.c_str(); *c; c++)
	{
		h = static_cast<size_t>(*c) ^ (h * internal::hash_multiplier);
	}
	return h;
}

template <typename F, typename S>
inline size_t tbb_hasher(const std::pair<F,S>& p)
{
	return tbb_hasher(p.first) ^ tbb_hasher(p.second);
}
}

using interface5::tbb_hasher;
template <typename Key>
class tbb_hash
{
public:
	tbb_hash() {}
	size_t operator()(const Key& key) const
	{
		return tbb_hasher(key);
	}
};

template <typename Key>
struct tbb_hash_compare {
	static size_t hash (const Key& key) {return tbb_hasher(key);}
	static bool equal(const Key& key1, const Key& key2) {return key1 == key2;}
};

}
#endif /* INCLUDE_TBB_INTERNAL__TBB_HASH_COMPARE_IMPL_H_ */
