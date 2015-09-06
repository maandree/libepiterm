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

typedef union libepiterm_term
{
  int is_hypo;
  libepiterm_hypoterm_t hypo;
  libepiterm_pty_t epi;
  
} libepiterm_term_t;

int libepiterm_loop(libepiterm_term_t** restrict terms, size_t termn,
		    int (*io_callback)(libepiterm_term_t* restrict read_term, char* read_buffer,
				       size_t read_size, int* restrict write_fd,
				       char** restrict write_buffer, size_t* restrict write_size),
		    int (*winch_callback)(void),
		    int (*wait_callback)(libepiterm_pty_t* restrict pty, int status));

#endif

