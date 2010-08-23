/*
 * syslinuxio.c -- Disk I/O module for the ext2fs/SYSLINUX library
 * 
 * Based on:
 *   dosio.c -- Disk I/O module for the ext2fs/DOS library.
 *     Copyright (c) 1997 by Theodore Ts'o.
 *     Copyright (c) 1997 Mark Habersack
 *
 * This file may be distributed under the terms of the GNU Public License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <disk/bootloaders.h>
#include <disk/common.h>
#include <disk/errno_disk.h>
#include <disk/error.h>
#include <disk/geom.h>
#include <disk/mbrs.h>
#include <disk/msdos.h>
#include <disk/partition.h>

#include <disk/read.h>
#include <disk/write.h>

#include <disk/swsusp.h>
#include <disk/util.h>

#include <et/com_err.h>
#include <ext2fs/ext2fs.h>
#include <ext2fs/ext2_types.h>
#include <ext2fs/ext2_io.h>
#include "syslinuxio.h"
#include "et/com_err.h"
#include "ext2_err.h"

struct driveinfo drive;
static PARTITION *requested_partition = NULL;
static int found_partition = 0;

static void parse_partition_callback(struct driveinfo *drive_info EXT2FS_ATTR((unused)),
				     struct part_entry *ptab,
				     int partition_offset,
				     int nb_partitions_seen)
{
  unsigned int start, end;
    
  start = partition_offset;
  end = start + ptab->length - 1;
 
#if 0
  if (nb_partitions_seen == 1)
    printf(" #  B       Start         End    Size     Type\n");
  
  printf("%2d  %s %11d %11d %11d\t%02X\n",
	 nb_partitions_seen, 
	 (ptab->active_flag == 0x80) ? "x" : " ",
	 start, end, ptab->length, ptab->ostype);
#endif
  
  /*
   * Update requested partition information if we found it.
   */
  if (nb_partitions_seen == requested_partition->pno) {
    found_partition = 1;
    requested_partition->start = start;
    requested_partition->len = ptab->length;
  }
}

static int disk_get_geometry(PARTITION *part)
{
  int ret = 0;
  struct driveinfo *d = &drive;
  
  memset(d, 0, sizeof(struct driveinfo));
  d->disk = part->phys;
  ret = get_drive_parameters(d);
    
  if (ret) {
    printf("Error 0x%Xh while reading disk 0x%X\n", ret, d->disk);
    return ret;
  }

#if 1
  printf("  DISK: 0x%x\n", d->disk);
  printf("  C/H/S: cbios=%d %d heads, %d cylinders\n",
	 d->cbios, d->legacy_max_head + 1, d->legacy_max_cylinder + 1);
  printf("         %d sectors/track, %d drives\n",
	 d->legacy_sectors_per_track, d->legacy_max_drive);
  printf("  EDD:   ebios=%d, EDD version: %X\n",
	 d->ebios, d->edd_version);
  printf("         %d heads, %d cylinders\n",
	 (int) d->edd_params.heads, (int) d->edd_params.cylinders);
  printf("         %d sectors, %d bytes/sector, %d sectors/track\n",
	 (int) d->edd_params.sectors, (int) d->edd_params.bytes_per_sector,
	 (int) d->edd_params.sectors_per_track);
  printf("         Host bus: %s, Interface type: %s\n\n",
	 d->edd_params.host_bus_type, d->edd_params.interface_type);
#endif

  if (d->ebios) {
    part->heads = d->edd_params.heads;
    part->cyls  = d->edd_params.cylinders;
    part->sects = d->edd_params.sectors_per_track;
  } else {
    part->heads = d->legacy_max_head + 1;
    part->cyls  = d->legacy_max_cylinder + 1;
    part->sects = d->legacy_sectors_per_track;
  }

  if (parse_partition_table(d, &parse_partition_callback)) {
    if (errno_disk) {
      printf("I/O error parsing disk 0x%X\n", d->disk);
      get_error("parse_partition_table");
    } else {
      printf("Disk 0x%X: unrecognized partition layout\n", d->disk);
    }
  }

  return 0;
}

/*
 * I/O Manager routine prototypes
 */
static errcode_t syslinux_open(const char *dev, int flags, io_channel *channel);
static errcode_t syslinux_close(io_channel channel);
static errcode_t syslinux_set_blksize(io_channel channel, int blksize);

static errcode_t syslinux_read_blk(io_channel channel, 
				   unsigned long int block,
				   int count, void *buf);

static errcode_t syslinux_write_blk(io_channel channel, 
				    unsigned long int block,
				    int count, const void *buf);

static errcode_t syslinux_flush(io_channel channel);

static struct struct_io_manager struct_syslinux_manager = {
  .magic       = EXT2_ET_MAGIC_IO_MANAGER,
  .name        = "SYSLINUX I/O Manager",
  .open        = syslinux_open,
  .close       = syslinux_close,
  .set_blksize = syslinux_set_blksize,
  .read_blk    = syslinux_read_blk,
  .write_blk   = syslinux_write_blk,
  .flush       = syslinux_flush,
};

io_manager syslinux_io_manager = &struct_syslinux_manager;

/*
 * Allocate libext2fs structures associated with I/O manager
 */
static io_channel alloc_io_channel(PARTITION *part)
{
  io_channel     ioch;

  ioch = (io_channel)malloc(sizeof(struct struct_io_channel));
  if (!ioch)
	  return NULL;
  memset(ioch, 0, sizeof(struct struct_io_channel));
  ioch->magic = EXT2_ET_MAGIC_IO_CHANNEL;
  ioch->manager = syslinux_io_manager;
  ioch->name = (char *)malloc(strlen(part->dev)+1);
  if (!ioch->name) {
	  free(ioch);
	  return NULL;
  }
  strcpy(ioch->name, part->dev);
  ioch->private_data = part;
  ioch->block_size = 1024; /* The smallest ext2fs block size */
  ioch->read_error = 0;
  ioch->write_error = 0;

  return ioch;
}

/*
 * Open the 'name' partition, initialize all information structures
 * we need to keep and create libext2fs I/O manager.
 */
static errcode_t syslinux_open(const char *dev, 
			       int flags EXT2FS_ATTR((unused)),
			       io_channel *channel)
{
  int ret = 0;
  char *p1;
  PARTITION *part;

  part = (PARTITION*)malloc(sizeof(PARTITION));
  if (!part) {
    printf("ERR: Can't malloc partition structure\n");
    return ENOMEM;
  }

  /*
   * Extract drive and partition number from device string.
   */
  p1 = strstr(dev, "hd");
  if (p1 && strlen(p1) >= 4) {
    p1 += 2;
    /* first drive, "a" -> 0x80 */
    part->phys = (int)p1[0] + 0x1f;
    ++p1;
    part->pno = atoi(p1);
    //printf("drive: 0x%x partition:%d (%s)\n", part->phys, part->pno, p1);
  } else {
    printf("ERR: Open must be of the form \"/dev/hda1\"\n");
    return EFAULT;
  }

  part->dev = strdup(dev);
  requested_partition = part;

  /*
   * Get drive's geometry & partition info
   */
  ret = disk_get_geometry(part);
  if(ret || !found_partition) {
    printf("ERR: Can't process disk \"%s\" (ret=%d, found=%d)\n", 
	   dev, ret, found_partition);
    free(part->dev);
    free(part);
    return EFAULT;
  }

  printf("Device \"%s\" (drive=0x%x pno=%d) found\n"
	 "        C/H/S = %d/%d/%d start=%ld len=%ld\n", 
	 dev, part->phys, part->pno,
	 part->cyls, part->heads, part->sects,
	 part->start, part->len);

  /*
   * Now alloc all libe2fs structures
   */
  *channel = alloc_io_channel(requested_partition);
  if (!*channel)
	  return ENOMEM;

  return 0;
}

static errcode_t syslinux_close(io_channel channel)
{
	free(channel->name);
	free(channel);

	return 0;
}

static errcode_t syslinux_set_blksize(io_channel channel, int blksize)
{
  channel->block_size = blksize;
  return 0;
}

static errcode_t syslinux_read_blk(io_channel channel, 
				   unsigned long int block,
				   int count, void *buf)
{
  char *sector_buf = NULL;
  PARTITION     *part;
  size_t        size;
  ext2_loff_t   lba;
  int ret = 0;
  struct driveinfo *d = &drive;

  EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
  part = (PARTITION*)channel->private_data;
  
  size = (size_t)((count < 0) ? -count : count * channel->block_size);
  lba  = (ext2_loff_t)(((block * channel->block_size) / SECTOR) + part->start);

  /*
   * Our minimum disk read is a sector (512 bytes) so allocate
   * a new buffer if we need less than that.
   *
   * The reason for this is that read_bitmaps(rw_bitmaps.c) 
   * allocates only 256 bytes for the inode_bitmaps, however
   * our syslinux_read_blk() will fill in 512 bytes leading to
   * a VERY(VERY!!) nasty corruption that took me weeks to
   * track down. -don
   */
  if (size < SECTOR) {
    //printf("read=%d < %d, alloc new buf.\n", size, SECTOR, buf);
    sector_buf = malloc(SECTOR);
    if (!sector_buf) {
      printf("ERR: can't malloc %d bytes!\n", size);
      return EFAULT;
    }
  }

  ret = read_sectors(d, 
		     size < SECTOR ? sector_buf : buf, 
		     lba, size < SECTOR ? 1 : size/SECTOR);

  if(ret == -1) {
    printf("ERR: dev_read() failed\n");
    return EFAULT;
  }

  /*
   * Copy sector read buffer to request buffer.
   */
  if (sector_buf) {
    memcpy(buf, sector_buf, size);
    free(sector_buf);
  }

  return 0;
}

static errcode_t syslinux_write_blk(io_channel channel, 
				    unsigned long int block,
				    int count, const void *buf)
{
  PARTITION     *part;
  ext2_loff_t   lba;
  size_t        size;
  int ret = 0;

  EXT2_CHECK_MAGIC(channel, EXT2_ET_MAGIC_IO_CHANNEL);
  part = (PARTITION*)channel->private_data;
  
  if (count < 0) {
    printf("Writing superblocks\n");
    size = (size_t)-count;
  } else {
    size = (size_t)(count * channel->block_size);
  }
  lba  = (ext2_loff_t)(((block * channel->block_size) / SECTOR) + part->start);

  //printf("write_sectors() size=%d lba:0x%lx\n", (size < SECTOR ? 1 : size/SECTOR), lba);
  ret = dev_write(part->phys, buf, lba, size < SECTOR ? 1 : size/SECTOR);

  if(ret == -1) {
    printf("ERR: write failed\n");
    return EFAULT;
  }

  return 0;
}

static errcode_t syslinux_flush(io_channel channel EXT2FS_ATTR((unused)))
{
  /*
   * No buffers, no flush...
   */
  return 0;
}
