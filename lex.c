#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "lex.h"
#include "xalloc.h"
#include "stretchy_buffer.h"
#include "globals.h"

typedef enum LexMode
{
	LEX_MODE_TEXT,
	LEX_MODE_CODE
} LexMode;

static char *fileString;
static Token *tokens;

static int currentLine = 1;
static int currentIndentationLevel = 0;
static int currentChar = 0;

static LexMode currentLexMode = LEX_MODE_TEXT;

static void step_in_source()
{
    if (fileString[currentChar++] == '\0')
    {
        error("stepped after end of file.");
    }
}

static void steps_in_source(unsigned nb)
{
    while (nb--)
    {
        step_in_source();
    }
}

static Token get_next_token()
{
    Token token;

    while (isspace(fileString[currentChar]))
    {
        if (fileString[currentChar] == '\n')
        {
            currentLexMode = LEX_MODE_TEXT;
            currentLine++;
            currentIndentationLevel = 0;
        } else if (fileString[currentChar] == '\t') {
			if (currentLexMode == LEX_MODE_TEXT)
			{
				currentIndentationLevel++;
			} else {
				error("tab character is reserved for indentation, its use in the middle of line %d is forbidden.", currentLine);
			}
        }
        step_in_source();
    }

	token.line = currentLine;
	token.indentationLevel = currentIndentationLevel;

    if (fileString[currentChar] == '\0')
    {
        token.type = TOKEN_END_OF_FILE;
    } else if (fileString[currentChar] == ':' && fileString[currentChar + 1] == ':') {
		steps_in_source(2);
        token.type = TOKEN_SCOPE;
	} else if (fileString[currentChar] == '(') {
        token.type = TOKEN_OPEN_PARENTHESIS;
	} else if (fileString[currentChar] == ')') {
        token.type = TOKEN_CLOSE_PARENTHESIS;
	} else if (fileString[currentChar] == '+') {
        token.type = TOKEN_PLUS;
	} else if (fileString[currentChar] == '*') {
        token.type = TOKEN_STAR;
	} else if (fileString[currentChar] == '/') {
        token.type = TOKEN_SLASH;
	} else if (fileString[currentChar] == '-') {
        if (fileString[currentChar + 1] == '>')
        {
            steps_in_source(2);
            token.type = TOKEN_COMMAND;
			token.text = NULL;
			buf_add(token.text, 'G');
			buf_add(token.text, 'O');
			buf_add(token.text, '_');
			buf_add(token.text, 'T');
			buf_add(token.text, 'O');
			buf_add(token.text, '\0');
        } else {
            step_in_source();
            token.type = TOKEN_MINUS;
        }
    } else if (fileString[currentChar] == '>') {
		if (currentLexMode == LEX_MODE_TEXT)
		{
        	currentLexMode = LEX_MODE_CODE;
		}
        if (fileString[currentChar + 1] == '=')
        {
            steps_in_source(2);
            token.type = TOKEN_SUPERIOR_EQUALS;
        } else {
            step_in_source();
            token.type = TOKEN_SUPERIOR;
        }
    } else if (fileString[currentChar] == '<') {
        if (fileString[currentChar + 1] == '=')
        {
            steps_in_source(2);
            token.type = TOKEN_INFERIOR_EQUALS;
        } else {
            step_in_source();
            token.type = TOKEN_INFERIOR;
        }
    } else if (fileString[currentChar] == '=') {
        step_in_source();
        token.type = TOKEN_EQUALS;
    } else if (fileString[currentChar] == '!') {
        if (fileString[currentChar + 1] == '=')
        {
            steps_in_source(2);
            token.type = TOKEN_DIFFERS;
        } else {
            step_in_source();
            token.type = TOKEN_NOT;
        }
    } else if (fileString[currentChar] == '&' && fileString[currentChar + 1] == '&') {
        steps_in_source(2);
        token.type = TOKEN_AND;
    } else if (fileString[currentChar] == '|' && fileString[currentChar + 1] == '|') {
        steps_in_source(2);
        token.type = TOKEN_OR;
    } else if (fileString[currentChar] == '"') {
        step_in_source();
        char *string = NULL;
        char charToAdd;
        while (fileString[currentChar] != '"')
        {
            if (fileString[currentChar] == '\\')
            {
                if (fileString[currentChar + 1] == '"')
                {
                    charToAdd = '"';
                    steps_in_source(2);
                } else {
                    charToAdd = '\\';
                    step_in_source();
                }
            }
            if (fileString[currentChar] == '\0')
            {
                error("unclosed string at line %d.", currentLine);
            } else if (fileString[currentChar] == '\n') {
                error("unclosed string at line %d.", currentLine);
            } else {
                charToAdd = fileString[currentChar];
                step_in_source();
            }
			buf_add(string, charToAdd);
        }
        step_in_source(); // Avoid finishing " character
		buf_add(string, '\0');
        token.type = TOKEN_STRING;
        token.text = string;
	} else if (fileString[currentChar] == '@') {
		step_in_source();
		char *knot = NULL;
		while (fileString[currentChar] != '\n' && fileString[currentChar] != '\0')
		{
			buf_add(knot, fileString[currentChar]);
			step_in_source();
		}
		buf_add(knot, '\0');
		token.type = TOKEN_KNOT;
		token.text = knot;
    } else if (fileString[currentChar] == '#') {
        step_in_source();
		if (currentLexMode == LEX_MODE_TEXT)
		{
        	currentLexMode = LEX_MODE_CODE;
		}
        char *command = NULL;
        while (isalnum(fileString[currentChar]) || fileString[currentChar] == '_')
		{
            buf_add(command, fileString[currentChar]);
            step_in_source();
        }
        buf_add(command, '\0');
		if (!strcmp(command, "IF"))
		{
			token.type = TOKEN_IF;
			buf_free(command);
		} else if (!strcmp(command, "ELSE")) {
			token.type = TOKEN_ELSE;
			buf_free(command);
		} else {
	        token.type = TOKEN_COMMAND;
	        token.text = command;
		}
    } else if (isdigit(fileString[currentChar] && currentLexMode == LEX_MODE_CODE)) {
        char *begin = fileString + currentChar;
        char *end = begin;
        do {
            step_in_source();
            end++;
        } while (isdigit(fileString[currentChar]));
        token.type = TOKEN_NUMERIC;
        token.numeric = strtod(begin, &end);
    } else if (fileString[currentChar] == '\\') {
        if (currentLexMode == LEX_MODE_CODE)
        {
            error("stray \\ at line %d.", currentLine);
        }
        step_in_source();
        char *sentence = NULL;
        while (fileString[currentChar] != '\n' && fileString[currentChar] != '\0')
        {
            buf_add(sentence, fileString[currentChar]);
            step_in_source();
        }
        buf_add(sentence, '\0');
        token.type = TOKEN_SENTENCE;
        token.text = sentence;
    } else if (currentLexMode == LEX_MODE_TEXT) {
        char *sentence = NULL;
        while (fileString[currentChar] != '\n' && fileString[currentChar] != '\0')
        {
            buf_add(sentence, fileString[currentChar]);
            step_in_source();
        }
        buf_add(sentence, '\0');
        token.type = TOKEN_SENTENCE;
        token.text = sentence;
    } else if (isalnum(fileString[currentChar]) || fileString[currentChar] == '_') {
        char *identifier = NULL;
        do {
            buf_add(identifier, fileString[currentChar]);
            step_in_source();
        } while (isalnum(fileString[currentChar]) || fileString[currentChar] == '_' || fileString[currentChar] == '\'');
        buf_add(identifier, '\0');
        if (!strcmp(identifier, "true"))
        {
            token.type = TOKEN_NUMERIC;
            token.numeric = 1;
            buf_free(identifier);
        } else if (!strcmp(identifier, "false")) {
            token.type = TOKEN_NUMERIC;
            token.numeric = 0;
            buf_free(identifier);
        } else {
            token.type = TOKEN_IDENTIFIER;
            token.text = identifier;
        }
    } else {
		error("unexpected char %c at line %d", fileString[currentChar], currentLine);
	}
    return token;
}

Token *lex(char *_fileString)
{
	fileString = _fileString;
	tokens = NULL;

    Token token = get_next_token();

    while (token.type != TOKEN_END_OF_FILE)
    {
        buf_add(tokens, token);
        token = get_next_token();
    }
	buf_add(tokens, token);

	return tokens;
}
