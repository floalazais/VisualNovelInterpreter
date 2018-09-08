#include <string.h>
#include <stdarg.h>

#include "xalloc.h"
#include "animated_sprite.h"
#include "globals.h"
#include "stretchy_buffer.h"
#include "lex.h"
#include "graphics.h"

static char *filePath;
static Token *tokens;

static int currentToken = 0;

static char **animationsNames = NULL;

static void step_in_tokens()
{
    if (tokens[currentToken++].type == TOKEN_END_OF_FILE)
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
		if (tokens[currentToken + i].type != va_arg(arg, int))
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
        if (tokens[currentToken + i].type != va_arg(arg, int) || tokens[currentToken + i].line != line)
        {
            match = false;
            break;
        }
    }
	va_end(arg);
    return match;
}

AnimationPhase *parse_animation_phase()
{
	AnimationPhase *animationPhase = xmalloc(sizeof (*animationPhase));

	if (token_match_on_line(tokens[currentToken].line, 2, TOKEN_STRING, TOKEN_NUMERIC))
	{
		animationPhase->textureId = get_texture_id_from_path(tokens[currentToken].text);
		animationPhase->length = tokens[currentToken + 1].numeric;
	} else {
		error("in %s at line %d, invalid syntax for animation phase declaration, expected texture as a string followed by a length as a number, got %s and %s instead.", filePath, tokens[currentToken].line, tokenStrings[tokens[currentToken].type], tokenStrings[tokens[currentToken + 1].type]);
	}
	steps_in_tokens(2);
	if (tokens[currentToken - 1].line == tokens[currentToken].line && tokens[currentToken].type != TOKEN_END_OF_FILE)
	{
		error("in %s at line %d, expected end of line after animation phase declaration.", filePath, tokens[currentToken].line);
	}
	return animationPhase;
}

Animation *parse_animation()
{
	Animation *animation = xmalloc(sizeof (*animation));

	if (tokens[currentToken].indentationLevel != 0)
	{
		error("in %s at line %d, indentation level of animation name must be 0, the indentation level is %d.", filePath, tokens[currentToken].line, tokens[currentToken].indentationLevel);
	}
	if (token_match_on_line(tokens[currentToken].line, 2, TOKEN_STRING, TOKEN_IDENTIFIER))
	{
		buf_add(animationsNames, tokens[currentToken].text);
		if (strmatch(tokens[currentToken + 1].text, "loop"))
		{
			animation->looping = true;
		} else {
			error("in %s at line %d, expected optional \"loop\" identifier or nothing after animation name, got %s identifier instead.", filePath, tokens[currentToken].line, tokens[currentToken + 1].text);
		}
		steps_in_tokens(2);
	} else if (token_match(1, TOKEN_STRING)) {
		buf_add(animationsNames, tokens[currentToken].text);
		step_in_tokens();
	} else {
		error("in %s at line %d, expected animation name as a string, got a %s token instead.", filePath, tokens[currentToken].line, tokenStrings[tokens[currentToken].type]);
	}

	if (tokens[currentToken - 1].line == tokens[currentToken].line)
	{
		error("in %s at line %d, expected end of line after animation declaration.", filePath, tokens[currentToken].line);
	}
	if (tokens[currentToken].indentationLevel != 1)
	{
		error("in %s at line %d, expected indentation level of 1 for animation phases declarations after animation declaration, got an indentation level of %d instead.", filePath, tokens[currentToken].line, tokens[currentToken].indentationLevel);
	}
	animation->animationPhases = NULL;
	while (tokens[currentToken].indentationLevel == 1 && tokens[currentToken].type != TOKEN_END_OF_FILE)
	{
		buf_add(animation->animationPhases, parse_animation_phase());
	}
	animation->timeDuringCurrentAnimationPhase = 0.0f;
	animation->finished = false;
	animation->currentAnimationPhase = 0;
	return animation;
}

AnimatedSprite *create_animated_sprite(char *animatedSpriteName)
{
	animationsNames = NULL;
	currentToken = 0;

	AnimatedSprite *animatedSprite = xmalloc(sizeof (*animatedSprite));

	filePath = NULL;

	for (int i = 0; i < strlen("Animation files/"); i++)
	{
		buf_add(filePath, "Animation files/"[i]);
	}
	for (int i = 0; i < buf_len(animatedSpriteName) - 1; i++)
	{
		buf_add(filePath, animatedSpriteName[i]);
	}
	for (int i = 0; i < strlen(".anm"); i++)
	{
		buf_add(filePath, ".anm"[i]);
	}
	buf_add(filePath, '\0');

	tokens = lex(filePath);

	animatedSprite->animations = NULL;
	while (tokens[currentToken].type != TOKEN_END_OF_FILE)
	{
		buf_add(animatedSprite->animations, parse_animation());
	}
	animatedSprite->animationsNames = animationsNames;
	animatedSprite->currentAnimation = 0;
	return animatedSprite;
}

void update_animated_sprite(AnimatedSprite *animatedSprite)
{
	animatedSprite->animations[animatedSprite->currentAnimation]->timeDuringCurrentAnimationPhase += deltaTime;
	if (animatedSprite->animations[animatedSprite->currentAnimation]->animationPhases[animatedSprite->animations[animatedSprite->currentAnimation]->currentAnimationPhase]->length <= animatedSprite->animations[animatedSprite->currentAnimation]->timeDuringCurrentAnimationPhase)
	{
		if (animatedSprite->animations[animatedSprite->currentAnimation]->currentAnimationPhase == buf_len(animatedSprite->animations[animatedSprite->currentAnimation]->animationPhases) - 1)
		{
			if (animatedSprite->animations[animatedSprite->currentAnimation]->looping)
			{
				animatedSprite->animations[animatedSprite->currentAnimation]->currentAnimationPhase = 0;
			}
		} else {
			animatedSprite->animations[animatedSprite->currentAnimation]->currentAnimationPhase++;
		}
		animatedSprite->animations[animatedSprite->currentAnimation]->timeDuringCurrentAnimationPhase = 0;
	}
}

void stop_animated_sprite(AnimatedSprite *animatedSprite)
{
	animatedSprite->animations[animatedSprite->currentAnimation]->currentAnimationPhase = 0;
	animatedSprite->animations[animatedSprite->currentAnimation]->timeDuringCurrentAnimationPhase = 0;
}

static void free_animation(Animation *animation)
{
	for (int i = 0; i < buf_len(animation->animationPhases); i++)
	{
		free(animation->animationPhases[i]);
	}
	buf_free(animation->animationPhases);
	free(animation);
}

void free_animated_sprite(AnimatedSprite *animatedSprite)
{
	for (int i = 0; i < buf_len(animatedSprite->animationsNames); i++)
	{
		buf_free(animatedSprite->animationsNames[i]);
	}
	buf_free(animatedSprite->animationsNames);
	for (int i = 0; i < buf_len(animatedSprite->animations); i++)
	{
		free_animation(animatedSprite->animations[i]);
	}
	buf_free(animatedSprite->animations);
	free(animatedSprite);
}
