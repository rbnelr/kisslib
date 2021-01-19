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
}
