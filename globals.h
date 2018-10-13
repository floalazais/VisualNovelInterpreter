#ifndef GLOBALS_H
#define GLOBALS_H

extern char *windowName;
extern ivec2 windowDimensions;
extern mat4 projection;
extern float deltaTime;
extern ivec2 mousePosition;
extern char **variablesNames;
extern Variable **variablesValues;
extern Dialog *interpretingDialog;
extern char *interpretingDialogName;
extern char *nextDialogName;
extern bool dialogChanged;

#endif /* end of include guard: GLOBALS_H */