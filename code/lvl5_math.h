#ifndef LVL5_MATH

#include <math.h>
#include "lvl5_types.h"


template<typename T>
T min(T a, T b) {
  T result = a > b ? b : a;
  return result;
}

template<typename T>
T max(T a, T b) {
  T result = a > b ? a : b;
  return result;
}

union V2 {
    struct {
        f32 x, y;
    };
    f32 e[2];
};


V2 v2(i32 x, i32 y) {
  V2 result = {(f32)x, (f32)y};
  return result;
}
V2 operator+(V2 a, V2 b) {
    V2 result = {a.x + b.x, a.y + b.y};
    return result;
}
V2 operator*(V2 a, f32 s) {
    V2 result = {a.x*s, a.y*s};
    return result;
}

V2 operator-(V2 a, V2 b) {
    V2 result = a + b*-1;
    return result;
}

V2 operator*(V2 a, V2 b) {
  V2 result = {a.x*b.x, a.y*b.y};
  return result;
}

V2 operator/(V2 a, V2 b) {
  V2 result = {a.x/b.x, a.y/b.y};
  return result;
}

V2 operator+=(V2 &a, V2 b) {
    a = a + b;
    return a;
}

V2 operator/(V2 a, f32 s) {
    V2 result = a*(1/s);
    return result;
}

V2 operator/(f32 s, V2 a) {
  V2 result = {s/a.x, s/a.y};
  return result;
}

f32 dot(V2 a, V2 b) {
    f32 result = a.x*b.x + a.y*b.y;
    return result;
}

f32 len_sqr(V2 a) {
    f32 result = a.x*a.x + a.y*a.y;
    return result;
}

f32 len(V2 a) {
    f32 result = sqrtf(len_sqr(a));
    return result;
}

V2 hadamard(V2 a, V2 b) {
  V2 result = {a.x*b.x, a.y*b.y};
  return result;
}

union V3 {
  struct {
    f32 x, y, z;
  };
  f32 e[3];
};

V3 v3(V2 a, f32 z) {
  V3 result = {a.x, a.y, z};
  return result;
}



union V4 {
  struct {
    f32 x, y, z, w;
  };
  struct {
    f32 r, g, b, a;
  };
  struct {
    V2 xy, zw;
  };
  V3 xyz;
  V3 rgb;
};


struct Rect2 {
  V2 min;
  V2 max;
};

V2 get_size(Rect2 rect) {
  V2 result = rect.max - rect.min;
  return result;
}

Rect2 rect2_min_size(V2 min, V2 size) {
  Rect2 result = {
    min,
    min + size,
  };
  return result;
}

Rect2 rect2_center_size(V2 center, V2 size) {
  V2 half = size*0.5f;
  Rect2 result = {
    center - half,
    center + half,
  };
  return result;
}

Rect2 intersect(Rect2 a, Rect2 b) {
  Rect2 result = {
    {max(a.min.x, b.min.x), max(a.min.y, b.min.y)},
    {min(a.max.x, b.max.x), min(a.max.y, b.max.y)},
  };
  return result;
}

bool has_area(Rect2 rect) {
  V2 size = get_size(rect);
  bool result = size.x > 0 && size.y > 0;
  return result;
}

V2 get_center(Rect2 r) {
  V2 result = r.min + get_size(r)*0.5f;
  return result;
}

Rect2 add_radius(Rect2 a, V2 radius) {
  Rect2 result = a;
  result.min.x -= radius.x;
  result.min.y -= radius.y;
  result.max.x += radius.x;
  result.max.y += radius.y;
  return result;
}

Rect2 inverted_infinity_rect() {
  Rect2 result = {
    {INFINITY, INFINITY},
    {-INFINITY, -INFINITY},
  };
  return result;
}


struct V2i {
  i32 x, y;
};

V2i v2i(V2 a) {
  V2i result = {(i32)a.x, (i32)a.y};
  return result;
}

V2i operator-(V2i a, V2i b) {
  V2i result = {a.x - b.x, a.y - b.y};
  return result;
}

V2 v2(V2i a) {
  V2 result = v2(a.x, a.y);
  return result;
}



struct Rect2i {
  V2i min, max;
};

Rect2 rect2(Rect2i r) {
  Rect2 result = {v2(r.min), v2(r.max)};
  return result;
}

Rect2i rect2i(Rect2 r) {
  Rect2i result = {v2i(r.min), v2i(r.max)};
  return result;
}

Rect2i intersect(Rect2i a, Rect2i b) {
  Rect2i result = {
    {max(a.min.x, b.min.x), max(a.min.y, b.min.y)},
    {min(a.max.x, b.max.x), min(a.max.y, b.max.y)},
  };
  return result;
}

V2i get_size(Rect2i a) {
  V2i result = a.max - a.min;
  return result;
}

bool has_area(Rect2i rect) {
  V2i size = get_size(rect);
  bool result = size.x > 0 && size.y > 0;
  return result;
}

Rect2i add_radius(Rect2i rect, V2i radius) {
  Rect2i result = rect;
  result.min.x -= radius.x;
  result.min.y -= radius.y;
  result.max.x += radius.x;
  result.max.y += radius.y;
  return result;
}



V2 fract(V2 a) {
  V2 result = {a.x - (f32)(i32)a.x, a.y - (f32)(i32)a.y};
  return result;
}

V2 clamp01(V2 a) {
  V2 result = {
    min(1.0f, max(0.0f, a.x)),
    min(1.0f, max(0.0f, a.y)),
  };
  return result;
}

V2 floor(V2 a) {
  V2 result = v2((i32)a.x, (i32)a.y);
  return result;
}


template<typename T>
T sqr(T a) {
  T result = a*a;
  return result;
}




#include <immintrin.h>
#include <avxintrin.h>

typedef __m256 f32_8x;

f32_8x set8(f32 a) {
  f32_8x result = {a, a, a, a, a, a, a, a};
  return result;
}

union V2_8x {
  struct {
    __m256 x, y;
  };
};

V2_8x set8(V2 a) {
  V2_8x result = {
    .x = set8(a.x),
    .y = set8(a.y),
  };
  return result;
}

V2_8x operator+(V2_8x a, V2_8x b) {
  V2_8x result = { 
    .x = _mm256_add_ps(a.x, b.x),
    .y = _mm256_add_ps(a.y, b.y),
  };
  return result;
}

V2_8x operator+=(V2_8x &a, V2_8x b) {
  a = a + b;
  return a;
}

V2_8x operator*(V2_8x a, V2_8x b) {
  V2_8x result = { 
    .x = _mm256_mul_ps(a.x, b.x),
    .y = _mm256_mul_ps(a.y, b.y),
  };
  return result;
}

V2_8x operator/(f32 a, V2_8x b) {
  V2_8x result = { 
    .x = a / b.x,
    .y = a / b.y,
  };
  return result;
}

V2_8x operator-(V2_8x a, V2_8x b) {
  V2_8x result = { 
    .x = _mm256_sub_ps(a.x, b.x),
    .y = _mm256_sub_ps(a.y, b.y),
  };
  return result;
}

f32_8x dot(V2_8x a, V2_8x b) {
   f32_8x result =  _mm256_add_ps(_mm256_mul_ps(a.x, b.x), _mm256_mul_ps(a.y, b.y));
   return result;
}

V2_8x floor(V2_8x a) {
  V2_8x result = {
    .x = _mm256_floor_ps(a.x),
    .y = _mm256_floor_ps(a.y),
  };
  return result;
}

V2_8x fract(V2_8x a) {
  V2_8x result = {
    .x = a.x - _mm256_floor_ps(a.x),
    .y = a.y - _mm256_floor_ps(a.y),
  };
  return result;
}

V2_8x clamp01(V2_8x a) {
  V2_8x result = {
    .x = _mm256_min_ps(_mm256_max_ps(set8(0), a.x), set8(1)),
    .y = _mm256_min_ps(_mm256_max_ps(set8(0), a.y), set8(1)),
  };
  return result;
}


union i32_8x {
  __m256i full;
  i32 e[8];
};

i32_8x set8i(i32 v) {
  i32_8x result = {.e = {v, v, v, v, v, v, v, v}};
  return result;
}

i32_8x operator&(i32_8x a, u32 b) {
  i32_8x result = {
    .full = _mm256_and_si256(a.full, set8i((i32)b).full),
  };
  return result;
}

i32_8x operator&(i32_8x a, i32_8x b) {
  i32_8x result = {
    .full = a.full & b.full,
  };
  return result;
}

i32_8x operator|(i32_8x a, u32 b) {
  i32_8x result = {
    .full = a.full | b,
  };
  return result;
}

i32_8x operator|(i32_8x a, i32_8x b) {
  i32_8x result = {
    .full = a.full | b.full,
  };
  return result;
}

i32_8x operator<<(i32_8x a, i32 b) {
  i32_8x result = {
    .full = _mm256_slli_epi32(a.full, b),
  };
  return result;
}

i32_8x operator>>(i32_8x a, i32 b) {
  i32_8x result = {
    .full = _mm256_srli_epi32(a.full, b),
  };
  return result;
}


struct V2i_8x {
  i32_8x x, y;
};

V2i_8x v2i_8x(V2_8x a) {
  V2i_8x result = {
    .x = _mm256_cvtps_epi32(a.x),
    .y = _mm256_cvtps_epi32(a.y),
  };
  return result;
}


union V3_8x {
  struct {
    f32_8x x, y, z;
  };
  struct {
    f32_8x r, g, b;
  };
};

V3_8x operator+(V3_8x a, V3_8x b) {
  V3_8x result = {a.x + b.x, a.y + b.y, a.z + b.z};
  return result;
}

V3_8x operator*(V3_8x a, V3_8x b) {
  V3_8x result = {a.x*b.x, a.y*b.y, a.z*b.z};
  return result;
}

V3_8x operator*(V3_8x a, f32_8x s) {
  V3_8x result = {a.x*s, a.y*s, a.z*s};
  return result;
}

union V4_8x {
  struct {
    f32_8x x, y, z, w;
  };
  struct {
    f32_8x r, g, b, a;
  };
  struct {
    V3_8x xyz;
  };
  struct {
    V3_8x rgb;
  };
}; 

V4_8x v4_8x(V3_8x xyz, f32_8x w) {
  V4_8x result;
  result.xyz = xyz;
  result.w = w;
  return result;
}

V4_8x operator+(V4_8x a, V4_8x b) {
  V4_8x result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
  return result;
}

V4_8x operator*(V4_8x a, V4_8x b) {
  V4_8x result = {a.x*b.x, a.y*b.y, a.z*b.z, a.w*b.w};
  return result;
}

V4_8x operator*(V4_8x a, f32_8x s) {
  V4_8x result = {a.x*s, a.y*s, a.z*s, a.w*s};
  return result;
}

#define LVL5_MATH
#endif