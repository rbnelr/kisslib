#pragma once
#include "kissmath.hpp"

struct Ray {
	float3 pos;
	float3 dir; // normalized
};
struct Plane {
	float3 pos;
	float3 normal;
};
struct AABB {
	float3 lo;
	float3 hi;
};

struct View_Frustrum {
	// The view frustrum planes in world space
	// in the order: near, left, right, bottom, up, far
	// far was put last because it is not really needed in view frustrum culling since it is undesirable for the far plane to intersect any geometry anyway, so it usually gets get to far enough to never cull anything
	Plane		planes[6];

	// Frustrum corners in world space
	// in the order: LBN, RBN, RTN, LTN, LBF, RBF, RTF, LTF
	// (L=left R=right B=bottom T=top N=near F=far)
	float3		corners[8];
};

// intersection test between circle and 1x1 square going from 0,0 to 1,1
bool circle_square_intersect (float2 circ_origin, float circ_radius);

// intersection test between cylinder and 1x1x1 cube going from 0,0,0 to 1,1,1
// cylinder origin is at the center of the circle at the base of the cylinder (-z circle)
bool cylinder_cube_intersect (float3 cyl_origin, float cyl_radius, float cyl_height);

// nearest distance from point to square (square covers [square_pos, square_pos +square_size] on each axis)
float point_square_nearest_dist (float2 square_pos, float2 square_size, float2 point);

// nearest distance from point to box (box covers [box_pos, box_pos + box_size] on each axis)
float point_box_nearest_dist_sqr (float3 box_pos, float3 box_size, float3 point);
// nearest distance from point to box (box covers [box_pos, box_pos + box_size] on each axis)
float point_box_nearest_dist (float3 box_pos, float3 box_size, float3 point);

// cull (return true) if AABB is completely outside of one of the view frustrums planes
// this cull 99% of the AABB that are invisible, but returns a false negative sometimes
bool frustrum_cull_aabb (View_Frustrum const& frust, float lx, float ly, float lz, float hx, float hy, float hz);

inline bool frustrum_cull_aabb (View_Frustrum const& frust, AABB aabb) {
	return frustrum_cull_aabb(frust, aabb.lo.x, aabb.lo.y, aabb.lo.z, aabb.hi.x, aabb.hi.y, aabb.hi.z);
}

struct CollisionHit {
	float dist; // how far obj A moved relative to obj B to hit it
	float3 pos; // pos of obj A relative to obj B at collision time
	float3 normal; // normal of the collision pointing from obj B to obj A
};

// Calculate first collision CollisionHit between axis aligned unit cube and a axis aligned cylinder (cylinder axis is z)
//  offset = cylinder.pos - cube.pos  (cylinder.pos is it's center)
//  dir    = cylinder.vel - cube.vel  (does not need to be normalized)
//  cyl_r  = cylinder.radius
//  cyl_h  = cylinder.height
// coll gets written to if calculated dist < coll->dist (init coll->dist to INF intially)
void cylinder_cube_cast (float3 offset, float3 dir, float cyl_r, float cyl_h, CollisionHit* coll);
