#ifndef VARIABLE_H
#define VARIABLE_H

typedef enum VariableType
{
	VARIABLE_NUMERIC,
	VARIABLE_STRING
} VariableType;

typedef struct Variable
{
	VariableType type;
	char *string;
	double numeric;
} Variable;

void print_variable(Variable *variable);
void free_variable(Variable *variable);

#endif /* end of include guard: VARIABLE_H */