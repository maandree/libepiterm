#ifndef LIBEPITERM_HYPOTERM_H
#define LIBEPITERM_HYPOTERM_H

#include <termios.h>

typedef struct libepiterm_hypoterm
{
  int in;
  int out;
  struct termios saved_termios;
  
} libepiterm_hypoterm_t;

int libepiterm_intialise(libepiterm_hypoterm_t* restrict hypoterm, int hypoin, int hypoout);
int libepiterm_restore(libepiterm_hypoterm_t* restrict hypoterm);

#endif


