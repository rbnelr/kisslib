#pragma once
#include <vector>
#include <string>
#include <unordered_map>

// Tracy tracked stl containers
#ifdef TRACY_ENABLE
	#include "Tracy.hpp"

	#define STL_PROFILE_SCOPED(name) ZoneScopedNC(name, tracy::Color::Crimson)
	#define STL_PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
	#define STL_PROFILE_FREE(ptr) TracyFree(ptr)

	inline void* _malloc (std::size_t size) {
		STL_PROFILE_SCOPED("malloc");
		return std::malloc(size);
	}
	inline void _free (void* ptr) {
		STL_PROFILE_SCOPED("free");
		std::free(ptr);
	}

	template <typename T>
	struct TracySTLAllocator {
		typedef T value_type;

		// Not sure why these ctors are needed
		TracySTLAllocator () noexcept {};
		template<typename U>
		TracySTLAllocator (const TracySTLAllocator<U>& other) throw() {};

		T* allocate (std::size_t n) {
			T* ptr = (T*)_malloc(n * sizeof(T));
			STL_PROFILE_ALLOC(ptr, n * sizeof(T));
			return ptr;
		}
		void deallocate (T* ptr, std::size_t n) {
			STL_PROFILE_FREE(ptr);
			_free(ptr);
		}
	};
	// Not sure why these are needed
	template <typename T, typename U>
	inline bool operator == (const TracySTLAllocator<T>&, const TracySTLAllocator<U>&) { return true; }
	template <typename T, typename U>
	inline bool operator != (const TracySTLAllocator<T>& a, const TracySTLAllocator<U>& b) { return !(a == b); }

	template <typename T>
	using std_vector = std::vector<T, TracySTLAllocator<T>>;

	using std_string = std::basic_string<char, std::char_traits<char>, TracySTLAllocator<char>>;

	template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
	using std_unordered_map = std::unordered_map<Key, T, Hash, Pred, TracySTLAllocator<std::pair<const Key, T>>>;
#else
	template <typename T>
	using std_vector = std::vector<T>;

	using std_string = std::basic_string<char, std::char_traits<char>>;

	template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
	using std_unordered_map = std::unordered_map<Key, T, Hash, Pred>;
#endif

namespace kiss {
	// typename Alloc to handle overloaded allocators

	// returns lowest index where are_equal(vec[i], r) returns true or -1 if none are found
	// bool are_equal(VT const& l, T const& r)
	template <typename VT, typename T, typename EQUAL, typename Alloc>
	inline int indexof (std::vector<VT, Alloc>& vec, T& r, EQUAL are_equal) {
		for (int i=0; i<(int)vec.size(); ++i)
			if (are_equal(vec[i], r))
				return i;
		return -1;
	}

	// returns lowest index where (vec[i] == r) returns true or -1 if none are found
	template <typename VT, typename T, typename Alloc>
	inline int indexof (std::vector<VT, Alloc>& vec, T& r) {
		for (int i=0; i<(int)vec.size(); ++i)
			if (vec[i] == r)
				return i;
		return -1;
	}

	// returns true if any are_equal(vec[i], r) returns true
	// bool are_equal(VT const& l, T const& r)
	template <typename VT, typename T, typename EQUAL, typename Alloc>
	inline bool contains (std::vector<VT, Alloc>& vec, T const& r, EQUAL are_equal) {
		for (auto& i : vec)
			if (are_equal(i, r))
				return true;
		return false;
	}
	
	// returns true if any (vec[i] == r) returns true
	template <typename VT, typename T, typename Alloc>
	inline bool contains (std::vector<VT, Alloc>& vec, T const& r) {
		for (auto& i : vec)
			if (i == r)
				return true;
		return false;
	}

	// like unordered_map but keeps the order of the elements for iteration
	// implemented as a vector with a unordered_map for constant time lookups
	// consider if element removal is needed, since it is O(N) because of the use of a vector of elements
	template <typename KEY, typename VAL>
	struct ordered_map {
		struct KeyValue {
			const KEY key;
			VAL value;

			KeyValue (KEY const& key, VAL const& value): key{key}, value{value} {}
		};

		std::vector<KeyValue> elements;
		std::unordered_map<KEY, int> indexmap;

		// get index of element by key or -1 if key not found
		int indexof (KEY const& key) const {
			auto res = indexmap.find(key);
			if (res == indexmap.end())
				return -1;
			return (int)res->second;
		}

		// get ref to element by index, ignore out of range indices
		VAL& byindex (int index) {
			return elements[index];
		}

		// get ref to element by index, ignore out of range indices
		VAL const& byindex (int index) const {
			return elements[index];
		}

		// get ptr to element by key, return nullptr if key not found
		VAL* bykey (KEY const& key) {
			auto res = indexmap.find(key);
			if (res == indexmap.end())
				return nullptr;
			return &elements[res.second];
		}

		// get ptr to element by key, return nullptr if key not found
		VAL const* bykey (KEY const& key) const {
			auto res = indexmap.find(key);
			if (res == indexmap.end())
				return nullptr;
			return &elements[res.second];
		}

		// get ref to existing element or to newly default constructed element
		VAL& operator[] (KEY const& key) {
			auto res = indexmap.find(key);
			if (res == indexmap.end()) {
				// key not found, default construct a value
				indexmap.insert(key, (int)elements.size());
				elements.emplace_back(key, VAL());
				return elements.back();
			} else {
				return elements[res.second];
			}
		}

		// insert value if key is not yet in set
		// returns true if insertion happened
		// optionally returns index if inserted element
		bool insert (KEY const& key, VAL const& val, int* out_index = nullptr) {
			auto res = indexmap.emplace(key, (int)elements.size());
			if (out_index) *out_index = (int)elements.size();
			elements.emplace_back(key, val);
			return res.second;
		}

		// insert or replace value with key
		// returns true if key was new and insertion happened, false if value was replaced
		// optionally returns index if inserted element
		bool replace (KEY const& key, VAL const& val, int* out_index = nullptr) {
			auto res = indexmap.emplace(key, (int)elements.size());
			if (!res.second) {
				// existing key, replace value
				if (out_index) *out_index = res.first->second;
				elements[res.first->second] = val;
				return false;
			} else {
				// new key, add value
				if (out_index) *out_index = (int)elements.size();
				elements.emplace_back(key, val);
				return true;
			}
		}

	#if 0 // TODO: implement the element shuffling
		// remove element if it exists
		// NOTE: O(N) because elements have to be moved to moved to close gap in array
		// consider if order is actually needed
		// returns true if element was removed (key existed)
		bool remove (KEY const& key, int* out_index = nullptr) {
			auto res = indexmap.find(key);
			if (res == indexmap.end()) {
				// no such key found
				return false;
			}

			if (out_index) *out_index = res->second;
			indexmap.erase(res);
			return true;
		}
	#endif

		void clear () {
			indexmap.clear();
			elements.clear();
		}
	};
}
