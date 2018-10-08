#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "window.h"
#include "maths.h"
#include "user_input.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "stroperation.h"
#include "token.h"
#include "lex.h"
#include "interpret.h"
#include "graphics.h"
#include "dialog.h"
#include "globals.h"

Dialog *interpretingDialog = NULL;
char *interpretingDialogName = NULL;
char *nextDialogName = NULL;
bool dialogChanged = true;
char **variablesNames = NULL;
double *variablesValues = NULL;

int main()
{
	init_window();

	init_graphics();

	init_dialog_ui();
	strcopy(&interpretingDialogName, "Dialogs/start.dlg");
	Token **tokens = lex(interpretingDialogName);
	interpretingDialog = create_dialog(interpretingDialogName, tokens);

	update_window_name();

	init_window_clock();

	while (true)
	{
		if (!update_window())
		{
			break;
		}

		if (is_input_key_pressed(INPUT_KEY_R))
		{
			nextDialogName = interpretingDialogName;
		}

		if (nextDialogName)
		{
			free_dialog(interpretingDialog);
			tokens = lex(nextDialogName);
			interpretingDialog = create_dialog(nextDialogName, tokens);
			if (interpretingDialogName != nextDialogName)
			{
				buf_free(interpretingDialogName);
				interpretingDialogName = nextDialogName;
			}
			nextDialogName = NULL;
			dialogChanged = true;
		}
		if (!interpret_current_dialog())
		{
			ask_window_to_close();
		}

		draw_all();

		display_window();
	}
	free_dialog(interpretingDialog);
	free_dialog_ui();

	free_graphics();

	print_leaks();

	return shutdown_window();
}
