#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "keyboard.h"
#include "types.h"
#include "paging.h"
#include "lib.h"
#include "systemcall.h"

typedef struct _terminal_t {
    //pointer to the video memory
    uint8_t* video_mem;
    //since later need multiple terminal, this is a dummy id for now
    uint32_t term_id;
    //determine whether if the current terminal is on
    uint32_t term_on;
    //determine the current active process index
    uint32_t cur_process_index;
    //current key buffer index
    uint32_t key_buffer_index;
    //need enter flag for each terminal
    volatile uint32_t enter;
    //position of the cursor
    uint32_t cursor_xcor;
    uint32_t cursor_ycor;
    //Each terminal has its own key buffer
    uint8_t term_key_buffer[KEY_SUM];
    //terminal esp0
    uint32_t term_esp0;
    uint32_t term_esp;
    uint32_t term_ebp;

    uint8_t color;
} terminal_t;


extern terminal_t terminals[NUM_OF_TERMINALS];
extern volatile uint32_t cur_active_term;
extern volatile uint32_t enter_occured;

void init_terminals();
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
void restore_terminal(uint32_t term_id);
void save_terminal(uint32_t term_id);
void switch_terminal(uint32_t from, uint32_t to);
#endif /* end of _TERMINAL_H*/
