#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include <stdio.h>

#include "window.h"
#include "maths.h"
#include "user_input.h"
#include "error.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "stroperation.h"
#include "token.h"
#include "lex.h"
#include "interpret.h"
#include "graphics.h"
#include "variable.h"
#include "dialog.h"
#include "globals.h"

Dialog *interpretingDialog = NULL;
char *interpretingDialogName = NULL;
char *nextDialogName = NULL;
bool dialogChanged = true;
char **variablesNames = NULL;
Variable **variablesValues = NULL;

static float timeDuringCurrentSecond = 0.0f;
static Text *fpsDisplayText;
static char *fpsDisplayString;
static int fpsNumber = 0;

int main(int argc, char** argv)
{
	init_window();

	init_graphics();

	fpsDisplayText = create_text();
	set_font_to_text(fpsDisplayText, "Fonts/arial.ttf", TEXT_SIZE_SMALL);
	vec3 white = {1.0f, 1.0f, 1.0f};
	fpsDisplayText->color = white;
	fpsDisplayString = xmalloc(sizeof(char) * 4);

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

		timeDuringCurrentSecond += deltaTime;
		fpsNumber++;
		if (timeDuringCurrentSecond >= 1.0f)
		{
			timeDuringCurrentSecond = 0.0f;
			sprintf(fpsDisplayString, "%d", fpsNumber);
			set_string_to_text(fpsDisplayText, fpsDisplayString);
			fpsNumber = 0;
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

		add_text_to_draw_list(fpsDisplayText, DRAW_LAYER_UI);

		draw_all();

		display_window();
	}
	free_dialog(interpretingDialog);
	free_dialog_ui();

	for (unsigned int i = 0; i < buf_len(variablesValues); i++)
	{
		printf("%s", variablesNames[i]);
		if (variablesValues[i]->type == VARIABLE_NUMERIC) {
			printf(" : \"%f\"\n", variablesValues[i]->numeric);
		} else if (variablesValues[i]->type == VARIABLE_STRING)
		{
			printf(" : \"%s\"", variablesValues[i]->string);
		} else {
			error("unknown variable type %d.", variablesValues[i]->type);
		}
		printf("\n");
		buf_free(variablesNames[i]);
		free_variable(variablesValues[i]);
	}

	xfree(fpsDisplayString);
	free_text(fpsDisplayText);

	free_graphics();

	print_leaks();

	return shutdown_window();
}
