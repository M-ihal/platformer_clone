#ifndef _MATHS_H
#define _MATHS_H

#include "common.h"

#define PI 3.14159
#define sign(n) (((n) < 0) ? -1 : ((n) > 0) ? 1 : 0)
#define square(n) ((n)*(n))
#define min_value(a, b) (((a) < (b)) ? (a) : (b))
#define max_value(a, b) (((a) > (b)) ? (a) : (b))
#define divide_or_zero(n, d) (((d) != 0) ? ((n) / (d)) : 0)
#define absolute_value(n) (((n) < 0) ? (-(n)) : (n))

template <typename T, typename T1> T clamp_min(T value, T1 min);
template <typename T, typename T1> T clamp_max(T value, T1 max);
template <typename T, typename T1, typename T2> T clamp(T value, T1 min, T2 max);
template <typename T, typename T1> void clamp_min(T *value, T1 min);
template <typename T, typename T1> void clamp_max(T *value, T1 max);
template <typename T, typename T1, typename T2> void clamp(T *value, T1 min, T2 max);
template <typename T, typename T1> T move_toward(T value, T1 to, float32_t t);

struct vec2;
struct vec3;
struct vec4;

float32_t deg_to_rad(float32_t degrees);
float32_t rad_to_deg(float32_t radians);
float32_t lerp(float32_t value,  float32_t target, float32_t t);
void lerp(float32_t *value, float32_t target, float32_t t);
vec2 lerp(const vec2 &value, const vec2 &target, float32_t t);
vec3 lerp(const vec3 &value, const vec3 &target, float32_t t);
vec4 lerp(const vec4 &value, const vec4 &target, float32_t t);
void lerp(vec2 *value, const vec2 &target, float32_t t);
void lerp(vec3 *value, const vec3 &target, float32_t t);
void lerp(vec4 *value, const vec4 &target, float32_t t);

struct vec2 {
    union {
        float32_t e[2];
        struct {
            float32_t x;
            float32_t y;
        };
    };

    static vec2 make(const float32_t x, const float32_t y) {
        vec2 result;
        result.x = x;
        result.y = y;
        return result;
    }

    static vec2 make(const float32_t xy = 0.0f) {
        vec2 result = make(xy, xy);
        return result;
    }

    static vec2 make(const vec2 from, const vec2 to) {
        vec2 result = make(to.x - from.x, to.y - from.y);
        return result;
    }

    static vec2 vector(const vec2 a_vector, const vec2 b_vector);
    static float32_t length(const vec2 vector);
    static vec2 normalize(const vec2 vector);
    static vec2 from_angle(const float32_t theta);
    static float32_t dot(const vec2 a_vector, const vec2 b_vector);
    static vec2 rotate_around_origin(vec2 vec, float32_t theta);            // @check , @copied
    static vec2 rotate_around_point(vec2 vec, vec2 point, float32_t theta); // @check , @copied
}; /* vec2 */

vec2 operator - (const vec2 rhs);
vec2 operator + (const vec2 lhs, const vec2 rhs);
vec2 operator + (const vec2 lhs, const float32_t rhs);
vec2 operator - (const vec2 lhs, const vec2 rhs);
vec2 operator * (const vec2 lhs, const vec2 rhs);
vec2 operator * (const vec2 lhs, const float32_t rhs);
vec2 operator * (const float32_t lhs, const vec2 rhs);
vec2 operator / (const vec2 lhs, const vec2 rhs);
vec2 operator / (const vec2 lhs, const float32_t rhs);
vec2 &operator += (vec2 &lhs, const vec2 rhs);
vec2 &operator += (vec2 &lhs, const float32_t rhs);
vec2 &operator -= (vec2 &lhs, const vec2 rhs);
vec2 &operator *= (vec2 &lhs, const vec2 rhs);
vec2 &operator *= (vec2 &lhs, const float32_t rhs);
vec2 &operator /= (vec2 &lhs, const vec2 rhs);
vec2 &operator /= (vec2 &lhs, const float32_t rhs);
bool operator == (const vec2 lhs, const vec2 rhs);
bool operator != (const vec2 lhs, const vec2 rhs);

struct vec2i {
    union {
        int32_t e[2];
        struct {
            int32_t x;
            int32_t y;
        };
    };

    static vec2i make(int32_t x, int32_t y) {
        return { x, y };
    }

    static vec2i make(int32_t xy) {
        return { xy, xy };
    }

    static vec2i make(vec2 vec) {
        return { (int32_t)vec.x, (int32_t)vec.y };
    }

    static vec2i make(int32_t v[2]) {
        return { v[0], v[1] };
    }

    vec2 to_vec2(void) {
        return { (float32_t)this->x, (float32_t)this->y };
    }
}; /* vec2i */

vec2i operator - (const vec2i lhs);
vec2i operator + (const vec2i lhs, const vec2i rhs);
vec2i operator - (const vec2i lhs, const vec2i rhs);
vec2i operator * (const vec2i lhs, const vec2i rhs);
vec2i operator * (const vec2i lhs, const int32_t rhs);
vec2i operator / (const vec2i lhs, const vec2i rhs);
vec2i operator / (const vec2i lhs, const int32_t rhs);
vec2i &operator += (vec2i &lhs, const vec2i rhs);
vec2i &operator -= (vec2i &lhs, const vec2i rhs);
vec2i &operator *= (vec2i &lhs, const vec2i rhs);
vec2i &operator *= (vec2i &lhs, const int32_t rhs);
vec2i &operator /= (vec2i &lhs, const vec2i rhs);
vec2i &operator /= (vec2i &lhs, const int32_t rhs);
bool operator == (const vec2i lhs, const vec2i rhs);
bool operator != (const vec2i lhs, const vec2i rhs);

struct vec3 {
    union {
        float32_t e[3];
        struct {
            float32_t x;
            float32_t y;
            float32_t z;
        };
        struct {
            float32_t r;
            float32_t g;
            float32_t b;
        };
        struct {
            vec2 xy;
        };
    };
            
    static vec3 make(float32_t x, float32_t y, float32_t z) {
        vec3 result;
        result.x = x;
        result.y = y;
        result.z = z;
        return result;
    }

    static vec3 make(float32_t xyz = 0.0f) {
        vec3 result = make(xyz, xyz, xyz);
        return result;
    }

    static vec3 make(vec2 xy, float32_t z = 0.0f) {
        vec3 result = make(xy.x, xy.y, z);
        return result;
    }

    static float32_t length(const vec3 vector);
    static vec3 normalize(const vec3 vector);
    static vec3 cross(const vec3 l_vector, const vec3 r_vector);
    static float32_t dot(const vec3 l_vector, const vec3 r_vector);
}; /* vec3 */

vec3 operator - (const vec3 lhs);
vec3 operator + (const vec3 lhs, const vec3 rhs);
vec3 operator - (const vec3 lhs, const vec3 rhs);
vec3 operator * (const vec3 lhs, const vec3 rhs);
vec3 operator * (const vec3 lhs, const float32_t rhs);
vec3 operator / (const vec3 lhs, const vec3 rhs);
vec3 operator / (const vec3 lhs, const float32_t rhs);
vec3 &operator += (vec3 &lhs, const vec3 rhs);
vec3 &operator -= (vec3 &lhs, const vec3 rhs);
vec3 &operator *= (vec3 &lhs, const vec3 rhs);
vec3 &operator *= (vec3 &lhs, const float32_t rhs);
vec3 &operator /= (vec3 &lhs, const vec3 rhs);
vec3 &operator /= (vec3 &lhs, const float32_t rhs);
bool operator == (const vec3 lhs, const vec3 rhs);
bool operator != (const vec3 lhs, const vec3 rhs);

struct vec4 {
    union {
        float32_t e[4];
        struct {
            float32_t x;
            float32_t y;
            float32_t z;
            float32_t w;
        };
        struct {
            float32_t r;
            float32_t g;
            float32_t b;
            float32_t a;
        };
        struct {
            vec3 xyz;
            float32_t _w;
        };
        struct {
            vec3 rgb;
            float32_t _a;
        };
    };

    static vec4 make(float32_t x, float32_t y, float32_t z, float32_t w) {
        vec4 result;
        result.x = x;
        result.y = y;
        result.z = z;
        result.w = w;
        return result;
    }

    static vec4 make(float32_t xyzw = 0.0f) {
        vec4 result = make(xyzw, xyzw, xyzw, xyzw);
        return result;
    }

    static vec4 make(vec3 xyz, float32_t w) {
        vec4 result = make(xyz.x, xyz.y, xyz.z, w);
        return result;
    }
}; /* vec4 */

vec4 operator - (const vec4 lhs);
vec4 operator + (const vec4 lhs, const vec4 rhs);
vec4 operator - (const vec4 lhs, const vec4 rhs);
vec4 operator * (const vec4 lhs, const vec4 rhs);
vec4 operator * (const vec4 lhs, const float32_t rhs);
vec4 operator / (const vec4 lhs, const vec4 rhs);
vec4 operator / (const vec4 lhs, const float32_t rhs);
vec4 &operator += (vec4 &lhs, const vec4 rhs);
vec4 &operator -= (vec4 &lhs, const vec4 rhs);
vec4 &operator *= (vec4 &lhs, const vec4 rhs);
vec4 &operator *= (vec4 &lhs, const float32_t rhs);
vec4 &operator /= (vec4 &lhs, const vec4 rhs);
vec4 &operator /= (vec4 &lhs, const float32_t rhs);
bool operator == (const vec4 lhs, const vec4 rhs);
bool operator != (const vec4 lhs, const vec4 rhs);

struct mat4x4 {
    union {
        float32_t e[16];
        struct {
            float32_t v00, v10, v20, v30;
            float32_t v01, v11, v21, v31;
            float32_t v02, v12, v22, v32;
            float32_t v03, v13, v23, v33;
        };
    };

    static mat4x4 identity(void) {
        mat4x4 identity = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        return identity;
    }

    static mat4x4 translate(float32_t x, float32_t y, float32_t z);
    static mat4x4 scale(float32_t x, float32_t y, float32_t z);
    static mat4x4 rotate_z(float32_t theta);
    static mat4x4 orthographic(float32_t left, float32_t bottom, float32_t right, float32_t top, float32_t near, float32_t far);
    static mat4x4 look_at(vec3 from, vec3 to, vec3 up);
}; /* mat4x4 */

mat4x4 operator * (const mat4x4 &lhs, const mat4x4 &rhs);
mat4x4 &operator *= (mat4x4 &lhs, const mat4x4 &rhs);
vec3 operator * (const mat4x4 &lhs, const vec3 &rhs);
vec4 operator * (const mat4x4 &lhs, const vec4 &rhs);
vec4 operator * (const vec4 &lhs, const mat4x4 &rhs);

struct rectfloat32_t {
    float32_t x;
    float32_t y;
    float32_t w;
    float32_t h;

    static rectfloat32_t make(float32_t x, float32_t y, float32_t w, float32_t h) {
        rectfloat32_t result;
        result.x = x;
        result.y = y;
        result.w = w;
        result.h = h;
        return result;
    }

    static rectfloat32_t make(vec2 xy, vec2 wh) {
        rectfloat32_t result = rectfloat32_t::make(xy.x, xy.y, wh.x, wh.y);
        return result;
    }

    vec2 size(void) const {
        vec2 result = vec2::make(this->w, this->h);
        return result;
    }

    vec2 top_left(void) const {
        vec2 result = vec2::make(this->x, this->y + this->h);
        return result;
    }

    vec2 top_right(void) const {
        vec2 result = vec2::make(this->x + this->w, this->y + this->h);
        return result;
    }

    vec2 bottom_left(void) const {
        vec2 result = vec2::make(this->x, this->y);
        return result;
    }

    vec2 bottom_right(void) const {
        vec2 result = vec2::make(this->x + this->w, this->y);
        return result;
    }
}; /* rectfloat32_t */

struct recti32 {
    union {
        struct {
            int32_t x;
            int32_t y;
            int32_t w;
            int32_t h;
        };
        struct {
            vec2i xy;
            vec2i wh;
        };
    };

    static recti32 make(int32_t x, int32_t y, int32_t w, int32_t h) {
        recti32 result;
        result.x = x;
        result.y = y;
        result.w = w;
        result.h = h;
        return result;
    }

    vec2i size(void) const {
        vec2i result = vec2i::make(this->w, this->h);
        return result;
    }

    vec2i top_left(void) const {
        vec2i result = vec2i::make(this->x, this->y + this->h);
        return result;
    }

    vec2i top_right(void) const {
        vec2i result = vec2i::make(this->x + this->w, this->y + this->h);
        return result;
    }

    vec2i bottom_left(void) const {
        vec2i result = vec2i::make(this->x, this->y);
        return result;
    }

    vec2i bottom_right(void) const {
        vec2i result = vec2i::make(this->x + this->w, this->y);
        return result;
    }
}; /* recti32 */

template <typename T, typename T1>
T clamp_min(T value, T1 min) {
    T result = value;
    if(result < min) {
        result = min;
    }
    return result;
}

template <typename T, typename T1>
T clamp_max(T value, T1 max) {
    T result = value;
    if(result > max) {
        result = max;
    }
    return result;
}

template <typename T, typename T1, typename T2>
T clamp(T value, T1 min, T2 max) {
    value = clamp_min(value, min);
    value = clamp_max(value, max);
    return value;
}

template <typename T, typename T1>
void clamp_min(T *value, T1 min) {
    *value = clamp_min(*value, min);
}

template <typename T, typename T1>
void clamp_max(T *value, T1 max) {
    *value = clamp_max(*value, max);
}

template <typename T, typename T1, typename T2>
void clamp(T *value, T1 min, T2 max) {
    *value = clamp(*value, min, max);
}

// todo
template <typename T, typename T1>
T move_toward(T value, T1 to, float32_t t) {
    if(value == to) {
        return value;
    }

    T result = value;
    if(result < to) {
        result += t;
        if(result > to) {
            result = to;
        }
    } else {
        result -= t;
        if(result < to) {
            result = to;
        }
    }
    return result;
}

#endif /* _MATHS_H */