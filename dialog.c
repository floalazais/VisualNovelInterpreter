#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "maths.h"
#include "token.h"
#include "stroperation.h"
#include "stretchy_buffer.h"
#include "error.h"
#include "xalloc.h"
#include "graphics.h"
#include "variable.h"
#include "dialog.h"
#include "globals.h"

static char *filePath;
static Token **tokens;
static Dialog *currentDialog;
static int currentToken;
static int currentIndentationLevel;
static bool firstKnot;

typedef enum CueMode
{
	CUE_MODE_SENTENCE,
	CUE_MODE_CHOICE
} CueMode;

static CueMode currentCueMode;

static void step_in_tokens()
{
    if (tokens[currentToken++]->type == TOKEN_END_OF_FILE)
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

static LogicExpression *create_logic_expression_literal_numeric(double value)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->returnType = VARIABLE_NUMERIC;
	logicExpression->literal = xmalloc(sizeof (*logicExpression->literal));
	logicExpression->literal->type = LOGIC_EXPRESSION_LITERAL_NUMERIC;
	logicExpression->literal->numeric = value;
	return logicExpression;
}

static LogicExpression *create_logic_expression_literal_string(char *string)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->returnType = VARIABLE_STRING;
	logicExpression->literal = xmalloc(sizeof (*logicExpression->literal));
	logicExpression->literal->type = LOGIC_EXPRESSION_LITERAL_STRING;
	logicExpression->literal->string = NULL;
	logicExpression->literal->string = strcopy(logicExpression->literal->string, string);
	return logicExpression;
}

static LogicExpression *create_logic_expression_literal_identifier(char *identifier)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->literal = xmalloc(sizeof (*logicExpression->literal));
	logicExpression->literal->type = LOGIC_EXPRESSION_LITERAL_IDENTIFIER;
	logicExpression->literal->string = NULL;
	logicExpression->literal->string = strcopy(logicExpression->literal->string, identifier);
	return logicExpression;
}

static LogicExpression *create_logic_expression_grouping(LogicExpression *groupedLogicExpression)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_GROUPING;
	logicExpression->grouping = xmalloc(sizeof (*logicExpression->grouping));
	logicExpression->grouping->expression = groupedLogicExpression;
	return logicExpression;
}

static LogicExpression *create_logic_expression_unary(LogicExpressionUnaryType operation, LogicExpression *logicExpression)
{
	LogicExpression *negativeLogicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_UNARY;
	logicExpression->unary = xmalloc(sizeof (*logicExpression->unary));
	logicExpression->unary->type = operation;
	logicExpression->unary->expression = logicExpression;
	return negativeLogicExpression;
}

static LogicExpression *create_logic_expression_binary(LogicExpression *left, LogicExpressionBinaryOperation operation, LogicExpression *right)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_BINARY;
	logicExpression->binary = xmalloc(sizeof (*logicExpression->binary));
	logicExpression->binary->left = left;
	logicExpression->binary->operation = operation;
	logicExpression->binary->right = right;
	return logicExpression;
}

static LogicExpression *parse_logic_expression(int line);

static LogicExpression *parse_logic_expression_base(int line)
{
	LogicExpression *logicExpression;
	if (token_match_on_line(line, 1, TOKEN_NUMERIC)) {
		logicExpression = create_logic_expression_literal_numeric(tokens[currentToken]->numeric);
	} else if (token_match_on_line(line, 1, TOKEN_STRING)) {
		logicExpression = create_logic_expression_literal_string(tokens[currentToken]->string);
	} else if (token_match_on_line(line, 1, TOKEN_IDENTIFIER)) {
		logicExpression = create_logic_expression_literal_identifier(tokens[currentToken]->string);
	} else if (token_match_on_line(line, 1, TOKEN_OPEN_PARENTHESIS)) {
		step_in_tokens();
		LogicExpression *groupedLogicExpression = parse_logic_expression(line);
		if (token_match_on_line(line, 1, TOKEN_CLOSE_PARENTHESIS))
		{
			logicExpression = create_logic_expression_grouping(groupedLogicExpression);
		} else {
			error("in %s at line %d, expected close parenthesis token before %s token.", filePath, line, tokenStrings[tokens[currentToken]->type]);
		}
	} else {
		error("in %s at line %d, unexpected token %s in logic expression.", filePath, line, tokenStrings[tokens[currentToken]->type]);
	}
	step_in_tokens();
	return logicExpression;
}

static LogicExpression *parse_logic_expression_unary(int line)
{
	if (token_match_on_line(line, 1, TOKEN_MINUS)) {
		step_in_tokens();
		LogicExpression *logicExpression = parse_logic_expression_base(line);
		return create_logic_expression_unary(LOGIC_EXPRESSION_UNARY_NEGATION, logicExpression);
	} else {
		return parse_logic_expression_base(line);
	}
}

static LogicExpression *parse_logic_expression_multiplication(int line)
{
	LogicExpression *left = parse_logic_expression_unary(line);
	while (token_match_on_line(line, 1, TOKEN_STAR) || token_match_on_line(line, 1, TOKEN_SLASH))
	{
		LogicExpressionBinaryOperation operation;
		if (tokens[currentToken]->type == TOKEN_STAR)
		{
			operation = LOGIC_EXPRESSION_BINARY_MULTIPLY;
		} else if (tokens[currentToken]->type == TOKEN_SLASH) {
			operation = LOGIC_EXPRESSION_BINARY_DIVISE;
		} else {
			error("in %s at line %d, expected a multiplication or division operator token, got a %s token instead.", filePath, line, tokenStrings[tokens[currentToken]->type]);
		}
		step_in_tokens();
		LogicExpression *right = parse_logic_expression_unary(line);
		left = create_logic_expression_binary(left, operation, right);
	}
	return left;
}

static LogicExpression *parse_logic_expression_addition(int line)
{
	LogicExpression *left = parse_logic_expression_multiplication(line);
	while (token_match_on_line(line, 1, TOKEN_PLUS) || token_match_on_line(line, 1, TOKEN_MINUS))
	{
		LogicExpressionBinaryOperation operation;
		if (tokens[currentToken]->type == TOKEN_PLUS)
		{
			operation = LOGIC_EXPRESSION_BINARY_ADD;
		} else if (tokens[currentToken]->type == TOKEN_MINUS) {
			operation = LOGIC_EXPRESSION_BINARY_SUBTRACT;
		} else {
			error("in %s at line %d, expected an addition or subtraction operator token, got a %s token instead.", filePath, line, tokenStrings[tokens[currentToken]->type]);
		}
		step_in_tokens();
		LogicExpression *right = parse_logic_expression_multiplication(line);
		left = create_logic_expression_binary(left, operation, right);
	}
	return left;
}

static LogicExpression *parse_logic_expression_comparison(int line)
{
	LogicExpression *left = parse_logic_expression_addition(line);
	while (token_match_on_line(line, 1, TOKEN_EQUALS) || token_match_on_line(line, 1, TOKEN_DIFFERS) || token_match_on_line(line, 1, TOKEN_INFERIOR_EQUALS) || token_match_on_line(line, 1, TOKEN_INFERIOR) || token_match_on_line(line, 1, TOKEN_SUPERIOR_EQUALS) || token_match_on_line(line, 1, TOKEN_SUPERIOR))
	{
		LogicExpressionBinaryOperation operation;
		if (tokens[currentToken]->type == TOKEN_EQUALS)
		{
			operation = LOGIC_EXPRESSION_BINARY_EQUALS;
		} else if (tokens[currentToken]->type == TOKEN_DIFFERS) {
			operation = LOGIC_EXPRESSION_BINARY_DIFFERS;
		} else if (tokens[currentToken]->type == TOKEN_INFERIOR_EQUALS) {
			operation = LOGIC_EXPRESSION_BINARY_INFERIOR_EQUALS;
		} else if (tokens[currentToken]->type == TOKEN_INFERIOR) {
			operation = LOGIC_EXPRESSION_BINARY_INFERIOR;
		} else if (tokens[currentToken]->type == TOKEN_SUPERIOR_EQUALS) {
			operation = LOGIC_EXPRESSION_BINARY_SUPERIOR_EQUALS;
		} else if (tokens[currentToken]->type == TOKEN_SUPERIOR) {
			operation = LOGIC_EXPRESSION_BINARY_SUPERIOR;
		} else {
			error("in %s at line %d, expected a comparison operator token, got a %s token instead.", filePath, line, tokenStrings[tokens[currentToken]->type]);
		}
		step_in_tokens();
		LogicExpression *right = parse_logic_expression_addition(line);
		left = create_logic_expression_binary(left, operation, right);
	}
	return left;
}

static LogicExpression *parse_logic_expression_and(int line)
{
	LogicExpression *left = parse_logic_expression_comparison(line);
	while (token_match_on_line(line, 1, TOKEN_AND))
	{
		step_in_tokens();
		LogicExpression *right = parse_logic_expression_comparison(line);
		left = create_logic_expression_binary(left, LOGIC_EXPRESSION_BINARY_AND, right);
	}
	return left;
}

static LogicExpression *parse_logic_expression_or(int line)
{
	LogicExpression *left = parse_logic_expression_and(line);
	while (token_match_on_line(line, 1, TOKEN_OR))
	{
		step_in_tokens();
		LogicExpression *right = parse_logic_expression_and(line);
		left = create_logic_expression_binary(left, LOGIC_EXPRESSION_BINARY_OR, right);
	}
	return left;
}

static LogicExpression *parse_logic_expression(int line)
{
	return parse_logic_expression_or(line);
}

Variable *get_variable(char *variableName)
{
	Variable *variable = xmalloc(sizeof (*variable));
	for (unsigned int i = 0; i < buf_len(variablesNames); i++)
	{
		if (strmatch(variablesNames[i], variableName))
		{
			variable->type = variablesValues[i]->type;
			if (variable->type == VARIABLE_NUMERIC)
			{
				variable->numeric = variablesValues[i]->numeric;
			} else if (variable->type == VARIABLE_STRING) {
				variable->string = NULL;
				variable->string = strcopy(variable->string, variablesValues[i]->string);
			} else {
				error("unknown variable type %d.", variablesValues[i]->type);
			}
			return variable;
		}
	}
	error("variable %s not found.", variableName);
}

static Variable *convert_variable_content_to_bool(Variable *variable)
{
	if (variable->type == VARIABLE_NUMERIC)
	{
		variable->numeric = variable->numeric;
	} else if (variable->type == VARIABLE_STRING) {
		if (strlen(variable->string) == 1)
		{
			if (variable->string[0] == '\0')
			{
				variable->numeric = false;
			} else {
				variable->numeric = true;
			}
		} else {
			variable->numeric = true;
		}
		buf_free(variable->string);
		variable->string = NULL;
		variable->type = VARIABLE_NUMERIC;
	} else {
		error("unknown variable type %d.", variable->type);
	}
	return variable;
}

Variable *resolve_logic_expression(LogicExpression *logicExpression)
{
	if (logicExpression->type == LOGIC_EXPRESSION_LITERAL)
	{
		if (logicExpression->literal->type == LOGIC_EXPRESSION_LITERAL_STRING)
		{
			Variable *variable = xmalloc(sizeof (*variable));
			variable->type = VARIABLE_STRING;
			variable->string = NULL;
			variable->string = strcopy(variable->string, logicExpression->literal->string);
			return variable;
		} else if (logicExpression->literal->type == LOGIC_EXPRESSION_LITERAL_IDENTIFIER) {
			return get_variable(logicExpression->literal->string);
		} else if (logicExpression->literal->type == LOGIC_EXPRESSION_LITERAL_NUMERIC) {
			Variable *variable = xmalloc(sizeof (*variable));
			variable->type = VARIABLE_NUMERIC;
			variable->numeric = logicExpression->literal->numeric;
			return variable;
		} else {
			error("unknown literal type %d.", logicExpression->literal->type);
		}
	} else if (logicExpression->type == LOGIC_EXPRESSION_UNARY) {
		if (logicExpression->unary->type == LOGIC_EXPRESSION_UNARY_NEGATION)
		{
			Variable *variable = convert_variable_content_to_bool(resolve_logic_expression(logicExpression));
			variable->numeric = !variable->numeric;
			return variable;
		} else {
			error("unknown unary expression type %d.", logicExpression->unary->type);
		}
	} else if (logicExpression->type == LOGIC_EXPRESSION_BINARY) {
		Variable *left;
		Variable *right;
		if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_OR)
		{
			left = convert_variable_content_to_bool(resolve_logic_expression(logicExpression->binary->left));
			right = convert_variable_content_to_bool(resolve_logic_expression(logicExpression->binary->right));
			left->numeric = left->numeric || right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_AND) {
			left = convert_variable_content_to_bool(resolve_logic_expression(logicExpression->binary->left));
			right = convert_variable_content_to_bool(resolve_logic_expression(logicExpression->binary->right));
			left->numeric = left->numeric && right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_ADD) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_NUMERIC)
			{
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_STRING)
				{
					error("cannot compute \"%f + %s\".", left->numeric, right->string);
				} else {
					error("unknown variable type %d.", right->type);
				}
				left->numeric += right->numeric;
				free_variable(right);
			} else if (left->type == VARIABLE_STRING) {
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_NUMERIC)
				{
					error("cannot compute \"%s + %f\".", left->string, right->numeric);
				} else {
					error("unknown variable type %d.", right->type);
				}
				left->string = strappend(left->string, right->string);
				free_variable(right);
			} else {
				error("unknown variable type %d.", left->type);
			}
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_DIVISE) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a division on a string : \"%s\".", left->string);
			} else {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a division on a string : \"%s\".", right->string);
			} else {
				error("unknown variable type %d.", right->type);
			}
			left->numeric /= right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_EQUALS) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_NUMERIC)
			{
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_STRING)
				{
					error("cannot compute \"%f == %s\".", left->numeric, right->string);
				} else {
					error("unknown variable type %d.", right->type);
				}
				left->numeric = left->numeric == right->numeric;
				free_variable(right);
			} else if (left->type == VARIABLE_STRING) {
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_NUMERIC)
				{
					error("cannot compute \"%s == %f\".", left->string, right->numeric);
				} else {
					error("unknown variable type %d.", right->type);
				}
				left->numeric = strmatch(left->string, right->string);
				buf_free(left->string);
				left->string = NULL;
				left->type = VARIABLE_NUMERIC;
				free_variable(right);
			} else {
				error("unknown variable type %d.", left->type);
			}
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_DIFFERS) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_NUMERIC)
			{
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_STRING)
				{
					error("cannot compute \"%f != %s\".", left->numeric, right->string);
				} else {
					error("unknown variable type %d.", right->type);
				}
				left->numeric = left->numeric != right->numeric;
				free_variable(right);
			} else if (left->type == VARIABLE_STRING) {
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_NUMERIC)
				{
					error("cannot compute \"%s == %f\".", left->string, right->numeric);
				} else {
					error("unknown variable type %d.", right->type);
				}
				left->numeric = !strmatch(left->string, right->string);
				buf_free(left->string);
				left->type = VARIABLE_NUMERIC;
				free_variable(right);
			} else {
				error("unknown variable type %d.", left->type);
			}
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_INFERIOR) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute an inferior comparison on a string : \"%s\".", left->string);
			} else {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute an inferior comparison on a string : \"%s\".", right->string);
			} else {
				error("unknown variable type %d.", right->type);
			}
			left->numeric = left->numeric < right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_MULTIPLY) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a multiplication on a string : \"%s\".", left->string);
			} else {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a multiplication on a string : \"%s\".", right->string);
			} else {
				error("unknown variable type %d.", right->type);
			}
			left->numeric *= right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_SUBTRACT) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a subtraction on a string : \"%s\".", left->string);
			} else {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a subtraction on a string : \"%s\".", right->string);
			} else {
				error("unknown variable type %d.", right->type);
			}
			left->numeric -= right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_SUPERIOR) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a superior comparison on a string : \"%s\".", left->string);
			} else {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a superior comparison on a string : \"%s\".", right->string);
			} else {
				error("unknown variable type %d.", right->type);
			}
			left->numeric = left->numeric > right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_SUPERIOR_EQUALS) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a superior or equals comparison on a string : \"%s\".", left->string);
			} else {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a superior or equals comparison on a string : \"%s\".", right->string);
			} else {
				error("unknown variable type %d.", right->type);
			}
			left->numeric = left->numeric >= right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_INFERIOR_EQUALS) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute an inferior or equals comparison on a string : \"%s\".", left->string);
			} else {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute an inferior or equals comparison on a string : \"%s\".", right->string);
			} else {
				error("unknown variable type %d.", right->type);
			}
			left->numeric = left->numeric <= right->numeric;
			free_variable(right);
		} else {
			error("unknown operation %d.", logicExpression->binary->operation);
		}
		return left;
	} else if (logicExpression->type == LOGIC_EXPRESSION_GROUPING) {
		return resolve_logic_expression(logicExpression->grouping->expression);
	} else {
		error("unknown logic expression type %d.", logicExpression->type);
	}
}

static int position_identifier_to_int(char *identifier, int line)
{
	if (strmatch(identifier, "FULL_LEFT"))
	{
		return 0;
	} else if (strmatch(identifier, "LEFT")) {
		return 1;
	} else if (strmatch(identifier, "CENTER_LEFT")) {
		return 2;
	} else if (strmatch(identifier, "CENTER")) {
		return 3;
	} else if (strmatch(identifier, "CENTER_RIGHT")) {
		return 4;
	} else if (strmatch(identifier, "RIGHT")) {
		return 5;
	} else if (strmatch(identifier, "FULL_RIGHT")) {
		return 6;
	} else {
		error("in %s at line %d, \"%s\" identifier is not a position identifier.", filePath, line, identifier);
	}
}

static void add_to_background_list(char *backgroundPackName)
{
	bool foundPack = false;
	for (unsigned int index = 0; index < buf_len(currentDialog->backgroundPacksNames); index++)
	{
		if (strmatch(backgroundPackName, currentDialog->backgroundPacksNames[index]))
		{
			foundPack = true;
			break;
		}
	}
	if (!foundPack)
	{
		char *newBackgroundPackName = NULL;
		newBackgroundPackName = strcopy(newBackgroundPackName, backgroundPackName);
		buf_add(currentDialog->backgroundPacksNames, newBackgroundPackName);
		char *animationFilePath = NULL;
		animationFilePath = strcopy(animationFilePath, "Animation files/");
		animationFilePath = strappend(animationFilePath, backgroundPackName);
		animationFilePath = strappend(animationFilePath, ".anm");
		buf_add(currentDialog->backgroundPacks, get_animations_from_file(animationFilePath, backgroundPackName));
		buf_free(animationFilePath);
	}
}

static void add_to_character_list(char *characterName)
{
	bool foundCharacter = false;
	for (unsigned int index = 0; index < buf_len(currentDialog->charactersNames); index++)
	{
		if (strmatch(characterName, currentDialog->charactersNames[index]))
		{
			foundCharacter = true;
			break;
		}
	}
	if (!foundCharacter)
	{
		char *newCharacterName = NULL;
		newCharacterName = strcopy(newCharacterName, characterName);
		buf_add(currentDialog->charactersNames, newCharacterName);
		char *animationFilePath = NULL;
		animationFilePath = strcopy(animationFilePath, "Animation files/");
		animationFilePath = strappend(animationFilePath, characterName);
		animationFilePath = strappend(animationFilePath, ".anm");
		buf_add(currentDialog->charactersAnimations, get_animations_from_file(animationFilePath, characterName));
		buf_free(animationFilePath);
	}
}

static int nbArguments[] =
{
    [COMMAND_SET_BACKGROUND] = 2,
    [COMMAND_CLEAR_BACKGROUND] = 0,

    [COMMAND_SET_CHARACTER] = 3,
    [COMMAND_CLEAR_CHARACTER_POSITION] = 1,
    [COMMAND_CLEAR_CHARACTER_POSITIONS] = 0,

    [COMMAND_END] = 2,

    [COMMAND_ASSIGN] = 2,

    [COMMAND_GO_TO] = 1,

	[COMMAND_HIDE_UI] = 0
};

static Command *parse_command()
{
	Command *command = xmalloc(sizeof (*command));

	if (strmatch(tokens[currentToken]->string, "SET_BACKGROUND")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1]->line, 3, TOKEN_STRING, TOKEN_SCOPE, TOKEN_STRING))
		{
			command->type = COMMAND_SET_BACKGROUND;
			command->arguments = xmalloc(sizeof (*command->arguments) * nbArguments[command->type]);
			for (int i = 0; i < nbArguments[COMMAND_SET_BACKGROUND]; i++)
			{
				command->arguments[i] = xmalloc(sizeof (*command->arguments[i]));
			}
			command->arguments[0]->type = PARAMETER_STRING;
			command->arguments[0]->string = NULL;
			command->arguments[0]->string = strcopy(command->arguments[0]->string, tokens[currentToken]->string);
			command->arguments[1]->type = PARAMETER_STRING;
			command->arguments[1]->string = NULL;
			command->arguments[1]->string = strcopy(command->arguments[1]->string, tokens[currentToken + 2]->string);
			steps_in_tokens(3);
			add_to_background_list(command->arguments[0]->string);
		} else {
			error("in %s at line %d, bad argument for #%s command , usage is #%s background-pack-identifier::background-identifier.", filePath, tokens[currentToken - 1]->line, tokens[currentToken - 1]->string, tokens[currentToken - 1]->string);
		}
	} else if (strmatch(tokens[currentToken]->string, "CLEAR_BACKGROUND")) {
		step_in_tokens();
		command->type = COMMAND_CLEAR_BACKGROUND;
		command->arguments = NULL;
	} else if (strmatch(tokens[currentToken]->string, "SET_CHARACTER")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1]->line, 4, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_SCOPE, TOKEN_STRING))
		{
			command->type = COMMAND_SET_CHARACTER;
			command->arguments = xmalloc(sizeof (*command->arguments) * nbArguments[command->type]);
			for (int i = 0; i < nbArguments[COMMAND_SET_CHARACTER]; i++)
			{
				command->arguments[i] = xmalloc(sizeof (*command->arguments[i]));
			}
			command->arguments[0]->type = PARAMETER_NUMERIC;
			command->arguments[0]->numeric = position_identifier_to_int(tokens[currentToken]->string, tokens[currentToken - 1]->line);
			command->arguments[1]->type = PARAMETER_STRING;
			command->arguments[1]->string = NULL;
			command->arguments[1]->string = strcopy(command->arguments[1]->string, tokens[currentToken + 1]->string);
			command->arguments[2]->type = PARAMETER_STRING;
			command->arguments[2]->string = NULL;
			command->arguments[2]->string = strcopy(command->arguments[2]->string, tokens[currentToken + 3]->string);
			steps_in_tokens(4);
			add_to_character_list(command->arguments[1]->string);
		} else {
			error("in %s at line %d, bad arguments for #%s command, usage is #%s position-identifier character-identifier::animation-identifier.", filePath, tokens[currentToken - 1]->line, tokens[currentToken - 1]->string, tokens[currentToken - 1]->string);
		}
	} else if (strmatch(tokens[currentToken]->string, "CLEAR_CHARACTER_POSITION")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1]->line, 1, TOKEN_IDENTIFIER))
		{
			command->type = COMMAND_CLEAR_CHARACTER_POSITION;
			command->arguments = xmalloc(sizeof (*command->arguments) * nbArguments[command->type]);
			for (int i = 0; i < nbArguments[COMMAND_CLEAR_CHARACTER_POSITION]; i++)
			{
				command->arguments[i] = xmalloc(sizeof (*command->arguments[i]));
			}
			command->arguments[0]->type = PARAMETER_NUMERIC;
			command->arguments[0]->numeric = position_identifier_to_int(tokens[currentToken]->string, tokens[currentToken - 1]->line);
			step_in_tokens();
		} else {
			error("in %s at line %d, bad argument for #%s command, expected position identifier token.", filePath, tokens[currentToken - 1]->line, tokens[currentToken - 1]->string);
		}
	} else if (strmatch(tokens[currentToken]->string, "CLEAR_CHARACTER_POSITIONS")) {
		step_in_tokens();
		command->type = COMMAND_CLEAR_CHARACTER_POSITIONS;
		command->arguments = NULL;
	} else if (strmatch(tokens[currentToken]->string, "END")) {
		step_in_tokens();
		command->type = COMMAND_END;
		command->arguments = xmalloc(sizeof (*command->arguments) * nbArguments[command->type]);
		for (int i = 0; i < nbArguments[COMMAND_END]; i++)
		{
			command->arguments[i] = xmalloc(sizeof (*command->arguments[i]));
		}
		command->arguments[0]->type = PARAMETER_STRING;
		command->arguments[0]->string = NULL;
		command->arguments[1]->type = PARAMETER_STRING;
		command->arguments[1]->string = NULL;
		if (token_match_on_line(tokens[currentToken - 1]->line, 1, TOKEN_STRING))
		{
			command->arguments[0]->string = strcopy(command->arguments[0]->string, tokens[currentToken]->string);
			step_in_tokens();
			if (token_match_on_line(tokens[currentToken - 1]->line, 1, TOKEN_KNOT))
			{
				command->arguments[1]->string = strcopy(command->arguments[1]->string, tokens[currentToken]->string);
				step_in_tokens();
			}
		}
	} else if (strmatch(tokens[currentToken]->string, "ASSIGN")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1]->line, 1, TOKEN_IDENTIFIER))
		{
			command->type = COMMAND_ASSIGN;
			command->arguments = xmalloc(sizeof (*command->arguments) * nbArguments[command->type]);
			for (int i = 0; i < nbArguments[COMMAND_ASSIGN]; i++)
			{
				command->arguments[i] = xmalloc(sizeof (*command->arguments[i]));
			}
			command->arguments[0]->type = PARAMETER_IDENTIFIER;
			command->arguments[0]->string = NULL;
			command->arguments[0]->string = strcopy(command->arguments[0]->string, tokens[currentToken]->string);
			step_in_tokens();
			command->arguments[1]->type = PARAMETER_LOGIC_EXPRESSION;
			command->arguments[1]->logicExpression = parse_logic_expression(tokens[currentToken - 1]->line);
		} else {
			error("in %s at line %d, bad arguments for #%s command, expected variable identifier token first.", filePath, tokens[currentToken - 1]->line, tokens[currentToken - 1]->string);
		}
	} else if (strmatch(tokens[currentToken]->string, "GO_TO")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1]->line, 1, TOKEN_SENTENCE))
		{
			command->type = COMMAND_GO_TO;
			command->arguments = xmalloc(sizeof (*command->arguments) * nbArguments[command->type]);
			for (int i = 0; i < nbArguments[COMMAND_GO_TO]; i++)
			{
				command->arguments[i] = xmalloc(sizeof (*command->arguments[i]));
			}
			command->arguments[0]->type = PARAMETER_STRING;
			command->arguments[0]->string = NULL;
			command->arguments[0]->string = strcopy(command->arguments[0]->string, tokens[currentToken]->string);
			step_in_tokens();
		} else {
			error("in %s at line %d, bad argument for #%s command, expected knot name token.", filePath, tokens[currentToken - 1]->line, tokens[currentToken - 1]->string);
		}
	} else if (strmatch(tokens[currentToken]->string, "HIDE_UI")) {
		step_in_tokens();
		command->type = COMMAND_HIDE_UI;
		command->arguments = NULL;
	} else {
		error("in %s at line %d, unknown command #%s found.", filePath, tokens[currentToken]->line, tokens[currentToken]->string);
	}
	return command;
}

static Choice *parse_choice()
{
	Choice *choice = xmalloc(sizeof (*choice));

	if (tokens[currentToken + 1]->type != TOKEN_SENTENCE)
	{
		error("in %s at line %d, expected sentence after \"-\" choice indicator, got a %s token instead.", filePath, tokens[currentToken - 1]->line, tokenStrings[tokens[currentToken]->type]);
	}
	if (tokens[currentToken]->line != tokens[currentToken + 1]->line)
	{
		error("in %s at line %d, the \"-\" choice indicator and its corresponding sentence must be on the same line.", filePath, tokens[currentToken - 1]->line);
	}
	steps_in_tokens(2);
	if (!token_match(2, TOKEN_COMMAND, TOKEN_SENTENCE))
	{
		error("in %s at line %d, expected a \"->\" go to indicator and a sentence after a choice declaration.", filePath, tokens[currentToken]->line);
	}
	if (!strmatch(tokens[currentToken]->string, "GO_TO"))
	{
		error("in %s at line %d, expected \"->\" go to indicator and sentence after a choice declaration, got a %s token instead.", filePath, tokens[currentToken]->line, tokenStrings[tokens[currentToken]->type]);
	}
	if (tokens[currentToken]->indentationLevel != currentIndentationLevel + 1)
	{
		error("in %s at line %d, expected an indentation level of %d after a choice declaration, got an indentation level of %d instead.", filePath, tokens[currentToken]->line, currentIndentationLevel + 1, tokens[currentToken]->indentationLevel);
	}
	if (tokens[currentToken]->line != tokens[currentToken + 1]->line)
	{
		error("in %s at line %d, the \"->\" go to indicator and its corresponding sentence must be on the same line.", filePath, tokens[currentToken + 1]->line);
	}
	choice->sentence = xmalloc(sizeof (*choice->sentence));
	choice->sentence->string = NULL;
	choice->sentence->autoSkip = false;
	choice->sentence->string = strcopy(choice->sentence->string, tokens[currentToken - 1]->string);
	if (buf_len(choice->sentence->string) >= 6)
	{
		if (strmatch(&choice->sentence->string[buf_len(choice->sentence->string) - 6], " AUTO"))
		{
			choice->sentence->string[buf_len(choice->sentence->string) - 6] = '\0';
			choice->sentence->autoSkip = true;
		}
	}
	choice->goToCommand = xmalloc(sizeof (*choice->goToCommand));
	choice->goToCommand->type = COMMAND_GO_TO;
	choice->goToCommand->arguments = xmalloc(sizeof (*choice->goToCommand->arguments) * nbArguments[COMMAND_GO_TO]);
	for (int i = 0; i < nbArguments[COMMAND_GO_TO]; i++)
	{
		choice->goToCommand->arguments[i] = xmalloc(sizeof (*choice->goToCommand->arguments[i]));
	}
	choice->goToCommand->arguments[0]->type = PARAMETER_STRING;
	choice->goToCommand->arguments[0]->string = NULL;
	choice->goToCommand->arguments[0]->string = strcopy(choice->goToCommand->arguments[0]->string, tokens[currentToken + 1]->string);
	steps_in_tokens(2);
	return choice;
}

static CueExpression *parse_cue_expression();

static CueCondition *parse_cue_condition()
{
	CueCondition *cueCondition = xmalloc(sizeof (*cueCondition));

	step_in_tokens();

	cueCondition->logicExpression = parse_logic_expression(tokens[currentToken - 1]->line);

	cueCondition->resolved = false;

	cueCondition->currentExpression = 0;

	currentIndentationLevel++;

	cueCondition->cueExpressionsIf = NULL;
	while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != TOKEN_END_OF_FILE)
	{
		buf_add(cueCondition->cueExpressionsIf, parse_cue_expression());
	}
	if (!cueCondition->cueExpressionsIf)
	{
		error("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
	}
	currentIndentationLevel--;
	cueCondition->cueExpressionsElse = NULL;
	if (tokens[currentToken]->type == TOKEN_ELSE)
	{
		if (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->line != tokens[currentToken - 1]->line)
		{
			currentIndentationLevel++;

			while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != TOKEN_END_OF_FILE)
			{
				buf_add(cueCondition->cueExpressionsElse, parse_cue_expression());
			}
			if (!cueCondition->cueExpressionsElse)
			{
				error("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
			}
			currentIndentationLevel--;
		} else {
			error("in %s at line %d, token \"else\" must have the same indentation level as its corresponding \"if\" token.", filePath, tokens[currentToken]->line);
		}
	}
	return cueCondition;
}

static CueExpression *parse_cue_expression()
{
	CueExpression *cueExpression = xmalloc(sizeof (*cueExpression));

	if (tokens[currentToken - 1]->line == tokens[currentToken]->line)
	{
		error("in %s at line %d, current expression is followed by a %s token on the same line.", filePath, tokens[currentToken - 1]->line, tokenStrings[tokens[currentToken]->type]);
	}
	if (tokens[currentToken]->type == TOKEN_COMMAND)
	{
		if (currentCueMode == CUE_MODE_SENTENCE)
		{
			cueExpression->type = CUE_EXPRESSION_COMMAND;
			cueExpression->command = parse_command();
		} else {
			error("in %s at line %d, found command after a choice.", filePath, tokens[currentToken]->line);
		}
	} else if (tokens[currentToken]->type == TOKEN_MINUS) {
		if (currentCueMode == CUE_MODE_SENTENCE)
		{
			currentCueMode = CUE_MODE_CHOICE;
		}
		cueExpression->type = CUE_EXPRESSION_CHOICE;
		cueExpression->choice = parse_choice();
	} else if (tokens[currentToken]->type == TOKEN_SENTENCE) {
		if (currentCueMode == CUE_MODE_SENTENCE)
		{
			cueExpression->type = CUE_EXPRESSION_SENTENCE;
			cueExpression->sentence = xmalloc(sizeof (*cueExpression->sentence));
			cueExpression->sentence->string = NULL;
			cueExpression->sentence->autoSkip = false;
			cueExpression->sentence->string = strcopy(cueExpression->sentence->string, tokens[currentToken]->string);
			if (buf_len(cueExpression->sentence->string) >= 6)
			{
				if (strmatch(&cueExpression->sentence->string[buf_len(cueExpression->sentence->string) - 6], " AUTO"))
				{
					cueExpression->sentence->string[buf_len(cueExpression->sentence->string) - 6] = '\0';
					cueExpression->sentence->autoSkip = true;
				}
			}
			step_in_tokens();
		} else {
			error("in %s at line %d, found sentence after a choice.", filePath, tokens[currentToken]->line);
		}
	} else if (tokens[currentToken]->type == TOKEN_IF) {
		cueExpression->type = CUE_EXPRESSION_CUE_CONDITION;
		cueExpression->cueCondition = parse_cue_condition();
	} else {
		error("in %s at line %d, expected a cue expression, got a %s token instead.", filePath, tokens[currentToken]->line, tokenStrings[tokens[currentToken]->type]);
	}
	return cueExpression;
}

static Cue *parse_cue()
{
	Cue *cue = xmalloc(sizeof (*cue));

	currentCueMode = CUE_MODE_SENTENCE;

	cue->currentExpression = 0;

	cue->cueExpressions = NULL;

	step_in_tokens();
	if (tokens[currentToken - 1]->line != tokens[currentToken]->line)
	{
		cue->characterName = NULL;
	} else {
		if (!token_match(2, TOKEN_STRING, TOKEN_IDENTIFIER))
		{
			error("in %s at line %d, expected a speaker name identifier and a position identifier after \">\" speaker indicator token, found %s token instead.", filePath, tokens[currentToken]->line, tokenStrings[tokens[currentToken]->type]);
		}
		if (tokens[currentToken - 1]->line != tokens[currentToken + 1]->line)
		{
			error("in %s at line %d, the speaker name identifier and the position identifier must be on the same line as their corresponding \">\" speaker indicator token.", filePath, tokens[currentToken]->line);
		}
		cue->characterName = NULL;
		cue->characterName = strcopy(cue->characterName, tokens[currentToken]->string);
		cue->characterNamePosition = position_identifier_to_int(tokens[currentToken + 1]->string, tokens[currentToken - 1]->line);
		steps_in_tokens(2);

		if (token_match_on_line(tokens[currentToken - 1]->line, 2, TOKEN_STRING, TOKEN_IDENTIFIER))
		{
			cue->setCharacterInDeclaration = true;
			CueExpression *cueExpression = xmalloc(sizeof (*cueExpression));
			cueExpression->type = CUE_EXPRESSION_COMMAND;
			Command *set_character_command = xmalloc(sizeof (*set_character_command));
			set_character_command->type = COMMAND_SET_CHARACTER;
			set_character_command->arguments = xmalloc(sizeof (*set_character_command->arguments) * nbArguments[COMMAND_SET_CHARACTER]);
			for (int i = 0; i < nbArguments[COMMAND_SET_CHARACTER]; i++)
			{
				set_character_command->arguments[i] = xmalloc(sizeof (*set_character_command->arguments[i]));
			}
			set_character_command->arguments[0]->type = PARAMETER_NUMERIC;
			set_character_command->arguments[0]->numeric = position_identifier_to_int(tokens[currentToken + 1]->string, tokens[currentToken - 1]->line);
			set_character_command->arguments[1]->type = PARAMETER_STRING;
			set_character_command->arguments[1]->string = NULL;
			set_character_command->arguments[1]->string = strcopy(set_character_command->arguments[1]->string, tokens[currentToken - 2]->string);
			set_character_command->arguments[2]->type = PARAMETER_STRING;
			set_character_command->arguments[2]->string = NULL;
			set_character_command->arguments[2]->string = strcopy(set_character_command->arguments[2]->string, tokens[currentToken]->string);
			cueExpression->command = set_character_command;
			buf_add(cue->cueExpressions, cueExpression);
			steps_in_tokens(2);
			add_to_character_list(set_character_command->arguments[1]->string);
		} else {
			cue->setCharacterInDeclaration = false;
		}
	}

	currentIndentationLevel++;

	while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != TOKEN_END_OF_FILE)
	{
		buf_add(cue->cueExpressions, parse_cue_expression());
	}
	currentIndentationLevel--;
	return cue;
}

static KnotExpression *parse_knot_expression();

static KnotCondition *parse_knot_condition()
{
	KnotCondition *knotCondition = xmalloc(sizeof (*knotCondition));

	step_in_tokens();

	knotCondition->logicExpression = parse_logic_expression(tokens[currentToken - 1]->line);
	knotCondition->resolved = false;
	knotCondition->currentExpression = 0;

	currentIndentationLevel++;

	knotCondition->knotExpressionsIf = NULL;
	while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != TOKEN_END_OF_FILE)
	{
		buf_add(knotCondition->knotExpressionsIf, parse_knot_expression());
	}
	if (!knotCondition->knotExpressionsIf)
	{
		error("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
	}
	currentIndentationLevel--;
	knotCondition->knotExpressionsElse = NULL;
	if (tokens[currentToken]->type == TOKEN_ELSE)
	{
		if (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->line != tokens[currentToken - 1]->line)
		{
			step_in_tokens();
			currentIndentationLevel++;

			while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != TOKEN_END_OF_FILE)
			{
				buf_add(knotCondition->knotExpressionsElse, parse_knot_expression());
			}
			if (!knotCondition->knotExpressionsElse)
			{
				error("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
			}
			currentIndentationLevel--;
		} else {
			error("in %s at line %d, token \"else\" must have the same indentation level as its corresponding \"if\" token.", filePath, tokens[currentToken]->line);
		}
	}
	return knotCondition;
}

static KnotExpression *parse_knot_expression()
{
	KnotExpression *knotExpression = xmalloc(sizeof (*knotExpression));

	if (tokens[currentToken - 1]->line == tokens[currentToken]->line)
	{
		error("in %s at line %d, current expression is followed by a %s token on the same line.", filePath, tokens[currentToken - 1]->line, tokenStrings[tokens[currentToken]->type]);
	}
	if (tokens[currentToken]->indentationLevel != currentIndentationLevel)
	{
		error("in %s at line %d, indentation level is %d, expected an indentation level of %d.", filePath, tokens[currentToken]->line, tokens[currentToken]->indentationLevel, currentIndentationLevel);
	}
	if (tokens[currentToken]->type == TOKEN_COMMAND)
	{
		knotExpression->type = KNOT_EXPRESSION_COMMAND;
		knotExpression->command = parse_command();
	} else if (tokens[currentToken]->type == TOKEN_SUPERIOR) {
		knotExpression->type = KNOT_EXPRESSION_CUE;
		knotExpression->cue = parse_cue();
	} else if (tokens[currentToken]->type == TOKEN_IF) {
		knotExpression->type = KNOT_EXPRESSION_KNOT_CONDITION;
		knotExpression->knotCondition = parse_knot_condition();
	} else {
		error("in %s at line %d, expected a knot expression, got a %s token instead.", filePath, tokens[currentToken]->line, tokenStrings[tokens[currentToken]->type]);
	}
	return knotExpression;
}

static Knot *parse_knot()
{
	if (currentToken != 0)
	{
		if (tokens[currentToken - 1]->line == tokens[currentToken]->line)
		{
			error("in %s at line %d, current expression is followed by a %s token on the same line.", filePath, tokens[currentToken - 1]->line, tokenStrings[tokens[currentToken]->type]);
		}
	}
	Knot *knot = xmalloc(sizeof (*knot));
	knot->currentExpression = 0;
	currentIndentationLevel = 0;
	knot->name = NULL;
    if (firstKnot)
    {
        firstKnot = false;
		knot->name = strcopy(knot->name, "start");
    } else {
        if (tokens[currentToken]->type == TOKEN_KNOT)
        {
			if (tokens[currentToken]->indentationLevel != 0)
			{
				error("in %s at line %d, knot declarations must have an indentation level of 0, indentation level is %d.", filePath, tokens[currentToken]->line, tokens[currentToken]->indentationLevel);
			}
			for (unsigned int i = 0; i < buf_len(currentDialog->knots) - 1; i++)
			{
				if (strmatch(currentDialog->knots[i]->name, tokens[currentToken]->string))
				{
					error("in %s at line %d, knot name %s was already used.", filePath, tokens[currentToken]->line, tokens[currentToken]->string);
				}
			}
			knot->name = strcopy(knot->name, tokens[currentToken]->string);
			step_in_tokens();
        } else {
			error("in %s at line %d, expected knot token, got a %s token instead.", filePath, tokens[currentToken]->line, tokenStrings[tokens[currentToken]->type]);
		}
    }

	knot->knotExpressions = NULL;
	while (tokens[currentToken]->type != TOKEN_KNOT && tokens[currentToken]->type != TOKEN_END_OF_FILE)
	{
		buf_add(knot->knotExpressions, parse_knot_expression());
	}
	return knot;
}

Dialog *create_dialog(char *_filePath, Token **_tokens)
{
	filePath = _filePath;
	tokens = _tokens;
	Dialog *dialog = xmalloc(sizeof (*dialog));
	currentDialog = dialog;

	currentToken = 0;
	currentIndentationLevel = 0;
	firstKnot = true;

	currentCueMode = CUE_MODE_SENTENCE;

	if (strmatch(filePath, "Dialogs/start.dlg"))
	{
		if (token_match(1, TOKEN_SENTENCE))
		{
			windowName = strcopy(windowName, tokens[currentToken]->string);
			step_in_tokens();
		} else {
			error("expected game name at beginning of start.dlg, got %s token instead.", tokenStrings[tokens[currentToken]->type]);
		}
	}

	dialog->backgroundPacksNames = NULL;
	dialog->backgroundPacks = NULL;

	dialog->charactersNames = NULL;
	dialog->charactersAnimations = NULL;

	dialog->knots = NULL;
    while (tokens[currentToken]->type != TOKEN_END_OF_FILE)
    {
        buf_add(dialog->knots, parse_knot());
    }
	dialog->currentKnot = 0;
	dialog->end = false;

	for (unsigned int index = 0; index < buf_len(tokens); index++)
	{
		free_token(tokens[index]);
	}
	buf_free(tokens);

	return dialog;
}

static void free_logic_expression(LogicExpression *logicExpression)
{
	if (logicExpression->type == LOGIC_EXPRESSION_LITERAL)
	{
		if (logicExpression->literal->type == LOGIC_EXPRESSION_LITERAL_STRING)
		{
			buf_free(logicExpression->literal->string);
		} else if (logicExpression->literal->type == LOGIC_EXPRESSION_LITERAL_IDENTIFIER) {
			buf_free(logicExpression->literal->string);
		}
		xfree(logicExpression->literal);
	} else if (logicExpression->type == LOGIC_EXPRESSION_UNARY) {
		free_logic_expression(logicExpression->unary->expression);
		xfree(logicExpression->unary);
	} else if (logicExpression->type == LOGIC_EXPRESSION_BINARY) {
		free_logic_expression(logicExpression->binary->left);
		free_logic_expression(logicExpression->binary->right);
		xfree(logicExpression->binary);
	} else if (logicExpression->type == LOGIC_EXPRESSION_GROUPING) {
		free_logic_expression(logicExpression->grouping->expression);
		xfree(logicExpression->grouping);
	}
	xfree(logicExpression);
}

static void free_argument(Argument *argument)
{
	if (argument->type == PARAMETER_STRING)
	{
		if (argument->string)
		{
			buf_free(argument->string);
		}
	} else if (argument->type == PARAMETER_IDENTIFIER) {
		if (argument->string)
		{
			buf_free(argument->string);
		}
	} else if (argument->type == PARAMETER_LOGIC_EXPRESSION) {
		free_logic_expression(argument->logicExpression);
	}
	xfree(argument);
}

void free_command(Command *command)
{
	for (int i = 0; i < nbArguments[command->type]; i++)
	{
		free_argument(command->arguments[i]);
	}
	xfree(command->arguments);
	xfree(command);
}

static void free_sentence(Sentence *sentence)
{
	buf_free(sentence->string);
	xfree(sentence);
}

static void free_choice(Choice *choice)
{
	free_sentence(choice->sentence);
	free_command(choice->goToCommand);
	xfree(choice);
}

static void free_cue_expression(CueExpression *cueExpression);

static void free_cue_condition(CueCondition *cueCondition)
{
	free_logic_expression(cueCondition->logicExpression);
	for (unsigned int index = 0; index < buf_len(cueCondition->cueExpressionsIf); index++)
	{
		free_cue_expression(cueCondition->cueExpressionsIf[index]);
	}
	buf_free(cueCondition->cueExpressionsIf);
	for (unsigned int index = 0; index < buf_len(cueCondition->cueExpressionsElse); index++)
	{
		free_cue_expression(cueCondition->cueExpressionsElse[index]);
	}
	buf_free(cueCondition->cueExpressionsElse);
	xfree(cueCondition);
}

static void free_cue_expression(CueExpression *cueExpression)
{
	if (cueExpression->type == CUE_EXPRESSION_CHOICE)
	{
		free_choice(cueExpression->choice);
	} else if (cueExpression->type == CUE_EXPRESSION_COMMAND) {
		free_command(cueExpression->command);
	} else if (cueExpression->type == CUE_EXPRESSION_SENTENCE) {
		free_sentence(cueExpression->sentence);
	} else if (cueExpression->type == CUE_EXPRESSION_CUE_CONDITION) {
		free_cue_condition(cueExpression->cueCondition);
	}
	xfree(cueExpression);
}

static void free_cue(Cue *cue)
{
	buf_free(cue->characterName);
	for (unsigned int index = 0; index < buf_len(cue->cueExpressions); index++)
	{
		free_cue_expression(cue->cueExpressions[index]);
	}
	buf_free(cue->cueExpressions);
	xfree(cue);
}

static void free_knot_expression(KnotExpression *knotExpression);

static void free_knot_condition(KnotCondition *knotCondition)
{
	free_logic_expression(knotCondition->logicExpression);
	for (unsigned int index = 0; index < buf_len(knotCondition->knotExpressionsIf); index++)
	{
		free_knot_expression(knotCondition->knotExpressionsIf[index]);
	}
	buf_free(knotCondition->knotExpressionsIf);
	for (unsigned int index = 0; index < buf_len(knotCondition->knotExpressionsElse); index++)
	{
		free_knot_expression(knotCondition->knotExpressionsElse[index]);
	}
	buf_free(knotCondition->knotExpressionsElse);
	xfree(knotCondition);
}

static void free_knot_expression(KnotExpression *knotExpression)
{
	if (knotExpression->type == KNOT_EXPRESSION_COMMAND)
	{
		free_command(knotExpression->command);
	} else if (knotExpression->type == KNOT_EXPRESSION_CUE) {
		free_cue(knotExpression->cue);
	} else if (knotExpression->type == KNOT_EXPRESSION_KNOT_CONDITION) {
		free_knot_condition(knotExpression->knotCondition);
	}
	xfree(knotExpression);
}

static void free_knot(Knot *knot)
{
	buf_free(knot->name);
	for (unsigned int index = 0; index < buf_len(knot->knotExpressions); index++)
	{
		free_knot_expression(knot->knotExpressions[index]);
	}
	buf_free(knot->knotExpressions);
	xfree(knot);
}

void free_dialog(Dialog *dialog)
{
	for (unsigned int index = 0; index < buf_len(dialog->backgroundPacks); index++)
    {
		for (unsigned int index2 = 0; index2 < buf_len(dialog->backgroundPacks[index]); index2++)
		{
			free_animation(dialog->backgroundPacks[index][index2]);
		}
		buf_free(dialog->backgroundPacks[index]);
	}
	buf_free(dialog->backgroundPacks);
	for (unsigned int index = 0; index < buf_len(dialog->backgroundPacksNames); index++)
    {
		buf_free(dialog->backgroundPacksNames[index]);
	}
	buf_free(dialog->backgroundPacksNames);
	for (unsigned int index = 0; index < buf_len(dialog->charactersAnimations); index++)
    {
		for (unsigned int index2 = 0; index2 < buf_len(dialog->charactersAnimations[index]); index2++)
		{
			free_animation(dialog->charactersAnimations[index][index2]);
		}
		buf_free(dialog->charactersAnimations[index]);
	}
	buf_free(dialog->charactersAnimations);
	for (unsigned int index = 0; index < buf_len(dialog->charactersNames); index++)
    {
		buf_free(dialog->charactersNames[index]);
	}
	buf_free(dialog->charactersNames);
	for (unsigned int index = 0; index < buf_len(dialog->knots); index++)
    {
		free_knot(dialog->knots[index]);
	}
	buf_free(dialog->knots);
	xfree(dialog);
}
