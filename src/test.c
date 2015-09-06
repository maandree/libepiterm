#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#include "macros.h"
#include "shell.h"
#include "hypoterm.h"
#include "pty.h"



static volatile sig_atomic_t winched = 0;
static volatile sig_atomic_t chlded = 0;


static void sigwinch()
{
  winched = 1;
}

static void sigchld()
{
  chlded = 1;
}


static int copy_winsize(int whither, int whence)
{
  struct winsize winsize;
  try (TEMP_FAILURE_RETRY(ioctl(whence,  TIOCGWINSZ, &winsize)));
  try (TEMP_FAILURE_RETRY(ioctl(whither, TIOCSWINSZ, &winsize)));
  return 0;
 fail:
  return -1;
}


int main(void)
{
  int have_pty = 0, have_hypo = 0;
  libepiterm_pty_t pty;
  libepiterm_hypoterm_t hypo;
  char* free_this = NULL;
  const char* shell;
  sigset_t sigset;
  char buffer[1024];
  ssize_t got;
  struct sigaction action;
  int epoll_fd = -1;
  struct epoll_event events[2];
  int n; /* [sic] */
  void* event;
  
  fail_if (LIBEPITERM_INITIALISE(&hypo));
  have_hypo = 1;
  
  fail_unless (shell = libepiterm_get_shell(&free_this));
  fail_if (libepiterm_pty_create(&pty, 0, shell, NULL, NULL, NULL, &(hypo.saved_termios), NULL));
  free(free_this), free_this = NULL;
  have_pty = 1;
  
  memset(events, 0, sizeof(struct epoll_event));
  events->events = EPOLLIN;
  try (epoll_fd = epoll_create1(0));
  try (events->data.ptr = &hypo, epoll_ctl(epoll_fd, EPOLL_CTL_ADD, hypo.in,    events));
  try (events->data.ptr = &pty,  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pty.master, events));
  
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
  
  while (!chlded)
    {
      if (winched)
	{
	  winched = 0;
	  try (copy_winsize(pty.master, hypo.in));
	}
      
      try (n = epoll_wait(epoll_fd, events, 2, -1));
      while (n--)
	{
	  event = events[n].data.ptr;
	  if (event == &hypo)
	    {
	      fail_if (got = read(hypo.in, buffer, sizeof(buffer)), got <= 0);
	      fail_if (write(pty.master, buffer, (size_t)got) < 0);
	    }
	  else
	    {
	      fail_if (got = read(((libepiterm_pty_t*)event)->master, buffer, sizeof(buffer)), got <= 0);
	      fail_if (write(hypo.out, buffer, (size_t)got) < 0);
	    }
	}
    }
  
  close(epoll_fd);
  libepiterm_pty_close(&pty);
  libepiterm_restore(&hypo);
  return 0;
  
 fail:
  fprintf(stderr, "%s: %s\r\n", caller, strerror(errno));
  free(free_this);
  if (epoll_fd >= 0)  close(epoll_fd);
  if (have_pty)       libepiterm_pty_close(&pty);
  if (have_hypo)      libepiterm_restore(&hypo);
  return 1;
}

