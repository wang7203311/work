# all linkages are defined in interrupt_table.h
.text
.global division_by_zero_linkage, single_step_interrupt_linkage, nmi_linkage, breakpoint_linkage,
.global overflow_linkage, bounds_linkage, invalid_opcode_linkage, coprocessor_not_available_linkage,
.global double_fault_linkage, coprocessor_segment_overrun_linkage,
.global invalid_task_state_segment_linkage, segment_not_present_linkage,
.global stack_fault_linkage, general_protection_fault_linkage
.global page_fault_linkage, math_fault_linkage, alignment_check_linkage, machine_check_linkage
.global SIMD_floating_point_exception_linkage, virtualization_exception_linkage, control_protection_exception_linkage

# use pushal for all registers
# use pushfl for all flags
division_by_zero_linkage:
    cli
    pushal
    pushfl
    call division_by_zero
    popfl
    popal
    sti
    iret

single_step_interrupt_linkage:
    cli
    pushal
    pushfl
    call single_step_interrupt
    popfl
    popal
    sti
    iret

nmi_linkage:
    cli
    pushal
    pushfl
    call nmi
    popfl
    popal
    sti
    iret

breakpoint_linkage:
    cli
    pushal
    pushfl
    call breakpoint
    popfl
    popal
    sti
    iret

overflow_linkage:
    cli
    pushal
    pushfl
    call overflow
    popfl
    popal
    sti
    iret

bounds_linkage:
    cli
    pushal
    pushfl
    call bounds
    popfl
    popal
    sti
    iret

invalid_opcode_linkage:
    cli
    pushal
    pushfl
    call invalid_opcode
    popfl
    popal
    sti
    iret

double_fault_linkage:
    cli
    pushal
    pushfl
    call double_fault
    popfl
    popal
    sti
    iret

coprocessor_segment_overrun_linkage:
    cli
    pushal
    pushfl
    call coprocessor_segment_overrun
    popfl
    popal
    sti
    iret

coprocessor_not_available_linkage:
    cli
    pushal
    pushfl
    call coprocessor_not_available
    popfl
    popal
    sti
    iret

invalid_task_state_segment_linkage:
    cli
    pushal
    pushfl
    call invalid_task_state_segment
    popfl
    popal
    sti
    iret

segment_not_present_linkage:
    cli
    pushal
    pushfl
    call segment_not_present
    popfl
    popal
    sti
    iret

stack_fault_linkage:
    cli
    pushal
    pushfl
    call stack_fault
    popfl
    popal
    sti
    iret

general_protection_fault_linkage:
    cli
    pushal
    pushfl
    call general_protection_fault
    popfl
    popal
    sti
    iret

page_fault_linkage:
    cli
    pushal
    pushfl
    call page_fault
    popfl
    popal
    sti
    iret

math_fault_linkage:
    cli
    pushal
    pushfl
    call math_fault
    popfl
    popal
    sti
    iret

alignment_check_linkage:
    cli
    pushal
    pushfl
    call alignment_check
    popfl
    popal
    sti
    iret

machine_check_linkage:
    cli
    pushal
    pushfl
    call machine_check
    popfl
    popal
    sti
    iret

SIMD_floating_point_exception_linkage:
    cli
    pushal
    pushfl
    call SIMD_floating_point_exception
    popfl
    popal
    sti
    iret

virtualization_exception_linkage:
    cli
    pushal
    pushfl
    call virtualization_exception
    popfl
    popal
    sti
    iret

control_protection_exception_linkage:
    cli
    pushal
    pushfl
    call control_protection_exception
    popfl
    popal
    sti
    iret
