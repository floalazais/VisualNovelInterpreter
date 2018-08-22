#include "gl.h"
#include "graphics.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "globals.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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

void add_sprite_to_draw_list(Sprite *sprite, DrawLayer drawLayer)
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

void add_text_to_draw_list(Text text, DrawLayer drawLayer)
{
	if (drawLayer == DRAW_LAYER_BACKGROUND)
	{
		for (int i = 0; i < buf_len(text); i++)
		{
			buf_add(backgroundSprites, text[i]);
		}
	} else if (drawLayer == DRAW_LAYER_MIDDLEGROUND) {
		for (int i = 0; i < buf_len(text); i++)
		{
			buf_add(middlegroundSprites, text[i]);
		}
	} else if (drawLayer == DRAW_LAYER_FOREGROUND) {
		for (int i = 0; i < buf_len(text); i++)
		{
			buf_add(foregroundSprites, text[i]);
		}
	} else if (drawLayer == DRAW_LAYER_UI) {
		for (int i = 0; i < buf_len(text); i++)
		{
			buf_add(UISprites, text[i]);
		}
	} else {
		error("unsupported draw layer %d.", drawLayer);
	}
}

Sprite *create_color_sprite(vec2 position, float width, float height, vec3 color)
{
	Sprite *sprite = xmalloc(sizeof (*sprite));
	sprite->type = SPRITE_COLOR;
	ivec2 pixelPosition = {.x = position.x * windowDimensions.x, .y = position.y * windowDimensions.y};
	sprite->position = pixelPosition;
	sprite->width = width * windowDimensions.x;
	sprite->height = height * windowDimensions.y;
	sprite->color = color;
	return sprite;
}

Sprite *create_texture_sprite(vec2 position, float width, float height, char *texturePath)
{
	Sprite *sprite = xmalloc(sizeof (*sprite));
	sprite->type = SPRITE_TEXTURE;
	ivec2 pixelPosition = {.x = position.x * windowDimensions.x, .y = position.y * windowDimensions.y};
	sprite->position = pixelPosition;
	sprite->width = width * windowDimensions.x;
	sprite->height = height * windowDimensions.y;

	unsigned int textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int textureWidth;
	int textureHeight;
	int nrChannels;
    unsigned char *data = stbi_load(texturePath, &textureWidth, &textureHeight, &nrChannels, 4);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        sprite->textureId = textureId;
    } else {
		error("failed to load texture %s.", texturePath);
    }
	return sprite;
}

Sprite *create_glyph(ivec2 position, int width, int height, unsigned int textureId, vec3 color)
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

#define MAXUNICODE	0x10FFFF

static int offset;

static int utf8_decode(const char *o)
{
	static const unsigned int limits[] = {0xFF, 0x7F, 0x7FF, 0xFFFF};
	const unsigned char *s = (const unsigned char *)o;
	unsigned int c = s[0];
	unsigned int res = 0;  /* final result */
	if (c < 0x80)  /* ascii? */
	{
		res = c;
		offset = 1;
	} else {
		int count = 0;  /* to count number of continuation bytes */
		while (c & 0x40) {  /* still have continuation bytes? */
			int cc = s[++count];  /* read next byte */
			if ((cc & 0xC0) != 0x80)  /* not a continuation byte? */
				return -1;  /* invalid byte sequence */
			res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
			c <<= 1;  /* to test next bit */
		}
		res |= ((c & 0x7F) << (count * 5));  /* add first byte */
		if (count > 3 || res > MAXUNICODE || res <= limits[count])
			return -1;  /* invalid byte sequence */
		offset = count + 1;  /* skip continuation bytes read and first byte */
	}
	return res;
}

Text create_text(vec2 position, int textHeight, char *string, char *fontPath, vec3 color)
{
	Text text = NULL;
	stbtt_fontinfo font;
	unsigned char *ttf_buffer = (unsigned char *)file_to_string(fontPath);
	stbtt_InitFont(&font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
	float scale = stbtt_ScaleForPixelHeight(&font, textHeight);
	int ascent;
	int descent;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, NULL);
    ascent *= scale;
	descent *= scale;
	ivec2 glyphPosition;
	int x = 0;
    while (*string != '\0')
    {
		int code = utf8_decode(string);
		string += offset;
		int x0;
		int y0;
		int x1;
		int y1;
		stbtt_GetCodepointBitmapBox(&font, code, scale, scale, &x0, &y0, &x1, &y1);
		int w = x1 - x0;
		int h = y1 - y0;

		unsigned char *bitmap = xmalloc(h * w * sizeof (*bitmap));
		stbtt_MakeCodepointBitmap(&font, bitmap, w, h, w, scale, scale, code);

		unsigned int glyphTextureId;
		glGenTextures(1, &glyphTextureId);
		glBindTexture(GL_TEXTURE_2D, glyphTextureId);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
		glGenerateMipmap(GL_TEXTURE_2D);
		free(bitmap);

		glyphPosition.x = position.x * windowDimensions.x + x;
		glyphPosition.y = position.y * windowDimensions.y + y0 + ascent;

		int advance;
		stbtt_GetCodepointHMetrics(&font, code, &advance, NULL);
		x += (advance + stbtt_GetCodepointKernAdvance(&font, code, utf8_decode(string))) * scale + 2;

		Sprite *glyph = create_glyph(glyphPosition, w, h, glyphTextureId, color);
		buf_add(text, glyph);
    }
	return text;
}

static int get_uniform_location(unsigned shaderProgramId, const char *uniformName)
{
    int location = glGetUniformLocation(shaderProgramId, uniformName);
    if (location == -1)
    {
        printf("uniform %s not found in shader.", uniformName);
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

static void shader_set_mat4_uniform(unsigned int shaderProgramId, char *uniformName, mat4 *value)
{
	glUniformMatrix4fv(get_uniform_location(shaderProgramId, uniformName), 1, GL_FALSE, &(value->elements[0][0]));
}

static void draw(Sprite *sprite)
{
	mat4 model = mat4_identity();
	model = mat4_translate(&model, sprite->position);
	model = mat4_scale(&model, sprite->width, sprite->height);

	if (sprite->type == SPRITE_COLOR)
	{
		glUseProgram(colorShaderProgramId);
		shader_set_mat4_uniform(colorShaderProgramId, "projection", &projection);
		shader_set_mat4_uniform(colorShaderProgramId, "model", &model);
		shader_set_color_uniform(colorShaderProgramId, "color", sprite->color);
	} else if (sprite->type == SPRITE_TEXTURE) {
		glUseProgram(textureShaderProgramId);
		shader_set_mat4_uniform(textureShaderProgramId, "projection", &projection);
		shader_set_mat4_uniform(textureShaderProgramId, "model", &model);
		glActiveTexture(GL_TEXTURE0);
		shader_set_int_uniform(textureShaderProgramId, "textureId", 0);
		glBindTexture(GL_TEXTURE_2D, sprite->textureId);
	} else if (sprite->type == SPRITE_GLYPH) {
		glUseProgram(glyphShaderProgramId);
		shader_set_mat4_uniform(glyphShaderProgramId, "projection", &projection);
		shader_set_mat4_uniform(glyphShaderProgramId, "model", &model);
		glActiveTexture(GL_TEXTURE0);
	    shader_set_int_uniform(glyphShaderProgramId, "glyphTextureId", 0);
	    glBindTexture(GL_TEXTURE_2D, sprite->textureId);
		shader_set_color_uniform(glyphShaderProgramId, "textColor", sprite->color);
	} else {
		error("unsupported sprite type %d.", sprite->type);
	}
	glBindVertexArray(VAO);

	glDrawArrays(GL_TRIANGLES, 0, 6);
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
