#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
  int r;
  sigset_t sigset;
  fd_set fds;
  char buffer[1024];
  ssize_t got;
  struct sigaction action;
  
  fail_if (LIBEPITERM_INITIALISE(&hypo));
  have_hypo = 1;
  
  fail_unless (shell = libepiterm_get_shell(&free_this));
  fail_if (libepiterm_pty_create(&pty, 0, shell, NULL, NULL, NULL, &(hypo.saved_termios), NULL));
  free(free_this), free_this = NULL;
  have_pty = 1;
  
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
      
      FD_ZERO(&fds);
      FD_SET(hypo.in, &fds);
      FD_SET(pty.master, &fds);
      
      try (r = select(pty.master + 1, &fds, NULL, NULL, NULL));
      
      if (FD_ISSET(pty.master, &fds))
	{
	  fail_if (got = read(pty.master, buffer, sizeof(buffer)), got <= 0);
	  fail_if (write(hypo.out, buffer, (size_t)got) < 0);
	}
      if (FD_ISSET(hypo.in, &fds))
	{
	  fail_if (got = read(hypo.in, buffer, sizeof(buffer)), got <= 0);
	  fail_if (write(pty.master, buffer, (size_t)got) < 0);
	}
    }
  
  libepiterm_pty_close(&pty);
  libepiterm_restore(&hypo);
  return 0;
  
 fail:
  fprintf(stderr, "%s\r\n", strerror(errno));
  free(free_this);
  if (have_pty)   libepiterm_pty_close(&pty);
  if (have_hypo)  libepiterm_restore(&hypo);
  return 1;
}

