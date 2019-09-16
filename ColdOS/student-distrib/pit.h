
/* Reference Pages:
 *     http://www.osdever.net/bkerndev/Docs/pit.htm
 *     https://wiki.osdev.org/Programmable_Interval_Timer
 */
#ifndef _PIT_H
#define _PIT_H

#include "lib.h"
#include "i8259.h"
#include "types.h"
#include "terminal.h"

/* read/write ports */
#define CHANNEL_0_DATA 0x40
#define CHANNEL_1_DATA 0x41
#define CHANNEL_2_DATA 0x42

/* The Mode/Command register ( write only port, a read is ignored )
 * Bits      ---    Usage
 * 6 and 7   ---    Select channel: ( which channel is configured )
 *                         0 0 = Channel 0
 *                         0 1 = Channel 1
 *                         1 0 = Channel 2
 *                         1 1 = Read-back command
 * 4 and 5    ---   Access mode: ( access mode to use for the selected channel )
 *                         0 0 = Latch count value command
 *                         0 1 = lobyte only ( only the lowest 8 bits of the
 *                                             counter value is read/written
 *                                             to/from the data port )
 *                         1 0 = hibyte only ( only the highest 8 bits of the
 *                                             counter value is read/written
 *                                             to/from the data port )
 *                         1 1 = lobyte/highbyte ( 16 bits are always transferred
 *                                                 as a pair, with the lowest 8
 *                                                 bits followed by the hightest
 *                                                 8 bits )
 * 1 to 3     ---   Operating mode: ( operate mode of the selected channel )
 *                       0 0 0 = Mode 0 ( interrupt on terminal count )
 *                       0 0 1 = Mode 1 ( hardware re-triggerable one-shot )
 *                       0 1 0 = Mode 2 ( rate generator )
 *                       0 1 1 = Mode 3 ( square wave generator )
 *                       1 0 0 = Mode 4 ( software triggered strobe )
 *                       1 0 1 = Mode 5 ( hardware triggerd strobe )
 *                       1 1 0 = Mode 2 ( rate generator, same as 010b )
 *                       1 1 1 = Mode 3 ( square wave generator, same as 011b )
 * 0          ---   BCD/Binary mode: ( determine binary/BCD[1] mode of the
 *                                     selected channel operates )
 *                           0 = 16-bit binary
 *                           1 = four-digit BCD
 *
 * BCD[1]: each 4 bits of the counter represent a decimal digit, and the counter
 *         holds values from '0000' to '9999'.
 */
#define MODE_REGISTER 0x43

extern int32_t terminal_index;

int32_t pit_call;

void init_pit();

void pit_handler();


#endif /* _PIT_H */
