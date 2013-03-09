#ifndef FILE_LOCATION_H
#define FILE_LOCATION_H

#include "libc.h"

struct file_location {
	// needed after buffer is closed
	char *filename;

	// Needed if buffer doesn't have filename.
	// Pointer would have to be set to NULL after closing buffer.
	unsigned int buffer_id;

	// if pattern is set then line and column are 0 and vice versa
	char *pattern; // regex from tag file
	int line, column;
};

void file_location_free(struct file_location *loc);
bool file_location_equals(const struct file_location *a, const struct file_location *b);

#endif
