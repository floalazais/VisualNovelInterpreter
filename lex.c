#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "xalloc.h"
#include "stretchy_buffer.h"
#include "file.h"
#include "error.h"
#include "str.h"
#include "token.h"
#include "lex.h"

static const char *filePath;
static char *fileString;

static int currentLine = 1;
static int currentIndentationLevel = 0;
static int currentCharIndex = 0;
static int currentCommentDepth = 0;

static buf(DialogToken *) dialogTokens;

typedef enum LexMode
{
	LEX_MODE_TEXT,
	LEX_MODE_CODE
} LexMode;

static LexMode currentLexMode = LEX_MODE_TEXT;

static buf(AnimationToken *)animationTokens;

static void step_in_source()
{
	if (fileString[currentCharIndex++] == '\0')
	{
		error("in %s at line %d stepped after end of file.", filePath, currentLine);
	}
}

static void steps_in_source(unsigned nb)
{
	while (nb--)
	{
		step_in_source();
	}
}

static buf(char) get_string()
{
	if (fileString[currentCharIndex] != '"')
	{
		error("in %s at line %d unexpected char %c found while searching for string delimiter \".", filePath, currentLine, fileString[currentCharIndex]);
	}
	step_in_source();
	buf(char) result = NULL;
	char charToAdd;
	while (fileString[currentCharIndex] != '"')
	{
		if (fileString[currentCharIndex] == '\0')
		{
			error("in %s at line %d unclosed string found.", filePath, currentLine);
		} else if (fileString[currentCharIndex] == '\n' || fileString[currentCharIndex] == '\r') {
			error("in %s at line %d unclosed string found.", filePath, currentLine);
		} else if (fileString[currentCharIndex] == '\t') {
			error("in %s at line %d tab character is reserved for indentation, its use in the middle of a string is forbidden.", filePath, currentLine);
		} else if (fileString[currentCharIndex] == '\\') {
			if (fileString[currentCharIndex + 1] == '"')
			{
				charToAdd = '"';
				step_in_source();
			} else {
				charToAdd = '\\';
			}
		} else {
			charToAdd = fileString[currentCharIndex];
		}
		buf_add(result, charToAdd);
		step_in_source();
	}
	step_in_source();
	buf_add(result, '\0');
	return result;
}

static buf(char) get_identifier()
{
	if (!isalpha(fileString[currentCharIndex]) && fileString[currentCharIndex] != '_')
	{
		error("in %s at line %d unexpected char %c found while searching for identifier, identifier name must begin with a letter or an underscore.", filePath, currentLine, fileString[currentCharIndex]);
	}
	buf(char) result = NULL;
	buf_add(result, fileString[currentCharIndex]);
	step_in_source();
	while (isalnum(fileString[currentCharIndex]) || fileString[currentCharIndex] == '_'|| fileString[currentCharIndex] == '-'|| fileString[currentCharIndex] == '\'')
	{
		buf_add(result, fileString[currentCharIndex]);
		step_in_source();
	}
	buf_add(result, '\0');
	return result;
}

static buf(char) get_sentence()
{
	buf(char) result = NULL;
	if (fileString[currentCharIndex] == '\\')
	{
		if (fileString[currentCharIndex + 1] == '@' || fileString[currentCharIndex + 1] == '>' || fileString[currentCharIndex + 1] == '#' || fileString[currentCharIndex + 1] == '-' || fileString[currentCharIndex + 1] == '\\')
		{
			step_in_source();
		}
	}
	while (fileString[currentCharIndex] != '\n' && fileString[currentCharIndex] != '\r' && fileString[currentCharIndex] != '\t' && fileString[currentCharIndex] != '\0')
	{
		buf_add(result, fileString[currentCharIndex]);
		step_in_source();
	}
	buf_add(result, '\0');
	return result;
}

#define stack(a) buf(a)

static stack(int) multilineCommentsLines = NULL;

static void stack_push(stack(int) *stack, int value)
{
	buf_add(*stack, value);
}

static int stack_pop(stack(int) *stack)
{
	int result = (*stack)[buf_len(*stack) - 1];
	_buf_header(*stack)->count--;
	return result;
}

static int stack_top(stack(int) *stack)
{
	return (*stack)[buf_len(*stack) - 1];
}

static DialogToken *get_next_dialog_token()
{
	DialogToken *token = xmalloc(sizeof (*token));

	startlex:

	while (isspace(fileString[currentCharIndex]))
	{
		if (fileString[currentCharIndex] == '\n')
		{
			currentLexMode = LEX_MODE_TEXT;
			currentLine++;
			currentIndentationLevel = 0;
		} else if (fileString[currentCharIndex] == '\t') {
			if (currentLexMode == LEX_MODE_TEXT && (buf_len(dialogTokens) == 0 || dialogTokens[buf_len(dialogTokens) - 1]->line != currentLine))
			{
				currentIndentationLevel++;
			} else {
				error("in %s at line %d tab character is reserved for indentation, its use in the middle of a line is forbidden.", filePath, currentLine);
			}
		}
		step_in_source();
	}

	if (fileString[currentCharIndex] == '/' && fileString[currentCharIndex + 1] == '/')
	{
		steps_in_source(2);
		while (fileString[currentCharIndex] != '\n' && fileString[currentCharIndex] != '\0')
		{
			step_in_source();
		}
		goto startlex;
	} else if (fileString[currentCharIndex] == '/' && fileString[currentCharIndex + 1] == '*') {
		stack_push(&multilineCommentsLines, currentLine);
		steps_in_source(2);
		currentCommentDepth = 1;
		while (currentCommentDepth != 0)
		{
			if (fileString[currentCharIndex] == '/' && fileString[currentCharIndex + 1] == '*')
			{
				currentCommentDepth++;
				stack_push(&multilineCommentsLines, currentLine);
				steps_in_source(2);
			} else if (fileString[currentCharIndex] == '*' && fileString[currentCharIndex + 1] == '/') {
				currentCommentDepth--;
				stack_pop(&multilineCommentsLines);
				steps_in_source(2);
			} else if (fileString[currentCharIndex] == '\n') {
				currentLexMode = LEX_MODE_TEXT;
				currentLine++;
				currentIndentationLevel = 0;
				step_in_source();
			} else if (fileString[currentCharIndex] == '\0') {
				error("in %s unclosed multiline comment at line %d.", filePath, stack_top(&multilineCommentsLines));
			} else {
				step_in_source();
			}
		}
		goto startlex;
	}

	token->line = currentLine;
	token->indentationLevel = currentIndentationLevel;

	char currentChar = fileString[currentCharIndex];

	if (currentChar == '\0')
	{
		token->type = DIALOG_TOKEN_END_OF_FILE;
	} else if (currentLexMode == LEX_MODE_TEXT) {
		if (currentChar == '@')
		{
			step_in_source();
			buf(char) knot = get_identifier();
			token->type = DIALOG_TOKEN_KNOT;
			token->string = knot;
		} else if (currentChar == '>') {
			step_in_source();
			currentLexMode = LEX_MODE_CODE;
			buf(char) speaker = NULL;
			if (fileString[currentCharIndex] == '"')
			{
				speaker = get_string();
			}
			token->type = DIALOG_TOKEN_SPEAKER;
			token->string = speaker;
		} else if (currentChar == '#') {
			step_in_source();
			currentLexMode = LEX_MODE_CODE;
			buf(char) command = get_identifier();
			if (strmatch(command, "if"))
			{
				token->type = DIALOG_TOKEN_IF;
				buf_free(command);
			} else if (strmatch(command, "else")) {
				token->type = DIALOG_TOKEN_ELSE;
				buf_free(command);
			} else if (strmatch(command, "assign")) {
				token->type = DIALOG_TOKEN_ASSIGN;
				buf_free(command);
			} else {
				token->type = DIALOG_TOKEN_COMMAND;
				token->string = command;
			}
		} else if (currentChar == '-') {
			step_in_source();
			if (fileString[currentCharIndex] == '>')
			{
				step_in_source();
				currentLexMode = LEX_MODE_CODE;
				token->type = DIALOG_TOKEN_GO_TO;
			} else {
				buf(char) choice = get_sentence();
				token->type = DIALOG_TOKEN_CHOICE;
				token->string = choice;
			}
		} else {
			buf(char) sentence = get_sentence();
			token->type = DIALOG_TOKEN_SENTENCE;
			token->string = sentence;
		}
	} else {
		if (currentChar == ':' && fileString[currentCharIndex + 1] == ':')
		{
			steps_in_source(2);
			token->type = DIALOG_TOKEN_SCOPE;
		} else if (currentChar == '(') {
			step_in_source();
			token->type = DIALOG_TOKEN_GROUPING_BEGIN;
		} else if (currentChar == ')') {
			step_in_source();
			token->type = DIALOG_TOKEN_GROUPING_END;
		} else if (currentChar == '&' && fileString[currentCharIndex + 1] == '&') {
			steps_in_source(2);
			token->type = DIALOG_TOKEN_AND;
		} else if (currentChar == '|' && fileString[currentCharIndex + 1] == '|') {
			steps_in_source(2);
			token->type = DIALOG_TOKEN_OR;
		} else if (currentChar == '=' && fileString[currentCharIndex + 1] == '=') {
				steps_in_source(2);
				token->type = DIALOG_TOKEN_EQUALS;
		} else if (currentChar == '!') {
			if (fileString[currentCharIndex + 1] == '=')
			{
				steps_in_source(2);
				token->type = DIALOG_TOKEN_DIFFERS;
			} else {
				step_in_source();
				token->type = DIALOG_TOKEN_NOT;
			}
		} else if (currentChar == '+') {
			step_in_source();
			token->type = DIALOG_TOKEN_ADD;
		} else if (currentChar == '-') {
			step_in_source();
			token->type = DIALOG_TOKEN_SUBTRACT;
		} else if (currentChar == '*') {
			step_in_source();
			token->type = DIALOG_TOKEN_MULTIPLY;
		} else if (currentChar == '/') {
			step_in_source();
			token->type = DIALOG_TOKEN_DIVIDE;
		} else if (currentChar == '<') {
			if (fileString[currentCharIndex + 1] == '=')
			{
				steps_in_source(2);
				token->type = DIALOG_TOKEN_INFERIOR_EQUALS;
			} else {
				step_in_source();
				token->type = DIALOG_TOKEN_INFERIOR;
			}
		} else if (currentChar == '>') {
			if (fileString[currentCharIndex + 1] == '=')
			{
				steps_in_source(2);
				token->type = DIALOG_TOKEN_SUPERIOR_EQUALS;
			} else {
				step_in_source();
				token->type = DIALOG_TOKEN_SUPERIOR;
			}
		} else if (currentChar == '"') {
			buf(char) string = get_string();
			token->type = DIALOG_TOKEN_STRING;
			token->string = string;
		} else if (isdigit(currentChar)) {
			char *begin = fileString + currentCharIndex;
			char *end = begin;
			bool foundDot = false;
			do
			{
				if (fileString[currentCharIndex] == '.')
				{
					foundDot = true;
				}
				step_in_source();
				end++;
			} while (isdigit(fileString[currentCharIndex]) || (fileString[currentCharIndex] == '.' && !foundDot));
			token->type = DIALOG_TOKEN_NUMERIC;
			token->numeric = strtod(begin, &end);
		} else if (isalnum(currentChar) || currentChar == '_') {
			buf(char) identifier = get_identifier();
			if (strmatch(identifier, "true"))
			{
				token->type = DIALOG_TOKEN_NUMERIC;
				token->numeric = 1;
				buf_free(identifier);
			} else if (strmatch(identifier, "false")) {
				token->type = DIALOG_TOKEN_NUMERIC;
				token->numeric = 0;
				buf_free(identifier);
			} else if (strmatch(identifier, "full-left")) {
				token->type = DIALOG_TOKEN_POSITION_IDENTIFIER;
				token->numeric = 0;
				buf_free(identifier);
			} else if (strmatch(identifier, "left")) {
				token->type = DIALOG_TOKEN_POSITION_IDENTIFIER;
				token->numeric = 1;
				buf_free(identifier);
			} else if (strmatch(identifier, "center-left")) {
				token->type = DIALOG_TOKEN_POSITION_IDENTIFIER;
				token->numeric = 2;
				buf_free(identifier);
			} else if (strmatch(identifier, "center")) {
				token->type = DIALOG_TOKEN_POSITION_IDENTIFIER;
				token->numeric = 3;
				buf_free(identifier);
			} else if (strmatch(identifier, "center-right")) {
				token->type = DIALOG_TOKEN_POSITION_IDENTIFIER;
				token->numeric = 4;
				buf_free(identifier);
			} else if (strmatch(identifier, "right")) {
				token->type = DIALOG_TOKEN_POSITION_IDENTIFIER;
				token->numeric = 5;
				buf_free(identifier);
			} else if (strmatch(identifier, "full-right")) {
				token->type = DIALOG_TOKEN_POSITION_IDENTIFIER;
				token->numeric = 6;
				buf_free(identifier);
			} else {
				token->type = DIALOG_TOKEN_IDENTIFIER;
				token->string = identifier;
			}
		} else {
			error("in %s at line %d unexpected char %c found.", filePath, currentLine, currentChar);
		}
	}
	return token;
}

buf(DialogToken *) lex_dialog(const char *_filePath)
{
	currentLine = 1;
	currentIndentationLevel = 0;
	currentCharIndex = 0;

	currentLexMode = LEX_MODE_TEXT;
	filePath = _filePath;
	fileString = file_to_string(filePath);
	dialogTokens = NULL;

	DialogToken *token = get_next_dialog_token();

	while (token->type != DIALOG_TOKEN_END_OF_FILE)
	{
		buf_add(dialogTokens, token);
		token = get_next_dialog_token();
	}
	buf_add(dialogTokens, token);

	xfree(fileString);

	buf_free(multilineCommentsLines);
	multilineCommentsLines = NULL;

	return dialogTokens;
}

static AnimationToken *get_next_animation_token()
{
	AnimationToken *token = xmalloc(sizeof (*token));

	while (isspace(fileString[currentCharIndex]))
	{
		if (fileString[currentCharIndex] == '\n')
		{
			currentLine++;
			currentIndentationLevel = 0;
		} else if (fileString[currentCharIndex] == '\t') {
			if (buf_len(animationTokens) == 0 || animationTokens[buf_len(animationTokens) - 1]->line != currentLine)
			{
				currentIndentationLevel++;
			} else {
				error("in %s at line %d tab character is reserved for indentation, its use in the middle of a line is forbidden.", filePath, currentLine);
			}
		}
		step_in_source();
	}

	token->line = currentLine;
	token->indentationLevel = currentIndentationLevel;

	char currentChar = fileString[currentCharIndex];

	if (currentChar == '\0')
	{
		token->type = ANIMATION_TOKEN_END_OF_FILE;
	} else if (currentChar == '"') {
		buf(char) string = get_string();
		token->type = ANIMATION_TOKEN_STRING;
		token->string = string;
	} else if (isdigit(currentChar)) {
		char *begin = fileString + currentCharIndex;
		char *end = begin;
		bool foundDot = false;
		do
		{
			if (fileString[currentCharIndex] == '.')
			{
				foundDot = true;
			}
			step_in_source();
			end++;
		} while (isdigit(fileString[currentCharIndex]) || (fileString[currentCharIndex] == '.' && !foundDot));
		token->type = ANIMATION_TOKEN_NUMERIC;
		token->numeric = strtod(begin, &end);
	} else if (isalnum(currentChar) || currentChar == '_') {
		buf(char) identifier = get_identifier();
		token->type = ANIMATION_TOKEN_IDENTIFIER;
		token->string = identifier;
	} else {
		error("in %s at line %d unexpected char %c found.", filePath, currentLine, currentChar);
	}
	return token;
}

buf(AnimationToken *) lex_animations(const char *_filePath)
{
	currentLine = 1;
	currentIndentationLevel = 0;
	currentCharIndex = 0;

	filePath = _filePath;
	fileString = file_to_string(filePath);
	animationTokens = NULL;

	AnimationToken *token = get_next_animation_token();

	while (token->type != ANIMATION_TOKEN_END_OF_FILE)
	{
		buf_add(animationTokens, token);
		token = get_next_animation_token();
	}
	buf_add(animationTokens, token);

	xfree(fileString);

	return animationTokens;
}