#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

#include "parse.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "globals.h"

typedef enum CueMode
{
	CUE_MODE_SENTENCE,
	CUE_MODE_CHOICE
} CueMode;

static Token *tokens;

static char **backgroundPacksUsed;
static struct AnimatedSprite *backgroundPacksLoaded;

void add_to_background_list(char *backgroundPackName)
{
	bool match = false;
	for (int index = 0; index < buf_len(backgroundPacksUsed); index++)
	{
		if (strmatch(backgroundPackName, backgroundPacksUsed[index]))
		{
			match = true;
			break;
		}
	}
	if (!match)
	{
		buf_add(backgroundPacksUsed, backgroundPackName);
		buf_add(backgroundPacksLoaded, create_animated_sprite(backgroundPackName));
	}
}

static char **charactersUsed;
static struct AnimatedSprite *charactersLoaded;

void add_to_character_list(char *characterName)
{
	bool match = false;
	for (int index = 0; index < buf_len(charactersUsed); index++)
	{
		if (strmatch(characterName, charactersUsed[index]))
		{
			match = true;
			break;
		}
	}
	if (!match)
	{
		buf_add(charactersUsed, characterName);
		buf_add(charactersLoaded, create_animated_sprite(characterName));
	}
}

static int currentToken = 0;
static int currentIndentationLevel = 0;

static CueMode currentCueMode = CUE_MODE_SENTENCE;

static bool firstKnot = true;

static void step_in_tokens()
{
    if (tokens[currentToken++].type == TOKEN_END_OF_FILE)
    {
        error("stepped after end of tokens.");
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

static LogicExpression *create_logic_expression_literal_int(int integer)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->literal.type = LOGIC_EXPRESSION_LITERAL_NUMERIC;
	logicExpression->literal.numeric = integer;
	return logicExpression;
}

static LogicExpression *create_logic_expression_literal_string(char *string)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->literal.type = LOGIC_EXPRESSION_LITERAL_STRING;
	logicExpression->literal.text = string;
	return logicExpression;
}

static LogicExpression *create_logic_expression_literal_identifier(char *name)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_LITERAL;
	logicExpression->literal.type = LOGIC_EXPRESSION_LITERAL_IDENTIFIER;
	logicExpression->literal.text = name;
	return logicExpression;
}

static LogicExpression *create_logic_expression_grouping(LogicExpression *logicExpression)
{
	LogicExpression *groupedLogicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_GROUPING;
	logicExpression->grouping.expression = logicExpression;
	return groupedLogicExpression;
}

static LogicExpression *create_logic_expression_unary(LogicExpressionUnaryType operation, LogicExpression *logicExpression)
{
	LogicExpression *negativeLogicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_UNARY;
	logicExpression->unary.type = LOGIC_EXPRESSION_UNARY_NEGATION;
	logicExpression->unary.expression = logicExpression;
	return negativeLogicExpression;
}

static LogicExpression *create_logic_expression_binary(LogicExpression *left, LogicExpressionBinaryOperation operation, LogicExpression *right)
{
	LogicExpression *logicExpression = xmalloc(sizeof (*logicExpression));
	logicExpression->type = LOGIC_EXPRESSION_BINARY;
	logicExpression->binary.left = left;
	logicExpression->binary.operation = operation;
	logicExpression->binary.right = right;
	return logicExpression;
}

static LogicExpression *parse_logic_expression(int line);

static LogicExpression *parse_logic_expression_base(int line)
{
	LogicExpression *logicExpression;
	if (token_match_on_line(line, 1, TOKEN_NUMERIC)) {
		logicExpression = create_logic_expression_literal_int(tokens[currentToken].numeric);
	} else if (token_match_on_line(line, 1, TOKEN_STRING)) {
		logicExpression = create_logic_expression_literal_string(tokens[currentToken].text);
	} else if (token_match_on_line(line, 1, TOKEN_IDENTIFIER)) {
		logicExpression = create_logic_expression_literal_identifier(tokens[currentToken].text);
	} else if (token_match_on_line(line, 1, TOKEN_OPEN_PARENTHESIS)) {
		LogicExpression *groupedLogicExpression = parse_logic_expression(line);
		if (token_match_on_line(line, 1, TOKEN_CLOSE_PARENTHESIS))
		{
			logicExpression = create_logic_expression_grouping(groupedLogicExpression);
		} else {
			error("expected close parenthesis token at line %d before %s token.", line, tokenStrings[tokens[currentToken].type]);
		}
	} else {
		error("unexpected token %s in logic expression at line %d.", tokenStrings[tokens[currentToken].type], line);
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
		if (tokens[currentToken].type == TOKEN_STAR)
		{
			operation = LOGIC_EXPRESSION_BINARY_MULTIPLY;
		} else if (tokens[currentToken].type == TOKEN_SLASH) {
			operation = LOGIC_EXPRESSION_BINARY_DIVISE;
		} else {
			error("expected a multiplication or division operator token at line %d, got a %s token instead.", line, tokenStrings[tokens[currentToken].type]);
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
		if (tokens[currentToken].type == TOKEN_PLUS)
		{
			operation = LOGIC_EXPRESSION_BINARY_ADD;
		} else if (tokens[currentToken].type == TOKEN_MINUS) {
			operation = LOGIC_EXPRESSION_BINARY_SUBTRACT;
		} else {
			error("expected an addition or subtraction operator token at line %d, got a %s token instead.", line, tokenStrings[tokens[currentToken].type]);
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
		if (tokens[currentToken].type == TOKEN_EQUALS)
		{
			operation = LOGIC_EXPRESSION_BINARY_EQUALS;
		} else if (tokens[currentToken].type == TOKEN_DIFFERS) {
			operation = LOGIC_EXPRESSION_BINARY_DIFFERS;
		} else if (tokens[currentToken].type == TOKEN_INFERIOR_EQUALS) {
			operation = LOGIC_EXPRESSION_BINARY_INFERIOR_EQUALS;
		} else if (tokens[currentToken].type == TOKEN_INFERIOR) {
			operation = LOGIC_EXPRESSION_BINARY_INFERIOR;
		} else if (tokens[currentToken].type == TOKEN_SUPERIOR_EQUALS) {
			operation = LOGIC_EXPRESSION_BINARY_SUPERIOR_EQUALS;
		} else if (tokens[currentToken].type == TOKEN_SUPERIOR) {
			operation = LOGIC_EXPRESSION_BINARY_SUPERIOR;
		} else {
			error("expected a comparison operator token at line %d, got a %s token instead.", line, tokenStrings[tokens[currentToken].type]);
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
		error("\"%s\" identifier at line %d is not a position identifier.", identifier, line);
	}
}

static Command parse_command()
{
	Command command;

	if (strmatch(tokens[currentToken].text, "SET_BACKGROUND")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1].line, 3, TOKEN_STRING, TOKEN_SCOPE, TOKEN_STRING))
		{
			command.type = COMMAND_SET_BACKGROUND;
			command.arguments = xmalloc(sizeof (*command.arguments) * nbArguments[command.type]);
			command.arguments[0].type = PARAMETER_STRING;
			command.arguments[0].text = tokens[currentToken].text;
			command.arguments[1].type = PARAMETER_STRING;
			command.arguments[1].text = tokens[currentToken + 2].text;
			steps_in_tokens(3);
			add_to_background_list(command.arguments[0].text);
		} else {
			error("bad argument for #%s command at line %d, usage is #%s background-pack-identifier::background-identifier.", tokens[currentToken - 1].text, tokens[currentToken - 1].line, tokens[currentToken - 1].text);
		}
	} else if (strmatch(tokens[currentToken].text, "CLEAR_BACKGROUND")) {
		step_in_tokens();
		command.type = COMMAND_CLEAR_BACKGROUND;
	} else if (strmatch(tokens[currentToken].text, "SET_CHARACTER")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1].line, 4, TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_SCOPE, TOKEN_STRING))
		{
			command.type = COMMAND_SET_CHARACTER;
			command.arguments = xmalloc(sizeof (*command.arguments) * nbArguments[command.type]);
			command.arguments[0].type = PARAMETER_NUMERIC;
			command.arguments[0].numeric = position_identifier_to_int(tokens[currentToken].text, tokens[currentToken - 1].line);
			command.arguments[1].type = PARAMETER_STRING;
			command.arguments[1].text = tokens[currentToken + 1].text;
			command.arguments[2].type = PARAMETER_STRING;
			command.arguments[2].text = tokens[currentToken + 3].text;
			steps_in_tokens(4);
			add_to_character_list(command.arguments[1].text);
		} else {
			error("bad arguments for #%s command at line %d, usage is #%s position-identifier character-identifier::animation-identifier.", tokens[currentToken - 1].text, tokens[currentToken - 1].line, tokens[currentToken - 1].text);
		}
	} else if (strmatch(tokens[currentToken].text, "CLEAR_CHARACTER_POSITION")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1].line, 1, TOKEN_IDENTIFIER))
		{
			command.type = COMMAND_CLEAR_CHARACTER_POSITION;
			command.arguments = xmalloc(sizeof (*command.arguments) * nbArguments[command.type]);
			command.arguments[0].type = PARAMETER_NUMERIC;
			command.arguments[0].numeric = position_identifier_to_int(tokens[currentToken].text, tokens[currentToken - 1].line);
			step_in_tokens();
		} else {
			error("bad argument for #%s command at line %d, expected position identifier token.", tokens[currentToken - 1].text, tokens[currentToken - 1].line);
		}
	} else if (strmatch(tokens[currentToken].text, "CLEAR_CHARACTER_POSITIONS")) {
		step_in_tokens();
		command.type = COMMAND_CLEAR_CHARACTER_POSITIONS;
	} else if (strmatch(tokens[currentToken].text, "PAUSE")) {
		step_in_tokens();
		command.type = COMMAND_PAUSE;
	} else if (strmatch(tokens[currentToken].text, "END")) {
		step_in_tokens();
		command.type = COMMAND_END;
	} else if (strmatch(tokens[currentToken].text, "ASSIGN")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1].line, 1, TOKEN_IDENTIFIER))
		{
			command.type = COMMAND_ASSIGN;
			command.arguments = xmalloc(sizeof (*command.arguments) * nbArguments[command.type]);
			command.arguments[0].type = PARAMETER_IDENTIFIER;
			command.arguments[0].text = tokens[currentToken].text;
			step_in_tokens();
			command.arguments[1].type = PARAMETER_LOGIC_EXPRESSION;
			command.arguments[1].logicExpression = parse_logic_expression(tokens[currentToken - 1].line);
		} else {
			error("bad arguments for #%s command at line %d, expected variable identifier token first.", tokens[currentToken - 1].text, tokens[currentToken - 1].line);
		}
	} else if (strmatch(tokens[currentToken].text, "GO_TO")) {
		step_in_tokens();
		if (token_match_on_line(tokens[currentToken - 1].line, 1, TOKEN_SENTENCE))
		{
			command.type = COMMAND_CLEAR_CHARACTER_POSITION;
			command.arguments = xmalloc(sizeof (*command.arguments) * nbArguments[command.type]);
			command.arguments[0].type = PARAMETER_STRING;
			command.arguments[0].text = tokens[currentToken].text;
			step_in_tokens();
		} else {
			error("bad argument for #%s command at line %d, expected knot name token.", tokens[currentToken - 1].text, tokens[currentToken - 1].line);
		}
	} else {
		error("unknown command #%s at line %d.", tokens[currentToken].text, tokens[currentToken].line);
	}
	return command;
}

static Choice parse_choice()
{
	Choice choice;

	if (tokens[currentToken + 1].type != TOKEN_SENTENCE)
	{
		error("expected sentence at line %d after \"-\" choice indicator, got a %s token instead.", tokens[currentToken - 1].line, tokenStrings[tokens[currentToken].type]);
	}
	if (tokens[currentToken].line != tokens[currentToken + 1].line)
	{
		error("the \"-\" choice indicator at line %d and its corresponding sentence must be on the same line.", tokens[currentToken - 1].line);
	}
	steps_in_tokens(2);
	if (!token_match(2, TOKEN_COMMAND, TOKEN_SENTENCE))
	{
		error("expected a \"->\" go to indicator and a sentence at line %d after a choice declaration.", tokens[currentToken].line);
	}
	if (!strmatch(tokens[currentToken].text, "GO_TO"))
	{
		error("expected \"->\" go to indicator and sentence at line %d after a choice declaration, got a %s token instead.", tokens[currentToken].line, tokenStrings[tokens[currentToken].type]);
	}
	if (tokens[currentToken].indentationLevel != currentIndentationLevel + 1)
	{
		error("expected an indentation level of %d at line %d after a choice declaration, got an indentation level of %d instead.", currentIndentationLevel + 1, tokens[currentToken].line, tokens[currentToken].indentationLevel);
	}
	if (tokens[currentToken].line != tokens[currentToken + 1].line)
	{
		error("the \"->\" go to indicator at line %d and its corresponding sentence must be on the same line.", tokens[currentToken + 1].line);
	}
	choice.sentence = tokens[currentToken - 1].text;
	choice.knotToGo = tokens[currentToken + 1].text;
	steps_in_tokens(2);
	return choice;
}

static CueExpression parse_cue_expression();

static CueCondition parse_cue_condition()
{
	CueCondition cueCondition;

	step_in_tokens();

	cueCondition.logicExpression = parse_logic_expression(tokens[currentToken - 1].line);

	currentIndentationLevel++;

	cueCondition.cueExpressionsIf = NULL;
	while (tokens[currentToken].indentationLevel == currentIndentationLevel)
	{
		buf_add(cueCondition.cueExpressionsIf,  parse_cue_expression());
	}
	currentIndentationLevel--;
	if (tokens[currentToken].type == TOKEN_ELSE)
	{
		if (tokens[currentToken].indentationLevel == currentIndentationLevel && tokens[currentToken].line != tokens[currentToken - 1].line)
		{
			currentIndentationLevel++;

			cueCondition.cueExpressionsElse = NULL;
			while (tokens[currentToken].indentationLevel == currentIndentationLevel)
			{
				buf_add(cueCondition.cueExpressionsElse, parse_cue_expression());
			}
			currentIndentationLevel--;
		} else {
			error("at line %d, token \"else\" must have the same indentation level as its corresponding \"if\" token.", tokens[currentToken].line);
		}
	}
	return cueCondition;
}

static CueExpression parse_cue_expression()
{
	CueExpression cueExpression;

	if (tokens[currentToken - 1].line == tokens[currentToken].line)
	{
		error("the expression at line %d is followed by a %s token on the same line.", tokens[currentToken - 1].line, tokenStrings[tokens[currentToken].type]);
	}
	if (tokens[currentToken].type == TOKEN_COMMAND)
	{
		if (currentCueMode == CUE_MODE_SENTENCE)
		{
			cueExpression.type = CUE_EXPRESSION_COMMAND;
			cueExpression.command = parse_command();
		} else {
			error("found command after a choice at line %d.", tokens[currentToken].line);
		}
	} else if (tokens[currentToken].type == TOKEN_MINUS) {
		if (currentCueMode == CUE_MODE_SENTENCE)
		{
			currentCueMode = CUE_MODE_CHOICE;
		}
		cueExpression.type = CUE_EXPRESSION_CHOICE;
		cueExpression.choice = parse_choice();
	} else if (tokens[currentToken].type == TOKEN_SENTENCE) {
		if (currentCueMode == CUE_MODE_SENTENCE)
		{
			cueExpression.type = CUE_EXPRESSION_SENTENCE;
			cueExpression.sentence = tokens[currentToken].text;
			step_in_tokens();
		} else {
			error("found sentence after a choice at line %d.", tokens[currentToken].line);
		}
	} else if (tokens[currentToken].type == TOKEN_IF) {
		cueExpression.type = CUE_EXPRESSION_CUE_CONDITION;
		cueExpression.cueCondition = parse_cue_condition();
	} else {
		error("expected a cue expression at line %d, got a %s token instead.", tokens[currentToken].line, tokenStrings[tokens[currentToken].type]);
	}
	return cueExpression;
}

static Cue parse_cue()
{
	Cue cue;

	currentCueMode = CUE_MODE_SENTENCE;

	cue.cueExpressions = NULL;

	step_in_tokens();
	if (tokens[currentToken - 1].line != tokens[currentToken].line)
	{
		cue.characterName = NULL;
	} else {
		if (!token_match(2, TOKEN_STRING, TOKEN_IDENTIFIER))
		{
			error("expected a speaker name identifier and a position identifier after \">\" speaker indicator token, found %s token at line %d instead.", tokenStrings[tokens[currentToken].type], tokens[currentToken - 1].line);
		}
		if (tokens[currentToken - 1].line != tokens[currentToken + 1].line)
		{
			error("the speaker name identifier and the position identifier at line %d must be on the same line as their corresponding \">\" speaker indicator token.", tokens[currentToken].line);
		}
		cue.characterName = tokens[currentToken].text;
		cue.characterNamePosition = position_identifier_to_int(tokens[currentToken + 1].text, tokens[currentToken - 1].line);
		steps_in_tokens(2);

		if (token_match_on_line(tokens[currentToken - 1].line, 2, TOKEN_STRING, TOKEN_IDENTIFIER))
		{
			CueExpression cueExpression;
			cueExpression.type = CUE_EXPRESSION_COMMAND;
			Command set_character_command;
			set_character_command.type = COMMAND_SET_CHARACTER;
			set_character_command.arguments = xmalloc(sizeof (*set_character_command.arguments) * 3);
			set_character_command.arguments[0].type = PARAMETER_NUMERIC;
			set_character_command.arguments[0].numeric = position_identifier_to_int(tokens[currentToken + 1].text, tokens[currentToken - 1].line);
			set_character_command.arguments[1].type = PARAMETER_STRING;
			set_character_command.arguments[1].text = tokens[currentToken - 2].text;
			set_character_command.arguments[2].type = PARAMETER_STRING;
			set_character_command.arguments[2].text = tokens[currentToken].text;
			cueExpression.command = set_character_command;
			buf_add(cue.cueExpressions, cueExpression);
			steps_in_tokens(2);
			add_to_character_list(set_character_command.arguments[1].text);
		}
	}

	currentIndentationLevel++;

	while (tokens[currentToken].indentationLevel == currentIndentationLevel)
	{
		buf_add(cue.cueExpressions, parse_cue_expression());
	}
	currentIndentationLevel--;
	return cue;
}

static KnotExpression parse_knot_expression();

static KnotCondition parse_knot_condition()
{
	KnotCondition knotCondition;

	step_in_tokens();

	knotCondition.logicExpression = parse_logic_expression(tokens[currentToken - 1].line);

	currentIndentationLevel++;

	knotCondition.knotExpressionsIf = NULL;
	while (tokens[currentToken].indentationLevel == currentIndentationLevel)
	{
		buf_add(knotCondition.knotExpressionsIf, parse_knot_expression());
	}
	currentIndentationLevel--;
	if (tokens[currentToken].type == TOKEN_ELSE)
	{
		if (tokens[currentToken].indentationLevel == currentIndentationLevel && tokens[currentToken].line != tokens[currentToken - 1].line)
		{
			step_in_tokens();
			currentIndentationLevel++;

			knotCondition.knotExpressionsElse = NULL;
			while (tokens[currentToken].indentationLevel == currentIndentationLevel)
			{
				buf_add(knotCondition.knotExpressionsElse, parse_knot_expression());
			}
			currentIndentationLevel--;
		} else {
			error("at line %d, token \"else\" must have the same indentation level as its corresponding \"if\" token.", tokens[currentToken].line);
		}
	}
	return knotCondition;
}

static KnotExpression parse_knot_expression()
{
	KnotExpression knotExpression;

	if (tokens[currentToken - 1].line == tokens[currentToken].line)
	{
		error("the expression at line %d is followed by a %s token on the same line.", tokens[currentToken - 1].line, tokenStrings[tokens[currentToken].type]);
	}
	if (tokens[currentToken].indentationLevel != currentIndentationLevel)
	{
		error("indentation level at line %d is %d, expected an indentation level of %d.", tokens[currentToken].line, tokens[currentToken].indentationLevel, currentIndentationLevel);
	}
	if (tokens[currentToken].type == TOKEN_COMMAND)
	{
		knotExpression.type = KNOT_EXPRESSION_COMMAND;
		knotExpression.command = parse_command();
	} else if (tokens[currentToken].type == TOKEN_SUPERIOR) {
		knotExpression.type = KNOT_EXPRESSION_CUE;
		knotExpression.cue = parse_cue();
	} else if (tokens[currentToken].type == TOKEN_IF) {
		knotExpression.type = KNOT_EXPRESSION_KNOT_CONDITION;
		knotExpression.knotCondition = parse_knot_condition();
	} else {
		error("expected a knot expression at line %d, got a %s token instead.", tokens[currentToken].line, tokenStrings[tokens[currentToken].type]);
	}
	return knotExpression;
}

static Knot parse_knot()
{
	Knot knot;
	currentIndentationLevel = 0;
    if (firstKnot)
    {
        firstKnot = false;
    	knot.name = NULL;
		buf_add(knot.name, 's');
		buf_add(knot.name, 't');
		buf_add(knot.name, 'a');
		buf_add(knot.name, 'r');
		buf_add(knot.name, 't');
		buf_add(knot.name, '\0');
    } else {
        if (tokens[currentToken].type == TOKEN_KNOT)
        {
			if (tokens[currentToken].indentationLevel != 0)
			{
				error("knot declarations must have an indentation level of 0, indentation level at line %d is %d.", tokens[currentToken].line, tokens[currentToken].indentationLevel);
			}
			knot.name = tokens[currentToken].text;
			step_in_tokens();
        } else {
			error("expected knot token at line %d, got a %s token instead.", tokens[currentToken].line, tokenStrings[tokens[currentToken].type]);
		}
    }

	knot.knotExpressions = NULL;
	while (tokens[currentToken].type != TOKEN_KNOT && tokens[currentToken].type != TOKEN_END_OF_FILE)
	{
		buf_add(knot.knotExpressions, parse_knot_expression());
	}
	return knot;
}

Dialog parse(Token *_tokens)
{
	tokens = _tokens;
	Dialog dialog;

	dialog.knots = NULL;
    while (tokens[currentToken].type != TOKEN_END_OF_FILE)
    {
        buf_add(dialog.knots, parse_knot());
    }
	dialog.backgroundPacksNames = backgroundPacksUsed;
	dialog.backgroundPacks = backgroundPacksLoaded;
	dialog.charactersNames = charactersUsed;
	dialog.charactersSprites = charactersLoaded;
	return dialog;
}
