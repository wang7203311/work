#ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "i8259.h"
#include "lib.h"
#include "systemcall.h"

/* 6th bit in REGISTER_B */
#define PIE_BIT 0x40
#define RTC_PORT 0x70 //specify the index and disable NMI
#define CMOS_PORT 0x71
/* Macros of status registers
 * References:
 *     https://www.compuphase.com/int70.txt
 *     https://wiki.osdev.org/RTC
 *     http://stanislavs.org/helppc/cmos_ram.html
 */

/* used to select an interrupt rate, the lower four bits (0-3)
 * of register A select a divider for the 32,768 Hz frequency (The basic oscillator frequency).
 * The system initializes these bits to 0110 binary (or 6 decimal)
 * => 1,024 frequency (and interrupt rate)
 *
 * |7|6|5|4|3|2|1|0|  RTC Status Register A
 *  | | | | `---------- rate selection Bits for divider output
 *  | | | |		 frequency (set to 0110 = 1.024kHz, 976.562æs)
 *  | `-------------- 22 stage divider, time base being used;
 *  |			  (initialized to 010 = 32.768kHz)
 *  `-------------- 1=time update in progress, 0=time/date available
 */
#define REGISTER_A 0x8A

/* contains a number of flags
 *
 * |7|6|5|4|3|2|1|0|  RTC Status Register B
 *  | | | | | | | `---- 1=enable daylight savings, 0=disable (default)
 *  | | | | | | `----- 1=24 hour mode, 0=12 hour mode (24 default)
 *  | | | | | `------ 1=time/date in binary, 0=BCD (BCD default)
 *  | | | | `------- 1=enable square wave frequency, 0=disable
 *  | | | `-------- 1=enable update ended interrupt, 0=disable
 *  | | `--------- 1=enable alarm interrupt, 0=disable
 *  | `---------- 1=enable periodic interrupt, 0=disable
 *  `----------- 1=disable clock update, 0=update count normally
 */
#define REGISTER_B 0x8B

/* holds a bit mask that tells what kind of interupt occurred:
 * periodic interrupt, alarm interrupt or update ended interrupt.
 * need to read from this port to generate more interupt
 *
 * |7|6|5|4|3|2|1|0|  RTC Status Register C  (read only)
 *  | | | | `---------- reserved (set to 0)
 *  | | | `---------- update ended interrupt enabled
 *  | | `---------- alarm interrupt enabled
 *  | `---------- periodic interrupt enabled
 *  `---------- IRQF flag
 */
 #define REGISTER_C 0x0C

 volatile int rtc_interupt_occurred;

/* RTC initialization */
void init_rtc();

/* RTC interrupt handler */
void rtc_handler();

/* set freqency*/
int32_t rtc_set_freq(int32_t freq);

/* System Calls */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t rtc_open(const uint8_t* filename);

int32_t rtc_close(int32_t fd);
/* end of system calls */

#endif /* _RTC_H */
