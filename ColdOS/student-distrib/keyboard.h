#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "types.h"
#include "lib.h"
#include "i8259.h"
#include "terminal.h"

#define KEY_MODE 4
#define NUM_OF_KEYS 60
#define RELEASED 0
#define PRESSED 1
#define NO_CAP_NO_SHIFT 0
#define NO_CAP_SHIFT 1
#define CAP_NO_SHIFT 2
#define CAP_SHIFT 3
#define CTRL 5
#define ENTER 0x1C
#define CAPS 0x3A
#define LEFT_SHIFT 0x2A
#define RIGHT_SHIFT 0x36
#define LEFT_SHIFT_RELEASE 0xAA
#define RIGHT_SHIFT_RELEASE 0xB6
#define BACKSPACE	0x0E
#define LEFT_CTRL_PRESSED 0x1D
#define LEFT_CTRL_RELEASED 0x9D
#define RELEASE_FLAG 0x81
#define CRTL_SCANCODE 0x26

//the last key we typed in
extern uint8_t last_key;
extern uint8_t scancode[KEY_MODE][NUM_OF_KEYS];
//keyboard buffer
extern uint8_t* key_buffer_ptr;
extern uint8_t cur_key_mode;
//keep track how many keys had already in key buffer
extern int32_t cur_key_buffer_index;
//All terminals has the three button status
extern uint32_t caps_button_status;
extern uint32_t shift_button_status;
extern uint32_t enter_button_status;


/* Initialize keyboard*/
void init_keys(void);

/* Keyboard interrupt handler */
void keyboard_handler(void);

/*handle_enter*/
void handle_enter();

void new_line_char();
#endif
