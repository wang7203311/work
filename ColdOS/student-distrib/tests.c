#include "tests.h"
#include "x86_desc.h"
#include "terminal.h"
#include "lib.h"
#include "rtc.h"
#include "filesystem.h"


#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* EXCEPTION Test - DIVISION_BY_ZERO
 *
 * CREATE division_by_zero EXCEPTION
 * Inputs: None
 * Outputs: FAIL
 * Side Effects: Freeze screen, and deny any input
 * Coverage: Load IDT, IDT definition
 * Files: interrupt_table.c/h
 */
int exception_division_by_zero(){
	TEST_HEADER;
	int a = 0;
	int b = 0;
	b = a / b; // DIVISION_BY_ZERO
	return FAIL;
}

/* EXCEPTION Test - NMI
 *
 * CREATE NMI EXCEPTION
 * Inputs: None
 * Outputs: FAIL
 * Side Effects: Freeze screen, and deny any input
 * Coverage: Load IDT, IDT definition
 * Files: interrupt_table.c/h
 */
int exception_nmi() {
	TEST_HEADER;
	asm("int $2");
	return FAIL;
}

/* EXCEPTION Test - bounds
 *
 * CREATE bounds EXCEPTION
 * Inputs: None
 * Outputs: FAIL
 * Side Effects: Freeze screen, and deny any input
 * Coverage: Load IDT, IDT definition
 * Files: interrupt_table.c/h
 */
int exception_bounds() {
	TEST_HEADER;
	asm("int $5");
	return FAIL;
}

/* Paging test - page fault
 * CREATE page fault
 * INPUTS: None
 * OUTPUS: Fail
 * SIDE Effects: Freeze screen, and deny any input
 * Coverage: Load IDT, IDT definition, Page initialization
 * Files: interrupt_table.c/h, paging.c/h
 */
int page_fault_test() {
	TEST_HEADER;
	int * a = (int *)0x00;
	printf("%d\n", *a);
	return FAIL;
}

int page_fault_test2(){
	TEST_HEADER;
	int * a = (int *)0x800000;
	printf("%d\n", *a);
	return FAIL;
}

int page_not_fault_test() {
	TEST_HEADER;
	int * a = (int*)0xB8000;
	printf("%d\n", *a);
	return PASS;
}



/* Checkpoint 2 tests */
// int terminal_read_write_test(){
// 	uint8_t buf[128];
// 	memset(buf,0,128);
// 	terminal_read((uint32_t)0,buf,(uint32_t)0);
// 	terminal_write((uint32_t)0,buf,(uint32_t)0);
// 	return PASS;
// }


/* Checkpoint 3 tests */

int execute_shell(){
	TEST_HEADER;
	execute((uint8_t*)"shell");
	return PASS;
}

int execute_ls(){
	TEST_HEADER;
	execute((uint8_t*)"ls");
	return PASS;
}

int execute_pingpong(){
	TEST_HEADER;
	execute((uint8_t*)"pingpong");
	return PASS;
}

int execute_testprint(){
	TEST_HEADER;
	execute((uint8_t*)"testprint");
	return PASS;
}
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	/*checkpoint 1 tests*/
	//TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
	// TEST_OUTPUT("exception_nmi", exception_nmi());
	// TEST_OUTPUT("exception_bounds", exception_bounds());
	// TEST_OUTPUT("page_fault_test", page_fault_test());
	// TEST_OUTPUT("page_fault_test", page_fault_test2());
	//TEST_OUTPUT("page_not_fault_test", page_not_fault_test());
	//TEST_OUTPUT("terminal_test", terminal_read_write_test());
	
	/*checkpoint 3 tests*/
	// TEST_OUTPUT("test_shell", execute_shell());
	// TEST_OUTPUT("test_print", execute_testprint());
	//TEST_OUTPUT("test_ls", execute_ls());
	//TEST_OUTPUT("pingpong", execute_pingpong());
}
