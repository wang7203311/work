#include "rtc.h"


/*init_rtc
 *    DESCRIPTION: initialize RTC
 *    INPUTS: none
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: modify registers
 */
void init_rtc() {

  unsigned char old;
  outb(REGISTER_B, RTC_PORT); //disable NMI and sellect register b

  old = inb(CMOS_PORT);
  outb(REGISTER_B, RTC_PORT);
  outb(old|PIE_BIT, CMOS_PORT); //turn on the 6th bit of the control_rgt_b

  rtc_interupt_occurred = 0; // initialization for the volatile flag
  enable_irq(RTC_IRQ);
}

/*rtc_handler
 *    DESCRIPTION: RTC interrupt handler
 *    INPUTS: none
 *    OUTPUS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: called when the interrupt occur, send EOI and set registers
 */
 void rtc_handler() {
   cli();
   send_eoi(RTC_IRQ);

   /* interact with rtc_read(), raise flag */
   rtc_interupt_occurred = 1;
   sti();
   /* Enable for test purpose
    test_interrupts();*/

   /*status register C will contain a bitmask telling which interrupt happen*/
   outb(REGISTER_C, RTC_PORT); // select register c
   inb(CMOS_PORT); // read register c in order to get more interrupt
 }

/*rtc_set_freq
 *    DESCRIPTION: set RTC frequency
 *    INPUTS: freq - new frequency, this value should be in the
 *                   range of 2Hz to 256Hz, setting the frequency to
 *                   0 disables RTC
 *    OUTPUS: none
 *    RETURN VALUES: return 0 on success, return -1 on invalid input
 *    SIDE EFFECTS: modify RTC frequency
 */
 int32_t rtc_set_freq(int32_t freq) {

   /* 0x10 = 0b10000
    * It takes 1 right shift for 2 Hz reaches to 1 Hz
    * 0x0F + 0x01 = 0x10
    */
   int32_t rate = 0x10;
   char prev;

   /* The maximum frequency is 1024 Hz, the minimum frequency is 2 Hz*/
   if (freq > FREQUENCY_UP_BOUND || freq < FREQUENCY_LOW_BOUND) {
     return -1;
   }

   /* frequency (Hz) |  rate
    *       2        |  0b1111 (0x0F)
    *       4        |  0b1110 (0x0E)
    *       8        |  0b1101 (0x0D)
    *      16        |  0b1100 (0x0C)
    *      32        |  0b1011 (0x0B)
    *      ...       |    ...
    *     1024       |  0b0110 (0x06)
    */
   while(!(freq & 0x1)) { // continue when frequency is even
     freq >>= 1;
     rate --;
   }

   /* frequency is not a power of 2*/
   if (freq != 1) {
     return -1;
   }

   cli();
   outb(REGISTER_A, RTC_PORT);
   prev = inb(CMOS_PORT);
   outb(REGISTER_A, RTC_PORT);
   /* rate is the bottom 4 bits */
   outb((prev & 0xF0) | rate, CMOS_PORT);
   sti();

   return 0;
 }


/*rtc_read
 *     DESCRIPTION: wait until the next RTC interupt has occured before it returns
 *     INPUTS: *ignored*
 *     OUTPUTS: none
 *     RETURN VALUES: always return 0
 *     SIDE EFFECT: modify RTC frequency
 */
 int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {

   /* lower the flag*/
   rtc_interupt_occurred = 0;

   while(!rtc_interupt_occurred) {
     /* Wait until the next RTC interrupt occured*/
   }

   return 0;
 }


 /*rtc_write
  *     DESCRIPTION: Write data to the RTC, the system call should always accept only a 4-bytes
  *                  integer specifying the interrupt rate in Hz, and should set the rate of periodic
  *                  interrupts accordingly.
  *     INPUTS: buf -- frequency
  *             nbytes -- should be 4
  *     OUTPUTS: none
  *     RETURN VALUES: return the number of bytes written, or -1 on failure
  *     SIDE EFFECT: modify RTC frequency
  */
  int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    /* variable declaration */
    int32_t cur_pid;
    pcb_t* cur_pcb;

    /* only accept a 4-bytess integer, and reasonable interrupt rate*/
    if (nbytes != 4 || buf == NULL) {
      return EXIT_FAILURE;
    }

    /* get current pcb */
    cur_pid = get_pcb();
    cur_pcb = (pcb_t *) get_pcb_addr(cur_pid);

    /* store RTC frequency in pcb struct */
    cur_pcb ->rtc_freq = *(int32_t *) buf;

    /*
     * though rtc_write() should modify RTC frequency, we can shift this
     * procedure into pit_handler() in pit.c with the help of context switching
     * to save some code
     */

    return nbytes;
  }

/*rtc_open
 *     DESCRIPTION: initialize RTC frequency to 2 Hz
 *     INPUTS: *ignored*
 *     OUTPUTS: none
 *     RETURN VALUES: always return 0
 *     SIDE EFFECT: modify RTC frequency
 */
  int32_t rtc_open(const uint8_t* filename) {
    rtc_set_freq(FREQUENCY_DEFAULT);
    return 0;
  }

/*rtc_close
 *     DESCRIPTION: Does nothing, unless you virtualize RTC
 *     INPUTS: *ignored*
 *     OUTPUTS: none
 *     RETURN VALUES: return 0
 *     SIDE EFFECT: none
 */
  int32_t rtc_close(int32_t fd) {
    return 0;
  }
