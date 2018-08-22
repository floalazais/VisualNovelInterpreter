#ifndef SPRITE_H
#define SPRITE_H

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

Sprite *create_color_sprite(vec2 position, float width, float height, vec3 color);
Sprite *create_texture_sprite(vec2 position, float width, float height, char *texturePath);
Sprite *create_glyph(ivec2 position, int width, int height, unsigned int textureId, vec3 color);
Text create_text(vec2 position, int height, char *string, char *fontPath, vec3 color);

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
