#ifndef WINDOW_H
#define WINDOW_H

typedef enum WindowMode
{
	WINDOW_MODE_BORDERLESS,
	WINDOW_MODE_FULLSCREEN,
	WINDOW_MODE_WINDOWED
} WindowMode;

void init_window(WindowMode windowMode, int width, int height);
bool update_window();
void swap_window_buffers();
void ask_window_to_close();
unsigned int get_window_shutdown_return_code();
void set_window_name(const char *windowName);
void set_window_vsync(bool sync);
void set_window_clear_color(float r, float g, float b, float a);
void set_window_mode(WindowMode windowMode);
void resize_window(int width, int height);

#endif /* end of include guard: WINDOW_H */
