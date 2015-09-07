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
#ifndef LIBEPITERM_HYPOTERM_H
#define LIBEPITERM_HYPOTERM_H


#include <termios.h>
#include <values.h>



/**
 * Wrapper for `libepiterm_initialise`
 * that can be used if the terminal is
 * running on stdin and stdout
 * 
 * It is idential to `libepiterm_initialise',
 * except it only has its first argument,
 * and the other two are fill in automatically
 * with `STDIN_FILENO` and `STDOUT_FILENO`
 * (except the value of the macros are used
 * rather than the macros)
 */
#define LIBEPITERM_INITIALISE(hypoterm)  ((libepiterm_initialise)(hypoterm, 0, 1))


/**
 * Information on a hypoterminal,
 * a terminal upon which as epiterminal
 * (another terminal) is created
 */
typedef struct libepiterm_hypoterm
{
  /**
   * Will be set to 1
   */
  int is_hypo;
  
  /**
   * The terminals input pipe,
   * most likely `STDIN_FILNO`
   */
  int in;
  
  /**
   * You may set this to anything you like,
   * be aware, the will be set to `NULL` by
   * `libepiterm_initialise`, so it must be
   * set after calling `libepiterm_initialise`
   */
  void* user_data;
  
  /* Order of the above is important. */
  
  /**
   * The terminals output pipe,
   * most likely `STDOUT_FILNO`
   */
  int out;
  
#if LONG_MAX != INT_MAX
  char __padding1[sizeof(long) - sizeof(int)];
#endif
  
  /**
   * The settings the terminal had when
   * `libepiterm_initialise` was called
   */
  struct termios saved_termios;
  
  char __padding2[sizeof(long) - (sizeof(struct termios) % sizeof(long))];
  
} libepiterm_hypoterm_t;



/**
 * Initialise a hypoterminal, this entails making it raw,
 * and storing its file descriptors
 * 
 * @param   hypoterm  Output parameter for information on the hypoterminal
 * @param   hypoin    The input file descriptor of the hypoterminal, this is probably `STDIN_FILENO`
 * @param   hypoout   The output file descriptor of the hypoterminal, this is probably `STDOUT_FILENO`
 * @return            Zero on success, -1 on error, on error `errno` is set to describe to error
 */
int libepiterm_initialise(libepiterm_hypoterm_t* restrict hypoterm, int hypoin, int hypoout);

/**
 * Restore the original settings of the terminal
 * 
 * @param   hypoterm  Information on the hypoterminal
 * @return            Zero on success, -1 on error, on error `errno` is set to describe to error
 */
int libepiterm_restore(libepiterm_hypoterm_t* restrict hypoterm);



#endif

