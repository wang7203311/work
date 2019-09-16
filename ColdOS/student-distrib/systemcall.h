#ifndef _SYSTEMCALL_H
#define _SYSTEMCALL_H


#define ENTRY_START 24
#define KERNEL_STACK_SIZE 0x2000
#define PAGE_STARTING_ADDRESS 0x800000
#define BUF_SIZE 100
#define RETURN_FROM_EXCEPTION 255

#include "rtc.h"
#include "terminal.h"
#include "filesystem.h"
#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "paging.h"
#include "pit.h"

/*Function Operation Table
 * read:  function pointer to a specific read function
 * write: function pointer to a specific write function
 * open:  function pointer to a specific open function
 * close: function pointer to a specific close function
 */
typedef struct operation_ptr {
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
    int32_t (*open)(const uint8_t* filename);
    int32_t (*close)(int32_t fd);
} operation_table_t;

/*File Descriptor
 *fun_ptr:  function pointer to a table of file operations
 *inode:    inode number of the file in the file system
 *position: current position where we are reading the file, update this everytime we read it.
 *flags:    either use or unused
 */
typedef struct file_discriptor{
    operation_table_t* fun_ptr;
    int32_t inode;
    int32_t position;
    int32_t flag;
}file_discriptor_t;

/*Pcb
 * fd_arr:      open up to 8 files
 * pid   :      process_id
 * ppid  :      parent process id
 * parent_esp:  parent stack pointer
 * parent_ebp:  parent ebp   pointer
 */
typedef struct pct{
    file_discriptor_t fd_arr[MAX_NUM_FILE];
    int32_t pid;
    /* parent's pid */
    int32_t ppid;
    int32_t parent_esp;
    int32_t parent_ebp;
    int32_t parent_esp0;
    int8_t argument_buf[BUF_SIZE];
    int32_t term_id;
    int32_t rtc_freq;
} pcb_t;

/*system call halt*/
extern int32_t halt(uint8_t status);
/*system call execute*/
extern int32_t execute(const uint8_t* command);
/*system call read*/
extern int32_t read(int32_t fd, void* buf, int32_t nbytes);
/*system call write*/
extern int32_t write(int32_t fd, const void* buf, int32_t nbytes);
/*system call open*/
extern int32_t open(const uint8_t* filename);
/*system call close*/
extern int32_t close(uint32_t fd);
/*system call getargs*/
extern int32_t getargs(uint8_t* buf, uint32_t nbytes);
/*system call vidmap*/
extern int32_t vidmap(uint8_t** screen_start);
/*get the next the available process id*/
extern int32_t get_next_available_id();
/*Initialize the fd_array*/
extern void init_fd_array(file_discriptor_t* fd_arr);

extern int32_t set_handler (int32_t signum, void* handler_address);

extern int32_t sigreturn (void);
/*get pcb addr in the 4MB to 8MB page*/
uint32_t get_pcb_addr(int32_t pid);
/*get the current pcb*/
int32_t get_pcb( );
/*get the current process_id*/
uint32_t get_pid();
/*This function is used to replace NULL in the operatio table*/
int32_t fail_function();

int32_t fail_write_function(int32_t fd, const void* buf, int32_t nbytes);

int32_t fail_open_function(const uint8_t* filename);

int32_t fail_close_function(int32_t fd);

int32_t fail_read_function(int32_t fd, void* buf, int32_t nbytes);

uint32_t copy_buffer(uint8_t* dest, uint32_t limit, const uint8_t* source, uint32_t source_start);

#endif
