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
#include <libepiterm.h>
#include "libepiterm/macros.h"

#include <stdio.h>



static int io_callback(int from_epiterm, char* read_buffer, size_t read_size,
		       char** restrict write_buffer, size_t* restrict write_size)
{
  *write_buffer = read_buffer;
  *write_size = read_size;
  return 0;
  (void) from_epiterm;
}


int main(void)
{
  try (libepiterm_121(NULL, NULL, io_callback));
  return 0;
  
 fail:
  perror("libepiterm");
  return 1;
}

