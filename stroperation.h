#ifndef STR_OPERATION_H
#define STR_OPERATION_H

char *strcopy(char *destination, char *source);
char *strappend(char *destination, char *suffix);
bool strmatch(char *a, char *b);
extern int utf8Offset;
int utf8_decode(const char *o);

#endif /* end of include guard: STR_OPERATION_H */
