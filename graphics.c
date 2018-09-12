#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "gl.h"
#include "graphics.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "globals.h"
#include "stb_image.h"

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

	unsigned int vertexShaderId = compile_shader("Shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
	unsigned int colorFragmentShaderId = compile_shader("Shaders/color_fragment_shader.glsl", GL_FRAGMENT_SHADER);
	unsigned int textureFragmentShaderId = compile_shader("Shaders/texture_fragment_shader.glsl", GL_FRAGMENT_SHADER);
	unsigned int glyphFragmentShaderId = compile_shader("Shaders/glyph_fragment_shader.glsl", GL_FRAGMENT_SHADER);

	colorShaderProgramId = link_shaders(vertexShaderId, colorFragmentShaderId);
	textureShaderProgramId = link_shaders(vertexShaderId, textureFragmentShaderId);
	glyphShaderProgramId = link_shaders(vertexShaderId, glyphFragmentShaderId);
}

unsigned int get_texture_id_from_path(char *texturePath)
{
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
		return textureId;
    } else {
		error("failed to load texture %s.", texturePath);
    }
}

unsigned int get_texture_id_from_path_ex(char *texturePath, int *width, int *height)
{
	unsigned int textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int nrChannels;
    unsigned char *data = stbi_load(texturePath, width, height, &nrChannels, 4);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
		return textureId;
    } else {
		error("failed to load texture %s.", texturePath);
    }
}

Sprite *create_sprite(SpriteType spriteType)
{
	Sprite *sprite = xmalloc(sizeof (*sprite));
	if (spriteType == SPRITE_COLOR)
	{
		sprite->type = SPRITE_COLOR;
	} else if (spriteType == SPRITE_TEXTURE) {
		sprite->type = SPRITE_TEXTURE;
	} else if (SPRITE_GLYPH) {
		sprite->type = SPRITE_GLYPH;
	} else {
		error("sprite type %d not supported.", spriteType);
	}
	return sprite;
}

static Font **fonts = NULL;

static void load_glyph(Font *font, int code)
{
	int leftSideBearing;
	int advance;
	int x0;
	int y0;
	int x1;
	int y1;
	stbtt_GetCodepointBitmapBox(&font->fontInfo, code, font->scale, font->scale, &x0, &y0, &x1, &y1);
	font->glyphs[code] = xmalloc(sizeof (*font->glyphs[code]));
	font->glyphs[code]->width = x1 - x0;
	font->glyphs[code]->height = y1 - y0;

	unsigned char *bitmap = xmalloc(font->glyphs[code]->height * font->glyphs[code]->width * sizeof (*bitmap));
	stbtt_MakeCodepointBitmap(&font->fontInfo, bitmap, font->glyphs[code]->width, font->glyphs[code]->height, font->glyphs[code]->width, font->scale, font->scale, code);

	glGenTextures(1, &font->glyphs[code]->textureId);
	glBindTexture(GL_TEXTURE_2D, font->glyphs[code]->textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->glyphs[code]->width, font->glyphs[code]->height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
	glGenerateMipmap(GL_TEXTURE_2D);

	free(bitmap);

	font->glyphs[code]->yOffset = y0 + font->ascent - font->descent;

	stbtt_GetCodepointHMetrics(&font->fontInfo, code, &advance, &leftSideBearing);
	font->glyphs[code]->xOffset = (int)((advance - leftSideBearing) * font->scale);

	font->loaded[code] = true;
}

static void load_font(char *fontPath, int textHeight)
{
	stbtt_fontinfo fontInfo;
	unsigned char *ttf_buffer = (unsigned char *)file_to_string(fontPath);
	stbtt_InitFont(&fontInfo, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer, 0));
	int ascent;
	int descent;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, NULL);

	Font *font = xmalloc(sizeof (*font));
	font->glyphs = xmalloc(0xFFFF * sizeof (*font->glyphs));
	font->loaded = xmalloc(0xFFFF * sizeof (*font->loaded));
	for (int i = 0; i < 0xFFFF; i++)
	{
		font->loaded[i] = false;
	}
	font->fontInfo = fontInfo;
	font->fontPath = xmalloc(strlen(fontPath) * sizeof (*font->fontPath));
	for (unsigned int i = 0; i < strlen(fontPath); i++)
	{
		font->fontPath[i] = fontPath[i];
	}
	font->height = textHeight;
	font->scale = stbtt_ScaleForPixelHeight(&font->fontInfo, (float)textHeight);
    font->ascent = (int)(ascent * font->scale);
	font->descent = (int)(descent * font->scale);
    for (int code = 0; code < 128; code++)
    {
		load_glyph(font, code);
	}

	buf_add(fonts, font);
}

static int offset;

#define MAXUNICODE 0x10FFFF
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
		while (c & 0x40)
		{  /* still have continuation bytes? */
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

Text *create_text()
{
	Text *text = xmalloc(sizeof (*text));
	text->font = NULL;
	text->string = NULL;
	text->sprites = NULL;
	return text;
}

static void update_text(Text *text)
{
	text->width = 0;
	char *cursor = text->string;
	int count = 0;
    while (*cursor != '\0')
    {
		int code = utf8_decode(cursor);
		if (code == -1)
		{
			error("found incorrect utf-8 sequence in %s.", text->string);
		} else if (code > 0xFFFF) {
			error("unsupported character code %d.", code);
		}
		cursor += offset;

		if (!text->font->loaded[code])
		{
			load_glyph(text->font, code);
		}

		if (count == buf_len(text->sprites))
		{
			buf_add(text->sprites, create_sprite(SPRITE_GLYPH));
		}

		text->sprites[count]->width = text->font->glyphs[code]->width;
		text->sprites[count]->height = text->font->glyphs[code]->height;
		text->sprites[count]->textureId = text->font->glyphs[code]->textureId;
		text->sprites[count]->color = text->color;

		text->sprites[count]->position.x = text->position.x + text->width;
		text->sprites[count]->position.y = text->position.y + text->font->glyphs[code]->yOffset;
		text->width += text->font->glyphs[code]->xOffset + (int)(stbtt_GetCodepointKernAdvance(&text->font->fontInfo, code, utf8_decode(cursor)) * text->font->scale);

		count++;
    }
}

void set_position_to_text(Text *text, ivec2 position)
{
	text->position = position;
	if (text->string)
	{
		update_text(text);
	}
}

void set_font_to_text(Text *text, char *fontPath, int textHeight)
{
	text->font = NULL;
	for (unsigned int i = 0; i < buf_len(fonts); i++)
	{
		if (strmatch(fonts[i]->fontPath, fontPath) && fonts[i]->height == textHeight)
		{
			text->font = fonts[i];
			break;
		}
	}
	if (!text->font)
	{
		if (textHeight == TEXT_SIZE_SMALL)
		{
			load_font(fontPath, TEXT_SIZE_SMALL);
		} else if (textHeight == TEXT_SIZE_NORMAL) {
			load_font(fontPath, TEXT_SIZE_NORMAL);
		} else if (textHeight == TEXT_SIZE_BIG) {
			load_font(fontPath, TEXT_SIZE_BIG);
		} else if (textHeight == TEXT_SIZE_HUGE) {
			load_font(fontPath, TEXT_SIZE_HUGE);
		} else {
			error("unsupported text height %d.", textHeight);
		}
		text->font = fonts[buf_len(fonts) - 1];
	}

	text->height = text->font->ascent - text->font->descent;

	if (text->string)
	{
		update_text(text);
	}
}

void set_string_to_text(Text *text, char *string)
{
	buf_free(text->string);
	text->string = NULL;
	for (unsigned int i = 0; i < strlen(string); i++)
	{
		buf_add(text->string, string[i]);
	}
	buf_add(text->string, '\0');
	text->nbCharToDisplay = strlen(string);

	if (text->string)
	{
		update_text(text);
	}
}

void free_text(Text *text)
{
	buf_free(text->string);
	buf_free(text->sprites);
	for (unsigned int i = 0; i < buf_len(text->sprites); i++)
	{
		free(text->sprites[i]);
	}
	buf_free(text);
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

static void set_int_uniform_to_shader(unsigned int shaderProgramId, char *uniformName, int value)
{
	glUniform1i(get_uniform_location(shaderProgramId, uniformName), value);
}

static void set_color_uniform_to_shader(unsigned int shaderProgramId, char *uniformName, vec3 value)
{
	glUniform3f(get_uniform_location(shaderProgramId, uniformName), value.x, value.y, value.z);
}

static void set_mat4_uniform_to_shader(unsigned int shaderProgramId, char *uniformName, mat4 *value)
{
	glUniformMatrix4fv(get_uniform_location(shaderProgramId, uniformName), 1, GL_FALSE, &(value->elements[0][0]));
}

static void draw(Sprite *sprite)
{
	mat4 model = mat4_identity();
	model = mat4_translate(&model, sprite->position);
	model = mat4_scale(&model, (float)sprite->width, (float)sprite->height);

	if (sprite->type == SPRITE_COLOR)
	{
		glUseProgram(colorShaderProgramId);
		set_mat4_uniform_to_shader(colorShaderProgramId, "projection", &projection);
		set_mat4_uniform_to_shader(colorShaderProgramId, "model", &model);
		set_color_uniform_to_shader(colorShaderProgramId, "color", sprite->color);
	} else if (sprite->type == SPRITE_TEXTURE) {
		if (sprite->textureId == -1)
		{
			return;
		}
		glUseProgram(textureShaderProgramId);
		set_mat4_uniform_to_shader(textureShaderProgramId, "projection", &projection);
		set_mat4_uniform_to_shader(textureShaderProgramId, "model", &model);
		glActiveTexture(GL_TEXTURE0);
		set_int_uniform_to_shader(textureShaderProgramId, "textureId", 0);
		glBindTexture(GL_TEXTURE_2D, sprite->textureId);
	} else if (sprite->type == SPRITE_GLYPH) {
		if (sprite->textureId == -1)
		{
			return;
		}
		glUseProgram(glyphShaderProgramId);
		set_mat4_uniform_to_shader(glyphShaderProgramId, "projection", &projection);
		set_mat4_uniform_to_shader(glyphShaderProgramId, "model", &model);
		glActiveTexture(GL_TEXTURE0);
	    set_int_uniform_to_shader(glyphShaderProgramId, "glyphTextureId", 0);
	    glBindTexture(GL_TEXTURE_2D, sprite->textureId);
		set_color_uniform_to_shader(glyphShaderProgramId, "textColor", sprite->color);
	} else {
		error("unsupported sprite type %d.", sprite->type);
	}
	glBindVertexArray(VAO);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void draw_all()
{
	for (unsigned int i = 0; i < buf_len(backgroundSprites); i++)
	{
		draw(backgroundSprites[i]);
	}
	buf_free(backgroundSprites);
	backgroundSprites = NULL;
	for (unsigned int i = 0; i < buf_len(middlegroundSprites); i++)
	{
		draw(middlegroundSprites[i]);
	}
	buf_free(middlegroundSprites);
	middlegroundSprites = NULL;
	for (unsigned int i = 0; i < buf_len(foregroundSprites); i++)
	{
		draw(foregroundSprites[i]);
	}
	buf_free(foregroundSprites);
	foregroundSprites = NULL;
	for (unsigned int i = 0; i < buf_len(UISprites); i++)
	{
		draw(UISprites[i]);
	}
	buf_free(UISprites);
	UISprites = NULL;
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

void add_text_to_draw_list(Text *text, DrawLayer drawLayer)
{
	if (text->string)
	{
		if (drawLayer == DRAW_LAYER_BACKGROUND)
		{
			for (int i = 0; i < text->nbCharToDisplay; i++)
			{
				buf_add(backgroundSprites, text->sprites[i]);
			}
		} else if (drawLayer == DRAW_LAYER_MIDDLEGROUND) {
			for (int i = 0; i < text->nbCharToDisplay; i++)
			{
				buf_add(middlegroundSprites, text->sprites[i]);
			}
		} else if (drawLayer == DRAW_LAYER_FOREGROUND) {
			for (int i = 0; i < text->nbCharToDisplay; i++)
			{
				buf_add(foregroundSprites, text->sprites[i]);
			}
		} else if (drawLayer == DRAW_LAYER_UI) {
			for (int i = 0; i < text->nbCharToDisplay; i++)
			{
				buf_add(UISprites, text->sprites[i]);
			}
		} else {
			error("unsupported draw layer %d.", drawLayer);
		}
	}
}
