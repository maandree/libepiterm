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
#ifndef LIBEPITERM_PTY_H
#define LIBEPITERM_PTY_H

#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <limits.h>

typedef struct libepiterm_pty
{
  int is_hypo;
  int master;
  void* user_data;
  /* Order of the above is important. */
  int slave;
  pid_t pid;
  char* tty;
  
} libepiterm_pty_t;

int libepiterm_pty_create(libepiterm_pty_t* restrict pty, int use_path, const char* shell, char* const argv[],
			  char *const envp[], char* (*get_record_name)(libepiterm_pty_t* pty),
			  struct termios* restrict termios, struct winsize* restrict winsize);
void libepiterm_pty_close(libepiterm_pty_t* restrict pty);

#endif


