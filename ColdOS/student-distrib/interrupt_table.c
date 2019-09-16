#include "interrupt_table.h"
#include "lib.h"
#include "x86_desc.h"
#include "rtc.h"
#include "keyboard.h"
#include "interrupt_linkage.h"
#include "systemcall.h"

#define THROW_EXCEPTION(argument_handler, argument) \
void argument_handler() {                           \
  printf("%s\n", argument);                         \
  halt(RETURN_FROM_EXCEPTION);                      \
}                                                   \

THROW_EXCEPTION(division_by_zero, "Division by zero");
THROW_EXCEPTION(single_step_interrupt, "Single-step interrupt");
THROW_EXCEPTION(nmi, "NMI");
THROW_EXCEPTION(breakpoint, "Breakpoint");
THROW_EXCEPTION(overflow, "Overflow");
THROW_EXCEPTION(bounds, "Bounds");
THROW_EXCEPTION(invalid_opcode, "Invalid Opcode");
THROW_EXCEPTION(coprocessor_not_available, "Coprocessor not available");
THROW_EXCEPTION(double_fault, "Double fault");
THROW_EXCEPTION(coprocessor_segment_overrun, "Coprocessor Segment Overrun");
THROW_EXCEPTION(invalid_task_state_segment, "Invalid Task State Segment");
THROW_EXCEPTION(segment_not_present, "Segment not present");
THROW_EXCEPTION(stack_fault, "Stack Fault");
THROW_EXCEPTION(general_protection_fault, "General protection fault");
THROW_EXCEPTION(page_fault, "Page Fault");
// THROW_EXCEPTION(reserved, "Reserved");
THROW_EXCEPTION(math_fault, "Math Fault");
THROW_EXCEPTION(alignment_check, "Alignment Check");
THROW_EXCEPTION(machine_check, "Machine Check");
THROW_EXCEPTION(SIMD_floating_point_exception, "SIMD Floating-Point Exception");
THROW_EXCEPTION(virtualization_exception, "Virtualization Exception");
THROW_EXCEPTION(control_protection_exception, "Control Protection Exception");


/*set_trap_mode
 *    DESCRIPTION: set the given descriptor into trap gate mode
 *    INPUTS: descriptor -- a point of idt table
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: modify the values in the descriptor
 *
 *      uint8_t  reserved4 = 0;
 *      uint32_t reserved3 = 1;
 *      uint32_t reserved2 = 1;
 *      uint32_t reserved1 = 1;
 *      uint32_t size      = 1;
 *      uint32_t reserved0 = 0;
 *      uint32_t dpl       = 0;
 *      uint32_t present   = 1;
 */
void set_trap_mode(void *descriptor_ptr) {
  idt_desc_t* descriptor = (idt_desc_t *) descriptor_ptr;
  descriptor->reserved4 = 0; // there are 5 bits are reserved
  descriptor->reserved3 = 1;
  descriptor->reserved2 = 1;
  descriptor->reserved1 = 1;
  descriptor->size =  1;
  descriptor->reserved0 = 0;
  descriptor->dpl = 0;
  descriptor->present = 1;
  descriptor->seg_selector = KERNEL_CS;
}

/*set_exceptions
 *     DESCRIPTION: set system exceptions
 *     INPUTS: none
 *     OUTPUTS: none
 *     RETURN VALUES: none
 *     SIDE EFFECTS: set first few IDT entries
 *
 * 0x00	Division by zero
 * 0x01	Single-step interrupt (see trap flag)
 * 0x02	NMI
 * 0x03	Breakpoint (callable by the special 1-byte instruction 0xCC, used by debuggers)
 * 0x04	Overflow
 * 0x05	Bounds
 * 0x06	Invalid Opcode
 * 0x07	Coprocessor not available
 * 0x08	Double fault
 * 0x09	Coprocessor Segment Overrun (386 or earlier only)
 * 0x0A	Invalid Task State Segment
 * 0x0B	Segment not present
 * 0x0C	Stack Fault
 * 0x0D	General protection fault
 * 0x0E	Page fault
 * 0x0F	reserved
 * 0x10	Math Fault
 * 0x11	Alignment Check
 * 0x12	Machine Check
 * 0x13	SIMD Floating-Point Exception
 * 0x14	Virtualization Exception
 * 0x15	Control Protection Exception
 */
void set_exceptions() {
  SET_IDT_ENTRY(idt[0x00], division_by_zero_linkage);
  SET_IDT_ENTRY(idt[0x01], single_step_interrupt_linkage);
  SET_IDT_ENTRY(idt[0x02], nmi_linkage);
  SET_IDT_ENTRY(idt[0x03], breakpoint_linkage);
  SET_IDT_ENTRY(idt[0x04], overflow_linkage);
  SET_IDT_ENTRY(idt[0x05], bounds_linkage);
  SET_IDT_ENTRY(idt[0x06], invalid_opcode_linkage);
  SET_IDT_ENTRY(idt[0x07], coprocessor_not_available_linkage);
  SET_IDT_ENTRY(idt[0x08], double_fault_linkage);
  SET_IDT_ENTRY(idt[0x09], coprocessor_segment_overrun_linkage);
  SET_IDT_ENTRY(idt[0x0A], invalid_task_state_segment_linkage);
  SET_IDT_ENTRY(idt[0x0B], segment_not_present_linkage);
  SET_IDT_ENTRY(idt[0x0C], stack_fault_linkage);
  SET_IDT_ENTRY(idt[0x0D], general_protection_fault_linkage);
  SET_IDT_ENTRY(idt[0x0E], page_fault_linkage);
  // SET_IDT_ENTRY(idt[0x0F], reserved); this is reserved
  SET_IDT_ENTRY(idt[0x10], math_fault_linkage);
  SET_IDT_ENTRY(idt[0x11], alignment_check_linkage);
  SET_IDT_ENTRY(idt[0x12], machine_check_linkage);
  SET_IDT_ENTRY(idt[0x13], SIMD_floating_point_exception_linkage);
  SET_IDT_ENTRY(idt[0x14], virtualization_exception_linkage);
  SET_IDT_ENTRY(idt[0x15], control_protection_exception_linkage);
}

/*init_idt
 *    DESCRIPTION: Initialize IDT, this function should be called in boot.S
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUES: none
 *    SIDE EFFECTS: Setup IDT
 *
 *   struct {
 *         uint16_t offset_15_00;
 *         uint16_t seg_selector;
 *         uint8_t  reserved4;
 *         uint32_t reserved3 : 1;
 *         uint32_t reserved2 : 1;
 *         uint32_t reserved1 : 1;
 *         uint32_t size      : 1;
 *         uint32_t reserved0 : 1;
 *         uint32_t dpl       : 2;
 *         uint32_t present   : 1;
 *         uint16_t offset_31_16;
 *     } __attribute__ ((packed));
 */
void init_idt(void) {
  uint16_t index;

  // Initialize the whole IDT
  for(index = 0; index < NUM_VEC; index++) {
    set_trap_mode(&idt[index]);

    /* First 32 are general exception
     * Others are not
     */
    if (index >= 32 && index != SYSCALL_ENTRY ) {
      idt[index].reserved3 = 0;
    }
  }

  /* SYSCALLs are user functions , so their dpl should be set as 3 (0b11) */
  idt[ SYSCALL_ENTRY ].dpl = 3;

  /* add first few exceptions */
  set_exceptions();

  /* set RTC entry, view IDT diagram for magic number;) */
  SET_IDT_ENTRY(idt[ RTC_ENTRY ], rtc_linkage);
  SET_IDT_ENTRY(idt[ KEYBOARD_ENTRY ], keyboard_linkage);
  SET_IDT_ENTRY(idt[ SYSCALL_ENTRY ], sys_call_linkage);
  SET_IDT_ENTRY(idt[ PIT_ENTRY ], pit_linkage);
  /* load IDT */
  lidt(idt_desc_ptr);
}
