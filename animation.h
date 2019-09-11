#ifndef ANIMATION_H
#define ANIMATION_H

typedef struct AnimationPhase
{
	int textureId;
	int width;
	int height;
	double length;
} AnimationPhase;

typedef enum AnimationType
{
	ANIMATION_DEFAULT,
	ANIMATION_STATIC,
	ANIMATION_LOOP
} AnimationType;

typedef enum AnimationState
{
	ANIMATION_STATE_PLAY,
	ANIMATION_STATE_STOPPING,
	ANIMATION_STATE_STOP
} AnimationState;

typedef struct Animation
{
	AnimationType animationType;
	buf(char) name;
	buf(AnimationPhase *) animationPhases;
	int currentAnimationPhase;
	double timeDuringCurrentAnimationPhase;
	AnimationState animationState;
} Animation;

buf(Animation *) get_animations_from_file(const char *_animationFilePath, const char *spriteName);
void update_animation(Animation *animation);
void reset_animation(Animation *animation);
void free_animation(Animation *animation);

#endif /* end of include guard: ANIMATION_H */
