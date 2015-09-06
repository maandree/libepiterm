#ifndef LIBEPITERM_MACROS_H
#define LIBEPITERM_MACROS_H

#define fail_if(...)      do if ((__VA_ARGS__)) goto fail; while(0)
#define fail_unless(...)  fail_if (!(__VA_ARGS__))
#define try(...)          fail_if ((__VA_ARGS__) < 0)

#endif

