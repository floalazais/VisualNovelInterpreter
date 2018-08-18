#include <stdbool.h>

#include "maths.h"
#include "user_input.h"

extern ivec2 windowDimensions;
extern mat4 projection;
extern float deltaTime;
extern ivec2 mousePosition;

__attribute__((noreturn)) void error(char *string, ...);
char *file_to_string(char *filePath);
bool strmatch(char *a, char *b);
