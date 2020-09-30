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
	template <typename T> inline constexpr T round_up_pot (T x, T y) {
		return (x + y - 1) & ~(y - 1);
	}
	// check if power of two
	template <typename T> inline constexpr T is_pot (T x) {
		return (x & (x - 1)) == 0;
	}

	inline size_t hash (int3 v) {
		size_t h;
		h  = ::std::hash<int>()(v.x);
		h = 53ull * (h + 53ull);
		h += ::std::hash<int>()(v.y);
		h = 53ull * (h + 53ull);
		h += ::std::hash<int>()(v.z);
		return h;
	};
	inline size_t hash (int4 v) {
		size_t h;
		h  = ::std::hash<int>()(v.x);
		h = 53ull * (h + 53ull);
		h += ::std::hash<int>()(v.y);
		h = 53ull * (h + 53ull);
		h += ::std::hash<int>()(v.z);
		h = 53ull * (h + 53ull);
		h += ::std::hash<int>()(v.w);
		return h;
	};

	// Hashmap key type for vectors
	template <typename VEC>
	struct vector_key {
		VEC v;

		vector_key (VEC const& v): v{v} {}
		bool operator== (vector_key<VEC> const& r) const {
			return equal(v, r.v);
		}
	};
}

namespace std {
	template <typename VEC>
	struct hash<kissmath::vector_key<VEC>> {
		size_t operator() (kissmath::vector_key<VEC> const& v) const {
			return kissmath::hash(v.v);
		}
	};
}

using namespace kissmath;

