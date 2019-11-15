#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include <stdbool.h>
#include <stddef.h>

#include "stb_image.h"
#include "stb_truetype.h"
#include "maths.h"
#include "globals.h"
#include "gl.h"
#include "error.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "token.h"
#include "lex.h"
#include "file.h"
#include "str.h"
#include "animation.h"
#include "graphics.h"

static buf(Sprite *) backgroundSprites;
static buf(Sprite *) middlegroundSprites;
static buf(Sprite *) foregroundSprites;
static buf(Sprite *) UISprites;
static buf(Sprite *) UISpritesScissor;

static buf(buf(char)) texturesPaths;
static buf(unsigned int) texturesIds;
static buf(int) texturesWidths;
static buf(int) texturesHeigts;

static buf(Font *) fonts;
static buf(unsigned char *) ttfBuffers;
static buf(buf(char)) ttfFilesPaths;

static unsigned int vao;
static unsigned int vbo;
static const float quad[24] =
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

static unsigned int compile_shader(const char *path, int shaderType)
{
	char *code = file_to_string(path);

	unsigned int shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, (const char **)&code, NULL);
	glCompileShader(shader);
	xfree(code);

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
	backgroundSprites = NULL;
	middlegroundSprites = NULL;
	foregroundSprites = NULL;
	UISprites = NULL;

	texturesPaths = NULL;
	texturesIds = NULL;
	texturesWidths = NULL;
	texturesHeigts = NULL;

	fonts = NULL;
	ttfBuffers = NULL;
	ttfFilesPaths = NULL;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof (float) * 24, quad, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof (float), (void*)(2 * sizeof (float)));

	glBindVertexArray(0);

	unsigned int vertexShaderId = compile_shader("Shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
	unsigned int colorFragmentShaderId = compile_shader("Shaders/color_fragment_shader.glsl", GL_FRAGMENT_SHADER);
	unsigned int textureFragmentShaderId = compile_shader("Shaders/texture_fragment_shader.glsl", GL_FRAGMENT_SHADER);
	unsigned int glyphFragmentShaderId = compile_shader("Shaders/glyph_fragment_shader.glsl", GL_FRAGMENT_SHADER);

	colorShaderProgramId = link_shaders(vertexShaderId, colorFragmentShaderId);
	textureShaderProgramId = link_shaders(vertexShaderId, textureFragmentShaderId);
	glyphShaderProgramId = link_shaders(vertexShaderId, glyphFragmentShaderId);
}

static void free_font(Font *font)
{
	buf_free(font->fontPath);
	xfree(font->fontInfo);
	for (unsigned int i = 0; i < 0xFFFF; i++)
	{
		if (font->loaded[i])
		{
			xfree(font->glyphs[i]);
		}
	}
	xfree(font->glyphs);
	xfree(font->loaded);
	xfree(font);
}

void free_graphics()
{
	buf_free(backgroundSprites);
	buf_free(middlegroundSprites);
	buf_free(foregroundSprites);
	buf_free(UISprites);
	buf_free(UISpritesScissor);

	for (unsigned int i = 0; i < buf_len(ttfBuffers); i++)
	{
		xfree(ttfBuffers[i]);
		buf_free(ttfFilesPaths[i]);
	}
	buf_free(ttfBuffers);
	buf_free(ttfFilesPaths);

	for (unsigned int i = 0; i < buf_len(texturesPaths); i++)
	{
		buf_free(texturesPaths[i]);
	}
	buf_free(texturesPaths);
	buf_free(texturesIds);
	buf_free(texturesWidths);
	buf_free(texturesHeigts);

	for (unsigned int i = 0; i < buf_len(fonts); i++)
	{
		free_font(fonts[i]);
	}
	buf_free(fonts);
}

unsigned int get_texture_id_from_path(const char *texturePath, int *_width, int *_height)
{
	for (unsigned int i = 0; i < buf_len(texturesPaths); i++)
	{
		if(strmatch(texturesPaths[i], texturePath))
		{
			if (_width)
			{
				*_width = texturesWidths[i];
			}
			if (_height)
			{
				*_height = texturesHeigts[i];
			}
			return texturesIds[i];
		}
	}

	unsigned int textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width;
	int height;
	int nrChannels;
	unsigned char *data = stbi_load(texturePath, &width, &height, &nrChannels, 4);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
		if (_width)
		{
			*_width = width;
		}
		if (_height)
		{
			*_height = height;
		}

		buf_add(texturesPaths, strclone(texturePath));
		buf_add(texturesIds, textureId);
		buf_add(texturesWidths, width);
		buf_add(texturesHeigts, height);

		return textureId;
	} else {
		error("failed to load texture %s.", texturePath);
	}
}

Sprite *create_sprite(SpriteType spriteType)
{
	Sprite *sprite = xmalloc(sizeof (*sprite));
	sprite->position.x = 0;
	sprite->position.y = 0;
	sprite->width = 0;
	sprite->height = 0;
	sprite->opacity = 1.0f;
	if (spriteType == SPRITE_COLOR)
	{
		sprite->type = SPRITE_COLOR;
		sprite->color.x = 0.0f;
		sprite->color.y = 0.0f;
		sprite->color.z = 0.0f;
	} else if (spriteType == SPRITE_TEXTURE) {
		sprite->type = SPRITE_TEXTURE;
		sprite->textureId = -1;
	} else if (spriteType == SPRITE_GLYPH) {
		sprite->type = SPRITE_GLYPH;
		sprite->color.x = 0.0f;
		sprite->color.y = 0.0f;
		sprite->color.z = 0.0f;
		sprite->textureId = -1;
	} else if (spriteType == SPRITE_ANIMATED) {
		sprite->type = SPRITE_ANIMATED;
		sprite->animations = NULL;
		sprite->currentAnimation = 0;
	} else {
		error("sprite type %d not supported.", spriteType);
	}
	return sprite;
}

void free_sprite(Sprite *sprite)
{
	if (sprite->type == SPRITE_COLOR)
	{
	} else if (sprite->type == SPRITE_TEXTURE) {
	} else if (sprite->type == SPRITE_GLYPH) {
	} else if (sprite->type == SPRITE_ANIMATED) {
		for (unsigned int i = 0; i < buf_len(sprite->animations); i++)
		{
			free_animation(sprite->animations[i]);
		}
		buf_free(sprite->animations);
	} else {
		error("sprite type %d not supported.", sprite->type);
	}
	xfree(sprite);
}

static void load_glyph(Font *font, int code)
{
	int leftSideBearing;
	int advance;
	int x0;
	int y0;
	int x1;
	int y1;
	stbtt_GetCodepointBitmapBox(font->fontInfo, code, font->scale, font->scale, &x0, &y0, &x1, &y1);
	font->glyphs[code] = xmalloc(sizeof (*font->glyphs[code]));
	font->glyphs[code]->width = x1 - x0;
	font->glyphs[code]->height = y1 - y0;

	unsigned char *bitmap = xmalloc(font->glyphs[code]->height * font->glyphs[code]->width * sizeof (*bitmap));
	stbtt_MakeCodepointBitmap(font->fontInfo, bitmap, font->glyphs[code]->width, font->glyphs[code]->height, font->glyphs[code]->width, font->scale, font->scale, code);

	unsigned int textureId;
	glGenTextures(1, &textureId);
	font->glyphs[code]->textureId = textureId;
	glBindTexture(GL_TEXTURE_2D, font->glyphs[code]->textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->glyphs[code]->width, font->glyphs[code]->height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);

	xfree(bitmap);

	font->glyphs[code]->yOffset = y0 + font->ascent - font->descent;

	stbtt_GetCodepointHMetrics(font->fontInfo, code, &advance, &leftSideBearing);
	font->glyphs[code]->xOffset = ceil((advance - leftSideBearing) * font->scale);

	font->loaded[code] = true;
}

static void load_font(const char *fontPath, int textHeight)
{
	stbtt_fontinfo *fontInfo = xmalloc(sizeof (*fontInfo));
	unsigned char *ttfBuffer = NULL;
	for (unsigned int i = 0; i < buf_len(ttfFilesPaths); i++)
	{
		if (strmatch(fontPath, ttfFilesPaths[i]))
		{
			ttfBuffer = ttfBuffers[i];
		}
	}
	if (!ttfBuffer)
	{
		ttfBuffer = (unsigned char *)file_to_string(fontPath);
		buf_add(ttfBuffers, ttfBuffer);
		buf_add(ttfFilesPaths, strclone(fontPath));
	}
	stbtt_InitFont(fontInfo, ttfBuffer, stbtt_GetFontOffsetForIndex(ttfBuffer, 0));
	int ascent;
	int descent;
	stbtt_GetFontVMetrics(fontInfo, &ascent, &descent, NULL);

	Font *font = xmalloc(sizeof (*font));
	font->glyphs = xmalloc(0xFFFF * sizeof (*font->glyphs));
	font->loaded = xmalloc(0xFFFF * sizeof (*font->loaded));
	for (int i = 0; i < 0xFFFF; i++)
	{
		font->loaded[i] = false;
	}
	font->fontInfo = fontInfo;
	font->fontPath = strclone(fontPath);
	font->height = textHeight;
	font->scale = stbtt_ScaleForPixelHeight(font->fontInfo, (float)textHeight);
	font->ascent = ceil(ascent * font->scale);
	font->descent = ceil(descent * font->scale);
	for (int code = 0; code < 128; code++)
	{
		load_glyph(font, code);
	}

	buf_add(fonts, font);
}

Text *create_text()
{
	Text *text = xmalloc(sizeof (*text));
	text->font = NULL;
	text->codes = NULL;
	text->sprites = NULL;
	text->widthLimit = -1;
	text->position.x = 0;
	text->position.y = 0;
	return text;
}

static void update_text(Text *text)
{
	text->width = 0;
	int currentLineWidth = 0;
	int currentLine = 1;
	int displayedSpriteCount = 0;

	for (unsigned int currentCodeIndex = 0; currentCodeIndex < buf_len(text->codes); currentCodeIndex++)
	{
		int currentCode = text->codes[currentCodeIndex];

		if (currentLineWidth == 0)
		{
			if (currentCode == ' ' && currentLine != 1)
			{
				continue;
			}
		}

		if (!text->font->loaded[currentCode])
		{
			load_glyph(text->font, currentCode);
		}

		if (displayedSpriteCount == buf_len(text->sprites))
		{
			buf_add(text->sprites, create_sprite(SPRITE_GLYPH));
		}

		Sprite *currentSprite = text->sprites[displayedSpriteCount];

		currentSprite->width = text->font->glyphs[currentCode]->width;
		currentSprite->height = text->font->glyphs[currentCode]->height;
		currentSprite->textureId = text->font->glyphs[currentCode]->textureId;
		currentSprite->color = text->color;

		currentSprite->position.x = text->position.x + currentLineWidth;
		currentSprite->position.y = text->position.y + (currentLine - 1) * (text->font->ascent - text->font->descent) + text->font->glyphs[currentCode]->yOffset;

		if (currentCodeIndex == buf_len(text->codes) - 1)
		{
			currentLineWidth += text->font->glyphs[currentCode]->xOffset + ceil(stbtt_GetCodepointKernAdvance(text->font->fontInfo, currentCode, 0) * text->font->scale);
		} else {
			currentLineWidth += text->font->glyphs[currentCode]->xOffset + ceil(stbtt_GetCodepointKernAdvance(text->font->fontInfo, currentCode, text->codes[currentCodeIndex + 1]) * text->font->scale);
		}

		displayedSpriteCount++;

		if (text->widthLimit != -1 && currentLineWidth > text->widthLimit)
		{
			int oldCodeIndex = currentCodeIndex;
			int oldSpriteCount = displayedSpriteCount;
			int oldLineWidth = currentLineWidth;
			bool newLine = false;
			while (true)
			{
				if (currentCodeIndex == 0)
				{
					currentCodeIndex = oldCodeIndex;
					displayedSpriteCount = oldSpriteCount;
					currentLineWidth = oldLineWidth;
					break;
				}

				if (currentCodeIndex == buf_len(text->codes) - 1)
				{
					currentLineWidth -= text->font->glyphs[text->codes[currentCodeIndex]]->xOffset + ceil(stbtt_GetCodepointKernAdvance(text->font->fontInfo, text->codes[currentCodeIndex], 0) * text->font->scale);
				} else {
					currentLineWidth -= text->font->glyphs[text->codes[currentCodeIndex]]->xOffset + ceil(stbtt_GetCodepointKernAdvance(text->font->fontInfo, text->codes[currentCodeIndex], text->codes[currentCodeIndex + 1]) * text->font->scale);
				}

				if (text->codes[currentCodeIndex] == ' ')
				{
					displayedSpriteCount++;
					newLine = true;
					break;
				}

				if (currentLineWidth <= 0)
				{
					currentCodeIndex = oldCodeIndex;
					displayedSpriteCount = oldSpriteCount;
					currentLineWidth = oldLineWidth;
					break;
				}

				currentCodeIndex--;
				displayedSpriteCount--;
			}
			if (newLine)
			{
				displayedSpriteCount--;
				if (currentLineWidth > text->width)
				{
					text->width = currentLineWidth;
				}
				currentLineWidth = 0;
				currentLine++;
			}
		}
	}
	if (currentLineWidth > text->width)
	{
		text->width = currentLineWidth;
	}
	text->height = currentLine * (text->font->ascent - text->font->descent) - 2;
	text->nbMaxCharToDisplay = displayedSpriteCount;
}

void set_text_width_limit(Text *text, int limit)
{
	text->widthLimit = limit;
	if (text->codes)
	{
		update_text(text);
	}
}

void set_text_position(Text *text, ivec2 position)
{
	text->position = position;
	text->position.y -= 2;
	if (text->codes)
	{
		update_text(text);
	}
}

void set_text_font(Text *text, const char *fontPath, int textHeight)
{
	text->font = NULL;
	for (unsigned int i = 0; i < buf_len(fonts); i++)
	{
		if (strmatch(fonts[i]->fontPath, fontPath))
		{
			if (fonts[i]->height == textHeight)
			{
				text->font = fonts[i];
				break;
			}
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

	text->height = text->font->ascent - text->font->descent - 2;

	if (text->codes)
	{
		update_text(text);
	}
}

void set_text_string(Text *text, const char *string)
{
	buf_free(text->codes);
	text->codes = NULL;

	if (string)
	{
		text->codes = utf8_decode(string);
	}

	if (text->codes)
	{
		update_text(text);
		text->nbCharToDisplay = text->nbMaxCharToDisplay;
	}
}

void free_text(Text *text)
{
	buf_free(text->codes);
	for (unsigned int i = 0; i < buf_len(text->sprites); i++)
	{
		free_sprite(text->sprites[i]);
	}
	buf_free(text->sprites);
	xfree(text);
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

static void set_int_uniform_to_shader(unsigned int shaderProgramId, const char *uniformName, int value)
{
	glUniform1i(get_uniform_location(shaderProgramId, uniformName), value);
}

static void set_float_uniform_to_shader(unsigned int shaderProgramId, const char *uniformName, float value)
{
	glUniform1f(get_uniform_location(shaderProgramId, uniformName), value);
}

static void set_color_uniform_to_shader(unsigned int shaderProgramId, const char *uniformName, vec3 value)
{
	glUniform3f(get_uniform_location(shaderProgramId, uniformName), value.x, value.y, value.z);
}

static void set_mat4_uniform_to_shader(unsigned int shaderProgramId, const char *uniformName, mat4 *value)
{
	glUniformMatrix4fv(get_uniform_location(shaderProgramId, uniformName), 1, GL_FALSE, &(value->e00));
}

static void draw(Sprite *sprite)
{
	AnimationPhase *currentAnimationPhase;
	if (sprite->type == SPRITE_ANIMATED)
	{
		currentAnimationPhase = sprite->animations[sprite->currentAnimation]->animationPhases[sprite->animations[sprite->currentAnimation]->currentAnimationPhase];
		if (!sprite->fixedSize)
		{
			if (currentAnimationPhase->responsiveHeight)
			{
				sprite->width = (int)(currentAnimationPhase->responsiveWidth * windowDimensions.x);
				sprite->height = (int)(currentAnimationPhase->responsiveHeight * windowDimensions.y);
			} else {
				sprite->width = currentAnimationPhase->pixelWidth;
				sprite->height = currentAnimationPhase->pixelHeight;
			}
		}
		update_animation(sprite->animations[sprite->currentAnimation]);
	}

	mat4 model = mat4_identity();
	model = mat4_translate(&model, sprite->position);
	model = mat4_scale(&model, (float)sprite->width, (float)sprite->height);

	if (sprite->type == SPRITE_COLOR)
	{
		glUseProgram(colorShaderProgramId);
		set_mat4_uniform_to_shader(colorShaderProgramId, "projection", &projection);
		set_mat4_uniform_to_shader(colorShaderProgramId, "model", &model);
		set_color_uniform_to_shader(colorShaderProgramId, "color", sprite->color);
		set_float_uniform_to_shader(colorShaderProgramId, "opacity", sprite->opacity);
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
		set_float_uniform_to_shader(textureShaderProgramId, "opacity", sprite->opacity);
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
		set_float_uniform_to_shader(glyphShaderProgramId, "opacity", sprite->opacity);
	} else if (sprite->type == SPRITE_ANIMATED) {
		if (!sprite->animations)
		{
			return;
		}
		glUseProgram(textureShaderProgramId);
		set_mat4_uniform_to_shader(textureShaderProgramId, "projection", &projection);
		set_mat4_uniform_to_shader(textureShaderProgramId, "model", &model);
		glActiveTexture(GL_TEXTURE0);
		set_int_uniform_to_shader(textureShaderProgramId, "textureId", 0);
		glBindTexture(GL_TEXTURE_2D, currentAnimationPhase->textureId);
		set_float_uniform_to_shader(textureShaderProgramId, "opacity", sprite->opacity);
	} else {
		error("unsupported sprite type %d.", sprite->type);
	}
	glBindVertexArray(vao);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

void draw_all()
{
	glClear(GL_COLOR_BUFFER_BIT);

	for (unsigned int i = 0; i < buf_len(backgroundSprites); i++)
	{
		draw(backgroundSprites[i]);
	}
	buf_clear(backgroundSprites);
	for (unsigned int i = 0; i < buf_len(middlegroundSprites); i++)
	{
		draw(middlegroundSprites[i]);
	}
	buf_clear(middlegroundSprites);
	for (unsigned int i = 0; i < buf_len(foregroundSprites); i++)
	{
		draw(foregroundSprites[i]);
	}
	buf_clear(foregroundSprites);
	for (unsigned int i = 0; i < buf_len(UISprites); i++)
	{
		draw(UISprites[i]);
	}
	buf_clear(UISprites);
	glScissor(0, 0, windowDimensions.x, 0.2f * windowDimensions.y);
	glEnable(GL_SCISSOR_TEST);
	for (unsigned int i = 0; i < buf_len(UISpritesScissor); i++)
	{
		draw(UISpritesScissor[i]);
	}
	glDisable(GL_SCISSOR_TEST);
	buf_clear(UISpritesScissor);
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
	} else if (drawLayer == DRAW_LAYER_UI_SCISSOR) {
		buf_add(UISpritesScissor, sprite);
	} else {
		error("unsupported draw layer %d.", drawLayer);
	}
}

void add_text_to_draw_list(Text *text, DrawLayer drawLayer)
{
	if (text->codes)
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
		} else if (drawLayer == DRAW_LAYER_UI_SCISSOR) {
			for (int i = 0; i < text->nbCharToDisplay; i++)
			{
				buf_add(UISpritesScissor, text->sprites[i]);
			}
		} else {
			error("unsupported draw layer %d.", drawLayer);
		}
	}
}
