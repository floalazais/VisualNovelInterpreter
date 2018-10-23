#ifndef ERROR_H
#define ERROR_H

#if __GNUC__
    #define NO_RETURN __attribute__((noreturn))
#elif _MSC_VER
    #define NO_RETURN __declspec(noreturn)
#else
    #error "Unsupported compiler"
	#define NO_RETURN
#endif

NO_RETURN void error(char *string, ...);
void warning(char *string, ...);

#endif /* end of include guard: ERROR_H */