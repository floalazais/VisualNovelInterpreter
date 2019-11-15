#ifndef GL_H
#define GL_H

#define GL_FALSE				0x0000
#define GL_TRUE					0x0001

#define GL_TRIANGLES			0x0004

#define GL_DEPTH_BUFFER_BIT		0x0100

#define GL_SRC_ALPHA			0x0302
#define GL_ONE_MINUS_SRC_ALPHA	0x0303

#define GL_VIEWPORT				0x0BA2

#define GL_BLEND				0x0BE2

#define GL_SCISSOR_TEST			0x0C11

#define GL_UNPACK_ALIGNMENT		0x0CF5

#define GL_TEXTURE_2D			0x0DE1

#define GL_UNSIGNED_BYTE		0x1401
#define GL_FLOAT				0x1406

#define GL_RED					0x1903
#define GL_RGBA					0x1908

#define GL_NEAREST				0x2600
#define GL_LINEAR				0x2601

#define GL_TEXTURE_MAG_FILTER	0x2800
#define GL_TEXTURE_MIN_FILTER	0x2801
#define GL_TEXTURE_WRAP_S		0x2802
#define GL_TEXTURE_WRAP_T		0x2803

#define GL_REPEAT				0x2901

#define GL_COLOR_BUFFER_BIT		0x4000

#define GL_CLAMP_TO_EDGE		0x812F

#define GL_TEXTURE0				0x84C0

#define GL_ARRAY_BUFFER			0x8892

#define GL_STATIC_DRAW			0x88E4

#define GL_COMPILE_STATUS		0x8B81

#define GL_FRAGMENT_SHADER		0x8B30
#define GL_VERTEX_SHADER		0x8B31

#define GL_LINK_STATUS			0x8B82

#define GL_LIST \
	GL_FUNCTION(unsigned int,	glCreateShader,				int shaderType) \
	GL_FUNCTION(void,			glShaderSource,				unsigned int shader, int count, const char **string, const int *length) \
	GL_FUNCTION(void,			glCompileShader,			unsigned int shader) \
	GL_FUNCTION(void,			glGetShaderiv,				unsigned int shader, int pname, int *params) \
	GL_FUNCTION(void,			glGetShaderInfoLog,			unsigned int shader, int maxLength, int *length, char *infoLog) \
	GL_FUNCTION(unsigned int,	glCreateProgram) \
	GL_FUNCTION(void,			glAttachShader,				unsigned int program, unsigned int shader) \
	GL_FUNCTION(void,			glLinkProgram,				unsigned int program) \
	GL_FUNCTION(void,			glGetProgramiv,				unsigned int program, int pname, int *params) \
	GL_FUNCTION(void,			glGetProgramInfoLog,		unsigned int program, int maxLength, int *length, char *infoLog) \
	GL_FUNCTION(void,			glActiveTexture,			int texture)	\
	GL_FUNCTION(void,			glBindBuffer,				int target, unsigned int buffer) \
	GL_FUNCTION(void,			glBindVertexArray,			unsigned int array) \
	GL_FUNCTION(void,			glBufferData,				int target, ptrdiff_t size, const void *data, int usage) \
	GL_FUNCTION(void,			glEnableVertexAttribArray,	unsigned int index) \
	GL_FUNCTION(void,			glGenBuffers,				int n, unsigned int *buffers) \
	GL_FUNCTION(void,			glGenVertexArrays,			int n, unsigned int *arrays) \
	GL_FUNCTION(int,			glGetUniformLocation,		unsigned int program,	const char *name) \
	GL_FUNCTION(void,			glUniform1i,				int location,	int v0) \
	GL_FUNCTION(void,			glUniform1f,				int location,	float v0) \
	GL_FUNCTION(void,			glUniform3f,				int location, float v0, float v1, float v2) \
	GL_FUNCTION(void,			glUniformMatrix4fv,			int location, int count, unsigned char transpose, const float *value) \
	GL_FUNCTION(void,			glUseProgram,				unsigned int program) \
	GL_FUNCTION(void,			glVertexAttribPointer,		unsigned int index, int size, int type, unsigned char normalized, int stride, const void *pointer) \
	GL_FUNCTION(void,			glBindTexture,				int target, unsigned int texture) \
	GL_FUNCTION(void,			glBlendFunc,				int sfactor,	int dfactor) \
	GL_FUNCTION(void,			glClear,					int mask) \
	GL_FUNCTION(void,			glClearColor,				float red, float green, float blue, float alpha) \
	GL_FUNCTION(void,			glDrawArrays,				int mode, int first, int count) \
	GL_FUNCTION(void,			glEnable,					int cap) \
	GL_FUNCTION(void,			glDisable,					int cap) \
	GL_FUNCTION(void,			glGenTextures,				int n, unsigned int *textures) \
	GL_FUNCTION(void,			glPixelStorei,				int pname, int param) \
	GL_FUNCTION(void,			glTexImage2D,				int target, int level,	int internalFormat, int width, int height, int border, int format, int type, const void *data) \
	GL_FUNCTION(void,			glTexParameteri,			int target, int pname, int param) \
	GL_FUNCTION(void,			glViewport,					int x, int y, int width, int height) \
	GL_FUNCTION(int,			glGetError) \
	GL_FUNCTION(void,			glGetIntegerv,				int pname, int *params) \
	GL_FUNCTION(void,			glScissor,					int x, int y, unsigned int width, unsigned int height) \


#define GL_CALL_CONVENTION __stdcall

#define GL_FUNCTION(ret, name, ...) extern ret (GL_CALL_CONVENTION *name)(__VA_ARGS__);
	GL_LIST
#undef GL_FUNCTION

#endif /* end of include guard: GL_H */
