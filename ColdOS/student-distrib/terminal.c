#include "terminal.h"

#define DUMMY_VALUE 0xFF

terminal_t terminals[NUM_OF_TERMINALS];
uint8_t colors[NUM_OF_TERMINALS] = {0x0C, 0x03, 0x0F};
volatile uint32_t cur_active_term;

/*init_terminals
 *    DESCRIPTION: initialize terminals
 *    INPUTS: none
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: modify terminals, cur_key_buffer_index, cur_active_term, and key_buffer_ptr
 */
void init_terminals() {
  int i, terminal_page_addr;
  for (i = 0; i < NUM_OF_TERMINALS; i++) {
    terminals[i].term_id = i;
    terminals[i].cur_process_index = -1;
    terminals[i].cursor_xcor = 0;
    terminals[i].cursor_ycor = 0;
    terminals[i].key_buffer_index = 0;
    terminals[i].enter = 0;
    terminals[i].term_esp0 = 0;
    terminals[i].term_esp = 0;
    terminals[i].term_ebp = 0;
    terminals[i].color = colors[i];
    /* initialize the keybuffer as 0 */
    memset(terminals[i].term_key_buffer, 0, KEY_SUM);
    /* skip main_video_mem page */
    terminal_page_addr = (VIDEO + (1 + i) * _4 * KB);
    map_terminal_page(terminal_page_addr);
    terminals[i].video_mem = (uint8_t*)(terminal_page_addr);
  }

  /* cur_key_buffer_index is from keyboard.h */
  cur_key_buffer_index = 0;
  cur_active_term = 0;
  key_buffer_ptr = terminals[0].term_key_buffer;
}

/*restore_terminal
 *      DESCRIPTION: restore the specific terminal back to the video memory
 *      INPUTS: term_id
 *      OUTPUTS: 0 if successful
 */
void restore_terminal(uint32_t term_id){
  /*
   * change global variables to change the terminal that
   * we are reading from or writing to
   */
  cur_key_buffer_index = terminals[term_id].key_buffer_index;
  key_buffer_ptr = terminals[term_id].term_key_buffer;

  /* restore screen_x and screen_y */
  set_screen_x(terminals[term_id].cursor_xcor);
  set_screen_y(terminals[term_id].cursor_ycor);

  /* restore video display */
  memcpy((uint8_t*)VIDEO_MEM_POINTER,(uint8_t*)terminals[term_id].video_mem, VIDEO_MEM_SPACE);
}

/*save_terminal
 *      DESCRIPTION: save the current terminal
 *      INPUTS: term_id
 *      OUTPUTS: 0 if successful
 */
void save_terminal(uint32_t term_id){
  /* store keyboard buffer */
  terminals[term_id].key_buffer_index = cur_key_buffer_index;

  /* save current screen_x and screen_y */
  terminals[term_id].cursor_xcor = get_screen_x();
  terminals[term_id].cursor_ycor = get_screen_y();

  /* save video display into corresponding terminal page */
  memcpy((uint8_t*)terminals[term_id].video_mem,(uint8_t*)VIDEO_MEM_POINTER, VIDEO_MEM_SPACE);
}


/*switch_terminal
 *      DESCRIPTION: save the current terminal, restore the desired terminal back to video memory
 *      INPUTS: from -- old terminal index
 *              to   -- new terminal index
 *      OUTPUTS: 0 is successful
 */
 void switch_terminal(uint32_t from, uint32_t to){
   /* save current terminal info */
   save_terminal(from);

   /* load next terminal info */
  restore_terminal(to);
  /* update cur_active_term */
  cur_active_term = to;

  /* update cursor */
  update_cursor();

 }


 /*terminal_write
  *     DESCRIPTION: write TO the screen from buf
  *     INPUTS: buf -- write buf data to terminal
  *             nbytes -- length of the input
  *     OUTPUTS: none
  *     RETURN VALUES: return the number of bytes written, or -1 on failure
  *     SIDE EFFECT: modify terminal
  */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
  cli();
  /* variable declaration */
  int32_t i, cur_pid, cur_term_id;
  pcb_t* cur_pcb;

  /* invalid case */
  if(buf == NULL) {
    return EXIT_FAILURE;
  }

  /* initialize counter */
  i = 0;

  /* find current pcb and current terminal_index */
  cur_pid = get_pcb();
  cur_pcb = (pcb_t*)get_pcb_addr(cur_pid);
  cur_term_id = cur_pcb -> term_id;

  /*
   * current terminal_index matches with cur_active_term so we can write on
   * video memory display
   */
  if(cur_active_term == cur_term_id) {
    for(i = 0; i < nbytes; i++) {
      putc(((uint8_t*)buf)[i]);
    }
  } else { /* write on terminal page in background */
    for(i = 0; i < nbytes; i++) {
      terminal_putc(((uint8_t*)buf)[i]);
    }
  }
  sti();
  /* return bytes written */
  return i;
}

/*terminal_read
 *     DESCRIPTION: read FROM the keyboard buffer into buf
 *     INPUTS: buf -- write keyboard buffer to buf
 *             nbytes -- number of possible bytes
 *     OUTPUTS: none
 *     RETURN VALUES: return number of bytes read
 *     SIDE EFFECT: modify buf
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){

  /* variable declaration */
  int32_t retval, cur_pid, cur_term_id;
  pcb_t* cur_pcb;

  /* get current process */
  cur_pid = get_pcb();
  cur_pcb = (pcb_t*)get_pcb_addr(cur_pid);
  cur_term_id = cur_pcb -> term_id;

  /* unmask interrupt flag to allow interrupt */
  sti();

  while(!terminals[cur_term_id].enter){ /* wait until ENTER is pressed */}

  /* critical section */
  cli();

  /* copy keyboard buffer into given buf */
  buf = memcpy(buf, key_buffer_ptr, cur_key_buffer_index);

  /* store number of bytes copied into buf */
  retval = cur_key_buffer_index;

  /* reset last_key */
  last_key = DUMMY_VALUE;

  /* flush keyboard buffer */
  memset(key_buffer_ptr, 0 , cur_key_buffer_index);

  /* reset cur_key_buffer_index */
  cur_key_buffer_index = 0;

  /* lower enter flag for next ENTER */
  terminals[cur_active_term].enter = 0;

  /* end of critical section */
  sti();

  /* return bytes read */
  return retval;
}


/*terminal_open
 *     DESCRIPTION: initialize terminal stuff (or nothing), return 0
 *     INPUTS: *ignored*
 *     OUTPUTS: none
 *     RETURN VALUES: always return 0
 *     SIDE EFFECT: none
 */
int32_t terminal_open(const uint8_t* filename){
    return 0;
}

/*terminal_close
 *     DESCRIPTION: clears any terminal specific variables (or do nothing), return 0
 *     INPUTS: *ignored*
 *     OUTPUTS: none
 *     RETURN VALUES: always return 0
 *     SIDE EFFECT: none
 */
int32_t terminal_close(int32_t fd){
    return 0;
}
