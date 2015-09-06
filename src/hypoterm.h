#ifndef LIBEPITERM_HYPOTERM_H
#define LIBEPITERM_HYPOTERM_H

#include <termios.h>

typedef struct libepiterm_hypoterm
{
  void* user_data;
  int in;
  int out;
  struct termios saved_termios;
  
} libepiterm_hypoterm_t;

int libepiterm_initialise(libepiterm_hypoterm_t* restrict hypoterm, int hypoin, int hypoout);
int libepiterm_restore(libepiterm_hypoterm_t* restrict hypoterm);

#define LIBEPITERM_INITIALISE(hypoterm)  libepiterm_initialise(hypoterm, 0, 1)

#endif


