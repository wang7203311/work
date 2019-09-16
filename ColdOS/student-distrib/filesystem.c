#include "filesystem.h"
#include "lib.h"

/* ------------------- File Section --------------------- */

/*file_open
 *    DESCRIPTION: initialize any temporary structures, return 0.
 *                 find the directory entry corresponding to the named file,
 *                 allocate an unused file descriptor, and set up any data
 *                 necessary to handle regular file, if the named file does
 *                 not exist or no descriptors are free, the call return -1
 *    INPUTS: filename -- filename to open
 *    OUTPUTS: none
 *    RETURN VALUES: file descriptor on success, -1 on failure
 *    SIDE EFFECTS: modify file descriptor array
 */
int32_t file_open(const uint8_t* filename) {
  return 0;
}

/*file_close
 *    DESCRIPTION: undo what you did in the open function, return 0
 *                 closes the specified file descriptor and makes
 *                 it available for return from later calls to open(),
 *                 file descriptor
 *    INPUTS: fd -- file descriptor to close
 *    OUTPUTS: none
 *    RETURN VALUES: 0 on success, -1 on failure
 *    SIDE EFFECTS: modify file descriptor array
 */
int32_t file_close(int32_t fd) {
  return 0;
}

/*file_read
 *    DESCRIPTION: reads count bytes of data from file into buf, uses read_data()
 *                 In the case of a file, data should be read to the end of the
 *                 file or the end of the buffer provided, whichever occurs
 *                 sooner reads nbytes of data from file into buf.
 *    INPUTS : fd - file descriptor
 *             buf : buffer we need to store the data
 *             nbyte: number of byte we need to read
 *    Output : none
 *    Return : the number of byte read successfully or failure(-1)
*/
int32_t file_read(int32_t fd, void* buf, int32_t nbytes) {
  /* variable declaration */
  int32_t retval, inode, offset, temp_id;
  pcb_t *addr;

  /* get pcb struct */
  temp_id = get_pcb();
  addr = (pcb_t*)get_pcb_addr(temp_id);

  /* find current inode and current file descriptor offset */
  inode = addr->fd_arr[fd].inode;
  offset = addr->fd_arr[fd].position;

  /* read_data(uint8_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
   *
   * load maximum nbytes of data from inode starting at offset into buf
   */
  retval = read_data(inode, offset, buf, nbytes);

  /* update file descriptor */
  addr->fd_arr[fd].position += retval;

  return retval;
}


/*file_write
 *    DESCRIPTION: writes to a regular file should always return -1
 *                 to indicate failure since the file ssytem is read only
 *    INPUTS : fd - file descriptor
 *             buf : buffer we need to store the data
 *             nbyte: number of byte we need to read
 *    Output : none
 *    Return : the number of byte read successfully or failure(-1)
 *    SIDE EFFECTS: read-only file system, always return -1
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes) {
  return -1;
}


/* ------------------- Directory Section ----------------*/


/*dir_open
 *    DESCRIPTION: opens a directory file (note file types), return 0
 *                 need read_dentry_by_name(): name means filename
 *    INPUTS:      filename -- filename
 *    OUTPUTS:     none
 *    RETURN VALUES: always return 0
 *    SIDE EFFECTS: modify file descriptor array
 */
int32_t dir_open(const uint8_t* filename) {
  return 0;
}

/*dir_close
 *    DESCRIPTION: closes the specified file descriptor and makes
 *                 it available for return from later calls to open
 *                 probably does nothing, return 0
 *    INPUTS:      fd -- file descriptor
 *    OUTPUTS:     none
 *    RETURN VALUES: always return 0
 *    SIDE EFFECTS: modify file descriptor array
 */
int32_t dir_close(int32_t fd) {
  return 0;
}

/*dir_read
 *    DESCRIPTION: In the case of reads to the directory, only the filename
 *                 should be provided (as much as fits, or all 32 bytes), and
 *                 subsequent reads should read from successive directory
 *                 entries until the last is reached, at which point read
 *                 should repeatedly return 0
 *                 read_dentry_by_index() index is NOT inode number
 *    INPUTS: fd -- fire descriptor
 *            buf -- buffer we need to store the data
 *           nbyte -- number of byte we need to read
 *    OUTPUTS : none
 *    RETURN VALUES: the number of byte read successfully or failure(-1)
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes) {

  /* varaible declaration */
  int32_t retval, temp_id, index;
  pcb_t *addr;
  dentry_t current_file;
  uint32_t num_of_dir = *(uint32_t *)(filesys_addr);

  /* get current pcb structure */
  temp_id = get_pcb();
  addr = (pcb_t*)get_pcb_addr(temp_id);

  /* get the index of the filename we want to read
   *
   * doing preincrement can let us skip the filename of
   * current directory (i.e, ".") when we execute program like "ls"
   */
  index = addr->fd_arr[fd].position++;

  /* the file descriptor has reached the end */
  if (index >= num_of_dir) {
    addr->fd_arr[fd].position--;
    return 0;
  }

  /* read from filesystem by index and store data in current_file */
  retval = read_dentry_by_index(index, &current_file);

  /* copy filename into buf in normal case */
  if (retval != -1) {
    memcpy(buf, (void *)(current_file.f_name), retval);
  }

  return retval;
}



/*dir_write
 *    DESCRIPTION: writes to a directory should always return -1
 *                 to indicate failure since the file ssytem is read only
 *    INPUTS : fd - file descriptor
 *             buf : buffer we need to store the data
 *             nbyte: number of byte we need to read
 *    Output : none
 *    Return : the number of byte read successfully or failure(-1)
 *    SIDE EFFECTS: read-only file system, always return -1
 */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes) {
  return -1;
}


/*helper function*/

/*read_dentry_by_name
 *    DESCRIPTION: finds the correspondring dentry in the filesystem
 *                 by filename, write information to dentry
 *    INPUTS : fname -- filename that looking for
 *    Output : dentry -- fill file information
 *    Return : 0 on success, -1 on failure
 *    SIDE EFFECTS: modify dentry
 */
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry) {
  /* varaible declaration */
  uint32_t num_of_dir, dir_index, name_start, file_length, dentry_flag;

  /* find the number of directory */
  num_of_dir = *(uint32_t *)(filesys_addr);

  /* find the length of the file name */
  file_length = strlen((int8_t*)fname);

  /* file_length exceeds the limit */
  if (file_length > FILE_NAME_SIZE) {
     return -1;
  }

  /* initialize counter and flag */
  dir_index = 0;
  dentry_flag = 0;

  //check if the name match
  while (dir_index++  < num_of_dir) {

    /* additional offset to skip first 64 bytes of unrelative data */
    name_start = DIRECTOTY_OFF * dir_index;

    /* compare two names */
    if(0 == strncmp((int8_t*)fname, (int8_t*)(filesys_addr + name_start),file_length)) {

      /* if the name is 32 bytes long */
      if (file_length == FILE_NAME_SIZE) {
        /* copy the name */
        strncpy((int8_t*)dentry->f_name, (int8_t*)(filesys_addr + name_start), FILE_NAME_SIZE);

        /* raise flag */
        dentry_flag = 1;

      } else if(*(uint8_t *)(filesys_addr + name_start + file_length) == '\0') { /* next character is null*/

        /* copy the name */
        strncpy((int8_t*)dentry->f_name, (int8_t*)(filesys_addr + name_start), file_length);

        /* raise flag */
        dentry_flag = 1;
      }

      /* if flag is raised */
      if ( dentry_flag ) {

        /* set file type */
        dentry->f_type = *(uint8_t *)(filesys_addr + name_start + FILE_NAME_SIZE);

        /* set inode */
        dentry->inode = *(uint8_t *)(filesys_addr + name_start + INODE_OFFSET);

        /* return success */
        return 0;
      }
    }
  }

  return -1;
}


/*read_dentry_by_index
*    DESCRIPTION: finds the correspondring dentry in the filesystem
*                 by index, write information to dentry
*    INPUTS : index -- bootblock entry index
*    Output : dentry -- fill file information
*    Return : 0 on success, -1 on failure
*    SIDE EFFECTS: modify dentry
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
  /* variable declaration */
  uint32_t num_of_dir, start, len;

  /* get the number of directory */
  num_of_dir = *(uint32_t *)(filesys_addr);

  /* index out of bound, return -1 */
  if (index >= num_of_dir) {
    return -1;
  }

  /* find the start by the index
   *
   * add one to skip boot block
   */
  start = filesys_addr + DIRECTOTY_OFF * (index + 1);

  /* copy filename */
  strncpy((int8_t*)dentry->f_name, (int8_t*)(start), FILE_NAME_SIZE);

  /* set file type */
  dentry->f_type = *(uint8_t *)(start + FILE_NAME_SIZE);

  /* set inode */
  dentry->inode = *(uint8_t *)(start + INODE_OFFSET);

  /* find file name length */
  len = 0;

  /* loop until we find the null terminator or we reach the limit */
  while(len < FILE_NAME_SIZE) {
    /* check if current char is the null terminator */
    if( *(dentry->f_name + len) == '\0') {
      break;
    }

    /* increment filename length */
    len ++;
  }

  return len;
}

/*read_data
 *    DESCRIPTION: read bytes in the file at given offset, and copy
 *                 bytes into the given buffer
 *    INPUTS : inode -- given file
 *             offset -- starting position
 *             length -- num of bytes to buffer
 *    Output : buf -- store file information
 *    Return : 0 on success, -1 on failure
 *    SIDE EFFECTS: read data from file and copy it to the buffer
 */
int32_t read_data(uint8_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
  /* varaible declaration */
  uint32_t inode_start, start, data_size, inode_addr;
  uint32_t data_block_index, counter, data_block_addr;
  uint32_t buf_offset, position, num_inode, data_block_start;

  /* inode start inedx */
  inode_start = filesys_addr + _4 * KB;

  /* the start of the inode block */
  start = inode_start + _4 * KB * inode;

  /* the data's size */
  data_size = *(uint32_t *)(start); //the data's size

  /* the block 0's address, add offset to skip data_size */
  inode_addr = start + _4;

  /* the file descriptor's position */
  position = offset;

  /* number of inode */
  num_inode = *(uint8_t *)(filesys_addr + _4);

  /* data_block_start address */
  data_block_start = inode_start + num_inode * _4 * KB;

  counter = offset;

  /* check for input */
  if(inode >= num_inode) {
    /* wrong inode index */
    return -1;
  }

  /*initialize buf_offset*/
  buf_offset = 0;

  /* check the data block */
  while (*(uint8_t*)(inode_addr) != '\0') {

    /* use the index of the data block to get the adress of data block */
    data_block_index = *(uint8_t*)(inode_addr);
    data_block_addr = data_block_start + _4 * KB * data_block_index;

    /* loop the data block */
    while (counter < (_4 * KB)) {

      /* we have reached the limit of either buf length or data_size */
      if (buf_offset >= length || position >= data_size) {
        return buf_offset;
      }

      /* copy data into buf */
      buf[buf_offset] = *(uint8_t *)(data_block_addr + counter);

      /* increment offsets */
      buf_offset++;
      position++;
      counter++;
    }

    /* reduce counter by 1 data block size */
    counter -=  _4 * KB;

    /* go to next data block */
    inode_addr += _4;
  }

  return buf_offset;
}
