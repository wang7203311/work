#ifndef _INTERRUPT_TABLE_H
#define _INTERRUPT_TABLE_H

#include "keyboard.h"
#include "rtc.h"
#include "pit.h"
#include "types.h"

/* function that should be called in boot.S */
void init_idt(void);

/* helper function of setting to trap gate mode */
void set_trap_mode(void *descriptor_ptr);

/* helper function of setting first few exceptions */
void set_exceptions(void);

/* Those functions are created using THROW_EXCEPTION()*/
void division_by_zero();
void single_step_interrupt();
void nmi();
void breakpoint();
void overflow();
void bounds();
void invalid_opcode();
void coprocessor_not_available();
void double_fault();
void coprocessor_segment_overrun();
void invalid_task_state_segment();
void segment_not_present();
void stack_fault();
void general_protection_fault();
void page_fault();
//void reserved();
void math_fault();
void alignment_check();
void machine_check();
void SIMD_floating_point_exception();
void virtualization_exception();
void control_protection_exception();


/* global function for linkage purpose */
extern void rtc_linkage();
extern void pit_linkage();
extern void keyboard_linkage();
extern void division_by_zero_linkage();
extern void single_step_interrupt_linkage();
extern void nmi_linkage();
extern void breakpoint_linkage();
extern void overflow_linkage();
extern void bounds_linkage();
extern void invalid_opcode_linkage();
extern void coprocessor_not_available_linkage();
extern void double_fault_linkage();
extern void coprocessor_segment_overrun_linkage();
extern void invalid_task_state_segment_linkage();
extern void segment_not_present_linkage();
extern void stack_fault_linkage();
extern void general_protection_fault_linkage();
extern void page_fault_linkage();
extern void math_fault_linkage();
extern void alignment_check_linkage();
extern void machine_check_linkage();
extern void SIMD_floating_point_exception_linkage();
extern void virtualization_exception_linkage();
extern void control_protection_exception_linkage();

#endif
