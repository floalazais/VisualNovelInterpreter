#ifndef GRAPHICS_H
#define GRAPHICS_H

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
	float opacity;
	int textureId;
	buf(Animation *) animations;
	int currentAnimation;
	bool fixedSize;
} Sprite;

Sprite *create_sprite(SpriteType spriteType);
void free_sprite(Sprite *sprite);

typedef struct Glyph
{
	int xOffset;
	int yOffset;
	int width;
	int height;
	int textureId;
} Glyph;

typedef struct stbtt_fontinfo stbtt_fontinfo;

typedef	struct Font
{
	buf(char) fontPath;
	stbtt_fontinfo *fontInfo;
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
	int *codes;
	buf(Sprite *) sprites;
	ivec2 position;
	int width;
	int height;
	int widthLimit;
	vec3 color;
	int nbCharToDisplay;
	int nbMaxCharToDisplay;
} Text;

typedef enum TextSize
{
	TEXT_SIZE_SMALL = 16,
	TEXT_SIZE_NORMAL = 32,
	TEXT_SIZE_BIG = 64,
	TEXT_SIZE_HUGE = 128
} TextSize;

Text *create_text();
void set_text_position(Text *text, ivec2 position);
void set_text_font(Text *text, const char *fontPath, int textHeight);
void set_text_string(Text *text, const char *string);
void set_text_width_limit(Text *text, int limit);
void free_text(Text *text);

typedef enum DrawLayer
{
	DRAW_LAYER_BACKGROUND,
	DRAW_LAYER_MIDDLEGROUND,
	DRAW_LAYER_FOREGROUND,
	DRAW_LAYER_UI
} DrawLayer;

void init_graphics();
void free_graphics();
void add_sprite_to_draw_list(Sprite *sprite, DrawLayer drawLayer);
void add_text_to_draw_list(Text *text, DrawLayer drawLayer);
void draw_all();

unsigned int get_texture_id_from_path(const char *texturePath, int *width, int *height);

#endif /* end of include guard: GRAPHICS_H */
