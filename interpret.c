#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <stdio.h>

#include "maths.h"
#include "error.h"
#include "user_input.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "token.h"
#include "stroperation.h"
#include "graphics.h"
#include "variable.h"
#include "dialog.h"
#include "interpret.h"
#include "globals.h"

static char *nextDialogStartKnotName;

static char *charactersNames[7];
static Sprite *oldCharactersSprites[7];
static Sprite *charactersSprites[7];
static int currentSpeakerSpriteIndex;
static Sprite *oldBackgroundSprite;
static Sprite *backgroundSprite;
static Sprite *choiceMarker;
static Sprite *characterNameBox;
static Sprite *sentenceBox;
static Text *currentSpeaker;
static Text *currentSentence;
static Text **currentChoices;
static Command **goToCommands;
static bool choosing;
static int nbChoices;
static int currentChoice;
static bool choicesDisplayed;
static bool moving;
static bool end;
static bool displayedSpeakerName;
static bool appearingBackground;
static bool appearingCharacter;
static bool displayDialogUI;
static bool displaySpeakerName;
static bool sentenceFirstUpdate;
static float timeDuringCurrentChar;

void init_dialog_ui()
{
	nextDialogStartKnotName = NULL;

	vec3 black = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
	vec3 white = {.x = 1.0f, .y = 1.0f, .z = 1.0f};

	currentSentence = create_text();
	set_font_to_text(currentSentence, "Fonts/arial.ttf", TEXT_SIZE_NORMAL);
	set_width_limit_to_text(currentSentence, 0.97f * windowDimensions.x);
	currentSentence->position.x = (int)(0.015f * windowDimensions.x);
	currentSentence->position.y = (int)(0.8f * windowDimensions.y + 2);
	currentSentence->color = white;

	currentSpeaker = create_text();
	set_font_to_text(currentSpeaker, "Fonts/arial.ttf", TEXT_SIZE_BIG);
	currentSpeaker->color = white;

	sentenceBox = create_sprite(SPRITE_COLOR);
	sentenceBox->position.x = 0;
	sentenceBox->position.y = (int)(0.8f * windowDimensions.y);
	sentenceBox->width = windowDimensions.x;
	sentenceBox->height = (int)(0.2f * windowDimensions.y);
	sentenceBox->color = black;
	sentenceBox->opacity = 0.75f;

	characterNameBox = create_sprite(SPRITE_COLOR);
	characterNameBox->color = black;
	characterNameBox->opacity = 0.75f;

	choiceMarker = create_sprite(SPRITE_COLOR);
	choiceMarker->position.x = (int)(0.005f * windowDimensions.x);
	choiceMarker->width = (int)(0.005 * windowDimensions.x);
	choiceMarker->height = choiceMarker->width;
	choiceMarker->color = white;

	oldBackgroundSprite = create_sprite(SPRITE_ANIMATED);
	oldBackgroundSprite->position.x = 0;
	oldBackgroundSprite->position.y = 0;
	oldBackgroundSprite->width = windowDimensions.x;
	oldBackgroundSprite->height = windowDimensions.y;
	backgroundSprite = create_sprite(SPRITE_ANIMATED);
	backgroundSprite->position.x = 0;
	backgroundSprite->position.y = 0;
	backgroundSprite->width = windowDimensions.x;
	backgroundSprite->height = windowDimensions.y;

	for (int i = 0; i < 7; i++)
	{
		charactersNames[i] = NULL;
		oldCharactersSprites[i] = create_sprite(SPRITE_ANIMATED);
		charactersSprites[i] = create_sprite(SPRITE_ANIMATED);
	}
	currentChoices = NULL;
	goToCommands = NULL;
}

void free_dialog_ui()
{
	free_text(currentSentence);
	free_text(currentSpeaker);
	free_sprite(sentenceBox);
	free_sprite(characterNameBox);
	free_sprite(choiceMarker);
	for (unsigned int i = 0; i < buf_len(currentChoices); i++)
	{
		free_text(currentChoices[i]);
	}
	buf_free(currentChoices);
	oldBackgroundSprite->animations = NULL;
	backgroundSprite->animations = NULL;
	free_sprite(oldBackgroundSprite);
	free_sprite(backgroundSprite);
	for (int i = 0; i < 7; i++)
	{
		oldCharactersSprites[i]->animations = NULL;
		free_sprite(oldCharactersSprites[i]);
		charactersSprites[i]->animations = NULL;
		free_sprite(charactersSprites[i]);
	}
}

static bool update_command(Command *command)
{
	if (command->type == COMMAND_SET_BACKGROUND)
	{
		displayDialogUI = false;
		if (!appearingBackground)
		{
			bool foundPack = false;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->backgroundPacksNames); i++)
			{
				if (strmatch(command->arguments[0]->string, interpretingDialog->backgroundPacksNames[i]))
				{
					if (backgroundSprite->animations)
					{
						oldBackgroundSprite->animations = backgroundSprite->animations;
						oldBackgroundSprite->currentAnimation = backgroundSprite->currentAnimation;
						oldBackgroundSprite->textureId = oldBackgroundSprite->animations[oldBackgroundSprite->currentAnimation]->animationPhases[0]->textureId;
					}
					backgroundSprite->animations = interpretingDialog->backgroundPacks[i]->animations;
					bool foundAnimation = false;
					for (unsigned int j = 0; j < buf_len(backgroundSprite->animations); j++)
					{
						if (strmatch(command->arguments[1]->string, backgroundSprite->animations[j]->name))
						{
							backgroundSprite->currentAnimation = j;
							backgroundSprite->textureId = backgroundSprite->animations[j]->animationPhases[0]->textureId;
							foundAnimation = true;
							break;
						}
					}
					if (!foundAnimation)
					{
						error("background %s of background pack %s does not exist.", command->arguments[1]->string, command->arguments[0]->string);
					}
					foundPack = true;
					break;
				}
			}
			if (!foundPack)
			{
				error("background pack %s does not exist.", command->arguments[0]->string);
			}
			appearingBackground = true;
			backgroundSprite->opacity = 0.0f;
			if (oldBackgroundSprite->animations)
			{
				oldBackgroundSprite->opacity = 1.0f;
			}
		} else {
			if (backgroundSprite->opacity >= 1.0f)
			{
				backgroundSprite->opacity = 1.0f;
				if (oldBackgroundSprite->animations)
				{
					oldBackgroundSprite->opacity = 1.0f;
				}
				oldBackgroundSprite->animations = NULL;
				appearingBackground = false;
				return true;
			} else {
				if (oldBackgroundSprite->animations)
				{
					oldBackgroundSprite->opacity -= deltaTime;
				}
				backgroundSprite->opacity += deltaTime;
			}
		}
		return false;
	} else if (command->type == COMMAND_CLEAR_BACKGROUND) {
		if (backgroundSprite->opacity > 0.0f)
		{
			backgroundSprite->opacity -= deltaTime;
			return false;
		} else {
			backgroundSprite->opacity = 1.0f;
			backgroundSprite->animations = NULL;
		}
	} else if (command->type == COMMAND_SET_CHARACTER) {
		int position = (int)command->arguments[0]->numeric;
		if (!appearingCharacter)
		{
			bool foundCharacter = false;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->charactersNames); i++)
			{
				if (strmatch(command->arguments[1]->string, interpretingDialog->charactersNames[i]))
				{
					charactersNames[position] = interpretingDialog->charactersNames[i];
					if (charactersSprites[position]->animations)
					{
						oldCharactersSprites[position]->animations = charactersSprites[position]->animations;
						oldCharactersSprites[position]->position.x = charactersSprites[position]->position.x;
						oldCharactersSprites[position]->position.y = charactersSprites[position]->position.y;
						oldCharactersSprites[position]->width = charactersSprites[position]->width;
						oldCharactersSprites[position]->height = charactersSprites[position]->height;
						oldCharactersSprites[position]->currentAnimation = charactersSprites[position]->currentAnimation;
						oldCharactersSprites[position]->textureId = oldCharactersSprites[position]->animations[oldCharactersSprites[position]->currentAnimation]->animationPhases[0]->textureId;
					}
					charactersSprites[position]->animations = interpretingDialog->charactersSprites[i]->animations;
					bool foundAnimation = false;
					for (unsigned int j = 0; j < buf_len(charactersSprites[position]->animations); j++)
					{
						if (strmatch(command->arguments[2]->string, charactersSprites[position]->animations[j]->name))
						{
							charactersSprites[position]->position.x = (int)((windowDimensions.x * position / 6.0f) - (charactersSprites[position]->animations[j]->animationPhases[0]->width / 2));
							charactersSprites[position]->position.y = (int)(windowDimensions.y - charactersSprites[position]->animations[j]->animationPhases[0]->height);
							charactersSprites[position]->width = charactersSprites[position]->animations[j]->animationPhases[0]->width;
							charactersSprites[position]->height = charactersSprites[position]->animations[j]->animationPhases[0]->height;
							charactersSprites[position]->currentAnimation = j;
							charactersSprites[position]->textureId = charactersSprites[position]->animations[j]->animationPhases[0]->textureId;
							foundAnimation = true;
							break;
						}
					}
					if (!foundAnimation)
					{
						error("animation %s of character %s does not exist.", command->arguments[2]->string, command->arguments[1]->string);
					}
					foundCharacter = true;
					break;
				}
			}
			if (!foundCharacter)
			{
				error("character %s does not exist.", command->arguments[1]->string);
			}
			appearingCharacter = true;
			charactersSprites[position]->opacity = 0.0f;
			if (oldCharactersSprites[position]->animations)
			{
				oldCharactersSprites[position]->opacity = 1.0f;
			}
		} else {
			if (charactersSprites[position]->opacity >= 1.0f)
			{
				charactersSprites[position]->opacity = 1.0f;
				if (oldCharactersSprites[position]->animations)
				{
					oldCharactersSprites[position]->opacity = 1.0f;
				}
				oldCharactersSprites[position]->animations = NULL;
				appearingCharacter = false;
				return true;
			} else {
				if (oldCharactersSprites[position]->animations)
				{
					oldCharactersSprites[position]->opacity -= deltaTime * 2;
				}
				charactersSprites[position]->opacity += deltaTime * 2;
			}
		}
		return false;
	} else if (command->type == COMMAND_CLEAR_CHARACTER_POSITION) {
		int position = (int)command->arguments[0]->numeric;
		if (charactersSprites[position]->opacity > 0.0f)
		{
			charactersSprites[position]->opacity -= deltaTime * 2;
			return false;
		} else {
			charactersSprites[position]->opacity = 1.0f;
			charactersSprites[position]->animations = NULL;
			charactersNames[position] = NULL;
		}
	} else if (command->type == COMMAND_CLEAR_CHARACTER_POSITIONS) {
		bool fading = false;
		for (int i = 0; i < 7; i++)
		{
			if (charactersSprites[i]->animations)
			{
				if (charactersSprites[i]->opacity > 0.0f)
				{
					charactersSprites[i]->opacity -= deltaTime * 2;
					fading = true;
				} else {
					charactersSprites[i]->opacity = 1.0f;
					charactersSprites[i]->animations = NULL;
					charactersNames[i] = NULL;
				}
			}
		}
		if (fading)
		{
			return false;
		}
	} else if (command->type == COMMAND_END) {
		end = true;
		moving = true;
		if (command->arguments[0]->string)
		{
			nextDialogName = strcopy(nextDialogName, "Dialogs/");
			nextDialogName = strappend(nextDialogName, command->arguments[0]->string);
			if (command->arguments[0]->string)
			{
				nextDialogStartKnotName = strcopy(nextDialogStartKnotName, command->arguments[1]->string);
			}
		}
	} else if (command->type == COMMAND_ASSIGN) {
		for (unsigned int i = 0; i < buf_len(variablesNames); i++)
		{
			if (strmatch(command->arguments[0]->string, variablesNames[i]))
			{
				free_variable(variablesValues[i]);
				variablesValues[i] = resolve_logic_expression(command->arguments[1]->logicExpression);
				return true;
			}
		}
		char *variableName = NULL;
		variableName = strcopy(variableName, command->arguments[0]->string);
		buf_add(variablesNames, variableName);
		buf_add(variablesValues, resolve_logic_expression(command->arguments[1]->logicExpression));
	} else if (command->type == COMMAND_GO_TO) {
		for (int i = 0; buf_len(interpretingDialog->knots); i++)
		{
			if (strmatch(interpretingDialog->knots[i]->name, command->arguments[0]->string))
			{
				interpretingDialog->currentKnot = i;
				moving = true;
				break;
			}
		}
	} else if (command->type == COMMAND_HIDE_UI) {
		displayDialogUI = false;
	} else {
		error("unknown command type %d", command->type);
	}
	return true;
}

static bool update_sentence(Sentence *sentence)
{
	if (sentenceFirstUpdate)
	{

		sentenceFirstUpdate = false;
		timeDuringCurrentChar = 0.0f;
		set_string_to_text(currentSentence, sentence->string);
		currentSentence->nbCharToDisplay = 0;
		if (currentSpeakerSpriteIndex != -1)
		{
			charactersSprites[currentSpeakerSpriteIndex]->animations[charactersSprites[currentSpeakerSpriteIndex]->currentAnimation]->updating = true;
		}
	}

	if (currentSentence->nbCharToDisplay < currentSentence->nbMaxCharToDisplay)
	{
		if (is_input_key_pressed(INPUT_KEY_SPACE))
		{
			currentSentence->nbCharToDisplay = currentSentence->nbMaxCharToDisplay;
		} else {
			timeDuringCurrentChar += deltaTime;
			if (timeDuringCurrentChar >= 0.02f)
			{
				int nbCharToSkip = timeDuringCurrentChar / 0.02f;
				currentSentence->nbCharToDisplay += nbCharToSkip;
				if (currentSentence->nbCharToDisplay >= currentSentence->nbMaxCharToDisplay)
				{
					currentSentence->nbCharToDisplay = currentSentence->nbMaxCharToDisplay;
				}
				timeDuringCurrentChar = 0.0f;
			}
		}

		if (currentSentence->nbCharToDisplay == currentSentence->nbMaxCharToDisplay)
		{
			if (currentSpeakerSpriteIndex != -1)
			{
				charactersSprites[currentSpeakerSpriteIndex]->animations[charactersSprites[currentSpeakerSpriteIndex]->currentAnimation]->stopping = true;
			}
		}
	} else if (is_input_key_pressed(INPUT_KEY_ENTER) || sentence->autoSkip) {
		sentenceFirstUpdate = true;
		set_string_to_text(currentSentence, NULL);
		if (currentSpeakerSpriteIndex != -1)
		{
			Animation *currentSpeakerSpriteAnimation = charactersSprites[currentSpeakerSpriteIndex]->animations[charactersSprites[currentSpeakerSpriteIndex]->currentAnimation];
			currentSpeakerSpriteAnimation->stopping = false;
			currentSpeakerSpriteAnimation->updating = false;
			currentSpeakerSpriteAnimation->currentAnimationPhase = 0;
			currentSpeakerSpriteAnimation->timeDuringCurrentAnimationPhase = 0.0f;
		}
		return true;
	}

	if (currentSpeakerSpriteIndex != -1)
	{
		Sprite *currentSpeakerSprite = charactersSprites[currentSpeakerSpriteIndex];
		AnimationPhase *currentSpeakerSpriteAnimationPhase = currentSpeakerSprite->animations[currentSpeakerSprite->currentAnimation]->animationPhases[currentSpeakerSprite->animations[currentSpeakerSprite->currentAnimation]->currentAnimationPhase];
		charactersSprites[currentSpeakerSpriteIndex]->position.x = (int)((windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentSpeakerSpriteAnimationPhase->width / 2));
		charactersSprites[currentSpeakerSpriteIndex]->position.y = (int)(windowDimensions.y - currentSpeakerSpriteAnimationPhase->height);
		charactersSprites[currentSpeakerSpriteIndex]->width = currentSpeakerSpriteAnimationPhase->width;
		charactersSprites[currentSpeakerSpriteIndex]->height = currentSpeakerSpriteAnimationPhase->height;
	}
	return false;
}

static bool update_cue_expression(CueExpression *cueExpression);

static bool update_cue_condition(CueCondition *cueCondition)
{
	if (!cueCondition->resolved)
	{
		Variable *variable = resolve_logic_expression(cueCondition->logicExpression);
		if (variable->type == VARIABLE_NUMERIC)
		{
			cueCondition->result = (bool)variable->numeric;
		} else if (variable->type == VARIABLE_STRING) {
			if (buf_len(variable->string) == 1)
			{
				if (variable->string[0] == '\0')
				{
					cueCondition->result = false;
				} else {
					cueCondition->result = true;
				}
			} else {
				cueCondition->result = false;
			}
		} else {
			error("unknown logic expression type %d.", variable->type);
		}
		free_variable(variable);
		cueCondition->resolved = true;
	}
	if (cueCondition->result)
	{
		if ((unsigned int)cueCondition->currentExpression == buf_len(cueCondition->cueExpressionsIf))
		{
			cueCondition->currentExpression = 0;
			return true;
		}
		if (update_cue_expression(cueCondition->cueExpressionsIf[cueCondition->currentExpression]))
		{
			if (moving)
			{
				cueCondition->currentExpression = 0;
			} else {
				cueCondition->currentExpression++;
			}
		}
	} else {
		if ((unsigned int)cueCondition->currentExpression == buf_len(cueCondition->cueExpressionsElse))
		{
			cueCondition->currentExpression = 0;
			return true;
		}
		if (update_cue_expression(cueCondition->cueExpressionsElse[cueCondition->currentExpression]))
		{
			if (moving)
			{
				cueCondition->currentExpression = 0;
			} else {
				cueCondition->currentExpression++;
			}
		}
	}
	return false;
}

static bool update_cue_expression(CueExpression *cueExpression)
{
	if (cueExpression->type == CUE_EXPRESSION_SENTENCE)
	{
		return update_sentence(cueExpression->sentence);
	} else if (cueExpression->type == CUE_EXPRESSION_CHOICE) {
		choosing = true;
	} else if (cueExpression->type == CUE_EXPRESSION_CUE_CONDITION) {
		return update_cue_condition(cueExpression->cueCondition);
	} else if (cueExpression->type == CUE_EXPRESSION_COMMAND) {
		return update_command(cueExpression->command);
	} else {
		error("unknown cue expression type %d.", cueExpression->type);
	}
	return false;
}

static void display_choice(CueExpression *cueExpression)
{
	if (cueExpression->type == CUE_EXPRESSION_CHOICE)
	{
		if ((unsigned int)nbChoices == buf_len(currentChoices))
		{
			buf_add(currentChoices, create_text());
			set_font_to_text(currentChoices[nbChoices], "Fonts/arial.ttf", TEXT_SIZE_NORMAL);
			currentChoices[nbChoices]->position.x = (int)(0.015f * windowDimensions.x);
			if (nbChoices == 0)
			{
				currentChoices[nbChoices]->position.y = (int)(0.8f * windowDimensions.y + 2);
			}
			currentChoices[nbChoices]->color.x = 1.0f;
			currentChoices[nbChoices]->color.y = 1.0f;
			currentChoices[nbChoices]->color.z = 1.0f;
		}
		if (nbChoices != 0)
		{
			currentChoices[nbChoices]->position.y = currentChoices[nbChoices - 1]->position.y + currentChoices[nbChoices - 1]->height + 4;
		}
		set_string_to_text(currentChoices[nbChoices], cueExpression->choice->sentence->string);
		buf_add(goToCommands, cueExpression->choice->goToCommand);
		nbChoices++;
	} else if (cueExpression->type == CUE_EXPRESSION_CUE_CONDITION) {
		if (!cueExpression->cueCondition->resolved)
		{
			Variable *variable = resolve_logic_expression(cueExpression->cueCondition->logicExpression);
			if (variable->type == VARIABLE_NUMERIC)
			{
				cueExpression->cueCondition->result = (bool)variable->numeric;
			} else if (variable->type == VARIABLE_STRING) {
				if (buf_len(variable->string) == 1)
				{
					if (variable->string[0] == '\0')
					{
						cueExpression->cueCondition->result = false;
					} else {
						cueExpression->cueCondition->result = true;
					}
				} else {
					cueExpression->cueCondition->result = false;
				}
			} else {
				error("unknown logic expression type %d.");
			}
			free_variable(variable);
			cueExpression->cueCondition->resolved = true;
		}
		if (cueExpression->cueCondition->result)
		{
			for (unsigned int i = cueExpression->cueCondition->currentExpression; i < buf_len(cueExpression->cueCondition->cueExpressionsIf); i++)
			{
				display_choice(cueExpression->cueCondition->cueExpressionsIf[cueExpression->cueCondition->currentExpression]);
			}
		} else {
			for (unsigned int i = cueExpression->cueCondition->currentExpression; i < buf_len(cueExpression->cueCondition->cueExpressionsElse); i++)
			{
				display_choice(cueExpression->cueCondition->cueExpressionsElse[cueExpression->cueCondition->currentExpression]);
			}
		}
	} else {
		error("got sentence or command after entering choice mode.");
	}
}

static bool update_cue(Cue *cue)
{
	if (!choosing)
	{
		if (!displayedSpeakerName)
		{
			displayDialogUI = true;
			displayedSpeakerName = true;
			currentSpeakerSpriteIndex = -1;
			if (cue->characterName)
			{
				displaySpeakerName = true;
				if (cue->setCharacterInDeclaration)
				{
					update_command(cue->cueExpressions[0]->command);
				}
				for (int i = 0; i < 7; i++)
				{
					if (charactersNames[i])
					{
						if (strmatch(cue->characterName, charactersNames[i]))
						{
							currentSpeakerSpriteIndex = i;
							break;
						}
					}
				}
				set_string_to_text(currentSpeaker, cue->characterName);
				ivec2 currentSpeakerPosition;
				if (cue->characterNamePosition == 1)
				{
					currentSpeakerPosition.x = (int)(0.015f * windowDimensions.x);
				} else {
					currentSpeakerPosition.x = (int)(windowDimensions.x * 0.985f - currentSpeaker->width);
				}
				currentSpeakerPosition.y = (int)(windowDimensions.y * 0.8f - currentSpeaker->height - 2);
				set_position_to_text(currentSpeaker, currentSpeakerPosition);
				characterNameBox->position.x = currentSpeaker->position.x - 2;
				characterNameBox->position.y = currentSpeaker->position.y - currentSpeaker->font->descent - 2;
				characterNameBox->width = currentSpeaker->width + 4;
				characterNameBox->height = currentSpeaker->height + currentSpeaker->font->descent + 4;
			} else {
				displaySpeakerName = false;
			}
		}

		if (cue->setCharacterInDeclaration && appearingCharacter && cue->currentExpression == 0)
		{
			if (!update_command(cue->cueExpressions[0]->command))
			{
				return false;
			} else {
				cue->currentExpression = 1;
			}
		}

		if ((unsigned int)cue->currentExpression == buf_len(cue->cueExpressions))
		{
			cue->currentExpression = 0;
			displayedSpeakerName = false;
			set_string_to_text(currentSpeaker, NULL);
			return true;
		}

		if (update_cue_expression(cue->cueExpressions[cue->currentExpression]))
		{
			cue->currentExpression++;
			if (moving)
			{
				cue->currentExpression = 0;
				displayedSpeakerName = false;
				set_string_to_text(currentSpeaker, NULL);
				return true;
			}
		}
	} else {
		if (!choicesDisplayed)
		{
			nbChoices = 0;
			for (unsigned int i = cue->currentExpression; i < buf_len(cue->cueExpressions); i++)
			{
				display_choice(cue->cueExpressions[i]);
			}
			choicesDisplayed = true;
			choiceMarker->position.y = currentChoices[0]->position.y + ((currentChoices[0]->height - choiceMarker->height) / 2) - currentChoices[0]->font->descent;
		}

		if (is_input_key_pressed(INPUT_KEY_DOWN_ARROW))
		{
			currentChoice++;
			if ((unsigned int)currentChoice == buf_len(currentChoices))
			{
				currentChoice = 0;
			}
			choiceMarker->position.y = currentChoices[currentChoice]->position.y + ((currentChoices[currentChoice]->height - choiceMarker->height) / 2) - currentChoices[currentChoice]->font->descent;
		} else if (is_input_key_pressed(INPUT_KEY_UP_ARROW)) {
			currentChoice--;
			if (currentChoice == -1)
			{
				currentChoice = buf_len(currentChoices) - 1;
			}
			choiceMarker->position.y = currentChoices[currentChoice]->position.y + ((currentChoices[currentChoice]->height - choiceMarker->height) / 2) - currentChoices[currentChoice]->font->descent;
		} else if (is_input_key_pressed(INPUT_KEY_ENTER)) {
			update_command(goToCommands[currentChoice]);
			buf_free(goToCommands);
			for (unsigned int i = 0; i < buf_len(currentChoices); i++)
			{
				set_string_to_text(currentChoices[i], NULL);
			}
			goToCommands = NULL;
			cue->currentExpression = 0;
			choosing = false;
			choicesDisplayed = false;
			currentChoice = 0;
			displayedSpeakerName = false;
			set_string_to_text(currentSpeaker, NULL);
			return true;
		}
	}
	return false;
}

static bool update_knot_expression(KnotExpression *knotExpression);

static bool update_knot_condition(KnotCondition *knotCondition)
{
	if (!knotCondition->resolved)
	{
		Variable *variable = resolve_logic_expression(knotCondition->logicExpression);
		if (variable->type == VARIABLE_NUMERIC)
		{
			knotCondition->result = (bool)variable->numeric;
		} else if (variable->type == VARIABLE_STRING) {
			if (buf_len(variable->string) == 1)
			{
				if (variable->string[0] == '\0')
				{
					knotCondition->result = false;
				} else {
					knotCondition->result = true;
				}
			} else {
				knotCondition->result = false;
			}
		} else {
			error("unknown logic expression type %d.");
		}
		free_variable(variable);
		knotCondition->resolved = true;
	}
	if (knotCondition->result)
	{
		if ((unsigned int)knotCondition->currentExpression == buf_len(knotCondition->knotExpressionsIf))
		{
			knotCondition->currentExpression = 0;
			return true;
		}
		if (update_knot_expression(knotCondition->knotExpressionsIf[knotCondition->currentExpression]))
		{
			if (moving)
			{
				knotCondition->currentExpression = 0;
			} else {
				knotCondition->currentExpression++;
			}
		}
	} else {
		if ((unsigned int)knotCondition->currentExpression == buf_len(knotCondition->knotExpressionsElse))
		{
			knotCondition->currentExpression = 0;
			return true;
		}
		if (update_knot_expression(knotCondition->knotExpressionsElse[knotCondition->currentExpression]))
		{
			if (moving)
			{
				knotCondition->currentExpression = 0;
			} else {
				knotCondition->currentExpression++;
			}
		}
	}
	return false;
}

static bool update_knot_expression(KnotExpression *knotExpression)
{
	if (knotExpression->type == KNOT_EXPRESSION_CUE)
	{
		if (update_cue(knotExpression->cue))
		{
			return true;
		}
	} else if (knotExpression->type == KNOT_EXPRESSION_KNOT_CONDITION) {
		if (update_knot_condition(knotExpression->knotCondition))
		{
			return true;
		}
	} else if (knotExpression->type == KNOT_EXPRESSION_COMMAND) {
		return update_command(knotExpression->command);
	} else {
		error("unknown knot expression type %d", knotExpression->type);
	}
	return false;
}

static bool update_knot(Knot *knot)
{
	if ((unsigned int)knot->currentExpression == buf_len(knot->knotExpressions))
	{
		knot->currentExpression = 0;
		return true;
	} else {
		if (update_knot_expression(knot->knotExpressions[knot->currentExpression]))
		{
			if (moving)
			{
				knot->currentExpression = 0;
				moving = false;
			} else {
				knot->currentExpression++;
			}
		}
		return false;
	}
}

bool interpret_current_dialog()
{
	if (dialogChanged)
	{
		dialogChanged = false;
		backgroundSprite->animations = NULL;
		oldBackgroundSprite->animations = NULL;
		for (int i = 0; i < 7; i++)
		{
			charactersNames[i] = NULL;
			oldCharactersSprites[i]->animations = NULL;
			charactersSprites[i]->animations = NULL;
		}
		currentSpeakerSpriteIndex = -1;
		choosing = false;
		nbChoices = 0;
		currentChoice = 0;
		choicesDisplayed = false;
		moving = false;
		end = false;
		displayedSpeakerName = false;
		appearingBackground = false;
		appearingCharacter = false;
		displayDialogUI = false;
		displaySpeakerName = false;
		sentenceFirstUpdate = true;

		if (nextDialogStartKnotName)
		{
			int knotIndex = -1;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->knots); i++)
			{
				if (strmatch(interpretingDialog->knots[i]->name, nextDialogStartKnotName))
				{
					knotIndex = i;
					break;
				}
			}
			if (knotIndex == -1)
			{
				error("could not find knot labeled %s in %s.", nextDialogStartKnotName, interpretingDialogName);
			} else {
				interpretingDialog->currentKnot = knotIndex;
			}

			buf_free(nextDialogStartKnotName);
			nextDialogStartKnotName = NULL;
		}
	}

	if ((unsigned int)interpretingDialog->currentKnot == buf_len(interpretingDialog->knots))
	{
		return false;
	}

	if (update_knot(interpretingDialog->knots[interpretingDialog->currentKnot]))
	{
		interpretingDialog->currentKnot++;
	}

	if (end)
	{
		end = false;
		if (!nextDialogName)
		{
			return false;
		} else {
			interpretingDialog->currentKnot = 0;
		}
	}

	if (oldBackgroundSprite->animations)
	{
		add_sprite_to_draw_list(oldBackgroundSprite, DRAW_LAYER_BACKGROUND);
	}
	if (backgroundSprite->animations)
	{
		add_sprite_to_draw_list(backgroundSprite, DRAW_LAYER_BACKGROUND);
	}
	for (int i = 0; i < 7; i++)
	{
		if (oldCharactersSprites[i]->animations)
		{
			add_sprite_to_draw_list(oldCharactersSprites[i], DRAW_LAYER_FOREGROUND);
		}
		if (charactersSprites[i]->animations)
		{
			add_sprite_to_draw_list(charactersSprites[i], DRAW_LAYER_FOREGROUND);
		}
	}

	if (displayDialogUI)
	{
		add_sprite_to_draw_list(sentenceBox, DRAW_LAYER_UI);
		if (!choosing)
		{
			add_text_to_draw_list(currentSentence, DRAW_LAYER_UI);
		} else {
			add_sprite_to_draw_list(choiceMarker, DRAW_LAYER_UI);
			for (int i = 0; i < nbChoices; i++)
			{
				add_text_to_draw_list(currentChoices[i], DRAW_LAYER_UI);
			}
		}

		if (displaySpeakerName)
		{
			add_sprite_to_draw_list(characterNameBox, DRAW_LAYER_UI);
			add_text_to_draw_list(currentSpeaker, DRAW_LAYER_UI);
		}
	}
	return true;
}
