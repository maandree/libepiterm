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
#include "loop.h"
#include "macros.h"

#include <alloca.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>



/**
 * Has the hypoterminal changed size?
 */
static volatile sig_atomic_t winched = 0;

/**
 * Has a child died?
 */
static volatile sig_atomic_t chlded = 0;



/**
 * Called when the hypoterminal change size
 * 
 * @param  signo  Will always be `SIGWINCH`
 */
static void sigwinch(int signo)
{
  winched = 1;
  (void) signo;
}


/**
 * Called when a child dies
 * 
 * @param  signo  Will always be `SIGCHLD`
 */
static void sigchld(int signo)
{
  chlded = 1;
  (void) signo;
}


/**
 * Fully write a buffer to a file, and continue even if interrupted by a signal
 * 
 * @param   fd      The file descriptor of the file to write
 * @param   buffer  The buffer with the data to write
 * @param   size    The number of bytes to write from the beginning of the buffer to the file
 * @return          Zero on success, -1 on error, on error `errno` is set to describe to error
 */
static ssize_t uninterrupted_write(int fd, void* buffer, size_t size)
{
  ssize_t wrote;
  size_t ptr = 0;
  
  while (ptr < size)
    {
      wrote = write(fd, ((char*)buffer) + ptr, size - ptr);
      if (wrote < 0)
	{
	  if (errno == EINTR)
	    continue;
	  return -1;
	}
      ptr += (size_t)wrote;
    }
  
  return (ssize_t)ptr;
}



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
		    int (*wait_callback)(libepiterm_pty_t* restrict pty, int status))
{
  size_t i, length, epin = 0;
  int fd, events_max, epoll_fd = -1, status;
  struct epoll_event* events;
  struct sigaction action;
  libepiterm_term_t* event;
  char buffer[1024];
  char* output;
  ssize_t n, got;
  sigset_t sigset;
  pid_t pid;
  int saved_errno;
  
  /* epoll cannot poll more than `INT_MAX` events. */
  events_max = termn > (size_t)INT_MAX ? INT_MAX : (int)termn;
  events = alloca((size_t)events_max * sizeof(struct epoll_event));
  
  /* Set up epoll for I/O events. */
  memset(events, 0, sizeof(struct epoll_event));
  events->events = EPOLLIN;
  try (epoll_fd = epoll_create1(0));
  for (i = 0; i < termn; i++)
    {
      try (events->data.ptr = terms[i], epoll_ctl(epoll_fd, EPOLL_CTL_ADD, terms[i]->epi.master, events));
      epin += (size_t)(1 - terms[i]->is_hypo);
    }
  
  /* Set up signal handler for detecting when the hypoterminal is resized. */
  sigemptyset(&sigset);
  action.sa_handler = sigwinch;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  fail_if (sigaction(SIGWINCH, &action, NULL));
  
  /* Set up signal handler for detecting when a child process dies, is stopped or is continue. */
  sigemptyset(&sigset);
  action.sa_handler = sigchld;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  fail_if (sigaction(SIGCHLD, &action, NULL));
  
  /* The loop. */
  while (epin)
    {
      /* Has the hypoterminal changed size. */
      if (winched)
	{
	  winched = 0;
	  try (winch_callback());
	}
      
      /* Has a child dies, been stopped or been continued. */
      if (chlded)
	for (chlded = 0, i = 0; i < termn; i++)
	  if (terms[i]->is_hypo == 0)
	    {
	      try (pid = (pid_t)TEMP_FAILURE_RETRY(waitpid(terms[i]->epi.pid, &status,
							   WNOHANG | WUNTRACED | WCONTINUED)));
	      if (pid == 0)
		continue;
	      fail_if (pid != terms[i]->epi.pid);
	      try (wait_callback(&(terms[i]->epi), status));
	      if (WIFSTOPPED(status) || WIFCONTINUED(status))
		continue;
	      memmove(terms + i, terms + i + 1, --termn - i), i--, epin--;
	      terms[termn] = NULL;
	      if (epin == 0)
		goto done;
	    }
      
      /* Wait for input. */
      n = (ssize_t)epoll_wait(epoll_fd, events, events_max, -1);
      if ((n < 0) && (errno == EINTR))
	continue;
      fail_if (n < 0);
      
      /* Translate and transmit input. */
      while (n--)
	{
	  event = events[n].data.ptr;
	  try (got = TEMP_FAILURE_RETRY(read(event->hypo.in, buffer, sizeof(buffer))));
	  if (got == 0)
	    continue;
	  try (io_callback(event, buffer, (size_t)got, &fd, &output, &length));
	  if (fd >= 0)
	    try (uninterrupted_write(fd, output, length));
	}
    }
  
 done:
  close(epoll_fd);
  return 0;
 fail:
  saved_errno = errno;
  if (epoll_fd >= 0)
    close(epoll_fd);
  return errno = saved_errno, -1;
}

