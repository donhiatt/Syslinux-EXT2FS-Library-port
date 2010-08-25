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
 * Sample module to exercise EXT2FS library
 *
 * Usage: ext2fs.c32 /dev/hd[a-z][1-9] cmd cmd-options
 */

#include <stdio.h>
#include <string.h>
#include <console.h>
#include "ext2fs_utils.h"

ext2_filsys current_fs = NULL;
ext2_ino_t root, cwd;
char *c32_name = NULL;

static void usage(void)
{
  printf("Usage: %s /dev/hd[a-z][1-9] cmd cmd-options\n", c32_name);
  printf("examples:\n");
  printf(" %s /dev/hdb1 cat /root/file.txt\n", c32_name);
  printf(" %s /dev/hdb1 cp /root/file.txt /root/newfile.txt\n", c32_name);
  printf(" %s /dev/hdb1 ls /boot\n", c32_name);
  printf(" %s /dev/hdb1 mkdir /foo\n", c32_name);
  printf(" %s /dev/hdb1 rm /root/file.txt\n", c32_name);
}

/*
 * load_bitmaps() need for writes
 */
static int load_bitmaps(void)
{
  errcode_t ret = 0;
  printf("Loading bitmaps...\n");
  ret = ext2fs_read_inode_bitmap(current_fs);
  if (ret) {
    printf("ERR: can't read inode bitmap (ret=%d)\n", (int)ret);
    return (int)ret;
  }
  
  ret = ext2fs_read_block_bitmap(current_fs);
  if (ret) {
    printf("ERR: can't read block bitmap (ret=%d)\n", (int)ret);
    return (int)ret;
  }

  return 0;
}

/*
 * main()
 */
int main(int argc, char *argv[])
{
  int read_only = 0; // NB: file reads require write access flag
  int write_changes = 0;
  int ret = 0;
  char *buf = NULL;
  char *dev_string = NULL;
  char *cmd_string = NULL;
  ext2_filsys fs;
  ext2_off_t size = 0;
  errcode_t retval = 0;
  
  openconsole(&dev_stdcon_r, &dev_stdcon_w);
  c32_name = argv[0];

  if (argc > 3) {
    dev_string = argv[1];
    cmd_string = argv[2];

    retval = ext2fs_open(dev_string,
			 (read_only ? 0 : EXT2_FLAG_RW),
			 0, 0, syslinux_io_manager, &fs);
    if (retval) {
      printf("ERR: Can't open '%s' (ret=%d)\n", dev_string, (int)retval);
      return -1;
    }
    
    root = cwd = EXT2_ROOT_INO;
    current_fs = fs;
    
    if (current_fs->super != NULL) {
      printf("'%s' has a valid superblock and is mounted %s\n",
	     dev_string, 
	     current_fs->flags & EXT2_FLAG_RW ? "Read/Write" : "Read Only");

      if (!strcmp(cmd_string, "cat")) {
	/*
	 * cat
	 */
	write_changes = 0;
	printf("Displaying '%s' file..\n", argv[3]);
	ret = load_file(argv[3], &buf, &size);
	if (ret) {
	  printf("ERR: Can't cat '%s' (ret=%d)\n", argv[3], ret);
	} else {
	  printf("'%s' is %d bytes\n", argv[3], size);
	  for (unsigned int i = 0; i < size; i++) {
	    printf("%c", buf[i]);
	  }
	  free(buf);
	}
      } else if (!strcmp(cmd_string, "cp")) {
	/*
	 * cp
	 */
	if (argc !=5) {
	  printf("ERR: '%s' missing target name\n", cmd_string);
	} else {
	  printf("Copying '%s' to '%s'\n", argv[3], argv[4]);
	  ret = load_bitmaps();
	  if (ret) {
	    write_changes = 0;
	    printf("ERR: Could not copy '%s' (ret=%d)\n", argv[3], ret);
	    goto close_filesys;
	  }
	  ret = load_file(argv[3], &buf, &size);
	  if (ret) {
	    printf("ERR: Can't load '%s' (ret=%d)\n", argv[3], ret);
	  } else {
	    /*
	     * TODO: don't hardcode file mode
	     */
	    ret = write_file(argv[4], buf, &size, 
			     (LINUX_S_IFREG | LINUX_S_IRWXU |
			      LINUX_S_IRGRP | LINUX_S_IXGRP |
			      LINUX_S_IROTH | LINUX_S_IXOTH));
	    free(buf);
	    if (ret) {
	      write_changes = 0;
	      printf("ERR: Can't write '%s' (ret=%d)\n", argv[4], ret);
	    } else {
	      write_changes = 1;
	      printf("'%s' has been copied to '%s'\n", argv[4], argv[3]);
	    }
	  }
	}
      } else if (!strcmp(cmd_string, "ls")) {
	/*
	 * ls
	 */
	printf("Displaying '%s' dir..\n", argv[3]);
	write_changes = 0;
	display_dir(argv[3]);
      } else if (!strcmp(cmd_string, "mkdir")) {
	/*
	 * mkdir
	 */
	printf("Making '%s' dir..\n", argv[3]);
	ret = load_bitmaps();
	if (ret) {
	  write_changes = 0;
	  printf("ERR: Could not mkdir '%s' (ret=%d)\n", argv[3], ret);
	  goto close_filesys;
	}
	ret = mkdir(argv[3]);
	if (ret) {
	  write_changes = 0;
	  printf("ERR: Could not mkdir '%s' (ret=%d)\n", argv[3], ret);
	} else {
	  write_changes = 1;
	  printf("Created '%s'\n", argv[3]);
	}
      } else if (!strcmp(cmd_string, "rm")) {
	/*
	 * rm 
	 */
	printf("Removing '%s' file..\n", argv[3]);
	ret = load_bitmaps();
	if (ret) {
	  write_changes = 0;
	  printf("ERR: Could not rm '%s' (ret=%d)\n", argv[3], ret);
	  goto close_filesys;
	}
	ret = delete_file(argv[3]);
	if (ret) {
	  write_changes = 0;
	  printf("ERR: Could not delete '%s' (ret=%d)\n", argv[3], ret);
	} else {
	  write_changes = 1;
	  printf("'%s' has been deleted\n", argv[3]);
	}
      } else {
	printf("ERR: Unknown command '%s'\n", cmd_string);
      }

    close_filesys:
      retval = close_fs(write_changes);
      if (retval) {
	printf("ERR: could not close filesytem!\n");
      } else {
	printf("Filesystem closed.\n");
      }
    }
  } else {
    usage();
    return 0;
  }
 
  return 0;
}

