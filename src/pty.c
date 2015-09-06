#define _GNU_SOURCE
#include "pty.h"
#include "macros.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <utempter.h> /* -lutempter */



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


static ssize_t uninterrupted_read(int fd, void* buffer, size_t size)
{
  ssize_t got;
  size_t ptr = 0;
  
  while (ptr < size)
    {
      got = read(fd, ((char*)buffer) + ptr, size - ptr);
      if (got < 0)
	{
	  if (errno == EINTR)
	    continue;
	  return -1;
	}
      else if (got == 0)
	break;
      ptr += (size_t)got;
    }
  
  return (ssize_t)ptr;
}


int libepiterm_pty_create(libepiterm_pty_t* restrict pty, int use_path, const char* shell, char* const argv[],
			  char *const envp[], char* (*get_record_name)(libepiterm_pty_t* pty),
			  struct termios* restrict termios, struct winsize* restrict winsize)
{
  int r, n, saved_errno;
  char* new;
  char* utmp_record;
  size_t bufsize = 8;
  ssize_t got;
  int pipe_rw[2];
  struct termios termios_;
  struct winsize winsize_;
  
  pty->master = -1;
  pty->slave = -1;
  pty->tty = NULL;
  pty->pid = -1;
  pty->user_data = NULL;
  
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  if (argv == NULL)
    argv = (char* const[]) { shell, NULL };
# pragma GCC diagnostic pop
  
  try (pipe(pipe_rw));
  
  try (pty->master = posix_openpt(O_RDWR | O_NOCTTY));
  fail_if (grantpt(pty->master));
  fail_if (unlockpt(pty->master));
  
  if (termios == NULL)
    {
      termios = &termios_;
      fail_if (tcgetattr(STDIN_FILENO, termios));
    }
  fail_if (TEMP_FAILURE_RETRY(tcsetattr(pty->master, TCSANOW, termios)));
  
 retry_ptsname:
  fail_unless (new = realloc(pty->tty, bufsize <<= 1));
  pty->tty = new;
  if (ptsname_r(pty->master, pty->tty, bufsize))
    {
      if (errno == ERANGE)
	goto retry_ptsname;
      return -1;
    }
  
  try (pty->slave = open(pty->tty, O_RDWR | O_NOCTTY));
  
  if (winsize == NULL)
    {
      winsize = &winsize_;
      try(TEMP_FAILURE_RETRY(ioctl(STDIN_FILENO, TIOCGWINSZ, winsize)));
    }
  try(TEMP_FAILURE_RETRY(ioctl(pty->slave, TIOCSWINSZ, winsize)));
  
  try (pty->pid = fork());
  if (pty->pid == 0)
    goto child;
  
  try (TEMP_FAILURE_RETRY(close(pipe_rw[1])));
  
  if ((utmp_record = (get_record_name == NULL ? NULL : get_record_name(pty))))
    {
      for (;;)
	{
	  r = utempter_add_record(pty->master, utmp_record);
	  if ((r == 0) && (errno == EINTR))
	    continue;
	  fail_unless (r);
	  break;
	}
      free(utmp_record), utmp_record = NULL;
    }
  
  try (got = uninterrupted_read(pipe_rw[0], &saved_errno, sizeof(int)));
  if (got && ((size_t)got < sizeof(int)))
    fail_if ((errno = EPIPE));
  fail_if (got && (errno = saved_errno));
  try (TEMP_FAILURE_RETRY(close(pipe_rw[0])));
  
  return 0;
 fail:
  saved_errno = errno;
  if (pty->master >= 0)  close(pty->master), pty->master = -1;
  if (pty->slave  >= 0)  close(pty->slave),  pty->slave  = -1;
  free(pty->tty), pty->tty = NULL;
  free(utmp_record), utmp_record = NULL;
  return errno = saved_errno, -1;
  
 child:
#define fail cfail
  
  prctl(PR_SET_PDEATHSIG, SIGKILL);
  
  for (r = 0, n = getdtablesize(); r < n; r++)
    if ((r != pty->slave) && (r != pipe_rw[1]))
      TEMP_FAILURE_RETRY(close(r));
  
  try (setsid());
  try (TEMP_FAILURE_RETRY(ioctl(pty->slave, TIOCSCTTY, 0)));
  
  if (pty->slave != STDIN_FILENO)
    {
      try (dup2(pty->slave, STDIN_FILENO));
      try (TEMP_FAILURE_RETRY(close(pty->slave)));
    }
  try (dup2(STDIN_FILENO, STDOUT_FILENO));
  try (dup2(STDIN_FILENO, STDERR_FILENO));
  
  TEMP_FAILURE_RETRY(close(pipe_rw[1]));
  prctl(PR_SET_PDEATHSIG, 0);
  if (envp == NULL)
    (use_path ? execvp : execv)(shell, argv);
  else
    (use_path ? execvpe : execve)(shell, argv, envp);
  
#undef fail
 cfail:
  saved_errno = errno;
  uninterrupted_write(pipe_rw[1], &saved_errno, sizeof(int));
  TEMP_FAILURE_RETRY(close(pipe_rw[1]));
  exit(1);
}


void libepiterm_pty_close(libepiterm_pty_t* restrict pty)
{
  TEMP_FAILURE_RETRY(close(pty->master)), pty->master = -1;
  TEMP_FAILURE_RETRY(close(pty->slave)),  pty->slave  = -1;
  free(pty->tty), pty->tty = NULL;
}

