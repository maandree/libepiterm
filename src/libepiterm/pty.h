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



/**
 * Pseudoterminal information
 */
typedef struct libepiterm_pty
{
  /**
   * Will be set to 0
   */
  int is_hypo;
  
  /**
   * File descriptor of the master device
   * of the pseudoterminal, the device the
   * creator of the pseudoterminal use
   */
  int master;
  
  /**
   * You may set this to anything you like,
   * be aware, the will be set to `NULL` by
   * `libepiterm_pty_create`, so it must be
   * set after calling `libepiterm_pty_create`
   */
  void* user_data;
  
  /* Order of the above is important. */
  
  /**
   * File descriptor of the slave device
   * of the pseudoterminal, the device the
   * shell on the pseudoterminal use
   */
  int slave;
  
  /**
   * The process ID of the child process
   * that was created, in which the shell
   * will be running
   */
  pid_t pid;
  
  /**
   * The pathname of the created pseudoterminal
   */
  char* tty;
  
} libepiterm_pty_t;



/**
 * Create a pseudoterminal and fork–exec a shell on it
 * 
 * @param   pty              Output parameter for information on the created pseudoterminal
 * @param   use_path         Whether `shell` shall be resolved with $PATH unless it contains a slash
 * @param   shell            The shell to spawn
 * @param   argv             `NULL`-terminated list of arguments for the shell, including `shell` itself,
 *                           or a substitute, as the first argument; `NULL` to only pass `shell`,
 *                           that is, execute by calling something like `execv(shell, (char*[]){shell, NULL})`
 * @param   envp             `NULL`-terminated list of all environment variables the shell shall be spawned
 *                           with, `NULL` to inherit the list from the spawner
 * @param   get_record_name  Callback function used to get the name of the login session, `NULL`
 *                           if a record in utmp shall not be created, the function will be passed
 *                           `pty`, which will be filled in, and the function shall follow the return
 *                           semantics of this function
 * @param   termios          Settings for the pseudoterminal, `NULL` to inherit from stdin
 * @param   winsize          Size for the pseudoterminal, `NULL` to inherit from stdin
 * @return                   Zero on success, -1 on error, on error `errno` is set to describe to error
 */
int libepiterm_pty_create(libepiterm_pty_t* restrict pty, int use_path, const char* shell, char* const argv[],
			  char *const envp[], char* (*get_record_name)(libepiterm_pty_t* pty),
			  struct termios* restrict termios, struct winsize* restrict winsize);

/**
 * Close a pseudoterminal
 * 
 * @param  pty  Information on the pseudoterminal to close
 */
void libepiterm_pty_close(libepiterm_pty_t* restrict pty);


#endif


