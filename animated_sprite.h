#ifndef ANIMATED_SPRITE_H
#define ANIMATED_SPRITE_H

#include <stdbool.h>

typedef struct AnimationPhase
{
	int textureId;
	double length;
} AnimationPhase;

typedef struct Animation
{
	AnimationPhase **animationPhases;
	bool looping;
	int currentAnimationPhase;
	double timeDuringCurrentAnimationPhase;
	bool finished;
} Animation;

typedef struct AnimatedSprite
{
	char **animationsNames;
	Animation **animations;
	int currentAnimation;
} AnimatedSprite;

AnimatedSprite *create_animated_sprite(char *animatedSpriteName);

void update_animated_sprite(AnimatedSprite *animatedSprite);

void stop_animated_sprite(AnimatedSprite *animatedSprite);

void free_animated_sprite(AnimatedSprite *animatedSprite);

#endif /* end of include guard: ANIMATED_SPRITE_H */
