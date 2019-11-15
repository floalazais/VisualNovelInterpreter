#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#include "error.h"
#include "maths.h"
#include "globals.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "token.h"
#include "lex.h"
#include "str.h"
#include "animation.h"

static const char *filePath;
static buf(AnimationToken *) tokens;
static int currentToken;
static bool parsingStaticAnimation;

static void step_in_tokens()
{
	if (tokens[currentToken++]->type == ANIMATION_TOKEN_END_OF_FILE)
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

unsigned int get_texture_id_from_path(const char *texturePath, int *width, int *height);

static AnimationPhase *parse_animation_phase(const char *spriteName)
{
	AnimationPhase *animationPhase = xmalloc(sizeof (*animationPhase));

	if (parsingStaticAnimation)
	{
		if (token_match(1, ANIMATION_TOKEN_STRING))
		{
			buf(char) textureFilePath = strclone("Textures/");
			strappend(&textureFilePath, spriteName);
			strappend(&textureFilePath, "/");
			strappend(&textureFilePath, tokens[currentToken]->string);
			animationPhase->textureId = get_texture_id_from_path(textureFilePath, &animationPhase->pixelWidth, &animationPhase->pixelHeight);
			animationPhase->length = -1;
			buf_free(textureFilePath);
			step_in_tokens();
		} else {
			error("in %s at line %d, invalid syntax for static animation phase declaration, expected texture as a string (and an optional size as two numbers), got %s instead.", filePath, tokens[currentToken]->line, animation_token_to_string(tokens[currentToken]));
		}
	} else if (token_match_on_line(tokens[currentToken]->line, 2, ANIMATION_TOKEN_STRING, ANIMATION_TOKEN_NUMERIC)) {
		buf(char) textureFilePath = strclone("Textures/");
		strappend(&textureFilePath, spriteName);
		strappend(&textureFilePath, "/");
		strappend(&textureFilePath, tokens[currentToken]->string);
		animationPhase->textureId = get_texture_id_from_path(textureFilePath, &animationPhase->pixelWidth, &animationPhase->pixelHeight);
		if (tokens[currentToken + 1]->numeric == 0)
		{
			error("in %s at line %d, cannot specify a no-time length animtion phase.", filePath, tokens[currentToken]->line);
		}
		animationPhase->length = tokens[currentToken + 1]->numeric;
		buf_free(textureFilePath);
		steps_in_tokens(2);
	} else {
		if (tokens[currentToken]->type == ANIMATION_TOKEN_END_OF_FILE)
		{
			error("in %s at line %d, incomplete animation phase declaration, expected texture as a string followed by a length as a number (and an optional size as two numbers).", filePath, tokens[currentToken]->line);
		} else {
			error("in %s at line %d, invalid syntax for animation phase declaration, expected texture as a string followed by a length as a number (and an optional size as two numbers), got %s and %s instead.", filePath, tokens[currentToken]->line, animation_token_to_string(tokens[currentToken]), animation_token_to_string(tokens[currentToken + 1]));
		}
	}
	if (token_match_on_line(tokens[currentToken - 1]->line, 2, ANIMATION_TOKEN_NUMERIC, ANIMATION_TOKEN_NUMERIC))
	{
		animationPhase->responsiveWidth = tokens[currentToken]->numeric;
		animationPhase->responsiveHeight = tokens[currentToken + 1]->numeric;
		animationPhase->responsive = true;
		steps_in_tokens(2);
	} else {
		animationPhase->responsive = false;
	}
	if (tokens[currentToken - 1]->line == tokens[currentToken]->line && tokens[currentToken]->type != ANIMATION_TOKEN_END_OF_FILE)
	{
		error("in %s at line %d, expected end of line after animation phase declaration.", filePath, tokens[currentToken]->line);
	}
	return animationPhase;
}

static Animation *parse_animation(const char *spriteName)
{
	parsingStaticAnimation = false;

	Animation *animation = xmalloc(sizeof (*animation));

	if (tokens[currentToken]->indentationLevel != 0)
	{
		error("in %s at line %d when declaring an animation, indentation level of animation name must be 0, the indentation level is %d.", filePath, tokens[currentToken]->line, tokens[currentToken]->indentationLevel);
	}
	if (token_match_on_line(tokens[currentToken]->line, 2, ANIMATION_TOKEN_STRING, ANIMATION_TOKEN_IDENTIFIER))
	{
		animation->name = strclone(tokens[currentToken]->string);
		if (strmatch(tokens[currentToken + 1]->string, "loop"))
		{
			animation->animationType = ANIMATION_LOOP;
		} else if (strmatch(tokens[currentToken + 1]->string, "static")) {
			animation->animationType = ANIMATION_STATIC;
			parsingStaticAnimation = true;
		} else {
			error("in %s at line %d, expected optional \"loop\" or \"static\" identifier or nothing after animation name, got %s identifier instead.", filePath, tokens[currentToken]->line, tokens[currentToken + 1]->string);
		}
		steps_in_tokens(2);
	} else if (token_match(1, ANIMATION_TOKEN_STRING)) {
		animation->name = strclone(tokens[currentToken]->string);
		animation->animationType = ANIMATION_DEFAULT;
		step_in_tokens();
	} else {
		error("in %s at line %d when declaring an animation, expected animation name as a string, got a %s instead.", filePath, tokens[currentToken]->line, animation_token_to_string(tokens[currentToken]));
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
	while (tokens[currentToken]->indentationLevel == 1 && tokens[currentToken]->type != ANIMATION_TOKEN_END_OF_FILE)
	{
		buf_add(animation->animationPhases, parse_animation_phase(spriteName));
		if (animation->animationType == ANIMATION_STATIC && buf_len(animation->animationPhases) > 1)
		{
			error("in %s at line %d, static animations imply only one animation phase, got another.", filePath, tokens[currentToken]->line);
		}
	}
	animation->timeDuringCurrentAnimationPhase = 0.0f;
	animation->currentAnimationPhase = 0;
	animation->animationState = ANIMATION_STATE_STOP;
	return animation;
}

buf(Animation *) get_animations_from_file(const char *animationFilePath, const char *spriteName)
{
	filePath = animationFilePath;
	currentToken = 0;
	tokens = lex_animations(filePath);

	buf(Animation *) animations = NULL;
	while (tokens[currentToken]->type != ANIMATION_TOKEN_END_OF_FILE)
	{
		buf_add(animations, parse_animation(spriteName));
	}

	for (unsigned int index = 0; index < buf_len(tokens); index++)
	{
		free_animation_token(tokens[index]);
	}
	buf_free(tokens);

	return animations;
}

void update_animation(Animation *animation)
{
	AnimationPhase *currentAnimationPhase = animation->animationPhases[animation->currentAnimationPhase];
	if (animation->animationState != ANIMATION_STATE_STOP && animation->animationType != ANIMATION_STATIC)
	{
		animation->timeDuringCurrentAnimationPhase += deltaTime;
		while (animation->timeDuringCurrentAnimationPhase >= currentAnimationPhase->length)
		{
			if ((unsigned int)animation->currentAnimationPhase == buf_len(animation->animationPhases) - 1)
			{
				if (animation->animationType == ANIMATION_LOOP)
				{
					animation->currentAnimationPhase = 0;
				}
				if (animation->animationState == ANIMATION_STATE_STOPPING)
				{
					animation->animationState = ANIMATION_STATE_STOP;
				}
			} else {
				animation->currentAnimationPhase++;
			}
			if (animation->animationState != ANIMATION_STATE_STOP)
			{
				animation->timeDuringCurrentAnimationPhase -= currentAnimationPhase->length;
				currentAnimationPhase = animation->animationPhases[animation->currentAnimationPhase];
			} else {
				break;
			}
		}
	}
}

void reset_animation(Animation *animation)
{
	animation->timeDuringCurrentAnimationPhase = 0.0f;
	animation->currentAnimationPhase = 0;
	animation->animationState = ANIMATION_STATE_STOP;
}

void free_animation(Animation *animation)
{
	buf_free(animation->name);
	for (unsigned int i = 0; i < buf_len(animation->animationPhases); i++)
	{
		xfree(animation->animationPhases[i]);
	}
	buf_free(animation->animationPhases);
	xfree(animation);
}