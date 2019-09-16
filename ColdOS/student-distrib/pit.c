#include "pit.h"
#include "paging.h"

#define ASM 1

/*init_pit
 * DESCRIPTION : Initialize PIT
 * INPUT : None
 * OUTPUT : None
 * RETURN VALUE: None
 */
int32_t terminal_index;
void init_pit() {
  /*initialize the terminal_index as 0*/
  terminal_index = 0;

  /*pit_call initialized as flag use for first three iteration*/
  pit_call = 0;

  /* 0x36 == 0b 0011 0110
   * Select channel |  Access mode    |  Operating mode | BCD/Binary mode
   *    Channel 0     lobyte/highbyte       Mode 3           Binary mode
   */
  outb(0x36, MODE_REGISTER);

  /* send lowest 8 bits */
  outb(PIT_HZ & 0xFF, CHANNEL_0_DATA);

  /* send highest 8 bits */
  outb(PIT_HZ >> _8, CHANNEL_0_DATA);

  /* enable IRQ */
  enable_irq(PIT_IRQ);
}

/*pit_handler
 * DESCRIPTION: function that is triggered when the interrupt occurs
 * INPUT: None
 * OUTPUT: None
 * RETURN VALUE: None
 */
void pit_handler() {
  /* variable declaration */
  pcb_t* prev_pcb, *cur_pcb;
  int prev;

  /* send EOI to acknowledge interrupt */
  cli();
  send_eoi( PIT_IRQ );
  sti();

  /* save current terminal_index */
  prev = terminal_index;

  /* update terminal_index to next terminal's index */
  terminal_index = (terminal_index + 1) % NUM_OF_TERMINALS;

  /* save current terminal's status */
  asm (
      "movl %%esp, %0;"
      "movl %%ebp, %1;"
      :"=r"(terminals[prev].term_esp), "=r"(terminals[prev].term_ebp)
      :
      :"cc"
    );

  /* next terminal has no running shell, we need to boot shell program first  */
  if(-1 == terminals[terminal_index].cur_process_index) {

    /* raise pit_call flag for execute() */
    pit_call = 1;

    /* boot shell */
    execute((uint8_t*) "shell");
  }

  /*
   * context switching section
   *
   * Due to the effect of Round Robin, we have booted shell for each terminal,
   * and we only need to remap paging, move esp, ebp, tss.esp0, and set RTC frequency
   * to perform context switching
   */

  /* lower pit_call flag since we are doing context switching */
  pit_call = 0;

  /* allocate paging */
  map_page(_8 * MB + terminals[terminal_index].cur_process_index * _4 * MB);

  /* allocate video memory, 12 bit to align the address */
  map_video_page((uint32_t)(VIDEO_MEM << 12), terminal_index);

  /* restore esp0 for next terminal */
  tss.esp0 =  _8 * MB - _8 * KB *terminals[terminal_index].cur_process_index - _4;

  /* get current terminal's pcb */
  cur_pcb = (pcb_t *)get_pcb_addr(terminals[terminal_index].cur_process_index );

  /* get previous terminal's pcb */
  prev_pcb = (pcb_t *)get_pcb_addr(terminals[prev].cur_process_index );

  /*
   * if we see the difference RTC frequency between current pcb and previous pcb,
   * we need to modify RTC frequency, otherwise we can skip this procedure
   */
  rtc_set_freq(cur_pcb->rtc_freq);

  /*
   * modify esp and ebp to shift to next terminal's stack
   *
   * since we store the terminal's esp and ebp in pit_handler() [line 56],
   * when we modify esp and ebp, the function call we are in should be
   * next terminal's interrupt call of pit_handler(). When we leave and ret,
   * we will return from this function call and get back to pit_linkage() from
   * interrupt_linkage.S. With the help of pre-pushed iret context by MMU, when
   * we iret, we will return to next terminal's process's user space, which is
   * the goal of context switching
   */
   asm (
    "movl %0, %%esp;"
    "movl %1, %%ebp;"
    "leave;"
    "ret;"
    : /* no output */
    : "r"(terminals[terminal_index].term_esp), "r"(terminals[terminal_index].term_ebp)
    : "cc", "%esp", "%ebp"
  );

}
