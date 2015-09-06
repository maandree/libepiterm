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
#define _POSIX_SOURCE
#include "shell.h"
#include "macros.h"

#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>



/**
 * Figure out what shell shall be started
 * 
 * @param   free_this  Ouput parameter for a pointer that
 *                     the caller shall free once the returned
 *                     string is no longer needed
 * @return             The shell, do not free this, it may
 *                     be statically allocated (in which case
 *                     `*free_this == NULL`), `NULL` is returned
 *                     on failure and `errno` is set to indicate
 *                     the error, additionally, on error
 *                     `*free_this` is guaranteed to be `NULL`
 */
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
  
  /* Did $SHELL not contain a valid shell, read /etc/passwd. */
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
  
  /* Did /etc/passwd not specify a shell (perhaps the user
     has been removed,) fall back to /bin/sh. */
  if (shell == NULL)
    shell = "/bin/sh", free(*free_this), *free_this = NULL;
  
  return shell;
 fail:
  saved_errno = errno;
  free(*free_this), *free_this = NULL;
  return errno = saved_errno, NULL;
}

