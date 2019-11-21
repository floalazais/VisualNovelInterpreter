#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "audio.h"
#include "maths.h"
#include "stretchy_buffer.h"
#include "token.h"
#include "lex.h"
#include "str.h"
#include "error.h"
#include "xalloc.h"
#include "animation.h"
#include "graphics.h"
#include "variable.h"
#include "dialog.h"
#include "globals_dialog.h"

static const char *filePath;
static buf(DialogToken *) tokens;
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
	if (tokens[currentToken++]->type == DIALOG_TOKEN_END_OF_FILE)
	{
		error("in %s stepped after end of tokens.", filePath);
	}
}

static void steps_in_tokens(unsigned stepsNumber)
{
	while (stepsNumber--)
	{
		step_in_tokens();
	}
}

static bool token_match(int matchListLength, ...)
{
	va_list arguments;
	bool match = true;
	va_start(arguments, matchListLength);
	for (int i = 0; i < matchListLength; i++)
	{
		if ((int)tokens[currentToken + i]->type != va_arg(arguments, int))
		{
			match = false;
			break;
		}
	}
	va_end(arguments);
	return match;
}

static bool token_match_on_line(int line, int matchListLength, ...)
{
	va_list arguments;
	bool match = true;
	va_start(arguments, matchListLength);
	for (int i = 0; i < matchListLength; i++)
	{
		if ((int)tokens[currentToken + i]->type != va_arg(arguments, int) || tokens[currentToken + i]->line != line)
		{
			match = false;
			break;
		}
	}
	va_end(arguments);
	return match;
}

/*static bool token_match_array(buf(int) tokenList)
{
	bool match = true;
	for (int i = 0; i < buf_len(tokenList); i++)
	{
		if ((int)tokens[currentToken + i]->type != tokenList[i])
		{
			match = false;
			break;
		}
	}
	return match;
}*/

static bool token_match_on_line_array(int line, int listLength, int *tokenList)
{
	bool match = true;
	for (int i = 0; i < listLength; i++)
	{
		if (tokens[currentToken + i]->type != tokenList[i] || tokens[currentToken + i]->line != line)
		{
			match = false;
			break;
		}
	}
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

static LogicExpression *create_logic_expression_literal_string(const char *string)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->returnType = VARIABLE_STRING;
	logicExpression->literal = xmalloc(sizeof (*logicExpression->literal));
	logicExpression->literal->type = LOGIC_EXPRESSION_LITERAL_STRING;
	logicExpression->literal->string = strclone(string);
	return logicExpression;
}

static LogicExpression *create_logic_expression_literal_identifier(const char *identifier)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->literal = xmalloc(sizeof (*logicExpression->literal));
	logicExpression->literal->type = LOGIC_EXPRESSION_LITERAL_IDENTIFIER;
	logicExpression->literal->string = strclone(identifier);
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
	if (token_match_on_line(line, 1, DIALOG_TOKEN_NUMERIC)) {
		logicExpression = create_logic_expression_literal_numeric(tokens[currentToken]->numeric);
	} else if (token_match_on_line(line, 1, DIALOG_TOKEN_STRING)) {
		logicExpression = create_logic_expression_literal_string(tokens[currentToken]->string);
	} else if (token_match_on_line(line, 1, DIALOG_TOKEN_IDENTIFIER)) {
		logicExpression = create_logic_expression_literal_identifier(tokens[currentToken]->string);
	} else if (token_match_on_line(line, 1, DIALOG_TOKEN_GROUPING_BEGIN)) {
		step_in_tokens();
		LogicExpression *groupedLogicExpression = parse_logic_expression(line);
		if (token_match_on_line(line, 1, DIALOG_TOKEN_GROUPING_END))
		{
			logicExpression = create_logic_expression_grouping(groupedLogicExpression);
		} else {
			error("in %s at line %d, expected close parenthesis token before %s.", filePath, line, dialog_token_to_string(tokens[currentToken]));
		}
	} else {
		error("in %s at line %d, unexpected %s in logic expression.", filePath, line, dialog_token_to_string(tokens[currentToken]));
	}
	step_in_tokens();
	return logicExpression;
}

static LogicExpression *parse_logic_expression_unary(int line)
{
	if (token_match_on_line(line, 1, DIALOG_TOKEN_SUBTRACT)) {
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
	while (token_match_on_line(line, 1, DIALOG_TOKEN_MULTIPLY) || token_match_on_line(line, 1, DIALOG_TOKEN_DIVIDE))
	{
		LogicExpressionBinaryOperation operation;
		if (tokens[currentToken]->type == DIALOG_TOKEN_MULTIPLY)
		{
			operation = LOGIC_EXPRESSION_BINARY_MULTIPLY;
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_DIVIDE) {
			operation = LOGIC_EXPRESSION_BINARY_DIVISE;
		} else {
			error("in %s at line %d, expected a multiplication or division operator token, got a %s instead.", filePath, line, dialog_token_to_string(tokens[currentToken]));
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
	while (token_match_on_line(line, 1, DIALOG_TOKEN_ADD) || token_match_on_line(line, 1, DIALOG_TOKEN_SUBTRACT))
	{
		LogicExpressionBinaryOperation operation;
		if (tokens[currentToken]->type == DIALOG_TOKEN_ADD)
		{
			operation = LOGIC_EXPRESSION_BINARY_ADD;
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_SUBTRACT) {
			operation = LOGIC_EXPRESSION_BINARY_SUBTRACT;
		} else {
			error("in %s at line %d, expected an addition or subtraction operator token, got a %s instead.", filePath, line, dialog_token_to_string(tokens[currentToken]));
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
	while (token_match_on_line(line, 1, DIALOG_TOKEN_EQUALS) || token_match_on_line(line, 1, DIALOG_TOKEN_DIFFERS) || token_match_on_line(line, 1, DIALOG_TOKEN_INFERIOR_EQUALS) || token_match_on_line(line, 1, DIALOG_TOKEN_INFERIOR) || token_match_on_line(line, 1, DIALOG_TOKEN_SUPERIOR_EQUALS) || token_match_on_line(line, 1, DIALOG_TOKEN_SUPERIOR))
	{
		LogicExpressionBinaryOperation operation;
		if (tokens[currentToken]->type == DIALOG_TOKEN_EQUALS)
		{
			operation = LOGIC_EXPRESSION_BINARY_EQUALS;
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_DIFFERS) {
			operation = LOGIC_EXPRESSION_BINARY_DIFFERS;
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_INFERIOR_EQUALS) {
			operation = LOGIC_EXPRESSION_BINARY_INFERIOR_EQUALS;
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_INFERIOR) {
			operation = LOGIC_EXPRESSION_BINARY_INFERIOR;
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_SUPERIOR_EQUALS) {
			operation = LOGIC_EXPRESSION_BINARY_SUPERIOR_EQUALS;
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_SUPERIOR) {
			operation = LOGIC_EXPRESSION_BINARY_SUPERIOR;
		} else {
			error("in %s at line %d, expected a comparison operator token, got a %s instead.", filePath, line, dialog_token_to_string(tokens[currentToken]));
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
	while (token_match_on_line(line, 1, DIALOG_TOKEN_AND))
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
	while (token_match_on_line(line, 1, DIALOG_TOKEN_OR))
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

Variable *get_variable(const char *variableName)
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
				variable->string = strclone(variablesValues[i]->string);
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
			variable->string = strclone(logicExpression->literal->string);
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
				} else if (right->type != VARIABLE_NUMERIC) {
					error("unknown variable type %d.", right->type);
				}
				left->numeric += right->numeric;
				free_variable(right);
			} else if (left->type == VARIABLE_STRING) {
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_NUMERIC)
				{
					error("cannot compute \"%s + %f\".", left->string, right->numeric);
				} else if (right->type != VARIABLE_STRING) {
					error("unknown variable type %d.", right->type);
				}
				strappend(&left->string, right->string);
				free_variable(right);
			} else {
				error("unknown variable type %d.", left->type);
			}
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_DIVISE) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a division on a string : \"%s\".", left->string);
			} else if (left->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a division on a string : \"%s\".", right->string);
			} else if (right->type != VARIABLE_NUMERIC) {
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
				} else if (right->type != VARIABLE_NUMERIC) {
					error("unknown variable type %d.", right->type);
				}
				left->numeric = left->numeric == right->numeric;
				free_variable(right);
			} else if (left->type == VARIABLE_STRING) {
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_NUMERIC)
				{
					error("cannot compute \"%s == %f\".", left->string, right->numeric);
				} else if (right->type != VARIABLE_STRING) {
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
				} else if (right->type != VARIABLE_NUMERIC) {
					error("unknown variable type %d.", right->type);
				}
				left->numeric = left->numeric != right->numeric;
				free_variable(right);
			} else if (left->type == VARIABLE_STRING) {
				right = resolve_logic_expression(logicExpression->binary->right);
				if (right->type == VARIABLE_NUMERIC)
				{
					error("cannot compute \"%s != %f\".", left->string, right->numeric);
				} else if (right->type != VARIABLE_STRING) {
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
			} else if (left->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute an inferior comparison on a string : \"%s\".", right->string);
			} else if (right->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", right->type);
			}
			left->numeric = left->numeric < right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_MULTIPLY) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a multiplication on a string : \"%s\".", left->string);
			} else if (left->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a multiplication on a string : \"%s\".", right->string);
			} else if (right->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", right->type);
			}
			left->numeric *= right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_SUBTRACT) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a subtraction on a string : \"%s\".", left->string);
			} else if (left->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a subtraction on a string : \"%s\".", right->string);
			} else if (right->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", right->type);
			}
			left->numeric -= right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_SUPERIOR) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a superior comparison on a string : \"%s\".", left->string);
			} else if (left->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a superior comparison on a string : \"%s\".", right->string);
			} else if (right->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", right->type);
			}
			left->numeric = left->numeric > right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_SUPERIOR_EQUALS) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute a superior or equals comparison on a string : \"%s\".", left->string);
			} else if (left->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute a superior or equals comparison on a string : \"%s\".", right->string);
			} else if (right->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", right->type);
			}
			left->numeric = left->numeric >= right->numeric;
			free_variable(right);
		} else if (logicExpression->binary->operation == LOGIC_EXPRESSION_BINARY_INFERIOR_EQUALS) {
			left = resolve_logic_expression(logicExpression->binary->left);
			if (left->type == VARIABLE_STRING)
			{
				error("cannot compute an inferior or equals comparison on a string : \"%s\".", left->string);
			} else if (left->type != VARIABLE_NUMERIC) {
				error("unknown variable type %d.", left->type);
			}
			right = resolve_logic_expression(logicExpression->binary->right);
			if (right->type == VARIABLE_STRING)
			{
				error("cannot compute an inferior or equals comparison on a string : \"%s\".", right->string);
			} else if (right->type != VARIABLE_NUMERIC) {
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

static void add_to_background_list(const char *backgroundPackName)
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
		buf_add(currentDialog->backgroundPacksNames, strclone(backgroundPackName));
		buf(char) animationFilePath = strclone("Animation files/");
		strappend(&animationFilePath, backgroundPackName);
		strappend(&animationFilePath, ".anm");
		buf_add(currentDialog->backgroundPacks, get_animations_from_file(animationFilePath, backgroundPackName));
		buf_free(animationFilePath);
	}
}

static void add_to_character_list(const char *characterName)
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
		buf_add(currentDialog->charactersNames, strclone(characterName));
		buf(char) animationFilePath = strclone("Animation files/");
		strappend(&animationFilePath, characterName);
		strappend(&animationFilePath, ".anm");
		buf_add(currentDialog->charactersAnimations, get_animations_from_file(animationFilePath, characterName));
		buf_free(animationFilePath);
	}
}

static void add_to_sound_list(const char *soundName)
{
	for (unsigned int index = 0; index < buf_len(currentDialog->soundsNames); index++)
	{
		if (strmatch(soundName, currentDialog->soundsNames[index]))
		{
			return;
		}
	}
	buf_add(currentDialog->soundsNames, strclone(soundName));
	buf(char) soundFilePath = strclone("Sounds/");
	//buf(char) soundFilePath = strclone("Sound files/");
	strappend(&soundFilePath, soundName);
	//strappend(&soundFilePath, ".snd");
	buf_free(soundFilePath);
}

static void add_to_music_list(const char *musicName)
{
	for (unsigned int index = 0; index < buf_len(currentDialog->musicsNames); index++)
	{
		if (strmatch(musicName, currentDialog->musicsNames[index]))
		{
			return;
		}
	}
	buf_add(currentDialog->musicsNames, strclone(musicName));
	buf(char) musicFilePath = strclone("Musics/");
	//buf(char) musicFilePath = strclone("Sound files/");
	strappend(&musicFilePath, musicName);
	//strappend(&musicFilePath, ".snd");
	buf_free(musicFilePath);
}

static GoTo *parse_go_to()
{
	GoTo *goTo = xmalloc(sizeof (*goTo));

	step_in_tokens();

	if (token_match_on_line(tokens[currentToken - 1]->line, 3, DIALOG_TOKEN_STRING, DIALOG_TOKEN_SCOPE, DIALOG_TOKEN_IDENTIFIER))
	{
		goTo->dialogFile = strclone(tokens[currentToken]->string);
		goTo->knotToGo = strclone(tokens[currentToken + 2]->string);
		steps_in_tokens(3);
	} else if (token_match_on_line(tokens[currentToken - 1]->line, 1, DIALOG_TOKEN_IDENTIFIER)) {
		goTo->dialogFile = NULL;
		goTo->knotToGo = strclone(tokens[currentToken]->string);
		step_in_tokens();
	} else {
		error("in %s at line %d, expected an identifier or a fileName as a string followed by a scope separator \"::\" and a knot identifier after %s, got %s instead.", filePath, tokens[currentToken]->line, tokens[currentToken - 1], dialog_token_to_string(tokens[currentToken]));
	}

	return goTo;
}

static Assignment *parse_assign()
{
	Assignment *assignment = xmalloc(sizeof (*assignment));

	step_in_tokens();

	if (!token_match_on_line(tokens[currentToken - 1]->line, 1, DIALOG_TOKEN_IDENTIFIER))
	{
		error("in %s at line %d, expected an identifier after #assignment keyword, got %s instead.", filePath, tokens[currentToken]->line, dialog_token_to_string(tokens[currentToken]));
	}
	assignment->identifier = strclone(tokens[currentToken]->string);
	step_in_tokens();
	assignment->logicExpression = parse_logic_expression(tokens[currentToken]->line);

	return assignment;
}

const char *argumentTypeDescriptions[] =
{
	[ARGUMENT_STRING] = "string",
	[ARGUMENT_IDENTIFIER] = "identifier",
	[ARGUMENT_NUMERIC] = "numeric"
};

typedef struct CommandPrototype
{
	buf(char) commandName;
	int tokenNumber;
	DialogTokenType *tokenTypes;
	CommandType commandType;
	int argumentNumber;
	ArgumentType *argumentTypes;
} CommandPrototype;

static const CommandPrototype commandPrototypes[] =
{
	{"set_background", 3, (DialogTokenType[3]){DIALOG_TOKEN_STRING, DIALOG_TOKEN_SCOPE, DIALOG_TOKEN_STRING}, COMMAND_SET_BACKGROUND, 2, (ArgumentType[2]){ARGUMENT_STRING, ARGUMENT_STRING}},
	{"clear_background", 0, NULL, COMMAND_CLEAR_BACKGROUND, 0, NULL},

	{"set_character", 4, (DialogTokenType[4]){DIALOG_TOKEN_POSITION_IDENTIFIER, DIALOG_TOKEN_STRING, DIALOG_TOKEN_SCOPE, DIALOG_TOKEN_STRING}, COMMAND_SET_CHARACTER, 3, (ArgumentType[3]){ARGUMENT_NUMERIC, ARGUMENT_STRING, ARGUMENT_STRING}},
	{"clear_character_position", 1, (DialogTokenType[1]){DIALOG_TOKEN_POSITION_IDENTIFIER}, COMMAND_CLEAR_CHARACTER_POSITION, 1, (ArgumentType[1]){ARGUMENT_NUMERIC}},
	{"clear_character_positions", 0, NULL, COMMAND_CLEAR_CHARACTER_POSITIONS, 0, NULL},

	{"play_music", 1, (DialogTokenType[1]){DIALOG_TOKEN_STRING}, COMMAND_PLAY_MUSIC, 1, (ArgumentType[1]){ARGUMENT_STRING}},
	{"stop_music", 0, NULL, COMMAND_STOP_MUSIC, 0, NULL},
	{"set_music_volume", 1, (DialogTokenType[1]){DIALOG_TOKEN_NUMERIC}, COMMAND_SET_MUSIC_VOLUME, 1, (ArgumentType[1]){ARGUMENT_NUMERIC}},

	{"play_sound", 1, (DialogTokenType[1]){DIALOG_TOKEN_STRING}, COMMAND_PLAY_SOUND, 1, (ArgumentType[1]){ARGUMENT_STRING}},
	{"stop_sound", 0, NULL, COMMAND_STOP_SOUND, 0, NULL},
	{"set_sound_volume", 1, (DialogTokenType[1]){DIALOG_TOKEN_NUMERIC}, COMMAND_SET_SOUND_VOLUME, 1, (ArgumentType[1]){ARGUMENT_NUMERIC}},

	{"hide_ui", 0, NULL, COMMAND_HIDE_UI, 0, NULL},

	{"wait", 1, (DialogTokenType[1]){DIALOG_TOKEN_NUMERIC}, COMMAND_WAIT, 1, (ArgumentType[1]){ARGUMENT_NUMERIC}},

	{"set_window_name", 1, (DialogTokenType[1]){DIALOG_TOKEN_STRING}, COMMAND_SET_WINDOW_NAME, 1, (ArgumentType[1]){ARGUMENT_STRING}},

	{"set_speaker_name_color", 4, (DialogTokenType[4]){DIALOG_TOKEN_STRING, DIALOG_TOKEN_NUMERIC, DIALOG_TOKEN_NUMERIC, DIALOG_TOKEN_NUMERIC}, COMMAND_SET_SPEAKER_NAME_COLOR, 4, (ArgumentType[4]){ARGUMENT_STRING, ARGUMENT_NUMERIC, ARGUMENT_NUMERIC, ARGUMENT_NUMERIC}}
};

static Command *parse_command()
{
	Command *command = xmalloc(sizeof (*command));

	bool foundCommand = false;
	for (int commandPrototypeIndex = 0; commandPrototypeIndex < NB_COMMANDS; commandPrototypeIndex++)
	{
		if (strmatch(tokens[currentToken]->string, commandPrototypes[commandPrototypeIndex].commandName))
		{
			step_in_tokens();
			if (token_match_on_line_array(tokens[currentToken]->line, commandPrototypes[commandPrototypeIndex].tokenNumber, (int *)commandPrototypes[commandPrototypeIndex].tokenTypes))
			{
				command->arguments = xmalloc(sizeof (*command->arguments) * commandPrototypes[commandPrototypeIndex].argumentNumber);
				int argumentNumber = 0;
				for (int tokenNumber = 0; tokenNumber < commandPrototypes[commandPrototypeIndex].tokenNumber; tokenNumber++)
				{
					ArgumentType argumentType = commandPrototypes[commandPrototypeIndex].argumentTypes[argumentNumber];
					if (tokens[currentToken + tokenNumber]->type == DIALOG_TOKEN_SCOPE)
					{
						continue;
					} else if (argumentType == ARGUMENT_NUMERIC) {
						command->arguments[argumentNumber] = xmalloc(sizeof (*command->arguments[argumentNumber]));
						command->arguments[argumentNumber]->type = argumentType;
						command->arguments[argumentNumber]->numeric = tokens[currentToken + tokenNumber]->numeric;
						argumentNumber++;
					} else {
						command->arguments[argumentNumber] = xmalloc(sizeof (*command->arguments[argumentNumber]));
						command->arguments[argumentNumber]->type = argumentType;
						command->arguments[argumentNumber]->string = strclone(tokens[currentToken + tokenNumber]->string);
						argumentNumber++;
					}
				}
				command->type = commandPrototypes[commandPrototypeIndex].commandType;
				if (command->type == COMMAND_SET_BACKGROUND)
				{
					add_to_background_list(command->arguments[0]->string);
				} else if (command->type == COMMAND_SET_CHARACTER) {
					add_to_character_list(command->arguments[1]->string);
				} else if (command->type == COMMAND_PLAY_MUSIC) {
					add_to_music_list(command->arguments[0]->string);
				} else if (command->type == COMMAND_PLAY_SOUND) {
					add_to_sound_list(command->arguments[0]->string);
				}
				steps_in_tokens(commandPrototypes[commandPrototypeIndex].tokenNumber);
				foundCommand = true;
				break;
			} else {
				buf(char) syntax = NULL;
				for (int tokenNumber = 0; tokenNumber < commandPrototypes[commandPrototypeIndex].tokenNumber; tokenNumber++)
				{
					strappend(&syntax, " ");
					strappend(&syntax, argumentTypeDescriptions[commandPrototypes[commandPrototypeIndex].argumentTypes[tokenNumber]]);
				}
				buf(char) arguments = NULL;
				for (int tokenNumber = 0; tokenNumber < commandPrototypes[commandPrototypeIndex].tokenNumber && tokens[currentToken + tokenNumber] != DIALOG_TOKEN_END_OF_FILE; tokenNumber++)
				{
					if (tokenNumber != 0)
					{
						strappend(&arguments, ", ");
					}
					strappend(&arguments, dialog_token_to_string(tokens[currentToken + tokenNumber]));
				}
				error("in %s at line %d, syntax for command #%s is : #%s%s, got %s instead.", filePath, tokens[currentToken]->line, tokens[currentToken - 1]->string, tokens[currentToken - 1]->string, syntax, arguments);
			}
		}
	}
	if (!foundCommand)
	{
		error("in %s at line %d, unknown command #%s.", filePath, tokens[currentToken]->line, tokens[currentToken]->string);
	}
	return command;
}

static Choice *parse_choice()
{
	Choice *choice = xmalloc(sizeof (*choice));

	choice->sentence = xmalloc(sizeof (*choice->sentence));
	choice->sentence->string = strclone(tokens[currentToken]->string);
	choice->sentence->autoSkip = false;
	step_in_tokens();

	if (!token_match_on_line(tokens[currentToken - 1]->line + 1, 1, DIALOG_TOKEN_GO_TO))
	{
		error("in %s at line %d, expected a go to indicator \"->\" and a knot identifier on the line below a choice declaration.", filePath, tokens[currentToken - 1]->line + 1);
	}
	if (tokens[currentToken]->indentationLevel != currentIndentationLevel + 1)
	{
		error("in %s at line %d, expected an indentation level of %d after a choice declaration, got an indentation level of %d instead.", filePath, tokens[currentToken]->line, currentIndentationLevel + 1, tokens[currentToken]->indentationLevel);
	}
	choice->goToCommand = parse_go_to();
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
	while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != DIALOG_TOKEN_END_OF_FILE)
	{
		buf_add(cueCondition->cueExpressionsIf, parse_cue_expression());
	}
	if (!cueCondition->cueExpressionsIf)
	{
		warning("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
	}
	currentIndentationLevel--;
	cueCondition->cueExpressionsElse = NULL;
	if (tokens[currentToken]->type == DIALOG_TOKEN_ELSE)
	{
		if (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->line != tokens[currentToken - 1]->line)
		{
			currentIndentationLevel++;

			while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != DIALOG_TOKEN_END_OF_FILE)
			{
				buf_add(cueCondition->cueExpressionsElse, parse_cue_expression());
			}
			if (!cueCondition->cueExpressionsElse)
			{
				warning("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
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
		error("in %s at line %d, current expression is followed by a %s on the same line.", filePath, tokens[currentToken - 1]->line, dialog_token_to_string(tokens[currentToken]));
	}
	if (tokens[currentToken]->type == DIALOG_TOKEN_IF)
	{
		cueExpression->type = CUE_EXPRESSION_CUE_CONDITION;
		cueExpression->cueCondition = parse_cue_condition();
	} else if (tokens[currentToken]->type == DIALOG_TOKEN_CHOICE) {
		currentCueMode = CUE_MODE_CHOICE;
		cueExpression->type = CUE_EXPRESSION_CHOICE;
		cueExpression->choice = parse_choice();
	} else if (currentCueMode == CUE_MODE_SENTENCE) {
		if (tokens[currentToken]->type == DIALOG_TOKEN_COMMAND)
		{
			cueExpression->type = CUE_EXPRESSION_COMMAND;
			cueExpression->command = parse_command();
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_ASSIGN) {
			cueExpression->type = CUE_EXPRESSION_ASSIGNMENT;
			cueExpression->assignment = parse_assign();
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_GO_TO) {
			cueExpression->type = CUE_EXPRESSION_GO_TO;
			cueExpression->goTo = parse_go_to();
		} else if (tokens[currentToken]->type == DIALOG_TOKEN_SENTENCE) {
			cueExpression->type = CUE_EXPRESSION_SENTENCE;
			cueExpression->sentence = xmalloc(sizeof (*cueExpression->sentence));
			cueExpression->sentence->string = strclone(tokens[currentToken]->string);
			step_in_tokens();
			if (token_match_on_line(tokens[currentToken - 1]->line, 1, DIALOG_TOKEN_IDENTIFIER) && strmatch(tokens[currentToken]->string, "auto"))
			{
				cueExpression->sentence->autoSkip = true;
				step_in_tokens();
			} else {
				cueExpression->sentence->autoSkip = false;
			}
		} else {
			error("in %s at line %d, expected a cue expression, got a %s instead.", filePath, tokens[currentToken]->line, dialog_token_to_string(tokens[currentToken]));
		}
	} else {
		error("in %s at line %d, found %s after a choice.", filePath, tokens[currentToken]->line, dialog_token_to_string(tokens[currentToken]));
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
	if (!tokens[currentToken - 1]->string)
	{
		cue->characterName = NULL;
	} else {
		cue->characterName = strclone(tokens[currentToken - 1]->string);
		if (!token_match(1, DIALOG_TOKEN_POSITION_IDENTIFIER))
		{
			error("in %s at line %d, expected a position identifier after %s, found %s instead.", filePath, tokens[currentToken]->line, dialog_token_to_string(tokens[currentToken - 1]), dialog_token_to_string(tokens[currentToken]));
		}
		if (tokens[currentToken - 1]->line != tokens[currentToken]->line)
		{
			error("in %s at line %d, the %s and the position identifier must be on the same line.", filePath, tokens[currentToken]->line, dialog_token_to_string(tokens[currentToken - 1]));
		}
		cue->characterNamePosition = tokens[currentToken]->numeric;
		step_in_tokens();

		if (token_match_on_line(tokens[currentToken - 1]->line, 2, DIALOG_TOKEN_STRING, DIALOG_TOKEN_POSITION_IDENTIFIER))
		{
			cue->setCharacterCommandInDeclaration = true;
			CueExpression *cueExpression = xmalloc(sizeof (*cueExpression));
			cueExpression->type = CUE_EXPRESSION_COMMAND;
			Command *setCharacterCommand = xmalloc(sizeof (*setCharacterCommand));
			setCharacterCommand->type = COMMAND_SET_CHARACTER;
			setCharacterCommand->arguments = xmalloc(sizeof (*setCharacterCommand->arguments) * 3);
			setCharacterCommand->arguments[0] = xmalloc(sizeof (*setCharacterCommand->arguments[0]));
			setCharacterCommand->arguments[0]->type = ARGUMENT_NUMERIC;
			setCharacterCommand->arguments[0]->numeric = tokens[currentToken + 1]->numeric;
			setCharacterCommand->arguments[1] = xmalloc(sizeof (*setCharacterCommand->arguments[1]));
			setCharacterCommand->arguments[1]->type = ARGUMENT_STRING;
			setCharacterCommand->arguments[1]->string = strclone(cue->characterName);
			setCharacterCommand->arguments[2] = xmalloc(sizeof (*setCharacterCommand->arguments[2]));
			setCharacterCommand->arguments[2]->type = ARGUMENT_STRING;
			setCharacterCommand->arguments[2]->string = strclone(tokens[currentToken]->string);
			cueExpression->command = setCharacterCommand;
			add_to_character_list(cue->characterName);
			buf_add(cue->cueExpressions, cueExpression);
			steps_in_tokens(2);
		} else {
			cue->setCharacterCommandInDeclaration = false;
		}
	}

	currentIndentationLevel++;

	while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != DIALOG_TOKEN_END_OF_FILE)
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
	while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != DIALOG_TOKEN_END_OF_FILE)
	{
		buf_add(knotCondition->knotExpressionsIf, parse_knot_expression());
	}
	if (!knotCondition->knotExpressionsIf)
	{
		warning("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
	}
	currentIndentationLevel--;
	knotCondition->knotExpressionsElse = NULL;
	if (tokens[currentToken]->type == DIALOG_TOKEN_ELSE)
	{
		if (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->line != tokens[currentToken - 1]->line)
		{
			step_in_tokens();
			currentIndentationLevel++;

			while (tokens[currentToken]->indentationLevel == currentIndentationLevel && tokens[currentToken]->type != DIALOG_TOKEN_END_OF_FILE)
			{
				buf_add(knotCondition->knotExpressionsElse, parse_knot_expression());
			}
			if (!knotCondition->knotExpressionsElse)
			{
				warning("in %s at line %d, condition has no effect.", filePath, tokens[currentToken - 1]->line);
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

	if (currentToken != 0)
	{
		if (tokens[currentToken - 1]->line == tokens[currentToken]->line)
		{
			error("in %s at line %d, current expression is followed by a %s on the same line.", filePath, tokens[currentToken - 1]->line, dialog_token_to_string(tokens[currentToken]));
		}
	}
	if (tokens[currentToken]->indentationLevel != currentIndentationLevel)
	{
		error("in %s at line %d, indentation level is %d, expected an indentation level of %d.", filePath, tokens[currentToken]->line, tokens[currentToken]->indentationLevel, currentIndentationLevel);
	}
	if (tokens[currentToken]->type == DIALOG_TOKEN_COMMAND)
	{
		knotExpression->type = KNOT_EXPRESSION_COMMAND;
		knotExpression->command = parse_command();
	} else if (tokens[currentToken]->type == DIALOG_TOKEN_ASSIGN) {
		knotExpression->type = KNOT_EXPRESSION_ASSIGNMENT;
		knotExpression->assignment = parse_assign();
	} else if (tokens[currentToken]->type == DIALOG_TOKEN_GO_TO) {
		knotExpression->type = KNOT_EXPRESSION_GO_TO;
		knotExpression->goTo = parse_go_to();
	} else if (tokens[currentToken]->type == DIALOG_TOKEN_SPEAKER) {
		knotExpression->type = KNOT_EXPRESSION_CUE;
		knotExpression->cue = parse_cue();
	} else if (tokens[currentToken]->type == DIALOG_TOKEN_IF) {
		knotExpression->type = KNOT_EXPRESSION_KNOT_CONDITION;
		knotExpression->knotCondition = parse_knot_condition();
	} else {
		error("in %s at line %d, expected a knot expression, got a %s instead.", filePath, tokens[currentToken]->line, dialog_token_to_string(tokens[currentToken]));
	}
	return knotExpression;
}

static Knot *parse_knot()
{
	if (currentToken != 0)
	{
		if (tokens[currentToken - 1]->line == tokens[currentToken]->line)
		{
			error("in %s at line %d, current expression is followed by a %s on the same line.", filePath, tokens[currentToken - 1]->line, dialog_token_to_string(tokens[currentToken]));
		}
	}
	Knot *knot = xmalloc(sizeof (*knot));
	knot->currentExpression = 0;
	currentIndentationLevel = 0;
	knot->name = NULL;
	if (firstKnot)
	{
		firstKnot = false;
		strcopy(&knot->name, "start");
	} else {
		if (tokens[currentToken]->type == DIALOG_TOKEN_KNOT)
		{
			if (tokens[currentToken]->indentationLevel != 0)
			{
				error("in %s at line %d, knot declarations must have an indentation level of 0, indentation level is %d.", filePath, tokens[currentToken]->line, tokens[currentToken]->indentationLevel);
			}

			if (strmatch(tokens[currentToken]->string, "start"))
			{
				error("in %s at line %d, knot identifier \"start\" is reserved.", filePath, tokens[currentToken]->line);
			} else if (strmatch(tokens[currentToken]->string, "end")) {
				error("in %s at line %d, knot identifier \"end\" is reserved.", filePath, tokens[currentToken]->line);
			} else {
				for (unsigned int i = 0; i < buf_len(currentDialog->knots) - 1; i++)
				{
					if (strmatch(currentDialog->knots[i]->name, tokens[currentToken]->string))
					{
						error("in %s at line %d, knot identifier %s was already used.", filePath, tokens[currentToken]->line, tokens[currentToken]->string);
					}
				}
			}
			strcopy(&knot->name, tokens[currentToken]->string);
			step_in_tokens();
		} else {
			error("in %s at line %d, expected knot token, got a %s instead.", filePath, tokens[currentToken]->line, dialog_token_to_string(tokens[currentToken]));
		}
	}

	knot->knotExpressions = NULL;
	while (tokens[currentToken]->type != DIALOG_TOKEN_KNOT && tokens[currentToken]->type != DIALOG_TOKEN_END_OF_FILE)
	{
		buf_add(knot->knotExpressions, parse_knot_expression());
	}
	return knot;
}

Dialog *get_dialog_from_file(const char *_filePath)
{
	filePath = _filePath;
	tokens = lex_dialog(_filePath);
	Dialog *dialog = xmalloc(sizeof (*dialog));
	currentDialog = dialog;

	currentToken = 0;
	currentIndentationLevel = 0;
	firstKnot = true;

	currentCueMode = CUE_MODE_SENTENCE;

	dialog->backgroundPacksNames = NULL;
	dialog->backgroundPacks = NULL;

	dialog->charactersNames = NULL;
	dialog->charactersAnimations = NULL;

	dialog->namesColors = NULL;
	dialog->coloredNames = NULL;

	dialog->soundsNames = NULL;

	dialog->musicsNames = NULL;

	dialog->knots = NULL;

	while (tokens[currentToken]->type != DIALOG_TOKEN_END_OF_FILE)
	{
		buf_add(dialog->knots, parse_knot());
	}
	dialog->currentKnot = 0;
	dialog->end = false;

	for (unsigned int index = 0; index < buf_len(tokens); index++)
	{
		free_dialog_token(tokens[index]);
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

void free_go_to(GoTo *goTo)
{
	buf_free(goTo->dialogFile);
	buf_free(goTo->knotToGo);
	xfree(goTo);
}

static void free_assign(Assignment *assignment)
{
	buf_free(assignment->identifier);
	free_logic_expression(assignment->logicExpression);
	xfree(assignment);
}

static void free_argument(Argument *argument)
{
	if (argument->type == ARGUMENT_STRING)
	{
		if (argument->string)
		{
			buf_free(argument->string);
		}
	} else if (argument->type == ARGUMENT_IDENTIFIER) {
		if (argument->string)
		{
			buf_free(argument->string);
		}
	}
	xfree(argument);
}

static void free_command(Command *command)
{
	for (int i = 0; i < commandPrototypes[command->type].argumentNumber; i++)
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
	free_go_to(choice->goToCommand);
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
	} else if (cueExpression->type == CUE_EXPRESSION_GO_TO) {
		free_go_to(cueExpression->goTo);
	} else if (cueExpression->type == CUE_EXPRESSION_ASSIGNMENT) {
		free_assign(cueExpression->assignment);
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
	if (knotExpression->type == KNOT_EXPRESSION_GO_TO)
	{
		free_go_to(knotExpression->goTo);
	} else if (knotExpression->type == KNOT_EXPRESSION_ASSIGNMENT) {
		free_assign(knotExpression->assignment);
	} else if (knotExpression->type == KNOT_EXPRESSION_COMMAND) {
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
	buf_free(dialog->namesColors);
	for (unsigned int index = 0; index < buf_len(dialog->coloredNames); index++)
	{
		buf_free(dialog->coloredNames[index]);
	}
	buf_free(dialog->coloredNames);
	for (unsigned int index = 0; index < buf_len(dialog->soundsNames); index++)
	{
		buf_free(dialog->soundsNames[index]);
	}
	buf_free(dialog->soundsNames);
	for (unsigned int index = 0; index < buf_len(dialog->musicsNames); index++)
	{
		buf_free(dialog->musicsNames[index]);
	}
	buf_free(dialog->musicsNames);
	for (unsigned int index = 0; index < buf_len(dialog->knots); index++)
	{
		free_knot(dialog->knots[index]);
	}
	buf_free(dialog->knots);
	xfree(dialog);
}
