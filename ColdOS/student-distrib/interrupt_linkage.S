.text
.globl keyboard_linkage, sys_call_linkage, rtc_linkage, pit_linkage

keyboard_linkage:
	cli
	pushal
	pushfl
	call keyboard_handler
	popfl
	popal
	sti
	iret

rtc_linkage:
	cli
	pushal
	pushfl
	call rtc_handler
	popfl
	popal
	sti
	iret

pit_linkage:
	cli
	pushal
	pushfl
	call pit_handler
	popfl
	popal
	sti
	iret


sys_call_linkage:
	cli
	# Save all registers and flags, %eax is not needed since it is the return value
	pushl %fs
	pushl %es
  pushl %ds
  pushl %ebx
  pushl %ecx
  pushl %edx
  pushl %esi
  pushl %edi
  pushl %ebp
  pushfl

	# push args
	pushl %edx		#arg 3
  pushl %ecx 		#arg 2
  pushl %ebx		#arg 1


	# Check if it is out of the range of system call number
	# the lowest index for our system call is 1
  cmpl $1, %eax
  jl invalid
	# we have in total 10 system calls
  cmpl $10, %eax
  jg invalid

	# Call the correct system call according to the jumptable
	sti
  call *jumptable(,%eax,4)
	cli
  jmp restore

invalid:
 	movl $-1, %eax

restore:
  # pop args
	popl %ebx
	popl %ecx
	popl %edx

  # Restore all regs, except for eax, and flags
  popfl
  popl %ebp
  popl %edi
  popl %esi
  popl %edx
 	popl %ecx
  popl %ebx
  popl %ds
  popl %es
	popl %fs
	sti
  # return from interrupt
  iret

jumptable:
	.long 0x0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
