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

int libepiterm_121(const char* shell, char* (*get_record_name)(libepiterm_pty_t* pty),
		   int (*io_callback)(int from_epiterm, char* read_buffer, size_t read_size,
				      char** restrict write_buffer, size_t* restrict write_size));

#endif

