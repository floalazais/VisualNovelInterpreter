#ifndef DIALOG_H
#define DIALOG_H

#include "animated_sprite.h"

typedef enum LogicExpressionType
{
	LOGIC_EXPRESSION_LITERAL,
	LOGIC_EXPRESSION_UNARY,
	LOGIC_EXPRESSION_BINARY,
	LOGIC_EXPRESSION_GROUPING,
} LogicExpressionType;

typedef enum LogicExpressionLiteralType
{
	LOGIC_EXPRESSION_LITERAL_NUMERIC,
	LOGIC_EXPRESSION_LITERAL_IDENTIFIER,
	LOGIC_EXPRESSION_LITERAL_STRING,
} LogicExpressionLiteralType;

typedef enum LogicExpressionUnaryType
{
	LOGIC_EXPRESSION_UNARY_NEGATION,
} LogicExpressionUnaryType;

typedef enum LogicExpressionBinaryOperation
{
	LOGIC_EXPRESSION_BINARY_ADD,
	LOGIC_EXPRESSION_BINARY_SUBTRACT,
	LOGIC_EXPRESSION_BINARY_MULTIPLY,
	LOGIC_EXPRESSION_BINARY_DIVISE,
	LOGIC_EXPRESSION_BINARY_AND,
	LOGIC_EXPRESSION_BINARY_OR,
	LOGIC_EXPRESSION_BINARY_SUPERIOR,
	LOGIC_EXPRESSION_BINARY_SUPERIOR_EQUALS,
	LOGIC_EXPRESSION_BINARY_INFERIOR,
	LOGIC_EXPRESSION_BINARY_INFERIOR_EQUALS,
	LOGIC_EXPRESSION_BINARY_EQUALS,
	LOGIC_EXPRESSION_BINARY_DIFFERS,
} LogicExpressionBinaryOperation;

typedef struct LogicExpression
{
	LogicExpressionType type;
	union
	{
		struct
		{
			LogicExpressionLiteralType type;
			union {double numeric; char *text;};
		} literal;

		struct
		{
			LogicExpressionUnaryType type;
			struct LogicExpression *expression;
		} unary;

		struct
		{
			struct LogicExpression *left;
			LogicExpressionBinaryOperation operation;
			struct LogicExpression *right;
		} binary;

		struct
		{
			struct LogicExpression *expression;
		} grouping;
	};
} LogicExpression;

typedef enum CommandType
{
    COMMAND_SET_BACKGROUND,
    COMMAND_CLEAR_BACKGROUND,

    COMMAND_SET_CHARACTER,
    COMMAND_CLEAR_CHARACTER_POSITION,
    COMMAND_CLEAR_CHARACTER_POSITIONS,

    COMMAND_PAUSE,
    COMMAND_END,

    COMMAND_ASSIGN,

    COMMAND_GO_TO
} CommandType;

typedef enum ArgumentType
{
    PARAMETER_STRING,
    PARAMETER_IDENTIFIER,
    PARAMETER_NUMERIC,
	PARAMETER_LOGIC_EXPRESSION
} ArgumentType;

typedef struct Argument
{
    ArgumentType type;
    union {char *text; double numeric; LogicExpression *logicExpression;};
} Argument;

typedef struct Command
{
    CommandType type;
    Argument *arguments;
} Command;

typedef struct Choice
{
    char *sentence;
	char *knotToGo;
} Choice;

struct CueExpression;

typedef struct CueCondition
{
	LogicExpression *logicExpression;
	struct CueExpression *cueExpressionsIf;
	struct CueExpression *cueExpressionsElse;
} CueCondition;

typedef enum CueExpressionType
{
    CUE_EXPRESSION_SENTENCE,
    CUE_EXPRESSION_CHOICE,
    CUE_EXPRESSION_COMMAND,
    CUE_EXPRESSION_CUE_CONDITION
} CueExpressionType;

typedef struct CueExpression
{
    CueExpressionType type;
    union
    {
        char *sentence;
        Choice choice;
        Command command;
        CueCondition cueCondition;
    };
} CueExpression;

typedef struct Cue
{
    char *characterName;
	int characterNamePosition;
    CueExpression *cueExpressions;
} Cue;

struct KnotExpression;

typedef struct KnotCondition
{
    LogicExpression *logicExpression;
    struct KnotExpression *knotExpressionsIf;
    struct KnotExpression *knotExpressionsElse;
} KnotCondition;

typedef enum KnotExpressionType
{
    KNOT_EXPRESSION_CUE,
    KNOT_EXPRESSION_COMMAND,
    KNOT_EXPRESSION_KNOT_CONDITION
} KnotExpressionType;

typedef struct KnotExpression
{
    KnotExpressionType type;
    union
    {
        Cue cue;
        Command command;
        KnotCondition knotCondition;
    };
} KnotExpression;

typedef struct Knot
{
	char *name;
    KnotExpression *knotExpressions;
} Knot;

typedef struct Dialog
{
    char **backgroundPacksNames;
    struct AnimatedSprite *backgroundPacks;
    int currentBackgroundPack;
	int currentBackgroundName;
    char **charactersNames;
    struct AnimatedSprite *charactersSprites;
    int visibleCharacters[7];
    Knot *knots;
    int currentKnot;
} Dialog;

extern int nbArguments[];

void free_logic_expression(LogicExpression *logicExpression);

void free_argument(Argument argument);

void free_command(Command command);

void free_choice(Choice choice);

void free_cue_condition(CueCondition cueCondition);

void free_cue_expression(CueExpression cueExpression);

void free_cue(Cue cue);

void free_knot_condition(KnotCondition knotCondition);

void free_knot_expression(KnotExpression knotExpression);

void free_knot(Knot knot);

void free_dialog(Dialog dialog);

#endif /* end of include guard: DIALOG_H */
