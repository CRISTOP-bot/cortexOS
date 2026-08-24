#ifndef CORTEXOS_VIRTIO_H
#define CORTEXOS_VIRTIO_H

#include <stdint.h>
#include <stddef.h>

/* VirtIO legacy constants shared by the early x86_64 drivers. */
#define VIRTIO_VENDOR_ID          0x1AF4u
#define VIRTIO_PCI_LEGACY         0x1000u
#define VIRTIO_DEVICE_NET         1u
#define VIRTIO_DEVICE_BLOCK       2u
#define VIRTIO_DEVICE_CONSOLE     3u
#define VIRTIO_DEVICE_ENTROPY     4u
#define VIRTIO_DEVICE_BALLOON     5u
#define VIRTIO_DEVICE_9P          9u

#define VIRTQ_DESC_F_NEXT         1u
#define VIRTQ_DESC_F_WRITE        2u
#define VIRTQ_DESC_F_INDIRECT     4u

#define VIRTQ_AVAIL_F_NO_INTERRUPT 1u
#define VIRTQ_USED_F_NO_NOTIFY     1u

struct virtio_pci_device {
    uint16_t vendor_id;
        uint16_t device_id;
            uint16_t io_base;
                uint8_t  irq_line;
                    uint8_t  present;
                    };

                    struct virtq_desc {
                        uint64_t address;
                            uint32_t length;
                                uint16_t flags;
                                    uint16_t next;
                                    } __attribute__((packed));

                                    struct virtq_avail {
                                        uint16_t flags;
                                            uint16_t index;
                                                uint16_t ring[];
                                                } __attribute__((packed));

                                                struct virtq_used_elem {
                                                    uint32_t id;
                                                        uint32_t length;
                                                        } __attribute__((packed));

                                                        struct virtq_used {
                                                            uint16_t flags;
                                                                uint16_t index;
                                                                    struct virtq_used_elem ring[];
                                                                    } __attribute__((packed));

                                                                    /* Safe, side-effect-free helpers used by discovery and tests. */
                                                                    static inline int virtio_is_device(uint16_t vendor, uint16_t device)
                                                                    {
                                                                        return vendor == VIRTIO_VENDOR_ID &&
                                                                                   device >= VIRTIO_PCI_LEGACY && device < VIRTIO_PCI_LEGACY + 0x100u;
                                                                                   }

                                                                                   static inline uint16_t virtio_queue_size_bytes(uint16_t entries)
                                                                                   {
                                                                                       return (uint16_t)(sizeof(struct virtq_desc) * entries +
                                                                                                             sizeof(uint16_t) * (3u + entries) +
                                                                                                                                   sizeof(struct virtq_used_elem) * entries +
                                                                                                                                                         sizeof(uint16_t) * 2u);
                                                                                                                                                         }

                                                                                                                                                         #endif /* CORTEXOS_VIRTIO_H */