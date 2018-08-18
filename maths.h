#ifndef MATHS_H
#define MATHS_H

typedef struct ivec2
{
	int x;
	int y;
} ivec2;

typedef struct vec2
{
	float x;
	float y;
} vec2;

typedef struct ivec3
{
	int x;
	int y;
	int z;
} ivec3;

typedef struct vec3
{
	float x;
	float y;
	float z;
} vec3;

typedef struct mat4
{
	float elements[16];
} mat4;

mat4 mat4_create();
void mat4_copy(mat4 *destination, mat4 *source);
mat4 mat4_ortho(int left, int right, int bottom, int top);
mat4 mat4_scale(mat4 mat, float scaleX, float scaleY);
mat4 mat4_translate(mat4 mat, ivec2 translation);

#endif /* end of include guard: MATHS_H */
