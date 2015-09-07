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
#define _GNU_SOURCE
#include "overlay.h"
#include "macros.h"
#include "shell.h"
#include "loop.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>



/**
 * Wrapper around `ioctl` to handle the fact that
 * request constants have to wrong type and we are
 * compiling with tranditional convertion.
 */
#define ioctl(a, b, c)  ((ioctl)((a), (unsigned long)(b), (c)))



/**
 * The epiterminal
 */
static libepiterm_pty_t pty;

/**
 * The hypoterminal
 */
static libepiterm_hypoterm_t hypo;

/**
 * I/O callback function, as passed to `libepiterm_121`
 */
static int (*io_subcallback)(int from_epiterm, char* read_buffer, size_t read_size,
			     char** restrict write_buffer, size_t* restrict write_size);



/**
 * Callback function that is called when the hypoterminal changes size,
 * it will copy the hypoterminal's new size to the epiterminal
 * 
 * @return  Zero on success, -1 on error, on error `errno` is set to describe to error
 */
static int winch_callback(void)
{
  struct winsize winsize;
  try (TEMP_FAILURE_RETRY(ioctl(pty.master, TIOCGWINSZ, &winsize)));
  try (TEMP_FAILURE_RETRY(ioctl(hypo.in,    TIOCSWINSZ, &winsize)));
  return 0;
 fail:
  return -1;
}


/**
 * Callback function that is called when the epiterminal is terminated,
 * stopped or continued
 * 
 * @param   epiterm  Information on the epiterminal
 * @param   status   Status fetched by `waitpid`
 * @return           This function is always success and thus returns zero
 */
static int wait_callback(libepiterm_pty_t* restrict epiterm, int status)
{
  return 0;
  (void) epiterm, (void) status;
}


/**
 * Wrapper for the I/O function
 * 
 * @param   read_term     The terminal that has pending input
 * @param   read_buffer   Buffer with the input
 * @param   read_size     The number of bytes stored in `read_buffer`
 * @param   write_fd      Output parameter for the file descriptor of the output terminal
 * @param   write_buffer  Output parameter for the buffer the function fills with data to write
 * @param   write_size    Output parameter for the number of bytes to write
 * @return                Zero on success, -1 on error, on error `errno` is set to describe to error
 */
static int io_supercallback(libepiterm_term_t* restrict read_term, char* read_buffer, size_t read_size,
			    int* restrict write_fd, char** restrict write_buffer, size_t* restrict write_size)
{
  *write_fd = (int)(intptr_t)(read_term->hypo.user_data);
  return io_subcallback(read_term->is_hypo == 0, read_buffer, read_size, write_buffer, write_size);
}


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
				      char** restrict write_buffer, size_t* restrict write_size))
{
  int have_pty = 0, have_hypo = 0, saved_errno;
  void* terms[2];
  char* free_this = NULL;
  
  io_subcallback = io_callback;
  
  fail_if (LIBEPITERM_INITIALISE(&hypo));
  have_hypo = 1;
  
  fail_unless ((shell != NULL) || ((shell = libepiterm_get_shell(&free_this))));
  fail_if (libepiterm_pty_create(&pty, 0, shell, NULL, NULL, get_record_name, &(hypo.saved_termios), NULL));
  free(free_this), free_this = NULL;
  have_pty = 1;
  
  hypo.user_data = (void*)(intptr_t)(pty.master);
  pty.user_data = (void*)(intptr_t)(hypo.out);
  
  terms[0] = &hypo;
  terms[1] = &pty;
  try (libepiterm_loop((libepiterm_term_t**)terms, (size_t)2,
		       io_supercallback, winch_callback, wait_callback));
  
  libepiterm_pty_close(&pty);
  libepiterm_restore(&hypo);
  return 0;
  
 fail:
  saved_errno = errno;
  free(free_this);
  if (have_pty)   libepiterm_pty_close(&pty);
  if (have_hypo)  libepiterm_restore(&hypo);
  return errno = saved_errno, 1;
}

