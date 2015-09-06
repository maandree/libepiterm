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



static volatile sig_atomic_t winched = 0;
static volatile sig_atomic_t chlded = 0;



static void sigwinch(int signo)
{
  winched = 1;
  (void) signo;
}


static void sigchld(int signo)
{
  chlded = 1;
  (void) signo;
}


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
  
  events_max = termn > (size_t)INT_MAX ? INT_MAX : (int)termn;
  events = alloca((size_t)events_max * sizeof(struct epoll_event));
  
  memset(events, 0, sizeof(struct epoll_event));
  events->events = EPOLLIN;
  try (epoll_fd = epoll_create1(0));
  for (i = 0; i < termn; i++)
    {
      try (events->data.ptr = terms[i], epoll_ctl(epoll_fd, EPOLL_CTL_ADD, terms[i]->epi.master, events));
      epin += (size_t)(1 - terms[i]->is_hypo);
    }
  
  sigemptyset(&sigset);
  action.sa_handler = sigwinch;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  fail_if (sigaction(SIGWINCH, &action, NULL));
  
  sigemptyset(&sigset);
  action.sa_handler = sigchld;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  fail_if (sigaction(SIGCHLD, &action, NULL));
  
  while (epin)
    {
      if (winched)
	{
	  winched = 0;
	  try (winch_callback());
	}
      if (chlded)
	for (chlded = 0, i = 0; i < termn; i++)
	  if (terms[i]->is_hypo == 0)
	    {
	      try (pid = (pid_t)TEMP_FAILURE_RETRY(waitpid(terms[i]->epi.pid, &status,
							   WNOHANG | WUNTRACED | WCONTINUED)));
	      if (pid == 0)  continue;
	      fail_if (pid != terms[i]->epi.pid);
	      try (wait_callback(&(terms[i]->epi), status));
	      memmove(terms + i, terms + i + 1, --termn - i), i--, epin--;
	      terms[termn] = NULL;
	      goto done;
	    }
      
      n = (ssize_t)epoll_wait(epoll_fd, events, events_max, -1);
      if ((n < 0) && (errno == EINTR))
	continue;
      fail_if (n < 0);
      
      while (n--)
	{
	  event = events[n].data.ptr;
	  try (got = TEMP_FAILURE_RETRY(read(event->hypo.in, buffer, sizeof(buffer))));
	  if (got == 0)
	    continue;
	  try (io_callback(event, buffer, (size_t)got, &fd, &output, &length));
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

