#define _GNU_SOURCE
#include "overlay.h"
#include "macros.h"
#include "shell.h"
#include "loop.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


#define ioctl(a, b, c)  ((ioctl)((a), (unsigned long)(b), (c)))


static libepiterm_pty_t pty;
static libepiterm_hypoterm_t hypo;
static int (*io_subcallback)(int from_epiterm, char* read_buffer, size_t read_size,
			     char** restrict write_buffer, size_t* restrict write_size);

static int copy_winsize(int whither, int whence)
{
  struct winsize winsize;
  try (TEMP_FAILURE_RETRY(ioctl(whence,  TIOCGWINSZ, &winsize)));
  try (TEMP_FAILURE_RETRY(ioctl(whither, TIOCSWINSZ, &winsize)));
  return 0;
 fail:
  return -1;
}


static int winch_callback(void)
{
  return copy_winsize(pty.master, hypo.in);
}


static int wait_callback(libepiterm_pty_t* restrict epiterm, int status)
{
  return 0;
  (void) epiterm, (void) status;
}


static int io_supercallback(libepiterm_term_t* restrict read_term, char* read_buffer, size_t read_size,
			    int* restrict write_fd, char** restrict write_buffer, size_t* restrict write_size)
{
  *write_fd = (int)(intptr_t)(read_term->hypo.user_data);
  return io_subcallback(read_term->is_hypo == 0, read_buffer, read_size, write_buffer, write_size);
}


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

