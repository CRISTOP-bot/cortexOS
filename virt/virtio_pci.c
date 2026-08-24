#include "virtio_pci.h"

#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA    0xCFCu
#define PCI_VENDOR_INVALID 0xFFFFu

static inline void pci_out32(uint16_t port, uint32_t value)
{
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t pci_in32(uint16_t port)
{
    uint32_t value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static uint32_t pci_address(uint8_t bus, uint8_t slot,
                            uint8_t function, uint8_t offset)
{
    return 0x80000000u | ((uint32_t)bus << 16) |
           ((uint32_t)slot << 11) | ((uint32_t)function << 8) |
           (offset & 0xFCu);
}

uint32_t virtio_pci_config_read(uint8_t bus, uint8_t slot,
                                uint8_t function, uint8_t offset)
{
    pci_out32(PCI_CONFIG_ADDRESS, pci_address(bus, slot, function, offset));
    return pci_in32(PCI_CONFIG_DATA);
}

void virtio_pci_config_write(uint8_t bus, uint8_t slot,
                             uint8_t function, uint8_t offset,
                             uint32_t value)
{
    pci_out32(PCI_CONFIG_ADDRESS, pci_address(bus, slot, function, offset));
    pci_out32(PCI_CONFIG_DATA, value);
}

int virtio_pci_scan(struct virtio_pci_scan_result *result)
{
    if (!result)
        return -1;

    result->count = 0;
    for (uint16_t bus = 0; bus < 256 && result->count < 32; ++bus) {
        for (uint8_t slot = 0; slot < 32 && result->count < 32; ++slot) {
            uint32_t first_id = virtio_pci_config_read(
                (uint8_t)bus, slot, 0, 0);
            if ((uint16_t)(first_id & 0xFFFFu) == PCI_VENDOR_INVALID)
                continue;

            uint32_t header = virtio_pci_config_read(
                (uint8_t)bus, slot, 0, 0x0C);
            uint8_t functions = (header & 0x00800000u) ? 8 : 1;
            for (uint8_t function = 0;
                 function < functions && result->count < 32; ++function) {
                uint32_t id = (function == 0)
                    ? first_id
                    : virtio_pci_config_read(
                        (uint8_t)bus, slot, function, 0);
                uint16_t vendor = (uint16_t)(id & 0xFFFFu);
                uint16_t device = (uint16_t)(id >> 16);

                if (vendor == PCI_VENDOR_INVALID ||
                    !virtio_is_device(vendor, device))
                    continue;

                uint32_t bar0 = virtio_pci_config_read(
                    (uint8_t)bus, slot, function, 0x10);
                if (!(bar0 & 1u))
                    continue;

                struct virtio_pci_device *found =
                    &result->devices[result->count];
                found->vendor_id = vendor;
                found->device_id = device;
                found->io_base = (uint16_t)(bar0 & 0xFFFCu);
                found->irq_line = (uint8_t)(virtio_pci_config_read(
                    (uint8_t)bus, slot, function, 0x3C) & 0xFFu);
                found->present = 1;
                result->count++;
            }
        }
    }

    return result->count;
}
