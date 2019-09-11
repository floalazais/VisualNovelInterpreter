#include <math.h>

#include "maths.h"

vec3 COLOR_BLACK = {0.0f, 0.0f, 0.0f};
vec3 COLOR_RED = {1.0f, 0.0f, 0.0f};
vec3 COLOR_GREEN = {0.0f, 1.0f, 0.0f};
vec3 COLOR_BLUE = {0.0f, 0.0f, 1.0f};
vec3 COLOR_YELLOW = {1.0f, 1.0f, 0.0f};
vec3 COLOR_MAGENTA = {1.0f, 0.0f, 1.0f};
vec3 COLOR_CYAN = {0.0f, 1.0f, 1.0f};
vec3 COLOR_WHITE = {1.0f, 1.0f, 1.0f};

ivec2 ivec2_add(ivec2 a, ivec2 b)
{
	return (ivec2){a.x + b.x, a.y + b.y};
}

ivec2 ivec2_subtract(ivec2 a, ivec2 b)
{
	return (ivec2){a.x - b.x, a.y - b.y};
}

ivec2 ivec2_multiply(ivec2 vec, float f)
{
	return (ivec2){vec.x * f, vec.y * f};
}

ivec2 ivec2_divide(ivec2 vec, float f)
{
	return (ivec2){vec.x / f, vec.y / f};
}

ivec2 ivec2_modulo(ivec2 vec, int i)
{
	return (ivec2){vec.x % i, vec.y % i};
}

vec2 vec2_add(vec2 a, vec2 b)
{
	return (vec2){a.x + b.x, a.y + b.y};
}

vec2 vec2_subtract(vec2 a, vec2 b)
{
	return (vec2){a.x - b.x, a.y - b.y};
}

vec2 vec2_multiply(vec2 vec, float f)
{
	return (vec2){vec.x * f, vec.y * f};
}

vec2 vec2_divide(vec2 vec, float f)
{
	return (vec2){vec.x / f, vec.y / f};
}

vec2 vec2_modulo(vec2 vec, float f)
{
	return (vec2){fmodf(vec.x, f), fmodf(vec.y, f)};
}

ivec3 ivec3_add(ivec3 a, ivec3 b)
{
	return (ivec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

ivec3 ivec3_subtract(ivec3 a, ivec3 b)
{
	return (ivec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

ivec3 ivec3_multiply(ivec3 vec, float f)
{
	return (ivec3){vec.x * f, vec.y * f, vec.z * f};
}

ivec3 ivec3_divide(ivec3 vec, float f)
{
	return (ivec3){vec.x / f, vec.y / f, vec.z / f};
}

ivec3 ivec3_modulo(ivec3 vec, int i)
{
	return (ivec3){vec.x % i, vec.y % i, vec.z % i};
}

vec3 vec3_add(vec3 a, vec3 b)
{
	return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 vec3_subtract(vec3 a, vec3 b)
{
	return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3 vec3_multiply(vec3 vec, float f)
{
	return (vec3){vec.x * f, vec.y * f, vec.z * f};
}

vec3 vec3_divide(vec3 vec, float f)
{
	return (vec3){vec.x / f, vec.y / f, vec.z / f};
}

vec3 vec3_modulo(vec3 vec, float f)
{
	return (vec3){fmodf(vec.x, f), fmodf(vec.y, f), fmodf(vec.z, f)};
}

mat4 mat4_identity()
{
	mat4 mat =
	{
		.e00 = 1.0f, .e10 = 0.0f, .e20 = 0.0f, .e30 = 0.0f,
		.e01 = 0.0f, .e11 = 1.0f, .e21 = 0.0f, .e31 = 0.0f,
		.e02 = 0.0f, .e12 = 0.0f, .e22 = 1.0f, .e32 = 0.0f,
		.e03 = 0.0f, .e13 = 0.0f, .e23 = 0.0f, .e33 = 1.0f
	};
	return mat;
}

mat4 mat4_ortho(float left, float right, float bottom, float top)
{
	mat4 ortho =
	{
		.e00 = 2.0f / (right - left),			 .e10 = 0.0f,							  .e20 = 0.0f, .e30 = 0.0f,
		.e01 = 0.0f,							 .e11 = 2.0f / (top - bottom),			  .e21 = 0.0f, .e31 = 0.0f,
		.e02 = 0.0f,							 .e12 = 0.0f,							  .e22 = 0.0f, .e32 = 0.0f,
		.e03 = -(right + left) / (right - left), .e13 = -(top + bottom) / (top - bottom), .e23 = 0.0f, .e33 = 1.0f
	};
	return ortho;
}

mat4 mat4_scale(mat4 *mat, float scaleX, float scaleY)
{
	mat4 scale = *mat;
	scale.e00 *= scaleX;
	scale.e11 *= scaleY;
	return scale;
}

mat4 mat4_translate(mat4 *mat, ivec2 translation)
{
	mat4 translate = *mat;
	translate.e03 += translation.x * translate.e00;
	translate.e13 += translation.y * translate.e11;
	return translate;
}