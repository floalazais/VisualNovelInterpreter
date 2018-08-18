#include "gl.h"
#include "graphics.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "globals.h"

#include <stdio.h>

static Sprite **backgroundSprites = NULL;
static Sprite **middlegroundSprites = NULL;
static Sprite **foregroundSprites = NULL;
static Sprite **UISprites = NULL;

static unsigned int VAO;
static unsigned int VBO;
static const float QUAD[24] =
{
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f, 0.0f
};

static int colorShaderProgramId;
static int textureShaderProgramId;
static int glyphShaderProgramId;

static unsigned int compile_shader(char *path, GLenum shaderType)
{
	const char *code = file_to_string(path);

    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    char infoLog[512];
    if(!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        error("compilation of shader file \"%s\" failed :\n%s", path, infoLog);
    }
	return shader;
}

static unsigned int link_shaders(unsigned int vertexShaderId, unsigned int fragmentShaderId)
{
	unsigned int shaderProgramId = glCreateProgram();
    glAttachShader(shaderProgramId, vertexShaderId);
    glAttachShader(shaderProgramId, fragmentShaderId);
    glLinkProgram(shaderProgramId);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(shaderProgramId, 512, NULL, infoLog);
        error("linking of shader program failed :\n%s", infoLog);
    }
    return shaderProgramId;
}

void init_graphics()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof (GLfloat) * 24, QUAD, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (GLfloat), (GLvoid*)(2 * sizeof (GLfloat)));

	unsigned int vertexShaderId = compile_shader("vertex_shader.glsl", GL_VERTEX_SHADER);
	unsigned int colorFragmentShaderId = compile_shader("color_fragment_shader.glsl", GL_FRAGMENT_SHADER);
	unsigned int textureFragmentShaderId = compile_shader("texture_fragment_shader.glsl", GL_FRAGMENT_SHADER);
	unsigned int glyphFragmentShaderId = compile_shader("glyph_fragment_shader.glsl", GL_FRAGMENT_SHADER);

	colorShaderProgramId = link_shaders(vertexShaderId, colorFragmentShaderId);
	textureShaderProgramId = link_shaders(vertexShaderId, textureFragmentShaderId);
	glyphShaderProgramId = link_shaders(vertexShaderId, glyphFragmentShaderId);
}

void add_to_draw_list(Sprite *sprite, DrawLayer drawLayer)
{
	if (drawLayer == DRAW_LAYER_BACKGROUND)
	{
		buf_add(backgroundSprites, sprite);
	} else if (drawLayer == DRAW_LAYER_MIDDLEGROUND) {
		buf_add(middlegroundSprites, sprite);
	} else if (drawLayer == DRAW_LAYER_FOREGROUND) {
		buf_add(foregroundSprites, sprite);
	} else if (drawLayer == DRAW_LAYER_UI) {
		buf_add(UISprites, sprite);
	} else {
		error("unsupported draw layer %d.", drawLayer);
	}
}

Sprite *create_color_sprite(vec2 position, int width, int height, vec3 color)
{
	Sprite *sprite = xmalloc(sizeof (*sprite));
	sprite->type = SPRITE_COLOR;
	sprite->position = position;
	sprite->width = width;
	sprite->height = height;
	sprite->color = color;
	return sprite;
}

Sprite *create_texture_sprite(vec2 position, int width, int height, int textureId)
{
	Sprite *sprite = xmalloc(sizeof (*sprite));
	sprite->type = SPRITE_TEXTURE;
	sprite->position = position;
	sprite->width = width;
	sprite->height = height;
	sprite->textureId = textureId;
	return sprite;
}

Sprite *create_glyph_sprite(vec2 position, int width, int height, int textureId, vec3 color)
{
	Sprite *sprite = xmalloc(sizeof (*sprite));
	sprite->type = SPRITE_GLYPH;
	sprite->position = position;
	sprite->width = width;
	sprite->height = height;
	sprite->textureId = textureId;
	sprite->color = color;
	return sprite;
}

static int get_uniform_location(unsigned shaderProgramId, const char *uniformName)
{
    int location = glGetUniformLocation(shaderProgramId, uniformName);
    if (location == -1)
    {
        error("uniform %s not found in shader.", uniformName);
    }
    return location;
}

static void shader_set_int_uniform(unsigned int shaderProgramId, char *uniformName, int value)
{
	glUniform1i(get_uniform_location(shaderProgramId, uniformName), value);
}

static void shader_set_color_uniform(unsigned int shaderProgramId, char *uniformName, vec3 value)
{
	glUniform3f(get_uniform_location(shaderProgramId, uniformName), value.x, value.y, value.z);
}

static void shader_set_mat4_uniform(unsigned int shaderProgramId, char *uniformName, mat4 value)
{
	glUniformMatrix4fv(get_uniform_location(shaderProgramId, uniformName), 1, GL_FALSE, &(value.elements[0]));
}

static void draw(Sprite *sprite)
{
	ivec2 position = {.x = sprite->position.x * windowDimensions.x, .y = sprite->position.y * windowDimensions.y};
	mat4 model = mat4_create();
	model = mat4_translate(model, position);
	model = mat4_scale(model, sprite->width, sprite->height);

	if (sprite->type == SPRITE_COLOR)
	{
		glUseProgram(colorShaderProgramId);
		shader_set_mat4_uniform(colorShaderProgramId, "projection", projection);
		shader_set_mat4_uniform(colorShaderProgramId, "model", model);
		shader_set_color_uniform(colorShaderProgramId, "color", sprite->color);
	} else if (sprite->type == SPRITE_TEXTURE) {
		glUseProgram(textureShaderProgramId);
		shader_set_mat4_uniform(textureShaderProgramId, "projection", projection);
		shader_set_mat4_uniform(textureShaderProgramId, "model", model);
		glActiveTexture(GL_TEXTURE0);
		shader_set_int_uniform(textureShaderProgramId, "textureId", 0);
		glBindTexture(GL_TEXTURE_2D, sprite->textureId);
	} else if (sprite->type == SPRITE_GLYPH) {
		glUseProgram(glyphShaderProgramId);
		shader_set_mat4_uniform(glyphShaderProgramId, "projection", projection);
		shader_set_mat4_uniform(glyphShaderProgramId, "model", model);
		glActiveTexture(GL_TEXTURE0);
	    shader_set_int_uniform(glyphShaderProgramId, "glyphTextureId", 0);
	    glBindTexture(GL_TEXTURE_2D, sprite->textureId);
		shader_set_color_uniform(glyphShaderProgramId, "textColor", sprite->color);
	} else {
		error("unsupported sprite type %d.", sprite->type);
	}
	glBindVertexArray(VAO);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf("%f ", projection.elements[i * 4 + j]);
		}
		printf("\n");
	}
	printf("\n");
}

void draw_all()
{
	for (int i = 0; i < buf_len(backgroundSprites); i++)
	{
		draw(backgroundSprites[i]);
	}
	buf_free(backgroundSprites);
	backgroundSprites = NULL;
	for (int i = 0; i < buf_len(middlegroundSprites); i++)
	{
		draw(middlegroundSprites[i]);
	}
	buf_free(middlegroundSprites);
	middlegroundSprites = NULL;
	for (int i = 0; i < buf_len(foregroundSprites); i++)
	{
		draw(foregroundSprites[i]);
	}
	buf_free(foregroundSprites);
	foregroundSprites = NULL;
	for (int i = 0; i < buf_len(UISprites); i++)
	{
		draw(UISprites[i]);
	}
	buf_free(UISprites);
	UISprites = NULL;
}
