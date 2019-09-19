#ifndef STR_H
#define STR_H

buf(char) strclone(const char *source);
buf(char) strclonen(const char *source, size_t lastIndex);
buf(char) strcopy(buf(char) *destination, const char *source);
buf(char) strcopyn(buf(char) *destination, const char *source, size_t lastIndex);
buf(char) strappend(buf(char) *destination, const char *suffix);
buf(char) strnappend(buf(char) *destination, int appendListLength, ...);
buf(char) strmerge(const char *prefix, const char *suffix);
bool strmatch(const char *a, const char *b);
buf(int) utf8_decode(const char *string);
buf(unsigned short) codepoint_to_utf16(const int *codepoints);

#endif /* end of include guard: STR_H */