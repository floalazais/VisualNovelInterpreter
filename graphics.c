#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "gl.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "globals.h"
#include "stb_image.h"
#include "lex.h"
#include "graphics.h"

const char *spritesStrings[] =
{
    [SPRITE_COLOR] = "color sprite",
    [SPRITE_TEXTURE] = "texture sprite",
    [SPRITE_GLYPH] = "glyph sprite",
	[SPRITE_ANIMATED] = "animated sprite"
};

static char *filePath;
static Token **tokens;
static int currentToken;

static Sprite **backgroundSprites;
static Sprite **middlegroundSprites;
static Sprite **foregroundSprites;
static Sprite **UISprites;

static char **texturesPaths;
static unsigned int *texturesIds;
static int *texturesWidths;
static int *texturesHeigts;

static Font **fonts;
static unsigned char **ttfBuffers;
static char **ttfFilesPaths;

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

static unsigned int compile_shader(char *path, GLenum shaderType)
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

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof (GLfloat) * 24, quad, GL_STATIC_DRAW);

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

static void free_font(Font *font)
{
	buf_free(font->fontPath);
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
	for (unsigned int i = 0; i < buf_len(ttfBuffers); i++)
	{
		xfree(ttfBuffers[i]);
		buf_free(ttfFilesPaths[i]);
	}

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
}

unsigned int get_texture_id_from_path(char *texturePath, int *_width, int *_height)
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
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
		if (_width)
		{
			*_width = width;
		}
		if (_height)
		{
			*_height = height;
		}

		char *newTexturePath = NULL;
		strcopy(&newTexturePath, texturePath);
		buf_add(texturesPaths, newTexturePath);
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

static void step_in_tokens()
{
    if (tokens[currentToken++]->type == TOKEN_END_OF_FILE)
    {
        error("in %s stepped after end of tokens.", filePath);
    }
}

static void steps_in_tokens(unsigned nb)
{
    while (nb--)
    {
        step_in_tokens();
    }
}

static bool token_match(int nb, ...)
{
    va_list arg;
    bool match = true;
	va_start(arg, nb);
    for (int i = 0; i < nb; i++)
    {
		if ((int)tokens[currentToken + i]->type != va_arg(arg, int))
        {
            match = false;
            break;
        }
    }
	va_end(arg);
    return match;
}

static bool token_match_on_line(int line, int nb, ...)
{
    va_list arg;
    bool match = true;
	va_start(arg, nb);
    for (int i = 0; i < nb; i++)
    {
        if ((int)tokens[currentToken + i]->type != va_arg(arg, int) || tokens[currentToken + i]->line != line)
        {
            match = false;
            break;
        }
    }
	va_end(arg);
    return match;
}

static AnimationPhase *parse_animation_phase(char *spriteName)
{
	AnimationPhase *animationPhase = xmalloc(sizeof (*animationPhase));

	if (token_match_on_line(tokens[currentToken]->line, 2, TOKEN_STRING, TOKEN_NUMERIC))
	{
		char *textureFilePath = NULL;
		strcopy(&textureFilePath, "Textures/");
		strappend(&textureFilePath, spriteName);
		strappend(&textureFilePath, "/");
		strappend(&textureFilePath, tokens[currentToken]->text);
		animationPhase->textureId = get_texture_id_from_path(textureFilePath, &animationPhase->width, &animationPhase->height);
		animationPhase->length = tokens[currentToken + 1]->numeric;
		buf_free(textureFilePath);
	} else {
		error("in %s at line %d, invalid syntax for animation phase declaration, expected texture as a string followed by a length as a number, got %s and %s instead.", filePath, tokens[currentToken]->line, tokenStrings[tokens[currentToken]->type], tokenStrings[tokens[currentToken + 1]->type]);
	}
	steps_in_tokens(2);
	if (token_match_on_line(tokens[currentToken - 1]->line, 2, TOKEN_NUMERIC, TOKEN_NUMERIC))
	{
		animationPhase->width = tokens[currentToken]->numeric * windowDimensions.x;
		animationPhase->height = tokens[currentToken + 1]->numeric * windowDimensions.y;
		steps_in_tokens(2);
	}
	if (tokens[currentToken - 1]->line == tokens[currentToken]->line && tokens[currentToken]->type != TOKEN_END_OF_FILE)
	{
		error("in %s at line %d, expected end of line after animation phase declaration.", filePath, tokens[currentToken]->line);
	}
	return animationPhase;
}

static Animation *parse_animation(char *spriteName)
{
	Animation *animation = xmalloc(sizeof (*animation));

	if (tokens[currentToken]->indentationLevel != 0)
	{
		error("in %s at line %d, indentation level of animation name must be 0, the indentation level is %d.", filePath, tokens[currentToken]->line, tokens[currentToken]->indentationLevel);
	}
	if (token_match_on_line(tokens[currentToken]->line, 2, TOKEN_STRING, TOKEN_IDENTIFIER))
	{
		animation->name = NULL;
		strcopy(&animation->name, tokens[currentToken]->text);
		if (strmatch(tokens[currentToken + 1]->text, "loop"))
		{
			animation->looping = true;
		} else {
			error("in %s at line %d, expected optional \"loop\" identifier or nothing after animation name, got %s identifier instead.", filePath, tokens[currentToken]->line, tokens[currentToken + 1]->text);
		}
		steps_in_tokens(2);
	} else if (token_match(1, TOKEN_STRING)) {
		animation->name = NULL;
		strcopy(&animation->name, tokens[currentToken]->text);
		animation->looping = false;
		step_in_tokens();
	} else {
		error("in %s at line %d, expected animation name as a string, got a %s token instead.", filePath, tokens[currentToken]->line, tokenStrings[tokens[currentToken]->type]);
	}

	if (tokens[currentToken - 1]->line == tokens[currentToken]->line)
	{
		error("in %s at line %d, expected end of line after animation declaration.", filePath, tokens[currentToken]->line);
	}
	if (tokens[currentToken]->indentationLevel != 1)
	{
		error("in %s at line %d, expected indentation level of 1 for animation phases declarations after animation declaration, got an indentation level of %d instead.", filePath, tokens[currentToken]->line, tokens[currentToken]->indentationLevel);
	}
	animation->animationPhases = NULL;
	while (tokens[currentToken]->indentationLevel == 1 && tokens[currentToken]->type != TOKEN_END_OF_FILE)
	{
		buf_add(animation->animationPhases, parse_animation_phase(spriteName));
	}
	animation->timeDuringCurrentAnimationPhase = 0.0f;
	animation->currentAnimationPhase = 0;
	animation->updating = false;
	animation->stopping = false;
	return animation;
}

static void free_animation(Animation *animation)
{
	buf_free(animation->name);
	for (unsigned int i = 0; i < buf_len(animation->animationPhases); i++)
	{
		xfree(animation->animationPhases[i]);
	}
	buf_free(animation->animationPhases);
	xfree(animation);
}

void set_animations_to_animated_sprite(Sprite *sprite, char *animationFilePath, char *spriteName)
{
	if (sprite->type != SPRITE_ANIMATED)
	{
		error("cannot set animations to non animated sprite.");
	}

	filePath = animationFilePath;
	currentToken = 0;
	tokens = lex(filePath);

	for (unsigned int i = 0; i < buf_len(sprite->animations); i++)
	{
		free_animation(sprite->animations[i]);
	}
	buf_free(sprite->animations);
	sprite->animations = NULL;
	while (tokens[currentToken]->type != TOKEN_END_OF_FILE)
	{
		buf_add(sprite->animations, parse_animation(spriteName));
	}
	sprite->currentAnimation = 0;

	for (unsigned int index = 0; index < buf_len(tokens); index++)
	{
		free_token(tokens[index]);
	}
	buf_free(tokens);
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
	stbtt_GetCodepointBitmapBox(&font->fontInfo, code, font->scale, font->scale, &x0, &y0, &x1, &y1);
	font->glyphs[code] = xmalloc(sizeof (*font->glyphs[code]));
	font->glyphs[code]->width = x1 - x0;
	font->glyphs[code]->height = y1 - y0;

	unsigned char *bitmap = xmalloc(font->glyphs[code]->height * font->glyphs[code]->width * sizeof (*bitmap));
	stbtt_MakeCodepointBitmap(&font->fontInfo, bitmap, font->glyphs[code]->width, font->glyphs[code]->height, font->glyphs[code]->width, font->scale, font->scale, code);

	unsigned int textureId;
	glGenTextures(1, &textureId);
	font->glyphs[code]->textureId = textureId;
	glBindTexture(GL_TEXTURE_2D, font->glyphs[code]->textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->glyphs[code]->width, font->glyphs[code]->height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
	glGenerateMipmap(GL_TEXTURE_2D);

	xfree(bitmap);

	font->glyphs[code]->yOffset = y0 + font->ascent - font->descent;

	stbtt_GetCodepointHMetrics(&font->fontInfo, code, &advance, &leftSideBearing);
	font->glyphs[code]->xOffset = (int)((advance - leftSideBearing) * font->scale);

	font->loaded[code] = true;
}

static void load_font(char *fontPath, int textHeight)
{
	stbtt_fontinfo fontInfo;
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
		char *newFontPath = NULL;
		strcopy(&newFontPath, fontPath);
		buf_add(ttfFilesPaths, newFontPath);
	}
	stbtt_InitFont(&fontInfo, ttfBuffer, stbtt_GetFontOffsetForIndex(ttfBuffer, 0));
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
	font->fontPath = NULL;
	strcopy(&font->fontPath, fontPath);
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

Text *create_text()
{
	Text *text = xmalloc(sizeof (*text));
	text->font = NULL;
	text->codes = NULL;
	text->sprites = NULL;
	text->widthLimit = -1;
	return text;
}

static void update_text(Text *text)
{
	text->width = 0;
	int currentLineWidth = 0;
	int nbLine = 1;
	int count = 0;

	for (unsigned int i = 0; i < buf_len(text->codes); i++)
	{
		if (currentLineWidth == 0)
		{
			while (text->codes[i] == ' ')
			{
				i++;
			}
		}

		if (!text->font->loaded[text->codes[i]])
		{
			load_glyph(text->font, text->codes[i]);
		}

		if (i == buf_len(text->sprites))
		{
			buf_add(text->sprites, create_sprite(SPRITE_GLYPH));
		}

		text->sprites[count]->width = text->font->glyphs[text->codes[i]]->width;
		text->sprites[count]->height = text->font->glyphs[text->codes[i]]->height;
		text->sprites[count]->textureId = text->font->glyphs[text->codes[i]]->textureId;
		text->sprites[count]->color = text->color;

		text->sprites[count]->position.x = text->position.x + currentLineWidth;
		text->sprites[count]->position.y = text->position.y + (nbLine - 1) * (text->font->ascent - text->font->descent) + text->font->glyphs[text->codes[i]]->yOffset;

		if (i == buf_len(text->codes) - 1)
		{
			currentLineWidth += text->font->glyphs[text->codes[i]]->xOffset + (int)(stbtt_GetCodepointKernAdvance(&text->font->fontInfo, text->codes[i], 0) * text->font->scale);
		} else {
			currentLineWidth += text->font->glyphs[text->codes[i]]->xOffset + (int)(stbtt_GetCodepointKernAdvance(&text->font->fontInfo, text->codes[i], text->codes[i + 1]) * text->font->scale);
		}

		count++;

		if (text->widthLimit != -1 && currentLineWidth > text->widthLimit)
		{
			int oldI = i;
			int oldCount = count;
			bool foundSpace = true;
			while (text->codes[i] != ' ')
			{
				if (i == 0)
				{
					i = oldI;
					count = oldCount;
					foundSpace = false;
					break;
				}
				i--;
				count--;
				if (i == buf_len(text->codes) - 1)
				{
					currentLineWidth -= text->font->glyphs[text->codes[i]]->xOffset + (int)(stbtt_GetCodepointKernAdvance(&text->font->fontInfo, text->codes[i], 0) * text->font->scale);
				} else {
					currentLineWidth -= text->font->glyphs[text->codes[i]]->xOffset + (int)(stbtt_GetCodepointKernAdvance(&text->font->fontInfo, text->codes[i], text->codes[i + 1]) * text->font->scale);
				}
			}
			if (foundSpace)
			{
				i--;
				count--;
				currentLineWidth -= text->font->glyphs[text->codes[i]]->xOffset + (int)(stbtt_GetCodepointKernAdvance(&text->font->fontInfo, text->codes[i], text->codes[i + 1]) * text->font->scale);
			}
			if (currentLineWidth > text->width)
			{
				text->width = currentLineWidth;
			}
			currentLineWidth = 0;
			nbLine++;
		}
    }
	if (currentLineWidth > text->width)
	{
		text->width = currentLineWidth;
	}
	text->height = nbLine * (text->font->ascent - text->font->descent);
	text->nbMaxCharToDisplay = count;
}

void set_width_limit_to_text(Text *text, int limit)
{
	text->widthLimit = limit;
	if (text->codes)
	{
		update_text(text);
	}
}

void set_position_to_text(Text *text, ivec2 position)
{
	text->position = position;
	if (text->codes)
	{
		update_text(text);
	}
}

void set_font_to_text(Text *text, char *fontPath, int textHeight)
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

	text->height = text->font->ascent - text->font->descent;

	if (text->codes)
	{
		update_text(text);
	}
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

void set_string_to_text(Text *text, char *string)
{
	buf_free(text->codes);
	text->codes = NULL;

	if (string)
	{
		int code;
		while (*string != '\0')
	    {
			code = utf8_decode(string);
			if (code == -1)
			{
				error("found incorrect utf-8 sequence in %s.", string);
			} else if (code > 0xFFFF) {
				error("unsupported character code %d.", code);
			}
			string += offset;
			buf_add(text->codes, code);
		}
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

static void set_int_uniform_to_shader(unsigned int shaderProgramId, char *uniformName, int value)
{
	glUniform1i(get_uniform_location(shaderProgramId, uniformName), value);
}

static void set_float_uniform_to_shader(unsigned int shaderProgramId, char *uniformName, float value)
{
	glUniform1f(get_uniform_location(shaderProgramId, uniformName), value);
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
		Animation *currentAnimation = sprite->animations[sprite->currentAnimation];
		AnimationPhase *currentAnimationPhase = sprite->animations[sprite->currentAnimation]->animationPhases[sprite->animations[sprite->currentAnimation]->currentAnimationPhase];
	    glBindTexture(GL_TEXTURE_2D, currentAnimationPhase->textureId);
		set_float_uniform_to_shader(textureShaderProgramId, "opacity", sprite->opacity);
		if (currentAnimation->updating)
		{
			currentAnimation->timeDuringCurrentAnimationPhase += deltaTime;
			if (currentAnimationPhase->length <= currentAnimation->timeDuringCurrentAnimationPhase)
			{
				if ((unsigned int)currentAnimation->currentAnimationPhase == buf_len(currentAnimation->animationPhases) - 1)
				{
					if (currentAnimation->looping)
					{
						currentAnimation->currentAnimationPhase = 0;
					}
					if (currentAnimation->stopping)
					{
						currentAnimation->updating = false;
						currentAnimation->stopping = false;
					}
				} else {
					currentAnimation->currentAnimationPhase++;
				}
				currentAnimation->timeDuringCurrentAnimationPhase = 0;
			}
		}
	} else {
		error("unsupported sprite type %d.", sprite->type);
	}
	glBindVertexArray(vao);

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
		} else {
			error("unsupported draw layer %d.", drawLayer);
		}
	}
}
