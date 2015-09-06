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
#ifndef LIBEPITERM_MACROS_H
#define LIBEPITERM_MACROS_H


/* This macro is used internally, it is not installed. */



/**
 * Go to `fail` if the given expression evaluates to non-zero
 * 
 * @param  ...  The expression, may contain the sequence operator
 */
#define fail_if(...)      do if ((__VA_ARGS__)) goto fail; while(0)

/**
 * Go to `fail` if the given expression evaluates to zero
 * 
 * @param  ...  The expression, may contain the sequence operator
 */
#define fail_unless(...)  fail_if (!(__VA_ARGS__))

/**
 * Go to `fail` if the given expression evaluates to a negative value
 * 
 * @param  ...  The expression, may contain the sequence operator
 */
#define try(...)          fail_if ((__VA_ARGS__) < 0);



#endif

