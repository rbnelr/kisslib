#include "allocator.hpp"
#include "clean_windows_h.hpp"

uint32_t get_os_page_size () {
	SYSTEM_INFO info;
	GetSystemInfo(&info);

	return (uint32_t)info.dwPageSize;
}

void* reserve_address_space (uintptr_t size) {
	void* baseptr = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
	assert(baseptr != nullptr);
	return baseptr;
}

void release_address_space (void* baseptr, uintptr_t size) {
	auto ret = VirtualFree(baseptr, 0, MEM_RELEASE);
	assert(ret != 0);
}

void commit_pages (void* ptr, uintptr_t size) {
	auto ret = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
	assert(ret != NULL);

#if DBG_MEMSET
	memset(ptr, DBG_MEMSET_VAL, size);
#endif
}

void decommit_pages (void* ptr, uintptr_t size) {
	auto ret = VirtualFree(ptr, size, MEM_DECOMMIT);
	assert(ret != 0);
}

#define ONES (~0ull) //0xffffffffffffffffull

uint32_t _bsf_1 (uint64_t val) {
	unsigned long idx;
	auto ret = _BitScanForward64(&idx, val);
	assert(ret);
	return idx;
}
uint32_t _bsr_0 (uint64_t val) {
	unsigned long idx;
	auto ret = _BitScanReverse64(&idx, ~val);
	assert(ret);
	return idx;
}

// get index of first free (1) bit, starting at some point in the array
// returns one past end of array if no free (1) bit found, because that one needs to be the next one allocated
uint32_t scan_forward_free (uint64_t* bits, uint32_t count, uint32_t start) {
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
uint32_t scan_reverse_allocated (uint64_t* bits, uint32_t start) {
	uint32_t i = start;
	for (;;) {
		if (bits[i] != ONES)
			return (i << 6) + _bsr_0(bits[i]) + 1;
		if (i == 0) break;
		--i;
	}
	return 0;
}

uint32_t BitsetAllocator::alloc () {
	// alloc at the cached first_free index
	uint32_t idx = first_free;

	// append to bits if needed
	if (idx == ((uint32_t)bits.size() << 6))
		bits.push_back(ONES);

	// clear bit
	assert(bits[idx >> 6] & (1ull << (idx & 0b111111u)));
	bits[idx >> 6] &= ~(1ull << (idx & 0b111111u));

	// update first_free by scanning to right for next free bit, skips bits before current first_free
	first_free = scan_forward_free(bits.data(), (uint32_t)bits.size(), first_free >> 6);

	// update alloc_end
	alloc_end = std::max(idx+1, alloc_end);

	return idx;
}
void BitsetAllocator::free (uint32_t idx) {
	assert(alloc_end > 0);

	// set bit in freeset to 1
	bits[idx >> 6] |= 1ull << (idx & 0b111111u);
	
	if (idx >= alloc_end-1) {
		assert(idx == alloc_end-1);
		// update alloc_end by scanning to left for next allocated bit, skips bits after current first_free
		alloc_end = scan_reverse_allocated(bits.data(), (alloc_end-1) >> 6);
		
		// shrink bits if there are contiguous zero ints at the end
		uint32_t needed_bits = alloc_end >> 6; 
		if (needed_bits < (uint32_t)bits.size())
			bits.resize(needed_bits);
	}

	// update first_set
	first_free = std::min(first_free, idx);
}
