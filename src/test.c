#define _XOPEN_SOURCE  700
#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/prctl.h>
#include <utempter.h> /* -lutempter */



#define t(...)  do if (called = #__VA_ARGS__, __VA_ARGS__) goto fail; while (0)
static const char* called = NULL;



typedef struct libepiterm_pty
{
  int master;
  int slave;
  char* tty;
  pid_t pid;
  
} libepiterm_pty_t;



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


static void copy_winsize(int whither, int whence)
{
  struct winsize winsize;
  ioctl(whence,  TIOCGWINSZ, &winsize);
  ioctl(whither, TIOCSWINSZ, &winsize);
}


#define fail_if(...)      do if ((__VA_ARGS__)) goto fail; while(0)
#define fail_unless(...)  fail_if (!(__VA_ARGS__))
#define try(...)          fail_if ((__VA_ARGS__) < 0)



char* libepiterm_get_shell(char** restrict free_this)
{
  struct passwd pw_;
  struct passwd* pw;
  char* shell = getenv("SHELL");
  char* new;
  int uid = getuid();
  size_t bufsize = 64;
  int saved_errno;
  
  *free_this = NULL;
  
  if ((shell == NULL) || (access(shell, X_OK) < 0))
    {
    retry:
      fail_unless (new = realloc(*free_this, bufsize));
      errno = getpwuid_r(uid, &pw_, *free_this = new, bufsize, &pw);
      switch (errno)
	{
	case ERANGE:  bufsize <<= 1;
	case EINTR:   goto retry;
	case 0:       break;
	default:      goto fail;
	}
      if (pw)
	shell = pw->pw_shell;
    }
  if (shell == NULL)
    shell = "/bin/sh", free(*free_this), *free_this = NULL;
  
  return shell;
 fail:
  saved_errno = errno;
  free(*free_this), *free_this = NULL;
  return errno = saved_errno, NULL;
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
  fail_if (tcsetattr(pty->master, TCSANOW, termios));
  
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
  fail_if ((errno = saved_errno));
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


int main(void)
{
  libepiterm_pty_t pty;
  int sttyed = 0, r;
  struct sigaction action;
  sigset_t sigset;
  const char* shell;
  struct termios termios;
  struct termios saved_termios;
  fd_set fds;
  char buffer[1024];
  ssize_t got;
  char* free_this = NULL;
  
  shell = libepiterm_get_shell(&free_this);
  t (libepiterm_pty_create(&pty, 0, shell, NULL, NULL, NULL, NULL, NULL));
  free(free_this), free_this = NULL;
  
  t (tcgetattr(STDIN_FILENO, &termios));
  saved_termios = termios;
  cfmakeraw(&termios);
  t (tcsetattr(STDIN_FILENO, TCSANOW, &termios));
  sttyed = 1;
  
  sigemptyset(&sigset);
  action.sa_handler = sigwinch;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  t (sigaction(SIGWINCH, &action, NULL));
  
  sigemptyset(&sigset);
  action.sa_handler = sigchld;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  t (sigaction(SIGCHLD, &action, NULL));
  
  while (!chlded)
    {
      if (winched)
	{
	  winched = 0;
	  copy_winsize(pty.master, STDIN_FILENO);
	}
      
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);
      FD_SET(pty.master, &fds);
      
      t (r = select(pty.master + 1, &fds, NULL, NULL, NULL), r < 0);
      
      if (FD_ISSET(pty.master, &fds))
	{
	  t (got = read(pty.master, buffer, sizeof(buffer)), got <= 0);
	  t (write(STDOUT_FILENO, buffer, (size_t)got) < 0);
	}
      if (FD_ISSET(STDIN_FILENO, &fds))
	{
	  t (got = read(STDIN_FILENO, buffer, sizeof(buffer)), got <= 0);
	  t (write(pty.master, buffer, (size_t)got) < 0);
	}
    }
  
  t (tcsetattr(STDIN_FILENO, TCSADRAIN, &saved_termios));
  libepiterm_pty_close(&pty);
  return 0;
  
 fail:
  fprintf(stderr, "%s: %s\r\n", called, strerror(errno));
  free(free_this);
  if (sttyed)
    tcsetattr(STDIN_FILENO, TCSADRAIN, &saved_termios);
  return 1;
}

