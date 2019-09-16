#include "keyboard.h"

#define LEFT_ALT_PRESSED 0x38
#define LEFT_ALT_RELEASED 0xB8
#define F1 0x3B
#define F2 0x3C
#define F3 0x3D

uint8_t scancode[KEY_MODE][NUM_OF_KEYS] = {
    //implemented the function key separatelly, such as shift, enter in scan code maps to '\0
	//no caplocks no shift
	{'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\0', '\0', 'a', 's',
	 'd', 'f', 'g', 'h', 'j', 'k', 'l' , ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
	 'b', 'n', 'm',',', '.', '/', '\0', '*', '\0', ' ', '\0'},
	//no caplocks with shift
	{'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\0', '\0', 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L' , ':', '"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
	 'B', 'N', 'M', '<', '>', '?', '\0', '*', '\0', ' ', '\0'},
	//with caplocks no shift
	{'\0', '\0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\0', '\0',
	 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\0', '\0', 'A', 'S',
	 'D', 'F', 'G', 'H', 'J', 'K', 'L' , ';', '\'', '`', '\0', '\\', 'Z', 'X', 'C', 'V',
	 'B', 'N', 'M', ',', '.', '/', '\0', '*', '\0', ' ', '\0'},
	//with caplocks with shift
	{'\0', '\0', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\0', '\0',
	 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\0', '\0', 'a', 's',
	 'd', 'f', 'g', 'h', 'j', 'k', 'l' , ':', '"', '~', '\0', '\\', 'z', 'x', 'c', 'v',
	 'b', 'n', 'm', '<', '>', '?', '\0', '*', '\0', ' ', '\0'}
};
/* end of keyboard definition*/

volatile uint32_t enter_occured = 0;
uint8_t last_key = 0;
uint8_t* key_buffer_ptr = 0;
uint8_t cur_key_mode = 0;
int32_t cur_key_buffer_index = 0;
uint32_t caps_button_status = RELEASED;
uint32_t shift_button_status = RELEASED;
uint32_t shift_button_status_l = RELEASED;
uint32_t shift_button_status_r = RELEASED;
uint32_t enter_button_status = RELEASED;
uint32_t left_control_status = RELEASED;
uint32_t left_alt_status = RELEASED;

/*keyboard_handler
 *    DESCRIPTION: keyboard interrupt handler
 *    INPUTS: none
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: called when there is an interrupt, print input
 */
void keyboard_handler() {
  /* send EOI */
  cli();
  send_eoi(KEYBOARD_IRQ);



  uint8_t cur_key;
  /* check KEYBOARD_STATUS port's lowest bit */
  while( !(inb(KEYBOARD_STATUS) & 0x01) );

  /* read KEYBOARD_DATA port for data */
  cur_key = inb(KEYBOARD_DATA);

	/* update the last key*/
	if(cur_key <= RELEASE_FLAG) last_key = cur_key;
  //if the keystrike is enter
  if(cur_key == ENTER) {
		key_buffer_ptr[cur_key_buffer_index++] = '\n';
		//cli();
    handle_enter();
		//sti();
    return;
  }
  //depending on the cur_key strike, determinethe mode of the keyboard
  switch (cur_key){
      case CAPS:
        caps_button_status = !caps_button_status;
        break;
      case LEFT_SHIFT:
        shift_button_status_l = PRESSED;
        break;
      case RIGHT_SHIFT:
        shift_button_status_r = PRESSED;
        break;
      case LEFT_SHIFT_RELEASE:
        shift_button_status_l = RELEASED;
        break;
      case RIGHT_SHIFT_RELEASE:
        shift_button_status_r = RELEASED;
        break;
			case LEFT_CTRL_PRESSED:
				left_control_status = PRESSED;
				break;
			case LEFT_CTRL_RELEASED:
				left_control_status = RELEASED;
				break;
			case LEFT_ALT_PRESSED:
				left_alt_status = PRESSED;
				break;
			case LEFT_ALT_RELEASED:
				left_alt_status = RELEASED;
				break;
			case F1:
				if(left_alt_status == PRESSED){
					switch_terminal(cur_active_term,0);
				}
				break;
			case F2:
				if(left_alt_status == PRESSED){
					switch_terminal(cur_active_term,1);
				}
				break;
			case F3:
				if(left_alt_status == PRESSED){
					switch_terminal(cur_active_term,2);
				}
				break;
			case BACKSPACE:
				if(cur_key_buffer_index>0){
				cur_key_buffer_index--;
				key_buffer_ptr[cur_key_buffer_index] = '\0';
				deletec();
			}

  }

  shift_button_status = shift_button_status_l | shift_button_status_r;
  //change the mode of the keyboard accordingly
      if(caps_button_status == PRESSED && shift_button_status == PRESSED) cur_key_mode = CAP_SHIFT;
      else if(caps_button_status == PRESSED) cur_key_mode = CAP_NO_SHIFT;
      else if(shift_button_status == PRESSED) cur_key_mode = NO_CAP_SHIFT;
			else if(left_control_status == PRESSED) cur_key_mode = CTRL;
      else cur_key_mode = NO_CAP_NO_SHIFT;

	if(left_control_status==PRESSED && cur_key==CRTL_SCANCODE){
		int idx;
		//clear screen
		for(idx = 0; idx < cur_key_buffer_index;idx++){
			key_buffer_ptr[idx] = '\0';
		}
		reset_left_corner();
		update_cursor();
		return;
	}



  if(cur_key > RELEASE_FLAG || cur_key == CAPS || cur_key == LEFT_SHIFT || cur_key == RIGHT_SHIFT || cur_key == LEFT_CTRL_PRESSED ||cur_key == LEFT_ALT_PRESSED)
	return;
	if(scancode[cur_key_mode][cur_key]=='\0' && cur_key!=BACKSPACE)
	return;

	/* leave one key for new line */
  if(cur_key_buffer_index < KEY_SUM - 2 && cur_key!=BACKSPACE){
    key_buffer_ptr[cur_key_buffer_index] = scancode[cur_key_mode][cur_key];
    putc(key_buffer_ptr[cur_key_buffer_index]);
		cur_key_buffer_index++;
  }
	update_cursor();
  sti();
  return;
}

/*handle_enter
 *    DESCRIPTION: Handles condition when enter is pressed
 *    INPUTS: none
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Handles the situation accordingly when enter is pressed
 */
void handle_enter() {
		//enter_occured = 1;
		terminals[cur_active_term].enter = 1;
		new_line_char();
		update_cursor();
}


/*init_keys
 *    DESCRIPTION: initialize keyboard
 *    INPUTS: none
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: modify code
 */
void init_keys(void){

  enable_irq(KEYBOARD_IRQ);
	reset_left_corner();
  /* set every element in code to 0*/
}
