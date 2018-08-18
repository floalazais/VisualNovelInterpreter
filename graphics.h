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
	vec2 position;
	float width;
	float height;
	vec3 color;
	int textureId;
} Sprite;

Sprite *create_color_sprite(vec2 position, int width, int height, vec3 color);
Sprite *create_texture_sprite(vec2 position, int width, int height, int textureId);
Sprite *create_glyph_sprite(vec2 position, int width, int height, int textureId, vec3 color);

typedef enum DrawLayer
{
	DRAW_LAYER_BACKGROUND,
	DRAW_LAYER_MIDDLEGROUND,
	DRAW_LAYER_FOREGROUND,
	DRAW_LAYER_UI
} DrawLayer;

void init_graphics();
void add_to_draw_list(Sprite *sprite, DrawLayer drawLayer);
void draw_all();

#endif /* end of include guard: SPRITE_H */
