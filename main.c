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
static Sprite *fpsDisplayBox;
static char *fpsDisplayString;
static int fpsNumber = 0;

int main(int argc, char** argv)
{
	init_window();

	init_graphics();

	fpsDisplayText = create_text();
	set_font_to_text(fpsDisplayText, "Fonts/arial.ttf", TEXT_SIZE_SMALL);
	vec3 green = {0.0f, 1.0f, 0.0f};
	fpsDisplayText->color = green;
	fpsDisplayString = xmalloc(sizeof(char) * 8);

	fpsDisplayBox = create_sprite(SPRITE_COLOR);
	vec3 black = {0.0f, 0.0f, 0.0f};
	fpsDisplayBox->color = black;
	fpsDisplayBox->height = fpsDisplayText->height + 2;

	init_dialog_ui();
	strcopy(&interpretingDialogName, "Dialogs/start.dlg");

	Token **tokens = lex(interpretingDialogName);
	/*for (unsigned int i = 0; i < buf_len(tokens); i++)
	{
		print_token(tokens[i]);
	}*/

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
			if (fpsNumber >= 1000)
			{
				fpsNumber = 999;
			}
			sprintf(fpsDisplayString, "%d FPS", fpsNumber);
			set_string_to_text(fpsDisplayText, fpsDisplayString);
			fpsDisplayBox->width = fpsDisplayText->width + 2;
			fpsNumber = 0;
		}

		if (is_input_key_pressed(INPUT_KEY_R))
		{
			nextDialogName = interpretingDialogName;
		}

		if (nextDialogName)
		{
			free_dialog(interpretingDialog);
			//print_leaks();
			tokens = lex(nextDialogName);
			interpretingDialog = create_dialog(nextDialogName, tokens);
			if (interpretingDialogName != nextDialogName)
			{
				buf_free(interpretingDialogName);
				interpretingDialogName = NULL;
				strcopy(&interpretingDialogName, nextDialogName);
				buf_free(nextDialogName);
			}
			nextDialogName = NULL;
			dialogChanged = true;
		}
		if (!interpret_current_dialog())
		{
			ask_window_to_close();
		}

		add_sprite_to_draw_list(fpsDisplayBox, DRAW_LAYER_UI);
		add_text_to_draw_list(fpsDisplayText, DRAW_LAYER_UI);

		draw_all();

		display_window();
	}
	free_dialog(interpretingDialog);
	free_dialog_ui();

	for (unsigned int i = 0; i < buf_len(variablesValues); i++)
	{
		printf("%s", variablesNames[i]);
		buf_free(variablesNames[i]);
		print_variable(variablesValues[i]);
		free_variable(variablesValues[i]);
	}
	buf_free(variablesNames);
	buf_free(variablesValues);

	xfree(fpsDisplayString);
	free_text(fpsDisplayText);
	free_sprite(fpsDisplayBox);

	free_graphics();

	print_leaks();

	return shutdown_window();
}
