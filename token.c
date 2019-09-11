#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "xalloc.h"
#include "stretchy_buffer.h"
#include "token.h"

const char *dialogTokenDescriptions[] =
{
	[DIALOG_TOKEN_END_OF_FILE] = "end of file",

	[DIALOG_TOKEN_KNOT] = "knot \"@\"",

	[DIALOG_TOKEN_SPEAKER] = "speaker \">\"",

	[DIALOG_TOKEN_IF] = "if \"#if\"",
	[DIALOG_TOKEN_ELSE] = "else \"#else\"",
	[DIALOG_TOKEN_ASSIGN] = "assign \"#assign\"",
	[DIALOG_TOKEN_COMMAND] = "command \"#\"",
	[DIALOG_TOKEN_GO_TO] = "go to \"->\"",

	[DIALOG_TOKEN_SENTENCE] = "sentence",
	[DIALOG_TOKEN_CHOICE] = "choice",
	[DIALOG_TOKEN_IDENTIFIER] = "identifier",
	[DIALOG_TOKEN_POSITION_IDENTIFIER] = "position identifier",
	[DIALOG_TOKEN_STRING] = "string",
	[DIALOG_TOKEN_NUMERIC] = "numeric",

	[DIALOG_TOKEN_SCOPE] = "scope \"::\"",
	[DIALOG_TOKEN_GROUPING_BEGIN] = "grouping begin \"(\"",
	[DIALOG_TOKEN_GROUPING_END] = "grouping end \")\"",
	[DIALOG_TOKEN_AND] = "and \"&&\"",
	[DIALOG_TOKEN_OR] = "or \"||\"",
	[DIALOG_TOKEN_NOT] = "not \"!\"",
	[DIALOG_TOKEN_EQUALS] = "equals \"==\"",
	[DIALOG_TOKEN_DIFFERS] = "differs \"!=\"",
	[DIALOG_TOKEN_ADD] = "add \"+\"",
	[DIALOG_TOKEN_SUBTRACT] = "subtract \"-\"",
	[DIALOG_TOKEN_MULTIPLY] = "multiply \"*\"",
	[DIALOG_TOKEN_DIVIDE] = "divide \"/\"",
	[DIALOG_TOKEN_INFERIOR] = "inferior \"<\"",
	[DIALOG_TOKEN_INFERIOR_EQUALS] = "inferior equals \"<=\"",
	[DIALOG_TOKEN_SUPERIOR] = "superior \">\"",
	[DIALOG_TOKEN_SUPERIOR_EQUALS] = "superior equals \">=\""
};

const char *positionIdentifierTokenDescriptions[] =
{
	"full-left",
	"left",
	"center-left",
	"center",
	"center-right",
	"right",
	"full-right"
};

char *dialog_token_to_string(DialogToken *token)
{
	char *result = xmalloc(sizeof (*result) * 1024);
	snprintf(result, 1024, "%s token", dialogTokenDescriptions[token->type]);
	int bufferLength = 1024 - strlen(dialogTokenDescriptions[token->type]) - 6;
	if (token->type == DIALOG_TOKEN_KNOT && token->string)
	{
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_SPEAKER && token->string) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_COMMAND && token->string) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_SENTENCE && token->string) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_CHOICE && token->string) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_IDENTIFIER && token->string) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_POSITION_IDENTIFIER && token->string) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", positionIdentifierTokenDescriptions[(int)token->numeric]);
	} else if (token->type == DIALOG_TOKEN_STRING && token->string) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_NUMERIC) {
		snprintf(result + strlen(dialogTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%f\"", token->numeric);
	}
	return result;
}

void print_dialog_token(DialogToken *token)
{
	printf("%s", dialogTokenDescriptions[token->type]);
	if (token->type == DIALOG_TOKEN_KNOT && token->string)
	{
		printf(" : \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_SPEAKER && token->string) {
		printf(" : \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_COMMAND && token->string) {
		printf(" : \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_SENTENCE && token->string) {
		printf(" : \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_CHOICE && token->string) {
		printf(" : \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_IDENTIFIER && token->string) {
		printf(" : \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_STRING && token->string) {
		printf(" : \"%s\"", token->string);
	} else if (token->type == DIALOG_TOKEN_NUMERIC) {
		printf(" : \"%f\"", token->numeric);
	}
	printf("\n");
}

void free_dialog_token(DialogToken *token)
{
	if (token->type == DIALOG_TOKEN_KNOT)
	{
		buf_free(token->string);
	} else if (token->type == DIALOG_TOKEN_SPEAKER) {
		buf_free(token->string);
	} else if (token->type == DIALOG_TOKEN_COMMAND) {
		buf_free(token->string);
	} else if (token->type == DIALOG_TOKEN_SENTENCE) {
		buf_free(token->string);
	} else if (token->type == DIALOG_TOKEN_CHOICE) {
		buf_free(token->string);
	} else if (token->type == DIALOG_TOKEN_IDENTIFIER) {
		buf_free(token->string);
	} else if (token->type == DIALOG_TOKEN_STRING) {
		buf_free(token->string);
	}
	xfree(token);
}

const char *animationTokenDescriptions[] =
{
	[ANIMATION_TOKEN_END_OF_FILE] = "end of file",

	[ANIMATION_TOKEN_IDENTIFIER] = "identifier",
	[ANIMATION_TOKEN_STRING] = "string",
	[ANIMATION_TOKEN_NUMERIC] = "numeric"
};

char *animation_token_to_string(AnimationToken *token)
{
	char *result = xmalloc(sizeof (*result) * 1024);
	snprintf(result, 1024, "%s token", animationTokenDescriptions[token->type]);
	int bufferLength = 1024 - strlen(animationTokenDescriptions[token->type]) - 6;
	if (token->type == ANIMATION_TOKEN_IDENTIFIER && token->string)
	{
		snprintf(result + strlen(animationTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == ANIMATION_TOKEN_STRING && token->string) {
		snprintf(result + strlen(animationTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%s\"", token->string);
	} else if (token->type == ANIMATION_TOKEN_NUMERIC) {
		snprintf(result + strlen(animationTokenDescriptions[token->type]) + 6, bufferLength, " of value \"%f\"", token->numeric);
	}
	return result;
}

void print_animation_token(AnimationToken *token)
{
	printf("%s", animationTokenDescriptions[token->type]);
	if (token->type == ANIMATION_TOKEN_IDENTIFIER && token->string)
	{
		printf(" : \"%s\"", token->string);
	} else if (token->type == ANIMATION_TOKEN_STRING && token->string) {
		printf(" : \"%s\"", token->string);
	} else if (token->type == ANIMATION_TOKEN_NUMERIC) {
		printf(" : \"%f\"", token->numeric);
	}
	printf("\n");
}

void free_animation_token(AnimationToken *token)
{
	if (token->type == ANIMATION_TOKEN_IDENTIFIER)
	{
		buf_free(token->string);
	} else if (token->type == ANIMATION_TOKEN_STRING) {
		buf_free(token->string);
	}
	xfree(token);
}