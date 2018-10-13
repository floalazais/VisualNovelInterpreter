#include <stdlib.h>
#include <stddef.h>

#include "stretchy_buffer.h"
#include "xalloc.h"
#include "variable.h"

void free_variable(Variable *variable)
{
	if (variable->type == VARIABLE_STRING)
	{
		buf_free(variable->string);
	}
	xfree(variable);
}