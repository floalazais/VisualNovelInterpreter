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
#include "str.h"
#include "animation.h"
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
static int characterNamePosition;
static Sprite *sentenceBox;
static Text *currentSpeaker;
static Text *currentSentence;
static buf(Text *) currentChoices;
static buf(GoTo *) goToCommands;
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
static buf(char) nextDialogStartKnotName;
static AudioSource *music;
static AudioSource *oldMusic;
static AudioSource *sound;
static AudioSource *oldSound;
static AudioSource *blipSound;
static bool fadingMusic;
static bool fadingSound;
static int textScrollOffset;

void init_dialog_ui()
{
	nextDialogStartKnotName = NULL;

	currentSentence = create_text();
	set_text_font(currentSentence, "Fonts/OpenSans-Regular.ttf", TEXT_SIZE_NORMAL);
	currentSentence->color = COLOR_WHITE;

	currentSpeaker = create_text();
	set_text_font(currentSpeaker, "Fonts/OpenSans-Regular.ttf", TEXT_SIZE_BIG);
	currentSpeaker->color = COLOR_WHITE;

	sentenceBox = create_sprite(SPRITE_COLOR);
	sentenceBox->color = COLOR_BLACK;
	sentenceBox->opacity = 0.75f;

	characterNameBox = create_sprite(SPRITE_COLOR);
	characterNameBox->color = COLOR_BLACK;
	characterNameBox->opacity = 0.75f;

	choiceMarker = create_sprite(SPRITE_COLOR);
	choiceMarker->color = COLOR_WHITE;

	oldBackgroundSprite = create_sprite(SPRITE_ANIMATED);
	oldBackgroundSprite->fixedSize = true;
	backgroundSprite = create_sprite(SPRITE_ANIMATED);
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

	currentSpeakerSpriteIndex = -1;

	reset_dialog_ui();

	waitTimer = 0.0f;

	music = NULL;
	oldMusic = NULL;
	sound = NULL;
	oldSound = NULL;

	blipSound = create_audio_source("Sounds/blip normal.wav");
}

void reset_dialog_ui()
{
	set_text_position(currentSentence, (ivec2){0.015f * windowDimensions.x, 0.8f * windowDimensions.y - 4});
	set_text_width_limit(currentSentence, 0.97f * windowDimensions.x);

	sentenceBox->position.y = 0.8f * windowDimensions.y;
	sentenceBox->width = windowDimensions.x;
	sentenceBox->height = 0.2f * windowDimensions.y;

	choiceMarker->position.x = 0.005f * windowDimensions.x;
	choiceMarker->width = 0.005f * windowDimensions.x;
	choiceMarker->height = choiceMarker->width;

	oldBackgroundSprite->width = windowDimensions.x;
	oldBackgroundSprite->height = windowDimensions.y;

	backgroundSprite->width = windowDimensions.x;
	backgroundSprite->height = windowDimensions.y;

	for (int position = 0; position < 7; position++)
	{
		Sprite *characterSprite = charactersSprites[position];
		if (!characterSprite->animations)
		{
			continue;
		}
		Animation *currentAnimation = characterSprite->animations[characterSprite->currentAnimation];
		if (currentAnimation->animationPhases[0]->responsive)
		{
			characterSprite->position.x = (windowDimensions.x * position / 6.0f) - (currentAnimation->animationPhases[0]->responsiveWidth * windowDimensions.x / 2);
			characterSprite->position.y = windowDimensions.y - (currentAnimation->animationPhases[0]->responsiveHeight * windowDimensions.y);
		} else {
			characterSprite->position.x = (windowDimensions.x * position / 6.0f) - (currentAnimation->animationPhases[0]->pixelWidth / 2);
			characterSprite->position.y = windowDimensions.y - currentAnimation->animationPhases[0]->pixelHeight;
		}
	}

	if (currentSpeakerSpriteIndex != -1)
	{
		Sprite *currentSpeakerSprite = charactersSprites[currentSpeakerSpriteIndex];
		if (currentSpeakerSprite->currentAnimation != -1 && currentSpeakerSprite->animations[currentSpeakerSprite->currentAnimation]->currentAnimationPhase != -1)
		{
			AnimationPhase *currentAnimationPhase = currentSpeakerSprite->animations[currentSpeakerSprite->currentAnimation]->animationPhases[currentSpeakerSprite->animations[currentSpeakerSprite->currentAnimation]->currentAnimationPhase];
			if (currentAnimationPhase->responsive)
			{
				currentSpeakerSprite->position.x = (windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentAnimationPhase->responsiveWidth * windowDimensions.x / 2);
				currentSpeakerSprite->position.y = windowDimensions.y - (currentAnimationPhase->responsiveHeight * windowDimensions.y);
			} else {
				currentSpeakerSprite->position.x = (windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentAnimationPhase->pixelWidth / 2);
				currentSpeakerSprite->position.y = windowDimensions.y - currentAnimationPhase->pixelHeight;
			}
		}
	}

	for (int i = 0; i < nbChoices; i++)
	{
		int x = 0.015f * windowDimensions.x;
		int y;
		if (i == 0)
		{
			y = 0.8f * windowDimensions.y + 2;
		} else {
			y = currentChoices[i - 1]->position.y + currentChoices[i - 1]->height + 4;
		}
		set_text_position(currentChoices[i], (ivec2){x, y});
		set_text_width_limit(currentChoices[i], 0.985f * windowDimensions.x);
		if (currentChoice == i)
		{
			choiceMarker->position.y = y + ((currentChoices[currentChoice]->height - choiceMarker->height) / 2) - currentChoices[currentChoice]->font->descent + 2;
		}
	}

	ivec2 currentSpeakerPosition;
	if (characterNamePosition == 1)
	{
		currentSpeakerPosition.x = 0.015f * windowDimensions.x;
	} else {
		currentSpeakerPosition.x = windowDimensions.x * 0.985f - currentSpeaker->width;
	}
	currentSpeakerPosition.y = windowDimensions.y * 0.8f - currentSpeaker->height - 8;
	set_text_position(currentSpeaker, currentSpeakerPosition);
	characterNameBox->position.x = currentSpeaker->position.x - 2;
	characterNameBox->position.y = currentSpeaker->position.y - currentSpeaker->font->descent + 6;
	characterNameBox->width = currentSpeaker->width + 4;
	characterNameBox->height = currentSpeaker->height + currentSpeaker->font->descent + 4;
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
	buf_free(goToCommands);
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

static bool update_go_to(GoTo *goTo)
{
	if (goTo->dialogFile)
	{
		end = true;
		moving = true;
		strcopy(&nextDialogName, "Dialogs/");
		strappend(&nextDialogName, goTo->dialogFile);
		strcopy(&nextDialogStartKnotName, goTo->knotToGo);
	} else {
		if (strmatch(goTo->knotToGo, "end"))
		{
			end = true;
			moving = true;
		} else {
			bool foundGoToDestination = false;
			for (int i = 0; buf_len(interpretingDialog->knots); i++)
			{
				if (strmatch(interpretingDialog->knots[i]->name, goTo->knotToGo))
				{
					interpretingDialog->currentKnot = i;
					moving = true;
					foundGoToDestination = true;
					break;
				}
			}
			if (!foundGoToDestination)
			{
				error("could not find knot labeled %s in %s.", goTo->knotToGo, interpretingDialogName);
			}
		}
	}
	return true;
}

static bool update_assign(Assignment *assign)
{
	bool foundVariable = false;
	for (unsigned int i = 0; i < buf_len(variablesNames); i++)
	{
		if (strmatch(assign->identifier, variablesNames[i]))
		{
			free_variable(variablesValues[i]);
			variablesValues[i] = resolve_logic_expression(assign->logicExpression);
			foundVariable = true;
		}
	}
	if (!foundVariable)
	{
		buf_add(variablesNames, strclone(assign->identifier));
		buf_add(variablesValues, resolve_logic_expression(assign->logicExpression));
	}
	return true;
}

static bool update_command(Command *command)
{
	if (command->type == COMMAND_SET_BACKGROUND)
	{
		displayDialogUI = false;
		if (!appearingBackground)
		{
			const char *backgroundName = command->arguments[0]->string;
			const char *animationName = command->arguments[1]->string;
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
		int position = command->arguments[0]->numeric;
		Sprite *characterSprite = charactersSprites[position];
		Sprite *oldCharacterSprite = oldCharactersSprites[position];
		if (!appearingCharacter)
		{
			const char *characterName = command->arguments[1]->string;
			const char *animationName = command->arguments[2]->string;
			bool foundCharacter = false;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->charactersNames); i++)
			{
				if (strmatch(characterName, interpretingDialog->charactersNames[i]))
				{
					charactersNames[position] = interpretingDialog->charactersNames[i];
					if (characterSprite->animations)
					{
						oldCharacterSprite->animations = characterSprite->animations;
						oldCharacterSprite->position = characterSprite->position;
						oldCharacterSprite->currentAnimation = characterSprite->currentAnimation;
						oldCharacterSprite->animations[oldCharacterSprite->currentAnimation]->currentAnimationPhase = 0;
					}
					characterSprite->animations = interpretingDialog->charactersAnimations[i];
					bool foundAnimation = false;
					for (unsigned int j = 0; j < buf_len(characterSprite->animations); j++)
					{
						Animation *currentAnimation = characterSprite->animations[j];
						if (strmatch(animationName, currentAnimation->name))
						{
							if (currentAnimation->animationPhases[0]->responsive)
							{
								characterSprite->position.x = (windowDimensions.x * position / 6.0f) - (currentAnimation->animationPhases[0]->responsiveWidth * windowDimensions.x / 2);
								characterSprite->position.y = windowDimensions.y - (currentAnimation->animationPhases[0]->responsiveHeight * windowDimensions.y);
							} else {
								characterSprite->position.x = (windowDimensions.x * position / 6.0f) - (currentAnimation->animationPhases[0]->pixelWidth / 2);
								characterSprite->position.y = windowDimensions.y - currentAnimation->animationPhases[0]->pixelHeight;
							}
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
		int position = command->arguments[0]->numeric;
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
			const char *musicName = command->arguments[0]->string;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->musicsNames); i++)
			{
				if (strmatch(musicName, interpretingDialog->musicsNames[i]))
				{
					if (music)
					{
						oldMusic = music;
					}
					buf(char) newMusicName = strclone("Musics/");
					strappend(&newMusicName, musicName);
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
			const char *soundName = command->arguments[0]->string;
			for (unsigned int i = 0; i < buf_len(interpretingDialog->soundsNames); i++)
			{
				if (strmatch(soundName, interpretingDialog->soundsNames[i]))
				{
					if (sound)
					{
						oldSound = sound;
					}
					buf(char) newSoundName = strclone("Sounds/");
					strappend(&newSoundName, soundName);
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
	} else if (command->type == COMMAND_HIDE_UI) {
		displayDialogUI = false;
	} else if (command->type == COMMAND_WAIT) {
		if (waitTimer == 0.0f)
		{
			waitTimer = command->arguments[0]->numeric;
		}
		waitTimer -= deltaTime;
		if (waitTimer <= 0.0f)
		{
			waitTimer = 0.0f;
		} else {
			return false;
		}
	} else if (command->type == COMMAND_SET_WINDOW_NAME) {
		set_window_name(command->arguments[0]->string);
		return true;
	} else if (command->type == COMMAND_SET_SPEAKER_NAME_COLOR) {
		bool foundColoredName = false;
		const char *nameToColor = command->arguments[0]->string;
		vec3 newNameColor = {command->arguments[1]->numeric, command->arguments[2]->numeric, command->arguments[3]->numeric};
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
			buf_add(interpretingDialog->coloredNames, strclone(nameToColor));
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
		textScrollOffset = 0;
		sentenceFirstUpdate = false;
		timeDuringCurrentChar = 0.0f;
		timeDuringCurrentBlip = 0.0f;
		set_text_string(currentSentence, sentence->string);
		currentSentence->nbCharToDisplay = 0;
		if (currentSpeakerSpriteIndex != -1)
		{
			currentAnimation->animationState = ANIMATION_STATE_PLAY;
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
				currentAnimation->animationState = ANIMATION_STATE_STOPPING;
			}
		}
	} else if (is_input_key_pressed(INPUT_KEY_ENTER) || sentence->autoSkip) {
		sentenceFirstUpdate = true;
		set_text_string(currentSentence, NULL);
		if (currentSpeakerSpriteIndex != -1)
		{
			reset_animation(currentAnimation);
		}
		return true;
	}

	if (currentSpeakerSpriteIndex != -1)
	{
		if (currentAnimationPhase->responsive)
		{
			currentSpeakerSprite->position.x = (windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentAnimationPhase->responsiveWidth * windowDimensions.x / 2);
			currentSpeakerSprite->position.y = windowDimensions.y - (currentAnimationPhase->responsiveHeight * windowDimensions.y);
		} else {
			currentSpeakerSprite->position.x = (windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentAnimationPhase->pixelWidth / 2);
			currentSpeakerSprite->position.y = windowDimensions.y - currentAnimationPhase->pixelHeight;
		}
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
		textScrollOffset = 0;
		choosing = true;
	} else if (cueExpression->type == CUE_EXPRESSION_CUE_CONDITION) {
		return update_cue_condition(cueExpression->cueCondition);
	} else if (cueExpression->type == CUE_EXPRESSION_GO_TO) {
		return update_go_to(cueExpression->goTo);
	} else if (cueExpression->type == CUE_EXPRESSION_ASSIGNMENT) {
		return update_assign(cueExpression->assignment);
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
			set_text_width_limit(currentChoices[nbChoices], 0.985f * windowDimensions.x);
			currentChoices[nbChoices]->position.x = 0.015f * windowDimensions.x;
			if (nbChoices == 0)
			{
				currentChoices[nbChoices]->position.y = 0.8f * windowDimensions.y + 2;
			}
			currentChoices[nbChoices]->color = COLOR_WHITE;
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
				if (cue->setCharacterCommandInDeclaration)
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
				vec3 nameColor = {-1.0f};
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
				characterNamePosition = cue->characterNamePosition;
				if (characterNamePosition == 1)
				{
					currentSpeakerPosition.x = 0.015f * windowDimensions.x;
				} else {
					currentSpeakerPosition.x = windowDimensions.x * 0.985f - currentSpeaker->width;
				}
				currentSpeakerPosition.y = windowDimensions.y * 0.8f - currentSpeaker->height - 8;
				set_text_position(currentSpeaker, currentSpeakerPosition);
				characterNameBox->position.x = currentSpeaker->position.x - 2;
				characterNameBox->position.y = currentSpeaker->position.y - currentSpeaker->font->descent + 6;
				characterNameBox->width = currentSpeaker->width + 4;
				characterNameBox->height = currentSpeaker->height + currentSpeaker->font->descent + 4;
			} else {
				displaySpeakerName = false;
			}
		}

		if (cue->setCharacterCommandInDeclaration && appearingCharacter && cue->currentExpression == 0)
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
			currentSpeaker->color = COLOR_WHITE;
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
				currentSpeaker->color = COLOR_WHITE;
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
			update_go_to(goToCommands[currentChoice]);
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
			currentSpeaker->color = COLOR_WHITE;
			return true;
		}
		choiceMarker->position.y = currentChoices[currentChoice]->position.y + ((currentChoices[currentChoice]->height - choiceMarker->height) / 2) - currentChoices[currentChoice]->font->descent + 2;
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
	} else if (knotExpression->type == KNOT_EXPRESSION_GO_TO) {
		return update_go_to(knotExpression->goTo);
	} else if (knotExpression->type == KNOT_EXPRESSION_ASSIGNMENT) {
		return update_assign(knotExpression->assignment);
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
		buf_free(goToCommands);
		goToCommands = NULL;
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
		textScrollOffset = 0;

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
		if (!choosing && currentSentence->height > sentenceBox->height - 4 && mouseScrollOffset != 0)
		{
			textScrollOffset += mouseScrollOffset;
			if (textScrollOffset > 0)
			{
				textScrollOffset = 0;
			}
			if (textScrollOffset < (currentSentence->font->ascent - currentSentence->font->descent) - 4 - currentSentence->height)
			{
				textScrollOffset = (currentSentence->font->ascent - currentSentence->font->descent) - 4 - currentSentence->height;
			}
			set_text_position(currentSentence, (ivec2){0.015f * windowDimensions.x, 0.8f * windowDimensions.y - 4 + textScrollOffset});
		} else if (choosing && nbChoices != 0 && currentChoices[nbChoices - 1]->position.y + currentChoices[nbChoices - 1]->height > sentenceBox->height - 4 && mouseScrollOffset != 0) {
			textScrollOffset += mouseScrollOffset;
			if (textScrollOffset > 0)
			{
				textScrollOffset = 0;
			}
			int choicesHeight = 0;
			for (int i = 0; i < nbChoices; i++)
			{
				choicesHeight += currentChoices[i]->height;
			}
			if (textScrollOffset < currentChoices[nbChoices - 1]->height - 4 - choicesHeight)
			{
				textScrollOffset = currentChoices[nbChoices - 1]->height - 4 - choicesHeight;
			}
			int choiceYOffset = 0;
			for (int i = 0; i < nbChoices; i++)
			{
				set_text_position(currentChoices[i], (ivec2){0.015f * windowDimensions.x, 0.8f * windowDimensions.y + choiceYOffset - 4 + textScrollOffset});
				if (currentChoice == i)
				{
					choiceMarker->position.y = currentChoices[currentChoice]->position.y + ((currentChoices[currentChoice]->height - choiceMarker->height) / 2) - currentChoices[currentChoice]->font->descent + 2;
				}
				choiceYOffset += currentChoices[i]->height + 4;
			}
		}
		if (!choosing)
		{
			add_text_to_draw_list(currentSentence, DRAW_LAYER_UI_SCISSOR);
		} else {
			add_sprite_to_draw_list(choiceMarker, DRAW_LAYER_UI_SCISSOR);
			for (int i = 0; i < nbChoices; i++)
			{
				add_text_to_draw_list(currentChoices[i], DRAW_LAYER_UI_SCISSOR);
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
