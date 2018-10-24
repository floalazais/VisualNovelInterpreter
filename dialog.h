#ifndef DIALOG_H
#define DIALOG_H

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

typedef struct LogicExpression LogicExpression;

typedef struct LogicExpression
{
	VariableType returnType;
	LogicExpressionType type;
	union
	{
		struct
		{
			LogicExpressionLiteralType type;
			union {double numeric; char *string;};
		} *literal;

		struct
		{
			LogicExpressionUnaryType type;
			LogicExpression *expression;
		} *unary;

		struct
		{
			LogicExpression *left;
			LogicExpressionBinaryOperation operation;
			LogicExpression *right;
		} *binary;

		struct
		{
			LogicExpression *expression;
		} *grouping;
	};
} LogicExpression;

typedef enum CommandType
{
    COMMAND_SET_BACKGROUND,
    COMMAND_CLEAR_BACKGROUND,

    COMMAND_SET_CHARACTER,
    COMMAND_CLEAR_CHARACTER_POSITION,
    COMMAND_CLEAR_CHARACTER_POSITIONS,

	COMMAND_PLAY_MUSIC,
	COMMAND_STOP_MUSIC,
	COMMAND_SET_MUSIC_VOLUME,

	COMMAND_PLAY_SOUND,
	COMMAND_STOP_SOUND,
	COMMAND_SET_SOUND_VOLUME,

    COMMAND_END,

    COMMAND_ASSIGN,

    COMMAND_GO_TO,

	COMMAND_HIDE_UI,

	COMMAND_WAIT,

	COMMAND_SET_WINDOW_NAME,

	COMMAND_SET_NAME_COLOR
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
    union {char *string; double numeric; LogicExpression *logicExpression;};
} Argument;

typedef struct Command
{
    CommandType type;
    Argument **arguments;
} Command;

typedef struct Sentence
{
	char *string;
	bool autoSkip;
} Sentence;

typedef struct Choice
{
    Sentence *sentence;
	Command *goToCommand;
} Choice;

typedef struct CueExpression CueExpression;

typedef struct CueCondition
{
	LogicExpression *logicExpression;
	bool resolved;
	bool result;
	CueExpression **cueExpressionsIf;
	CueExpression **cueExpressionsElse;
	int currentExpression;
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
        Sentence *sentence;
        Choice *choice;
        Command *command;
        CueCondition *cueCondition;
    };
} CueExpression;

typedef struct Cue
{
    char *characterName;
	int characterNamePosition;
    CueExpression **cueExpressions;
	int currentExpression;
	bool setCharacterInDeclaration;
} Cue;

typedef struct KnotExpression KnotExpression;

typedef struct KnotCondition
{
    LogicExpression *logicExpression;
	bool resolved;
	bool result;
    KnotExpression **knotExpressionsIf;
    KnotExpression **knotExpressionsElse;
	int currentExpression;
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
        Cue *cue;
        Command *command;
        KnotCondition *knotCondition;
    };
} KnotExpression;

typedef struct Knot
{
	char *name;
    KnotExpression **knotExpressions;
	int currentExpression;
} Knot;

typedef struct Dialog
{
    char **backgroundPacksNames;
    Animation ***backgroundPacks;
    char **charactersNames;
    Animation ***charactersAnimations;
	char **coloredNames;
	vec3 *namesColors;
	char **soundsNames;
	Sound **sounds;
	char **musicsNames;
	Sound **musics;
    Knot **knots;
    int currentKnot;
	bool end;
} Dialog;

Variable *get_variable(char *variableName);
Variable *resolve_logic_expression(LogicExpression *logicExpression);
Dialog *create_dialog(char *_filePath, Token **_tokens);
void free_dialog(Dialog *dialog);
void free_command(Command *command);

#endif /* end of include guard: DIALOG_H */
