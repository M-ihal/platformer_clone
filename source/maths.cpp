#include "maths.h"
#include <math.h>

float32_t deg_to_rad(float32_t degrees) {
    float32_t result = (degrees * 2.0f * PI) / 360.0f;
    return result;
}

float32_t rad_to_deg(float32_t radians) {
    float32_t result = (radians * 360.0f) / (2.0f * PI);
    return result;
}

float32_t lerp(float32_t value, float32_t target, float32_t t) {
    float32_t result = value * (1.0f - t) + target * t;
    return result;
}

void lerp(float32_t *value, float32_t target, float32_t t) {
    *value = lerp(*value, target, t);
}

vec2 lerp(const vec2 &value, const vec2 &target, float32_t t) {
    vec2 result = {
        lerp(value.x, target.x, t),
        lerp(value.y, target.y, t),
    };
    return result;
}

vec3 lerp(const vec3 &value, const vec3 &target, float32_t t) {
    vec3 result = {
        lerp(value.x, target.x, t),
        lerp(value.y, target.y, t),
        lerp(value.z, target.z, t),
    };
    return result;
}

vec4 lerp(const vec4 &value, const vec4 &target, float32_t t) {
    vec4 result = {
        lerp(value.x, target.x, t),
        lerp(value.y, target.y, t),
        lerp(value.z, target.z, t),
        lerp(value.w, target.w, t),
    };
    return result;
}

void lerp(vec2 *value, const vec2 &target, float32_t t) {
    *value = lerp(*value, target, t);
}

void lerp(vec3 *value, const vec3 &target, float32_t t) {
    *value = lerp(*value, target, t);
}

void lerp(vec4 *value, const vec4 &target, float32_t t) {
    *value = lerp(*value, target, t);
}

vec2 vec2::vector(const vec2 a_vector, const vec2 b_vector) {
    vec2 result = {
        b_vector.x - a_vector.x,
        b_vector.y - a_vector.y,
    };
    return result;
}

float32_t vec2::length(const vec2 vector) {
    float32_t result = sqrtf(square(vector.x) + square(vector.y));
    return result;
}

vec2 vec2::normalize(const vec2 vector) {
    float32_t length = vec2::length(vector);
    vec2 normalized = vector / length;
    return normalized;
}

vec2 vec2::from_angle(const float32_t theta) {
    vec2 result = vec2::make(
        cos(theta),
        sin(theta)
    );
    return result;
}

float32_t vec2::dot(const vec2 a_vector, const vec2 b_vector) {
    float32_t result = (a_vector.x * b_vector.x) + (a_vector.y * b_vector.y);
    return result;
}

vec2 vec2::rotate_around_origin(vec2 vec, float32_t theta) {
    vec2 result = {
        cosf(theta) * vec.x - sinf(theta) * vec.y,
        cosf(theta) * vec.y + sinf(theta) * vec.x,
    };
    return result;
}

vec2 vec2::rotate_around_point(vec2 vec, vec2 point, float32_t theta) {
    vec2 result = vec2::rotate_around_origin(vec - point, theta) + point;
    return result;
}

float32_t vec3::length(const vec3 vector) {
    float32_t dot = vec3::dot(vector, vector);
    float32_t result = sqrtf(dot);
    return result;
}

vec3 vec3::normalize(const vec3 vector) {
    float32_t v_len = vec3::length(vector);
    if(v_len == 0) {
        return vec3::make(0.0f);
    }

    vec3 result;
    result.x = vector.x / v_len;
    result.y = vector.y / v_len;
    result.z = vector.z / v_len;
    return result;
}

vec3 vec3::cross(const vec3 l_vector, const vec3 r_vector) {
    vec3 result;
    result.x = (l_vector.y * r_vector.z) - (l_vector.z * r_vector.y);
    result.y = (l_vector.z * r_vector.x) - (l_vector.x * r_vector.z);
    result.z = (l_vector.x * r_vector.y) - (l_vector.y * r_vector.x);
    return result;
}

float32_t vec3::dot(const vec3 l_vector, const vec3 r_vector) {
    float32_t result = (l_vector.x * r_vector.x) + (l_vector.y * r_vector.y) + (l_vector.z * r_vector.z);
    return result;
}

mat4x4 mat4x4::translate(float32_t x, float32_t y, float32_t z) {
    mat4x4 result = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1,
    };
    return result;
}

mat4x4 mat4x4::scale(float32_t x, float32_t y, float32_t z) {
    mat4x4 result = {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1,
    };
    return result;
}

mat4x4 mat4x4::rotate_z(float32_t theta) {
    float32_t m00 = cos(theta);
    float32_t m10 = -sin(theta);
    float32_t m01 = sin(theta);
    float32_t m11 = cos(theta);
    mat4x4 result = {
        m00,  m10,  0.0f, 0.0f,
        m01,  m11,  0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    return result;
}

mat4x4 mat4x4::orthographic(float32_t left, float32_t bottom, float32_t right, float32_t top, float32_t near, float32_t far) {
    float32_t m00 = 2.0f / (right - left);
    float32_t m11 = 2.0f / (top - bottom);
    float32_t m22 = 2.0f / (far - near);
    float32_t m03 = -((right + left) / (right - left));
    float32_t m13 = -((top + bottom) / (top - bottom));
    float32_t m23 = -((far + near) / (far - near));
    mat4x4 projection = {
        m00,  0.0f, 0.0f, 0.0f,
        0.0f, m11,  0.0f, 0.0f,
        0.0f, 0.0f, m22,  0.0f,
        m03,  m13,  m23,  1.0f,
    };
    return projection;
}

mat4x4 mat4x4::look_at(vec3 from, vec3 to, vec3 up) {
    vec3 forward = vec3::normalize(to - from);
    vec3 side = vec3::normalize(vec3::cross(forward, up));
    vec3 up_ = vec3::cross(side, forward);
    float32_t m03 = -vec3::dot(side, from);
    float32_t m13 = -vec3::dot(up, from);
    float32_t m23 = vec3::dot(forward, from);
    mat4x4 view = {
        side.e[0], up_.e[0], -forward.e[0], 0.0f,
        side.e[1], up_.e[1], -forward.e[1], 0.0f,
        side.e[2], up_.e[2], -forward.e[2], 0.0f,
        m03,       m13,      m23,          1.0f,
    };
    return view;
}

/* --- vec2 --- */

vec2 operator - (const vec2 rhs) {
    vec2 result = vec2::make(-rhs.x, -rhs.y);
    return result;
}

vec2 operator + (const vec2 lhs, const vec2 rhs) {
    vec2 result = vec2::make(lhs.x + rhs.x, lhs.y + rhs.y);
    return result;
}

vec2 operator + (const vec2 lhs, const float32_t rhs) {
    vec2 result = vec2::make(lhs.x + rhs, lhs.y + rhs);
    return result;
}

vec2 operator - (const vec2 lhs, const vec2 rhs) {
    vec2 result = vec2::make(lhs.x - rhs.x, lhs.y - rhs.y);
    return result;
}

vec2 operator * (const vec2 lhs, const vec2 rhs) {
    vec2 result = vec2::make(lhs.x * rhs.x, lhs.y * rhs.y);
    return result;
}

vec2 operator * (const vec2 lhs, const float32_t rhs) {
    vec2 result = vec2::make(lhs.x * rhs, lhs.y * rhs);
    return result;
}

vec2 operator * (const float32_t lhs, const vec2 rhs) {
    vec2 result = rhs * lhs;
    return result;
}

vec2 operator / (const vec2 lhs, const vec2 rhs) {
    vec2 result = vec2::make(lhs.x / rhs.x, lhs.y / rhs.y);
    return result;
}

vec2 operator / (const vec2 lhs, const float32_t rhs) {
    vec2 result = vec2::make(lhs.x / rhs, lhs.y / rhs);
    return result;
}

vec2 &operator += (vec2 &lhs, const vec2 rhs) {
    lhs = lhs + rhs;
    return lhs;
}

vec2 &operator += (vec2 &lhs, const float32_t rhs) {
    lhs = lhs + rhs;
    return lhs;
}

vec2 &operator -= (vec2 &lhs, const vec2 rhs) {
    lhs = lhs - rhs;
    return lhs;
}

vec2 &operator *= (vec2 &lhs, const vec2 rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec2 &operator *= (vec2 &lhs, const float32_t rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec2 &operator /= (vec2 &lhs, const vec2 rhs) {
    lhs = lhs / rhs;
    return lhs;
}

vec2 &operator /= (vec2 &lhs, const float32_t rhs) {
    lhs = lhs / rhs;
    return lhs;
}

bool operator == (const vec2 lhs, const vec2 rhs) {
    bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
    return result;
}

bool operator != (const vec2 lhs, const vec2 rhs) {
    bool result = !(lhs == rhs);
    return result;
}

/* --- vec2i --- */

vec2i operator - (const vec2i lhs) {
    vec2i result = vec2i::make(-lhs.x, -lhs.y);
    return result;
}

vec2i operator + (const vec2i lhs, const vec2i rhs) {
    vec2i result = vec2i::make(lhs.x + rhs.x, lhs.y + rhs.y);
    return result;
}

vec2i operator - (const vec2i lhs, const vec2i rhs) {
    vec2i result = vec2i::make(lhs.x - rhs.x, lhs.y - rhs.y);
    return result;
}

vec2i operator * (const vec2i lhs, const vec2i rhs) {
    vec2i result = vec2i::make(lhs.x * rhs.x, lhs.y * rhs.y);
    return result;
}

vec2i operator * (const vec2i lhs, const int32_t rhs) {
    vec2i result = vec2i::make(lhs.x * rhs, lhs.y * rhs);
    return result;
}

vec2i operator / (const vec2i lhs, const vec2i rhs) {
    vec2i result = vec2i::make(lhs.x / rhs.x, lhs.y / rhs.y);
    return result;
}

vec2i operator / (const vec2i lhs, const int32_t rhs) {
    vec2i result = vec2i::make(lhs.x / rhs, lhs.y / rhs);
    return result;
}

vec2i &operator += (vec2i &lhs, const vec2i rhs) {
    lhs = lhs + rhs;
    return lhs;
}

vec2i &operator -= (vec2i &lhs, const vec2i rhs) {
    lhs = lhs - rhs;
    return lhs;
}

vec2i &operator *= (vec2i &lhs, const vec2i rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec2i &operator *= (vec2i &lhs, const int32_t rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec2i &operator /= (vec2i &lhs, const vec2i rhs) {
    lhs = lhs / rhs;
    return lhs;
}

vec2i &operator /= (vec2i &lhs, const int32_t rhs) {
    lhs = lhs / rhs;
    return lhs;
}

bool operator == (const vec2i lhs, const vec2i rhs) {
    bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y);
    return result;
}

bool operator != (const vec2i lhs, const vec2i rhs) {
    bool result = !(lhs == rhs);
    return result;
}

/* --- vec3 --- */

vec3 operator - (const vec3 lhs) {
    vec3 result = vec3::make(-lhs.x, -lhs.y, -lhs.z);
    return result;
}

vec3 operator + (const vec3 lhs, const vec3 rhs) {
    vec3 result = vec3::make(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
    return result;
}

vec3 operator - (const vec3 lhs, const vec3 rhs) {
    vec3 result = vec3::make(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
    return result;
}

vec3 operator * (const vec3 lhs, const vec3 rhs) {
    vec3 result = vec3::make(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
    return result;
}

vec3 operator * (const vec3 lhs, const float32_t rhs) {
    vec3 result = vec3::make(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
    return result;
}

vec3 operator / (const vec3 lhs, const vec3 rhs) {
    vec3 result = vec3::make(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z);
    return result;
}

vec3 operator / (const vec3 lhs, const float32_t rhs) {
    vec3 result = vec3::make(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
    return result;
}

vec3 &operator += (vec3 &lhs, const vec3 rhs) {
    lhs = lhs + rhs;
    return lhs;
}

vec3 &operator -= (vec3 &lhs, const vec3 rhs) {
    lhs = lhs - rhs;
    return lhs;
}

vec3 &operator *= (vec3 &lhs, const vec3 rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec3 &operator *= (vec3 &lhs, const float32_t rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec3 &operator /= (vec3 &lhs, const vec3 rhs) {
    lhs = lhs / rhs;
    return lhs;
}

vec3 &operator /= (vec3 &lhs, const float32_t rhs) {
    lhs = lhs / rhs;
    return lhs;
}

bool operator == (const vec3 lhs, const vec3 rhs) {
    bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
    return result;
}

bool operator != (const vec3 lhs, const vec3 rhs) {
    bool result = !(lhs == rhs);
    return result;
}

/* --- vec4 --- */

vec4 operator - (const vec4 lhs) {
    vec4 result = vec4::make(-lhs.x, -lhs.y, -lhs.z, -lhs.w);
    return result;
}

vec4 operator + (const vec4 lhs, const vec4 rhs) {
    vec4 result = vec4::make(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
    return result;
}

vec4 operator - (const vec4 lhs, const vec4 rhs) {
    vec4 result = vec4::make(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
    return result;
}

vec4 operator * (const vec4 lhs, const vec4 rhs) {
    vec4 result = vec4::make(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
    return result;
}

vec4 operator * (const vec4 lhs, const float32_t rhs) {
    vec4 result = vec4::make(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
    return result;
}

vec4 operator / (const vec4 lhs, const vec4 rhs) {
    vec4 result = vec4::make(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w);
    return result;
}

vec4 operator / (const vec4 lhs, const float32_t rhs) {
    vec4 result = vec4::make(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs);
    return result;
}

vec4 &operator += (vec4 &lhs, const vec4 rhs) {
    lhs = lhs + rhs;
    return lhs;
}

vec4 &operator -= (vec4 &lhs, const vec4 rhs) {
    lhs = lhs - rhs;
    return lhs;
}

vec4 &operator *= (vec4 &lhs, const vec4 rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec4 &operator *= (vec4 &lhs, const float32_t rhs) {
    lhs = lhs * rhs;
    return lhs;
}

vec4 &operator /= (vec4 &lhs, const vec4 rhs) {
    lhs = lhs / rhs;
    return lhs;
}

vec4 &operator /= (vec4 &lhs, const float32_t rhs) {
    lhs = lhs / rhs;
    return lhs;
}

bool operator == (const vec4 lhs, const vec4 rhs) {
    bool result = (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w && rhs.w);
    return result;
}

bool operator != (const vec4 lhs, const vec4 rhs) {
    bool result = !(lhs == rhs);
    return result;
}

/* --- mat4x4 --- */

mat4x4 operator * (const mat4x4 &lhs, const mat4x4 &rhs) {
    mat4x4 result;
    for(int32_t i = 0; i < 4; ++i) {
        for(int32_t j = 0; j < 4; ++j) {
            result.e[j * 4 + i]
                = (lhs.e[0 * 4 + i] * rhs.e[j * 4 + 0])
                + (lhs.e[1 * 4 + i] * rhs.e[j * 4 + 1])
                + (lhs.e[2 * 4 + i] * rhs.e[j * 4 + 2])
                + (lhs.e[3 * 4 + i] * rhs.e[j * 4 + 3]);
        }
    }
    return result;
}

mat4x4 &operator *= (mat4x4 &lhs, const mat4x4 &rhs) {
    lhs = lhs * rhs;
    return lhs;
}

// @check
vec3 operator * (const mat4x4 &lhs, const vec3 &rhs) {
    vec3 result = (lhs * vec4::make(rhs, 1.0f)).xyz;
    return result;
}

vec4 operator * (const mat4x4 &lhs, const vec4 &rhs) {
    vec4 result = vec4::make();
    for(int32_t i = 0; i < 4; ++i) {
        for(int32_t j = 0; j < 1; ++j) {
            result.e[i]
                = (lhs.e[0 * 4 + i] * rhs.e[0])
                + (lhs.e[1 * 4 + i] * rhs.e[1])
                + (lhs.e[2 * 4 + i] * rhs.e[2])
                + (lhs.e[3 * 4 + i] * rhs.e[3]);
        }
    }
    return result;
}

vec4 operator * (const vec4 &lhs, const mat4x4 &rhs) {
    vec4 result = rhs * lhs;
    return result;
}