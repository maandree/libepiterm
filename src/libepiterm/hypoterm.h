#ifndef LIBEPITERM_HYPOTERM_H
#define LIBEPITERM_HYPOTERM_H

#include <termios.h>
#include <values.h>

typedef struct libepiterm_hypoterm
{
  int is_hypo;
  int in;
  void* user_data;
  /* Order of the above is important. */
  int out;
#if LONG_MAX != INT_MAX
  char __padding1[sizeof(long) + sizeof(int)];
#endif
  struct termios saved_termios;
  char __padding2[sizeof(long) - (sizeof(struct termios) % sizeof(long))];
  
} libepiterm_hypoterm_t;

int libepiterm_initialise(libepiterm_hypoterm_t* restrict hypoterm, int hypoin, int hypoout);
int libepiterm_restore(libepiterm_hypoterm_t* restrict hypoterm);

#define LIBEPITERM_INITIALISE(hypoterm)  libepiterm_initialise(hypoterm, 0, 1)

#endif


