#include <libepiterm.h>
#include "libepiterm/macros.h"

#include <stdio.h>



static int io_callback(int from_epiterm, char* read_buffer, size_t read_size,
		       char** restrict write_buffer, size_t* restrict write_size)
{
  *write_buffer = read_buffer;
  *write_size = read_size;
  return 0;
  (void) from_epiterm;
}


int main(void)
{
  try (libepiterm_121(NULL, NULL, io_callback));
  return 0;
  
 fail:
  perror("libepiterm");
  return 1;
}

