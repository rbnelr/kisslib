#pragma once

#include "kissmath/output/bool2.hpp"
#include "kissmath/output/bool3.hpp"
#include "kissmath/output/bool4.hpp"

#include "kissmath/output/int2.hpp"
#include "kissmath/output/int3.hpp"
#include "kissmath/output/int4.hpp"

#include "kissmath/output/int64v2.hpp"
#include "kissmath/output/int64v3.hpp"
#include "kissmath/output/int64v4.hpp"

#include "kissmath/output/uint8v2.hpp"
#include "kissmath/output/uint8v3.hpp"
#include "kissmath/output/uint8v4.hpp"

#include "kissmath/output/float2.hpp"
#include "kissmath/output/float3.hpp"
#include "kissmath/output/float4.hpp"

#include "kissmath/output/float2x2.hpp"
#include "kissmath/output/float3x3.hpp"
#include "kissmath/output/float4x4.hpp"
#include "kissmath/output/float3x4.hpp"

#include "kissmath/output/transform2d.hpp"
#include "kissmath/output/transform3d.hpp"

#include "kissmath_colors.hpp"

#include <type_traits>

namespace kissmath {
	// round up x to y, assume y is power of two
	template <typename T> inline constexpr T align_up (T x, T y) {
		return (x + y - 1) & ~(y - 1);
	}
	// check if power of two
	template <typename T> inline constexpr T is_pot (T x) {
		return (x & (x - 1)) == 0;
	}
	
	// from https://github.com/aappleby/smhasher/blob/master/src/MurmurHash1.cpp
	inline uint32_t MurmurHash1_32 (uint32_t const* key, int len) {
		constexpr uint32_t m = 0xc6a4a793;

		//uint32_t h = seed ^ (len * m);
		uint32_t h = 0;

		for (int i=0; i<len; ++i) {
			h += key[i];
			h *= m;
			h ^= h >> 16;
		}

		h *= m;
		h ^= h >> 10;
		h *= m;
		h ^= h >> 17;

		return h;
	}
	inline uint32_t MurmurHash1_8 (uint8_t const* key, int len) {
		constexpr uint32_t m = 0xc6a4a793;

		//uint32_t h = seed ^ (len * m);
		uint32_t h = 0;

		for (int i=0; i<len; i += 4) {
			h += *(uint32_t*)(key + i);
			h *= m;
			h ^= h >> 16;
		}

		switch(len) {
			case 3:
				h += (uint32_t)key[2] << 16;
			case 2:
				h += (uint32_t)key[1] << 8;
			case 1:
				h += (uint32_t)key[0];
				h *= m;
				h ^= h >> 16;
		};

		h *= m;
		h ^= h >> 10;
		h *= m;
		h ^= h >> 17;

		return h;
	} 

	inline size_t hash (int2 const& v) { return MurmurHash1_32((uint32_t*)&v.x, 2); };
	inline size_t hash (int3 const& v) { return MurmurHash1_32((uint32_t*)&v.x, 3); };
	inline size_t hash (int4 const& v) { return MurmurHash1_32((uint32_t*)&v.x, 4); };

	inline size_t hash (float2 const& v) { return MurmurHash1_32((uint32_t*)&v.x, 2); };
	inline size_t hash (float3 const& v) { return MurmurHash1_32((uint32_t*)&v.x, 3); };
	inline size_t hash (float4 const& v) { return MurmurHash1_32((uint32_t*)&v.x, 4); };

	inline size_t hash (uint8v2 const& v) { return MurmurHash1_8(&v.x, 2); };
	inline size_t hash (uint8v3 const& v) { return MurmurHash1_8(&v.x, 3); };
	inline size_t hash (uint8v4 const& v) { return MurmurHash1_8(&v.x, 4); };
}

namespace std {
	template<> struct hash<kissmath::int2   > { size_t operator() (kissmath::int2    const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::int3   > { size_t operator() (kissmath::int3    const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::int4   > { size_t operator() (kissmath::int4    const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::float2 > { size_t operator() (kissmath::float2  const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::float3 > { size_t operator() (kissmath::float3  const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::float4 > { size_t operator() (kissmath::float4  const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::uint8v2> { size_t operator() (kissmath::uint8v2 const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::uint8v3> { size_t operator() (kissmath::uint8v3 const& x) const { return kissmath::hash(x); } };
	template<> struct hash<kissmath::uint8v4> { size_t operator() (kissmath::uint8v4 const& x) const { return kissmath::hash(x); } };
}

using namespace kissmath;
