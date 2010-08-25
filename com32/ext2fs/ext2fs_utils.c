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

#include <stdio.h>
#include <string.h>
#include "ext2fs_utils.h"

extern ext2_filsys	current_fs;
extern ext2_ino_t	root, cwd;

/*
 * file_preset()
 */
int file_present(const char *filename) 
{
  int retval = 0;
  ext2_ino_t inode = 0;

  retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			filename, &inode);

  if (retval == EXT2_ET_FILE_NOT_FOUND) 
    return 0;
  else 
    return (retval < EXT2_ET_BASE ? 1 : 0);
}

/*
 * is_dir()
 */
int is_dir(const char *filename) 
{
  int retval = 0;
  ext2_ino_t ino = 0;

  /* get inode */
  retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			filename, &ino);
  if (retval) {
    return 0;
  }
  
  /* directory? */
  retval = ext2fs_check_directory(current_fs, ino);
  if (!retval) {
    return 1;
  } else {
    return 0;
  }
}

/* 
 * read_inode()
 */
static int read_inode(ext2_ino_t ino, struct ext2_inode *inode)
{
  int retval = 0;

  retval = ext2fs_read_inode(current_fs, ino, inode);

  if (retval) {
    printf("ERR: Can't read inode %u (ret=%d)", ino, retval);
    return retval;
  }
  return 0;
}

/*
 * write_inode()
 */
static int write_inode(ext2_ino_t ino, struct ext2_inode *inode)
{
  int retval = 0;
  
  retval = ext2fs_write_inode(current_fs, ino, inode);
  if (retval) {
    printf("ERR: Can't write inode %u (ret=%d)\n", ino, retval);
    return 1;
  }
  return 0;
}

/*
 * close_fs()
 */
int close_fs(int write_changes)
{
  int retval = 0;
  
  printf("Closing Filesystem...Changes will %s written to disk.\n", 
	 write_changes ? "be" : "NOT be");

  if (write_changes) {
    if (current_fs->flags & EXT2_FLAG_IB_DIRTY) {
      printf("Syncing inode_bitmap...\n");
      retval = ext2fs_write_inode_bitmap(current_fs);
      if (retval) {
	printf("ERR: ext2fs_write_inode_bitmap (retval=%d)", retval);
	goto close_fail;
      }
    }

    if (current_fs->flags & EXT2_FLAG_BB_DIRTY) {
      printf("Syncing block_bitmap...\n");
      retval = ext2fs_write_block_bitmap(current_fs);
      if (retval) {
	printf("ERR: ext2fs_write_block_bitmap (retval=%d)", retval);
	goto close_fail;
      }
    }
  }

  retval = ext2fs_close(current_fs);
  if (retval) {
    printf("ERR: ext2fs_close (retval=%d)", retval);
    goto close_fail;
  }

  retval = 0;

 close_fail:
  current_fs = NULL;
  
  return retval;
}

/*
 * display_dir_callback()
 */
static int display_dir_callback(struct ext2_dir_entry *dirent,
				int	offset EXT2FS_ATTR((unused)),
				int	blocksize EXT2FS_ATTR((unused)),
				char	*buf EXT2FS_ATTR((unused)),
				void	*private EXT2FS_ATTR((unused)))
{
  int len = 0;
  char name[EXT2_NAME_LEN + 1];
  struct ext2_inode inode;
  ext2_ino_t ino;

  len = ((dirent->name_len & 0xFF) < EXT2_NAME_LEN) ?
    (dirent->name_len & 0xFF) : EXT2_NAME_LEN;
  
  memset(name, '\0', EXT2_NAME_LEN + 1);
  strncpy(name, dirent->name, len);
  name[len] = '\0';
  ino = dirent->inode;

  if (ino && read_inode(ino, &inode)) {
    return 0;
  }

  printf("'%s' (inode=%u, mode=%06o, uid=%d, gid=%d, ", 
	 name,
	 ino, 
	 inode.i_mode, 
	 inode.i_uid, 
	 inode.i_gid);

  if (LINUX_S_ISDIR(inode.i_mode))
    printf("dir");
  else
    printf("%lld bytes", inode.i_size | ((__u64)inode.i_size_high << 32));

  printf(")\n");

  return 0;
}

/*
 * display_dir()
 */
void display_dir(const char *name)
{
  int flags = 0;      
  ext2_ino_t inode = 0;
  errcode_t retval = 0;
  char *buf = NULL;

  retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			name, &inode); 

  if (retval) {
    printf("ERR: Can't resolve dir '%s' (retval=%d)\n", name, (int)retval);
    return;
  }

  printf("display dir '%s' (inode=%d)\n", name, inode);
  buf = malloc(current_fs->blocksize);
  if (!buf) {
    printf("ERR: can't malloc %d bytes!\n", current_fs->blocksize);
    return;
  }
  memset(buf, '\0', current_fs->blocksize);

  retval = ext2fs_dir_iterate(current_fs, inode, flags,
			      buf, display_dir_callback, NULL);
  
  if (retval) {
    printf("ERR: dir_iterate2 retval=%d\n", (int)retval);
    return;
  }
}

/*
 * load_file()
 */
int load_file(const char *filename, char **contents, ext2_off_t *size)
{
  int retval = 0;
  unsigned int got = 0;
  unsigned int wanted = 0;
  char *ptr = NULL;
  ext2_ino_t inode = 0;
  ext2_file_t file;

  retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			filename, &inode);

  if (retval >= EXT2_ET_BASE) {
    printf("ERR: Can't resolve file %s (ret=%d)\n", filename, retval);
    return retval;
  }

  retval = ext2fs_file_open(current_fs, inode, EXT2_FILE_WRITE, &file);
  if (retval) {
    printf("ERR: Can't open %s (ret=%d)\n", filename, retval);
    return retval;
  }

  *size = ext2fs_file_get_size(file);

  printf("filename: %s (inode=%d) is %ld bytes\n", 
	 filename, inode, (long int)*size);

  *contents = (char *)malloc(*size+1);
  ptr = *contents;
  if (ptr == NULL) {
    printf("ERR: can't malloc buffer\n");
    return -1;
  }

  wanted = *size;
  while (wanted > 0) {
    retval = ext2fs_file_read(file, ptr, wanted, &got);
    
    if (retval) {
      printf("ERR: Can't read %s (retval=%d)\n", filename, retval);
      goto load_fail;
    }

    //printf("wanted: %d got %d\n", wanted, got);
    wanted -= got;
    ptr += got;
    
  }

  (*contents)[*size] = 0;
  retval = 0;

 load_fail:
  (void) ext2fs_file_close(file);

  return retval;
}

/*
 * write_file()
 */
int write_file(const char *filename, char *contents, 
	       ext2_off_t *size, __u16 i_mode)
{
  int got = 0;
  int len = 0;
  unsigned int written = 0;
  char *ptr = NULL;
  char *pathname = NULL;
  char *name = NULL;
  ext2_file_t e2_file;
  ext2_ino_t newfile;
  ext2_ino_t parent;
  errcode_t retval;
  struct ext2_inode inode;

  /* file exists? */
  retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			filename, &newfile);
  if (retval == 0) {
    printf("ERR: The file \"%s\" already exists\n", filename);
    return retval;
  }

  /* 
   * get parent directory and filename
   */
  len = strlen(filename);
  pathname = malloc(len + 1);
  if (!pathname) {
    printf("ERR: can't malloc %d bytes!\n", len+1);
    return -1;
  }
  memset(pathname, '\0', len + 1);
  strncpy(pathname, filename, len);

  ptr = strrchr(pathname, '/');
  if (ptr) {
    char *p = strchr(pathname, '/');
    name = ptr + 1;

    if (ptr == p) {
      /* root directory */
      parent = EXT2_ROOT_INO;
      printf("parent dir = '/', file: '%s'\n", name);
    } else {
      /* sub directory */
      ext2_ino_t ino;
      pathname[ptr-p] = '\0';
      printf("parent dir = '%s', file: '%s'\n", pathname, name);

      /* get parent inode */
      retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			    pathname, &ino);
      if (retval) {
	printf("ERR: \"%s\" does not exist\n", pathname);
	return retval;
      }
      
      /* make sure parent is a directory */
      retval = ext2fs_check_directory(current_fs, ino);
      if (retval) {
	printf("ERR: \"%s\" is not a directory\n", pathname);
	return retval;
      }
      parent = ino;
    }
  } else {
    parent = cwd;
  }

  printf("parent inode: %d\n", parent);

  /* allocate new inode */
  retval = ext2fs_new_inode(current_fs, parent, 010755, 0, &newfile);
  if (retval) {
    printf("ERR: ext2fs_new_inode() failed (retval=%d)\n", (int)retval);
    return retval;
  }
  printf("Allocated inode: %u\n", newfile);

  /* link inode */
  retval = ext2fs_link(current_fs, parent, name, newfile, EXT2_FT_REG_FILE);
  if (retval == EXT2_ET_DIR_NO_SPACE) {
    retval = ext2fs_expand_dir(current_fs, parent);
    if (retval) {
      printf("ERR: ext2fs_expand_dir failed (retval=%d)\n", (int)retval);
      return retval;
    }
    retval = ext2fs_link(current_fs, parent, name, newfile, EXT2_FT_REG_FILE);
  }

  if (retval) {
    printf("ERR: ext2fs_link failed (retval=%d)\n", (int)retval);
    return retval;
  }

  if (ext2fs_test_inode_bitmap(current_fs->inode_map, newfile)) {
    printf("Warning: inode already set (retval=%d)\n", (int)retval);
  }

  /* setup file stats */
  ext2fs_inode_alloc_stats2(current_fs, newfile, +1, 0);
  memset(&inode, 0, sizeof(inode));

  /* set file permissions */
  inode.i_mode = i_mode;

  /* set file times */
  inode.i_atime = inode.i_ctime = inode.i_mtime =
    current_fs->now ? current_fs->now : time(0);

  inode.i_links_count = 1;
  inode.i_size = *size;

  if (current_fs->super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS) {
    inode.i_flags |= EXT4_EXTENTS_FL;
  }

  /* write new inode */
  retval = ext2fs_write_new_inode(current_fs, newfile, &inode);
  if (retval) {
    printf("ERR: ext2fs_write_new_inode failed (retval=%d)\n", (int)retval);
    return retval;
  }

  /* open file for writes */
  retval = ext2fs_file_open(current_fs, newfile,
			    EXT2_FILE_WRITE, &e2_file);
  if (retval) {
    printf("ERR: ext2fs_file_open failed (retval=%d)\n", (int)retval);
    return retval;
  }

  /*
   * write the file
   */
  got = *size;
  ptr = contents;

  while (got > 0) {
    retval = ext2fs_file_write(e2_file, ptr, got, &written);
    if (retval) {
      printf("ERR: ext2fs_file_write failed (retval=%d)\n", (int)retval);
      goto write_fail;
    } 
    
    got -= written;
    ptr += written;
  }

  retval = 0;

 write_fail:
  (void) ext2fs_file_close(e2_file);

  free(pathname);

  return retval;
}

/*
 * mkdir()
 */
int mkdir(char *dirname)
{
  int retval = 0;
  char *ptr = NULL;
  char *name = NULL;
  ext2_ino_t parent;

  ptr = strrchr(dirname, '/');

  if (ptr) {
    *ptr = 0;
    retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			  dirname, &parent);
    if (!parent) {
      printf("ERR: '%s' has no parent!\n", dirname);
      return retval;
    }
    name = ptr + 1;
  } else {
    parent = cwd;
    name = dirname;
  }

 try_again:
  retval = ext2fs_mkdir(current_fs, parent, 0, name);
  
  if (retval == EXT2_ET_DIR_NO_SPACE) {
    printf("expanding dir...\n");
    retval = ext2fs_expand_dir(current_fs, parent);
    if (retval) {
      printf("ERR: Can't expand dir\n");
      return retval;
    }
    goto try_again;
  }

  if (retval) {
    printf("ERR: ext2fs_mkdir failed (ret=%d)\n", retval);
  }  

  return retval;
}

/*
 * block_iterate_callback()
 */
static int block_iterate_callback(ext2_filsys fs, blk_t *blocknr,
				  int blockcnt EXT2FS_ATTR((unused)),
				  void *private EXT2FS_ATTR((unused)))
{
  ext2fs_block_alloc_stats(fs, *blocknr, -1);
  return 0;
}

/*
 * delete_inode()
 */
static void delete_inode(ext2_ino_t inode)
{
  struct ext2_inode ino;

  if (read_inode(inode, &ino)) {
    goto ino_out;
  }
  ino.i_dtime = current_fs->now ? current_fs->now : time(0);

  if (write_inode(inode, &ino)) {
    goto ino_out;
  }

  if (!ext2fs_inode_has_valid_blocks(&ino)) {
    printf("ERR: inode has invalid blocks\n");
    goto ino_out;
  }

  ext2fs_block_iterate(current_fs, 
		       inode, 
		       BLOCK_FLAG_READ_ONLY, 
		       NULL,
		       block_iterate_callback, 
		       NULL);

  ext2fs_inode_alloc_stats2(current_fs, 
			    inode, 
			    -1,
			    LINUX_S_ISDIR(ino.i_mode));

 ino_out:
  return;
}

/*
 * unlink_file()
 */
int unlink_file(const char *filename)
{
  int retval = 0;
  int len = strlen(filename);
  char *ptr = NULL;
  char *pathname = NULL;
  ext2_ino_t inode = 0;
  
  /* get pathname */
  pathname = malloc(len + 1);
  if (!pathname) {
    printf("ERR: can't malloc %d bytes!\n", len+1);
    return -1;
  }
  memset(pathname, '\0', len + 1);
  strncpy(pathname, filename, len);

  ptr = strrchr(pathname, '/');
  if (ptr) {
    *ptr++ = '\0';

    retval = ext2fs_namei(current_fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 
			  pathname, &inode);
    if (retval) {
      printf("ERR: \"%s\" does not exist\n", pathname);
      return retval;
    }
  } else {
    inode = cwd;
    ptr = pathname;
  }

  retval = ext2fs_unlink(current_fs, inode, ptr, 0, 0);
  if (retval) {
    printf("ERR: Can't unlink '%s' (ret=%d)\n", pathname, retval);
  }

  return retval;
}

/*
 * delete_file()
 */
int delete_file(const char *filename)
{
  int retval;
  ext2_ino_t ino;
  struct ext2_inode inode;

  retval = ext2fs_namei(current_fs, root, cwd, filename, &ino);
  if (retval) {
    printf("ERR: \"%s\" does not exist (ret=%d)\n", filename, retval);
    return retval;
  }

  retval = read_inode(ino, &inode);
  if (retval) {
    return retval;
  }

  if (LINUX_S_ISDIR(inode.i_mode)) {
    printf("ERR: \"%s\" is a directory\n", filename);
    return 1;
  }

  --inode.i_links_count;
  retval = write_inode(ino, &inode);
  if (retval) {
    return retval;
  }

  unlink_file(filename);

  if (inode.i_links_count == 0) {
    delete_inode(ino);
  }

  return 0;
}
