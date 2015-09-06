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
#ifndef LIBEPITERM_SHELL_H
#define LIBEPITERM_SHELL_H


/**
 * Figure out what shell shall be started
 * 
 * @param   free_this  Ouput parameter for a pointer that
 *                     the caller shall free once the returned
 *                     string is no longer needed
 * @return             The shell, do not free this, it may
 *                     be statically allocated (in which case
 *                     `*free_this == NULL`), `NULL` is returned
 *                     on failure and `errno` is set to indicate
 *                     the error, additionally, on error
 *                     `*free_this` is guaranteed to be `NULL`
 */
const char* libepiterm_get_shell(char** restrict free_this);


#endif


