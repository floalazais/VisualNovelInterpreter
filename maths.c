#include "maths.h"

mat4 mat4_create()
{
	mat4 mat;
	for (int i = 0; i < 16; i++)
	{
		if (i % 5)
		{
			mat.elements[i] = 0.0f;
		} else {
			mat.elements[i] = 1.0f;
		}
	}
	return mat;
}

void mat4_copy(mat4 *destination, mat4 *source)
{
	for (int i = 0; i < 16; i++)
	{
		destination->elements[i] = source->elements[i];
	}
}

mat4 mat4_ortho(int left, int right, int bottom, int top)
{
	mat4 ortho = mat4_create();
	ortho.elements[0] = 2.0f / (float)(right - left);
	ortho.elements[5] = 2.0f / (float)(top - bottom);
	ortho.elements[10] = -1.0f;
	ortho.elements[12] = -((float)(right + left) / (float)(right - left));
	ortho.elements[13] = -((float)(top + bottom) / (float)(top - bottom));
	return ortho;
}

mat4 mat4_scale(mat4 mat, float scaleX, float scaleY)
{
	mat4 scale;
	mat4_copy(&scale, &mat);
	scale.elements[0] *= scaleX;
	scale.elements[5] *= scaleY;
	return scale;
}

mat4 mat4_translate(mat4 mat, ivec2 translation)
{
	mat4 translate;
	mat4_copy(&translate, &mat);
	translate.elements[12] += translation.x * translate.elements[0];
	translate.elements[13] += translation.y * translate.elements[5];
	return translate;
}
