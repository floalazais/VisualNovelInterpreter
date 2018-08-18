#ifndef ANIMATED_SPRITE_H
#define ANIMATED_SPRITE_H

typedef struct AnimatedSprite
{
	int id;
} AnimatedSprite;

AnimatedSprite create_animated_sprite(char *animationFilePath);

void free_animated_sprite(AnimatedSprite animatedSprite);

#endif /* end of include guard: ANIMATED_SPRITE_H */
