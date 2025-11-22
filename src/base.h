#ifndef BASE_H
#define BASE_H

#include <stdlib.h>

#define MEOW() printf("MEOW\n")

#define global   static
#define local    static
#define function static

#define TYPED_CALLOC(NMEMB, TYPE) \
    (TYPE *) calloc((NMEMB), sizeof(TYPE));

#define FREE(ptr)     \
    free((ptr));      \
    (ptr) = nullptr;

#define DIE()  \
    *(int *) 0;

#define VERIFY(cond, code) \
    if (!(cond)) {code};

#define POSASSERT(cond) \
    VERIFY(cond, ERROR_MSG("possasert killed program, the cond (" #cond ") was false"); DIE();)

#endif // BASE_H