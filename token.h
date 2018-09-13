#ifndef TOKEN_H
#define TOKEN_H

typedef enum DialogTokenType
{
    TOKEN_END_OF_FILE,

    TOKEN_IF,
    TOKEN_ELSE,
	TOKEN_COMMAND,
	TOKEN_KNOT,

    TOKEN_SENTENCE,
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
	TOKEN_NUMERIC,

	TOKEN_OPEN_PARENTHESIS,
	TOKEN_CLOSE_PARENTHESIS,
	TOKEN_SCOPE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_EQUALS,
    TOKEN_DIFFERS,
	TOKEN_PLUS,
    TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_SLASH,
    TOKEN_INFERIOR,
    TOKEN_INFERIOR_EQUALS,
    TOKEN_SUPERIOR,
    TOKEN_SUPERIOR_EQUALS
} DialogTokenType;

typedef struct Token
{
    DialogTokenType type;
    int line;
    int indentationLevel;
    union {char *text; double numeric;};
} Token;

extern const char *tokenStrings[];

void print_token(Token *token);

void free_token(Token *token);

#endif /* end of include guard: TOKEN_H */
