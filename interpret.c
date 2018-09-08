#include <stdlib.h>

#include <stdio.h>

#include "interpret.h"
#include "graphics.h"
#include "globals.h"
#include "stretchy_buffer.h"

static Dialog *currentDialog = NULL;

static Sprite *charactersSprites[7];
static Sprite *backgroundSprite;
static Sprite *choiceMarker = NULL;
static Sprite *characterNameBox = NULL;
static Sprite *sentenceBox = NULL;
static Text currentSpeaker = NULL;
static Text currentSentence = NULL;
static Text *currentChoices = NULL;
static Command **goToCommands = NULL;
static bool choosing = false;
static int currentChoice = 0;
static bool choicesDisplayed = false;
static bool moving = false;
static AnimatedSprite *currentSpeakerAnimatedSprite = NULL;
static bool displayedSpeakerName = false;

void init_dialog_ui()
{
	vec3 black = {.x = 0.0f, .y = 0.0f, .z = 0.0f};

	vec2 characterNameBoxPosition = {.x = 0.0f, .y = 0.725f};
	characterNameBox = create_color_sprite(characterNameBoxPosition, 0.3f, (64.0f / (double)windowDimensions.y), black);

	vec2 sentenceBoxPosition = {.x = 0.0f, .y = 0.8f};
	sentenceBox = create_color_sprite(sentenceBoxPosition, 1.0f, 0.2f, black);

	vec3 white = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
	vec2 choiceMarkerPosition = {.x = 0.005f, .y = 0.8f + (16.0f / (double)windowDimensions.y)};
	choiceMarker = create_color_sprite(choiceMarkerPosition, (8.0f / (double)windowDimensions.x), (8.0f / (double)windowDimensions.y), white);

	vec2 spritePosition = {.x = 0.0f, .y = 0.0f};
	backgroundSprite = create_texture_sprite(spritePosition, 1.0f, 1.0f, "no texture");
	for (int i = 0; i < 7; i++)
	{
		spritePosition.x = i * (1.0f / 7.0f);
		charactersSprites[i] = create_texture_sprite(spritePosition, (1.0f / 7.0f), 1.0f, "no texture");
	}
}

static void process_command(Command *command)
{
	if (command->type == COMMAND_SET_BACKGROUND)
	{
		AnimatedSprite *currentBackgroundPack;
		for (int i = 0; i < buf_len(currentDialog->backgroundPacksNames); i++)
		{
			if (strmatch(command->arguments[0].text, currentDialog->backgroundPacksNames[i]))
			{
				currentBackgroundPack = currentDialog->backgroundPacks[i];
				break;
			}
		}
		bool match = false;
		for (int i = 0; i < buf_len(currentBackgroundPack->animationsNames); i++)
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
		AnimatedSprite *currentCharacterAnimatedSprite;
		for (int i = 0; i < buf_len(currentDialog->charactersNames); i++)
		{
			if (strmatch(command->arguments[1].text, currentDialog->charactersNames[i]))
			{
				currentCharacterAnimatedSprite = currentDialog->charactersAnimatedSprites[i];
				break;
			}
		}
		bool match = false;
		for (int i = 0; i < buf_len(currentCharacterAnimatedSprite->animationsNames); i++)
		{
			if (strmatch(command->arguments[2].text, currentCharacterAnimatedSprite->animationsNames[i]))
			{
				currentCharacterAnimatedSprite->currentAnimation = i;
				charactersSprites[(int)command->arguments[0].numeric]->textureId = currentCharacterAnimatedSprite->animations[i]->animationPhases[0]->textureId;
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
	} else if (command->type == COMMAND_CLEAR_CHARACTER_POSITIONS) {
		for (int i = 0; i < 7; i++)
		{
			charactersSprites[i]->textureId = -1;
		}
	} else if (command->type == COMMAND_END) {
		currentDialog->end = true;
		moving = true;
		// TO DO : permit to get into another dialog or end game
	} else if (command->type == COMMAND_ASSIGN) {
		for (int i = 0; i < buf_len(variablesNames); i++)
		{
			if (strmatch(command->arguments[0].text, variablesNames[i]))
			{
				variablesValues[i] = resolve_logic_expression(command->arguments[1].logicExpression);
				return;
			}
		}
		char *variableName = NULL;
		for (int i = 0; i < buf_len(command->arguments[0].text); i++)
		{
			buf_add(variableName, command->arguments[0].text[i]);
		} // avoid free of pointer if dialog gets freed
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
	if (sentence->currentChar != buf_len(sentence->string))
	{
		if (is_input_key_pressed(INPUT_KEY_SPACE))
		{
			sentence->currentChar = buf_len(sentence->string);
		} else {
			sentence->currentChar++;
		}
		char *sentenceToDisplay = NULL;
		for (int i = 0; i < sentence->currentChar; i++)
		{
			buf_add(sentenceToDisplay, sentence->string[i]);
		}
		buf_add(sentenceToDisplay, '\0');
		free_text(currentSentence);
		vec2 position = {.x = 0.015f, 0.8f};
		vec3 color = {1.0f, 1.0f, 1.0f};
		currentSentence = create_text(position, TEXT_SIZE_NORMAL, sentenceToDisplay, "Fonts/arial.ttf", color);
		buf_free(sentenceToDisplay);
		if (currentSpeakerAnimatedSprite)
		{
			update_animated_sprite(currentSpeakerAnimatedSprite);
		}
	} else {
		if (currentSpeakerAnimatedSprite)
		{
			stop_animated_sprite(currentSpeakerAnimatedSprite);
		}
		if (is_input_key_pressed(INPUT_KEY_ENTER))
		{
			free_text(currentSentence);
			currentSentence = NULL;
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
		vec2 position = {.x = 0.015f, .y = 0.8f + (24.0f / (double)windowDimensions.y) * buf_len(currentChoices)};
		vec3 white = {.x = 1.0f, .y = 1.0f, .z = 1.0f};
		Text choice = create_text(position, TEXT_SIZE_NORMAL, cueExpression->choice.sentence.string, "Fonts/arial.ttf", white);
		buf_add(currentChoices, choice);
		buf_add(goToCommands, &cueExpression->choice.goToCommand);
	} else if (cueExpression->type == CUE_EXPRESSION_CUE_CONDITION) {
		if (!cueExpression->cueCondition.resolved)
		{
			cueExpression->cueCondition.result = resolve_logic_expression(cueExpression->cueCondition.logicExpression);
			cueExpression->cueCondition.resolved = true;
		}
		if (cueExpression->cueCondition.result)
		{
			for (int i = cueExpression->cueCondition.currentExpression; i < buf_len(cueExpression->cueCondition.cueExpressionsIf); i++)
			{
				display_choice(&cueExpression->cueCondition.cueExpressionsIf[cueExpression->cueCondition.currentExpression]);
			}
		} else {
			for (int i = cueExpression->cueCondition.currentExpression; i < buf_len(cueExpression->cueCondition.cueExpressionsElse); i++)
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
		if (!cue->characterName)
		{
			displayedSpeakerName = true;
		}

		if (!displayedSpeakerName)
		{
			displayedSpeakerName = true;
			for (int i = 0; i < buf_len(currentDialog->charactersNames); i++)
			{
				if (strmatch(cue->characterName, currentDialog->charactersNames[i]))
				{
					currentSpeakerAnimatedSprite = currentDialog->charactersAnimatedSprites[i];
					break;
				}
			}
			free_text(currentSpeaker);
			vec2 position = {.x = 0.0f, .y = 0.725f};
			if (cue->characterNamePosition == 1)
			{
				position.x = 0.025f;
			} else {
				position.x = 0.725f;
			}
			vec3 color = {1.0f, 1.0f, 1.0f};
			currentSpeaker = create_text(position, TEXT_SIZE_BIG, cue->characterName, "Fonts/arial.ttf", color);
			characterNameBox->position.x = position.x * windowDimensions.x;
			characterNameBox->position.y = position.y * windowDimensions.y;
		}

		if (cue->currentExpression == buf_len(cue->cueExpressions)) {
			cue->currentExpression = 0;
			currentSpeakerAnimatedSprite = NULL;
			free_text(currentSpeaker);
			currentSpeaker = NULL;
			displayedSpeakerName = false;
			return true;
		}

		if (update_cue_expression(&cue->cueExpressions[cue->currentExpression]))
		{
			if (moving)
			{
				cue->currentExpression = 0;
				currentSpeakerAnimatedSprite = NULL;
				free_text(currentSpeaker);
				currentSpeaker = NULL;
				displayedSpeakerName = false;
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
			for (int i = cue->currentExpression; i < buf_len(cue->cueExpressions); i++)
			{
				display_choice(&cue->cueExpressions[i]);
			}
			choicesDisplayed = true;
		}
		if (is_input_key_pressed(INPUT_KEY_DOWN_ARROW))
		{
			currentChoice++;
			choiceMarker->position.y += 24;
			if (currentChoice == buf_len(currentChoices))
			{
				currentChoice = 0;
				choiceMarker->position.y = 0.8f * windowDimensions.y + 16;
			}
		} else if (is_input_key_pressed(INPUT_KEY_UP_ARROW)) {
			currentChoice--;
			choiceMarker->position.y -= 24;
			if (currentChoice == -1)
			{
				currentChoice = buf_len(currentChoices) - 1;
				choiceMarker->position.y = 0.8f * windowDimensions.y + 16 + currentChoice * 24;
			}
		} else if (is_input_key_pressed(INPUT_KEY_ENTER))
		{
			process_command(goToCommands[currentChoice]);
			buf_free(goToCommands);
			goToCommands = NULL;
			cue->currentExpression = 0;
			choosing = false;
			choicesDisplayed = false;
			currentChoice = 0;
			for (int i = 0; i < buf_len(currentChoices); i++)
			{
				free_text(currentChoices[i]);
			}
			buf_free(currentChoices);
			currentChoices = NULL;
			currentSpeakerAnimatedSprite = NULL;
			free_text(currentSpeaker);
			currentSpeaker = NULL;
			displayedSpeakerName = false;
			return true;
		}
		add_sprite_to_draw_list(sentenceBox, DRAW_LAYER_UI);
		add_sprite_to_draw_list(choiceMarker, DRAW_LAYER_UI);
		for (int i = 0; i < buf_len(currentChoices); i++)
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
