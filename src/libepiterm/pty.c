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
#include "pty.h"
#include "macros.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <utempter.h>



/**
 * Wrapper around `ioctl` to handle the fact that
 * request constants have to wrong type and we are
 * compiling with tranditional convertion.
 */
#define ioctl(a, b, c)  ((ioctl)((a), (unsigned long)(b), (c)))



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
 * Fully read a file into a buffer file, and continue even if interrupted by a signal 
 * 
 * @param   fd      The file descriptor of the file to read
 * @param   buffer  The buffer to load with the data from the file
 * @param   size    The maximum number of bytes to write to the buffer
 * @return          Zero on success, -1 on error, on error `errno` is set to describe to error
 */
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
			  struct termios* restrict termios, struct winsize* restrict winsize)
{
  int r, n, saved_errno;
  char* new;
  char* utmp_record = NULL;
  size_t bufsize = 8;
  ssize_t got;
  int pipe_rw[2];
  struct termios termios_;
  struct winsize winsize_;
  
  pty->is_hypo = 0;
  pty->master = -1;
  pty->slave = -1;
  pty->tty = NULL;
  pty->pid = -1;
  pty->user_data = NULL;
  pty->utempted = 0;
  
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
  if (argv == NULL)
    argv = (char* const[]) { shell, NULL };
# pragma GCC diagnostic pop
  
  /* Send of channel for the child to send an errno code over in case of failure. */
  try (pipe2(pipe_rw, O_CLOEXEC));
  
  /* Create pseudoterminal. */
  try (pty->master = posix_openpt(O_RDWR | O_NOCTTY));
  fail_if (grantpt(pty->master));
  fail_if (unlockpt(pty->master));
  
  /* Configure pseudoterminal. */
  if (termios == NULL)
    {
      termios = &termios_;
      fail_if (tcgetattr(STDIN_FILENO, termios));
    }
  fail_if (TEMP_FAILURE_RETRY(tcsetattr(pty->master, TCSANOW, termios)));
  
  /* Get the pathname of the pseudoterminal. */
 retry_ptsname:
  fail_unless (new = realloc(pty->tty, bufsize <<= 1));
  pty->tty = new;
  if (ptsname_r(pty->master, pty->tty, bufsize))
    {
      if (errno == ERANGE)
	goto retry_ptsname;
      return -1;
    }
  
  /* Open the pseudoterminal's slave device. */
  try (pty->slave = open(pty->tty, O_RDWR | O_NOCTTY));
  
  /* Set the size of the pseudoterminal. */
  if (winsize == NULL)
    {
      winsize = &winsize_;
      try(TEMP_FAILURE_RETRY(ioctl(STDIN_FILENO, TIOCGWINSZ, winsize)));
    }
  try(TEMP_FAILURE_RETRY(ioctl(pty->slave, TIOCSWINSZ, winsize)));
  
  /* Fork. */
  try (pty->pid = fork());
  if (pty->pid == 0)
    goto child;
  
  /* The parent only needs the read-end of the channel for the errno code. */
  try (TEMP_FAILURE_RETRY(close(pipe_rw[1])));
  
  /* Record the login on utmp. */
  if ((utmp_record = (get_record_name == NULL ? NULL : get_record_name(pty))))
    {
      for (;;)
	{
	  r = utempter_add_record(pty->master, utmp_record);
	  if ((r == 0) && (errno == EINTR))
	    continue;
	  fail_unless (r);
	  pty->utempted = 1;
	  break;
	}
      free(utmp_record), utmp_record = NULL;
    }
  
  /* Fetch errno code, and close the channel. */
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
  
  /* Die if the parent does. */
  prctl(PR_SET_PDEATHSIG, SIGKILL);
  
  /* Close all file descriptors, except the pseudoterminals's slave
     device and the write end of the channel for the errno code. */
  for (r = 0, n = getdtablesize(); r < n; r++)
    if ((r != pty->slave) && (r != pipe_rw[1]))
      TEMP_FAILURE_RETRY(close(r));
  
  /* Promote to session leader and set controlling TTY. */
  try (setsid());
  try (TEMP_FAILURE_RETRY(ioctl(pty->slave, TIOCSCTTY, 0)));
  
  /* Set up stdin, stdout and stderr. */
  if (pty->slave != STDIN_FILENO)
    {
      try (dup2(pty->slave, STDIN_FILENO));
      try (TEMP_FAILURE_RETRY(close(pty->slave)));
    }
  try (dup2(STDIN_FILENO, STDOUT_FILENO));
  try (dup2(STDIN_FILENO, STDERR_FILENO));
  
  /* We do not have to die if the parent dies. */
  prctl(PR_SET_PDEATHSIG, 0);
  /* Exec shell. */
  if (envp == NULL)
    (use_path ? execvp : execv)(shell, argv);
  else
    (use_path ? execvpe : execve)(shell, argv, envp);
  
  /* The write-end of the channel for the errno close
     is closed automatically if exec is successful. */
  
#undef fail
 cfail:
  saved_errno = errno;
  uninterrupted_write(pipe_rw[1], &saved_errno, sizeof(int));
  TEMP_FAILURE_RETRY(close(pipe_rw[1]));
  exit(1);
}


/**
 * Close a pseudoterminal
 * 
 * @param  pty  Information on the pseudoterminal to close
 */
void libepiterm_pty_close(libepiterm_pty_t* restrict pty)
{
  if (pty->utempted)
    TEMP_FAILURE_RETRY(utempter_remove_record(pty->master)), pty->utempted = 0;
  TEMP_FAILURE_RETRY(close(pty->master)), pty->master = -1;
  TEMP_FAILURE_RETRY(close(pty->slave)),  pty->slave  = -1;
  free(pty->tty), pty->tty = NULL;
}

