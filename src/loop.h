#ifndef LIBEPITERM_LOOP_H
#define LIBEPITERM_LOOP_H

#include "hypoterm.h"
#include "pty.h"

#include <stddef.h>

typedef union libepiterm_term
{
  int is_hypo;
  libepiterm_hypoterm_t hypo;
  libepiterm_pty_t epi;
  
} libepiterm_term_t;

int libepiterm_loop(libepiterm_term_t** restrict terms, size_t termn,
		    int (*io_callback)(libepiterm_term_t* restrict read_term, char* read_buffer,
				       size_t read_size, int* restrict write_fd,
				       char** restrict write_buffer, size_t* restrict write_size),
		    int (*winch_callback)(void),
		    int (*wait_callback)(libepiterm_pty_t* restrict pty, int status));

#endif

