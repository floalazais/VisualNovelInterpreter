#ifndef SPRITE_H
#define SPRITE_H

#include <stdbool.h>

#include "stb_truetype.h"
#include "maths.h"

typedef enum SpriteType
{
	SPRITE_COLOR,
	SPRITE_TEXTURE,
	SPRITE_GLYPH
} SpriteType;

typedef struct Sprite
{
	SpriteType type;
	ivec2 position;
	float width;
	float height;
	vec3 color;
	int textureId;
} Sprite;

typedef Sprite ** Text;

typedef enum TextSize
{
	TEXT_SIZE_SMALL = 16,
	TEXT_SIZE_NORMAL = 32,
	TEXT_SIZE_BIG = 64,
	TEXT_SIZE_HUGE = 128
} TextSize;

#define MAXUNICODE 0x10FFFF

typedef struct Glyph
{
	int width;
	int height;
	unsigned int textureId;
	int xOffset;
	int yOffset;
} Glyph;

typedef	struct Font
{
	char *fontPath;
	stbtt_fontinfo fontInfo;
	int height;
	float scale;
	int ascent;
	int descent;
	int advance;
	Glyph glyphs[0xFFFF];
	bool loaded[0xFFFF];
} Font;

unsigned int get_texture_id_from_path(char *texturePath);
Sprite *create_color_sprite(vec2 position, float width, float height, vec3 color);
Sprite *create_texture_sprite(vec2 position, float width, float height, char *texturePath);
Sprite *create_glyph(ivec2 position, int width, int height, unsigned int textureId, vec3 color);
Text create_text(vec2 position, TextSize height, char *string, char *fontPath, vec3 color);
void free_text(Text text);

typedef enum DrawLayer
{
	DRAW_LAYER_BACKGROUND,
	DRAW_LAYER_MIDDLEGROUND,
	DRAW_LAYER_FOREGROUND,
	DRAW_LAYER_UI
} DrawLayer;

void init_graphics();
void add_sprite_to_draw_list(Sprite *sprite, DrawLayer drawLayer);
void add_text_to_draw_list(Text text, DrawLayer drawLayer);
void draw_all();

#endif /* end of include guard: SPRITE_H */
