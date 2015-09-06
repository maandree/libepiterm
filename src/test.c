#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "macros.h"
#include "shell.h"
#include "loop.h"



static libepiterm_pty_t pty;
static libepiterm_hypoterm_t hypo;


static int copy_winsize(int whither, int whence)
{
  struct winsize winsize;
  try (TEMP_FAILURE_RETRY(ioctl(whence,  TIOCGWINSZ, &winsize)));
  try (TEMP_FAILURE_RETRY(ioctl(whither, TIOCSWINSZ, &winsize)));
  return 0;
 fail:
  return -1;
}


static int io_callback(libepiterm_term_t* restrict read_term, char* read_buffer, size_t read_size,
		       int* restrict write_fd, char** restrict write_buffer, size_t* restrict write_size)
{
  *write_fd = (int)(intptr_t)(read_term->hypo.user_data);
  *write_buffer = read_buffer;
  *write_size = read_size;
  return 0;
}


static int winch_callback(void)
{
  return copy_winsize(pty.master, hypo.in);
}


static int wait_callback(libepiterm_pty_t* restrict pty, int status)
{
  return 0;
  (void) pty, (void) status;
}


int main(void)
{
  int have_pty = 0, have_hypo = 0, saved_errno;
  libepiterm_term_t* terms[2];
  char* free_this = NULL;
  const char* shell;
  
  fail_if (LIBEPITERM_INITIALISE(&hypo));
  have_hypo = 1;
  
  fail_unless (shell = libepiterm_get_shell(&free_this));
  fail_if (libepiterm_pty_create(&pty, 0, shell, NULL, NULL, NULL, &(hypo.saved_termios), NULL));
  free(free_this), free_this = NULL;
  have_pty = 1;
  
  hypo.user_data = (void*)(intptr_t)(pty.master);
  pty.user_data = (void*)(intptr_t)(hypo.out);
  
  terms[0] = (libepiterm_term_t*)&hypo;
  terms[1] = (libepiterm_term_t*)&pty;
  try (libepiterm_loop(terms, 2, io_callback, winch_callback, wait_callback));
  
  libepiterm_pty_close(&pty);
  libepiterm_restore(&hypo);
  return 0;
  
 fail:
  saved_errno = errno;
  free(free_this);
  if (have_pty)       libepiterm_pty_close(&pty);
  if (have_hypo)      libepiterm_restore(&hypo);
  errno = saved_errno;
  perror("libepiterm");
  return 1;
}

