/**
 * Copyright © 2015  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE
#include "hypoterm.h"
#include "macros.h"

#include <unistd.h>
#include <errno.h>



int libepiterm_initialise(libepiterm_hypoterm_t* restrict hypoterm, int hypoin, int hypoout)
{
  struct termios termios;
  int sttyed = 0;
  
  hypoterm->is_hypo = 1;
  hypoterm->in = hypoin;
  hypoterm->out = hypoout;
  hypoterm->user_data = NULL;
  
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

