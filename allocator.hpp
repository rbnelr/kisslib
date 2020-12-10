#pragma once
#include "stdint.h"
#include "macros.hpp"
#include "assert.h"
#include <vector>
#include "stl_extensions.hpp"

/*
	Allocators implemented using OS-level virtual memory
*/

// a macro for code that does manual memory management to aid debugging
// called on regions of memory in debug mode to give some clues as to the state of the memory
#define DBG_MEMSET_UNINITED 0xCC
#define DBG_MEMSET_FREED 0xDD

#ifdef NDEBUG
	#define DBG_MEMSET
#else
	#define DBG_MEMSET memset
#endif

// return null on fail or simply return out of bound (unchecked) ptr for less branching
// (1) -> return null on overflow  (0) -> return unchecked ptr
#ifndef ALLOCATOR_NULLFAIL
	#define ALLOCATOR_NULLFAIL 1
#endif

#ifdef TRACY_ENABLE
	#include "Tracy.hpp"

	#define ALLOCATOR_PROFILE_SCOPED(name) ZoneScopedNC(name, tracy::Color::Crimson)
	#define ALLOCATOR_PROFILE_ALLOC(ptr, size) TracyAlloc(ptr, size)
	#define ALLOCATOR_PROFILE_FREE(ptr) TracyFree(ptr)
#else
	#define ALLOCATOR_PROFILE_SCOPED(name)
	#define ALLOCATOR_PROFILE_ALLOC(ptr, size)
	#define ALLOCATOR_PROFILE_FREE(ptr)
#endif

uint32_t get_os_page_size ();

inline const int os_page_size = get_os_page_size();

void* reserve_address_space (size_t size);
void release_address_space (void* baseptr, size_t size);
void commit_pages (void* ptr, size_t size);
void decommit_pages (void* ptr, size_t size);

// like std::vector but with a reserved (contigous) max size so that no reallocation is ever needed
class VirtualPushAllocator {
	char* baseptr; // base address of reserved memory pages
	char* allocptr; // [top] one past the last allocated item (next ptr to be allocated)
	char* commitptr; // [baseptr, commitptr) is the commited memory region
	char* reserveptr; // end of reserved memory, illegal to grow beyond this
public:

	inline VirtualPushAllocator (size_t max_size) {
		baseptr = (char*)reserve_address_space(max_size);
		allocptr = baseptr;
		commitptr = baseptr;
		reserveptr = baseptr + max_size;
	}

	inline ~VirtualPushAllocator () {
		release_address_space(baseptr, reserveptr - baseptr);
	}

	inline char* top () {
		return allocptr;
	}
	inline size_t size () {
		return allocptr - baseptr;
	}
	
	// Allocate [size] bytes from the top by 
	inline char* push (size_t size, size_t align=1) {
		char* ptr = allocptr;
		
		// align ptr to desired [align], which is assumed to be a power of two
		ptr = (char*)( ((uintptr_t)ptr + (align-1)) & ~(align-1) );

		// allocate desired [size] bytes
		ptr += size;

		// assert on overflow
		assert(ptr <= reserveptr);
		
	#if ALLOCATOR_NULLFAIL == 1
		if (ptr > reserveptr)
			return nullptr;
	#endif

		if (ptr > commitptr) // at least one page to be committed
			_grow(ptr);

		DBG_MEMSET(allocptr, DBG_MEMSET_UNINITED, ptr - allocptr);

		return ptr;
	}

	// Reset allocator to a previous top ptr than the current top
	void reset (char* ptr) {
		assert(ptr >= baseptr && ptr <= allocptr);

		DBG_MEMSET(ptr, DBG_MEMSET_FREED, allocptr - ptr);

		if (ptr <= commitptr - os_page_size) // at least 1 page to be decommitted
			_shrink(ptr);

		allocptr = ptr;
	}
	
	// called when at least one new page need to be commited
	void _grow (char* ptr) {
		assert(ptr > commitptr);
		
		// get new page aligned commit ptr
		ptr = (char*)( ((uintptr_t)ptr + (os_page_size-1)) & ~(os_page_size-1) );

		commit_pages(commitptr, ptr - commitptr);
		commitptr = ptr;
	}

	// called when at least one page need to be decommited
	void _shrink (char* ptr) {
		assert(ptr < commitptr);

		// get new page aligned commit ptr
		ptr = (char*)( ((uintptr_t)ptr + (os_page_size-1)) & ~(os_page_size-1) );

		decommit_pages(ptr, commitptr - ptr);
		commitptr = ptr;
	}
};

// An acceleration structure to speed up finding the first free slot in structures like arrays where items can be freed
// esentially checks 64 slots at once using tiny uint64_t loops followed by a single bitscan instruction
// keeps track of the index of the first free and (one past) the index of the last allocated slot which allows growing and shrinking structures to easily know when to resize their memory

#define ONES (~0ull) //0xffffffffffffffffull

uint32_t _bsf_1 (uint64_t val);
uint32_t _bsr_0 (uint64_t val);

// get index of first free (1) bit, starting at some point in the array
// returns one past end of array if no free (1) bit found, because that one needs to be the next one allocated
inline uint32_t scan_forward_free (uint64_t* bits, uint32_t count, uint32_t start) {
	uint32_t i = start;
	while (i < count) {
		if (bits[i] != 0ull)
			return (i << 6) + _bsf_1(bits[i]);
		++i;
	}
	return count << 6;
}

// get index of last allocated (0) bit plus 1
// returns 0 if no 0 bits are found, because all bits can be freed
inline uint32_t scan_reverse_allocated (uint64_t* bits, uint32_t start) {
	uint32_t i = start;
	for (;;) {
		if (bits[i] != ONES)
			return (i << 6) + _bsr_0(bits[i]) + 1;
		if (i == 0) break;
		--i;
	}
	return 0;
}

struct AllocatorBitset {
	std_vector<uint64_t>	bits;
	uint32_t				first_free = 0; // index of first free (1) bit in bits, to speed up alloc
	uint32_t				alloc_end = 0; // index of the free region of 1 bits starting after the last allocated (0) bit, to speed up paging for users
	uint32_t				count = 0; // index of the free region of 1 bits starting after the last allocated (0) bit, to speed up paging for users

	// finds the first free (1) bit and sets it to 0, returns the index of the slot
	uint32_t alloc () {
		// alloc at the cached first_free index
		uint32_t idx = first_free;

		// append to bits if needed
		if (idx == ((uint32_t)bits.size() << 6))
			_grow();

		// clear bit
		assert(bits[idx >> 6] & (1ull << (idx & 0b111111u)));
		bits[idx >> 6] &= ~(1ull << (idx & 0b111111u));

		// update first_free by scanning to right for next free bit, skips bits before current first_free
		first_free = scan_forward_free(bits.data(), (uint32_t)bits.size(), first_free >> 6);

		// update alloc_end
		alloc_end = std::max(idx+1, alloc_end);

		return idx;
	}

	void _grow () {
		bits.push_back(ONES);
	}

	// free an allocated (0) bit by setting it to 1
	// it's safe to set a free a slot that's already freed
	void free (uint32_t idx) {
		assert(alloc_end > 0);

		// set bit in freeset to 1
		bits[idx >> 6] |= 1ull << (idx & 0b111111u);

		// only rescan for alloc_end (and potentially shrink bit array) if the last allocated bit was freed
		if (idx >= alloc_end-1)
			_shrink(idx);

		// update first_set
		first_free = std::min(first_free, idx);
	}

	void _shrink (uint32_t idx) {
		assert(idx == alloc_end-1);
		// update alloc_end by scanning to left for next allocated bit, skips bits after current first_free
		alloc_end = scan_reverse_allocated(bits.data(), (alloc_end-1) >> 6);

		// shrink bits if there are contiguous zero ints at the end
		uint32_t needed_bits = ((alloc_end-1) >> 6) + 1; 
		if (needed_bits < (uint32_t)bits.size())
			bits.resize(needed_bits);
	}
};
