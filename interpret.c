#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "interpret.h"
#include "graphics.h"
#include "globals.h"
#include "stretchy_buffer.h"

static Dialog *currentDialog = NULL;

static Sprite *charactersSprites[7];
static char *charactersNames[7];
static int currentSpeakerSpriteIndex = -1;
static Sprite *backgroundSprite;
static Sprite *choiceMarker = NULL;
static Sprite *characterNameBox = NULL;
static Sprite *sentenceBox = NULL;
static Text *currentSpeaker = NULL;
static Text *currentSentence = NULL;
static Text **currentChoices = NULL;
static Command **goToCommands = NULL;
static bool choosing = false;
static int nbChoices = 0;
static int currentChoice = 0;
static bool choicesDisplayed = false;
static bool moving = false;
static AnimatedSprite *currentSpeakerAnimatedSprite = NULL;
static bool displayedSpeakerName = false;

void init_dialog_ui()
{
	vec3 black = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
	vec3 white = {.x = 1.0f, .y = 1.0f, .z = 1.0f};

	currentSentence = create_text();
	set_font_to_text(currentSentence, "Fonts/arial.ttf", TEXT_SIZE_NORMAL);
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

	characterNameBox = create_sprite(SPRITE_COLOR);
	characterNameBox->color = black;

	choiceMarker = create_sprite(SPRITE_COLOR);
	choiceMarker->position.x = (int)(0.005f * windowDimensions.x);
	choiceMarker->width = (int)(0.005 * windowDimensions.x);
	choiceMarker->height = choiceMarker->width;
	choiceMarker->color = white;

	backgroundSprite = create_sprite(SPRITE_TEXTURE);
	backgroundSprite->position.x = 0;
	backgroundSprite->position.y = 0;
	backgroundSprite->width = windowDimensions.x;
	backgroundSprite->height = windowDimensions.y;

	for (int i = 0; i < 7; i++)
	{
		charactersSprites[i] = create_sprite(SPRITE_TEXTURE);
		charactersNames[i] = NULL;
	}
}

static void process_command(Command *command)
{
	if (command->type == COMMAND_SET_BACKGROUND)
	{
		AnimatedSprite *currentBackgroundPack = NULL;
		bool match = false;
		for (unsigned int i = 0; i < buf_len(currentDialog->backgroundPacksNames); i++)
		{
			if (strmatch(command->arguments[0].text, currentDialog->backgroundPacksNames[i]))
			{
				currentBackgroundPack = currentDialog->backgroundPacks[i];
				match = true;
				break;
			}
		}
		if (!match)
		{
			error("background pack %s does not exist.", command->arguments[0].text);
		}
		match = false;
		for (unsigned int i = 0; i < buf_len(currentBackgroundPack->animationsNames); i++)
		{
			if (strmatch(command->arguments[1].text, currentBackgroundPack->animationsNames[i]))
			{
				currentBackgroundPack->currentAnimation = i;
				backgroundSprite->textureId = currentBackgroundPack->animations[i]->animationPhases[0]->textureId;
				match = true;
				break;
			}
		}
		if (!match)
		{
			error("background %s of background pack %s does not exist.", command->arguments[1].text, command->arguments[0].text);
		}
	} else if (command->type == COMMAND_CLEAR_BACKGROUND) {
		backgroundSprite->textureId = -1;
	} else if (command->type == COMMAND_SET_CHARACTER) {
		AnimatedSprite *currentCharacterAnimatedSprite = NULL;
		bool match = false;
		for (unsigned int i = 0; i < buf_len(currentDialog->charactersNames); i++)
		{
			if (strmatch(command->arguments[1].text, currentDialog->charactersNames[i]))
			{
				currentCharacterAnimatedSprite = currentDialog->charactersAnimatedSprites[i];
				charactersNames[(int)command->arguments[0].numeric] = currentDialog->charactersNames[i];
				match = true;
				break;
			}
		}
		if (!match)
		{
			error("character %s does not exist.", command->arguments[1].text);
		}
		match = false;
		for (unsigned int i = 0; i < buf_len(currentCharacterAnimatedSprite->animationsNames); i++)
		{
			if (strmatch(command->arguments[2].text, currentCharacterAnimatedSprite->animationsNames[i]))
			{
				currentCharacterAnimatedSprite->currentAnimation = i;
				charactersSprites[(int)command->arguments[0].numeric]->textureId = currentCharacterAnimatedSprite->animations[i]->animationPhases[0]->textureId;
				charactersSprites[(int)command->arguments[0].numeric]->position.x = (int)((windowDimensions.x * (int)command->arguments[0].numeric / 6.0f) - (currentCharacterAnimatedSprite->animations[i]->animationPhases[0]->width / 2));
				charactersSprites[(int)command->arguments[0].numeric]->position.y = (int)(windowDimensions.y * 0.8f - currentCharacterAnimatedSprite->animations[i]->animationPhases[0]->height);
				charactersSprites[(int)command->arguments[0].numeric]->width = currentCharacterAnimatedSprite->animations[i]->animationPhases[0]->width;
				charactersSprites[(int)command->arguments[0].numeric]->height = currentCharacterAnimatedSprite->animations[i]->animationPhases[0]->height;
				match = true;
				break;
			}
		}
		if (!match)
		{
			error("animation %s of character %s does not exist.", command->arguments[2].text, command->arguments[1].text);
		}
	} else if (command->type == COMMAND_CLEAR_CHARACTER_POSITION) {
		charactersSprites[(int)command->arguments[0].numeric]->textureId = -1;
		charactersNames[(int)command->arguments[0].numeric] = NULL;
	} else if (command->type == COMMAND_CLEAR_CHARACTER_POSITIONS) {
		for (int i = 0; i < 7; i++)
		{
			charactersSprites[i]->textureId = -1;
			charactersNames[i] = NULL;
		}
	} else if (command->type == COMMAND_END) {
		currentDialog->end = true;
		moving = true;
		// TO DO : permit to get into another dialog or end game
	} else if (command->type == COMMAND_ASSIGN) {
		for (unsigned int i = 0; i < buf_len(variablesNames); i++)
		{
			if (strmatch(command->arguments[0].text, variablesNames[i]))
			{
				variablesValues[i] = resolve_logic_expression(command->arguments[1].logicExpression);
				return;
			}
		}
		char *variableName = NULL;
		for (unsigned int i = 0; i < strlen(command->arguments[0].text); i++)
		{
			buf_add(variableName, command->arguments[0].text[i]);
		}
		buf_add(variableName, '\0');
		buf_add(variablesNames, variableName);
		buf_add(variablesValues, resolve_logic_expression(command->arguments[1].logicExpression));
	} else if (command->type == COMMAND_GO_TO) {
		for (int i = 0; buf_len(currentDialog->knots); i++)
		{
			if (strmatch(currentDialog->knots[i].name, command->arguments[0].text))
			{
				currentDialog->currentKnot = i;
				moving = true;
				break;
			}
		}
	}
}

static bool update_sentence(Sentence *sentence)
{
	if (sentence->currentChar != strlen(sentence->string))
	{
		if (sentence->currentChar == 0)
		{
			set_string_to_text(currentSentence, sentence->string);
			currentSentence->nbCharToDisplay = 0;
		}

		if (is_input_key_pressed(INPUT_KEY_SPACE))
		{
			sentence->currentChar = strlen(sentence->string);
			currentSentence->nbCharToDisplay = strlen(sentence->string);
		} else {
			sentence->currentChar++;
			currentSentence->nbCharToDisplay++;
		}

		if (currentSpeakerAnimatedSprite)
		{
			update_animated_sprite(currentSpeakerAnimatedSprite);
			charactersSprites[currentSpeakerSpriteIndex]->position.x = (int)((windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->width / 2));
			charactersSprites[currentSpeakerSpriteIndex]->position.y = (int)((windowDimensions.y * 0.8f) - currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->height);
			charactersSprites[currentSpeakerSpriteIndex]->width = currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->width;
			charactersSprites[currentSpeakerSpriteIndex]->height = currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->height;
			charactersSprites[currentSpeakerSpriteIndex]->textureId = currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->textureId;
		}
	} else {
		if (currentSpeakerAnimatedSprite)
		{
			stop_animated_sprite(currentSpeakerAnimatedSprite);
			charactersSprites[currentSpeakerSpriteIndex]->position.x = (int)((windowDimensions.x * currentSpeakerSpriteIndex / 6.0f) - (currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->width / 2));
			charactersSprites[currentSpeakerSpriteIndex]->position.y = (int)((windowDimensions.y * 0.8f) - currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->height);
			charactersSprites[currentSpeakerSpriteIndex]->width = currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->width;
			charactersSprites[currentSpeakerSpriteIndex]->height = currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->height;
			charactersSprites[currentSpeakerSpriteIndex]->textureId = currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->animationPhases[currentSpeakerAnimatedSprite->animations[currentSpeakerAnimatedSprite->currentAnimation]->currentAnimationPhase]->textureId;
		}
		if (is_input_key_pressed(INPUT_KEY_ENTER))
		{
			sentence->currentChar = 0;
			return true;
		}
	}
	add_sprite_to_draw_list(sentenceBox, DRAW_LAYER_UI);
	add_text_to_draw_list(currentSentence, DRAW_LAYER_UI);
	return false;
}

static bool update_cue_expression(CueExpression *cueExpression);

static bool update_cue_condition(CueCondition *cueCondition)
{
	if (!cueCondition->resolved)
	{
		cueCondition->result = resolve_logic_expression(cueCondition->logicExpression);
		cueCondition->resolved = true;
	}
	if (cueCondition->result)
	{
		if (cueCondition->currentExpression == buf_len(cueCondition->cueExpressionsIf))
		{
			cueCondition->currentExpression = 0;
			return true;
		}
		if (update_cue_expression(&cueCondition->cueExpressionsIf[cueCondition->currentExpression]))
		{
			if (moving)
			{
				cueCondition->currentExpression = 0;
			} else {
				cueCondition->currentExpression++;
			}
		}
	} else {
		if (cueCondition->currentExpression == buf_len(cueCondition->cueExpressionsElse))
		{
			cueCondition->currentExpression = 0;
			return true;
		}
		if (update_cue_expression(&cueCondition->cueExpressionsElse[cueCondition->currentExpression]))
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
		if (update_sentence(&cueExpression->sentence))
		{
			return true;
		}
	} else if (cueExpression->type == CUE_EXPRESSION_CHOICE) {
		if (!choosing)
		{
			choosing = true;
		}
	} else if (cueExpression->type == CUE_EXPRESSION_CUE_CONDITION) {
		if (update_cue_condition(&cueExpression->cueCondition))
		{
			return true;
		}
	} else if (cueExpression->type == CUE_EXPRESSION_COMMAND) {
		process_command(&cueExpression->command);
		return true;
	}
	return false;
}

static void display_choice(CueExpression *cueExpression)
{
	if (cueExpression->type == CUE_EXPRESSION_CHOICE)
	{
		if (nbChoices == buf_len(currentChoices))
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
		set_string_to_text(currentChoices[nbChoices], cueExpression->choice.sentence.string);
		buf_add(goToCommands, &cueExpression->choice.goToCommand);
		nbChoices++;
	} else if (cueExpression->type == CUE_EXPRESSION_CUE_CONDITION) {
		if (!cueExpression->cueCondition.resolved)
		{
			cueExpression->cueCondition.result = resolve_logic_expression(cueExpression->cueCondition.logicExpression);
			cueExpression->cueCondition.resolved = true;
		}
		if (cueExpression->cueCondition.result)
		{
			for (unsigned int i = cueExpression->cueCondition.currentExpression; i < buf_len(cueExpression->cueCondition.cueExpressionsIf); i++)
			{
				display_choice(&cueExpression->cueCondition.cueExpressionsIf[cueExpression->cueCondition.currentExpression]);
			}
		} else {
			for (unsigned int i = cueExpression->cueCondition.currentExpression; i < buf_len(cueExpression->cueCondition.cueExpressionsElse); i++)
			{
				display_choice(&cueExpression->cueCondition.cueExpressionsElse[cueExpression->cueCondition.currentExpression]);
			}
		}
	} else {
		error("found anything else than a choice or a conditional choice in cue choices listing.");
	}
}

static bool update_cue(Cue *cue)
{
	if (!choosing)
	{
		if (!displayedSpeakerName)
		{
			displayedSpeakerName = true;
			if (cue->characterName)
			{
				if (cue->setCharacterInDeclaration)
				{
					process_command(&cue->cueExpressions[0].command);
				}
				bool hasToUpdate = false;
				for (int i = 0; i < 7; i++)
				{
					if (charactersNames[i])
					{
						if (strmatch(cue->characterName, charactersNames[i]))
						{
							currentSpeakerSpriteIndex = i;
							hasToUpdate = true;
							break;
						}
					}
				}
				if (hasToUpdate)
				{
					for (unsigned int i = 0; i < buf_len(currentDialog->charactersNames); i++)
					{
						if (strmatch(cue->characterName, currentDialog->charactersNames[i]))
						{
							currentSpeakerAnimatedSprite = currentDialog->charactersAnimatedSprites[i];
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
			}
		}

		if (cue->currentExpression == buf_len(cue->cueExpressions)) {
			cue->currentExpression = 0;
			displayedSpeakerName = false;
			currentSpeakerAnimatedSprite = NULL;
			currentSpeakerSpriteIndex = -1;
			return true;
		}

		if (update_cue_expression(&cue->cueExpressions[cue->currentExpression]))
		{
			if (moving)
			{
				cue->currentExpression = 0;
				displayedSpeakerName = false;
				currentSpeakerAnimatedSprite = NULL;
				currentSpeakerSpriteIndex = -1;
				return true;
			} else {
				if (!choosing)
				{
					cue->currentExpression++;
				}
			}
		}
	}
	if (cue->characterName)
	{
		add_sprite_to_draw_list(characterNameBox, DRAW_LAYER_UI);
		add_text_to_draw_list(currentSpeaker, DRAW_LAYER_UI);
	}
	if (choosing)
	{
		if (!choicesDisplayed)
		{
			nbChoices = 0;
			for (unsigned int i = cue->currentExpression; i < buf_len(cue->cueExpressions); i++)
			{
				display_choice(&cue->cueExpressions[i]);
			}
			choicesDisplayed = true;
			choiceMarker->position.y = currentChoices[0]->position.y + ((currentChoices[0]->height - choiceMarker->height) / 2) - currentChoices[0]->font->descent;
		}

		if (is_input_key_pressed(INPUT_KEY_DOWN_ARROW))
		{
			currentChoice++;
			if (currentChoice == buf_len(currentChoices))
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
			process_command(goToCommands[currentChoice]);
			buf_free(goToCommands);
			goToCommands = NULL;
			cue->currentExpression = 0;
			choosing = false;
			choicesDisplayed = false;
			currentChoice = 0;
			displayedSpeakerName = false;
			currentSpeakerAnimatedSprite = NULL;
			currentSpeakerSpriteIndex = -1;
			return true;
		}

		add_sprite_to_draw_list(sentenceBox, DRAW_LAYER_UI);
		add_sprite_to_draw_list(choiceMarker, DRAW_LAYER_UI);
		for (int i = 0; i < nbChoices; i++)
		{
			add_text_to_draw_list(currentChoices[i], DRAW_LAYER_UI);
		}
	}
	return false;
}

static bool update_knot_expression(KnotExpression *knotExpression);

static bool update_knot_condition(KnotCondition *knotCondition)
{
	if (!knotCondition->resolved)
	{
		knotCondition->result = resolve_logic_expression(knotCondition->logicExpression);
		knotCondition->resolved = true;
	}
	if (knotCondition->result)
	{
		if (knotCondition->currentExpression == buf_len(knotCondition->knotExpressionsIf))
		{
			knotCondition->currentExpression = 0;
			return true;
		}
		if (update_knot_expression(&knotCondition->knotExpressionsIf[knotCondition->currentExpression]))
		{
			if (moving)
			{
				knotCondition->currentExpression = 0;
			} else {
				knotCondition->currentExpression++;
			}
		}
	} else {
		if (knotCondition->currentExpression == buf_len(knotCondition->knotExpressionsElse))
		{
			knotCondition->currentExpression = 0;
			return true;
		}
		if (update_knot_expression(&knotCondition->knotExpressionsElse[knotCondition->currentExpression]))
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
		if (update_cue(&knotExpression->cue))
		{
			return true;
		}
	} else if (knotExpression->type == KNOT_EXPRESSION_KNOT_CONDITION) {
		if (update_knot_condition(&knotExpression->knotCondition))
		{
			return true;
		}
	} else if (knotExpression->type == KNOT_EXPRESSION_COMMAND) {
		process_command(&knotExpression->command);
		return true;
	}
	return false;
}

static bool update_knot(Knot *knot)
{
	if (knot->currentExpression == buf_len(knot->knotExpressions))
	{
		knot->currentExpression = 0;
		return true;
	} else {
		if (update_knot_expression(&knot->knotExpressions[knot->currentExpression]))
		{
			if (moving)
			{
				knot->currentExpression = 0;
			} else {
				knot->currentExpression++;
			}
		}
		return false;
	}
}

void interpret(Dialog *dialog)
{
	if (currentDialog != dialog)
	{
		currentDialog = dialog;
	}

	if (moving)
	{
		moving = false;
	}


	if (dialog->currentKnot == buf_len(dialog->knots))
	{
		gameEnd = true;
	} else {
		if (update_knot(&dialog->knots[dialog->currentKnot]))
		{
			dialog->currentKnot++;
		}
	}

	if (dialog->end)
	{
		if (!nextDialog)
		{
			gameEnd = true;
		} else {
			for (int i = 0; i < 7; i++)
			{
				charactersSprites[i]->textureId = -1;
			}
			backgroundSprite->textureId = -1;
			dialog->currentKnot = 0;
			dialog->end = false;
		}
	}

	if (backgroundSprite->textureId != -1)
	{
		add_sprite_to_draw_list(backgroundSprite, DRAW_LAYER_BACKGROUND);
	}
	for (int i = 0; i < 7; i++)
	{
		if (charactersSprites[i]->textureId != -1)
		{
			add_sprite_to_draw_list(charactersSprites[i], DRAW_LAYER_FOREGROUND);
		}
	}
}
