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
#ifndef LIBEPITERM_OVERLAY_H
#define LIBEPITERM_OVERLAY_H


#include "pty.h"

#include <stddef.h>



/**
 * Create a 1-to-1 epiterminal, it is not possible create more than one
 * if this function is used
 * 
 * @param   shell            The pathname of the shell, `NULL` for automatic
 * @param   get_record_name  Callback function used to get the name of the login session, `NULL`
 *                           if a record in utmp shall not be created, the function will be passed
 *                           `pty`, which will be filled in, and the function shall follow the return
 *                           semantics of this function
 * @param   io_callback      This function is called on I/O-events, the arguments are:
 *                           whether the input comes from the epiterminal (otherwise it comes from
 *                           the hypoterminal,) a buffer of received input, the number of bytes of
 *                           the received input, output parameter for a buffer (that will not be
 *                           freed) with the data to write to the the other terminal, output parameter
 *                           for the number of bytes to write to the other terminal, the function
 *                           shall follow the return semantics of this function
 * @return                   Zero on success, -1 on error, on error `errno` is set to describe to error
 */
int libepiterm_121(const char* shell, char* (*get_record_name)(libepiterm_pty_t* pty),
		   int (*io_callback)(int from_epiterm, char* read_buffer, size_t read_size,
				      char** restrict write_buffer, size_t* restrict write_size));


#endif

