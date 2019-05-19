#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <stdio.h>

#include "audio.h"
#include "window.h"
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
#include "globals_dialog.h"
#include "globals.h"

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
static float timeDuringCurrentBlip;
static float waitTimer;
static char *nextDialogStartKnotName;
static AudioSource *music;
static AudioSource *oldMusic;
static AudioSource *sound;
static AudioSource *oldSound;
static AudioSource *blipSound;
static bool fadingMusic;
static bool fadingSound;

void init_dialog_ui()
{
	nextDialogStartKnotName = NULL;

	vec3 black = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
	vec3 white = {.x = 1.0f, .y = 1.0f, .z = 1.0f};

	currentSentence = create_text();
	set_text_font(currentSentence, "Fonts/OpenSans-Regular.ttf", TEXT_SIZE_NORMAL);
	set_text_width_limit(currentSentence, (int)(0.97f * windowDimensions.x));
	currentSentence->position.x = (int)(0.015f * windowDimensions.x);
	currentSentence->position.y = (int)(0.8f * windowDimensions.y - 4);
	currentSentence->color = white;

	currentSpeaker = create_text();
	set_text_font(currentSpeaker, "Fonts/OpenSans-Regular.ttf", TEXT_SIZE_BIG);
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
	oldBackgroundSprite->fixedSize = true;
	backgroundSprite = create_sprite(SPRITE_ANIMATED);
	backgroundSprite->position.x = 0;
	backgroundSprite->position.y = 0;
	backgroundSprite->width = windowDimensions.x;
	backgroundSprite->height = windowDimensions.y;
	backgroundSprite->fixedSize = true;

	for (int i = 0; i < 7; i++)
	{
		charactersNames[i] = NULL;
		oldCharactersSprites[i] = create_sprite(SPRITE_ANIMATED);
		oldCharactersSprites[i]->fixedSize = false;
		charactersSprites[i] = create_sprite(SPRITE_ANIMATED);
		charactersSprites[i]->fixedSize = false;
	}
	currentChoices = NULL;
	goToCommands = NULL;

	waitTimer = 0.0f;

	music = NULL;
	oldMusic = NULL;
	sound = NULL;
	oldSound = NULL;

	blipSound = create_audio_source("Sounds/blip normal.wav");
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

	if (music)
	{
		stop_audio_source(music);
		xfree(music);
	}

	if (oldMusic)
	{
		stop_audio_source(oldMusic);
		xfree(oldMusic);
	}

	if (sound)
	{
		stop_audio_source(sound);
		xfree(sound);
	}

	if (oldSound)
	{
		stop_audio_source(oldSound);
		xfree(oldSound);
	}

	stop_audio_source(blipSound);
	xfree(blipSound);
}

static bool update_command(Command *command)
{
	if (command->type == COMMAND_SET_BACKGROUND)
	{
		displayDialogUI = false;
		if (!appearingBackground)
		{
			char *backgroundName = command->arguments[0]->string;
			char *animationName = command->arguments[1]->string;
			bool foundPack = false;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->backgroundPacksNames); i++)
			{
				if (strmatch(backgroundName, interpretingDialog->backgroundPacksNames[i]))
				{
					if (backgroundSprite->animations)
					{
						oldBackgroundSprite->animations = backgroundSprite->animations;
						oldBackgroundSprite->currentAnimation = backgroundSprite->currentAnimation;
					}
					backgroundSprite->animations = interpretingDialog->backgroundPacks[i];
					foundPack = true;
					bool foundAnimation = false;
					for (unsigned int j = 0; j < buf_len(backgroundSprite->animations); j++)
					{
						if (strmatch(animationName, backgroundSprite->animations[j]->name))
						{
							backgroundSprite->currentAnimation = j;
							backgroundSprite->animations[j]->currentAnimationPhase = 0;
							if (backgroundSprite->animations == oldBackgroundSprite->animations && oldBackgroundSprite->currentAnimation == j && oldBackgroundSprite->animations[j]->currentAnimationPhase == 0)
							{
								oldBackgroundSprite->animations = NULL;
								return true;
							}
							foundAnimation = true;
							break;
						}
					}
					if (!foundAnimation)
					{
						error("background %s of background pack %s does not exist.", animationName, backgroundName);
					}
					break;
				}
			}
			if (!foundPack)
			{
				error("background pack %s does not exist.", backgroundName);
			}
			backgroundSprite->opacity = 0.0f;
			appearingBackground = true;
			return false;
		} else {
			if (backgroundSprite->opacity >= 1.0f)
			{
				if (oldBackgroundSprite->animations)
				{
					oldBackgroundSprite->opacity = 1.0f;
					oldBackgroundSprite->animations = NULL;
				}
				backgroundSprite->opacity = 1.0f;
				appearingBackground = false;
			} else {
				if (oldBackgroundSprite->animations)
				{
					oldBackgroundSprite->opacity -= deltaTime;
				}
				backgroundSprite->opacity += deltaTime;
				return false;
			}
		}
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
		Sprite *characterSprite = charactersSprites[position];
		Sprite *oldCharacterSprite = oldCharactersSprites[position];
		if (!appearingCharacter)
		{
			char *characterName = command->arguments[1]->string;
			char *animationName = command->arguments[2]->string;
			bool foundCharacter = false;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->charactersNames); i++)
			{
				if (strmatch(characterName, interpretingDialog->charactersNames[i]))
				{
					charactersNames[position] = interpretingDialog->charactersNames[i];
					if (characterSprite->animations)
					{
						oldCharacterSprite->animations = characterSprite->animations;
						oldCharacterSprite->position.x = characterSprite->position.x;
						oldCharacterSprite->position.y = characterSprite->position.y;
						oldCharacterSprite->currentAnimation = characterSprite->currentAnimation;
						oldCharacterSprite->animations[oldCharacterSprite->currentAnimation]->currentAnimationPhase = 0;
					}
					characterSprite->animations = interpretingDialog->charactersAnimations[i];
					bool foundAnimation = false;
					for (unsigned int j = 0; j < buf_len(characterSprite->animations); j++)
					{
						if (strmatch(animationName, characterSprite->animations[j]->name))
						{
							characterSprite->position.x = (int)((windowDimensions.x * position / 6.0f) - (characterSprite->animations[j]->animationPhases[0]->width / 2));
							characterSprite->position.y = (int)(windowDimensions.y - characterSprite->animations[j]->animationPhases[0]->height);
							characterSprite->currentAnimation = j;
							characterSprite->animations[characterSprite->currentAnimation]->currentAnimationPhase = 0;
							if (characterSprite->animations == oldCharacterSprite->animations && oldCharacterSprite->currentAnimation == j && oldCharacterSprite->animations[j]->currentAnimationPhase == 0)
							{
								oldCharacterSprite->animations = NULL;
								return true;
							}
							foundAnimation = true;
							break;
						}
					}
					if (!foundAnimation)
					{
						error("animation %s of character %s does not exist.", animationName, characterName);
					}
					foundCharacter = true;
					break;
				}
			}
			if (!foundCharacter)
			{
				error("character %s does not exist.", characterName);
			}
			characterSprite->opacity = 0.0f;
			appearingCharacter = true;
			return false;
		} else {
			if (characterSprite->opacity >= 1.0f)
			{
				if (oldCharacterSprite->animations)
				{
					oldCharacterSprite->opacity = 1.0f;
					oldCharacterSprite->animations = NULL;
				}
				characterSprite->opacity = 1.0f;
				appearingCharacter = false;
			} else {
				if (oldCharacterSprite->animations)
				{
					oldCharacterSprite->opacity -= deltaTime * 2;
				}
				characterSprite->opacity += deltaTime * 2;
				return false;
			}
		}
	} else if (command->type == COMMAND_CLEAR_CHARACTER_POSITION) {
		int position = (int)command->arguments[0]->numeric;
		Sprite *characterSprite = charactersSprites[position];
		if (characterSprite->opacity > 0.0f)
		{
			characterSprite->opacity -= deltaTime * 2;
			return false;
		} else {
			characterSprite->opacity = 1.0f;
			characterSprite->animations = NULL;
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
	} else if (command->type == COMMAND_PLAY_MUSIC) {
		if (!fadingMusic)
		{
			bool foundMusic = false;
			char *musicName = command->arguments[0]->string;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->musicsNames); i++)
			{
				if (strmatch(musicName, interpretingDialog->musicsNames[i]))
				{
					if (music)
					{
						oldMusic = music;
					}
					char *newMusicName = NULL;
					newMusicName = strcopy(newMusicName, "Musics/");
					newMusicName = strappend(newMusicName, musicName);
					music = create_audio_source(newMusicName);
					buf_free(newMusicName);
					foundMusic = true;
					break;
				}
			}
			if (!foundMusic)
			{
				error("music %s does not exist.", musicName);
			}
			fadingMusic = true;
			music->volume = 0.0f;
			music->playing = true;
			return false;
		} else {
			if (music->volume >= 1.0f)
			{
				if (oldMusic)
				{
					stop_audio_source(oldMusic);
					xfree(oldMusic);
					oldMusic = NULL;
				}
				music->volume = 1.0f;
				fadingMusic = false;
			} else {
				if (oldMusic)
				{
					oldMusic->volume -= deltaTime * 1.0f;
				}
				music->volume += deltaTime * 1.0f;
				return false;
			}
		}
	} else if (command->type == COMMAND_STOP_MUSIC) {
		stop_audio_source(music);
		xfree(music);
		music = NULL;
	} else if (command->type == COMMAND_SET_MUSIC_VOLUME) {
		music->volume = command->arguments[0]->numeric;
	} else if (command->type == COMMAND_PLAY_SOUND) {
		if (!fadingSound)
		{
			bool foundSound = false;
			char *soundName = command->arguments[0]->string;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->soundsNames); i++)
			{
				if (strmatch(soundName, interpretingDialog->soundsNames[i]))
				{
					if (sound)
					{
						oldSound = sound;
					}
					char *newSoundName = NULL;
					newSoundName = strcopy(newSoundName, "Sounds/");
					newSoundName = strappend(newSoundName, soundName);
					sound = create_audio_source(newSoundName);
					buf_free(newSoundName);
					foundSound = true;
					break;
				}
			}
			if (!foundSound)
			{
				error("sound %s does not exist.", soundName);
			}
			fadingSound = true;
			sound->volume = 0.0f;
			sound->playing = true;
			return false;
		} else {
			if (sound->volume >= 1.0f)
			{
				if (oldSound)
				{
					stop_audio_source(oldSound);
					xfree(oldSound);
					oldSound = NULL;
				}
				sound->volume = 1.0f;
				fadingSound = false;
			} else {
				if (oldSound)
				{
					oldSound->volume -= deltaTime * 1.0f;
				}
				sound->volume += deltaTime * 1.0f;
				return false;
			}
		}
	} else if (command->type == COMMAND_STOP_SOUND) {
		stop_audio_source(sound);
		xfree(sound);
		sound = NULL;
	} else if (command->type == COMMAND_SET_SOUND_VOLUME) {
		sound->volume = command->arguments[0]->numeric;
	} else if (command->type == COMMAND_END) {
		end = true;
		moving = true;
		if (command->arguments[0]->string)
		{
			nextDialogName = strcopy(nextDialogName, "Dialogs/");
			nextDialogName = strappend(nextDialogName, command->arguments[0]->string);
			if (command->arguments[1]->string)
			{
				nextDialogStartKnotName = strcopy(nextDialogStartKnotName, command->arguments[1]->string);
			}
		}
	} else if (command->type == COMMAND_ASSIGN) {
		bool foundVariable = false;
		char *variableName = command->arguments[0]->string;
		LogicExpression *assignedValue = command->arguments[1]->logicExpression;
		for (unsigned int i = 0; i < buf_len(variablesNames); i++)
		{
			if (strmatch(variableName, variablesNames[i]))
			{
				free_variable(variablesValues[i]);
				variablesValues[i] = resolve_logic_expression(assignedValue);
				foundVariable = true;
			}
		}
		if (!foundVariable)
		{
			char *newVariableName = NULL;
			newVariableName = strcopy(newVariableName, variableName);
			buf_add(variablesNames, newVariableName);
			buf_add(variablesValues, resolve_logic_expression(assignedValue));
		}
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
	} else if (command->type == COMMAND_WAIT) {
		if (waitTimer == 0.0f)
		{
			waitTimer = command->arguments[0]->numeric;
		}
		waitTimer -= deltaTime * 1000;
		if (waitTimer <= 0.0f)
		{
			waitTimer = 0.0f;
		} else {
			return false;
		}
	} else if (command->type == COMMAND_SET_WINDOW_NAME) {
		set_window_name(command->arguments[0]->string);
		return true;
	} else if (command->type == COMMAND_SET_NAME_COLOR) {
		bool foundColoredName = false;
		char *nameToColor = command->arguments[0]->string;
		vec3 newNameColor = {.x = command->arguments[1]->numeric, .y = command->arguments[2]->numeric, .z = command->arguments[3]->numeric};
		for (unsigned int i = 0; i < buf_len(interpretingDialog->coloredNames); i++)
		{
			if (strmatch(nameToColor, interpretingDialog->coloredNames[i]))
			{
				interpretingDialog->namesColors[i] = newNameColor;
				foundColoredName = true;
				break;
			}
		}
		if (!foundColoredName)
		{
			char *newColoredName = NULL;
			newColoredName = strcopy(newColoredName, nameToColor);
			buf_add(interpretingDialog->coloredNames, newColoredName);
			buf_add(interpretingDialog->namesColors, newNameColor);
		}
	} else {
		error("unknown command type %d", command->type);
	}
	return true;
}

static bool update_sentence(Sentence *sentence)
{
	Sprite *currentSpeakerSprite = NULL;
	Animation *currentAnimation = NULL;
	AnimationPhase *currentAnimationPhase = NULL;

	if (currentSpeakerSpriteIndex != -1)
	{
		currentSpeakerSprite = charactersSprites[currentSpeakerSpriteIndex];
		currentAnimation = currentSpeakerSprite->animations[currentSpeakerSprite->currentAnimation];
		currentAnimationPhase = currentAnimation->animationPhases[currentAnimation->currentAnimationPhase];
	}

	if (sentenceFirstUpdate)
	{
		sentenceFirstUpdate = false;
		timeDuringCurrentChar = 0.0f;
		timeDuringCurrentBlip = 0.0f;
		set_text_string(currentSentence, sentence->string);
		currentSentence->nbCharToDisplay = 0;
		if (currentSpeakerSpriteIndex != -1)
		{
			currentAnimation->updating = true;
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
				int nbCharToSkip = (int)(timeDuringCurrentChar / 0.02f);
				currentSentence->nbCharToDisplay += nbCharToSkip;
				if (currentSentence->nbCharToDisplay >= currentSentence->nbMaxCharToDisplay)
				{
					currentSentence->nbCharToDisplay = currentSentence->nbMaxCharToDisplay;
				}
				timeDuringCurrentChar -= nbCharToSkip * 0.02f;
			}
			timeDuringCurrentBlip += deltaTime;
			if (timeDuringCurrentBlip >= 0.075f)
			{
				timeDuringCurrentBlip = 0.0f;
				reset_audio_source(blipSound);
				blipSound->playing = true;
			}
		}

		if (currentSentence->nbCharToDisplay == currentSentence->nbMaxCharToDisplay)
		{
			if (currentSpeakerSpriteIndex != -1)
			{
				currentAnimation->stopping = true;
			}
		}
	} else if (is_input_key_pressed(INPUT_KEY_ENTER) || sentence->autoSkip) {
		sentenceFirstUpdate = true;
		set_text_string(currentSentence, NULL);
		if (currentSpeakerSpriteIndex != -1)
		{
			currentAnimation->stopping = false;
			currentAnimation->updating = false;
			currentAnimation->currentAnimationPhase = 0;
			currentAnimation->timeDuringCurrentAnimationPhase = 0.0f;
		}
		return true;
	}

	if (currentSpeakerSpriteIndex != -1)
	{
		currentSpeakerSprite->position.x = (int)((windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentAnimationPhase->width / 2));
		currentSpeakerSprite->position.y = (int)(windowDimensions.y - currentAnimationPhase->height);
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
				return true;
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
				return true;
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
			set_text_font(currentChoices[nbChoices], "Fonts/OpenSans-Regular.ttf", TEXT_SIZE_NORMAL);
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
		set_text_string(currentChoices[nbChoices], cueExpression->choice->sentence->string);
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
				set_text_string(currentSpeaker, cue->characterName);
				vec3 nameColor = {.x = -1.0f};
				for (unsigned int i = 0; i < buf_len(interpretingDialog->coloredNames); i++)
				{
					if (strmatch(cue->characterName, interpretingDialog->coloredNames[i]))
					{
						nameColor = interpretingDialog->namesColors[i];
					}
				}
				if (nameColor.x != -1.0f)
				{
					currentSpeaker->color = nameColor;
				}
				ivec2 currentSpeakerPosition;
				if (cue->characterNamePosition == 1)
				{
					currentSpeakerPosition.x = (int)(0.015f * windowDimensions.x);
				} else {
					currentSpeakerPosition.x = (int)(windowDimensions.x * 0.985f - currentSpeaker->width);
				}
				currentSpeakerPosition.y = (int)(windowDimensions.y * 0.8f - currentSpeaker->height - 8);
				set_text_position(currentSpeaker, currentSpeakerPosition);
				characterNameBox->position.x = currentSpeaker->position.x - 2;
				characterNameBox->position.y = currentSpeaker->position.y - currentSpeaker->font->descent + 6;
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
			set_text_string(currentSpeaker, NULL);
			currentSpeaker->color.x = 1.0f;
			currentSpeaker->color.y = 1.0f;
			currentSpeaker->color.z = 1.0f;
			return true;
		}

		if (update_cue_expression(cue->cueExpressions[cue->currentExpression]))
		{
			cue->currentExpression++;
			if (moving)
			{
				cue->currentExpression = 0;
				displayedSpeakerName = false;
				set_text_string(currentSpeaker, NULL);
				currentSpeaker->color.x = 1.0f;
				currentSpeaker->color.y = 1.0f;
				currentSpeaker->color.z = 1.0f;
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
		}
		if (is_input_key_pressed(INPUT_KEY_DOWN_ARROW))
		{
			currentChoice++;
			if ((unsigned int)currentChoice == buf_len(currentChoices))
			{
				currentChoice = 0;
			}
		} else if (is_input_key_pressed(INPUT_KEY_UP_ARROW)) {
			currentChoice--;
			if (currentChoice == -1)
			{
				currentChoice = buf_len(currentChoices) - 1;
			}
		} else if (is_input_key_pressed(INPUT_KEY_ENTER)) {
			update_command(goToCommands[currentChoice]);
			buf_free(goToCommands);
			for (unsigned int i = 0; i < buf_len(currentChoices); i++)
			{
				set_text_string(currentChoices[i], NULL);
			}
			goToCommands = NULL;
			cue->currentExpression = 0;
			choosing = false;
			choicesDisplayed = false;
			currentChoice = 0;
			displayedSpeakerName = false;
			set_text_string(currentSpeaker, NULL);
			currentSpeaker->color.x = 1.0f;
			currentSpeaker->color.y = 1.0f;
			currentSpeaker->color.z = 1.0f;
			return true;
		}
		choiceMarker->position.y = currentChoices[currentChoice]->position.y + ((currentChoices[currentChoice]->height - choiceMarker->height) / 2) - currentChoices[currentChoice]->font->descent;
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
				return true;
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
				return true;
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
		return update_cue(knotExpression->cue);
	} else if (knotExpression->type == KNOT_EXPRESSION_KNOT_CONDITION) {
		return update_knot_condition(knotExpression->knotCondition);
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
		if (sound)
		{
			stop_audio_source(sound);
			xfree(sound);
		}
		sound = NULL;
		if (oldSound)
		{
			stop_audio_source(oldSound);
			xfree(oldSound);
		}
		oldSound = NULL;
		if (music)
		{
			stop_audio_source(music);
			xfree(music);
		}
		music = NULL;
		if (oldMusic)
		{
			stop_audio_source(oldMusic);
			xfree(oldMusic);
		}
		oldMusic = NULL;
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
		fadingSound = false;
		fadingMusic = false;

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
