#ifndef CORTEXOS_PIC_H
#define CORTEXOS_PIC_H

void pic_init(void);
void pic_mask(unsigned char mask_master, unsigned char mask_slave);
void pic_eoi(unsigned char irq);

#endif
