#include <stddef.h>
#include "stretchy_buffer.h"
#include "dialog.h"

int nbArguments[] =
{
    [COMMAND_SET_BACKGROUND] = 2,
    [COMMAND_CLEAR_BACKGROUND] = 0,

    [COMMAND_SET_CHARACTER] = 3,
    [COMMAND_CLEAR_CHARACTER_POSITION] = 1,
    [COMMAND_CLEAR_CHARACTER_POSITIONS] = 0,

    [COMMAND_PAUSE] = 0,
    [COMMAND_END] = 0,

    [COMMAND_ASSIGN] = 2,

    [COMMAND_GO_TO] = 1
};

void free_logic_expression(LogicExpression *logicExpression)
{
	if (logicExpression->type == LOGIC_EXPRESSION_LITERAL)
	{
		if (logicExpression->literal.type == LOGIC_EXPRESSION_LITERAL_STRING)
		{
			buf_free(logicExpression->literal.text);
		} else if (logicExpression->literal.type == LOGIC_EXPRESSION_LITERAL_IDENTIFIER) {
			buf_free(logicExpression->literal.text);
		}
	} else if (logicExpression->type == LOGIC_EXPRESSION_UNARY) {
		free_logic_expression(logicExpression->unary.expression);
	} else if (logicExpression->type == LOGIC_EXPRESSION_BINARY) {
		free_logic_expression(logicExpression->binary.left);
		free_logic_expression(logicExpression->binary.right);
	} else if (logicExpression->type == LOGIC_EXPRESSION_GROUPING) {
		free_logic_expression(logicExpression->grouping.expression);
	}
	free(logicExpression);
}

void free_argument(Argument argument)
{
	if (argument.type == PARAMETER_STRING)
	{
		buf_free(argument.text);
	} else if (argument.type == PARAMETER_IDENTIFIER) {
		buf_free(argument.text);
	} else if (argument.type == PARAMETER_LOGIC_EXPRESSION) {
		free_logic_expression(argument.logicExpression);
	}
}

void free_command(Command command)
{
	for (int index = 0; index < nbArguments[command.type]; index++)
	{
		free_argument(command.arguments[index]);
	}
	free(command.arguments);
}

void free_choice(Choice choice)
{
	buf_free(choice.sentence);
	buf_free(choice.knotToGo);
}

void free_cue_condition(CueCondition cueCondition)
{
	free_logic_expression(cueCondition.logicExpression);
	for (int index = 0; index < buf_len(cueCondition.cueExpressionsIf); index++)
	{
		free_cue_expression(cueCondition.cueExpressionsIf[index]);
	}
	buf_free(cueCondition.cueExpressionsIf);
	for (int index = 0; index < buf_len(cueCondition.cueExpressionsElse); index++)
	{
		free_cue_expression(cueCondition.cueExpressionsElse[index]);
	}
	buf_free(cueCondition.cueExpressionsElse);
}

void free_cue_expression(CueExpression cueExpression)
{
	if (cueExpression.type == CUE_EXPRESSION_CHOICE)
	{
		free_choice(cueExpression.choice);
	} else if (cueExpression.type == CUE_EXPRESSION_COMMAND) {
		free_command(cueExpression.command);
	} else if (cueExpression.type == CUE_EXPRESSION_SENTENCE) {
		buf_free(cueExpression.sentence);
	} else if (cueExpression.type == CUE_EXPRESSION_CUE_CONDITION) {
		free_cue_condition(cueExpression.cueCondition);
	}
}

void free_cue(Cue cue)
{
	buf_free(cue.characterName);
	for (int index = 0; index < buf_len(cue.cueExpressions); index++)
	{
		free_cue_expression(cue.cueExpressions[index]);
	}
	buf_free(cue.cueExpressions);
}

void free_knot_condition(KnotCondition knotCondition)
{
	free_logic_expression(knotCondition.logicExpression);
	for (int index = 0; index < buf_len(knotCondition.knotExpressionsIf); index++)
	{
		free_knot_expression(knotCondition.knotExpressionsIf[index]);
	}
	buf_free(knotCondition.knotExpressionsIf);
	for (int index = 0; index < buf_len(knotCondition.knotExpressionsElse); index++)
	{
		free_knot_expression(knotCondition.knotExpressionsElse[index]);
	}
	buf_free(knotCondition.knotExpressionsElse);
}

void free_knot_expression(KnotExpression knotExpression)
{
	if (knotExpression.type == KNOT_EXPRESSION_COMMAND)
	{
		free_command(knotExpression.command);
	} else if (knotExpression.type == KNOT_EXPRESSION_CUE) {
		free_cue(knotExpression.cue);
	} else if (knotExpression.type == KNOT_EXPRESSION_KNOT_CONDITION) {
		free_knot_condition(knotExpression.knotCondition);
	}
}

void free_knot(Knot knot)
{
	buf_free(knot.name);
	for (int index = 0; index < buf_len(knot.knotExpressions); index++)
	{
		free_knot_expression(knot.knotExpressions[index]);
	}
	buf_free(knot.knotExpressions);
}

void free_dialog(Dialog dialog)
{
	for (int index = 0; index < buf_len(dialog.backgroundPacksNames); index++)
    {
		buf_free(dialog.backgroundPacksNames[index]);
	}
	buf_free(dialog.backgroundPacksNames);
	for (int index = 0; index < buf_len(dialog.backgroundPacks); index++)
    {
		free_animated_sprite(dialog.backgroundPacks[index]);
	}
	buf_free(dialog.backgroundPacks);
	for (int index = 0; index < buf_len(dialog.charactersNames); index++)
    {
		buf_free(dialog.charactersNames[index]);
	}
	buf_free(dialog.charactersNames);
	for (int index = 0; index < buf_len(dialog.charactersSprites); index++)
    {
		free_animated_sprite(dialog.charactersSprites[index]);
	}
	buf_free(dialog.charactersSprites);
	for (int index = 0; index < buf_len(dialog.knots); index++)
    {
		free_knot(dialog.knots[index]);
	}
	buf_free(dialog.knots);
}
