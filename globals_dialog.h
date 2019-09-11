#ifndef GLOBALS_DIALOG_H
#define GLOBALS_DIALOG_H

extern Dialog *interpretingDialog;
extern buf(char) interpretingDialogName;
extern buf(char) nextDialogName;
extern bool dialogChanged;

extern buf(buf(char)) variablesNames;
extern buf(Variable *) variablesValues;

#endif /* end of include guard: GLOBALS_DIALOG_H */