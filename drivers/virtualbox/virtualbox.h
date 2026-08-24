#ifndef VIRTUALBOX_H
#define VIRTUALBOX_H

#include <stdbool.h>

/* Oracle VirtualBox VMMDev PCI identity. */
#define VBOX_VENDOR_ID 0x80EEu
#define VBOX_VMMDEV_DEVICE_ID 0xCAFEu

void virtualbox_init(void);
bool virtualbox_present(void);
void virtualbox_print_info(void);

#endif
