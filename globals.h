#include <stdbool.h>

#include "maths.h"
#include "user_input.h"

extern ivec2 windowDimensions;
extern mat4 projection;
extern float deltaTime;
extern ivec2 mousePosition;
extern bool gameEnd;
extern char **variablesNames;
extern double *variablesValues;
extern char *nextDialog;

#if __GNUC__
    #define NO_RETURN __attribute__((noreturn))
#elif _MSC_VER
    #define NO_RETURN __declspec(noreturn)
#else
    #error "Unsupported compiler"
	#define NO_RETURN
#endif

NO_RETURN void error(char *string, ...);
char *file_to_string(char *filePath);
bool strmatch(char *a, char *b);
