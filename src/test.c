#define _XOPEN_SOURCE  700
#define _GCC_SOURCE
#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>



static volatile sig_atomic_t winched = 0;


static void sigwinch(int signo)
{
  winched = 1;
  (void) signo;
}


static void copy_winsize(int whither, int whence)
{
  struct winsize winsize;
  ioctl(whence,  TIOCGWINSZ, &winsize);
  ioctl(whither, TIOCSWINSZ, &winsize);
}


int main(void)
{
  int master, slave, dups;
  const char* slavename;
  struct sigaction action;
  sigset_t sigset;
  pid_t pid;
  const char* shell;
  struct passwd* pw;
  struct termios termios;
  struct termios saved_termios;
  
  shell = getenv("SHELL");
  if ((shell == NULL) || (access(shell, X_OK) < 0))
    {
      pw = getpwuid(getuid());
      if (pw)
	shell = pw->pw_shell;
    }
  if (shell == NULL)
    shell = "/bin/sh";
  
  master = posix_openpt(O_RDWR | O_NOCTTY);
  grantpt(master);
  unlockpt(master);
  slavename = ptsname(master);
  slave = open(slavename, O_RDWR | O_NOCTTY);
  
  sigemptyset(&sigset);
  action.sa_handler = sigwinch;
  action.sa_mask = sigset;
  action.sa_flags = 0;
  
  tcgetattr(STDIN_FILENO, &termios);
  tcsetattr(master, TCSANOW, &termios);
  copy_winsize(master, STDIN_FILENO);
  saved_termios = termios;
  cfmakeraw(&termios);
  tcsetattr(STDIN_FILENO, TCSANOW, &termios);
  
  sigaction(SIGWINCH, &action, NULL);
  
  pid = fork();
  if (pid == 0)
    goto child;
  
  for (;;)
    {
      if (winched)
	{
	  winched = 0;
	  copy_winsize(master, STDIN_FILENO);
	}
      
      /* SELECT AND SPLICE  */
    }
  
  tcsetattr(STDIN_FILENO, TCSADRAIN, &saved_termios);
  close(master);
  return 0;
  
 child:
  close(master);
  dups = 0;
  if (slave != STDIN_FILENO)   dups++, dup2(slave, STDIN_FILENO);
  if (slave != STDOUT_FILENO)  dups++, dup2(slave, STDOUT_FILENO);
  if (slave != STDERR_FILENO)  dups++, dup2(slave, STDERR_FILENO);
  if (dups == 3)
    close(slave);
  execl(shell, shell, NULL);
  perror(shell);
  return 1;
}

