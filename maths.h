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
	union
	{
		float elements[4][4];
		struct
		{
			float e00; float e10; float e20; float e30;
			float e01; float e11; float e21; float e31;
			float e02; float e12; float e22; float e32;
			float e03; float e13; float e23; float e33;
		};
	};
} mat4;

mat4 mat4_identity();
mat4 mat4_ortho(float left, float right, float bottom, float top);
mat4 mat4_scale(mat4 *mat, float scaleX, float scaleY);
mat4 mat4_translate(mat4 *mat, ivec2 translation);

#endif /* end of include guard: MATHS_H */
