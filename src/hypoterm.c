#define _GNU_SOURCE
#include "hypoterm.h"
#include "macros.h"

#include <unistd.h>
#include <errno.h>



int libepiterm_intialise(libepiterm_hypoterm_t* restrict hypoterm, int hypoin, int hypoout)
{
  struct termios termios;
  int sttyed = 0;
  
  hypoterm->in = hypoin;
  hypoterm->out = hypoout;
  
  fail_if (TEMP_FAILURE_RETRY(tcgetattr(hypoin, &termios)));
  hypoterm->saved_termios = termios;
  cfmakeraw(&termios);
  fail_if (TEMP_FAILURE_RETRY(tcsetattr(hypoin, TCSANOW, &termios)));
  sttyed = 1;
  
  return 0;
 fail:
  if (sttyed)
    TEMP_FAILURE_RETRY(tcsetattr(hypoin, TCSADRAIN, &(hypoterm->saved_termios)));
  return -1;
}


int libepiterm_restore(libepiterm_hypoterm_t* restrict hypoterm)
{
  fail_if (TEMP_FAILURE_RETRY(tcsetattr(hypoterm->in, TCSADRAIN, &(hypoterm->saved_termios))));
  
  return 0;
 fail:
  return -1;
}

