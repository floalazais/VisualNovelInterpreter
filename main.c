#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "audio.h"
#include "window.h"
#include "maths.h"
#include "globals.h"
#include "user_input.h"
#include "stretchy_buffer.h"
#include "xalloc.h"
#include "stroperation.h"
#include "token.h"
#include "lex.h"
#include "interpret.h"
#include "graphics.h"
#include "variable.h"
#include "dialog.h"
#include "globals_dialog.h"

char *nextDialogName = NULL;
Dialog *interpretingDialog = NULL;
char *interpretingDialogName = NULL;
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
	init_window(WINDOW_MODE_WINDOWED, 800, 600);
	set_window_name("Visual Novel Interpreter Window");

	init_graphics();

	fpsDisplayText = create_text();
	set_text_font(fpsDisplayText, "Fonts/arial.ttf", TEXT_SIZE_SMALL);
	vec3 green = {0.0f, 1.0f, 0.0f};
	fpsDisplayText->color = green;
	fpsDisplayString = xmalloc(sizeof(char) * 8);

	fpsDisplayBox = create_sprite(SPRITE_COLOR);
	vec3 black = {0.0f, 0.0f, 0.0f};
	fpsDisplayBox->color = black;
	fpsDisplayBox->height = fpsDisplayText->height + 2;

	init_dialog_ui();
	interpretingDialogName = strcopy(interpretingDialogName, "Dialogs/start.dlg");

	Token **tokens = lex(interpretingDialogName);
	/*for (unsigned int i = 0; i < buf_len(tokens); i++)
	{
		print_token(tokens[i]);
	}*/

	interpretingDialog = create_dialog(interpretingDialogName, tokens);

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
			set_text_string(fpsDisplayText, fpsDisplayString);
			fpsDisplayBox->width = fpsDisplayText->width + 2;
			fpsNumber = 0;
		}

		if (is_input_key_pressed(INPUT_KEY_R))
		{
			buf_free(nextDialogName);
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
				interpretingDialogName = NULL;
				interpretingDialogName = strcopy(interpretingDialogName, nextDialogName);
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

		swap_window_buffers();
	}
	free_dialog(interpretingDialog);
	free_dialog_ui();

	printf("---Variables---\n");
	for (unsigned int i = 0; i < buf_len(variablesValues); i++)
	{
		printf("	-%s", variablesNames[i]);
		buf_free(variablesNames[i]);
		print_variable(variablesValues[i]);
		free_variable(variablesValues[i]);
	}
	printf("%d variables.\n\n", buf_len(variablesValues));
	buf_free(variablesNames);
	buf_free(variablesValues);

	xfree(fpsDisplayString);
	free_text(fpsDisplayText);
	free_sprite(fpsDisplayBox);

	buf_free(interpretingDialogName);
	buf_free(nextDialogName);
	free_graphics();

	free_audio();

	print_leaks();

	return get_window_shutdown_return_code();
}
