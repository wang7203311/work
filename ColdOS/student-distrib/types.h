/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

/* general defines */
#define NULL 0

#define MB 0x100000
#define KB 0x400
#define B 0x1

#define _16 0x10
#define _8 0x8
#define _4 0x4

#define EXIT_SUCCESS 0
#define EXIT_FAILURE -1
/* end of general defines */

/* lib.h */
#define NUM_COLS    80
#define NUM_ROWS    25

/* end of lib.h */

/* interrupt_table.h */

/* list of entries in IDT for each device and systemcall */
#define SYSCALL_ENTRY 0x80
#define PIT_ENTRY 0x20
#define KEYBOARD_ENTRY 0x21
#define RTC_ENTRY 0x28

/* end of interrupt_table.h */


/* systemcall.h */

/* file type classification */
#define TYPE_RTC 0
#define TYPE_DIRECOTRY 1
#define TYPE_REGULAR_FILE 2

/* signal enums
 *
 * Kill the task : DIV_ZERO, SEGFAULT, INTERRUPT
 * Ignore : ALARM, USER1
 */
#define DIV_ZERO 0
#define SEGFAULT 1
#define INTERRUPT 2
#define ALARM 3
#define USER1 4

/*
 * ELF MAGIC NUMBER
 * reverse order of each magic number due to little endien
 */
#define MAGIC_NUM 0x464c457f

/* limits */
#define MAX_ARG_SIZE 100
#define MAX_CMD_SIZE 32
#define MAX_NUM_FILE 8
#define MAX_FILE_SIZE 100000


/* end of systemcall.h */

/* rtc.h */

/* IRQ for RTC, first port on slave PIC */
#define RTC_IRQ 8

/* macros used when modifying RTC frequency */
#define FREQUENCY_UP_BOUND 1024
#define FREQUENCY_LOW_BOUND 2
#define FREQUENCY_DEFAULT 2

/* specific register macros are listed in rtc.h instead */

/* end of rtc.h */

/* paging.h */

/* Maximum number of tasks that we allow */
#define MAX_TASK 6

/* since our page directory and page table have same page size,
 *
 * entry number and size are same
 */
#define NUM_ENTRIES 1024
#define NUM_SIZE 4096

/* Video Memory's page offset */
#define VIDEO_MEM 0xB8

/* defined for mapping program pages and video pages */

/* Used to map program page to the user program
 *
 * 128MB(starting virtual addr) / 4MB(page size) = 32
 */
#define VIRTUAL_ENTRY 32

/* Used to map video memory page to the user space
 *
 * 133MB(starting virtual addr) / 4MB(page size) ≈ 33
 */
#define VIRTUAL_VIDEO_ENTRY 33

/* end of paging.h */

/* filesystem.h */

/* macros of offsets */
#define DIRECTOTY_OFF 64
#define INODE_OFFSET 36

/* filename limit */
#define FILE_NAME_SIZE 32

/* end of filesystem.h */

/* keyboard.h */

/* limit of keyboard buffer */
#define KEY_SUM 128

/* IRQ for keyboard interrupt */
#define KEYBOARD_IRQ 1

/* ports for keyboard interrupt */
#define KEYBOARD_DATA 0x60
#define KEYBOARD_STATUS 0x64

/* end of keyboard.h */

/* pit.h */

/* IRQ for PIT interrupt */
#define PIT_IRQ 0

/*
 * 10 ms == 100 hz
 * 50 ms == 20 hz
 */

/* default PIT frequency */
#define DEFAULT_HZ 1193180

/* hard code frequency to 20, we use 40 as divisor because of
 * Mode 3 (squre wave generator ) */
#define PIT_HZ ( DEFAULT_HZ / 40 )

/* end of pit.h */

/* terminal.h */
#define NUM_OF_TERMINALS 3

/* 1 byte for color and 1 byte for character */
#define VIDEO_CHAR_LENGTH 2

#define VIDEO_MEM_SPACE (VIDEO_CHAR_LENGTH * NUM_COLS * NUM_ROWS)

#define FIRST_KERNEL_PAGE_ADDR 0x400000

#define VIDEO_MEM_POINTER 0xB8000
/* end of terminal.h */

#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

#endif /* ASM */

#endif /* _TYPES_H */
