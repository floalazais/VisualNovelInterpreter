#ifndef SPRITE_H
#define SPRITE_H

#include <stdbool.h>

#include "stb_truetype.h"
#include "maths.h"

typedef struct AnimationPhase
{
	int textureId;
	int width;
	int height;
	double length;
} AnimationPhase;

typedef struct Animation
{
	char *name;
	AnimationPhase **animationPhases;
	int currentAnimationPhase;
	double timeDuringCurrentAnimationPhase;
	bool looping;
	bool updating;
	bool stopping;
} Animation;

typedef enum SpriteType
{
	SPRITE_COLOR,
	SPRITE_TEXTURE,
	SPRITE_GLYPH,
	SPRITE_ANIMATED
} SpriteType;

typedef struct Sprite
{
	SpriteType type;
	ivec2 position;
	int width;
	int height;
	vec3 color;
	unsigned int textureId;
	Animation **animations;
	int currentAnimation;
} Sprite;

typedef struct Glyph
{
	int xOffset;
	int yOffset;
	int width;
	int height;
	unsigned int textureId;
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
	Glyph **glyphs;
	bool *loaded;
} Font;

typedef struct Text
{
	Font *font;
	char *string;
	Sprite **sprites;
	ivec2 position;
	int width;
	int height;
	vec3 color;
	int nbCharToDisplay;
} Text;

typedef enum TextSize
{
	TEXT_SIZE_SMALL = 16,
	TEXT_SIZE_NORMAL = 32,
	TEXT_SIZE_BIG = 64,
	TEXT_SIZE_HUGE = 128
} TextSize;

unsigned int get_texture_id_from_path(char *texturePath, int *width, int *height);
Sprite *create_sprite(SpriteType spriteType);
void set_animations_to_animated_sprite(Sprite *sprite, char *animationFilePath, char *spriteName);
void free_sprite(Sprite *sprite);
Text *create_text();
void set_position_to_text(Text *text, ivec2 position);
void set_font_to_text(Text *text, char *fontPath, int textHeight);
void set_string_to_text(Text *text, char *string);
void free_text(Text *text);

typedef enum DrawLayer
{
	DRAW_LAYER_BACKGROUND,
	DRAW_LAYER_MIDDLEGROUND,
	DRAW_LAYER_FOREGROUND,
	DRAW_LAYER_UI
} DrawLayer;

void init_graphics();
void add_sprite_to_draw_list(Sprite *sprite, DrawLayer drawLayer);
void add_text_to_draw_list(Text *text, DrawLayer drawLayer);
void draw_all();

#endif /* end of include guard: SPRITE_H */
