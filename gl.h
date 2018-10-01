#ifndef GL_H
#define GL_H

#include <stddef.h>

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#define GL_TRUE                           1
#define GL_FALSE                          0

#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_2_BYTES                        0x1407
#define GL_3_BYTES                        0x1408
#define GL_4_BYTES                        0x1409
#define GL_DOUBLE                         0x140A

#define GL_RED                            0x1903
#define GL_RGBA                           0x1908

#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505

#define GL_TRIANGLES                      0x0004

#define GL_ARRAY_BUFFER                   0x8892
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_COMPILE_STATUS                 0x8B81
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506
#define GL_LINK_STATUS                    0x8B82
#define GL_STATIC_DRAW                    0x88E4
#define GL_TEXTURE0                       0x84C0
#define GL_VERTEX_SHADER                  0x8B31
#define GL_LINEAR                         0x2601
#define GL_LINE_STRIP                     0x0003
#define GL_TEXTURE_2D                     0x0DE1
#define GL_NEAREST                        0x2600
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_REPEAT                         0x2901
#define GL_BLEND                          0x0BE2
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_UNPACK_ALIGNMENT               0x0CF5

#define GL_LIST \
	GL_FUNCTION(GLuint, glCreateShader,				GLenum shaderType) \
	GL_FUNCTION(void,	glShaderSource,				GLuint shader, GLsizei count, const GLchar** string, const GLint* length) \
	GL_FUNCTION(void,	glCompileShader,			GLuint shader) \
	GL_FUNCTION(void,	glGetShaderiv,				GLuint shader, GLenum pname, GLint* params) \
	GL_FUNCTION(void,	glGetShaderInfoLog,			GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog) \
	GL_FUNCTION(void,	glDeleteShader,				GLuint shader) \
	GL_FUNCTION(GLuint, glCreateProgram) \
	GL_FUNCTION(void,	glAttachShader,				GLuint program, GLuint shader) \
	GL_FUNCTION(void,	glLinkProgram,				GLuint program) \
	GL_FUNCTION(void,	glGetProgramiv,				GLuint program, GLenum pname, GLint* params) \
	GL_FUNCTION(void,	glDeleteProgram,			GLuint program) \
	GL_FUNCTION(void,	glGetProgramInfoLog,		GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) \
	GL_FUNCTION(void,	glDetachShader,				GLuint program, GLuint shader) \
	GL_FUNCTION(void,	glActiveTexture,			GLenum texture)	\
	GL_FUNCTION(void,	glBindBuffer,				GLenum target, GLuint buffer) \
	GL_FUNCTION(void,	glBindVertexArray,  		GLuint array) \
	GL_FUNCTION(void,	glBufferData,				GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage) \
	GL_FUNCTION(void,	glBufferSubData,			GLenum target, GLintptr offset,	GLsizeiptr size, const GLvoid* data) \
	GL_FUNCTION(void,	glDeleteBuffers,  			GLsizei n, const GLuint* buffers) \
	GL_FUNCTION(void,	glDeleteVertexArrays,		GLsizei n, const GLuint* arrays) \
	GL_FUNCTION(void,	glEnableVertexAttribArray,	GLuint index) \
	GL_FUNCTION(void,	glGenBuffers,				GLsizei n, GLuint* buffers) \
	GL_FUNCTION(void,	glGenVertexArrays,			GLsizei n, GLuint* arrays) \
	GL_FUNCTION(GLint,	glGetUniformLocation,		GLuint program,	const GLchar *name) \
	GL_FUNCTION(void,	glUniform1i,				GLint location,	GLint v0) \
	GL_FUNCTION(void,	glUniform1f,				GLint location,	GLfloat v0) \
	GL_FUNCTION(void,	glUniform3f,				GLint location, GLfloat v0, GLfloat v1, GLfloat v2) \
	GL_FUNCTION(void,	glUniformMatrix4fv,			GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) \
	GL_FUNCTION(void,	glUseProgram,				GLuint program) \
	GL_FUNCTION(void,	glVertexAttribPointer,		GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* pointer) \
	GL_FUNCTION(void,	glBindTexture,				GLenum target, GLuint texture) \
	GL_FUNCTION(void,	glBlendFunc, 				GLenum sfactor,	GLenum dfactor) \
	GL_FUNCTION(void,	glClear,					GLbitfield mask) \
	GL_FUNCTION(void,	glClearColor, 				GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) \
	GL_FUNCTION(void,	glDeleteTextures,			GLsizei n, const GLuint* textures) \
	GL_FUNCTION(void,	glDisable,					GLenum cap) \
	GL_FUNCTION(void,	glDrawArrays, 				GLenum mode, GLint first, GLsizei count) \
	GL_FUNCTION(void,	glDrawElements, 			GLenum mode, GLsizei count, GLenum type, const GLvoid* indices) \
	GL_FUNCTION(void,	glEnable,					GLenum  cap) \
	GL_FUNCTION(void,	glGenTextures,				GLsizei n, GLuint* textures) \
	GL_FUNCTION(GLenum, glGetError) \
	GL_FUNCTION(void,	glPixelStorei,				GLenum pname, GLint param) \
	GL_FUNCTION(void,	glTexImage2D, 				GLenum target, GLint level,	GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* data) \
	GL_FUNCTION(void,	glTexParameteri, 			GLenum target, GLenum pname, GLint param) \
	GL_FUNCTION(void,	glViewport,					GLint x, GLint y, GLsizei width, GLsizei height) \
	GL_FUNCTION(void,	glGenerateMipmap,			GLenum target)

#define GL_CALL_CONVENTION __stdcall

#define GL_FUNCTION(ret, name, ...) extern ret (GL_CALL_CONVENTION *name)(__VA_ARGS__);
	GL_LIST
#undef GL_FUNCTION

#endif /* end of include guard: GL_H */
