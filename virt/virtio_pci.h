#ifndef CORTEXOS_VIRTIO_PCI_H
#define CORTEXOS_VIRTIO_PCI_H

#include <stdint.h>
#include "virtio.h"

struct virtio_pci_scan_result {
    struct virtio_pci_device devices[32];
        uint8_t count;
        };

        /* Scan the conventional PCI bus and collect legacy VirtIO devices. */
        int virtio_pci_scan(struct virtio_pci_scan_result *result);

        /* Read/write the legacy PCI configuration mechanism (CF8/CFC). */
        uint32_t virtio_pci_config_read(uint8_t bus, uint8_t slot,
                                        uint8_t function, uint8_t offset);
                                        void virtio_pci_config_write(uint8_t bus, uint8_t slot,
                                                                     uint8_t function, uint8_t offset,
                                                                                                  uint32_t value);

                                                                                                  #endif
                                                                                                  