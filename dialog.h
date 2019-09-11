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
			union {double numeric; buf(char) string;};
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

Variable *resolve_logic_expression(LogicExpression *logicExpression);

typedef struct GoTo
{
	buf(char) dialogFile;
	buf(char) knotToGo;
} GoTo;

typedef struct Assignment
{
	buf(char) identifier;
	LogicExpression *logicExpression;
} Assignment;

extern const char *argumentTypeDescriptions[];

typedef enum ArgumentType
{
	ARGUMENT_STRING,
	ARGUMENT_IDENTIFIER,
	ARGUMENT_NUMERIC
} ArgumentType;

typedef struct Argument
{
	ArgumentType type;
	union {buf(char) string; double numeric;};
} Argument;

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

	COMMAND_HIDE_UI,

	COMMAND_WAIT,

	COMMAND_SET_WINDOW_NAME,

	COMMAND_SET_SPEAKER_NAME_COLOR,

	NB_COMMANDS
} CommandType;

typedef struct Command
{
	CommandType type;
	Argument **arguments;
} Command;

typedef struct Sentence
{
	buf(char) string;
	bool autoSkip;
} Sentence;

typedef struct Choice
{
	Sentence *sentence;
	GoTo *goToCommand;
} Choice;

typedef struct CueExpression CueExpression;

typedef struct CueCondition
{
	LogicExpression *logicExpression;
	bool resolved;
	bool result;
	buf(CueExpression *) cueExpressionsIf;
	buf(CueExpression *) cueExpressionsElse;
	int currentExpression;
} CueCondition;

typedef enum CueExpressionType
{
	CUE_EXPRESSION_SENTENCE,
	CUE_EXPRESSION_CHOICE,
	CUE_EXPRESSION_COMMAND,
	CUE_EXPRESSION_GO_TO,
	CUE_EXPRESSION_ASSIGNMENT,
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
		GoTo *goTo;
		Assignment *assignment;
		CueCondition *cueCondition;
	};
} CueExpression;

typedef struct Cue
{
	buf(char) characterName;
	int characterNamePosition;
	buf(CueExpression *) cueExpressions;
	int currentExpression;
	bool setCharacterCommandInDeclaration;
} Cue;

typedef struct KnotExpression KnotExpression;

typedef struct KnotCondition
{
	LogicExpression *logicExpression;
	bool resolved;
	bool result;
	buf(KnotExpression *) knotExpressionsIf;
	buf(KnotExpression *) knotExpressionsElse;
	int currentExpression;
} KnotCondition;

typedef enum KnotExpressionType
{
	KNOT_EXPRESSION_CUE,
	KNOT_EXPRESSION_COMMAND,
	KNOT_EXPRESSION_GO_TO,
	KNOT_EXPRESSION_ASSIGNMENT,
	KNOT_EXPRESSION_KNOT_CONDITION
} KnotExpressionType;

typedef struct KnotExpression
{
	KnotExpressionType type;
	union
	{
		Cue *cue;
		Command *command;
		GoTo *goTo;
		Assignment *assignment;
		KnotCondition *knotCondition;
	};
} KnotExpression;

typedef struct Knot
{
	buf(char) name;
	buf(KnotExpression *) knotExpressions;
	int currentExpression;
} Knot;

typedef struct Dialog
{
	buf(buf(char)) backgroundPacksNames;
	buf(buf(Animation *)) backgroundPacks;
	buf(buf(char)) charactersNames;
	buf(buf(Animation *)) charactersAnimations;
	buf(buf(char)) coloredNames;
	buf(vec3) namesColors;
	buf(buf(char)) soundsNames;
	buf(buf(char)) musicsNames;
	buf(Knot *) knots;
	int currentKnot;
	bool end;
} Dialog;

Dialog *get_dialog_from_file(const char *_filePath);
void free_dialog(Dialog *dialog);

Variable *get_variable(const char *variableName);

#endif /* end of include guard: DIALOG_H */
