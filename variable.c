#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "error.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "variable.h"

void print_variable(Variable *variable)
{
	if (variable->type == VARIABLE_NUMERIC)
	{
		printf(" : \"%f\"", variable->numeric);
	} else if (variable->type == VARIABLE_STRING) {
		printf(" : \"%s\"", variable->string);
	} else {
		error("unknown variable type %d.", variable->type);
	}
	printf("\n");
}

void free_variable(Variable *variable)
{
	if (variable->type == VARIABLE_STRING)
	{
		buf_free(variable->string);
	}
	xfree(variable);
}
