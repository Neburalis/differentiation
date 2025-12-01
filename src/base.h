#ifndef BASE_H
#define BASE_H

#include <stdlib.h>

#define MEOW() printf("MEOW\n")

#define $(x) printf("%s = %d", #x, (x))

#define global   static
#define local    static
#define function static

#define STR_(s) #s
#define STR(s)  STR_(s)
#define GLUE_(a, b) a##b
#define GLUE(a, b) GLUE_(a, b)

#define ARRAY_COUNT(a) (sizeof(a) / (sizeof(a[0])))

#define TYPED_CALLOC(NMEMB, TYPE) \
    (TYPE *) calloc((NMEMB), sizeof(TYPE));

#define FREE(ptr)     \
    free((ptr));      \
    (ptr) = nullptr;

#define DIE()  \
    abort();

#define VERIFY(cond, code) \
    if (!(cond)) {code};

#define POSASSERT(cond) \
    VERIFY(cond, ERROR_MSG("possasert killed program, the cond (" #cond ") was false"); DIE();)

#endif // BASE_H