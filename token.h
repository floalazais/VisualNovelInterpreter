#ifndef DIALOG_TOKEN_H
#define DIALOG_TOKEN_H

typedef enum DialogTokenType
{
	DIALOG_TOKEN_END_OF_FILE,

	DIALOG_TOKEN_KNOT,

	DIALOG_TOKEN_SPEAKER,

	DIALOG_TOKEN_IF,
	DIALOG_TOKEN_ELSE,
	DIALOG_TOKEN_ASSIGN,
	DIALOG_TOKEN_COMMAND,
	DIALOG_TOKEN_GO_TO,

	DIALOG_TOKEN_SENTENCE,
	DIALOG_TOKEN_CHOICE,
	DIALOG_TOKEN_IDENTIFIER,
	DIALOG_TOKEN_POSITION_IDENTIFIER,
	DIALOG_TOKEN_STRING,
	DIALOG_TOKEN_NUMERIC,

	DIALOG_TOKEN_SCOPE,
	DIALOG_TOKEN_GROUPING_BEGIN,
	DIALOG_TOKEN_GROUPING_END,
	DIALOG_TOKEN_AND,
	DIALOG_TOKEN_OR,
	DIALOG_TOKEN_NOT,
	DIALOG_TOKEN_EQUALS,
	DIALOG_TOKEN_DIFFERS,
	DIALOG_TOKEN_ADD,
	DIALOG_TOKEN_SUBTRACT,
	DIALOG_TOKEN_MULTIPLY,
	DIALOG_TOKEN_DIVIDE,
	DIALOG_TOKEN_INFERIOR,
	DIALOG_TOKEN_INFERIOR_EQUALS,
	DIALOG_TOKEN_SUPERIOR,
	DIALOG_TOKEN_SUPERIOR_EQUALS
} DialogTokenType;

typedef struct DialogToken
{
	DialogTokenType type;
	int line;
	int indentationLevel;
	union {buf(char) string; double numeric;};
} DialogToken;

char *dialog_token_to_string(DialogToken *token);
void print_dialog_token(DialogToken *token);
void free_dialog_token(DialogToken *token);

extern const char *animationTokenDescriptions[];

typedef enum AnimationTokenType
{
	ANIMATION_TOKEN_END_OF_FILE,

	ANIMATION_TOKEN_IDENTIFIER,
	ANIMATION_TOKEN_STRING,
	ANIMATION_TOKEN_NUMERIC,
} AnimationTokenType;

typedef struct AnimationToken
{
	AnimationTokenType type;
	int line;
	int indentationLevel;
	union {buf(char) string; double numeric;};
} AnimationToken;

char *animation_token_to_string(AnimationToken *token);
void print_animation_token(AnimationToken *token);
void free_animation_token(AnimationToken *token);

#endif /* end of include guard: DIALOG_TOKEN_H */