#define _POSIX_SOURCE
#include "shell.h"
#include "macros.h"

#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>



const char* libepiterm_get_shell(char** restrict free_this)
{
  struct passwd pw_;
  struct passwd* pw;
  const char* shell = getenv("SHELL");
  char* new;
  uid_t uid = getuid();
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

