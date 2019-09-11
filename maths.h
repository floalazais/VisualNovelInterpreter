#ifndef MATHS_H
#define MATHS_H

typedef struct ivec2
{
	int x;
	int y;
} ivec2;

ivec2 ivec2_add(ivec2 a, ivec2 b);
ivec2 ivec2_subtract(ivec2 a, ivec2 b);
ivec2 ivec2_multiply(ivec2 vec, float f);
ivec2 ivec2_divide(ivec2 vec, float f);
ivec2 ivec2_modulo(ivec2 vec, int i);

typedef struct vec2
{
	float x;
	float y;
} vec2;

vec2 vec2_add(vec2 a, vec2 b);
vec2 vec2_subtract(vec2 a, vec2 b);
vec2 vec2_multiply(vec2 vec, float f);
vec2 vec2_divide(vec2 vec, float f);
vec2 vec2_modulo(vec2 vec, float f);

typedef struct ivec3
{
	int x;
	int y;
	int z;
} ivec3;

ivec3 ivec3_add(ivec3 a, ivec3 b);
ivec3 ivec3_subtract(ivec3 a, ivec3 b);
ivec3 ivec3_multiply(ivec3 vec, float f);
ivec3 ivec3_divide(ivec3 vec, float f);
ivec3 ivec3_modulo(ivec3 vec, int i);

typedef struct vec3
{
	float x;
	float y;
	float z;
} vec3;

extern vec3 COLOR_BLACK;
extern vec3 COLOR_RED;
extern vec3 COLOR_GREEN;
extern vec3 COLOR_BLUE;
extern vec3 COLOR_YELLOW;
extern vec3 COLOR_MAGENTA;
extern vec3 COLOR_CYAN;
extern vec3 COLOR_WHITE;

vec3 vec3_add(vec3 a, vec3 b);
vec3 vec3_subtract(vec3 a, vec3 b);
vec3 vec3_multiply(vec3 vec, float f);
vec3 vec3_divide(vec3 vec, float f);
vec3 vec3_modulo(vec3 vec, float f);

typedef struct mat4
{
	float e00; float e10; float e20; float e30;
	float e01; float e11; float e21; float e31;
	float e02; float e12; float e22; float e32;
	float e03; float e13; float e23; float e33;
} mat4;

mat4 mat4_identity();
mat4 mat4_ortho(float left, float right, float bottom, float top);
mat4 mat4_scale(mat4 *mat, float scaleX, float scaleY);
mat4 mat4_translate(mat4 *mat, ivec2 translation);

#endif /* end of include guard: MATHS_H */
