#include <stdbool.h>

#include "maths.h"
#include "user_input.h"
#include "dialog.h"

extern char *windowName;
extern ivec2 windowDimensions;
extern mat4 projection;
extern float deltaTime;
extern ivec2 mousePosition;
extern char **variablesNames;
extern double *variablesValues;
extern Dialog *interpretingDialog;
extern char *interpretingDialogName;
extern char *nextDialogName;
extern bool dialogChanged;

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
void strcopy(char **destination, char *source);
void strappend(char **destination, char *suffix);
bool strmatch(char *a, char *b);
