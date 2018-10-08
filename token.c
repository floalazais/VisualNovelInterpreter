#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "token.h"
#include "xalloc.h"
#include "stretchy_buffer.h"

const char *tokenStrings[] =
{
    [TOKEN_END_OF_FILE] = "end of file",

    [TOKEN_IF] = "if",
    [TOKEN_ELSE] = "else",
	[TOKEN_COMMAND] = "command",
	[TOKEN_KNOT] = "knot",

    [TOKEN_SENTENCE] = "sentence",
    [TOKEN_IDENTIFIER] = "identifier",
    [TOKEN_STRING] = "string",
	[TOKEN_NUMERIC] = "numeric",

	[TOKEN_OPEN_PARENTHESIS] = "open parenthesis",
	[TOKEN_CLOSE_PARENTHESIS] = "close parenthesis",
	[TOKEN_SCOPE] = "scope",
    [TOKEN_AND] = "and",
    [TOKEN_OR] = "or",
    [TOKEN_NOT] = "not",
    [TOKEN_EQUALS] = "equals",
    [TOKEN_DIFFERS] = "differs",
	[TOKEN_PLUS] = "plus",
    [TOKEN_MINUS] = "minus",
	[TOKEN_STAR] = "star",
	[TOKEN_SLASH] = "slash",
    [TOKEN_INFERIOR] = "inferior",
    [TOKEN_INFERIOR_EQUALS] = "inferior equals",
    [TOKEN_SUPERIOR] = "superior",
    [TOKEN_SUPERIOR_EQUALS] = "superior equals"
};

void print_token(Token *token)
{
	printf("%s", tokenStrings[token->type]);
	if (token->type == TOKEN_NUMERIC)
	{
		printf(" : \"%f\"", token->numeric);
	} else if (token->type == TOKEN_IDENTIFIER) {
		printf(" : \"%s\"", token->text);
	} else if (token->type == TOKEN_STRING) {
		printf(" : \"%s\"", token->text);
	} else if (token->type == TOKEN_SENTENCE) {
		printf(" : \"%s\"", token->text);
	} else if (token->type == TOKEN_COMMAND) {
		printf(" : \"%s\"", token->text);
	} else if (token->type == TOKEN_KNOT) {
		printf(" : \"%s\"", token->text);
	}
	printf("\n");
}

void free_token(Token *token)
{
	if (token->type == TOKEN_COMMAND)
	{
		buf_free(token->text);
	} else if (token->type == TOKEN_IDENTIFIER) {
		buf_free(token->text);
	} else if (token->type == TOKEN_STRING) {
		buf_free(token->text);
	} else if (token->type == TOKEN_SENTENCE) {
		buf_free(token->text);
	} else if (token->type == TOKEN_KNOT) {
		buf_free(token->text);
	}
	xfree(token);
}
