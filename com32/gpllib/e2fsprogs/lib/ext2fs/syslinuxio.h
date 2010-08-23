/*
 * syslinuxio.c -- Disk I/O module for the ext2fs/SYSLINUX library
 * 
 * Based on:
 *   v1.0 Disk I/O include file for the ext2fs/DOS library.
 *   Copyright (c) 1997 Mark Habersack
 *
 * This file may be distributed under the terms of the GNU Public License.
 *
 */
#ifndef __syslinuxio_h
#define __syslinuxio_h

/*
 * All partition data we need is here
 */
typedef struct
{
  char                 *dev;  /* _Linux_ device name (like "/dev/hda1") */
  unsigned char        phys;  /* Physical DOS drive number */
  unsigned long        start; /* LBA address of partition start */
  unsigned long        len;   /* length of partition in sectors */
  unsigned char        pno;   /* Partition number (read from *dev) */

  /* This partition's drive geometry */
  unsigned short       cyls;
  unsigned short       heads;
  unsigned short       sects;
} PARTITION;

/*
 * PC partition table entry format
 */
typedef struct
{
  unsigned char        active;
  unsigned char        start_head;
  unsigned char        start_sec;
  unsigned char        start_cyl;
  unsigned char        type;
  unsigned char        end_head;
  unsigned char        end_sec;
  unsigned char        end_cyl;
  unsigned long        first_sec_rel;
  unsigned long        size;
} PTABLE_ENTRY;

#endif /* __syslinuxio_h */
