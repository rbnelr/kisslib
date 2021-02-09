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

	static constexpr uint64_t KB = 1024ull;
	static constexpr uint64_t MB = 1024ull*1024;
	static constexpr uint64_t GB = 1024ull*1024*1024;
	static constexpr uint64_t TB = 1024ull*1024*1024*1024;

	//// Getting scalar type + vector dimension from type

	template <typename T> inline constexpr bool is_matrix () { return false; }

	template<> inline constexpr bool is_matrix<float2x2> () { return true; }
	template<> inline constexpr bool is_matrix<float3x3> () { return true; }
	template<> inline constexpr bool is_matrix<float4x4> () { return true; }
	template<> inline constexpr bool is_matrix<float3x4> () { return true; }

	// Include all eventualities
	enum class ScalarType {
		FLOAT,	DOUBLE,	FLOAT8, FLOAT16,
		INT,	UINT,
		INT8,	UINT8,
		INT64,	UINT64,
		INT16,	UINT16,
		BOOL
	};
	struct VectorTypeInfo {
		ScalarType type;
		int components;
	};

	template <typename T> inline constexpr VectorTypeInfo get_type ();

	template<> inline constexpr VectorTypeInfo get_type<float   > () { return { ScalarType::FLOAT, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<float2  > () { return { ScalarType::FLOAT, 2 }; }
	template<> inline constexpr VectorTypeInfo get_type<float3  > () { return { ScalarType::FLOAT, 3 }; }
	template<> inline constexpr VectorTypeInfo get_type<float4  > () { return { ScalarType::FLOAT, 4 }; }
	template<> inline constexpr VectorTypeInfo get_type<int     > () { return { ScalarType::INT, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<int2    > () { return { ScalarType::INT, 2 }; }
	template<> inline constexpr VectorTypeInfo get_type<int3    > () { return { ScalarType::INT, 3 }; }
	template<> inline constexpr VectorTypeInfo get_type<int4    > () { return { ScalarType::INT, 4 }; }
	template<> inline constexpr VectorTypeInfo get_type<int64_t > () { return { ScalarType::INT64, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<int64v2 > () { return { ScalarType::INT64, 2 }; }
	template<> inline constexpr VectorTypeInfo get_type<int64v3 > () { return { ScalarType::INT64, 3 }; }
	template<> inline constexpr VectorTypeInfo get_type<int64v4 > () { return { ScalarType::INT64, 4 }; }
	template<> inline constexpr VectorTypeInfo get_type<uint8_t > () { return { ScalarType::UINT8, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<uint8v2 > () { return { ScalarType::UINT8, 2 }; }
	template<> inline constexpr VectorTypeInfo get_type<uint8v3 > () { return { ScalarType::UINT8, 3 }; }
	template<> inline constexpr VectorTypeInfo get_type<uint8v4 > () { return { ScalarType::UINT8, 4 }; }
	template<> inline constexpr VectorTypeInfo get_type<bool    > () { return { ScalarType::BOOL, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<bool2   > () { return { ScalarType::BOOL, 2 }; }
	template<> inline constexpr VectorTypeInfo get_type<bool3   > () { return { ScalarType::BOOL, 3 }; }
	template<> inline constexpr VectorTypeInfo get_type<bool4   > () { return { ScalarType::BOOL, 4 }; }

	template<> inline constexpr VectorTypeInfo get_type<int8_t  > () { return { ScalarType::INT8, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<int16_t > () { return { ScalarType::INT16, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<uint16_t> () { return { ScalarType::UINT16, 1 }; }
	template<> inline constexpr VectorTypeInfo get_type<uint64_t> () { return { ScalarType::UINT64, 1 }; }

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
