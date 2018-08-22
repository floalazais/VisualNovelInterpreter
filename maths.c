#include "maths.h"

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
	scale.elements[0][0] *= scaleX;
	scale.elements[1][1] *= scaleY;
	return scale;
}

mat4 mat4_translate(mat4 *mat, ivec2 translation)
{
	mat4 translate = *mat;
	translate.elements[3][0] += translation.x * translate.elements[0][0];
	translate.elements[3][1] += translation.y * translate.elements[1][1];
	return translate;
}
