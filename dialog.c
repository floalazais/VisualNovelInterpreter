#include <stddef.h>

#include <stdio.h>

#include "stretchy_buffer.h"
#include "dialog.h"
#include "globals.h"

int nbArguments[] =
{
    [COMMAND_SET_BACKGROUND] = 2,
    [COMMAND_CLEAR_BACKGROUND] = 0,

    [COMMAND_SET_CHARACTER] = 3,
    [COMMAND_CLEAR_CHARACTER_POSITION] = 1,
    [COMMAND_CLEAR_CHARACTER_POSITIONS] = 0,

    [COMMAND_END] = 0,

    [COMMAND_ASSIGN] = 2,

    [COMMAND_GO_TO] = 1
};

double resolve_variable(char *variableName)
{
	for (unsigned int i = 0; i < buf_len(variablesNames); i++)
	{
		if (strmatch(variablesNames[i], variableName))
		{
			return variablesValues[i];
		}
	}
	error("variable %s not found.", variableName);
}

bool resolve_logic_expression(LogicExpression *logicExpression)
{
	if (logicExpression->type == LOGIC_EXPRESSION_LITERAL)
	{
		if (logicExpression->literal.type == LOGIC_EXPRESSION_LITERAL_STRING)
		{
			return logicExpression->literal.text;
		} else if (logicExpression->literal.type == LOGIC_EXPRESSION_LITERAL_IDENTIFIER) {
			return resolve_variable(logicExpression->literal.text);
		} else if (logicExpression->literal.type == LOGIC_EXPRESSION_LITERAL_NUMERIC) {
			return logicExpression->literal.numeric;
		} else {
			error("unknown literal type %d.", logicExpression->literal.type);
		}
	} else if (logicExpression->type == LOGIC_EXPRESSION_UNARY) {
		if (logicExpression->unary.type == LOGIC_EXPRESSION_UNARY_NEGATION)
		{
			return !resolve_logic_expression(logicExpression->unary.expression);
		} else {
			error("unknown unary expression type %d.", logicExpression->unary.type);
		}
	} else if (logicExpression->type == LOGIC_EXPRESSION_BINARY) {
		if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_OR)
		{
			return resolve_logic_expression(logicExpression->binary.left) || resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_AND) {
			return resolve_logic_expression(logicExpression->binary.left) && resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_ADD) {
			return resolve_logic_expression(logicExpression->binary.left) + resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_DIVISE) {
			return resolve_logic_expression(logicExpression->binary.left) / resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_EQUALS) {
			return resolve_logic_expression(logicExpression->binary.left) == resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_DIFFERS) {
			return resolve_logic_expression(logicExpression->binary.left) != resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_INFERIOR) {
			return resolve_logic_expression(logicExpression->binary.left) < resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_MULTIPLY) {
			return resolve_logic_expression(logicExpression->binary.left) * resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_MULTIPLY) {
			return resolve_logic_expression(logicExpression->binary.left) * resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_SUBTRACT) {
			return resolve_logic_expression(logicExpression->binary.left) - resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_SUPERIOR) {
			return resolve_logic_expression(logicExpression->binary.left) > resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_SUPERIOR_EQUALS) {
			return resolve_logic_expression(logicExpression->binary.left) >= resolve_logic_expression(logicExpression->binary.right);
		} else if (logicExpression->binary.operation == LOGIC_EXPRESSION_BINARY_INFERIOR_EQUALS) {
			return resolve_logic_expression(logicExpression->binary.left) <= resolve_logic_expression(logicExpression->binary.right);
		} else {
			error("unknown operation %d.", logicExpression->binary.operation);
		}
	} else if (logicExpression->type == LOGIC_EXPRESSION_GROUPING) {
		return resolve_logic_expression(logicExpression->grouping.expression);
	} else {
		error("unknown logic expression type %d.", logicExpression->type);
	}
}

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
	for (int i = 0; i < nbArguments[command.type]; i++)
	{
		free_argument(command.arguments[i]);
	}
	free(command.arguments);
}

void free_sentence(Sentence sentence)
{
	buf_free(sentence.string);
}

void free_choice(Choice choice)
{
	free_sentence(choice.sentence);
	free_command(choice.goToCommand);
}

void free_cue_condition(CueCondition cueCondition)
{
	free_logic_expression(cueCondition.logicExpression);
	for (unsigned int index = 0; index < buf_len(cueCondition.cueExpressionsIf); index++)
	{
		free_cue_expression(cueCondition.cueExpressionsIf[index]);
	}
	buf_free(cueCondition.cueExpressionsIf);
	for (unsigned int index = 0; index < buf_len(cueCondition.cueExpressionsElse); index++)
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
		free_sentence(cueExpression.sentence);
	} else if (cueExpression.type == CUE_EXPRESSION_CUE_CONDITION) {
		free_cue_condition(cueExpression.cueCondition);
	}
}

void free_cue(Cue cue)
{
	buf_free(cue.characterName);
	for (unsigned int index = 0; index < buf_len(cue.cueExpressions); index++)
	{
		free_cue_expression(cue.cueExpressions[index]);
	}
	buf_free(cue.cueExpressions);
}

void free_knot_condition(KnotCondition knotCondition)
{
	free_logic_expression(knotCondition.logicExpression);
	for (unsigned int index = 0; index < buf_len(knotCondition.knotExpressionsIf); index++)
	{
		free_knot_expression(knotCondition.knotExpressionsIf[index]);
	}
	buf_free(knotCondition.knotExpressionsIf);
	for (unsigned int index = 0; index < buf_len(knotCondition.knotExpressionsElse); index++)
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
	for (unsigned int index = 0; index < buf_len(knot.knotExpressions); index++)
	{
		free_knot_expression(knot.knotExpressions[index]);
	}
	buf_free(knot.knotExpressions);
}

void free_dialog(Dialog dialog)
{
	for (unsigned int index = 0; index < buf_len(dialog.backgroundPacksNames); index++)
    {
		buf_free(dialog.backgroundPacksNames[index]);
	}
	buf_free(dialog.backgroundPacksNames);
	for (unsigned int index = 0; index < buf_len(dialog.backgroundPacks); index++)
    {
		free_animated_sprite(dialog.backgroundPacks[index]);
	}
	buf_free(dialog.backgroundPacks);
	for (unsigned int index = 0; index < buf_len(dialog.charactersNames); index++)
    {
		buf_free(dialog.charactersNames[index]);
	}
	buf_free(dialog.charactersNames);
	for (unsigned int index = 0; index < buf_len(dialog.charactersAnimatedSprites); index++)
    {
		free_animated_sprite(dialog.charactersAnimatedSprites[index]);
	}
	buf_free(dialog.charactersAnimatedSprites);
	fprintf(stderr, "wut\n");
	for (unsigned int index = 0; index < buf_len(dialog.knots); index++)
    {
		free_knot(dialog.knots[index]);
	}
	buf_free(dialog.knots);
}
