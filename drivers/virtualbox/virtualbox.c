#include "virtualbox.h"
#include "pci.h"
#include "console.h"
#include "kstring.h"
#include <stdint.h>

static bool detected;
static uint8_t device_bus, device_slot, device_function;
static uint32_t io_base;

void virtualbox_init(void)
{
	detected = false;
	for (int i = 0; i < pci_get_device_count(); ++i) {
		const struct pci_device *dev = pci_get_device(i);
		if (!dev || dev->vendor_id != VBOX_VENDOR_ID ||
			dev->device_id != VBOX_VMMDEV_DEVICE_ID)
			continue;
		device_bus = dev->bus;
		device_slot = dev->device;
		device_function = dev->function;
		io_base = pci_config_read(device_bus, device_slot, device_function, 0x10);
		if (io_base & 1u)
			io_base &= 0xFFFCu;
		else
			io_base = 0;
		detected = true;
		break;
	}
	console_print(detected ? "VirtualBox VMMDev detected\n" :
		"VirtualBox VMMDev not detected\n");
}

bool virtualbox_present(void)
{
	return detected;
}

void virtualbox_print_info(void)
{
	char buf[16];
	if (!detected) {
		console_print("VirtualBox: VMMDev not detected\n");
		return;
	}
	console_print("VirtualBox VMMDev\n  PCI: ");
	kitoa(device_bus, buf, sizeof(buf)); console_print(buf); console_print(":");
	kitoa(device_slot, buf, sizeof(buf)); console_print(buf); console_print(".");
	kitoa(device_function, buf, sizeof(buf)); console_print(buf);
	console_print("\n  I/O BAR: ");
	if (io_base) { kxtoa(io_base, buf, sizeof(buf)); console_print("0x"); console_print(buf); }
	else console_print("unavailable (MMIO BAR or disabled)");
	console_print("\n  Status: detection only; VMMDev protocol is not enabled\n");
}
