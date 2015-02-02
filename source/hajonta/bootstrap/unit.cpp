#include <stdio.h>

#include "hajonta/math.cpp"

#define STRVALUE(x) #x
#define UNITTEST(x) bool result_##x = x(); if (!result_##x) return fail(STRVALUE(x), __FILE__, __LINE__);

void
fail(char *msg, char *file, int line)
{
    printf("%s (%d) : TEST FAILED: %s\n", file, line, msg);
}

void
main()
{
    UNITTEST(v2unittests);
    UNITTEST(v3unittests);
}
