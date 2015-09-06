#ifndef LIBEPITERM_OVERLAY_H
#define LIBEPITERM_OVERLAY_H

#include "pty.h"
#include <stddef.h>

int libepiterm_121(const char* shell, char* (*get_record_name)(libepiterm_pty_t* pty),
		   int (*io_callback)(int from_epiterm, char* read_buffer, size_t read_size,
				      char** restrict write_buffer, size_t* restrict write_size));

#endif

