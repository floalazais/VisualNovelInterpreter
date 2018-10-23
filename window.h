#ifndef WINDOW_H
#define WINDOW_H

void init_window();
void init_window_clock();
bool update_window();
bool is_window_active();
void swap_window_buffers();
void set_window_name(char *windowName);
void ask_window_to_close();
unsigned int shutdown_window();

#endif /* end of include guard: WINDOW_H */