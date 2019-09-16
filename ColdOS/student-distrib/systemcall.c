#include "systemcall.h"
#define ASM 1
uint32_t Process_id[ MAX_TASK ] = {0,0,0,0,0,0};

//initialize the operation_table_t
operation_table_t stdin = {terminal_read,fail_write_function,fail_open_function,fail_close_function};
operation_table_t stdout = {fail_read_function,terminal_write, fail_open_function, fail_close_function};
operation_table_t terminal_op = {terminal_read, terminal_write,terminal_open, terminal_close};
operation_table_t rtc_op = {rtc_read, rtc_write, rtc_open, rtc_close};
operation_table_t dir_op = {dir_read, dir_write, dir_open, dir_close};
operation_table_t file_op = {file_read, file_write, file_open, file_close};

/*This function is here to avoid warning
 *This function is used to replace NULL in the operation table
 */
int32_t fail_write_function(int32_t fd, const void* buf, int32_t nbytes) {
  return EXIT_FAILURE;
}

/*This function is here to avoid warning
 *This function is used to replace NULL in the operation table
 */
int32_t fail_open_function(const uint8_t* filename) {
  return EXIT_FAILURE;
}

/*This function is here to avoid warning
 *This function is used to replace NULL in the operation table
 */
int32_t fail_close_function(int32_t fd) {
  return EXIT_FAILURE;
}

/*This function is here to avoid warning
 *This function is used to replace NULL in the operation table
 */
int32_t fail_read_function(int32_t fd, void* buf, int32_t nbytes) {
  return EXIT_FAILURE;
}


/* Execute()
 * input:  command
 * output: 0 if suessful
 *         -1 if fail
 * This function execute the typed command, it follows the steps:
 * 1. Parse Args
 * 2. Check File Validity
 * 3. Set up Paging
 * 4. Load file into memory
 * 5. Create PCB/Open FDs
 * 6. Prepare for context to stack
 * 7. "fake" the iret context to stack
 * 8. Iret
 * 9. return.
 * return -1 if failed
 */
int32_t execute(const uint8_t* command) {
   cli();
    /* varaible declaration */
    /* place to hold the executable file names */
    uint8_t cmd_buf[ MAX_CMD_SIZE ];
    uint8_t arg_buf[ MAX_ARG_SIZE ];

    /* counters that keep track of current index */
    uint32_t buf_ptr, command_ptr;
    int32_t magic, process_id;
    uint32_t entry_point;
    dentry_t temp_dentry;
    pcb_t *new_pcb;
    int32_t wrnm = terminal_index;

    /****************
     *  Parse Args  *
     ****************/

     /* initialize counters, cmd_buf and arg_buf */
     command_ptr = 0;
     buf_ptr = 0;
     memset(cmd_buf, 0, MAX_CMD_SIZE);
     memset(arg_buf, 0, MAX_ARG_SIZE);

     /* ignore space on the front of the command */
     while (command[ command_ptr ] == ' ') {
       command_ptr ++;
     }

     /* copy command into cmd_buf */
     buf_ptr = copy_buffer(cmd_buf, MAX_CMD_SIZE, command, command_ptr);

     /* update command_ptr */
     command_ptr += buf_ptr;

     /* increment pointer if current byte is ' ' */
     while (command[ command_ptr ] == ' ') {
       command_ptr ++;
     }

     /* copy argument into arg_buf */
     buf_ptr = copy_buffer(arg_buf, MAX_ARG_SIZE, command, command_ptr);

    /***********************
     * Check File Validity *
     ***********************/

    /* check if there exists a file with same name */
    ASSERT_DIFF(read_dentry_by_name((uint8_t*)cmd_buf, &temp_dentry), -1);

    /* check the executable magic number */
    read_data(temp_dentry.inode, 0, (uint8_t *)&magic, _4);
    ASSERT_EQUAL(magic, MAGIC_NUM);

    /* get the entry point of the program */
    read_data(temp_dentry.inode, ENTRY_START, (uint8_t *)&entry_point, _4);

    /*****************
     * Set Up Paging *
     *****************/

     /* get current process id */
     process_id = get_next_available_id();
     /* set the current terminal's cur process id as process id*/
    //  terminals[cur_active_term].cur_process_index = process_id;

     /* process_id is not valid and no reasonable pid found */
     ASSERT_DIFF(process_id, -1);

     /* starting from physical memory 8 MB, and use pid to distinguish */
     map_page(_8 * MB + process_id * _4 * MB);

     /*************************
      * Load file into memory *
      *************************/

     /* copy the file into 0x08048000 in virtual memory */
     read_data(temp_dentry.inode, 0, (uint8_t*)0x08048000, MAX_FILE_SIZE);

    /*************************
     * Create PCB && Open fd *
     *************************/

    /* initialize Process Control Block (PCB) */
    new_pcb = (pcb_t *)get_pcb_addr(process_id);
    new_pcb -> pid = process_id;
    new_pcb -> rtc_freq = FREQUENCY_DEFAULT;

    if(pit_call) { /* only happen for the first time of shell boot */
      new_pcb->term_id = wrnm;
      terminals[wrnm].cur_process_index = process_id;
      new_pcb->ppid = process_id;
    } else {
       pit_call = 0;
       new_pcb->ppid = terminals[cur_active_term].cur_process_index;
      terminals[cur_active_term].cur_process_index = process_id;
      new_pcb->term_id = cur_active_term;
    }

    /* initialize argument buffer in PCB */
    memset(new_pcb->argument_buf, 0, MAX_ARG_SIZE);

    /* copy the arg_buf to the pcb->argument_buf */
    memcpy(new_pcb->argument_buf, arg_buf, buf_ptr);

    /* initialize file descriptor array */
    init_fd_array(new_pcb->fd_arr);

    /* Saving the current EBP into parent stack pointer*/
    asm volatile("movl %%ebp, %0;"
                 "movl %%esp, %1;"
                 :"=r"(new_pcb->parent_ebp), "=r"(new_pcb->parent_esp)
                 :/*no inputs*/
                 :"cc");

    /********************************
     * Prepare for context to stack *
     ********************************/
    tss.ss0 = KERNEL_DS;

    new_pcb -> parent_esp0 = tss.esp0;

    /* we cannot use the last 4 bytes */
    tss.esp0 = _8 * MB - _8 * KB * process_id -4;

    if(pit_call) { /* only happen for the first time of shell boot */
      terminals[wrnm].term_esp0 = tss.esp0;
    } else {
      terminals[cur_active_term].term_esp0 = tss.esp0;
    }

    /*Re-enable Interrupts*/
    sti();

    /************************************
     * "fake" the iret context to stack *
     ************************************/
    asm volatile(
                 /* esp */
                 "movl $0x83FFFFC, %%eax;"
                 /* push user_ds */
                 "pushl $0x2B;"
                 /* push esp that goes to user_stack_pointer */
                 "pushl %%eax;"
                 /* push flags */
                 "pushfl;"
                 /* push user code segment */
                 "pushl $0x23;"
                 /* set eip to entry_point */
                 "pushl %0;"

                 /********
                  * Iret *
                  ********/
                 "iret;"

                 /* label that is used in halt() */
                 "RETURN_FROM_IRET:;"
                 "LEAVE;"
                 "RET;"
                 :	/* no outputs */
                 :"r"(entry_point)	/* input */
                 :"%edx","%eax"	/* clobbered register */);

   /******************
    * return failure *
    ******************/
    /* the expected result should never reach here,
     * if we are returned from here,
     * an unknown error occured
     */
    return EXIT_FAILURE;

}

/*getargs():
 *inputs: buf, nbytes
 *outputs: return 0 on success
 *         return -1 on failure
 *Description: it copyes nbytes of data from current pcb to the user
 *             level buf
 */
int32_t getargs(uint8_t* buf, uint32_t nbytes){
  int32_t pid;
  pcb_t* pcb_entry;

  /*Check if the buf is invalid*/
  ASSERT_DIFF(buf, NULL);

  /* clear buffer to avoid garbage value */
  memset(buf, 0, nbytes);
  /*get the current pcb*/

  // pid = get_pid();
  pid = terminals[cur_active_term].cur_process_index;
  /* invalid pid */
  ASSERT_DIFF(pid, -1);

  pcb_entry = (pcb_t*)get_pcb_addr(pid);

  if (strlen(pcb_entry->argument_buf) > 0) {
    copy_buffer(buf, nbytes, (uint8_t *)pcb_entry->argument_buf, 0);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}


/*vidmap():
 *inputs: screen_start
 *outputs: return virtual memory address if successful
 *         return -1 if failure
 *Description: maps the text-mode video memory into user space at a pre-set
 *             virtual address
 */
int32_t vidmap(uint8_t** screen_start){
  uint32_t term_id;
  pcb_t* cur_pcb;
  /*error checking*/
  ASSERT_DIFF(screen_start, NULL);
  ASSERT_DIFF(screen_start, (uint8_t**)(_4 * MB));

  cur_pcb = (pcb_t*)get_pcb_addr(terminals[cur_active_term].cur_process_index);
  term_id = cur_pcb -> term_id;
  map_video_page((uint32_t)(VIDEO_MEM<<12), term_id);
  *screen_start = (uint8_t*)(132 * MB);
  return 132 * MB;
}

/*read()
 *inputs: fd, buf, nbytes
 *outputs: return value of the specific read
 *         -1 if failed to read
 *Description: this function called the specific read
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes){
  /* declare local varaibles */
  int32_t retval;
  pcb_t *pcb_entry;
  file_discriptor_t *pcb_fd;

  /* invalid inputs */
  if ((fd >= MAX_NUM_FILE || fd < 0) || (buf == NULL)) {
    return EXIT_FAILURE;
  }

  /* failed to get pcb */
  int32_t temp_pid = get_pcb();

  /* failed to get pcb */
  ASSERT_DIFF(temp_pid, -1);

  pcb_entry =(pcb_t*)get_pcb_addr(temp_pid);

  /* get file_discriptor */
  pcb_fd = &pcb_entry->fd_arr[fd];

  /* flag is lowed => an invalid fd */
  ASSERT_DIFF(pcb_fd->flag, 0);

  /* file_discriptor has no relative function call */
  ASSERT_DIFF(pcb_fd->fun_ptr, NULL);

  /* make relative funtion call */
  retval = pcb_fd->fun_ptr->read(fd, buf, nbytes);

  /* update file descriptor position */
  return retval;

}
/*write()
 *inputs: fd, buf, nbytes
 *outputs: return value of the specific write
 *         -1 if failed to read
 *Description: this function called the specific write
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes){
  /* declare local varaibles */
  int32_t retval;
  pcb_t* pcb_entry;
  file_discriptor_t *pcb_fd;
  /* invalid inputs */
  if ((fd >= MAX_NUM_FILE || fd < 0) || (buf == NULL)) {
    return EXIT_FAILURE;
  }

  /* failed to get pcb */
  int32_t temp_pid = get_pcb();
  ASSERT_DIFF(temp_pid, -1);

  /* get file_discriptor */
  pcb_entry =(pcb_t*)get_pcb_addr(temp_pid);
  pcb_fd = &(pcb_entry->fd_arr[fd]);

  /* flag is lowed => an invalid fd */
  ASSERT_DIFF(pcb_fd->flag, 0);

  /* file_discriptor has no relative function call */
  ASSERT_DIFF(pcb_fd->fun_ptr, NULL);

  /* make relative funtion call */
  retval = pcb_fd->fun_ptr->write(fd, buf, nbytes);

  return retval;
}


/*Open()
 *inputs: filename
 *outputs: index in the fd_arr
 *         -1 if failed to read
 *Description: Open a file into current running process control block
 */
int32_t open(const uint8_t* filename){
     /* declare local varaibles */
  int32_t counter;
  pcb_t *pcb_entry;
  file_discriptor_t* pcb_fd;
  dentry_t file_entry;

  /* invalid input */
  ASSERT_DIFF(filename, NULL);

  /* file not found in file system */
  ASSERT_DIFF(read_dentry_by_name(filename, &file_entry), -1);

  int32_t temp_pid = get_pcb();
  /* failed to get pcb */
  ASSERT_DIFF(temp_pid, -1);

  pcb_entry =(pcb_t*)get_pcb_addr(temp_pid);
  /* look for an empty file descriptor */
  /* first 2 are for stdin, stdout, skip them */
  for (counter = 2; counter < MAX_NUM_FILE; counter++)
  {
    /* found an empty file_discriptor, break the loop */
    if (0 == pcb_entry->fd_arr[counter].flag)
    {
      /* use pcb_fd to represent the file_discriptor */
      pcb_fd = &pcb_entry->fd_arr[counter];

      /* raise flag, occupy the slot*/
      pcb_fd->flag = 1;

      /* initialize the file position */
      pcb_fd->position = 0;

      /* assign inode */
      pcb_fd->inode = file_entry.inode;

      break;
    }
  }

  /* no empty file descriptor found */
  ASSERT_DIFF(counter, MAX_NUM_FILE);

  /* assign different function pointers for different file type */
  switch (file_entry.f_type)
  {
    case TYPE_RTC:
      pcb_fd->fun_ptr = &rtc_op;
      break;

    case TYPE_REGULAR_FILE:
      pcb_fd->fun_ptr = &file_op;
      break;

    case TYPE_DIRECOTRY:
      pcb_fd->fun_ptr = &dir_op;
      break;

    default: /* no matching file type */
      /* lower flag to free the slot */
      pcb_fd->flag = 0;
      return EXIT_FAILURE;
  }

  /* failed to open */
  if(-1 == pcb_fd->fun_ptr->open(filename))
  {
    /* lower flag to free the slot */
    pcb_fd->flag = 0;
    return EXIT_FAILURE;
  }

  /* return index in fd_arr */
  return counter;

}


/*Close()
 *inputs: fd
 *outputs: 0 if sucessful
 *         -1 if failed to read
 *Description: close file
 */
int32_t close(uint32_t fd){
    /* declare local varaibles */
  pcb_t *pcb_entry;
  file_discriptor_t pcb_fd;

  /* invalid input
   *
   * stdin and stdout cannot be closed
   */
  if ((fd < 2) || (fd > MAX_NUM_FILE)) {
    return EXIT_FAILURE;
  }

  /* failed to get pcb */
  int32_t temp_pid = get_pcb();

  /* failed to get pcb */
  ASSERT_DIFF(temp_pid, -1);

  pcb_entry = (pcb_t*)get_pcb_addr(temp_pid);

  /* find file_descriptor */
  pcb_fd = pcb_entry->fd_arr[fd];

  /* trying to close a file_discriptor that does not exist */
  ASSERT_DIFF(pcb_fd.flag, 0);

  /* lower file_discriptor flag */
  pcb_entry->fd_arr[fd].flag = 0;

  return pcb_fd.fun_ptr->close(fd);
}

/*Halt()
 *inputs: status
 *outputs: return status
 *Description: finish a current process, returning the specific status to its parent process
 */
int32_t halt(uint8_t status) {
  /* declaration */
  cli();
  int32_t ppid, counter;
  pcb_t * current_pcb, *previous_pcb;
  uint32_t cur_pid, cur_term;

  cur_pid = get_pcb();
  current_pcb = (pcb_t*)get_pcb_addr(cur_pid);
  cur_term = current_pcb->term_id;


  ppid = current_pcb->ppid;

  previous_pcb = (pcb_t*)get_pcb_addr(ppid);

  /* free up pid_states */
  Process_id[current_pcb->pid] = 0;

  /* clear file_descriptor in current_pcb */
  for (counter = 0; counter < MAX_NUM_FILE; counter ++) {
    /* clean up each file descriptor */
    if (current_pcb->fd_arr[counter].flag) {
      current_pcb->fd_arr[counter].fun_ptr->close(counter);
      current_pcb->fd_arr[counter].flag = 0;
      current_pcb->fd_arr[counter].inode = 0;
      current_pcb->fd_arr[counter].fun_ptr = NULL;
    }
  }

  tss.esp0 = current_pcb->parent_esp0;
  terminals[cur_term].term_esp0 = tss.esp0;

  /* reboot shell when there is no running process left */
  if(current_pcb -> pid == ppid) {
    /* reset esp0 */
    pit_call = 1;
    execute((uint8_t*) "shell");
  }

  /* active terminal pid should be ppid */
  terminals[cur_term].cur_process_index = ppid;

  /* each page has 4MB and each proces take 1 page */
  map_page(current_pcb->ppid * 4 * MB + _8 * MB);



  uint32_t return_status = status;

  if(status == RETURN_FROM_EXCEPTION){
    return_status ++;
  }
  sti();
  /* return to previous process */
  asm ("               \n\
    movl %0, %%eax     \n\
    movl %1, %%esp     \n\
    movl %2, %%ebp     \n\
    jmp RETURN_FROM_IRET  \n\
    "
    :/* no output */
    : "r"((uint32_t) return_status), "r"(current_pcb->parent_esp), "r"(current_pcb->parent_ebp)
    : "%eax"
  );

  /* this return should be ignored */
  return EXIT_FAILURE;
}

/* set_handler()
 * DESCRIPTION: use handler_address function when the given signum is triggered
 * INPUT : signum -- suggests the corresponding signal
 *         handler_address -- the address of the replacing function
 * OUTPUT : none
 * RETURN VALUES: 0 on success, -1 on failure
 */
int32_t set_handler (int32_t signum, void* handler_address) {
  return EXIT_FAILURE;
}


int32_t sigreturn (void) {
  return EXIT_FAILURE;
}

/* get_next_available_id()
 * return the available process id
 * return index between 0 to 5 if successful
 * return -1 if it exceeds 6 process
 */
int32_t get_next_available_id(){
    int32_t i;
    for(i = 0; i < MAX_TASK; i++){
        if(Process_id[i]==0){
            Process_id[i] = 1;
            return i;
        }
    }
    return EXIT_FAILURE;
}


/* init_fd_array()
 * inputs: fd_array
 * outputs: None
 * initialize the fd_array with all 8 files,
 * the first two are stdin and stout
 */
void init_fd_array(file_discriptor_t* fd_arr)
{
    fd_arr[0].fun_ptr = &stdin;
    fd_arr[0].inode = 0;
    fd_arr[0].position = 0;
    fd_arr[0].flag = 1;
    fd_arr[1].fun_ptr = &stdout;
    fd_arr[1].inode = 0;
    fd_arr[1].position = 0;
    fd_arr[1].flag = 1;
    int i;
    for(i = 2; i < MAX_NUM_FILE; i++)
    {
        fd_arr[i].fun_ptr = NULL;
        fd_arr[i].inode = 0;
        fd_arr[i].position = 0;
        fd_arr[i].flag = 0;
    }
}


/* get_pcb()
 * input: pcb_entry
 * output: return the current running pid
 * Description: return the current running process_id
 */
int32_t get_pcb( ) {
  /* declaration */
  int32_t pid;

  /* get the current pid */
  /* pid starts from 1 and the array is 0 indexed */
  pid = get_pid();

  /* no running process with given pid */
  ASSERT_DIFF(Process_id[ pid ], 0);

  return pid;
}
/* get_pcb_addr()
 * Description: return the addr of pid's pcb
 * input: pid
 * output: the address of the pcb inside the 4MB to 8MB page
 */
uint32_t get_pcb_addr(int32_t pid)
{
    uint32_t addr = PAGE_STARTING_ADDRESS - (pid + 1) * KERNEL_STACK_SIZE;
    //printf("addr:: %x\n", addr);
    return addr;
}

/* get_pid()
 * Description: return the current running process id
 * input: None
 * output: the current pid number
 */
uint32_t get_pid() {
  /* declaration */
  uint32_t pid, esp, kernel_base;

  /* initialization */
  esp = tss.esp0;
  pid = 0;
  kernel_base = PAGE_STARTING_ADDRESS;

  /* search from the bottom of the kernel stack to find the process */
  while(kernel_base > esp) {
    kernel_base -= KERNEL_STACK_SIZE;
    pid ++;
  }

  return pid - 1;
}

/*copy_buffer()
 * DESCRIPTION : copy data from source starting at source_start
 *               into dest at most of limit size
 * INPUT : dest - destination, write data to this buffer
 *         limit - maximum capacity of the dest buffer
 *         source - source buffer, read data from this buffer
 *         source_start - offset of source buffer
 * OUTPUT : None
 * RETURN VALUE: number of bytes read
 * SIDE EFFECT : None
 */
uint32_t copy_buffer(uint8_t* dest, uint32_t limit, const uint8_t* source, uint32_t source_start) {
  uint32_t dest_off, source_off;
  uint8_t current_char;

  /* loop buffer */
  for(source_off = source_start, dest_off = 0; dest_off < limit; dest_off++, source_off++) {
    /* load current char in source buffer */
    current_char = source[ source_off ];

    /* terminate when current_char is space or null terminator */
    if (current_char == ' ' || current_char =='\0') {
      break;
    }

    /* copy data into dest */
    dest[ dest_off ] = source[ source_off ];
  }

  /* return number of bytes read into dest */
  return dest_off;
}
