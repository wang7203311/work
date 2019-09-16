/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"
/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/*Description: Initialize the 8259 PIC
* Input : none
* Output : none
* Return : none
* Side Effect: Initialize the slave pic and master
 */
void i8259_init(void) {

  /* Initialize PIC masks */
  master_mask = 0xFF;
  slave_mask = 0xFF;

  /* Initialize PIC masks */
  master_mask = 0xFF;
  slave_mask = 0xFF;

  /*mask all port of PICs*/
  outb(master_mask, MASTER_8259_PORT + DATA_PORT_OFFSET);//mask all of 8259A-1
  outb(slave_mask, SLAVE_8259_PORT + DATA_PORT_OFFSET);// mask all of 8259A-2

  //port 0
  /* Initialize ICW1*/
  outb(ICW1, MASTER_8259_PORT);
  outb(ICW1, SLAVE_8259_PORT);

  //port 1
  /* Initialize ICW2
  high bit of vector#*/
  outb(ICW2_MASTER,MASTER_8259_PORT + DATA_PORT_OFFSET);
  outb(ICW2_SLAVE, SLAVE_8259_PORT + DATA_PORT_OFFSET);

  //port 1
  /*Initialize ICW3
  master: bit vector of slave,
  slave: input pin on master*/
  outb(ICW3_MASTER, MASTER_8259_PORT + DATA_PORT_OFFSET); //8259A-1 has a slave on IR2
  outb(ICW3_SLAVE, SLAVE_8259_PORT + DATA_PORT_OFFSET); //8259A-2 is a slave on master's IR2

  //port 1
  /*Initialize ICW4
  ISA = x86, normal/auto EOI*/
  outb(ICW4, MASTER_8259_PORT + DATA_PORT_OFFSET);
  outb(ICW4, SLAVE_8259_PORT + DATA_PORT_OFFSET);
  enable_irq(SLAVE_IRQ_PORT);
}

/*Description: Enable (unmask) the specified IRQ
* Input : irq_num: the irq number we need to unmask
* Output : none
* Return : none
* Side Effect : unmask the specified irq
*/
void enable_irq(uint32_t irq_num) {
  if (irq_num >= PIC_NUM) { //check if it is slave or not
    slave_mask &= ~(0x01 << (irq_num - PIC_NUM));
    outb(slave_mask, SLAVE_8259_PORT + DATA_PORT_OFFSET);
  } else {
    master_mask &= ~(0x01 << (irq_num));
    outb(master_mask, MASTER_8259_PORT + DATA_PORT_OFFSET);
  }
}

/*Description: disable (mask) the specified IRQ
* Input : irq_num: the irq number we need to mask
* Output : none
* Return : none
* Side Effect : mask the specified irq
*/
void disable_irq(uint32_t irq_num) {
  if(irq_num >= PIC_NUM) {  //check if it is slave or not
    slave_mask |= (0x01 << (irq_num - PIC_NUM));
    outb(slave_mask, SLAVE_8259_PORT + DATA_PORT_OFFSET);
  } else {
    master_mask |= (0x01 << (irq_num));
    outb(master_mask, MASTER_8259_PORT + DATA_PORT_OFFSET);
  }
}

/* Description : end-of-interrupt signal for the specified IRQ
* Input : irq_num : the irq number we need to send back the EOI signal
* Output : none
* Return : none
* Side Effect : send the EOI signal
 */
void send_eoi(uint32_t irq_num) {
  // check from slave or master
  if(irq_num >= PIC_NUM) { // if irq come from salve
    outb(EOI|(irq_num - PIC_NUM),SLAVE_8259_PORT);
    outb(EOI + SLAVE_IRQ_PORT, MASTER_8259_PORT);
  }
  if(irq_num < PIC_NUM) {
    outb(EOI|irq_num, MASTER_8259_PORT);
  }
}
