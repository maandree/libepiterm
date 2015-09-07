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
#ifndef LIBEPITERM_LOOP_H
#define LIBEPITERM_LOOP_H


#include "hypoterm.h"
#include "pty.h"

#include <stddef.h>



/**
 * Tagged union for `libepiterm_hypoterm_t`
 * and `libepiterm_pty_t`
 */
typedef union libepiterm_term
{
  /**
   * 1 if the item is a `libepiterm_hypoterm_t`,
   * 0 if the item is a `libepiterm_pty_t`
   */
  int is_hypo;
  
  /**
   * Hypoterminal type
   */
  libepiterm_hypoterm_t hypo;
  
  /**
   * Epiterminal type
   */
  libepiterm_pty_t epi;
  
} libepiterm_term_t;


/**
 * A rather standard loop, that is not interrupted by signals,
 * for epiterminals
 * 
 * This funcion will set up signal handlers for `SIGWINCH` and `SIGCHLD`
 * 
 * @param   terms           List of all hypo- and epiterminals, when an epiterminal is reaped,
 *                          it will be unlisted, and all following elements will be shifted
 *                          to remove the gap, the new gap at the end will be set to `NULL`
 * @param   termn           The number of terminals in `terms`
 * @param   io_callback     This function is called on I/O-events, the arguments are:
 *                          the terminal whence input is read, a buffer of received input,
 *                          the number of bytes of the received input, output parameter for
 *                          the file descriptor of the terminal to which data shall be written
 *                          (-1 if none), output parameter for a buffer (that will not be freed)
 *                          with the data to write to the the other terminal, output parameter
 *                          for the number of bytes to write to the other terminal, the function
 *                          shall follow the return semantics of this function
 * @param   winch_callback  This function is called with the hypoterminal changes size,
 *                          it shall follow the return semantics of this function
 * @param   wait_callback   This function is called when an epiterminal is closed,
 *                          the first argument will the information, from `terms`, on the
 *                          epiterminal that died, *or* was stopped or continued, the second
 *                          argument is the status that `waitpid` returned, the function shall
 *                          follow the return semantics of this function
 * @return                  Zero on success, -1 on error, on error `errno` is set to describe to error
 */
int libepiterm_loop(libepiterm_term_t** restrict terms, size_t termn,
		    int (*io_callback)(libepiterm_term_t* restrict read_term, char* read_buffer,
				       size_t read_size, int* restrict write_fd,
				       char** restrict write_buffer, size_t* restrict write_size),
		    int (*winch_callback)(void),
		    int (*wait_callback)(libepiterm_pty_t* restrict pty, int status));


#endif

