#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "types.h"
#include "systemcall.h"

/* initialized in kernel.c */
unsigned int filesys_addr;

typedef struct _dentry_t {
  /* file name */
  int8_t f_name[ FILE_NAME_SIZE ];

  /* file type */
  int32_t f_type;

  /* inode info */
  uint32_t inode;

  /* inode reserved bytes, read the document for more detailed explanation */
  uint8_t reserved[ 24 ];
} dentry_t;



/* File Section */
int32_t file_open(const uint8_t* filename);

int32_t file_close(int32_t fd);

int32_t file_read(int32_t fd, void* buf, int32_t nbytes);

int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

/* Directory Section */
int32_t dir_open(const uint8_t* filename);

int32_t dir_close(int32_t fd);

int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);

/*writes to a regular file shoul dalways return -1 to indicate failure since the file ssytem is read only*/
int32_t dir_write(int32_t fd, const void* buf, int32_t nbyte);

int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry);

int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);

int32_t read_data(uint8_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

#endif /* _FILESYSTEM_H */
