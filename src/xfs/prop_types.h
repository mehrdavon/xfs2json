#ifndef PROP_TYPES_H
#define PROP_TYPES_H

#include <stdint.h>

typedef struct xfs_point {
    int32_t x;
    int32_t y;
} xfs_point;

typedef struct xfs_size {
    int32_t w;
    int32_t h;
} xfs_size;

typedef struct xfs_rect {
    int32_t l;
    int32_t t;
    int32_t r;
    int32_t b;
} xfs_rect;

typedef struct xfs_matrix {
    float m[4][4];
} xfs_matrix;

typedef struct xfs_vector3 {
    float x;
    float y;
    float z;
    float _pad;
} xfs_vector3;

typedef struct xfs_vector4 {
    float x;
    float y;
    float z;
    float w;
} xfs_vector4;

typedef struct xfs_quaternion {
    float x;
    float y;
    float z;
    float w;
} xfs_quaternion;

typedef struct xfs_time {
    int64_t time;
} xfs_time;

typedef struct xfs_float2 {
    float x;
    float y;
} xfs_float2;

typedef struct xfs_float3 {
    float x;
    float y;
    float z;
} xfs_float3;

typedef struct xfs_float4 {
    float x;
    float y;
    float z;
    float w;
} xfs_float4;

typedef uint32_t xfs_color;

typedef struct xfs_float3x3 {
    float m[3][3];
} xfs_float3x3;

typedef struct xfs_float4x3 {
    float m[4][3];
} xfs_float4x3;

typedef struct xfs_float4x4 {
    float m[4][4];
} xfs_float4x4;

typedef struct xfs_easecurve {
    float p1;
    float p2;
} xfs_easecurve;

typedef struct xfs_line {
    xfs_vector3 from;
    xfs_vector3 dir;
} xfs_line;

typedef struct xfs_linesegment {
    xfs_vector3 p0;
    xfs_vector3 p1;
} xfs_linesegment;

typedef struct xfs_ray {
    xfs_vector3 from;
    xfs_vector3 dir;
} xfs_ray;

typedef struct xfs_plane {
    xfs_float3 normal;
    float dist;
} xfs_plane;

typedef struct xfs_sphere {
    xfs_float3 center;
    float radius;
} xfs_sphere;

typedef struct xfs_capsule {
    xfs_vector3 p0;
    xfs_vector3 p1;
    float radius;
    float _pad[3];
} xfs_capsule;

typedef struct xfs_aabb {
    xfs_vector3 min;
    xfs_vector3 max;
} xfs_aabb;

typedef struct xfs_obb {
    xfs_matrix transform;
    xfs_vector3 extent;
} xfs_obb;

typedef xfs_capsule xfs_cylinder;

typedef struct xfs_triangle {
    xfs_vector3 p0;
    xfs_vector3 p1;
    xfs_vector3 p2;
} xfs_triangle;

typedef struct xfs_cone {
    xfs_float3 p0;
    float r0;
    xfs_float3 p1;
    float r1;
} xfs_cone;

typedef struct xfs_torus {
    xfs_vector3 pos;
    float r;
    xfs_vector3 axis;
    float cr;
} xfs_torus;

typedef struct xfs_ellipsoid {
    xfs_vector3 pos;
    xfs_vector3 r;
} xfs_ellipsoid;

typedef struct xfs_range {
    int32_t s;
    uint32_t r;
} xfs_range;

typedef struct xfs_rangef {
    float s;
    float r;
} xfs_rangef;

typedef struct xfs_rangeu16 {
    uint32_t s : 16;
    uint32_t r : 16;
} xfs_rangeu16;

typedef struct xfs_hermitecurve {
    float x[8];
    float y[8];
} xfs_hermitecurve;

typedef struct xfs_float3x4 {
    float m[3][4];
} xfs_float3x4;

typedef struct xfs_soa_vector3 {
    xfs_vector4 x;
    xfs_vector4 y;
    xfs_vector4 z;
} xfs_soa_vector3;

typedef struct xfs_linesegment4 {
    xfs_soa_vector3 p0_4;
    xfs_soa_vector3 p1_4;
} xfs_linesegment4;

typedef struct xfs_aabb4 {
    xfs_soa_vector3 min_4;
    xfs_soa_vector3 max_4;
} xfs_aabb4;

typedef struct xfs_vector2 {
    float x;
    float y;
} xfs_vector2;

typedef struct xfs_matrix33 {
    float m[3][3];
} xfs_matrix33;

typedef struct xfs_rect3d_xz {
    xfs_vector2 lt;
    xfs_vector2 lb;
    xfs_vector2 rt;
    xfs_vector2 rb;
    float height;
} xfs_rect3d_xz;

typedef struct xfs_rect3d {
    xfs_vector3 normal;
    float size_w;
    xfs_vector3 center;
    float size_h;
} xfs_rect3d;

typedef struct xfs_plane_xz {
    float dist;
} xfs_plane_xz;

typedef struct xfs_ray_y {
    xfs_float3 from;
    float dir;
} xfs_ray_y;

typedef struct xfs_pointf {
    float x;
    float y;
} xfs_pointf;

typedef struct xfs_sizef {
    float w;
    float h;
} xfs_sizef;

typedef struct xfs_rectf {
    float l;
    float t;
    float r;
    float b;
} xfs_rectf;

#endif // PROP_TYPES_H
