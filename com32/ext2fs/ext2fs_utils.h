/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2010 Don Hiatt - All Rights Reserved
 *
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, and to permit persons to whom
 *   the Software is furnished to do so, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------- */

/*
 * ext2fs
 * Support code for ext2fs module
 */

#ifndef UTIL_H
#define UTIL_H

#include <ext2fs/ext2fs.h>
#include <ext2fs/ext2_io.h>

int file_present(const char *filename);
int load_file(const char *filename, char **contents, 
	      ext2_off_t *size);
int write_file(const char *filename, char *contents, 
	       ext2_off_t *size, __u16 i_mode);
int close_fs(int write_changes);
int mkdir(char *dirname);
int delete_file(const char *filename);
int is_dir(const char *filename);
void display_dir(const char *name);

#endif /* UTIL_H */
